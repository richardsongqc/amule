//------------------------------------------------------------------------------
// Header generated by wxDesigner from file: muuli.wdr
// Do not modify this file, all changes will be lost!
//------------------------------------------------------------------------------

#ifndef __WDR_muuli_H__
#define __WDR_muuli_H__

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
    #pragma interface "muuli_wdr.h"
#endif

// Include wxWidgets' headers

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/image.h>
#include <wx/statline.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/grid.h>
#include <wx/toolbar.h>

// Declare window functions

const int ID_TEXT = 10000;
const int ID_SYSTRAYSELECT = 10001;
const int ID_OK = 10002;
wxSizer *desktopDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

extern wxSizer *s_dlgcnt;
extern wxSizer *buttonToolbar;
extern wxSizer *contentSizer;
extern wxSizer *s_fed2klh;
const int ID_TEXTCTRL = 10003;
const int ID_BUTTON_FAST = 10004;
const int IDC_SHOWSTATUSTEXT = 10005;
const int ID_LINE = 10006;
const int ID_STATICBITMAP = 10007;
wxSizer *muleDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_SRV_SPLITTER = 10008;
wxSizer *serverListDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

extern wxSizer *s_searchdlgsizer;
extern wxSizer *s_searchsizer;
const int IDC_SEARCHNAME = 10009;
const int IDC_SEARCH_RESET = 10010;
const int ID_SEARCHTYPE = 10011;
const int ID_EXTENDEDSEARCHCHECK = 10012;
extern wxSizer *s_extendedsizer;
const int IDC_TypeSearch = 10013;
const int ID_AUTOCATASSIGN = 10014;
const int IDC_EDITSEARCHEXTENSION = 10015;
const int IDC_SPINSEARCHMIN = 10016;
const int IDC_SEARCHMINSIZE = 10017;
const int IDC_SPINSEARCHMAX = 10018;
const int IDC_SEARCHMAXSIZE = 10019;
const int IDC_SPINSEARCHAVAIBILITY = 10020;
const int IDC_STARTS = 10021;
const int IDC_CANCELS = 10022;
const int IDC_SDOWNLOAD = 10023;
const int IDC_CLEAR_RESULTS = 10024;
const int ID_NOTEBOOK = 10025;
const int ID_SEARCHPROGRESS = 10026;
wxSizer *searchDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

