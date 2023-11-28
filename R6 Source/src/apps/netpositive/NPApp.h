// ===========================================================================
//	NPApp.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __NETPOSTIVEAPP__
#define __NETPOSTIVEAPP__

#ifndef _APPLICATION_H
#include <Application.h>
#endif
class BEntry;

#include <UTF8.h>
#include <String.h>
#include <Window.h>
#include "Translation.h"
#include "AnchorGlyph.h"

enum {
	N_AUTOJ_CONVERSION	= 256,
	N_NO_CONVERSION = 258,
	N_USE_DEFAULT_CONVERSION = 259
};


// ===========================================================================
// Messages

#define HTML_MSG		'html'
#define FORM_MSG		'form'
#define ENTR_MSG		'entr'
#define INFO_MSG		'info'

const uint32	NP_BACKBUTTON			= 'gobk';
const uint32	NP_FORWBUTTON			= 'gofw';
const uint32	NP_STOPBUTTON			= 'stop';
const uint32	NP_RELOADBUTTON			= 'rlod';
const uint32	NP_HOMEBUTTON			= 'home';
const uint32	NP_SEARCHBUTTON			= 'srch';

const uint32	OPEN_LOCATION			= 'opnl';
const uint32	OPEN_FILE				= 'opnf';
const uint32	OPEN_HTML				= 'ophm';
const uint32	OPEN_TEXT				= 'optx';
const uint32	OPEN_NEW				= 'onew';
const uint32	SAVE					= 'save';
const uint32	SAVE_LINK				= 'slnk';
const uint32	BOOKMARKS				= 'book';
const uint32	ADD_BOOKMARK			= 'adbk';
const uint32	OPEN_BOOKMARK			= 'opbk';
const uint32	OPEN_HIST_BOOKMARK		= 'ohbm';
const uint32	DO_PRINT				= 'dpnt';
const uint32	DO_QUIT					= 'dqit';
const uint32	DO_PAGE_SETUP			= 'dpsu';

const uint32	CLOSE					= 'clos';
const uint32	CLEAR					= 'cler';

const uint32	DO_ADKILLER				= 'adki';
const uint32	DO_MESSAGES				= 'mess';
const uint32	DO_RESOURCES			= 'resr';
const uint32	DO_RANDOM_LINK			= 'rndl';
const uint32	DO_RANDOM_SPAM			= 'rnds';
const uint32	DO_GRAZE_LINKS			= 'graz';

const uint32	DO_DUMPOBJECTS			= 'dmpo';
const uint32	DO_DUMPGLYPHS			= 'dmpg';
const uint32	DO_DOWNLOAD				= 'down';
const uint32	DO_FLUSHCACHE			= 'flsh';
const uint32	DO_TABLEDRAWINFO		= 'dinf';
const uint32	DO_TABLEDRAWCELL		= 'dcll';
const uint32	DO_USELATIN5			= 'dul5';
const uint32	LOAD_IMAGES				= 'limg';
const uint32	LOAD_DOPE				= 'ldpe';
const uint32	RELOAD_PAGE				= 'rldp';

const uint32	FIND					= 'find';
const uint32	FIND_AGAIN				= 'fndg';
const uint32	FULL_SCREEN				= 'fscr';
const uint32	SHOW_DOWNLOADS			= 'sdwn';
const uint32	BOOKMARK_MENU			= 'book';

const uint32	HTML_OPEN_LINK			= 'hopn';
const uint32	HTML_OPEN_LINK_IN_NEW	= 'holn';
const uint32	HTML_OPEN_IMG			= 'hopi';
const uint32	HTML_OPEN_IMG_IN_NEW	= 'hoin';
const uint32	HTML_ADD_BOOKMARK		= 'habm';
const uint32	HTML_SAVE_LINK			= 'hsav';
const uint32	HTML_COPY_LINK			= 'hcpy';
const uint32	HTML_RELOAD_FRAME		= 'rfrm';
const uint32	HTML_FILTER_IMAGE_ALL_SITES	= 'hfas';
const uint32	HTML_FILTER_IMAGE_THIS_SITE	= 'hfts';
const uint32	HTML_FILTER_FRAME_THIS_SITE	= 'hffs';
const uint32	HTML_SHOW_FILTERS		= 'hsfl';
const uint32	DOWNLOADS_STOPPED		= 'dlso';
const uint32	DOWNLOADS_STARTED		= 'dlsa'; 

