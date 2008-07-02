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

#ifndef SEARCHDLG_H
#define SEARCHDLG_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include <wx/timer.h>		// Needed for wxTimer and wxTimerEvent

#include "types.h"		// Needed for uint16 and uint32


class CMuleNotebook;
class CSearchListCtrl;
class CMuleNotebookEvent;
class Packet;
class CServer;
class wxListEvent;
class wxNotebookEvent;
class wxSpinEvent;
class wxGauge;
class CSearchFile;


/**
 * This class represents the Search Dialog, which takes care of 
 * enabling the user to search and to display results in a readable
 * manner.
 */
class CSearchDlg : public wxPanel
{
public:
	/**
	 * Constructor.
	 *
	 * @param pParent The parent widget passed to the wxPanel constructor.
	 */
	CSearchDlg(wxWindow* pParent);

	/**
	 * Destructor.
	 */
	~CSearchDlg();


	/**
	 * Adds the provided result to the right result-list.
	 *
	 * Please note that there is no duplicates checking, so the files should
	 * indeed be a new result.
	 */
	void AddResult(CSearchFile* toadd);

	/**
	 * Updates a changed result.
	 * 
	 * @param A pointer to the updated CSearchFile.
	 *
	 * This function will update the source-count and color of the result, and
	 * if needed, it will also move the result so that the current sorting 
	 * is maintained.
	 */
	void UpdateResult(CSearchFile* toupdate);

	/**
	 * Checks if a result-page with the specified heading exists.
	 *
	 * @param searchString The heading to look for.
	 */
	bool		CheckTabNameExists(const wxString& searchString);

	/**
	 * Creates a new tab and displays the specified results.
	 *
	 * @param searchString This will be the heading of the new page.
	 * @param nSearchID The results with this searchId will be displayed.
	 */
	void		CreateNewTab(const wxString& searchString, long nSearchID);


	/**
	 * Call this function to signify that the local search is over.
	 */
	void		LocalSearchEnd();


	/**
	 * This function updates the category list according to existing categories.
	 */
	void		UpdateCatChoice();


	/**
	 * This function displays the the hit-count in the heading for the specified page.
	 *
	 * @param page The page to have its heading updated.
	 */
	void		UpdateHitCount(CSearchListCtrl* page);

	/**
	 * Helper function which resets the controls.
	 */
	void		ResetControls();

	
	// Event handlers
	void		OnBnClickedStart(wxCommandEvent& evt);
	void		OnBnClickedDownload(wxCommandEvent& ev);
	void		OnBnClickedCancel(wxCommandEvent& evt);


	CSearchListCtrl* GetSearchList( long id );

	wxGauge*	m_progressbar;

private:
	// Event handlers
	void		OnFieldChanged(wxEvent& evt);
	
	void		OnListItemSelected(wxListEvent& ev);
	void		OnBnClickedReset(wxCommandEvent& ev);
	void		OnBnClickedClear(wxCommandEvent& ev);
	void		OnExtendedSearchChange(wxCommandEvent& ev);

	void		StartNewSearch();
	void		OnSearchClosed(wxNotebookEvent& evt);

	/**
	 * Event-handler for page-chages which takes care of enabling/disabling the download button.
	 */
	void		OnSearchPageChanged(wxNotebookEvent& evt);

	bool		m_canceld;
	bool		m_globalsearch;

	uint32		m_last_search_time;
	
	CMuleNotebook*	m_notebook;

	DECLARE_EVENT_TABLE()
};

#endif
