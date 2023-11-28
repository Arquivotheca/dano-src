
#include "BrowserWindow.h"
#include "ViewTree.h"
#include <www/TellBrowser.h>

#include <Application.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <RadioButton.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <TextView.h>
#include <TraverseViews.h>
#include <stdio.h>
#include <OS.h>
#include <www/Content.h>
#include <www/Protocol.h>

#include "Wotan.h"

const float		kToolbarHeight = 30;
const uint32 	kSize = 'size';
const uint32	kReallyQuit = 'rqut';
const URL 		randomLinkServer("http://random.yahoo.com/bin/ryl");

// debugging commands
const uint32	kPrintHelp = 'phlp';
const uint32	kMarkLeakReport = 'mlrp';
const uint32	kPrintLastReport = 'plrp';
const uint32	kPrintCurrentReport = 'pcrp';
const uint32	kPrintAtomReport = 'parp';
const uint32	kFlushCache = 'flch';
const uint32	kPrintResources = 'prsr';

#define DOUBLE_BUFFER 1

using namespace Wagner;

BrowserWindow::BrowserWindow(BRect rect, browser_style s, bool quit, uint32 flags) :
	BWindow(rect, "Wagner:Untitled", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS),
	fBrowserStyle(s),
	fContentView(NULL),
	fFlags(flags),
	fCurrentAtomMark(0)
{
	if (!quit) RemoveShortcut('Q', B_COMMAND_KEY);
		
	uint32 extraFlags = 0;

	if (s == BS_ALERT)
	{
		SetLook(B_NO_BORDER_WINDOW_LOOK);
		SetFeel(B_MODAL_APP_WINDOW_FEEL);
		AddShortcut('W', B_COMMAND_KEY, new BMessage(B_CLOSE_REQUESTED));
		extraFlags |= cvoIsClosable;
	}
	else {
		if (s == BS_FULL_SCREEN)
		{
			SetLook((window_look) 4);
			SetFeel((window_feel) 1024);
			RemoveShortcut('W', B_COMMAND_KEY);	// Remove close shortcut
			rect = BScreen().Frame();
			Zoom(B_ORIGIN, rect.Width(), rect.Height());
		}
	}
	
	AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO), (BView *)NULL);

	// Shortcuts for debugging commands.
	if (quit) {
		AddShortcut('H', B_COMMAND_KEY|B_CONTROL_KEY|B_SHIFT_KEY,
					new BMessage(kPrintHelp), this);
		AddShortcut('M', B_COMMAND_KEY|B_CONTROL_KEY|B_SHIFT_KEY,
					new BMessage(kMarkLeakReport), this);
		AddShortcut('L', B_COMMAND_KEY|B_CONTROL_KEY|B_SHIFT_KEY,
					new BMessage(kPrintLastReport), this);
		AddShortcut('C', B_COMMAND_KEY|B_CONTROL_KEY|B_SHIFT_KEY,
					new BMessage(kPrintCurrentReport), this);
		AddShortcut('A', B_COMMAND_KEY|B_CONTROL_KEY|B_SHIFT_KEY,
					new BMessage(kPrintAtomReport), this);
		AddShortcut('F', B_COMMAND_KEY|B_CONTROL_KEY|B_SHIFT_KEY,
					new BMessage(kFlushCache), this);
		AddShortcut('R', B_COMMAND_KEY|B_CONTROL_KEY|B_SHIFT_KEY,
					new BMessage(kPrintResources), this);
	}
	
	rect = Bounds();
	rect.OffsetTo(B_ORIGIN);
	
	fContentView = new ContentView(rect, "content_view", B_FOLLOW_ALL_SIDES,
		0, (DOUBLE_BUFFER?cvoDoubleBuffer:0) | /*cvoHScrollbar | cvoVScrollbar |*/  extraFlags);
	
	AddChild(fContentView);
}


BrowserWindow::~BrowserWindow()
{
	be_app->PostMessage(BW_CLOSED);
}

bool BrowserWindow::QuitRequested()
{
	// This looks a little strange, but is necessary to make some
	// guarantees to content plugins.  This make sure that any message
	// posted by a child view in DetachedFromWindow will be processed
	// before the window goes away.  See Java stuff.
	fContentView = NULL;
	BView *child;
	while ((child = ChildAt(0)) != 0) {
		child->RemoveSelf();
		delete child;
	}
	
	PostMessage(kReallyQuit);
	return false;
}

