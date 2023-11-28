// ===========================================================================
//	JSInterface.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifdef JAVASCRIPT

#include "jseopt.h"
#include "seall.h"
#include "sebrowse.h"
#include "HTMLWindow.h"
#include "URL.h"
#include "HTMLView.h"
#include "UResource.h"
#include "Form.h"
#include "ImageGlyph.h"
#include "HTMLTags.h"
#include "InputGlyph.h"
#include "Strings.h"
#include "BeInput.h"
#include "Cookie.h"
#include "NetPositive.h"
#include "AnchorGlyph.h"
#include "Parser.h"
#include "URLView.h"
#include "NPApp.h"

#include <Alert.h>
#include <TextControl.h>
#include <Screen.h>

void browserSetDocumentVariable(jseContext jsecontext, jseVariable docVar, struct BrowserDocument *doc)
{
	Document *htmlDoc = (Document *)doc;
	htmlDoc->SetDocumentVar(docVar);
}


class SetContainerArgs {
public:
	SetContainerArgs(ContainerUser *object, jseVariable container) : mObject(object), mContainer(container) {}

	ContainerUser *mObject;
	jseVariable mContainer;
};

int32 SetContainerEntry(void *args)
{
	SetContainerArgs *sca = (SetContainerArgs*)args;
	sca->mObject->SetContainer(sca->mContainer);
	delete sca;
	return 0;
}

static void SetContainer(ContainerUser *object, jseVariable container)
{
	SetContainerArgs *args = new SetContainerArgs(object, container);
	resume_thread(spawn_thread(SetContainerEntry, "SetContainer", B_NORMAL_PRIORITY + 3, args));
}

/* Return the top (active) window. */
struct BrowserWindow *browserGetTopWindow(jseContext jsecontext,
                                          struct BrowserWindow *current_window)
{
	if (gRunningAsReplicant) {
		HTMLView *view = current_window->view;
		while (view) {
			HTMLView *parent = dynamic_cast<HTMLView *>(view->Parent());
			if (parent)
				view = parent;
			else
				break;
		}
		return view->GetBrowserWindow();
	} else {
		HTMLWindow *w = HTMLWindow::FrontmostHTMLWindow();
		if (!w)
			return NULL;
		return w->MainHTMLView()->GetBrowserWindow();
	}
}


/* Return the parent of this window, or NULL if this is a top-level window. */
struct BrowserWindow *browserGetParentWindow(jseContext jsecontext,
                                             struct BrowserWindow *current_window)
{
	if (!current_window->doc)
		return current_window;
	Document *parent = current_window->doc->GetParent();
	if (!parent)
		return current_window;
		
	return parent->GetBrowserWindow();
/*
	HTMLView *view = current_window->view;
	HTMLView *parent = dynamic_cast<HTMLView*>(view->Parent());
	if (!parent)
		return view->GetBrowserWindow();
	return parent->GetBrowserWindow();
*/
}



/* Return the window that called 'open()' to open this window, or NULL if
 * none did (this is the only window for example.)
 */
struct BrowserWindow *browserGetOpenerWindow(jseContext jsecontext,
                                             struct BrowserWindow *current_window)
{
	HTMLView *view = current_window->view;
	if (view)
		return view->GetOpener();
	else
		return NULL;
}


/* Return the window's name */
BString browserGetNameOfWindow(jseContext jsecontext,
                                struct BrowserWindow *current_window)
{
	if (!current_window)
		return "";
		
	if (gRunningAsReplicant)
		return "";
		
	HTMLView *view = current_window->view;
	if (view->IsSubview())
		return (view->Name());
	BWindow *window = view->Window();
	if (!window)
		return "";
	return (window->Title());
}


/* Get the default value of the status line for this window */
BString browserGetDefaultStatus(jseContext jsecontext,
                                 struct BrowserWindow *current_window)
{
	HTMLView *view = current_window->view;
	return view->GetDefaultProgressStr();
}


/* Set the default value of the status line for this window */
void browserSetDefaultStatus(jseContext jsecontext,
                             struct BrowserWindow *current_window,
                             const jsechar *text)
{
	HTMLView *view = current_window->view;
	view->SetDefaultProgressStr(text);
}


/* Get the current value of the status line for this window. */
BString browserGetStatus(jseContext jsecontext,
                          struct BrowserWindow *current_window)
{
	HTMLView *view = current_window->view;
	return view->GetProgressStr();
}


/* Set the current value of the status line for this window */
void browserSetStatus(jseContext jsecontext,
                      struct BrowserWindow *current_window,
                      const jsechar *text)
{
	HTMLView *view = current_window->view;
	view->SetProgressStr(text, false);
}



/* These routine is called to retrieve the frames under a parent window. It
 * is initially called with NULL as the last_child, then is called iteratively
 * with the last_child being the last returned window. You return the first
 * (if NULL) else the next window in the list of frames. You return NULL when
 * there are no more frames (the first window can be NULL if there are no frames.)
 */
struct BrowserWindow *browserGetNextFrame(jseContext jsecontext,
                                          struct BrowserWindow *parent,
                                          struct BrowserWindow *last_child)
{
	if (!parent)
		return NULL;
	HTMLView *parentView = parent->view;
	HTMLView *childView = last_child ? last_child->view : NULL;
	
	int num = parentView->CountChildren();
	bool foundChild = (childView != NULL);
	for (int i = 0; i < num; i++) {
		HTMLView *curChild = dynamic_cast<HTMLView*>(parentView->ChildAt(i));
		if (curChild && foundChild)
			return curChild->GetBrowserWindow();
		else if (curChild == childView)
			foundChild = true;
	}
	return NULL;
}


/* Return the location magic cookie for this window */
struct BrowserLocation *browserGetLocation(jseContext jsecontext,
                                           struct BrowserWindow *current_window)
{
	HTMLView *view = current_window->view;
	return view->GetLocation();
}


