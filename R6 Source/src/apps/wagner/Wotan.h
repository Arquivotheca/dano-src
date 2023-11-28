/* Wotan.h */

#ifndef _WOTAN_H
#define _WOTAN_H

#include <Application.h>
#include <CursorManager.h>
#include <Locker.h>
#include "BrowserWindow.h"
#include "Timer.h"
#include "URL.h"
#include "kb_mouse_driver.h"

namespace Wagner {
class ContentInstance;
}

using namespace Wagner;

class BrowserWindow;
class PPPListener;

class Wotan : public BApplication {
public:
							Wotan();

	virtual	void			ReadyToRun();
	virtual	void			MessageReceived(BMessage *msg);
	virtual	void			ArgvReceived(int32 argc, char **argv);
	virtual	void			RefsReceived(BMessage *msg);
	virtual bool			QuitRequested();
	
			status_t		GoTo(ContentInstance *instance);
			void 			GotoContent(const URL &url, const char *target=NULL, int32 groupID=-1);
	atom<ContentInstance>	GetTopContentInstance();
	ContentView	*			GetContentView();

			void			SetPPPCursor(bool connecting);
	BLocker					fCursorLock;
			bool			IsSet_NoCache(){ return fNoCache; };

private:
	// This gets called when nothing is currently being downloaded.
	static void 			Idle();
	static void 			Busy();
	static void				NotifyTellBrowser(const BMessage*);
	static void				UpdateThrobber(const BMessage*);
			bool 			RefToURL(entry_ref &ref, URL &url);



			int32 			ConsoleThread();
	static	int32 			_ConsoleThread(void *);
			void 			LaunchConsole();
			
			void			InitMouseSettings();
			void			WriteMouseSettings();
			void			InitCursor();
			
			void			InitPrinting();

			void			GoTo(const char *url, int32 window = 0);
			void 			GoTo(const URL &url);
			void			SetEnv(BMessage *msg);

			void			SetBusyCursor(bool busy);

	atom<Timer>				fTimer;
	atom<PPPListener> 		fPPPListener;
	BrowserWindow *			fMainWindow;
	uint32					fBrowserWindows;
	thread_id				fConsoleThid;
	mouse_settings          fMouseSettings;
	BCursorManager::cursor_token	fBusyCursorToken;
	BCursorManager::queue_token		fBusyQueueToken;
	BCursorManager::queue_token		fPPPQueueToken;	
	bool					fRandom;
	bool					fFull;
	bool					fNotifiedTellBrowser;
	bool					fQuitWindows;
	bool					fIsRelaunched;
	bool					fIsRunning;
	bool					fNoCache;
};

#endif
