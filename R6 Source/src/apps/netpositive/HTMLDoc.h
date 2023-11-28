//  ===========================================================================
//	HTMLDoc.h
//  Copyright 1998 by Be Incorporated.
//  Copyright 1995 by Peter Barrett, All rights reserved.
//  ===========================================================================

#ifndef __HTMLDOC__
#define __HTMLDOC__

#include "PageGlyph.h"
#include "URL.h"
#include "Store.h"
#ifdef JAVASCRIPT
#include "jseopt.h"
#include "seall.h"
#endif

#include <Point.h>
#include <Locker.h>
#include <String.h>
#include <List.h>


class Form;
class Selection;
class ImageGlyph;
class ImageMap;
class Consumer;
class TextGlyph;
class ConnectionManager;
class UResourceImp;
struct BrowserWindow;
class ObjectGlyph;

enum {
	kNoScrolling,
	kYesScrolling,
	kAutoScrolling,
	kScrollingNotSet
};

enum {
	kShowToolbar,
	kHideToolbar,
	kToolbarUnspecified
};

class HTMLView;

#ifdef JAVASCRIPT
class AnchorGlyph;

typedef struct BrowserLocation {
	BString location;
	HTMLView *view;
	AnchorGlyph *glyph;
};

class ContextWrapper : public Counted {
public:
	ContextWrapper(jseContext context) : mLocker("JavaScript context lock") {mContext = context;}
	
	jseContext	mContext;
	TLocker		mLocker;
	jseVariable	mOriginalGlobal;
private:
	~ContextWrapper();
};
#endif

class ExtraPageData {
public:
	ExtraPageData();
	~ExtraPageData();	
	
	void				SetScrollPos(BPoint pos) {mScrollPos = pos;}
	BPoint				ScrollPos() {return mScrollPos;}
	
	void				SetUserValues(CLinkedList *list) {if (mUserValues) delete mUserValues; mUserValues = list;}
	CLinkedList			*UserValues() {return mUserValues;}
	
	void				SetPluginData(BList *list);
	BList*				GetPluginData() {return mPluginData;}

protected:
	BPoint				mScrollPos;
	CLinkedList			*mUserValues;
	BList				*mPluginData;
};

class FrameItem;

class FrameList : public CLinkedList, public Counted {
public:
						FrameList(const BString& URL) {mURL = URL;}
	const char *		URL() {return mURL.String();}
	
	bool				operator==(const FrameList &fl) const;
	FrameItem *			FindFrame(const char *name) const;
protected:
	BString				mURL;
// Since this is reference counted and can't be destroyed directly,
// make the destructor private.
//private:
	virtual				~FrameList() {}
};

class FrameItem : public CLinkable {
public:
						FrameItem(FrameItem *jumpBackLeaf);
	virtual				~FrameItem();

	void				SetColValue(int32 value, bool isPercentage = false);
	int32				ColValue(bool *isPercentage);

	void				SetRowValue(int32 value, bool isPercentage = false);
	int32				RowValue(bool *isPercentage);

	void				SetColPosition(int32 value);
	int32				ColPosition();

	void				SetRowPosition(int32 value);
	int32				RowPosition();
		
	void				SetScrolling(int32 scroll);
	int32				Scrolling();

	void				SetMarginWidth(int32 width);
	int32				MarginWidth();

	void				SetMarginHeight(int32 height);
	int32				MarginHeight();

	void				SetName(const char *name);
	const char			*Name();

	void				SetURL(const char *url);
	const char			*URL();
	
	void				SetView(HTMLView *view);
	HTMLView*			View();

	void				SetBorder(bool border);
	bool				Border();
	void				SetBorderValue(int32 border);
	int32				BorderValue();
	
	void				SetFrames(FrameList *frames);
	FrameList			*Frames();
	ExtraPageData*		GetExtra() {return &mExtra;}

	FrameItem			*JumpBackLeaf();

