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


// SearchDlg.cpp : implementation file
//

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/sizer.h>

#include "SearchDlg.h"		// Interface declarations.
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "CMemFile.h"		// Needed for CMemFile
#include "SearchList.h"		// Needed for CSearchList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "otherfunctions.h"	// Needed for URLEncode
#include "packets.h"		// Needed for Packet
#include "server.h"			// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "sockets.h"		// Needed for CServerConnect
#include "amule.h"			// Needed for theApp
#include "SearchListCtrl.h"	// Needed for CSearchListCtrl
#include "muuli_wdr.h"		// Needed for IDC_STARTS
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "MuleNotebook.h"

#define ID_SEARCHTIMER 55219

// just to keep compiler happy
static wxCommandEvent nullEvent;


BEGIN_EVENT_TABLE(CSearchDlg, wxPanel)
	EVT_BUTTON(IDC_STARTS, CSearchDlg::OnBnClickedStarts)
	
	EVT_TEXT_ENTER(IDC_SEARCHNAME, CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHMIN, CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHMAX, CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHEXTENSION, CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_EDITSEARCHAVAIBILITY, CSearchDlg::OnBnClickedStarts)
	EVT_TEXT_ENTER(IDC_SEARCHWEBNAME, CSearchDlg::OnBtnWebSearch)
	
	EVT_TEXT(IDC_SEARCHNAME, CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_SEARCHWEBNAME, CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHMIN, CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHMAX, CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHEXTENSION, CSearchDlg::OnFieldsChange)
	EVT_TEXT(IDC_EDITSEARCHAVAIBILITY, CSearchDlg::OnFieldsChange)
	
	EVT_BUTTON(IDC_CANCELS, CSearchDlg::OnBnClickedCancels)
	EVT_BUTTON(IDC_CLEARALL, CSearchDlg::OnBnClickedClearall)
	
	EVT_TIMER(ID_SEARCHTIMER, CSearchDlg::OnTimer)
	EVT_LIST_ITEM_SELECTED(ID_SEARCHLISTCTRL, CSearchDlg::OnListItemSelected)
	EVT_BUTTON(IDC_SDOWNLOAD, CSearchDlg::OnBnClickedSdownload)
	EVT_BUTTON(IDC_SEARCH_RESET, CSearchDlg::OnBnClickedSearchReset)
	EVT_MULENOTEBOOK_PAGE_CLOSED(ID_NOTEBOOK, CSearchDlg::OnSearchClosed)
	EVT_BUTTON(ID_BTN_DDLOAD, CSearchDlg::DirectDownload)
	EVT_RIGHT_DOWN(CSearchDlg::OnRMButton)
	EVT_BUTTON(ID_WEBSEARCH_SUBMIT, CSearchDlg::OnBtnWebSearch)

	EVT_MENU(MP_CLOSE_TAB, CSearchDlg::OnPopupClose)
	EVT_MENU(MP_CLOSE_ALL_TABS, CSearchDlg::OnPopupCloseAll)
	EVT_MENU(MP_CLOSE_OTHER_TABS, CSearchDlg::OnPopupCloseOthers)
END_EVENT_TABLE()



CSearchDlg::CSearchDlg(wxWindow* pParent)
: wxPanel(pParent, CSearchDlg::IDD), m_timer(this, ID_SEARCHTIMER)
{
	globalsearch = false;

	searchpacket = NULL;

	wxSizer* content=searchDlg(this, true);
	content->Show(this, true);

	progressbar = (wxGauge*)FindWindowById(ID_SEARCHPROGRESS);
	wxASSERT( progressbar );
	
	notebook = (CMuleNotebook*)FindWindowById(ID_NOTEBOOK);
	wxASSERT( notebook );

	// Initialise the image list
	wxImageList* m_ImageList = new wxImageList(16,16);
	m_ImageList->Add(amuleSpecial(3));
	m_ImageList->Add(amuleSpecial(4));
	notebook->AssignImageList(m_ImageList);

	// allow notebook to dispatch right mouse clicks to us
	notebook->SetMouseListener(GetEventHandler());

	ToggleLinksHandler();
}


// Enable the download button when there are items selected
void CSearchDlg::OnListItemSelected(wxListEvent& WXUNUSED(event))
{
	FindWindowById(IDC_SDOWNLOAD)->Enable(true);
}

