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

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for wxGetTranslation
#include <wx/utils.h>
#include <wx/tokenzr.h>

#ifdef __WXMSW__
    #include <wx/msw/winundef.h>
	#include <wx/msw/registry.h>
#endif

#include "otherfunctions.h"	// Interface declarations
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"			// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "KnownFile.h"		// Needed for CAbstractFile
#include <ctype.h>

wxString ConvertChar2Unicode(const char* c_string) {
	 return wxConvLocal.cMB2WX(c_string);
}

const char* ConvertUnicode2Char(wxString unic_string) {
	return (const char*)wxConvLocal.cWX2MB(unic_string);
}

// Formats a filesize in bytes to make it suitable for displaying
wxString CastItoXBytes( uint64 count )
{
	wxString buffer;

	if (count < 1024)
		buffer.Printf( wxT("%.0f %s"), (float)count, _("Bytes") );
	else if (count < 1048576)
		buffer.Printf( wxT("%.0f %s"), (float)count/1024, _("KB") );
	else if (count < 1073741824)
		buffer.Printf( wxT("%.2f %s"), (float)count/1048576, _("MB") );
	else if (count < 1099511627776LL)
		buffer.Printf( wxT("%.2f %s"), (float)count/1073741824, _("GB") );
	else
		buffer.Printf( wxT("%.3f %s"), (float)count/1099511627776LL, _("TB") );

	return buffer;
}


wxString CastItoIShort(uint64 count)
{
	wxString output;

	if (count < 1000)
		output.Printf(wxT("%llu"), count);
	else if (count < 1000000)
		output.Printf(wxT("%.0f%s"),(float)count/1000, _("K") );
	else if (count < 1000000000)
		output.Printf(wxT("%.2f%s"),(float)count/1000000, _("M") );
	else if (count < 1000000000000LL)
		output.Printf(wxT("%.2f%s"),(float)count/1000000000LL, _("G") );
	else if (count < 1000000000000000LL)
		output.Printf(wxT("%.2f%s"),(float)count/1000000000000LL, _("T") );

	return output;
}


// Make a time value in millieseconds suitable for displaying
wxString CastSecondsToHM(sint32 count)
{
	wxString buffer;
	
	if (count < 0)
		buffer = wxT("?");
	else if (count < 60)
		buffer.Printf( wxT("%02i %s"), count, _("secs") );
	else if (count < 3600)
		buffer.Printf( wxT("%i:%02i %s"), count/60, (count % 60), _("mins") );
	else if (count < 86400)
		buffer.Printf( wxT("%i:%02i %s"), count/3600, (count % 3600)/60, _("h") );
	else
		buffer.Printf( wxT("%i %s %02i %s"), count/86400, _("D") , (count % 86400) / 3600, _("h") );
		
	return buffer;
}