	bool				operator==(const FrameItem &fi) const;

private:
	void				Reset();

private:
	FrameItem			*mJumpBackLeaf;
	int32				mColValue;
	bool				mColIsPercentage;
	int32				mColPosition;
	int32				mRowValue;
	bool				mRowIsPercentage;
	int32				mRowPosition;
	int32				mScrolling;
	int32				mMarginWidth;
	int32				mMarginHeight;
	char				*mName;
	char				*mURL;
	FrameList			*mFrames;
	ExtraPageData		mExtra;
	HTMLView			*mView;
	bool				mBorder;
	int32				mBorderValue;
};

//=============================================================================
// Handles progressive drawing and background

class DocumentGlyph : public PageGlyph {
public:
					DocumentGlyph(Document* htmlDoc, bool isLayer);
			virtual	~DocumentGlyph();

			void	SetClipRect(BRect *r);
			bool	GetDirty(BRect *r);
			void	Invalidate(UpdateRegion& updateRegion);
			
			void 	SetBGImage(ImageGlyph* BGImage);
			void	DrawBackground(DrawPort *drawPort);
			bool	HasBGImage();
			ImageGlyph*	GetBGImage() {return mBGImage;}
			void	SetBGColor(long color) {mBGColor = color;}
			long	GetBGColor() {return mBGColor;}
			void	SetVisibility(bool visibility);
			bool	IsVisible() {return mIsVisible;}

	virtual	void	Draw(DrawPort *drawPort);
	virtual	void	Layout(DrawPort *drawPort);
	virtual	void	ResetLayout();
	
	virtual bool	IsDocument();

			float	Pagenate(float width, float height, float marginWidth);

			void	SetDrawBackground(bool draw);
			
			void	InflateBGImage(Pixels *srcPixels, long *w, long *h);
			
			void	SetNeedsRelayout();

protected:
	virtual	float		LayoutLineV(float vPos, float space, short align, Glyph *firstGlyph, Glyph *lastGlyph);

		BRect		mClipRect;
		BRect		mDirty;
		ImageGlyph*	mBGImage;
		float		mPagenateHeight;
		bool		mDrawBackground;
		Pixels *	mBGPixels;
		bool		mWeOwnBGPixels;
		long		mBGColor;
		bool		mNeedsRelayout;
		float		mOldVPos;
		long		mBGImageWidth;
		long		mBGImageHeight;
		bool		mIsLayer;
		bool		mIsVisible;
};


class LayerItem {
public:
	~LayerItem();

	DocumentGlyph*	mDocumentGlyph;
	Document*		mDocument;
	
	int32			mLeft;
	int32			mTop;
	int32			mWidth;
	int32			mHeight;
	int32			mZIndex;
	int32			mVisibility;
	BString			mName;
	BString			mSRC;
	BString			mAbove;
	BString			mBelow;
	BString			mClip;
	long			mBGColor;
	BString			mBackground;
};

// ============================================================================
// Document is the container for a built HTML document

//	User input returns messages ....

enum HTMLMessage {
	HM_NOTHING = 0,
	HM_ONTOANCHOR,		// Mouse moved onto an anchor
	HM_OVERANCHOR,		// Mouse is currently over an anchor
	HM_OFFANCHOR,		// Mouse moved off an anchor
	HM_TRACKSELECTION,	// Dragging a selection
	HM_SELECTION,		// Made a selection
	HM_TRACKANCHOR,		// Tracking an anchor
	HM_ANCHOR,			// Selected an anchor
	HM_TRACKIMAGE,		// Tracking an image
	HM_TRACKIMG_OR_ANC,	// Tracking both an image and anchor.  You decide what to do.
	HM_TRACKFORM,		// Tracking a click in a form
	HM_SUBMITGET,		// Submit form by GET
	HM_SUBMITPOST,		// Submit form by POST
	HM_TRACKCONTROL,	// Tracking in a control
	HM_CONTROL,			// Selected a control
	
	HM_UPDATE,			// Idle detected drawing was required
	HM_IDLEAGAIN,		// Idle wants to be called again soon.
	HM_UPDATEANDIDLEAGAIN	// Idle wants to be called again soon.
};

