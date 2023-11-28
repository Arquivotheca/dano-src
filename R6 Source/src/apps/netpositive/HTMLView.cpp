// ===========================================================================
//	HTMLView.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "HTMLView.h"
#include "BeDrawPort.h"
#include "Protocols.h"
#include "UResource.h"
#include "NPApp.h"
#include "URLView.h"
#include "ProgressView.h"
#include "HTMLWindow.h"
#include "Cache.h"
#include "Bookmarks.h"
#include "DownloadManager.h"
#include "HTMLTags.h"
#ifdef ADFILTER
#include "AdFilter.h"
#endif
#include "HistoryMenu.h"
#ifdef JAVASCRIPT
#include "sebrowse.h"
#endif
#include "InputGlyph.h"
#include "Form.h"
#include "MessageWindow.h"
#include "AnchorGlyph.h"
#include "NetPositivePlugins.h"

#ifdef PLUGINS
#include "NetPositiveStreamIO.h"
#endif

#include <Autolock.h>
#include <Window.h>
#include <ScrollBar.h>
#include <Beep.h>
#include <Clipboard.h>
#include <PopUpMenu.h>
#include <PrintJob.h>
#include <Roster.h>
#include <malloc.h>
#include <FilePanel.h>
#include <Alert.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <stdlib.h>
#include <Bitmap.h>
#include <Path.h>
#include <E-mail.h>
#include <stdio.h>
#include <ctype.h>
#include <Autolock.h>
#include <Screen.h>
#include <OS.h>
#include <NodeInfo.h>
#include "NetPositive.h"

extern bool CanHandleFile(const char *mimeType);
bool gUseLatin5 = false;
bool	gRunningAsReplicant = false;
extern const char *gWindowTitle;
extern ConnectionManager *gConnectionMgr;

#define SAVE_PANEL_CLOSED 'spcl'

const uint32 msg_WorkerFinished = 'Wfin';

class ConsumerInfo {
public:
	Consumer*		mConsumer;
	StoreStatus		mStatus;
	long			mDone;
	long			mOf;
};

class HTMLWorker : public BLooper {
public:
					HTMLWorker(HTMLView *view);
					~HTMLWorker();
	virtual void	MessageReceived(BMessage *msg);
	BMessenger*		GetMessenger() {return &mMessenger;}
	virtual bool	QuitRequested();
	void			Cleanup();
	bool			DocConsumerComplete() {return mDocConsumerComplete;}

	// The DetachView() call is dangerous if you call it while the thread
	// is doing work.  Call it only as a last resort.
	void			DetachView() {mHTMLView = 0;}

private:	
	void			UpdateStatus();
	ConsumerInfo*	FindConsumerInfo(Consumer *c);
	
	HTMLView	*mHTMLView;
	BList		mConsumerList;
	BMessenger	mMessenger;
	bool		mDocConsumerComplete;
	Consumer*	mMainDocParser;
	int			mMaxQueueLength;
	bool		mNeedsToDoWork;
};


//=======================================================================

PageHistory::PageHistory(const BString &URL, const BString &target, FrameList *frames, Document *)
{
	mURL = URL;
	mTarget = target;
	mFrames = frames;
	if (mFrames)
		mFrames->Reference();
}


PageHistory::~PageHistory()
{
	if (mFrames)
		mFrames->Dereference();
}


PageHistory*
PageHistory::GetForw()
{
	return ((PageHistory *)Next());
}


PageHistory*
PageHistory::GetBack()
{
	return ((PageHistory *)Previous());
}


PageHistory* PageHistory::AddPage(const BString &url, const BString &target, FrameList *frames, Document *doc, bool isForm)
{
	PageHistory *p = NULL;

	// Don't add the URL to the history if it's identical to the current entry.
	if (!isForm && url == URL())
		return this;
		
	p = new PageHistory(url, target, frames, doc);
	AddAfter(p);

	PageHistory *removeItem = NULL;
	while ((removeItem = (PageHistory *)p->Next()) != NULL)
		delete (removeItem);

	if (Count() > 50)
		delete (First());

	return (p);
}

#ifdef PLUGINS
int32 HTMLView::CreateStreamIOThread(void *args)
{
	BMessage *msg = (BMessage *)args;
	if (!msg)
		return 0;
		
	BString error;
	const char *url = msg->FindString("url");
	const char *postData = msg->FindString("PostData");
	
	BString urlString = url;


	UResourceImp *resource;
	if (postData) {
		BString post;
		resource = GetUResourceFromForm(gConnectionMgr, urlString, &post, 0, error, NULL, NULL);
	} else
		resource = GetUResource(gConnectionMgr, urlString, 0, error,
		false, NULL, NULL, NULL, false, 0, true);
	
	while (resource->GetStore() == 0 && resource->GetStatus() != kComplete && resource->GetStatus() != kError)
		snooze(100000);
	
	Store *store = resource->GetStore();
	StreamStore *streamStore = dynamic_cast<StreamStore *>(store);
	
	bool gotStream = false;
	BMessage reply(B_NETPOSITIVE_OPEN_URL);
	
	if (streamStore) {
		while (streamStore->GetStatus() != kComplete && streamStore->GetStatus() != kError && !streamStore->GetStream())
			snooze(100000);

		if (streamStore->GetStream()) {
			streamStore->GetStream()->Reference();
			reply.AddPointer("Stream", streamStore->GetStream());
			gotStream = true;
		}
	}

	// The StreamStore maintains an extra refcount on the resource.  We can dispose of the one we're holding now.
	resource->RefCount(-1);

	reply.AddInt32("error", gotStream ? B_OK : B_ERROR);
	
	msg->SendReply(&reply);
	
	delete msg;
	return 0;
}
#endif

PluginData::PluginData()
{
	mData = 0;
}


PluginData::~PluginData()
{
	if (mData)
		delete mData;
}



HTMLView::HTMLView(
	BRect		rect, 
	const char	*name, 
	UResourceImp	*resource, 
	LinkHistory	*linkHistory,
	bool		openAsText,
	uint32		encoding,
	bool		isSubview,
	int			scrollOption,
	Document	*parentDoc,
	int			showTitleInToolbar,
	BMessage	*originatingMessage)
		: BView(rect, name, isSubview ? B_FOLLOW_NONE : B_FOLLOW_ALL,
				B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED /*| B_DRAW_ON_CHILDREN */), mLinkHistory(linkHistory), mMessenger(this)
{
	if (encoding == N_USE_DEFAULT_CONVERSION)
		encoding = gPreferences.FindInt32("DefaultEncoding");
	mEncoding = encoding;
	mNewDocResource = resource;
	mIsForm = false;
	mOpenAsText = openAsText;
	mHTMLDoc = NULL;
	mTitle = "";
	mFragment[0] = 0;
	mBGSound = 0;

	mProgStatus = kIdle;
	mProgDone = mProgOf = -1;

	mScrollPos.Set(-1.0, -1.0);
	mScrolling = kScrollingNotSet;
	mFrameNum = -1;
	mFrames = NULL;
	mFrameDepth = 0;
	mExtra = NULL;
	mIsSubview = isSubview;
	mParentDoc = parentDoc,
	mMarginWidth = 8;
	mMarginHeight = 8;
	mSecure = false;
	mScrollOption = scrollOption;

	mSavePanel = NULL;
	
	mWorkerThread = NULL;
	
	mPrevProgMsg.what = PROG_CLR;
	
	mConnectionMgr = new ConnectionManager;
	
	mNeedsToOpenURL = false;
	
	mMouseIsOverLink = false;
	mForcedCacheLoad = false;
	mCurrentCursor = 0;
	mLastBlink = 0;
	mShowTitleInToolbar = showTitleInToolbar;
	mOriginatingMessage = originatingMessage;
	
	
#ifdef JAVASCRIPT
	mBrowserWindow = new BrowserWindow;
	mBrowserWindow->open = true;
	mBrowserWindow->view = this;
	mBrowserWindow->mTimeout = NULL;
	sBrowserWindowList.AddItem(mBrowserWindow);

	mOpener = NULL;

	mLocation.view = this;
	mLocation.glyph = NULL;
	mExecutingJSURL = false;

#endif

	mPendingPOSTRequest = false;
	mDocIsPOSTRequest = false;
	mStopEnabled = false;
}

HTMLView::~HTMLView()
{
	if (mNewDocResource) {
		mNewDocResource->RemoveListener(&mMessenger);
		mNewDocResource->RefCount(-1);
		mNewDocResource = 0;
	}

//	delete mNewDocResource;
	DeleteDoc();
	
	delete (mSavePanel);
	if (mFrames)
		mFrames->Dereference();		
		
	mConnectionMgr->Dereference();
	
	if (mOriginatingMessage)
		delete mOriginatingMessage;
		
#ifdef JAVASCRIPT
	mBrowserWindow->view = NULL;
#endif
}

void HTMLView::AttachedToWindow()
{
	if (gRunningAsReplicant) {
		BView *view = Parent();
		while (view && dynamic_cast<NPBaseView *>(view) == NULL)
			view = view->Parent();
		if (view)
			mLinkHistory = ((NPBaseView *)view)->GetLinkHistory();
	}

	BView::AttachedToWindow();

	if (mScrollOption == mandatoryVScroller)	
		CreateScrollBar(B_VERTICAL);

	mMessenger = BMessenger(this);

	MakeFocus();
	
	if (gRunningAsReplicant && mNeedsToOpenURL)
		NewHTMLDocFromURL(mDocumentURL);
		
	if (sOffscreenEnabled)
		SetViewColor(B_TRANSPARENT_32_BIT);

	if (gPreferences.FindBool("BusyCursor") && !mIsSubview)
		Window()->SetPulseRate(ONE_SECOND / kNumCursorPhases);
}

void HTMLView::DetachedFromWindow()
{
	// There's a bug in BScrollbar that if you delete the scroll bar
	// after the view has been deleted, it will try to call UnsetScroller
	// on the no-longer-existent target view.  Let's proactively kill
	// the scroll bars before that can happen.
//	ResetScrollBars();
	
	DeleteDoc();
		
	BView::DetachedFromWindow();

#ifdef JAVASCRIPT
	mBrowserWindow->open = false;
#endif
}


void HTMLView::DeleteDoc(bool killWorkerSynchronously)
{
	if (mBGSound) {
//		delete mBGSound;
		mBGSound->RefCount(-1);
		mBGSound = 0;
		NetPositive::StopSong();
	}
	
	if (mWorkerThread) {
		if (killWorkerSynchronously) {

			// If the window is locked, then we need to unlock it before trying to lock the
			// worker thread, or we could deadlock, as the worker thread acquires its own thread
			// before locking the window.
			int32 lockLevel = 0;
			BWindow *window = Window();
			while (window->IsLocked()) {
				lockLevel++;
				window->Unlock();
			}
			
			if (mWorkerThread->LockWithTimeout(ONE_SECOND * 5) == B_OK) {
				int32 lockLevel2 = 0;
				while (mHTMLDoc->IsLocked()) {
					lockLevel2++;
					mHTMLDoc->Unlock();
				}

				mWorkerThread->Cleanup();

				mHTMLDoc->Lock();
					
				BLooper *looper = mWorkerThread;
				mWorkerThread = 0;
				looper->Quit();
				if (mHTMLDoc->Dereference() > 0)
					mHTMLDoc->Unlock();
				mHTMLDoc = 0;
				
				for (int32 i = 0; i < lockLevel; i++)
					window->Lock();
				return;
			}
			for (int32 i = 0; i < lockLevel; i++)
				window->Lock();
		}
		
		if (!killWorkerSynchronously) {
			mWorkerThread->PostMessage(B_QUIT_REQUESTED);
			mWorkerThread = 0;
			mHTMLDoc->Lock();
			if (mHTMLDoc->Dereference() > 0)
				mHTMLDoc->Unlock();
			mHTMLDoc = 0;
		} else {
			mWorkerThread->DetachView();
			mHTMLDoc->Lock();
			if (mHTMLDoc->Dereference() > 0)
				mHTMLDoc->Unlock();
			mHTMLDoc = 0;
			mWorkerThread = 0;
		}
	}
}

void HTMLView::CloneInNewWindow()
{
	if (mDocumentURL.Length() > 0 && !mDocIsPOSTRequest)
		NetPositive::NewWindowFromURL(mDocumentURL);
	else
		NetPositive::NewWindowFromResource(NULL);		
}

//	Set the text in the url edit field

void HTMLView::SetURLText(BString& url, bool select)
{
	BString correctedURL = url;
	if (mDocIsPOSTRequest) {
		const char *questionMark = strchr(correctedURL.String(), '?');
		if (questionMark)
			correctedURL.Truncate(questionMark - correctedURL.String());
	}
	
	BMessage msg(HTML_MSG);
	msg.AddString("url",correctedURL.String());
	msg.AddBool("select",select);
	Window()->PostMessage(&msg, Window()->FindView("url"));
}

//	Set the text in the url edit field

void HTMLView::SetProgress(StoreStatus status, long done, long of)
{
	if (status == mProgStatus && done == mProgDone && of == mProgOf)
		return;
		
	mProgStatus = status;
	mProgDone = done;
	mProgOf = of;
	
	BMessage msg(PROG_MSG);
	msg.AddInt32("status",status);
	msg.AddInt32("done",done);
	msg.AddInt32("of",of);
	
	if ((status == kDNS || status == kConnect) && mNewDocResource) {
		URLParser p;
		p.SetURL(mNewDocResource->GetURL());
		const char *tmp = p.HostName();
		if (tmp && *tmp)
			msg.AddString("Host", tmp);
	} else if ((status == kRequest || status == kIdle) && mNewDocResource) {
		URLParser p;
		p.SetURL(mNewDocResource->GetURL());
		const char *tmp = p.Path();
		if (tmp && *tmp)
			msg.AddString("Request", tmp);
	}
	Window()->PostMessage(&msg,Window()->FindView("Progress"));
	
	if (status == kComplete || status == kError) {
		mPrevProgMsg.MakeEmpty();
		mPrevProgMsg.what = PROG_CLR;
	} else
		mPrevProgMsg = msg;
}

void HTMLView::SetProgressStr(const char *str, bool permanent)
{
	BMessage msg(PROG_STR);
	msg.AddString("str",str);
	Window()->PostMessage(&msg,Window()->FindView("Progress"));
	
	if (permanent)
		mPrevProgMsg = msg;
}

void HTMLView::ClearProgressStr()
{
	BMessage msg(PROG_CLR);
	Window()->PostMessage(&msg,Window()->FindView("Progress"));
	
	mPrevProgMsg = msg;
}

void HTMLView::RestoreProgressStr()
{
	Window()->PostMessage(&mPrevProgMsg, Window()->FindView("Progress"));
}

const char *HTMLView::GetProgressStr()
{
	ProgressView* view = (ProgressView*)Window()->FindView("Progress");
	if (!view)
		return "";
	return view->GetStatus();
}

const char *HTMLView::GetDefaultProgressStr()
{
	ProgressView* view = (ProgressView*)Window()->FindView("Progress");
	if (!view)
		return "";
	return view->GetDefaultStatus();
}

void HTMLView::SetDefaultProgressStr(const char *str)
{
	ProgressView* view = (ProgressView*)Window()->FindView("Progress");
	if (!view)
		return;
	view->SetDefaultStatus(str);
}

void HTMLView::ResetScrollBars()
{	
	BAutolock lock(Window());
	if (mScrollOption != mandatoryVScroller || mFrames)
		DestroyScrollBar(B_VERTICAL);
	DestroyScrollBar(B_HORIZONTAL);
}
//	After a draw or an idle, update anyhting that needs to be updated

static void SetupScrollBar(BScrollBar *bar, float range, float proportion, float maxSteps, float& value)
{
	float min,max;
	bar->GetRange(&min,&max);
	if (max != range) {
		bar->SetRange(0,range);
		bar->SetProportion(proportion);
		bar->SetSteps(12,maxSteps);
		if ((value != -1.0) && (range >= value)) {
			bar->SetValue(value);
			value = -1.0;
		}
	}
}

