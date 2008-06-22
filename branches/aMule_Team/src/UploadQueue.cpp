//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <cstring>
#include <cmath>			// Needed for std::exp
#include "types.h"
#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#else
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include "UploadQueue.h"	// Interface declarations
#include "ini2.h"		// Needed for CIni
#include "FriendList.h"		// Needed for CFriendList
#include "ServerList.h"		// Needed for CServerList
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "OScopeCtrl.h"		// Needed for DelayPoints
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "ServerWnd.h"		// Needed for CServerWnd
#include "server.h"			// Needed for CServer
#include "QueueListCtrl.h"	// Needed for CQueueListCtrl
#include "sockets.h"		// Needed for CServerConnect
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "KnownFile.h"		// Needed for CKnownFile
#include "packets.h"		// Needed for Packet
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "opcodes.h"		// Needed for MAX_PURGEQUEUETIME
#include "updownclient.h"	// Needed for CUpDownClient
#include "otherfunctions.h"	// Needed for GetTickCount
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"			// Needed for theApp

#define ID_UQTIMER 59742

//TODO rewrite the whole networkcode, use overlapped sockets

CUploadQueue::CUploadQueue(CPreferences* in_prefs){
	app_prefs = in_prefs;
	//h_timer = SetTimer(0,141,100,TimerProc);
	h_timer=new wxTimer(theApp.amuledlg,ID_UQTIMER);
	if (!h_timer) {
		theApp.amuledlg->AddLogLine(false, CString(_("Fatal Error: Failed to create Timer")));
	}
	msPrevProcess = ::GetTickCount();
	kBpsEst = 2.0;
	kBpsUp = 0.0;
	bannedcount = 0;
	successfullupcount = 0;
	failedupcount = 0;
	totaluploadtime = 0;
	m_nUpDataRateMSOverhead = 0;
	m_nUpDatarateOverhead = 0;
	m_nUpDataOverheadSourceExchange = 0;
	m_nUpDataOverheadFileRequest = 0;
	m_nUpDataOverheadOther = 0;
	m_nUpDataOverheadServer = 0;
	m_nUpDataOverheadSourceExchangePackets = 0;
	m_nUpDataOverheadFileRequestPackets = 0;
	m_nUpDataOverheadOtherPackets = 0;
	m_nUpDataOverheadServerPackets = 0;
	m_nLastStartUpload = 0;
	// Note: wxTimer can be off by more than 10% !!!
	// In addition to the systematic error introduced by wxTimer, we are losing
	// timer cycles due to high CPU load.  I've observed about 0.5% random loss of cycles under
	// low load, and more than 6% lost cycles with heavy download traffic and/or other tasks
	// in the system, such as a video player or a VMware virtual machine.
	// The upload queue process loop has now been rewritten to compensate for timer errors.
	// When adding functionality, assume that the timer is only approximately correct;
	// for measurements, always use the system clock [::GetTickCount()].
	h_timer->Start(100);
	lastupslotHighID = true; // Uninitialized on eMule
}

