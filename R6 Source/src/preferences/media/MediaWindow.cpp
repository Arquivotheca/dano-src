#include "MediaWindow.h"
#include "MediaViews.h"
#include "MediaListView.h"

#include <Application.h>
#include <AutoLock.h>
#include <Debug.h>
#include <ParameterWeb.h>
#include <MediaDefs.h>
#include <MediaTheme.h>
#include <MediaRoster.h>
#include <MediaAddOn.h>
#include <ListView.h>
#include <ListItem.h>
#include <StringView.h>
#include <Font.h>
#include <ScrollView.h>
#include <TabView.h>
#include <Box.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Screen.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <FindDirectory.h>
#include <String.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LIST_WIDTH 160
#define TITLE_HEIGHT 30

#define H_OFFSET LIST_WIDTH/2
#define V_OFFSET (TITLE_HEIGHT/2)+4

#define SPACING 14

#define FPrintf fprintf

#define TOUCH(x) ((void)(x))

// ************************************************************************** //

static void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

class TPulseBox : public BView {
public:
						TPulseBox(BRect frame, int32 countdown,
							const char* title, const char* message);
						~TPulseBox();

		void			AttachedToWindow();
		void			Draw(BRect);
		
		void			GetIcon(alert_type);
		
		void			SetTitle(const char* t);
		void			SetMessage(const char* m);
		
private:
		BBitmap*		fOffScreenBits;
		BView*			fOffScreenView;

		BBitmap*		fIconBits;
		BString*		fTitle;
		BString*		fMessage;
		
		float			fBoldFontHeight;
};

class TStatusWindow : public BWindow {
public:
						TStatusWindow(int32 seconds,
							const char* title, const char* message);
				
		void			SetTitle(const char* t) { fBG->SetTitle(t); }
		void			SetMessage(const char* m) { fBG->SetMessage(m); }
private:
		TPulseBox*		fBG;
};

// ************************************************************************** //

#include <Bitmap.h>
#include <Resources.h>

const int32 kConfirmWindowWidth	= 300;
const int32 kConfirmWindowHeight = 60;

TPulseBox::TPulseBox(BRect frame, int32 count,
	const char* title, const char* message)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
	TOUCH(count);

	SetFont(be_bold_font);
	GetIcon(B_WARNING_ALERT);
	fTitle = new BString(title);
	fMessage = new BString(message);
	
	font_height fh;
	be_bold_font->GetHeight(&fh);
	fBoldFontHeight = fh.ascent + fh.descent + fh.leading;
	
	fOffScreenView = new BView(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW);
	fOffScreenBits = new BBitmap(Bounds(), B_COLOR_8_BIT, true);
	
	if (fOffScreenBits && fOffScreenView)
		fOffScreenBits->AddChild(fOffScreenView);
}

TPulseBox::~TPulseBox()
{
	delete fOffScreenBits;
	delete fTitle;
	delete fMessage;
	if (fIconBits)
		delete fIconBits;
}

void
TPulseBox::AttachedToWindow()
{
	if (Parent()) {
		rgb_color c = Parent()->ViewColor();
		SetViewColor(c);
		SetLowColor(c);
	} else {
		SetViewColor(216, 216, 216, 255);
		SetLowColor(216, 216, 216, 255);
	}
}

const float kTextLeft = 60;
const float kFirstLineTextBottom = 20;
const float kTextGap = 5;