void HTMLView::UpdateScrollBars()
{
//	if (mFrames && ScrollBar(B_VERTICAL))
//		DestroyScrollBar(B_VERTICAL);
	if (mFrames) {
		DestroyScrollBar(B_VERTICAL);
		DestroyScrollBar(B_HORIZONTAL);
	}

	if (mHTMLDoc == NULL) return;

	DocAndWindowAutolock lock(mHTMLDoc);

	BRect frame = Frame();
	
	float viewHeight = frame.Height();
	float docHeight = mHTMLDoc->GetHeight();
	float range = 0;

	BRect bounds = Bounds();
	
	range = MAX(0,docHeight - viewHeight);
	BScrollBar *v = ScrollBar(B_VERTICAL);
	if (v != NULL) {			
		if ((((range < 1) || (mScrolling == kNoScrolling)) && (mScrolling != kYesScrolling)) && (mScrollOption != mandatoryVScroller || mFrames))
			DestroyScrollBar(B_VERTICAL);
		else {
			SetupScrollBar(v, range, (float)viewHeight/(float)docHeight, viewHeight, mScrollPos.y);
		}
	}
	else {
		if (((range > 0) || (mScrolling == kYesScrolling)) && (mScrolling != kNoScrolling))
			CreateScrollBar(B_VERTICAL);
		else if (bounds.top != 0.0)
			ScrollTo(BPoint(bounds.left, 0.0));
	}

	float viewWidth = frame.Width();
	float docWidth = mHTMLDoc->GetUsedWidth() + 8;
	bounds = Bounds();
	
	range = MAX(0,docWidth - viewWidth);
	BScrollBar *h = ScrollBar(B_HORIZONTAL);
	if (h != NULL) {
		if (((range < 1) || (mScrolling == kNoScrolling)) && (mScrolling != kYesScrolling))
			DestroyScrollBar(B_HORIZONTAL);
		else {
			SetupScrollBar(h, range, (float)viewWidth/(float)docWidth, viewWidth, mScrollPos.x);
		}
	}
	else {
		if (((range > 0) || (mScrolling == kYesScrolling)) && (mScrolling != kNoScrolling))
			CreateScrollBar(B_HORIZONTAL);
		else if (bounds.left != 0.0)
			ScrollTo(BPoint(0.0, bounds.top));
	}
}


static void NewScrollBar(BRect frame, BView *view, orientation direction)
{
	BScrollBar *scrollbar = new BScrollBar(frame, B_EMPTY_STRING, view, 0.0, 0.0, direction);
	view->Parent()->AddChild(scrollbar);
	
	float hResize, vResize;
	if (direction == B_HORIZONTAL) {
		hResize = 0.0;
		vResize = -B_H_SCROLL_BAR_HEIGHT;
		scrollbar = view->ScrollBar(B_VERTICAL);
	} else {
		hResize = -B_V_SCROLL_BAR_WIDTH;
		vResize = 0.0;
		scrollbar = view->ScrollBar(B_HORIZONTAL);
	}
	
	if (scrollbar)
		scrollbar->ResizeBy(hResize, vResize);
	view->ResizeBy(hResize, vResize);
}

void
HTMLView::CreateScrollBar(
	orientation	direction)
{
	if (ScrollBar(direction) != NULL)
		return;
		
	if (mScrollOption == disallowScrollBars)
		return;

	if (mHTMLDoc && mHTMLDoc->ShouldShowToolbar() == kHideToolbar)
		return;

	BAutolock lock(Window());
	
	BRect sFrame = Frame();
	
	if (direction == B_HORIZONTAL)
		NewScrollBar(BRect(sFrame.left - 1.0, sFrame.bottom - B_H_SCROLL_BAR_HEIGHT + 1.0, sFrame.right + 1.0, sFrame.bottom + 1.0),
					 this, B_HORIZONTAL);
	else
		NewScrollBar(BRect(sFrame.right - B_V_SCROLL_BAR_WIDTH + 1.0, sFrame.top - 1.0, sFrame.right + 1.0, sFrame.bottom + 1.0),
					 this, B_VERTICAL);

	UpdateScrollBars();
}

void
HTMLView::DestroyScrollBar(
	orientation	direction)
{
	BScrollBar *s = ScrollBar(direction);
	BAutolock lock(Window());
		
	if (s != NULL) {
		s->RemoveSelf();
		delete s;

		float hResize, vResize;
		if (direction == B_VERTICAL) {
			hResize = B_V_SCROLL_BAR_WIDTH;
			vResize = 0.0;
			mScrollPos.y = 0;
		} else {
			hResize = 0.0;
			vResize = B_H_SCROLL_BAR_HEIGHT;
			mScrollPos.x = 0;
		}
		
		ResizeBy(hResize, vResize);

		s = ScrollBar((direction == B_VERTICAL) ? B_HORIZONTAL : B_VERTICAL);
		if (s != NULL)
			s->ResizeBy(hResize, vResize);
	}
}

void
HTMLView::SetScrolling(
	int32 	scroll)
{
	mScrolling = scroll;	
	UpdateScrollBars();
}


//	Create a new HTML document when mNewDocResource is 'qualified'

void HTMLView::QualifyNewDoc()
{
	if (mNewDocResource == NULL)
		return;

	if (mShowTitleInToolbar != kShowTitleUnspecified && gPreferences.FindBool("ShowTitleInToolbar")) {
		URLView *urlView = (URLView *)Window()->FindView("URLView");
		BTextControl *control = dynamic_cast<BTextControl *>(urlView->FindView("url"));
		if (control) {
			control->TextView()->MakeEditable(mShowTitleInToolbar == kShowURL);
			control->TextView()->MakeSelectable(mShowTitleInToolbar == kShowURL);
		}
	}		

//	Delete resource if an error occurred while loading

	if (mNewDocResource->GetStatus() == kError) {
		mStopEnabled = false;
		UpdateURLView();
		ClearProgressStr();

		BString str;
		mNewDocResource->GetErrorMessage(str);
		if (str.Length() == 0) {
			str = GetErrorGeneric();
			str.ReplaceFirst("%s", mNewDocResource->GetURL());
		}
		if(str != SHOW_NO_ERROR){
//			SimpleAlert(str.String());
			void* data = (void*)(str.String());
			UResourceImp* r = NewResourceFromData(data,str.Length(),mNewDocResource->GetURL(),BString("text/html"));	// Keep UResourceImp and url around
			//delete mNewDocResource;
			if (mNewDocResource)
				mNewDocResource->RefCount(-1);
			mNewDocResource = r;
			mIsForm = false;

			mStopEnabled = false;
			UpdateURLView();
			ClearProgressStr();
		} else {

			//delete mNewDocResource;
			mNewDocResource->RefCount(-1);
			mNewDocResource = NULL;
			mIsForm = false;
			return;
		}
	}
		
//	Wait until type is set before it is qualified

	// There's a race condition here.  What happens if the content type changes while we're executing this code?
	// the contentType variable becomes invalid.
	const char *contentType = mNewDocResource->GetContentType();
	if (contentType == NULL) {
		return;
	}
	
	if (mNewDocResource->GetLastVisited() < mNewDocResource->GetLastModified())
		mNewDocResource->SetLastVisited(mNewDocResource->GetLastModified());

#ifdef UPDATEBOOKMARKS
	BookmarkFolder::GetBookmarkFolder()->SetItemTimes(mNewDocResource->GetURL(), (int)mNewDocResource->GetLastVisited(), (int)mNewDocResource->GetLastModified());
#endif
	mNewDocResource->RemoveListener(&mMessenger);

//	Identify as text, html, gif or jpeg

	mImageURL = "";
	bool textType = strstr(contentType,"text/html") || strstr(contentType,"text/plain") || strstr(contentType,"text/text");
	if (!textType) {
		bool imageType = strstr(contentType,"image/gif") || strstr(contentType,"image/jpeg") || strstr(contentType,"image/jpg") || strstr(contentType,"image/png");
		
	//	Deterine the 'name' of the file
	
		const char *url = mNewDocResource->GetURL();
		const char *name = url;
		const char *s;
		while ((bool)(s = strchr(name,'/')))
			name = s + 1;

	//	If it is a GIF or a JPEG file, create a nice HTML wrapper

		if (imageType) {
			BString htmldata;
			htmldata = "<HTML><HEAD><TITLE>";
			htmldata += name;
			htmldata += "</TITLE></HEAD><BODY BGCOLOR=FFFFFF><CENTER><IMG SRC=\"";
			htmldata += url;
			htmldata += "\"></BODY>";
			mImageURL = url;
			void* data = (void*)(htmldata.String());
			UResourceImp* r = NewResourceFromData(data,htmldata.Length(),BString(url),BString("text/html"));	// Keep UResourceImp and url around
			//delete mNewDocResource;
			mNewDocResource->RefCount(-1);
			mNewDocResource = r;
			mIsForm = false;

			mStopEnabled = false;
			UpdateURLView();
			ClearProgressStr();
		} else {
			//	Otherwise save it to disk
			mStopEnabled = false;
			UpdateURLView();
			ClearProgressStr();
	
			DownloadManager::DownloadFile(mNewDocResource, mConnectionMgr, true);
			mNewDocResource->RefCount(-1);
			mNewDocResource = NULL;
			mIsForm = false;
			return;
		}
	}
	
//	Resource is approved and ready, make a document out of it

	pprint("QualifyNewDoc: '%s' (%s)\n",mNewDocResource->GetURL(),contentType);
	mConnectionMgr->SetReferrer(mNewDocResource->GetURL());
	NewHTMLDoc(mNewDocResource,mOpenAsText);
	mNewDocResource->RefCount(-1);
	mNewDocResource = NULL;
	mIsForm = false;
	return;
}

void HTMLView::NewHTMLDoc(UResourceImp *resource, bool openAsText, bool forceCache)
{
	const char *f = resource->GetFragment();
	if (f)
		strcpy(mFragment,f);			// Copy fragment
	else
		*mFragment = 0;

	if (mScrolling == kScrollingNotSet) {
		mScrolling = kAutoScrolling;
	}
	
	if (mFrames) {
		pprint("NewHTMLDoc is decomissioning frames for 0x%x", this);
		mFrames->Dereference();
		mFrames = NULL;
	}

	BWindow *window = Window();
	DeleteDoc();
	KillChildren();
	SetDefaultProgressStr(" ");
	mMouseIsOverLink = false;
	
//	Construct a document
	DrawPort *drawPort = new BeDrawPort;
	((BeDrawPort *)drawPort)->SetView(this);
	((BeDrawPort *)drawPort)->SetEncoding(mEncoding);
	
	// In the future, get this out of a preference.
	{
		BAutolock lock(Window());
		if (sOffscreenEnabled /* && !openAsText */) {
// If offscreen is enabled, the view color never gets changed.
//			SetViewColor(B_TRANSPARENT_32_BIT);
			SetLowColor(0xff, 0xff, 0xff);
		} else {
			SetLowColor(0xff, 0xff, 0xff);
			SetViewColor(LowColor());
			ClearViewBitmap();
		}
	}

	mDocIsPOSTRequest = mPendingPOSTRequest;
	mReferrer = mPendingReferrer;
	
	if (mAddToHistory) {
		mAddToHistory = false;
		BString temp = resource->GetURL();
		if (*mFragment) {
			temp += '#';
			temp += mFragment;
		}
		mLinkHistory->AddToHistory(temp.String(), strcmp(Name(), kHTMLViewName) == 0 ? NULL : Name(), NULL);
	}

	if (!mIsSubview) {
		if (mLinkHistory &&
			mLinkHistory->GetCurrentHistory() &&
			mLinkHistory->GetCurrentHistory()->GetExtra())
			SetExtra(mLinkHistory->GetCurrentHistory()->GetExtra());
		else
			SetExtra(NULL);
	} else
		mScrollPos = BPoint(0,0);

	CLinkedList *userValues = NULL;
	BList *pluginData = NULL;
	PageHistory *hist = mLinkHistory->GetCurrentHistory();
	if (hist && hist->GetExtra() && (forceCache || mForcedCacheLoad)) {
		userValues = hist->GetExtra()->UserValues();
		pluginData = hist->GetExtra()->GetPluginData();
	}

	if (!mWorkerThread) {
		mWorkerThread = new HTMLWorker(this);
		NetPositive::AddThread(mWorkerThread->Run());
	}

	pprint("CreateFromResource.  mWorkerThread 0x%x  messenger 0x%x", mWorkerThread, mWorkerThread->GetMessenger());
#ifdef JAVASCRIPT
	mHTMLDoc = Document::CreateFromResource(mConnectionMgr, resource, drawPort, (long)this, openAsText, 
											mEncoding, forceCache, userValues, pluginData, mWorkerThread->GetMessenger(),
											mIsSubview ? Name() : NULL, mFrameNum, mParentDoc, mBrowserWindow);
#else
	mHTMLDoc = Document::CreateFromResource(mConnectionMgr, resource, drawPort, (long)this, openAsText, 
											mEncoding, forceCache, userValues, pluginData, mWorkerThread->GetMessenger(),
											mIsSubview ? Name() : NULL, mFrameNum, mParentDoc);
#endif
											
	mTitle = "";
	mFirstUpdate = true;
	{
		DocAutolock lock(mHTMLDoc);
		mHTMLDoc->SetMarginWidth(mMarginWidth);
		mHTMLDoc->SetMarginHeight(mMarginHeight);
		mHTMLDoc->ShowImages(gPreferences.FindBool("ShowImages"));
		mHTMLDoc->ShowBGImages(gPreferences.FindBool("ShowBGImages"));
	
	#ifdef JAVASCRIPT
		mHTMLDoc->EnableJavaScript(!openAsText && (gPreferences.FindBool("EnableJavaScript") ||
			strncmp(resource->GetURL(), "netpositive:", 12) == 0), this);
	#endif
	}
		
//	A new document is here.. add to list, update forward and back buttons
	mDocumentURL = resource->GetURL();
#ifdef JAVASCRIPT
	mLocation.location = mDocumentURL;
#endif
	
	BMessage msg(PROG_LOCK);
	msg.AddBool("locked", strncmp(mDocumentURL.String(), "https", 5) == 0);
	Window()->PostMessage(&msg,Window()->FindView("Progress"));	

	mStopEnabled = true;
	UpdateURLView();
//	Now show its url...

	if (!mIsSubview)
		SetURLText(mDocumentURL,true);

//	Reset ScollBars and buttons and things. Idle the document to get it going

	mProgStatus = kIdle;
	mProgDone = mProgOf = -1;
	
//	Start doc layout
	BRect bounds;
	{
		BAutolock lock(window);
		bounds = Bounds();
	}

	DocAutolock lock(mHTMLDoc);
	mHTMLDoc->Layout(bounds.Width()/*-16*/);
	return;
}

#ifdef JAVASCRIPT
void HTMLView::CreateBlankDocument()
{
	DrawPort *drawPort = new BeDrawPort;
	((BeDrawPort *)drawPort)->SetView(this);
	((BeDrawPort *)drawPort)->SetEncoding(mEncoding);
	mHTMLDoc = Document::CreateFromResource(mConnectionMgr, NULL, drawPort, (long)this, false, 
											mEncoding, false, NULL, NULL, NULL, 0, NULL, mBrowserWindow);
	mHTMLDoc->EnableJavaScript(gPreferences.FindBool("EnableJavaScript"), this);
}

bool HTMLView::AllowPopupWindow()
{
	if (mExecutingJSURL)
		return true;
	return gPreferences.FindBool("AllowJSPopupWindows");
}
#endif

//	Create a new doc from a url

