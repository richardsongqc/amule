// This file is part of aMule Project
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


#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "types.h"				// Needed for ints
#include "CMD4Hash.h"

#include <wx/string.h>			// Needed for wxString
#if wxCHECK_VERSION(2, 5, 2)
#	include <wx/arrstr.h>		// Needed for wxArrayString
#endif

#include <map>
#include <vector>


class wxConfigBase;
class wxWindow;


#define DEFAULT_COL_SIZE 65535

enum EViewSharedFilesAccess{
	vsfaEverybody = 0,
	vsfaFriends = 1,
	vsfaNobody = 2
};

// DO NOT EDIT VALUES like making a uint16 to uint32, or insert any value. ONLY append new vars
#pragma pack(1)
struct Preferences_Ext_Struct
{
	int8	version;
	unsigned char	userhash[16];
	WINDOWPLACEMENT EmuleWindowPlacement;
};
#pragma pack()


struct Category_Struct
{
	wxString	incomingpath;
	wxString	title;
	wxString	comment;
	DWORD		color;
	uint8		prio;
};


/**
 * Base-class for automatically loading and saving of preferences.
 *
 * The purpose of this class is to perform two tasks:
 * 1) To load and save a variable using wxConfig
 * 2) If nescecarry, to syncronize it with a widget
 *
 * This pure-virtual class servers as the base of all the Cfg types
 * defined below, and exposes the entire interface.
 *
 * Please note that for reasons of simplicity these classes dont provide
 * direct access to the variables they maintain, as there is no real need
 * for this.
 *
 * To create a sub-class you need only provide the Load/Save functionality,
 * as it is given that not all variables have a widget assosiated.
 */
class Cfg_Base
{
public:
	/**
	 * Constructor.
	 *
	 * @param keyname This is the keyname under which the variable is to be saved.
	 */
	Cfg_Base( const wxString& keyname )
	 : m_key( keyname ),
	   m_changed( false )
	{}

	/**
	 * Destructor.
	 */
	virtual ~Cfg_Base() {}

	/**
	 * This function loads the assosiated variable from the provided config object.
	 */
	virtual void LoadFromFile(wxConfigBase* cfg) = 0;
	/**
	 * This function saves the assosiated variable to the provided config object.
	 */
	virtual void SaveToFile(wxConfigBase* cfg) = 0;

	/**
	 * Syncs the variable with the contents of the widget.
	 *
	 * @return True of success, false otherwise.
	 */
	virtual bool TransferFromWindow()	{ return false; }
	/**
	 * Syncs the widget with the contents of the variable.
	 *
	 * @return True of success, false otherwise.
	 */
	virtual bool TransferToWindow()		{ return false; }

	/**
	 * Connects a widget with the specified ID to the Cfg object.
	 *
	 * @param id The ID of the widget.
	 * @param parent A pointer to the widgets parent, to speed up searches.
	 * @return True on success, false otherwise.
	 *
	 * This function only makes sense for Cfg-classes that have the capability
	 * to interact with a widget, see Cfg_Tmpl::ConnectToWidget().
	 */	 
	virtual	bool ConnectToWidget( int WXUNUSED(id), wxWindow* WXUNUSED(parent) = NULL )	{ return false; }

	/**
	 * Gets the key assosiated with Cfg object.
	 *
	 * @return The config-key of this object.
	 */
	virtual const wxString& GetKey()	{ return m_key; }


	/**
	 * Specifies if the variable has changed since the TransferFromWindow() call.
	 *
	 * @return True if the variable has changed, false otherwise.
	 */
	virtual bool HasChanged() 			{ return m_changed; }


protected:
	/**
	 * Sets the changed status.
	 *
	 * @param changed The new status.
	 */
	virtual void SetChanged( bool changed )
	{ 
		m_changed = changed;
	};
	
private:

	//! The Config-key under which to save the variable
	wxString	m_key;

	//! The changed-status of the variable
	bool		m_changed;
};



const int cntStatColors = 13;


class CPreferences
{
public:
	friend class PrefsUnifiedDlg;

	CPreferences();
	~CPreferences();

	bool			Save();
	void			SaveCats();

