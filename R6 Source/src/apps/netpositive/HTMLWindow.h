// ===========================================================================
//	HTMLWindow.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef HTML_WINDOW_H
#define HTML_WINDOW_H

#include "Strings.h"
#include "NPApp.h"

#include <Window.h>
#include <UTF8.h>
#include <List.h>
#include <String.h>

class FrameList;
class Document;
class PageHistory;
class HTMLView;
class URLView;
class UResourceImp;


// ============================================================================
//	HTMLWindow has a url edit field at the top and 

#define NP_HTML_WIDTH (8 + 640 + 8)
#define NP_HTML_HEIGHT 450

#ifdef __GNUC__
_EXPORT class NPBaseView;
#else
class _EXPORT NPBaseView;
#endif

class LinkHistory {
public:
				LinkHistory();
				~LinkHistory();
		void	AddToHistory(const BString &URL, const char *targetName, FrameList *frames = NULL, Document *doc = NULL);
PageHistory*	GetCurrentHistory() {return mHistory;}
		void	LoadPageFromHistory(PageHistory *p);
		bool	CanGoForward();
		bool	CanGoBack();
		void	RemoveCurrentPage();
				
		void	GoForward();
		void	GoBack();
		
		void	Rewind();
		
		void	SetBaseView(NPBaseView *baseView) {mBaseView = baseView;}
		int32	CountItems();
		PageHistory* ItemAt(int32 index);
private:
		PageHistory*	mHistory;
		NPBaseView*		mBaseView;
};


class NPBaseView : public BView {
public:
					NPBaseView(BRect frame, const char *name, UResourceImp *resource, bool openAsText, uint32 encoding, bool showToolbar, bool showProgress, bool resizable, int showTitleInToolbar, BMessage *originatingMessage);
					NPBaseView(BMessage *data);
virtual				~NPBaseView();
virtual status_t	Archive(BMessage *data, bool deep = true) const;
static BArchivable	*Instantiate(BMessage *data);
		HTMLView *	GetTopHTMLView() const;
		LinkHistory* GetLinkHistory() {return &mLinkHistory;}
		void		Init(UResourceImp *resource, bool openAsText, uint32 encoding, bool showToolbar, bool showProgress, bool resizable, int showTitleInToolbar, BMessage *originatingMessage);
		HTMLView*	GetHTMLView() {return mHTMLView;}
		URLView*	GetURLView() {return mURLView;}

virtual void		MessageReceived(BMessage *message);
virtual void		FrameResized(float width, float height);
virtual void		AttachedToWindow();
		void		NewHTML(const char *url, const char *target, bool forceCache = false, const char *formData = NULL, int showTitleInToolbar = kShowTitleUnspecified, const char *referrer = NULL, BMessage *originatingMessage = NULL);	
		void		NewHTML(const char *url, int32 num, bool forceCache = false, const char *referrer = NULL, BMessage *originatingMessage = NULL);
		
		void		SetBookmarkMenu(BMenu *menu) {mBMMenu = menu;}
		BMenu *		GetBookmarkMenu() {return mBMMenu;}
private:
		LinkHistory	mLinkHistory;
		BMenu		*mBMMenu;
		HTMLView*		mHTMLView;
		URLView*		mURLView;
		BString			mReplicantURL;
};


class HTMLWindow : public BWindow {

public:
				HTMLWindow(BRect frame, UResourceImp *resource, bool openAsText = false, uint32 encoding = N_USE_DEFAULT_CONVERSION, bool fullScreen = false, bool showToolbar = true, bool showProgress = true, bool resizable = true, int showTitleInToolbar = kShowTitleUnspecified, BMessage *originatingMessage = NULL); 
virtual			~HTMLWindow();
virtual	bool	QuitRequested();
virtual	void	MessageReceived(BMessage *msg);
virtual void	MenusBeginning();
virtual	void	WindowActivated(bool state);
		void	EnableBackForw(bool back, bool forw);
		void	EnableLoadImages(bool enable);
		bool	IsFullScreen() {return mFullScreen;}
static	HTMLWindow*	FrontmostHTMLWindow();
HTMLView*		MainHTMLView() {return mHTMLView;}


void			SendMessageToHTMLViews(uint32 message, BView *top);

protected:
		void	Init(UResourceImp *resource, uint32 encoding, bool showToolbar, bool showProgress, bool resizable, int showTitleInToolbar, BMessage *originatingMessage);
		void	InitTextOnly(UResourceImp *resource, uint32 encoding);
		
		BMenuBar*		mMenuBar;
		BMenu*			mFileMenu;
		BMenu*			mEditMenu;
		BMenu*			mGoMenu;
		BMenu*			mBookmarksMenu;
		BMenu*			mViewMenu;
#ifdef DEBUGMENU
		BMenu*			mDebugMenu;
#endif		
		BMenuItem*		mTableDrawInfo;
		BMenuItem*		mTableDrawCell;
		
		NPBaseView*		mBaseView;
		HTMLView*		mHTMLView;
		URLView*		mURLView;
		
		LinkHistory*	mLinkHistory;
		
		BRect			mOldWindowRect;
		bool			mFullScreen;
		bool			mSaveWindowPos;
static  BList			sWindowList;
};

#endif