void HTMLView::NewHTMLDocFromURL(const BString& url, bool isForm, BString* post, bool forceCache, bool addToHistory, const char *referrer, BMessage *originatingMessage)
{
	if (HandleFragment(url))
		return;
		
	if (mOriginatingMessage)
		delete mOriginatingMessage;
	
	mOriginatingMessage = originatingMessage;

	mPendingPOSTRequest = false;
	BString correctedURL = url;
	
	pprint("HTMLView::NewHTMLDocFromURL: %s", url.String());
	
	if (ValidateURL(correctedURL) == false) {			// Add http:// or www.xxx.com if required...
		pprint("HTMLView::NewHTMLDocFromURL: Validate Failed");
		return;
	}
	
//	Send a message to the mail app if url is a mailto
	
#ifdef JAVASCRIPT
	if (strncmp(correctedURL.String(), "javascript:", 11) == 0) {
		int32 offset = 11;
		if (correctedURL[offset] == '/')
			offset++;
		if (mHTMLDoc) {
			if (!mHTMLDoc->LockDocAndWindow())
				return;
			mExecutingJSURL = true;
			mHTMLDoc->ExecuteScript(correctedURL.String() + offset);
			mExecutingJSURL = false;
			mHTMLDoc->UnlockDocAndWindow();
		}
		return;
	}
#endif

	UResourceImp *resource = NULL;

	// If the URL points to an executable file, launch the file.  For security reasons, this
	// only works if a link from a document stored locally on the machine is clicked, so check
	// mDocumentURL.
	if (gPreferences.FindBool("LaunchLocalApps") && (mDocumentURL.Compare("file:/", 6) == 0 || (referrer && strcmp(referrer, "__NetPositive_toolbar") == 0)) && strncmp(url.String(), "file:", 5) == 0) {
		URLParser parser;
		parser.SetURL(url.String());
		char path[2048];				// I hate this static allocation shit
		char *arguments[256];
		int argc = 0;
		
		if (parser.Path() == NULL)			// Default path is "/"
			strcpy(path,"/");
		else
			CleanName(parser.Path(),path);	// Strip %20 escape codes from url
		
		// Command line arguments follow the colon, if there is one.
		char *pos = strchr(path, ':');
		if (pos && strncmp(pos, ":stream:", 8) == 0) {
			// In stream mode, run the application, read whatever the app spits to stdout,
			// and load the result as a page.
			
			// Build the command line to execute.
			*pos = 0;
			pos += 8;
			BString escapedPath = path;
			escapedPath.ReplaceAll(" ", "\\ ");
			escapedPath += " ";
			escapedPath += pos;
			
			FILE *file = popen(escapedPath.String(), "r");
			

			// This is inefficient as hell.			
			// Don't you hate seeing comments in code like this?
			BString returnedData;
			char buffer[1025];
			int32 read;
			int32 length = 0;
			do {
				read = fread(buffer, 1, 1024, file);
				length += read;
				if (read > 0) {
					buffer[read] = 0;
					returnedData += buffer;
				}
			} while (read > 0 && returnedData.Length() < 1024 * 1024);

			resource = NewResourceFromData(returnedData.String(),returnedData.Length(),BString(url),BString("text/html"));	// Keep UResourceImp and url around
			resource->SetContentLength(length);
			resource->Flush(kComplete);
			if (resource) {
				resource->AddListener(&mMessenger);
				resource->NotifyListeners(msg_ResourceChanged);
				resource->NotifyListeners(msg_ResourceFlushed);
			}
			
			pclose(file);
		} else {
			if (pos) {
				*pos++ = 0;
				while (pos && *pos) {
					while (pos && *pos && isspace(*pos))
						pos++;
	
					if (!pos || !*pos)
						break;
					if (*pos == '"') {
						arguments[argc] = ++pos;
						pos = strchr(pos, '"');
					} else if (*pos == '\'') {
						arguments[argc] = ++pos;
						pos = strchr(pos, '\'');
					} else {
						arguments[argc] = pos;
						pos = strchr(pos, ' ');
					}
					if (pos && *pos)
						*pos++ = 0;
					if (++argc == 256)
						break;
				}
			}
				
			entry_ref ref;
			if (path[strlen(path) - 1] != '/' && get_ref_for_path(path,&ref) == B_NO_ERROR) {
				BNode node(&ref);
				if (node.InitCheck() == B_OK) {
					BNodeInfo info(&node);
					if (info.InitCheck() == B_OK) {
						char mimeType[B_MIME_TYPE_LENGTH];
						*mimeType = 0;
						info.GetType(mimeType);
						if (*mimeType &&
						    strstr(mimeType,B_PE_APP_MIME_TYPE) ||
						    strstr(mimeType,B_ELF_APP_MIME_TYPE) || 
						    strstr(mimeType,B_PEF_APP_MIME_TYPE) ||
							!CanHandleFile(mimeType)) {
							if (argc > 0)
								be_roster->Launch(&ref, argc, arguments);
							else
								be_roster->Launch(&ref);
							return;
						}
					}
				}
			}
		}
	}
	
	if (referrer && strcmp(referrer, "__NetPositive_toolbar") == 0)
		referrer = 0;
	
	mConnectionMgr->KillAllConnections();
		
//	Get a resource
//	Wait until resource data arrives and has its type set
	
	BString msg;
	mForcedCacheLoad = forceCache;
	
#ifndef NOSSL
	if (!mIsSubview) {
		bool pageIsSecure = (strncmp(correctedURL.String(), "https", 5) == 0);
		if (!mSecure && pageIsSecure && gPreferences.FindBool("SSLEnterWarning"))
			SimpleAlert(kWarningEnteringSSL);
		else if (mSecure && !pageIsSecure && gPreferences.FindBool("SSLLeaveWarning"))
			SimpleAlert(kWarningLeavingSSL);
			
		mSecure = pageIsSecure;
	}
#endif

	mPendingReferrer = referrer;
	if (url.Length() && !resource)
		if (isForm) {
			mPendingPOSTRequest = post && post->Length();
			resource = GetUResourceFromForm(mConnectionMgr, correctedURL,post,(long)this, msg, &mMessenger, referrer);
		} else 
			resource = GetUResource(mConnectionMgr, correctedURL,(long)this, msg, forceCache, NULL, &mMessenger, referrer, true);
		
#ifdef NOSSL
	if (resource == NULL && url.Length() && msg == kErrorSSLNotImplemented) {
		BAlert *alert = new BAlert("", msg.String(), kErrorSSLGoThere, kOKButtonTitle);
		if (alert->Go() == 0)
			OpenLink(BString(kErrorSSLNetPositiveSite), "_top");
	} else 
#endif
	if (resource == NULL && url.Length()) {
//		SimpleAlert(msg.String());
		void* data = (void*)(msg.String());
		mNewDocResource = resource = NewResourceFromData(data,msg.Length(),BString(url),BString("text/html"));	// Keep UResourceImp and url around
		resource->AddListener(&mMessenger);
		mNewDocResource->NotifyListeners(msg_ResourceChanged);
		mNewDocResource->NotifyListeners(msg_ResourceFlushed);
	}
	
	if (url.Length()){
		mNewDocResource = resource;		// Wait for this resource to 'qualify'
		mIsForm = isForm;
		SetProgress(kIdle, 0, 0);

		pprint("HTMLView::NewHTMLDocFromURL: got resource");
		
		mAddToHistory = addToHistory;

		if (!mIsSubview)
			mScrolling = kScrollingNotSet;
		
		SetExtra(NULL);

		PageHistory *curHistory = mLinkHistory->GetCurrentHistory();
		if (curHistory->GetExtra() != NULL) {
			CLinkedList *values = GetUserValues();
			if (values)
				curHistory->GetExtra()->SetUserValues(GetUserValues());
			}

		mStopEnabled = true;
		UpdateURLView();
	}
}


void HTMLView::SetExtra(ExtraPageData *extra)
{
	mExtra = extra;
	if (mExtra)
		mScrollPos = mExtra->ScrollPos();
}

CLinkedList * HTMLView::GetUserValues()
{
	CLinkedList *val = NULL;
	if (mHTMLDoc && mWorkerThread) {
		DocAutolock lock(mHTMLDoc);
//printf("UserVal   0x%x\n", mHTMLDoc->GetResource());
		if (mWorkerThread->DocConsumerComplete() && mLinkHistory->GetCurrentHistory()) {
			if (strcmp(mHTMLDoc->GetResource()->GetURL(), mLinkHistory->GetCurrentHistory()->URL()) == 0)
				val = mHTMLDoc->UserValues();
		}
	}
	return val;			
}

//	Make something visible if it isn't already

void HTMLView::MakeVisible(float h, float v)
{
	BAutolock lock(Window());
	BRect bounds = Bounds();
	if (h >= bounds.left && h + 20 < bounds.right)
		h = bounds.left;
	if (v >= bounds.top && v + 30 < bounds.bottom)
		v = bounds.top;
	if (h != bounds.left || v != bounds.top)
		ScrollTo(h, v);
}

//	Try and find something

bool HTMLView::FindString(const char *findThis)
{
	if (mHTMLDoc == NULL)
		return false;
	if (findThis == NULL)
		return false;
	
//	Start from current selection or from the beginning of the pool
	
	DocAutolock lock(mHTMLDoc);
	char *text = mHTMLDoc->GetTextPool();
	long count = mHTMLDoc->GetTextLength();
	long start = 0;
	long selcount;
	if (mHTMLDoc->GetSelectedText(&start,&selcount))
		start += selcount;
	if (count == 0 || text == NULL)
		return false;
	
//	Do the find

	long found = -1;
	char lc = tolower(findThis[0]);
	char uc = toupper(findThis[0]);
	for (long i = start; i < count; i++) {
		if (text[i] == lc || text[i] == uc) {
			const char *s = findThis;
			const char *t = text + i;
			while (*s && (tolower(*s) == tolower(*t))) {
				s++;
				t++;
			}
			if (*s == 0) {
				found = i;
				break;
			}
		}
	}
	
//	Select the text if it worked

	if (found != -1) {
		mHTMLDoc->SelectText(found,strlen(findThis));
		float v = mHTMLDoc->GetSelectionTop();
		if (v < 0)
			v = Bounds().top;
		float h = mHTMLDoc->GetSelectionLeft();
		if (h < 0)
			h = Bounds().left;
		MakeVisible(h,v);
	} else
		beep();
		
	return found != -1;
}

//	Print the unpaginated html

void HTMLView::PageSetup()
{
	BPrintJob	print("NetPositive_print");
	status_t	result;

	if (gPrintSettings)
		print.SetSettings(new BMessage(*gPrintSettings));

	if ((result = print.ConfigPage()) == B_NO_ERROR) {
		delete gPrintSettings;
		gPrintSettings = print.Settings();
	}
}

void HTMLView::Print()
{
	long			loop;
	long			pages;
	BRect			r;
	float			docHeight;

	if (mFrames) {
		docHeight = Bounds().Height();
	} else {
		if (mHTMLDoc == NULL)
			return;
			
		DocAutolock lock(mHTMLDoc);
		docHeight = mHTMLDoc->GetHeight();
		if (docHeight == 0)
			return;
	}
	
	const char *title = mTitle.Length() ? mTitle.String() : kPrintJobTitle;
	BPrintJob the_print(title);

	if (!gPrintSettings) {
		PageSetup();
		if (!gPrintSettings)
			return;
	}
	
	the_print.SetSettings(new BMessage(*gPrintSettings));
	
	if (the_print.ConfigJob() != B_NO_ERROR)
		return;

	r = the_print.PrintableRect();
	pprint("Printable Rect: %f %f %f %f",r.top,r.left,r.bottom,r.right);
	
	r.OffsetTo(0,0);

	the_print.BeginJob();
	if (!the_print.CanContinue())
		return;

	BWindow *window = Window();
	window->DisableUpdates();
	window->UpdateIfNeeded();
	SetPrintingMode(true);
	RealRelayoutView(r.Width(), false);

	//since we relayout-ed we have to figure out the new docHeight
	mHTMLDoc->Lock();
	docHeight = mHTMLDoc->GetHeight();
	mHTMLDoc->Unlock();
	pages = (long)(docHeight / r.Height());

	int32 curPage = 1;	
	for (loop = 0; loop <= pages; loop++) {
		pprint("Printing Rect: %f %f %f %f",r.top,r.left,r.bottom,r.right);
		if ((curPage >= the_print.FirstPage()) && (curPage <= the_print.LastPage())) {
			the_print.DrawView(this, r, BPoint(0, 0));
			the_print.SpoolPage();
			if (!the_print.CanContinue()) {
				SetPrintingMode(false);
				RealRelayoutView(-1.0, false);
				window->EnableUpdates();
				Invalidate();
				return;
			}
		}
		r.OffsetBy(0,r.Height());
		curPage++;
	}
	SetPrintingMode(false);
	RealRelayoutView(-1.0, false);
	Invalidate();
	window->EnableUpdates();
	if (!the_print.CanContinue())
		return;
	the_print.CommitJob();
}


void HTMLView::Reset(bool purgeCache)
{
	BWindow *window = Window();
	BAutolock lock(window);

	// If there are children, reset them, too.
	if (mFrames && mHTMLDoc == NULL) {
		FrameItem *frame = (FrameItem *)mFrames->First();
		while(frame != NULL) {
			if (frame->View()) {
				HTMLView *view = frame->View();
				if (view && view != this)
					view->Reset(purgeCache);
			}
 			if (!frame->Next() && frame->Frames())
 				frame = (FrameItem *)frame->Frames()->First();
 			else
 				frame = (FrameItem *)frame->Next();
		}
	}

	DeleteDoc();
	
	if (purgeCache && mDocumentURL.Length())			
		UResourceCache::Purge(mDocumentURL.String());
		
	ResetScrollBars();

	BRect r = window->Bounds();

	UpdateDocRect(r);
}

void HTMLView::GrazeLinks(BList *urlList)
{
	BWindow *window = Window();
	BAutolock lock(window);

	// If there are children, reset them, too.
	if (mFrames) {
		FrameItem *frame = (FrameItem *)mFrames->First();
		while(frame != NULL) {
			if (frame->View()) {
				HTMLView *view = frame->View();
				if (view && view != this)
					view->GrazeLinks(urlList);
			}
 			if (!frame->Next() && frame->Frames())
 				frame = (FrameItem *)frame->Frames()->First();
 			else
 				frame = (FrameItem *)frame->Next();
		}
	}
	
	if (mHTMLDoc) {
		DocAutolock lock2(mHTMLDoc);
		mHTMLDoc->GrazeLinks(urlList);
	}
}

int32 HTMLView::GetHistoryLength()
{
	if (mLinkHistory)
		return mLinkHistory->CountItems();
	else
		return 0;
}

const char *HTMLView::GetHistoryItem(int32 index)
{
	if (mLinkHistory) {
		PageHistory *hist = mLinkHistory->ItemAt(index);
		if (hist)
			return hist->URL();
		else
			return "";
	} else
		return "";
}

void HTMLView::KillChildren()
{
	ResetScrollBars();
// Destroy any old child views.
	BWindow *window = Window();
	BAutolock lock(window);

	window->BeginViewTransaction();
	for (int32 i = CountChildren() - 1; i >= 0; i--) {
		BView *view = ChildAt(i);
		if (view) {
			RemoveChild(view);
			delete view;
		}
	}
	window->EndViewTransaction();
}

void HTMLView::Reload()
{
	if (mDocumentURL.Length() == 0)
		return;

	BWindow *window = Window();
	bool extraLock = false;

	// If the window is locked, then we need to unlock it before trying to lock the
	// worker thread, or we could deadlock, as the worker thread acquires its own thread
	// before locking the window.
	int32 lockLevel = 0;
	while (window->IsLocked()) {
		lockLevel++;
		window->Unlock();
	}

	BRect r;
	if (mHTMLDoc) {
			if (mWorkerThread->LockWithTimeout(ONE_SECOND * 5) != B_OK) {
			goto done;
		}
		
		window->Lock();
		extraLock = true;
		
		if (!mHTMLDoc->Lock()) {
			mWorkerThread->Unlock();
			goto done;
		} else {
			mHTMLDoc->KillImageResources();
			UResourceImp *res = mHTMLDoc->GetResource();
			if (res) {
				res->MarkForDeath();
//				res->RefCount(-1);
			}
			//delete res;
			mHTMLDoc->SetResource(NULL);
			if (mNewDocResource == res)
				mNewDocResource = NULL;
			DeleteDoc();
		}
	}
	if (mNewDocResource) {
		//delete mNewDocResource;
		mNewDocResource->RefCount(-1);
		mNewDocResource = NULL;
	}

	{
		PageHistory *hist = mLinkHistory->GetCurrentHistory();
		if (hist && hist->GetExtra())
			hist->GetExtra()->SetUserValues(NULL);
	}

	Reset();
	NewHTMLDocFromURL(mDocumentURL);
	{
		BAutolock lock(window);
		r = window->Bounds();
	}
	UpdateDocRect(r);
	
done:
	for (int32 i = 0; i < lockLevel; i++)
		window->Lock();
	if (extraLock)
		window->Unlock();
}