class Document : public Counted {
friend class DocumentBuilder;
public:
							Document();
			
#ifdef DEBUGMENU
			void			Print();
#endif
			
#ifdef JAVASCRIPT
			static	Document*	CreateFromResource(ConnectionManager *mgr, UResourceImp *r, DrawPort *drawPort, long docRef, bool openAsText, 
												   uint32 encoding, bool forceCache, CLinkedList *list, BList *pluginData, BMessenger *workerMessenger,
													const char *subframeName = NULL, int32 frameNumber = 0, Document *parent = NULL, struct BrowserWindow *bw = NULL);
#else
			static	Document*	CreateFromResource(ConnectionManager *mgr, UResourceImp *r, DrawPort *drawPort, long docRef, bool openAsText, 
												   uint32 encoding, bool forceCache, CLinkedList *list, BList *pluginData, BMessenger *workerMessenger, const char *subframeName = NULL, int32 frameNumber = 0, Document *parent = NULL);
#endif
              
			void			SetForceCache(bool forceCache);
			const char*		GetTitle();
			DrawPort		*GetDrawPort();
			float			GetHeight();
			float			GetUsedWidth();
			bool			IsParsingComplete();
			bool			IsIndex();
			
			void			SetResource(UResourceImp* r);
			UResourceImp*		GetResource();
			ConnectionManager *GetConnectionManager() {return mConnectionMgr;}
			
			void			SetBase(const char* base);
			URLParser*		GetBase() {return &mBase;}
			void			SetRefreshTime(int seconds);
			int				GetRefreshTime() {return mRefreshTime;}
			void			SetRefreshURL(const char* url);
			bool			GetRefreshURL(BString& url);
			const char *	GetStoredRefreshURL() {return mRefreshURL.String();}
			
			void			SetMarginWidth(float width);
			void			SetMarginHeight(float height);

			void			ResolveURL(BString& URL,const char* partialURL);				// Relsolve URL path

			void			ShowImages(int showImages);	// 1 = yes, 0 = no, -1 = no ads
			void			ShowBGImages(int showBGImages);	// 1 = yes, 0 = no, -1 = no ads

			void			Layout(float width);
			float			Pagenate(float width, float height);
			void			DrawBackground(BRect *r);
			void			Draw(BRect *r, bool back = true);
			HTMLMessage 	Idle(BRect *r);
			HTMLMessage 	Stop(BRect *r);
			void			SpacingChanged();
			
			AnchorGlyph*	GetAnchor(BString& url, BString& target, int& showTitleInToolbar);
			void			FormReset(InputGlyph *g);
			BString* 		FormSubmission(InputGlyph *g,BString& url);
			BString* 		FormSubmission(Form *f, InputGlyph *g, BString& url);
			const char*		FormTargetFrame(InputGlyph *g);
			
			
			void			SelectText(long start,long length);
			bool			GetSelectedText(long *start,long *length);
			long			GetTextLength();
			char*			GetTextPool();
			char*			GetFormattedSelection(long *length);

			float		GetCurrentPosition(BRect *r);
			float			GetPositionTop(float position);
			float			GetSelectionTop();
			float			GetSelectionLeft();
			float			GetFragmentTop(char *fragment);
			
			const char*		GetBGSound() {return mBGSoundURL.String();}
			bool			GetBGSoundLoops() {return mBGSoundLoops;}
			
			long			GetALinkColor() {return mALinkColor;}
			long			GetLinkColor() {return mLinkColor;}
			long			GetVLinkColor() {return mVLinkColor;}
			long			GetBGColor() {return mBGColor;}
			long			GetFGColor() {return mFGColor;}
			void			SetALinkColor(long color);
			void			SetLinkColor(long color);
			void			SetVLinkColor(long color);
			void			SetBGColor(long color);
			void			SetFGColor(long color);
			
			HTMLMessage		MouseDown(float h, float v, bool isRightClick);
			HTMLMessage		MouseMove(float h, float v);
			HTMLMessage		MouseUp(float h, float v);
			bool			MouseURL(BString& url, bool ignoreAnchor = false, BRect *anchorBounds = 0);
			
			HTMLMessage		Key(short key);
			bool			IsTarget();
			void			Untarget();

			CLinkedList		*UserValues();

