// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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


#include <cerrno>
#include <csignal>
#include <cmath>
#include <wx/toolbar.h>
#include <wx/utils.h>
#include <wx/file.h>
#include <wx/datetime.h>
#include <wx/config.h>

#ifndef __SYSTRAY_DISABLED__
#include "pixmaps/mule_TrayIcon.ico.xpm"
#include "pixmaps/mule_Tr_yellow.ico.xpm"
#include "pixmaps/mule_Tr_grey.ico.xpm"
#endif // __SYSTRAY_DISABLED__
#include "amuleDlg.h"		// Interface declarations.
#include "otherfunctions.h"	// Needed for CastItoIShort
#include "ED2KLink.h"		// Needed for CED2KLink
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "QueueListCtrl.h"	// Needed for CQueueListCtrl
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "sockets.h"		// Needed for CServerConnect
#include "ClientList.h"		// Needed for CClientList
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "SearchList.h"		// Needed for CSearchList
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "ListenSocket.h"	// Needed for CListenSocket
#include "ServerList.h"		// Needed for CServerList
#include "SysTray.h"		// Needed for CSysTray
#include "Preferences.h"	// Needed for CPreferences
#include "ChatWnd.h"		// Needed for CChatWnd
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "SharedFilesWnd.h"	// Needed for CSharedFilesWnd
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "ServerWnd.h"		// Needed for CServerWnd
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "PartFile.h"		// Needed for CPartFile
#include "KnownFile.h"		// Needed for CKnownFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"			// Needed for theApp
#include "opcodes.h"		// Needed for TM_FINISHEDHASHING
#include "muuli_wdr.h"		// Needed for ID_BUTTONSERVERS

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__) || defined(__WXX11__)
#include "aMule.xpm"
#endif

#ifdef __USE_SPLASH__
#include "splash.xpm"
#endif

BEGIN_EVENT_TABLE(CamuleDlg, wxFrame)

	EVT_TOOL(ID_BUTTONSERVERS, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSEARCH, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONTRANSFER, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSHARED, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONMESSAGES, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSTATISTICS, CamuleDlg::OnToolBarButton)

	EVT_TOOL(ID_BUTTONNEWPREFERENCES, CamuleDlg::OnPrefButton)
	
	EVT_TOOL(ID_BUTTONCONNECT, CamuleDlg::OnBnConnect)

	EVT_CLOSE(CamuleDlg::OnClose)
	EVT_ICONIZE(CamuleDlg::OnMinimize)
	
	EVT_BUTTON(ID_BUTTON_FAST, CamuleDlg::OnBnClickedFast)
	EVT_BUTTON(ID_PREFS_OK_TOP, CamuleDlg::OnBnClickedPrefOk)
	
	EVT_TIMER(ID_GUITIMER, CamuleDlg::OnGUITimer)
	
END_EVENT_TABLE()

#ifndef wxCLOSE_BOX
#	define wxCLOSE_BOX 0
#endif

