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


#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wx/defs.h>		// Needed before any other wx/*.h
#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <utime.h>		// Needed for utime()
#include <wx/intl.h>		// Needed for _
#include <wx/setup.h>
#include <wx/gdicmn.h>
#include <wx/filename.h>	// Needed for wxFileName
#include <wx/file.h>		// Needed for wxFile
#include <wx/msgdlg.h>		// Needed for wxMessageBox

#include "PartFile.h"		// Interface declarations.
#include "otherfunctions.h"	// Needed for nstrdup
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "SysTray.h"		// Needed for TBN_DLOAD
#include "UploadQueue.h"	// Needed for CFileHash
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "IPFilter.h"		// Needed for CIPFilter
#include "server.h"		// Needed for CServer
#include "sockets.h"		// Needed for CServerConnect
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "updownclient.h"	// Needed for CUpDownClient
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "AddFileThread.h"	// Needed for CAddFileThread
#include "SafeFile.h"		// Needed for CSafeFile
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"		// Needed for theApp
#include "ED2KLink.h"		// Needed for CED2KLink
#include "packets.h"		// Needed for CTag
#include "SearchList.h"		// Needed for CSearchFile
#include "BarShader.h"		// Needed for CBarShader
#include "mfc.h"			// itoa

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(ListOfChunks);

#define PROGRESS_HEIGHT 3

void dump16f(FILE*f, uchar* d16)
{
	int i;
	for(i=0;i<16;i++) {
		fprintf(f,"%02X",*d16++);
	}
}

void dump16(uchar* d16)
{
	int i;
	for(i=0;i<16;i++) {
		printf("%02X",*d16++);
	}
}

extern void dump16(uchar*);

wxMutex CPartFile::m_FileCompleteMutex; 

CBarShader CPartFile::s_LoadBar(PROGRESS_HEIGHT); 
CBarShader CPartFile::s_ChunkBar(16); 

CPartFile::CPartFile()
{
	Init();
}

CPartFile::CPartFile(CSearchFile* searchresult)
{
	Init();
	md4cpy(m_abyFileHash, searchresult->GetFileHash());
	for (int i = 0; i < searchresult->taglist.GetCount();i++){
		const CTag* pTag = searchresult->taglist[i];
		switch (pTag->tag.specialtag){
			case FT_FILENAME:{
				if (pTag->tag.type == 2)
					SetFileName(pTag->tag.stringvalue);
				break;
			}
			case FT_FILESIZE:{
				if (pTag->tag.type == 3) {
					SetFileSize(pTag->tag.intvalue);
				}
				break;
			}
			default:{
				bool bTagAdded = false;
				if (pTag->tag.specialtag == 0 && pTag->tag.tagname != NULL && (pTag->tag.type == 2 || pTag->tag.type == 3))
				{
					static const struct
					{
						LPCSTR	pszName;
						uint8	nType;
					} _aMetaTags[] = 
					{
						{ FT_MEDIA_ARTIST,  2 },
						{ FT_MEDIA_ALBUM,   2 },
						{ FT_MEDIA_TITLE,   2 },
						{ FT_MEDIA_LENGTH,  2 },
						{ FT_MEDIA_BITRATE, 3 },
						{ FT_MEDIA_CODEC,   2 }
					};
					for (int t = 0; t < ARRSIZE(_aMetaTags); t++)
					{
						if (pTag->tag.type == _aMetaTags[t].nType && !stricmp(pTag->tag.tagname, _aMetaTags[t].pszName))
						{
							// skip string tags with empty string values
							if (pTag->tag.type == 2 && (pTag->tag.stringvalue == NULL || pTag->tag.stringvalue[0] == '\0'))
								break;

							// skip "length" tags with "0: 0" values
							if (!stricmp(pTag->tag.tagname, FT_MEDIA_LENGTH) && (!strcmp(pTag->tag.stringvalue, "0: 0") || !strcmp(pTag->tag.stringvalue, "0:0")))
								break;

							// skip "bitrate" tags with '0' values
							if (!stricmp(pTag->tag.tagname, FT_MEDIA_BITRATE) && pTag->tag.intvalue == 0)
								break;

							printf("CPartFile::CPartFile(CSearchFile*): added tag %s\n", pTag->GetFullInfo().c_str());
							CTag* newtag = new CTag(pTag->tag);
							taglist.Add(newtag);
							bTagAdded = true;
							break;
						}
					}
				}
				else if (pTag->tag.specialtag != 0 && pTag->tag.tagname == NULL && (pTag->tag.type == 2 || pTag->tag.type == 3))
				{
					static const struct
					{
						uint8	nID;
						uint8	nType;
					} _aMetaTags[] = 
					{
						{ FT_FILETYPE,		2 },
						{ FT_FILEFORMAT,	2 }
					};
					for (int t = 0; t < ARRSIZE(_aMetaTags); t++)
					{
						if (pTag->tag.type == _aMetaTags[t].nType && pTag->tag.specialtag == _aMetaTags[t].nID)
						{
							// skip string tags with empty string values
							if (pTag->tag.type == 2 && (pTag->tag.stringvalue == NULL || pTag->tag.stringvalue[0] == '\0'))
								break;

							printf("CPartFile::CPartFile(CSearchFile*): added tag %s\n", pTag->GetFullInfo().c_str());
							CTag* newtag = new CTag(pTag->tag);
							taglist.Add(newtag);
							bTagAdded = true;
							break;
						}
					}
				}

				if (!bTagAdded)
					printf("CPartFile::CPartFile(CSearchFile*): ignored tag %s\n", pTag->GetFullInfo().c_str());
			}
		}
	}
	CreatePartFile();
}

CPartFile::CPartFile(CString edonkeylink)
{
	CED2KLink* pLink = 0;
	try {
		pLink = CED2KLink::CreateLinkFromUrl(edonkeylink);
		wxASSERT(pLink != 0);
		CED2KFileLink* pFileLink = pLink->GetFileLink();
		if (pFileLink==0) {
			throw wxString(wxT("Not a file link"));
		}
		InitializeFromLink(pFileLink);
	} catch (wxString error) {
		char buffer[200];
		sprintf(buffer, CString(_("This ed2k link is invalid (%s)")), error.GetData());
		theApp.amuledlg->AddLogLine(true, CString(_("Invalid link: %s")), buffer);
		SetPartFileStatus(PS_ERROR);
	}
	delete pLink;
}

void
CPartFile::InitializeFromLink(CED2KFileLink* fileLink)
{
	Init();
	try {
		m_strFileName = fileLink->GetName();
		SetFileSize(fileLink->GetSize());
		md4cpy(m_abyFileHash, fileLink->GetHashKey());
		if (!theApp.downloadqueue->IsFileExisting(m_abyFileHash)) {
			CreatePartFile();
		} else {
			SetPartFileStatus(PS_ERROR);
		}
	} catch(wxString error) {
		char buffer[200];
		sprintf(buffer,CString(_("This ed2k link is invalid (%s)")),error.GetData());
		theApp.amuledlg->AddLogLine(true, CString(_("Invalid link: %s")), buffer);
		SetPartFileStatus(PS_ERROR);
	}
}

CPartFile::CPartFile(CED2KFileLink* fileLink)
{
	InitializeFromLink(fileLink);
}

void CPartFile::Init()
{
	fullname = NULL; // new
	
	m_nLastBufferFlushTime = 0;

	newdate = true;
	lastsearchtime = 0;
	lastpurgetime = ::GetTickCount();
	paused = false;
	stopped = false;
	SetPartFileStatus(PS_EMPTY);
	insufficient = false;
	
	//m_bCompletionError = false; // added
	
	transfered = 0;
	m_iLastPausePurge = time(NULL);
	
	if(theApp.glob_prefs->GetNewAutoDown()) {
		m_iDownPriority = PR_HIGH;
		m_bAutoDownPriority = true;
	} else {
		m_iDownPriority = PR_NORMAL;
		m_bAutoDownPriority = false;
	}
	srcarevisible = false;
	
	memset(m_anStates,0,sizeof(m_anStates));
	
	transferingsrc = 0; // new
	
#ifdef DOWNLOADRATE_FILTERED
	kBpsDown = 0.0;
#else
	datarate = 0;
#endif
	
	hashsetneeded = true;
	count = 0;
	percentcompleted = 0;
	m_partmetfilename = 0;
	completedsize=0;
	m_bPreviewing = false;
	lastseencomplete = 0;
	availablePartsCount=0;
	m_ClientSrcAnswered = 0;
	m_LastNoNeededCheck = 0;
	m_iRate = 0;
	m_strComment = "";
	m_nTotalBufferData = 0;
	m_nLastBufferFlushTime = 0;
	m_bPercentUpdated = false;
	m_bRecoveringArchive = false;
	m_iGainDueToCompression = 0;
	m_iLostDueToCorruption = 0;
	m_iTotalPacketsSavedDueToICH = 0;
	m_nSavedReduceDownload = 0; // new
	hasRating = false;
	hasComment = false; 
	hasBadRating = false;
	m_lastdatetimecheck = 0;
	m_category = 0;
	m_lastRefreshedDLDisplay = 0;
	m_is_A4AF_auto = false;
	m_bShowOnlyDownloading = false;
	m_bLocalSrcReqQueued = false;
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 0;
	m_nCompleteSourcesCountLo = 0;
	m_nCompleteSourcesCountHi = 0;
	
	// Kry - added to cache the source count
	CleanCount = 0;
	IsCountDirty = true;
	
	// Sources dropping
	m_LastSourceDropTime = 0;
	
}

CPartFile::~CPartFile()
{
	
	// Barry - Ensure all buffered data is written

	// Kry - WTF? 
	// eMule had same problem with lseek error ... and override with a simple 
	// check for INVALID_HANDLE_VALUE (that, btw, does not exist on linux)
	// So we just guess is < 0 on error and > 2 if ok (0 stdin, 1 stdout, 2 stderr)
	// But, where does this wrong handle comes from?
	
	if (m_hpartfile.IsOpened() && (m_hpartfile.fd() > 2)) { 
			FlushBuffer();
	}
	
	if (m_hpartfile.IsOpened() && (m_hpartfile.fd() > 2)) {
		m_hpartfile.Close();
		// Update met file (with current directory entry)
		SavePartFile();			
	}

	if (fullname) {
		delete[] fullname;
	}
	if (m_partmetfilename) {
		delete[] m_partmetfilename;
	}
	
	m_SrcpartFrequency.Clear();
	
	POSITION pos;
	for (pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)) {
		delete gaplist.GetAt(pos);
	}
	pos = m_BufferedData_list.GetHeadPosition();
	while (pos){
		PartFileBufferedData *item = m_BufferedData_list.GetNext(pos);
		delete[] item->data;
		delete item;
	}	
}

void CPartFile::CreatePartFile()
{
	// use lowest free partfilenumber for free file (InterCeptor)
	int i = 0; 
	CString filename; 	
	do { 
		i++; 
		filename.Format("%s/%03i.part", theApp.glob_prefs->GetTempDir(), i); 
	} while (wxFileName::FileExists(filename));
	m_partmetfilename = new char[15]; 
	sprintf(m_partmetfilename,"%03i.part.met",i);
	fullname = new char[strlen(theApp.glob_prefs->GetTempDir())+strlen(m_partmetfilename)+MAX_PATH];
	sprintf(fullname,"%s/%s",theApp.glob_prefs->GetTempDir(),m_partmetfilename);
	char* buffer = nstrdup(m_partmetfilename);
	buffer[strlen(buffer)-4] = 0;
	
	CTag* partnametag = new CTag(FT_PARTFILENAME,buffer);
	delete[] buffer;
	taglist.Add(partnametag);
	
	Gap_Struct* gap = new Gap_Struct;
	gap->start = 0;
	gap->end = m_nFileSize-1;
	gaplist.AddTail(gap);
	
	dateC = time(NULL);

	char* partfull = nstrdup(fullname);
	partfull[strlen(partfull)-4] = 0;
	
	
	if (!m_hpartfile.Create(partfull,TRUE)) {
		theApp.amuledlg->AddLogLine(false,CString(_("ERROR: Failed to create partfile)")));
		SetPartFileStatus(PS_ERROR);
	}
	// jesh.. luotu. nyt se vaan pitää avata uudestaan read-writeen..
	m_hpartfile.Close();
	if(!m_hpartfile.Open(partfull,CFile::read_write)) {
		theApp.amuledlg->AddLogLine(false,CString(_("ERROR: Failed to create partfile)")));
		SetPartFileStatus(PS_ERROR);
	}
	
	if (theApp.glob_prefs->GetAllocFullPart()) {
		#warning Code for full file alloc - should be done on thread.
	}
	
	delete[] partfull;
	
	if (GetED2KPartHashCount() == 0)
		hashsetneeded = false;
	
	m_SrcpartFrequency.Clear();
	m_SrcpartFrequency.Alloc(GetPartCount());
	
	m_SrcpartFrequency.Insert(/*Item*/0, /*pos*/0, GetPartCount());
	
	paused = false;
	SavePartFile(true);
}