	static bool		Score()				{ return s_scorsystem; }
	static bool		Reconnect()			{ return s_reconnect; }
	static bool		DeadServer()			{ return s_deadserver; }
	static const wxString&	GetUserNick()			{ return s_nick; }

	static uint16		GetPort()			{ return s_port; }
	static uint16		GetUDPPort()			{ return s_udpport; }
	static const wxString&	GetIncomingDir()		{ return s_incomingdir; }
	static const wxString&	GetTempDir()			{ return s_tempdir; }
	const CMD4Hash&		GetUserHash()			{ return m_userhash; }
	static uint16		GetMaxUpload()			{ return s_maxupload; }
	static uint16		GetSlotAllocation()		{ return s_slotallocation; }
	static bool		IsICHEnabled()			{ return s_ICH; }
	static bool IsTrustingEveryHash() { return s_AICHTrustEveryHash; }
	static bool		AutoServerlist()		{ return s_autoserverlist; }
	static bool		DoMinToTray()			{ return s_mintotray; }
	static bool		DoAutoConnect()			{ return s_autoconnect; }
	static void		SetAutoConnect( bool inautoconnect)
       					{s_autoconnect = inautoconnect; }
	static bool		AddServersFromServer()		{ return s_addserversfromserver; }
	static bool		AddServersFromClient()		{ return s_addserversfromclient; }
	static uint16		GetTrafficOMeterInterval()	{ return s_trafficOMeterInterval; }
	static void		SetTrafficOMeterInterval(uint16 in)
       					{ s_trafficOMeterInterval = in; }
	static uint16		GetStatsInterval()		{ return s_statsInterval;}
	static void		SetStatsInterval(uint16 in)	{ s_statsInterval = in; }
	static void		Add2TotalDownloaded(uint64 in)	{ s_totalDownloadedBytes += in; }
	static void		Add2TotalUploaded(uint64 in)	{ s_totalUploadedBytes += in; }
	static uint64		GetTotalDownloaded()		{ return s_totalDownloadedBytes; }
	static uint64		GetTotalUploaded()		{ return s_totalUploadedBytes; }
	static bool		IsConfirmExitEnabled()		{ return s_confirmExit; }
	static bool		FilterBadIPs()			{ return s_filterBadIP; }
	static bool		IsOnlineSignatureEnabled()	{ return s_onlineSig; }
	static uint32		GetMaxGraphUploadRate()		{ return s_maxGraphUploadRate; }
	static uint32		GetMaxGraphDownloadRate()	{ return s_maxGraphDownloadRate; }
	static void		SetMaxGraphUploadRate(uint32 in){ s_maxGraphUploadRate=in; }
	static void		SetMaxGraphDownloadRate(uint32 in)
       					{ s_maxGraphDownloadRate = in; }

	static uint16		GetMaxDownload()		{ return s_maxdownload; }
	static uint16		GetMaxConnections()		{ return s_maxconnections; }
	static uint16		GetMaxSourcePerFile()		{ return s_maxsourceperfile; }
	static uint16		GetMaxSourcePerFileSoft() {
					uint16 temp = (uint16)(s_maxsourceperfile*0.9);
					if( temp > 1000 ) return 1000;
                                        return temp; }
	static uint16		GetMaxSourcePerFileUDP() {
					uint16 temp = (uint16)(s_maxsourceperfile*0.75);
					if( temp > 100 ) return 100;
                                        return temp; }
	static uint16		GetDeadserverRetries()		{ return s_deadserverretries; }
	static DWORD		GetServerKeepAliveTimeout()	{ return s_dwServerKeepAliveTimeoutMins*60000; }
	
	static WORD		GetLanguageID()			{ return s_languageID; }
	static void		SetLanguageID(WORD new_id)	{ s_languageID = new_id; }
	static uint8		CanSeeShares()			{ return s_iSeeShares; }

	static uint8		GetStatsMax()			{ return s_statsMax; }
	static bool		UseFlatBar()			{ return (s_depth3D==0); }
	static uint8		GetStatsAverageMinutes()	{ return s_statsAverageMinutes; }
	static void		SetStatsAverageMinutes(uint8 in){ s_statsAverageMinutes = in; }