// Return a parent view that is scrollable in the given direction,
// or the original view if none were found.
static BView* find_key_handler(BView* view, orientation dir)
{
	BView* found=view;
	while (found && (!found->ScrollBar(dir) ||
					 found->ScrollBar(dir)->IsHidden())) {
		found = found->Parent();
	}
	return found ? found : view;
}

enum {
	CURSOR_LEFT_RIGHT_MASK	= (1<<0),
	CURSOR_UP_DOWN_MASK		= (1<<1),
	PAGE_UP_MASK			= (1<<2),
	PAGE_DOWN_MASK			= (1<<3),
	HOME_MASK				= (1<<4),
	END_MASK				= (1<<5),
	
	VERTICAL_MASK			= (CURSOR_UP_DOWN_MASK|PAGE_UP_MASK|PAGE_DOWN_MASK
							   |HOME_MASK|END_MASK),
	HORIZONTAL_MASK			= (CURSOR_LEFT_RIGHT_MASK)
};

void
BrowserWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	// This little mess makes it so that scrolling keyboard events
	// (page up/down, cursor, etc) sent to a focus view that can't
	// use them will be redirected to a parent that can.  There are
	// two ways this is determined:
	// * The view can include in its name character codes between {}
	//   to indicate which scrolling keys it does NOT use.  The
	//   start bracket must be the first or second character in the
	//   name (so that a view name can start with '*' for Opera.)
	//   The following codes are defined:
	//   h: doesn't use left/right cursor keys.
	//   v: doesn't use up/down cursor keys.
	//   U: doesn't use page up key.
	//   D: doesn't use page down key.
	//   H: doesn't use home key.
	//   E: doesn't use end key.
	// * If no {} specification is supplied, we will try to type
	//   cast the target handler to see if it is one of the standard
	//   controls whose behaviour we know.
	if (message->what == B_KEY_DOWN || message->what == B_KEY_UP) {
		
		// First classify this key.
		uint32 keyType = 0;
		
		const char* bytes;
		if (message->FindString("bytes", &bytes) == B_OK &&
				bytes[0] && !bytes[1]) {
			switch (*bytes) {
				case B_UP_ARROW:
				case B_DOWN_ARROW:
					keyType = CURSOR_UP_DOWN_MASK;
					break;
				case B_LEFT_ARROW:
				case B_RIGHT_ARROW:
					keyType = CURSOR_LEFT_RIGHT_MASK;
					break;
				case B_PAGE_UP:
					keyType = PAGE_UP_MASK;
					break;
				case B_PAGE_DOWN:
					keyType = PAGE_DOWN_MASK;
					break;
				case B_HOME:
					keyType = HOME_MASK;
					break;
				case B_END:
					keyType = END_MASK;
					break;
			}
		}
		
		// If this is a cursor key, figure out what to do with it
		if (keyType) {
			uint32 keyMask = 0;
			
			BView* view = dynamic_cast<BView*>(handler);
			
			// First check for an explicit key handling specification
			// in the handler's name.
			const char* name = view ? view->Name() : "";
			if (name && *name != '{') {
				if (*name) name++;
				if (name && *name != '{') name = NULL;
			}
			
			if (name) {
				// Parse out the specifiers.
				name++;
				while (*name && *name != '}') {
					switch (*name) {
						case 'h':	keyMask |= CURSOR_LEFT_RIGHT_MASK;	break;
						case 'v':	keyMask |= CURSOR_UP_DOWN_MASK;		break;
						case 'U':	keyMask |= PAGE_UP_MASK;			break;
						case 'D':	keyMask |= PAGE_DOWN_MASK;			break;
						case 'H':	keyMask |= HOME_MASK;				break;
						case 'E':	keyMask |= END_MASK;				break;
					}
					name++;
				}
			} else if (view) {
				// Look for standard control types.
				BButton* button = dynamic_cast<BButton*>(view);
				BCheckBox* checkbox = dynamic_cast<BCheckBox*>(view);
				BMenuField* menufield = dynamic_cast<BMenuField*>(view);
				BRadioButton* radio = dynamic_cast<BRadioButton*>(view);
				BTextView* textview = dynamic_cast<BTextView*>(view);
				bool oneline = textview ? textview->IsResizable() : false;
			
				if (button || checkbox) {
					keyMask = CURSOR_LEFT_RIGHT_MASK | CURSOR_UP_DOWN_MASK
							| PAGE_UP_MASK | PAGE_DOWN_MASK | HOME_MASK | END_MASK;
				} else if (radio || menufield) {
					keyMask = PAGE_UP_MASK | PAGE_DOWN_MASK | HOME_MASK | END_MASK;
				} else if (textview && oneline) {
					keyMask = CURSOR_UP_DOWN_MASK | PAGE_UP_MASK | PAGE_DOWN_MASK;
				} else if (textview) {
					keyMask = 0;
				}
			}
			
			// Should this key be passed up to its parent?
			if ((keyMask&keyType) != 0 && view) {
				BHandler* newhandler = find_key_handler(view,
					((keyType&VERTICAL_MASK) ? B_VERTICAL : B_HORIZONTAL));
				if (newhandler) handler = newhandler;
			}
		}
	} else if (message->what == B_MODIFIERS_CHANGED) {
		// forward CAPSLOCK toggle events to the content, since some
		// Javascript UIs need to display an on-screen caps-lock indicator
		int32 modflags = modifiers();
		
		BMessage modMsg('modC');
		
		modMsg.AddInt32("SHIFT_KEY", ((modflags & B_SHIFT_KEY) == B_SHIFT_KEY));
		modMsg.AddInt32("LEFT_SHIFT_KEY", ((modflags & B_LEFT_SHIFT_KEY) == B_LEFT_SHIFT_KEY));
		modMsg.AddInt32("RIGHT_SHIFT_KEY", ((modflags & B_RIGHT_SHIFT_KEY) == B_RIGHT_SHIFT_KEY));

		modMsg.AddInt32("OPTION_KEY", ((modflags & B_OPTION_KEY) == B_OPTION_KEY));
		modMsg.AddInt32("LEFT_OPTION_KEY", ((modflags & B_LEFT_OPTION_KEY) == B_LEFT_OPTION_KEY));
		modMsg.AddInt32("RIGHT_OPTION_KEY", ((modflags & B_RIGHT_OPTION_KEY) == B_RIGHT_OPTION_KEY));

		modMsg.AddInt32("CONTROL_KEY", ((modflags & B_CONTROL_KEY) == B_CONTROL_KEY));
		modMsg.AddInt32("LEFT_CONTROL_KEY", ((modflags & B_LEFT_CONTROL_KEY) == B_LEFT_CONTROL_KEY));
		modMsg.AddInt32("RIGHT_CONTROL_KEY", ((modflags & B_RIGHT_CONTROL_KEY) == B_RIGHT_CONTROL_KEY));

		modMsg.AddInt32("COMMAND_KEY", ((modflags & B_COMMAND_KEY) == B_COMMAND_KEY));
		modMsg.AddInt32("LEFT_COMMAND_KEY", ((modflags & B_LEFT_COMMAND_KEY) == B_LEFT_COMMAND_KEY));
		modMsg.AddInt32("RIGHT_COMMAND_KEY", ((modflags & B_RIGHT_COMMAND_KEY) == B_RIGHT_COMMAND_KEY));

		modMsg.AddInt32("MENU_KEY", ((modflags & B_MENU_KEY) == B_MENU_KEY));
		modMsg.AddInt32("SCROLL_LOCK", ((modflags & B_SCROLL_LOCK) == B_SCROLL_LOCK));
		modMsg.AddInt32("NUM_LOCK", ((modflags & B_NUM_LOCK) == B_NUM_LOCK));
		modMsg.AddInt32("CAPS_LOCK", ((modflags & B_CAPS_LOCK) == B_CAPS_LOCK));
								
		// Notify the top-level ContentInstance
		NotifyContent(&modMsg);

	}
		
	
	BWindow::DispatchMessage(message, handler);
}

