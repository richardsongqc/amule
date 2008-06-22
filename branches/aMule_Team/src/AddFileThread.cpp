// This file is part of the aMule project.
//
// Copyright (c) 2003,
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

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/event.h>		// Needed for wxCommandEvent

#include "AddFileThread.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for nstrdup
#include "opcodes.h"		// Needed for TM_HASHTHREADFINISHED
#include "CamuleAppBase.h"	// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "KnownFile.h"		// Needed for CKnownFile

struct UnknownFile_Struct
{
	char*		name;
	char*		directory;
	CPartFile*	owner;
};

wxMutex CAddFileThread::m_lockWaitingForHashList;
wxCondition CAddFileThread::m_runWaitingForHashList(m_lockWaitingForHashList);
CTypedPtrList<CPtrList, UnknownFile_Struct*> CAddFileThread::m_sWaitingForHashList;
volatile int CAddFileThread::m_endWaitingForHashList;

CAddFileThread::CAddFileThread() : wxThread(wxTHREAD_DETACHED)
{
}

void CAddFileThread::Setup()
{
	CAddFileThread *th = new CAddFileThread();
	th->Create();
	th->Run();
}

void CAddFileThread::Shutdown()
{
	m_lockWaitingForHashList.Lock();

	printf("Signaling hashing thread to terminate\n");

	// Tell the thread to exit
	m_endWaitingForHashList = 1;

	// Signal the thread there is something to do
	m_runWaitingForHashList.Signal();

	m_lockWaitingForHashList.Unlock();
}

void CAddFileThread::AddFile(const char *path, const char *name, CPartFile* part)
{
	UnknownFile_Struct* hashfile = new UnknownFile_Struct;
	hashfile->directory = nstrdup(path);
	hashfile->name = nstrdup(name);
	hashfile->owner = part;

	wxMutexLocker sLock(m_lockWaitingForHashList);
	m_sWaitingForHashList.AddTail(hashfile);
	m_runWaitingForHashList.Signal();
}

wxThread::ExitCode CAddFileThread::Entry()
{
   while ( 1 ) {
	m_lockWaitingForHashList.Lock();

	if (m_endWaitingForHashList) {
  		m_lockWaitingForHashList.Unlock();

		wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_HASHTHREADFINISHED);
    		wxPostEvent(theApp.amuledlg,evt);
		return 0;
	}

  	if (m_sWaitingForHashList.IsEmpty()) {
		m_runWaitingForHashList.Wait();
  		m_lockWaitingForHashList.Unlock();
  		continue;
  	}

	UnknownFile_Struct* hashfile = m_sWaitingForHashList.RemoveHead();
  	m_lockWaitingForHashList.Unlock();

	CKnownFile* newrecord = new CKnownFile();
  	printf("Sharing %s/%s\n",hashfile->directory,hashfile->name);

	// TODO: What we are supposed to do if the following does fail?
	newrecord->CreateFromFile(hashfile->directory,hashfile->name,&m_endWaitingForHashList);
	if (!m_endWaitingForHashList) {
		wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_FINISHEDHASHING);
		evt.SetClientData(newrecord);
		evt.SetExtraLong((long)hashfile->owner);
		wxPostEvent(theApp.amuledlg,evt);
	}

	delete[] hashfile->name;
	delete[] hashfile->directory;
  	delete hashfile;
    }
}

int CAddFileThread::GetCount()
{
	return m_sWaitingForHashList.GetCount();
}