const uint32 msg_ISO1			= 'ISO1';	// this really means Windows CP1252
const uint32 msg_ISO2			= 'ISO2';
const uint32 msg_KOI8R			= 'KOI8';
const uint32 msg_ISO5			= 'ISO5';
const uint32 msg_ISO7			= 'ISO7';
const uint32 msg_MacRoman		= 'MacR';
const uint32 msg_AutodetectJ	= 'AutJ';
const uint32 msg_SJIS			= 'SJIS';
const uint32 msg_EUC			= 'EUC!';
const uint32 msg_Unicode		= 'UNIC';
const uint32 msg_UTF8			= 'UTF8';
const uint32 msg_MSDOS866		= 'D866';
const uint32 msg_WINDOWS1251	= '1251';
const uint32 msg_Prefs			= 'Prf!';	
const uint32 msg_RelayoutView	= 'RlVw';
const uint32 msg_ShowBookmarks = 'SBkm';
//const uint32 msg_FindUpdatedBookmarks = 'Fupb';

const uint32	msg_WritePrefs	= 'Wprf';
const uint32	msg_ReloadAll	= 'RAll';
	
const uint32	msg_WesternEncoding		= 'WEnc';
const uint32	msg_JapaneseEncoding	= 'JEnc';
const uint32	msg_CEEncoding			= 'EEnc';
const uint32	msg_CyrillicEncoding	= 'CEnc';
const uint32	msg_GreekEncoding		= 'GEnc';
const uint32	msg_UnicodeEncoding		= 'UEnc';
const uint32	msg_ToggleProxy			= 'TPrx';
const uint32	msg_ProFamilySelected	= 'PFSe';
const uint32	msg_ProSizeSelected		= 'PSSe';
const uint32	msg_FixFamilySelected	= 'FFSe';
const uint32	msg_FixSizeSelected		= 'FSSe';
const uint32	msg_CacheEveryTime		= 'CEtm';
const uint32	msg_CacheOncePerSession	= 'COps';
const uint32	msg_CacheOncePerDay		= 'COpd';
const uint32	msg_CacheNever			= 'CNev';
const uint32	msg_ClearCacheNow		= 'CClr';
const uint32	msg_MinFixSizeSelected	= 'MFSs';
const uint32	msg_MinProSizeSelected	= 'MPSs';
const uint32	msg_ShowFilters			= 'SAfl';
const uint32	msg_NewWindowClone		= 'NWcl';
const uint32	msg_NewWindowHome		= 'NWhm';
const uint32	msg_NewWindowBlank		= 'NWbl';
const uint32	msg_WindowClosing		= 'Wcls';
const uint32	msg_FormSubmitNever		= 'FSnv';
const uint32	msg_FormSubmitOneLine	= 'FSol';
const uint32	msg_FormSubmitAlways	= 'FSal';
const uint32	msg_CookieAccept		= 'Cacc';
const uint32	msg_CookieReject		= 'Crjt';
const uint32	msg_CookieAsk			= 'Cask';
const uint32	msg_AddThread			= 'Athr';
const uint32	msg_RemoveThread		= 'Rthr';
const uint32	msg_ZoomIn				= 'Zmin';
const uint32	msg_ZoomOut				= 'Zmou';
const uint32	msg_ListViewChanged		= 'Lvch';
const uint32 	msg_DeleteDownloadItem 	= 'Ddni';
const uint32 	msg_StopDownloadItem 	= 'Sdni';
const uint32 	msg_RetryDownloadItem 	= 'Rdni';
const uint32 	msg_BeginDownload 		= 'Bdnl';
const uint32 	msg_NewBrowserWindow 	= 'Nbwd';
const uint32 	msg_PauseAll 			= 'Pall';
const uint32 	msg_StopAll 			= 'Sall';
const uint32 	msg_SetSimultaneous 	= 'Ssim';
const uint32	msg_UnselectAllButOne	= 'Uabo';
const uint32	msg_DragAllSelected		= 'Dasl';
const uint32	msg_LaunchAllSelected	= 'Lasl';
const uint32	msg_DeleteAllSelected	= 'Dras';
const uint32	msg_DeleteAll			= 'DLda';
const uint32	msg_RetryAllStopped		= 'Drst';
const uint32 	msg_AutoLaunch			= 'Atla';