// Examins a filename and determines the filetype
FileType GetFiletype(const wxString& filename)
{
	if( filename.Find('.' ) == -1 )
		return ftAny;

	wxString ext = filename.AfterLast('.').MakeLower();

	if ( ext==wxT("avi")  || ext==wxT("mpg")  || ext==wxT("mpeg") || ext==wxT("ogm")  ||
		 ext==wxT("ram")  || ext==wxT("rm")   || ext==wxT("asf")  || ext==wxT("vob")  ||
		 ext==wxT("divx") || ext==wxT("vivo") || ext==wxT("mov")  || ext==wxT("wmv")  ||
		 ext==wxT("m2v")  || ext==wxT("swf")  || ext==wxT("qt")   || ext==wxT("mkv") )
		 return ftVideo;
		 
	if ( ext==wxT("cue")  || ext==wxT("bin")  || ext==wxT("iso")  || ext==wxT("ccd")  ||
		 ext==wxT("sub")  || ext==wxT("nrg")  || ext==wxT("img")  || ext==wxT("bwa")  ||
		 ext==wxT("bwi")  || ext==wxT("bws")  || ext==wxT("bwt")  || ext==wxT("mds")  ||
		 ext==wxT("mdf") )
	  return ftCDImage;
		
	if ( ext==wxT("mpc")  || ext==wxT("mp4")  || ext==wxT("aac")  || ext==wxT("ape")  ||
	     ext==wxT("mp3")  || ext==wxT("mp2")  || ext==wxT("wav")  || ext==wxT("au")   ||
		 ext==wxT("ogg")  || ext==wxT("wma")  || ext==wxT("rma")  || ext==wxT("mid") )
		 return ftAudio;

	if ( ext==wxT("jpg")  || ext==wxT("jpeg") || ext==wxT("bmp")  || ext==wxT("gif")  ||
	     ext==wxT("tiff") || ext==wxT("png")  || ext==wxT("rle")  || ext==wxT("psp")  ||
		 ext==wxT("tga")  || ext==wxT("wmf")  || ext==wxT("xpm")  || ext==wxT("pcx") )
		 return ftPicture;

	if ( ext==wxT("rar")  || ext==wxT("zip")  || ext==wxT("ace")  || ext==wxT("gz")   ||
	     ext==wxT("bz2")  || ext==wxT("tar")  || ext==wxT("arj")  || ext==wxT("lhz")  ||
		 ext==wxT("bz") )
		return ftArchive;

	if ( ext==wxT("exe")  || ext==wxT("com") )
		return ftProgram;

	if ( ext==wxT("txt")  || ext==wxT("html") || ext==wxT("htm")  || ext==wxT("doc")  ||
	     ext==wxT("pdf")  || ext==wxT("ps")   || ext==wxT("sxw")  || ext==wxT("log") )
		return ftText;

	return ftAny;
}


// Returns the (translated) description assosiated with a FileType
wxString GetFiletypeDesc(FileType type)
{
	switch ( type ) {
		case ftVideo:	return wxString( _("Videos") );
		case ftAudio:	return wxString( _("Audio") );
		case ftArchive:	return wxString( _("Archives") );
		case ftCDImage:	return wxString( _("CD-Images") );
		case ftPicture:	return wxString( _("Pictures") );
		case ftText:	return wxString( _("Texts") ); // ?
		case ftProgram:	return wxString( _("Programs") );
		default:		return wxString(_("Any"));
	}
}


// Returns the Typename, examining the extention of the given filename
wxString GetFiletypeByName(const wxString& filename)
{
	return GetFiletypeDesc( GetFiletype( filename ) );
}


wxString MakeStringEscaped(wxString in)
{
	in.Replace(wxT("&"),wxT("&&"));
	
	return in;
}


// Get the max number of connections that the OS supports, or -1 for default
int GetMaxConnections() {

#ifdef __WXMSW__

	int os = wxGetOsVersion();
		
	// Try to get the max connection value in the registry
	wxRegKey key( "HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Services\\VxD\\MSTCP\\MaxConnections" );
	
	wxString value;
	if ( key.Exists() )
		value = key.QueryDefaultValue();

	if ( !value.IsEmpty() && value.IsNumber() ) {
		long maxconn = -1;

		value.ToLong( &maxconn );

		return maxconn;		
	} else {
		switch ( os ) {
			case wxWIN95:		return 50;	// This includes all Win9x versions
			case wxWINDOWS_NT:	return 500;	// This includes NT based windows
			default:			return -1;	// Anything else. Let aMule decide...
		}
	}

#endif

	// Any other OS can just use the default number of connections
	return -1;
}


// Return the text assosiated with a rating of a file
wxString GetRateString(uint16 rate)
{
	switch ( rate ) {
		case 0: return _("Not rated");
		case 1: return _("Invalid / Corrupt / Fake");
		case 2: return _("Poor");
		case 3: return _("Good");
		case 4: return _("Fair");
		case 5: return _("Excellent");
		default: return _("Not rated");
	}
}


