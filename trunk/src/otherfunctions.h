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


#ifndef OTHERFUNCTIONS_H
#define OTHERFUNCTIONS_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString
#include <wx/strconv.h>
#include "types.h"		// Needed for uint16, uint32 and uint64
#include "filefn.h"		// Needed for wxCopyFile_fat32
#include "endianfix.h"

class CPartFile;

static wxCSConv aMuleConv(wxT("iso8859-1"));

#define unicode2char(x) (const char*) aMuleConv.cWX2MB(x)
#define char2unicode(x) aMuleConv.cMB2WX(x)

/*
#define unicode2char(x) ConvertUnicode2Char(x)
#define char2unicode(x) ConvertChar2Unicode(x)

wxString ConvertChar2Unicode(const char* c_string);
const char* ConvertUnicode2Char(wxString unic_string);
*/

// From Gnucleus project [found by Tarod]
// Converts 'buffer' with length 'bufLen' to a wxString
wxString EncodeBase16(const unsigned char* buffer, unsigned int bufLen);
// Converts the string 'base16Buffer' with length 'base16BufLen' to a hash in 'buffer'.
void DecodeBase16(const char *base16Buffer, unsigned int base16BufLen, unsigned char *buffer);


// Converts the number of bytes to human readable form.
wxString CastItoXBytes(uint64 count);
// Converts the number to human readable form, abbreviating when nessecary.
wxString CastItoIShort(uint64 number);
// Converts an ammount of seconds to human readable time.
wxString CastSecondsToHM(sint32 seconds);
// Returns the string assosiated with a file-rating value.
wxString GetRateString(uint16 rate);


// The following functions are used to identify and/or name the type of a file
enum FileType { ftAny, ftVideo, ftAudio, ftArchive, ftCDImage, ftPicture, ftText, ftProgram };
// Examins a filename and returns the enumerated value assosiated with it, or ftAny if unknown extension
FileType GetFiletype(const wxString& filename);
// Returns the description of a filetype: Movies, Audio, Pictures and so on...
wxString GetFiletypeDesc(FileType type);
// Shorthand for GetFiletypeDesc(GetFiletype(filename))
wxString GetFiletypeByName(const wxString& filename);