CamuleDlg::CamuleDlg(wxWindow* pParent, wxString title, wxPoint where, wxSize dlg_size) : wxFrame(
	pParent, CamuleDlg::IDD, title, where, dlg_size,
	wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxDIALOG_NO_PARENT|
	wxTHICK_FRAME|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX )
{

	wxInitAllImageHandlers();

	imagelist.Create(16,16);	
	
	for (uint32 i=0; i<22; i++) {
		imagelist.Add(wxBitmap(clientImages(i)));
	}
	
	bool override_where = (where != wxDefaultPosition);
	bool override_size = ((dlg_size.x != DEFAULT_SIZE_X) || (dlg_size.y != DEFAULT_SIZE_Y));
	
	if (!LoadGUIPrefs(override_where, override_size)) {
		// Prefs not loaded for some reason, exit
		printf("ERROR!!! Unable to load Preferences\n");
		return;
	}
	
	is_safe_state = false;
	is_hidden = false;
	
	SetIcon(wxICON(aMule));

	srand(time(NULL));

	// get rid of sigpipe
#ifndef __WXMSW__
	signal(SIGPIPE, SIG_IGN);
#endif

	// Create new sizer and stuff a wxPanel in there.
	wxFlexGridSizer *s_main = new wxFlexGridSizer(1);
	s_main->AddGrowableCol(0);
	s_main->AddGrowableRow(0);
	
	wxPanel* p_cnt = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize);
	s_main->Add(p_cnt, 0, wxGROW|wxEXPAND, 0);
	muleDlg(p_cnt, false, true);

	SetSizer( s_main, true );
	
	// Create ToolBar from the one designed by wxDesigner (BigBob)
	m_wndToolbar = CreateToolBar( wxTB_HORIZONTAL|wxNO_BORDER|wxTB_TEXT|
	                                wxTB_3DBUTTONS|wxTB_FLAT|wxCLIP_CHILDREN );
	m_wndToolbar->SetToolBitmapSize(wxSize(32, 32));
	muleToolbar( m_wndToolbar );

	serverwnd = new CServerWnd(p_cnt);
	
	AddLogLine(true, PACKAGE_STRING);
	
	searchwnd = new CSearchDlg(p_cnt);
	transferwnd = new CTransferWnd(p_cnt);
	prefsunifiedwnd = new PrefsUnifiedDlg(p_cnt);
	sharedfileswnd = new CSharedFilesWnd(p_cnt);
	statisticswnd = new CStatisticsDlg(p_cnt);
	chatwnd = new CChatWnd(p_cnt);

	serverwnd->Show(FALSE);
	searchwnd->Show(FALSE);
	transferwnd->Show(FALSE);
	sharedfileswnd->Show(FALSE);
	statisticswnd->Show(FALSE);
	chatwnd->Show(FALSE);
	
	// Create the GUI timer
	gui_timer=new wxTimer(this,ID_GUITIMER);
	if (!gui_timer) {
		AddLogLine(false, CString(_("Fatal Error: Failed to create Timer")));
	}
		
	// Set Serverlist as active window
	activewnd=NULL;
	SetActiveDialog(serverwnd);
	m_wndToolbar->ToggleTool(ID_BUTTONSERVERS, true );	

	ToggleFastED2KLinksHandler();
	
	is_safe_state = true;
	
	// Start the Gui Timer
	
	// Note: wxTimer can be off by more than 10% !!!
	// In addition to the systematic error introduced by wxTimer, we are losing
	// timer cycles due to high CPU load.  I've observed about 0.5% random loss of cycles under
	// low load, and more than 6% lost cycles with heavy download traffic and/or other tasks
	// in the system, such as a video player or a VMware virtual machine.
	// The upload queue process loop has now been rewritten to compensate for timer errors.
	// When adding functionality, assume that the timer is only approximately correct;
	// for measurements, always use the system clock [::GetTickCount()].
	gui_timer->Start(100);	
	Show(TRUE);

#ifndef __SYSTRAY_DISABLED__
	CreateSystray(wxString::Format(wxT("%s %s"), wxT(PACKAGE), wxT(VERSION)));
#endif 
	
	// splashscreen
	#ifdef __USE_SPLASH__
	if (theApp.glob_prefs->UseSplashScreen() && !theApp.glob_prefs->GetStartMinimized()) {
		new wxSplashScreen( wxBitmap(splash_xpm),
		                    wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_TIMEOUT,
		                    5000, NULL, -1, wxDefaultPosition, wxDefaultSize,
		                    wxSIMPLE_BORDER|wxSTAY_ON_TOP
		);
	}
	#endif

	// Init statistics stuff, better do it asap
	statisticswnd->Init();
	statisticswnd->SetUpdatePeriod();

	// must do initialisations here.. 
	serverwnd->serverlistctrl->Init(theApp.serverlist);	

	// Initialize and sort all lists.
	// FIX: Remove from here and put these back to the OnInitDialog()s
	// and call the OnInitDialog()s here!
	transferwnd->downloadlistctrl->InitSort();
	transferwnd->uploadlistctrl->InitSort();
	transferwnd->queuelistctrl->InitSort();
	serverwnd->serverlistctrl->InitSort();
	sharedfileswnd->sharedfilesctrl->InitSort();

	// call the initializers
	transferwnd->OnInitDialog();	
	
	searchwnd->UpdateCatChoice();
	
	// Must we start minimized?
	if (theApp.glob_prefs->GetStartMinimized()) {
		#ifndef __SYSTRAY_DISABLED__	
		// Send it to tray?
		if (theApp.glob_prefs->DoMinToTray()) {
			Hide_aMule();
		} else {
			Iconize(TRUE);
		}
		#else
			Iconize(TRUE);
		#endif
	}
	
}


