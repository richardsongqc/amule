//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( http://www.amule.org )
// Copyright (c) 2003 Barry Dunne (http://www.emule-project.net)
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

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#ifndef __KAD_DEFINES_H__
#define __KAD_DEFINES_H__

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

#define SEARCHTOLERANCE				16777216
#define K							10
#define KBASE						4
#define KK							5
#define ALPHA_QUERY					3
#define LOG_BASE_EXPONENT			5
#define HELLO_TIMEOUT				20
#define SEARCH_JUMPSTART			10
#define SEARCH_LIFETIME				100
#define SEARCHFILE_LIFETIME			100
#define SEARCHKEYWORD_LIFETIME		100
#define SEARCHNODE_LIFETIME			100
#define SEARCHNODECOMP_LIFETIME		10
#define SEARCHSTOREFILE_LIFETIME	140
#define SEARCHSTOREKEYWORD_LIFETIME	140
#define SEARCHFINDBUDDY_LIFETIME	100
#define SEARCHFINDSOURCE_LIFETIME	45
#define SEARCHFILE_TOTAL			300
#define SEARCHKEYWORD_TOTAL			300
#define SEARCHSTOREFILE_TOTAL		10
#define SEARCHSTOREKEYWORD_TOTAL	10
#define SEARCHNODECOMP_TOTAL		10
#define SEARCHFINDBUDDY_TOTAL		10
#define SEARCHFINDSOURCE_TOTAL		20

} // End namespace

#endif // __KAD_DEFINES_H__