// Base16 chars for encode an decode functions
static byte base16Chars[17] = "0123456789ABCDEF";
#define BASE16_LOOKUP_MAX 23
static byte base16Lookup[BASE16_LOOKUP_MAX][2] = {
	{ '0', 0x0 },
	{ '1', 0x1 },
	{ '2', 0x2 },
	{ '3', 0x3 },
	{ '4', 0x4 },
	{ '5', 0x5 },
	{ '6', 0x6 },
	{ '7', 0x7 },
	{ '8', 0x8 },
	{ '9', 0x9 },
	{ ':', 0x9 },
	{ ';', 0x9 },
	{ '<', 0x9 },
	{ '=', 0x9 },
	{ '>', 0x9 },
	{ '?', 0x9 },
	{ '@', 0x9 },
	{ 'A', 0xA },
	{ 'B', 0xB },
	{ 'C', 0xC },
	{ 'D', 0xD },
	{ 'E', 0xE },
	{ 'F', 0xF }
};


// Make string safe for use in URLs
wxString URLEncode(wxString sIn)
{
	wxString sOut;
	unsigned char curChar;
	
	for ( unsigned int i = 0; i < sIn.Length(); i ++ ) {
		curChar = sIn.GetChar( i );

		if ( isalnum( curChar ) ) {
	        sOut += curChar;
	    } else if( isspace ( curChar ) ) {
		    sOut += wxT("+");
		} else {
			sOut += wxT("%");
			sOut += base16Chars[ curChar >> 4];
			sOut += base16Chars[ curChar & 0xf];
		}

	}

	return sOut;
}


// Returns a BASE16 encoded byte array
//
// [In]
//   buffer: Pointer to byte array
//   bufLen: Lenght of buffer array
//
// [Return]
//   wxString object with BASE16 encoded byte array
wxString EncodeBase16(const unsigned char* buffer, unsigned int bufLen)
{
	wxString Base16Buff;

	for(unsigned int i = 0; i < bufLen; i++) {
		Base16Buff += base16Chars[buffer[i] >> 4];
		Base16Buff += base16Chars[buffer[i] & 0xf];
	}

    return Base16Buff;
}


// Decodes a BASE16 string into a byte array
//
// [In]
//   base16Buffer: String containing BASE16
//   base16BufLen: Lenght BASE16 coded string's length
//
// [Out]
//   buffer: byte array containing decoded string
void DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, byte *buffer)
{
    memset( buffer, 0, base16BufLen / 2 );
	
	for(unsigned int i = 0; i < base16BufLen; i++) {
		int lookup = toupper(base16Buffer[i]) - '0';

        // Check to make sure that the given word falls inside a valid range
		byte word = 0;

		if ( lookup < 0 || lookup >= BASE16_LOOKUP_MAX)
           word = 0xFF;
        else
           word = base16Lookup[lookup][1];

		if(i % 2 == 0) {
			buffer[i/2] = word << 4;
		} else {
			buffer[(i-1)/2] |= word;
		}
	}
}


// Returns the text assosiated with a category type
wxString GetCatTitle(int catid)
{
	switch (catid) {
		case 0:	 return _("all");
		case 1:  return _("all others");
		case 2:  return _("Incomplete");
		case 3:  return _("Completed");
		case 4:  return _("Waiting");
		case 5:  return _("Downloading");
		case 6:  return _("Erroneous");
		case 7:  return _("Paused");
		case 8:  return _("Stopped");
		case 9:  return _("Video");
		case 10: return _("Audio");
		case 11: return _("Archive");
		case 12: return _("CD-Images");
		case 13: return _("Pictures");
		case 14: return _("Text");
		default: return _("?");
	}
}