void
TPulseBox::Draw(BRect u)
{
	TOUCH(u);

	if (!fOffScreenBits)
		return;

	AutoLock<BBitmap> lock(fOffScreenBits);
	if (!lock)
		return;

	BRect r(Bounds());
		
	fOffScreenView->SetLowColor(ViewColor());
	
	//	draw gray box
	r.right = r.left + 30;
	fOffScreenView->SetHighColor(184, 184, 184, 255);
	fOffScreenView->FillRect(r);
	
	r.left = r.right;
	r.right = Bounds().Width();
	fOffScreenView->SetHighColor(ViewColor());
	fOffScreenView->FillRect(r);

	//	draw icon
	r.Set(18, 6, 49, 37);		// carefully selected rect determined by moving it 1 pixel at a time
	fOffScreenView->SetDrawingMode(B_OP_OVER);
	fOffScreenView->DrawBitmap(fIconBits, BRect(0,0,31,31), r);
	fOffScreenView->SetDrawingMode(B_OP_COPY);
	
	//	draw the msg
	fOffScreenView->SetHighColor(0, 0, 0, 255);
	fOffScreenView->SetFont(be_bold_font);
	
	fOffScreenView->MovePenTo(kTextLeft, kFirstLineTextBottom);
	fOffScreenView->DrawString(fTitle->String());
	
	fOffScreenView->MovePenTo(kTextLeft, kFirstLineTextBottom + kTextGap + fBoldFontHeight);
	fOffScreenView->DrawString(fMessage->String());
	
	fOffScreenView->Sync();
	DrawBitmap(fOffScreenBits, Frame().LeftTop());
}

void
TPulseBox::GetIcon(alert_type alertType)
{
	if (alertType != B_EMPTY_ALERT) {
		BPath path;
		if (find_directory (B_BEOS_SERVERS_DIRECTORY, &path) == B_OK) {
			path.Append ("app_server");
			BFile		file(path.Path(), O_RDONLY);
			BResources	rfile;

			if (rfile.SetTo(&file) == B_NO_ERROR) {
				size_t	size;
				char	*name = "";
				switch(alertType) {
					case B_INFO_ALERT:		name = "info"; break;
					case B_IDEA_ALERT:		name = "idea"; break;
					case B_WARNING_ALERT:	name = "warn"; break;
					case B_STOP_ALERT:		name = "stop"; break;
					default:
						TRESPASS();
				}
				void *data = rfile.FindResource('ICON', name, &size);

				if (data) {
					fIconBits = new BBitmap(BRect(0,0,31,31), B_COLOR_8_BIT);
					fIconBits->SetBits(data, size, 0, B_COLOR_8_BIT);
					free(data);
				}
			} 
		}
		if (!fIconBits) {
			// couldn't find icon so make this a B_EMPTY_ALERT
			alertType = B_EMPTY_ALERT;
		}
	} else
		fIconBits = NULL;
}

void
TPulseBox::SetTitle(const char* t)
{
	fTitle->SetTo(t);
	if (Window()->Lock()) {
		if (fOffScreenBits->Lock()) {

			fOffScreenView->SetLowColor(ViewColor());
			fOffScreenView->SetFont(be_bold_font);
					
			BRect invalRect(kTextLeft, kFirstLineTextBottom - fBoldFontHeight,
				Bounds().Width(), kFirstLineTextBottom + 5);
			fOffScreenView->SetHighColor(ViewColor());
			fOffScreenView->FillRect(invalRect);
			
			fOffScreenView->SetHighColor(0, 0, 0, 255);		
			fOffScreenView->MovePenTo(kTextLeft, kFirstLineTextBottom);
			fOffScreenView->DrawString(fTitle->String());
	
			fOffScreenView->Sync();
			
			DrawBitmap(fOffScreenBits, invalRect, invalRect);
			
			fOffScreenBits->Unlock();
		}
		Window()->Unlock();
	}
}

void
TPulseBox::SetMessage(const char* m)
{
	fMessage->SetTo(m);
	if (Window()->Lock()) {
		if (fOffScreenBits->Lock()) {

			fOffScreenView->SetLowColor(ViewColor());
			fOffScreenView->SetFont(be_bold_font);
					
			BRect invalRect(kTextLeft, kFirstLineTextBottom + kTextGap,
				Bounds().Width(), kFirstLineTextBottom + kTextGap + fBoldFontHeight + 5);
			fOffScreenView->SetHighColor(ViewColor());
			fOffScreenView->FillRect(invalRect);
			
			fOffScreenView->SetHighColor(0, 0, 0, 255);		
			fOffScreenView->MovePenTo(kTextLeft, kFirstLineTextBottom + kTextGap + fBoldFontHeight);
			fOffScreenView->DrawString(fMessage->String());
	
			fOffScreenView->Sync();
			
			DrawBitmap(fOffScreenBits, invalRect, invalRect);
			
			fOffScreenBits->Unlock();
		}
		Window()->Unlock();
	}
}
// ************************************************************************** //

