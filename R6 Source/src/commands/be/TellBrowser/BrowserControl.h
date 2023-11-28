#ifndef _BROWSER_CONTROL_H
#define	_BROWSER_CONTROL_H

#include <app/Application.h>
#include <support/String.h>
#include <app/MessageQueue.h>
#include <driver/kb_mouse_driver.h>
#include <interface/CursorManager.h>

class BrowserWindow;
class CrashWindow;

enum bc_mode {
	BC_NORMAL,
	BC_UPGRADE,
	BC_BROWSER_DOWN,
	BC_QUIT
};

class BrowserControl : public BApplication
{
	public:
								BrowserControl();
								~BrowserControl();
								
		bool					QuitRequested();
		void					ReadyToRun();
		void					ArgvReceived(int32 argc, char **argv);
		void					MessageReceived(BMessage *msg);
	
	private:
		bool					fLaunchBrowser;
		bool					fWatchBrowser;
		status_t				StartBrowser();
		
		volatile thread_id		fBrowser;
		port_id					fWatchPort;
		port_id					fKernelPort;
		volatile thread_id		fMonitor;
		static int32			StartMonitoring(void *tellbrowser);
		int32					BrowserRelaunchLoop();

		void					Reboot();
		bool					fCrashed;
		bool					fInitRcvd;
		volatile bc_mode		fMode;
		volatile bigtime_t		fLastCrashTime;
						
		void					GetMessenger();
		BMessenger				fMsgr;
		
		BMessageQueue			fQ;
		
		void					TellBrowser(BMessage *msg);
		void					SendSimpleMessage(uint32 what);
		void					GoTo(const char *url, const char * frame = NULL);
		void					GoToTop(const char *url);
		void					Rewind();
		void					Launch();
		void					Upgrade(BMessage *msg);
		void					ShowAlert(const char *url,int32 height = 0);
		void					PrintHelp();
		void					SetEnv(BMessage *msg);

		void					OpenStatus(const char *url);
		void					UpdateStatus(const char *url);
		void					CloseStatus();
		void					InitCursors();
		void					SetCrashCursor(bool crash);
		void					SetBusyCursor(bool busy);
		
		
		BrowserWindow *			fStatusWin;
		CrashWindow *			fCrashWin;
		BString					fURL;
		char *					fBrowserSig;
		BString					fBrowserPath;
		int32					fHistory;
		bool					fQuitWindows;
		mouse_settings			fOriginalMouse;
		mouse_settings			fCurrentMouse;
		BCursorManager::cursor_token	fCrashCursorToken;
		BCursorManager::cursor_token	fBusyCursorToken;
		BCursorManager::queue_token		fCrashQueueToken;
		BCursorManager::queue_token		fBusyQueueToken;
};

/* in MediaControl.cpp */
void SetVolume(BMessage * msg);
void GetVolume(BMessage * msg);

/* in Settings.cpp */
void SetSettings(BMessage *msg);
void GetSettings(BMessage *msg, BMessage *reply);

#endif