bool OpenSource(UResourceImp* resource);

//	Messages about opening other documents

void HTMLView::MessageReceived(BMessage *msg)
{
	InputGlyph *g;
	char *data;
	long length;
	long start,count;
	
	BString URL;
	BString bookmarkURL;
	
	if (msg->WasDropped()) {
		if (mIsSubview)
			Parent()->MessageReceived(msg);
		else {
			// Do not open the url if it was dragged from this page
			BLooper *looper;
			BHandler *handler = msg->ReturnAddress().Target(&looper);
			if (handler != this) {
				const char *url = msg->FindString("be:url");
				if (url && *url) {
					NewHTMLDocFromURL(url);
					return;
				}
			}
		}
	}

	switch (msg->what) {

		case 'find': {
			if (mHTMLDoc == NULL) break;
			const char *s = 0;
			msg->FindString("findthis", &s);
			DocAutolock lock(mHTMLDoc);
			if (FindString(s) == false && mHTMLDoc->GetSelectedText(&start,&count)) {	// Try again if there was a selection
				mHTMLDoc->SelectText(0,0);
				FindString(s);
			}
			break;
		}
			
		case B_SIMPLE_DATA: {
			ulong type;
			pprint("Got a B_SIMPLE_DATA!");
			msg->GetInfo("refs", &type, &count);
			pprint("Got a type(%d) and count (%d)!",type,count);
			if (count > 0) {
				NetPositive *np = (NetPositive *)be_app;
				
				entry_ref ref;
				msg->FindRef("refs",&ref);		// the first file will replace the document assoc with 'this'
				if (np->RefToURL(ref,URL))
					NewHTMLDocFromURL(URL);
				
				for (long i = 1; i < count; i++) {			// any remaining files will get opened in new windows.
					pprint("Got a ref!");
					msg->FindRef("refs", i, &ref);
					if (np->RefToURL(ref,URL))
						np->NewWindowFromURL(URL, mEncoding);
				}
			}
			break;
		}

		case B_CUT:
		case B_COPY: {
			pprint("HTML View got cut or copy");
			DocAutolock lock(mHTMLDoc);
			if ((bool)(data = mHTMLDoc->GetFormattedSelection(&length))) {
				be_clipboard->Lock();
				be_clipboard->Clear();
				BMessage *m = be_clipboard->Data();
				m->AddData("text/plain",B_MIME_TYPE,data,length);
				be_clipboard->Commit();
				be_clipboard->Unlock();
				free(data);
			}
			break;
		}
		
		case SAVE:
			pprint("SAVE in view");
			if (mImageURL.Length() != 0)
				DownloadManager::DownloadFile(mImageURL.String(), false, true);
			else
				if (mDocumentURL.Length() != 0)
					DownloadManager::DownloadFile(mDocumentURL.String(), false, true);
			break;

#ifdef DEBUGMENU
		case DO_DUMPGLYPHS:
			if (mHTMLDoc != NULL)
				mHTMLDoc->Print();
			break;
#endif
			
		case HTML_MSG + HM_ANCHOR:
			{
			BString target(msg->FindString("target"));
			bool forceCache = false;
			msg->FindBool("forceCache", &forceCache);
			BString post(msg->FindString("post"));
			bool isForm = false;
			msg->FindBool("isForm", &isForm);
			const char *referrer = msg->FindString("referrer");

			if (target.Length() > 0) {
				BView *targetView = FindView(target.String());
				BMessage	msg2(HTML_MSG + HM_ANCHOR);
				msg2.AddString("url", msg->FindString("url"));
				msg2.AddBool("forceCache", forceCache);
				if (post.Length() > 0)
					msg2.AddString("post", post.String());
				msg2.AddBool("isForm", isForm);
				if (targetView) {
					pprint("HTMLView %x received HM_ANCHOR.  Redirecting to child %x", this, targetView);
					Window()->PostMessage(&msg2, targetView);
				} else {
					if (target.Length() > 0)
						msg2.AddString("target", target.String());
					pprint("HTMLView %x received HM_ANCHOR.  Redirecting to parent %x", this, Parent());
					Window()->PostMessage(&msg2, Parent());
				}		
				break;			
			}
			
			pprint("HTMLView %x received HM_ANCHOR.  Handling.", this);
			mPendingReferrer = referrer;
			NewHTMLDocFromURL(BString(msg->FindString("url")), isForm, post.Length() > 0 ? &post : NULL, forceCache, true, referrer);
			break;
			}
						
		case NP_STOPBUTTON: {
			if (mNewDocResource) {
				//delete mNewDocResource;
				mNewDocResource->RefCount(-1);
				mNewDocResource = NULL;
				mIsForm = false;
				SetProgressStr(kStatusCancelled);
				mConnectionMgr->KillAllConnections();
			} else
				Stop();
			mStopEnabled = false;
			UpdateURLView();
			break;
		}
	
		case ADD_BOOKMARK:
			{
			BookmarkFolder::GetBookmarkFolder()->AddEntry(mDocumentURL.String(),mTitle.String());
			break;
			}
			
		case OPEN_HIST_BOOKMARK:
		case OPEN_BOOKMARK: {
			const char *url = 0;
			msg->FindString("url", &url);
			if (url)
				NewHTMLDocFromURL(BString(url));
			break;
		}

		case DO_PRINT:
			Print();
			break;

		case OPEN_HTML: {
			UResourceImp *r = NULL;
			
			if (mHTMLDoc) {
				DocAutolock lock(mHTMLDoc);
				r = mHTMLDoc->GetResource();	// Clone resource
				r->RefCount(1);
			} else if (mDocumentURL.Length()){
				BString msg;
				r = GetUResource(mConnectionMgr, mDocumentURL, 0, msg, true, NULL, &mMessenger);
			}
			if (r)
				((NetPositive *)be_app)->NewWindowFromResource(r, true, mEncoding);

			break;
		}
		case LOAD_DOPE: {
			NewHTMLDocFromURL(BString("http://www.bedope.com"));
			break;
		}		
//		Forms

		case FORM_MSG + AV_BUTTON: {
			void* obj = NULL;
			msg->FindPointer("InputGlyph",&obj);
			g = (InputGlyph *)obj;
#ifdef JAVASCRIPT
			g->ExecuteOnClickScript();
#endif
			break;
		}

		case FORM_MSG + AV_SUBMIT: {
			void* obj = NULL;
			g = NULL;
			msg->FindPointer("InputGlyph",&obj);
			g = (InputGlyph *)obj;

			Form *f = 0;
			if (!g) {
				msg->FindPointer("Form",&obj);
				f = (Form *)obj;
			}

			DocAutolock lock(mHTMLDoc);
	
			BString* post;
			if (f)
				post = mHTMLDoc->FormSubmission(f,g,URL);
			else
				post = mHTMLDoc->FormSubmission(g,URL);

			BMessage msg2(HTML_MSG + HM_ANCHOR);
			msg2.AddString("url", URL.String());
			msg2.AddBool("isForm", true);
			if (post && post->Length() > 0)
				msg2.AddString("post", (*post).String());

			msg2.AddString("referrer", mDocumentURL.String());

			const char *target = "";
			if (g)
				target = mHTMLDoc->FormTargetFrame(g);
			pprint("Form submission.  Target is %s", target);

			int32 formOption = gPreferences.FindInt32("FormSubmitOption");
			if (!mSecure && post && post->Length() > 0 && (formOption == 2 || formOption == 1 && strchr((*post).String(), '&') != 0)) {
				BAlert *alert = new BAlert("",kWarningInsecureForm,kCancelButtonTitle,kContinueButtonTitle);
				if (alert->Go() == 0)
					return;
			}
		
			if (target)
				msg2.AddString("target", target);
			Window()->PostMessage(&msg2, this);
#ifdef JAVASCRIPT
			if (!f && g)
				f = g->GetForm();
			f->ExecuteSubmitScript();
#endif
			break;
		}
			
		case FORM_MSG + AV_RESET: {
			void *obj = NULL;
			msg->FindPointer("InputGlyph",&obj);
			g = (InputGlyph *)obj;
			DocAutolock lock(mHTMLDoc);
			mHTMLDoc->FormReset(g);
			break;
		}
		
		case RELOAD_PAGE:
			Reload();
			break;

		case msg_ISO1:
		case msg_MacRoman:
		case msg_AutodetectJ:
		case msg_SJIS:
		case msg_EUC:
		case msg_ISO2:
		case msg_KOI8R:
		case msg_ISO5:
		case msg_ISO7:
		case msg_Unicode:
		case msg_UTF8:
		case msg_MSDOS866:
		case msg_WINDOWS1251:
			switch(msg->what) {
				case msg_ISO1:
					mEncoding = B_MS_WINDOWS_CONVERSION;
					break;
				case msg_MacRoman:
					mEncoding = B_MAC_ROMAN_CONVERSION;
					break;
				case msg_AutodetectJ:
					mEncoding = N_AUTOJ_CONVERSION;
					break;
				case msg_SJIS:
					mEncoding = B_SJIS_CONVERSION;
					break;
				case msg_EUC:
					mEncoding = B_EUC_CONVERSION;
					break;
				case msg_ISO2:
					mEncoding = B_ISO2_CONVERSION;
					break;
				case msg_KOI8R:
					mEncoding = B_KOI8R_CONVERSION;
					break;
				case msg_ISO5:
					mEncoding = B_ISO5_CONVERSION;
					break;
				case msg_ISO7:
					mEncoding = B_ISO7_CONVERSION;
					break;
				case msg_Unicode:
					mEncoding = B_UNICODE_CONVERSION;
					break;
				case msg_UTF8:
					mEncoding = N_NO_CONVERSION;
					break;
				case msg_MSDOS866:
					mEncoding = B_MS_DOS_866_CONVERSION;
					break;
				case msg_WINDOWS1251:
					mEncoding = B_MS_WINDOWS_1251_CONVERSION;
					break;
			}
			RelayoutView();
			gPreferences.ReplaceInt32("DefaultEncoding", mEncoding);
			break;

		case msg_RelayoutView:
			mOldFrame.Set(0,0,0,0);
			RelayoutView(false);
			break;

		case B_PASTE:
			break;

		case B_SELECT_ALL:
			if (mHTMLDoc != NULL) {
				DocAutolock lock(mHTMLDoc);
				int32 endOffset = mHTMLDoc->GetTextLength() - 1;
				if (endOffset > 0)
					mHTMLDoc->SelectText(0, endOffset);			
			}
			break;
		
		case msg_ResourceSwitched: {
			UResourceImp *oldResource = 0;
			msg->FindPointer("OldImp", (void **)&oldResource);
			if (oldResource == mNewDocResource)
				msg->FindPointer("NewImp", (void **)&mNewDocResource);
			break;
		}

		case msg_ResourceFlushed:
		case msg_ResourceChanged: {
			pprint("Get ResChg msg.  mNewDocRes = 0x%x", mNewDocResource);

			void *imp = NULL;
			if (msg->FindPointer("ResourceImp", &imp) == B_OK) {

				if (mNewDocResource) {
					StoreStatus status = mNewDocResource->GetStatus();
					SetProgress(status,0,0);
				}

				 if (mNewDocResource && imp == mNewDocResource) {
					mLastModified = mNewDocResource->GetLastModified();
				 	QualifyNewDoc();
				 } else if (msg->what == msg_ResourceFlushed && mBGSound && imp == mBGSound &&
							 mBGSound->GetContentType() &&
							(strcasecmp(mBGSound->GetContentType(), "audio/midi") == 0 ||
							 strcasecmp(mBGSound->GetContentType(), "audio/x-midi") == 0 ||
							 strcasecmp(mBGSound->GetContentType(), "audio/x-mid") == 0)) {
					pprint("Starting background sound file");
					entry_ref ref = UResourceCache::GetFileFromCache(mBGSound->GetCacheName());
					NetPositive::StartSong(&ref, mHTMLDoc->GetBGSoundLoops());
				 }
			}
			// Give it an extra kick in the teeth.  If the document has parsed extra-quick, it may not get
			// any resource changed notifications because it'll already be done by the time the worker thread
			// comes to life.
			mWorkerThread->PostMessage(msg_DoSomeWork);
			break;
		}
		
		case LOAD_IMAGES:
			if (mHTMLDoc) {
				DocAutolock lock(mHTMLDoc);
				mHTMLDoc->LoadImages();
				SetProgress(kLoadingImages,0,0);
				Invalidate();
			}
			if (!gRunningAsReplicant) {
				HTMLWindow *w = (HTMLWindow *)Window();
				w->EnableLoadImages(false);
			}
			break;
			
		case B_NETPOSITIVE_OPEN_URL: {
			printf("OpenURL\n"); fflush(stdout);
			const char *url = msg->FindString("url");
			const char *postData = msg->FindString("PostData");
			const char *target = msg->FindString("Target");
			const char *referrer = msg->FindString("Referrer");
#ifdef PLUGINS
			bool returnStream;
			if (msg->FindBool("ReturnStream", &returnStream) == B_OK && returnStream) {
				if (mHTMLDoc) {
					DocAutolock lock(mHTMLDoc);
					BString fullURL;				
					mHTMLDoc->ResolveURL(fullURL, url);
					msg->ReplaceString("url", fullURL.String());
				}
				thread_id tid = spawn_thread(CreateStreamIOThread, "Create BNetPositiveStreamIO", B_NORMAL_PRIORITY, Window()->DetachCurrentMessage());
				resume_thread(tid);
				NetPositive::AddThread(tid);
			} else
#endif
				OpenLink(url, target, postData, referrer);						
		}
		break;
		
#ifdef PLUGINS
		case B_NETPOSITIVE_STATUS_MESSAGE: {
			const char *status = msg->FindString("Message");
			if (status)
				SetProgressStr(status, false);
		}
#endif

		case B_COPY_TARGET:
			dynamic_cast<NetPositive*>(be_app)->HandleCopyTarget(msg);
			break;
			
		case B_NETPOSITIVE_SAVE_INSTANCE_DATA: {
			const char *url = msg->FindString("PageURL");
			const char *tag = msg->FindString("PluginTag");
			if (!url || !tag || !*url || !*tag)
				break;
			for (int32 i = 0; i < mLinkHistory->CountItems(); i++) {
				PageHistory *histItem = mLinkHistory->ItemAt(i);
				if (strcmp(histItem->URL(), url) != 0)
					continue;
				ExtraPageData *extra = histItem->GetExtra();
				if (!extra)
					continue;
				BList *list = extra->GetPluginData();
				if (!list) {
					list = new BList;
					extra->SetPluginData(list);
				}
				
				bool foundEntry = false;

				for (int32 j = 0; j < list->CountItems(); j++) {
					PluginData *data = (PluginData *)list->ItemAt(j);
					if (data->mParameters == tag) {
						if (!data->mData)
							data->mData = new BMessage;
						data->mData->MakeEmpty();
						msg->FindMessage("InstanceData", data->mData);
						foundEntry = true;
						break;
					}
				}
				if (!foundEntry) {
					// Didn't find a match, create one.
					PluginData *data = new PluginData;
					data->mParameters = tag;
					data->mData = new BMessage;
					msg->FindMessage("InstanceData", data->mData);
					list->AddItem(data);
				}
				break;
			}
			break;
		}

		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
HTMLView::ScrollTo(
	BPoint	where)
{
	BView::ScrollTo(where);
	if (mExtra != NULL)
		mExtra->SetScrollPos(where);
}


void HTMLView::DoWork(bool updateView)
{
	BString refresh;
	
	if (mLinkHistory == NULL) {
		mLinkHistory = ((NPBaseView *)Window()->FindView("NPBaseView"))->GetLinkHistory();
		if (mNewDocResource) {
			BString temp = mNewDocResource->GetURL();
			if (*mFragment) {
				temp += '#';
				temp += mFragment;
			}
			mLinkHistory->AddToHistory(temp.String(), NULL);
			SetExtra(mLinkHistory->GetCurrentHistory()->GetExtra());
		}
	}

//	Waiting for a new page to load

	if (mNewDocResource) {
		StoreStatus status = mNewDocResource->GetStatus();
		SetProgress(status,0,0);
	}

//	mHTMLDoc exists, manage updates

	if (mHTMLDoc == NULL){
		SetViewColor(0,0,0);
		return;
	}

	PageHistory *hist = mLinkHistory->GetCurrentHistory();

//	Display title when it arrives

	if (mTitle.Length() == 0) {
		DocAutolock lock(mHTMLDoc);
		mTitle = mHTMLDoc->GetTitle();
		if (mTitle.Length()) {
			// pjp - only set window title in the reall NetPositive app
			if (!gRunningAsReplicant && !mIsSubview && (!gWindowTitle || !*gWindowTitle)) {
				Window()->SetTitle(mTitle.String());
				if (hist)
					hist->SetTitle(mTitle.String());
			}
			if ((mShowTitleInToolbar == kShowTitle || (gPreferences.FindBool("ShowTitleInToolbar")) && mShowTitleInToolbar != kShowURL))
				SetURLText(mTitle);
		} else if (!gRunningAsReplicant && !mIsSubview && mHTMLDoc->ReadyForDisplay() && (!gWindowTitle || !*gWindowTitle)) {
			// A title will never show up, since the document has laid out its first spatial
			// glyph, and we don't have one.
			Window()->SetTitle(kApplicationName);
			mTitle = kApplicationName;
		}
		if (!mIsSubview && mTitle != mPreviousTitle) {
			HistoryFolder::GetHistoryFolder()->AddEntry(mDocumentURL.String(), mTitle.String());
			mPreviousTitle = mTitle;
		}
	}
	
	bool hasFrames = false;
	if (mFrames == NULL) {
		DocAutolock lock(mHTMLDoc);
		hasFrames = mHTMLDoc->GetFrameset(&mFrames);
	}
	if (hasFrames) {
		// With nested frame documents, don't add subdocuments' frame lists to the history.
		if (hist->GetFrameList() == NULL) {
			pprint("Frame list 0x%x arrived.  Adding to history.", mFrames);
			hist->SetFrameList(mFrames);
		}
		mFrames->Reference();
		SetScrolling(kNoScrolling);
		BAutolock lock(Window());
		UpdateScrollBars();
		BRect r = Frame();
		r.OffsetTo(0,0);
		SetupFrames(mFrames, r, 0);
#ifndef JAVASCRIPT
		DeleteDoc(false);
#endif
		//This is a one time Invalidate for the view underneath the framesets
		Invalidate();
		return;
	}
	else if (sOffscreenEnabled)
		SetViewColor(B_TRANSPARENT_32_BIT);

	const char *soundURL;
	if (!mBGSound && (bool)(soundURL = mHTMLDoc->GetBGSound()) && *soundURL) {
		pprint("mBGSound 0x%x  URL %s", mBGSound, mHTMLDoc->GetBGSound());
		BString msg;
		mBGSound = GetUResource(mConnectionMgr, soundURL,(long)this, msg, false, NULL, &mMessenger);
	}

	BWindow *window = Window();
	HTMLMessage m;
	float newHeight, docHeight;
	BRect r;
	{
		DocAndWindowAutolock lock(mHTMLDoc);
		docHeight = mHTMLDoc->GetHeight();
		
		r = window->Bounds();
		window->BeginViewTransaction();
		m = mHTMLDoc->Idle(&r);
		window->EndViewTransaction();
		newHeight = mHTMLDoc->GetHeight();
	}
		
	if (newHeight != docHeight)
		UpdateScrollBars();
		
//	Scroll to a fragment anchor if present

	if (mFragment[0]) {
		DocAutolock lock(mHTMLDoc);
		float v = mHTMLDoc->GetFragmentTop(mFragment);	// Scroll to top of screen
		if (v >= 0) {
			pprint("Scrolling.");
			MakeVisible(Bounds().left, v - 20);
			mScrollPos = BPoint(Bounds().left, v-20);
			mFragment[0] = 0;
			if (updateView)
				UpdateDocRect(r);
			return;
		} else {
			pprint("Deferring display.");
			// If the anchor is not present, then bail out now.  We want to defer document
			// display until the anchor arrives.
			return;
		}
	}
	

	if ((m == HM_UPDATE || m == HM_UPDATEANDIDLEAGAIN) && updateView && !mFragment[0]) {
		pprint("Updating due to idle.");
		UpdateDocRect(r);
	} else if (m == HM_IDLEAGAIN) {
		mWorkerThread->PostMessage(msg_RequestViewUpdate);
	} else
	
	// The refresh mechanism is often used with a short delay to redirect browsers when a site moves.
	// If this happens, we don't want to store the old site in the history.  If the refresh is less than
	// 15 seconds and the refresh URL is different from the page URL, put the refresh URL in the history.
	if (mHTMLDoc) {
		DocAutolock lock(mHTMLDoc);
		int32 refreshTime = mHTMLDoc->GetRefreshTime();
		const char *refreshURL = mHTMLDoc->GetStoredRefreshURL();
		PageHistory *hist = mLinkHistory->GetCurrentHistory();

		if (refreshTime != -1 && refreshTime < 15 && refreshURL && strcmp(refreshURL, hist->URL()) != 0) {
			hist->SetURL(refreshURL);
			hist->SetTarget(NULL);
			hist->SetFrameList(NULL);
			hist->GetExtra()->SetScrollPos(BPoint(0,0));
			hist->GetExtra()->SetUserValues(NULL);
			return;
		}
	}

//	Check for refresh

	DocAutolock lock(mHTMLDoc);
	bool hasRefresh = mHTMLDoc->GetRefreshURL(refresh);
	if (hasRefresh) {
		if (refresh.Length() && refresh != mDocumentURL) {
			// This page is redirecting to another page.  If we leave it in
			// the page history, when the user clicks the back button, it
			// will immediately redirect forward again.  Remove this page from
			// the history, as the user went straight to the target page.
			if (mHTMLDoc->GetRefreshTime() <= 0)
				mLinkHistory->RemoveCurrentPage();
		
			BMessage msg(HTML_MSG + HM_ANCHOR);
			msg.AddString("url", refresh.String());
			window->PostMessage(&msg, this);
		} else
			window->PostMessage(RELOAD_PAGE, this);
	} else if (mHTMLDoc->GetRefreshTime() != -1) {
		// We have a refresh URL, but its time hasn't come yet.
		// Request view updates until it's ready.
		mWorkerThread->PostMessage(msg_RequestViewUpdate);
	}

// Check for blink
	if (mHTMLDoc->NeedsBlink()) {
		bigtime_t theTime = system_time();
		if (theTime - mLastBlink > 200000) {
			mHTMLDoc->DoBlink();
			mLastBlink = theTime;
		}
		mWorkerThread->PostMessage(msg_RequestViewUpdate);
	}
}


void HTMLView::Draw(BRect updateRect)
{
	if (mHTMLDoc == NULL) {
		FillRect(updateRect, B_SOLID_LOW);
		return;
	}
	BRect r;
	r.top = updateRect.top;
	r.left = updateRect.left;
	r.bottom = updateRect.bottom+1;
	r.right = updateRect.right+1;
		
	BWindow *window = Window();
	BAutolock lock(window);
		
	BRect bounds = Bounds();
	
	BPoint lt = bounds.LeftTop();
	if (!mFragment[0] ||
		(mExtra == NULL || mExtra->ScrollPos() == lt)) {
		if (!mHTMLDoc->Lock()) {
			if (sOffscreenEnabled && mFrames == NULL)
				SetLowColor(255,255,255);
			FillRect(updateRect, B_SOLID_LOW);
			if (sOffscreenEnabled && mFrames == NULL)
				SetLowColor(B_TRANSPARENT_32_BIT);
			return;
		}
		
		bool isOffscreen = (sOffscreenEnabled && !IsPrinting() /* && !mOpenAsText */ && mFrames == NULL);
		BBitmap *offscreenBitmap = NULL;
		BView *offscreenView = NULL;
		BRect srcRect(updateRect);
		srcRect.OffsetTo(0,0);
		if (isOffscreen) {
			offscreenBitmap = RequestOffscreenBitmap(updateRect.Width(), updateRect.Height());
			if (offscreenBitmap) {
				offscreenBitmap->Lock();
				offscreenView = offscreenBitmap->ChildAt(0);
				if (!offscreenView) {
					offscreenBitmap->Unlock();
					OffscreenFinished(offscreenBitmap);
					offscreenBitmap = NULL;
				} else {	
					offscreenView->SetLowColor(LowColor());
					offscreenView->SetHighColor(HighColor());
					((BeDrawPort *)mHTMLDoc->GetDrawPort())->SetView(offscreenView);
					mHTMLDoc->GetDrawPort()->SetOffscreen(true);
					offscreenView->ScrollTo(updateRect.LeftTop());
				}
			}
		}
		mHTMLDoc->Draw(&r, !IsPrinting());		
		mHTMLDoc->Unlock();

		if (offscreenBitmap) {
			offscreenView->Sync();
			DrawBitmap(offscreenBitmap, srcRect, updateRect);
			((BeDrawPort *)mHTMLDoc->GetDrawPort())->SetView(this);
			mHTMLDoc->GetDrawPort()->SetOffscreen(false);
			offscreenBitmap->Unlock();
			OffscreenFinished(offscreenBitmap);
		}
	}
}

bool	HTMLView::sOffscreenEnabled;
BList	HTMLView::sFreeOffscreenViewList;
BList	HTMLView::sUsedOffscreenViewList;
int32	HTMLView::sOffscreenViewListSize;
TLocker	HTMLView::sOffscreenListLocker("Offscreen Lock");


void HTMLView::Init()
{
	BAutolock autolock(sOffscreenListLocker);
	int32 option = gPreferences.FindInt32("DrawOffscreen");
	if (option == offscreenNever)
		sOffscreenEnabled = false;
	else {
		system_info sysInfo;
		get_system_info(&sysInfo);
		// Smart option.  Enable offscreen drawing only if the CPU is 180 MHz
		// or greater, and there is at least 64 MB of RAM.  Set the value of
		// the preference to either on or off once we make the determination
		// so that the checkbox in the prefs will work properly.
		if ((option == offscreenSmart) && (sysInfo.cpu_clock_speed < 180000000 || sysInfo.max_pages < 16384)) {
			sOffscreenEnabled = false;
			gPreferences.ReplaceInt32("DrawOffscreen", offscreenNever);
		} else {
			gPreferences.ReplaceInt32("DrawOffscreen", offscreenAlways);
			sOffscreenEnabled = true;
			// For 64 MB machines, allocate a maximum of one offscreen bitmap.
			// If there is more memory, set the maximum to two.
			if (sysInfo.max_pages == 16384)
				sOffscreenViewListSize = 1;
			else
				sOffscreenViewListSize = 2;
		}
	}
}

void HTMLView::Cleanup()
{
	BAutolock autolock(sOffscreenListLocker);
	BBitmap *bitmap;
	do {
		bitmap = (BBitmap *)sFreeOffscreenViewList.ItemAt(0);
		if (bitmap) {
			bitmap->Lock();
			BView *view = bitmap->ChildAt(0);
			if (view) {
				bitmap->RemoveChild(view);
				delete view;
			}
			delete bitmap;
			sFreeOffscreenViewList.RemoveItem((int32)0);
		}
	} while (bitmap);
}

BBitmap *HTMLView::RequestOffscreenBitmap(float width, float height)
{
	// Reject the request if offscreen is disabled or if we aren't drawing at least 200 x 200 pixels.
	// Smaller drawing requests are likely just animated GIF frames, which don't need to be buffered.
	if (!sOffscreenEnabled || width * height < 40000.0)
		return NULL;
		
	BAutolock autolock(sOffscreenListLocker);
	
	BBitmap *bitmap = (BBitmap *)sFreeOffscreenViewList.ItemAt(0);
	if (bitmap) {
		sFreeOffscreenViewList.RemoveItem((int32)0);
		if (width > bitmap->Bounds().Width() ||
			height > bitmap->Bounds().Height()) {
			bitmap->Lock();
			BView *view = bitmap->ChildAt(0);
			bitmap->RemoveChild(view);
			BRect rect(0,0,MAX(width, bitmap->Bounds().Width()),MAX(height, bitmap->Bounds().Height()));
			delete bitmap;
			view->ResizeTo(rect.Width(), rect.Height());
			bitmap = new BBitmap(rect,NetPositive::MainScreenColorSpace(),TRUE);
			bitmap->AddChild(view);
		}
		sUsedOffscreenViewList.AddItem(bitmap);
		return bitmap;
	}	
	if (sUsedOffscreenViewList.CountItems() >= sOffscreenViewListSize)
		return NULL;
	
	BRect rect(0, 0, width, height);
	BView *view = new BView(rect,"off",B_FOLLOW_ALL,B_WILL_DRAW);
	bitmap = new BBitmap(rect,NetPositive::MainScreenColorSpace(),TRUE);
	bitmap->AddChild(view);
	sUsedOffscreenViewList.AddItem(bitmap);

	return bitmap;
}

void HTMLView::OffscreenFinished(BBitmap *bitmap)
{
	if (!bitmap)
		return;
	BAutolock autolock(sOffscreenListLocker);
	sUsedOffscreenViewList.RemoveItem(bitmap);
	sFreeOffscreenViewList.AddItem(bitmap);	
}


//	Get the part of the HTML view that is visible

void HTMLView::UpdateDocRect(BRect& r)
{
	r.right = MIN(r.right, 0xfffffff);
	r.bottom = MIN(r.bottom, 0xfffffff);
	if (r.top < 0 || r.left < 0)
		return;


	BAutolock lock(Window());

	pprint("Processing Update: %f,%f,%f,%f.",r.left,r.top,r.right,r.bottom);
	if (mFirstUpdate) {
		Invalidate();
		mFirstUpdate = false;
	} else
		Invalidate(r);
}

//	Stop loading images, generate update if req

void HTMLView::Stop()
{
	if (mHTMLDoc == NULL)
		return;
		
	BRect r;
	BWindow *window = Window();
	{
		BAutolock lock(window);
		r = window->Bounds();
	}
	
	mConnectionMgr->KillAllConnections();
	DocAutolock lock(mHTMLDoc);
	HTMLMessage m = mHTMLDoc->Stop(&r);
	if (m == HM_UPDATE)
		UpdateDocRect(r);
}

//	Track the mouse! Update the url so people can figure out where they are going....

void HTMLView::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
	BView::MouseMoved(where,code,a_message);
	if (mHTMLDoc == NULL) return;
	
	BString url;
	if (code == B_INSIDE_VIEW) {
		DocAndWindowAutolock lock(mHTMLDoc);
		HTMLMessage m = mHTMLDoc->MouseMove(where.x,where.y);
		switch (m) {
			case HM_ONTOANCHOR: {					// Mouse moved onto an anchor
				bool hasURL = mHTMLDoc->MouseURL(url);
				if (hasURL) {
					mMouseIsOverLink = true;
					SetProgressStr(url.String(), false);
					break;
				} // else fall through and reset string.
			}
			case HM_OFFANCHOR:					// Mouse moved off an anchor
				mMouseIsOverLink = false;
				RestoreProgressStr();
				break;
			
			default:
				break;
			
		}
	}
	ChangeCursor();
}


void HTMLView::ChangeCursor()
{
	if (gPreferences.FindBool("BusyCursor") && GetStatus() != kComplete && GetStatus() != kError) {
		bigtime_t now = system_time();
		if (now - mLastPulse > (ONE_SECOND / kNumCursorPhases)) {
			if (mMouseIsOverLink)
				((NetPositive *)be_app)->SetCursor(gBusyLinkCursor[mCursorPhase]);
			else
				((NetPositive *)be_app)->SetCursor(gBusyCursor[mCursorPhase]);
			if (++mCursorPhase >= kNumCursorPhases)
				mCursorPhase = 0;
			mLastPulse = now;
			mCurrentCursor = 1;
		}
	} else {
		mCursorPhase = 0;
		if (mMouseIsOverLink) {
			if (mCurrentCursor != 2) {
				((NetPositive *)be_app)->SetCursor(gLinkCursor);
				mCurrentCursor = 2;
			}
		} else {
			if (mCurrentCursor != 3) {
				((NetPositive *)be_app)->SetCursor(B_HAND_CURSOR);
				mCurrentCursor = 3;
			}
		}
	}
}

void HTMLView::Pulse()
{
	if (gPreferences.FindBool("BusyCursor") /*&& GetStatus() != kComplete && GetStatus() != kError*/)
		ChangeCursor();
}

bool HTMLView::HandleFragment(const BString& url)
{
	bool retval = false;
	BString temp = url;
	char *f;
	if ((bool)(f = strrchr(temp.String(),'#'))) {
		temp.Truncate(f - temp.String());
	}	

	if (mDocumentURL.Length() > 0 && (temp.Length() == 0 || (temp == mDocumentURL && !mIsSubview))) {
		f = strchr(url.String(),'#');
		if (f) {
			CleanName(f + 1, mFragment);
			pprint("Local Fragment: %s",mFragment);
			BString temp;
			if (url[0] == '#')
				temp = mDocumentURL;
			temp += url;
			mLinkHistory->AddToHistory(temp.String(), NULL);
			SetExtra(mLinkHistory->GetCurrentHistory()->GetExtra());
			retval = true;
		}
		SetURLText(mDocumentURL, true);
		mWorkerThread->PostMessage(msg_ViewUpdate);
		mStopEnabled = false;
		UpdateURLView();
	}
	return retval;
}

void HTMLView::RewindHistory()
{
	if (mLinkHistory)
		mLinkHistory->Rewind();
}

void HTMLView::UpdateURLView()
{
	BWindow *window = Window();
	
	if (!mStopEnabled && mOriginatingMessage) {
		BMessage reply(B_NETPOSITIVE_OPEN_URL);
		mOriginatingMessage->SendReply(&reply);		
		
		delete mOriginatingMessage;
		mOriginatingMessage = 0;
	}
	
	URLView *urlView = (URLView *)window->FindView("URLView");
	if (!gRunningAsReplicant && !mOpenAsText) {
		if (window) {
			((HTMLWindow *)window)->EnableBackForw(mLinkHistory->CanGoBack(), mLinkHistory->CanGoForward());
			((HTMLWindow *)window)->EnableLoadImages(!gPreferences.FindBool("ShowImages") || !gPreferences.FindBool("ShowBGImages"));
		}
	}
	if (urlView) {
		urlView->SetButtonEnabled(NP_BACKBUTTON, mLinkHistory->CanGoBack() != 0);
		urlView->SetButtonEnabled(NP_FORWBUTTON, mLinkHistory->CanGoForward() != 0);
		urlView->SetButtonEnabled(NP_STOPBUTTON, mStopEnabled);

		if (mHTMLDoc && mHTMLDoc->ShouldShowToolbar() != kHideToolbar && urlView->IsHidden()) {
			float urlViewHeight = urlView->Frame().Height();
			BView *htmlAreaView = Parent();

			urlView->Show();

			window->BeginViewTransaction();
			htmlAreaView->ResizeTo(htmlAreaView->Frame().Width(), htmlAreaView->Frame().Height() - urlViewHeight);
			if (!gPreferences.FindBool("URLViewOnBottom"))
				htmlAreaView->MoveTo(0,urlViewHeight + 1);
			window->EndViewTransaction();
			
//			if (progressView)
//				progressView->Show();
		} else if (mHTMLDoc && mHTMLDoc->ShouldShowToolbar() == kHideToolbar && !urlView->IsHidden()) {
			float urlViewHeight = urlView->Frame().Height();
			BView *htmlAreaView = Parent();

			urlView->Hide();

			window->BeginViewTransaction();
			htmlAreaView->ResizeTo(htmlAreaView->Frame().Width(), htmlAreaView->Frame().Height() + urlViewHeight);
			if (!gPreferences.FindBool("URLViewOnBottom"))
				htmlAreaView->MoveTo(0,0);
			window->EndViewTransaction();

//			if (progressView)
//				progressView->Hide();
		}
	}

}
//	Someone clicked in this link, open it

void HTMLView::OpenLink(const BString& url, const char *target, const char *formData, const char *referrer, int showTitleInToolbar, BMessage *originatingMessage)
{
	if (LaunchExternalURLHandler(url))
		return;
		
	pprint("HTMLView::OpenLink: %s  target %s",url.String(), target);
	if (url.Length() == 0)
		return;
		
	mFragment[0] = 0;

	if ((!target || !(*target)) && HandleFragment(url))
		return;

	if (mOriginatingMessage)
		delete mOriginatingMessage;
	
	mOriginatingMessage = originatingMessage;

	if ((!target || !(*target)) && showTitleInToolbar != kShowTitleUnspecified && gPreferences.FindBool("ShowTitleInToolbar")) {
		mShowTitleInToolbar = showTitleInToolbar;
		URLView *urlView = (URLView *)Window()->FindView("URLView");
		BTextControl *control = dynamic_cast<BTextControl *>(urlView->FindView("url"));
		if (control) {
			control->TextView()->MakeEditable(showTitleInToolbar == kShowURL);
			control->TextView()->MakeSelectable(showTitleInToolbar == kShowURL);
		}
	}		

//	Load resource

	BMessage msg(HTML_MSG + HM_ANCHOR);
	msg.AddString("url",url.String());
	msg.AddInt32("showTitleInToolbar", showTitleInToolbar);
	if (referrer && *referrer && strncmp(referrer, "http:", 5) == 0)
		msg.AddString("referrer", referrer);
	if (formData && *formData) {
		msg.AddBool("isForm", true);
		msg.AddString("post", formData);
	}
	BView *targetView = this;
	const char *targetString = NULL;
	
	if (target) {
		if ((target[0] == '\0') || (strcmp(target, "_self") == 0)) {
			targetView = this;
		} else if (strcmp(target, "_blank") == 0) {
			((NetPositive *)be_app)->NewWindowFromURL(BString(url), mEncoding, NULL, false, true, true, NULL, true, showTitleInToolbar);
			return;
		} else if (strcmp(target, "_parent") == 0) {
			targetString = "_self";
			targetView = Parent();
		} else {
			if (strcmp(target, "_top") != 0)
				targetString = target;
			else
				targetString = kHTMLViewName;
			targetView = Parent();
		}
	}

	if (targetString)
		msg.AddString("target", targetString);

	Window()->PostMessage(&msg,targetView);
}


void
HTMLView::CopyLink(
	const BString	&url)
{
	be_clipboard->Lock();
	be_clipboard->Clear();
	
	BMessage *clip_msg = be_clipboard->Data();
	clip_msg->AddData("text/plain", B_MIME_TYPE, url.String(), url.Length());
	
	be_clipboard->Commit();
	be_clipboard->Unlock();
}

//====================================================================

BMenuItem *AddMenuItem(BMenu *menu, const char *title, int32 command, const char equivalent, uint32 modifiers, bool enabled)
{
	BMenuItem *item = new BMenuItem(title, new BMessage(command), equivalent, modifiers);
	menu->AddItem(item);
	if (!enabled)
		item->SetEnabled(enabled);
	return item;
}

void AddMenuItems(BMenu *menu, const StaticMenuItem *items)
{
	bool allowShortcuts = gPreferences.FindBool("AllowKBShortcuts");

	bool fullScreen = false;
	if (!gRunningAsReplicant && gPreferences.FindBool("DesktopMode"))
		for (int32 i = 0; i < be_app->CountWindows(); i++) {
			HTMLWindow *window = dynamic_cast<HTMLWindow*>(be_app->WindowAt(i));
			if (window && window->IsFullScreen()) {
				fullScreen = true;
				break;
			}
		}

	while (items->title && *items->title) {
		if (items->enabled == 3 && !fullScreen) {
			items++;
			continue;
		}

		if (items->title[0] == '-')
			menu->AddSeparatorItem();
		else {
			bool enabled = true;
			
			switch(items->enabled) {
				case 0:	enabled = false;break;
				case 1: enabled = true;break;
				case 2: enabled = !gRunningAsReplicant;break;
			}
			AddMenuItem(menu, items->title, items->cmd, allowShortcuts ? items->equivalent : 0, allowShortcuts ? items->modifiers : 0, enabled);
		}
		items++;
	}
}

//===================================================================================
//	Track a click

void HTMLView::MouseDown(BPoint point)
{
	HTMLMessage message;
	HTMLMessage m = HM_NOTHING;
	ulong buttons;
	uint32 what = 0;
	BString url;
	BString target;
	int showTitleInToolbar = kShowTitleUnspecified;
	
	
	MakeFocus();
	
	BPoint oldPoint;
	GetMouse(&oldPoint,&buttons);
	BPoint previousPoint(oldPoint);
	bool isRightClick = buttons & B_SECONDARY_MOUSE_BUTTON;
	bool oldRightClick = isRightClick;
	bool isMiddleClick = buttons & B_TERTIARY_MOUSE_BUTTON;
	bool mouseMovedALot = false;
	bool allowRightClick = gPreferences.FindBool("AllowRightClick");
	BRect frame = Frame();
	float docHeight = 0;
	float docWidth = 0;
	AnchorGlyph *anchorGlyph = 0;

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) && mOpenAsText)
		return;
		
	if (mHTMLDoc) {
		DocAutolock lock(mHTMLDoc);
		message = mHTMLDoc->MouseDown(point.x,point.y, isRightClick);
		docHeight = mHTMLDoc->GetHeight();
		docWidth = mHTMLDoc->GetUsedWidth() + 8;
	} else
		message = HM_TRACKSELECTION;

	