uint8 CPartFile::LoadPartFile(LPCTSTR in_directory, LPCTSTR filename, bool getsizeonly)
{
	#warning getsizeonly is ignored because we do not import yet
	
	bool isnewstyle;
	uint8 version,partmettype=PMT_UNKNOWN;
	
	CMap<uint16, uint16, Gap_Struct*, Gap_Struct*> gap_map; // Slugfiller
	transfered = 0;
	
	m_partmetfilename = nstrdup(filename);
	directory = nstrdup(in_directory);
	char* buffer = new char[strlen(directory)+strlen(m_partmetfilename)+2];
	sprintf(buffer,"%s/%s",directory,m_partmetfilename);
	fullname = buffer;
	
	// read file creation time
	struct stat fileinfo;
	if (stat(fullname, &fileinfo) == 0) {
		dateC = fileinfo.st_ctime;	
	}
	
	CSafeFile metFile;
	bool load_from_backup = false;
	// readfile data form part.met file
	if (!metFile.Open(fullname,CFile::read)) {
		theApp.amuledlg->AddLogLine(false, CString(_("Error: Failed to open part.met file! (%s => %s)")), m_partmetfilename, m_strFileName.GetData());
		load_from_backup = true;
	} else {
		if (!(metFile.Length()>0)) {
			theApp.amuledlg->AddLogLine(false, CString(_("Error: part.met fileis 0 size! (%s => %s)")), m_partmetfilename, m_strFileName.GetData());
			metFile.Close();
			load_from_backup = true;
		}
	}

	if (load_from_backup) {
		theApp.amuledlg->AddLogLine(false, CString(_("Trying backup of met file on (%s%s)")), m_partmetfilename, PARTMET_BAK_EXT);
		wxString BackupFile;
		BackupFile.Printf("%s%s",fullname,PARTMET_BAK_EXT);
		if (!metFile.Open(BackupFile)) {
			theApp.amuledlg->AddLogLine(false, CString(_("Error: Failed to load backup file. Search http://forum.amule.org for .part.met recovery solutions")), m_partmetfilename, m_strFileName.GetData());				
			delete[] buffer;
			return false;
		} else {
			if (!(metFile.Length()>0)) {
				theApp.amuledlg->AddLogLine(false, CString(_("Error: part.met fileis 0 size! (%s => %s)")), m_partmetfilename, m_strFileName.GetData());
				metFile.Close();			
				delete[] buffer;
				return false;
			}
		}
	}
	
	try {
		metFile.Read(&version,1);
		if (version != PARTFILE_VERSION  && version!= PARTFILE_SPLITTEDVERSION ){
			metFile.Close();
			theApp.amuledlg->AddLogLine(false, CString(_("Error: Invalid part.met fileversion! (%s => %s)")), m_partmetfilename, m_strFileName.GetData());
			return false;
		}

		isnewstyle=(version== PARTFILE_SPLITTEDVERSION);
		partmettype= isnewstyle?PMT_SPLITTED:PMT_DEFAULTOLD;
		if (!isnewstyle) {
			uint8 test[4];
			metFile.Seek(24, wxFromStart);
			metFile.Read(&test[0],1);
			metFile.Read(&test[1],1);
			metFile.Read(&test[2],1);
			metFile.Read(&test[3],1);
		
			metFile.Seek(1, wxFromStart);
			if (test[0]==0 && test[1]==0 && test[2]==2 && test[3]==1) {
				isnewstyle=true;	// edonkeys so called "old part style"
				partmettype=PMT_NEWOLD;
			}
		}
		if (isnewstyle) {
			uint32 temp;
			metFile.Read(&temp,4);
			ENDIAN_SWAP_I_32(temp);
	
			if (temp==0) {	// 0.48 partmets - different again
				LoadHashsetFromFile(&metFile, false);
			} else {
				uchar gethash[16];
				metFile.Seek(2, wxFromStart);
				LoadDateFromFile(&metFile);
					metFile.Read(&gethash, 16);
				md4cpy(m_abyFileHash, gethash);
			}

		} else {
			LoadDateFromFile(&metFile);
			LoadHashsetFromFile(&metFile, false);
		}	

		uint32 tagcount = 0;	
		metFile.Read(&tagcount,4);
		ENDIAN_SWAP_I_32(tagcount);

		for (uint32 j = 0; j < tagcount;j++) {
			CTag* newtag = new CTag(&metFile);
			if (!getsizeonly || (getsizeonly && (newtag->tag.specialtag==FT_FILESIZE || newtag->tag.specialtag==FT_FILENAME))) {
				switch(newtag->tag.specialtag) {
					case FT_FILENAME: {
						if(newtag->tag.stringvalue == NULL) {
							theApp.amuledlg->AddLogLine(true, CString(_("Error: %s (%s) is corrupt")), m_partmetfilename, m_strFileName.GetData());
							delete newtag;
							return false;
						}
						printf(" - filename %s - ",newtag->tag.stringvalue);
						SetFileName(newtag->tag.stringvalue);
						delete newtag;
						break;
					}
					case FT_LASTSEENCOMPLETE: {
						if (newtag->tag.type == 3) {
						lastseencomplete = newtag->tag.intvalue;					
						}
						delete newtag;
						break;
					}
					case FT_FILESIZE: {
						SetFileSize(newtag->tag.intvalue);
						delete newtag;
						break;
					}
					case FT_TRANSFERED: {
						transfered = newtag->tag.intvalue;
					delete newtag;
						break;
					}
					case FT_FILETYPE:{
						#warning needs setfiletype string
						//SetFileType(newtag->tag.stringvalue);
						delete newtag;
						break;
					}					
					case FT_CATEGORY: {
						m_category = newtag->tag.intvalue;
						delete newtag;
						break;
					}
					case FT_OLDDLPRIORITY:
					case FT_DLPRIORITY: {
						if (!isnewstyle){
							m_iDownPriority = newtag->tag.intvalue;
							if( m_iDownPriority == PR_AUTO ){
								m_iDownPriority = PR_HIGH;
								SetAutoDownPriority(true);
							}
							else{
								if (m_iDownPriority != PR_LOW && m_iDownPriority != PR_NORMAL && m_iDownPriority != PR_HIGH)
									m_iDownPriority = PR_NORMAL;
								SetAutoDownPriority(false);
							}
						}
						delete newtag;
						break;
					}
					case FT_STATUS: {
						paused = newtag->tag.intvalue;
						stopped = paused;
						delete newtag;
						break;
						}
					case FT_OLDULPRIORITY:
					case FT_ULPRIORITY: {			
						if (!isnewstyle){
							SetUpPriority(newtag->tag.intvalue, false);
							if( GetUpPriority() == PR_AUTO ){
								SetUpPriority(PR_HIGH, false);
								SetAutoUpPriority(true);
							}
							else
								SetAutoUpPriority(false);
						}					
						delete newtag;
						break;
					}				
					case FT_KADLASTPUBLISHSRC:{
						//SetLastPublishTimeKadSrc(newtag->tag.intvalue);
						delete newtag;
						break;
					}
					// old tags: as long as they are not needed, take the chance to purge them
					case FT_PERMISSIONS:
						delete newtag;
						break;
					case FT_KADLASTPUBLISHKEY:
						delete newtag;
						break;					
					default: {
						// Start Changes by Slugfiller for better exception handling
						if ((!newtag->tag.specialtag) &&
						(newtag->tag.tagname[0] == FT_GAPSTART ||
						newtag->tag.tagname[0] == FT_GAPEND)) {
							Gap_Struct* gap;
							uint16 gapkey = atoi(&newtag->tag.tagname[1]);
							if (!gap_map.Lookup(gapkey, gap)) {
								gap = new Gap_Struct;
								gap_map.SetAt(gapkey, gap);
								gap->start = (uint32)-1;
								gap->end = (uint32)-1;
							}
							if (newtag->tag.tagname[0] == FT_GAPSTART) {
								gap->start = newtag->tag.intvalue;
							}
							if (newtag->tag.tagname[0] == FT_GAPEND) {
								gap->end = newtag->tag.intvalue-1;
							}
							delete newtag;
							// End Changes by Slugfiller for better exception handling
						} else {
							taglist.Add(newtag);
						}
					}
				}
			} else {
				delete newtag;
			}
		}
		
		// load the hashsets from the hybridstylepartmet
		if (isnewstyle && !getsizeonly && (metFile.Tell()<metFile.Length()) ) {
			int8 temp;
			metFile.Read(&temp,1);
			
			uint16 parts=GetPartCount();	// assuming we will get all hashsets
			
			for (uint16 i = 0; i < parts && (metFile.Tell()+16<metFile.Length()); i++){
				uchar* cur_hash = new uchar[16];
				metFile.Read(cur_hash, 16);
				hashlist.Add(cur_hash);
			}

			uchar* checkhash= new uchar[16];
			if (!hashlist.IsEmpty()){
				uchar* buffer = new uchar[hashlist.GetCount()*16];
				for (size_t i = 0; i < hashlist.GetCount(); i++)
					md4cpy(buffer+(i*16), hashlist[i]);
				CreateHashFromString(buffer, hashlist.GetCount()*16, checkhash);
				delete[] buffer;
			}
			bool flag=false;
			if (!md4cmp(m_abyFileHash, checkhash))
				flag=true;
			else{
				/*
				for (size_t i = 0; i < hashlist.GetCount(); i++)
					delete[] hashlist[i];
				*/
				hashlist.Clear();
				flag=false;
			}
			delete[] checkhash;
		}			
			
		metFile.Close();
			
	} catch (CInvalidPacket e) {

		if (metFile.Eof()) {
			theApp.amuledlg->AddLogLine(true, _("Error: %s (%s) is corrupt, unable to load file"), m_partmetfilename, GetFileName().GetData());
		} else {
			theApp.amuledlg->AddLogLine(true, _("Unexpected file error while reading server.met: %s, unable to load serverlist"), m_partmetfilename, GetFileName().GetData(), buffer);
		}
		printf(" - Catched an error - ");
		if (metFile.IsOpened()) {
			metFile.Close();		
		}
		return false;
	}

	if (getsizeonly) {
		return partmettype;
	}
	// Now to flush the map into the list (Slugfiller)
	for (POSITION pos = gap_map.GetStartPosition(); pos != NULL; ){
		Gap_Struct* gap;
		uint16 gapkey;
		gap_map.GetNextAssoc(pos, gapkey, gap);
		// SLUGFILLER: SafeHash - revised code, and extra safety
		if (((int)gap->start) != -1 && ((int)gap->end) != -1 && gap->start <= gap->end && gap->start < m_nFileSize){
			if (gap->end >= m_nFileSize) {
				gap->end = m_nFileSize-1; // Clipping
			}
			AddGap(gap->start, gap->end); // All tags accounted for, use safe adding
		}
		delete gap;
		// SLUGFILLER: SafeHash
	}

	//check if this is a backup
	if(strcasecmp(strrchr(fullname, '.'), ".backup") == 0) {
		char *shorten = strrchr(fullname, '.');
		*shorten = 0;
		char *oldfullname = fullname;
		fullname = new char[strlen(fullname)+1];
		strcpy(fullname, oldfullname);
		delete[] oldfullname;
	}

	// open permanent handle
	char* searchpath = nstrdup(fullname);
	searchpath[strlen(fullname)-4] = 0;
	if (!m_hpartfile.Open(searchpath,/*wxFile::write_append*/CFile::read_write)) {
		theApp.amuledlg->AddLogLine(false, CString(_("Failed to open %s (%s)")), fullname, m_strFileName.GetData());
		delete[] searchpath;
		return false;
	}
	delete[] searchpath; searchpath = NULL;

	SetFilePath(searchpath);

	// SLUGFILLER: SafeHash - final safety, make sure any missing part of the file is gap
	if ((uint64)m_hpartfile.GetLength() < m_nFileSize)
		AddGap(m_hpartfile.GetLength(), m_nFileSize-1);
	// Goes both ways - Partfile should never be too large
	if ((uint64)m_hpartfile.GetLength() > m_nFileSize){
		//printf("Partfile \"%s\" is too large! Truncating %I64u bytes.\n", GetFileName().c_str(), (uint64) (m_hpartfile.GetLength() - m_nFileSize));
		printf("Partfile \"%s\" is too large! Truncating %llu bytes.\n", GetFileName().c_str(), ((ULONGLONG)m_hpartfile.GetLength() - m_nFileSize));
		m_hpartfile.SetLength(m_nFileSize);
	}
	// SLUGFILLER: SafeHash

	m_SrcpartFrequency.Clear();
	m_SrcpartFrequency.Alloc(GetPartCount());
	
	m_SrcpartFrequency.Insert(/*Item*/0, /*pos*/0, GetPartCount());
	
	SetPartFileStatus(PS_EMPTY);
	
	// check hashcount, file status etc
	if (GetHashCount() != GetED2KPartHashCount()){	
		hashsetneeded = true;
		return true;
	} else {
		hashsetneeded = false;
		for (size_t i = 0; i < hashlist.GetCount(); i++) {
			if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)) {
				SetPartFileStatus(PS_READY);
			}
		}
	}
	
	if (gaplist.IsEmpty()) { // is this file complete already?
		CompleteFile(false);
		return true;
	}

	if (!isnewstyle) { // not for importing	
		// check date of .part file - if its wrong, rehash file
		//CFileStatus filestatus;
		//m_hpartfile.GetStatus(filestatus);
		//struct stat statbuf;
		//fstat(m_hpartfile.fd(),&statbuf);
		//if ((time_t)date != (time_t)statbuf.st_mtime) {

		if ((time_t)date != wxFileModificationTime(wxString::wxString(fullname))) {
			theApp.amuledlg->AddLogLine(false, CString(_("Warning: %s might be corrupted")), buffer, m_strFileName.GetData());
			// rehash
			SetPartFileStatus(PS_WAITINGFORHASH);
			//CAddFileThread::AddFile(directory, searchpath, this);
			char *partfilename = nstrdup(m_partmetfilename);
			partfilename[strlen(partfilename)-4] = 0;
			CAddFileThread::AddFile(directory, partfilename, this);
			delete[] partfilename;
		}

		delete[] searchpath;

	}

	UpdateCompletedInfos();
	if (completedsize > transfered) {
		m_iGainDueToCompression = completedsize - transfered;
	} else if (completedsize != transfered) {
		m_iLostDueToCorruption = transfered - completedsize;
	}
	
	return true;
}

bool CPartFile::SavePartFile(bool Initial)
{
	//struct stat sbf;
	wxString fName;
	
	switch (status) {
		case PS_WAITINGFORHASH:
		case PS_HASHING:
		case PS_COMPLETE:
			return false;
	}
	/* Don't write anything to disk if less than 5000 bytes of free space is left. */
	wxLongLong total, free;
	if (wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free) && free < 5000) {
		return false;
	}
	
	CFile file;
	try {
		char* searchpath = nstrdup(fullname);
		searchpath[strlen(fullname)-4] = 0;
		fName=::wxFindFirstFile(searchpath,wxFILE);
		delete[] searchpath; searchpath = NULL;
		if(fName.IsEmpty()) {
			if (file.IsOpened()) {
				file.Close();
			}
			theApp.amuledlg->AddLogLine(false,CString(_("ERROR while saving partfile: %s (%s => %s)")),CString(_(".part file not found")).GetData(),m_partmetfilename,m_strFileName.GetData());
			return false;
		}

		uint32 lsc = lastseencomplete; //mktime(lastseencomplete.GetLocalTm());

		if (!Initial) {
			BackupFile(fullname, ".backup");
			wxRemoveFile(fullname);
		}
		
		file.Open(fullname,CFile::write);
		if (!file.IsOpened()) {
			throw wxString(wxT("Failed to open part.met file"));
		}

		// version
		uint8 version = PARTFILE_VERSION;
		file.Write(&version,1);
		// date
		//stat(fName.GetData(),&sbf);
		//date=sbf.st_mtime;
		date = ENDIAN_SWAP_32(wxFileModificationTime(wxString::wxString(fullname)));
		file.Write(&date,4);
		// hash
		file.Write(&m_abyFileHash,16);
		uint16 parts = ENDIAN_SWAP_16(hashlist.GetCount());
		file.Write(&parts,2);
		parts = hashlist.GetCount();
		for (int x = 0; x != parts; x++) {
			file.Write(hashlist[x],16);
		}
		// tags
		uint32 tagcount = ENDIAN_SWAP_32(taglist.GetCount()+10+(gaplist.GetCount()*2));
		file.Write(&tagcount,4);
		tagcount = taglist.GetCount()+10+(gaplist.GetCount()*2);

		CTag(FT_FILENAME,m_strFileName).WriteTagToFile(&file);	// 1
		CTag(FT_FILESIZE,m_nFileSize).WriteTagToFile(&file);	// 2
		CTag(FT_TRANSFERED,transfered).WriteTagToFile(&file);	// 3
		CTag(FT_STATUS,(paused)? 1:0).WriteTagToFile(&file);	// 4

		CTag* prioritytag;
		uint8 autoprio = PR_AUTO;
		if(this->IsAutoDownPriority()) {
			prioritytag = new CTag(FT_DLPRIORITY,autoprio);
		} else {
			prioritytag = new CTag(FT_DLPRIORITY,m_iDownPriority);
		}
		prioritytag->WriteTagToFile(&file);			// 5
		delete prioritytag;
		if(this->IsAutoDownPriority()) {
			prioritytag = new CTag(FT_OLDDLPRIORITY,autoprio);
		} else {
			prioritytag = new CTag(FT_OLDDLPRIORITY,m_iDownPriority);
		}
		prioritytag->WriteTagToFile(&file);                      // 6
		delete prioritytag;
		
		CTag* lsctag = new CTag(FT_LASTSEENCOMPLETE,lsc);
		lsctag->WriteTagToFile(&file);				// 7
		delete lsctag;

		CTag* ulprioritytag;
		if(this->IsAutoUpPriority()) {
			ulprioritytag = new CTag(FT_ULPRIORITY,autoprio);
		} else {
			ulprioritytag = new CTag(FT_ULPRIORITY,GetUpPriority());
		}
		ulprioritytag->WriteTagToFile(&file);			// 8
		delete ulprioritytag;
		
		if(this->IsAutoUpPriority()) {
			ulprioritytag = new CTag(FT_OLDULPRIORITY,autoprio);
		} else {
			ulprioritytag = new CTag(FT_OLDULPRIORITY,GetUpPriority());
		}
		ulprioritytag->WriteTagToFile(&file);			// 9
		delete ulprioritytag;
		
		// Madcat - Category setting.
		CTag* categorytab = new CTag(FT_CATEGORY,m_category);
		categorytab->WriteTagToFile(&file);			// 10
		delete categorytab;

		for (uint32 j = 0; j != (uint32)taglist.GetCount();j++) {
			taglist[j]->WriteTagToFile(&file);
		}
		// gaps
		char* namebuffer = new char[10];
		char* number = &namebuffer[1];
		uint16 i_pos = 0;
		for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)) {
			// itoa(i_pos,number,10);
			sprintf(number,"%d",i_pos);
			namebuffer[0] = FT_GAPSTART;
			CTag* gapstarttag = new CTag(namebuffer,gaplist.GetAt(pos)->start);
			gapstarttag->WriteTagToFile(&file);
			// gap start = first missing byte but gap ends = first non-missing byte
			// in edonkey but I think its easier to user the real limits
			namebuffer[0] = FT_GAPEND;
			CTag* gapendtag = new CTag(namebuffer,(gaplist.GetAt(pos)->end)+1);
			gapendtag->WriteTagToFile(&file);
			delete gapstarttag;
			delete gapendtag;
			i_pos++;
		}
		delete[] namebuffer;
		
		// Breaks the app for some reason
		/*
		if (ferror((FILE*)file.fd())) {
			throw wxString(wxT("Unexpected write error"));
		}
		*/

	} catch(char* error) {
		if (file.IsOpened()) {
			file.Close();
		}
		theApp.amuledlg->AddLogLine(false, CString(_("ERROR while saving partfile: %s (%s => %s)")), error, m_partmetfilename, m_strFileName.GetData());
		return false;
	} catch(wxString error) {
		if (file.IsOpened()) {
			file.Close();
		}
		theApp.amuledlg->AddLogLine(false, CString(_("ERROR while saving partfile: %s (%s => %s)")), error.GetData(), m_partmetfilename, m_strFileName.GetData());
		return false;


	}
	
	file.Close();

	//file.Flush();
	
	if (!Initial) {
		wxRemoveFile(wxString(fullname) + ".backup");
	}
	
	// Kry -don't backup if it's 0 size but raise a warning!!!
	wxFile newpartmet;
	if (newpartmet.Open(fullname)!=TRUE) {
		wxMessageBox(wxString::Format(_("Unable to open %s file - using %s file.\n"),fullname, PARTMET_BAK_EXT));
		FS_wxCopyFile(wxString(fullname) + PARTMET_BAK_EXT,fullname);
	} else {
		if (newpartmet.Length()>0) {			
			// not error, just backup
			newpartmet.Close();
			BackupFile(fullname, PARTMET_BAK_EXT);
		} else {
			newpartmet.Close();
			wxMessageBox(wxString::Format(_("%s file is 0 size somehow - using %s file.\n Please report on http://forum.amule.org\n"),fullname, PARTMET_BAK_EXT));
			FS_wxCopyFile(wxString(fullname) + PARTMET_BAK_EXT,fullname);			
		}
	}
	
	/*
	#ifdef __WXGTK__
	if (!theApp.use_chmod) {
		struct stat sbf;
		// Kry - Set the utime() so that we make sure the file date tag == mtime
		//printf("Seeting the mtime of %s according to date tag %u... ", fName.GetData(), date);
		stat(fName.GetData(),&sbf);
		//printf(" stated...");
		time_t atime = sbf.st_atime;	
		struct utimbuf timebuf;
		timebuf.actime = atime;
		timebuf.modtime = date;
		
		utime(fName.GetData(), &timebuf);
		//printf(" done.\n");
	}
	#endif
	*/
	return true;
}


