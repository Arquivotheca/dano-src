// ===========================================================================
//	HTMLWindow.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include <Menu.h>

#include "HTMLWindow.h"
#include "URLView.h"
#include "HTMLView.h"
#include "Bookmarks.h"
#include "NPApp.h"
#include "ProgressView.h"
#include "UResource.h"
#include "Cache.h"
#include "HistoryMenu.h"
#include "FindWindow.h"
#include "NetPositive.h"
#include "MessageWindow.h"
#include "DownloadManager.h"

#include <Dragger.h>
#include <Alert.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <ScrollBar.h>
#include <FilePanel.h>
#include <Screen.h>
#include <stdio.h>

#define REPLICANTS

extern void DoPrefs();
extern void OpenLocationWindow();

// ============================================================================

extern bool gDrawInfo;
extern bool gDrawCell;
extern bool gUseLatin5;
extern const char *gWindowTitle;

BList	HTMLWindow::sWindowList;

#define PROGRESSBARWIDTH 320



class HTMLWindowFilter : public BMessageFilter {
public:
	HTMLWindowFilter(HTMLWindow *window);
	
	virtual	filter_result		Filter(BMessage *message, BHandler **target);
private:
	HTMLWindow *mWindow;	
};

HTMLWindowFilter::HTMLWindowFilter(HTMLWindow *window) : BMessageFilter(B_MOUSE_DOWN), mWindow(window) {}

filter_result HTMLWindowFilter::Filter(BMessage *message, BHandler **target)
{
	if (message->what == B_MOUSE_DOWN)
		mWindow->Activate();
	return B_DISPATCH_MESSAGE;
}


// ============================================================================
// ============================================================================

class TResizer : public BView {
public:
					TResizer(BRect rect, BView *target, uint32 rmask,
						uint32 flags = B_WILL_DRAW);

virtual	void		Draw(BRect update);
virtual	void		AttachedToWindow();
virtual	void		MouseDown(BPoint pt);

private:
};

class TDragger : public BDragger {
public:
					TDragger(BRect rect, BView *target, uint32 rmask,
						uint32 flags = B_WILL_DRAW);

virtual	void		Draw(BRect update);
virtual	void		AttachedToWindow();

virtual	void		Show();
virtual	void		Hide();

private:
		int32		mHidden;
};


NPBaseView::NPBaseView(
	BRect		frame,
	const char	*name,
	UResourceImp	*resource,
	bool		openAsText,
	uint32		encoding,
	bool		showToolbar,
	bool		showProgress,
	bool		resizable,
	int			showTitleInToolbar,
	BMessage	*originatingMessage)
		: BView(frame, name, B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS | B_WILL_DRAW)
{
	Init(resource, openAsText, encoding, showToolbar, showProgress, resizable, showTitleInToolbar, originatingMessage);
	mLinkHistory.SetBaseView(this);
}


NPBaseView::NPBaseView(
	BMessage	*data)
		: BView(data)
{
	const char *url = data->FindString("url");
	uint32 encoding = data->FindInt32("encoding");
	bool openAsText = data->FindBool("openAsText");
	bool showDecorations;
	if (data->FindBool("showToolbars", &showDecorations) != B_OK)
		showDecorations = true;
	Init(NULL, openAsText, encoding, showDecorations, showDecorations, true, kShowTitleUnspecified, NULL);
	mLinkHistory.SetBaseView(this);
	mReplicantURL = url;
	SetResizingMode(B_FOLLOW_NONE);
}


NPBaseView::~NPBaseView()
{
}