bool CheckShowItemInGivenCat(CPartFile* file, int inCategory)
{
	// easy normal cases
	bool IsInCat;
	bool IsNotFiltered = true;

	IsInCat = ((inCategory==0) || (inCategory>0 && inCategory==file->GetCategory()));

	switch (theApp.glob_prefs->GetAllcatType()) {
		case 1:
			IsNotFiltered = ((file->GetCategory()==0) || (inCategory>0));
			break;
		case 2:
			IsNotFiltered = (file->IsPartFile());
			break;
		case 3:
			IsNotFiltered = (!file->IsPartFile());
			break;
		case 4:
			IsNotFiltered = ((file->GetStatus()==PS_READY|| file->GetStatus()==PS_EMPTY) && file->GetTransferingSrcCount()==0);
			break;
		case 5:
			IsNotFiltered = ((file->GetStatus()==PS_READY|| file->GetStatus()==PS_EMPTY) && file->GetTransferingSrcCount()>0);
			break;
		case 6:
			IsNotFiltered = ( file->GetStatus() == PS_ERROR );
			break;
		case 7:
			IsNotFiltered = ((file->GetStatus()==PS_PAUSED) && (!file->IsStopped()));
			break;
		case 8:
			IsNotFiltered = file->IsStopped();
			break;
		case 9:
			IsNotFiltered = GetFiletype(file->GetFileName()) == ftVideo;
			break;
		case 10:
			IsNotFiltered = GetFiletype(file->GetFileName()) == ftAudio;
			break;
		case 11:
			IsNotFiltered = GetFiletype(file->GetFileName()) == ftArchive;
			break;
		case 12:
			IsNotFiltered = GetFiletype(file->GetFileName()) == ftCDImage;
			break;
		case 13:
			IsNotFiltered = GetFiletype(file->GetFileName()) == ftPicture;
			break;
		case 14:
			IsNotFiltered = GetFiletype(file->GetFileName()) == ftText;
			break;
	}
	
	return (IsNotFiltered && IsInCat);
}


wxString MakeFoldername(wxString path) {
	/*
	if ( !path.IsEmpty() && ( path.Right(1) == wxT('/' )) ) {
		path.RemoveLast();
	}
	*/
	return path;
}


void HexDump(const void *buffer, unsigned long buflen)
{
	const uint8* cbuf = (const uint8*)buffer;

	for ( unsigned long ofs = 0; ofs < buflen; /* no increment here */ )
	{
		printf("%08lx", ofs);

		for ( unsigned long i = 0; (i<8) && (ofs<buflen); i++,ofs++ )
		{
			printf(" %02x", (int)cbuf[ofs]);
		}
		printf("\n");
	}
}


// Backup the file by copying to the same filename with appendix added
bool BackupFile(const wxString& filename, const wxString& appendix)
{

	if ( !FS_wxCopyFile(filename, filename + appendix) ) {
		printf("info: Could not create backup of '%s'\n",unicode2char(filename));
		return false;
	}
	
	// Kry - Safe Backup
	CFile safebackupfile;
	safebackupfile.Open(filename + appendix,CFile::read);
	safebackupfile.Flush();
	safebackupfile.Close();
	// Kry - Safe backup end
	
	return true;
}


// Kry - Added to get rid of fstab quiet flag on vfat
bool FS_wxCopyFile(const wxString& file1, const wxString& file2,bool overwrite)
{
	bool result;
	
	if ( theApp.use_chmod ) {
		result = wxCopyFile(file1,file2,overwrite);
	} else {
		result = wxCopyFile_fat32(file1,file2,overwrite);
	}
	
	return result;
}


/* A function to rename files that will avoid chmoding under linux on FAT 
   partitions, which would otherwise result in a lot of warnings. */
bool FS_wxRenameFile(const wxString& file1, const wxString& file2)
{
	bool result;
	
	if ( theApp.use_chmod ) {
		result = wxRenameFile(file1,file2);
	} else {
		result = wxRenameFile_fat32(file1,file2);
	}
	
	return result;
}


// Check if a IP address is usable
bool IsGoodIP(uint32 nIP)
{
	// always filter following IP's
	// -------------------------------------------
	//	 0.0.0.0
	// 127.*.*.*						localhost

	if (nIP==0 || (uint8)nIP==127)
		return false;

	if (!theApp.glob_prefs->FilterBadIPs())
		return true;

	// filter LAN IP's
	// -------------------------------------------
	//	0.*
	//	10.0.0.0 - 10.255.255.255		class A
	//	172.16.0.0 - 172.31.255.255		class B
	//	192.168.0.0 - 192.168.255.255	class C

	uint8 nFirst = (uint8)nIP;
	uint8 nSecond = (uint8)(nIP >> 8);

	if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
		return false;

	if (nFirst==172 && nSecond>=16 && nSecond<=31)
		return false;

	if (nFirst==0 || nFirst==10)
		return false;

	return true;
}


