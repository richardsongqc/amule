//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ServerSocket.h"
#endif

#include <ctime>
#include <cerrno>
#include "Types.h"
#include <wx/tokenzr.h>

#include "ServerSocket.h"	// Interface declarations
#include "Packet.h"		// Needed for CPacket
#include "updownclient.h"	// Needed for CUpDownClient
#include "ClientList.h"		// Needed for CClientList
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "PartFile.h"		// Needed for CPartFile
#include "MemFile.h"		// Needed for CMemFile
#include "SearchList.h"		// Needed for CSearchList
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "OPCodes.h"		// Needed for OP_SERVERMESSAGE
#include "OtherFunctions.h"	// Needed for GetTickCount
#include "ServerConnect.h"		// Needed for CS_WAITFORLOGIN
#include "ServerList.h"		// Needed for CServerList
#include "Server.h"		// Needed for CServer
#include "amule.h"		// Needed for theApp
#include "amuleIPV4Address.h"	// Needed for amuleIPV4Address
#include "Statistics.h"		// Needed for CStatistics
#include "Logger.h"
#include "Format.h"

#ifndef AMULE_DAEMON
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#endif


//------------------------------------------------------------------------------
// CServerSocketHandler
//------------------------------------------------------------------------------

#ifndef AMULE_DAEMON
BEGIN_EVENT_TABLE(CServerSocketHandler, wxEvtHandler)
	EVT_SOCKET(SERVERSOCKET_HANDLER, CServerSocketHandler::ServerSocketHandler)
END_EVENT_TABLE()

CServerSocketHandler::CServerSocketHandler(CServerSocket *)
{
}

void CServerSocketHandler::ServerSocketHandler(wxSocketEvent& event)
{
	CServerSocket *socket = dynamic_cast<CServerSocket *>(event.GetSocket());
	wxASSERT(socket);
	if (!socket) {
		return;
	}

	if (socket->OnDestroy()) {
		return;
	}
	
	switch(event.GetSocketEvent()) {
		case wxSOCKET_CONNECTION:
			socket->OnConnect(wxSOCKET_NOERROR);
			break;
		case wxSOCKET_LOST:
			socket->OnError(socket->LastError());
			break;
		case wxSOCKET_INPUT:
			socket->OnReceive(wxSOCKET_NOERROR);
			break;
		case wxSOCKET_OUTPUT:
			socket->OnSend(wxSOCKET_NOERROR);
			break;
		default:
			wxASSERT(0);
			break;
	}
	
	
}

//
// There can be only one. :)
//
static CServerSocketHandler g_serverSocketHandler;

#else
CServerSocketHandler::CServerSocketHandler(CServerSocket *socket)
:
wxThread(wxTHREAD_JOINABLE)
{
	m_socket = socket;
	if ( Create() != wxTHREAD_NO_ERROR ) {
		AddLogLineM(true,_("CServerSocketHandler: can not create my thread"));
	}
}

void *CServerSocketHandler::Entry()
{
	while ( !TestDestroy() ) {
		if ( m_socket->WaitOnConnect(1,0) ) {
			break;
		}
	}
	if ( !m_socket->wxSocketClient::IsConnected() || TestDestroy()) {
		printf("CServerSocket: connection refused or timed out\n");
		return 0;
	}
	m_socket->OnConnect(wxSOCKET_NOERROR);
	m_socket->OnSend(wxSOCKET_NOERROR);
	while ( !TestDestroy() ) {
		if ( m_socket->WaitForLost(0, 0) ) {
			m_socket->OnError(m_socket->LastError());
			printf("CServerSocket: connection closed\n");
			return 0;
		}
		// lfroen: setting timeout to give app a chance gracefully destroy
		// thread before deleting object
		if ( m_socket->WaitForRead(1, 0) ) {
			m_socket->OnReceive(wxSOCKET_NOERROR);
		}
	}
	printf("CServerSocket: terminated\n");
	return 0;

}
#endif

//------------------------------------------------------------------------------
// CServerSocket
//------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(CServerSocket,CEMSocket)