void NPBaseView::Init(UResourceImp *resource, bool openAsText, uint32 encoding, bool showToolbar, bool showProgress, bool resizable, int showTitleInToolbar, BMessage *originatingMessage)
{
	BRect aRect;
	
	bool URLViewOnBottom = gPreferences.FindBool("URLViewOnBottom");

	float kURLHeight = gPreferences.FindInt32("URLViewHeight");

	if (showToolbar) {
		//	URLView
		aRect = Bounds();
		if (URLViewOnBottom) {
			aRect.bottom -= B_H_SCROLL_BAR_HEIGHT;
			aRect.top = aRect.bottom - kURLHeight - 1.0;
		} else
			aRect.bottom = aRect.top + kURLHeight - 1.0;
		mURLView = new URLView(aRect, this, URLViewOnBottom ? B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT : B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
		AddChild(mURLView);
		kURLHeight = mURLView->Frame().Height();
	}
	
	if (showProgress) {
		//	Resizer
		if (gRunningAsReplicant) {
			aRect = Bounds();	
			aRect.top = aRect.bottom - B_H_SCROLL_BAR_HEIGHT;
			aRect.left = aRect.right - B_V_SCROLL_BAR_WIDTH;
			TResizer *rs = new TResizer(aRect, this,
				B_FOLLOW_BOTTOM + B_FOLLOW_RIGHT, B_WILL_DRAW);
			AddChild(rs);
		}

		//	Progress bar
		aRect = Bounds();
		aRect.top = aRect.bottom - B_H_SCROLL_BAR_HEIGHT + 1.0;
		if (gPreferences.FindBool("DesktopMode"))
			aRect.right -= B_V_SCROLL_BAR_WIDTH /*- 1.0 */;

#ifdef REPLICANTS
		aRect.left += 8.0;
#endif
		AddChild(new ProgressView(aRect,"Progress", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, true));
#ifdef REPLICANTS
		// Dragger
		aRect.left -= 8.0;
		aRect.right = aRect.left + 7.0;
		TDragger *drag = new TDragger(aRect, this, B_FOLLOW_BOTTOM);
		AddChild(drag);
#endif
	} else {
		mURLView = NULL;
	}

	//	Main HTML View
	aRect = Bounds();

	if (showProgress)
		aRect.bottom -= B_H_SCROLL_BAR_HEIGHT;

	if (showToolbar) {
		if (URLViewOnBottom)
			aRect.bottom -= kURLHeight;
		else
			aRect.top += kURLHeight + 1;
	}
	

	BView *htmlAreaView = new BView(aRect, "HTMLAreaView", B_FOLLOW_ALL, 0);
	AddChild(htmlAreaView);
	aRect.OffsetTo(B_ORIGIN);
	mHTMLView = new HTMLView(aRect, kHTMLViewName, resource, &mLinkHistory, openAsText, encoding, false,
		(gPreferences.FindBool("DesktopMode") && !showProgress) ? optionalVScroller : (resizable ? mandatoryVScroller : disallowScrollBars),
		NULL, showTitleInToolbar, originatingMessage);
	htmlAreaView->AddChild(mHTMLView);
}

void NPBaseView::AttachedToWindow()
{
	BView::AttachedToWindow();
	BWindow *window = Window();
	if (!window->Lock())
		return;
	window->Unlock();
	if (gRunningAsReplicant && mReplicantURL.Length()) {
		BMessage msg(HTML_MSG + HM_ANCHOR);
		msg.AddString("url", mReplicantURL.String());
		Window()->PostMessage(&msg, mHTMLView);
	}
}

status_t
NPBaseView::Archive(
	BMessage	*data,
	bool) const
{
	HTMLView *htmlView = GetTopHTMLView();
	BPoint p;
	
	p = htmlView->Bounds().LeftTop();
	htmlView->DestroyScrollBar(B_VERTICAL);
	htmlView->DestroyScrollBar(B_HORIZONTAL);

	data->AddString("add_on", kApplicationSig);
	data->AddInt32("version", 2);

	const char *url = mHTMLView->GetDocumentURL();
	data->AddString("url", url ? url : "");
	data->AddBool("openAsText", mHTMLView->GetOpenAsText());
	data->AddInt32("encoding", mHTMLView->GetEncoding());
	
	status_t retval = (BView::Archive(data, false));

	htmlView->UpdateScrollBars();
	htmlView->ScrollTo(p);
		
	return retval;
}

/*
void PrintRecursively(BMessage *msg)
{	
	msg->PrintToStream();
	int32 i, j;
	type_code bogus;
	BMessage submsg;

	msg->GetInfo("_views", &bogus, &i);
	printf("Message 0x%x has %d subviews\n", msg, i);
	if (i > 0) {
		for (j = 0; j < i; j++) {
			printf("\nDumping submessage %d of %d of message 0x%x\n\n", j, i, msg);
			msg->FindMessage("_views", j, &submsg);
			PrintRecursively(&submsg);
		}
	}
}
*/

BArchivable*
NPBaseView::Instantiate(
	BMessage	*data)
{
#ifndef REPLICANTS
	return NULL;
#endif
	
//	PrintRecursively(data);

	if (!validate_instantiation(data, "NPBaseView"))
		return (NULL);
		
	int32 version = data->FindInt32("version");

	if (version != 0 && version != 2)
		return NULL;
		
	// If the version number is zero (no version number specified in the
	// BMessage), then we're dealing with a pre-v2.0 replicant archive.
	// Convert it to the new format.  The old format archived the NPBaseView
	// and all of its subviews; the new format archives just the NPBaseView
	// and moves all of the necessary information into the top level of the
	// message.  We'll convert by finding that necessary information in the
	// submessages, moving it up, and deleting all of the submessages.
	if (version == 0) {
		int32 i, j;
		type_code bogus;
		BMessage submsg;
		bool foundIt = false;
	
		data->GetInfo("_views", &bogus, &i);
		if (i > 0) {
			for (j = 0; j < i; j++) {
				data->FindMessage("_views", j, &submsg);
				const char *name = submsg.FindString("_name");
				if (name && strcmp(name, "HTMLAreaView") == 0) {
					BMessage submsg2;
					if (submsg.FindMessage("_views", 0, &submsg2) == B_OK) {
						foundIt = true;
						const char *url = submsg2.FindString("url");
						data->AddString("url", url ? url : kDefaultStartupURL);
						data->AddBool("openAsText", submsg2.FindBool("openAsText"));
						data->AddInt32("encoding", submsg2.FindInt32("encoding"));
						break;
					}
				}
			}
		}
		
		if (!foundIt) {
			data->AddString("url", kDefaultStartupURL);
			data->AddBool("openAsText", false);
			data->AddInt32("encoding", B_MS_WINDOWS_CONVERSION);
		}
		
		data->RemoveName("_views");
		data->AddInt32("version", 2);
	}
	
	gRunningAsReplicant = true;

	NetPositive::Init();

	
	return new NPBaseView(data);
}


void
NPBaseView::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			be_app->AboutRequested();
			break;

/*
		case HTML_MSG + HM_ANCHOR:
		{
			const char *url = NULL;
			const char *referrer = message->FindString("Referrer");
			bool forceCache = false;
			message->FindBool("forceCache", &forceCache);
			if (message->FindString("url", &url) == B_NO_ERROR) {
				int32 num = 0;
				if (message->FindInt32("parent", &num) == B_NO_ERROR)
					NewHTML(url, num, forceCache, referrer);
				else {
					const char *target = NULL;
					message->FindString("target", &target);
					const char *post = message->FindString("post");
					int32 showTitleInToolbar = kShowTitleUnspecified;
					message->FindInt32("showTitleInToolbar", &showTitleInToolbar);
					pprint ("Window received HM_ANCHOR.  Looking for target %s", target);
					NewHTML(url, target, forceCache, post, showTitleInToolbar, referrer);
				}
			}
			break;
		}
*/
		
		case HTML_MSG + HM_ANCHOR:
		case B_NETPOSITIVE_OPEN_URL:
		{
			const char *url = message->FindString("be:url");
			if (!url || !*url)
				url = message->FindString("url");
#ifdef PLUGINS
			bool returnStream;
			
			if (message->FindBool("ReturnStream", &returnStream) == B_OK && returnStream) {
				if (mHTMLView && mHTMLView->GetDocument()) {
					DocAutolock lock(mHTMLView->GetDocument());
					BString fullURL;				
					mHTMLView->GetDocument()->ResolveURL(fullURL, url);
					message->ReplaceString("url", fullURL.String());
				}
				thread_id tid = spawn_thread(HTMLView::CreateStreamIOThread, "Create BNetPositiveStreamIO", B_NORMAL_PRIORITY, Window()->DetachCurrentMessage());
				resume_thread(tid);
				NetPositive::AddThread(tid);
			} else {
#endif
				const char *postData = message->FindString("PostData");
				if (!postData || !*postData)
					postData = message->FindString("post");
				const char *target = message->FindString("Target");
				const char *referrer = message->FindString("Referrer");

				bool forceCache = false;
				message->FindBool("forceCache", &forceCache);
	
				if (!url)
					url = "";
				
				int32 num = 0;
				if (message->FindInt32("parent", &num) == B_NO_ERROR)
					NewHTML(url, num, forceCache, referrer);
				else {
					int32 showTitleInToolbar = kShowTitleUnspecified;
					message->FindInt32("showTitleInToolbar", &showTitleInToolbar);
					pprint ("Window received HM_ANCHOR.  Looking for target %s", target);
					NewHTML(url, target, forceCache, postData, showTitleInToolbar, referrer);
				}

//				mHTMLView->NewHTMLDocFromURL(BString(url));
/*
				BMessage newMessage(HTML_MSG + HM_ANCHOR);
				newMessage.AddString("url", url);
				if (referrer && *referrer) newMessage.AddString("Referrer", referrer);
				if (target && *target) newMessage.AddString("target", target);
				if (postData && *postData) newMessage.AddString("post", postData);
				MessageReceived(&newMessage);
*/
#ifdef PLUGINS
			}
#endif
		}
		break;

		case B_NETPOSITIVE_BACK:
		case NP_BACKBUTTON:
			mLinkHistory.GoBack();
			break;

		case B_NETPOSITIVE_DOWN:{
			HTMLView *htmlView = GetTopHTMLView();
			if (htmlView) 
				htmlView->ScrollTo(0,100000);
			break;
		}
		case B_NETPOSITIVE_UP:{
			HTMLView *htmlView = GetTopHTMLView();
			if (htmlView) 
				htmlView->ScrollTo(0,0);
			break;
		}
		case B_NETPOSITIVE_FORWARD:
		case NP_FORWBUTTON:
			mLinkHistory.GoForward();
			break;

		case B_NETPOSITIVE_RELOAD:
		case NP_RELOADBUTTON:
			{
			HTMLView *htmlView = GetTopHTMLView();
			if (htmlView)
				htmlView->Reload();
			break;
			}
			
		case B_NETPOSITIVE_STOP:
		case NP_STOPBUTTON:
			{
			HTMLView *htmlView = GetTopHTMLView();
			if (htmlView)
				Window()->PostMessage(message, htmlView);
			break;
			}
			
		case B_NETPOSITIVE_HOME:
		case NP_HOMEBUTTON:
		case NP_SEARCHBUTTON:
			{
			BString url;
			if (message->what == NP_SEARCHBUTTON)
				url = gPreferences.FindString("SearchURL");
			else
				url = gPreferences.FindString("DefaultURL");

			HTMLView *htmlView = GetTopHTMLView();
			if (htmlView) {
				htmlView->RewindHistory();
				htmlView->NewHTMLDocFromURL(url);
			}
			break;
			}
		case CLEAR: //this avoids infinite loops of CLEAR messages
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

void NPBaseView::FrameResized(float width, float height)
{
	static float oldWidth = 0.0, oldHeight = 0.0;
	HTMLView *htmlView = GetTopHTMLView();
	if (htmlView && (width != oldWidth || height != oldHeight)) {
		htmlView->RelayoutView(false);
		oldWidth = width;
		oldHeight = height;
	}
}


HTMLView *NPBaseView::GetTopHTMLView() const
{
	BWindow *window = Window();
	if (!window->Lock())
		return NULL;
	HTMLView *val = (HTMLView *)FindView(kHTMLViewName);
	window->Unlock();
	return val;
}


void
NPBaseView::NewHTML(
	const char	*url,
	const char	*target,
	bool		 forceCache,
	const char	*formData,
	int			 showTitleInToolbar,
	const char	*referrer,
	BMessage	*originatingMessage)
{
	HTMLView *firstHTMLView = NULL;

	// 0 = URLView, 1 = Resizer, 2 = Progress Bar, 3 = Dragger 4 = HTMLAreaView
	BWindow *window = Window();
	if (!window->Lock())
		return;
	BView *areaView = FindView("HTMLAreaView");
	BView *child = NULL;
	for (int32 i = 0; (bool)(child = areaView->ChildAt(i)); i++) {
		HTMLView *theHTMLView = dynamic_cast<HTMLView *>(child);

		if (target != NULL && strcmp(target, "_self") != 0 && strcmp(target, "_top") != 0) {
			// do this in the loop instead of FindView(target) because
			// we might have a namespace problem
			if (theHTMLView != NULL) {
				const char *theHTMLName = theHTMLView->Name();

				if (theHTMLName != NULL) {
					if (strcmp(theHTMLName, target) == 0) {
						pprint("Found target %s.  Opening URL.", target);
						BString urlString(url);
						theHTMLView->OpenLink(urlString, NULL, formData, referrer, kShowTitleUnspecified, originatingMessage);
						break;
					} else {
						// The target doesn't exist.  Open the URL in a new window.
						BString str(url);
						NetPositive::NewWindowFromURL(str, B_MS_WINDOWS_CONVERSION, formData, false, true, true, NULL, true, showTitleInToolbar, NULL, originatingMessage);
					}
				}
			}
		}
		else {
			if (firstHTMLView == NULL) {
				if (theHTMLView != NULL) {
					firstHTMLView = theHTMLView;
					continue;
				}
			}
	
//			child->RemoveSelf();
//			delete (child);		
//			i--;	// because we're ChildAt()-ing
		}
	}

	if (firstHTMLView != NULL) {
		BString	theURL = url;
//		BRect	viewRect = areaView->Bounds();
			
//		firstHTMLView->MoveTo(viewRect.LeftTop());
//		firstHTMLView->ResizeTo(viewRect.Width(), viewRect.Height());
		if (formData && *formData) {
			BString post(formData);
			firstHTMLView->NewHTMLDocFromURL(theURL, true, &post, forceCache, true, referrer, originatingMessage);
		} else
			firstHTMLView->NewHTMLDocFromURL(theURL, false, NULL, forceCache, true, referrer, originatingMessage);
	}
	window->Unlock();
}

void
NPBaseView::NewHTML(
	const char	*url,
	int32		num,
	bool		forceCache,
	const char	*referrer,
	BMessage	*originatingMessage)
{
	HTMLView	*firstHTMLView = NULL;
	BRect		totalRect(-1.0, -1.0, -2.0, -2.0);

	// 0 = URLView, 1 = Resizer, 2 = Progress Bar, 3 = Dragger 4 = HTMLAreaView
	BWindow *window = Window();
	if (!window->Lock())
		return;
	BView *areaView = FindView("HTMLAreaView");
	BView *child = NULL;
	for (int32 i = 0; (bool)(child = areaView->ChildAt(i)); i++) {
		HTMLView *theHTMLView = dynamic_cast<HTMLView *>(child);

		if (theHTMLView != NULL) {
			if (theHTMLView->FrameNum() == num) {
				if ((totalRect.Width() < 0.0) && (totalRect.Height() < 0.0))
					totalRect = theHTMLView->Frame();
				else
					totalRect = totalRect | theHTMLView->Frame();
				
				if (firstHTMLView == NULL)
					firstHTMLView = theHTMLView;
				else {
					theHTMLView->RemoveSelf();
					delete (theHTMLView);
					i--;
				}			
			}
		}
	}

	if (firstHTMLView != NULL) {
		firstHTMLView->MoveTo(totalRect.LeftTop());
		firstHTMLView->ResizeTo(totalRect.Width(), totalRect.Height());
		BString urlString(url);
		firstHTMLView->NewHTMLDocFromURL(urlString, false, NULL, forceCache, true, referrer, originatingMessage);
	}
	window->Unlock();
}

void TDragger::Show()
{
	mHidden++;
//+	if (mHidden == 0)
		Invalidate(Bounds());
//+	BDragger::Show();
}

void TDragger::Hide()
{
	mHidden--;
//+	if (mHidden == 0)
		Invalidate(Bounds());
//+	BDragger::Hide();
}

TDragger::TDragger(BRect r, BView *t, uint32 rmask, uint32 flags)
	: BDragger(r, t, rmask, flags)
{
	mHidden = 0;
}

void TDragger::Draw(BRect update)
{
	BRect r = Bounds();
	SetHighColor(152,152,152);
	MovePenTo(r.left,r.top);
	StrokeLine(BPoint(r.right,r.top));
	SetHighColor(255,255,255);
	MovePenTo(r.left,r.bottom-1);
	StrokeLine(BPoint(r.left,r.top+1));
	StrokeLine(BPoint(r.right,r.top+1));

	if (mHidden == 0)
		BDragger::Draw(update);
}

void TDragger::AttachedToWindow()
{
	BDragger::AttachedToWindow();
	SetViewColor(216,216,216);
	SetLowColor(216,216,216);
}

TResizer::TResizer(BRect r, BView *, uint32 rmask, uint32 flags)
	: BView(r, "resizer", rmask, flags)
{
}

void TResizer::MouseDown(BPoint where)
{
	BView	*p = Parent();
	BPoint	lt = p->Bounds().LeftTop();
	ulong	buttons;

	p->ConvertToScreen(&lt);

	do {
		snooze(40000);
		GetMouse(&where, &buttons);
		BPoint rb = where;
		ConvertToScreen(&rb);
		float	width;
		float	height;
		width = rb.x - lt.x;
		height = rb.y - lt.y;

		if (width < PROGRESSBARWIDTH + 64)
			width = PROGRESSBARWIDTH + 64;
		if (height < 80)
			height = 80;

		p->ResizeTo(width, height);
	} while (buttons);
//+	SetSizeLimits(PROGRESSBARWIDTH + 64,2048,128,2048);
}


void	draw_dot(BView *view, BPoint pt)
{
	view->SetHighColor(100,100,100);
	view->StrokeLine(pt, pt);

	pt.x += 1;
	pt.y += 1;

	view->SetHighColor(255,255,255);
	view->StrokeLine(pt, pt);
}

void TResizer::Draw(BRect)
{
	BRect r = Bounds();
	SetHighColor(152,152,152);
	MovePenTo(r.left,r.bottom-1);
	StrokeLine(BPoint(r.right-1,r.bottom-1));
	StrokeLine(BPoint(r.right-1,r.top));

	SetHighColor(255,255,255);
	MovePenTo(r.left,r.bottom);
	StrokeLine(BPoint(r.right,r.bottom));
	StrokeLine(BPoint(r.right,r.top));

	BPoint pt;
	pt = BPoint(r.left + 4, r.bottom - 4);
	draw_dot(this, pt);

	pt.x += 3;
	draw_dot(this, pt);

	pt.x += 3;
	draw_dot(this, pt);

	pt.y -= 3;
	draw_dot(this, pt);

	pt.x -= 3;
	draw_dot(this, pt);

	pt.y -= 3;
	pt.x += 3;
	draw_dot(this, pt);
}

void TResizer::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(216,216,216);
	SetLowColor(216,216,216);
}