	static bool		GetNotifierPopOnImportantError(){ return s_notifierImportantError; }

	static bool		GetStartMinimized()		{ return s_startMinimized; }
	static void		SetStartMinimized(bool instartMinimized)
					{ s_startMinimized = instartMinimized;}
	static bool		GetSmartIdCheck()		{ return s_smartidcheck; }
	static void		SetSmartIdCheck( bool in_smartidcheck )
       					{ s_smartidcheck = in_smartidcheck; }
	static uint8		GetSmartIdState()		{ return s_smartidstate; }
	static void		SetSmartIdState( uint8 in_smartidstate )
       					{ s_smartidstate = in_smartidstate; }
	static bool		GetVerbose()			{ return s_bVerbose; }
	static bool		GetPreviewPrio()		{ return s_bpreviewprio; }
	static void		SetPreviewPrio(bool in)		{ s_bpreviewprio = in; }
	static bool		GetUpdateQueueList()		{ return s_bupdatequeuelist; }
	static void		SetUpdateQueueList(bool new_state) { s_bupdatequeuelist = new_state; }
	static bool		TransferFullChunks()		{ return s_btransferfullchunks; }
	static void		SetTransferFullChunks( bool m_bintransferfullchunks ) 
					{s_btransferfullchunks = m_bintransferfullchunks; }
	static bool		StartNextFile()			{ return s_bstartnextfile; }
	static bool		ShowOverhead()			{ return s_bshowoverhead; }
	static void		SetNewAutoUp(bool m_bInUAP) 	{ s_bUAP = m_bInUAP; }
	static bool		GetNewAutoUp() 			{ return s_bUAP; }
	static void		SetNewAutoDown(bool m_bInDAP) 	{ s_bDAP = m_bInDAP; }
	static bool		GetNewAutoDown() 		{ return s_bDAP; }

	static const wxString&	GetVideoPlayer()		{ return s_VideoPlayer; }

	static uint32		GetFileBufferSize() 		{ return s_iFileBufferSize*15000; }
	static uint32		GetQueueSize() 			{ return s_iQueueSize*100; }

	// Barry
	static uint8		Get3DDepth() 			{ return s_depth3D;}
	static bool		AddNewFilesPaused()		{ return s_addnewfilespaused; }

	static void		SetMaxConsPerFive(int in)	{ s_MaxConperFive=in; }

	static uint16		GetMaxConperFive()		{ return s_MaxConperFive; }
	static uint16		GetDefaultMaxConperFive();

	static bool		IsSafeServerConnectEnabled()	{ return s_safeServerConnect; }
	static bool		IsMoviePreviewBackup()		{ return s_moviePreviewBackup; }
	
	static bool		IsCheckDiskspaceEnabled()	{ return s_checkDiskspace != 0; }
	static uint32		GetMinFreeDiskSpace()		{ return s_uMinFreeDiskSpace; }

	static const wxString&	GetYourHostname() 		{ return s_yourHostname; }

	// quick-speed changer [xrmb]
	static void		SetMaxUpload(uint16 in);
	static void		SetMaxDownload(uint16 in);
	static void		SetSlotAllocation(uint16 in) 	{ s_slotallocation = in; };

	wxArrayString shareddir_list;
	wxArrayString adresses_list;

	static void 		SetLanguage();
	static bool 		AutoConnectStaticOnly() 	{ return s_autoconnectstaticonly; }	
	void			LoadCats();
	static const wxString&	GetDateTimeFormat() 		{ return s_datetimeformat; }
	// Download Categories (Ornis)
	uint32			AddCat(Category_Struct* cat);
	void			RemoveCat(size_t index);
	uint32			GetCatCount();
	Category_Struct* GetCategory(size_t index);
	const wxString&	GetCatPath(uint8 index);
	DWORD			GetCatColor(size_t index);

	static uint32		GetAllcatType() 		{ return s_allcatType; }
	static void		SetAllcatType(uint32 in)	{ s_allcatType = in; }
	static bool		ShowAllNotCats() 		{ return s_showAllNotCats; }