CServerSocket::CServerSocket(CServerConnect* in_serverconnect, const CProxyData *ProxyData)
:
CEMSocket(ProxyData)
{
	serverconnect = in_serverconnect;
	connectionstate = 0;
	cur_server = 0;
	info.Clear();
	m_bIsDeleting = false;
#ifndef AMULE_DAEMON	
	my_handler = &g_serverSocketHandler;
	SetEventHandler(*my_handler, SERVERSOCKET_HANDLER);
	SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_OUTPUT_FLAG |
		wxSOCKET_LOST_FLAG);
	Notify(true);
#else
	my_handler = new CServerSocketHandler(this);
#endif
	m_dwLastTransmission = 0;	
	m_IsSolving = false;
}

CServerSocket::~CServerSocket()
{
	// remove event handler...
	SetNotify(0);
	Notify(FALSE);
	
	if (cur_server) {
		delete cur_server;
	}
	cur_server = NULL;
#ifdef AMULE_DAEMON
	printf("CServerSocket: destroying socket %p\n", (void*)this);
	my_handler->Delete();
#endif
}


void CServerSocket::OnConnect(wxSocketError nErrorCode)
{
	CALL_APP_DATA_LOCK;
	switch (nErrorCode) {
		case wxSOCKET_NOERROR:
			if (cur_server->HasDynIP()) {
				amuleIPV4Address tmpaddr;
				GetPeer(tmpaddr);
				uint32 server_ip = StringIPtoUint32(tmpaddr.IPAddress());
				cur_server->SetID(server_ip);
				// GetServerByAddress may return NULL, so we must test!
				// This was the reason why amule would crash when trying to
				// connect in wxWidgets 2.5.2
				CServer *pServer = theApp.serverlist->GetServerByAddress(
					cur_server->GetAddress(), cur_server->GetPort());
				if (pServer) {
					pServer->SetID(server_ip);
				} else {
					AddLogLineM(false,_("theApp.serverlist->GetServerByAddress() returned NULL"));
					return;
				}
			}
			SetConnectionState(CS_WAITFORLOGIN);
			break;

		case wxSOCKET_INVADDR:
		case wxSOCKET_NOHOST:
		case wxSOCKET_INVPORT:
		case wxSOCKET_TIMEDOUT:
			m_bIsDeleting = true;
			SetConnectionState(CS_SERVERDEAD);
			serverconnect->DestroySocket(this);
			return;
		
		case wxSOCKET_IOERR:
		case wxSOCKET_MEMERR:
		case wxSOCKET_INVOP:
		default:
			m_bIsDeleting = true;
			SetConnectionState(CS_FATALERROR);
			serverconnect->DestroySocket(this);
			return;
		
	}
	
}

void CServerSocket::OnReceive(wxSocketError nErrorCode)
{
	if (connectionstate != CS_CONNECTED && !serverconnect->IsConnecting()) {
		serverconnect->DestroySocket(this);
		return;
	}
	CEMSocket::OnReceive((int)nErrorCode);
	m_dwLastTransmission = GetTickCount();
}

