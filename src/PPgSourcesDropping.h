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

#ifndef PPGSOURCESDROPPING_H
#define PPGSOURCESDROPPING_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel

#include "resource.h"		// Needed for IDD_PPG_SOURCES_DROPPING

class CPreferences;

// Here comes the CPPgSourcesDropping dialog

class CPPgSourcesDropping : public wxPanel
{
  DECLARE_DYNAMIC_CLASS(CPPgSourcesDropping)

    CPPgSourcesDropping() {};

public:
	
	CPPgSourcesDropping(wxWindow* parent); // Constructor
	virtual ~CPPgSourcesDropping();  // Destructor

	// Dialog Data
	enum { IDD = IDD_PPG_SOURCES_DROPPING };
	
	// Set preferences method
	void SetPrefs(CPreferences* in_prefs) 
	{	
		app_prefs = in_prefs;
	}
	
	// Localize method
	void Localize(void);
	
	// Load Settings method
	void LoadSettings(void);
	
	// On Apply method
	void OnApply();
	
protected:
	wxPanel* _panel;
	CPreferences *app_prefs;

};

#endif // PPGSOURCESDROPPING_H
