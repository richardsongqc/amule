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

#ifndef FILEDETAILDIALOG_H
#define FILEDETAILDIALOG_H

#include <wx/dialog.h>		// Needed for wxDialog
#include <wx/timer.h>		// Needed for wxTimer

#include "resource.h"		// Needed for IDD_FILEDETAILS

class CPartFile;

// CFileDetailDialog dialog

class CFileDetailDialog : public wxDialog //CDialog
{
  //DECLARE_DYNAMIC(CFileDetailDialog)

public:
	CFileDetailDialog(wxWindow* parent, CPartFile* file);   // standard constructor
	virtual ~CFileDetailDialog();
	//virtual bool OnInitDialog();
	void Localize();
// Dialog Data
	enum { IDD = IDD_FILEDETAILS };

protected:
	void OnTimer(wxTimerEvent& evt);
#if 0
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void OnTimer(unsigned int nIDEvent);
	void OnDestroy();
	DECLARE_MESSAGE_MAP()

	static int CALLBACK CompareListNameItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);	// Tarod [Juanjo]
#endif
	DECLARE_EVENT_TABLE()

private:
	void UpdateData();
	CPartFile* m_file;
	struct SourcenameItem {
		wxString	name;
		long		count;
	};
	//uint32 m_timer;
	wxTimer m_timer;
	void OnClosewnd(wxCommandEvent& evt);
	//afx_msg void OnBnClickedButtonrename();	// Tarod [Juanjo]
	//afx_msg void OnBnClickedButtonStrip();
	//afx_msg void TakeOver();
	void FillSourcenameList();

	void OnBnClickedButtonrename(wxEvent& evt);
	void OnBnClickedButtonStrip(wxEvent& evt);
	void OnBnClickedFileinfo(wxEvent& evt);
	void OnBnClickedShowComment(wxEvent& evt);//for Comment//
	void TakeOver(wxEvent& evt);
	void Rename(wxEvent& evt);


};

#endif // FILEDETAILDIALOG_H