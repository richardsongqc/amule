//
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

#ifndef SERVERLISTCTRL_H
#define SERVERLISTCTRL_H

#include "MuleListCtrl.h"	// Needed for CMuleListCtrl


class CServer;
class CServerList;
class wxListEvent;
class wxCommandEvent;


/** 
 * The CServerListCtrl is used to display the list of servers which the user
 * can connect to and which we request sources from. It is a permanently sorted
 * list in that it always ensure that the items are sorted in the correct order.
 */
class CServerListCtrl : public CMuleListCtrl 
{
public:
	/**
	 * Constructor.
	 *
	 * @see CMuleListCtrl::CMuleListCtrl
	 */
	CServerListCtrl(
	            wxWindow *parent,
                wxWindowID winid = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                const wxString &name = wxT("mulelistctrl") );
	
	/**
	 * Destructor.
	 */
	virtual ~CServerListCtrl();


	/**
	 * Adds a server to the list.
	 *
	 * @param A pointer to the new server.
	 *
	 * Internally this function calls RefreshServer and ShowServerCount, with 
	 * the result that it is legal to add servers already in the list, though
	 * not recommended.
	 */
	void	AddServer( CServer* toadd );
	
	/**
	 * Removes a server from the list.
	 *
	 * @param server The server to be removed.
	 *
	 * Please note that this function ALSO removes the server from the actual
	 * server list, not just from the displayed one!
	 */
	void	RemoveServer( const CServer* server );
	
	/**
	 * Removes all servers with the specified state. 
	 *
	 * @param state All items with this state will be removed, default being all.
	 */
	void	RemoveAllServers( int state = wxLIST_STATE_DONTCARE );
	
	
	/**
	 * Updates the displayed information on a server.
	 *
	 * @param server The server to be updated.
	 *
	 * This function will not only update the displayed information, it will also 
	 * reposition the item should it be nescecarry to enforce the current sorting.
	 * Also note that this function does not require that the server actually is
	 * on the list already, since AddServer makes use of it, but this should 
	 * generally be avoided, since it will result in the server-count getting
	 * skewed until the next AddServer call.
	 */	
	void	RefreshServer( CServer* server );
	
	/**
	 * Sets the highlighting of the specified server.
	 *
	 * @param server The server to have its highlighting set.
	 * @param highlight The new highlighting state.
	 *
	 * Please note that only _one_ item is allowed to be highlighted at any
	 * one time, so calling this function while another item is already 
	 * highlighted will result in the old item not being highlighted any more.
	 */
	void	HighlightServer( const CServer* server, bool highlight );

	/**
	 * Marks the specified server as static or not.
	 *
	 * @param The server to be marked or unmarked as static.
	 * @param The new static state.
	 *
	 * Other than setting the static setting of the specified server, it
	 * also adds or removes the server from the static-list file.
	 */
	bool	SetStaticServer( CServer* server, bool isStatic );

	
	/**
	 * This function updates the server-count in the server-wnd.
	 */
	void	ShowServerCount();
	
private:

	/**
	 * Event-handler for handling item activation (connect).
	 */
	void	OnItemActivated( wxListEvent& event );
	
	/**
	 * Event-handler for displaying the popup-menu.
	 */
	void	OnItemRightClicked( wxListEvent& event );
	
	/**
	 * Event-handler for priority changes.
	 */
	void	OnPriorityChange( wxCommandEvent& event );
	
	/**
	 * Event-handler for static changes.
	 */
	void	OnStaticChange( wxCommandEvent& event );
	
	/**
	 * Event-handler for server connections.
	 */
	void	OnConnectToServer( wxCommandEvent& event );
	
	/**
	 * Event-handler for copying server-urls to the clipboard.
	 */
	void	OnGetED2kURL( wxCommandEvent& event );

	/**
	 * Event-handler for server removal.
	 */
	void	OnRemoveServers( wxCommandEvent& event );

	/**
	 * Sorter function.
	 *
	 * @see wxListCtrl::SortItems
	 */
	static int wxCALLBACK SortProc(long item1, long item2, long sortData);


	//! Used to keep track of the last high-lighted item.
	long	m_connected;
	
	
	DECLARE_EVENT_TABLE()
};

#endif