/* Return the document magic cookie for this window */
struct BrowserDocument *browserGetDocument(jseContext jsecontext,
                                           struct BrowserWindow *current_window)
{
	// Hmm.  This is a dilemma.  What if the window has a frameset and more than
	// one document?  Well, we'll do what they ask and get the topmost document
	// for the window.
	
	// Windows may also be frames.  We'll have to deal with that later.
	HTMLView *view = current_window->view;
	if (!view)
		return NULL;
	if (view->GetDocument())
		return (BrowserDocument *)view->GetDocument();
	else
		return (BrowserDocument *)current_window->doc;
}


/* Display an alert and return - the alert should be non-modal (i.e. return
 * immediately while the alert is displayed in the background.)
 */
void browserDisplayAlertDialog(jseContext jsecontext,
                               struct BrowserWindow *current_window,
                               const jsechar *msg)
{
	BAlert *alert = new BAlert(kJavaScriptDialogTitle, msg, kOKButtonTitle);
	alert->Go(NULL);
}


class PromptAlert : public BAlert {
public:
					PromptAlert(const char *txt, BString *defaultValue);
	virtual void	Quit();
	
private:
	BTextControl	*mTextControl;
	BString			*mValue;
};

PromptAlert::PromptAlert(const char *txt, BString *value)
	: BAlert(kJavaScriptDialogTitle, txt, kCancelButtonTitle, kOKButtonTitle)
{
	ResizeTo(Frame().Width(), Frame().Height() + 20);
	BRect r = Bounds();
	r.left += 45;
	r.right -= 10;
	r.bottom -= 40;
	r.top = r.bottom - 20;
	mValue = value;
	mTextControl = new BTextControl(r, "", "", mValue->String(), NULL);
	mTextControl->SetDivider(0);
	mTextControl->SetLowColor(192,192,192);
	AddChild(mTextControl);
	mTextControl->MakeFocus();
}

void PromptAlert::Quit()
{
	*mValue = mTextControl->Text();
	BAlert::Quit();
}

void browserDisplayPromptDialog(jseContext jsecontext,
								struct BrowserWindow *current_window,
								const jsechar *msg,
								const jsechar *defaultValue,
								BString *result)
{
	*result = defaultValue;
	PromptAlert *alert = new PromptAlert(msg, result);
	if (alert->Go() == 0)
		// What are we supposed to do if the user cancels?
		*result = "";
}


/* Display an alert with 'OK' and 'CANCEL' buttons and wait for a response
 * (i.e. it should be modal.) Return a boolean indicating whether 'OK'
 * was selected.
 */
jsebool browserDisplayQuestionDialog(jseContext jsecontext,
                                     struct BrowserWindow *current_window,
                                     const jsechar *msg)
{
	BAlert *alert = new BAlert(kJavaScriptDialogTitle, msg, kOKButtonTitle, kCancelButtonTitle);
	return (alert->Go() == 0);
}


/* Make the given window lose the keyboard focus. If the window is a frame,
 * then give keyboard focus to the top level window containing the frame,
 * otherwise cycle the window to the bottom of the window stack.
 */
void browserBlurWindow(jseContext jsecontext,
                       struct BrowserWindow *current_window)
{
	HTMLView *view = current_window->view;
	if (!view)
		return;
	HTMLView *parent = dynamic_cast<HTMLView*>(view->Parent());
	if (parent)
		parent->MakeFocus(true);
	else
		view->Window()->Activate(false);
}


/* Give the keyboard focus to the given window. */
void browserGiveFocusToWindow(jseContext jsecontext,
                              struct BrowserWindow *current_window)
{
	HTMLView *view = current_window->view;
	if (!view)
		return;
	view->Window()->Activate(true);
	view->MakeFocus(true);
}


/* Opens a new window and returns a handle to it. The url will always
 * be a string, but it may be blank (i.e. ""). The name and features arguments
 * will be NULL if not included. 'replace' defaults to False. Look up the
 * expected behavior in a Javascript book, as it is complex and lengthy.
 */