void CUploadQueue::AddUpNextClient(CUpDownClient* directadd){
	POSITION toadd = 0;
	POSITION toaddlow = 0;
	uint32	bestscore = 0;
	uint32	bestlowscore = 0;
	
	CUpDownClient* newclient;
	// select next client or use given client
	if (!directadd) {
		POSITION pos1, pos2;
		for (pos1 = waitinglist.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
			waitinglist.GetNext(pos1);
			CUpDownClient* cur_client =	waitinglist.GetAt(pos2);
			// clear dead clients
			if ((::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID()) ) {
				RemoveFromWaitingQueue(pos2,true);	
				if (!cur_client->socket) {
					cur_client->Disconnected();
				}
				continue;
			} 
			if (cur_client->IsBanned() || suspended_uploads_list.Find(cur_client->GetUploadFileID())) { // Banned client or suspended upload ?
			        continue;
			} 
			// finished clearing
			uint32 cur_score = cur_client->GetScore(true);
			if ( cur_score > bestscore){
				bestscore = cur_score;
				toadd = pos2;
			} else {
				cur_score = cur_client->GetScore(false);
				if ((cur_score > bestlowscore) && !cur_client->m_bAddNextConnect){
					bestlowscore = cur_score;
					toaddlow = pos2;
				}
			}			
		}

		if (bestlowscore > bestscore){
			newclient = waitinglist.GetAt(toaddlow);
			newclient->m_bAddNextConnect = true;
		}		

		if (!toadd) {
			return;
		}
		newclient = waitinglist.GetAt(toadd);
		lastupslotHighID = true; // VQB LowID alternate		
		RemoveFromWaitingQueue(toadd, true);
		theApp.amuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
	} else {
		//prevent another potential access of a suspended upload
		if (suspended_uploads_list.Find(directadd->GetUploadFileID())) {
			return;
		} else {
			newclient = directadd;
		}
	}
	
	// Thief clients handling [BlackRat]
	if (newclient->thief) {
		// what kind of thief is it ?
		switch (newclient->leechertype){
		/* Add log line according to leecher type */
			case 1 : {
				theApp.amuledlg->AddLogLine(false, CString(_("%s [%s:%i] using %s removed : invalide eMule client")),newclient->m_pszUsername,newclient->m_szFullUserIP,newclient->m_nUserPort,newclient->m_clientVerString.c_str());
				break;
			}
			case 2 : {
				theApp.amuledlg->AddLogLine(false, CString(_("%s [%s:%i] using %s removed : suspicious mod string change")),newclient->m_pszUsername,newclient->m_szFullUserIP,newclient->m_nUserPort,newclient->m_clientVerString.c_str());
				break;
			}
			case 3 : {
				theApp.amuledlg->AddLogLine(false, CString(_("%s [%s:%i] using %s removed : known leecher")),newclient->m_pszUsername,newclient->m_szFullUserIP,newclient->m_nUserPort,newclient->m_clientVerString.c_str());
				break;
			}
			case 4 : {
				theApp.amuledlg->AddLogLine(false, CString(_("%s [%s:%i] using %s removed : suspicious hash change")),newclient->m_pszUsername,newclient->m_szFullUserIP,newclient->m_nUserPort,newclient->m_clientVerString.c_str());
				break;
			}
			case 5 : {
				theApp.amuledlg->AddLogLine(false, CString(_("%s [%s:%i] using %s removed : use your own hash")),newclient->m_pszUsername,newclient->m_szFullUserIP,newclient->m_nUserPort,newclient->m_clientVerString.c_str());
				break;
			}
			default : {
				theApp.amuledlg->AddLogLine(false, CString(_("%s [%s:%i] using %s removed : suspicious name change")),newclient->m_pszUsername,newclient->m_szFullUserIP,newclient->m_nUserPort,newclient->m_clientVerString.c_str());
			}
		}
		// remove client !
		theApp.uploadqueue->RemoveFromUploadQueue(newclient,true);
		return;
	}

	// Anti-leecher mods and irregular clients [BlackRat - LSD]
	if (((strcmp(newclient->m_pszUsername,"celinesexy") == 0) ||
	(strcmp(newclient->m_pszUsername,"Chief") == 0) ||
	(strcmp(newclient->m_pszUsername,"darkmule") == 0) ||
	(strcmp(newclient->m_pszUsername,"dodgethis") == 0) ||
	(strcmp(newclient->m_pszUsername,"edevil") == 0) ||
	(strcmp(newclient->m_pszUsername,"energyfaker") == 0) ||
	(strcmp(newclient->m_pszUsername,"eVortex") == 0) ||
	(strcmp(newclient->m_pszUsername,"|eVorte|X|") == 0) ||
	(strcmp(newclient->m_pszUsername,"$GAM3R$") == 0) ||
	(strcmp(newclient->m_pszUsername,"G@m3r") == 0) ||		 
	(strcmp(newclient->m_pszUsername,"Leecha") == 0) ||
	(strcmp(newclient->m_pszUsername,"Mison") == 0) ||
	(strcmp(newclient->m_pszUsername,"phArAo") == 0) ||
	(strcmp(newclient->m_pszUsername,"RAMMSTEIN") == 0) ||
	(strcmp(newclient->m_pszUsername,"Reverse") == 0) ||
	(strcmp(newclient->m_pszUsername,"[toXic]") == 0) ||
	(strcmp(newclient->m_pszUsername,"$WAREZ$") == 0) ||
	(newclient->m_clientModString.Cmp("aldo") == 0 ) ||
	(newclient->m_clientModString.Cmp("booster") == 0 ) ||
	(newclient->m_clientModString.Cmp("darkmule") == 0 ) ||
	(newclient->m_clientModString.Cmp("d-unit") == 0 ) ||
	(newclient->m_clientModString.Cmp("DM-") == 0 ) ||       
	(newclient->m_clientModString.Cmp("dodgethis") == 0 ) ||
	(newclient->m_clientModString.Cmp("Dragon") == 0 ) ||
	(newclient->m_clientModString.Cmp("egomule") == 0 ) ||
	(newclient->m_clientModString.Cmp("eVortex") == 0 ) ||
	(newclient->m_clientModString.Cmp("father") == 0 ) ||
	(newclient->m_clientModString.Cmp("Freeza") == 0 ) ||
	(newclient->m_clientModString.Cmp("gt mod") == 0 ) ||
	(newclient->m_clientModString.Cmp("imperator") == 0 ) ||
	(newclient->m_clientModString.Cmp("LegoLas") == 0 ) ||
	(newclient->m_clientModString.Cmp("Max") == 0 )  ||
	(newclient->m_clientModString.Cmp("Mison") == 0 ) ||
	(newclient->m_clientModString.Cmp("SpeedLoad") == 0 ) ||
	(newclient->m_clientModString.Cmp("|X|") == 0 ) ||
	((newclient->m_clientModString.IsEmpty() == false) && (newclient->GetClientSoft() != SO_EMULE) && (newclient->GetClientSoft() != SO_LXMULE) && (newclient->GetClientSoft() != SO_AMULE) ) ||
 	((!newclient->GetMuleVersion() && (newclient->GetClientSoft()==SO_EMULE || newclient->GetClientSoft()==SO_OLDEMULE)) && (newclient->GetVersion()==60 || !newclient->GetVersion())) ||
 	(!newclient->ExtProtocolAvailable() && newclient->GetClientSoft()==SO_EMULE && (newclient->GetVersion()==60 || !newclient->GetVersion())) ||
 	((newclient->GetVersion()>589) && (newclient->GetSourceExchangeVersion()>0) && (newclient->GetClientSoft()== SO_EDONKEY))))
	{		
		// thief !
		newclient->thief=true;
		theApp.uploadqueue->RemoveFromUploadQueue(newclient,true);
		theApp.amuledlg->AddLogLine(false, CString(_("%s [%s:%i] using %s removed : leecher, invalid eMule or irregular Donkey")),newclient->GetUserName(),newclient->GetFullIP(),newclient->GetUserPort(),newclient->GetClientVerString().c_str());
		return;
	}	

	if (IsDownloading(newclient)) {
		return;
	}
	// tell the client that we are now ready to upload
	if (!newclient->socket || !newclient->socket->IsConnected()) {
		newclient->SetUploadState(US_CONNECTING);
		// newclient->TryToConnect(true);
		if (!newclient->TryToConnect(true)) {
			return;
		}
	} else {
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		newclient->socket->SendPacket(packet,true);
		newclient->SetUploadState(US_UPLOADING);
	}
	newclient->SetUpStartTime();
	newclient->ResetSessionUp();
	uploadinglist.AddTail(newclient);
	
	// statistic
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)newclient->GetUploadFileID());
	if (reqfile) {
		reqfile->statistic.AddAccepted();
	}
	theApp.amuledlg->transferwnd->uploadlistctrl->AddClient(newclient);

}