void CPartFile::SaveSourceSeeds() {
	// Kry - Sources seeds
	// Copyright (c) Angel Vidal (Kry) 2004
	// Based on a Feature request, this saves the last 5 sources of the file,
	// giving a 'seed' for the next run.
	// We save the last sources because:
	// 1 - They could be the hardest to get
	// 2 - They will more probably be available
	// However, if we have downloading sources, they have preference because
	// we probably have more credits on them.
	// Anyway, source exchange will get us the rest of the sources
	// This feature is currently used only on rare files (< 20 sources)
	// 
	
	if (GetSourceCount()>20) {
		return;	
	}	
	
	CTypedPtrList<CPtrList, CUpDownClient*>	source_seeds;
	int n_sources = 0;
	
	if (m_downloadingSourcesList.GetCount()>0) {
		POSITION pos1, pos2;
		for (pos1 = m_downloadingSourcesList.GetHeadPosition();(((pos2 = pos1)  != NULL) && (n_sources<5));) {
			CUpDownClient* cur_src = m_downloadingSourcesList.GetNext(pos1);		
			if (cur_src->HasLowID()) {
				continue;
			} else {
				source_seeds.AddTail(cur_src);
			}
			n_sources++;
		}
	}

	if (n_sources<5) {
		// Not enought downloading sources to fill the list, going to sources list	
		if (GetSourceCount()>0) {
			// Random first slot to avoid flooding same sources always
			uint32 fst_slot = (uint32)(rand()/(RAND_MAX/(SOURCESSLOTS-1)));
			for (uint32 sl=fst_slot;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty()) {
				POSITION pos1, pos2;
				for (pos1 = srclists[sl].GetTailPosition();(((pos2 = pos1)  != NULL) && (n_sources<5));) {
					CUpDownClient* cur_src = srclists[sl].GetPrev(pos1);		
					if (cur_src->HasLowID()) {
						continue;
					} else {
						source_seeds.AddTail(cur_src);
					}
					n_sources++;
				}		
			}
			// Not yet? wrap around
			for (uint32 sl=0;sl<fst_slot-1;sl++) if (!srclists[sl].IsEmpty()) {
				POSITION pos1, pos2;
				for (pos1 = srclists[sl].GetTailPosition();(((pos2 = pos1)  != NULL) && (n_sources<5));) {
					CUpDownClient* cur_src = srclists[sl].GetPrev(pos1);		
					if (cur_src->HasLowID()) {
						continue;
					} else {
						source_seeds.AddTail(cur_src);
					}
					n_sources++;
				}		
			}
		}			
	}	
	
	// Write the file
	if (!n_sources) {
		return;
	} 
	

	CFile file;
	wxString fName;
	
	file.Create(wxString(fullname) + ".seeds",true);
	
	if (!file.IsOpened()) {
		theApp.amuledlg->AddLogLine(false,CString(_("Failed to save part.met.seeds file for %s")),fullname);
	}	

	uint8 src_count = source_seeds.GetCount();
	file.Write(&src_count,1);
	
	POSITION pos1, pos2;
	for (pos1 = source_seeds.GetHeadPosition();(pos2 = pos1)  != NULL;) {
		CUpDownClient* cur_src = source_seeds.GetNext(pos1);		
		uint32 dwID = cur_src->GetUserID();
		uint16 nPort = cur_src->GetUserPort();
		//uint32 dwServerIP = cur_src->GetServerIP();
		//uint16 nServerPort =cur_src->GetServerPort();
		file.Write(&dwID,4);
		file.Write(&nPort,2);
		//file.Write(&dwServerIP,4);
		//file.Write(&nServerPort,2);
	}	
	file.Flush();
	file.Close();

	theApp.amuledlg->AddLogLine(false,CString(_("Saved %i sources seeds for partfile: %s (%s)")),n_sources,fullname,m_strFileName.GetData());
	
}	


void CPartFile::LoadSourceSeeds() {
	
	wxString fName;
	CFile file;
	CMemFile sources_data;
	
	if (!wxFileName::FileExists(wxString(fullname) + ".seeds")) {
		return;
	} 
	
	file.Open(wxString(fullname) + ".seeds",CFile::read);

	if (!file.IsOpened()) {
		theApp.amuledlg->AddLogLine(false,CString(_("Partfile %s (%s) has no seeds file")),m_partmetfilename,m_strFileName.GetData());
		return;
	}	
	
	if (!file.Length()>1) {
		theApp.amuledlg->AddLogLine(false,CString(_("Partfile %s (%s) has void seeds file")),m_partmetfilename,m_strFileName.GetData());
		return;
	}	
	
	uint8 src_count;
	file.Read(&src_count,1);	
	
	sources_data.Write((uint16)src_count);

	for (int i=0;i<src_count;i++) {
	
		uint32 dwID;
		uint16 nPort;
		file.Read(&dwID,4);
		file.Read(&nPort,2);
		
		sources_data.Write(dwID);
		sources_data.Write(nPort);
		sources_data.Write((uint32) 0);
		sources_data.Write((uint16) 0);	
	}
	
	sources_data.Seek(0);
	
	AddClientSources(&sources_data, 1 );
	
	file.Close();	
}		

void CPartFile::PartFileHashFinished(CKnownFile* result)
{

	newdate = true;
	bool errorfound = false;
	if (GetED2KPartHashCount() == 0){
		if (IsComplete(0, m_nFileSize-1)){
			if (md4cmp(result->GetFileHash(), GetFileHash())){
				theApp.amuledlg->AddLogLine(false, CString(_("Found corrupted part (%i) in 0 parts file %s - FileResultHash |%s| FileHash |%s|")), 1, m_strFileName.GetData(),result->GetFileHash(), GetFileHash());		
				AddGap(0, m_nFileSize-1);
				errorfound = true;
			}
		}
	}
	else{
		for (size_t i = 0; i < hashlist.GetCount(); i++){
			// Kry - trel_ar's completed parts check on rehashing.
			// Very nice feature, if a file is completed but .part.met don't belive it,
			// update it.
			
			/*
			if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
				if (!(result->GetPartHash(i) && !md4cmp(result->GetPartHash(i),this->GetPartHash(i)))){
					theApp.amuledlg->AddLogLine(false, CString(_("Found corrupted part (%i) in %i parts file %s - FileResultHash |%s| FileHash |%s|")), i+1, GetED2KPartHashCount(), m_strFileName.GetData(),result->GetPartHash(i),this->GetPartHash(i));							
//					theApp.amuledlg->AddLogLine(false, CString(_("Found corrupted part (%i) in %s")), i+1, m_strFileName.GetData());		
					AddGap(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
					errorfound = true;
				}
			}
			*/
			if (!(result->GetPartHash(i) && !md4cmp(result->GetPartHash(i),this->GetPartHash(i)))){
				if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
					theApp.amuledlg->AddLogLine(false, CString(_("Found corrupted part (%i) in %i parts file %s - FileResultHash |%s| FileHash |%s|")), i+1, GetED2KPartHashCount(), m_strFileName.GetData(),result->GetPartHash(i),this->GetPartHash(i));
					AddGap(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
					errorfound = true;
				}
			} else {
				if (!IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
					theApp.amuledlg->AddLogLine(false, CString(_("Found completed part (%i) in %s")), i+1, m_strFileName.GetData());
					FillGap(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
					RemoveBlockFromList(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
				}
			}						
		}
	}
	delete result;
	if (!errorfound){
		if (status == PS_COMPLETING){
			CompleteFile(true);
			return;
		}
		else {
			theApp.amuledlg->AddLogLine(false, CString(_("Finished rehashing %s")), m_strFileName.GetData());
		}
	}
	else{
		SetStatus(PS_READY);
		SavePartFile();
		return;
	}
	SetStatus(PS_READY);
	SavePartFile();
	theApp.sharedfiles->SafeAddKFile(this);		
}

void CPartFile::AddGap(uint32 start, uint32 end)
{
	POSITION pos1, pos2;
	for (pos1 = gaplist.GetHeadPosition();(pos2 = pos1) != NULL;) {
		Gap_Struct* cur_gap = gaplist.GetNext(pos1);
		if (cur_gap->start >= start && cur_gap->end <= end) {
			// this gap is inside the new gap - delete
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		} else if (cur_gap->start >= start && cur_gap->start <= end) {
			// a part of this gap is in the new gap - extend limit and delete
			end = cur_gap->end;
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		} else if (cur_gap->end <= end && cur_gap->end >= start) {
			// a part of this gap is in the new gap - extend limit and delete
			start = cur_gap->start;
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		} else if (start >= cur_gap->start && end <= cur_gap->end){
			// new gap is already inside this gap - return
			return;
		}
	}
	Gap_Struct* new_gap = new Gap_Struct;
	new_gap->start = start;
	new_gap->end = end;
	gaplist.AddTail(new_gap);
	//theApp.amuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
	UpdateDisplayedInfo();
	newdate = true;
}

bool CPartFile::IsComplete(uint32 start, uint32 end)
{
	if (end >= m_nFileSize) {
		end = m_nFileSize-1;
	}
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)) {
		Gap_Struct* cur_gap = gaplist.GetAt(pos);
		if ((cur_gap->start >= start && cur_gap->end <= end)||(cur_gap->start >= start 
		&& cur_gap->start <= end)||(cur_gap->end <= end && cur_gap->end >= start)
		||(start >= cur_gap->start && end <= cur_gap->end)) {
			return false;	
		}
	}
	return true;
}

bool CPartFile::IsPureGap(uint32 start, uint32 end)
{
	if (end >= m_nFileSize) {
		end = m_nFileSize-1;
	}
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)) {
		Gap_Struct* cur_gap = gaplist.GetAt(pos);
		if (start >= cur_gap->start  && end <= cur_gap->end) {
			return true;
		}
	}
	return false;
}

bool CPartFile::IsAlreadyRequested(uint32 start, uint32 end)
{
	for (POSITION pos =  requestedblocks_list.GetHeadPosition();pos != 0; requestedblocks_list.GetNext(pos)) {
		Requested_Block_Struct* cur_block =  requestedblocks_list.GetAt(pos);
		// if (cur_block->StartOffset == start && cur_block->EndOffset == end)
		/* eMule 0.30c manage the problem like that, i give it a try ... (Creteil) */
		if ((start <= cur_block->EndOffset) && (end >= cur_block->StartOffset)) {
			return true;
		}
	}
	return false;
}

bool CPartFile::GetNextEmptyBlockInPart(uint16 partNumber, Requested_Block_Struct *result)
{
	Gap_Struct *firstGap;
	Gap_Struct *currentGap;
	uint32 end;
	uint32 blockLimit;

	// Find start of this part
	uint32 partStart = (PARTSIZE * partNumber);
	uint32 start = partStart;

	// What is the end limit of this block, i.e. can't go outside part (or filesize)
	uint32 partEnd = (PARTSIZE * (partNumber + 1)) - 1;
	if (partEnd >= GetFileSize()) {
		partEnd = GetFileSize() - 1;
	}
	// Loop until find a suitable gap and return true, or no more gaps and return false
	while (true) {
		firstGap = NULL;

		// Find the first gap from the start position
		for (POSITION pos = gaplist.GetHeadPosition(); pos != 0; gaplist.GetNext(pos)) {
			currentGap = gaplist.GetAt(pos);
			// Want gaps that overlap start<->partEnd
			if ((currentGap->start <= partEnd) && (currentGap->end >= start)) {
				// Is this the first gap?
				if ((firstGap == NULL) || (currentGap->start < firstGap->start)) {
					firstGap = currentGap;
				}
			}
		}

		// If no gaps after start, exit
		if (firstGap == NULL) {
			return false;
		}
		// Update start position if gap starts after current pos
		if (start < firstGap->start) {
			start = firstGap->start;
		}
		// If this is not within part, exit
		if (start > partEnd) {
			return false;
		}
		// Find end, keeping within the max block size and the part limit
		end = firstGap->end;
		blockLimit = partStart + (BLOCKSIZE * (((start - partStart) / BLOCKSIZE) + 1)) - 1;
		if (end > blockLimit) {
			end = blockLimit;
		}
		if (end > partEnd) {
			end = partEnd;
		}
		// If this gap has not already been requested, we have found a valid entry
		if (!IsAlreadyRequested(start, end)) {
			// Was this block to be returned
			if (result != NULL) {
				result->StartOffset = start;
				result->EndOffset = end;
				memcpy(result->FileID, GetFileHash(), 16);
				result->transferred = 0;
			}
			return true;
		} else {
			// Reposition to end of that gap
			start = end + 1;
		}
		// If tried all gaps then break out of the loop
		if (end == partEnd) {
			break;
		}
	}
	// No suitable gap found
	return false;
}

void CPartFile::FillGap(uint32 start, uint32 end)
{
	POSITION pos1, pos2;
	for (pos1 = gaplist.GetHeadPosition();(pos2 = pos1) != NULL;) {
		Gap_Struct* cur_gap = gaplist.GetNext(pos1);
		if (cur_gap->start >= start && cur_gap->end <= end) {
			// our part fills this gap completly
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		} else if (cur_gap->start >= start && cur_gap->start <= end) {
			// a part of this gap is in the part - set limit
			cur_gap->start = end+1;
		} else if (cur_gap->end <= end && cur_gap->end >= start) {
			// a part of this gap is in the part - set limit
			cur_gap->end = start-1;
		} else if (start >= cur_gap->start && end <= cur_gap->end) {
			uint32 buffer = cur_gap->end;
			cur_gap->end = start-1;
			cur_gap = new Gap_Struct;
			cur_gap->start = end+1;
			cur_gap->end = buffer;
			gaplist.InsertAfter(pos1,cur_gap);
			break;
		}
	}
	UpdateCompletedInfos();
	UpdateDisplayedInfo();
	newdate = true;
}

void CPartFile::UpdateCompletedInfos()
{
   	uint32 allgaps = 0; 
	for (POSITION pos = gaplist.GetHeadPosition(); pos != 0;) {
		POSITION prev = pos;
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if ((cur_gap->end > m_nFileSize) || (cur_gap->start >= m_nFileSize)) {
			gaplist.RemoveAt(prev);
		} else {
			allgaps += cur_gap->end - cur_gap->start + 1;
		}
	}
	if (gaplist.GetCount() || requestedblocks_list.GetCount()) {
		percentcompleted = (1.0f-(float)allgaps/m_nFileSize) * 100;
		completedsize = m_nFileSize - allgaps;
	} else {
		percentcompleted = 100;
		completedsize = m_nFileSize;
	}
}

#if 0
#warning work in progress
void CPartFile::DrawShareStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat){ 
	COLORREF crProgress;
	COLORREF crHave;
	COLORREF crPending;
	COLORREF crMissing = RGB(255, 0, 0);

	if(bFlat) { 
		crProgress = RGB(0, 150, 0);
		crHave = RGB(0, 0, 0);
		crPending = RGB(255,208,0);
	} else { 
		crProgress = RGB(0, 224, 0);
		crHave = RGB(104, 104, 104);
		crPending = RGB(255, 208, 0);
	} 

	s_ChunkBar.SetFileSize(this->GetFileSize()); 
	s_ChunkBar.SetHeight(rect->bottom - rect->top); 
	s_ChunkBar.SetWidth(rect->right - rect->left); 
	s_ChunkBar.Fill(crMissing); 
	COLORREF color;
	if (!onlygreyrect && !m_SrcpartFrequency.IsEmpty()) { 
		for (int i = 0;i < GetPartCount();i++)
			if(m_SrcpartFrequency[i] > 0 ){
				color = RGB(0, (210-(22*(m_SrcpartFrequency[i]-1)) <  0)? 0:210-(22*(m_SrcpartFrequency[i]-1)), 255);
				s_ChunkBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),color);
			}
	}
   	s_ChunkBar.Draw(dc, rect->left, rect->top, bFlat); 
} 
#endif

void CPartFile::DrawStatusBar(wxMemoryDC* dc, wxRect rect, bool bFlat)
{
	COLORREF crProgress;
	COLORREF crHave;
	COLORREF crPending;
	COLORREF crMissing = RGB(255, 0, 0);

	if(bFlat) {
		crProgress = RGB(0, 150, 0);
		crHave = RGB(0, 0, 0);
		crPending = RGB(255,255,100);
	} else {
		crProgress = RGB(0, 224, 0);
		crHave = RGB(104, 104, 104);
		crPending = RGB(255, 208, 0);
	}

	s_ChunkBar.SetHeight(rect.height - rect.y);
	s_ChunkBar.SetWidth(rect.width - rect.x); 
	s_ChunkBar.SetFileSize(m_nFileSize);
	s_ChunkBar.Fill(crHave);

	uint32 allgaps = 0;

	if(status == PS_COMPLETE || status == PS_COMPLETING) {
		s_ChunkBar.FillRange(0, m_nFileSize, crProgress);
		s_ChunkBar.Draw(dc, rect.x, rect.y, bFlat); 
		percentcompleted = 100; completedsize=m_nFileSize;
		return;
	}

	// red gaps
	for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;gaplist.GetNext(pos)) {
		Gap_Struct* cur_gap = gaplist.GetAt(pos);
		allgaps += cur_gap->end - cur_gap->start + 1;
		bool gapdone = false;
		uint32 gapstart = cur_gap->start;
		uint32 gapend = cur_gap->end;
		for (uint32 i = 0; i != GetPartCount(); i++) {
			if (gapstart >= i*PARTSIZE && gapstart <=  (i+1)*PARTSIZE) {
				// is in this part?
				if (gapend <= (i+1)*PARTSIZE) {
					gapdone = true;
				} else {
					gapend = (i+1)*PARTSIZE; // and next part
				}
				// paint
				COLORREF color;
				if (m_SrcpartFrequency.GetCount() >= (size_t)i && m_SrcpartFrequency[i]) {
					// frequency?
					color = RGB(0,(210-(22*(m_SrcpartFrequency[i]-1)) <  0)? 0:210-(22*(m_SrcpartFrequency[i]-1)),255);
				} else {
					color = crMissing;
				}
				s_ChunkBar.FillRange(gapstart, gapend + 1,  color);

				if (gapdone) {
					// finished?
					break;
				} else {
					gapstart = gapend;
					gapend = cur_gap->end;
				}
			}
		}
	}
	// yellow pending parts
	for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0;requestedblocks_list.GetNext(pos)) {
		Requested_Block_Struct* block =  requestedblocks_list.GetAt(pos);
		s_ChunkBar.FillRange(block->StartOffset, block->EndOffset,  crPending);
	}
	s_ChunkBar.Draw(dc, rect.x, rect.y, bFlat); 

	// green progress
	float blockpixel = (float)(rect.width)/((float)m_nFileSize);
	RECT gaprect;
	gaprect.top = rect.y; 
	gaprect.bottom = gaprect.top + 4;
	gaprect.left = rect.x; //->left;

	if(!bFlat) {
		s_LoadBar.SetWidth((int) ((m_nFileSize - allgaps) * blockpixel));
		s_LoadBar.Fill(crProgress);
		s_LoadBar.Draw(dc, gaprect.left, gaprect.top, false);
	} else {
		gaprect.right = rect.x + (int)((m_nFileSize - allgaps) * blockpixel);
		dc->SetBrush(*(wxTheBrushList->FindOrCreateBrush(wxColour(crProgress),wxSOLID))); //wxBrush(crProgress));
		dc->DrawRectangle(gaprect.left,gaprect.top,gaprect.right,gaprect.bottom);
		//dc->FillRect(&gaprect, &CBrush(crProgress));
		//draw gray progress only if flat
		gaprect.left = gaprect.right;
		gaprect.right = rect.width;
		dc->SetBrush(*(wxTheBrushList->FindOrCreateBrush(wxColour(224,224,224),wxSOLID))); //wxBrush(crPending));
		dc->DrawRectangle(gaprect.left,gaprect.top,gaprect.right,gaprect.bottom);
		//dc->FillRect(&gaprect, &CBrush(RGB(224,224,224)));
	}
	if ((gaplist.GetCount() || requestedblocks_list.GetCount())) {
		percentcompleted = ((1.0f-(float)allgaps/m_nFileSize)) * 100;
		completedsize = m_nFileSize - allgaps;
	} else {
		percentcompleted = 100;
		completedsize=m_nFileSize;
	}
}

