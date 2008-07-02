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
/////////////////////////////////////////////////////////////////////////////
// Name:        file.cpp
// Purpose:     wxFile - encapsulates low-level "file descriptor"
//              wxTempFile
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "CFile.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include <unistd.h>		// Needed for close(2)

#include "CFile.h"		// Interface declarations.

#include "StringFunctions.h" // unicode2char
#include "Preferences.h"
#include "Logger.h"
#include "Format.h"

#ifdef HAVE_CONFIG_H
#include "config.h"             // Needed for HAVE_SYS_PARAM_H
#endif

// Test if we have _GNU_SOURCE before the next step will mess up 
// setting __USE_GNU 
// (only needed for gcc-2.95 compatibility, gcc 3.2 always defines it)
#include <wx/setup.h>

// Mario Sergio Fujikawa Ferreira <lioux@FreeBSD.org>
// to detect if this is a *BSD system
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef __BORLANDC__
#	pragma hdrstop
#endif

#include <wx/log.h>

// standard
#if defined(__WXMSW__) && !defined(__GNUWIN32__) && !defined(__WXWINE__) && !defined(__WXMICROWIN__)
#	include <io.h>

#ifndef __SALFORDC__
#	define   WIN32_LEAN_AND_MEAN
#	define   NOSERVICE
#	define   NOIME
#	define   NOATOM
#	define   NOGDI
#	define   NOGDICAPMASKS
#	define   NOMETAFILE
#	define   NOMINMAX
#	define   NOMSG
#	define   NOOPENFILE
#	define   NORASTEROPS
#	define   NOSCROLL
#	define   NOSOUND
#	define   NOSYSMETRICS
#	define   NOTEXTMETRIC
#	define   NOWH
#	define   NOCOMM
#	define   NOKANJI
#	define   NOCRYPT
#	define   NOMCX
#endif

#elif (defined(__UNIX__) || defined(__GNUWIN32__))
#	include <unistd.h>
#	ifdef __GNUWIN32__
#		include <windows.h>
#	endif
#elif defined(__DOS__)
#	if defined(__WATCOMC__)
#		include <io.h>
#	elif defined(__DJGPP__)
#		include <io.h>
#		include <unistd.h>
#		include <cstdio>
#	else
#		error  "Please specify the header with file functions declarations."
#	endif
#elif (defined(__WXPM__))
#	include <io.h>
#elif (defined(__WXSTUBS__))
	// Have to ifdef this for different environments
#	include <io.h>
#elif (defined(__WXMAC__))
#if __MSL__ < 0x6000
int access( const char *path, int mode ) { return 0 ; }
#else
int _access( const char *path, int mode ) { return 0 ; }
#endif
char* mktemp( char * path ) { return path ;}
#	include <stat.h>
#	include <unistd.h>
#else
#	error  "Please specify the header with file functions declarations."
#endif  //Win/UNIX

#include <cstdio>       // SEEK_xxx constants
#include <fcntl.h>       // O_RDONLY &c

#if !defined(__MWERKS__) || defined(__WXMSW__)
#	include <sys/types.h>   // needed for stat
#	include <sys/stat.h>    // stat
#endif

// Windows compilers don't have these constants
#ifndef W_OK
enum {
	F_OK = 0,   // test for existence
	X_OK = 1,   //          execute permission
	W_OK = 2,   //          write
	R_OK = 4    //          read
};
#endif // W_OK

// there is no distinction between text and binary files under Unix, so define
// O_BINARY as 0 if the system headers don't do it already
#if defined(__UNIX__) && !defined(O_BINARY)
#	define   O_BINARY    (0)
#endif  //__UNIX__

#ifdef __SALFORDC__
#	include <unix.h>
#endif

// some broken compilers don't have 3rd argument in open() and creat()
#ifdef __SALFORDC__
#	define ACCESS(access)
#	define stat    _stat
#else // normal compiler
#	define ACCESS(access)  , (access)
#endif // Salford C

// wxWindows
#ifndef WX_PRECOMP
#	include <wx/string.h>
#	include <wx/intl.h>
#	include <wx/log.h>
#endif // !WX_PRECOMP

#ifdef __WXMSW__
#include <wx/msw/mslu.h>
#endif
#include <wx/filename.h>
#include <wx/filefn.h>

// ============================================================================
// implementation of CFile
// ============================================================================

// ----------------------------------------------------------------------------
// static functions
// ----------------------------------------------------------------------------

bool CFile::Exists(const wxChar *name)
{
	return wxFileExists(name);
}