// Madcat - Toggles Fast ED2K Links Handler on/off.
void CamuleDlg::ToggleFastED2KLinksHandler()
{
	// Errorchecking in case the pointer becomes invalid ...
	if (s_fed2klh == NULL) {
		wxLogWarning(wxT("Unable to find Fast ED2K Links handler sizer! Hiding FED2KLH aborted."));
		return;
	}
	s_dlgcnt->Show(s_fed2klh, theApp.glob_prefs->GetFED2KLH());
	s_dlgcnt->Layout();
	searchwnd->ToggleLinksHandler();
}


void CamuleDlg::SetActiveDialog(wxWindow* dlg)
{
	switch ( dlg->GetId() ) {
		case IDD_SERVER:
		case IDD_SEARCH:
		case IDD_TRANSFER:
		case IDD_FILES:
		case IDD_CHAT:
		case IDD_STATISTICS:
			m_nActiveDialog = dlg->GetId();
			break;
		default:
			printf("Unknown window passed to SetActiveDialog!\n");
			return;
	}

	if ( activewnd ) {
		activewnd->Show(FALSE);
		contentSizer->Remove(activewnd);
	}
	
	contentSizer->Add(dlg, 1, wxALIGN_LEFT|wxEXPAND);
	dlg->Show(TRUE);
	activewnd=dlg;
	s_dlgcnt->Layout();
}