ContentInstance *
BrowserWindow::GetContentInstance()
{
	ContentInstance *ci = NULL;
	if (fContentView) ci = fContentView->GetContentInstance();
	return ci;
}

ContentView *
BrowserWindow::GetContentView()
{
	return fContentView;
}

void 
BrowserWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case 'notc': {
			BMessage m;
			msg->FindMessage("msg",&m);
			NotifyContent(&m);
		}	break;
		case kReallyQuit:
			Quit();
			break;
		case TB_SHOW_TOOLBAR:
		{
//			printf("BrowserWindow::TB_SHOW_TOOLBAR rcvd\n");
			break;
		}
		
		case TB_OPEN_ALERT:
		{
//			printf("BrowserWindow TB_OPEN_ALERT rcvd\n");
			be_app->PostMessage(TB_OPEN_ALERT);
			break;
		}

		// debugging
		case kPrintHelp:
		{
			printf(	"Command+Control+Shift+A: Print all atoms.\n"
					"Command+Control+Shift+C: Print atoms in current mark.\n"
					"Command+Control+Shift+F: Flush cache.\n"
					"Command+Control+Shift+L: Print atoms in last mark.\n"
					"Command+Control+Shift+M: Mark place.\n"
					"Command+Control+Shift+R: Print active resources.\n"
					);
			break;
		}
		case kMarkLeakReport:
		{
			fCurrentAtomMark = BAtom::MarkLeakReport();
			break;
		}
		case kPrintLastReport:
		{
			BAtom::LeakReport(fCurrentAtomMark-1, fCurrentAtomMark-1);
			break;
		}
		case kPrintCurrentReport:
		{
			BAtom::LeakReport(fCurrentAtomMark);
			break;
		}
		case kPrintAtomReport:
		{
			BAtom::LeakReport();
			break;
		}
		case kFlushCache:
		{
			resourceCache.EmptyCache();
			break;
		}
		case kPrintResources:
		{
			Resource::DumpResources();
			break;
		}
		
		default:
			BWindow::MessageReceived(msg);
	}
}