// ===========================================================================

void SimpleAlert(const char* str);
void PositionWindowRect(BRect *r);
void CenterWindowRect(BRect *r);
void ByteSizeStr(int size, BString& sizeStr);
void CopyFile(BEntry *srcEntry, BEntry *targetEntry);
void *GetNamedResource(const char *name, size_t& size);
void FFMSetMouseOverWindow(BWindow *window, BPoint offset);
status_t			GetIcon(BBitmap *icon);
	
const char*			GetShortVersionString();
const char*			GetLongVersionString();
const char*			GetVersionNumber();
			
status_t			GetIcon(BBitmap *icon);

extern bool gEZDebug;
extern bool gRunningAsReplicant;
extern BMessage* gPrintSettings;
extern const unsigned char gLinkCursor[];
#define kNumCursorPhases 11
extern const unsigned char gBusyCursor[kNumCursorPhases][68];
extern const unsigned char gBusyLinkCursor[kNumCursorPhases][68];
extern BTranslatorRoster *gTranslatorRoster;

// ===========================================================================

class BFilePanel;
class UResourceImp;

extern BMessage gPreferences;

class NetPositive : public BApplication {

public:
						NetPositive();
	virtual				~NetPositive();
	virtual bool		QuitRequested();
		
	virtual	void		ReadyToRun();
			
	virtual void		ArgvReceived(int32 argc, char **argv);			
	virtual	void		MessageReceived(BMessage *msg);
	virtual	void		AboutRequested();
	
	virtual	void		FilePanelClosed(BMessage *a_message);
			bool		RefToURL(entry_ref& ref, BString& url);
	virtual	void		RefsReceived(BMessage *msg);
		void			SetCursor(const void *cursor);	// Isn't defined as a virtual in BApplication,
														// but we'll call it directly.
	
			
			BWindow*	NewWindow();
static		BWindow*	NewWindowFromResource(UResourceImp *r, bool openAsText = false, uint32 encoding = N_USE_DEFAULT_CONVERSION, bool showWindow = true, bool fullScreen = false, bool showToolbar = true, bool showProgress = true, BRect *rect = NULL, bool resizable = true, int showTitleInToolbar = kShowTitleUnspecified, BMessage *originatingMessage = NULL);
static		BWindow*	NewWindowFromURL(const BString& url, uint32 encoding = N_USE_DEFAULT_CONVERSION, const char *formData = NULL, bool fullScreen = false, bool showToolbar = true, bool showProgress = true, BRect *rect = NULL, bool resizable = true, int showTitleInToolbar = kShowTitleUnspecified, const char *postData = NULL, BMessage *originatingMessage = NULL);
			BWindow*	NewWindowFromError(char *heading, char *info, uint32 encoding = N_USE_DEFAULT_CONVERSION);


	void				ReloadAllWindows();
	bool				HasDocumentWindow();
	void				KillThreads();
static void				AddThread(thread_id tid);
static void				RemoveThread(thread_id tid);
	
static void				ReadPrefs();
static void				WritePrefs();
static void				InitPrefs();
static void				Init();
static void				Cleanup();
static void				GetPrefsFile(char *pathname);
static void				InitMidi();
static void				StartSong(entry_ref *ref, bool loop);
static void				StopSong();
static void				HandleCopyTarget(BMessage *msg);
static void				DragLink(const char *url, const BRect rect, BView *view, BBitmap *dragImage, BPoint offset);
static color_space		MainScreenColorSpace();
static bool				IsQuitting();
static void				SetQuitting(bool mQuit);
				
protected:
			static bool mIsQuitting;
//			BList		mWindows;
			
			BFilePanel*	mFilePanel;
			bool		mLaunchRefs;
			BList		mThreads;
};

class FFMWarpingWindow : public BWindow {
public:
	FFMWarpingWindow(BRect frame,const char *title, window_type type,uint32 flags,uint32 workspace = B_CURRENT_WORKSPACE) :
		BWindow(frame, title, type, flags, workspace) {}
	FFMWarpingWindow(BRect frame,const char *title, window_look look,window_feel feel,uint32 flags,	uint32 workspace = B_CURRENT_WORKSPACE) :
		BWindow(frame, title, look, feel, flags, workspace) {}
	virtual void	Activate(bool on = true);
	virtual void	Show();
};

#endif