class QueryDlg : public wxDialog {
public:
	QueryDlg(wxWindow* parent) : wxDialog(
		parent, 21373, _("Desktop integration"), wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU )
	{
		wxSizer* content=desktopDlg(this, TRUE);
		content->Show(this, TRUE);
		Centre();
	};
protected:
	void OnOk(wxCommandEvent& WXUNUSED(evt)) { EndModal(0); };
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(QueryDlg, wxDialog)
	EVT_BUTTON(ID_OK, QueryDlg::OnOk)
END_EVENT_TABLE()


void CamuleDlg::changeDesktopMode()
{
	QueryDlg query(this);

	wxRadioBox* radiobox = (wxRadioBox*)query.FindWindow(ID_SYSTRAYSELECT);
	
	if ( theApp.glob_prefs->GetDesktopMode() )
		radiobox->SetSelection( theApp.glob_prefs->GetDesktopMode() - 1 );
	else
		radiobox->SetSelection( 0 );
	
	query.ShowModal();
	
	theApp.glob_prefs->SetDesktopMode( radiobox->GetSelection() + 1 );
}


#ifndef __SYSTRAY_DISABLED__
void CamuleDlg::CreateSystray(const wxString& title)
{
	// create the docklet (at this point we already have preferences!)
	if( !theApp.glob_prefs->GetDesktopMode() ) {
		// ok, it's not set yet.
		changeDesktopMode();
	}
	
	m_wndTaskbarNotifier = new CSysTray(this, theApp.glob_prefs->GetDesktopMode(), title);
}


void CamuleDlg::RemoveSystray()
{
	if (m_wndTaskbarNotifier) {
		delete m_wndTaskbarNotifier;
	}
}
#endif // __SYSTRAY_DISABLED__


void CamuleDlg::OnToolBarButton(wxCommandEvent& ev)
{
	static int lastbutton = ID_BUTTONSERVERS;
	
	// Kry - just if the app is ready for it
	if ( theApp.IsReady ) {

		if ( lastbutton != ev.GetId() ) {
			switch ( ev.GetId() ) {
				case ID_BUTTONSERVERS:
					SetActiveDialog(serverwnd);
					// Set serverlist splitter position
					((wxSplitterWindow*)FindWindow("SrvSplitterWnd"))->SetSashPosition(srv_split_pos, true);
					break;

				case ID_BUTTONSEARCH:
					SetActiveDialog(searchwnd);
					break;

				case ID_BUTTONTRANSFER:
					SetActiveDialog(transferwnd);
					// Set splitter position
					((wxSplitterWindow*)FindWindow("splitterWnd"))->SetSashPosition(split_pos, true);
					break;

				case ID_BUTTONSHARED:
					SetActiveDialog(sharedfileswnd);
					break;

				case ID_BUTTONMESSAGES:
					SetActiveDialog(chatwnd);
					break;

				case ID_BUTTONSTATISTICS:
					SetActiveDialog(statisticswnd);
					break;

				// This shouldn't happen, but just in case
				default:
					printf("Unknown button triggered CamuleApp::OnToolBarButton().\n");
					break;
			}
		}
		
		m_wndToolbar->ToggleTool(lastbutton, lastbutton == ev.GetId() );
		lastbutton = ev.GetId();
	}
}


void CamuleDlg::OnPrefButton(wxCommandEvent& WXUNUSED(ev))
{
	if ( theApp.IsReady ) {
		prefsunifiedwnd->ShowModal();
	}
}

CamuleDlg::~CamuleDlg()
{
	printf("Shutting down aMule...\n");

	SaveGUIPrefs();

	theApp.OnlineSig(true);

	printf("aMule dialog destroyed\n");
}


void CamuleDlg::OnBnConnect(wxCommandEvent& WXUNUSED(evt))
{
	if (!theApp.serverconnect->IsConnected()) {
		//connect if not currently connected
		if (!theApp.serverconnect->IsConnecting()) {
			AddLogLine(true, _("Connecting"));
			theApp.serverconnect->ConnectToAnyServer();
			ShowConnectionState(false);
		} else {
			theApp.serverconnect->StopConnectionTry();
			ShowConnectionState(false);
		}
	} else {
		//disconnect if currently connected
		theApp.serverconnect->Disconnect();
		theApp.OnlineSig();
	}
}

void CamuleDlg::ResetLog(uint8 whichone)
{
	wxTextCtrl* ct = NULL;

	switch (whichone){
		case 1:
			ct=(wxTextCtrl*)serverwnd->FindWindow(ID_LOGVIEW);
			// Delete log file aswell.
			wxRemoveFile(wxString::Format("%s/.aMule/logfile", getenv("HOME")));
			break;
		case 2:
			ct=(wxTextCtrl*)serverwnd->FindWindow(ID_SERVERINFO);
			break;
		default:
			return;
	}

	if(ct) {
		ct->SetValue("");
	}
}


void CamuleDlg::ResetDebugLog()
{
	serverwnd->logbox.Clear();
}


void CamuleDlg::AddLogLine(bool addtostatusbar, const wxChar* line, ...)
{

	va_list argptr;
	va_start(argptr, line);
	wxString bufferline = wxString::FormatV( line, argptr );
	bufferline.Truncate(1000); // Max size 1000 chars
	va_end(argptr);

	// Remove newlines at end, they cause problems with the layout...
	while ( bufferline.Last() == '\n' )
		bufferline.RemoveLast();

	// Escape "&"s, which would otherwise not show up
	bufferline.Replace("&", "&&");

	if (addtostatusbar) {
		wxStaticText* text=(wxStaticText*)FindWindow("infoLabel");
		text->SetLabel(bufferline);
		Layout();
	}

	bufferline = wxDateTime::Now().FormatDate() + wxT(" ") 
		+ wxDateTime::Now().FormatTime() + wxT(": ") 
		+ bufferline + wxT("\n");

	wxTextCtrl* ct= (wxTextCtrl*)serverwnd->FindWindow(ID_LOGVIEW);
	if(ct) {
		ct->AppendText(bufferline);
		ct->ShowPosition(ct->GetValue().Length()-1);
	}

	// Write into log file
	wxString filename = wxString::Format("%s/.aMule/logfile", getenv("HOME"));
	wxFile file(filename, wxFile::write_append);

	if ( file.IsOpened() ) {
		file.Write(bufferline.c_str());
		file.Close();
	}
}


void CamuleDlg::AddDebugLogLine(bool addtostatusbar, const wxChar* line, ...)
{
	if (theApp.glob_prefs->GetVerbose()) {
		va_list argptr;
		va_start(argptr, line);
		wxString bufferline = wxString::FormatV(line, argptr);
		bufferline.Truncate(1000); // Max size 1000 chars
		va_end(argptr);
		
		AddLogLine(addtostatusbar, "%s", bufferline.c_str());
	}
}


void CamuleDlg::AddServerMessageLine(char* line, ...)
{
	wxString content;
	va_list argptr;
	va_start(argptr, line);
	wxString bufferline = wxString::FormatV( line, argptr );
	bufferline.Truncate(500); // Max size 500 chars
	va_end(argptr);

	wxTextCtrl* cv=(wxTextCtrl*)serverwnd->FindWindow(ID_SERVERINFO);
	if(cv) {
		cv->AppendText(bufferline + "\n");
		cv->ShowPosition(cv->GetValue().Length()-1);
	}
}


void CamuleDlg::ShowConnectionState(bool connected, wxString server, bool iconOnly)
{
	enum state { sUnknown = -1, sDisconnected = 0, sLowID = 1, sConnecting = 2, sHighID = 3 };
	static state LastState = sUnknown;


	serverwnd->UpdateMyInfo();
	
	state NewState = sUnknown;
	if ( connected ) {
		if ( theApp.serverconnect->IsLowID() ) {
			NewState = sLowID;
		} else {
			NewState = sHighID;
		}
	} else if ( theApp.serverconnect->IsConnecting() ) {
		NewState = sConnecting;
	} else {
		NewState = sDisconnected;
	}


	if ( LastState != NewState ) {
		((wxStaticBitmap*)FindWindow("connImage"))->SetBitmap(connImages( NewState ));

		m_wndToolbar->DeleteTool(ID_BUTTONCONNECT);
		
		wxStaticText* connLabel = (wxStaticText*)FindWindow("connLabel");
		switch ( NewState ) {
			case sLowID:
			case sHighID: {
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, wxString(_("Disconnect")), connButImg(1), wxString(_("Disconnect from current server")));
				wxStaticText* tx=(wxStaticText*)FindWindow("infoLabel");
				tx->SetLabel(wxString(_("Connection established on:")) + wxString(server));
				connLabel->SetLabel(server);
				break;
			}
			
			case sConnecting:
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, wxString(_("Cancel")), connButImg(2), wxString(_("Stops the current connection attempts")));
				connLabel->SetLabel(wxString(_("Connecting")));
				break;
			
			case sDisconnected:
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, wxString(_("Connect")), connButImg(0), wxString(_("Connect to any server")));
				connLabel->SetLabel(wxString(_("Not Connected")));
				AddLogLine(true, wxString(_("Disconnected")));
				break;

