// ===========================================================================
//	NetPositiveApp.h
// 	©1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __NETPOSTIVEAPP__
#define __NETPOSTIVEAPP__

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#define DEBUG 0
#include <Debug.h>
#include <UTF8.h>

#include "MList.h"
#include "StLocker.h"

#include "URESOURC.H"

// ===========================================================================
// Messages

#define OPEN_LOCATION		'opnl'
#define OPEN_FILE			'opnf'
#define OPEN_HTML			'ophm'
#define OPEN_TEXT			'optx'
#define OPEN_NEW			'onew'
#define SAVE				'save'
#define SAVE_LINK			'slnk'
#define BOOKMARKS			'book'
#define ADD_BOOKMARK		'adbk'
#define OPEN_BOOKMARK		'opbk'
#define DO_PRINT			'dpnt'

#define CLOSE				'clos'
#define CLEAR				'cler'

#define DO_ADKILLER			'adki'
#define DO_MESSAGES			'mess'
#define DO_RESOURCES		'resr'
#define DO_MOVIE_SPEED		'mspd'

#define DO_DUMPOBJECTS		'dmpo'
#define DO_DUMPGLYPHS		'dmpg'
#define DO_DOWNLOAD			'down'
#define DO_FLUSHCACHE		'flsh'
#define DO_TABLEDRAWINFO	'dinf'
#define DO_TABLEDRAWCELL	'dcll'
#define DO_USELATIN5		'dul5'

#define FIND				'find'
#define FIND_AGAIN			'fndg'

#define WINDOW_CREATED		'wcre'
#define WINDOW_DELETED		'wdel'
#define WINDOW_ACTIVATED	'wact'

const uint32	msg_WritePrefs	= 'Wprf';
const uint32	msg_ReloadAll	= 'RAll';
	

// ===========================================================================

void SimpleAlert(const char* str);
bool OkCancelAlert(const char* str);
void PositionWindowRect(BRect *r);
void ByteSizeStr(int size, CString& sizeStr);

// ===========================================================================

class NPWindow;
class BFilePanel;

class NetPositive : public BApplication {

public:
						NetPositive();
	virtual				~NetPositive();
	
	virtual	void		ReadyToRun();
			
	virtual void		ArgvReceived(int32 argc, char **argv);			
	virtual	void		MessageReceived(BMessage *msg);
	virtual	void		AboutRequested();
	virtual	bool		QuitRequested();
	
	virtual	void		FilePanelClosed(BMessage *a_message);
			bool		RefToURL(entry_ref& ref, CString& url);
	virtual	void		RefsReceived(BMessage *msg);
	
	virtual	void		AppActivated(bool toActive);
			
	virtual	void		WindowCreated(NPWindow *window);
	virtual	void		WindowDeleted(NPWindow *window);
	virtual	void		WindowActivated(NPWindow *window);
			
			NPWindow*	NewWindowFromResource(UResource *r, bool openAsText = false, uint32 encoding = B_ISO1_CONVERSION);
			NPWindow*	NewWindowFromURL(CString& url, uint32 encoding = B_ISO1_CONVERSION);
			NPWindow*	NewWindowFromError(char *heading, char *info, uint32 encoding = B_ISO1_CONVERSION);

	void				ReadPrefs();
	void				WritePrefs();
	FILE*				GetPrefsFile(const char *mode);

	void				ReloadAllWindows();

static	void			GetDefaultURL(CString &url);
static	void			SetDefaultURL(CString &url);

protected:
			MList<NPWindow *>	mWindows;
			
			BFilePanel*	mFilePanel;
			bool		mLaunchRefs;

static	CString			sDefaultURL;
};

#endif
