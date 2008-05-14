//this file is part of eMule
// added by quekky
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

#ifndef CATDIALOG_H
#define CATDIALOG_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dialog.h>		// Needed for wxDialog
#include <wx/bitmap.h>		// Needed for wxBitmap
#include <wx/choice.h>		// Needed for wxChoice
#include <wx/statbmp.h>		// Needed for wxStaticBitmap
#include "resource.h"		// Needed for IDD_CAT

#include "types.h"		// Needed for DWORD

class Category_Struct;

// CCatDialog dialog

class CCatDialog : public wxDialog
{
	DECLARE_DYNAMIC_CLASS(CCatDialog)
	CCatDialog() {};
public:
	CCatDialog(wxWindow* parent,int catindex); // standard constructor
	virtual ~CCatDialog();
	virtual bool OnInitDialog();
	enum { IDD = IDD_CAT }; // Dialog Data

protected:
	DECLARE_EVENT_TABLE()
	wxStaticBitmap* bitmapcolor;
private:
	void UpdateData();
	Category_Struct* m_myCat;
	DWORD newcolor;
	wxBitmap* m_bitmap;
	wxChoice* m_prio;
public:
	void OnBnClickedBrowse(wxEvent& evt);
	void OnBnClickedOk(wxEvent& evt);
	void OnBnClickColor(wxEvent& evt);
	void mkBitmap(wxBitmap& bitmap);
};

#endif // CATDIALOG_H