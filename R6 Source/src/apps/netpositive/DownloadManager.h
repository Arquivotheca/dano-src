// ===========================================================================
//	DownloadManager.h
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef DOWNLOADMANAGER
#define DOWNLOADMANAGER

class UResourceImp;
class BWindow;
class ConnectionManager;

class DownloadManager {
friend class DownloadWindow;

public:
static void			DownloadFile(const char *url, bool openWhenDone, bool forceFilePanel = false);
static void			DownloadFile(UResourceImp *resource, ConnectionManager *srcMgr, bool openWhenDone, bool forceFilePanel = false);
static void			ShowDownloadWindow(bool force=false);
static BWindow*		GetDownloadWindow();
static bool			RequestQuit();

protected:
static void			BeginFileDownload(UResourceImp *resource, const char *url, const char *downloadPath, bool openWhenDone, bool wasMoved = false);
static status_t		GetDefaultDownloadPath(BString *result, const char *url);
static void			ShowFilePanel(const char *suggestedName, BMessage& openMessage);
static void 		GetSuggestedName(BString *result, const char *url);
};

#endif
