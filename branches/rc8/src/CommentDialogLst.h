// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

#ifndef COMMENTDIALOGLST_H
#define COMMENTDIALOGLST_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dialog.h>		// Needed for wxDialog

class CPartFile;
class wxListCtrl;
class wxCommandEvent;

// CCommentDialogLst dialog 

class CCommentDialogLst : public wxDialog
{ 
  //DECLARE_DYNAMIC(CCommentDialogLst) 

public: 
   CCommentDialogLst(wxWindow* pParent, CPartFile* file); 
   virtual ~CCommentDialogLst(); 
   void Localize(); 
   virtual bool OnInitDialog(); 
   //CHyperTextCtrl lstbox; 
   wxListCtrl* pmyListCtrl;

protected: 
   //virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support 
   //DECLARE_MESSAGE_MAP() 
   DECLARE_EVENT_TABLE()
public: 
   void OnBnClickedApply(wxCommandEvent& evt); 
   void OnBnClickedRefresh(wxCommandEvent& evt); 
private: 
   void CompleteList(); 
   CPartFile* m_file; 
};

#endif // COMMENTDIALOGLST_H