//	Extra work to update urltext bevel on resize


// ============================================================================
//	HTMLWindow has a url edit field at the top and 


extern bool gDrawInfo;
extern bool gDrawCell;

static BRect GetFullscreenRect()
{
	BRect rect = BScreen().Frame();
	rect.bottom++;
	return rect;
}

HTMLWindow::HTMLWindow(
	BRect		frame, 
	UResourceImp	*resource, 
	bool		openAsText,
	uint32		encoding,
	bool		fullScreen,
	bool		showToolbar,
	bool		showProgress,
	bool		resizable,
	int			showTitleInToolbar,
	BMessage	*originatingMessage) 
		: BWindow(fullScreen ? GetFullscreenRect() : frame,
				  (gWindowTitle && *gWindowTitle) ? gWindowTitle : kApplicationName,
				  (fullScreen && gPreferences.FindBool("DesktopMode")) ? B_NO_BORDER_WINDOW_LOOK : (resizable ? B_DOCUMENT_WINDOW_LOOK : B_TITLED_WINDOW_LOOK),
				  (fullScreen && gPreferences.FindBool("DesktopMode")) ? window_feel(1024) : B_NORMAL_WINDOW_FEEL,
				  ((fullScreen && gPreferences.FindBool("DesktopMode")) ? (B_AVOID_FRONT | B_WILL_ACCEPT_FIRST_CLICK ) : 0) | (resizable ? 0 : (B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE))),
				  mFullScreen(fullScreen)
{
	mOldWindowRect = frame;
	
	if (encoding == N_USE_DEFAULT_CONVERSION)
		encoding = gPreferences.FindInt32("DefaultEncoding");
		
	mSaveWindowPos = showToolbar && !openAsText;

	showToolbar = (showToolbar && !openAsText && (!gPreferences.FindBool("DesktopMode") || gPreferences.FindBool("ShowToolbar") || !fullScreen));
	showProgress = (showProgress && !openAsText && (!gPreferences.FindBool("DesktopMode") || gPreferences.FindBool("ShowProgress") || !fullScreen));

	if (!openAsText)
		Init(resource, encoding, showToolbar, showProgress, resizable, showTitleInToolbar, originatingMessage);
	else
		InitTextOnly(resource, encoding);
}

