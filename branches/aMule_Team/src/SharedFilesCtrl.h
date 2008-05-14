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


#ifndef SHAREDFILESCTRL_H
#define SHAREDFILESCTRL_H

#include <wx/defs.h>		// Needed before any other wx/*.h.
#include <wx/menu.h>		// Needed for wxMenu

#include "types.h"		// Needed for uint32
#include "MuleListCtrl.h"	// Needed for CMuleListCtrl

class CSharedFileList;
class CKnownFile;

// CSharedFilesCtrl
class CSharedFilesCtrl : public CMuleListCtrl {
public:
	CSharedFilesCtrl();
	CSharedFilesCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);
	virtual ~CSharedFilesCtrl();
	void	Init();
	void	InitSort();
	void	ShowFileList(CSharedFileList* in_sflist);
	void	ShowFile(CKnownFile* file);
	void	ShowFile(CKnownFile* file,uint32 itemnr);
	void	RemoveFile(CKnownFile* toremove);
	void	UpdateFile(CKnownFile* file,uint32 pos);
	void	UpdateItem(CKnownFile* toupdate);
	void	Localize();
	void	ShowFilesCount();
private:
	virtual void	OnDrawItem(int item,wxDC* dc,const wxRect& rect,const wxRect& rectHL,bool highlighted);
	static int wxCALLBACK SortProc(long lParam1, long lParam2, long lParamSort);
	void		OnColumnClick(wxListEvent& evt);
	void		OnNMRclick(wxListEvent& evt);
	virtual bool ProcessEvent(wxEvent& evt);
	CPreferences::Table TablePrefs()	{ return CPreferences::tableShared; }

	wxMenu* m_SharedFilesMenu;
	wxMenu		   m_PrioMenu;
	wxMenu		   m_PermMenu;
	CSharedFileList* sflist;
	bool		   sortstat[3];

	DECLARE_EVENT_TABLE()
};

#endif // SHAREDFILESCTRL_H