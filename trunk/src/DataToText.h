//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef DATATOTEXT_H
#define DATATOTEXT_H

class wxString;
class CUpDownClient;

// Returns the textual representation of a priority value
wxString PriorityToStr( int priority, bool isAuto );

// Returns the textual representation of download states
wxString DownloadStateToStr( int state, bool queueFull );

/**
 * @return Human-readable client software name.
 */
const wxString GetSoftName( unsigned int software_ident );


#ifndef EC_REMOTE

/**
 * Extract client info in textual form.
 *
 * Creates client detail strings (name, version, modname), and puts them into the
 * appropriate variables. Any of these variables may be NULL, in which case the
 * corresponding info won't be generated.
 *
 * @param pClient The CUpDownClient from which we'll generate the info.
 * @param clientName Extracted client name will go into this string.
 * @param clientVersion Properly formatted client version string.
 * @param clientModName The Mod name of the client, if any.
 *
 * @return OS_Info string of the client, if supported.
 *
 * @note The clientName returned may be different from what GetSoftName() returns,
 * namely for xmule clients.
 */
const wxString& GetClientDetails(const CUpDownClient *pClient, wxString *clientName, wxString *clientVersion, wxString *clientModName);

#endif /* !EC_REMOTE */

/**
 * Get "Source From" text, i.e. where we got the source from.
 *
 * @param source_from A ESourceFrom enum value.
 * @return Human-readable text for the ESourceFrom enum values.
 */
wxString OriginToText(unsigned int source_from);

#endif /* DATATOTEXT_H */