			default:
				break;
		}
		
		m_wndToolbar->Realize();

		ShowUserCount(0, 0);
	}

	LastState = NewState;
}

void CamuleDlg::ShowUserCount(uint32 user_toshow, uint32 file_toshow)
{
	uint32 totaluser = 0, totalfile = 0;
	
	if( user_toshow || file_toshow ) {
		theApp.serverlist->GetUserFileStatus( totaluser, totalfile );
	}
	
	wxString buffer = wxString::Format( "%s: %s(%s) | %s: %s(%s)", _("Users"), CastItoIShort(user_toshow).c_str(), CastItoIShort(totaluser).c_str(), _("Files"), CastItoIShort(file_toshow).c_str(), CastItoIShort(totalfile).c_str());
	
	wxStaticCast(FindWindow("userLabel"), wxStaticText)->SetLabel(buffer);

	Layout();
}

void CamuleDlg::ShowTransferRate()
{
	float	kBpsUp = theApp.uploadqueue->GetKBps();
	float 	kBpsDown = theApp.downloadqueue->GetKBps();

	wxString buffer;
	if( theApp.glob_prefs->ShowOverhead() )
	{
		float overhead_up = theApp.uploadqueue->GetUpDatarateOverhead();
		float overhead_down = theApp.downloadqueue->GetDownDatarateOverhead();
		buffer.Printf(_("Up: %.1f(%.1f) | Down: %.1f(%.1f)"), kBpsUp, overhead_up/1024, kBpsDown, overhead_down/1024);
	} else {
		buffer.Printf(_("Up: %.1f | Down: %.1f"), kBpsUp, kBpsDown);
	}
	buffer.Truncate(50); // Max size 50

	((wxStaticText*)FindWindow("speedLabel"))->SetLabel(buffer);
	Layout();


#ifndef __SYSTRAY_DISABLED__
	// set trayicon-icon
	int percentDown = (int)ceil((kBpsDown*100) / theApp.glob_prefs->GetMaxGraphDownloadRate());
	UpdateTrayIcon( ( percentDown > 100 ) ? 100 : percentDown);

	wxString buffer2;
	if ( theApp.serverconnect->IsConnected() ) {
		buffer2.Printf("aMule (%s | %s)", buffer.c_str(), _("Connected") );
	} else {
		buffer2.Printf("aMule (%s | %s)", buffer.c_str(), _("Disconnected") );
	}
	char* buffer3 = nstrdup(buffer2.c_str());
	m_wndTaskbarNotifier->TraySetToolTip(buffer3);
	delete[] buffer3;
#endif

	wxStaticBitmap* bmp=(wxStaticBitmap*)FindWindow("transferImg");
	bmp->SetBitmap(dlStatusImages((kBpsUp>0.01 ? 2 : 0) + (kBpsDown>0.01 ? 1 : 0)));
}