void CSearchDlg::OnSearchClosed(wxNotebookEvent& evt) 
{
	// Abort global search if it was last tab that was closed.
	if ( evt.GetSelection() == (notebook->GetPageCount() - 1 ) ) {
		OnBnClickedCancels(nullEvent);
	}
}

void CSearchDlg::OnBnClickedStarts(wxCommandEvent& WXUNUSED(evt))
{
	OnBnClickedCancels(nullEvent);
		
	StartNewSearch();
}


// Enables or disables the Reset and Start button depending on the conents of the text fields
void CSearchDlg::OnFieldsChange(wxCommandEvent& WXUNUSED(evt))
{
	// These are the IDs of the search-fields 
	int textfields[] = { IDC_SEARCHNAME, IDC_EDITSEARCHMIN, IDC_EDITSEARCHMAX, IDC_EDITSEARCHEXTENSION, IDC_EDITSEARCHAVAIBILITY, IDC_SEARCHWEBNAME};

	bool enable = false;
	for ( uint16 i = 0; i < itemsof(textfields); i++ ) {
		enable |= ((wxTextCtrl*)FindWindowById( textfields[i] ))->GetLineLength(0);
	}
	
	// Enable the Reset button if any fields contain text
	FindWindowById(IDC_SEARCH_RESET)->Enable( enable );
	
	// enable web search button
	FindWindowById(ID_WEBSEARCH_SUBMIT)->Enable( enable );
	
	// Enable the Start button if the Name field contains text
	FindWindowById(IDC_STARTS)->Enable( ((wxTextCtrl*)FindWindowById(IDC_SEARCHNAME))->GetLineLength(0) );
}


// This function handles global searches
void CSearchDlg::OnTimer(wxTimerEvent& WXUNUSED(evt))
{
	if ( theApp.serverconnect->IsConnected() ) {
		CServer* toask = NULL;
		
		for ( uint16 i = 0; i < theApp.serverlist->GetServerCount(); i++ ) {
			/* Get the next server. Safer than using GetServerAt() in this case,
			   as it will just wrap if the list is reduced. GetNextSearchServer 
			   position is not reset, which means that chances of getting a new
			   server immediatly are improved. */
			CServer* server = theApp.serverlist->GetNextSearchServer();
		
			// Check if we have asked this server before
			if ( askedlist.find( server ) == askedlist.end() ) {
				// We know know a server which havent been asked yet, no need to continue looking
				toask = server;
				break;
			}
		}
		
		// If we found a valid server
		if ( toask ) {
			// Add the the server to the list of asked servers
			askedlist.insert( toask );

			// Ask the server
			theApp.serverconnect->SendUDPPacket(searchpacket, toask, false);
			
			// Increment the progress bar only if there are tabs
			if ( notebook->GetPageCount() && ( progressbar->GetValue() < progressbar->GetRange() ) ) {
				progressbar->SetValue( progressbar->GetValue() + 1 );
			}
		} else {
				OnBnClickedCancels(nullEvent);
		};
	} else {
		// Cancel if we arn't connected
		OnBnClickedCancels(nullEvent);
	}
}


bool CSearchDlg::CheckTabNameExists(wxString searchString) 
{
	int nPages = notebook->GetPageCount();
	for ( int i = 0; i < nPages ; i++ ) {
		if ( notebook->GetPageText(i) == searchString ) {
			return true;
		}
	}
	
	return false;
}


void CSearchDlg::CreateNewTab(wxString searchString, uint32 nSearchID)
{
	wxPanel* sizCont = new wxPanel(notebook, -1);
	searchPage(sizCont, true);
	notebook->AddPage(sizCont, searchString, true, 0, nSearchID);

	progressbar->SetRange( theApp.serverlist->GetServerCount() - 1 );

	GetParent()->Layout();
	
	CSearchListCtrl* searchlistctrl = (CSearchListCtrl*)FindWindowById(ID_SEARCHLISTCTRL, sizCont);
	searchlistctrl->Init(theApp.searchlist);
	searchlistctrl->ShowResults(nSearchID);
	
	FindWindowById(IDC_CLEARALL)->Enable(true);
}


