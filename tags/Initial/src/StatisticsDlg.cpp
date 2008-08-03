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

// StatisticsDlg.cpp : implementation file
//


#include <cmath>		// Needed for std::exp
#include <wx/settings.h>

#include "muuli_wdr.h"		// Needed for statsDlg
#include "StatisticsDlg.h"	// Interface declarations
#include "GetTickCount.h"	// Needed for GetTickCount
#include "ServerList.h"		// Needed for CServerList
#include "ClientList.h"		// Needed for CClientList
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "otherfunctions.h"	// Needed for GetTickCount
#include "sockets.h"		// Needed for CServerConnect
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ListenSocket.h"	// Needed for CListenSocket
#include "ColorFrameCtrl.h"	// Needed for CColorFrameCtrl
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"			// Needed for theApp
#include "OScopeCtrl.h"		// Needed for COScopeCtrl
#include "amuleDlg.h"		// Needed for ShowStatistics

#define ID_EXPORT_HTML 20001

// CStatisticsDlg dialog

BEGIN_EVENT_TABLE(CStatisticsDlg,wxPanel)
	EVT_MENU(ID_EXPORT_HTML,CStatisticsDlg::ExportHTMLEvent)
END_EVENT_TABLE()


CStatisticsDlg::CStatisticsDlg(wxWindow* pParent)
: wxPanel(pParent,CStatisticsDlg::IDD)
{
	wxSizer* content=statsDlg(this,TRUE);
	content->Show(this,TRUE);

	// Resolution of the history list: in each sampling range we want at least one sample
	// per pixel, so we need the max plot width; take the window width in maximized state,
	// divide by 2 (2 graphs side-by-side) and subtract a constant for framing.  Note that
	// this is simply a "reasonable" calculation - if the user drags the aMule window
	// partly off-screen and then resizes it to fill the screen, he will get graph sizes
	// bigger than we assume here.  The graphs will still work then, but he won't get max
	// resolution if the graphs are rebuilt based on the history list.
	nPointsPerRange = wxGetTopLevelParent(this)->GetMaxSize().GetWidth()/2 - 80;

	pscopeDL=(COScopeCtrl*)FindWindowByName("dloadScope");
	pscopeUL=(COScopeCtrl*)FindWindowByName("uloadScope");
	pscopeConn=(COScopeCtrl*)FindWindowByName("otherScope");
	stattree=wxStaticCast(FindWindowByName("statTree"),wxTreeCtrl);

	m_ilastMaxConnReached = 0;
	peakconnections = 0;
	totalconnectionchecks = 0;
	averageconnections = 0;
	activeconnections = 0;
	maxDown = 0.0;
	maxDownavg = 0;
	kBpsUpCur = 0.0;
	kBpsUpAvg = 0.0;
	kBpsUpSession = 0.0;
	kBpsDownCur = 0.0;
	kBpsDownAvg = 0.0;
	kBpsDownSession = 0.0;
}



CStatisticsDlg::~CStatisticsDlg()
{
	// the destructor for listHR frees the memory occupied by the nodes
	delete[] aposRecycle;
}

void CStatisticsDlg::Init()
{
	InitGraphs();
	InitTree();
}


void CStatisticsDlg::InitGraphs()
{
	HR hr = {0.0, 0.0, 0.0, 0.0, 0.0, 0, 0, 0};
	hrInit = hr;
	nHistRanges = 7;	// =ceil(log(max_update_delay)/log(2))
	bitsHistClockMask = (1 << (nHistRanges-1)) - 1;
	aposRecycle = new POSITION[nHistRanges];
	POSITION *ppos = aposRecycle+nHistRanges-1;
	for (int i=nHistRanges; i>0; i--) {  // permanently allocated history list
		*ppos-- = listHR.AddTail(hr);
		for (int j=nPointsPerRange; j>1; j--)
			listHR.AddTail(hr);
	}

	// called after preferences get initialised
	for (int index=0; index<=10; index++)
		ApplyStatsColor(index);
	pscopeDL->SetRanges(0.0, (float)(theApp.glob_prefs->GetMaxGraphDownloadRate()+4));
	pscopeDL->SetYUnits(CString(_("kB/s")));
	pscopeUL->SetRanges(0.0, (float)(theApp.glob_prefs->GetMaxGraphUploadRate()+4));
	pscopeUL->SetYUnits(CString(_("kB/s")));
	pscopeConn->SetRanges(0.0, (float)(theApp.glob_prefs->GetStatsMax()));
	pscopeConn->SetYUnits("");
}


// this array is now used to store the current color settings and to define the defaults
COLORREF CStatisticsDlg::acrStat[cntStatColors] =
	{ 
		RGB(0,0,64),		RGB(192,192,255), 	RGB(128, 255, 128), RGB(0, 210, 0),
		RGB(0, 128, 0),		RGB(255, 128, 128), RGB(200, 0, 0),		RGB(140, 0, 0),
	  	RGB(150, 150, 255),	RGB(192, 0, 192),	RGB(255, 255, 128), RGB(0, 0, 0), 
	  	RGB(255, 255, 255) 
		};



void CStatisticsDlg::ApplyStatsColor(int index)
{
	static char aTrend[] = { 0,0,  2,     1,     0,           2,     1,     0,          1,    2,    0 };
	static int aRes[] = { 0,0, IDC_C0,IDC_C0_3,IDC_C0_2,  IDC_C1,IDC_C1_3,IDC_C1_2,  IDC_S0,IDC_S3,IDC_S1 };
	static COScopeCtrl** apscope[] = { NULL, NULL, &pscopeDL,&pscopeDL,&pscopeDL, &pscopeUL,&pscopeUL,&pscopeUL, &pscopeConn,&pscopeConn,&pscopeConn };
#ifndef UNIFIED_PREF_HANDLING
	COLORREF cr = theApp.glob_prefs->GetStatsColor(index);
#else
	COLORREF cr = acrStat[index];  
#endif
	int iRes = aRes[index];
	int iTrend = aTrend[index];
	COScopeCtrl** ppscope = apscope[index];
	CColorFrameCtrl* ctrl;
	switch (index) {
		case 0:	pscopeDL->SetBackgroundColor(cr);
				pscopeUL->SetBackgroundColor(cr);
				pscopeConn->SetBackgroundColor(cr);
				break;
		case 1:	pscopeDL->SetGridColor(cr);
				pscopeUL->SetGridColor(cr);
				pscopeConn->SetGridColor(cr);
				break;
		case 2:  case 3:  case 4:
		case 5:  case 6:  case 7:	
		case 8:  case 9:  case 10:
				(*ppscope)->SetPlotColor(cr, iTrend);
				if ((ctrl=(CColorFrameCtrl*)FindWindow(iRes)) == NULL) {
					printf("CStatisticsDlg::ApplyStatsColor: control missing (%d)\n",iRes);
					exit(1);
				}
				ctrl->SetBackgroundColor(cr);
				ctrl->SetFrameColor((COLORREF)RGB(0,0,0));
				break;
		default:
				break; // ignore unknown index, like SysTray speedbar color
	}
}