													//rowDims and colDims should be undefined by NULL
			void			OpenFrameset(const char *rowDims, const char *colDims, int32 border = 0);
			void			CloseFrameset();
			void			OpenLayer(DocumentGlyph *glyph, int32 left, int32 top, int32 width, int32 height, int32 zIndex,
									  int32 visibility, const char *id, const char *src,
									  const char *above, const char *below, const char *clip,
									  long bgColor, const char *background);
			void			CloseLayer();
			void			AddFrame(int32 scroll, int32 marginWidth, int32 marginHeight, 
									 const char *name, const char *url, bool border);
			void			RemoveFrame();
			bool			GetFrameset(FrameList **frames);
			int32			GetFrameBorder();
						
			bool			Lock();
			bool			LockWindow();
			bool			LockDocAndWindow();
			void			Unlock();
			void			UnlockWindow();
			void			UnlockDocAndWindow();
			bool			IsLocked();
			thread_id		LockingThread() {return mLocker.LockingThread();}
			Consumer*		GetParser() {return mParser;}
			void			KillImageResources();
			void			LoadImages();
			bool			ReadyForDisplay() {return mReadyForDisplay;}
			UResourceImp*	GetResourceSynchronously(const char *path, StoreStatus *status);
			void			GrazeLinks(BList *urlList);
#ifdef JAVASCRIPT
//			jseContext		GetJSEContext() {return mJSEContext;}
//			void			SetJSEContext(jseContext context, BrowserWindow *bw);
			BrowserWindow*	GetBrowserWindow() {return mBrowserWindow;}
			void			ExecuteScript(const char *script);
			void			ExecuteScriptFile(const char *path);
			jseVariable		AddHandler(jseVariable what, const char *handlerName, const char *script);
			void			ExecuteHandler(jseVariable what, jseVariable func);
			void			UpdateObject(jseVariable what);
			void			DeleteVariable(jseVariable what);
			void	SetTimeoutThread(struct BrowserTimeout *tm);
			void			KillJSEContext();
			void			SetDocumentVar(jseVariable docVar) {mDocumentVar = docVar;}
			void			SetupGeneralInfo(bool spoofARealBrowser, jseContext jsecontext = NULL);
			void			InitJavaScript();
			void			EnableJavaScript(bool on, void *jseLinkData) {mJavaScriptEnabled = on; mJSELinkData = jseLinkData;}
			void			Reopen();
			Document*		GetParent() {return mParent;}
			void			UpdateLayer(LayerItem *item);
#endif
			ImageGlyph*		GetNextImage(ImageGlyph* previousImage);
			Form*			GetNextForm(Form* previousForm);
			AnchorGlyph*	GetNextAnchor(AnchorGlyph* previousAnchor);
			void			LoadImage(ImageGlyph *image);
			bool			IsOpen() {return mOpen;}
			void			Finalize();
			void			AddBlinkGlyph(TextGlyph *glyph)	{if (mBlinkGlyphList.IndexOf(glyph) < 0) mBlinkGlyphList.AddItem(glyph);}
			void			RemoveBlinkGlyph(TextGlyph *glyph)	{mBlinkGlyphList.RemoveItem(glyph);}
			void			DoBlink();
			bool			NeedsBlink() {return mBlinkGlyphList.CountItems();}
			void			ShowToolbar(bool show);
			int				ShouldShowToolbar() {return mShowToolbar;}

protected:
			virtual			~Document();
#ifdef DEBUGMENU
			void			check_lock();
			void			check_window_lock();
#else
			void			check_lock() {}
			void			check_window_lock() {}
#endif
			void			SetTitle(const char* title);
			void			SetPath(char *path);
			PageGlyph*		GetRootGlyph();
			AnchorGlyph*	MouseInAnchor(float h, float v);
			ImageGlyph*		MouseInImage(float h, float v);
 			void			SetIsIndex(bool isIndex);

			void			AddBackground(char *background, long textColor, long bgColor, long link, long vlink, long alink);
			void			AddText(TextGlyph *glyph, Style& style, const char *text,long textCount);
			void			AddScript(const char *script, int32 scriptType = -1);
			void			AddImage(Glyph *image);
			void			AddAnchor(Glyph *anchor);
			void			AddForm(Form *form);
			void			FormFinished(Form *form);
			ImageMap*		AddImageMap(char *name);
			void			AddBackgroundSound(const char *src, bool loop);
			void			AddObject(ObjectGlyph *object);
			void			MakeListFromDims(BList *dimList, BList *percentList, const char *dims);