	// WebServer
	static uint16 		GetWSPort() 			{ return s_nWebPort; }
	static void		SetWSPort(uint16 uPort) 	{ s_nWebPort=uPort; }
	static const wxString&	GetWSPass() 			{ return s_sWebPassword; }
	static bool		GetWSIsEnabled() 		{ return s_bWebEnabled; }
	static void		SetWSIsEnabled(bool bEnable) 	{ s_bWebEnabled=bEnable; }
	static bool		GetWebUseGzip() 		{ return s_bWebUseGzip; }
	static void		SetWebUseGzip(bool bUse) 	{ s_bWebUseGzip=bUse; }
	static uint32 		GetWebPageRefresh() 		{ return s_nWebPageRefresh; }
	static void		SetWebPageRefresh(uint32 nRefresh) { s_nWebPageRefresh=nRefresh; }
	static bool		GetWSIsLowUserEnabled() 	{ return s_bWebLowEnabled; }
	static void		SetWSIsLowUserEnabled(bool in) 	{ s_bWebLowEnabled=in; }
	static const wxString&	GetWSLowPass() 			{ return s_sWebLowPassword; }

	static void		SetMaxSourcesPerFile(uint16 in) { s_maxsourceperfile=in;}
	static void		SetMaxConnections(uint16 in) 	{ s_maxconnections =in;}
	
	static bool		MsgOnlyFriends() 		{ return s_msgonlyfriends;}
	static bool		MsgOnlySecure() 		{ return s_msgsecure;}


	static uint32 		GetDesktopMode() 		{ return s_desktopMode; }
	static void 		SetDesktopMode(uint32 mode) 	{ s_desktopMode=mode; }

	static bool		ShowCatTabInfos() 		{ return s_showCatTabInfos; }
	static void		ShowCatTabInfos(bool in) 	{ s_showCatTabInfos=in; }
	
	// Madcat - Sources Dropping Tweaks
	static bool		DropNoNeededSources() 		{ return s_NoNeededSources > 0; }
	static bool		SwapNoNeededSources() 		{ return s_NoNeededSources == 2; }
	static bool		DropFullQueueSources() 		{ return s_DropFullQueueSources; }
	static bool		DropHighQueueRankingSources() 	{ return s_DropHighQueueRankingSources; }
	static uint32		HighQueueRanking() 		{ return s_HighQueueRanking; }
	static uint32		GetAutoDropTimer() 		{ return s_AutoDropTimer; }
	
	// Kry - External Connections
	static bool 		AcceptExternalConnections()	{ return s_AcceptExternalConnections; }
	static bool 		ECUseTCPPort()			{ return s_ECUseTCPPort; }
	static uint32 		ECPort()			{ return s_ECPort; }
	static const wxString&	ECPassword()			{ return s_ECPassword; }
	// Madcat - Fast ED2K Links Handler Toggling
	static bool 		GetFED2KLH()			{ return s_FastED2KLinksHandler; }

	// Kry - Ip filter 
	static bool		GetIPFilterOn()			{ return s_IPFilterOn; }
	static void		SetIPFilterOn(bool val)		{ s_IPFilterOn = val; }
	static uint8		GetIPFilterLevel()		{ return s_filterlevel;}
	static void		SetIPFilterLevel(uint8 level)	{ s_filterlevel = level;}
	static bool		IPFilterAutoLoad() { return s_IPFilterAutoLoad; }
	static const wxString& IPFilterURL() { return s_IPFilterURL; } 

	// Kry - Source seeds On/Off
	static bool		GetSrcSeedsOn() 		{ return s_UseSrcSeeds; }
	
	// Kry - Safe Max Connections
	static bool		GetSafeMaxConn()		{ return s_UseSafeMaxConn; }
	
	static bool		GetVerbosePacketError()		{ return s_VerbosePacketError; }
	
	static bool		IsSecureIdentEnabled()		{ return s_SecIdent; }
	
	static bool		GetExtractMetaData()		{ return s_ExtractMetaData; }
	
	static bool		ShowProgBar()			{ return s_ProgBar; }
	static bool		ShowPercent()			{ return s_Percent; }	
	
	static bool		GetAllocFullPart()		{ return s_AllocFullPart; };
	static bool		GetAllocFullChunk()		{ return s_AllocFullChunk; };