void CSearchDlg::OnBnClickedCancels(wxCommandEvent& WXUNUSED(evt))
{
	canceld = true;

	if (globalsearch) {
		delete searchpacket;
		globalsearch = false;
	}

	m_timer.Stop();
	progressbar->SetValue(0);

	FindWindowById(IDC_CANCELS)->Disable();
	FindWindowById(IDC_STARTS)->Enable();
}


void CSearchDlg::LocalSearchEnd(uint16 WXUNUSED(count))
{
	if (!canceld) {
		if (!globalsearch) {
			FindWindowById(IDC_CANCELS)->Disable();
			FindWindowById(IDC_STARTS)->Enable();
		} else {
			m_timer.Start(750);
		}
	}
}


void CSearchDlg::OnBnClickedSdownload(wxCommandEvent& WXUNUSED(evt))
{
	if( notebook->GetSelection() == -1 ) {
		return;
	}
	
	wxNotebookPage* page = notebook->GetPage(notebook->GetSelection());
	CSearchListCtrl* searchlistctrl = (CSearchListCtrl*)FindWindowById(ID_SEARCHLISTCTRL, page);

	int index = searchlistctrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while ( index > -1 ) {
		theApp.downloadqueue->AddSearchToDownload( (CSearchFile*)searchlistctrl->GetItemData(index));
		FindWindowById(IDC_SDOWNLOAD)->Enable(FALSE);

		index = searchlistctrl->GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}
}


void CSearchDlg::OnBnClickedClearall(wxCommandEvent& WXUNUSED(ev))
{
	OnBnClickedCancels(nullEvent);
	DeleteAllSearchs();
	OnBnClickedSearchReset(nullEvent);
}