//	If a selection was made, change focus

	if (message == HM_NOTHING) {
	} else {
		bigtime_t clickTime = system_time();
		do {
			GetMouse(&point,&buttons);
			if (mHTMLDoc) {
				DocAutolock lock(mHTMLDoc);
				m = mHTMLDoc->MouseMove(point.x,point.y);
			}

			if (buttons) {
					mouseMovedALot = mouseMovedALot || fabs(oldPoint.x - point.x) > 5.0 || fabs(oldPoint.y - point.y) > 5.0;
					isRightClick = allowRightClick ? (buttons & B_SECONDARY_MOUSE_BUTTON) || 
								   (((clickTime + 1000000) < system_time()) && (fabs(oldPoint.x - point.x) < 6.0) && (fabs(oldPoint.y - point.y) < 6.0) &&
									!mouseMovedALot) : false;
					if (isRightClick != oldRightClick) {
						oldRightClick = isRightClick;
						if (mHTMLDoc) {
							DocAutolock lock(mHTMLDoc);
							message = mHTMLDoc->MouseDown(point.x,point.y, isRightClick);
						}
					}
					
					if (mHTMLDoc && mouseMovedALot && !isRightClick && !isMiddleClick && (message == HM_TRACKANCHOR || message == HM_TRACKIMG_OR_ANC)) {
						BRect rect(0,0,0,0);
						DocAutolock lock(mHTMLDoc);
						mHTMLDoc->MouseURL(url, false, &rect);
						if (rect.Height() <= 0){
							rect.top = point.y - 16;
							rect.bottom = point.y + 16;
						}
						if (rect.Width() <= 0){
							rect.left = point.x - 16;
							rect.right = point.x + 16;
						}
						if (url.Length()) {
							NetPositive::DragLink(url.String(), rect, this, NULL, BPoint(0,0));
							// Tell the doc to stop tracking the mouse.
							DocAutolock lock(mHTMLDoc);
							mHTMLDoc->MouseUp(point.x,point.y);
							mHTMLDoc->SelectText(0,0);
							return;
						}
					}
					
		//			After a second, or if the rightmouse button is down, pop up a menu
		

					// If the user middle-clicks in a non-anchor part of the document,
					// then allow the user to scroll the document by dragging the mouse.
					// We'll take over control of the mouse here.
					if (mHTMLDoc && what == 0 && message == HM_TRACKSELECTION && isMiddleClick) {
						{
							// Tell the doc to stop tracking the mouse.
							DocAutolock lock(mHTMLDoc);
							mHTMLDoc->MouseUp(point.x,point.y);
							mHTMLDoc->SelectText(0,0);
						}
						
						BPoint newScrollPos, topLeft;
						float deltax, deltay, adeltax = 0, adeltay = 0;
						
						while (buttons) {
							topLeft = Bounds().LeftTop();

							// If the user moved the mouse a lot in one axis and not a lot
							// in the other, then pin the movement to one axis.
							if (adeltax >= 5 && adeltay <= adeltax / 2)
								adeltay = deltay = 0;
							if (adeltay >= 5 && adeltax <= adeltay / 2)
								adeltax = deltax = 0;
							
							// If the user moves the mouse quickly, do accelerated scrolling.
							deltax = previousPoint.x - point.x;
							deltay = previousPoint.y - point.y;
							adeltax = fabs(deltax);
							adeltay = fabs(deltay);
							if (adeltax >= 2.7) deltax *= log(adeltax);
							if (adeltay >= 2.7) deltay *= log(adeltay);
							
							newScrollPos.x = topLeft.x + deltax;
							newScrollPos.y = topLeft.y + deltay;
							
							if (!ScrollBar(B_HORIZONTAL))
								newScrollPos.x = topLeft.x;
							if (!ScrollBar(B_VERTICAL))
								newScrollPos.y = topLeft.y;
								
							newScrollPos.x = MIN(newScrollPos.x, docWidth - frame.Width() - 8);
							newScrollPos.x = MAX(0, newScrollPos.x);
							newScrollPos.y = MIN(newScrollPos.y, docHeight - frame.Height());
							newScrollPos.y = MAX(0, newScrollPos.y);
							
							if (newScrollPos != topLeft) {
								ScrollTo(newScrollPos);
								Window()->UpdateIfNeeded();
							}
	
							// Recalculate the previous point since the coordinate system has changed.
							previousPoint = point + (newScrollPos - topLeft);
							snooze(25000);
							GetMouse(&point, &buttons);
						}
						return;
					}
					
					if (what == 0 && (message == HM_TRACKSELECTION || message == HM_TRACKIMAGE) &&
						isRightClick) {
		
						// Untrack before showing popup
						if (mHTMLDoc) {
							DocAutolock lock(mHTMLDoc);
							mHTMLDoc->MouseUp(point.x,point.y);
						}
						
						ConvertToScreen(&point);
						bool fullScreen = false;
						if (!gRunningAsReplicant && gPreferences.FindBool("DesktopMode"))
							for (int32 i = 0; i < be_app->CountWindows(); i++) {
								HTMLWindow *window = dynamic_cast<HTMLWindow*>(be_app->WindowAt(i));
								if (window && window->IsFullScreen()) {
									fullScreen = true;
									break;
								}
							}

						if (mHTMLDoc && message == HM_TRACKIMAGE) {
							DocAutolock lock(mHTMLDoc);
							mHTMLDoc->MouseURL(url);			// Get the anchor, mouse will track off it
						}
						
						BMenuItem	*item;
						BPopUpMenu	*menu = new BPopUpMenu("Popup");	
						BuildMenu(menu, "RIGHT_CLICK", fullScreen, message == HM_TRACKIMAGE, false, mIsSubview);
						BMenu *bookmarkMenu = 0;
						BMenuItem *bookmarkItem = menu->FindItem(BOOKMARK_MENU);
						if (bookmarkItem) {
							int32 index = menu->IndexOf(bookmarkItem);
							menu->RemoveItem(index);
							bookmarkMenu = new BMenu(kBookmarksSubmenuTitle);
							BookmarkFolder::GetBookmarkFolder()->AddMenu(bookmarkMenu);
							menu->AddItem(bookmarkMenu, index);							
						}
	
						BRect rect(point, point);
						rect.InsetBy(-2,-2);
						
						item = menu->Go(point, false, false, rect, false);
						
						if (bookmarkMenu)
							BookmarkFolder::GetBookmarkFolder()->RemoveMenu(bookmarkMenu);

						if (item && item->Message()) {
							what = item->Message()->what;	// Menu selected something
						} else
							url = "";

						if (what == HTML_OPEN_LINK && mDocumentURL.Length()) {
							url = mDocumentURL;
							target = "_top";
						}
						
						if (what == (int32)OPEN_HIST_BOOKMARK) {
							what = HTML_OPEN_LINK;
							const char *bookmarkURL = item->Message()->FindString("url");
							if (bookmarkURL)
								url = bookmarkURL;
						}
						
						if (what == HTML_FILTER_FRAME_THIS_SITE || what == HTML_SHOW_FILTERS)
							url = mDocumentURL;
						
						if(what == HTML_OPEN_IMG || what == HTML_OPEN_IMG_IN_NEW){
							DocAutolock lock(mHTMLDoc);
							mHTMLDoc->MouseURL(url, true);
						}
												
						delete menu;
					}
					if (mHTMLDoc && what == 0 && 
						((message == HM_TRACKANCHOR && m == HM_TRACKANCHOR) ||
						 (message == HM_TRACKIMG_OR_ANC && m == HM_TRACKIMG_OR_ANC))) {
						if (isMiddleClick) {
							DocAutolock lock(mHTMLDoc);
							mHTMLDoc->MouseURL(url);			// Get the anchor, mouse will track off it
							
							what = HTML_OPEN_LINK_IN_NEW;
						}
						if (isRightClick) {
						
							DocAutolock lock(mHTMLDoc);
							mHTMLDoc->MouseURL(url);			// Get the anchor, mouse will track off it
		
							bool fullScreen = false;
							if (!gRunningAsReplicant && gPreferences.FindBool("DesktopMode"))
								for (int32 i = 0; i < be_app->CountWindows(); i++) {
									HTMLWindow *window = dynamic_cast<HTMLWindow*>(be_app->WindowAt(i));
									if (window && window->IsFullScreen()) {
										fullScreen = true;
										break;
									}
								}
	
							BPopUpMenu* menu = new BPopUpMenu("Popup");	
							BMenuItem* item;
							BuildMenu(menu, "RIGHT_CLICK", fullScreen, message == HM_TRACKIMG_OR_ANC, true, mIsSubview);
							BMenu *bookmarkMenu = 0;
							BMenuItem *bookmarkItem = menu->FindItem(BOOKMARK_MENU);
							if (bookmarkItem) {
								int32 index = menu->IndexOf(bookmarkItem);
								menu->RemoveItem(index);
								bookmarkMenu = new BMenu(kBookmarksSubmenuTitle);
								BookmarkFolder::GetBookmarkFolder()->AddMenu(bookmarkMenu);
								menu->AddItem(bookmarkMenu, index);							
							}
		
							DocAutolock lock2(mHTMLDoc);
							mHTMLDoc->MouseUp(point.x,point.y);	// Untrack before showing popup
							
							ConvertToScreen(&point);
	
							BRect rect(point, point);
							rect.InsetBy(-2,-2);
							
							item = menu->Go(point, false, false, rect, false);
							
							if (bookmarkMenu)
								BookmarkFolder::GetBookmarkFolder()->RemoveMenu(bookmarkMenu);
	
							what = 0;
							if (item) {
								what = item->Message()->what;	// Menu selected something
								DocAutolock lock(mHTMLDoc);
								switch (what) {
									case HTML_FILTER_IMAGE_ALL_SITES:
									case HTML_FILTER_IMAGE_THIS_SITE:
										mHTMLDoc->MouseURL(url, true);
										break;
									case HTML_OPEN_IMG:
									case HTML_OPEN_IMG_IN_NEW:
										mHTMLDoc->MouseURL(url, true);
										break;
									case HTML_FILTER_FRAME_THIS_SITE:
										url = mDocumentURL;
										break;
								}
							} else
								url = "";
							delete menu;
						}
					}
										
		//			Autoscroll selection
		
					if (m == HM_TRACKSELECTION && !isRightClick && !isMiddleClick) {
						BScrollBar *s;
						float small,large;
						BRect bounds;
						{
							BAutolock lock(Window());
							bounds = Bounds();
						}
						if (point.y < bounds.top || point.y >= bounds.bottom) {
							s = ScrollBar(B_VERTICAL);
							if (s != NULL) {
								s->GetSteps(&small,&large);
								if (point.y < bounds.top)
									small = -small;
								s->SetValue(s->Value() + small);
							}
						}
						if (point.x < bounds.left || point.x >= bounds.right) {
							s = ScrollBar(B_HORIZONTAL);
							if (s != NULL) {
								s->GetSteps(&small,&large);
								if (point.x < bounds.left)
									small = -small;
								s->SetValue(s->Value() + small);
							}
						}
						Window()->UpdateIfNeeded();
					}
//				}					
				previousPoint = point;
				snooze(25000);
			}
		} while (buttons);
		
		if (mHTMLDoc) {
			DocAutolock lock(mHTMLDoc);
			if (!isRightClick && mHTMLDoc->MouseUp(point.x,point.y) == HM_ANCHOR) {
				anchorGlyph = mHTMLDoc->GetAnchor(url, target, showTitleInToolbar);
#ifdef JAVASCRIPT
				if (anchorGlyph->ExecuteOnClickScript()) {
					mHTMLDoc->Unlock();
					return;
				}
#endif
			}
		}
	}
	
