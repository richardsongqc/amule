//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
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

#ifndef FRIENDLISTCTRL_H
#define FRIENDLISTCTRL_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "FriendListCtrl.h"
#endif

#include "MuleListCtrl.h"
#include "CMD4Hash.h"

class wxString;
class CUpDownClient;

class CDlgFriend {
public:
	CDlgFriend(const CMD4Hash& hash, const wxString& name, uint32 ip, uint16 port, bool IsLinked, bool HasFriendSlot);

	wxString m_name;
	uint32 m_ip;
	uint16 m_port;
	CMD4Hash	m_hash;
	bool islinked;
	bool hasfriendslot;
};

class CFriendListCtrl : public CMuleListCtrl
{
public:
	CFriendListCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize siz, int flags);
	~CFriendListCtrl();
	
	bool		IsAlreadyFriend( uint32 dwLastUsedIP, uint32 nLastUsedPort ); 
	void		LoadList();
	CDlgFriend*	FindFriend( const CMD4Hash& userhash, uint32 dwIP, uint16 nPort);	
	void 		AddFriend(CDlgFriend* toadd, bool send_to_core = true);
	void		AddFriend( CUpDownClient* toadd );
	void		AddFriend( const CMD4Hash& userhash, const wxString& name, uint32 lastUsedIP, uint32 lastUsedPort, bool IsLinked = false, bool HasFriendSlot = false, bool send_to_core = true);
	void		RemoveFriend(CDlgFriend* todel);
	void		RemoveFriend(CUpDownClient* todel);
	void		RefreshFriend(CDlgFriend* toupdate);
	
protected:
	DECLARE_EVENT_TABLE()

	void	OnNMRclick(wxMouseEvent& evt);
	void	OnPopupMenu(wxCommandEvent& evt);
		
private:
	void 	OnItemSelected(wxListEvent& evt);
	void	OnItemActivated(wxListEvent& evt);
	
};

#endif