BView* BrowserWindow::NextNavigableView(BView* currentFocus, uint32 flags)
  {
    uint32    i;                                   // Loop counter
    BView*    next;                                // View to return
    BView*    stop;                                // Don't loop forever
    ViewTree  tree(this, currentFocus, true);      // The navigate tree
    
    if ((flags & B_NAVIGATE_GROUP)                 // If going to next group
     && HasControlViews())                         // And there is a group
      for (stop = NULL, i = 0; i <= 1000; i++)     // Loop to next group
        {
          if (flags & B_NAVIGATE_NEXT)             // Going forward
            next = tree.Next();                    // Get next view
          else                                     // Going backward
            next = tree.Prev();                    // Get prev view
          
          if (!next) break;                        // In case of error
          
          if ((next->Flags() & B_NAVIGABLE_JUMP)&& // If navigable
           !next->IsHidden())                      // And not hidden
            break;                                 // That's the one we want
        
          if (!stop)                               // If no sentry view
            {
              stop = next;                         // Save this one
            }  
          else if (stop == next)                   // If all views checked
            {
              tree.SetCurrent(currentFocus);       // Reset current view
              break;                               // Stop the loop
            }  
        }
    
    for (stop = NULL, i = 0; i < 1000; i++)        // Loop to find next view
      {
        if (flags & B_NAVIGATE_NEXT)               // Going forward
          next = tree.Next();                      // Get next view
        else                                       // Going backward
          next = tree.Prev();                      // Get prev view
          
        if (!next) break;                          // In case of error
        
        if ((next->Flags() & B_NAVIGABLE) &&       // If it's navigable
         !next->IsHidden())                        // And not hidden
          break;                                   // We want that one
        
        if (!stop)                                 // If no sentry view
          {
            stop = next;                           // Save this one
          }  
        else if (stop == next)                     // If all views checked
          {
            next = currentFocus;                   // Back to start
            break;                                 // Stop the loop
          }  
      }
      
    if (!next) next = currentFocus;                // Might hit the end
    return next;                                   // There you go
  }

// This procedure returns true if one or more of this window's child
// views has the B_NAVIGABLE_JUMP flag set, false if not.

bool BrowserWindow::HasControlViews(void)
  {
    BTraverseViews  list(this, NULL, false, false);
    BView*          view;
    
    while (true)
      {
        view = list.Next();
        if (!view) break;
        
        if ((view->Flags() & B_NAVIGABLE_JUMP) &&
         !view->IsHidden())
          return true;
      }
      
    return false;
  }
  
status_t 
BrowserWindow::SetContent(ContentInstance *instance)
{
	if ((!instance) || (!fContentView)) return B_BAD_VALUE;
	return fContentView->SetContent(instance);
}

status_t 
BrowserWindow::OpenURL(const Wagner:: URL &url, uint32 flags, GroupID requestorsGroupID)
{
	if ((!url.IsValid()) || (!fContentView))
		return B_BAD_VALUE;

	fContentView->SetContent(url, flags, requestorsGroupID);

	if (fBrowserStyle == BS_ALERT)
	{
		// we need to resize
		ContentInstance *content = NULL;
		content = fContentView->GetContentInstance();
		if (content)
		{
			int32 width = 0;
			int32 height = 0;
			uint32 flags;
			content->GetSize(&width, &height, &flags);
			ResizeTo(width, height);
			BRect frame = BScreen().Frame();
			MoveTo((frame.Width() - width)/2, (frame.Height() - height)/2);
		}
	}
	
	return B_OK;
}