void CamuleDlg::OnClose(wxCloseEvent& evt)
{
	// Are we already shutting down or still on init?
	if ( is_safe_state == false ) {
		return;
	}

	if (evt.CanVeto() && theApp.glob_prefs->IsConfirmExitEnabled() ) {
		if (wxNO == wxMessageBox(wxString(_("Do you really want to exit aMule?")), wxString(_("Exit confirmation")), wxYES_NO)) {
			evt.Veto();
			return;
		}
	}

	// we are going DOWN
	is_safe_state = false;

	// Stop the GUI Timer
	delete gui_timer;	
	
	// Kry - Save the sources seeds on app exit
	if (theApp.glob_prefs->GetSrcSeedsOn()) {
		theApp.downloadqueue->SaveSourceSeeds();
	}
	
	theApp.OnlineSig(); // Added By Bouc7

	// Close sockets to avoid new clients coming in
	if (theApp.listensocket) {
		theApp.listensocket->StopListening();
	}
	if (theApp.clientudp) {
		theApp.clientudp->Destroy();
	}
	if (theApp.serverconnect) {
		theApp.serverconnect->Disconnect();
	}

	// saving data & stuff
	if (theApp.knownfiles) {
		theApp.knownfiles->Save();
	}

	if (theApp.glob_prefs) {
		theApp.glob_prefs->Add2TotalDownloaded(theApp.stat_sessionReceivedBytes);
		theApp.glob_prefs->Add2TotalUploaded(theApp.stat_sessionSentBytes);
	}

	if (theApp.glob_prefs) {
		theApp.glob_prefs->Save();
	}

	transferwnd->downloadlistctrl->DeleteAllItems();
	//amuledlg->chatwnd->chatselector->DeleteAllItems();
	if (theApp.clientlist) {
		theApp.clientlist->DeleteAll();
	}

#ifndef __SYSTRAY_DISABLED__
	//We want to delete the systray too!
	RemoveSystray();
#endif

	#warning This will be here till the core close is != app close
	theApp.ShutDown();
	
}


#ifndef __SYSTRAY_DISABLED__
void CamuleDlg::UpdateTrayIcon(int procent)
{
	// set the limits of where the bar color changes (low-high)
	int pLimits16[1] = {100};

	// set the corresponding color for each level
	COLORREF pColors16[1] = {statisticswnd->GetTrayBarColor()};  // ={theApp.glob_prefs->GetStatsColor(11)};

	// load our limit and color info
	m_wndTaskbarNotifier->SetColorLevels(pLimits16, pColors16, 1);

	// generate the icon (destroy these icon using DestroyIcon())
	int pVals16[1] = {procent};

	// ei hienostelua. tarvii kuitenki pelleill?gtk:n kanssa
	char** data;
	if(!theApp.serverconnect) {
		data=mule_Tr_grey_ico;
	} else {
		if (theApp.serverconnect->IsConnected()) {
			if(!theApp.serverconnect->IsLowID()) {
				data=mule_TrayIcon_ico;
			} else {
				data=mule_Tr_yellow_ico;
			}
		} else {
			data=mule_Tr_grey_ico;
		}
	}
	m_wndTaskbarNotifier->TraySetIcon(data, true, pVals16);
}
#endif // __SYSTRAY_DISABLED__