struct BrowserWindow *browserOpenWindow(jseContext jsecontext,
                                        struct BrowserWindow *opener,
                                        const jsechar *url,
                                        const jsechar *name,
                                        const jsechar *features,
                                        jsebool replace)
{
	HTMLView *view = opener->view;
	URLParser base;
	if (view) {
		if (!view->AllowPopupWindow()) {
			printf("Die!  Popup window rejected!\n");
			return NULL;
		}
		base.SetURL(view->GetDocumentURL());
	}
	URLParser newURL;
	newURL.SetURL(url);
	BString fullURL;
	newURL.BuildURL(fullURL, base);

	int32 width = 0;
	int32 height = 0;
	int32 x = 0;
	int32 y = 0;
	bool resizable = !(features && *features);
	bool showDecorations = !(features && *features);

	// Parse up the features.
	const jsechar *currentFeature = features;
	while (currentFeature && *currentFeature) {
		const char *equalsPos = strchr(currentFeature, '=');
		const char *endPos = strchr(currentFeature, ',');
		if (equalsPos) {
			BString attr = currentFeature;
			attr.Truncate(equalsPos - currentFeature);

			BString value = equalsPos + 1;
			if (endPos)
				value.Truncate(endPos - equalsPos - 1);
							
#warning Not implemented properly
			if (attr.ICompare("alwaysLowered") == 0) {
				// (JavaScript 1.2) If yes, creates a new window that floats below other windows,
				// whether it is active or not. This is a secure feature and must be set in signed
				// scripts. 
			} else if (attr.ICompare("alwaysRaised") == 0) {
				// (JavaScript 1.2) If yes, creates a new window that floats on top of other windows,
				// whether it is active or not. This is a secure feature and must be set in signed
				// scripts. 
			} else if (attr.ICompare("dependent") == 0) {
				// (JavaScript 1.2) If yes, creates a new window as a child of the current window.
				// A dependent window closes when its parent window closes. On Windows platforms,
				// a dependent window does not show on the task bar. 
			} else if (attr.ICompare("directories") == 0) {
				// If yes, creates the standard browser directory buttons, such as What's New and
				// What's Cool.
			} else if (attr.ICompare("height") == 0) {
				// (JavaScript 1.0 and 1.1) Specifies the height of the window in pixels.
				height = atoi(value.String());
				if (height < 100)
					height = 0;
			} else if (attr.ICompare("hotkeys") == 0) {
				// (JavaScript 1.2) If no (or 0), disables most hotkeys in a new window that has
				// no menu bar. The security and quit hotkeys remain enabled. 
			} else if (attr.ICompare("innerHeight") == 0) {
				// (JavaScript 1.2) Specifies the height, in pixels, of the window's content area.
				// To create a window smaller than 100 x 100 pixels, set this feature in a signed
				// script. This feature replaces height, which remains for backwards compatibility.
				height = atoi(value.String());
				if (height < 100)
					height = 0;
			} else if (attr.ICompare("innerWidth") == 0) {
				// (JavaScript 1.2) Specifies the width, in pixels, of the window's content area.
				// To create a window smaller than 100 x 100 pixels, set this feature in a signed
				// script. This feature replaces width, which remains for backwards compatibility.
				width = atoi(value.String());
				if (width < 100)
					width = 0;
			} else if (attr.ICompare("location") == 0) {
				// If yes, creates a Location entry field.
				if (value.ICompare("no") == 0 || value == "0")
					showDecorations = false;
			} else if (attr.ICompare("menubar") == 0) {
				// If yes, creates the menu at the top of the window.
				if (value.ICompare("no") == 0 || value == "0")
					showDecorations = false;
			} else if (attr.ICompare("outerHeight") == 0) {
				// (JavaScript 1.2) Specifies the vertical dimension, in pixels, of the outside
				// boundary of the window. To create a window smaller than 100 x 100 pixels, set
				//this feature in a signed script.
				height = atoi(value.String());
				if (height < 100)
					height = 0;
			} else if (attr.ICompare("outerWidth") == 0) {
				width = atoi(value.String());
				if (width < 100)
					width = 0;
			} else if (attr.ICompare("resizable") == 0) {
				// If yes, allows a user to resize the window.
				if (value.ICompare("no") == 0 || value == "0")
					resizable = false;
			} else if (attr.ICompare("screenX") == 0) {
				// (JavaScript 1.2) Specifies the distance the new window is placed from the left
				// side of the screen. To place a window offscreen, set this feature in a signed
				// scripts.
				x = atoi(value.String());
				x = MAX(x, 0);
				x = MIN(x, (int32)(BScreen(B_MAIN_SCREEN_ID).Frame().Width() - 50));
			} else if (attr.ICompare("screenY") == 0) {
				// (JavaScript 1.2) Specifies the distance the new window is placed from the top
				// of the screen. To place a window offscreen, set this feature in a signed scripts. 
				y = atoi(value.String());
				y = MAX(y, 0);
				y = MIN(y, (int32)(BScreen(B_MAIN_SCREEN_ID).Frame().Height()- 50));
			} else if (attr.ICompare("scrollbars") == 0) {
				// If yes, creates horizontal and vertical scrollbars when the Document grows larger
				// than the window dimensions.
				if (value.ICompare("no") == 0 || value == "0")
					resizable = false;
			} else if (attr.ICompare("status") == 0) {
				// If yes, creates the status bar at the bottom of the window.
				if (value.ICompare("no") == 0 || value == "0")
					showDecorations = false;
			} else if (attr.ICompare("titlebar") == 0) {
				// (JavaScript 1.2) If yes, creates a window with a title bar. To set the titlebar
				// to no, set this feature in a signed script.
			} else if (attr.ICompare("toolbar") == 0) {
				// If yes, creates the standard browser toolbar, with buttons such as Back and Forward.
				if (value.ICompare("no") == 0 || value == "0")
					showDecorations = false;
			} else if (attr.ICompare("width") == 0) {
				// (JavaScript 1.0 and 1.1) Specifies the width of the window in pixels.
				width = atoi(value.String());
				if (width < 100)
					width = 0;
			} else if (attr.ICompare("z-lock") == 0) {
				// (JavaScript 1.2) If yes, creates a new window that does not rise above other
				// windows when activated. This is a secure feature and must be set in signed scripts.
			}
		}
		if (endPos)
			currentFeature = endPos + 1;
		else
			currentFeature = NULL;
	}
	
	BRect frame;
	gPreferences.FindRect("DefaultBrowserWindowRect", &frame);
	PositionWindowRect(&frame);
	
	if (x > 0)
		frame.left = x;
	if (y > 0)
		frame.top = y;
	if (width > 0)
		frame.right = frame.left + width;
	if (height > 0)
		frame.bottom = frame.top + height;

	if (showDecorations)
		frame.bottom += 60;
	if (resizable)
		frame.right += B_V_SCROLL_BAR_WIDTH;
		
	HTMLWindow *window = (HTMLWindow*)NetPositive::NewWindowFromURL(fullURL, N_USE_DEFAULT_CONVERSION, NULL, false, showDecorations, showDecorations, &frame, resizable);
	HTMLView *returnView = window->MainHTMLView();
	returnView->SetOpener(opener);
	return returnView->GetBrowserWindow();
}


/* Close the given window. In Netscape, only windows opened via Javascript
 * can be closed in this manner, and you may want to make the same rule.
 * Even though the window is closed, the magic cookie 'current_window' cannot
 * be discarded, but should be marked as 'closed'. These magic cookies
 * must remain until the browser exits.
 */