status_t BrowserWindow::OpenURL(const Wagner:: URL &url, const char *target, 
	GroupID requestorsGroupID)
{
	BMessage msg(bmsgLinkTo);
	url.AddToMessage("url", &msg);
	msg.AddString("target", target);

	// Do something with the groupid
	msg.AddInt32("requestor_groupid", requestorsGroupID);
	
	return PostMessage(&msg, fContentView);
}

bool 
BrowserWindow::Closable()
{
	return (fFlags & BS_JS_CLOSABLE);
}

void 
BrowserWindow::NotifyContent(BMessage *msg)
{
	if (fContentView) {
		ContentInstance *instance = fContentView->GetContentInstance();
		if (instance)
			instance->Notification(msg);
	}
}

browser_style 
BrowserWindow::BrowserStyle() const
{
	return fBrowserStyle;
}

void 
BrowserWindow::StartAbuse(abuse_t *at)
{
	resume_thread(spawn_thread(Abuse, "abuse", B_NORMAL_PRIORITY, at));
}

int32 BrowserWindow::Abuse(void *arg)
{
	abuse_t *at = (abuse_t*)arg;
	BrowserWindow *window = at->window;
	char urlBuf[1024];
	FILE *fp_in = NULL;
	FILE *fp_out = NULL;

	// stuff for redirection
	Protocol *protocol = 0;
	status_t err;
	BMessage outErrors;

	if(at->url_file) {
		fp_in = fopen(at->url_file, "r");
	}
	if(at->save_urls) {
		fp_out = fopen(at->save_urls, "a");
	}

	srandom((int)system_time());
	while (true) {
		URL randomURL;
		if(fp_in) {
			if(fgets(urlBuf, sizeof(urlBuf), fp_in) == NULL) {
				if(at->loop_urls) {
					fseek(fp_in, 0, SEEK_SET);
					if(fgets(urlBuf, sizeof(urlBuf), fp_in) == NULL) {
						break;
					}
				}
				else {
					break;
				}
			}
			int nl = strlen(urlBuf)-1;
			if(urlBuf[nl] == '\n') {
				urlBuf[nl] = '\0';
			}
			randomURL.SetTo(urlBuf, true);
		}
		else {
			protocol = Protocol::InstantiateProtocol(randomLinkServer.GetScheme());
			if(protocol) {
				err = protocol->Open(randomLinkServer, randomLinkServer, &outErrors, 0);
				if(err == B_OK) {
					bigtime_t dummy;
					protocol->GetRedirectURL(randomURL, &dummy);
				}
				else {
					randomURL = randomLinkServer;
				}
				delete protocol;
			}
			else {
				randomURL = randomLinkServer;
			}
		}
		randomURL.GetString(urlBuf, sizeof(urlBuf));
		printf("\e[43mABUSE: %s\e[0m\n", urlBuf);
		if(fp_out) {
			fputs(urlBuf, fp_out);
			fputc('\n', fp_out);
			fflush(fp_out);
		}

		if (!window->Lock())
			break;
			
		GroupID internetGroupID = securityManager.RegisterGroup("internet");
		window->OpenURL(randomURL, FORCE_RELOAD | LOAD_ON_ERROR, internetGroupID);
		window->Unlock();

		snooze(random() % 10000000 + (at->num_windows-1)*1000000);
		
		if(((Wotan *)be_app)->IsSet_NoCache()) resourceCache.EmptyCache();
	}
	
	fclose(fp_in);
	fclose(fp_out);
	free(at);
	return 0;
}

void 
BrowserWindow::Reload()
{
	if (fContentView) fContentView->Reload();
}

void 
BrowserWindow::StopLoading()
{
#if 0
	// This would cancel a pending navigation, but not interrupt
	// a page load.
	if (fContentView) fContentView->StopLoading();
#endif

	// This is a little more aggressive
	resourceCache.AbortAll();
}

void 
BrowserWindow::GoForward()
{
//	printf("BrowserWindow::GoForward\n");
	if (fContentView) fContentView->GoForward();
}

void 
BrowserWindow::GoBackward()
{
//	printf("BrowserWindow::GoBackward\n");
	if (fContentView) fContentView->GoBackward();
}

void 
BrowserWindow::Rewind()
{
//	printf("BrowserWindow::Rewind()\n");
	if (fContentView) fContentView->Rewind();
}


void 
BrowserWindow::Print()
{
	if (fContentView) fContentView->Print();
}