void CUploadQueue::Process()
{
	if (AcceptNewClient() && waitinglist.GetCount()) {
		m_nLastStartUpload = ::GetTickCount();
		AddUpNextClient();
	}
	if (!uploadinglist.GetCount()) {
		return;
	}
	int16 clientsrdy = 0;
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0;uploadinglist.GetNext(pos)) {
		CUpDownClient* cur_client = uploadinglist.GetAt(pos);
		if ( (cur_client->socket) && (!cur_client->socket->IsBusy()) && cur_client->HasBlocks()) {
			clientsrdy++;
		}
	}
	if (!clientsrdy) {
		if ((kBpsEst -= 2.0) < 1.0)
			kBpsEst = 1.0;
		clientsrdy++;
	} else {
		if ((kBpsEst += 2.0) > (float)(app_prefs->GetMaxUpload()))
			kBpsEst = (float)(app_prefs->GetMaxUpload());
	}
	float	kBpsSendPerClient = kBpsEst/clientsrdy;
	uint32	bytesSent = 0;
	POSITION pos1,pos2;
	for (pos1 = uploadinglist.GetHeadPosition();( pos2 = pos1 ) != NULL; ) {
		uploadinglist.GetNext(pos1);
		CUpDownClient* cur_client = uploadinglist.GetAt(pos2);
		bytesSent += cur_client->SendBlockData(kBpsSendPerClient);
	}
	
	// smooth current UL rate with a first-order filter
	static uint32	bytesNotCounted;
	uint32	msCur = ::GetTickCount();
	if (msCur==msPrevProcess) {  		// sometimes we get two pulse quickly in a row
		bytesNotCounted += bytesSent;	// avoid divide-by-zero in rate computation then
	} else {
		float	msfDeltaT = (float)(msCur-msPrevProcess);
		float	lambda = std::exp(-msfDeltaT/4000.0);
		kBpsUp = kBpsUp*lambda + (((bytesSent+bytesNotCounted)/1.024)/msfDeltaT)*(1.0-lambda);
		bytesNotCounted = 0;
		msPrevProcess = msCur;
	}
}