void HTMLWindow::Init(UResourceImp *resource, uint32 encoding, bool showToolbar, bool showProgress, bool resizable, int showTitleInToolbar, BMessage *originatingMessage)
{
	BRect		aRect;
	float		menuHeight = 0.0;
//	Menu View

	if (!showToolbar) {
		mMenuBar = NULL;
		mGoMenu = NULL;
		mURLView = NULL;
		mViewMenu = NULL;
		mEditMenu = NULL;
		mFileMenu = NULL;
		mBookmarksMenu = NULL;
	} else {
		aRect.Set(0.0, 0.0, Frame().Width(), 0.0);
		mMenuBar = new BMenuBar(aRect, "HTMLMenu");//new HTMLMenu(aRect);
		
		mFileMenu = BuildMenu("FILE_MENU");	
		mEditMenu = BuildMenu("EDIT_MENU");		
		mGoMenu = BuildMenu("GO_MENU");
		HistoryFolder::GetHistoryFolder()->AddMenu(mGoMenu);
		
	//	Bookmarks menu
		
		mBookmarksMenu = BuildMenu("BOOKMARKS_MENU");
		mBookmarksMenu->SetTargetForItems(this);
		BookmarkFolder::GetBookmarkFolder()->AddMenu(mBookmarksMenu);
	//	BookmarkFolder::AddToMenuList(mBookmarksMenu, mBookmarksMenu->CountItems(), this);
	//	Bookmarks::BuildMenu(mBookmarksMenu, this);
		
	//	Debug menu
	#ifdef DEBUGMENU
		const StaticMenuItem kDebugMenuItems1[] = {
			{"Messages",						DO_MESSAGES,	'M', B_COMMAND_KEY, 1},
			{""}
		};
		
		const StaticMenuItem kDebugMenuItems2[] = {
			{"Dump Objects", 					DO_DUMPOBJECTS,	 0, 0, 1 },
			{"Dump Glyphs",						DO_DUMPGLYPHS,	 0, 0, 1 },
			{"Flush Cache",						DO_FLUSHCACHE,	 0, 0, 1 },
			{"Enter Debugger",					'dbug',			 0, 0, 1 },
			{"-"},
			{"Yahoo! Random Link",				DO_RANDOM_LINK,	 'L', B_COMMAND_KEY | B_SHIFT_KEY, 1 },
			{"Yahoo! Random Link Spam",			DO_RANDOM_SPAM,	 'S', B_COMMAND_KEY | B_SHIFT_KEY, 1 },
			{"-"},
			{"Graze Links",						DO_GRAZE_LINKS,	 0,0, 1 },
			{"-"},
			{""}
		};
	
		mDebugMenu = new BMenu("Debug");
		AddMenuItems(mDebugMenu, kDebugMenuItems1);
		AddMenuItems(mDebugMenu, kDebugMenuItems2);
		mTableDrawInfo = AddMenuItem(mDebugMenu, "Draw Table Info", DO_TABLEDRAWINFO);
		mTableDrawInfo->SetMarked(gDrawInfo);
		mTableDrawCell = AddMenuItem(mDebugMenu, "Draw Table Cells", DO_TABLEDRAWCELL);
		mTableDrawCell->SetMarked(gDrawCell);
	#endif
	
	// Options menu
	
		mViewMenu = BuildMenu("VIEW_MENU");
		BMenuItem *encodingsItem = mViewMenu->FindItem(kEncodingsMenuTitle);
		BMenu *encodingsMenu = 0;
		if (encodingsItem)
			encodingsMenu = encodingsItem->Submenu();
		if (encodingsMenu) {			
			encodingsMenu->SetRadioMode(true);
			
			int32 itemIndex = 0;
			switch (encoding) {
				case B_ISO2_CONVERSION:			itemIndex = 0; break;
				case B_ISO5_CONVERSION:			itemIndex = 1; break;
				case B_KOI8R_CONVERSION:		itemIndex = 2; break;
				case B_MS_DOS_866_CONVERSION:	itemIndex = 3; break;
				case B_MS_WINDOWS_1251_CONVERSION: itemIndex = 4; break;
				case B_ISO7_CONVERSION:			itemIndex = 5; break;
				case N_AUTOJ_CONVERSION:		itemIndex = 6;	break;
				case B_SJIS_CONVERSION:			itemIndex = 7; break;
				case B_EUC_CONVERSION:			itemIndex = 8; break;
				case B_UNICODE_CONVERSION:		itemIndex = 9; break;
				case N_NO_CONVERSION:			itemIndex = 10; break;
				case B_MS_WINDOWS_CONVERSION:	itemIndex = 11; break;
				case B_MAC_ROMAN_CONVERSION:	itemIndex = 12; break;
				default:						itemIndex = 11; break;
			}
			encodingsMenu->ItemAt(itemIndex)->SetMarked(true);	
		}
					
		BMenuItem *fullscreenItem = mViewMenu->FindItem(kFullScreenCommandTitle);
		if (fullscreenItem)
			fullscreenItem->SetMarked(mFullScreen);
	
	//	Create the menu bar
		
		mFileMenu->SetTargetForItems(this);
		mEditMenu->SetTargetForItems(this);
	//	mBookmarksMenu->SetTargetForItems(this);
	#ifdef DEBUGMENU
		mDebugMenu->SetTargetForItems(this);
	#endif
	
		mMenuBar->AddItem(mFileMenu);
		mMenuBar->AddItem(mEditMenu);
		mMenuBar->AddItem(mGoMenu);
		mMenuBar->AddItem(mBookmarksMenu);
		mMenuBar->AddItem(mViewMenu);
	// DEBUG
	#ifdef DEBUGMENU
		mMenuBar->AddItem(mDebugMenu);
	#endif
	
		AddChild(mMenuBar);
		menuHeight = mMenuBar->Frame().Height();
	}

//  Link History

	BRect baseFrame = Bounds();
	if (gPreferences.FindBool("DesktopMode")) {
		if (mFullScreen) {
			if (mMenuBar)
				mMenuBar->Hide();
			baseFrame.top = 0;
			if (gPreferences.FindBool("AllowKBShortcuts")) {
				AddShortcut(B_BACKSPACE,B_COMMAND_KEY, new BMessage(NP_BACKBUTTON),mHTMLView);
				AddShortcut('N', B_COMMAND_KEY, new BMessage(OPEN_NEW),mHTMLView);
				AddShortcut('L', B_COMMAND_KEY, new BMessage(OPEN_LOCATION),mHTMLView);
				AddShortcut('O', B_COMMAND_KEY, new BMessage(OPEN_FILE),mHTMLView);
				AddShortcut('S', B_COMMAND_KEY, new BMessage(SAVE),mHTMLView);
				AddShortcut('W', B_COMMAND_KEY, new BMessage(CLOSE),mHTMLView);
				AddShortcut('P', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(DO_PAGE_SETUP),mHTMLView);
				AddShortcut('P', B_COMMAND_KEY, new BMessage(DO_PRINT),mHTMLView);
				AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO), mHTMLView);
				AddShortcut('X', B_COMMAND_KEY, new BMessage(B_CUT),mHTMLView);
				AddShortcut('C', B_COMMAND_KEY, new BMessage(B_COPY),mHTMLView);
				AddShortcut('V', B_COMMAND_KEY, new BMessage(B_PASTE),mHTMLView);
				AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL),mHTMLView);
				AddShortcut('F', B_COMMAND_KEY, new BMessage(FIND),mHTMLView);
				AddShortcut('G', B_COMMAND_KEY, new BMessage(FIND_AGAIN),mHTMLView);
				AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY, new BMessage(NP_BACKBUTTON),mHTMLView);
				AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY, new BMessage(NP_FORWBUTTON),mHTMLView);
				AddShortcut('H', B_COMMAND_KEY, new BMessage(NP_HOMEBUTTON),mHTMLView);
				AddShortcut('B', B_COMMAND_KEY, new BMessage(ADD_BOOKMARK),mHTMLView);
				AddShortcut('H', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(OPEN_HTML),mHTMLView);
				AddShortcut('R', B_COMMAND_KEY, new BMessage(NP_RELOADBUTTON),mHTMLView);
				AddShortcut('I', B_COMMAND_KEY, new BMessage(LOAD_IMAGES),mHTMLView);
			}
		} else
			baseFrame.top += menuHeight + 1.0;
			
		HTMLWindowFilter *filter = new HTMLWindowFilter(this);
		AddCommonFilter(filter);
	} else
		baseFrame.top += menuHeight + 1.0;

	mBaseView = new NPBaseView(baseFrame, kApplicationName, resource, false, encoding, showToolbar, showProgress, resizable, showTitleInToolbar, originatingMessage);

	mHTMLView = mBaseView->GetHTMLView();
	mURLView = mBaseView->GetURLView();

	AddShortcut('D', B_COMMAND_KEY | B_SHIFT_KEY | B_CONTROL_KEY, new BMessage(LOAD_DOPE), mHTMLView);

	AddChild(mBaseView);

	if (mGoMenu)
		mGoMenu->SetTargetForItems(mBaseView);