void CPartFile::WritePartStatus(CMemFile* file)
{
	uint16 parts = GetED2KPartCount();
	file->Write(parts);
	uint16 done = 0;
	while (done != parts){
		uint8 towrite = 0;
		for (uint32 i = 0;i != 8;i++) {
			if (IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1)) {
				towrite |= (1<<i);
			}
			done++;
			if (done == parts) {
				break;
			}
		}
		file->Write(towrite);
	}
}

void CPartFile::WriteCompleteSourcesCount(CMemFile* file)
{
	file->Write(m_nCompleteSourcesCount);
}

int CPartFile::GetValidSourcesCount()
{
	int counter=0;
	POSITION pos1,pos2;
	for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty()) {
		for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;){
			srclists[sl].GetNext(pos1);
			CUpDownClient* cur_src = srclists[sl].GetAt(pos2);
			if (cur_src->GetDownloadState()!=DS_ONQUEUE && cur_src->GetDownloadState()!=DS_DOWNLOADING && cur_src->GetDownloadState()!=DS_NONEEDEDPARTS) {
				counter++;
			}
		}
	}
	return counter;
}

uint16 CPartFile::GetNotCurrentSourcesCount()
{
		uint16 counter=0;

		POSITION pos1,pos2;
		for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty()) {
			for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;){
				srclists[sl].GetNext(pos1);
				CUpDownClient* cur_src = srclists[sl].GetAt(pos2);
				if (cur_src->GetDownloadState()!=DS_ONQUEUE && cur_src->GetDownloadState()!=DS_DOWNLOADING) {
					counter++;
				}
			}
		}
		return counter;
}

uint8 CPartFile::GetStatus(bool ignorepause)
{
	if ((!paused && !insufficient) || status == PS_ERROR || status == PS_COMPLETING || status == PS_COMPLETE || ignorepause) {
		return status;
	} else {
		return PS_PAUSED;
	}
}

uint32 CPartFile::Process(uint32 reducedownload/*in percent*/,uint8 m_icounter)
{
	uint16 old_trans;
	CUpDownClient* cur_src;
	DWORD dwCurTick = ::GetTickCount();

	// If buffer size exceeds limit, or if not written within time limit, flush data
	if ((m_nTotalBufferData > theApp.glob_prefs->GetFileBufferSize())  || (dwCurTick > (m_nLastBufferFlushTime + BUFFER_TIME_LIMIT))) {
		// Avoid flushing while copying preview file
		if (!m_bPreviewing) {
			FlushBuffer();
		}
	}


	// check if we want new sources from server --> MOVED for 16.40 version
	old_trans=transferingsrc;
	transferingsrc = 0;
#ifdef DOWNLOADRATE_FILTERED
	kBpsDown = 0.0;
#else
	datarate = 0;  
#endif

	if (m_icounter < 10) {
		for(POSITION pos = m_downloadingSourcesList.GetHeadPosition();pos!=0;)
		{
			cur_src = m_downloadingSourcesList.GetNext(pos);
			if(cur_src && (cur_src->GetDownloadState() == DS_DOWNLOADING))
			{
				wxASSERT( cur_src->socket );
				if (cur_src->socket)
				{
					transferingsrc++;
	#ifdef DOWNLOADRATE_FILTERED
					float kBpsClient = cur_src->CalculateKBpsDown();
					kBpsDown += kBpsClient;
//					printf("ReduceDownload %i",reducedownload);
					if (reducedownload) {
						uint32 limit = (uint32)((float)reducedownload*kBpsClient);
//						printf(" Limit %i\n",limit);
	#else
					uint32 cur_datarate = cur_src->CalculateDownloadRate();
					datarate += cur_datarate;
					if (reducedownload) {
						uint32 limit = reducedownload*cur_datarate/1000;
	#endif
						if(limit<1000 && reducedownload == 200) {
							limit +=1000;
						} else if(limit<1) {
							limit = 1;
						}
						cur_src->socket->SetDownloadLimit(limit);
					} else { // Kry - Is this needed?
						cur_src->socket->DisableDownloadLimit();
					}
				}
			}
		}
	} else {
		
		POSITION pos1, pos2;
		for (uint32 sl = 0; sl < SOURCESSLOTS; sl++) {
			if (!srclists[sl].IsEmpty()) {
				for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;) {
					srclists[sl].GetNext(pos1);
					cur_src = srclists[sl].GetAt(pos2);
					uint8 download_state=cur_src->GetDownloadState();
					switch (download_state) {
						case DS_DOWNLOADING: {
							transferingsrc++;
	#ifdef DOWNLOADRATE_FILTERED
							float kBpsClient = cur_src->CalculateKBpsDown();
							kBpsDown += kBpsClient;
							if (reducedownload && download_state == DS_DOWNLOADING) {
								uint32 limit = (uint32)((float)reducedownload*kBpsClient);
	#else
							uint32 cur_datarate = cur_src->CalculateDownloadRate();
							datarate += cur_datarate;
							if (reducedownload && download_state == DS_DOWNLOADING) {
								uint32 limit = reducedownload*cur_datarate/1000;
	#endif
								if (limit < 1000 && reducedownload == 200) {
									limit += 1000;
								} else if (limit < 1) {
									limit = 1;
								}
								if (cur_src->socket) {
									cur_src->socket->SetDownloadLimit(limit);
								} else {
									break;
								}
							} else {
								if (cur_src->socket) {
									cur_src->socket->DisableDownloadLimit();
								} else {
									break;
								}
							}
							cur_src->SetValidSource(true);
							break;
						}
						case DS_BANNED: {
							break;
						}
						case DS_ERROR: {
							break;
						}
						case DS_LOWTOLOWIP: {
							if( cur_src->HasLowID() && (theApp.serverconnect->GetClientID() < 16777216) ) {
								//If we are almost maxed on sources, slowly remove these client to see if we can find a better source.
								if( ((dwCurTick - lastpurgetime) > 30000) && (this->GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8))) {
									theApp.downloadqueue->RemoveSource( cur_src );
									lastpurgetime = dwCurTick;
									break;
								}
								if (theApp.serverconnect->IsLowID()) {
									break;
								}
							} else {
								cur_src->SetDownloadState(DS_ONQUEUE);
							}
						}
						case DS_NONEEDEDPARTS: {
							// we try to purge noneeded source, even without reaching the limit
							if((dwCurTick - lastpurgetime) > 40000) {
								if(!cur_src->SwapToAnotherFile(false , false, false , NULL)) {
									//however we only delete them if reaching the limit
									if (GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8 )) {
										theApp.downloadqueue->RemoveSource(cur_src);
										lastpurgetime = dwCurTick;
										break; //Johnny-B - nothing more to do here (good eye!)
									}
								} else {
									cur_src->DontSwapTo(this);
									lastpurgetime = dwCurTick;
									break;
								}
							}
							// doubled reasktime for no needed parts - save connections and traffic
							if (!((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME*2)) {
								break;
							}
							// Recheck this client to see if still NNP.. Set to DS_NONE so that we force a TCP reask next time..
							cur_src->SetDownloadState(DS_NONE);						
						}
						case DS_ONQUEUE: {
							cur_src->SetValidSource(true);
							if( cur_src->IsRemoteQueueFull()) {
								cur_src->SetValidSource(false);
								if( ((dwCurTick - lastpurgetime) > 60000) && (this->GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8 )) ){
									theApp.downloadqueue->RemoveSource( cur_src );
									lastpurgetime = dwCurTick;
									break; //Johnny-B - nothing more to do here (good eye!)
								}
							}
							//Give up to 1 min for UDP to respond.. If we are within on min on TCP, do not try..
							if (theApp.serverconnect->IsConnected() && ((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME-20000)) {
								cur_src->UDPReaskForDownload();
							}
						}
						case DS_CONNECTING: 
						case DS_TOOMANYCONNS: 
						case DS_NONE: 
						case DS_WAITCALLBACK: {							
							if (theApp.serverconnect->IsConnected() && ((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME)) {
								if (!cur_src->AskForDownload()) {
									break; //I left this break here just as a reminder just in case re rearange things..
								}
							}
							break;
						}
						// Kry - this extra case is not processed on 0.42e
						/*
						case DS_CONNECTED: {
							if (download_state == DS_CONNECTED) {
								if( !(cur_src->socket && cur_src->socket->IsConnected()) ){
									cur_src->SetDownloadState(DS_NONE);
									break;
								}
								if (dwCurTick - cur_src->GetEnteredConnectedState() > CONNECTION_TIMEOUT + 20000){
									theApp.downloadqueue->RemoveSource( cur_src );
									break;
								}
							}
						}
						*/					
					}
				}
			}
		}

		/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
		if (IsA4AFAuto() && ((!m_LastNoNeededCheck) || (dwCurTick - m_LastNoNeededCheck > 900000))) {
			m_LastNoNeededCheck = dwCurTick;
			POSITION pos1, pos2;
			for (pos1 = A4AFsrclist.GetHeadPosition();(pos2=pos1)!=NULL;) {
				A4AFsrclist.GetNext(pos1);
				CUpDownClient *cur_source = A4AFsrclist.GetAt(pos2);
				uint8 download_state=cur_source->GetDownloadState();
				if( download_state != DS_DOWNLOADING
				&& cur_source->reqfile 
				&& ((!cur_source->reqfile->IsA4AFAuto()) || download_state == DS_NONEEDEDPARTS)
				&& !cur_source->IsSwapSuspended(this))
				{
					CPartFile* oldfile = cur_source->reqfile;
					if (cur_source->SwapToAnotherFile(false, false, false, this)) {
						cur_source->DontSwapTo(oldfile);
					}
				}
			}
		}
		/* eMule 0.30c implementation, i give it a try (Creteil) END ... */
		// swap No needed partfiles if possible
		/* Sources droping engine. Auto drop allowed type of sources at interval. */
		if (dwCurTick > m_LastSourceDropTime + theApp.glob_prefs->GetAutoDropTimer() * 1000) {
			m_LastSourceDropTime = dwCurTick;
			/* If all three are enabled, use CleanUpSources() function, will save us some CPU. */
			if (theApp.glob_prefs->DropNoNeededSources() && theApp.glob_prefs->DropFullQueueSources() && theApp.glob_prefs->DropHighQueueRankingSources()) {
				//printf("Cleaning up sources.\n");
				CleanUpSources();
			} else {
				/* Then check separately for each of them, and act accordingly. */
				if (theApp.glob_prefs->DropNoNeededSources()) {
					//printf("Dropping No Needed Sources.\n");
					RemoveNoNeededSources();
				}
				if (theApp.glob_prefs->DropFullQueueSources()) {
					//printf("Dropping Full Queue Sources.\n");
					RemoveFullQueueSources();
				}
				if (theApp.glob_prefs->DropHighQueueRankingSources()) {
					//printf("Dropping High Queue Rating Sources.\n");
					RemoveHighQueueRatingSources();
				}
			}
		}
	
		if (((old_trans==0) && (transferingsrc>0)) || ((old_trans>0) && (transferingsrc==0))) {
			SetPartFileStatus(status);
		}
	
		// check if we want new sources from server
		if ( !m_bLocalSrcReqQueued && ((!lastsearchtime) || (dwCurTick - lastsearchtime) > SERVERREASKTIME) && theApp.serverconnect->IsConnected()
		&& theApp.glob_prefs->GetMaxSourcePerFileSoft() > GetSourceCount() && !stopped ) {
			m_bLocalSrcReqQueued = true;
			theApp.downloadqueue->SendLocalSrcRequest(this);
		}
	
		// calculate datarate, set limit etc.
		
	}			

	count++;
	
	// Kry - does the 3 / 30 difference produce too much flickering or CPU?
	if (count >= 30) {
		count = 0;
		UpdateAutoDownPriority();
		UpdateDisplayedInfo();
		if(m_bPercentUpdated == false) {
			UpdateCompletedInfos();
		}
		m_bPercentUpdated = false;
	}
	
	
	
#ifdef DOWNLOADRATE_FILTERED
	return (uint32)(kBpsDown*1024.0);
#else
	return datarate;
#endif
}

bool CPartFile::CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, uint8* pdebug_lowiddropped)
{
	// MOD Note: Do not change this part - Merkur
	// check first if we are this source
	if (theApp.serverconnect->GetClientID() < 16777216 && theApp.serverconnect->IsConnected()){
		if ((theApp.serverconnect->GetClientID() == userid) && inet_addr(theApp.serverconnect->GetCurrentServer()->GetFullIP()) == serverip) {
			return false;
		}
	} else if (theApp.serverconnect->GetClientID() == userid) {
#ifdef __DEBUG__
		// It seems this can be used to test two amule's on one PC, using different ports --Aleric.
		if (!theApp.serverconnect->IsLowID() && theApp.glob_prefs->GetPort() != port)
		  return true;
#endif
		return false;
	} else if (userid < 16777216 && !theApp.serverconnect->IsLocalServer(serverip,serverport)) {
		if (pdebug_lowiddropped) {
			(*pdebug_lowiddropped)++;
		}
		return false;
	}
	// MOD Note - end
	return true;
}

void CPartFile::AddSources(CMemFile* sources,uint32 serverip, uint16 serverport)
{
	uint8 count;
	uint8 debug_lowiddropped = 0;
	uint8 debug_possiblesources = 0;

	sources->Read(count);
	if (stopped) {
		// since we may received multiple search source UDP results we have to "consume" all data of that packet
		sources->Seek(count*(4+2), wxFromStart);
		return;
	}

	for (int i = 0;i != count;i++) {
		uint32 userid;
		sources->Read(userid);
		uint16 port;
		sources->Read(port);
		
		// "Filter LAN IPs" and "IPfilter" the received sources IP addresses
		if (userid >= 16777216) {
			if (!IsGoodIP(userid)) { // check for 0-IP, localhost and optionally for LAN addresses
				//AddDebugLogLine(false, _T("Ignored source (IP=%s) received from server"), inet_ntoa(*(in_addr*)&userid));
				continue;
			}
			if (theApp.ipfilter->IsFiltered(userid)) {
				//AddDebugLogLine(false, _T("IPfiltered source IP=%s (%s) received from server"), inet_ntoa(*(in_addr*)&userid), theApp.ipfilter->GetLastHit());
				continue;
			}
		}

		if (!CanAddSource(userid, port, serverip, serverport, &debug_lowiddropped)) {
			continue;
		}
		if(theApp.glob_prefs->GetMaxSourcePerFile() > this->GetSourceCount()) {
			debug_possiblesources++;
			CUpDownClient* newsource = new CUpDownClient(port,userid,serverip,serverport,this);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		} else {
			// since we may received multiple search source UDP results we have to "consume" all data of that packet
			sources->Seek((count-i)*(4+2), wxFromStart);
			break;
		}
	}
}

