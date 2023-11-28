#ifndef _BROWSER_WINDOW_H
#define _BROWSER_WINDOW_H

#include <Window.h>
#include <www/URL.h>
#include <www/ContentView.h>
#include <www/History.h>

using namespace Wagner;

enum browser_style {
	BS_NORMAL,
	BS_FULL_SCREEN,
	BS_ALERT
};

enum browser_window_flags {
	BS_JS_CLOSABLE	= 0x00000001
};

enum broswer_cmd {
	bcHistoryBack = 'back',
	bcHistoryForward = 'forw'
};

const uint32 BW_CLOSED = 'bwCl';

class BrowserWindow : public BWindow
{
	public:
							BrowserWindow(BRect rect, browser_style s, bool quit = true, uint32 flags=0);
							~BrowserWindow();
							
virtual	void				DispatchMessage(BMessage *message, BHandler *handler);
virtual	void				MessageReceived(BMessage *msg);
virtual BView*				NextNavigableView(BView* currentFocus, uint32 flags);
		
		browser_style		BrowserStyle() const;					

		status_t			SetContent(ContentInstance *instance);
		ContentInstance *	GetContentInstance();
		ContentView *		GetContentView();
		status_t			OpenURL(const Wagner::URL & url, uint32 flags, GroupID requestorsGroupID);
		status_t			OpenURL(const Wagner::URL & url, const char *target, GroupID requestorsGroupID);

		struct abuse_t {
			BrowserWindow *window;
			const char *url_file;
			const char *save_urls;
			int loop_urls;
			int num_windows;
		};

		void				StartAbuse(abuse_t*);
		
		void				Reload();
		void				StopLoading();
		void				GoForward();
		void				GoBackward();
		void				Rewind();
		void				Print();
		virtual bool 		QuitRequested();		

		bool				Closable();
		void				NotifyContent(BMessage *msg);

	private:
		browser_style		fBrowserStyle;
		ContentView *		fContentView;
		uint32				fFlags;
		int32				fCurrentAtomMark;
		
		static int32		Abuse(void *arg);
		bool                HasControlViews(void);
};

#endif