void CSearchDlg::StartNewSearch()
{
	static uint16 m_nSearchID = 0;
	m_nSearchID++;

	// No searching if not connected
	if (!theApp.serverconnect->IsConnected()) {
		wxMessageDialog* dlg = new wxMessageDialog(this, wxString(_("You are not connected to a server!")), wxString(_("Not Connected")), wxOK|wxCENTRE|wxICON_INFORMATION);
		dlg->ShowModal();
		delete dlg;
		return;
	}

	FindWindowById(IDC_STARTS)->Disable();
	FindWindowById(IDC_CANCELS)->Enable();

	canceld = false;
	
		
	byte stringParameter = 1;
	byte typeParameter = 2;
	byte numericParameter = 3;
	uint16 andParameter = 0x0000;	
	uint32 typeNemonic = 0x00030001;
	uint32 minNemonic = 0x02000101;
	uint32 maxNemonic = 0x02000102;
	uint32 avaibilityNemonic = 0x15000101;
	uint32 extensionNemonic = 0x00040001;
				
			
	wxString searchString = ((wxTextCtrl*)FindWindowById(IDC_SEARCHNAME))->GetValue();
	if ( searchString.IsEmpty() ) {
		return;
	}
	
	wxString extension = ((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHEXTENSION))->GetValue();
	if ( !extension.IsEmpty() && !extension.StartsWith(wxT(".")) ) {
		extension = wxT(".") + extension;
	}		
		
	wxString typeText = ((wxChoice*)FindWindowById(IDC_TypeSearch))->GetStringSelection();
	theApp.searchlist->NewSearch(typeText, m_nSearchID);

	// Parameter Minimum Size
	wxString sizeMin=((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHMIN))->GetValue();
	uint32 min = atol(unicode2char(sizeMin)) * 1048576;
	
	// Parameter Maximum Size
	wxString sizeMax=((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHMAX))->GetValue();
	uint32 max = atol(unicode2char(sizeMax)) * 1048576;
	
	if ( max < min ) max = 0;
	
	// Parameter Availability
	wxString avaibilitystr = ((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHAVAIBILITY))->GetValue();
	uint32 avaibility = atol(unicode2char(avaibilitystr));
	
	
	switch ( ((wxChoice*)FindWindowById(IDC_TypeSearch))->GetSelection() ) {
		case 0: typeText = wxT("Any"); break;
		case 1: typeText = wxT("Archives"); break;
		case 2: typeText = wxT("Audio"); break;
		case 3: typeText = wxT("CD-Images"); break;
		case 4: typeText = wxT("Pictures"); break;
		case 5: typeText = wxT("Programs"); break;
		case 6: typeText = wxT("Videos"); break;
		default:
			printf("Warning! Unknown search-category ( %s ) selected!\n", unicode2char(typeText));
			break;
	}

	
	// Count the number of used parameters
	int parametercount = 0;
	if ( !searchString.IsEmpty() )	parametercount++;
	if ( !typeText.IsEmpty() ) 		parametercount++;
	if ( min > 0 )					parametercount++;
	if ( max > 0 ) 					parametercount++;
	if ( avaibility > 0 ) 			parametercount++;
	if ( !extension.IsEmpty() )		parametercount++;

	// Must write parametercount - 1 parameter headers
	CMemFile* data = new CMemFile(100);
	for ( int i = 0; i < parametercount - 1; i++ ) {

		data->Write(andParameter);
	}
	
	
	// Packet body:
	if ( !searchString.IsEmpty() ) {
		data->Write( stringParameter ); 	// Search-String is a string parameter type
		data->Write( searchString ); 		// Write the value of the string
	}
	
	if ( !typeText.IsEmpty() ) {
		data->Write( typeParameter );		// Search-Type is a type parameter type
		data->Write( typeText ); 				// Write the parameter
		data->WriteRaw(&typeNemonic, 3); 	// Nemonic for this kind of parameter (only 3 bytes!!)
	}
	
	if ( min > 0 ) {
		data->Write( numericParameter );	// Write the parameter type
		data->Write( min );					// Write the parameter
		data->Write( minNemonic );			// Nemonic for this kind of parameter
	}
	
	if ( max > 0 ) {
		data->Write( numericParameter );	// Write the parameter type
		data->Write( max );					// Write the parameter
		data->Write( maxNemonic );			// Nemonic for this kind of parameter
	}
	
	if ( avaibility > 0 ) {
		data->Write( numericParameter );	// Write the parameter type
		data->Write( avaibility );			// Write the parameter
		data->Write( avaibilityNemonic );	// Nemonic for this kind of parameter
	}
	
	if ( !extension.IsEmpty() ) {
		data->Write( stringParameter );		// Write the parameter type
		data->Write( extension );			// Write the parameter
		data->WriteRaw(&extensionNemonic, 3); // Nemonic for this kind of parameter (only 3 bytes!!)
	}
	
	Packet* packet = new Packet(data);
	packet->SetOpCode(OP_SEARCHREQUEST);
	delete data;
		
	globalsearch = ((wxCheckBox*)FindWindowById(IDC_SGLOBAL))->IsChecked();
	
	theApp.uploadqueue->AddUpDataOverheadServer(packet->GetPacketSize());
	theApp.serverconnect->SendPacket( packet, !globalsearch );
	if ( globalsearch ) {
		askedlist.clear();
		
		// Add the current server to the askedlist, so that we dont send a UDP packet to it
		CServer* current = theApp.serverconnect->GetCurrentServer();
		         current = theApp.serverlist->GetServerByIP( current->GetIP(), current->GetPort() );
		askedlist.insert( current );
				 
		searchpacket = packet;
		searchpacket->SetOpCode(OP_GLOBSEARCHREQ);
		globalsearch = true;
	}
	
	CreateNewTab(searchString, m_nSearchID);
}


// Only needed for CSearchListCtrl to be able to close us by passing only ID
void CSearchDlg::DeleteSearch(uint16 nSearchID)
{
	theApp.searchlist->RemoveResults(nSearchID);

	for ( int i = 0; i < notebook->GetPageCount(); i++ ) {
		wxWindow * page = (wxWindow *)notebook->GetPage( i );
		wxWindow * slctrl = FindWindowById(ID_SEARCHLISTCTRL, page);
		
		// Make sure we have a valid pointer
		if( slctrl ) {
			if( nSearchID == ((CSearchListCtrl*)slctrl)->GetSearchId() ) {
				// Force further actions on the searchlistctrl to fail
				((CSearchListCtrl *)slctrl)->InvalidateSearchId();
				
				notebook->DeletePage(i);
				break;
			}
		}
	}
}


void CSearchDlg::DeleteAllSearchs()
{
	theApp.searchlist->Clear();
	
	notebook->DeleteAllPages();

	FindWindowById(IDC_CLEARALL)->Enable(FALSE);
}