void CPartFile::UpdatePartsInfo() {

	if( !IsPartFile() ) {
		CKnownFile::UpdatePartsInfo();
		return;
	}
	
	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 
	
	// Reset Part Count and allocate it with 0es
	
	m_SrcpartFrequency.Clear();
	m_SrcpartFrequency.Alloc(partcount);
	
	m_SrcpartFrequency.Insert(/*Item*/0, /*pos*/0, partcount);

	ArrayOfUInts16 count;	
	
	if (flag) {
		count.Alloc(GetSourceCount());	
	}
	
	CUpDownClient* cur_src;
	
	for(int sl=0; sl<SOURCESSLOTS; ++sl) {
		if (!srclists[sl].IsEmpty()) {
			for (POSITION pos = srclists[sl].GetHeadPosition(); pos != 0; )	{
				cur_src = srclists[sl].GetNext(pos);
				for (uint16 i = 0; i < partcount; i++)	{
					if (cur_src->IsPartAvailable(i)) {
						m_SrcpartFrequency[i] +=1;
					}
				}
				if ( flag) {
					count.Add(cur_src->GetUpCompleteSourcesCount());
				}
			}
		}
	}

	if( flag ) {
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;
	
		for (uint16 i = 0; i < partcount; i++)	{
			if( !i )	{
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			}
			else if( m_nCompleteSourcesCount > m_SrcpartFrequency[i]) {
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			}
		}
	
		count.Add(m_nCompleteSourcesCount);
	
		count.Shrink();
	
		int32 n = count.GetCount();
		if (n > 0) {

			// Kry - Native wx functions instead
			count.Sort(Uint16CompareValues);
			
			// calculate range
			int32 i= n >> 1;		// (n / 2)
			int32 j= (n * 3) >> 2;	// (n * 3) / 4
			int32 k= (n * 7) >> 3;	// (n * 7) / 8
			
			//When still a part file, adjust your guesses by 20% to what you see..

			
			if (n < 5) {
				//Not many sources, so just use what you see..
				// welcome to 'plain stupid code'
				// m_nCompleteSourcesCount; 
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCountHi= m_nCompleteSourcesCount;
			} else if (n < 20) {
				// For low guess and normal guess count
				//	 If we see more sources then the guessed low and normal, use what we see.
				//	 If we see less sources then the guessed low, adjust network accounts for 80%, 
				//  we account for 20% with what we see and make sure we are still above the normal.
				// For high guess
				//  Adjust 80% network and 20% what we see.
				if ( count[i] < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
				} else {
					m_nCompleteSourcesCountLo = (uint16)((float)(count[i]*.8)+(float)(m_nCompleteSourcesCount*.2));
				}
				m_nCompleteSourcesCount= m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= (uint16)((float)(count[j]*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;	
				}					
			} else {
				// Many sources
				// ------------
				// For low guess
				//	 Use what we see.
				// For normal guess
				//	 Adjust network accounts for 80%, we account for 20% with what 
				//  we see and make sure we are still above the low.
				// For high guess
				//  Adjust network accounts for 80%, we account for 20% with what 
				//  we see and make sure we are still above the normal.

				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCount= (uint16)((float)(count[j]*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCount < m_nCompleteSourcesCountLo ) {
					m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
				}
				m_nCompleteSourcesCountHi= (uint16)((float)(count[k]*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
				}
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
	UpdateDisplayedInfo();
}	

// Kry - Importing nice feature from from 030d
// [Maella -Enhanced Chunk Selection- (based on jicxicmic)]
bool CPartFile::GetNextRequestedBlock(CUpDownClient* sender, Requested_Block_Struct** newblocks, uint16* count)
{

	// The purpose of this function is to return a list of blocks (~180KB) to
	// download. To avoid a prematurely stop of the downloading, all blocks that 
	// are requested from the same source must be located within the same 
	// chunk (=> part ~9MB).
	//  
	// The selection of the chunk to download is one of the CRITICAL parts of the 
	// edonkey network. The selection algorithm must insure the best spreading
	// of files.
	//  
	// The selection is based on 4 criteria:
	//  1.  Frequency of the chunk (availability), very rare chunks must be downloaded 
	//      as quickly as possible to become a new available source.
	//  2.  Parts used for preview (first + last chunk), preview or check a 
	//      file (e.g. movie, mp3)
	//  3.  Request state (downloading in process), try to ask each source for another 
	//      chunk. Spread the requests between all sources.
	//  4.  Completion (shortest-to-complete), partially retrieved chunks should be 
	//      completed before starting to download other one.
	//  
	// The frequency criterion defines three zones: very rare (<10%), rare (<50%)
	// and common (>30%). Inside each zone, the criteria have a specific weight, used 
	// to calculate the priority of chunks. The chunk(s) with the highest 
	// priority (highest=0, lowest=0xffff) is/are selected first.
	//  
	//          very rare   (preview)       rare                      common
	//    0% <---- +0 pt ----> 10% <----- +10000 pt -----> 50% <---- +20000 pt ----> 100%
	// 1.  <------- frequency: +25*frequency pt ----------->
	// 2.  <- preview: +1 pt --><-------------- preview: set to 10000 pt ------------->
	// 3.                       <------ request: download in progress +20000 pt ------>
	// 4a. <- completion: 0% +100, 25% +75 .. 100% +0 pt --><-- !req => completion --->
	// 4b.                                                  <--- req => !completion -->
	//  
	// Unrolled, the priority scale is:
	//  
	// 0..xxxx       unrequested and requested very rare chunks
	// 10000..1xxxx  unrequested rare chunks + unrequested preview chunks
	// 20000..2xxxx  unrequested common chunks (priority to the most complete)
	// 30000..3xxxx  requested rare chunks + requested preview chunks
	// 40000..4xxxx  requested common chunks (priority to the least complete)
	//
	// This algorithm usually selects first the rarest chunk(s). However, partially
	// complete chunk(s) that is/are close to completion may overtake the priority 
	// (priority inversion).
	// For the common chuncks, the algorithm tries to spread the dowload between
	// the sources
	//

	// Check input parameters
	if(count == NULL) {
		return false;
	}
	if(sender->GetPartStatus() == NULL) {
		return false;
	}
	// Define and create the list of the chunks to download
	const uint16 partCount = GetPartCount();
	ListOfChunks chunksList;
	ListOfChunks::Node *node;
	// Main loop
	uint16 newBlockCount = 0;
	while(newBlockCount != *count) {
		// Create a request block stucture if a chunk has been previously selected
		if(sender->m_lastPartAsked != 0xffff) {
			Requested_Block_Struct* pBlock = new Requested_Block_Struct;
			if(GetNextEmptyBlockInPart(sender->m_lastPartAsked, pBlock) == true) {
				// Keep a track of all pending requested blocks
				requestedblocks_list.AddTail(pBlock);
				// Update list of blocks to return
				newblocks[newBlockCount++] = pBlock;
				// Skip end of loop (=> CPU load)
				continue;
			} else {
				// All blocks for this chunk have been already requested
				delete pBlock;
				// => Try to select another chunk
				sender->m_lastPartAsked = 0xffff;
			}
		}

		// Check if a new chunk must be selected (e.g. download starting, previous chunk complete)
		if(sender->m_lastPartAsked == 0xffff) {
			// Quantify all chunks (create list of chunks to download) 
			// This is done only one time and only if it is necessary (=> CPU load)
			if(chunksList.IsEmpty() == TRUE) {
				// Indentify the locally missing part(s) that this source has
				for(uint16 i=0; i < partCount; i++) {
					if(sender->IsPartAvailable(i) == true && GetNextEmptyBlockInPart(i, NULL) == true) {
						// Create a new entry for this chunk and add it to the list
						Chunk* newEntry = new Chunk;;
						newEntry->part = i;
						newEntry->frequency = m_SrcpartFrequency[i];
						chunksList.Append(newEntry);
					}
				}

				// Check if any bloks(s) could be downloaded
				if(chunksList.IsEmpty() == TRUE) {
					break; // Exit main loop while()
				}

				// Define the bounds of the three zones (very rare, rare)
				// more depending on available sources
				uint8 modif=10;
				if (GetSourceCount()>200) {
					modif=5;
				} else if (GetSourceCount()>800) {
					modif=2;
				}
				uint16 limit= modif*GetSourceCount()/ 100;
				if (limit==0) {
					limit=1;
				}
				const uint16 veryRareBound = limit;
				const uint16 rareBound = 2*limit;

				// Cache Preview state (Criterion 2)
				const bool isPreviewEnable = theApp.glob_prefs->GetPreviewPrio() && (IsArchive() || IsMovie());
					
				node = chunksList.GetFirst();
				// Collect and calculate criteria for all chunks
				while (node) {
					Chunk* cur_chunk = node->GetData();
					node = node->GetNext();
					// Offsets of chunk
					const uint32 uStart = cur_chunk->part * PARTSIZE;
					const uint32 uEnd  = ((GetFileSize() - 1) < (uStart + PARTSIZE - 1)) ? (GetFileSize() - 1) : (uStart + PARTSIZE - 1);
					// Criterion 2. Parts used for preview
					// Remark: - We need to download the first part and the last part(s).
					//        - When the last part is very small, it's necessary to 
					//          download the two last parts.
					bool critPreview = false;
					if(isPreviewEnable == true) {
						if(cur_chunk->part == 0) {
							critPreview = true; // First chunk
						} else if(cur_chunk->part == partCount-1) {
							critPreview = true; // Last chunk
						} else if(cur_chunk->part == partCount-2) {
							// Last chunk - 1 (only if last chunk is too small)
							const uint32 sizeOfLastChunk = GetFileSize() - uEnd;
							if(sizeOfLastChunk < PARTSIZE/3) {
								critPreview = true; // Last chunk - 1
							}
						}
					}

					// Criterion 3. Request state (downloading in process from other source(s))
					// => CPU load
					const bool critRequested = cur_chunk->frequency > veryRareBound && IsAlreadyRequested(uStart, uEnd);

					// Criterion 4. Completion
					uint32 partSize = PARTSIZE;
					for(POSITION pos = gaplist.GetHeadPosition(); pos != NULL;) {
						const Gap_Struct* cur_gap = gaplist.GetNext(pos);
						// Check if Gap is into the limit
						if(cur_gap->start < uStart) {
							if(cur_gap->end > uStart && cur_gap->end < uEnd) {
								partSize -= cur_gap->end - uStart + 1;
							} else if(cur_gap->end >= uEnd) {
								partSize = 0;
								break; // exit loop for()
							}
						} else if(cur_gap->start <= uEnd) {
							if(cur_gap->end < uEnd) {
								partSize -= cur_gap->end - cur_gap->start + 1;
							} else {
								partSize -= uEnd - cur_gap->start + 1;
							}
						}
					}
					const uint16 critCompletion = (uint16)(partSize/(PARTSIZE/100)); // in [%]

					// Calculate priority with all criteria
					if(cur_chunk->frequency <= veryRareBound) {
						// 0..xxxx unrequested + requested very rare chunks
						cur_chunk->rank = (25 * cur_chunk->frequency) + // Criterion 1
						((critPreview == true) ? 0 : 1) + // Criterion 2
						(100 - critCompletion); // Criterion 4
					} else if(critPreview == true) {
						// 10000..10100  unrequested preview chunks
						// 30000..30100  requested preview chunks
						cur_chunk->rank = ((critRequested == false) ? 10000 : 30000) + // Criterion 3
						(100 - critCompletion); // Criterion 4
					} else if(cur_chunk->frequency <= rareBound) {
						// 10101..1xxxx  unrequested rare chunks
						// 30101..3xxxx  requested rare chunks
						cur_chunk->rank = (25 * cur_chunk->frequency) +                 // Criterion 1 
						((critRequested == false) ? 10101 : 30101) + // Criterion 3
						(100 - critCompletion); // Criterion 4
					} else {
						// common chunk
						if(critRequested == false) { // Criterion 3
							// 20000..2xxxx  unrequested common chunks
							cur_chunk->rank = 20000 + // Criterion 3
							(100 - critCompletion); // Criterion 4
						} else {
							// 40000..4xxxx  requested common chunks
							// Remark: The weight of the completion criterion is inversed
							//         to spead the requests over the completing chunks.
							//         Without this, the chunk closest to completion will
							//         received every new sources.
							cur_chunk->rank = 40000 + // Criterion 3
							(critCompletion); // Criterion 4
						}
					}
				}
			}

			// Select the next chunk to download
			if(chunksList.IsEmpty() == FALSE) {
				// Find and count the chunck(s) with the highest priority
				uint16 count = 0; // Number of found chunks with same priority
				uint16 rank = 0xffff; // Highest priority found
				node = chunksList.GetFirst();
				// Collect and calculate criteria for all chunks
				while (node) {
					const Chunk* cur_chunk = node->GetData();
					node = node->GetNext();
					if(cur_chunk->rank < rank) {
						count = 1;
						rank = cur_chunk->rank;
					} else if(cur_chunk->rank == rank) {
						count++;
					}
				}

				// Use a random access to avoid that everybody tries to download the 
				// same chunks at the same time (=> spread the selected chunk among clients)
				uint16 randomness = 1 + (int) (((float)(count-1))*rand()/(RAND_MAX+1.0));
				node = chunksList.GetFirst();
				ListOfChunks::Node* nextnode;
				while (node) {
					const Chunk* cur_chunk = node->GetData();
					nextnode = node->GetNext();
					if(cur_chunk->rank == rank) {
						randomness--;
						if(randomness == 0) {
							// Selection process is over
							sender->m_lastPartAsked = cur_chunk->part;
							// Remark: this list might be reused up to *count times
							delete cur_chunk;
							delete node;
							break; // exit loop for()
						}  
					}
					node = nextnode;
				}
			} else {
				// There is no remaining chunk to download
				break; // Exit main loop while()
			}
		}
	}
	// Return the number of the blocks 
	*count = newBlockCount;
	// Return
	return (newBlockCount > 0);
}
// Maella end
// Kry EOI

void  CPartFile::RemoveBlockFromList(uint32 start,uint32 end)
{
	POSITION pos1,pos2;
	for (pos1 = requestedblocks_list.GetHeadPosition();(pos2 = pos1) != NULL;) {
		requestedblocks_list.GetNext(pos1);
		if (requestedblocks_list.GetAt(pos2)->StartOffset <= start && requestedblocks_list.GetAt(pos2)->EndOffset >= end) {
			requestedblocks_list.RemoveAt(pos2);
		}
	}
}

void CPartFile::RemoveAllRequestedBlocks(void)
{
	requestedblocks_list.RemoveAll();
}

//#include <pthread.h>
//pthread_attr_t pattr;

void CPartFile::CompleteFile(bool bIsHashingDone)
{
	theApp.downloadqueue->RemoveLocalServerRequest(this);

	if(this->srcarevisible) {
		theApp.amuledlg->transferwnd->downloadlistctrl->HideSources(this);
	}
	if (!bIsHashingDone) {
		printf("HashNotDone\n");
		SetPartFileStatus(PS_COMPLETING);
#ifdef DOWNLOADRATE_FILTERED
		kBpsDown = 0.0;
#else
		datarate = 0;
#endif
		char* partfileb = nstrdup(m_partmetfilename);
		partfileb[strlen(m_partmetfilename)-4] = 0;
		CAddFileThread::AddFile(theApp.glob_prefs->GetTempDir(), partfileb, this);
		delete[] partfileb;
		return;
	} else {
		printf("HashDone\n");		
		StopFile();
		m_is_A4AF_auto=false;
		SetPartFileStatus(PS_COMPLETING);
		// guess I was wrong about not need to spaw a thread ...
		// It is if the temp and incoming dirs are on different
		// partitions/drives and the file is large...[oz]
		//
		// use pthreads

		/*
		pthread_t tid;
		pthread_attr_init(&pattr);
		pthread_attr_setdetachstate(&pattr,PTHREAD_CREATE_DETACHED);
		pthread_create(&tid,&pattr,(void*(*)(void*))CompleteThreadProc,this);
		*/

		PerformFileComplete();


	}
	theApp.amuledlg->transferwnd->downloadlistctrl->ShowFilesCount();
	UpdateDisplayedInfo(true);
}

#define UNEXP_FILE_ERROR			1
#define DELETE_FAIL_MET 			2
#define DELETE_FAIL_MET_BAK		4
#define SAME_NAME_RENAMED 	8
#define DELETE_FAIL_PART		 	16
#define DELETE_FAIL_SEEDS		32

// Kry - Anything to declare? ;)
// Free for new errors / messages

//#define UNEXP_FILE_ERROR 64
//#define UNEXP_FILE_ERROR 128


void CPartFile::CompleteFileEnded(int completing_result, wxString* newname) {
	
	
	if (!(completing_result & UNEXP_FILE_ERROR)) {
		delete [] fullname;
	
		fullname = nstrdup(newname->c_str());
	
		delete newname;
		
		delete[] directory;

		if(wxFileName::DirExists(theApp.glob_prefs->GetCategory(GetCategory())->incomingpath)) {
			directory = nstrdup(theApp.glob_prefs->GetCategory(m_category)->incomingpath);	
		} else {
			directory = nstrdup(theApp.glob_prefs->GetIncomingDir());
		}	
	
		SetPartFileStatus(PS_COMPLETE);
		paused = false;
		// TODO: What the f*** if it is already known?
		theApp.knownfiles->SafeAddKFile(this);
		// remove the file from the suspended uploads list
		theApp.uploadqueue->ResumeUpload(GetFileHash());
		SetAutoUpPriority(false);
		theApp.downloadqueue->RemoveFile(this);
		//theApp.amuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
		UpdateDisplayedInfo();
		theApp.amuledlg->transferwnd->downloadlistctrl->ShowFilesCount();

		//SHAddToRecentDocs(SHARD_PATH, fullname); // This is a real nasty call that takes ~110 ms on my 1.4 GHz Athlon and isn't really needed afai see...[ozon]
		// Barry - Just in case
		transfered = m_nFileSize;

		theApp.downloadqueue->StartNextFile();
	
	} else {
		paused = true;
		SetPartFileStatus(PS_ERROR);
		theApp.downloadqueue->StartNextFile();	
		theApp.QueueLogLine(true,CString(_("Unexpected file error while completing %s. File paused")),GetFileName().GetData());
		delete newname;
		return;
	}	
	
	if (completing_result & DELETE_FAIL_MET) {
		theApp.QueueLogLine(true,CString(_("WARNING: Failed to delete %s")),fullname);		
	}	
	
	if (completing_result & DELETE_FAIL_MET_BAK) {
		theApp.QueueLogLine(true,CString(_("WARNING: Failed to delete %s%s")),fullname, PARTMET_BAK_EXT);				
	}	
	
	if (completing_result & SAME_NAME_RENAMED) {
		theApp.QueueLogLine(true, CString(_("WARNING: A file with that name already exists, the file has been renamed")));
	}		

	if (completing_result & DELETE_FAIL_MET) {
		theApp.QueueLogLine(true,"WARNING: could not remove original '%s' after creating backup\n", wxString(m_partmetfilename).Left(strlen(m_partmetfilename)-4).c_str());
	}	
	
	if (completing_result & DELETE_FAIL_SEEDS) {
		theApp.QueueLogLine(true,"WARNING: Failed to delete %s.seeds\n", m_partmetfilename);
	}	

	
	theApp.QueueLogLine(true,CString(_("Finished downloading %s :-)")),GetFileName().GetData());
	theApp.amuledlg->ShowNotifier(CString(_("Downloaded:"))+"\n"+GetFileName(), TBN_DLOAD);
	
}

completingThread::completingThread(wxString FileName, wxString fullname, uint32 Category, CPartFile* caller):wxThread(wxTHREAD_DETACHED)
{
	wxASSERT(!FileName.IsEmpty());
	wxASSERT(caller);
	wxASSERT(fullname);
	completing = caller;
	Completing_FileName = FileName;
	Completing_Fullname = fullname;
	Completing_Category = Category;
}

completingThread::~completingThread()
{
	//maybe a thread deletion needed
}

wxThread::ExitCode completingThread::Entry()
{

	// Threaded Completion code.
	
	completing_result = 0;

	// Strip the .met
	wxString partfilename =  Completing_Fullname.Left(Completing_Fullname.Length()-4);
	
	Completing_FileName = theApp.StripInvalidFilenameChars(Completing_FileName.c_str());

	newname = new wxString();
	if(wxFileName::DirExists(theApp.glob_prefs->GetCategory(Completing_Category)->incomingpath)) {
		(*newname) =  theApp.glob_prefs->GetCategory(Completing_Category)->incomingpath;
	} else {
		(*newname) =  theApp.glob_prefs->GetIncomingDir();
	}	
	(*newname) += "/";
	(*newname) += Completing_FileName;
	
	if(wxFileName::FileExists(*newname)) {
		completing_result |= SAME_NAME_RENAMED;

		int namecount = 0;

		// the file extension & name
		wxString ext = Completing_FileName.AfterLast('.');
		wxString filename = Completing_FileName.BeforeLast('.');

		wxString strTestName;
		do {
			namecount++;
			if (ext.IsEmpty()) {
				strTestName = theApp.glob_prefs->GetIncomingDir(); 
				strTestName += "/";
				strTestName += filename + wxString::Format("(%d)", namecount);
			} else {
				strTestName = theApp.glob_prefs->GetIncomingDir(); 
				strTestName += "/";
				strTestName += filename + wxString::Format("(%d).", namecount);
				strTestName += ext;
			}
		} while(wxFileName::FileExists(strTestName));
		
		*newname = strTestName;
	}

	
	if (!FS_wxRenameFile(partfilename, *newname)) {
		if (!FS_wxCopyFile(partfilename, *newname)) {
			completing_result |= UNEXP_FILE_ERROR;
			return NULL;
		}
		if ( !wxRemoveFile(partfilename) ) {
			completing_result |= DELETE_FAIL_PART;
		}
	}
	
	if (!wxRemoveFile(Completing_Fullname)) {
		completing_result |= DELETE_FAIL_MET;
	}
	
	wxString BAKName(Completing_Fullname);
	BAKName += PARTMET_BAK_EXT;
	if (!wxRemoveFile(BAKName)) {
		completing_result |= DELETE_FAIL_MET_BAK;
	}

	wxString SEEDSName(Completing_Fullname);
	SEEDSName += ".seeds";
	if (wxFileName::FileExists(SEEDSName)) {
		if (!wxRemoveFile(SEEDSName)) {
			completing_result |= DELETE_FAIL_SEEDS;
		}
	}
	
	return NULL;
}

/*
void completingThread::setFile(CPartFile* pFile)
{
	if (pFile!=NULL) {
		completing = pFile;
	}
}
*/

void completingThread::OnExit(){
	
	// Kry - Notice the app that the completion has finished for this file.		
	
	wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_FILECOMPLETIONFINISHED);
	evt.SetClientData(completing);
	evt.SetInt((int)completing_result);
	evt.SetExtraLong((long)newname);
	wxPostEvent(theApp.amuledlg,evt);
	
}

/*
UINT CPartFile::CompleteThreadProc(CPartFile* pFile)
{
	UINT return_code;
	if (!pFile) {
		printf("NOT pFile !!!\n");
		return_code=-1;
	} else {
		printf("pFile->PerformFileComplete(); !!!\n");
   		pFile->PerformFileComplete();
		return_code=0;
	}
	// Kry - Fixed completethread not exiting
	pthread_exit((UINT*)&return_code);
   	return 0;
}
*/


// Lord KiRon - using threads for file completion
uint8 CPartFile::PerformFileComplete()
{
	uint8 completed_errno = 0;
	
	//CSingleLock(&m_FileCompleteMutex,TRUE); // will be unlocked on exit
	wxMutexLocker sLock(m_FileCompleteMutex);

	// add this file to the suspended uploads list
	theApp.uploadqueue->SuspendUpload(GetFileHash());
	FlushBuffer();

	// close permanent handle
	m_hpartfile.Close();
	
	// Call thread for completion
	cthread=new completingThread(GetFileName(), wxString(fullname), GetCategory(), this);
	cthread->Create();
	cthread->Run();
	
	return completed_errno;
}

void  CPartFile::RemoveAllSources(bool bTryToSwap)
{

	POSITION pos1,pos2;
	for (int sl=0;sl<SOURCESSLOTS;sl++) {
		if (!srclists[sl].IsEmpty()) {
			for(pos1 = srclists[sl].GetHeadPosition(); (pos2 = pos1) != NULL;) {
				srclists[sl].GetNext(pos1);
				if (bTryToSwap) {
					if (!srclists[sl].GetAt(pos2)->SwapToAnotherFile(true, true, true, NULL)) {
						theApp.downloadqueue->RemoveSource(srclists[sl].GetAt(pos2),true, false);
					}
				} else {
					theApp.downloadqueue->RemoveSource(srclists[sl].GetAt(pos2),true, false);
				}
			}
		}
	}

	UpdatePartsInfo(); 
	UpdateAvailablePartsCount();
	
	/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
	// remove all links A4AF in sources to this file
	if(!A4AFsrclist.IsEmpty()) {
		POSITION pos1, pos2;
		for(pos1 = A4AFsrclist.GetHeadPosition();(pos2=pos1)!=NULL;) {
			A4AFsrclist.GetNext(pos1);
			POSITION pos3 = A4AFsrclist.GetAt(pos2)->m_OtherRequests_list.Find(this);
			if(pos3) {
				A4AFsrclist.GetAt(pos2)->m_OtherRequests_list.RemoveAt(pos3);
				theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(this->A4AFsrclist.GetAt(pos2),this);
			} else {
				pos3 = A4AFsrclist.GetAt(pos2)->m_OtherNoNeeded_list.Find(this);
				if(pos3) {
					A4AFsrclist.GetAt(pos2)->m_OtherNoNeeded_list.RemoveAt(pos3);
					theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(A4AFsrclist.GetAt(pos2),this);
				}
			}
		}
		A4AFsrclist.RemoveAll();
	}
	/* eMule 0.30c implementation, i give it a try (Creteil) END ... */
	UpdateFileRatingCommentAvail();
}

void CPartFile::Delete()
{
	printf("Canceling\n");
	// Barry - Need to tell any connected clients to stop sending the file
	StopFile(true);
	printf("\tStopped\n");
	
	theApp.sharedfiles->RemoveFile(this);
	printf("\tRemoved from shared\n");
	theApp.downloadqueue->RemoveFile(this);
	printf("\tRemoved from download queue\n");
	theApp.amuledlg->transferwnd->downloadlistctrl->RemoveFile(this);
	printf("\tRemoved transferwnd\n");

	// Kry - WTF? 
	// eMule had same problem with lseek error ... and override with a simple 
	// check for INVALID_HANDLE_VALUE (that, btw, does not exist on linux)
	// So we just guess is < 0 on error and > 2 if ok (0 stdin, 1 stdout, 2 stderr)
	if (m_hpartfile.fd() > 2) {  // 0 stdin, 1 stdout, 2 stderr
		m_hpartfile.Close();
	}

	printf("\tClosed\n");
	
	if (!wxRemoveFile(fullname)) {
		theApp.amuledlg->AddLogLine(true,CString(_("Failed to delete %s")),fullname);
	}
	printf("\tRemoved .part.met\n");

	char* partfilename = nstrdup(fullname);
	partfilename[strlen(fullname)-4] = 0;

	if (!wxRemoveFile(partfilename)) {
		theApp.amuledlg->AddLogLine(true,CString(_("Failed to delete %s")),partfilename);
	}
	printf("\tRemoved .part\n");
	
	CString BAKName(fullname);
	BAKName.Append(PARTMET_BAK_EXT);

	if (!wxRemoveFile(BAKName)) {
		theApp.amuledlg->AddLogLine(true,CString(_("Failed to delete %s")), BAKName.c_str());
	}
	printf("\tRemoved .BAK\n");
	
	wxString SEEDSName(fullname);
	SEEDSName += ".seeds";
	
	if (wxFileName::FileExists(SEEDSName)) {
		if (!wxRemoveFile(SEEDSName)) {
			theApp.amuledlg->AddLogLine(true,CString(_("Failed to delete %s")), SEEDSName.c_str());
		}
		printf("\tRemoved .seeds\n");
	}

	partfilename[strlen(fullname)-4] = '.'; // I like delete to clear full string
	delete[] partfilename;
	printf("Done\n");
	delete this;
}

bool CPartFile::HashSinglePart(uint16 partnumber)
{
	if ((GetHashCount() <= partnumber) && (GetPartCount() > 1)) {
		theApp.amuledlg->AddLogLine(true,CString(_("Warning: Unable to hash downloaded part - hashset incomplete (%s)")),GetFileName().GetData());
		this->hashsetneeded = true;
		return true;
	} else if(!GetPartHash(partnumber) && GetPartCount() != 1) {
		theApp.amuledlg->AddLogLine(true,CString(_("Error: Unable to hash downloaded part - hashset incomplete (%s). This should never happen")),GetFileName().GetData());
		this->hashsetneeded = true;
		return true;		
	} else {
		uchar hashresult[16];
		m_hpartfile.Seek((off_t)PARTSIZE*partnumber,wxFromStart);
		uint32 length = PARTSIZE;
		if (((ULONGLONG)PARTSIZE*(partnumber+1)) > (ULONGLONG)m_hpartfile.GetLength()){
			length = (m_hpartfile.GetLength() - ((ULONGLONG)PARTSIZE*partnumber));
			wxASSERT( length <= PARTSIZE );
		}
		CreateHashFromFile(&m_hpartfile,length,hashresult);

		if (GetPartCount() > 1) {
			if (md4cmp(hashresult,GetPartHash(partnumber))) {
				printf("HashResult:\n");
				DumpMem(hashresult,16);
				printf("GetPartHash(%i) :\n",partnumber);
				DumpMem(GetPartHash(partnumber),16);		
				/* To output to stdout - we should output to file
				m_hpartfile.Seek((off_t)PARTSIZE*partnumber,wxFromStart);
				uint32 length = PARTSIZE;
				if (((ULONGLONG)PARTSIZE*(partnumber+1)) > (ULONGLONG)m_hpartfile.GetLength()){
					length = (m_hpartfile.GetLength() - ((ULONGLONG)PARTSIZE*partnumber));
					wxASSERT( length <= PARTSIZE );
				}				
				uchar mychar[length+1];
				m_hpartfile.Read(mychar,length);
				printf("Corrupt Data:\n");
				DumpMem(mychar,length);										
				*/
				return false;
			} else {
				return true;
			}
		} else {
			if (md4cmp(hashresult,m_abyFileHash)) {
				return false;
			} else {
				return true;
			}
		}
	}

}

bool CPartFile::IsCorruptedPart(uint16 partnumber)
{
	return corrupted_list.Find(partnumber);
}

bool CPartFile::IsMovie()
{
	bool it_is;
	wxString extension = GetFileName().Right(5);
	it_is = ((extension.CmpNoCase(".divx") == 0) || (extension.CmpNoCase(".mpeg") == 0) ||
	(extension.CmpNoCase(".vivo") == 0));

	extension = GetFileName().Right(4);
	it_is = (it_is || (extension.CmpNoCase(".avi") == 0) || (extension.CmpNoCase(".mpg") == 0) ||
	(extension.CmpNoCase(".m2v") == 0) || (extension.CmpNoCase(".wmv") == 0) ||
	(extension.CmpNoCase(".asf") == 0) || (extension.CmpNoCase(".mov") == 0) ||
	(extension.CmpNoCase(".bin") == 0) || (extension.CmpNoCase(".swf") == 0) ||
	(extension.CmpNoCase(".ogm") == 0));

	extension = GetFileName().Right(3);
	it_is = (it_is || (extension.CmpNoCase(".rm") == 0) || (extension.CmpNoCase(".qt") == 0));
	return (it_is);
}

bool CPartFile::IsArchive()
{
	bool it_is;
	wxString extension = GetFileName().Right(4);
	it_is = ((extension.CmpNoCase(".zip") == 0) || (extension.CmpNoCase(".rar") == 0) ||
	(extension.CmpNoCase(".ace") == 0) || (extension.CmpNoCase(".arj") == 0) ||
	(extension.CmpNoCase(".lhz") == 0) || (extension.CmpNoCase(".tar") == 0) ||
	(extension.CmpNoCase(".bz2") == 0));

	extension = GetFileName().Right(3);
	it_is = (it_is || (extension.CmpNoCase(".gz") == 0) || (extension.CmpNoCase(".bz") == 0));
	return (it_is);
}

bool CPartFile::IsSound()
{
	bool it_is;
	wxString extension = GetFileName().Right(4);
	it_is = ((extension.CmpNoCase(".mp3") == 0) || (extension.CmpNoCase(".mp2") == 0) ||
	(extension.CmpNoCase(".wma") == 0) || (extension.CmpNoCase(".ogg") == 0) ||
	(extension.CmpNoCase(".rma") == 0) || (extension.CmpNoCase(".wav") == 0) ||
	(extension.CmpNoCase(".mid") == 0));
	return (it_is);
}

bool CPartFile::IsCDImage()
{
	bool it_is;
	wxString extension = GetFileName().Right(4);
	it_is = ((extension.CmpNoCase(".bin") == 0) || (extension.CmpNoCase(".cue") == 0) ||
	(extension.CmpNoCase(".nrg") == 0) || (extension.CmpNoCase(".ccd") == 0) ||
	(extension.CmpNoCase(".img") == 0) || (extension.CmpNoCase(".iso") == 0));
	return (it_is);
}

bool CPartFile::IsImage()
{
	bool it_is;
	wxString extension = GetFileName().Right(5);
	it_is = (extension.CmpNoCase(".jpeg") == 0) || (extension.CmpNoCase(".tiff") == 0);
	extension = GetFileName().Right(4);
	it_is = (it_is || (extension.CmpNoCase(".jpg") == 0) || (extension.CmpNoCase(".gif") == 0) ||
	(extension.CmpNoCase(".bmp") == 0) || (extension.CmpNoCase(".rle") == 0) ||
	(extension.CmpNoCase(".psp") == 0) || (extension.CmpNoCase(".tga") == 0) ||
	(extension.CmpNoCase(".wmf") == 0) || (extension.CmpNoCase(".xpm") == 0) ||
	(extension.CmpNoCase(".png") == 0) || (extension.CmpNoCase(".pcx") == 0));
	return (it_is);
}

bool CPartFile::IsText()
{
	bool it_is;
	wxString extension = GetFileName().Right(5);
	it_is = (extension.CmpNoCase(".html") == 0);
	extension = GetFileName().Right(4);
	it_is = (it_is || (extension.CmpNoCase(".doc") == 0) || (extension.CmpNoCase(".txt") == 0) ||
	(extension.CmpNoCase(".pdf") == 0) || (extension.CmpNoCase(".ps") == 0) ||
	(extension.CmpNoCase(".htm") == 0) || (extension.CmpNoCase(".sxw") == 0) ||
	(extension.CmpNoCase(".log") == 0));
	return (it_is);
}


void CPartFile::SetDownPriority(uint8 np, bool bSave )
{
	m_iDownPriority = np;
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace();
	UpdateDisplayedInfo(true);
	if ( bSave )
		SavePartFile();
}

void CPartFile::StopFile(bool bCancel)
{
	// Barry - Need to tell any connected clients to stop sending the file
	stopped = true; // Kry - Need to set it here to get into SetPartFileStatus(status) correctly
	PauseFile();

	RemoveAllSources(true);
	paused = true;
	stopped=true;
#ifdef DOWNLOADRATE_FILTERED
	kBpsDown = 0.0;
#else
	datarate = 0;
#endif
	transferingsrc = 0;
	memset(m_anStates,0,sizeof(m_anStates));
	if (!bCancel) {
		FlushBuffer();
	}
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace();
	UpdateDisplayedInfo(true);
}

void CPartFile::StopPausedFile()
{
	//Once an hour, remove any sources for files which are no longer active downloads
	UINT uState = GetStatus();
	if( (uState==PS_PAUSED || uState==PS_INSUFFICIENT || uState==PS_ERROR) && !stopped && time(NULL) - m_iLastPausePurge > (60*60)) {
		StopFile();
	}
}

void CPartFile::PauseFile(bool bInsufficient)
{
	m_iLastPausePurge = time(NULL);
	theApp.downloadqueue->RemoveLocalServerRequest(this);

	if (status==PS_COMPLETE || status==PS_COMPLETING) {
		return;
	}

	Packet* packet = new Packet(OP_CANCELTRANSFER,0);
	for (int sl=0;sl<SOURCESSLOTS;sl++) {
		if (!srclists[sl].IsEmpty()) {
			for( POSITION pos = srclists[sl].GetHeadPosition(); pos != NULL;) {
				CUpDownClient* cur_src = srclists[sl].GetNext(pos);
				if (cur_src->GetDownloadState() == DS_DOWNLOADING) {
					if (!cur_src->GetSentCancelTransfer()) {				
						theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
						cur_src->socket->SendPacket(packet,false,true);
						cur_src->SetDownloadState(DS_ONQUEUE);
						cur_src->SetSentCancelTransfer(1);
					}
					cur_src->SetDownloadState(DS_ONQUEUE);
				}
			}
		}
	}
	delete packet;

	if (bInsufficient) {
		insufficient = true;
	} else {
		paused = true;
		insufficient = false;
	}
	
#ifdef DOWNLOADRATE_FILTERED
	kBpsDown = 0.0;
#else
	datarate = 0;
#endif
	transferingsrc = 0;
	m_anStates[DS_DOWNLOADING] = 0;
	
	SetStatus(status);
	
	if (!bInsufficient) {
		theApp.downloadqueue->SortByPriority();
		theApp.downloadqueue->CheckDiskspace();
		SavePartFile();
	}

}

void CPartFile::ResumeFile()
{
	
	if (status==PS_COMPLETE || status==PS_COMPLETING) {
		return;
	}
	
	paused = false;
	stopped = false;
	lastsearchtime = 0;
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace();
	SavePartFile();
	//SetPartFileStatus(status);
	UpdateDisplayedInfo(true);
	
}

CString CPartFile::getPartfileStatus()
{
	CString mybuffer=""; 
	if (GetTransferingSrcCount()>0) {
		mybuffer=CString(_("Downloading"));
	}	else {
		mybuffer=CString(_("Waiting"));
	}
	switch (GetStatus()) {
		case PS_HASHING: 
		case PS_WAITINGFORHASH:
			mybuffer=CString(_("Hashing"));
			break; 
		case PS_COMPLETING:
			mybuffer=CString(_("Completing"));
			break; 
		case PS_COMPLETE:
			mybuffer=CString(_("Complete"));
			break; 
		case PS_PAUSED:
			mybuffer=CString(_("Paused"));
			break; 
		case PS_ERROR:
			mybuffer=CString(_("Erroneous"));
			break;
	} 
	if (stopped && (GetStatus()!=PS_COMPLETE)) {
		mybuffer=CString(_("Stopped"));
	}
	return mybuffer; 
} 

int CPartFile::getPartfileStatusRang()
{
	
	int tempstatus=0;
	if (GetTransferingSrcCount()==0) tempstatus=1;
	switch (GetStatus()) {
		case PS_HASHING: 
		case PS_WAITINGFORHASH:
			tempstatus=3;
			break; 
		case PS_COMPLETING:
			tempstatus=4;
			break; 
		case PS_COMPLETE:
			tempstatus=5;
			break; 
		case PS_PAUSED:
			tempstatus=2;
			break; 
		case PS_ERROR:
			tempstatus=6;
			break;
	} 
	return tempstatus;
} 

sint32 CPartFile::getTimeRemaining()
{
#ifdef DOWNLOADRATE_FILTERED
	if (GetKBpsDown() < 0.001)
		return -1;
	else 
		return((GetFileSize()-GetCompletedSize()) / ((int)(GetKBpsDown()*1024.0)));
#else
	if (GetDatarate()==0)
		return -1;
	return((GetFileSize()-GetCompletedSize()) / GetDatarate());
#endif
} 

void CPartFile::PreviewFile()
{
	wxString command;

	// If no player set in preferences, use mplayer.
	if (theApp.glob_prefs->GetVideoPlayer()=="") {
		command.Append(wxT("mplayer"));
	} else {
		command.Append(theApp.glob_prefs->GetVideoPlayer());
	}
	// Need to use quotes in case filename contains spaces.
	command.Append(wxT(" \""));
	command.Append(GetFullName());
	
	if (!(GetStatus() == PS_COMPLETE)) {
		// Remove the .met from filename.
		for (int i=0;i<4;i++) {
			command.RemoveLast();
		}
	}
	
	command.Append(wxT("\""));
     wxShell(command.c_str());
}

bool CPartFile::PreviewAvailable()
{
	return (IsMovie() && IsComplete(0, PARTSIZE));
}

#if 0
bool CPartFile::PreviewAvailable()
{
	wxLongLong free;
	wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), NULL, &free);
	printf("\nFree Space (wxLongLong): %s\n", free.ToString().GetData());
	typedef unsigned long long uint64;
	uint64 space = free.GetValue();
	printf("\nFree Space (uint64): %lli\n", space);

	// Barry - Allow preview of archives of any length > 1k
	if (IsArchive(true)) {
		if (GetStatus() != PS_COMPLETE && GetStatus() != PS_COMPLETING && GetFileSize()>1024 && GetCompletedSize()>1024 && (!m_bRecoveringArchive) && ((space + 100000000) > (2*GetFileSize()))) {
			return true;
		} else {
			return false;
		}
	}
	if (theApp.glob_prefs->IsMoviePreviewBackup()) {
		return !( (GetStatus() != PS_READY && GetStatus() != PS_PAUSED)
		|| m_bPreviewing || GetPartCount() < 5 || !IsMovie() || (space + 100000000) < GetFileSize() 
		|| ( !IsComplete(0,PARTSIZE-1) || !IsComplete(PARTSIZE*(GetPartCount()-1),GetFileSize()-1)));
	} else {
		TCHAR szVideoPlayerFileName[_MAX_FNAME];
		_tsplitpath(theApp.glob_prefs->GetVideoPlayer(), NULL, NULL, szVideoPlayerFileName, NULL);

		// enable the preview command if the according option is specified 'PreviewSmallBlocks' 
		// or if VideoLAN client is specified
		if (theApp.glob_prefs->GetPreviewSmallBlocks() || !_tcsicmp(szVideoPlayerFileName, _T("vlc"))) {
			if (m_bPreviewing) {
				return false;
			}
			uint8 uState = GetStatus();
			if (!(uState == PS_READY || uState == PS_EMPTY || uState == PS_PAUSED)) {
				return false;
			}
			// default: check the ED2K file format to be of type audio, video or CD image. 
			// but because this could disable the preview command for some file types which eMule does not know,
			// this test can be avoided by specifying 'PreviewSmallBlocks=2'
			if (theApp.glob_prefs->GetPreviewSmallBlocks() <= 1) {
				// check the file extension
				EED2KFileType eFileType = GetED2KFileTypeID(GetFileName());
				if (!(eFileType == ED2KFT_VIDEO || eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_CDIMAGE)) {
					// check the ED2K file type
					LPCSTR pszED2KFileType = GetStrTagValue(FT_FILETYPE);
					if (pszED2KFileType == NULL || !(!stricmp(pszED2KFileType, "Audio") || !stricmp(pszED2KFileType, "Video"))) {
						return false;
					}
				}
			}

			// If it's an MPEG file, VLC is even capable of showing parts of the file if the beginning of the file is missing!
			bool bMPEG = false;
			LPCTSTR pszExt = _tcsrchr(GetFileName(), _T('.'));
			if (pszExt != NULL) {
				CString strExt(pszExt);
				strExt.MakeLower();
				bMPEG = (strExt==_T(".mpg") || strExt==_T(".mpeg") || strExt==_T(".mpe") || strExt==_T(".mp3") || strExt==_T(".mp2") || strExt==_T(".mpa"));
			}
			if (bMPEG) {
				// TODO: search a block which is at least 16K (Audio) or 256K (Video)
				if (GetCompletedSize() < 16*1024) {
					return false;
				}
			} else {
				// For AVI files it depends on the used codec..
				if (!IsComplete(0, 256*1024))
					return false;
				}
			}
			return true;
		} else {
			return !((GetStatus() != PS_READY && GetStatus() != PS_PAUSED)
			|| m_bPreviewing || GetPartCount() < 2 || !IsMovie() || !IsComplete(0,PARTSIZE-1));
		}
	}
}
#endif

void CPartFile::UpdateAvailablePartsCount()
{
	uint8 availablecounter = 0;
	bool breakflag = false;
	uint16 iPartCount = GetPartCount();
	for (uint32 ixPart = 0; ixPart < iPartCount; ixPart++) {
		breakflag = false;
		for (uint32 sl = 0; sl < SOURCESSLOTS && !breakflag; sl++) {
			if (!srclists[sl].IsEmpty()) {
				for(POSITION pos = srclists[sl].GetHeadPosition(); pos && !breakflag;) {
					if (srclists[sl].GetNext(pos)->IsPartAvailable(ixPart)) {
						availablecounter++;
						breakflag = true;
					}
				}
			}
		}
	}
	if (iPartCount == availablecounter && availablePartsCount < iPartCount) {
		lastseencomplete = time(NULL);//CTime::GetCurrentTime();
	}
	availablePartsCount = availablecounter;
}

Packet*	CPartFile::CreateSrcInfoPacket(CUpDownClient* forClient)
{
	int sl;
	for (sl=0;sl<SOURCESSLOTS;sl++) if (srclists[sl].IsEmpty()) return 0;

	CSafeMemFile data(1024);
	uint16 nCount = 0;

	data.WriteHash16(m_abyFileHash);
	data.Write(nCount);
	bool bNeeded;
	for (sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty())
	for (POSITION pos = srclists[sl].GetHeadPosition();pos != 0;srclists[sl].GetNext(pos)) {
		bNeeded = false;
		CUpDownClient* cur_src = srclists[sl].GetAt(pos);
		if (cur_src->HasLowID() || !cur_src->IsValidSource()) {
			continue;
		}
		// only send source which have needed parts for this client if possible
		uint8* srcstatus = cur_src->GetPartStatus();
		if (srcstatus) {
			uint8* reqstatus = forClient->GetPartStatus();
			if (reqstatus) {
				// only send sources which have needed parts for this client
				for (int x = 0; x < GetPartCount(); x++) {
					if (srcstatus[x] && !reqstatus[x]) {
						bNeeded = true;
						break;
					}
				}
			} else {
				// if we don't know the need parts for this client, return any source
				// currently a client sends it's file status only after it has at least one complete part,
				for (int x = 0; x < GetPartCount(); x++){
					if (srcstatus[x]) {
						bNeeded = true;
						break;
					}
				}
			}
		}
		if(bNeeded) {
			nCount++;
			uint32 dwID;
			#warning We should use the IDHybrid here... but is not implemented yet
			if(forClient->GetSourceExchangeVersion() > 2) {
				dwID = cur_src->GetUserID();
			} else {
				dwID = ntohl(cur_src->GetUserID());
			}
			data.Write(dwID);
			data.Write((uint16)cur_src->GetUserPort());
			data.Write((uint32)cur_src->GetServerIP());
			data.Write((uint16)cur_src->GetServerPort());
			if (forClient->GetSourceExchangeVersion()>1) {
				data.WriteHash16(cur_src->GetUserHash());
			}
			if (nCount > 500) {
				break;
			}
		}
	}
	if (!nCount) {
		return 0;
	}
	data.Seek(16,wxFromStart);
	data.Write(nCount);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->opcode = OP_ANSWERSOURCES;
	// 16+2+501*(4+2+4+2+16) = 14046 bytes max.
	if (result->size > 354) {
		result->PackPacket();
	}
	//if (thePrefs.GetDebugSourceExchange()) {
		theApp.amuledlg->AddDebugLogLine( false, "Send:Source User(%s) File(%s) Count(%i)", forClient->GetUserName(), GetFileName().GetData(), nCount );
	//}
	return result;
}

void CPartFile::AddClientSources(CMemFile* sources,uint8 sourceexchangeversion)
{
	if(stopped) {
		return;
	}
	uint16 nCount;
	sources->Read(nCount);
	for (int i = 0;i != nCount;i++) {
		uint32 dwID;
		uint16 nPort;
		uint32 dwServerIP;
		uint16 nServerPort;
		uchar achUserHash[16];
		sources->Read(dwID);
		sources->Read(nPort);
		sources->Read(dwServerIP);
		sources->Read(nServerPort);
		if (sourceexchangeversion > 1) {
			sources->ReadRaw(achUserHash,16);
		}
		// check first if we are this source
		if (theApp.serverconnect->GetClientID() < 16777216 && theApp.serverconnect->IsConnected()) {
			if ((theApp.serverconnect->GetClientID() == dwID) && theApp.serverconnect->GetCurrentServer()->GetIP() == dwServerIP) {
				continue;
			}
		} else if (theApp.serverconnect->GetClientID() == dwID) {
			continue;
		} else if (dwID < 16777216) {
			continue;
		}
		if(theApp.glob_prefs->GetMaxSourcePerFile() > this->GetSourceCount()) {
			CUpDownClient* newsource = new CUpDownClient(nPort,dwID,dwServerIP,nServerPort,this);
			if (sourceexchangeversion > 1) {
				newsource->SetUserHash(achUserHash);
			}
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		} else {
			break;
		}
	}
}

void CPartFile::UpdateAutoDownPriority()
{
	if (!IsAutoDownPriority()) {
		return;
	}
	if (GetSourceCount() <= RARE_FILE) {
		if ( GetDownPriority() != PR_HIGH )
			SetDownPriority(PR_HIGH, false);
	} else if (GetSourceCount() < 100) {
		if ( GetDownPriority() != PR_NORMAL )
			SetDownPriority(PR_NORMAL, false);
	} else {
		if ( GetDownPriority() != PR_LOW )
			SetDownPriority(PR_LOW, false);
	}
}

// making this function return a higher when more sources have the extended
// protocol will force you to ask a larger variety of people for sources

int CPartFile::GetCommonFilePenalty()
{
	//TODO: implement, but never return less than MINCOMMONPENALTY!
	return MINCOMMONPENALTY;
}

/* Barry - Replaces BlockReceived() 

	Originally this only wrote to disk when a full 180k block
	had been received from a client, and only asked for data in
	180k blocks.

	This meant that on average 90k was lost for every connection
	to a client data source. That is a lot of wasted data.

	To reduce the lost data, packets are now written to a buffer
	and flushed to disk regularly regardless of size downloaded.
	This includes compressed packets.

	Data is also requested only where gaps are, not in 180k blocks.
	The requests will still not exceed 180k, but may be smaller to
	fill a gap.
*/

uint32 CPartFile::WriteToBuffer(uint32 transize, BYTE *data, uint32 start, uint32 end, Requested_Block_Struct *block)
{
	// Increment transfered bytes counter for this file
	transfered += transize;

	// This is needed a few times
	uint32 lenData = end - start + 1;

	if(lenData > transize) {
		m_iGainDueToCompression += lenData-transize;
	}

	// Occasionally packets are duplicated, no point writing it twice
	if (IsComplete(start, end)) {
		theApp.amuledlg->AddDebugLogLine(false, "File '%s' has already been written from %ld to %ld\n", GetFileName().GetData(), start, end);
		return 0;
	}

	// Create copy of data as new buffer
	BYTE *buffer = new BYTE[lenData];
	memcpy(buffer, data, lenData);

	// Create a new buffered queue entry
	PartFileBufferedData *item = new PartFileBufferedData;
	item->data = buffer;
	item->start = start;
	item->end = end;
	item->block = block;

	// Add to the queue in the correct position (most likely the end)
	PartFileBufferedData *queueItem;
	bool added = false;
	POSITION pos = m_BufferedData_list.GetTailPosition();
	while (pos != NULL) {
		queueItem = m_BufferedData_list.GetPrev(pos);
		if (item->end > queueItem->end) {
			added = true;
			m_BufferedData_list.InsertAfter(pos, item);
			break;
		}
	}
	if (!added) {
		m_BufferedData_list.AddHead(item);
	}

	// Increment buffer size marker
	m_nTotalBufferData += lenData;

	// Mark this small section of the file as filled
	FillGap(item->start, item->end);

	// Update the flushed mark on the requested block 
	// The loop here is unfortunate but necessary to detect deleted blocks.
	pos = requestedblocks_list.GetHeadPosition();
	while (pos != NULL) {	
		if (requestedblocks_list.GetNext(pos) == item->block) {
			item->block->transferred += lenData;
		}
	}

	if (gaplist.IsEmpty()) {
		FlushBuffer();
	}

	// Return the length of data written to the buffer
	return lenData;
}

void CPartFile::FlushBuffer(void)
{
	m_nLastBufferFlushTime = GetTickCount();
	
	if (m_BufferedData_list.IsEmpty()) {
		return;
	}

	/*
	Madcat - Check if there is at least PARTSIZE amount of free disk space
	in temp dir before flushing. If not enough space, pause the file,
	add log line and abort flushing.
	*/
	wxLongLong total, free;
	if (wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free) && free < PARTSIZE) {
		theApp.amuledlg->AddLogLine(true, CString(_("ERROR: Cannot write to disk")));
		PauseFile();
		return;
	}
	
	uint32 partCount = GetPartCount();
	bool *changedPart = new bool[partCount];
	
	try {
		// Remember which parts need to be checked at the end of the flush
		for (int partNumber=0; (uint32)partNumber<partCount; partNumber++) {
			changedPart[partNumber] = false;
		}

		bool bCheckDiskspace = theApp.glob_prefs->IsCheckDiskspaceEnabled() && theApp.glob_prefs->GetMinFreeDiskSpace() > 0;

		wxLongLong total, free;
		wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free);
		typedef unsigned long long uint64;
		// WHY IS THIS NOT USED? ... //uint64 GetFreeDiskSpaceX = free.GetValue();
		// WHY IS THIS NOT USED? ... //ULONGLONG uFreeDiskSpace = bCheckDiskspace ? GetFreeDiskSpaceX : 0;

		// Ensure file is big enough to write data to (the last item will be the furthest from the start)
		PartFileBufferedData *item = m_BufferedData_list.GetTail();
		if ((unsigned)m_hpartfile.Length() <= item->end) {
			//m_hpartfile.SetLength(item->end + 1);
			#ifdef __WXMSW__
			chsize(m_hpartfile.fd(),item->end+1);
			#else
			ftruncate(m_hpartfile.fd(),item->end+1);
			#endif
		}
		
		// Loop through queue
		for (int i = m_BufferedData_list.GetCount(); i>0; i--) {
			if (i<1) { // Can this happen ? 
				return; // Well, if it did... abort then.
			}
			// Get top item
			item = m_BufferedData_list.GetHead();

			// This is needed a few times
			uint32 lenData = item->end - item->start + 1;

			// SLUGFILLER: SafeHash - could be more than one part
			for (uint32 curpart = item->start/PARTSIZE; curpart <= item->end/PARTSIZE; curpart++)
				changedPart[curpart] = true;
			// SLUGFILLER: SafeHash		
			
			// Go to the correct position in file and write block of data			
			m_hpartfile.Seek(item->start);
			m_hpartfile.Write(item->data, lenData);
			
			// Remove item from queue
			m_BufferedData_list.RemoveHead();

			// Decrease buffer size
			m_nTotalBufferData -= lenData;

			// Release memory used by this item
			delete [] item->data;
			delete item;
		}

		// Flush to disk
		m_hpartfile.Flush();

		// Check each part of the file
		uint32 partRange = (m_hpartfile.GetLength() % PARTSIZE) - 1;
		for (int partNumber = partCount-1; partNumber >= 0; partNumber--) {
			if (changedPart[partNumber] == false) {
				// Any parts other than last must be full size
				partRange = PARTSIZE - 1;
				continue;
			}

			// Is this 9MB part complete
			if (IsComplete(PARTSIZE * partNumber, (PARTSIZE * (partNumber + 1)) - 1)) {
				// Is part corrupt
				if (!HashSinglePart(partNumber)) {
					theApp.amuledlg->AddLogLine(true, CString(_("Downloaded part %i is corrupt :(  (%s)")), partNumber, GetFileName().GetData());
					AddGap(PARTSIZE*partNumber, (PARTSIZE*partNumber + partRange));
					corrupted_list.AddTail(partNumber);
					// Reduce transfered amount by corrupt amount
					this->m_iLostDueToCorruption += (partRange + 1);
				} else {
					if (!hashsetneeded) {
						theApp.amuledlg->AddDebugLogLine(false, "Finished part %u of \"%s\"", partNumber, GetFileName().GetData());
					}
					
					// Successfully completed part, make it available for sharing				
					if (status == PS_EMPTY) {
						SetStatus(PS_READY);
						theApp.sharedfiles->SafeAddKFile(this);
					}
				}
			} else if ( IsCorruptedPart(partNumber) && theApp.glob_prefs->IsICHEnabled()) {
				// Try to recover with minimal loss
				if (HashSinglePart(partNumber)) {
					m_iTotalPacketsSavedDueToICH++;
					FillGap(PARTSIZE*partNumber,(PARTSIZE*partNumber+partRange));
					RemoveBlockFromList(PARTSIZE*partNumber,(PARTSIZE*partNumber + partRange));
					theApp.amuledlg->AddLogLine(true,CString(_("ICH: Recovered corrupted part %i  (%s)")),partNumber,GetFileName().GetData());
				}
			}
			// Any parts other than last must be full size
			partRange = PARTSIZE - 1;
		}

		// Update met file
		SavePartFile();

		if (theApp.amuledlg->IsRunning()) { // may be called during shutdown!
			// Is this file finished ?
			if (gaplist.IsEmpty()) {
				CompleteFile(false);
			}

			// Check free diskspace
			//
			// Checking the free disk space again after the file was written could most likely be avoided, but because
			// we do not use real physical disk allocation units for the free disk computations, it should be more safe
			// and accurate to check the free disk space again, after file was written and buffers were flushed to disk.
			//
			// If useing a normal file, we could avoid the check disk space if the file was not increased.
			// If useing a compressed or sparse file, we always have to check the space 
			// regardless whether the file was increased in size or not.
			if (bCheckDiskspace) {
				switch(GetStatus()) {
					case PS_PAUSED:
					case PS_ERROR:
					case PS_COMPLETING:
					case PS_COMPLETE:
						break;
					default: {
						wxLongLong total, free;
						wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free);
						typedef unsigned long long uint64;
						uint64 GetFreeDiskSpaceX = free.GetValue();
						if (GetFreeDiskSpaceX < (unsigned)theApp.glob_prefs->GetMinFreeDiskSpace()) {
							// Normal files: pause the file only if it would still grow
							uint32 nSpaceToGrow = GetNeededSpace();
							if (nSpaceToGrow) {
								PauseFile(true/*bInsufficient*/);
							}
						}
					}
				}
			}
		}
	}
	catch(...) {
		theApp.amuledlg->AddLogLine(true, CString(_("Unexpected file error while writing %s : %s")), GetFileName().GetData(), CString(_("Unknown")).GetData());
		SetPartFileStatus(PS_ERROR);
		paused = true;
		m_iLastPausePurge = time(NULL);
		theApp.downloadqueue->RemoveLocalServerRequest(this);
#ifdef DOWNLOADRATE_FILTERED
		kBpsDown = 0.0;
#else
		datarate = 0;
#endif
		transferingsrc = 0;
		if (theApp.amuledlg->IsRunning()) { // may be called during shutdown!
			UpdateDisplayedInfo();
		}
	}
	delete[] changedPart;

}