bool CServerSocket::ProcessPacket(const char* packet, uint32 size, int8 opcode)
{
	try {
		AddDebugLogLineM( false, logServer, wxT("Processing Server Packet: ") );
		
		CServer* update;
		switch(opcode) {
			case OP_SERVERMESSAGE: {
				/* Kry import of lugdunum 16.40 new features */
				AddDebugLogLineM( false, logServer, wxT("Server: OP_SERVERMESSAGE") );
				
				theApp.statistics->AddDownDataOverheadServer(size);
				char* buffer = new char[size-1];
				memcpy(buffer,&packet[2],size-2);
				buffer[size-2] = 0;

				wxString strMessages(char2unicode(buffer));

				delete[] buffer;
				
				// 16.40 servers do not send separate OP_SERVERMESSAGE packets for each line;
				// instead of this they are sending all text lines with one OP_SERVERMESSAGE packet.

				wxStringTokenizer token(strMessages,wxT("\r\n"),wxTOKEN_DEFAULT );

				while (token.HasMoreTokens()) {
					wxString message = token.GetNextToken();

					bool bOutputMessage = true;
					if (message.StartsWith(wxT("server version"))) {
						wxString strVer = message.Mid(15,64); // truncate string to avoid misuse by servers in showing ads
						strVer.Trim();
						CServer* eserver = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						if (eserver) {
							eserver->SetVersion(strVer);
							Notify_ServerRefresh(eserver);
						}
					} else if (message.StartsWith(wxT("ERROR"))) {
						CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						wxString servername;
						if (pServer) {
							servername	= pServer->GetListName();	
						} else {	
							servername = _("Server");
						}
						AddLogLineM(false, CFormat( _("Error: %s (%s) - %s") )
							% servername
							% Uint32_16toStringIP_Port(cur_server->GetIP(), cur_server->GetPort())
							% message.Mid(5,message.Len()).Trim(wxT(" :")));
						bOutputMessage = false;

					} else if (message.StartsWith(wxT("WARNING"))) {

						CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						wxString servername;
						if (pServer) {
							servername	= pServer->GetListName();	
						} else {	
							servername = _("Server");
						}
						AddLogLineM(false, CFormat( _("Warning: %s (%s) - %s") )
							% servername
							% Uint32_16toStringIP_Port(cur_server->GetIP(), cur_server->GetPort())
							% message.Mid(5,message.Len()).Trim(wxT(" :")));

						bOutputMessage = false;
					}

					if (message.Find(wxT("[emDynIP: ")) != (-1) && message.Find(wxT("]")) != (-1) && message.Find(wxT("[emDynIP: ")) < message.Find(wxT("]"))){
						wxString dynip = message.Mid(message.Find(wxT("[emDynIP: "))+10,message.Find(wxT("]")) - (message.Find(wxT("[emDynIP: "))+10));
						dynip.Trim(wxT(" "));
						if ( dynip.Length() && dynip.Length() < 51){
							CServer* eserver = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
							if (eserver){
								eserver->SetDynIP(dynip);
								cur_server->SetDynIP(dynip);
								Notify_ServerRefresh(eserver);	
							}
						}
					}

					if (bOutputMessage) {
						theApp.AddServerMessageLine(message);
					}
				}
				break;
			}
			case OP_IDCHANGE:{
				AddDebugLogLineM(false,logServer,wxT("Server: OP_IDCHANGE"));
				
				theApp.statistics->AddDownDataOverheadServer(size);
				
				if (size < 4 /* uint32 (ID)*/) {
					throw wxString(wxT("Corrupt or invalid loginanswer from server received"));
				}

				CSafeMemFile data((byte*)packet, size);			
				
				uint32 new_id = data.ReadUInt32();

				/* Add more from 0.30c (Creteil) BEGIN */
				// save TCP flags in 'cur_server'
				wxASSERT(cur_server);
				uint32 ConnPort = 0;
				if (cur_server) {
					uint32 rport = cur_server->GetConnPort();					
					if (size >= 4+4 /* uint32 (ID) + uint32 (TCP flags)*/) {			
						cur_server->SetTCPFlags(data.ReadUInt32());
						if (size >= 4+4+4 /* uint32 (ID) + uint32 (TCP flags) + uint32 (aux port) */) {
							// aux port login : we should use the 'standard' port of this server to advertize to other clients
							ConnPort = data.ReadUInt32();
							cur_server->SetPort(ConnPort);
							if (cur_server->GetAuxPortsList().IsEmpty()) {
								cur_server->SetAuxPortsList(wxString::Format(wxT("%u"), rport));
//							} else {
//								cur_server->SetAuxPortsList(wxString::Format(wxT("%u"), rport) + wxT(",") + cur_server->GetAuxPortsList());
							}
						}
					} else {
						cur_server->SetTCPFlags(0);
					}
					// copy TCP flags into the server in the server list
					CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), rport);
					if (pServer) {
						pServer->SetTCPFlags(cur_server->GetTCPFlags());
						if (ConnPort) {
							pServer->SetPort(ConnPort);
							if (pServer->GetAuxPortsList().IsEmpty()) {
								pServer->SetAuxPortsList(wxString::Format(wxT("%u"), pServer->GetConnPort()));
//							} else {
//								pServer->SetAuxPortsList(wxString::Format(wxT("%u"), pServer->GetConnPort()) + wxT(",") + pServer->GetAuxPortsList());
							}
							Notify_ServerRefresh(pServer);
							Notify_ServerUpdateMyInfo();
						}
					}
				}
				/* Add more from 0.30c (Creteil) END */
				if (new_id == 0) {
					uint8 state = thePrefs::GetSmartIdState();
					if ( state > 0 ) {
						state++;
						if(state > 3) {
							thePrefs::SetSmartIdState(0);
						} else {
							thePrefs::SetSmartIdState(state);
						}
					}
					break;
				}
				if(thePrefs::GetSmartIdCheck()) {
					if (!IsLowID(new_id)) {
						thePrefs::SetSmartIdState(1);
					} else {
						uint8 state = thePrefs::GetSmartIdState();
						if ( state > 0 ) {
							state++;
							if(state > 3) {
								thePrefs::SetSmartIdState(0);
							} else {
								thePrefs::SetSmartIdState(state);
							}
							break;
						}
					}
				}
				// we need to know our client when sending our shared files (done indirectly on SetConnectionState)
				serverconnect->clientid = new_id;

				if (connectionstate != CS_CONNECTED) {
					AddDebugLogLineM(true,logServer,wxT("Connected"));
					
					SetConnectionState(CS_CONNECTED);
					theApp.OnlineSig();       // Added By Bouc7
				}
				serverconnect->SetClientID(new_id);
				AddLogLineM(false, wxString::Format(_("New clientid is %u"),new_id));
								
				theApp.downloadqueue->ResetLocalServerRequests();
				break;
			}
			case OP_SEARCHRESULT: {
				AddDebugLogLineM(false,logServer,wxT("Server: OP_SEARCHRESULT"));
				
				theApp.statistics->AddDownDataOverheadServer(size);
				CServer* cur_srv = (serverconnect) ? 
					serverconnect->GetCurrentServer() : NULL;
				theApp.searchlist->ProcessSearchanswer(
					packet,
					size, 
					true /*(cur_srv && cur_srv->GetUnicodeSupport())*/,
					cur_srv ? cur_srv->GetIP() : 0,
					cur_srv ? cur_srv->GetPort() : 0);
				theApp.searchlist->LocalSearchEnd();
				break;
			}
			case OP_FOUNDSOURCES: {
				AddDebugLogLineM(false,logServer,wxString::Format(wxT("ServerMsg - OP_FoundSources; sources = %u"), (uint32)(byte)packet[16]));
				theApp.statistics->AddDownDataOverheadServer(size);
				CSafeMemFile sources((byte*)packet,size);
				uint8 fileid[16];
				sources.ReadHash16(fileid);
				if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid)) {
					file->AddSources(sources,cur_server->GetIP(), cur_server->GetPort());
				}
				break;
			}
			case OP_SERVERSTATUS: {
				AddDebugLogLineM(false,logServer,wxT("Server: OP_SERVERSTATUS"));
				// FIXME some statuspackets have a different size -> why? structur?
				if (size < 8) {
					throw wxString(wxT("Invalid server status packet"));
					break;
				}
				update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort());
				if (update) {
					CSafeMemFile data((byte*)packet, size);
					update->SetUserCount(data.ReadUInt32());
					update->SetFileCount(data.ReadUInt32());
					Notify_ServerRefresh( update );
					Notify_ShowUserCount(update);
				}
				break;
			}
			// Cleaned.
			case OP_SERVERIDENT: {
				AddDebugLogLineM(false,logServer,wxT("Server: OP_SERVERIDENT"));

				theApp.statistics->AddDownDataOverheadServer(size);
				if (size<38) {
					AddLogLineM(false, _("Unknown server info received! - too short"));
					// throw wxString(wxT("Unknown server info received!"));
					break;
				}
				CServer* update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
				if (update) {
					CSafeMemFile data((byte*)packet,size);
					uint8 aucHash[16];
					data.ReadHash16(aucHash);				
					if (RawPeekUInt32(aucHash) == 0x2A2A2A2A){ // No endian problem here
						const wxString& rstrVersion = update->GetVersion();
						if (!rstrVersion.IsEmpty()) {
							update->SetVersion(wxT("eFarm ") + rstrVersion);
						} else {
							update->SetVersion(wxT("eFarm"));
						}
					}
					// Unused
					/*uint32 nServerIP = */data.ReadUInt32();
					/*uint16 nServerPort = */data.ReadUInt16();
				
					uint32 nTags = data.ReadUInt32();					
					for (uint32 i = 0; i < nTags; i++){
						CTag tag(data, update->GetUnicodeSupport());
						if (tag.GetNameID() == ST_SERVERNAME){
							if (tag.IsStr()){ //String
								update->SetListName(tag.GetStr());
							}
						}
						else if (tag.GetNameID() == ST_DESCRIPTION){
							if (tag.IsStr()){
								update->SetDescription(tag.GetStr());
							}
						} // No more known tags from server
					}				
								
					Notify_ShowConnState(true,update->GetListName());
					Notify_ServerRefresh(update);
				}
				break;
			}
			// tecxx 1609 2002 - add server's serverlist to own serverlist
			case OP_SERVERLIST: {
				AddDebugLogLineM(false,logServer,wxT("Server: OP_SERVERLIST"));
				
				CSafeMemFile* servers = new CSafeMemFile((byte*)packet,size);
				uint8 count = servers->ReadUInt8();
				if (((int32)(count*6 + 1) > size)) {
					count = 0;
				}
				int addcount = 0;
				while(count) {
					uint32 ip	= servers->ReadUInt32();
					uint16 port = servers->ReadUInt16();
					CServer* srv = new CServer(
								port ,				// Port
								Uint32toStringIP(ip)); 	// Ip
					srv->SetListName(srv->GetFullIP());
					if (!theApp.AddServer(srv)) {
						delete srv;
					} else {
						addcount++;
					}
					count--;
				}
				delete servers;
				if (addcount) {
					AddLogLineM(false, wxString::Format(_("Received %d new servers"), addcount));
				}
				theApp.serverlist->SaveServerMet();
				AddLogLineM(false, _("Saving of server-list completed."));
				break;
			}
			case OP_CALLBACKREQUESTED: {
				AddDebugLogLineM(false,logServer,wxT("Server: OP_CALLBACKREQUESTED"));
				
				theApp.statistics->AddDownDataOverheadServer(size);
				if (size == 6) {
					CSafeMemFile data((byte*)packet,size);
					uint32 dwIP = data.ReadUInt32();
					uint16 nPort = data.ReadUInt16();
					CUpDownClient* client = theApp.clientlist->FindClientByIP(dwIP,nPort);
					if (client) {
						client->TryToConnect();
					} else {
						client = new CUpDownClient(nPort,dwIP,0,0,0, true, true);
						theApp.clientlist->AddClient(client);
						client->TryToConnect();
					}
				}
				break;
			}
			case OP_CALLBACK_FAIL: {
				AddDebugLogLineM(false,logServer,wxT("Server: OP_CALLBACK_FAIL"));
				break;
			}
			case OP_REJECT: {
				AddDebugLogLineM(false,logServer,wxT("Server: OP_REJECT"));
				AddLogLineM(false, _("Server rejected last command"));
				break;
			}
			default:
				AddDebugLogLineM( false, logPacketErrors, 
					wxString::Format( wxT("Unknown server packet with OPcode %x"), opcode )
				);
		}
		return true;
	} catch (const CInvalidPacket& e) {
		AddLogLineM(false,CFormat( _("Bogus packet received from server: %s") ) % e.what());
	} catch (const wxString& error) {
		AddLogLineM(false,CFormat( _("Unhandled error while processing packet from server: %s") ) % error);
	} catch (...) {
		AddLogLineM(false, _("Unknown exception while processing packet from server!"));
	}

	// Don't disconnect because of wrong sources.
	if (opcode==OP_SEARCHRESULT || opcode==OP_FOUNDSOURCES) {
		return true;
	}				

	SetConnectionState(CS_DISCONNECTED);
	return false;	
}