void CSearchDlg::OnBnClickedSearchReset(wxCommandEvent& WXUNUSED(evt))
{
	((wxTextCtrl*)FindWindowById(IDC_SEARCHNAME))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHMIN))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHMAX))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHEXTENSION))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_EDITSEARCHAVAIBILITY))->Clear();
	((wxTextCtrl*)FindWindowById(ID_ED2KLINKHANDLER))->Clear();
	((wxTextCtrl*)FindWindowById(IDC_SEARCHWEBNAME))->Clear();
	
	wxChoice* Stypebox = (wxChoice*)FindWindowById(IDC_TypeSearch);
	Stypebox->SetSelection(Stypebox->FindString(wxString(_("Any"))));
	
	FindWindowById(IDC_SEARCH_RESET)->Enable(FALSE);
}


/**
 * Sends the contents of the directdownload textctrl box to fast links handler
 **/
void CSearchDlg::DirectDownload(wxCommandEvent& WXUNUSED(event))
{
	theApp.amuledlg->StartFast( (wxTextCtrl*)FindWindowById(ID_ED2KLINKHANDLER) );
}


/**
 * Update category autoassign choice box according to existing categories.
 **/
void CSearchDlg::UpdateCatChoice()
{
	CMuleNotebook* catbook = (CMuleNotebook*)FindWindowById(ID_CATEGORIES);
	wxASSERT(catbook);

	wxChoice *c_cat = (wxChoice*)FindWindow(ID_AUTOCATASSIGN);
	c_cat->Clear();

	for ( int i = 0; i < catbook->GetPageCount(); i++ ) {
		c_cat->Append( catbook->GetPageText(i) );
	}
	
	c_cat->SetSelection(0); 
}


/**
 * Returns current selected category-assignment choice.
 **/
uint8 CSearchDlg::GetCatChoice()
{
	wxChoice *c_cat = (wxChoice*)FindWindowById(ID_AUTOCATASSIGN);
	return (uint8)c_cat->GetSelection();
}


/**
 * Toggles Links Handler on this page on/off depending wether
 * global FastED2KLinksHandler is turned on or off.
 **/
void CSearchDlg::ToggleLinksHandler()
{
	s_srcopts->Show(s_srced2klh, !theApp.glob_prefs->GetFED2KLH());
	Layout();
}


void CSearchDlg::OnRMButton(wxMouseEvent& evt)
{
	if( !notebook->GetPageCount() ) {
		return;
	}
	
	// Translate the global position to a position relative to the notebook
	wxPoint pt=evt.GetPosition();
	wxPoint newpt=((wxWindow*)evt.GetEventObject())->ClientToScreen(pt);
	newpt = ScreenToClient(newpt);

	// Only show the popup-menu if we are inside the notebook widget
	if ( notebook->GetRect().Inside( newpt ) ) {
		wxMenu* menu = new wxMenu(wxString(_("Close")));
		menu->Append(MP_CLOSE_TAB, wxString(_("Close tab")));
		menu->Append(MP_CLOSE_ALL_TABS, wxString(_("Close all tabs")));
		menu->Append(MP_CLOSE_OTHER_TABS, wxString(_("Close other tabs")));
	
		PopupMenu(menu, newpt);
	
		delete menu;
	} else {
		evt.Skip();
	}
}


void CSearchDlg::OnPopupClose(wxCommandEvent& WXUNUSED(evt))
{
	if ( notebook->GetPageCount() == 1 ) {
		// Ensure that the dialog is tidied up
		DeleteAllSearchs();
	} else {
		notebook->DeletePage( notebook->GetSelection() );
	}
}


void CSearchDlg::OnPopupCloseAll(wxCommandEvent& WXUNUSED(evt))
{
	DeleteAllSearchs();
}


void CSearchDlg::OnPopupCloseOthers(wxCommandEvent& WXUNUSED(evt))
{
	wxNotebookPage* current = notebook->GetPage( notebook->GetSelection() );
	
	int i = 0;
	while ( i < notebook->GetPageCount() ) {
		if ( current == notebook->GetPage( i ) ) {
			i++;
			continue;
		}
		
		notebook->DeletePage( i );
	}
}



void CSearchDlg::OnBtnWebSearch(wxCommandEvent& WXUNUSED(evt))
{
	wxTextCtrl* txtctrl = (wxTextCtrl*)FindWindow(IDC_SEARCHWEBNAME);
    theApp.LaunchUrl(theApp.GenWebSearchUrl(txtctrl->GetValue()));
}