void browserCloseWindow(jseContext jsecontext,
                        struct BrowserWindow *current_window)
{
	HTMLView *view = current_window->view;
	if (!view)
		return;
	view->Window()->Close();
}


/* This returns whether or not the given window has been closed. */
jsebool browserIsWindowClosed(jseContext jsecontext,
                              struct BrowserWindow *current_window)
{
	return current_window->open;
}


/* Scroll the given window to the given location. The coordinate system
 * is undefined (most likely in pixels), but 0,0 is the top-left corner.
 * Note that the given location is absolute, not relative.
 */
void browserScrollWindow(jseContext jsecontext,
                         struct BrowserWindow *current_window,
                         ulong x, ulong y)
{
	HTMLView *view = current_window->view;
	if (!view)
		return;
	view->ScrollTo(x,y);
}


void browserMoveWindowBy(jseContext jsecontext,
                         struct BrowserWindow *current_window,
                         long x, long y)
{
	HTMLView *view = current_window->view;
	if (!view)
		return;
	view->Window()->MoveBy(x, y);
}

/* Like typing in a new URL in the window's URL entry field. If replace is TRUE,
 * then replace the history with the new document rather than appending it.
 */
void browserGotoURL(jseContext jsecontext,
                    struct BrowserLocation *location,
                    const jsechar *url_text,
                    jsebool replace)
{
#warning Not_implemented_properly
	HTMLView *view = location->view;
	BString fullURL;

//	while (view && dynamic_cast<HTMLView *>(view->Parent()))
//		view = dynamic_cast<HTMLView *>(view->Parent());
	if (!view)
		return;
	Document *doc = view->GetDocument();
	while (doc && doc->GetParent())
		doc = doc->GetParent();
	if (!doc)
		return;

	doc->Lock();
	doc->ResolveURL(fullURL, url_text);
	doc->Unlock();
	view->NewHTMLDocFromURL(fullURL);
}


/* Fill in the URL structure with the values for the given location */
void browserGetLocationValue(jseContext jsecontext,
                             struct BrowserLocation *location,
                             struct URL *fill_me_in,
							 jseVariable anchorContainer)
{
	URLParser parser;
	parser.SetURL(location->location.String());
	
	if (anchorContainer && location->glyph)
		SetContainer(location->glyph, anchorContainer);
//		location->glyph->SetContainer(anchorContainer);

	switch(parser.Scheme()) {
		default:
		case kUNKNOWNSCHEME:
		case kNOSCHEME:
			fill_me_in->protocol = "";
			break;
		case kNETPOSITIVE:
			fill_me_in->protocol = "netpositive";
			break;
		case kFILE:
			fill_me_in->protocol = "file";
			break;
		case kFTP:
			fill_me_in->protocol = "ftp";
			break;
		case kHTTP:
			fill_me_in->protocol = "http";
			break;
		case kHTTPS:
			fill_me_in->protocol = "https";
			break;
		case kNNTP:
			fill_me_in->protocol = "nntp";
			break;
		case kMAILTO:
			fill_me_in->protocol = "mailto";
			break;
		case kTELNET:
			fill_me_in->protocol = "telnet";
			break;
		case kGOPHER:
			fill_me_in->protocol = "gopher";
			break;
		case kJAVASCRIPT:
			fill_me_in->protocol = "javascript";
			break;
	}

	// We'll have to change how this works so that the caller has more ownership of the string storage.
	fill_me_in->port = parser.HostPort();
	fill_me_in->pathname = parser.Path();
	fill_me_in->search = parser.Query();
	fill_me_in->hash = parser.Fragment();
	if (location->glyph)
		fill_me_in->target = location->glyph->GetTarget();
	else
		fill_me_in->target = "";
	fill_me_in->hostname = parser.HostName();
	fill_me_in->host = parser.HostName();
	fill_me_in->host += ':';
	fill_me_in->host += parser.HostPort();
	parser.WriteURL(fill_me_in->href);
}


/* Change the given location to the values in the structure. This really
 * means change the location, so for example if the 'location' magic cookie
 * represents the window's current URL, the window must be updated to point
 * at the new URL. If the location is for a link, do what is appropriate.
 */
void browserSetLocationValue(jseContext jsecontext,
                             struct BrowserLocation *location,
                             struct URL *fill_me_in)
{
	BString fullURL;

	Document *doc = NULL;
	if (location->view)
		doc = location->view->GetDocument();
	if (!doc && location->glyph)
		doc = location->glyph->GetDocument();
		
	while (doc && doc->GetParent())
		doc = doc->GetParent();
	if (!doc) {
		return;
	}
		
	doc->Lock();
	doc->ResolveURL(fullURL, fill_me_in->href.String());
	doc->Unlock();
	
	if (location->glyph) {
		location->glyph->SetHREF(fullURL.String());
	} else if (location->view) {
		location->view->NewHTMLDocFromURL(fullURL.String());
	}
	location->location = fullURL;
}


/* This routine iterates over the links in a document. See browserGetNextFrame()
 * on how it works.
 */
struct BrowserLocation *browserGetNextLink(jseContext jsecontext,
                                           struct BrowserDocument *in_me,
                                           struct BrowserLocation *last_child)
{
	Document *doc = (Document *)in_me;
	AnchorGlyph *glyph = NULL;
	if (last_child)
		glyph = last_child->glyph;
	glyph = doc->GetNextAnchor(glyph);
	if (glyph)
		return glyph->GetLocation();
	else
		return NULL;
}


/* Get the number of entries in the given window's history list. */
ulong browserGetHistoryLength(jseContext jsecontext,
                              struct BrowserWindow *current_window)
{
	HTMLView *view = current_window->view;
	if (!view)
		return 0;
	return view->GetHistoryLength();
}


/* Go to a location relative to the current one. The offset species the location.
 * offset of '-1' means go back one, '1' means forward one, etc.
 */