// Returns the max number of connections the current OS can handle.
// Currently anything but windows will return the default value (-1);
int GetMaxConnections();
// Returns the name assosiated with a category value.
wxString GetCatTitle(int catid);
// Checks an ip to see if it is valid, depending on current preferences.
inline bool IsGoodIP(uint32 nIP)
{
	// always filter following IP's
	// -------------------------------------------
	//	 0.0.0.0
	// 127.*.*.*						localhost

	if (nIP==0 || (uint8)nIP==127)
		return false;

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

// Tests if a ID is low (behind firewall/router/...)
#define HIGHEST_LOWID_HYBRID	16777216
#define HIGHEST_LOWID_ED2K		16777216
inline bool IsLowIDHybrid(uint32 id){
	return (id < HIGHEST_LOWID_HYBRID);
}
inline bool IsLowIDED2K(uint32 id){
	return (id < HIGHEST_LOWID_ED2K); //Need to verify what the highest LowID can be returned by the server.
}


// Makes sIn suitable for inclusion in an URL, by escaping all chars that could cause trouble.
wxString URLEncode(wxString sIn);
// Replaces "&" with "&&" in 'in' for use with text-labels
inline wxString MakeStringEscaped(wxString in) {
	in.Replace(wxT("&"),wxT("&&"));
	return in;
}
inline wxString MakeFoldername(wxString path) {
	/*
	if ( !path.IsEmpty() && ( path.Right(1) == wxT('/' )) ) {
		path.RemoveLast();
	}
	*/
	return path;
}

// Makes a backup of a file, by copying the original file to filename + appendix
bool BackupFile(const wxString& filename, const wxString& appendix);
// This function is a replacement for wxCopyFile, with the added feature,
// that chmoding of the target file can be disabled. The reason for this
// is, that FAT partitons under linux generate warnings when chmoding.
bool FS_wxCopyFile(const wxString& file1, const wxString& file2,bool overwrite = TRUE);
// Same as above, but renames rather than copies.
bool FS_wxRenameFile(const wxString& file1, const wxString& file2);


/* Other */
void HexDump(const void *buffer, unsigned long buflen);
bool CheckShowItemInGivenCat(CPartFile* file,int inCategory);
// Compares first and second. For uint16 arrays sorting.
int wxCMPFUNC_CONV Uint16CompareValues(uint16* first, uint16* second);


#define ARRSIZE(x) (int) (sizeof(x)/sizeof(x[0]))
#define itemsof(x) (sizeof(x)/sizeof(x[0]))
#define ELEMENT_COUNT(X) (sizeof(X) / sizeof(X[0]))


enum EED2KFileType {
	ED2KFT_ANY,
	ED2KFT_AUDIO,
	ED2KFT_VIDEO,
	ED2KFT_IMAGE,
	ED2KFT_PROGRAM,
	ED2KFT_DOCUMENT,
	ED2KFT_ARCHIVE,
	ED2KFT_CDIMAGE
};


const uint8 PMT_UNKNOWN=0;
const uint8 PMT_DEFAULTOLD=1;
const uint8 PMT_SPLITTED=2;
const uint8 PMT_NEWOLD=3;


// Duplicates a string
inline char* nstrdup(const char* src)
{
	int len = (src ? strlen(src) : 0) + 1;
	char *res = new char[len];
	if ( src ) strcpy(res, src);
	res[len-1] = 0;
	return res;
}


// Replacements for atoi and atol that removes the need for converting
// a string to normal chars with unicode2char. The value returned is the
// value represented in the string or 0 if the conversion failed.
inline long StrToLong( const wxString& str ) {
	long value = 0;
	str.ToLong( &value );
	return value;
}
inline unsigned long StrToULong( const wxString& str ) {
	unsigned long value = 0;
	str.ToULong( &value );
	return value;
}

// md4cmp -- replacement for memcmp(hash1,hash2,16)
// Like 'memcmp' this function returns 0, if hash1==hash2, and !0, if hash1!=hash2.
// NOTE: Do *NOT* use that function for determining if hash1<hash2 or hash1>hash2.
inline int md4cmp(const void* hash1, const void* hash2) {
	return !(((uint32*)hash1)[0] == ((uint32*)hash2)[0] &&
		     ((uint32*)hash1)[1] == ((uint32*)hash2)[1] &&
		     ((uint32*)hash1)[2] == ((uint32*)hash2)[2] &&
		     ((uint32*)hash1)[3] == ((uint32*)hash2)[3]);
}


// md4clr -- replacement for memset(hash,0,16)
inline void md4clr(const void* hash) {
	((uint32*)hash)[0] = ((uint32*)hash)[1] = ((uint32*)hash)[2] = ((uint32*)hash)[3] = 0;
}


// md4cpy -- replacement for memcpy(dst,src,16)
inline void md4cpy(const void* dst, const void* src) {
	((uint32*)dst)[0] = ((uint32*)src)[0];
	((uint32*)dst)[1] = ((uint32*)src)[1];
	((uint32*)dst)[2] = ((uint32*)src)[2];
	((uint32*)dst)[3] = ((uint32*)src)[3];
}

// DumpMem ... Dumps mem ;)
void DumpMem(const void* where, uint32 size);

#ifdef __FreeBSD__
inline long long atoll(char const* s) {
  return (long long) strtoll(s, (char **)NULL, 10);
}
#endif /* __FreeBSD__ */

/*         BSD based OS support for gethostname_r       */
#if 	defined(__FreeBSD__) || defined(__OpenBSD__) \
	|| defined(__NetBSD__)  || defined(__DARWIN__)

#include <wx/thread.h>		// Needed for wxMutex

static wxMutex s_mutexProtectingGetHostByName;

struct hostent* gethostbyname_r(
	char const* name,
	struct hostent* result,
	char* buffer,
	int buflen,
	int* h_errnop
);

#endif /* BSD supoprt */


#endif // OTHERFUNCTIONS_H
