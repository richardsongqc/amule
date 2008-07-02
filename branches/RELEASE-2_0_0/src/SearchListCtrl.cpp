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
#pragma implementation "SearchListCtrl.h"
#endif

#include "SearchListCtrl.h"	// Interface declarations
#include "MuleNotebook.h"	// Needed for CMuleNotebook
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "OtherFunctions.h"	// Needed for CastItoXBytes
#include "PartFile.h"		// Needed for CPartFile and CKnownFile
#include "SearchList.h"		// Needed for CSearchFile
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "OPCodes.h"		// Needed for MP_RESUME
#include "amule.h"			// Needed for theApp

#include <wx/menu.h>
#include <wx/settings.h>
#include <algorithm>


BEGIN_EVENT_TABLE(CSearchListCtrl, CMuleListCtrl)
	EVT_RIGHT_DOWN(CSearchListCtrl::OnRightClick)
	EVT_LIST_COL_CLICK( -1,       CSearchListCtrl::OnColumnLClick)
	EVT_LIST_COL_END_DRAG( -1,    CSearchListCtrl::OnColumnResize)

	EVT_MENU( MP_GETED2KLINK,     CSearchListCtrl::OnPopupGetUrl)
	EVT_MENU( MP_GETHTMLED2KLINK, CSearchListCtrl::OnPopupGetUrl)
	EVT_MENU( MP_FAKECHECK1,      CSearchListCtrl::OnPopupFakeCheck)
	EVT_MENU( MP_FAKECHECK2,      CSearchListCtrl::OnPopupFakeCheck)
	EVT_MENU( MP_RAZORSTATS,      CSearchListCtrl::OnRazorStatsCheck)
	EVT_MENU( MP_RESUME,          CSearchListCtrl::OnPopupDownload)

	EVT_LIST_ITEM_ACTIVATED( -1,  CSearchListCtrl::OnItemActivated)
END_EVENT_TABLE()

using namespace otherfunctions;

std::list<CSearchListCtrl*> CSearchListCtrl::s_lists;

enum SearchListColumns {
	ID_SEARCH_COL_NAME = 0,
	ID_SEARCH_COL_SIZE,
	ID_SEARCH_COL_SOURCES,
	ID_SEARCH_COL_TYPE,
	ID_SEARCH_COL_FILEID
};

CSearchListCtrl::CSearchListCtrl( wxWindow* parent, wxWindowID winid, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name )
	: CMuleListCtrl( parent, winid, pos, size, style, validator, name )
{
	// Setting the sorter function.
	SetSortFunc( SortProc );

	InsertColumn( ID_SEARCH_COL_NAME,    _("File Name"), wxLIST_FORMAT_LEFT, 500);
	InsertColumn( ID_SEARCH_COL_SIZE,    _("Size"),      wxLIST_FORMAT_LEFT, 100);
	InsertColumn( ID_SEARCH_COL_SOURCES, _("Sources"),   wxLIST_FORMAT_LEFT, 50);
	InsertColumn( ID_SEARCH_COL_TYPE,    _("Type"),      wxLIST_FORMAT_LEFT, 65);
	InsertColumn( ID_SEARCH_COL_FILEID,  _("FileID"),    wxLIST_FORMAT_LEFT, 280);

	m_nResultsID = 0;

	// Only load settings for first list, otherwise sync with current lists
	if ( s_lists.empty() ) {
		// Set the name to enable loading of settings
		SetTableName( wxT("Search") );
	
		LoadSettings();

		// Unset the name to avoid the settings getting saved every time a list is closed
		SetTableName( wxEmptyString );
	} else {
		// Sync this list with one of the others
		SyncLists( s_lists.front(), this );
	}

	// Add the list so that it will be synced with the other lists
	s_lists.push_back( this );
}