bool CUploadQueue::AcceptNewClient()
{
	// check if we can allow a new client to start downloading from us
	if (::GetTickCount() - m_nLastStartUpload < 1000 || uploadinglist.GetCount() >= MAX_UP_CLIENTS_ALLOWED) {
		return false;
	}

	float kBpsUpPerClient = (float)theApp.glob_prefs->GetSlotAllocation();
	if (theApp.glob_prefs->GetMaxUpload() == UNLIMITED) {
		if ((uint32)uploadinglist.GetCount() < ((uint32)(kBpsUp/kBpsUpPerClient)+2)) {
			return true;
		}
	} else {
		uint16 nMaxSlots = 0;
		if (theApp.glob_prefs->GetMaxUpload() >= 10) {
			nMaxSlots = (uint16)floor((float)theApp.glob_prefs->GetMaxUpload() / kBpsUpPerClient + 0.5);
				// floor(x + 0.5) is a way of doing round(x) that works with gcc < 3 ...
			if (nMaxSlots < MIN_UP_CLIENTS_ALLOWED) {
				nMaxSlots=MIN_UP_CLIENTS_ALLOWED;
			}
		} else {
			nMaxSlots = MIN_UP_CLIENTS_ALLOWED;
		}

		if ((uint32)uploadinglist.GetCount() < nMaxSlots) {
			return true;
		}
	}
	return false;
}

CUploadQueue::~CUploadQueue()
{
	delete h_timer;
}

POSITION CUploadQueue::GetWaitingClient(CUpDownClient* client)
{
	return waitinglist.Find(client); 
}

CUpDownClient* CUploadQueue::GetWaitingClientByIP(uint32 dwIP)
{
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
		if (dwIP == waitinglist.GetAt(pos)->GetIP()) {
			return waitinglist.GetAt(pos);
		}
	}
	return 0;
}

POSITION CUploadQueue::GetDownloadingClient(CUpDownClient* client)
{
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0;uploadinglist.GetNext(pos)) {
		if (client == uploadinglist.GetAt(pos)) {
			return pos;
		}
	}
	return 0;
}

void CUploadQueue::UpdateBanCount()
{
	int count=0;
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
		CUpDownClient* cur_client= waitinglist.GetAt(pos);
		if(cur_client->IsBanned()) {
			count++;
		}
	}
	SetBanCount(count);
}