//	mURLView->SetTargetForButtons(mBaseView);

	mBaseView->SetBookmarkMenu(mBookmarksMenu);

	mLinkHistory = mBaseView->GetLinkHistory();
	
//	Keep it a reasonable size

	SetSizeLimits(PROGRESSBARWIDTH + 64,2048,128,2048);
	
	if (gPreferences.FindBool("AllowKBShortcuts"))
		AddShortcut(B_BACKSPACE,B_COMMAND_KEY, new BMessage(NP_BACKBUTTON),mHTMLView);

	BTextControl *urlTC = mURLView ? (BTextControl *)mURLView->FindView("url") : 0;
	if (urlTC)
		urlTC->MakeFocus();

	if (resource) {
		resource->AddListener(mHTMLView->GetMessenger());
		resource->NotifyListeners(msg_ResourceChanged);
		mLinkHistory->AddToHistory(resource->GetURL(), NULL);
	}
}

void HTMLWindow::InitTextOnly(UResourceImp *resource, uint32 encoding)
{
	BRect		aRect;
	float		menuHeight = 0.0;
//	Menu View

	aRect.Set(0.0, 0.0, Frame().Width(), 0.0);
	mMenuBar = new BMenuBar(aRect, "HTMLMenu");//new HTMLMenu(aRect);
	
	mFileMenu = BuildMenu("TEXT_FILE_MENU");
	mEditMenu = BuildMenu("TEXT_EDIT_MENU");
	
	mGoMenu = 0;
	mBookmarksMenu = 0;
#ifdef DEBUGMENU
	mDebugMenu = 0;
#endif
	mTableDrawInfo = 0;
	mTableDrawCell = 0;
	mURLView = 0;
	mLinkHistory = 0;

//	Create the menu bar
	
	mFileMenu->SetTargetForItems(this);
	mEditMenu->SetTargetForItems(this);

	mMenuBar->AddItem(mFileMenu);
	mMenuBar->AddItem(mEditMenu);

	AddChild(mMenuBar);
	menuHeight = mMenuBar->Frame().Height();

//  Link History

	BRect baseFrame = Bounds();
	baseFrame.top += menuHeight + 1.0;
	mBaseView = new NPBaseView(baseFrame, kApplicationName, resource, true, encoding, false, false, true, kShowTitleUnspecified, NULL);

	mHTMLView = mBaseView->GetHTMLView();

	AddChild(mBaseView);
	
//	Keep it a reasonable size

	SetSizeLimits(PROGRESSBARWIDTH + 64,2048,128,2048);
	
	if (resource) {
		resource->AddListener(mHTMLView->GetMessenger());
		resource->NotifyListeners(msg_ResourceChanged);
	}
}