CSearchListCtrl::~CSearchListCtrl()
{
	std::list<CSearchListCtrl*>::iterator it = std::find( s_lists.begin(), s_lists.end(), this );

	if ( it != s_lists.end() )
		s_lists.erase( it );

	// We only save the settings if the last list was closed
	if ( s_lists.empty() ) {
		// In order to get the settings saved, we need to set the name
		SetTableName( wxT("Search") );
	}
}


void CSearchListCtrl::AddResult(CSearchFile* toshow)
{
	// Ensure that only the right results gets added
	if ( toshow->GetSearchID() != m_nResultsID )
		return;

	// Insert the item before the item found by the search
	uint32 newid = InsertItem( GetInsertPos( (long)toshow ), toshow->GetFileName() );

	SetItemData( newid, (long)toshow );

	// Filesize
	SetItem(newid, ID_SEARCH_COL_SIZE, CastItoXBytes( toshow->GetFileSize() ) );

	// Source count
	wxString temp = wxString::Format( wxT("%d (%d)"), toshow->GetSourceCount(), toshow->GetCompleteSourceCount() );
	SetItem( newid, ID_SEARCH_COL_SOURCES, temp );

	// File-type
	SetItem( newid, ID_SEARCH_COL_TYPE, GetFiletypeByName( toshow->GetFileName() ) );

	// File-hash
	SetItem(newid, ID_SEARCH_COL_FILEID, toshow->GetFileHash().Encode() );

	// Set the color of the item
	UpdateColor( newid );
}


void CSearchListCtrl::UpdateResult(CSearchFile* toupdate)
{
	long index = FindItem( -1, (long)toupdate );

	if ( index != -1 ) {
		// Remove the old item
		DeleteItem( index );

		// Re-add it to update the displayed information and have it properly positioned
		AddResult( toupdate );
	}
}


void CSearchListCtrl::UpdateColor( long index )
{
	wxListItem item;
	item.SetId( index );
	item.SetColumn( ID_SEARCH_COL_SIZE );
	item.SetMask( wxLIST_MASK_STATE|wxLIST_MASK_TEXT|wxLIST_MASK_IMAGE|wxLIST_MASK_DATA|wxLIST_MASK_WIDTH|wxLIST_MASK_FORMAT );

	if ( GetItem(item) ) {
		wxColour newcol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

		CSearchFile* file = (CSearchFile*)GetItemData(index);

		CKnownFile* sameFile = theApp.downloadqueue->GetFileByID(file->GetFileHash());
		if ( !sameFile ) {
			sameFile = theApp.knownfiles->FindKnownFileByID(file->GetFileHash());
		}

		int red		= newcol.Red();
		int green	= newcol.Green();
		int blue	= newcol.Blue();

		if ( sameFile ) {
			if ( sameFile->IsPartFile() ) {
				// File is already being downloaded. Marks as red.
				red = 255;
			} else {
				// File has already been downloaded. Mark as green.
				green = 200;
			}
		} else {
			// File is new, colour after number of files
			blue += file->GetSourceCount() * 5;
			if ( blue > 255 ) {
				blue = 255;
			}
		}

		// don't forget to set the item data back...
		wxListItem newitem;
		newitem.SetId( index );
		newitem.SetTextColour( wxColour( red, green, blue ) );
		newitem.SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
		SetItem( newitem );
	}
}


void CSearchListCtrl::ShowResults( long ResultsID )
{
	DeleteAllItems();
	m_nResultsID = ResultsID;

	if ( ResultsID ) {
		CSearchList::ResultMap::iterator it = theApp.searchlist->m_Results.find( m_nResultsID );

		if ( it != theApp.searchlist->m_Results.end() ) {
			CSearchList::SearchList& list = it->second;

			for ( unsigned int i = 0; i < list.size(); i++ ) {
				AddResult( list[i] );
			}
		}
	}
}


long CSearchListCtrl::GetSearchId()
{
		return m_nResultsID;
}