void CUploadQueue::AddClientToQueue(CUpDownClient* client, bool bIgnoreTimelimit)
{

	if (theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() && !theApp.serverconnect->IsLocalServer(client->GetServerIP(),client->GetServerPort()) && client->GetDownloadState() == DS_NONE && !client->IsFriend() && GetWaitingUserCount() > 50) {
		// Well, all that issues finish in the same: don't allow to add to the queue
		return;
	}
	
	if (client->IsBanned()) {
		if (::GetTickCount() - client->GetBanTime() > 18000000) {
			client->UnBan();
		} else {
			return;
		}
	}

	client->AddAskedCount();
	client->SetLastUpRequest();

	if (!bIgnoreTimelimit) {
		client->AddRequestCount(client->GetUploadFileID());
	}

	// check for double
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
		CUpDownClient* cur_client= waitinglist.GetAt(pos);
		if (cur_client == client) { // already on queue
			if (client->m_bAddNextConnect && (uploadinglist.GetCount() < theApp.glob_prefs->GetMaxUpload())){
				if (lastupslotHighID) {
					client->m_bAddNextConnect = false;
					RemoveFromWaitingQueue(client, true);
					AddUpNextClient(client);
					lastupslotHighID = false; // LowID alternate
					return;
				}
			}
			// VQB end
			client->SendRankingInfo();
			theApp.amuledlg->transferwnd->queuelistctrl->RefreshClient(client);
			return;			
		} else if ( client->Compare(cur_client) ) {
			// another client with same ip or hash
			theApp.amuledlg->AddDebugLogLine(false,CString(_("Client '%s' and '%s' have the same userhash or IP - removed '%s'")),client->GetUserName(),cur_client->GetUserName(),cur_client->GetUserName() );
			RemoveFromWaitingQueue(pos,true);	
			if (!cur_client->socket) {
				cur_client->Disconnected();
			}
			return;
		}
	}
	// done

	// Add clients server to list.
	if (theApp.glob_prefs->AddServersFromClient()) {
		in_addr host;
		host.s_addr = client->GetServerIP();
		CServer* srv = new CServer(client->GetServerPort(), inet_ntoa(host));
		srv->SetListName(srv->GetAddress());
		
		if (!theApp.amuledlg->serverwnd->serverlistctrl->AddServer(srv, true)) {
			delete srv;
		}
		/*
		} else {
			theApp.amuledlg->AddLogLine(false, CString(_("Added new server: %s:%d")), srv->GetFullIP(), srv->GetPort());
		}
		*/
	}

	// statistic values
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
	if (reqfile) {
		reqfile->statistic.AddRequest();
	}
	// TODO find better ways to cap the list
	if ((uint32)waitinglist.GetCount() > (theApp.glob_prefs->GetQueueSize()+bannedcount)) {
		return;
	}
	if (client->IsDownloading()) {
		// he's already downloading and wants probably only another file
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		client->socket->SendPacket(packet,true);
		return;
	}
	if (waitinglist.IsEmpty() && AcceptNewClient()) {
		AddUpNextClient(client);
		m_nLastStartUpload = ::GetTickCount();
	} else {
		waitinglist.AddTail(client);
		client->SetUploadState(US_ONUPLOADQUEUE);
		client->SendRankingInfo();
		theApp.amuledlg->transferwnd->queuelistctrl->AddClient(client);
		theApp.amuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
	}
}

bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client, bool updatewindow)
{
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0;uploadinglist.GetNext(pos)) {
		if (client == uploadinglist.GetAt(pos)) {
			if (updatewindow) {
				theApp.amuledlg->transferwnd->uploadlistctrl->RemoveClient(uploadinglist.GetAt(pos));
			}
			uploadinglist.RemoveAt(pos);
			if( client->GetTransferedUp() ) {
				successfullupcount++;
				totaluploadtime += client->GetUpStartTimeDelay()/1000;
			} else {
				failedupcount++;
			}
			client->SetUploadState(US_NONE);
			client->ClearUploadBlockRequests();
			return true;
		}
	}
	return false;
}

uint32 CUploadQueue::GetAverageUpTime()
{
	if( successfullupcount ) {
		return totaluploadtime/successfullupcount;
	}
	return 0;
}

bool CUploadQueue::CheckForTimeOver(CUpDownClient* client)
{
	if (theApp.glob_prefs->TransferFullChunks()) {
		if( client->GetUpStartTimeDelay() > 3600000 ) { // Try to keep the clients from downloading for ever.
			return true;
		}
		// For some reason, some clients can continue to download after a chunk size.
		// Are they redownloading the same chunk over and over????
		if( client->GetSessionUp() > 10485760 ) {
			return true;
		}
	} else {
		for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
			if (client->GetScore(true,true) < waitinglist.GetAt(pos)->GetScore(true,false)) {
				return true;
			}
		}
	}
	return false;
}

void CUploadQueue::DeleteAll()
{
	waitinglist.RemoveAll();
	uploadinglist.RemoveAll();
}

uint16 CUploadQueue::GetWaitingPosition(CUpDownClient* client)
{
	if (!IsOnUploadQueue(client)) {
		return 0;
	}
	uint16 rank = 1;
	uint32 myscore = client->GetScore(false);
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)) {
		if (waitinglist.GetAt(pos)->GetScore(false) > myscore) {
			rank++;
		}
	}
	return rank;
}