TStatusWindow::TStatusWindow(int32 count, const char* title, const char* message)
	:BWindow( BRect(0, 0, kConfirmWindowWidth, kConfirmWindowHeight),
		"Restart Status", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_ASYNCHRONOUS_CONTROLS)
{	
	fBG = new TPulseBox(Bounds(), count, title, message);
	fBG->SetFont(be_plain_font);
	AddChild(fBG);

	CenterWindowOnScreen(this);
}

// ************************************************************************** //

struct prefs {
	prefs(const BRect & ir, int32 defaultitem) : rect(ir), item(defaultitem) { }
	BRect rect;
	int32 item;
};

static prefs g_prefs(BRect(32, 64, 32+MIN_WIDTH, 64+MIN_HEIGHT), 1);

static void read_prefs()
{
	BPath dir;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &dir, true) < B_OK)
		return;
	dir.Append("MediaPrefs Settings");
	FILE * fil = fopen(dir.Path(), "r");
	if (!fil)
		return;
	char line[1024];
	while (!feof(fil) && !ferror(fil))
	{
		line[0] = 0;
		fgets(line, 1024, fil);
		if (!line[0])
			break;
		if (line[0] == '#')
			continue;
		if (line[0] == '\n')
			continue;
		if (sscanf(line, " rect = %f , %f , %f , %f",
				&g_prefs.rect.left, &g_prefs.rect.top, 
				&g_prefs.rect.right, &g_prefs.rect.bottom) > 0)
			continue;
		if (sscanf(line, " item = %d",
				(int*)&g_prefs.item) > 0)
			continue;
		//	ignore line
		FPrintf(stderr, "unknown setting: %s", line);
	}
	fclose(fil);
	BScreen scrn;
	BRect f = scrn.Frame();
	if (g_prefs.rect.right < g_prefs.rect.left + MIN_WIDTH) g_prefs.rect.right = g_prefs.rect.left+MIN_WIDTH;
	if (g_prefs.rect.bottom < g_prefs.rect.top + MIN_HEIGHT) g_prefs.rect.bottom = g_prefs.rect.top+MIN_HEIGHT;
	if (g_prefs.rect.Width() > f.Width()) g_prefs.rect.right = g_prefs.rect.left+f.Width();
	if (g_prefs.rect.Height() > f.Height()) g_prefs.rect.bottom = g_prefs.rect.top+f.Height();
	if (g_prefs.rect.right > f.right) g_prefs.rect.OffsetBy(f.right-g_prefs.rect.right, 0);
	if (g_prefs.rect.bottom > f.bottom) g_prefs.rect.OffsetBy(0, f.bottom-g_prefs.rect.bottom);
	if (g_prefs.rect.left < f.left) g_prefs.rect.OffsetBy(f.left-g_prefs.rect.left, 0);
	if (g_prefs.rect.top < f.top) g_prefs.rect.OffsetBy(0, f.top-g_prefs.rect.top);
}

static void write_prefs()
{
	BPath dir;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &dir, true) < B_OK)
		return;
	dir.Append("MediaPrefs Settings");
	FILE * f = fopen(dir.Path(), "w");
	if (!f)
		return;
	FPrintf(f, "# MediaPrefs Settings\n");
	FPrintf(f, " rect = %.0f,%.0f,%.0f,%.0f\n", g_prefs.rect.left, g_prefs.rect.top,
			g_prefs.rect.right, g_prefs.rect.bottom);
	FPrintf(f, " item = %d\n", int(g_prefs.item));
	fclose(f);
}

class MediaApp : public BApplication {
public:
		MediaApp();
virtual	bool QuitRequested();
virtual	void MessageReceived(BMessage * message);
private:
		MediaWindow * w;
};