void CServerSocket::ConnectToServer(CServer* server)
{
	AddDebugLogLineM(true,logServer,wxT("Trying to connect"));
	
	cur_server = new CServer(server);
	AddLogLineM(false, CFormat( _("Connecting to %s (%s - %s:%i)") )
		% cur_server->GetListName()
		% server->GetAddress()
		% cur_server->GetFullIP()
		% cur_server->GetConnPort() );
	SetConnectionState(CS_CONNECTING);
	
	info = cur_server->GetListName();		

	// This must be used if we want to reverse-check the addr of the server
	if (cur_server->HasDynIP() || !cur_server->GetIP()) {
		m_IsSolving = true;
		// Send it to solving thread.
		CAsyncDNS* dns = new CAsyncDNS(server->GetAddress(), DNS_SERVER_CONNECT, this);
	
		if ( dns->Create() == wxTHREAD_NO_ERROR ) {
			if ( dns->Run() != wxTHREAD_NO_ERROR ) {
				dns->Delete();
				AddLogLineM(false, CFormat( _("Cannot create DNS solving thread for connecting to %s") ) % cur_server->GetAddress());
			}
		} else {
			dns->Delete();
			AddLogLineM(false, CFormat( _("Cannot create DNS solving thread for connecting to %s") ) % cur_server->GetAddress());
		}
	} else {
		// Nothing to solve, we already have the IP
		OnHostnameResolved(cur_server->GetIP());
	}	

}