// Barry - This will invert the gap list, up to caller to delete gaps when done
// 'Gaps' returned are really the filled areas, and guaranteed to be in order

void CPartFile::GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled)
{
	Gap_Struct *gap;
	Gap_Struct *best;
	POSITION pos;
	uint32 start = 0;
	uint32 bestEnd = 0;

	// Loop until done
	bool finished = false;
	while (!finished) {
		finished = true;
		// Find first gap after current start pos
		bestEnd = m_nFileSize;
		pos = gaplist.GetHeadPosition();
		while (pos != NULL) {
			gap = gaplist.GetNext(pos);
			if ((gap->start > start) && (gap->end < bestEnd)) {
				best = gap;
				bestEnd = best->end;
				finished = false;
			}
		}

		if (!finished) {
			// Invert this gap
			gap = new Gap_Struct;
			gap->start = start;
			gap->end = best->start - 1;
			start = best->end + 1;
			filled->AddTail(gap);
		} else if (best->end < m_nFileSize) {
			gap = new Gap_Struct;
			gap->start = best->end + 1;
			gap->end = m_nFileSize;
			filled->AddTail(gap);
		}
	}
}

void CPartFile::UpdateFileRatingCommentAvail()
{
	if (!this) {
		return;
	}

	bool prev=(hasComment || hasRating);
	bool prevbad=hasBadRating;

	hasComment=false;
	int badratings=0;
	int ratings=0;

	POSITION pos1,pos2;
	for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty()) {
		for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;) {
			srclists[sl].GetNext(pos1);
			CUpDownClient* cur_src = srclists[sl].GetAt(pos2);

			if (cur_src->GetFileComment().GetLength()>0) {
				hasComment=true;
			}

			if (cur_src->GetFileRate()>0) {
				ratings++;
			}
			if (cur_src->GetFileRate()==1) {
				badratings++;
			}
		}
	}
	hasBadRating=(badratings> (ratings/3));
	hasRating=(ratings>0);
	if ((prev!=(hasComment || hasRating)) || (prevbad!=hasBadRating)) {
		UpdateDisplayedInfo();
	}
}