//	An anchor may have been selected

	if (url.Length()) {
		pprintBig("Selected anchor: %s,  target: %s",url.String(), target.String());
		switch (what) {
			case 0:
			case HTML_OPEN_LINK:
				OpenLink(url, target.String(), NULL, mDocumentURL.String(),
						 (anchorGlyph && mDocumentURL.Compare("file:/", 6) == 0) ? showTitleInToolbar : kShowTitleUnspecified);
				break;
				
			case HTML_OPEN_LINK_IN_NEW:
			case HTML_OPEN_IMG_IN_NEW: {
				BString fullURL;
				if (url[0] == '#')
					fullURL = mDocumentURL;
				fullURL += url;
				
				((NetPositive *)be_app)->NewWindowFromURL(fullURL, mEncoding, NULL, false, true, true, NULL, true, (anchorGlyph && mDocumentURL.Compare("file:/", 6) == 0) ? showTitleInToolbar : kShowTitleUnspecified);
				if (!gPreferences.FindBool("NewWindFromMouseInFront"))
					Window()->Activate();
				break;
			}
				
			case HTML_OPEN_IMG:
			case HTML_SAVE_LINK:
				DownloadManager::DownloadFile(url.String(), false, true);
				break;
				
			case HTML_ADD_BOOKMARK:
				BookmarkFolder::GetBookmarkFolder()->AddEntry(url,NULL);
				break;

			case HTML_COPY_LINK:
				CopyLink(url);				
				break;
				
			case HTML_RELOAD_FRAME:
				Reload();
				break;
				
#ifdef ADFILTER
			case HTML_FILTER_IMAGE_ALL_SITES:
			case HTML_FILTER_IMAGE_THIS_SITE:
			case HTML_FILTER_FRAME_THIS_SITE:
			{
				DocAutolock lock(mHTMLDoc);
				URLParser *parser = mHTMLDoc->GetBase();
				BString urlBase;
				parser->WriteURL(urlBase, true);
				const char *imgurl = url.String();
				pprint("URL = %s  Base = %s", imgurl, urlBase.String());
				if (strncmp(urlBase.String(), imgurl, urlBase.Length()) == 0)
					imgurl += urlBase.Length();
				while (*imgurl == '/')
					imgurl++;
				pprint("Final result = %s", imgurl);
				
				const char *site;
				if (what == HTML_FILTER_IMAGE_ALL_SITES || !parser->HostName())
					site = "*";
				else
					site = parser->HostName();
				const char *tag;
				if (what == HTML_FILTER_FRAME_THIS_SITE) {
					tag = "FRAME";
					imgurl = mDocumentURL.String();
				} else
					tag = "IMG";
				AdFilter::AddNewFilter(site, imgurl, tag, "SRC", kAdFilterActionDeleteTag);
				break;
			}
							
			case HTML_SHOW_FILTERS:
				AdFilter::ShowAdFilters();
				break;
#endif
				
			default: {
				BMessage msg(what);
				Window()->PostMessage(&msg, this);
			}			

		}
	} else {
		BMessage msg(what);
		Window()->PostMessage(&msg, this);
	}
}