	static wxString 	GetBrowser();
	
	static const wxString& 	GetSkinFile()			{ return s_SkinFile; }
	
	static bool		UseSkin()			{ return s_UseSkinFile; }
	
	static const wxString& GetOSDir()			{ return s_OSDirectory; }

	static uint8		GetToolTipDelay()		{ return s_iToolDelayTime; }

	static int		GetFilePermissions();
	static void		SetFilePermissions( int perms );
	static int		GetDirPermissions();
	static void		SetDirPermissions( int perms );

	static void		CheckUlDlRatio();
	
	static void BuildItemList( const wxString& appdir );
	static void EraseItemList();
	
	static void LoadAllItems(wxConfigBase* cfg);
	static void SaveAllItems(wxConfigBase* cfg);

	static bool 		GetShowRatesOnTitle()		{ return s_ShowRatesOnTitle; }

protected:
	void	CreateUserHash();
	void	SetStandartValues();
	static	int32 GetRecommendedMaxConnections();

	//! Temporary storage for statistic-colors.
	static COLORREF	s_colors[cntStatColors];
	//! Reference for checking if the colors has changed.
	static COLORREF	s_colors_ref[cntStatColors];
	 
	typedef std::vector<Cfg_Base*>			CFGList;
	typedef std::map<int, Cfg_Base*>		CFGMap;
	typedef std::vector<Category_Struct*>	CatList;


	static CFGMap	s_CfgList;
	static CFGList	s_MiscList;
	CatList			m_CatList;

private:
	CMD4Hash m_userhash;

	void LoadPreferences();
	void SavePreferences();

protected:
////////////// USER
	static wxString	s_nick;

////////////// CONNECTION
	static uint16	s_maxupload;
	static uint16	s_maxdownload;
	static uint16	s_slotallocation;
	static uint16	s_port;
	static uint16	s_udpport;
	static bool	s_UDPDisable;
	static uint16	s_maxconnections;
	static bool	s_reconnect;
	static bool	s_autoconnect;
	static bool	s_autoconnectstaticonly;

////////////// SERVERS
	static bool	s_autoserverlist;
	static bool	s_deadserver;

////////////// FILES
	static wxString	s_incomingdir;
	static wxString	s_tempdir;
	static bool	s_ICH;
	static bool	s_AICHTrustEveryHash;
	static int	s_perms_files;
	static int	s_perms_dirs;

////////////// GUI
	static uint8	s_depth3D;
	
	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	static uint32	s_tableSortItemDownload;
	static uint32	s_tableSortItemUpload;
	static uint32	s_tableSortItemQueue;
	static uint32	s_tableSortItemSearch;
	static uint32	s_tableSortItemShared;
	static uint32	s_tableSortItemServer;
	static uint32	s_tableSortItemClientList;
	static bool	s_tableSortAscendingDownload;
	static bool	s_tableSortAscendingUpload;
	static bool	s_tableSortAscendingQueue;
	static bool	s_tableSortAscendingSearch;
	static bool	s_tableSortAscendingShared;
	static bool	s_tableSortAscendingServer;
	static bool	s_tableSortAscendingClientList;

	static bool	s_scorsystem;
	static bool	s_mintotray;
	static bool	s_addnewfilespaused;
	static bool	s_addserversfromserver;
	static bool	s_addserversfromclient;
	static uint16	s_maxsourceperfile;
	static uint16	s_trafficOMeterInterval;
	static uint16	s_statsInterval;
	static uint32	s_maxGraphDownloadRate;
	static uint32	s_maxGraphUploadRate;
	static bool	s_confirmExit;


	static bool	s_filterBadIP;
	static bool	s_onlineSig;

	static uint64  	s_totalDownloadedBytes;
	static uint64	s_totalUploadedBytes;
	static uint16	s_languageID;
	static bool	s_transferDoubleclick;
	static uint8	s_iSeeShares;		// 0=everybody 1=friends only 2=noone
	static uint8	s_iToolDelayTime;	// tooltip delay time in seconds
	static uint8	s_splitterbarPosition;
	static uint16	s_deadserverretries;
	static uint32	s_dwServerKeepAliveTimeoutMins;