void CUploadQueue::CompUpDatarateOverhead()
{
	this->m_AvarageUDRO_list.AddTail(m_nUpDataRateMSOverhead);
	if (m_AvarageUDRO_list.GetCount() > 150) {
		m_AvarageUDRO_list.RemoveAt(m_AvarageUDRO_list.GetHeadPosition());
	}
	m_nUpDatarateOverhead = 0;
	m_nUpDataRateMSOverhead = 0;
	for (POSITION pos = m_AvarageUDRO_list.GetHeadPosition();pos != 0;m_AvarageUDRO_list.GetNext(pos)) {
		m_nUpDatarateOverhead += m_AvarageUDRO_list.GetAt(pos);
	}
	if(m_AvarageUDRO_list.GetCount() > 10) {
		m_nUpDatarateOverhead = 10*m_nUpDatarateOverhead/m_AvarageUDRO_list.GetCount();
	} else {
		m_nUpDatarateOverhead = 0;
	}
	return;
}

/*
 * This function removes a file indicated by filehash from suspended_uploads_list.
 */
void CUploadQueue::ResumeUpload( const CFileHash& filehash )
{
	//Find the position of the filehash in the list and remove it.
	suspended_uploads_list.RemoveAt( suspended_uploads_list.Find(filehash) );
	printf("%s: Resuming uploads of file.\n", EncodeBase16(filehash, 16).GetBuffer() );
	
}

/*
 * This function adds a file indicated by filehash to suspended_uploads_list
 */
void CUploadQueue::SuspendUpload( const CFileHash& filehash )
{
	//Append the filehash to the list.
	suspended_uploads_list.Append(filehash);
	CString base16hash = EncodeBase16(filehash, 16);
	
	printf("%s: Suspending uploads of file.\n", base16hash.GetBuffer());
	
	POSITION pos = uploadinglist.GetHeadPosition();
	while(pos) { //while we have a valid position
		CUpDownClient *potential = uploadinglist.GetNext(pos);
		//check if the client is uploading the file we need to suspend
		if(memcmp(potential->GetUploadFileID(), filehash, 16) == 0) {
			//remove the unlucky client from the upload queue and add to the waiting queue
			RemoveFromUploadQueue(potential, 1);
			printf("%s: Removed user '%s'\n", base16hash.GetBuffer() ,potential->GetUserName() );
			//this code to add to the waitinglist was copied from the end of AddClientToQueue()
			//the function itself is not used as it could prevent the requeuing of the client
			waitinglist.AddTail(potential);
			potential->SetUploadState(US_ONUPLOADQUEUE);
			potential->SendRankingInfo();
			theApp.amuledlg->transferwnd->queuelistctrl->AddClient(potential);
			theApp.amuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
			printf("%s: ReQueued user '%s'\n", base16hash.GetBuffer(), potential->GetUserName() );
		}
	}
}

bool CUploadQueue::RemoveFromWaitingQueue(CUpDownClient* client, bool updatewindow)
{
	POSITION pos = waitinglist.Find(client);
	if (pos) {
		RemoveFromWaitingQueue(pos,updatewindow);
		if (updatewindow) {
			theApp.amuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
		}
		return true;
	} else {
		return false;
	}
}

void CUploadQueue::RemoveFromWaitingQueue(POSITION pos, bool updatewindow)
{
	CUpDownClient* todelete = waitinglist.GetAt(pos);
	waitinglist.RemoveAt(pos);
	if( todelete->IsBanned() ) {
		todelete->UnBan();
	}
	//if (updatewindow)
	theApp.amuledlg->transferwnd->queuelistctrl->RemoveClient(todelete);
	todelete->SetUploadState(US_NONE);
}

CUpDownClient* CUploadQueue::GetNextClient(CUpDownClient* lastclient)
{
	if (waitinglist.IsEmpty()) {
		return 0;
	}
	if (!lastclient) {
		return waitinglist.GetHead();
	}
	POSITION pos = waitinglist.Find(lastclient);
	if (!pos) {
		return waitinglist.GetHead();
	}
	waitinglist.GetNext(pos);
	if (!pos) {
		return NULL;
	} else {
		return waitinglist.GetAt(pos);
	}
}