/*
History List

  The basic idea here is that we want to keep as much history as we can without paying 
a high price in terms of memory space.  Because we keep the history for display purposes, 
we can take advantage of the fact that when the period shown in the graphs is long 
then each pixel represents a long period.  So as the most recent history we keep one 
window full of points at a resolution of 1 second, the next window full at 2 seconds, 
the next at 4 seconds and so on, up to the maximum desired.  This way there is always 
at least one sample point per pixel for any update delay set by the user, and the 
memory required grows with the *log* of the total time period covered.
  The history is kept in a doubly-linked list, with the most recent snapshot at the tail.  
The number of nodes in the list is fixed, and there are no calls to RemoveHead() and 
AddTail() which would add overhead and contribute to memory fragmentation.  Instead, 
every second when a new point gets recorded, one of the existing nodes is recycled; 
it is disjoined from its present place, put at the tail of the list, and then gets 
filled with new data.   [Emilio Sandoz]
*/

void CStatisticsDlg::RecordHistory()
{	// First we query and compute some values, then we store them in the history list
	
	// A few comments about the use of double and float in computations:
	// Even on a hi-res screen our graphs will have 10 bits of resolution at most,
	// so the 24 bits resolution of a float on 32 bit Intel processors is more than
	// enough for all displayed values.  Rate computations however, and especially 
	// running average computations, use differences (delta bytes/ delta time), and 
	// for long uptimes the difference between two timestamps can lose too much 
	// accuracy because the large mantissa causes less significant bits to be dropped
	// (same for the difference between two cumulative byte counts).  [We don't store 
	// these values as integers because they will be used in floating point calculations,
	// and we want to perform the conversion only once).  Therefore timestamps and 
	// Kbyte counts are stored in the history as doubles, while computed values use
	// float (to save space and execution time).

	double		sCur = theApp.GetUptimeMsecs()/1000.0;
	double		sTrans;
	static double	sPrev;
	float		sAvg = theApp.glob_prefs->GetStatsAverageMinutes()*60.0;
	double		kBytesRec = theApp.stat_sessionReceivedBytes/1024.0;
	double		kBytesSent = theApp.stat_sessionSentBytes/1024.0;
	static double	kBytesRecPrev, kBytesSentPrev;

	kBpsUpCur = theApp.uploadqueue->GetKBps();
	kBpsDownCur = theApp.downloadqueue->GetKBps();
    if (maxDown < kBpsDownCur)
		maxDown = kBpsDownCur;
	ComputeSessionAvg(kBpsUpSession, kBpsUpCur, kBytesSent, sCur, sTrans);
	ComputeRunningAvg(kBpsUpAvg, kBpsUpSession, kBytesSent, kBytesSentPrev, sTrans, sPrev, sAvg);
	ComputeSessionAvg(kBpsDownSession, kBpsDownCur, kBytesRec, sCur, sTrans);
	ComputeRunningAvg(kBpsDownAvg, kBpsDownSession, kBytesRec, kBytesRecPrev, sTrans, sPrev, sAvg);
	sPrev = sTrans;
	if (maxDownavg < kBpsDownSession)
		maxDownavg = kBpsDownSession;
	activeconnections = theApp.listensocket->GetOpenSockets();
	if(peakconnections < activeconnections)
		peakconnections = activeconnections;
/*
	Store values; first determine the node to be recycled (using the bits in iClock)
	
    oldest records ----------------- listHR ------------------ youngest records
	
	O-[Range 2^n sec]-O- ... -O-[Range 4 sec]-O-[Range 2 sec]-O-[Range 1 sec]-O
	|                 |       |               |               > every 2 secs -^
	|                 |  ...  |                >--------------- every 4 secs -^
	|                 |       >------------------------ recycle every 8 secs -^
	|                 |                                                 ...
	|                 >-the node at this position is recycled every 2^n secs -^	
	>-------------------(ditto for the oldest node at the head of the list) --^	
	^                                                         ^
    aposRecycle[nHistRanges-1]               ...              aposRecycle[0]  Tail
*/
	POSITION	*ppos;
	static int	iClock;
	int			iClockPrev = iClock++;
	int			bits = (iClockPrev^iClock) & iClock;  // identify the highest changed bit
	if (bits <= bitsHistClockMask) {
		ppos = aposRecycle;
		while ((bits /= 2) != 0)  // count to the highest bit that was just toggled to 1
			ppos++;	
		// recycle one node and jump over the next to move it to the next higher range
		*ppos = listHR.NextAt(listHR.RecycleNodeAsTail(*ppos));
	} else {
		ppos = aposRecycle+nHistRanges-1;
		// recycle the node at the head; there is no higher range to move nodes into
		*ppos = listHR.RecycleNodeAsTail(*ppos);
	}
	
	// now save the latest data point in this node
	HR* phr = listHR.GetDataPtr(listHR.GetTailPosition());
	phr->kBytesSent = kBytesSent;
	phr->kBytesReceived = kBytesRec;
	phr->kBpsUpCur = kBpsUpCur;
	phr->kBpsDownCur = kBpsDownCur;
	phr->cntUploads = theApp.uploadqueue->GetUploadQueueLength();
	phr->cntConnections = activeconnections;
	phr->sTimestamp = sCur;
	int iStats[6];
	theApp.downloadqueue->GetDownloadStats(iStats);
	phr->cntDownloads = iStats[1];
#ifdef __DEBUG__
//	if (bits > bitsHistClockMask)  // every 64 seconds - 
//		VerifyHistory();  // must do this AFTER phr->sTimestamp has been set
#endif
}