void browserHistoryGo(jseContext jsecontext,
                      struct BrowserWindow *current_window,
                      long offset)
{
	HTMLView *view = current_window->view;
	if (!view)
		return;
	view->Window()->PostMessage(offset < 0 ? B_NETPOSITIVE_BACK : B_NETPOSITIVE_FORWARD);
}


/* This routine iterates over the forms in a document. See browserGetNextFrame()
 * on how it works.
 */
struct BrowserForm *browserGetNextForm(jseContext jsecontext,
                                       struct BrowserDocument *in_me,
                                       struct BrowserForm *last_child)
{
	Document *doc = (Document *)in_me;
	Form *form = (Form *)last_child;
	return (BrowserForm *)doc->GetNextForm(form);
}


/* This routine iterates over the images in a document. See browserGetNextFrame()
 * on how it works.
 */
struct BrowserImage *browserGetNextImage(jseContext jsecontext,
                                         struct BrowserDocument *in_me,
                                         struct BrowserImage *last_child)
{
	Document *doc = (Document *)in_me;
	ImageGlyph *image = (ImageGlyph *)last_child;
	return (BrowserImage *)doc->GetNextImage(image);
}


/* Fill in the Image structure with the information about the given image
 * magic cookie.
 */
void browserGetImage(jseContext jsecontext,
                     struct BrowserImage *image,
                     struct SEImage *fill_in)
{
	ImageGlyph *glyph = (ImageGlyph *)image;
	if (!glyph) {
		fill_in->lowsrc = "";
		fill_in->src = "";
		fill_in->name = "";
		return;
	}

	fill_in->border = glyph->GetBorder();
	fill_in->complete = glyph->GetComplete();
	fill_in->height = (uint)glyph->GetHeight();
	fill_in->hspace = glyph->GetHSpace();
	// NetPositive does not support lowsrc (which is an alternate low-resolution version of the image).
	fill_in->lowsrc = "";
	fill_in->name = glyph->GetNameAttr();
	fill_in->src = glyph->GetSRC();
	fill_in->vspace = glyph->GetVSpace();
	fill_in->width = (uint)glyph->GetWidth();
}


/* Update using the Image structure with the information about the given image
 * magic cookie.
 */
void browserSetImage(jseContext jsecontext,
                     struct BrowserImage *image,
                     struct SEImage *update_to_match)
{
	if (!image)
		return;
	ImageGlyph *glyph = (ImageGlyph *)image;
	glyph->SetBorder(update_to_match->border);
	glyph->SetHeight(update_to_match->height);
	glyph->SetHSpace(update_to_match->hspace);
	glyph->SetNameAttr(update_to_match->name.String());
#warning Not_implemented_correctly
	HTMLView *view = (HTMLView *)jseGetLinkData(jsecontext);
	BString fullURL;

	Document *doc = view->GetDocument();
	while (doc && doc->GetParent())
		doc = doc->GetParent();
	if (!doc)
		return;

	doc->Lock();
	doc->ResolveURL(fullURL, update_to_match->src.String());
	doc->Unlock();
	glyph->SetSRC(fullURL.String());
	glyph->SetVSpace(update_to_match->vspace);
	glyph->SetWidth(update_to_match->width);
	glyph->Unload();
	doc->LoadImage(glyph);
printf("browserSetImage invalidate\n");
	view->Invalidate(BRect(glyph->GetLeft(), glyph->GetTop(), glyph->GetLeft() + glyph->GetWidth(), glyph->GetTop() + glyph->GetHeight()));
}

// ----------------------------------------------------------------------


/* Sets one of the document color properties. See a Javascript book for
 * documentation on the format of the 'color_value' string - you can
 * actually implement it any way you like, as this value is just passed along.
 */
void browserSetDocumentColor(jseContext jsecontext,
                             struct BrowserDocument *document,
                             enum DocumentColors color,
                             const jsechar *color_value)
{
	long colorVal = ParseColor(color_value);
	Document *doc = (Document*)document;
	if (!doc)
		return;
	switch(color) {
		case AlinkColor:	doc->SetALinkColor(colorVal);	break;
		case LinkColor:		doc->SetLinkColor(colorVal);	break;
		case VlinkColor:	doc->SetVLinkColor(colorVal);	break;
		case BgColor:		doc->SetBGColor(colorVal);		break;
		case FgColor:		doc->SetFGColor(colorVal);		break;
		default:											break;
	}
}


/* Gets one of the document color properties */
BString browserGetDocumentColor(jseContext jsecontext,
                                       struct BrowserDocument *document,
                                       enum DocumentColors color)
{
	long retColor;
	Document *doc = (Document*)document;
	if (!doc)
		return "#000000";
		
	switch(color) {
		case AlinkColor:	retColor = doc->GetALinkColor();	break;
		case LinkColor:		retColor = doc->GetLinkColor();		break;
		case VlinkColor:	retColor = doc->GetVLinkColor();	break;
		case BgColor:		retColor = doc->GetBGColor();		break;
		case FgColor:		retColor = doc->GetFGColor();		break;
		default:			retColor = 0;						break;
	}
	
	char buffer[16];
	sprintf(buffer, "#%06X", (unsigned int)(retColor & 0x00ffffff));

	// Is it kosher to return a pointer to bits on the stack?  Probably not.
	return buffer;
}


/* Opens the document so to write new contents to it, erasing any old
 * contents.
 */
void browserOpenDocument(jseContext jsecontext,
                         struct BrowserDocument *document,
                         const jsechar *mimetype)
{
	Document *doc = (Document*)document;
	if (!doc)
		return;
	doc->Lock();
	doc->Reopen();
	doc->Unlock();
}


/* Writes some text to the document. If the document is not open,
 * implictly do an open for "text/html".
 */