void HTMLView::KeyDown(const char *bytes, int32 numBytes)
{
	if (bytes[0] == B_LEFT_ARROW || bytes[0] == B_RIGHT_ARROW) {
		BScrollBar* s = ScrollBar(B_HORIZONTAL);
		if (s) {
			float  small,large;
			s->GetSteps(&small,&large);
			
			if (bytes[0] == B_LEFT_ARROW)
				s->SetValue(s->Value() - small);
			else
				s->SetValue(s->Value() + small);
			return;
		}
	}
	
	BScrollBar* s = ScrollBar(B_VERTICAL);
	if (s == NULL) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	float  small,large;
	s->GetSteps(&small,&large);

	switch (bytes[0]) {
	
//		Vertical scrolling
			
		case B_UP_ARROW:		
			s->SetValue(s->Value() - small);
			break;
			
		case B_DOWN_ARROW:
			s->SetValue(s->Value() + small);
			break;

		case B_HOME:
			s->SetValue(0);
			break;
			
		case B_END:
			s->GetRange(&small,&large);
			s->SetValue(large);
			break;
			
		case B_PAGE_UP:
		case B_BACKSPACE:
			s->SetValue(s->Value() - large);
			break;
			
		case B_PAGE_DOWN:
		case B_SPACE:
			s->SetValue(s->Value() + large);
			break;
		
		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}


void HTMLView::FrameResized(float, float)
{
	RelayoutView(false);
}

void
HTMLView::RelayoutView(bool reparse)
{
	BRect frame;
	{
		BAutolock lock(Window());
		frame = Frame();
	}
	
	if (mOldFrame == frame && !reparse)
		return;
	else
		mOldFrame = frame;
	if (reparse) {
		if (mHTMLDoc == NULL)
			return;
		DocAutolock lock(mHTMLDoc);
		UResourceImp *res = mHTMLDoc->GetResource();
		if (res == NULL)
			return;
	
//		UResourceImp* imp = res->GetImp();
//		UResource* resource = new UResource(imp);
		res->RefCount(1);
		NewHTMLDoc(res, mOpenAsText, true);
		res->RefCount(-1);
	} else if (mFrames && !mHTMLDoc) {
		RealRelayoutView();
	} else {
		BMessage msg(msg_RelayoutView);
		mWorkerThread->PostMessage(&msg);
	}
}

void HTMLView::SetPrintingMode(bool on)
{
	if (mHTMLDoc) {
		mHTMLDoc->GetDrawPort()->SetPrintingMode(true);
		DocAutolock lock(mHTMLDoc);
		mHTMLDoc->SpacingChanged();
	}
	if (mFrames) {
		for (int32 i = 0; i < CountChildren(); i++) {
			HTMLView *view = dynamic_cast<HTMLView*>(ChildAt(i));
			if (view)
				view->SetPrintingMode(on);
		}
	}
}

void HTMLView::RealRelayoutView(float width, bool invalidate)
{
	BWindow *window = Window();
	if (!window)
		return;
	BAutolock lock(window);
		
	window->BeginViewTransaction();
	
	BRect frame = Frame();
	pprint("RelayoutView 0x%x.  (%f, %f, %f, %f)", this, frame.top, frame.left, frame.bottom, frame.right);
	BRect bounds = Bounds();
	if (mHTMLDoc) {
		DocAutolock lock(mHTMLDoc);
		mHTMLDoc->Layout(width < 0 ? bounds.Width()/*-16*/ : width);
	}
#ifdef JAVASCRIPT
	if (mFrames)
#else
	if (mFrames && mHTMLDoc == NULL)
#endif
	{
		BRect r = frame;
		r.OffsetTo(0,0);
		if (width > 0)
			r.right = width;
		RelayoutFrames(mFrames, r);
	}
	DoWork(false);
	if (invalidate) {
		Invalidate(bounds);
		mScrollPos.x = -1;
		mScrollPos.y = -1;
		UpdateScrollBars();
	}
	window->EndViewTransaction();
}

void HTMLView::RelayoutFrames(FrameList *frames, BRect bounds)
{
	FrameItem	*firstFrame = (FrameItem *)frames->First();
	int32		numFrames = firstFrame->Count();
	BList		viewBounds;

	for (int32 i = 0; i < numFrames; i++) 
		viewBounds.AddItem(new BRect(bounds));
	SetupFrameRects(frames, bounds, &viewBounds);
	
	for (FrameItem *frame = firstFrame; frame != NULL; frame = (FrameItem *)frame->Next()) {
		BRect *curViewBound = (BRect *)viewBounds.RemoveItem((int32)0);
		HTMLView *view = NULL;
		view = frame->View();
		if (view == NULL && frame->Frames() && view != this)
			RelayoutFrames(frame->Frames(), *curViewBound);
		else if (view != NULL) {
			pprint("Setting frame 0x%x (%s) to (%f, %f, %f, %f)", view, frame->Name(), curViewBound->top, curViewBound->left, curViewBound->bottom, curViewBound->right);
			view->ResetScrollBars();
			view->MoveTo(curViewBound->left, curViewBound->top);
			view->ResizeTo(curViewBound->Width(), curViewBound->Height());
			view->ScrollTo(BPoint(0,0));
			view->RelayoutView(false);
			view->UpdateScrollBars();
		}
	}
}

void
HTMLView::SetSubviewFont(
	Style	inStyle,
	BView*	view)
{
	float baseline;
	CachedFont *font = ((BeDrawPort *)mHTMLDoc->GetDrawPort())->GetFontFromStyle(inStyle, baseline);
	font->SetViewFont(view, false);
}

void
HTMLView::SetupFrames(
	FrameList			*frames,
	BRect				bounds,
	int32				num,
	bool				forceCache)
{
	if ((frames == NULL) || (frames->First() == NULL))
		return;

	if (mFrames == NULL) {
		pprint("SetupFrames is comissioning frames for 0x%x", this);
		mFrames = frames;
		mFrames->Reference();
	}
	FrameItem	*firstFrame = (FrameItem *)frames->First();
	int32		numFrames = firstFrame->Count();
	BList		viewBounds;

	//used to remove frames that weren't defined here, but to comply with other
	//browsiers we now leave the space blank with the frame color showing through

	for (int32 i = 0; i < numFrames; i++) 
		viewBounds.AddItem(new BRect(bounds));

	SetupFrameRects(frames, bounds, &viewBounds);	
	SetViewColor(0,0,0);
	for (FrameItem *frame = firstFrame; frame != NULL; frame = (FrameItem *)frame->Next()) {
		BRect *curViewBound = (BRect *)viewBounds.RemoveItem((int32)0);

		const char *frameURL = frame->URL(); 
		if (frameURL == NULL) {
			SetupFrames(frame->Frames(), *curViewBound, num, forceCache);
		} else {
			if(mFrameDepth < 10){ //this prevents framesets going too deep.  evil.
				HTMLView *view = new HTMLView(*curViewBound, frame->Name(), NULL, mLinkHistory, false, mEncoding, true, optionalVScroller, mHTMLDoc);
				pprint("Parent frame %x creating child frame %x (%f, %f, %f, %f)", this, view, curViewBound->top, curViewBound->left, curViewBound->bottom, curViewBound->right);
				view->SetFrameDepth(mFrameDepth + 1);
				frame->SetView(view);
				AddChild(view);
				view->SetFrameNum(num);
				view->SetScrolling(frame->Scrolling());
				view->SetExtra(frame->GetExtra());
				BString url(frameURL);
				view->NewHTMLDocFromURL(url, false, NULL, forceCache, false);
				view->mMarginWidth = frame->MarginWidth();
				view->mMarginHeight = frame->MarginHeight();
			}
			else
				pprint("Frames are now deeper than 10 so we'll stop adding frames.");
			num++;
		}

		delete (curViewBound);
	}
}

void
HTMLView::SetupFrameRects(
	const FrameList		*frames,
	BRect				maxBounds,
	BList				*viewBounds)
{

	//arrange the viewBounds 
	SetupFrameRowsOrCols(frames, maxBounds.Width(), viewBounds, true);
	SetupFrameRowsOrCols(frames, maxBounds.Height(), viewBounds, false);

}

void 
HTMLView::SetupFrameRowsOrCols(const FrameList *frames, float maxBound, BList *viewBounds, bool isColumns)
{
	FrameItem	*firstFrame = (FrameItem *)frames->First();
	if (!firstFrame)
		return;
	
	BRect *firstBound = (BRect *)viewBounds->ItemAt(0);
	if (!firstBound)
		return;
		
	BList values;
	BList valuePercents;
	int32 num = 0;
	int32 numCols = 0;
	int32 numRows = 0;

	//get values to manipulate and count columns
	for(FrameItem *frame = firstFrame; frame != NULL && 0 == frame->RowPosition(); frame = (FrameItem *)frame->Next()){
		++numCols;
		if(isColumns){ //we don't touch these if we are doing rows
			bool *isPercent = new bool(false);
			int32 *val = new int32(frame->ColValue(isPercent));
			values.AddItem(val);
			valuePercents.AddItem(isPercent);		
		}
	}
	
	//put row values in heights to use below and count rows
	for(FrameItem *frame = firstFrame; frame != NULL; frame = (FrameItem *)frame->Next()){
		if(frame->RowPosition() != numRows - 1){
			++numRows;
			if(!isColumns){ //don't touch if we are doing columns
				bool *isPercent = new bool(false);
				int32 *val = new int32(frame->RowValue(isPercent));
				values.AddItem(val);
				valuePercents.AddItem(isPercent);
			}
		}
	}

	if(isColumns)
		num = numCols;
	else
		num = numRows;
	
	float valueRemaining = maxBound;
	int32 numValueWildcards = 0;
	float valueWildcard = 0;
		
	//set up initial values for non wildcard values and count the number of wildcards
	for(int32 i=0; i<values.CountItems(); ++i){
		bool *isPercentage = (bool *)valuePercents.ItemAt(i);
		int32 *valueValue = (int32 *)values.ItemAt(i);
		if(*valueValue < 0)
			numValueWildcards -= *valueValue;
		else {
			if(*isPercentage)
				*valueValue = (int32)(ceil(maxBound * (float)((float)*valueValue / (float)100.0)));
			if(*valueValue > 0)
				*valueValue -= 1;

			valueRemaining -= *valueValue;			
		}
	}

	//if we don't have enough space, hack some out of the non-wildcard frames
	if(valueRemaining < 0 && num - numValueWildcards > 0){
		float amountToCut = (-valueRemaining) / ((float)(num - numValueWildcards));
		for(int32 i=0; i<values.CountItems(); ++i){
			int32 *valueValue = (int32 *)values.ItemAt(i);
			if(*valueValue >= 0 && *valueValue - (int32)amountToCut > 0)
				*valueValue -= (int32)amountToCut;
		}
		valueRemaining = 0;
	}

	//figure out the value for adding to wildcard frames
	valueWildcard = numValueWildcards ? ceil(valueRemaining / numValueWildcards) : 0;

	//distribute extra space or setup wildcard frames with proper value
	for(int32 i=0; i<values.CountItems(); ++i){
		int32 *valueValue = (int32 *)values.ItemAt(i);
		if(numValueWildcards == 0 && valueRemaining > 0)
			*valueValue += (int32)(valueRemaining / (float)num);
		else if (*valueValue < 0) //<0 indicates a wildcard and valueValue is -multiplier for wildcard
			*valueValue = (int32)((*valueValue * -1 * valueWildcard) - 1.0); 
	}

	float valueAllocated = 0;
	int32 iter = 0;
	int32 position = 0;
	if(isColumns){
		for(FrameItem *frame = firstFrame; frame != NULL; frame = (FrameItem *)frame->Next()) {
			if(position != frame->RowPosition()){
				valueAllocated = 0;
				++position;
			}
			BRect *curViewBound = (BRect *)viewBounds->ItemAt(iter);
			int32 *tempValue = (int32 *)values.ItemAt(frame->ColPosition());
			curViewBound->left += valueAllocated;
			curViewBound->right = curViewBound->left + *tempValue;
			valueAllocated += *tempValue;
			++iter;
		}	
	}
	else {
		//save the top of the frame for later use
		float topOfRects = 0;
		if(firstBound)
			topOfRects = firstBound->top;
			
		FrameItem *frame = firstFrame;
		while(frame){
			BRect *curViewBound = (BRect *)viewBounds->ItemAt(iter++);
			int32 *tempValue = (int32 *)values.ItemAt(frame->RowPosition());
			curViewBound->top += valueAllocated;
			curViewBound->bottom = curViewBound->top + *tempValue;
			frame = (FrameItem *)frame->Next();
			if(frame && frame->RowPosition() != position){
				valueAllocated = curViewBound->bottom - topOfRects + 1;
				++position;
			}
		}
	}
	
	iter = 0;	
	for (FrameItem *frame = firstFrame; frame != NULL; frame = (FrameItem *)frame->Next()) {
		int32 border = frame->BorderValue();
		BRect 	*curViewBound = (BRect *)viewBounds->ItemAt(iter);
		if(!curViewBound)
			break;
		if(frame->Border()){
			if(isColumns){
				if(frame->ColPosition() != values.CountItems() - 1)
					curViewBound->right -= border;
				else
					curViewBound->right += 1;
				if(frame->ColPosition() != 0)
					curViewBound->left += border;
			}
			else {
				if(frame->RowPosition() != values.CountItems() - 1)
					curViewBound->bottom -= border;
				else
					curViewBound->bottom += 1;
				if(frame->RowPosition() != 0)
					curViewBound->top += border;
			}
		}
		++iter;
	}

	for(int i=0; i<values.CountItems(); ++i){
		int32 *deadVal = (int32 *)values.RemoveItem(i);
		bool *deadBool = (bool *)valuePercents.RemoveItem(i);
		delete deadVal;
		delete deadBool;
	}
}

void
HTMLView::SetFrameNum(
	int32	num)
{
	mFrameNum = num;
}

int32
HTMLView::FrameNum()
{
	return (mFrameNum);
}


HTMLWorker::HTMLWorker(HTMLView *view) : BLooper("HTMLWorker", B_NORMAL_PRIORITY + 1, 500), mHTMLView(view), mMessenger(this), mDocConsumerComplete(false), mMainDocParser(0)
{
	mMaxQueueLength = 0;
	mNeedsToDoWork = false;
}

HTMLWorker::~HTMLWorker()
{
	for (int32 i = 0; i < mConsumerList.CountItems(); i++) {
		ConsumerInfo *ci = (ConsumerInfo *)mConsumerList.ItemAt(i);
		delete ci;
	}
}

void HTMLWorker::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case msg_AddConsumer: 
			if (mHTMLView) {
			Consumer *c;
			if (msg->FindPointer("Consumer", (void **)&c) == B_NO_ERROR) {
				if (!c->AddListener(&mMessenger)) {
					if (c->LockWithTimeout(ONE_SECOND) != B_OK) {
						BMessage msg(B_QUIT_REQUESTED);
						c->PostMessage(&msg);
					} else
						c->Quit();
					break;
				}
				if (!mHTMLView->mHTMLDoc || c == mHTMLView->mHTMLDoc->GetParser())
					mMainDocParser = c;
			}			
					
			if (!FindConsumerInfo(c)) {
				ConsumerInfo *ci = new ConsumerInfo;
				ci->mConsumer = c;
				ci->mStatus = kIdle; // Until we find out otherwise, don't include us in the stats.
				ci->mDone = 0;
				ci->mOf = 0;
				mConsumerList.AddItem(ci);
				BMessage msg2(msg_ConsumerUpdate);
				PostMessage(&msg2);
			}
			break;
		}
			
		case msg_RelayoutView: 
			if (mHTMLView) {
				mHTMLView->RealRelayoutView(-1.0, true);
				break;
			}

		case msg_ConsumerUpdate: 
			if (mHTMLView) {
				Consumer *c;
				if (msg->FindPointer("Consumer", (void **)&c) == B_NO_ERROR) {				
					ConsumerInfo *ci = FindConsumerInfo(c);
					if (ci && ci->mStatus != kComplete) {
						int32 tmp;
						if (msg->FindInt32("Status", &tmp) == B_OK)
							ci->mStatus = (StoreStatus)tmp;
						if (msg->FindInt32("Done", &tmp) == B_OK)
							ci->mDone = tmp;
						if (msg->FindInt32("Of", &tmp) == B_OK)
							ci->mOf = tmp;
					}
				}
				BWindow *window = mHTMLView->Window();
				if (!window)
					return;
				if (window->LockWithTimeout(ONE_SECOND) != B_OK) {
					PostMessage(msg_ViewUpdate);
					return;
				}
				UpdateStatus();
				msg->MakeEmpty();
				if (!mNeedsToDoWork) {
					mNeedsToDoWork = true;
					PostMessage(msg_DoSomeWork);
				}
				window->Unlock();
				break;
			}
		
		case msg_ViewUpdate: {
			if (!mHTMLView)
				return;
			BWindow *window = mHTMLView->Window();
			if (!window)
				return;
			if (window->LockWithTimeout(ONE_SECOND) != B_OK) {
				PostMessage(msg_ViewUpdate);
				return;
			}
			if (!mNeedsToDoWork) {
				mNeedsToDoWork = true;
				PostMessage(msg_DoSomeWork);
			}
			window->Unlock();
			break;
		}
		
		case msg_RequestViewUpdate: {
			snooze(100000);
			PostMessage(msg_ViewUpdate);
		}
			
		case msg_ConsumerWantsQuit: {
			Consumer *c;
			if (msg->FindPointer("Consumer", (void **)&c) == B_NO_ERROR) {
				ConsumerInfo *ci = FindConsumerInfo(c);
				if (ci && c) {
					ci->mConsumer = 0;
					ci->mStatus = kComplete;
				}
				
				if (c) {
					if (c->LockWithTimeout(ONE_SECOND) != B_OK) {
						BMessage msg(B_QUIT_REQUESTED);
						c->PostMessage(&msg);
					} else
						c->Quit();
				}
			}
			//Fall through to next case
		}
		
		case msg_ConsumerFinished:
			 if (mHTMLView) {
				BWindow *window = mHTMLView->Window();
				if (!window)
					return;
				if (window->LockWithTimeout(ONE_SECOND) != B_OK) {
					BMessage msg2(*msg);
					PostMessage(&msg2);
					return;
				}
				Consumer *c;
				if (msg->FindPointer("Consumer", (void **)&c) == B_NO_ERROR) {
					ConsumerInfo *ci = FindConsumerInfo(c);
					if (ci) {
						ci->mStatus = kComplete;
					}
				}
				
				if (c == mMainDocParser) {
					mDocConsumerComplete = true;
				}
				UpdateStatus();
					
				if (!mNeedsToDoWork) {
					mNeedsToDoWork = true;
					PostMessage(msg_DoSomeWork);
				}
				window->Unlock();
				break;
			}
		
		case msg_WorkerFinished: {
			Cleanup();

			Document *doc = NULL;
			msg->FindPointer("HTMLDoc", (void **)&doc);
			
			if (doc) {
				doc->Lock();
				if (doc->Dereference() > 0)
					doc->Unlock();
			}
			
			// We need to quit immediately now.  If we try to process any resource/consumer update or add
			// messages, we'll get into trouble.
			Quit();
			break;
		}
		
		case msg_DoSomeWork: {
			if (mHTMLView) {
				BWindow *window = mHTMLView->Window();
				if (window->LockWithTimeout(ONE_SECOND) == B_OK) {
					mHTMLView->DoWork(true);
					mNeedsToDoWork = false;
					window->Unlock();
				} else
					PostMessage(msg_DoSomeWork);
			} else
				mNeedsToDoWork = false;
			break;
		}
			
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}

