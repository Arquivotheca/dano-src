// ===========================================================================
//	HTMLView.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef HTML_VIEW_H
#define HTML_VIEW_H

#include "HTMLDoc.h"
#include "Protocols.h"
#include "NPApp.h"
#include "AnchorGlyph.h"
#ifdef JAVASCRIPT
#include "jseopt.h"
#include "jselib.h"
#endif

#include <View.h>
#include <UTF8.h>
#include <String.h>

//=======================================================================
//	Maintains the history of browsing...

class PageHistory : public CLinkable {
public:
					PageHistory(const BString &url, const BString &target, FrameList *frames = NULL, Document *doc = NULL);
	virtual			~PageHistory();

	PageHistory*	GetForw();
	PageHistory*	GetBack();
	PageHistory*	AddPage(const BString &url, const BString &target, FrameList *frames = NULL, Document *doc = NULL, bool isForm = false);

	void			SetURL(const char *url) 		{mURL = url;}
	void			SetTarget(const char *target) 	{mTarget = target;}
	void			SetFrameList(FrameList *fl) 	{mFrames = fl;}
	void			SetTitle(const char *title) 	{mTitle = title;}

	const char*		URL() 							{return mURL.String();}
	const char*		Target() 						{return mTarget.String();}
	FrameList*		GetFrameList() 					{return mFrames;}
	ExtraPageData*	GetExtra() 						{return &mExtra;}
	const char *	GetTitle() 						{return mTitle.String();}

private:
	BString			mURL;
	BString			mTarget;
	FrameList		*mFrames;
	ExtraPageData	mExtra;
	BString			mTitle;
};

class PluginData {
public:
	PluginData();
	~PluginData();
	
	BString			mParameters;
	BMessage		*mData;
};

// ===========================================================================
//	Main HTML View

class LinkHistory;
class HTMLWorker;

enum {
	disallowScrollBars = 0,
	optionalVScroller,
	mandatoryVScroller
};

class HTMLView : public BView {
friend class HTMLWorker;

public:
						HTMLView(BRect frame, const char *name, UResourceImp *resource, LinkHistory *linkHistory,
								 bool openAsText = false, uint32 encoding = N_USE_DEFAULT_CONVERSION,
								 bool isSubview = false, int resizeOption = optionalVScroller,
								 Document *parentDoc = NULL, int showTitleInToolbar = kToolbarUnspecified,
								 BMessage *originatingMessage = NULL);
virtual					~HTMLView();

virtual	void			AttachedToWindow();
virtual	void			DetachedFromWindow();
virtual	void			Draw(BRect updateRect);
virtual void			ScrollTo(BPoint where);
		void			ScrollTo(float x, float y);
virtual	void			MessageReceived(BMessage *msg);
virtual void			KeyDown(const char *bytes, int32 numBytes);
virtual	void			MouseDown(BPoint point);
virtual	void			MouseMoved(	BPoint where,
									uint32 code,
									const BMessage *a_message);

virtual void			FrameResized(float width, float height);

		void			Print();

		void 			ResetScrollBars();
		void			UpdateScrollBars();
		void			CreateScrollBar(orientation direction);
		void			DestroyScrollBar(orientation direction);
		void			SetScrolling(int32 scroll);		
	
		void			UpdateDocRect(BRect& r);	// Update after a draw or an idle
		void			Stop();						// Stop loading images
		void			Reset(bool purgeCache = true);	// Reset window for reload
		void			Reload();

		void			QualifyNewDoc();
		void			NewHTMLDoc(UResourceImp *resource, bool openAsText, bool forceCache = false);
		void			NewHTMLDocFromURL(const BString& url, bool isForm = false, 
										  BString* post = NULL, bool forceCache = false, 
										  bool addToHistory = true, const char *referrer = NULL,
										  BMessage *originatingMessage = NULL);
		bool			HandleFragment(const BString& url);
		void			UpdateURLView();
		
		void			SetURLText(BString& url, bool select = false);
		void			SetProgress(StoreStatus status, long done, long of);
		void			SetProgressStr(const char *str, bool permanent = true);
		void			ClearProgressStr();
		void			RestoreProgressStr();

		bool			FindString(const char *findThis);
		void			MakeVisible(float h, float v);
		
		void			OpenLink(const BString& url, const char *target, const char *formData = NULL, const char *referrer = NULL, int showTitleInToolbar = kShowTitleUnspecified, BMessage *originatingMessage = NULL);
		void			CopyLink(const BString& url);
		void			RewindHistory();

		void			RelayoutView(bool reparse = true);
		void			RealRelayoutView(float width = -1.0, bool invalidate = true);
		void			RelayoutFrames(FrameList *frames, BRect bounds);
		void			SetPrintingMode(bool on);

		void			SetSubviewFont(Style inStyle, BView *view);

		void			SetupFrames(FrameList *frames, BRect bounds, int32 num, bool forceCache = false);
		void			SetupFrameRects(const FrameList *frames, BRect maxBounds, BList *viewBounds);
		void			SetupFrameRowsOrCols(const FrameList *frames, float maxBound, BList *viewBounds, bool isColumns);
		void			SetExtra(ExtraPageData *extra);
		void			KillChildren();
		FrameList*		GetFrameList()					{return mFrames;}
		int32			GetFrameDepth()					{return mFrameDepth;}
		void			SetFrameDepth(int32 depth)		{mFrameDepth = depth;}
		CLinkedList*	GetUserValues();
		BList*			GetPluginData();
#ifdef JAVASCRIPT
		void			CreateBlankDocument();
#endif