void browserDocumentWrite(jseContext jsecontext,
                          struct BrowserDocument *document,
                          const jsechar *text)
{
	Document *doc = (Document*)document;
	if (!doc)
		return;
	if (!doc->IsOpen()) {
		browserOpenDocument(jsecontext, document, "text/html");
		return;
	}
	Parser *parser = dynamic_cast<Parser *>(doc->GetParser());
	if (!parser)
		return;

	// The ReentrantWrite routine stores the data temporarily instead
	// of parsing it on the spot, so we'll be sure that the JavaScript
	// engine isn't called re-entrantly.
	parser->ReentrantWrite((uchar *)text, strlen(text));
}


/* An open document is an output stream for text. When it is closed,
 * all the text has been output and it can be displayed.
 */
void browserCloseDocument(jseContext jsecontext,
                          struct BrowserDocument *document)
{
	Document *doc = (Document*)document;
	if (!doc)
		return;
	if (doc->IsOpen())
		doc->Finalize();
}


/* Gets the values of the cookies for this document in the form
 * "name=value;name=value;..."
 */
BString browserGetCookie(jseContext jsecontext,
                                struct BrowserDocument *document)
{
#warning Not_implemented_correctly
	HTMLView *view = (HTMLView *)jseGetLinkData(jsecontext);
	if (!view)
		return "";
	BString cookies;
	URLParser parser;
	parser.SetURL(view->GetDocumentURL());
	Cookie::Add(parser, cookies);
	
	return cookies;
}


/* Sets the cookie value by adding a single "name=value" item which must
 * be integrated into the cookie list. It can have an 'expires' clause,
 * 'path' clause, 'domain' clause, and 'secure' clause.
 */
void browserSetCookie(jseContext jsecontext,
                                struct BrowserDocument *document,
                                const jsechar *cookie)
{
#warning Not_implemented_correctly
	HTMLView *view = (HTMLView *)jseGetLinkData(jsecontext);
	URLParser parser;
	parser.SetURL(view->GetDocumentURL());
	Cookie::Set(parser, cookie);
}


/* Returns a text string which is the date of the last modified field
 * or Jan 1st, 1970 (midnight) if none. The date should be in the format:
 *
 * 'Wed Dec 3 12:46:52 1997'
 */
TLocker unixLock("Unix Lock");

BString browserGetLastModified(jseContext jsecontext,
                                      struct BrowserDocument *document)
{
#warning Not_implemented_correctly
	HTMLView *view = (HTMLView *)jseGetLinkData(jsecontext);
	if (!view)
		return "Mon Jan 1 00:00:00 1970";
	long lastModified = view->GetLastModified();
	char buffer[100];
	unixLock.Lock();
	struct tm* t = localtime(&lastModified);
	strftime(buffer, 100, "%a %b %d %H:%M:%s %Y", t);
	unixLock.Unlock();
	
	// Is it kosher to return the string this way?
	return buffer;
}


/* Return the location magic cookie for this document */
struct BrowserLocation *browserGetDocumentLocation(jseContext jsecontext,
                                                   struct BrowserDocument *document)
{
	Document *htmlDoc = (Document*)document;
	if (!htmlDoc)
		return NULL;
	return browserGetLocation(jsecontext, htmlDoc->GetBrowserWindow());
}


/* Get a string representing the complete URL of the referring document,
 * or the empty string if none.
 */
BString browserGetReferrer(jseContext jsecontext,
                                  struct BrowserDocument *document)
{
#warning Not_implemented_correctly
	HTMLView *view = (HTMLView *)jseGetLinkData(jsecontext);
	if (!view)
		return "";
	return view->GetReferrer();
}


/* Get a string representing the title of the document. */
BString browserGetTitle(jseContext jsecontext,
                               struct BrowserDocument *document)
{
	Document *doc = (Document*)document;
	if (!doc)
		return "";
	return doc->GetTitle();
}


/* Reset all the fields of the form. */
void browserFormReset(jseContext jsecontext,
                      struct BrowserForm *form)
{
#warning Not_implemented_properly
	HTMLView *view = (HTMLView *)jseGetLinkData(jsecontext);
	if (!view)
		return;
	((Form *)form)->Reset(view->GetDocument()->GetDrawPort());
}


/* Submit the form. */
void browserFormSubmit(jseContext jsecontext,
                       struct BrowserForm *form)
{
#warning Not_implemented_properly
	HTMLView *view = (HTMLView *)jseGetLinkData(jsecontext);
	BMessage msg(FORM_MSG + AV_SUBMIT);
	msg.AddPointer("Form", form);
	view->Window()->PostMessage(&msg, view);
//	((Form *)form)->Submit(NULL);
}


/* Fill in the Form structure with the values from this form. */
void browserGetForm(jseContext jsecontext,
                    struct BrowserForm *form,
                    struct SEForm *fill_in,
					jseVariable formContainer)
{
	Form *theForm = (Form *)form;
	
	if (formContainer)
		SetContainer(theForm, formContainer);
//		theForm->SetContainer(formContainer);
	
	fill_in->action = theForm->Action();
	fill_in->encoding = theForm->Enctype();
	fill_in->method = theForm->Method();
	fill_in->target = theForm->Target();
	fill_in->name = theForm->Name();
}


/* Update the form with any changes in the Form structure. */
void browserSetForm(jseContext jsecontext,
                    struct BrowserForm *form,
                    struct SEForm *read_changes_from)
{
	Form *theForm = (Form *)form;
	if (!theForm)
		return;
	theForm->SetAction(read_changes_from->action.String());
	theForm->SetEnctype(read_changes_from->encoding.String());
	theForm->SetMethod(read_changes_from->method.String());
	theForm->SetTarget(read_changes_from->target.String());
	theForm->SetName(read_changes_from->name.String());
}


/* This routine iterates over the elements in a form. See browserGetNextFrame()
 * on how it works.
 */
