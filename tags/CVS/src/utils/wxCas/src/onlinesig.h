//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         OnLineSig Class
///
/// Purpose:      Monitor aMule Online Statistics by reading amulesig.dat file
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
///
/// Pixmats from aMule http://www.amule.org
///
/// This program is free software; you can redistribute it and/or modify
///  it under the terms of the GNU General Public License as published by
/// the Free Software Foundation; either version 2 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the
/// Free Software Foundation, Inc.,
/// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _ONLINESIG_H
#define _ONLINESIG_H

#ifdef __GNUG__
#pragma interface "onlinesig.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/filename.h>

/// amulesig.dat file monitoring
class OnLineSig
  {
  private:
    double m_maxDL;

    int  m_amuleState;
    wxString m_serverName;
    wxString m_serverIP;
    wxString m_serverPort;
    wxString m_connexionID;
    wxString m_ULRate;
    wxString m_DLRate;
    wxString m_queue;
    wxString m_sharedFiles;
    wxString m_user;
    wxString m_totalUL;
    wxString m_totalDL;
    wxString m_version;
    wxString m_sessionUL;
    wxString m_sessionDL;
    unsigned int m_runTimeS;

    wxFileName m_amulesig;

    wxString BytesConvertion (const wxString& bytes);
    unsigned int PullCount (unsigned int *runtime, const unsigned int count);


  public:
    /// Default constructor
    OnLineSig ();

    /// Constructor
    OnLineSig (const wxFileName& file);

    /// Destructor
    ~OnLineSig ();

    /// Set amulesig.dat file name and path
    void SetAmuleSig (const wxFileName& file);

    /// Refresh stored informations
    void Refresh ();

    /// Return TRUE if aMule is running
    int GetAmuleState () const;

    /// Get server name
    wxString GetServerName () const;

    /// Get server IP
    wxString GetServerIP () const;

    /// Get server Port
    wxString GetServerPort () const;

    /// Get server connexion ID: H or L
    wxString GetConnexionID () const;

    /// Get Upload rate
    wxString GetULRate () const;

    /// Get Download rate
    wxString GetDLRate () const;

    /// Get number of clients in queue
    wxString GetQueue () const;

    /// Get number of shared files
    wxString GetSharedFiles () const;

    /// Get user name
    wxString GetUser () const;

    /// Get total Upload
    wxString GetTotalUL () const;

    /// Get total Download
    wxString GetTotalDL () const;

    /// Get aMule version
    wxString GetVersion () const;

    /// Get session Upload
    wxString GetSessionUL () const;

    /// Get session Download
    wxString GetSessionDL () const;

    /// Get aMule runtime
    wxString GetRunTime ();

    /// Get total Upload in the best representative unit
    wxString GetConvertedTotalUL ();

    /// Get total Download in the best representative unit
    wxString GetConvertedTotalDL ();

    /// Get session Upload in the best representative unit
    wxString GetConvertedSessionUL ();

    /// Get session Download in the best representative unit
    wxString GetConvertedSessionDL ();

    /// Get server connexion ID: LowID or HighID
    wxString GetConnexionIDType () const;

    /// Get max Download rate sine wxCas is running
    wxString GetMaxDL () const;
  };

#endif /* _ONLINESIG_H */
