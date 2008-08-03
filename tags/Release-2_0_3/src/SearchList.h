//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "SearchList.h"
#endif

#include "Types.h"		// Needed for uint8, uint16 and uint32
#include "KnownFile.h"		// Needed for CAbstractFile
#include <wx/thread.h>

#include <map>
#include <vector>

class CSafeMemFile;
class CMD4Hash;
class CServer;
class CSearchList;

class CGlobalSearchThread : public wxThread 
{	
public:
	CGlobalSearchThread( CPacket *packet );
	~CGlobalSearchThread();

private:
	CPacket* 	m_packet;

	virtual void* Entry();
};


class CSearchFile : public CAbstractFile
{
	friend class CPartFile;
public:
	CSearchFile(const CSafeMemFile& in_data, bool bOptUTF8, long nSearchID, uint32 nServerIP=0, uint16 nServerPort=0, const wxString& pszDirectory = wxEmptyString);
	
	virtual ~CSearchFile();

#ifdef CLIENT_GUI
	friend class CSearchListRem;
	CSearchFile(class CEC_SearchFile_Tag *);
	
	uint32	GetSourceCount() { return m_SourceCount; }
	uint32	GetCompleteSourceCount() { return m_CompleteSourceCount; }
	uint32  GetFileSize() { return m_nFileSize; }
#else

	uint32	GetIntTagValue(uint8 tagname);
	wxString	GetStrTagValue(uint8 tagname);
	void	AddSources(uint32 count, uint32 count_complete);
	
	uint32	GetSourceCount();
	uint32	GetCompleteSourceCount();
	uint32  GetFileSize();
#endif
	long	GetSearchID() 					{ return m_nSearchID; }
	uint32	GetClientID() const				{ return m_nClientID; }
	void	SetClientID(uint32 nClientID)	{ m_nClientID = nClientID; }
	uint16	GetClientPort() const			{ return m_nClientPort; }
	void	SetClientPort(uint16 nPort)		{ m_nClientPort = nPort; }
	
private:
	long		m_nSearchID;
	
#ifdef CLIENT_GUI
	uint32 m_SourceCount, m_CompleteSourceCount;
#else
	typedef		std::vector<CTag*> TagList;
	TagList		m_taglist;
#endif
	uint32		m_nClientID;
	uint16		m_nClientPort;
	wxString	m_Directory;
};


class CSearchList
{
friend class CSearchListCtrl;
typedef std::vector<CSearchFile*> SearchList;

public:
	CSearchList();
	~CSearchList();
	void	Clear();

	bool	StartNewSearch(long nSearchID, 
								bool global_search, 
								const wxString& searchString, 
								const wxString& typeText,
								const wxString& extension, 
								uint32 min, 
								uint32 max, 
								uint32 availability);
	void	ProcessSearchanswer(const char* in_packet, uint32 size, CUpDownClient* Sender, bool* pbMoreResultsAvailable, const wxString& pszDirectory);
	void	ProcessSearchanswer(const char* packet, uint32 size, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort);
	void	ProcessUDPSearchanswer(const CSafeMemFile& packet, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort);	

	void	RemoveResults(long nSearchID);

	const SearchList GetSearchResults(long nSearchID);
	
	void	AddFileToDownloadByHash(const CMD4Hash& hash, uint8 cat = 0);
	void	LocalSearchEnd();

	void StopGlobalSearch();
	
	void ClearThreadData(CGlobalSearchThread* thread = NULL) 
	{ 
		// The thread going down might not be the actual one.
		if (!thread || (thread == m_searchthread)) {
			m_searchthread = NULL; 
		}
		
		m_SearchInProgress = false;
		
	};
	
	bool IsGlobalSearch() { return m_searchthread != NULL; };

	bool SearchInProgress() { return m_SearchInProgress; }
private:

	CPacket *CreateSearchPacket(const wxString &searchString, const wxString& typeText,
				const wxString &extension, uint32 min, uint32 max, uint32 avaibility);

	CPacket* m_searchpacket;

	bool AddToList(CSearchFile* toadd, bool bClientResponse = false);

	typedef std::map<long, SearchList> ResultMap;
	
	ResultMap	m_Results;
	
	wxString	m_resultType;
	
	long		m_CurrentSearch;

	CGlobalSearchThread* m_searchthread;

	bool m_SearchInProgress;
};

#endif // SEARCHLIST_H