			AnchorGlyph*	mMouseAnchor;
			ImageGlyph*		mMouseImage;
			HTMLMessage		mMouseTrack;
			
			UResourceImp*		mResource;
			Consumer*		mParser;
			DrawPort*		mDrawPort;
			DocumentGlyph*	mRootGlyph;

			BString			mTitle;
			URLParser		mBase;			// Base URL of document
			BString			mRefreshURL;
			int				mRefreshTime;
			int				mRefreshStart;
			
			ImageGlyph*		mBGImage;
			bool			mBGImageComplete;
			long			mBGColor;

			BList*			mImageList;
			BList*			mAnchorList;
			BList*			mFormList;
			CLinkedList		mImageMapList;
			
			CBucket*		mTextPool;		// Pool of all the text in the document
			AnchorGlyph*	mActionAnchor;

			Selection		*mSelection;
			bool			mInIdle;
			long			mDocRef;		// passed to CUGetResource
			int				mShowImages;
			bool			mShowBGImages;
			bool			mIsIndex;
			bool			mForceCache;
			int32			mFramesetLevel;
			FrameList*		mFrames;
			FrameItem*		mFrameLeaf;
			bool			mDoneWithFrameset;
			int32			mFrameBorder;
			ConnectionManager* mConnectionMgr;
			TLocker			mLocker;
			BMessenger*		mWorkerMessenger;
			bigtime_t		mBGBeginTime;
			float			mMarginWidth;
			float			mMarginHeight;
			BString			mBGSoundURL;
			bool			mBGSoundLoops;
			long			mALinkColor;
			long			mLinkColor;
			long			mVLinkColor;
			long			mFGColor;
			bool			mReadyForDisplay;
			bool			mOpen;
			bool			mIsSubview;
			BString			mFrameName;
			int32			mFrameNumber;
			Document*		mParent;
			uint32			mEncoding;
			BList			mBlinkGlyphList;
			BList			mLayers;
			BList			mObjects;
			LayerItem*		mCurrentLayer;
			int				mShowToolbar;
#ifdef JAVASCRIPT
			ContextWrapper*	mContextWrapper;
			BString			mOnKeyDownScript;
			BString			mOnKeyPressScript;
			BString			mOnKeyUpScript;
			BString			mOnLoadScript;
			BString			mOnMouseDownScript;
			BString			mOnMouseUpScript;
			BString			mOnUnloadScript;
			BrowserWindow*	mBrowserWindow;
			jseVariable		mDocumentVar;
			bool			mExecutedFirstScript;
			bool			mChangedSpoofState;
			bool			mJavaScriptEnabled;
			bool			mJavaScriptInitialized;
			void*			mJSELinkData;
			BList			mChildDocs;
			BList			mContainerList;
			BList			mHandlerList;
#endif
};


#ifdef JAVASCRIPT
class ContainerUser {
public:
		virtual void		SetContainer(jseVariable container) = 0;
protected:
		jseVariable			mContainer;
};
#endif


class DocAndWindowAutolock {
public:
					DocAndWindowAutolock(Document *doc);
					~DocAndWindowAutolock();
		bool		IsLocked();
		
private:
		Document*	mDocument;
		bool		mLocked;
};

inline DocAndWindowAutolock::DocAndWindowAutolock(Document *doc)
{
	mDocument = doc;
	mLocked = mDocument->LockDocAndWindow();
}

inline DocAndWindowAutolock::~DocAndWindowAutolock()
{
	if (mLocked)
		mDocument->UnlockDocAndWindow();
}

inline bool DocAndWindowAutolock::IsLocked()
{
	return mLocked;
}

class DocAutolock {
public:
					DocAutolock(Document *doc);
					~DocAutolock();
		bool		IsLocked();
		
private:
		Document*	mDocument;
		bool		mLocked;
};

inline DocAutolock::DocAutolock(Document *doc)
{
	mDocument = doc;
	mLocked = mDocument->Lock();
}

inline DocAutolock::~DocAutolock()
{
	if (mLocked)
		mDocument->Unlock();
}

inline bool DocAutolock::IsLocked()
{
	return mLocked;
}

#endif
