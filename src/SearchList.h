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


#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include "types.h"		// Needed for uint8, uint16 and uint32
#include "KnownFile.h"		// Needed for CAbstractFile
#include "CTypedPtrList.h"

#include <map>
#include <vector>

class CSafeMemFile;
class CMD4Hash;

class CSearchFile : public CAbstractFile {
	friend class CPartFile;
public:
	//CSearchFile() {};
	CSearchFile(const CSafeMemFile* in_data, long nSearchID, uint32 nServerIP=0, uint16 nServerPort=0, LPCTSTR pszDirectory = NULL);
	CSearchFile(long nSearchID, const CMD4Hash& pucFileHash, uint32 uFileSize, LPCTSTR pszFileName, int iFileType, int iAvailability);
	CSearchFile(CSearchFile* copyfrom);
	//CSearchFile(CFile* in_data, uint32 nSearchID);
	~CSearchFile();

	uint32	GetIntTagValue(uint8 tagname);
	char*	GetStrTagValue(uint8 tagname);
	uint32	AddSources(uint32 count, uint32 count_complete);
	uint32	GetSourceCount();
	uint32	GetCompleteSourceCount();
	long	GetSearchID() 				{ return m_nSearchID; }
	uint32	GetClientID() const			{ return m_nClientID; }
	void	SetClientID(uint32 nClientID)		{ m_nClientID = nClientID; }
	uint16	GetClientPort() const			{ return m_nClientPort; }
	void	SetClientPort(uint16 nPort)		{ m_nClientPort = nPort; }
	uint32	GetClientServerIP() const		{ return m_nClientServerIP; }
	void	SetClientServerIP(uint32 uIP)   	{ m_nClientServerIP = uIP; }
	uint16	GetClientServerPort() const		{ return m_nClientServerPort; }
	void	SetClientServerPort(uint16 nPort)	{ m_nClientServerPort = nPort; }
	CSearchFile* GetListParent() const		{ return m_list_parent; }
	
	struct SClient {
		SClient() {
			m_nIP = m_nPort = m_nServerIP = m_nServerPort = 0;
		}
		SClient(uint32 nIP, unsigned int nPort, uint32 nServerIP, unsigned int nServerPort) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_nServerIP = nServerIP;
			m_nServerPort = nServerPort;
		}
		uint32 m_nIP;
		uint32 m_nServerIP;
		uint16 m_nPort;
		uint16 m_nServerPort;
	};


	struct SServer {
		SServer() {
			m_nIP = m_nPort = 0;
			m_uAvail = 0;
		}
		SServer(uint32 nIP, unsigned int nPort) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_uAvail = 0;
		}
		uint32 m_nIP;
		uint16 m_nPort;
		unsigned int m_uAvail;
	};
	void AddServer(const SServer& server) { m_aServers.push_back(server); }

	const std::vector<SServer>& GetServers() const { return m_aServers; }
	SServer& GetServer(int iServer) { return m_aServers[iServer]; }


private:
	uint8	clientip[4];
	uint16	clientport;
	long	m_nSearchID;
	std::vector<CTag*> taglist;

	uint32	m_nClientID;
	uint16	m_nClientPort;
	uint32	m_nClientServerIP;
	uint16	m_nClientServerPort;
	std::vector<SServer> m_aServers;
	bool		 m_list_bExpanded;
	uint16	 m_list_childcount;
	CSearchFile* m_list_parent;
	bool		 m_bPreviewPossible;
	LPSTR m_pszDirectory;
};

class CSearchList
{
friend class CSearchListCtrl;
public:
	CSearchList();
	~CSearchList();
	void	Clear();

	void	NewSearch(const wxString& resTypes, long nSearchID);
	uint16	ProcessSearchanswer(const char* packet, uint32 size, CUpDownClient* Sender = NULL);
	uint16	ProcessSearchanswer(const char* packet, uint32 size, uint32 nServerIP, uint16 nServerPort);
	uint16	ProcessSearchanswer(const char* in_packet, uint32 size, CUpDownClient* Sender, bool* pbMoreResultsAvailable, LPCTSTR pszDirectory);
	uint16	ProcessUDPSearchanswer(CSafeMemFile* packet, uint32 nServerIP, uint16 nServerPort);	
   // uint16	ProcessUDPSearchanswer(char* packet, uint32 size);

	uint16	GetResultCount();
	uint16	GetResultCount(long nSearchID);
	void	RemoveResults(long nSearchID);
	void	RemoveResults( CSearchFile* todel );
	void	ShowResults(long nSearchID);
	wxString GetWebList(const wxString& linePattern,int sortby,bool asc) const;
	void	AddFileToDownloadByHash(const CMD4Hash& hash, uint8 cat = 0);
	uint16	GetFoundFiles(long searchID);

private:
	bool AddToList(CSearchFile* toadd, bool bClientResponse = false);
	CTypedPtrList<CPtrList, CSearchFile*> list;
	std::map<uint32, uint16> foundFilesCount;

	wxString myHashList;
	wxString resultType;
	
	long	m_nCurrentSearch;
};

#endif // SEARCHLIST_H