int wxCMPFUNC_CONV Uint16CompareValues(uint16* first, uint16* second) {
       return (((int)*first) - ((int)*second)) ;
}      

// DumpMem ... Dumps mem ;)
void DumpMem(const void* where, uint32 size) {
	for (uint32 i = 0; i< size; i++) {
		printf("|%2x",(uint8)((char*)where)[i]);
		if ((i % 16) == 15) {
			printf("\n");
		}			
	}	
	printf("\n");	
}

/*         BSD based OS support for gethostname_r       */

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)  || defined(__DARWIN__)
#include <sys/socket.h>
#include <netdb.h>

static int
convert (struct hostent *host, struct hostent *result,
       char *buf, int buflen, int *h_errnop)
{
  int len, i;

  if (!buf || !h_errnop) return -1;
  *h_errnop = h_errno;

  *result = *host;
  result->h_name = (char *) buf;
  /* This is the size. */
  len = strlen (host->h_name) + 1;
  if (len > buflen) return -1;
  buflen -= len;
  buf += len;
  strcpy ((char *) result->h_name, host->h_name);

  /* How many aliases and how big the buffer should be? There
     is always a NULL pointer. */
  for (len = sizeof (char *), i = 0; host->h_aliases [i]; i++)
  {
    /* It should be size of (char *) and the length of string
       plus 1. */
    len += strlen (host->h_aliases [i]) + 1 + sizeof (char *);
  }
  if (len > buflen) return -1;
  buflen -= len;

  /* This is an array of char * for h_aliases. */
#ifdef NEED_ALIGNED_ACCESS
  {
      int extra;
      extra = 4 - (((unsigned long) buf) & 3);
      if (extra != 4) {
         if (buflen < extra)
             return -1;
         buf = (char *) buf + extra;
      }
  }
#endif
  result->h_aliases = (char **) buf;
  buf += (i + 1) * sizeof (char *);

  /* We copy the aliases now. */
  for (i = 0; host->h_aliases [i]; i++)
  {
    result->h_aliases [i] = (char *) buf;
    strcpy (result->h_aliases [i], host->h_aliases [i]);
    buf += strlen (host->h_aliases [i]) + 1;
  }
  /* This is the last one */
  result->h_aliases [i] = NULL;

#if BSD >= 43 || defined(h_addr)
  for (len = sizeof (char *), i = 0; host->h_addr_list [i]; i++)
  {
    /* It should be size of (char *) and the length of string
       plus 1. */
    len += host->h_length + sizeof (char *);
  }
  if (len > buflen) return -1;

  /* This is an array of char * for h_addr_list. */
#ifdef NEED_ALIGNED_ACCESS
  {
      int extra;
      extra = 4 - (((unsigned long) buf) & 0x3);
      if (extra != 4) {
         if (buflen < extra)
             return -1;
         buf = ((char *) buf) + extra;
      }
  }
#endif
  result->h_addr_list = (char **) buf;
  buf += (i + 1) * sizeof (char *);

  /* We copy the h_addr_list now. */
  for (i = 0; host->h_addr_list [i]; i++)
  {
    result->h_addr_list [i] = (char *) buf;
    memcpy (result->h_addr_list [i], host->h_addr_list [i], host->h_length);
    buf += host->h_length;
  }
  /* This is the last one */
  result->h_addr_list [i] = NULL;
#else
  len = strlen (host->h_addr) + 1 + sizeof (char *);
  if (len > buflen) return -1;

  result->h_addr = (char *) buf;
  strcpy (result->h_addr, host->h_addr);
#endif
  return 0;
}


struct hostent *gethostbyname_r (const char *name, struct hostent *result, char *buffer, int buflen, int *h_errnop)
{
  struct hostent *host;

  s_mutexProtectingGetHostByName.Lock();

  host = gethostbyname (name);
  if (!host ||
       convert (host, result, buffer, buflen, h_errnop) != 0)
  {
    result = NULL;
  }

  s_mutexProtectingGetHostByName.Unlock();
  return result;
}

#endif /* __FreeBSD__ */