HTMLWindow::~HTMLWindow()
{
	BookmarkFolder::GetBookmarkFolder()->RemoveMenu(mBookmarksMenu);
//	Bookmarks::RemoveFromMenuList(mBookmarksMenu);
	HistoryFolder::GetHistoryFolder()->RemoveMenu(mGoMenu);
	sWindowList.RemoveItem(this);
	mGoMenu = 0;
	mViewMenu = 0;
}

void HTMLWindow::EnableBackForw(bool back, bool forw)
{
	if (mGoMenu) {
		mGoMenu->FindItem(kGoBackCommandTitle)->SetEnabled(back);
		mGoMenu->FindItem(kGoForwardCommandTitle)->SetEnabled(forw);
	}
}

void HTMLWindow::EnableLoadImages(bool enable)
{
	if (mViewMenu)
		mViewMenu->FindItem(kLoadImagesCommandTitle)->SetEnabled(enable);
}

#ifdef DEBUGMENU
int32 numSpammers = 0;
int32 numPages = 0;

int32 LinkSpam(void *arg)
{
	atomic_add(&numSpammers, 1);
	
	HTMLWindow *htmlWindow = (HTMLWindow *)arg;
	bigtime_t lastTime;
	HTMLView *htmlView = dynamic_cast<HTMLView *>(htmlWindow->FindView(kHTMLViewName));
	if (!htmlView)
		return 1;
	do {
		BMessage msg(DO_RANDOM_LINK);
		lastTime = system_time();
		htmlWindow->PostMessage(&msg);
		if (atomic_add(&numPages, 1) % 25 == 0 && numPages >= 25)
			printf("%6ld pages spammed\n", numPages - 1);
		char buffer[32];
		sprintf(buffer, "Link spam %ld", numPages);
		rename_thread(find_thread(NULL), buffer);
		snooze(ONE_SECOND * 2);
		while (system_time() - lastTime < TEN_SECONDS * 2 && htmlView->GetStatus() != kComplete) {
			snooze(ONE_SECOND);
		}
	} while (!(modifiers() & B_CAPS_LOCK));
	
	atomic_add(&numSpammers, -1);
	return 0;
}

int32 GrazeLinks(void *arg)
{
	HTMLWindow *htmlWindow = (HTMLWindow *)arg;
	bigtime_t lastTime;
	HTMLView *htmlView = dynamic_cast<HTMLView *>(htmlWindow->FindView(kHTMLViewName));
	
	BList linkList;
	int currentPos = 0;
	
	if (!htmlView)
		return 1;

	bool oldShowImages = gPreferences.FindBool("ShowImages");
	gPreferences.ReplaceBool("ShowImages", false);
	htmlView->GrazeLinks(&linkList);
	do {
		if (linkList.CountItems() > 0) {
			BString *str = (BString *)linkList.ItemAt(currentPos++);
			if (str) {
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", str->String());
	printf("Grazing %s\n", str->String());
				
				lastTime = system_time();
				htmlWindow->PostMessage(&msg);
				snooze(ONE_SECOND);
				int status = htmlView->GetStatus();
				while (system_time() - lastTime < TEN_SECONDS * 6 && status != kComplete && status != kLoadingImages && status != kIdle) {
					snooze(ONE_SECOND / 10);
					status = htmlView->GetStatus();
//	printf("Status %d\n", status);
				}
			}
		}
		htmlView->GrazeLinks(&linkList);
	} while (!(modifiers() & B_CAPS_LOCK) && currentPos < linkList.CountItems());
	
	gPreferences.ReplaceBool("ShowImages", oldShowImages);

	for (int i = 0; i < linkList.CountItems(); i++)
		delete (BString *)linkList.RemoveItem((int32)0);

	return 0;
}
#endif


//	Send copy to html view?