MediaApp::MediaApp() : BApplication("application/x-vnd.Be.MediaPrefs")
{
	read_prefs();
	w = new MediaWindow(g_prefs.rect, "Media");
}

bool MediaApp::QuitRequested()
{
	write_prefs();
	if (w->Lock()) {
		w->Quit();
	}
	return true;
}

void
MediaApp::MessageReceived(
	BMessage * message)
{
	switch (message->what) {
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


int
main()
{
	MediaApp app;
	app.Run();
	return 0;
}

MediaWindow::MediaWindow(const BRect area, const char *name)
	: BWindow(area, name, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	mInited(false),
	mRoster(NULL),
	mBackground(NULL),
	mDyno(NULL),
	mTitle(NULL),
	mList(NULL),
	mAudio(NULL),
	mVideo(NULL),
	mCurrent(NULL),
	fThreadID(-1),
	fStatusWindow(NULL),
	fInitWhenDoneRestarting(false),
	fServerIsDead(false)
{
	SetSizeLimits(MIN_WIDTH, 2048, MIN_HEIGHT, 1536);
	
	//background
	mBackground = new BBox(Bounds(), "background", B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	AddChild(mBackground);
	
	//title
	BRect r = mBackground->Bounds();
	r.InsetBy(H_OFFSET, 0);
	r.OffsetBy(H_OFFSET, 1);
	r.bottom = TITLE_HEIGHT;
	r.right--;
	mTitle = new BStringView(r, "title", "Swanky New Media Preference Panel");
	mTitle->SetAlignment(B_ALIGN_LEFT);
	mTitle->SetFont(be_bold_font);
	mBackground->AddChild(mTitle);

	//create dynamic scroll view
	r = mBackground->Bounds();
	r.InsetBy(H_OFFSET, V_OFFSET);
	r.OffsetBy(H_OFFSET, V_OFFSET+2);
	r.right -= SPACING;
	r.right += 2;
	r.bottom -= SPACING;
	mDyno = new BDynamicScrollView(r, "scroller", B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FRAME_EVENTS);
	mBackground->AddChild(mDyno);
	
	status_t err = Init();
	//	if the init fails, finish setting up the views and show the window,
	//	but do not try to make any media kit calls
	//	init will call restart and when media services are available again
	//	the rest of the initialization will be completed by calling INIT_SERVICES
	if (err == B_OK) {
		SetCurrentView();	
	}
	Show();
}

MediaWindow::~MediaWindow()
{
	mRoster->StopWatching(BMessenger(this), B_MEDIA_FLAVORS_CHANGED);
}

status_t
MediaWindow::Init()
{
	PRINT(("MediaWindow::Init\n"));
	//check for media server
	status_t err = B_OK;
	mRoster = BMediaRoster::Roster(&err);
	if (!mRoster || err) {
		HideStatusWindow();
		char msg[300];
		sprintf(msg, "Could not connect to the Media Server, \nWould you like to start it?");
		switch((new BAlert("", msg, "  Quit  ", "Start Media Server", 
							NULL, B_WIDTH_FROM_LABEL, B_EVEN_SPACING, B_WARNING_ALERT))->Go())
		{
			case 0:
				//	kill off the restart thread before bailing
				KillRestartThread();
				//	bail immediately
				be_app->PostMessage(B_QUIT_REQUESTED);
				return B_ERROR;
				break;
			case 1:
				fInitWhenDoneRestarting = true;		//	need to loop back and re-init
				fServerIsDead = true;				//	don't try to kill it again in restart
				ShowStatusWindow();					//	will get hidden at end of INIT_SERVICES msg
				RestartServer();					//	launch the restart thread
				return B_ERROR;
				break;
		}
	}

	//lock the window
	if(!Lock()) {
		return B_ERROR;
	}
	if(mInited) {
		mDyno->SetTarget(NULL);
	}
	mInited = true;
	
	mRoster->StartWatching(BMessenger(this), B_MEDIA_FLAVORS_CHANGED);

	SetupListView();
	SetupAudioView();
	SetupVideoView();
	
	Unlock();
	return B_OK;
}

void 
MediaWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case SELECTION_CHANGED:
			g_prefs.item = mList->CurrentSelection(0);
			SetCurrentView();
			break;
		case B_MEDIA_WEB_CHANGED: {
			PRINT(("B_MEDIA_WEB_CHANGED\n"));
			int32 nodeIndex = 0;
			media_node *node;
			ssize_t nodeSize;
			/* you can get more than one at a time */
			while (message->FindData("node", B_RAW_TYPE, nodeIndex, 
				(const void**) &node, &nodeSize) == B_OK) {
				nodeIndex++;
				if (mList->WebChanged(node)) {
					SetCurrentView();
				}
			}
			break;	
		}
		case B_MEDIA_FLAVORS_CHANGED:
			SetupListView();
			SetupAudioView();
			SetupVideoView();
			break;
		case RESTART_SERVER:
			//	this will be from the restart button
			//	passing true will cause the INIT_SERVICES message
			//	to be sent when done		
			fInitWhenDoneRestarting = true;
			mCurrent = NULL;
			ShowStatusWindow();
			RestartServer();
			break;
		case INIT_SERVICES:
			//	should be called at end of restartserver thread
			//	this will complete the restart sequence
			//	in the correct order
			if (fStatusWindow)
				fStatusWindow->SetMessage("Initializing Media Services...");
			Init();
			snooze(1000000);
			if (fStatusWindow)
				fStatusWindow->SetMessage("Ready For Use...");
			SetCurrentView();
			snooze(500000);
			HideStatusWindow();
			fInitWhenDoneRestarting = false;
			fServerIsDead = false;
			if(IsHidden())
				Show();
			break;
		default:
			BWindow::MessageReceived(message);
	}
}

bool 
MediaWindow::QuitRequested()
{
	//	kill off the restart thread before bailing
	KillRestartThread();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}

void 
MediaWindow::ClearDynoTarget()
{
	if (mDyno)
		mDyno->DiscardTarget();
}

void 
MediaWindow::SetVideoInputView(const dormant_node_info &info)
{
	PRINT(("MediaWindow::SetVideoInputView\n"));
	mList->SetVideoInputView(info);
}

void 
MediaWindow::SetVideoOutputView(const dormant_node_info &info)
{
	PRINT(("MediaWindow::SetVideoOutputView\n"));
	mList->SetVideoOutputView(info);
}

void 
MediaWindow::SetAudioInputView(const dormant_node_info &info)
{
	PRINT(("MediaWindow::SetAudioInputView\n"));
	mList->SetAudioInputView(info);
}

void 
MediaWindow::SetAudioOutputView(const dormant_node_info &info)
{
	PRINT(("MediaWindow::SetAudioOutputView\n"));
	mList->SetAudioOutputView(info);
}

void 
MediaWindow::SetMixerView(const dormant_node_info &info)
{
	TOUCH(info);
	PRINT(("MediaWindow::SetMixerView\n"));
}

void 
MediaWindow::SetupListView()
{
	PRINT(("MediaWindow::SetListView\n"));
	if (!mList) {
		BRect r(mBackground->Bounds());
		r.left += SPACING;
		r.right = LIST_WIDTH - SPACING;
		r.InsetBy(0,SPACING);
	
		mList = new MediaListView(r);
		BMessage *msg = new BMessage(SELECTION_CHANGED);
		mList->SetSelectionMessage(msg);
		BScrollView *scroller = new BScrollView("listscroller", mList, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, 
         									B_FRAME_EVENTS, false, false, B_FANCY_BORDER);
		mBackground->AddChild(scroller);
	}
	mList->SetRoster(mRoster);
	mList->PopulateList();
	mList->Select(g_prefs.item);
}

void 
MediaWindow::SetupAudioView()
{
	PRINT(("MediaWindow::SetAudioView\n"));
	BRect r = mBackground->Bounds();
	r.right = MIN_WIDTH - 2;
	r.bottom = MIN_HEIGHT - 2;
	r.InsetBy(H_OFFSET, V_OFFSET);
	r.OffsetBy(H_OFFSET, V_OFFSET+2);
	r.right -= SPACING;
	r.right += 2;
	r.bottom -= SPACING;
	if (mAudio) {
		mAudio->RemoveSelf();
		delete mAudio;
		mAudio = NULL;		
	}
	mAudio = new AudioView(r);
	mList->SetAudioView(mAudio);
	mAudio->SetViewColor(216, 216, 216);
	mDyno->SetTarget(mAudio);
}

void 
MediaWindow::SetupVideoView()
{
	PRINT(("MediaWindow::SetVideoView\n"));
	BRect r = mBackground->Bounds();
	r.right = MIN_WIDTH - 2;
	r.bottom = MIN_HEIGHT - 2;
	r.InsetBy(H_OFFSET, V_OFFSET);
	r.OffsetBy(H_OFFSET, V_OFFSET+2);
	r.right -= SPACING;
	r.right += 2;
	r.bottom -= SPACING;
	if(mVideo){
		mVideo->RemoveSelf();
		delete mVideo;
		mVideo = NULL;
	}
	mVideo = new VideoView(r);
	mList->SetVideoView(mVideo);
	mVideo->SetViewColor(216, 216, 216);
	mDyno->SetTarget(mVideo);
}

void
MediaWindow::SetCurrentView()
{
	PRINT(("MediaWindow::SetCurrentView\n"));
	if(!Lock()) {
		return;
	}
	int32 current = mList->CurrentSelection();
	BView *view = NULL;
	status_t status = mList->GetViewFor(current, &view);
	if (status < B_OK) {
		Unlock();
		return;
	}
	PRINT(("view 0x%x\n", view));
	if(view && view != mCurrent) {
		mCurrent = view;
		mTitle->SetText(mList->GetTitleFor(current));
		mDyno->SetTarget(mCurrent);
		mDyno->Invalidate();
		if(dynamic_cast<BEmptyView *>(mCurrent) || dynamic_cast<HardwareView *>(mCurrent)) {
			BRect r = mDyno->Bounds();
			r.InsetBy(2, 2);
			mDyno->SetTargetBounds(r);
		}
	}
	UpdateIfNeeded();
	Unlock();
}

static int32
Minnie(MediaWindow* window)
{
	return window->ServicesRestart();
}

//	called from button message
status_t
MediaWindow::RestartServer()
{
	//	launch a thread that does that actual services restart
	//	by doing so the window can get updates
  	fThreadID = spawn_thread((thread_entry)Minnie, "media_services_restart",
		B_NORMAL_PRIORITY, this);
	resume_thread(fThreadID);
	
	return B_OK;
}

//	media callback function
//	receives update/status message from media services restart
static bool
callback( int stage, const char * message, void * cookie)
{
	TOUCH(stage);

	TStatusWindow* window = (TStatusWindow*)cookie;
	if (window)
		window->SetMessage(message);
		
	return true;
}

//	call from a separate thread so we don't block the window
int32 
MediaWindow::ServicesRestart()
{
	int32 returnVal = 1;
	status_t err=B_OK;

	if(mList)
	{
		Lock();
		mList->PrepareListForServerRestart();
		Unlock();
	}

	while (1) {
		if (!fServerIsDead) {
			if (fStatusWindow)
				fStatusWindow->SetMessage("Stopping Media Server...");
			if(mRoster) {
				#if MEDIA_ROSTER_IS_LOOPER
					// Do crap
					mRoster->Lock();
					mRoster->Quit();
				#else
					// Do elegant, nice things
					delete mRoster;
				#endif
				mRoster = NULL;
			}
		
			err = shutdown_media_server(B_INFINITE_TIMEOUT, callback, fStatusWindow);
		}
		if (err == B_OK)
		{
			if (fStatusWindow)
				fStatusWindow->SetMessage("Starting Media Server...");
			err = launch_media_server();
			snooze(3000000);
		}
		
		if (err == B_OK || err == B_ALREADY_RUNNING)
		{
			break;
		}
		else
		{
			HideStatusWindow();
			
			char msg[1000];
			sprintf(msg, "A problem occurred while restarting the Media Server. \n%s (%x)",
				strerror(err), (uint)err);
			switch((new BAlert("Restart Error", msg, "Give Up", "Try Again"))->Go())
			{
				case 0:
					if(!mInited)		
						Run();	/* so Lock()/Quit() will work */
					//	kill off the restart thread before bailing
					KillRestartThread();
					be_app->PostMessage(B_QUIT_REQUESTED);
					return 0;
				case 1:
					//	continue through the loop ?
					ShowStatusWindow();
					//	don't try to kill it, its dead
					fServerIsDead = true;
					break;
			}	
		}
	}
	
	if (err == B_OK && fInitWhenDoneRestarting) {
		PostMessage(INIT_SERVICES);
	}
		
	return returnVal;
}

void
MediaWindow::ShowStatusWindow()
{
	if (fStatusWindow)
	{
		fStatusWindow->Activate();
		return;
	}
		
	//	create the custom alert dialog
	fStatusWindow = new TStatusWindow(-1,
		"Restarting Media Services", "Please Wait...");
	fStatusWindow->Show();
}

void
MediaWindow::HideStatusWindow()
{
	if (fStatusWindow) {
		fStatusWindow->Lock();	
		fStatusWindow->Quit();
		fStatusWindow = NULL;
		fThreadID = -1;
	}		
}

void
MediaWindow::KillRestartThread()
{
	if (fThreadID != -1) {
		kill_thread(fThreadID);
		fThreadID = -1;
	}	
}

void 
MediaWindow::FrameMoved(BPoint location)
{
	BWindow::FrameMoved(location);
	g_prefs.rect = Frame();
}

void 
MediaWindow::FrameResized(float width, float height)
{
	BWindow::FrameResized(width, height);
	g_prefs.rect = Frame();
	BView * vu = mDyno->Target();
	if(dynamic_cast<BEmptyView *>(vu) || dynamic_cast<HardwareView *>(vu))
	{
		BRect r = mDyno->Bounds();
		r.InsetBy(10, 10);
		mDyno->SetTargetBounds(r);
	}
}

#if 0
static void
dump_web(
	BParameterWeb * web)
{
	printf("Web: %ld parameters\n", web->CountParameters());
	for (int ix=0; ix<web->CountParameters(); ix++) {
		BParameter * p = web->ParameterAt(ix);
		BNullParameter * np;
		BDiscreteParameter * dp;
		BContinuousParameter * cp;
		if ((np = dynamic_cast<BNullParameter *>(p)) != NULL) {
			printf("%ld: NullParameter '%s': %s\n", p->ID(), np->Name(), np->Kind());
		}
		else if ((dp = dynamic_cast<BDiscreteParameter *>(p)) != NULL) {
			printf("%ld: DiscreteParameter '%s': %s\n", p->ID(), dp->Name(), dp->Kind());
		}
		else if ((cp = dynamic_cast<BContinuousParameter *>(p)) != NULL) {
			printf("%ld: ContinuousParameter '%s': %s\n", p->ID(), cp->Name(), cp->Kind());
		}
		else if (p) {
			printf("%ld: Unknown %s '%s': %s\n", p->ID(), typeid(*p).name(), p->Name(), p->Kind());
		}
		else {
			printf("NULL parameter at index %d!!!\n", ix);
			continue;
		}
		for (int i = 0; i<p->CountInputs(); i++) {
			if (i == 0) {
				printf("  inputs: ");
			}
			else {
				printf(", ");
			}
			printf("%ld", p->InputAt(i)->ID());
		}
		if (p->CountInputs()) {
			printf("\n");
		}
		for (int i = 0; i<p->CountOutputs(); i++) {
			if (i == 0) {
				printf("  outputs: ");
			}
			else {
				printf(", ");
			}
			printf("%ld", p->OutputAt(i)->ID());
		}
		if (p->CountOutputs()) {
			printf("\n");
		}
	}
}
#endif