extern wxSizer *transfer_top_boxsizer;
const int ID_BTNCLRCOMPL = 10027;
const int ID_CATEGORIES = 10028;
const int ID_BTNSWWINDOW = 10029;
const int ID_DLOADLIST = 10030;
wxSizer *transferTopPane( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

extern wxSizer *queueSizer;
const int ID_CLIENTTOGGLE = 10031;
const int ID_BTNSWITCHUP = 10032;
const int ID_CLIENTCOUNT = 10033;
const int ID_CLIENTLIST = 10034;
wxSizer *transferBottomPane( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_FRIENDLIST = 10035;
const int IDC_CHATSELECTOR = 10036;
const int IDC_CMESSAGE = 10037;
const int IDC_CSEND = 10038;
const int IDC_CCLOSE = 10039;
wxSizer *messagePage( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

extern wxSizer *IDC_FD_X0;
const int IDC_FD_X1 = 10040;
const int IDC_FNAME = 10041;
const int IDC_FD_X2 = 10042;
const int IDC_METFILE = 10043;
const int IDC_FD_X3 = 10044;
const int IDC_FHASH = 10045;
const int IDC_FD_X4 = 10046;
const int IDC_FSIZE = 10047;
const int IDC_FD_X5 = 10048;
const int IDC_PFSTATUS = 10049;
const int IDC_FD_X15 = 10050;
const int IDC_LASTSEENCOMPL = 10051;
extern wxSizer *IDC_MINF_0;
const int IDC_MINF_1 = 10052;
const int IDC_MINFCODEC = 10053;
const int IDC_MINF_2 = 10054;
const int IDC_MINFBIT = 10055;
const int IDC_MINF_3 = 10056;
const int IDC_MINFLEN = 10057;
extern wxSizer *IDC_FD_X6;
const int IDC_FD_X7 = 10058;
const int IDC_SOURCECOUNT = 10059;
const int IDC_FD_X10 = 10060;
const int IDC_FD_X9 = 10061;
const int IDC_PARTCOUNT = 10062;
const int IDC_FD_X11 = 10063;
const int IDC_FD_X13 = 10064;
const int IDC_DATARATE = 10065;
const int IDC_FD_X14 = 10066;
const int IDC_TRANSFERED = 10067;
const int IDC_FD_X12 = 10068;
const int IDC_SOURCECOUNT2 = 10069;
const int IDC_PARTAVAILABLE = 10070;
const int IDC_COMPLSIZE = 10071;
const int IDC_PROCCOMPL = 10072;
extern wxSizer *IDC_FD_ICH;
const int IDC_FD_LSTATS1 = 10073;
const int IDC_FD_STATS1 = 10074;
const int IDC_FD_LSTATS2 = 10075;
const int IDC_FD_STATS2 = 10076;
const int IDC_FD_LSTATS3 = 10077;
const int IDC_FD_STATS3 = 10078;
extern wxSizer *IDC_FD_SN;
const int IDC_LISTCTRLFILENAMES = 10079;
const int IDC_TAKEOVER = 10080;
const int IDC_BUTTONSTRIP = 10081;
const int IDC_FILENAME = 10082;
const int IDC_RENAME = 10083;
const int ID_CLOSEWNDFD = 5100;
const int IDC_CMTBT = 10084;
wxSizer *fileDetails( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_CMT_TEXT = 10085;
const int IDC_FC_CLEAR = 10086;
const int IDC_RATELIST = 10087;
const int IDCOK = 10088;
const int IDCCANCEL = 10089;
wxSizer *commentDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_LST = 10090;
const int IDC_CMSTATUS = 10091;
const int IDCREF = 10092;
wxSizer *commentLstDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_ANIMATE = 10093;
const int ID_HTTPDOWNLOADPROGRESS = 10094;
const int ID_CANCEL = 5101;
wxSizer *downloadDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_IPADDRESS = 10095;
const int ID_IPORT = 10096;
const int ID_USERNAME = 10097;
const int ID_USERHASH = 10098;
const int ID_ADDFRIEND = 10099;
const int ID_CLOSEDLG = 10100;
wxSizer *addFriendDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_BTNRELSHARED = 10101;
const int ID_SHFILELIST = 10102;
const int IDC_SREQUESTED = 10103;
const int ID_GAUGE = 10104;
const int IDC_SREQUESTED2 = 10105;
const int IDC_SACCEPTED = 10106;
const int IDC_SACCEPTED2 = 10107;
const int IDC_STRANSFERED = 10108;
const int IDC_STRANSFERED2 = 10109;
wxSizer *sharedfilesDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

extern wxSizer *testSizer;
const int ID_DLOADSCOPE = 10110;
const int IDC_C0 = 10111;
const int IDC_C0_3 = 10112;
const int IDC_C0_2 = 10113;
const int ID_OTHERS = 10114;
const int IDC_S3 = 10115;
const int IDC_S0 = 10116;
const int ID_ACTIVEC = 10117;
const int IDC_S1 = 10118;
const int ID_ULOADSCOPE = 10119;
const int IDC_C1 = 10120;
const int IDC_C1_3 = 10121;
const int IDC_C1_2 = 10122;
const int ID_TREECTRL = 10123;
wxSizer *statsDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_DNAME = 10124;
const int ID_DHASH = 10125;
const int ID_DSOFT = 10126;
const int ID_DIP = 10127;
const int ID_DSIP = 10128;
const int ID_DVERSION = 10129;
const int ID_DID = 10130;
const int ID_DSNAME = 10131;
const int ID_DDOWNLOADING = 10132;
const int ID_DDOWN = 10133;
const int ID_DAVDR = 10134;
const int ID_DDOWNTOTAL = 10135;
const int ID_DDUP = 10136;
const int ID_DAVUR = 10137;
const int ID_DUPTOTAL = 10138;
const int ID_DRATIO = 10139;
const int ID_DRATING = 10140;
const int IDC_CDIDENT = 10141;
const int ID_DSCORE = 10142;
const int ID_CLOSEWND = 10143;
wxSizer *clientDetails( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_NICK = 10144;
const int IDC_LANGUAGE = 10145;
const int IDC_DBLCLICK = 10146;
const int IDC_MINTRAY = 10147;
const int IDC_EXIT = 10148;
const int IDC_TOOLTIPDELAY_LBL = 10149;
const int IDC_TOOLTIPDELAY = 10150;
const int ID_DESKTOPMODE = 10151;
const int IDC_STARTMIN = 10152;
const int IDC_FCHECK = 10153;
const int ID_CUSTOMBROWSETEXT = 10154;
const int IDC_FCHECKSELF = 10155;
const int IDC_FCHECKTABS = 10156;
wxSizer *PreferencesGeneralTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_DLIMIT_LBL = 10157;
const int IDC_MAXDOWN = 10158;
const int IDC_KBS1 = 10159;
const int IDC_ULIMIT_LBL = 10160;
const int IDC_MAXUP = 10161;
const int IDC_KBS4 = 10162;
const int IDC_SLOTALLOC = 10163;
const int IDC_DCAP_LBL = 10164;
const int IDC_DOWNLOAD_CAP = 10165;
const int IDC_KBS2 = 10166;
const int IDC_UCAP_LBL = 10167;
const int IDC_UPLOAD_CAP = 10168;
const int IDC_KBS3 = 10169;
const int IDC_PORT = 10170;
const int IDC_UDPPORT = 10171;
const int IDC_UDPDISABLE = 10172;
const int IDC_MAXSRCHARD_LBL = 10173;
const int IDC_MAXSOURCEPERFILE = 10174;
const int IDC_MAXCONLABEL = 10175;
const int IDC_MAXCON = 10176;
const int IDC_AUTOCONNECT = 10177;
const int IDC_RECONN = 10178;
const int IDC_SHOWOVERHEAD = 10179;
wxSizer *PreferencesConnectionTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_REMOVEDEAD = 10180;
const int IDC_SERVERRETRIES = 10181;
const int IDC_RETRIES_LBL = 10182;
const int IDC_AUTOSERVER = 10183;
const int IDC_EDITADR = 10184;
const int IDC_UPDATESERVERCONNECT = 10185;
const int IDC_UPDATESERVERCLIENT = 10186;
const int IDC_SCORE = 10187;
const int IDC_SMARTIDCHECK = 10188;
const int IDC_SAFESERVERCONNECT = 10189;
const int IDC_AUTOCONNECTSTATICONLY = 10190;
const int IDC_MANUALSERVERHIGHPRIO = 10191;
wxSizer *PreferencesServerTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_ICH = 10192;
const int IDC_AICHTRUST = 10193;
const int IDC_ADDNEWFILESPAUSED = 10194;
const int IDC_DAP = 10195;
const int IDC_PREVIEWPRIO = 10196;
const int IDC_UAP = 10197;
const int IDC_FULLCHUNKTRANS = 10198;
const int IDC_STARTNEXTFILE = 10199;
const int IDC_SRCSEEDS = 10200;
const int IDC_METADATA = 10201;
const int IDC_CHUNKALLOC = 10202;
const int IDC_FULLALLOCATE = 10203;
const int IDC_CHECKDISKSPACE = 10204;
const int ID_MINDISKTEXT = 10205;
const int IDC_MINDISKSPACE = 10206;
wxSizer *PreferencesFilesTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_INCFILES = 10207;
const int IDC_SELINCDIR = 10208;
const int IDC_TEMPFILES = 10209;
const int IDC_SELTEMPDIR = 10210;
const int IDC_SHARESELECTOR = 10211;
const int IDC_SHAREHIDDENFILES = 10212;
const int IDC_VIDEOPLAYER = 10213;
const int IDC_BROWSEV = 10214;
const int IDC_VIDEOBACKUP = 10215;
wxSizer *PreferencesDirectoriesTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_SLIDERINFO = 10216;
const int IDC_SLIDER = 10217;
const int IDC_SLIDERINFO3 = 10218;
const int IDC_SLIDER3 = 10219;
const int IDC_SLIDERINFO4 = 10220;
const int IDC_SLIDER4 = 10221;
const int IDC_PREFCOLORS = 10222;
const int IDC_COLORSELECTOR = 10223;
const int IDC_COLOR_BUTTON = 10224;
const int IDC_SLIDERINFO2 = 10225;
const int IDC_SLIDER2 = 10226;
wxSizer *PreferencesStatisticsTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_CB_TBN_USESOUND = 10227;
const int IDC_EDIT_TBN_WAVFILE = 10228;
const int IDC_BTN_BROWSE_WAV = 10229;
const int IDC_CB_TBN_ONLOG = 10230;
const int IDC_CB_TBN_ONCHAT = 10231;
const int IDC_CB_TBN_POP_ALWAYS = 10232;
const int IDC_CB_TBN_ONDOWNLOAD = 10233;
const int IDC_CB_TBN_ONNEWVERSION = 10234;
const int IDC_CB_TBN_IMPORTATNT = 10235;
const int IDC_SENDMAIL = 10236;
const int IDC_SMTP = 10237;
const int IDC_EMAIL = 10238;
wxSizer *PreferencesNotifyTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_WARNING = 10239;
const int IDC_STATIC = 10240;
const int IDC_MAXCON5SECLABEL = 10241;
const int IDC_MAXCON5SEC = 10242;
const int IDC_SAFEMAXCONN = 10243;
const int IDC_VERBOSE = 10244;
const int IDC_VERBOSEPACKETERROR = 10245;
const int IDC_FILEBUFFERSIZE_STATIC = 10246;
const int IDC_FILEBUFFERSIZE = 10247;
const int IDC_QUEUESIZE_STATIC = 10248;
const int IDC_QUEUESIZE = 10249;
const int IDC_SERVERKEEPALIVE_LABEL = 10250;
const int IDC_SERVERKEEPALIVE = 10251;
wxSizer *PreferencesaMuleTweaksTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_PERCENT = 10252;
const int IDC_PROGBAR = 10253;
const int IDC_3DDEP = 10254;
const int IDC_3DDEPTH = 10255;
const int IDC_FLAT = 10256;
const int IDC_ROUND = 10257;
const int IDC_USESKIN = 10258;
const int IDC_SKINFILE = 10259;
const int IDC_SELSKINFILE = 10260;
const int IDC_AUTOSORT = 10261;
const int IDC_FED2KLH = 10262;
const int IDC_EXTCATINFO = 10263;
const int IDC_SHOWRATEONTITLE = 10264;
wxSizer *PreferencesGuiTweaksTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_NNS_HANDLING = 10265;
const int IDC_ENABLE_AUTO_FQS = 10266;
const int IDC_ENABLE_AUTO_HQRS = 10267;
const int IDC_HQR_VALUE = 10268;
const int IDC_AUTO_DROP_TIMER = 10269;
wxSizer *PreferencesSourcesDroppingTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_ENABLE_WEB = 10270;
const int IDC_WEB_PORT = 10271;
const int IDC_WEB_REFRESH_TIMEOUT = 10272;
const int IDC_WEB_GZIP = 10273;
const int IDC_ENABLE_WEB_LOW = 10274;
const int IDC_WEB_PASSWD = 10275;
const int IDC_WEB_PASSWD_LOW = 10276;
const int IDC_EXT_CONN_ACCEPT = 10277;
const int IDC_EXT_CONN_USETCP = 10278;
const int IDC_EXT_CONN_TCP_PORT = 10279;
const int IDC_EXT_CONN_PASSWD_ENABLE = 10280;
const int IDC_EXT_CONN_PASSWD = 10281;
wxSizer *PreferencesRemoteControlsTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

extern wxSizer *prefs_main_sizer;
extern wxSizer *prefs_sizer;
extern wxSizer *prefs_list_sizer;
const int ID_PREFSLISTCTRL = 10282;
const int ID_PREFS_OK_TOP = 10283;
const int ID_PREFS_CANCEL_TOP = 10284;
wxSizer *preferencesDlgTop( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_STATIC_TITLE = 10285;
const int IDC_TITLE = 10286;
const int IDC_STATIC_COMMENT = 10287;
const int IDC_COMMENT = 10288;
const int IDC_STATIC_INCOMING = 10289;
const int IDC_INCOMING = 10290;
const int IDC_BROWSE = 10291;
const int IDC_STATIC_PRIO = 10292;
const int IDC_PRIOCOMBO = 10293;
const int IDC_STATIC_COLOR = 10294;
const int ID_BOX_CATCOLOR = 10295;
const int IDC_CATCOLOR = 10296;
wxSizer *CategoriesEditWindow( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_SPLATTER = 10297;
wxSizer *transferDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_SERVERINFO = 10298;
const int ID_BTN_RESET_SERVER = 10299;
wxSizer *ServerInfoLog( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_LOGVIEW = 10300;
const int ID_BTN_RESET = 10301;
wxSizer *aMuleLog( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_UPDATELIST = 10302;
const int IDC_SERVERLISTURL = 10303;
const int IDC_SERVERNAME = 10304;
const int IDC_IPADDRESS = 10305;
const int IDC_SPORT = 10306;
const int ID_ADDTOLIST = 10307;
const int ID_SERVERLIST = 10308;
wxSizer *serverListDlgUp( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_SRVLOG_NOTEBOOK = 10309;
wxSizer *serverListDlgDown( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_LOCALETEXT = 10310;
const int ID_LOCALENEVERAGAIN = 10311;
wxSizer *LocaleWarning( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_NODELIST = 10312;
const int ID_NODE_IP1 = 10313;
const int ID_NODE_IP2 = 10314;
const int ID_NODE_IP3 = 10315;
const int ID_NODE_IP4 = 10316;
const int ID_NODE_PORT = 10317;
const int ID_NODECONNECT = 10318;
const int ID_KNOWNNODECONNECT = 10319;
const int ID_KADSEARCHLIST = 10320;
wxSizer *KadDlg( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int ID_MYSERVINFO = 10321;
wxSizer *MyInfoLog( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_SEESHARES = 10322;
const int IDC_SPIN_PERM_FU = 10323;
const int IDC_SPIN_PERM_FG = 10324;
const int IDC_SPIN_PERM_FO = 10325;
const int IDC_SPIN_PERM_DU = 10326;
const int IDC_SPIN_PERM_DG = 10327;
const int IDC_SPIN_PERM_DO = 10328;
const int IDC_IPFONOFF = 10329;
const int IDC_IPFRELOAD = 10330;
const int IDC_AUTOIPFILTER = 10331;
const int IDC_IPFILTERURL = 10332;
const int IDC_IPFILTERUPDATE = 10333;
const int ID_IPFILTERLEVEL = 10334;
const int IDC_FILTER = 10335;
const int IDC_SECIDENT = 10336;
wxSizer *PreferencesSecurityTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_ONLINESIG = 10337;
const int ID_SPINCTRL = 10338;
const int IDC_OSDIR = 10339;
const int IDC_SELOSDIR = 10340;
wxSizer *PreferencesOnlineSigTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

const int IDC_MSGFILTER = 10341;
const int IDC_MSGFILTER_ALL = 10342;
const int IDC_MSGFILTER_NONFRIENDS = 10343;
const int IDC_MSGFILTER_NONSECURE = 10344;
const int IDC_MSGFILTER_WORD = 10345;
const int IDC_MSGWORD = 10346;
wxSizer *PreferencesMessagesTab( wxWindow *parent, bool call_fit = TRUE, bool set_sizer = TRUE );

// Declare menubar functions

// Declare toolbar functions

const int ID_BUTTONCONNECT = 10347;
const int ID_BUTTONSERVERS = 10348;
const int ID_BUTTONKAD = 10349;
const int ID_BUTTONSEARCH = 10350;
const int ID_BUTTONTRANSFER = 10351;
const int ID_BUTTONSHARED = 10352;
const int ID_BUTTONMESSAGES = 10353;
const int ID_BUTTONSTATISTICS = 10354;
const int ID_BUTTONNEWPREFERENCES = 10355;
const int ID_ABOUT = 10356;
void muleToolbar( wxToolBar *parent );

// Declare bitmap functions

wxBitmap clientImages( size_t index );

wxBitmap dlStatusImages( size_t index );

wxBitmap connImages( size_t index );

wxBitmap moreImages( size_t index );

wxBitmap amuleSpecial( size_t index );

wxBitmap connButImg( size_t index );

wxBitmap amuleDlgImages( size_t index );

#endif

// End of generated file