unsigned CStatisticsDlg::GetHistory(  // Assemble arrays of sample points for a graph
	unsigned cntPoints,		// number of sample points to assemble
	double sStep,			// time difference between sample points
	double sFinal,			// latest allowed timestamp
	float** ppf,			// an array of pointers to arrays of floats for the result
	COScopeCtrl* pscope)	// the graph which will receive the points
{	
	if (sStep==0.0 || cntPoints==0)
		return(0);
	float		*pf1 = *ppf;
	float		*pf2 = *(ppf+1);
	float		*pf3 = *(ppf+2);
	unsigned	cntFilled = 0;
	POSITION	pos = listHR.GetTailPosition(), posPrev;
	HR			*phr = listHR.GetDataPtr(pos);  	// pointer to history record

	// start of list should be an integer multiple of the sampling period for samples 
	// to be consistent when the graphs are resized horizontally
	double	sTarget;
	if (sFinal >= 0.0)
		sTarget = sFinal;
	else
		sTarget = (sStep==1.0 ? phr->sTimestamp : std::floor(phr->sTimestamp/sStep) * sStep); 

	HR		**ahr, **pphr;
	bool	bRateGraph = (pscope!=pscopeConn);	// rate graph or connections graph?
	if (bRateGraph) {
		ahr = new HR* [cntPoints];
		pphr = ahr;
	}
	
	do {
		while ((posPrev=listHR.PrevAt(pos)) != NULL  // find next history record
			&& ((phr=listHR.GetDataPtr(posPrev))->sTimestamp > sTarget))
			pos = posPrev;
		if (bRateGraph) {	// assemble an array of pointers for ComputeAverages
			*pphr++ = phr;
		} else {			// or build the arrays if possible
			*pf1++ = (float)phr->cntUploads;
			*pf2++ = (float)phr->cntConnections;
			*pf3++ = (float)phr->cntDownloads;
		}
		if (++cntFilled  == cntPoints)		// enough points 
			break;
		if (phr->sTimestamp == 0.0)			// reached beginning of uptime
			break;
		if ((sTarget -= sStep) <= 0.0) {	// don't overshoot the beginning
			if (bRateGraph)
				*pphr++ = &hrInit;
			else
				*pf1++ = *pf2++ = *pf3++ = 0.0;
			cntFilled++;
			break;
		}
	} while (posPrev != NULL);

	if (!bRateGraph)
		return cntFilled;
	else if  (cntFilled > 0)
		ComputeAverages(pphr, pos, cntFilled, sStep, ppf, pscope);
	delete[] ahr;
	return cntFilled;
}


void CStatisticsDlg::ComputeAverages(
	HR			**pphr,			// pointer to (end of) array of assembled history records
	POSITION	pos,			// position in history list from which to backtrack
	unsigned	cntFilled,		// number of points in the sample data
	double		sStep,			// time difference between two samples
	float		**ppf,			// an array of pointers to arrays of floats with sample data
	COScopeCtrl	*pscope)		// the graph which will receive the points
{	
	double		sCur, sPrev, sTarget, sTrans, kBytesPrev, kBytesRun;
	float		kBpsAvg;
	float 		sAvg = (float)theApp.glob_prefs->GetStatsAverageMinutes()*60.0;
	POSITION	posPrev = listHR.PrevAt(pos);
	HR			*phr = listHR.GetDataPtr(pos);
		
	kBpsAvg = 0.0;
	sCur = phr->sTimestamp;
	sPrev = max(0.0, sCur-sStep);
	kBytesPrev = 0.0;
	// running average: backtrack in history far enough so that the residual error 
	// of the running avg computation will be <1/2 pixel for the first sample point
	if (posPrev != NULL  &&  sCur > 0.0) {
		// how long would the low-pass averaging filter need to decay the max to half a pixel
		sTarget = max(0.0, sCur - sAvg*std::log((float)(pscope->GetPlotHeightPixels()*2)));
		for (POSITION *ppos=aposRecycle; ppos<aposRecycle+nHistRanges; ppos++) {
			// accelerate search by using our intermediate pointers into the history list
			if (listHR.GetDataPtr(*ppos)->sTimestamp >= sTarget)
				pos = *ppos;
			else
				break;
		}
		while ((posPrev=listHR.PrevAt(pos)) != NULL) {
			if (listHR.GetDataPtr(pos=posPrev)->sTimestamp <= sTarget)
				break;
		}
		// backtracked enough, now compute running average up to first sample point
		phr = listHR.GetDataPtr(pos);
		sPrev = phr->sTimestamp;
		kBytesPrev = (pscope==pscopeDL ? phr->kBytesReceived : phr->kBytesSent);
		do  {
			float kBpsSession;
			pos = listHR.NextAt(pos);
			phr = listHR.GetDataPtr(pos);
			kBytesRun = (pscope==pscopeDL ? phr->kBytesReceived : phr->kBytesSent);
			ComputeSessionAvg(kBpsSession, (pscope==pscopeDL ? phr->kBpsDownCur : phr->kBpsUpCur), 
								kBytesRun, phr->sTimestamp, sTrans);
			ComputeRunningAvg(kBpsAvg, kBpsSession, kBytesRun, kBytesPrev, sTrans, sPrev, sAvg);
			sPrev = sTrans;
		} while (phr->sTimestamp < sCur);
	}

	// now compute averages in returned arrays, starting with the earliest values
	float	*pf1 = *ppf++ + cntFilled -1;  // holds session avg
	float	*pf2 = *ppf++ + cntFilled -1;  // holds running avg
	float	*pf3 = *ppf + cntFilled -1;    // holds current rate

	for (int cnt=cntFilled; cnt>0; cnt--, pf1--, pf2--, pf3--) {
		phr = *(--pphr);
		if (pscope == pscopeDL) {
			kBytesRun = phr->kBytesReceived;
			*pf3 = phr->kBpsDownCur;
		} else {
			kBytesRun = phr->kBytesSent;
			*pf3 = phr->kBpsUpCur;
		}
		ComputeSessionAvg(*pf1, *pf3, kBytesRun, phr->sTimestamp, sTrans);
		ComputeRunningAvg(kBpsAvg, *pf1, kBytesRun, kBytesPrev, sTrans, sPrev, sAvg);
		*pf2 = kBpsAvg;
		sPrev = sTrans;
	}
	// make sure the "running average" line will continue smoothly the next time a point
	// is appended, especially if the averaging time constant has been changed
	(pscope==pscopeDL ? kBpsDownAvg : kBpsUpAvg) = kBpsAvg;
}