		void			SetFrameNum(int32 num);
		int32			FrameNum();
		
		void			DoWork(bool updateView);
		BMessenger*		GetMessenger()					{return &mMessenger;}
		void			CloneInNewWindow();
		const char *	GetDocumentURL()				{return mDocumentURL.String();}
		uint32			GetEncoding()					{return mEncoding;}
		bool			GetOpenAsText()					{return mOpenAsText;}
		const char *	GetProgressStr();
		const char *	GetDefaultProgressStr();
		void			SetDefaultProgressStr(const char *str);
		URLParser*		GetURLParser() 					{return &mURLParser;}
		int32			GetHistoryLength();
		const char*		GetHistoryItem(int32 index);
		const char*		GetReferrer()					{return mReferrer.String();}
		long			GetLastModified()				{return mLastModified;}
		bool			IsSubview()						{return mIsSubview;}
		void			GrazeLinks(BList* urlList);
		void			ChangeCursor();
		virtual void	Pulse();
#ifdef PLUGINS
static 	int32			CreateStreamIOThread(void *args);

#endif
#ifdef JAVASCRIPT
		bool			AllowPopupWindow();
		void			SetOpener(struct BrowserWindow *bw) {mOpener = bw;}
struct BrowserWindow*	GetOpener()						{return mOpener;}
struct BrowserLocation*	GetLocation()					{return &mLocation;}
struct BrowserWindow*	GetBrowserWindow()				{return mBrowserWindow;}
		BList			sBrowserWindowList;
#endif
		status_t		GetStatus() 					{return mProgStatus;}
		Document*		GetDocument()					{return mHTMLDoc;}
static void				Init();
static void				PageSetup();
static void				Cleanup();
static BBitmap *		RequestOffscreenBitmap(float width, float height);
static void				OffscreenFinished(BBitmap *offscreenView);
static bool				OffscreenEnabled() 				{return sOffscreenEnabled;}

protected:
		void			DeleteDoc(bool killWorkerSynchronously = true);
		
		uint32			mEncoding;

		UResourceImp*		mNewDocResource;	// Resource Waiting for our favors
		
		char			mFragment[256];		// #anchor
		BString			mTitle;
		BString			mPreviousTitle;
		
		Document*		mHTMLDoc;
		BString			mDocumentURL;
		FrameList*		mFrames;
		int32			mFrameDepth;
		ExtraPageData*	mExtra;
		
		BString			mImageURL;			// When we create a fake doc for an image, remember the image

		StoreStatus		mProgStatus;
		long			mProgDone;
		long			mProgOf;

		BPoint			mScrollPos;
		int32			mScrolling;
		int32			mFrameNum;
		
		LinkHistory	*	mLinkHistory;
		BRect			mOldFrame;
		URLParser		mURLParser;
				
//		Save Panel temps

		BFilePanel*		mSavePanel;
		
		ConnectionManager *mConnectionMgr;
		HTMLWorker	*	mWorkerThread;
		
		BMessenger		mMessenger;
		BMessage		mPrevProgMsg;
		float			mMarginWidth;
		float			mMarginHeight;
		
		UResourceImp*		mBGSound;
		BString			mReferrer;
		BString			mPendingReferrer;
		long			mLastModified;
		int				mScrollOption;
		Document*		mParentDoc;
		bigtime_t		mLastBlink;
		BMessage*		mOriginatingMessage;
#ifdef JAVASCRIPT
struct BrowserWindow*	mOpener;
struct BrowserLocation	mLocation;
struct BrowserWindow *	mBrowserWindow;
#endif
		
		unsigned		mPendingPOSTRequest : 1;
		unsigned		mDocIsPOSTRequest : 1;
		unsigned		mMouseIsOverLink : 1;
		unsigned		mSecure : 1;
		unsigned		mFirstUpdate : 1;
		unsigned		mIgnoreUpdate : 1;
		unsigned		mAddToHistory : 1;
		unsigned		mIsSubview : 1;
		unsigned		mIsForm : 1;
		unsigned		mOpenAsText : 1;
		unsigned		mNeedsToOpenURL : 1;
		unsigned		mStopEnabled : 1;
		unsigned		mForcedCacheLoad : 1;
		unsigned		mCurrentCursor : 2;
		unsigned		mShowTitleInToolbar : 2;
		unsigned		mCursorPhase : 4;
#ifdef JAVASCRIPT
		unsigned		mExecutingJSURL : 1;
#endif
		bigtime_t		mLastPulse;
		
static bool				sOffscreenEnabled;
static BList 			sFreeOffscreenViewList;
static BList 			sUsedOffscreenViewList;
static int32 			sOffscreenViewListSize;
static TLocker 			sOffscreenListLocker;
};

inline void	HTMLView::ScrollTo(float x, float y)
	{ ScrollTo(BPoint(x, y)); }

#ifdef JAVASCRIPT
struct BrowserWindow {
	HTMLView *			view;
	Document *			doc;
struct BrowserTimeout *	mTimeout;
	bool				open;
};
#endif


#endif