struct BrowserElement *browserGetNextElement(jseContext jsecontext,
                                             struct BrowserForm *in_me,
                                             struct BrowserElement *last_child)
{
	Form *form = (Form *)in_me;
	InputGlyph *element = (InputGlyph *)last_child;
	return (BrowserElement *)form->GetNextElement(element);
}


/* Fills in an Element structure based on the element. */
void browserGetElement(jseContext jsecontext,
                       struct BrowserElement *elem,
                       struct Element *fill_in,
					   jseVariable elementContainer)
{
	InputGlyph *glyph = (InputGlyph *)elem;
	if (elementContainer)
		SetContainer(glyph, elementContainer);
//		glyph->SetContainer(elementContainer);
	if (glyph->IsRadio() || glyph->IsCheckBox()) {
		fill_in->checked = glyph->GetCheck();
		fill_in->checked_used = true;
		fill_in->defaultChecked = glyph->GetOrigCheck();
		fill_in->dc_used = true;
	} else {
		fill_in->checked_used = false;
		fill_in->dc_used = false;
	}
	fill_in->defaultValue = glyph->GetOrigValue();
	fill_in->dv_used = true;
	if (glyph->IsSelect()) {
		BeSelect* select = (BeSelect*)glyph;
		fill_in->num_options = select->GetNumOptions();
		fill_in->options_used = true;
		fill_in->selectedIndex = select->GetSelectedIndex();
		fill_in->si_used = true;
	} else {
		fill_in->options_used = false;
		fill_in->si_used = false;
	}
	fill_in->value = glyph->GetValue();
	fill_in->value_used = true;
	if (glyph->IsTextField())
		fill_in->type = "text";
	else if (glyph->IsRadio())
		fill_in->type = "radio";
	else if (glyph->IsCheckBox())
		fill_in->type = "check";
	else if (glyph->IsReset())
		fill_in->type = "reset";
	else if (glyph->IsSubmit())
		fill_in->type = "submit";
	else if (glyph->IsPassword())
		fill_in->type = "password";
	else if (glyph->IsImage())
		fill_in->type = "image";
	else if (glyph->IsSelect())
		fill_in->type = "select";
	else if (glyph->IsHidden())
		fill_in->type = "hidden";
	fill_in->name = glyph->GetName();
}


/* Updates the element */
void browserSetElement(jseContext jsecontext,
                       struct BrowserElement *elem,
                       struct Element *fill_in)
{
	InputGlyph *glyph = (InputGlyph *)elem;
	if (glyph->IsRadio() || glyph->IsCheckBox()) {
		if (fill_in->checked_used)
			glyph->SetCheck(fill_in->checked, NULL);
//		if (fill_in->dc_used)
	}
	if (glyph->IsSelect()) {
		BeSelect* select = (BeSelect*)glyph;
//		if (fill_in->options_used)
		if (fill_in->si_used)
			select->SetSelectedIndex(fill_in->selectedIndex);
	}
	if (fill_in->value_used)
		glyph->SetValue(fill_in->value.String(), true);
//	if (fill_in->dv_used)
}


/* Standard element actions */
void browserElementBlur(jseContext jsecontext,
                        struct BrowserElement *elem)
{
	InputGlyph *glyph = (InputGlyph *)elem;
	glyph->Blur();
}

void browserElementClick(jseContext jsecontext,
                         struct BrowserElement *elem)
{
	InputGlyph *glyph = (InputGlyph *)elem;
	glyph->Click();
}

void browserElementFocus(jseContext jsecontext,
                         struct BrowserElement *elem)
{
	InputGlyph *glyph = (InputGlyph *)elem;
	glyph->Focus();
}

void browserElementSelect(jseContext jsecontext,
                          struct BrowserElement *elem)
{
	InputGlyph *glyph = (InputGlyph *)elem;
	glyph->Select();
}


/* Read the state of an option of an element (only applies to
 * select elements.)
 */
void browserGetElementOption(jseContext jsecontext,
                             struct BrowserElement *elem,
                             ulong index,
                             struct SEOption *fill_in)
{
	BeSelect *glyph = (BeSelect *)elem;
	Option *o = glyph->OptionAt(index);
	if (o) {
		fill_in->defaultSelected = o->mOn;
		fill_in->index = index;
		fill_in->selected = o->mSelected;
		fill_in->text = o->mOption.String();
		fill_in->value = o->mValue.String();
	}
}


/* Set the state of an option of an element (only applies to
 * select elements.) Note that if the Option's index is one past
 * the end of the array, a new options is being added and you
 * must allow that to work.
 */
void browserSetElementOption(jseContext jsecontext,
                             struct BrowserElement *elem,
                             struct SEOption *fill_in)
{
	BeSelect *glyph = (BeSelect *)elem;
	if (!fill_in)
		return;
	if (fill_in->index >= (uint)glyph->GetNumOptions()) {
		glyph->AddOption(fill_in->text.String(), true, fill_in->value.String(), fill_in->selected);
	} else {
		Option *o = glyph->OptionAt(fill_in->index);
		o->mOn = fill_in->defaultSelected;
		o->mSelected = fill_in->selected;
		o->mOption = fill_in->text;
		o->mValue = fill_in->value;
		glyph->UpdateOption(o);
	}
}

void browserReloadLocation(jseContext jsecontext,
                           struct BrowserLocation *location,
                           jsebool force,
                           jseVariable thisvar)
{
printf("browserReloadLocation\n");
#warning Not_implemented_yet
}

BString browserGetHistoryItem(jseContext jsecontext,
                                     struct BrowserWindow *current_window,
                                     ulong itemnum)
{
	HTMLView *view = current_window->view;
	if (!view)
		return "";
	return view->GetHistoryItem(itemnum);
}