void CStatisticsDlg::VerifyHistory(bool bMsgIfOk)
// Debugging only: This performs a basic sanity check on the history list: 
// link integrity, correct number of nodes, sequentiality of timestamps, 
// approximate gap length between sample points.
// See graph in RecordHistory() for the mechanics of the list.
// Un-comment the call at the end of RecordHistory() to activate these checks.
// Note: if the system is very busy we sometimes get a long gap followed by a short one,
// and they will be flagged as they move through the list.
{
#ifdef __DEBUG__
	int	cnt, cntRanges=1, cntInRange=0, cntExpected=nHistRanges*nPointsPerRange;
	double sStart = (double)(theApp.GetUptimeSecs()) + 0.01;
	double sStep=1.0, sPrev=sStart, sCur;
	POSITION *ppos = aposRecycle;
	POSITION posPrev = NULL;
	POSITION posCur = listHR.GetTailPosition();
	
	for (cnt=1; cnt<=cntExpected; cnt++) {
		cntInRange++;
		if (posCur==NULL) {
			printf("History list too short: % elements (% expected), ends at t=%.2f\n", cnt-1, sPrev);
			return;
		}
		sCur = (listHR.GetDataPtr(posCur))->sTimestamp;
		if (!(sCur >= 0.0)) {
			printf("History list has INVALID timestamp at position %i (t1=%.2f)\n", cnt, sCur);
			return;
		}
		if (sCur > sPrev) {
			printf("History list is non-sequential at position %i (t1=%.2f t0=%.2f dT=%.2f)\n", cnt, sCur, sPrev, sCur-sPrev);
			return;
		}
		if (sCur==sPrev) {
			if (sCur>0.0) {
				printf("History list has duplicate timestamp at position %i (t1=%.2f)\n", cnt, sCur);
				return;
			}
		} else if (sStep>1.0 && cntInRange>1) {
			float sDelta = std::abs(sPrev-sCur-sStep);
			if (sCur != 0.0  &&  sDelta > sStep*0.5)
				printf("T=%i History list: gap of %.2f (vs. %i) #%i is %i in range  [%i]t=%.2f [%i]t=%.2f [%i]t=%.2f\n", 
							(int)round(sStart), sPrev-sCur, (int)round(sStep), cnt, cntInRange,
									 cnt+1, (listHR.GetDataPtr(listHR.PrevAt(posCur)))->sTimestamp, cnt, sCur, cnt-1, sPrev);
		}
		if (listHR.NextAt(posCur) != posPrev) {
			printf("History list has bad backlink at position %i (t=%.2f seconds)\n", cnt, sCur);
			return;
		}
		if (cntInRange > nPointsPerRange+1) {
			printf("History list: range %i is too long (>=%i points, expected %i +/-1)\n", cntRanges, cntInRange, nPointsPerRange);
		}
		if (posCur == *ppos) {
			ppos++;
			cntRanges++;
			if (cntInRange < nPointsPerRange-1) {
				printf("History list: range %i is too short (<>=%i points, expected %i +/-1)\n", cntRanges, cntInRange, nPointsPerRange);
			}
			cntInRange = 0;
			sStep *= 2.0;
		}
		posPrev = posCur;
		posCur = listHR.PrevAt(posCur);
		sPrev = sCur;
	}
	
	if (posCur != NULL) {
		printf("History list: head->prev!=NULL (t=%.2f)\n", sCur);
		return;
	}
	if (bMsgIfOk)
		printf("History list is OK (t=%f)\n", sStart);
#endif
}


void CStatisticsDlg::UpdateStatGraphs(bool bStatsVisible)
{
	if (!bStatsVisible) {
		pscopeDL->DelayPoints();
		pscopeUL->DelayPoints();
		pscopeConn->DelayPoints();
	}
	// Update scaling of the active connections graph
/*  this older variant of the code allows for up-scaling (e.g. 3:1) in addition to down-scaling
	but this seems rather pointless, because the number of active connections will always be
	higher than the number of active up/downloads, so if their scale is appropriate, then 
	(1:1) should be the lower limit of the active connections scale. -- Emilio
	static uint32 savedMaxConns;
	if (savedMaxConns != peakconnections) {
		savedMaxConns = peakconnections;
		unsigned nMult, 
		unsigned nDiv;
		unsigned nMax = theApp.glob_prefs->GetStatsMax();
		float fUpperLimit;
		if (savedMaxConns < nMax) {
			nMult = nMax / savedMaxConns;
			nDiv = 1;
			fUpperLimit = (float)nMax / (float)nMult;
		} else {
			nDiv = (savedMaxConns+nMax-1) / nMax;
			nMult = 1;
			fUpperLimit = (float)(nMax * nDiv);
		}
		wxString txt = wxString::Format(CString(_("Active connections (%u:%u)")), nMult, nDiv);
		wxStaticCast(wxWindow::FindWindowById(ID_ACTIVEC),wxStaticText)->SetLabel(txt);
		pscopeConn->SetRange(0.0, fUpperLimit, 1);
	}
*/
	static unsigned nScalePrev=1;
	unsigned nScale = (unsigned)std::ceil((float)peakconnections / pscopeConn->GetUpperLimit());
	if (nScale != nScalePrev) {
		nScalePrev = nScale;
		wxString txt = wxString::Format(CString(_("Active connections (1:%u)")), nScale);
		wxStaticCast(wxWindow::FindWindowById(ID_ACTIVEC),wxStaticText)->SetLabel(txt);
		pscopeConn->SetRange(0.0, (float)nScale*pscopeConn->GetUpperLimit(), 1);
	}
	if (!bStatsVisible)
		return;
	
	HR* phr = listHR.GetDataPtr(listHR.GetTailPosition());
	float cUp, cDown, cConn;
	cUp   = (float)phr->cntUploads;
	cDown = (float)phr->cntConnections;
	cConn = (float)phr->cntDownloads;
	float *apfDown[] = { &kBpsDownSession, &kBpsDownAvg, &kBpsDownCur };
	float *apfUp[] = { &kBpsUpSession, &kBpsUpAvg, &kBpsUpCur };
	float *apfConn[] = { &cUp, &cDown, &cConn };
	pscopeDL->AppendPoints(phr->sTimestamp, apfDown);
	pscopeUL->AppendPoints(phr->sTimestamp, apfUp);
	pscopeConn->AppendPoints(phr->sTimestamp, apfConn);
}


void CStatisticsDlg::SetUpdatePeriod()
{
	// this gets called after the value in Preferences/Statistics/Update delay has been changed
	double sStep = theApp.glob_prefs->GetTrafficOMeterInterval();
	if (sStep == 0.0) {
	 	pscopeDL->Stop();
 		pscopeUL->Stop();
	 	pscopeConn->Stop();
	} else {
	 	pscopeDL->Reset(sStep);
 		pscopeUL->Reset(sStep);
	 	pscopeConn->Reset(sStep);
	}
}


void CStatisticsDlg::ResetAveragingTime()
{
	// this gets called after the value in Preferences/Statistics/time for running avg has been changed
 	pscopeDL->InvalidateGraph();
 	pscopeUL->InvalidateGraph();
}


void CStatisticsDlg::UpdateConnectionsStatus()
{
	if( theApp.serverconnect->IsConnected() ) {
		totalconnectionchecks++;
		float percent;
		percent = (float)((float)(totalconnectionchecks-1)/(float)totalconnectionchecks);
		if( percent > .99 ) {
			percent = (float).99;
			}
		averageconnections = (averageconnections*percent) + (float)((float)activeconnections*(float)(1-percent));
	}
}

	
	
float CStatisticsDlg::GetMaxConperFiveModifier(){
	//This is an alpha test.. Will clean up for b version.
	float SpikeSize = theApp.listensocket->GetOpenSockets() - averageconnections ;
	if ( SpikeSize < 1 ) {
		return 1;
	}
	float SpikeTolerance = 25*(float)theApp.glob_prefs->GetMaxConperFive()/(float)10;
	if ( SpikeSize > SpikeTolerance ) {
		return 0;
	}
	float Modifier = (1-(SpikeSize/SpikeTolerance));
	return Modifier;
}


