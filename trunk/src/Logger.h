#ifndef LOGGER_H
#define LOGGER_H

#include <wx/string.h>
#include <wx/intl.h>

enum DebugType 
{
	//! General warnings/errors.
	logGeneral = 0,
	//! Warnings/Errors for the main hashing thread.
	logHasher,
	//! Warnings/Errors for client-objects.
	logClient,
	//! Warnings/Errors for the local client protocol.
	logLocalClient,
	//! Warnings/Errors for the remote client protocol.
	logRemoteClient,
	//! Warnings/Errors when parsing packets.
	logPacketErrors,
	//! Warnings/Errors for the CFile class.
	logCFile,
	//! Warnings/Errors related to reading/writing files.
	logFileIO,
	//! Warnings/Errors when using the zLib library.
	logZLib,
	//! Warnings/Errors for the AICH-syncronization thread.
	logAICHThread,
	//! Warnings/Errors for transfering AICH hash-sets.
	logAICHTransfer,
	//! Warnings/Errors when recovering with AICH.
	logAICHRecovery,
	//! Warnings/Errors for the CListenSocket class.
	logListenSocket,
	//! Warnings/Errors for Client-Credits
	logCredits,
	//! Warnings/Errors for the client UDP socket.
	logClientUDP,
	//! Warnings/Errors for the download-queue.
	logDownloadQueue,
	//! Warnings/Errors for the IP-Filter.
	logIPFilter,
	//! Warnings/Errors for known-files.
	logKnownFiles,
	//! Warnings/Errors for part-files.	
	logPartFile,
	//! Warnings/Errors for SHA-hashset creation.
	logSHAHashSet,
	//! Warnings/Errors for servers, server connections
	logServer,
};



/**
 * Container-class for the debugging categories.
 */
class CDebugCategory
{
public:
	/**
	 * Constructor.
	 * 
	 * @param type The actual debug-category type.
	 * @param name The user-readable name.
	 */
	CDebugCategory( DebugType type, const wxString& name );

	
	/**
	 * Returns true if the category is enabled.
	 */
	bool IsEnabled() const;

	/**
	 * Enables/Disables the category.
	 */
	void SetEnabled( bool );
	

	/**
	 * Returns the user-readable name.
	 */
	const wxString& GetName() const;
	
	/**
	 * Returns the actual type.
	 */
	DebugType GetType() const;

private:
	//! The user-readable name.
	wxString	m_name;
	//! The actual type.
	DebugType	m_type;
	//! Whenever or not the category is enabled.
	bool		m_enabled;
};


/**
 * Namespace containing functions for logging operations.
 */
namespace CLogger
{
	/**
	 * Returns true if debug-messages should be generated for a specific category.
	 */
	bool 	IsEnabled( DebugType );
	
	/**
	 * Enables or disables debug-messages for a specific category.
	 */
	void		SetEnabled( DebugType type, bool enabled );

	
	/**
	 * Logs the specified line of text.
	 *
	 * @param critical If true, then the message will be made visible directly to the user.
	 * @param str The actual line of text.
	 *
	 * This function is thread-safe. If it is called by the main thread, the 
	 * event will be sent directly to the application, otherwise it will be
	 * queued in the event-loop.
	 */
	void		AddLogLine( bool critical, const wxString str );
	
	/**
	 * Logs the specified line of text as debug-info.
	 *
	 * @param critical If true, then the message will be made visible directly to the user.
	 * @param type The debug-category, the name of which will be prepended to the line.
	 * @param str The actual line of text.
	 *
	 * This function is thread-safe. If it is called by the main thread, the 
	 * event will be sent directly to the application, otherwise it will be
	 * queued in the event-loop.
	 */
	void		AddDebugLogLine( bool critical, DebugType type, const wxString& str );

	
	/**
	 * Returns a category specified by index.
	 */
	const CDebugCategory&	GetDebugCategory( int index );

	/**
	 * Returns the number of debug-categories.
	 */
	unsigned int 			GetDebugCategoryCount();
};



//
// Temporarary workaround. Avoids the issue of having to fix everything at once.
//
#if 0
#define AddDebugLogLineM( critical, type, string ) \
if ( critical || CLogger::IsEnabled( type ) ) { \
	CLogger::AddDebugLogLine( critical, type, string ); \
}
#else
inline void AddDebugLogLineM( bool critical, DebugType type, const wxString& str ) 
{
	CLogger::AddDebugLogLine( critical, type, str );
}

inline void AddDebugLogLineM( bool critical, const wxString& str ) 
{
	CLogger::AddDebugLogLine( critical, logGeneral, str );
}
#endif

#define AddLogLineM( critical, string ) \
	CLogger::AddLogLine( critical, string )


#endif