void browserGetLayer(jseContext jsecontext,
					struct BrowserLayer *layer,
					struct SELayer *fill_in,
					jseVariable layerContainer)
{
	LayerItem *item = (LayerItem *)layer;
	
	fill_in->name = item->mName;
	fill_in->src = item->mSRC;
	switch(item->mVisibility) {
		case AV_HIDE:	fill_in->visibility = "hide"; break;
		case AV_SHOW:	fill_in->visibility = "show"; break;
	}
	fill_in->background = item->mBackground;
#warning Not implemented properly
//	fill_in->bgColor =
	fill_in->left = item->mLeft;
	fill_in->top = item->mTop;
	fill_in->zIndex = item->mZIndex;
// ...
}
					
void browserSetLayer(jseContext jsecontext,
					struct BrowserLayer *layer,
					struct SELayer *read_changed_from)
{
	LayerItem *item = (LayerItem *)layer;
	
//	item->mSRC = fill_in->src;
	if (read_changed_from->visibility == "hide")
		item->mVisibility = AV_HIDE;
	else if (read_changed_from->visibility == "show")
		item->mVisibility = AV_SHOW;

//	item->mBackground = fill_in->background;
#warning Not implemented properly
//	item->bgColor =
	item->mLeft = read_changed_from->left;
	item->mTop = read_changed_from->top;
	item->mZIndex = read_changed_from->zIndex;
// ...

	Document *doc = item->mDocument;
	DocAndWindowAutolock lock(doc);
	doc->UpdateLayer(item);
}

void browserLayerLoad(jseContext jsecontext,
						struct BrowserLayer *layer,
						const char *src,
						int width)
{
}
						
void browserLayerMoveBy(jseContext jsecontext,
						struct BrowserLayer *layer,
						int x, int y)
{
}

void browserLayerMoveTo(jseContext jsecontext,
						struct BrowserLayer *layer,
						int x, int y)
{
}

void browserLayerMoveToAbsolute(jseContext jsecontext,
						struct BrowserLayer *layer,
						int x, int y)
{
}

void browserLayerResizeBy(jseContext jsecontext,
						struct BrowserLayer *layer,
						int x, int y)
{
}

void browserLayerResizeTo(jseContext jsecontext,
						struct BrowserLayer *layer,
						int x, int y)
{
}

/* ---------------------------------------------------------------------- */


class TimeoutArgs {
public:
	TimeoutArgs(jseContext context, const jsechar* c, BrowserWindow* window, bigtime_t interval, thread_id id) :
		jsecontext(context), code(c), current_window(window), snoozeInterval(interval), tid(id) {}
		
	jseContext				jsecontext;
	BString					code;
	struct BrowserWindow*	current_window;
	bigtime_t				snoozeInterval;
	thread_id				tid;
};

int32 timeoutEntry(void *args)
{
	TimeoutArgs* ta = (TimeoutArgs*)args;
	snooze(ta->snoozeInterval);
	HTMLView *view = ta->current_window->view;
	if (!view || !ta->current_window->open) {
		delete ta;
		return 0;
	}
	view->GetDocument()->SetTimeoutThread(NULL);

	DocAndWindowAutolock lock(view->GetDocument());
	if (lock.IsLocked())
		view->GetDocument()->ExecuteScript(ta->code.String());
	
	delete ta;
	return 0;
}

/* You must handle timeouts. After the specified time has gone by, run
 * the Javascript code. You could run it in a new context (but with the
 * same current window) or you could run it in the original context
 * (but then you must wait for any currently executing code to finish.)
 */

/* Register some code to be called within the context of the given
 * window in a given number of milliseconds. The return is a magic handle
 * that can be used to cancel the timeout code. The code is to be executed
 * once when the timeout goes off.
 */
struct BrowserTimeout *browserSetTimeout(jseContext jsecontext,
                                         struct BrowserWindow *current_window,
                                         const jsechar *js_code,
                                         ulong timeout_in_millisecs)
{
	if (!current_window->open)
		return NULL;
	TimeoutArgs *ta = new TimeoutArgs(jsecontext, js_code, current_window, timeout_in_millisecs * 1000, 0);
	ta->tid = spawn_thread(timeoutEntry, "JavaScript timeout", B_NORMAL_PRIORITY, ta);
	resume_thread(ta->tid);
	NetPositive::AddThread(ta->tid);
	HTMLView *view = current_window->view;
	if (!view) {
		delete ta;
		return NULL;
	}
	Document *doc = view->GetDocument();
	if (!doc) {
		delete ta;
		return NULL;
	}
	doc->SetTimeoutThread((BrowserTimeout *)ta);
	return (BrowserTimeout *)ta;
}


/* Clear a previously-set timeout if possible. */
void browserClearTimeout(jseContext jsecontext,
                         struct BrowserWindow *current_window,
                         struct BrowserTimeout *tm)
{
// This maybe isn't a good idea, but since this routine is only called
// when the jseContext is locked, the timeout will never actually be in the engine;
// it will either be sleeping or waiting for the jseContext semaphore.
// So, we can just kill its thread, as it shouldn't do anything bad or
// leak memory.
	if (!tm || !current_window)
		return;
	if (tm != current_window->mTimeout)
		return;
	current_window->mTimeout = NULL;
	kill_thread(((TimeoutArgs *)tm)->tid);
	NetPositive::RemoveThread(((TimeoutArgs *)tm)->tid);
// Are we supposed to delete this?  Probably.
	delete (TimeoutArgs *)tm;
}

jsenumber jseGetNumber(jseContext jsecontext ,jseVariable variable)
{
   jsenumber GetFloat;
   jseGetFloatIndirect(jsecontext,variable,&GetFloat);
   return GetFloat;
}

void JSE_CFUNC FAR_CALL browserErrorMessageFunc(jseContext jseContext, const char *ErrorString)
{
	printf("JavaScript error: %s\n", ErrorString);
}

#endif