uint16 CPartFile::GetSourceCount()
{
	if (!IsCountDirty) {
		return CleanCount;
	} else {
		int count=0;
		for (int i=0;i<SOURCESSLOTS;i++) {
			count+=srclists[i].GetCount();
		}
		CleanCount = count;
		IsCountDirty = false;
		return (uint16) count;
	}
}

void CPartFile::UpdateDisplayedInfo(bool force)
{
	DWORD curTick = ::GetTickCount();

	if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000))) {
		theApp.amuledlg->transferwnd->downloadlistctrl->UpdateItem(this);
		m_lastRefreshedDLDisplay = curTick;
	}
	
}

time_t CPartFile::GetLastChangeDatetime(bool forcecheck)
{
	if ((::GetTickCount()-m_lastdatetimecheck)<60000 && !forcecheck) {
		return m_lastdatecheckvalue;
	}

	m_lastdatetimecheck=::GetTickCount();
	if (!::wxFileExists(m_hpartfile.GetFilePath())) {
		m_lastdatecheckvalue=-1;
	} else {
		//CFileStatus filestatus;
		struct stat filestatus;
		fstat(m_hpartfile.fd(),&filestatus);
		//m_hpartfile.GetStatus(filestatus); // this; "...returns m_attribute without high-order flags" indicates a known MFC bug, wonder how many unknown there are... :)
		m_lastdatecheckvalue=filestatus.st_mtime;
	}
	return m_lastdatecheckvalue;
}