int CSearchListCtrl::SortProc( long item1, long item2, long sortData )
{
	CSearchFile* file1 = (CSearchFile*)item1;
	CSearchFile* file2 = (CSearchFile*)item2;

	// Modifies the result, 1 for ascending, -1 for decending
	int modifier = 1;
	bool alternate = false;

	wxASSERT( SORT_OFFSET_ASC < SORT_OFFSET_DEC );
	wxASSERT( SORT_OFFSET_DEC < SORT_OFFSET_ALT_ASC );
	wxASSERT( SORT_OFFSET_ALT_ASC < SORT_OFFSET_ALT_DEC );

	if ( sortData >= SORT_OFFSET_ALT_DEC ) {
		modifier = -1;
		alternate = true;
		sortData -= SORT_OFFSET_ALT_DEC;
	}

	if ( sortData >= SORT_OFFSET_ALT_ASC ) {
		alternate = true;
		sortData -= SORT_OFFSET_ALT_ASC;
	}

	if ( sortData >= SORT_OFFSET_DEC ) {
		modifier = -1;
		sortData -= SORT_OFFSET_DEC;
	}

	switch ( sortData ) {
		// Sort by filename
		case ID_SEARCH_COL_NAME:
			return modifier * file1->GetFileName().CmpNoCase( file2->GetFileName() );

		// Sort file-size
		case ID_SEARCH_COL_SIZE:
			return modifier * CmpAny( file1->GetFileSize(), file2->GetFileSize() );

		// Sort by sources
		case ID_SEARCH_COL_SOURCES: {
			
			int cmp = CmpAny( file1->GetSourceCount(), file2->GetSourceCount() );

			int cmp2 = CmpAny( file1->GetCompleteSourceCount(), file2->GetCompleteSourceCount() );

			if ( alternate ) {
				// Swap criterias
				int temp = cmp2;
				cmp2 = cmp;
				cmp = temp;
			}

			if ( cmp == 0 ) {
				cmp = cmp2;
			}

			return modifier * cmp;
		}
		
		// Sort by file-types
		case ID_SEARCH_COL_TYPE:
			return modifier *  GetFiletypeByName( file1->GetFileName() ).Cmp(GetFiletypeByName(file2->GetFileName()));

		// Sort by file-hash
		case ID_SEARCH_COL_FILEID:
			return modifier * file2->GetFileHash().Encode().Cmp( file1->GetFileHash().Encode() );

		default:
			return 0;
	}
}


void CSearchListCtrl::SyncLists( CSearchListCtrl* src, CSearchListCtrl* dst )
{
	wxASSERT( src );
	wxASSERT( dst );

	// Column widths
	for ( int i = 0; i < src->GetColumnCount(); i++ ) {
		// We do this check since just setting the width causes a redraw
		if ( dst->GetColumnWidth( i ) != src->GetColumnWidth( i ) ) {
			dst->SetColumnWidth( i, src->GetColumnWidth( i ) );
		}
	}

	// Sync sorting
	if ( src->GetSortColumn() != dst->GetSortColumn() ||
	     src->GetSortAsc() != dst->GetSortAsc() )
	{
		dst->SetSortColumn( src->GetSortColumn() );
		dst->SetSortAsc( src->GetSortAsc() );
		dst->SetSortAlt( src->GetSortAlt() );

		dst->SortList();
	}
}


void CSearchListCtrl::SyncOtherLists( CSearchListCtrl* src )
{
	std::list<CSearchListCtrl*>::iterator it;

	for ( it = s_lists.begin(); it != s_lists.end(); ++it ) {
		if ( (*it) != src ) {
			SyncLists( src, *it );
		}
	}
}


