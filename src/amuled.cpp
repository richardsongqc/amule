//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <unistd.h>           // Needed for close(2) and sleep(3)
#include <wx/defs.h>

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#ifdef __BSD__
     	  #include <sys/types.h>
	#endif /* __BSD__ */
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#ifdef __WXGTK__

	#ifdef __BSD__
     	#include <sys/param.h>
       	#include <sys/mount.h>
	#else 
		#include <execinfo.h>
		#include <mntent.h>
	#endif /* __BSD__ */

	
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT, HAVE_SYS_RESOURCE_H,
#endif				//   LOCALEDIR, PACKAGE, PACKAGE_STRING and VERSION

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>                // Needed for wxFileName::GetPathSeparator()
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/config.h>
#include <wx/socket.h>			// Needed for wxSocket
#include <wx/utils.h>
#include <wx/ipc.h>
#include <wx/intl.h>			// Needed for i18n
#include <wx/mimetype.h>		// For launching default browser
#include <wx/textfile.h>		// Needed for wxTextFile
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer
#include <wx/url.h>

#include "amule.h"				// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "server.h"				// Needed for GetListName
#include "CFile.h"				// Needed for CFile
#include "otherfunctions.h"		// Needed for GetTickCount
#include "IPFilter.h"			// Needed for CIPFilter
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ClientCredits.h"		// Needed for CClientCreditsList
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "sockets.h"			// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerSocket.h"	// Needed for CServerSocket
#include "UDPSocket.h"		// Needed for CUDPSocket
#include "PartFile.h"		// Needed for CPartFile
#include "AddFileThread.h"	// Needed for CAddFileThread
#include "packets.h"
#include "PrefsUnifiedDlg.h"
#include "AICHSyncThread.h"

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif


BEGIN_EVENT_TABLE(CamuleDaemonApp, wxAppConsole)
	// Socket timers (TCP + UDO)
	EVT_CUSTOM(wxEVT_AMULE_TIMER, TM_UDPSOCKET, CamuleDaemonApp::OnUDPTimer)
	EVT_CUSTOM(wxEVT_AMULE_TIMER, TM_TCPSOCKET, CamuleDaemonApp::OnTCPTimer)

	// Core timer is OnRun
	EVT_CUSTOM(wxEVT_NOTIFY_EVENT, -1, CamuleDaemonApp::OnNotifyEvent)

	// Async dns handling
	EVT_CUSTOM(wxEVT_CORE_DNS_DONE, -1, CamuleDaemonApp::OnDnsDone)
	
	EVT_CUSTOM(wxEVT_CORE_SOURCE_DNS_DONE, -1, CamuleDaemonApp::OnSourcesDnsDone)

	// Hash ended notifier
	EVT_CUSTOM(wxEVT_CORE_FILE_HASHING_FINISHED, -1, CamuleDaemonApp::OnFinishedHashing)

	// Hashing thread finished and dead
	EVT_CUSTOM(wxEVT_CORE_FILE_HASHING_SHUTDOWN, -1, CamuleDaemonApp::OnHashingShutdown)

	// File completion ended notifier
	EVT_CUSTOM(wxEVT_CORE_FINISHED_FILE_COMPLETION, -1, CamuleDaemonApp::OnFinishedCompletion)

	// HTTPDownload finished
	EVT_CUSTOM(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, -1, CamuleDaemonApp::OnFinishedHTTPDownload)
END_EVENT_TABLE()

IMPLEMENT_APP(CamuleDaemonApp)

CamuleLocker::CamuleLocker() : wxMutexLocker(theApp.data_mutex)
{
	msStart = GetTickCount();
}

CamuleLocker::~CamuleLocker()
{
	uint msDone = GetTickCount();
	assert( (msDone - msStart) < 100);
}