uint8 CPartFile::GetCategory()
{
	if(m_category>theApp.glob_prefs->GetCatCount()-1) {
		m_category=0;
	}
	return m_category;
}

CString CPartFile::GetDownloadFileInfo()
{
	if(this == NULL) {
		return "";
	}

	CString sRet;
	CString strHash=EncodeBase16(GetFileHash(), 16);;
	char lsc[50]; char complx[50]; char lastprogr[50];
	sprintf(complx,"%s/%s",CastItoXBytes(GetCompletedSize()).GetData(),CastItoXBytes(GetFileSize()).GetData());
	if (lastseencomplete==0) {
		sprintf(lsc, CString(_("Unknown")).MakeLower()); 
	} else {
		strftime(lsc,sizeof(lsc),theApp.glob_prefs->GetDateTimeFormat(),localtime((time_t*)&lastseencomplete));
	}
	
	if (GetFileDate()==0) {
		sprintf(lastprogr, CString(_("Unknown")));
	} else {
		strftime(lastprogr,sizeof(lsc),theApp.glob_prefs->GetDateTimeFormat(),localtime(GetCFileDate()));
	}

	float availability = 0;
	if(GetPartCount() != 0) {
		availability = GetAvailablePartCount() * 100 / GetPartCount();
	}

	sRet.Format(CString(_("File Name"))+": %s (%s %s)\n\n%s\n\n"
	+CString(_("Hash :")) +" %s\n"+CString(_("Partfilename: %s\nParts: %d , %s: %d (%.1f%%)\n"))+
	CString(_("%d%% done (%s) - Transferring from %d sources"))+"\n%s\n%s", GetFileName().GetData(), CastItoXBytes(GetFileSize()).GetData(),
	CString(_("Bytes")).GetData(),(CString(_("Status"))+": "+getPartfileStatus()).GetData(),
	strHash.GetData(),GetPartMetFileName(), GetPartCount(),CString(_("Available")).GetData(),
	GetAvailablePartCount(),availability,(int)GetPercentCompleted(), complx, GetTransferingSrcCount(),
	(CString(_("Last Seen Complete :"))+" "+wxString(lsc)).GetData(),(CString(_("Last Reception:"))
	+" "+wxString(lastprogr)).GetData());

        return sRet;
}

wxString CPartFile::GetProgressString(uint16 size)
{
	char crProgress = '0';	//green
	char crHave = '1';	// black
	char crPending='2';	// yellow
	//char crWaiting='3';	// blue
	//char crMissing='4';	// red
	//added lemonfan's progressbar patch
	char crMissing='3';	// red
	
	char crWaiting[6];
	crWaiting[0]='4'; // blue few source
	crWaiting[1]='5';
	crWaiting[2]='6';
	crWaiting[3]='7';
	crWaiting[4]='8';
	crWaiting[5]='9'; // full sources

	wxString my_ChunkBar="";

	for (uint16 i=0;i<=size+1;i++) {
		my_ChunkBar.Append(crHave,1); //.AppendChar(crHave);
	}
	// one more for safety

	float unit= (float)size/(float)m_nFileSize;
	uint32 allgaps = 0;

	if(GetStatus() == PS_COMPLETE || GetStatus() == PS_COMPLETING) {  
		CharFillRange(&my_ChunkBar,0,(uint32)(m_nFileSize*unit), crProgress);
	} else {	
		// red gaps
		for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;gaplist.GetNext(pos)) {
			Gap_Struct* cur_gap = gaplist.GetAt(pos);
			allgaps += cur_gap->end - cur_gap->start + 1;
			bool gapdone = false;
			uint32 gapstart = cur_gap->start;
			uint32 gapend = cur_gap->end;
			for (uint32 i = 0; i < GetPartCount(); i++){
				if (gapstart >= i*PARTSIZE && gapstart <=  (i+1)*PARTSIZE) { // is in this part?
					if (gapend <= (i+1)*PARTSIZE) {
						gapdone = true;
					} else {
						gapend = (i+1)*PARTSIZE; // and next part
					}
					// paint
					uint8 color;
					if (m_SrcpartFrequency.GetCount() >= (size_t)i && m_SrcpartFrequency[i]) {  // frequency?
						//color = crWaiting;
						//added lemonfan's progressbar patch
						color = m_SrcpartFrequency[i] < 10 ? crWaiting[m_SrcpartFrequency[i]/2]:crWaiting[5];
					} else {
						color = crMissing;
					}
					CharFillRange(&my_ChunkBar,(uint32)(gapstart*unit), (uint32)(gapend*unit + 1),  color);
					if (gapdone) { // finished?
						break;
					} else {
						gapstart = gapend;
						gapend = cur_gap->end;
					}
				}
			}
		}
	}

	// yellow pending parts
	for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0;requestedblocks_list.GetNext(pos)) {
		Requested_Block_Struct* block =  requestedblocks_list.GetAt(pos);
		CharFillRange(&my_ChunkBar, (uint32)((block->StartOffset + block->transferred)*unit),(uint32)(block->EndOffset*unit),crPending);
	}
	return my_ChunkBar;
}
                                                                                
void CPartFile::CharFillRange(wxString* buffer,uint32 start, uint32 end, char color)
{
	for (uint32 i = start;i <= end;i++) {
		buffer->SetChar(i,color);
	}
}

/* Razor 1a - Modif by MikaelB
   RemoveNoNeededSources function */

void CPartFile::RemoveNoNeededSources()
{
	POSITION position, temp_position;

	for (int slot = 0;slot < SOURCESSLOTS;slot++) {
		if (! srclists[slot].IsEmpty()) {
			position = srclists[slot].GetHeadPosition();
			while (position != NULL) {
				temp_position = position;
				srclists[slot].GetNext(position);
				CUpDownClient* client = srclists[slot].GetAt(temp_position);
				if (client->GetDownloadState() == DS_NONEEDEDPARTS) {
					/* If allowed, try to swap to other file. If swapping fails, remove from this one. */
					if (theApp.glob_prefs->SwapNoNeededSources()) {
						if (!client->SwapToAnotherFile(true, true, true, NULL)) {
							theApp.downloadqueue->RemoveSourceFromPartFile(this, client, temp_position);
						}
					/* If not allowed to swap, simply remove from this one. */
					} else {
						theApp.downloadqueue->RemoveSourceFromPartFile(this, client, temp_position);
					}
				}
			}
		}
	}
}
/* End modif */

/* Razor 1a - Modif by MikaelB
   RemoveFullQueueSources function */

void CPartFile::RemoveFullQueueSources()
{
	POSITION position, temp_position;

	for (int slot = 0;slot < SOURCESSLOTS;slot++) {
		if (! srclists[slot].IsEmpty()) {
			position = srclists[slot].GetHeadPosition();
			while (position != NULL) {
				temp_position = position;
				srclists[slot].GetNext(position);
				CUpDownClient* client = srclists[slot].GetAt(temp_position);
				if ((client->GetDownloadState() == DS_ONQUEUE) && (client->IsRemoteQueueFull())) {
					theApp.downloadqueue->RemoveSourceFromPartFile(this, client, temp_position);
				}
			}
		}
	}
}
/* End modif */

/* Razor 1a - Modif by MikaelB
   RemoveHighQueueRatingSources function */

void CPartFile::RemoveHighQueueRatingSources()
{
	POSITION position, temp_position;

	for (int slot = 0;slot < SOURCESSLOTS;slot++) {
		if (! srclists[slot].IsEmpty()) {
			position = srclists[slot].GetHeadPosition();
			while (position != NULL) {
				temp_position = position;
				srclists[slot].GetNext(position);
				CUpDownClient* client = srclists[slot].GetAt(temp_position);
				if ((client->GetDownloadState() == DS_ONQUEUE) && (client->GetRemoteQueueRank() > theApp.glob_prefs->HighQueueRanking())) {
					theApp.downloadqueue->RemoveSourceFromPartFile(this, client, temp_position);
				}
			}
		}
	}
}
/* End modif */

/* Razor 1a - Modif by MikaelB
   CleanUpSources function */

void CPartFile::CleanUpSources()
{
	POSITION position, temp_position;

	for (int slot = 0;slot < SOURCESSLOTS;slot++) {
		if (! srclists[slot].IsEmpty()) {
			position = srclists[slot].GetHeadPosition();
			while (position != NULL) {
				temp_position = position;
				srclists[slot].GetNext(position);
				CUpDownClient* client = srclists[slot].GetAt(temp_position);
				if (client->GetDownloadState() == DS_NONEEDEDPARTS) {
					if ((theApp.glob_prefs->DropNoNeededSources()) && (!client->SwapToAnotherFile(true, true, true, NULL))) {
						theApp.downloadqueue->RemoveSourceFromPartFile(this, client, temp_position);
					}
				}
				if ((client->GetDownloadState() == DS_ONQUEUE) && (client->IsRemoteQueueFull())) {
					theApp.downloadqueue->RemoveSourceFromPartFile(this, client, temp_position);
				} else if ((client->GetDownloadState() == DS_ONQUEUE) && (client->GetRemoteQueueRank() > theApp.glob_prefs->HighQueueRanking())) {
					theApp.downloadqueue->RemoveSourceFromPartFile(this, client, temp_position);
				}
			}
		}
	}
}
/* End modif */

/* Razor 1a - Modif by MikaelB
   AddDownloadingSource function */

void CPartFile::AddDownloadingSource(CUpDownClient* client)
{
	POSITION position = m_downloadingSourcesList.Find(client);
	if (position == NULL) {
		m_downloadingSourcesList.AddTail(client);
	}
}
/* End modif */

/* Razor 1a - Modif by MikaelB
   RemoveDownloadingSource function */

void CPartFile::RemoveDownloadingSource(CUpDownClient* client)
{
	POSITION position = m_downloadingSourcesList.Find(client); 
	if (position != NULL) {
		m_downloadingSourcesList.RemoveAt(position);
	}
}
/* End modif */

void CPartFile::SetPartFileStatus(uint8 newstatus)
{
	status=newstatus;
	if (theApp.glob_prefs->GetAllcatType()>1) {
		theApp.amuledlg->transferwnd->downloadlistctrl->Freeze();
		
		if (!CheckShowItemInGivenCat(this, theApp.amuledlg->transferwnd->downloadlistctrl->curTab)) {
			theApp.amuledlg->transferwnd->downloadlistctrl->HideFile(this);
		} else {
			theApp.amuledlg->transferwnd->downloadlistctrl->ShowFile(this);
		}
		
		theApp.amuledlg->transferwnd->downloadlistctrl->Thaw();

		theApp.amuledlg->transferwnd->downloadlistctrl->ShowFilesCount();
	}
	theApp.amuledlg->transferwnd->downloadlistctrl->InitSort();
}

void CPartFile::ResumeFileInsufficient()
{
	if (status==PS_COMPLETE || status==PS_COMPLETING) {
		return;
	}
	if (!insufficient) {
		return;
	}
	insufficient = false;
	lastsearchtime = 0;
	UpdateDisplayedInfo(true);
}

uint32 CPartFile::GetNeededSpace()
{
	if ((unsigned)m_hpartfile.Length() > GetFileSize()) {
		return 0;	// Shouldn't happen, but just in case
	}
	return GetFileSize()-m_hpartfile.Length();
}

void CPartFile::SetStatus(uint8 in)
{
	wxASSERT( in != PS_PAUSED && in != PS_INSUFFICIENT );
	
	status=in;
	if (theApp.amuledlg->IsRunning()) {
		if (theApp.amuledlg->transferwnd->downloadlistctrl->curTab==0) {
			theApp.amuledlg->transferwnd->downloadlistctrl->ChangeCategory(0);
		} else {
			UpdateDisplayedInfo(true);
		}
/* TODO!
		if (theApp.glob_prefs->ShowCatTabInfos()) {
			theApp.amuledlg->transferwnd->UpdateCatTabTitles();		
		}
*/
	}
}