//BEGIN - enkeyDEV(kei-kun) -TaskbarNotifier-
void CamuleDlg::ShowNotifier(wxString Text, int MsgType, bool ForceSoundOFF)
{
}
//END - enkeyDEV(kei-kun) -TaskbarNotifier-


void CamuleDlg::OnBnClickedFast(wxCommandEvent& WXUNUSED(evt))
{
	if (!theApp.serverconnect->IsConnected()) {
		wxMessageDialog* bigbob = new wxMessageDialog(this, wxT(_("The ED2K link has been added but your download won't start until you connect to a server.")), wxT(_("Not Connected")), wxOK|wxICON_INFORMATION);
		bigbob->ShowModal();
		delete bigbob;
	}
	
	StartFast((wxTextCtrl*)FindWindow("FastEd2kLinks"));
}


// Pass pointer to textctrl which contains the links as argument
void CamuleDlg::StartFast(wxTextCtrl *ctl)
{
	for ( int i = 0; i < ctl->GetNumberOfLines(); i++ ) {
		wxString strlink = ctl->GetLineText(i);
		
		if ( strlink.IsEmpty() )
			continue;

		if ( strlink.Last() != '/' )
			strlink += "/";
			
		try {
			CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(strlink);
			
			if ( pLink ) {
				if( pLink->GetKind() == CED2KLink::kFile ) {
					theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink());
				} else {
					throw wxString("Bad link");
				}
				
				delete pLink;
			} else {
				printf("Failed to create ED2k link from URL\n");
			}
		}
		catch(wxString error) {
			wxString msg = wxString::Format( _("This ed2k link is invalid (%s)"), error.c_str() );
			theApp.amuledlg->AddLogLine( true, _("Invalid link: %s"), msg.c_str());
		}
	}
ctl->SetValue("");
}


// Formerly known as LoadRazorPrefs()
bool CamuleDlg::LoadGUIPrefs(bool override_pos, bool override_size)
{
	// Create a config base for loading razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without loading
	if (config == NULL) {
		return false;
	}

	// The section where to save in in file
	wxString section = wxT("/Razor_Preferences/");

	// Get window size and position
	int x1 = config->Read(_T(section+wxT("MAIN_X_POS")), -1l);
	int y1 = config->Read(_T(section+wxT("MAIN_Y_POS")), -1l);
	int x2 = config->Read(_T(section+wxT("MAIN_X_SIZE")), 0l);
	int y2 = config->Read(_T(section+wxT("MAIN_Y_SIZE")), 0l);

	split_pos = config->Read(_T(section+wxT("SPLITTER_POS")), 463l);
	// Kry - Random usable pos for srv_split_pos
	srv_split_pos = config->Read(_T(section+wxT("SRV_SPLITTER_POS")), 463l);

	if (!override_pos) {
		// If x1 and y1 != 0 Redefine location
		if((x1 != -1) && (y1 != -1)) {
			Move(x1, y1);
		}
	}

	if (!override_size) {
		if (x2 > 0 && y2 > 0) {
			SetClientSize(x2, y2);
		}
	}

	return true;
}


bool CamuleDlg::SaveGUIPrefs()
{
	/* Razor 1a - Modif by MikaelB
	   Save client size and position */

	// Create a config base for saving razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without saving
	if (config == NULL) {
		return false;
	}
	// The section where to save in in file
	wxString section = "/Razor_Preferences/";

	// Main window location and size
	int x1, y1, x2, y2;
	GetPosition(&x1, &y1);
	GetSize(&x2, &y2);

	// Saving window size and position
	config->Write(_T(section+"MAIN_X_POS"), (long) x1);
	config->Write(_T(section+"MAIN_Y_POS"), (long) y1);
	config->Write(_T(section+"MAIN_X_SIZE"), (long) x2);
	config->Write(_T(section+"MAIN_Y_SIZE"), (long) y2);

	// Saving sash position of splitter in transfer window
	config->Write(_T(section+"SPLITTER_POS"), (long) split_pos);

	// Saving sash position of splitter in server window
	config->Write(_T(section+"SRV_SPLITTER_POS"), (long) srv_split_pos);

	config->Flush(true);

	/* End modif */

	return true;
}