void  CStatisticsDlg::InitTree()
{
	wxTreeItemId root=stattree->AddRoot(_("Statistics"));
	h_uptime = stattree->InsertItem(root,0L,_("Waiting..."));
	h_transfer= stattree->AppendItem(root,CString(_("Transfer")));
	tran0= stattree->AppendItem(h_transfer,_("Waiting..."));

	h_upload = stattree->AppendItem(h_transfer,CString(_("Uploads")));
	up1= stattree->AppendItem(h_upload,_("Waiting..."));
	up2= stattree->AppendItem(h_upload,_("Waiting..."));
	up3= stattree->AppendItem(h_upload,_("Waiting..."));
	up4= stattree->AppendItem(h_upload,_("Waiting..."));
	up5= stattree->AppendItem(h_upload,_("Waiting..."));
	up6= stattree->AppendItem(h_upload,_("Waiting..."));
	up7= stattree->AppendItem(h_upload,_("Waiting..."));
	up8= stattree->AppendItem(h_upload,_("Waiting..."));
	up9= stattree->AppendItem(h_upload,_("Waiting..."));
	up10= stattree->AppendItem(h_upload,_("Waiting..."));

	h_download = stattree->AppendItem(h_transfer,CString(_("Downloads")));
	down1= stattree->AppendItem(h_download,_("Waiting..."));
	down2= stattree->AppendItem(h_download,_("Waiting..."));
	down3= stattree->AppendItem(h_download,_("Waiting..."));
	down4= stattree->AppendItem(h_download,_("Waiting..."));
	down5= stattree->AppendItem(h_download,_("Waiting..."));
	down6= stattree->AppendItem(h_download,_("Waiting..."));
	down7= stattree->AppendItem(h_download,_("Waiting..."));

	h_connection = stattree->AppendItem(root,CString(_("Connection")));
	con1= stattree->AppendItem(h_connection,_("Waiting..."));
	con2= stattree->AppendItem(h_connection,_("Waiting..."));
	con12=stattree->AppendItem(h_connection,_("Waiting..."));
	con13=stattree->AppendItem(h_connection,_("Waiting..."));
	con3= stattree->AppendItem(h_connection,_("Waiting..."));
	con4= stattree->AppendItem(h_connection,_("Waiting..."));
	con5= stattree->AppendItem(h_connection,_("Waiting..."));
	con6= stattree->AppendItem(h_connection,_("Waiting..."));
	con7= stattree->AppendItem(h_connection,_("Waiting..."));
	con8= stattree->AppendItem(h_connection,_("Waiting..."));
	con9= stattree->AppendItem(h_connection,_("Waiting..."));

	h_clients = stattree->AppendItem(root,CString(_("Clients")));
	cli1= stattree->AppendItem(h_clients,_("Waiting..."));
	cli_versions[0]= stattree->AppendItem(cli1,_("Waiting..."));
	cli_versions[1]= stattree->AppendItem(cli1,_("Waiting..."));
	cli_versions[2]= stattree->AppendItem(cli1,_("Waiting..."));
	cli_versions[3]= stattree->AppendItem(cli1,_("Waiting..."));
	cli10= stattree->AppendItem(h_clients,_("Waiting..."));
	cli8= stattree->AppendItem(h_clients,_("Waiting..."));
	cli2= stattree->AppendItem(h_clients,_("Waiting..."));
	cli_versions[4]= stattree->AppendItem(cli2,_("Waiting..."));
	cli_versions[5]= stattree->AppendItem(cli2,_("Waiting..."));
	cli_versions[6]= stattree->AppendItem(cli2,_("Waiting..."));
	cli_versions[7]= stattree->AppendItem(cli2,_("Waiting..."));
	cli3= stattree->AppendItem(h_clients,_("Waiting..."));
	cli_versions[8]= stattree->AppendItem(cli3,_("Waiting..."));
	cli_versions[9]= stattree->AppendItem(cli3,_("Waiting..."));
	cli_versions[10]= stattree->AppendItem(cli3,_("Waiting..."));
	cli_versions[11]= stattree->AppendItem(cli3,_("Waiting..."));
	cli4= stattree->AppendItem(h_clients,_("Waiting..."));
	cli5= stattree->AppendItem(h_clients,_("Waiting..."));
	cli9= stattree->AppendItem(h_clients,_("Waiting..."));
	cli6= stattree->AppendItem(h_clients,_("Waiting..."));
	cli7= stattree->AppendItem(h_clients,CString(_("Filtered: %i")));

	h_servers = stattree->AppendItem(root,CString(_("Servers")));
	srv1= stattree->AppendItem(h_servers,_("Waiting..."));
	srv2= stattree->AppendItem(h_servers,_("Waiting..."));
	srv3= stattree->AppendItem(h_servers,_("Waiting..."));
	srv4= stattree->AppendItem(h_servers,_("Waiting..."));
	srv5= stattree->AppendItem(h_servers,_("Waiting..."));
	srv6= stattree->AppendItem(h_servers,_("Waiting..."));
	srv7= stattree->AppendItem(h_servers,_("Waiting..."));
	srv8= stattree->AppendItem(h_servers,_("Waiting..."));
	srv9= stattree->AppendItem(h_servers,_("Waiting..."));

	h_shared = stattree->AppendItem(root, CString(_("Shared Files")) );
	shar1= stattree->AppendItem(h_shared,_("Waiting..."));
	shar2= stattree->AppendItem(h_shared,_("Waiting..."));
	shar3= stattree->AppendItem(h_shared,_("Waiting..."));
  
	stattree->Expand(root);
	stattree->Expand(h_uptime);
	stattree->Expand(h_transfer);//,TVE_EXPAND);
	stattree->Expand(h_connection);//,TVE_EXPAND);
	stattree->Expand(h_clients);//,TVE_EXPAND);
	stattree->Expand(h_servers);//,TVE_EXPAND);
	stattree->Expand(h_shared);// ,TVE_EXPAND);
	stattree->Expand(h_upload);//,TVE_EXPAND);
	stattree->Expand(h_download);//,TVE_EXPAND);
}