int CamuleDaemonApp::OnRun()
{
	const uint uLoop = 100;
	AddDebugLogLineM(true, wxT("CamuleDaemonApp::OnRun()"));
	// lfroen: this loop is instead core timer.
	uint msWait = uLoop;
	m_Exit = false;
	while ( !m_Exit ) {
		if ( msWait <= uLoop) {
			wxThread::Sleep(msWait);
		}
		// lock data after sleep
		uint msRun = GetTickCount();
		CALL_APP_DATA_LOCK;

		OnCoreTimer(*((wxEvent *)0));
		ProcessPendingEvents();
		msRun = GetTickCount() - msRun;
		msWait = uLoop - msRun;
	}
	/*
	 * Stop all socket threads before entering
	 * shutdown sequence.
	 */
	listensocket->Delete();
	delete listensocket;
	listensocket = 0;

	clientudp->Delete();
	delete clientudp;
	clientudp = 0;
	
	ShutDown();
	return 0;
}

int CamuleDaemonApp::InitGui(bool ,wxString &)
{
	wxPendingEventsLocker = new wxCriticalSection;
	
	return 0;
}

int CamuleDaemonApp::OnExit()
{
	// lfroen: delete socket threads
	if (ECServerHandler) {
		ECServerHandler->Delete();
		ECServerHandler = 0;
	}
	
	return CamuleApp::OnExit();
}

void CamuleDaemonApp::ShowAlert(wxString msg, wxString title, int flags)
{
	if ( flags | wxICON_ERROR ) {
		title = _("ERROR:") + title;
	}
	AddLogLine(title + wxT(" ") + msg);
}


void CamuleDaemonApp::NotifyEvent(GUIEvent event)
{
	switch (event.ID) {
		// GUI->CORE events
		// it's daemon, so gui isn't here, but macros can be used as function calls
		case SHOW_CONN_STATE:
			if ( event.byte_value ) {
				const wxString id = theApp.serverconnect->IsLowID() ? _("with LowID") : _("with HighID");
				AddLogLine(_("Connected to ") + event.string_value + id);
			} else {
				if ( theApp.serverconnect->IsConnecting() ) {
					AddLogLine(_("Connecting to ") + event.string_value);
				} else {
					AddLogLine(_("Disconnected\n"));
				}
			}
			break;
		// search
	        case SEARCH_REQ:
			uploadqueue->AddUpDataOverheadServer(((Packet *)event.ptr_value)->GetPacketSize());
			serverconnect->SendPacket( (Packet *)event.ptr_value, 0 );
			if ( event.byte_value ) {
				searchlist->m_searchpacket = (Packet *)event.ptr_value;
			} else {
				searchlist->m_searchpacket = 0;
			}
			break;
	        case SEARCH_ADD_TO_DLOAD:
			downloadqueue->AddSearchToDownload((CSearchFile *)event.ptr_value, event.byte_value);
			break;

		case SHAREDFILES_SHOW_ITEM:
			//printf("SHAREDFILES_SHOW_ITEM: %p\n", event.ptr_value);
			break;
			
		case DOWNLOAD_CTRL_ADD_SOURCE:
		/*
		printf("ADD_SOURCE: adding source %p to partfile %s\n",
		       event.ptr_aux_value, ((CPartFile*)event.ptr_value)->GetFullName().c_str());
		*/
			break;

		case ADDLOGLINE:
			AddLogLine(event.string_value);
			break;
		case ADDDEBUGLOGLINE:
			//printf("DEBUGLOG: %s\n", event.string_value.c_str());
			break;
		default:
			//printf("WARNING: event %d in daemon should not happen\n", event.ID);
			break;
	}
}



CFriend *CamuleDaemonApp::FindFriend(CMD4Hash *WXUNUSED(hash), uint32 WXUNUSED(ip), uint16 WXUNUSED(port))
{
// 	if ( amuledlg && amuledlg->chatwnd ) {
// 		return 	amuledlg->chatwnd->FindFriend(*hash, ip, port);
// 	}
	return NULL;
}