void HTMLWindow::MessageReceived(BMessage *msg)
{
	const char *url;
	entry_ref dir;
	
	switch (msg->what) {
		case DOWNLOADS_STOPPED:
			if(mURLView)
				mURLView->SetDownloading(false);
			break;
		case DOWNLOADS_STARTED:
			if(mURLView)
				mURLView->SetDownloading(true);
			break;
		case OPEN_FILE:
			be_app->PostMessage(OPEN_FILE);
			break;
			
		case B_CUT:
		case B_COPY:
		case B_PASTE:
		case CLEAR:
		case B_SELECT_ALL:
		case B_UNDO: {
			BView* focus = CurrentFocus();
			if (focus == NULL || focus == mHTMLView) {
				pprint("Window got cut or copy, sending to mHTMLView 0x%x", mHTMLView);
				PostMessage(msg->what, mHTMLView);
			} else {
				pprint("Window got cut or copy, sending to focus 0x%x", focus);
				PostMessage(msg->what, focus);
			}
			break;
		}

		case CLOSE:
		{
			if (QuitRequested())
				Quit();
			break;
		}
		
		case OPEN_HIST_BOOKMARK:
		case OPEN_BOOKMARK:
			url = msg->FindString("url");
			if (url)
				mHTMLView->NewHTMLDocFromURL(BString(url));
			break;

		case ADD_BOOKMARK:
			PostMessage(msg, mHTMLView);
			break;
			
		case msg_ZoomIn:
		case msg_ZoomOut:
			SendMessageToHTMLViews(msg->what, NULL);
			break;

		case msg_ShowBookmarks:
			BookmarkFolder::GetBookmarkFolder()->Show();
			break;
			
//		case msg_FindUpdatedBookmarks:
//			BookmarkFolder::GetBookmarkFolder()->FindUpdatedBookmarks();
//			break;

//		Pass to view

		case SAVE:
		case OPEN_HTML:
		case DO_PRINT:
			PostMessage(msg, mHTMLView);
			break;
			
		case DO_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
	
		case DO_PAGE_SETUP:
			HTMLView::PageSetup();
			break;
			
			
//		Debug stuff

#ifdef DEBUGMENU
		case 'dbug':
			debugger("Entering Debugger...");
			break;

		case DO_FLUSHCACHE:
			UResourceCache::Close();
			break;
			
		case DO_USELATIN5:
			gUseLatin5 = !gUseLatin5;
			break;
			
		case DO_TABLEDRAWCELL:
			gDrawCell = !gDrawCell;
			break;

		case DO_TABLEDRAWINFO:
			gDrawInfo = !gDrawInfo;
			break;
			
		case DO_MESSAGES:
			ShowMessages();
			break;
			
		case DO_RANDOM_LINK:
			mHTMLView->NewHTMLDocFromURL(BString(kDebugYahooRandomLink));
			break;
			
		case DO_RANDOM_SPAM: {
			thread_id tid = spawn_thread(LinkSpam, "Link spam", B_NORMAL_PRIORITY, this);
			resume_thread(tid);
			NetPositive::AddThread(tid);
			break;
		}

		case DO_GRAZE_LINKS: {
			thread_id tid = spawn_thread(GrazeLinks, "Link Grazer", B_NORMAL_PRIORITY, this);
			resume_thread(tid);
			NetPositive::AddThread(tid);
			break;
		}
#endif

//		Open New Location
			
		case OPEN_LOCATION: {
#ifdef JAVASCRIPT
			BRect rect(100,100,380,170);
			CenterWindowRect(&rect);
			NetPositive::NewWindowFromURL("netpositive:OpenLocation.html", N_USE_DEFAULT_CONVERSION, NULL, false, false, false, &rect, false);			
#else
			OpenLocationWindow();
#endif
			break;
		}

//		Open New Window

		case OPEN_NEW:
			switch (gPreferences.FindInt32("NewWindowOption")) {
				case 0:
					if (mHTMLView)
						mHTMLView->CloneInNewWindow();
					else
						NetPositive::NewWindowFromResource(NULL);
					break;

				case 1:
					NetPositive::NewWindowFromURL(gPreferences.FindString("DefaultURL"));
					break;

				case 2:
					NetPositive::NewWindowFromResource(NULL);
					break;
			}
			break;

//		Open Find window

		case FIND: {
			FindWindow::Find(this);
			BMenuItem *item = mEditMenu->FindItem(FIND_AGAIN);
			if (item)
				item->SetEnabled(true);
			break;
		}
		
		case FIND_AGAIN:
			FindWindow::FindAgain(this);
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
			SendMessageToHTMLViews(msg->what, NULL);
			break;

		case msg_Prefs:
			DoPrefs();
			break;
		
		case B_ABOUT_REQUESTED:
			be_app->AboutRequested();
			break;

		case msg_RelayoutView:
			SendMessageToHTMLViews(msg->what, NULL);
			break;

		case B_NETPOSITIVE_BACK:
		case B_NETPOSITIVE_FORWARD:
		case B_NETPOSITIVE_HOME:
		case B_NETPOSITIVE_RELOAD:
		case B_NETPOSITIVE_STOP:
			switch(msg->what) {
				case B_NETPOSITIVE_RELOAD:
					msg->what = NP_RELOADBUTTON;
					break;
				case B_NETPOSITIVE_FORWARD:
					msg->what = NP_FORWBUTTON;
					break;
				case B_NETPOSITIVE_BACK:
					msg->what = NP_BACKBUTTON;
					break;
				case B_NETPOSITIVE_HOME:
					msg->what = NP_HOMEBUTTON;
					break;
				case B_NETPOSITIVE_STOP:
					msg->what = NP_STOPBUTTON;
					break;
			}
			// Fall through
			
		case NP_RELOADBUTTON:
		case HTML_MSG + HM_ANCHOR:
		case B_NETPOSITIVE_OPEN_URL:
		case B_NETPOSITIVE_DOWN:
		case B_NETPOSITIVE_UP:
			mBaseView->MessageReceived(msg);
			break;

		case LOAD_IMAGES:
		case LOAD_DOPE:
			SendMessageToHTMLViews(msg->what, NULL);
			break;
			
		case SHOW_DOWNLOADS:
			DownloadManager::ShowDownloadWindow(true);
			break;
			
		case FULL_SCREEN:
			if (mFullScreen) {
				mFullScreen = false;
				MoveTo(mOldWindowRect.left, mOldWindowRect.top);
				ResizeTo(mOldWindowRect.Width(), mOldWindowRect.Height());

				if (gPreferences.FindBool("DesktopMode")) {
					SetLook(B_DOCUMENT_WINDOW_LOOK);
					SetFeel(B_NORMAL_WINDOW_FEEL);
					SetFlags(Flags() & ~(B_AVOID_FRONT | B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE));
					float menuHeight = 0.0;
					if (mMenuBar) {
						mMenuBar->Show();
						menuHeight = mMenuBar->Frame().Height();
					}
	
					BRect baseFrame = Bounds();
					baseFrame.top = menuHeight + 1;
					mBaseView->MoveTo(baseFrame.LeftTop());
					mBaseView->ResizeTo(baseFrame.Width(), baseFrame.Height());
					gPreferences.ReplaceBool("FullScreen", false);
				} else
					SetLook(B_DOCUMENT_WINDOW_LOOK);

				if (mViewMenu)
					mViewMenu->FindItem(kFullScreenCommandTitle)->SetMarked(false);
			} else {
				mFullScreen = true;
				mOldWindowRect = Frame();
				MoveTo(0,0);
				BRect r = BScreen().Frame();
				ResizeTo(r.Width(), r.Height());

				if (gPreferences.FindBool("DesktopMode")) {
					SetLook(B_NO_BORDER_WINDOW_LOOK);
					SetFeel(window_feel(1024));
					SetFlags(Flags() | B_AVOID_FRONT | B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE);
					if (mMenuBar)
						mMenuBar->Hide();
					BRect baseFrame = Bounds();
					mBaseView->MoveTo(baseFrame.LeftTop());
					mBaseView->ResizeTo(baseFrame.Width(), baseFrame.Height());
					gPreferences.ReplaceBool("FullScreen", true);
				} else
					SetLook(B_TITLED_WINDOW_LOOK);

				if (mViewMenu)
					mViewMenu->FindItem(kFullScreenCommandTitle)->SetMarked(true);
			}
			break;
			
			
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

void HTMLWindow::MenusBeginning()
{
	BWindow::MenusBeginning();
}

void HTMLWindow::WindowActivated(bool status)
{
	if (status) {
		sWindowList.RemoveItem(this);
		sWindowList.AddItem(this, 0);
	}
}


HTMLWindow*	HTMLWindow::FrontmostHTMLWindow()
{
	if (sWindowList.CountItems() > 0)
		return (HTMLWindow*)sWindowList.ItemAt(0);
	else
		return NULL;
}


//	Send copy to html view?

bool HTMLWindow::QuitRequested()
{
	//if the app already knows it is quitting, don't send this message
	if(dynamic_cast<NetPositive*>(be_app)->IsQuitting() == false){
		BMessage msg(msg_WindowClosing);
		BMessage replyMsg;
		msg.AddPointer("window", this);
		if (!gPreferences.FindBool("DesktopMode"))
			msg.AddBool("FullScreen", mFullScreen);

	//use a Messenger to ensure that msg gets there
		BMessenger mMessenger(be_app);
		mMessenger.SendMessage(&msg, &replyMsg);
	}
	
	if (!mFullScreen && gPreferences.FindBool("SaveWindowPos") && mSaveWindowPos)
		gPreferences.ReplaceRect("DefaultBrowserWindowRect", Frame());
	return BWindow::QuitRequested();
}


void
HTMLWindow::SendMessageToHTMLViews(
	uint32	message,
	BView	*top)
{
	int32 numViews = (top != NULL) ? top->CountChildren() : CountChildren();
	for (int32 i = 0; i < numViews; i++)
		SendMessageToHTMLViews(message, (top != NULL) ? top->ChildAt(i) : ChildAt(i));

	if (top != NULL) {
		if (dynamic_cast<HTMLView *>(top) != NULL)
			PostMessage(message, top);
	}
}


LinkHistory::LinkHistory()
{
	mHistory = NULL;
	mBaseView = NULL;
}


LinkHistory::~LinkHistory()
{
	if (mHistory)
		mHistory->DeleteAll();
}


void LinkHistory::LoadPageFromHistory(PageHistory *p)
{
	mHistory = p;

	HTMLView *htmlView = mBaseView->GetTopHTMLView();	
	if (!htmlView)
		return;
	
	const char *target = mHistory->Target();
	pprint("Load from History url = %s  target = %s  htmlView->FrameList() = 0x%x  mHistory->FrameList() = 0x%x", mHistory->URL(), target, htmlView->GetFrameList(), mHistory->GetFrameList());

	FrameList *histFrames = mHistory->GetFrameList();
	FrameList *curFrames = htmlView->GetFrameList();
	
	if ((histFrames && !curFrames) ||
		(histFrames && curFrames && !(*curFrames == *histFrames))) {
		pprint("Rebuilding frame from scratch");

		FrameItem *frame = histFrames->FindFrame(target);
		if (frame)
			frame->SetURL(mHistory->URL());

		if (!gRunningAsReplicant && (!gWindowTitle || !*gWindowTitle)) {
			htmlView->Window()->SetTitle(mHistory->GetTitle());
		}
		htmlView->SetScrolling(kNoScrolling);
		htmlView->Reset(false);
		htmlView->KillChildren();
#ifdef JAVASCRIPT
		htmlView->CreateBlankDocument();
#endif
		htmlView->DestroyScrollBar(B_VERTICAL);
		htmlView->DestroyScrollBar(B_HORIZONTAL);
		htmlView->SetupFrames(mHistory->GetFrameList(), htmlView->Frame(), 0, true);
		htmlView->ScrollTo(BPoint(0,0));
		BString url(mHistory->GetFrameList()->URL());
		htmlView->SetURLText(url,false);
	} else if (target && *target) {
		pprint("Recycling existing frame");
		BMessage	msg(HTML_MSG + HM_ANCHOR);
		msg.AddString("url", mHistory->URL());
		msg.AddString("target", target);
		msg.AddBool("forceCache", true);
		htmlView->Window()->PostMessage(&msg, htmlView);
	} else {
		pprint("Non-frame");
		BString url(mHistory->URL());
		htmlView->NewHTMLDocFromURL(url, false, NULL, true, false);
	}
}


void LinkHistory::AddToHistory(const BString &URL, const char *targetName, FrameList *frames, Document *doc)
{
	HTMLView *htmlView = mBaseView->GetTopHTMLView();	
	if (!htmlView)
		return;

	pprint("AddToHistory: %s : %s : 0x%x\n", URL.String(), (const char *)targetName, frames);
	if (frames == NULL && htmlView->GetFrameList() && targetName && *targetName) {
		pprint("Adding framelist 0x%x to history", htmlView->GetFrameList());
		frames = htmlView->GetFrameList();
	}	
	if (mHistory == NULL)
		mHistory = new PageHistory(URL, targetName, frames, doc);
	else
		mHistory = mHistory->AddPage(URL, targetName, frames, doc);
}

bool LinkHistory::CanGoBack()
{
	return mHistory && mHistory->GetBack() != NULL;
}

bool LinkHistory::CanGoForward()
{
	return mHistory && mHistory->GetForw() != NULL;
}

void LinkHistory::RemoveCurrentPage()
{
	if (mHistory) {
		PageHistory *currentPage = mHistory;
		mHistory = (PageHistory*) mHistory->Previous();
		currentPage->Remove();
		delete currentPage;
	}
}

void LinkHistory::Rewind()
{
	while (mHistory && mHistory->GetBack())
		mHistory = mHistory->GetBack();
}


void LinkHistory::GoBack()
{
	if (mHistory) {
		PageHistory *p = mHistory->GetBack();
		if (p != NULL)
			LoadPageFromHistory(p);
	}
}


void LinkHistory::GoForward()
{
	if (mHistory) {
		PageHistory *p = mHistory->GetForw();
		if (p != NULL)
			LoadPageFromHistory(p);
	}
}

int32 LinkHistory::CountItems()
{
	PageHistory *p = mHistory;
	PageHistory *p2;
	while ((p2 = p->GetBack()))
		p = p2;
	int i = 0;
	while (p) {
		p = p->GetForw();
		i++;
	}
	return i;
}

PageHistory* LinkHistory::ItemAt(int32 index)
{
	PageHistory *p = mHistory;
	PageHistory *p2;
	while ((p2 = p->GetBack()))
		p = p2;
	int i = 0;
	while (p && i < index) {
		p = p->GetForw();
		i++;
	}
	if (i == index)
		return p;
	else
		return NULL;
}