void CSearchListCtrl::OnRightClick(wxMouseEvent& event)
{

	CheckSelection(event);
	
	if ( GetSelectedItemCount() != 0 ) {

		// Create the popup-menu
		wxMenu* menu = new wxMenu( _("File") );
		menu->Append( MP_RESUME, _("Download"));
		menu->Append( MP_GETED2KLINK, _("Copy ED2k link to clipboard"));
		menu->Append( MP_GETHTMLED2KLINK, _("Copy ED2k link to clipboard (HTML)"));
		menu->AppendSeparator();
		menu->Append( MP_RAZORSTATS, _("Get Razorback 2's stats for this file"));
		menu->AppendSeparator();
		menu->Append( MP_FAKECHECK2, _("jugle.net Fake Check")); // deltahf -> fakecheck
		menu->Append( MP_FAKECHECK1, _("'Donkey Fakes' Fake Check"));
		menu->AppendSeparator();

		// These should only be enabled for single-selections
		bool enable = GetSelectedItemCount();
		menu->Enable( MP_GETED2KLINK, enable );
		menu->Enable( MP_GETHTMLED2KLINK, enable );
		menu->Enable( MP_FAKECHECK1, enable );
		menu->Enable( MP_FAKECHECK2, enable );
	

		PopupMenu( menu, event.GetPosition() );

		delete menu;
	} else {
		event.Skip();
	}
}


void CSearchListCtrl::OnColumnLClick( wxListEvent& event )
{
	// Let the real event handler do its work first
	CMuleListCtrl::OnColumnLClick( event );
	
	SyncOtherLists( this );
}


void CSearchListCtrl::OnColumnResize( wxListEvent& WXUNUSED(event) )
{
	SyncOtherLists( this );
}


void CSearchListCtrl::OnPopupGetUrl( wxCommandEvent& event )
{
	wxString URIs;	
	
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while( index != -1 ) {
		CSearchFile* file = (CSearchFile*)GetItemData( index );

		switch ( event.GetId() ) {
			case MP_GETED2KLINK:				URIs += theApp.CreateED2kLink( file ) + wxT("\n");					break;
			case MP_GETHTMLED2KLINK:			URIs += theApp.CreateHTMLED2kLink( file ) + wxT("\n");				break;
		}

		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	if ( !URIs.IsEmpty() ) {	
		theApp.CopyTextToClipboard( URIs.RemoveLast() );
	}
	
}

void CSearchListCtrl::OnPopupFakeCheck( wxCommandEvent& event )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( item == -1 ) {
		return;
	}

	CSearchFile* file = (CSearchFile*)GetItemData( item );
	wxString URL;

	// Add new cases as nescesarry
	switch ( event.GetId() ) {
		case MP_FAKECHECK1:
			URL = theApp.GenFakeCheckUrl( file );
			break;
		
		case MP_FAKECHECK2:
			URL = theApp.GenFakeCheckUrl2( file );
			break;
	}

	if ( !URL.IsEmpty() ) {
		theApp.amuledlg->LaunchUrl( URL );
	}
}

void CSearchListCtrl::OnRazorStatsCheck( wxCommandEvent& WXUNUSED(event) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( item == -1 ) {
		return;
	}

	CSearchFile* file = (CSearchFile*)GetItemData( item );
	theApp.amuledlg->LaunchUrl(wxT("http://stats.razorback2.com/ed2khistory?ed2k=") + file->GetFileHash().Encode());
}


void CSearchListCtrl::OnPopupDownload( wxCommandEvent& WXUNUSED(event) )
{
	wxCommandEvent nullEvt;
	theApp.amuledlg->searchwnd->OnBnClickedDownload(nullEvt);
}


void CSearchListCtrl::OnItemActivated( wxListEvent& WXUNUSED(event) )
{
	wxCommandEvent nullEvt;
	theApp.amuledlg->searchwnd->OnBnClickedDownload(nullEvt);
}

bool CSearchListCtrl::AltSortAllowed( int column )
{
	switch ( column ) {
		case ID_SEARCH_COL_SOURCES:
			return true;
		
		default:
			return false;
	}
}