bool HTMLWorker::QuitRequested()
{
	Cleanup();
	NetPositive::RemoveThread(Thread());
	return true;
}


void HTMLWorker::Cleanup()
{
	mHTMLView = 0;
	for (int32 i = 0; i < mConsumerList.CountItems(); i++) {
		ConsumerInfo *ci = (ConsumerInfo *)mConsumerList.ItemAt(i);
		if (ci->mConsumer) {
			if (ci->mConsumer->LockWithTimeout(ONE_SECOND * 2) != B_OK) {
				BMessage msg(B_QUIT_REQUESTED);
				ci->mConsumer->PostMessage(&msg);
			} else
				ci->mConsumer->Quit();
			ci->mConsumer = 0;
		}
	}
}


void HTMLWorker::UpdateStatus()
{
	StoreStatus status = kComplete;
	long done = 0;
	long of = 0;
	
//	If HTML parsing is under way, return kLoadingHTML

	if (mHTMLView->mNewDocResource)
		return;
		
	BWindow *window = mHTMLView->Window();
	
	if (!mDocConsumerComplete) {
		ConsumerInfo *ci = FindConsumerInfo(mMainDocParser);
		if (ci) {
			done = ci->mDone;
			of = ci->mOf;
		}
		status = kLoadingHTML;			// Still loading html
	} else {
		for (int32 i = 0; i < mConsumerList.CountItems(); i++) {
			ConsumerInfo *ci = (ConsumerInfo *)mConsumerList.ItemAt(i);
			
			bool icomplete = false;
			
			if (ci->mStatus == kAbort || ci->mStatus == kComplete || ci->mStatus == kError || !ci->mConsumer) {
				icomplete = true;
			} else {
				status = kLoadingImages;
				icomplete = ci->mDone >= ci->mOf;
				if (ci->mDone > ci->mOf)
					ci->mDone = ci->mOf;
				of += ci->mOf;
			}
			done += ci->mDone;
		}
	}
	
	// If there are still connections pending but no consumers, that means that we have parsed
	// all of the images we have received but are still waiting for more.
	if (status == kComplete && mHTMLView->mConnectionMgr->GetTotalNumConnections() > 0)
		status = kLoadingImages;
	
	if (done > of)
		of = 0;
		
	mHTMLView->SetProgress(status,done,of);

	if (window->LockWithTimeout(ONE_SECOND) != B_OK)
		return;

	switch (status) {
		case kUninitialized:
		case kIdle:
		case kComplete:
		case kTimeout:
		case kAbort:
		case kError:
		{
			mHTMLView->mStopEnabled = false;
			mHTMLView->UpdateURLView();
			mHTMLView->ChangeCursor();
	
			break;
		}

		default:
			break;
	}
	window->Unlock();
}

ConsumerInfo *HTMLWorker::FindConsumerInfo(Consumer *c)
{
	ConsumerInfo *retval = 0;
	for (int32 i = 0; i < mConsumerList.CountItems(); i++) {
		ConsumerInfo *ci = (ConsumerInfo *)mConsumerList.ItemAt(i);
		if (ci->mConsumer == c)
			retval = ci;
	}
	return retval;
}