bool CFile::Access(const wxChar *name, OpenMode mode)
{
	int how;

	switch ( mode ) {
	default:
		wxFAIL_MSG(wxT("bad CFile::Access mode parameter."));
		// fall through
	case read:
		how = R_OK;
		break;
	
	case write:
		how = W_OK;
		break;
	case read_write:
		how = R_OK | W_OK;
		break;
	}

	return wxAccess(name, how) == 0;
}

// ----------------------------------------------------------------------------
// opening/closing
// ----------------------------------------------------------------------------

// ctors
CFile::CFile(const wxString& sFileName, OpenMode mode)
{
	wxASSERT(!sFileName.IsEmpty());

	m_fd = fd_invalid;
	m_error = false;
	
	Open(sFileName, mode);
}

//
// create the file, fail if it already exists and !bOverwrite
// 
// When creating files, we will always first try to create an ANSI file name,
// even if that means an extended ANSI file name. Only if it is not possible
// to do that, we fall back to  UTF-8 file names. This is unicode safe and is
// the only way to guarantee that we can later open any file in the file system,
// even if it is not an UTF-8 valid sequence.
// 
bool CFile::Create(const wxString& sFileName, bool bOverwrite, int accessMode)
{
	wxASSERT(!sFileName.IsEmpty());

	if ( accessMode == -1 ) {
		accessMode = thePrefs::GetFilePermissions();
	}
	m_filePath = sFileName;
	if (m_fd != fd_invalid) {
		Close();	
	}
	if (wxFileExists(sFileName) && !bOverwrite) {
		return false;
	}
	// Test if it is possible to use an ANSI name
	Unicode2CharBuf tmpFileName(unicode2char(sFileName));
	if (tmpFileName) {
		// Use an ANSI name
		m_fd = creat(tmpFileName, accessMode);
	} 
	
	if (m_fd == fd_invalid) { // Wrong conversion or can't create.
		// Try an UTF-8 name
		m_fd = creat(unicode2UTF8(sFileName), accessMode);
	}
	
	
	if (m_fd == fd_invalid) {
		AddDebugLogLineM( true, logCFile, 
			CFormat( wxT("Failed to created file '%s'!") )
				% m_filePath
				% m_fd );
		
		return false;
	} else {
		AddDebugLogLineM( false, logCFile, 
			CFormat( wxT("Created file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );
		
		return true;
	}
}

//
// open the file
// 
// When opening files, we will always first try to create an ANSI file name,
// even if that means an extended ANSI file name. Only if it is not possible
// to do that, we fall back to  UTF-8 file names. This is unicode safe and is
// the only way to guarantee that we can open any file in the file system,
// even if it is not an UTF-8 valid sequence.
//
bool CFile::Open(const wxString& sFileName, OpenMode mode, int accessMode)
{
	wxASSERT(!sFileName.IsEmpty());
	
	if ( accessMode == -1 ) {
		accessMode = thePrefs::GetFilePermissions();
	}
	int flags = O_BINARY;
#ifdef __linux__
	flags |=  O_LARGEFILE;
#endif
	m_filePath = sFileName;

	switch ( mode ) {
	case read:
		flags |= O_RDONLY;
		break;
	
	case write_append:
		if (CFile::Exists(sFileName))
		{
			flags |= O_WRONLY | O_APPEND;
			break;
		}
		//else: fall through as write_append is the same as write if the
		//      file doesn't exist
	
	case write:
		flags |= O_WRONLY | O_CREAT | O_TRUNC;
		break;
	
	case write_excl:
		flags |= O_WRONLY | O_CREAT | O_EXCL;
		break;

	case read_write:
		flags |= O_RDWR;
        	break;
	}

	if (m_fd != fd_invalid) {
		Close();	
	}
	// Test if it is possible to use an ANSI name
	Unicode2CharBuf tmpFileName(unicode2char(sFileName));
	if (tmpFileName) {
		// Use an ANSI name
		m_fd = open(tmpFileName, flags ACCESS(accessMode));
	} 
	
	if (m_fd == fd_invalid) { // Wrong conversion or can't open.
		// Try an UTF-8 name
		m_fd = open(unicode2UTF8(sFileName), flags ACCESS(accessMode));
	}
      
    
	if (m_fd == fd_invalid) {
		AddDebugLogLineM( true, logCFile, wxT("Failed to open file '") + sFileName + wxT("'!") );
		
		return false;
	} else {
		AddDebugLogLineM( false, logCFile,
			CFormat( wxT("Opened file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );
		
		return true;
	}    
}

//
// close
// 
bool CFile::Close() 
{
	wxASSERT(!m_filePath.IsEmpty());

	if ( IsOpened() ) {
		if (close(m_fd) == -1) {
			AddDebugLogLineM( true, logCFile,
				CFormat( wxT("Failed to close file '%s' with file descriptor '%d'.") )
					% m_filePath
					% m_fd );
			
			m_fd = fd_invalid;
			return false;
		} else {
			AddDebugLogLineM( false, logCFile,
				CFormat( wxT("Closed file '%s' with file descriptor '%d'.") )
					% m_filePath
					% m_fd );
			
			m_fd = fd_invalid;
		}
	} else {
		wxASSERT(false);
	}
	
	return true;
}

// ----------------------------------------------------------------------------
// read/write
// ----------------------------------------------------------------------------

// read
off_t CFile::Read(void *pBuf, off_t nCount) const
{
	wxCHECK( (pBuf != NULL) && IsOpened(), 0 );

#ifdef __MWERKS__
	off_t iRc = ::read(m_fd, (char*) pBuf, nCount);
#else
	off_t iRc = ::read(m_fd, pBuf, nCount);
#endif
	if ( iRc == -1 ) {
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't read from file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );
		
		m_error = true;
		return wxInvalidOffset;
	} else {
		return (off_t)iRc;
	}
}

// write
size_t CFile::Write(const void *pBuf, size_t nCount)
{
	wxASSERT(pBuf != NULL);
	wxASSERT(IsOpened());
#ifdef __MWERKS__
#if __MSL__ >= 0x6000
	size_t iRc = ::write(m_fd, (void*) pBuf, nCount);
#else
	size_t iRc = ::write(m_fd, (const char*) pBuf, nCount);
#endif
#else
	size_t iRc = ::write(m_fd, pBuf, nCount);
#endif
	if ( ((int)iRc) == -1 ) {
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't write to file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );		
		
		m_error = true;
		return (size_t)0;
	} else {
		return iRc;
	}
}

// flush
bool CFile::Flush()
{
	if ( IsOpened() ) {
#ifdef __WXMSW__
		if (_commit(m_fd) == -1) {
#else
	        if ( fsync(m_fd) == -1 ) {
#endif
			AddDebugLogLineM( true, logCFile,
				CFormat( wxT("Can't flush file '%s' with file descriptor '%d'.") )
					% m_filePath
					% m_fd );

			m_error = true;			
			return false;
		}
    }
    return true;
}

// ----------------------------------------------------------------------------
// seek
// ----------------------------------------------------------------------------
#include <cerrno>
#include <cstring>

// seek
off_t CFile::Seek(off_t ofs, CFile::SeekMode mode) const
{
	wxASSERT( IsOpened() );

	int origin;
	switch ( mode ) {
	default:
		wxFAIL_MSG(wxT("Unknown seek origin"));

	case wxFromStart:
		origin = SEEK_SET;
		break;

	case wxFromCurrent:
		origin = SEEK_CUR;
		break;

	case wxFromEnd:
		origin = SEEK_END;
		break;
	}

	off_t iRc = lseek(m_fd, ofs, origin);
	if ( iRc == -1 ) {
		AddDebugLogLineM( true, logCFile, wxString(wxT("Error in lseek: ")) + wxString(char2unicode(strerror(errno))));
		m_error = true;
		return wxInvalidOffset;
	} else {
		return (off_t) iRc;
	}
}

// get current off_t
off_t CFile::GetPosition() const
{
	wxASSERT( IsOpened() );
	
	off_t iRc = wxTell(m_fd);
	if (iRc == -1) {
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't get seek position for file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );		
		
		return wxInvalidOffset;
	} else {
		return (off_t)iRc;
	}
}

// get current file length
off_t CFile::Length() const
{
	wxASSERT( IsOpened() );
	
#ifdef __VISUALC__
	off_t iRc = _filelength(m_fd);
#else // !VC++
	off_t iRc = wxTell(m_fd);
	if ( iRc != -1 ) {
		// @ have to use const_cast :-(
		off_t iLen = ((CFile *)this)->SeekEnd();
		if ( iLen != -1 ) {
			// restore old position
			if ( ((CFile *)this)->Seek(iRc) == -1 ) {
				// error
				iLen = -1;
			}
		}
		iRc = iLen;
	}
#endif  // VC++
	
	if ( iRc == -1 ) {
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't find length of file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );		
		
		return wxInvalidOffset;
	} else {
		return (off_t)iRc;
	}
}
bool CFile::SetLength(off_t new_len) {
#ifdef __WXMSW__
	return chsize(this->fd(), new_len);
#else
	return ftruncate(this->fd(), new_len);
#endif
}	

// is end of file reached?
bool CFile::Eof() const
{
	wxASSERT( IsOpened() );
	
	off_t iRc;
	
#if defined(__DOS__) || defined(__UNIX__) || defined(__GNUWIN32__) || defined( __MWERKS__ ) || defined(__SALFORDC__)
	// @@ this doesn't work, of course, on unseekable file descriptors
	off_t ofsCur = GetPosition(),
	ofsMax = Length();
	if ( ofsCur == (off_t)wxInvalidOffset || ofsMax == (off_t)wxInvalidOffset ) {
		iRc = -1;
	} else {
		iRc = ofsCur == ofsMax;
	}
#else  // Windows and "native" compiler
	iRc = eof(m_fd);
#endif // Windows/Unix
	
	switch ( iRc ) {
	case 1:
		break;
	
	case 0:
		return false;
	
	case -1:
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't determine if the end of file is reached for file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );	
		break;
	
	default:
		wxFAIL_MSG(wxT("invalid eof() return value."));
	}
	
	return true;
}

int CFile::Stat( const wxString& file_name, struct stat *buf)
{

	Unicode2CharBuf tmpFile(unicode2char(file_name));
	
	int stat_error = -1;
	
	if (tmpFile) {
		stat_error = stat(tmpFile, buf);
	}
	
	if (stat_error) {
		stat_error = stat(unicode2UTF8(file_name), buf);
	}

	return stat_error;
}

//
// When moving file, first try an ANSI move, only then try UTF-8.
// 
bool UTF8_MoveFile(const wxString& from, const wxString& to) {
	bool ret = false;
	Unicode2CharBuf tmpFrom(unicode2char(from));
	Unicode2CharBuf tmpTo(unicode2char(to));
	if (tmpFrom) {
		if (tmpTo) {
			ret = rename(tmpFrom, tmpTo) == 0;
		} else {
			ret = rename(tmpFrom, unicode2UTF8(to)) == 0;
		}
	} else {
		if (tmpTo) {
			ret = rename(unicode2UTF8(from), tmpTo) == 0;
		} else {
			ret = rename(unicode2UTF8(from), unicode2UTF8(to)) == 0;
		}
	}

	return ret;
}

#define FILE_COPY_BUFFER 5*1024

//
// When copying file, first try an ANSI name, only then try UTF-8.
// This is done in the CFile constructor.
// 
bool UTF8_CopyFile(const wxString& from, const wxString& to)
{
	char buffer[FILE_COPY_BUFFER];
	CFile input_file(from, CFile::read);
	if (!input_file.IsOpened()) {
		AddDebugLogLineM( true, logFileIO, wxT("Error on file copy. Can't open original file: ") + from );
		return false;
	}
	CFile output_file(to, CFile::write);
	if (!output_file.IsOpened()) {
		AddDebugLogLineM( true, logFileIO, wxT("Error on file copy. Can't create destination file: ") + to );
		return false;
	}
	
	int total_read, total_write;
	while ((total_read = input_file.Read(buffer,FILE_COPY_BUFFER))) {
		if (total_read == -1) {
			AddDebugLogLineM( true, logFileIO, wxT("Unexpected error copying file! (read error)") );
			return false;
		}
		total_write = output_file.Write(buffer,total_read);
		if (total_write != total_read) {
			AddDebugLogLineM( true, logFileIO, wxT("Unexpected error copying file! (write error)") );
			return false;			
		}	
	}
	
	return true;
}

// When iterating dir, first try an ANSI file name, then try an UTF-8 file name.
CDirIterator::CDirIterator(const wxString& dir) {
	DirStr = dir;
	if (DirStr.Last() != wxFileName::GetPathSeparator()) {
		DirStr += wxFileName::GetPathSeparator();
	}
	
	DirPtr = NULL;
	
	Unicode2CharBuf tmpDir(unicode2char(dir));
	if (tmpDir) {
		DirPtr = opendir(tmpDir);
	}
	
	if (DirPtr == NULL) { // Wrong conversion or error opening
		// Try UTF8
		DirPtr = opendir(unicode2UTF8(dir));
	}
	
	if (!DirPtr) {
		AddDebugLogLineM( false, logFileIO, wxT("Error enumerating files for dir ") + dir + wxT(" (permissions?)") );
	}
}

CDirIterator::~CDirIterator() {	
	if (DirPtr) {
		closedir (DirPtr);
	}
}

wxString CDirIterator::GetFirstFile(FileType search_type, const wxString& search_mask) {
	if (!DirPtr) {
		return wxEmptyString;
	}
	seekdir(DirPtr, 0);// 2 if we want to skip . and ..
	FileMask = search_mask;
	type = search_type;
	return GetNextFile();
}

// First try an ANSI name, only then try UTF-8.
wxString  CDirIterator::GetNextFile() {

	if (!DirPtr) {
		return wxEmptyString;
	}
	struct dirent *dp;
	dp = readdir(DirPtr);
	
	bool found = false;
	wxString FoundName;
	struct stat* buf = (struct stat*)malloc(sizeof(struct stat));
	while (dp!=NULL && !found) {
		if (type == CDirIterator::Any) {
			// return anything.
			found = true;
		} else {
#if 0
			switch (dp->d_type) {
			case DT_DIR:
				if (type == CDirIterator::Dir)  {
					found = true;
				} else {
					dp = readdir(DirPtr);	
				}
				break;
			case DT_REG:
				if (type == CDirIterator::File)  {
					found = true;
				} else {
					dp = readdir(DirPtr);					
				}
				break;
			default:
#endif
				// Fallback to stat
				//
				// The file name came from the OS, it is a sequence of
				// bytes ending in a zero. First try an UTF-8 conversion,
				// so that we don't loose information. Only then stick
				// to an ANSI name. UTF82Unicode might fail because
				// dp->name may not be a valid UTF-8 sequence.
				Char2UnicodeBuf tmpFoundName(UTF82unicode(dp->d_name));
				FoundName = tmpFoundName ?
					tmpFoundName : char2unicode(dp->d_name);
				wxString FullName(DirStr + FoundName);
				// First, we try to use an ANSI name, but it might not be
				// possible to use ANSI for the full name, so we test.
				Unicode2CharBuf tmpFullName(unicode2char(FullName));		
				int stat_error = -1;
				if (tmpFullName) {
					stat_error = stat(tmpFullName, buf);
#ifndef __WXMSW__
					// Check if it is a broken symlink
					if (stat_error) {
						stat_error = lstat(tmpFullName, buf);
						if (!stat_error && S_ISLNK(buf->st_mode)) {
							// Ok, just a broken symlink. Next, please!
							dp = readdir(DirPtr);
							continue;
						}
					}
#endif
				}
				// Fallback to UTF-8
				if (stat_error) {
					Unicode2CharBuf tmpUTF8FullName(unicode2UTF8(FullName));
					stat_error = stat(tmpUTF8FullName, buf);
#ifndef __WXMSW__
					// Check if it is a broken symlink
					if (stat_error) {
						stat_error = lstat(tmpUTF8FullName, buf);
						if (!stat_error && S_ISLNK(buf->st_mode)) {
							// Ok, just a broken symlink. Next, please!
							dp = readdir(DirPtr);
							continue;
						}
					}
#endif
				}
				
				if (!stat_error) {
					if (S_ISREG(buf->st_mode)) {
						if (type == CDirIterator::File) { 
							found = true; 
						} else { 
							dp = readdir(DirPtr);
						} 
					} else {
						if (S_ISDIR(buf->st_mode)) {
							if (type == CDirIterator::Dir) {
								found = true; 
							} else { 
								dp = readdir(DirPtr);
							}
						} else {				
							// unix socket, block device, etc
							dp = readdir(DirPtr);
						}
					}
				} else {
					// Stat failed. Assert.
					printf("CFile: serious error, stat failed\n");
					wxASSERT(0);
					AddDebugLogLineM( true, logFileIO,
						wxT("Unexpected error calling stat on a file!") );
					dp = readdir(DirPtr);
				}
#if 0
				break;
			}
#endif
		}
		if (found) {
			if (	(!FileMask.IsEmpty() && !FoundName.Matches(FileMask)) ||
				FoundName.IsSameAs(wxT(".")) ||
				FoundName.IsSameAs(wxT(".."))) {
				found = false;	
				dp = readdir(DirPtr);
			}
		}
	}
	free(buf);
	if (dp != NULL) {
		return DirStr + FoundName;	
	} else {
		return wxEmptyString;
	}
}

// First try an ANSI name, only then try UTF-8.
time_t GetLastModificationTime(const wxString& file) {
	struct stat buf;
		
	int stat_error = CFile::Stat(file, &buf);
	
	if (stat_error) {
		return 0;
	} else {
		return buf.st_mtime;
	}
}

bool CheckDirExists(const wxString& dir)
{
	struct stat st;
	return (CFile::Stat(dir, &st) == 0 && ((st.st_mode & S_IFMT) == S_IFDIR));
}