void CStatisticsDlg::ShowStatistics()
{
	CString cbuffer;
	CString cbuffer2;
	bool resize;
	DWORD running;
	int myStats[11];

	resize=false;
	theApp.downloadqueue->GetDownloadStats(myStats);

	uint64 DownOHTotal = theApp.downloadqueue->GetDownDataOverheadFileRequest() + theApp.downloadqueue->GetDownDataOverheadSourceExchange() + theApp.downloadqueue->GetDownDataOverheadServer() + theApp.downloadqueue->GetDownDataOverheadOther();
	uint64 DownOHTotalPackets = theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() + theApp.downloadqueue->GetDownDataOverheadServerPackets() + theApp.downloadqueue->GetDownDataOverheadOtherPackets();
	cbuffer.Format(CString(_("Uptime: ")));
	stattree->SetItemText(h_uptime, cbuffer);
	cbuffer.Format(CString(_("Downloaded Data (Session (Total)): %s (%s)")),CastItoXBytes( theApp.stat_sessionReceivedBytes).GetData(),	CastItoXBytes( theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded()).GetData());
	stattree->SetItemText(down1, cbuffer);
	cbuffer.Format(CString(_("Total Overhead (Packets): %s (%s)")),	CastItoXBytes(DownOHTotal).GetData(), CastItoIShort(DownOHTotalPackets).GetData());
	stattree->SetItemText(down2, cbuffer);
	cbuffer.Format(CString(_("File Request Overhead (Packets): %s (%s)")), CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadFileRequest()).GetData(), CastItoIShort(theApp.downloadqueue->GetDownDataOverheadFileRequestPackets()).GetData());
	stattree->SetItemText(down3, cbuffer);
	cbuffer.Format(CString(_("Source Exchange Overhead (Packets): %s (%s)")), CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadSourceExchange()).GetData(), CastItoIShort(theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets()).GetData() );
	stattree->SetItemText(down4, cbuffer);
	cbuffer.Format(CString(_("Server Overhead (Packets): %s (%s)")), CastItoXBytes( theApp.downloadqueue->GetDownDataOverheadServer()).GetData(), CastItoIShort(theApp.downloadqueue->GetDownDataOverheadServerPackets()).GetData());
	stattree->SetItemText(down5, cbuffer);
	cbuffer.Format(CString(_("Found Sources: %i")),myStats[0]);
	stattree->SetItemText(down6, cbuffer);
	cbuffer.Format(CString(_("Active Downloads (chunks): %i")),myStats[1]);
	stattree->SetItemText(down7, cbuffer);
	uint64 UpOHTotal = theApp.uploadqueue->GetUpDataOverheadFileRequest() + theApp.uploadqueue->GetUpDataOverheadSourceExchange() + theApp.uploadqueue->GetUpDataOverheadServer() + theApp.uploadqueue->GetUpDataOverheadOther();
	uint64 UpOHTotalPackets = theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets() + theApp.uploadqueue->GetUpDataOverheadServerPackets() + theApp.uploadqueue->GetUpDataOverheadOtherPackets();
	cbuffer.Format(CString(_("Uploaded Data (Session (Total)): %s (%s)")),CastItoXBytes( theApp.stat_sessionSentBytes).GetData(),CastItoXBytes( theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded()).GetData());
	stattree->SetItemText(up1, cbuffer);
	cbuffer.Format(CString(_("Total Overhead (Packets): %s (%s)")), CastItoXBytes( UpOHTotal).GetData(), CastItoIShort(UpOHTotalPackets).GetData());
	stattree->SetItemText(up2, cbuffer);
	cbuffer.Format(CString(_("File Request Overhead (Packets): %s (%s)")), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadFileRequest()).GetData(), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadFileRequestPackets()).GetData());
	stattree->SetItemText(up3, cbuffer);
	cbuffer.Format(CString(_("Source Exchange Overhead (Packets): %s (%s)")), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadSourceExchange()).GetData(), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets()).GetData());
	stattree->SetItemText(up4, cbuffer);
	cbuffer.Format(CString(_("Server Overhead (Packets): %s (%s)")), CastItoXBytes( theApp.uploadqueue->GetUpDataOverheadServer()).GetData(), CastItoIShort(theApp.uploadqueue->GetUpDataOverheadServerPackets()).GetData());
	stattree->SetItemText(up5, cbuffer);
	cbuffer.Format(CString(_("Active Uploads: %i")),theApp.uploadqueue->GetUploadQueueLength());
	stattree->SetItemText(up6, cbuffer);
	cbuffer.Format(CString(_("Waiting Uploads: %i")),theApp.uploadqueue->GetWaitingUserCount());
	stattree->SetItemText(up7, cbuffer);
	cbuffer.Format(CString(_("Total successful upload sessions: %i")),theApp.uploadqueue->GetSuccessfullUpCount());
	stattree->SetItemText(up8, cbuffer);
	cbuffer.Format(CString(_("Total failed upload sessions: %i")),theApp.uploadqueue->GetFailedUpCount());
	stattree->SetItemText(up9, cbuffer);
	running=theApp.uploadqueue->GetAverageUpTime();
	cbuffer.Format(CString(_("Average upload time: %s")),CastSecondsToHM(running).GetData());
	stattree->SetItemText(up10, cbuffer);

	if (theApp.stat_transferStarttime>0) {
		cbuffer.Format(CString(_("Average Downloadrate (Session): %.2f kB/s")),kBpsDownSession);
		stattree->SetItemText(con1, cbuffer);

		cbuffer.Format(CString(_("Average Uploadrate (Session): %.2f kB/s")),GetKBpsUpSession());
		stattree->SetItemText(con2, cbuffer);

		cbuffer.Format(CString(_("Max Downloadrate Average (Session): %.2f kB/s")),maxDownavg);
		stattree->SetItemText(con12, cbuffer);
		cbuffer.Format(CString(_("Max Downloadrate (Session): %.2f kB/s")),maxDown);
		stattree->SetItemText(con13, cbuffer);
	}
	if (theApp.stat_reconnects>0) {
		cbuffer.Format(CString(_("Reconnects: %i")),theApp.stat_reconnects-1);
	} else {
		cbuffer.Format(CString(_("Reconnects: %i")),0);
	}
	stattree->SetItemText(con3, cbuffer);

	if (theApp.stat_transferStarttime==0) {
		cbuffer.Format(CString(_("waiting for transfer...")));
	} else {
		cbuffer.Format(CString(_("Time Since First Transfer: %s")),CastSecondsToHM(theApp.GetTransferSecs()).GetData());
	}
	stattree->SetItemText(con4, cbuffer);

	if (theApp.Start_time>0) {
		cbuffer.Format(CString(_("Uptime: %s")), CastSecondsToHM(theApp.GetUptimeSecs()).GetData());
		stattree->SetItemText(h_uptime, cbuffer);
	}

	if (theApp.stat_serverConnectTime==0) {
		cbuffer.Format(CString(_("waiting for connection...")));
	}	else {
		cbuffer.Format(CString(_("Connected To Server Since: %s")),CastSecondsToHM(theApp.GetServerSecs()).GetData());
	}
	stattree->SetItemText(con5, cbuffer);

	if (theApp.stat_sessionReceivedBytes>0 && theApp.stat_sessionSentBytes>0 ) {
		if (theApp.stat_sessionReceivedBytes<theApp.stat_sessionSentBytes) {
			cbuffer.Format("%s %.2f : 1",CString(_("Session UL:DL Ratio (Total):")).GetData(),(float)theApp.stat_sessionSentBytes/theApp.stat_sessionReceivedBytes);
			stattree->SetItemText(tran0, cbuffer);
		} else {
			cbuffer.Format("%s 1 : %.2f",CString(_("Session UL:DL Ratio (Total):")).GetData(),(float)theApp.stat_sessionReceivedBytes/theApp.stat_sessionSentBytes);
			stattree->SetItemText(tran0, cbuffer);
		}
	} else {
			cbuffer.Format(_("%s Not available"),CString(_("Session UL:DL Ratio (Total):")).GetData());
			stattree->SetItemText(tran0, cbuffer);
	}


	// shared files stats
	cbuffer.Format(CString(_("Number of Shared Files: %i")),theApp.sharedfiles->GetCount());stattree->SetItemText(shar1, cbuffer);

	uint64 allsize=theApp.sharedfiles->GetDatasize();
	cbuffer.Format(CString(_("Total size of Shared Files: %s")),CastItoXBytes(allsize).GetData());stattree->SetItemText(shar2, cbuffer);
	
	if(theApp.sharedfiles->GetCount() != 0) {
		cbuffer2.Format( "%s", CastItoXBytes((uint64)allsize/theApp.sharedfiles->GetCount()).GetData());
	} else {
		cbuffer2 = "-";
	}

	cbuffer.Format(CString(_("Average filesize: %s")),cbuffer2.GetData());
	stattree->SetItemText(shar3, cbuffer);
	// get clientversion-counts

	// statsclientstatus : original idea and code by xrmb
	
	CMap<uint8, uint8, uint32, uint32> clientStatus;
	CMap<uint16, uint16, uint32, uint32> clientVersionEDonkey;
	CMap<uint16, uint16, uint32, uint32> clientVersionEDonkeyHybrid;
	CMap<uint8, uint8, uint32, uint32> clientVersionEMule;
	uint32 totalclient;
	theApp.clientlist->GetStatistics(totalclient, myStats, &clientStatus, &clientVersionEDonkey, &clientVersionEDonkeyHybrid, &clientVersionEMule);
	totalclient -= myStats[0];
	if( !totalclient ) {
		totalclient = 1;
	}
	cbuffer.Format(_("eMule: %i (%1.1f%%)"),myStats[2],(double)100*myStats[2]/totalclient);stattree->SetItemText(cli1, cbuffer);
	cbuffer.Format(_("aMule: %i (%1.1f%%)"),myStats[8],(double)100*myStats[8]/totalclient);stattree->SetItemText(cli10, cbuffer);
	cbuffer.Format(_("lMule/xMule: %i (%1.1f%%)"),myStats[6],(double)100*myStats[6]/totalclient);stattree->SetItemText(cli8, cbuffer);
	cbuffer.Format(_("eDonkeyHybrid: %i (%1.1f%%)"),myStats[4],(double)100*myStats[4]/totalclient);stattree->SetItemText(cli2, cbuffer);
	cbuffer.Format(_("eDonkey: %i (%1.1f%%)"),myStats[1],(double)100*myStats[1]/totalclient);stattree->SetItemText(cli3, cbuffer);
	cbuffer.Format(_("cDonkey: %i (%1.1f%%)"),myStats[5],(double)100*myStats[5]/totalclient);stattree->SetItemText(cli4, cbuffer);
	cbuffer.Format(_("Old MLDonkey: %i (%1.1f%%)"),myStats[3],(double)100*myStats[3]/totalclient);stattree->SetItemText(cli5, cbuffer);
	cbuffer.Format(_("New MLDonkey: %i (%1.1f%%)"),myStats[7],(double)100*myStats[7]/totalclient);stattree->SetItemText(cli9, cbuffer);
	cbuffer.Format(CString(_("Unknown: %i")).GetData(),myStats[0]);stattree->SetItemText(cli6, cbuffer);
	cbuffer.Format(CString(_("Filtered: %i")).GetData(),theApp.stat_filteredclients);stattree->SetItemText(cli7, cbuffer);
	
	if(stattree->IsExpanded(cli3)) {

		uint32 totcnt = myStats[1];

		//--- find top 4 eDonkey client versions ---
		uint32	currtop = 0;
		uint32	lasttop = 0xFFFFFFFF;
		for(uint32 i=0; i<4; i++) {
			POSITION pos=clientVersionEDonkey.GetStartPosition();
			uint32 topver=0;
			uint32 topcnt=0;
			double topper=0;
			while(pos) {
				uint16	ver;
				uint32	cnt;
				clientVersionEDonkey.GetNextAssoc(pos, ver, cnt);
				if(currtop<ver && ver<lasttop && ver != 0x91) {
					double percent = (double)cnt/totcnt;
					if( lasttop == 0xFFFFFFFF && ((totcnt > 75 && ((cnt <= 2) || percent < 0.01)) || (totcnt > 50 && cnt <= 1))) {
						continue;
					}
					topper=percent;
					topver=ver;
					topcnt=cnt;
					currtop=ver;
				}
			}
			lasttop=currtop;
			currtop=0;
			if(topcnt) {
				cbuffer.Format("v%i: %i (%1.1f%%)", topver, topcnt, topper*100);
			} else {
				cbuffer="";
			}
			stattree->SetItemText(cli_versions[i+8], cbuffer);
		}
	}

	if(stattree->IsExpanded(cli2)) {

		uint32 totcnt = myStats[4];

		//--- find top 4 eDonkey Hybrid client versions ---
		uint32	currtop = 0;
		uint32	lasttop = 0xFFFFFFFF;
		for(uint32 i=0; i<4; i++) {
			POSITION pos=clientVersionEDonkeyHybrid.GetStartPosition();
			uint32 topver=0;
			uint32 topcnt=0;
			double topper=0;
			while(pos) {
				uint16	ver;
				uint32	cnt;
				clientVersionEDonkeyHybrid.GetNextAssoc(pos, ver, cnt);
				if(currtop<ver && ver<lasttop && ver != 0x91) {
					double percent = (double)cnt/totcnt;
					if( lasttop == 0xFFFFFFFF && ((totcnt > 75 && ((cnt <= 2) || percent < 0.01)) || (totcnt > 50 && cnt <= 1))) {
						continue;
					}
					topper=percent;
					topver=ver;
					topcnt=cnt;
					currtop=ver;
				}
			}
			lasttop=currtop;
			currtop=0;
			if(topcnt) {
				cbuffer.Format("v%i: %i (%1.1f%%)", topver, topcnt, topper*100);
			} else {
				cbuffer="";
			}
			stattree->SetItemText(cli_versions[i+4], cbuffer);
		}
	}

	if(stattree->IsExpanded(cli1)) {
		uint32 totcnt = myStats[2];

		//--- find top 4 eMule client versions ---
		uint32	currtop = 0;
		uint32	lasttop = 0xFFFFFFFF;
		for(uint32 i=0; i<4; i++) {
			POSITION pos=clientVersionEMule.GetStartPosition();
			uint32 topver=0;
			uint32 topcnt=0;
			double topper=0;
			while(pos) {
				uint8	ver;
				uint32	cnt;
				clientVersionEMule.GetNextAssoc(pos, ver, cnt);
				if(currtop<ver && ver<lasttop )	{
					double percent = (double)cnt/totcnt;
					if( lasttop == 0xFFFFFFFF && ((totcnt > 75 && ((cnt <= 2) || percent < 0.01)) || (totcnt > 50 && cnt <= 1))) {
						continue;
					}
					topper=percent;
					topver=ver;
					topcnt=cnt;
					currtop=ver;
				}
			}
			lasttop=currtop;
			currtop=0;
			if(topcnt) {
				cbuffer.Format("v0.%02X: %i (%1.1f%%)", topver, topcnt, topper*100);
			} else {
				cbuffer="";
			}
			stattree->SetItemText(cli_versions[i], cbuffer);
		}
	}

	// get serverstats
	uint32 servtotal;
	uint32 servfail;
	uint32 servuser;
	uint32 servfile;
	uint32 servtuser;
	uint32 servtfile;
	float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servtuser, servtfile,servocc);
	cbuffer.Format("%s: %i",CString(_("Working Servers")).GetData(),servtotal-servfail);stattree->SetItemText(srv1, cbuffer);
	cbuffer.Format("%s: %i",CString(_("Failed Servers")).GetData(),servfail);stattree->SetItemText(srv2, cbuffer);
	cbuffer.Format("%s: %i",CString(_("Total")).GetData(),servtotal);stattree->SetItemText(srv3, cbuffer);
	cbuffer.Format("%s: %i",CString(_("Deleted Servers")).GetData(),theApp.serverlist->GetDeletedServerCount());stattree->SetItemText(srv4, cbuffer);
	cbuffer.Format("%s: %i",CString(_("Users on Working Servers")).GetData(),servuser);stattree->SetItemText(srv5, cbuffer);
	cbuffer.Format("%s: %i",CString(_("Files on Working Servers")).GetData(),servfile);stattree->SetItemText(srv6, cbuffer);
	cbuffer.Format("%s: %i",CString(_("Total Users")).GetData(),servtuser);stattree->SetItemText(srv7, cbuffer);
	cbuffer.Format("%s: %i",CString(_("Total Files")).GetData(),servtfile);stattree->SetItemText(srv8, cbuffer);
	cbuffer.Format("%s: %i",CString(_("Active Connections (estimate)")).GetData(),activeconnections);stattree->SetItemText(con6, cbuffer);
	cbuffer.Format(CString(_("Server Occupation: %.2f%%")),servocc);stattree->SetItemText(srv9, cbuffer);
	uint32 m_itemp = theApp.listensocket->GetMaxConnectionReached();
	if( m_itemp != m_ilastMaxConnReached ) {
		char osDate[60];
		//_strtime( osTime );
		//_strdate( osDate );

		time_t mytime=time(NULL);
		struct tm* now=localtime(&mytime);
		strftime(osDate,sizeof(osDate)-1,"%d.%m.%Y %H:%M:%S",now);

		cbuffer.Format("%s: %i : %s",CString(_("Max Connection Limit Reached")).GetData(),m_itemp,osDate);stattree->SetItemText(con7, cbuffer);
		m_ilastMaxConnReached = m_itemp;
	} else if( m_itemp == 0 ) {
		cbuffer.Format("%s: %i",CString(_("Max Connection Limit Reached")).GetData(),m_itemp);
		stattree->SetItemText(con7, cbuffer);
	}

	if(theApp.serverconnect->IsConnected()) {
		cbuffer.Format("%s: %i",CString(_("Average Connections (estimate)")).GetData(),(int)averageconnections);
		stattree->SetItemText(con8, cbuffer);
	} else {
		stattree->SetItemText(con8, CString(_("waiting for connection...")));
	}
	cbuffer.Format("%s: %i",CString(_("Peak Connections (estimate)")).GetData(),peakconnections);
	stattree->SetItemText(con9, cbuffer);
}