void CServerSocket::OnError(wxSocketError nErrorCode)
{
	AddDebugLogLineM(false, logServer, wxT("Error in serversocket: ") + cur_server->GetListName() + wxT("(") + cur_server->GetFullIP() + wxString::Format(wxT(":%i): %u"),cur_server->GetPort(), (int)nErrorCode));
	SetConnectionState(CS_DISCONNECTED);
}

bool CServerSocket::PacketReceived(CPacket* packet)
{
	CALL_APP_DATA_LOCK;
	AddDebugLogLineM(false,logServer,wxT("Server: Packet Received: "));
	
	try {
		if (packet->GetProtocol() == OP_PACKEDPROT) {
			if (!packet->UnPackPacket(250000)){
				AddDebugLogLineM(false, logZLib, wxString::Format(wxT("Failed to decompress server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0));
				theApp.statistics->AddDownDataOverheadServer(packet->GetPacketSize());
				return true;
			}
			packet->SetProtocol(OP_EDONKEYPROT);
		}
		if (packet->GetProtocol() == OP_EDONKEYPROT) {
			ProcessPacket(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
		} else {
			AddDebugLogLineM(false, logServer, wxString::Format(wxT("Received server TCP packet with unknown protocol: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0));
			theApp.statistics->AddDownDataOverheadServer(packet->GetPacketSize());
		}
	} catch(...) {
		AddDebugLogLineM(false, logServer, wxString::Format(wxT("Error: Unhandled exception while processing server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0));
		wxASSERT(0);
		return false;		
	}
	return true;
}

void CServerSocket::OnClose(wxSocketError WXUNUSED(nErrorCode))
{
	CEMSocket::OnClose(0);
	
	switch (connectionstate) {
		case CS_WAITFORLOGIN:	SetConnectionState(CS_SERVERFULL);		break;
		case CS_DISCONNECTED:	SetConnectionState(CS_DISCONNECTED);	break;
		default:				SetConnectionState(CS_NOTCONNECTED);	
	}
	
	serverconnect->DestroySocket(this);
}

void CServerSocket::SetConnectionState(sint8 newstate)
{
	connectionstate = newstate;
	if (newstate < CS_CONNECTING) {
		serverconnect->ConnectionFailed(this);
	} else if (newstate == CS_CONNECTED || newstate == CS_WAITFORLOGIN) {
		if (serverconnect) {
			serverconnect->ConnectionEstablished(this);
		}
	}
}


void CServerSocket::SendPacket(CPacket* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize)
{
	m_dwLastTransmission = GetTickCount();
	CEMSocket::SendPacket(packet, delpacket, controlpacket, actualPayloadSize);
}


void CServerSocket::OnHostnameResolved(uint32 ip) {
	
	m_IsSolving = false;
	if (ip) {
		amuleIPV4Address addr;
		addr.Hostname(ip);
		addr.Service(cur_server->GetConnPort());
		AddDebugLogLineM(false, logServer, wxT("Server ") + cur_server->GetAddress() + wxT("(") + Uint32toStringIP(ip) + wxT(")") + wxString::Format(wxT(" Port %i"), cur_server->GetConnPort()));
		Connect(addr, false);
	} else {
		AddLogLineM(true, CFormat( _("Could not solve dns for server %s: Unable to connect!") )
			% cur_server->GetAddress() );
		OnConnect(wxSOCKET_NOHOST);
	}
	
}

#ifdef AMULE_DAEMON
bool CServerSocket::Connect(wxIPV4address &addr, bool wait)
{
	bool res = CEMSocket::Connect(addr, wait);
	my_handler->Run();
	return res;
}
#endif
