// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
// Copyright (C) 2002 Tiku ( ) & Hetfield <hetfield@email.it>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef HTTPDOWNLOAD_H
#define HTTPDOWNLOAD_H

#include <wx/dialog.h>		// Needed for wxDialog

#include "GuiEvents.h"

class HTTPThread;
class wxGauge;

class MuleGifCtrl;

class CHTTPThreadDlg : public wxDialog
{
public:
	CHTTPThreadDlg(wxWindow*parent, HTTPThread* thread);
	~CHTTPThreadDlg();

	void StopAnimation(); 
	void UpdateGauge(int dltotal,int dlnow);

private:
	DECLARE_EVENT_TABLE()

	void OnBtnCancel(wxCommandEvent& evt);
  
	HTTPThread* parent_thread;
	MuleGifCtrl* ani;
	wxGauge* 	m_progressbar;
};

class HTTPThread : public wxThread
{
private:
	wxString url;
	wxString tempfile;
	wxThread::ExitCode Entry();
	int result;
	HTTP_Download_File file_type;

#ifndef AMULE_DAEMON 
	CHTTPThreadDlg* myDlg;
#endif

 public:
//	myThread::myThread(wxEvtHandler* parent,char* urlname,char* filename);
	~HTTPThread();

	HTTPThread(wxString urlname, wxString filename, HTTP_Download_File file_id);

	virtual void OnExit();
};

#endif // HTTPDOWNLOAD_H