void CUploadQueue::FindSourcesForFileById(CTypedPtrList<CPtrList, CUpDownClient*>* srclist, uchar* filehash)
{
	POSITION pos;
	
	pos = uploadinglist.GetHeadPosition();
	while(pos) {
		CUpDownClient *potential = uploadinglist.GetNext(pos);
		if(memcmp(potential->GetUploadFileID(), filehash, 16) == 0) {
			srclist->AddTail(potential);
		}
	}

	pos = waitinglist.GetHeadPosition();
	while(pos) {
		CUpDownClient *potential = waitinglist.GetNext(pos);
		if(memcmp(potential->GetUploadFileID(), filehash, 16) == 0) {
			srclist->AddTail(potential);
		}
	}
}

void TimerProc()
{	
	static uint32	msPrev1, msPrev5, msPrevGraph, msPrevStats, msPrevSave;
	static uint32	msPrevHist;
	uint32 			msCur = theApp.GetUptimeMsecs();

	// can this actually happen under wxwin ?
	if (!theApp.amuledlg->IsRunning() || !theApp.IsReady) {
		return;
	}
	theApp.uploadqueue->Process();
	theApp.downloadqueue->Process();
	//theApp.clientcredits->Process();
	theApp.uploadqueue->CompUpDatarateOverhead();
	theApp.downloadqueue->CompDownDatarateOverhead();
#warning BIG WARNING: FIX STATS ON MAC!
#warning Can it be related to the fact we have two timers now?
#warning I guess so - there MUST be a reason Tiku only added one.
	if (msCur-msPrevHist > 1000) {
		// unlike the other loop counters in this function this one will sometimes 
		// produce two calls in quick succession (if there was a gap of more than one 
		// second between calls to TimerProc) - this is intentional!  This way the 
		// history list keeps an average of one node per second and gets thinned out 
		// correctly as time progresses.
		msPrevHist += 1000;
		#ifndef __WXMAC__
		theApp.amuledlg->statisticswnd->RecordHistory();
		#endif
	}

	if (msCur-msPrev1 > 950) {  // approximately every second
		msPrev1 = msCur;
		theApp.clientcredits->Process();
		theApp.serverlist->Process();
		theApp.friendlist->Process();
		if( theApp.serverconnect->IsConnecting() && !theApp.serverconnect->IsSingleConnect() ) {
			theApp.serverconnect->TryAnotherConnectionrequest();
		}
		theApp.amuledlg->statisticswnd->UpdateConnectionsStatus();
		if (theApp.serverconnect->IsConnecting()) {
			theApp.serverconnect->CheckForTimeout();
		}
	}

	bool bStatsVisible = (!theApp.amuledlg->IsIconized() && theApp.amuledlg->StatisticsWindowActive());
	int msGraphUpdate=theApp.glob_prefs->GetTrafficOMeterInterval()*1000;
	if ((msGraphUpdate > 0)  && ((msCur / msGraphUpdate) > (msPrevGraph / msGraphUpdate))) {
		// trying to get the graph shifts evenly spaced after a change in the update period
		msPrevGraph = msCur;
		#ifndef __WXMAC__
		theApp.amuledlg->statisticswnd->UpdateStatGraphs(bStatsVisible);
		#endif
	}

	int sStatsUpdate = theApp.glob_prefs->GetStatsInterval();
	if ((sStatsUpdate > 0) && ((int)(msCur - msPrevStats) > sStatsUpdate*1000)) {
		if (bStatsVisible) {
			msPrevStats = msCur;
			theApp.amuledlg->ShowStatistics();
		}
	}

	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		theApp.listensocket->Process();
		theApp.OnlineSig(); // Added By Bouc7
		theApp.amuledlg->ShowTransferRate();
	}
	if (msCur-msPrevSave >= 60000) {
		msPrevSave = msCur;
		CString buffer;
		char* fullpath = new char[strlen(theApp.glob_prefs->GetAppDir())+16];
		sprintf(fullpath,"%spreferences.ini",theApp.glob_prefs->GetAppDir());
		wxString fp(fullpath), bf("aMule");
		CIni ini(fp, bf);
		delete[] fullpath;
		buffer.Format("%llu",theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded());
		ini.WriteString("TotalDownloadedBytes",buffer ,"Statistics");

		buffer.Format("%llu",theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded());
		ini.WriteString("TotalUploadedBytes",buffer ,"Statistics");
	}
	// Recomended by lugdunummaster himself - from emule 0.30c
	theApp.serverconnect->KeepConnectionAlive();

	theApp.amuledlg->UpdateLists(msCur);

}