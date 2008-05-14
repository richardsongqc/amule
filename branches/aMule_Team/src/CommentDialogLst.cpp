// This file is part of the aMule project.
//
// Copyright (c) 2003,
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

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/dialog.h>
#include <wx/settings.h>

#include "CommentDialogLst.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for GetRateString
#include "PartFile.h"		// Needed for CPartFile
#include "opcodes.h"		// Needed for SOURCESSLOTS
#include "updownclient.h"	// Needed for CUpDownClient
#include "CString.h"	// Needed for CString
#include "muuli_wdr.h"		// Needed for commentLstDlg

//IMPLEMENT_DYNAMIC(CCommentDialogLst, CDialog)
CCommentDialogLst::CCommentDialogLst(wxWindow*parent,CPartFile* file)
: wxDialog(parent,CCommentDialogLst::IDD,_("File Comments"),
wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	m_file = file;
	wxSizer* content=commentLstDlg(this,TRUE);
	content->Show(this,TRUE);
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWFRAME));
	Centre();
	pmyListCtrl = (wxListCtrl*)FindWindowById(IDC_LST);
	OnInitDialog();
}

CCommentDialogLst::~CCommentDialogLst()
{
}

BEGIN_EVENT_TABLE(CCommentDialogLst,wxDialog)
	EVT_BUTTON(IDCOK,CCommentDialogLst::OnBnClickedApply)
	EVT_BUTTON(IDCREF,CCommentDialogLst::OnBnClickedRefresh)
END_EVENT_TABLE()

void CCommentDialogLst::OnBnClickedApply(wxEvent& evt)
{
	EndModal(0);
}

void CCommentDialogLst::OnBnClickedRefresh(wxEvent& evt)
{
	CompleteList();
}

#define LVCFMT_LEFT wxLIST_FORMAT_LEFT

bool CCommentDialogLst::OnInitDialog()
{
	pmyListCtrl->InsertColumn(0, CString(_("Username:")), LVCFMT_LEFT, 130);
	pmyListCtrl->InsertColumn(1, CString(_("File Name")), LVCFMT_LEFT, 130);
	pmyListCtrl->InsertColumn(2, CString(_("Rating")), LVCFMT_LEFT, 80);
	pmyListCtrl->InsertColumn(3, CString(_("Comment :")), LVCFMT_LEFT, 340);
	CompleteList();
	return TRUE;
}

void CCommentDialogLst::CompleteList()
{
	POSITION pos1,pos2;
	CUpDownClient* cur_src;
	int count=0;
	pmyListCtrl->DeleteAllItems();
   
	for (int sl=0;sl<SOURCESSLOTS;sl++) if (!m_file->srclists[sl].IsEmpty()) {
		for (pos1 = m_file->srclists[sl].GetHeadPosition(); (pos2 = pos1) != NULL;) {
			m_file->srclists[sl].GetNext(pos1);
			cur_src = m_file->srclists[sl].GetAt(pos2);

			if (cur_src->GetFileComment().Length()>0 || cur_src->GetFileRate()>0) {
				pmyListCtrl->InsertItem(count, cur_src->GetUserName());
				pmyListCtrl->SetItem(count, 1, cur_src->GetClientFilename());
				pmyListCtrl->SetItem(count, 2, GetRateString(cur_src->GetFileRate()));
				pmyListCtrl->SetItem(count, 3, cur_src->GetFileComment());
				count++;
			}
		}
		wxString info="";
		if (count==0) {
			info=CString(_("No comments"));
		} else {
			info=(CastItoIShort(count))+" comment(s)";
		}
		FindWindowById(IDC_CMSTATUS)->SetLabel(info);
		m_file->UpdateFileRatingCommentAvail();
	}
}