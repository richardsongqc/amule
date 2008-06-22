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

#ifndef UPLOADQUEUE_H
#define UPLOADQUEUE_H

#include <cstring>		// Needed for std::memset
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/timer.h>		// Needed for wxTimer

#include "types.h"		// Needed for uint16, uint32 and uint64
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "position.h"		// Needed for POSITION

class CPreferences;
class CUpDownClient;

// Helper class holding a copy of a filehash
// The CCKey class is similiar but not useable here.
class CFileHash {
public:
	CFileHash()			{ std::memset(m_hash, 0, 16); }
	CFileHash(const unsigned char* k)	{ std::memcpy(m_hash, k, 16); }
	CFileHash(const CFileHash& s)	{ std::memcpy(m_hash, s.m_hash,16); }

	friend bool operator==(const CFileHash& h1, const CFileHash& h2)
			{ return !std::memcmp(h1.m_hash, h2.m_hash,16); }

	operator const unsigned char*() const	{ return m_hash; }

private:
	unsigned char	m_hash[16];
};

class CUploadQueue{
public:
	CUploadQueue(CPreferences* in_prefs);
	~CUploadQueue();
	void	Process();
	void	AddClientToQueue(CUpDownClient* client,bool bIgnoreTimelimit = false);
	bool	RemoveFromUploadQueue(CUpDownClient* client,bool updatewindow = true);
	bool	RemoveFromWaitingQueue(CUpDownClient* client,bool updatewindow = true);
	bool	IsOnUploadQueue(CUpDownClient* client)	{return GetWaitingClient(client);}
	bool	IsDownloading(CUpDownClient* client)	{return GetDownloadingClient(client);}
	float	GetKBps()								{return kBpsUp;}
	bool	CheckForTimeOver(CUpDownClient* client);
	int		GetWaitingUserCount()					{return waitinglist.GetCount();}
	int		GetUploadQueueLength()					{return uploadinglist.GetCount();}
        POSITION GetFirstFromUploadList()                               {return
uploadinglist.GetHeadPosition();}
        CUpDownClient* GetNextFromUploadList(POSITION &curpos)  {return uploadinglist.GetNext(curpos);}
        CUpDownClient* GetQueueClientAt(POSITION &curpos)       {return uploadinglist.GetAt(curpos);}
 
        POSITION GetFirstFromWaitingList()                              {return
waitinglist.GetHeadPosition();}
        CUpDownClient* GetNextFromWaitingList(POSITION &curpos) {return waitinglist.GetNext(curpos);}
        CUpDownClient* GetWaitClientAt(POSITION &curpos)        {return waitinglist.GetAt(curpos);}
 

	void	UpdateBanCount();
	CUpDownClient*	GetWaitingClientByIP(uint32 dwIP);
	CUpDownClient*	GetNextClient(CUpDownClient* update);

	
	void	DeleteAll();
	uint16	GetWaitingPosition(CUpDownClient* client);
	void	SetBanCount(uint32 in_count)			{bannedcount = in_count;}
	uint32	GetBanCount()							{return bannedcount;}
	uint32	GetSuccessfullUpCount()					{return successfullupcount;}
	uint32	GetFailedUpCount()						{return failedupcount;}
	uint32	GetAverageUpTime();
	void	FindSourcesForFileById(CTypedPtrList<CPtrList, CUpDownClient*>* srclist, unsigned char* filehash);
	void	AddUpDataOverheadSourceExchange(uint32 data)	{ m_nUpDataRateMSOverhead += data;
															  m_nUpDataOverheadSourceExchange += data;
															  m_nUpDataOverheadSourceExchangePackets++;}
	void	AddUpDataOverheadFileRequest(uint32 data)		{ m_nUpDataRateMSOverhead += data;
															  m_nUpDataOverheadFileRequest += data;
															  m_nUpDataOverheadFileRequestPackets++;}
	void	AddUpDataOverheadServer(uint32 data)			{ m_nUpDataRateMSOverhead += data;
															  m_nUpDataOverheadServer += data;
															  m_nUpDataOverheadServerPackets++;}
	void	AddUpDataOverheadOther(uint32 data)				{ m_nUpDataRateMSOverhead += data;
															  m_nUpDataOverheadOther += data;
															  m_nUpDataOverheadOtherPackets++;}
	uint32	GetUpDatarateOverhead()						{return m_nUpDatarateOverhead;}
	uint64	GetUpDataOverheadSourceExchange()			{return m_nUpDataOverheadSourceExchange;}
	uint64	GetUpDataOverheadFileRequest()				{return m_nUpDataOverheadFileRequest;}
	uint64	GetUpDataOverheadServer()					{return m_nUpDataOverheadServer;}
	uint64	GetUpDataOverheadOther()					{return m_nUpDataOverheadOther;}
	uint64	GetUpDataOverheadSourceExchangePackets()	{return m_nUpDataOverheadSourceExchangePackets;}
	uint64	GetUpDataOverheadFileRequestPackets()		{return m_nUpDataOverheadFileRequestPackets;}
	uint64	GetUpDataOverheadServerPackets()			{return m_nUpDataOverheadServerPackets;}
	uint64	GetUpDataOverheadOtherPackets()				{return m_nUpDataOverheadOtherPackets;}
	void	CompUpDatarateOverhead();
	void	SuspendUpload( const CFileHash& );
	void	ResumeUpload( const CFileHash& );
protected:
	void	RemoveFromWaitingQueue(POSITION pos, bool updatewindow);
	POSITION	GetWaitingClient(CUpDownClient* client);
//	POSITION	GetWaitingClientByID(CUpDownClient* client);
	POSITION	GetDownloadingClient(CUpDownClient* client);
	bool		AcceptNewClient();
	void		AddUpNextClient(CUpDownClient* directadd = 0);
private:
	CTypedPtrList<CPtrList, CUpDownClient*> waitinglist;
	CTypedPtrList<CPtrList, CUpDownClient*> uploadinglist;
	CList<CFileHash> suspended_uploads_list;  //list for suspended uploads
	uint32	msPrevProcess;
	float	kBpsUp;
	float	kBpsEst;
	CPreferences* app_prefs;
	wxTimer* h_timer;
	uint32	bannedcount;
	uint32	successfullupcount;
	uint32	failedupcount;
	uint32	totaluploadtime;
	uint32	m_nLastStartUpload;
	uint32	m_nUpDatarateOverhead;
	uint32	m_nUpDataRateMSOverhead;
	uint64	m_nUpDataOverheadSourceExchange;
	uint64	m_nUpDataOverheadFileRequest;
	uint64	m_nUpDataOverheadServer;
	uint64	m_nUpDataOverheadOther;
	uint64	m_nUpDataOverheadSourceExchangePackets;
	uint64	m_nUpDataOverheadFileRequestPackets;
	uint64	m_nUpDataOverheadServerPackets;
	uint64	m_nUpDataOverheadOtherPackets;
	bool	lastupslotHighID; // VQB lowID alternation
	CList<int, int>	m_AvarageUDRO_list;

};

#endif // UPLOADQUEUE_H