// the idea: we must dispatch this request to the main thread as web thread
// can't touch the GUI controls. Only allowed function is wxPostEvent which
// unfortunately just passes the event, so we must implement a wxCondition
// so we can wait until the main thread has processed the request and built
// a valid string for us.

wxString CStatisticsDlg::ExportHTML() {
	// only one shall pass
	wxCriticalSectionLocker locker(exportCrit);

	// then lock the signalling mutex
	wxCondition myCond(exportMutex);
	exportCondition=&myCond;
	exportMutex.Lock();

	// post the message
	wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,ID_EXPORT_HTML);
	wxPostEvent(this,evt);

	// and wait until it is completed
	exportCondition->Wait();

	// and now we can return the string
	exportMutex.Unlock();
	printf("*** WEB THREAD: returning the value\n");
	return exportString;
}

void CStatisticsDlg::ExportHTMLEvent(wxEvent& evt)
{
	int8 ix;
	wxString temp;
	wxString text="";
	wxTreeItemId item;

	// update it
	ShowStatistics();

	item=stattree->GetRootItem();
	text=stattree->GetItemText(item);
	while (item) {
		item=stattree->GetNextVisible(item);
		if (!item) {
			break;
		}
		stattree->Expand(item);//,TVE_EXPAND);

		temp="";
		for (ix=0;ix<3*(int)stattree->GetItemData(item);ix++) {
			temp+="&nbsp;";
			}
		text+="<br>"+temp+stattree->GetItemText(item);
		}
 
	// set the string
	exportString=text;
	// and signal
	printf("** MAIN THREAD: Return value ready.\n");
	wxMutexLocker locker(exportMutex);
	exportCondition->Broadcast();
	printf("** MAIN THREAD: All done.\n");
}

// This is the primary function for generating HTML output of the statistics tree.
wxString CStatisticsDlg::GetHTML() {
	
	int8 ix;
	wxString temp;
	wxString strBuffer="";
	wxTreeItemId item;

	strBuffer.Printf("<font face=\"Verdana,Courier New,Helvetica\" size=\"2\">\r\n<b>aMule v%s %s [%s]</b>\r\n<br><br>\r\n", PACKAGE_VERSION, GetResString(IDS_SF_STATISTICS).GetData(), theApp.glob_prefs->GetUserNick());
	// update it
	ShowStatistics();

	item=stattree->GetRootItem();
	strBuffer+=stattree->GetItemText(item);
	while (item) {
		item=stattree->GetNextVisible(item);
		if (!item) {
			break;
		}
		stattree->Expand(item);

		temp="";
		wxTreeItemId tempItem = item;
		int level = 0;
		while (tempItem = stattree->GetItemParent(tempItem)) level+=1;
		for (ix=0;ix<level;ix++)
			temp+="&nbsp;&nbsp;&nbsp;";
		
		strBuffer+="<br>"+temp+stattree->GetItemText(item);
	}
 	strBuffer+="</font>";

	// return the string
	return(strBuffer);
}