	static uint8	s_statsMax;
	static uint8	s_statsAverageMinutes;

	static bool	s_useDownloadNotifier;
	static bool	s_useChatNotifier;
	static bool	s_useLogNotifier;	
	static bool	s_useSoundInNotifier;
	static bool	s_sendEmailNotifier;
	static bool	s_notifierPopsEveryChatMsg;
	static bool	s_notifierImportantError;
	static bool	s_notifierNewVersion;
	static wxString	s_notifierSoundFilePath;

	static bool	s_bpreviewprio;
	static bool	s_smartidcheck;
	static uint8	s_smartidstate;
	static bool	s_safeServerConnect;
	static bool	s_startMinimized;
	static uint16	s_MaxConperFive;
	static bool	s_checkDiskspace;
	static uint32 	s_uMinFreeDiskSpace;
	static wxString	s_yourHostname;
	static bool	s_bVerbose;
	static bool	s_bupdatequeuelist;
	static bool	s_bmanualhighprio;
	static bool	s_btransferfullchunks;
	static bool	s_bstartnextfile;
	static bool	s_bshowoverhead;
	static bool	s_bDAP;
	static bool	s_bUAP;
	static bool	s_bDisableKnownClientList;
	static bool	s_bDisableQueueList;

	static bool	s_ShowRatesOnTitle;

	static wxString	s_VideoPlayer;
	static bool	s_moviePreviewBackup;
	static bool	s_indicateratings;
	static bool	s_showAllNotCats;
	static bool	s_filterserverbyip;
	static bool	s_bFirstStart;
	
	static bool	s_msgonlyfriends;
	static bool	s_msgsecure;

	static uint8	s_iFileBufferSize;
	static uint8	s_iQueueSize;

	static uint16	s_maxmsgsessions;
	static wxString	s_datetimeformat;

	// Web Server [kuchin]
	static wxString	s_sWebPassword;
	static wxString	s_sWebLowPassword;
	static uint16	s_nWebPort;
	static bool	s_bWebEnabled;
	static bool	s_bWebUseGzip;
	static uint32	s_nWebPageRefresh;
	static bool	s_bWebLowEnabled;
	static wxString	s_sWebResDir;

	static wxString	s_sTemplateFile;
	static bool	s_bIsASCWOP;

	static bool	s_showCatTabInfos;
	static bool	s_resumeSameCat;
	static bool	s_dontRecreateGraphs;
	static uint32	s_allcatType;
	
	static uint32 	s_desktopMode;
	
	// Madcat - Sources Dropping Tweaks
	static uint8	s_NoNeededSources; // 0: Keep, 1: Drop, 2:Swap
	static bool	s_DropFullQueueSources;
	static bool	s_DropHighQueueRankingSources;
	static uint32	s_HighQueueRanking;
	static uint32	s_AutoDropTimer;
	
	// Kry - external connections
	static bool 	s_AcceptExternalConnections;
	static bool 	s_ECUseTCPPort;
	static uint32	s_ECPort;
	static wxString	s_ECPassword;
	
	// Kry - IPFilter 
	static bool	s_IPFilterOn;
	static uint8	s_filterlevel;
	static bool	s_IPFilterAutoLoad;
	static wxString s_IPFilterURL;
	
	// Kry - Source seeds on/off
	static bool	s_UseSrcSeeds;
	
	// Kry - Safe Max Connections
	static bool	s_UseSafeMaxConn;
	
	static bool	s_VerbosePacketError;

	static bool	s_ProgBar;
	static bool	s_Percent;	
	
	static bool	s_SecIdent;
	
	static bool	s_ExtractMetaData;
	
	static bool	s_AllocFullPart;
	static bool	s_AllocFullChunk;
	
	static uint16	s_Browser;
	static wxString	s_CustomBrowser;
	static bool	s_BrowserTab;     // Jacobo221 - Open in tabs if possible
	
	static wxString	s_OSDirectory;
	
	static wxString	s_SkinFile;
	
	static bool	s_UseSkinFile;
	
	static bool	s_FastED2KLinksHandler;	// Madcat - Toggle Fast ED2K Links Handler
};

#endif // PREFERENCES_H