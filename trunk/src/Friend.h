// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifndef FRIEND_H
#define FRIEND_H

#include <wx/string.h>		// Needed for wxString

#include "types.h"		// Needed for uint32

class CUpDownClient;
class CFile;

#define	FF_NAME		0x01

class CFriend {
public:
	CFriend(void);
	CFriend(CUpDownClient* client);
	CFriend( unsigned char t_m_abyUserhash[16], uint32 tm_dwLastSeen, uint32 tm_dwLastUsedIP, uint32 tm_nLastUsedPort, uint32 tm_dwLastChatted, wxString tm_strName, uint32 tm_dwHasHash);
	~CFriend(void);
	unsigned char	m_abyUserhash[16];
	uint32	m_dwLastSeen;
	uint32	m_dwLastUsedIP;
	uint32	m_nLastUsedPort;
	uint32	m_dwLastChatted;
	uint32	m_dwHasHash;
	wxString	m_strName;

	CUpDownClient* m_LinkedClient;

	static const char sm_abyNullHash[16];
	void	LoadFromFile(CFile* file);
	void	WriteToFile(CFile* file);
};

#endif // FRIEND_H