//hides amule
void CamuleDlg::Hide_aMule(bool iconize)
{
	
	if (!is_hidden) {
		transferwnd->downloadlistctrl->Freeze();
		transferwnd->uploadlistctrl->Freeze();
		serverwnd->serverlistctrl->Freeze();
		sharedfileswnd->sharedfilesctrl->Freeze();
		transferwnd->downloadlistctrl->Show(FALSE);
		serverwnd->serverlistctrl->Show(FALSE);
		transferwnd->uploadlistctrl->Show(FALSE);
		sharedfileswnd->sharedfilesctrl->Show(FALSE);
		Freeze();
		if (iconize) {
			Iconize(TRUE);
		}
		Show(FALSE);
		
		is_hidden = true;
		
	} 
	
}


//shows amule
void CamuleDlg::Show_aMule(bool uniconize)
{

	if (is_hidden) {
		
		transferwnd->downloadlistctrl->Show(TRUE);
		transferwnd->uploadlistctrl->Show(TRUE);
		serverwnd->serverlistctrl->Show(TRUE);
		sharedfileswnd->sharedfilesctrl->Show(TRUE);
		transferwnd->downloadlistctrl->Thaw();
		serverwnd->serverlistctrl->Thaw();
		transferwnd->uploadlistctrl->Thaw();
		sharedfileswnd->sharedfilesctrl->Thaw();
		Thaw();
		Update();
		Refresh();
		if (uniconize) {
			Show(TRUE);
		}
		
		is_hidden = false;

	} 
	
}


void CamuleDlg::OnMinimize(wxIconizeEvent& evt)
{
#ifndef __SYSTRAY_DISABLED__
	if (theApp.glob_prefs->DoMinToTray()) {
		if (evt.Iconized()) {
			Hide_aMule(false);
		} else {
			if (SafeState()) {
				Show_aMule(true);
			} else {
				Show_aMule(false);
			}
		}
	}
#endif
}


void CamuleDlg::OnBnClickedPrefOk(wxCommandEvent& WXUNUSED(event))
{
	prefsunifiedwnd->EndModal(TRUE);
}

void CamuleDlg::OnGUITimer(wxTimerEvent& WXUNUSED(evt))
{
	// Former TimerProc section
	
	static uint32	msPrev1, msPrev5, msPrevGraph, msPrevStats;
	static uint32	msPrevHist;
	
	uint32 			msCur = theApp.GetUptimeMsecs();

	// can this actually happen under wxwin ?
	if (!SafeState()) {
		return;
	}
	
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
		statisticswnd->RecordHistory();
		#endif
	}

	if (msCur-msPrev1 > 950) {  // approximately every second
		msPrev1 = msCur;		
		statisticswnd->UpdateConnectionsStatus();
	}

	bool bStatsVisible = (!IsIconized() && StatisticsWindowActive());
	int msGraphUpdate= theApp.glob_prefs->GetTrafficOMeterInterval()*1000;
	if ((msGraphUpdate > 0)  && ((msCur / msGraphUpdate) > (msPrevGraph / msGraphUpdate))) {
		// trying to get the graph shifts evenly spaced after a change in the update period
		msPrevGraph = msCur;
		#ifndef __WXMAC__
		statisticswnd->UpdateStatGraphs(bStatsVisible);
		#endif
	}

	int sStatsUpdate = theApp.glob_prefs->GetStatsInterval();
	if ((sStatsUpdate > 0) && ((int)(msCur - msPrevStats) > sStatsUpdate*1000)) {
		if (bStatsVisible) {
			msPrevStats = msCur;
			statisticswnd->ShowStatistics();
		}
	}

	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		ShowTransferRate();
	}
	
}
