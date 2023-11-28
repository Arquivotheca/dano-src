#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <Alert.h>
#include <Button.h>
#include <Debug.h>
#include <CheckBox.h>
#include <ClassInfo.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Path.h>
#include <PrivateScreen.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <StringView.h>
#include <TextView.h>

#include <interface_misc.h>
#include <screen_private.h>

#include "screen_utils.h"
#include "screen.h"

#define _CONSTRAIN_REFRESH_RATE_ 1

#define min(a,b) ((a)>(b)?(b):(a))
#define max(a,b) ((a)>(b)?(a):(b))

const rgb_color kWhite 		= 	{255,255,255,255};
const rgb_color kMediumGray = 	{140,140,140,255};

bool gShowPoofDialog;

#if __MWERKS__
#define _UNUSED(x)
#endif

#if __GNUC__
#define _UNUSED(x) x
#endif

#if DEBUG
static void
What(const char* what)
{
	BAlert* a = new BAlert("", what, "Cancel", NULL, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT );
	a->Go();
}
#endif

static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

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

static rgb_color
SetRGBColor(int8 r, int8 g, int8 b)
{
	rgb_color c;
	c.red = r;
	c.green = g;
	c.blue = b;
	c.alpha = 255;
	
	return c;
}

static int32
myround(float f)
{
	int32 t = (int32)f;
	if ((f-t) >= 0.5)
		t++;
	return t;
}

static bool
CompareFloats(float f1, float f2)
{
	char str1[10], str2[10];
	sprintf(str1, "%.1f", f1);
	sprintf(str2, "%.1f", f2);
	return (strcmp(str1,str2) == 0);
}

//************************************************************************************

class TAlertView : public BView {
public:
				TAlertView(BRect b, alert_type type=B_WARNING_ALERT);
				~TAlertView();
	void		AttachedToWindow();
	void		Draw(BRect);
	void		GetIcon();
private:
	alert_type	fMsgType;
	BBitmap*	fIconBits;				
};

TAlertView::TAlertView(BRect b, alert_type type)
	: BView(b, "", B_FOLLOW_ALL, B_WILL_DRAW),
		fMsgType(type)
{
	GetIcon();
}

TAlertView::~TAlertView()
{
	if (fIconBits)
		delete fIconBits;
}

void
TAlertView::AttachedToWindow()
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

#define SHOW_COLOR 0	// this idea is not done yet, an example of a colored alert as per tim
void
TAlertView::Draw(BRect u)
{
	BView::Draw(u);
	BRect r(Bounds());

#if SHOW_COLOR
	//	gray border
	SetHighColor(80, 80, 80);
	StrokeRect(r);
	
	// 	lt yellow border
	r.InsetBy(1,1);
	SetHighColor(255,255,102);
	StrokeRect(r);
	
	// dk yellow border
	r.InsetBy(-1,-1);
	r.right--; r.bottom--;
	SetHighColor(255,152,0);
	StrokeRect(r);
	
	// fill of yellow
	r.InsetBy(1,1);
	r.top++;
	r.left++;
	SetHighColor(255,203,0,255);
	FillRect(r);
	
	//	lt yellow frame
	r.right -= 2; r.left += 2;
	r.top += 16; r.bottom -= 2;
	SetHighColor(255,255,102,255);
	StrokeLine(r.LeftBottom(), r.RightBottom());
	StrokeLine(r.RightBottom(), r.RightTop());
	
	SetHighColor(255,152,0,255);
	StrokeLine(r.LeftBottom(), r.LeftTop());
	StrokeLine(r.LeftTop(), r.RightTop());
	
	// gray border
	SetHighColor(152, 152, 152, 255);
	r.InsetBy(1,1);
	StrokeRect(r);
	
	r.InsetBy(1,1);
	SetHighColor(184,184,184,255);
	StrokeLine(r.LeftBottom(), r.RightBottom());
	StrokeLine(r.RightBottom(), r.RightTop());
	
	SetHighColor(255,255,255,255);
	StrokeLine(r.LeftBottom(), r.LeftTop());
	StrokeLine(r.LeftTop(), r.RightTop());
	
	// interior
	r.InsetBy(1,1);
	BRect tempr(r);
	tempr.right = tempr.left + 40;
	SetHighColor(200, 200, 200, 255);
	SetLowColor(ViewColor());
	FillRect(tempr);
	
	tempr = r;
	tempr.left = r.left+41;
	SetHighColor(216,216,216, 255);
	FillRect(tempr);

	//	draw icon
	r.Set(30, 38, 61, 69);
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fIconBits, BRect(0,0,31,31), r);
	SetDrawingMode(B_OP_COPY);
	
	// window title
	SetHighColor(0,0,0,255);
	MovePenTo(15, 13);
	SetFont(be_bold_font);
	DrawString("Warning");
#else	
	//	draw gray box
	r.right = r.left + 30;
	SetHighColor(184, 184, 184, 255);
	SetLowColor(ViewColor());
	FillRect(r);

	//	draw icon
	r.Set(18, 6, 49, 37);
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fIconBits, BRect(0,0,31,31), r);
	SetDrawingMode(B_OP_COPY);
#endif
}

void
TAlertView::GetIcon()
{
	if (fMsgType != B_EMPTY_ALERT) {
		BPath path;
		if (find_directory (B_BEOS_SERVERS_DIRECTORY, &path) == B_OK) {
			// why the hell is this code copied 13+ x times in all
			// the preferences???
			path.Append ("app_server");
			BFile		file(path.Path(), O_RDONLY);
			BResources	rfile;

			if (rfile.SetTo(&file) == B_NO_ERROR) {
				size_t	size;
				char	*name = "";
				switch(fMsgType) {
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
			// couldn't find icon so make this an B_EMPTY_ALERT
			fMsgType = B_EMPTY_ALERT;
		}
	} else
		fIconBits = NULL;
}

class TPoofAlert : public BWindow {
public:
				TPoofAlert(alert_type type=B_WARNING_ALERT);
				~TPoofAlert();
				
	void		MessageReceived(BMessage* m);	
	int32		Go();
	
	bool		Skip() { return fSkip; }
	
private:
	alert_type	fAlertType;
	TAlertView*	fBG;
		
	BTextView*	fMsgFld;
	
	BButton*	fPoofBtn;
	BButton*	fCancelBtn;
	
	BCheckBox*	fBypassWarning;
	
	sem_id		fAlertSem;
	int32		fAlertVal;
	
	bool		fSkip;
};

const char* poofstr1 = "WARNING: This is a very high resolution. ";
const char* poofstr2 = "You risk damaging your display and even starting a fire if you ";
const char* poofstr3 = "select a resolution your display isn't designed to support. ";
const char* poofstr4 = "Read the owner's guide that came with your display ";
const char* poofstr5 = "to make sure it supports this very high resolution.";

const float kAlertWidth = 350;
const float kAlertHeight = 240;

#if SHOW_COLOR
TPoofAlert::TPoofAlert(alert_type type)
	: BWindow(BRect(0,0,kAlertWidth, kAlertHeight), "Poof",
		B_NO_BORDER_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE),
			fAlertType(type)
#else
TPoofAlert::TPoofAlert(alert_type type)
	: BWindow(BRect(0,0,kAlertWidth, kAlertHeight), "Poof",
		B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE),
			fAlertType(type)
#endif
{
	fSkip = false;
	BRect r(Bounds());

	fBG = new TAlertView(r, fAlertType);
	fBG->SetFont(be_plain_font);
	AddChild(fBG);
	
	float fh = FontHeight(fBG, true);
	
	r.left = r.left + 55;
	r.right -= 10;
	r.top += 6;
	r.bottom = r.top + 4 + (fh * 5) + 4;
	BRect r2(r);
	r2.OffsetTo(0, 0);
	fMsgFld = new BTextView(r, "msg", r2, B_FOLLOW_NONE, B_WILL_DRAW);
	fMsgFld->MakeEditable(false);
	fMsgFld->MakeSelectable(false);
	fMsgFld->SetViewColor(fBG->ViewColor());
	fBG->AddChild(fMsgFld);
	fMsgFld->SetText(poofstr1);
	fMsgFld->Insert(strlen(poofstr1), poofstr2, strlen(poofstr2));
	fMsgFld->Insert(fMsgFld->TextLength(), poofstr3, strlen(poofstr3));
	fMsgFld->Insert(fMsgFld->TextLength(), poofstr4, strlen(poofstr4));
	fMsgFld->Insert(fMsgFld->TextLength(), poofstr5, strlen(poofstr5));

	r.right = Bounds().Width() - 10;
	r.left = r.right - 75;
	r.top = fMsgFld->Frame().bottom + 14;
	r.bottom = r.top + 20;	
	fCancelBtn = new BButton(r, "Cancel", "Cancel", new BMessage('exit'));
	fBG->AddChild(fCancelBtn);
	
	r.right = r.left - 10;
	r.left = r.right - 75;
	fPoofBtn = new BButton(r, "Poof!", "Poof!", new BMessage('poof'));
	fBG->AddChild(fPoofBtn);
	
	r.top += 4; r.bottom += 4;
	r.left = 55;
	r.right = r.left + fBG->StringWidth("Dont' show again") + 24;
	fBypassWarning = new BCheckBox(r, "bypass", "Don't show again", new BMessage('skip'));
	fBG->AddChild(fBypassWarning);
	
	ResizeTo(kAlertWidth, fPoofBtn->Frame().bottom + 10);
	
	CenterWindowOnScreen(this);
	SetDefaultButton(fCancelBtn);
}

TPoofAlert::~TPoofAlert()
{
}

void 
TPoofAlert::MessageReceived(BMessage *m)
{
	switch(m->what) {
		case 'skip':
			fSkip = fBypassWarning->Value();
			break;
		case 'exit':
			fAlertVal = 1;
			delete_sem(fAlertSem);
			fAlertSem = -1;
			break;
		case 'poof':
			fAlertVal = 0;
			delete_sem(fAlertSem);
			fAlertSem = -1;
			break;
	}
}

int32
TPoofAlert::Go()
{
	long		value;
	thread_id	this_tid = find_thread(NULL);
	BLooper		*loop;
	BWindow		*wind = NULL;

	fAlertSem = create_sem(0, "AlertSem");
	loop = BLooper::LooperForThread(this_tid);
	if (loop)
		wind = cast_as(loop, BWindow);

	Show();

	long err;

	if (wind) {
		// A window is being blocked. We'll keep the window updated
		// by calling UpdateIfNeeded.
		while (1) {
			while ((err = acquire_sem_etc(fAlertSem, 1, B_TIMEOUT, 50000)) == B_INTERRUPTED)
				;
			if (err == B_BAD_SEM_ID)
				break;
			wind->UpdateIfNeeded();
		}
	} else {
		do {
			err = acquire_sem(fAlertSem);
		} while (err == B_INTERRUPTED);
	}

	// synchronous call to close the alert window. Remember that this will
	// 'delete' the object that we're in. That's why the return value is
	// saved on the stack.
	value = fAlertVal;

	if (Lock()) {
		Quit();
	}

	return value;
}

//************************************************************************************

//	checks the resolution and color space against a known list
//	of possibly damaging configurations
//	returns true if the combination is okay or if the user
//	wants to use the combination regardless of the warning
static bool
_ValidateResolution(BWindow* w, bool protect, resolution res, color_space colors)
{
	if (!protect)
		return true;
	
	bool retval = true;
	float width, height;
	DimensionsFor(res, &width, &height);
	uint32 screenSpace = PartsToScreenSpace(colors, width, height);

	if (gShowPoofDialog &&
		(screenSpace & (	B_8_BIT_1152x900 | B_8_BIT_1280x1024  | B_8_BIT_1600x1200|
					B_15_BIT_1152x900| B_15_BIT_1280x1024 | B_15_BIT_1600x1200|
					B_16_BIT_1152x900| B_16_BIT_1280x1024 | B_16_BIT_1600x1200|
					B_32_BIT_1152x900| B_32_BIT_1280x1024 | B_32_BIT_1600x1200))) {

		TPoofAlert* a = new TPoofAlert();
		if (a->Go() == 1)
			retval = false;
		else
			//	disable the dialog from here on
			gShowPoofDialog = !a->Skip();
	}
	
	return retval;
}

//	positions the window relative to the screen size
static void
PositionWindow(BWindow* w, float oldWidth, float oldHeight,
	float newWidth, float newHeight)
{
	if (!w)
		return;
		
	if (oldWidth != newWidth || oldHeight != newHeight) {
		BPoint loc;
		float x = w->Frame().left / (oldWidth - w->Frame().Width());
		loc.x = (int)((newWidth - w->Frame().Width()) * x);	// + .5);
		float y = w->Frame().top / (oldHeight - w->Frame().Height());
		loc.y = (int)((newHeight - w->Frame().Height()) * y);	// + .5);

		w->MoveTo(loc);
	}
}

//	sets the current workspace to the requested settings
//	
static bool
_Configure(BWindow* w, bool confirmChanges, bool protect,
	resolution res, color_space colors, float rate, bool checkAllSettings)
{
	bool 			changesMade = false;
	float			oldWidth = ScreenWidth();
	float			oldHeight = ScreenHeight();
	color_space		oldColors = BitsPerPixel();
	float			oldRefreshRate = RefreshRate();
	
	float			width, height;
	
	DimensionsFor(res, &width, &height);
	
	//	make absolutely certain they want to blow up their monitor
	if (!_ValidateResolution(w, protect, res, colors))
		return false;
	
	//	position the window in the same relative location
	//	based on the old dimensions and the new ones
	BPoint oldLoc;
	window_feel windowFeel = B_NORMAL_WINDOW_FEEL;
	uint32 windowFlags = 0;
	if (w) {	
		oldLoc = w->Frame().LeftTop();	
		PositionWindow(w, oldWidth, oldHeight, width, height);

		if (checkAllSettings) {
			//	lock the window to the current workspace
			windowFeel = w->Feel();
			windowFlags = w->Flags();
			w->SetFeel(B_MODAL_ALL_WINDOW_FEEL);
			w->SetFlags(w->Flags()|0x00100000);
		}
	}

	float min, max;
	RateLimits(&min, &max);

	if (checkAllSettings) {
		if (colors != BitsPerPixel() && colors != B_NO_COLOR_SPACE)
			changesMade = true;
		
		if (colors != BitsPerPixel() || width != ScreenWidth() || height != ScreenHeight())
			changesMade = true;
	} else
		changesMade = true;
		
	if (changesMade) {
		SetResolution(colors, width, height);
	}
	
	if (rate >= min && rate <= max) {
		if (rate != RefreshRate()) {
			SetRefreshRate(rate);
			changesMade = true;
		}
	} else
		printf("ERROR: refresh rate of %.1f is out of bounds (%.1f / %.1f)\n",
			rate, min, max);
			
	//	nothing has changed
	if (!changesMade || !confirmChanges)
	{
		if (checkAllSettings && w) {
			// revert the window to its original state
			w->SetFeel(windowFeel);
			w->SetFlags(windowFlags);
		}
		return false;
	}
		
	//	if the ok button is not pressed in the confirm dialog
	//	all the settings except the desktop color will be reverted
	//	to the last settings
	bool retVal = true;
	TConfirmWindow	*wind = new TConfirmWindow();	
	if  (wind->Go() == 1) {
		SetResolution(oldColors, oldWidth, oldHeight);
		SetRefreshRate(oldRefreshRate);
		
		if (w) {
			w->MoveTo(oldLoc);

		}

		retVal = false;		
	}
	
	if (checkAllSettings && w) {
		// revert the window to its original state
		w->SetFeel(windowFeel);
		w->SetFlags(windowFlags);
	}

	return retVal;
}

static void
_ConfigureTo(BWindow* w, bool c, bool p, monitor_settings s)
{
	_Configure(w, c, p, s.dimensions, s.colorSpace, s.refreshRate, true);
}

// 	if the user did not confirm the changes then
//	this will return false
static bool
_ConfigureWorkspace(BWindow* w, bool confirmChanges, bool protect,
	resolution res, color_space colors, float rate, int32 ws)
{
	bool retVal = false;
	int32 startWS = current_workspace();
	if (ws == -1) {
		BAlert* a = new BAlert("","Change all workspaces? This action cannot be reverted.",
			"Okay", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT );
			
		if (a->Go() == 0) {

			//	validate the settings just once
			if (!_ValidateResolution(w, protect, res, colors)) {
				retVal = false;
				goto DONE;
			}

			// 	configure the first workspace,
			//	prompt the user to confirm
			//	if okay, change the remaining workspaces
			if (_Configure(w, true, false, res, colors, rate, false)) {
				int32 ws_count = count_workspaces();

				for (int32 i=0 ; i< ws_count ; i++) {
					if (i != startWS) {
						activate_workspace(i);
						
						//	no dialogs confirming changes
						//	no resolution validation
						_Configure(w, false, false, res, colors, rate, false);
					}
				}
			}
		}
				
	} else {
		if (ws != current_workspace())
			activate_workspace(ws);

		retVal = _Configure(w, confirmChanges, protect, res, colors, rate, true);
	}		

	if (startWS != current_workspace())
		activate_workspace(startWS);

DONE:	
	return retVal;
}

static int
SettingsFileRef(const char* fileName, char* path, bool create)
{
	BPath	filePath;
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &filePath) == B_OK) {
		int ref = -1;
		
		filePath.Append(fileName);
		
		ref = open(filePath.Path(), O_RDWR);
		
		if (ref < 0 && create) {
			ref = creat(filePath.Path(), 0777);
			if (ref < 0) 
				goto FAIL;
		} 
		strcpy(path, filePath.Path());
		
		return ref;
	}
	
FAIL:
	path[0] = 0;
	
	return -1;
}

// ************************************************************************** //

int main()
{	
	TApp app;
	app.Run();
	
	return B_NO_ERROR;
}

// ************************************************************************** //

#if SHOW_PICTURE_BTN
const int32 msg_show_help				= 'help';
#endif
const int32 msg_cmdline_set_workspace	= 'wcst';

const int32 msg_default_resolution 		= 'dres';
const int32 msg_default_refreshrate 	= 'dref';
const int32 msg_default_position 		= 'dpos';

#if SHOW_CONFIG_WS
const int32	msg_ws_set					= 'wsst';
#endif
const int32 msg_refresh_apply_now		= 'rnow';
const int32 msg_refresh_set				= 'rfst';
const int32 msg_refresh_done 			= 'rdne';
const int32 msg_refresh_cancel 			= 'rcan';
const int32 msg_refresh_apply			= 'rapl';

const int32 msg_revert 					= 'rvrt';
const int32 msg_defaults 				= 'dflt';
const int32 msg_save_settings 			= 'sset';
const int32 msg_use_settings 			= 'uset';
const int32 msg_use_all 				= 'usal';

const int32 msg_ws_all 					= 'wsal';
const int32 msg_config_ws				= 'wscn';

const int32 msg_choose_resolution 		= 'res ';
const int32 msg_choose_color 			= 'colr';
const int32 msg_choose_refresh_rate 	= 'rate';
const int32 msg_choose_desktop_color 	= 'desk';
const int32 msg_choose_decor 			= 'deco';
const int32 msg_change_workspace_count 	= 'work';
const int32 msg_set_monitor				= 'mset';

const int32 msg_config_update = 'cupd';
const int32 msg_refresh_update = 'rupd';

const char* const kSettingsFile = "Screen_data";

TApp::TApp()
	:BApplication("application/x-vnd.Be-SCRN")
{
	gShowPoofDialog = true;
	fShowGUI = true;
	fConfirmChanges = true;
	fWindow = new TWindow();
}

TApp::~TApp()
{
}

void
TApp::AboutRequested()
{
	BAlert *myAlert;

	myAlert = new BAlert("","The place to configure your monitor.", "OK");
	if (myAlert->Go() == 0) {
		;
	}
}

// Command line constants
const char*	const kHelp 		= "--help"; 	// print help.
const char*	const kForceChange 	= "-f"; 		// no ack on changes.
const char*	const kResolution 	= "-res"; 		// set resolution.
const char*	const kRefreshRate 	= "-ref"; 		// set ref. rate.
const char*	const kNoGUI 		= "-nogui"; 	// don't show gui.
const char* const kWorkspace	= "-w";			

const char*	const k800x600 		= "800x600";
const char*	const k640x480 		= "640x480";
const char*	const k1024x768 	= "1024x768";
const char*	const k1152x900 	= "1152x900";
const char*	const k1280x1024 	= "1280x1024";
const char*	const k1600x1200 	= "1600x1200";

void
TApp::ArgvReceived(int32 argc, char** argv)
{
	int32 		workspace = current_workspace();
	resolution 	dimensions = Resolution();
	color_space c_space = BitsPerPixel();
	float 		refresh_rate = RefreshRate();
	
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], kResolution) == 0) {
			i++;
			if (strcmp(argv[i], k640x480) == 0)
				dimensions = k640x480Resolution;
			else if (strcmp(argv[i], k800x600) == 0)
				dimensions = k800x600Resolution;
			else if (strcmp(argv[i], k1024x768) == 0)
				dimensions = k1024x768Resolution;
			else if (strcmp(argv[i], k1152x900) == 0)
				dimensions = k1152x900Resolution;
			else if (strcmp(argv[i], k1280x1024) == 0)
				dimensions = k1280x1024Resolution;
			else if (strcmp(argv[i], k1600x1200) == 0)
				dimensions = k1600x1200Resolution;
			
			i++;
			if (i < argc) {
				if (strcmp(argv[i], "8") == 0) 
					c_space = B_COLOR_8_BIT;
				else if (strcmp(argv[i], "15") == 0) 
					c_space = B_RGB15;
				else if (strcmp(argv[i], "16") == 0) 
					c_space = B_RGB16;
				else if (strcmp(argv[i], "32") == 0) 
					c_space = B_RGB32;
			}

		} else if (strcmp(argv[i], kRefreshRate) == 0)
			refresh_rate = atof(argv[++i]);
		else if (strcmp(argv[i], kNoGUI) == 0)
			fShowGUI = false;
		else if (strcmp(argv[i], kForceChange) == 0)
			fConfirmChanges = false;
		else if (strcmp(argv[i], kHelp) == 0) {
			fShowGUI = false;
			PrintHelp();
		} else if (strcmp(argv[i], kWorkspace) == 0)
			workspace = atoi(argv[++i]);

	}
	
	// send a message to the window
	//	resolution, refresh rate, dont confirm
	if (fShowGUI) {
		BMessage msg(msg_cmdline_set_workspace);
		msg.AddInt32("workspace", workspace);
		msg.AddInt32("resolution", dimensions);
		msg.AddInt32("colors", c_space);
		msg.AddFloat("refresh", refresh_rate);
		msg.AddBool("dontconfirm", fConfirmChanges);
		msg.AddBool("nogui", fShowGUI);
		fWindow->PostMessage(&msg);
	} else {
		//	from command line, just do it
		_ConfigureWorkspace(NULL, fConfirmChanges, true,
			dimensions, c_space, refresh_rate, workspace);
	}
}

void 
TApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}

void
TApp::ReadyToRun()
{
	if (fShowGUI == false) {
		BMessage quitMsg(B_QUIT_REQUESTED);
		PostMessage(&quitMsg);
	} else 
		fWindow->Show();
}

void
TApp::PrintHelp()
{
	printf("Screen Preferences\n");
	printf("Usage: Screen [-f] [-nogui] [-res <resolution> <depth>] [-ref <rate>]\n");
	printf("\n\t-f\tForce changes - no warnings issued if resolution or refresh rate\n");
	printf("\t\tcould potentially damage your monitor.\n");
	printf("\t-nogui\tExits when done, window is not presented\n");
	printf("\t-res\tSet the resolution and color depth to the specified values\n");
	printf("\t\tValid resolution values are: \n");
	printf("\t\t\t640x480 800x600 1024x768 1280x1024 1600x1200\n");
	printf("\t\tValid color depth values are:\n");
	printf("\t\t\t8 15 16 32\n");
	printf("\t-ref\tSet the refresh rate; values normally between 56 and 84 Hz\n");
	printf("\t-w\tThe workspace to modify\n");
	printf("\t\t-1 for all workspaces (NOTE: no confirmation dialogs will be presented)\n");
	printf("\t\t0  for the first workspace and so on. \n\n");
}

// ************************************************************************** //

class TKeyFilter : public BMessageFilter {
public:
					TKeyFilter(	message_delivery delivery,
								message_source source,
								filter_hook func = NULL);
							
	filter_result	Filter(BMessage *message, BHandler **target);
};

TKeyFilter::TKeyFilter(message_delivery delivery, message_source source,
	filter_hook func)
	: BMessageFilter(delivery, source, func)
{
}

filter_result 
TKeyFilter::Filter(BMessage *message, BHandler **target)
{
	if (message->what == B_KEY_DOWN) {
		int8 byte;
		message->FindInt8("byte", &byte);

		TWindow* parent = dynamic_cast<TWindow*>(*target);
		if (!parent)
			goto DISPATCH;
		
		ulong mods = message->FindInt32("modifiers");
	
		switch(byte) {
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
				parent->DoKeyboardControl( byte, mods);
				break;
			default:
				goto DISPATCH;
				break;
		}
		return B_SKIP_MESSAGE;
	}

DISPATCH:
		return B_DISPATCH_MESSAGE;
}

// ************************************************************************** //

const int32 kWindowWidth = 356;
const int32 kWindowHeight = 232;

TWindow::TWindow()
	 :BWindow( BRect(0, 0, kWindowWidth, kWindowHeight), "Screen",
		B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS,
		B_ALL_WORKSPACES)
{
	bool applyCustomNow=false;
	GetPrefs(&applyCustomNow);

	BRect r(Bounds());
	r.InsetBy(-1, -1);
	fMainBG = new TBox(r, applyCustomNow);
	AddChild(fMainBG);

	//	add shortcuts for
	//		default resolution
	//		default refresh rate
	//		default monitor position
	//		workspace count dialog
	AddShortcut('I', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));

	AddShortcut('D', B_COMMAND_KEY, new BMessage(msg_default_resolution),
		fMainBG);
	AddShortcut('R', B_COMMAND_KEY, new BMessage(msg_default_refreshrate),
		fMainBG);
	AddShortcut('P', B_COMMAND_KEY, new BMessage(msg_default_position),
		fMainBG);

#if SHOW_CONFIG_WS
	AddShortcut('X', B_COMMAND_KEY, new BMessage(msg_ws_set), fMainBG);
#endif
	AddShortcut('C', B_COMMAND_KEY, new BMessage(msg_refresh_set), fMainBG);

	AddShortcut('D', B_SHIFT_KEY, new BMessage(msg_defaults), fMainBG);
	AddShortcut('R', B_SHIFT_KEY, new BMessage(msg_revert), fMainBG);
	AddShortcut('S', B_COMMAND_KEY, new BMessage(msg_save_settings), fMainBG);
	AddShortcut('U', B_COMMAND_KEY, new BMessage(msg_use_settings), fMainBG);
	AddShortcut('U', B_SHIFT_KEY, new BMessage(msg_use_all), fMainBG);

	AddShortcut('Z', B_COMMAND_KEY, new BMessage('blah'), fMainBG);

	//	grab the arrow keys so that
	//	the monitor can be configured
	TKeyFilter* filter = new TKeyFilter(B_ANY_DELIVERY, B_ANY_SOURCE);
	AddFilter(filter);
}

TWindow::~TWindow()
{
}

void
TWindow::FrameResized(float w, float h)
{
	BWindow::FrameResized(w,h);
}

void
TWindow::MessageReceived(BMessage* m)
{
	switch(m->what) {
		case B_ABOUT_REQUESTED:
			// 	a modal dialog is about to appear
			//	when it goes away, don't let some other
			//	window grab the focus
			be_app->MessageReceived(m);
			break;
			
		case 'skip':
			{
				bool skip;
				m->FindBool("skip", &skip);
				printf("skipping %i\n", skip);
			}
			break;

		case msg_refresh_done:
		case msg_refresh_cancel:
		case msg_refresh_apply_now:
#if SHOW_CONFIG_WS
		case msg_config_cancel:
		case msg_config_okay:
#endif
			//	revert the window to its original settings
			//	as per the oddities listed above
			PostMessage(m, fMainBG);
			break;
			
		case msg_cmdline_set_workspace:
			PostMessage(m, fMainBG);
			break;
			
		default:
			BWindow::MessageReceived(m);
	}
}

void
TWindow::GetPrefs(bool *applyCustomNow)
{
	char path[B_PATH_NAME_LENGTH];
	
	int fileRef = SettingsFileRef(kSettingsFile, path, false);
	
	if (fileRef >= 0){
		BPoint loc;

		read(fileRef, &loc, sizeof(BPoint));
		
		if (read(fileRef, applyCustomNow, sizeof(bool)) != sizeof(bool))
			*applyCustomNow = false;
		if (read(fileRef, &gShowPoofDialog, sizeof(bool)) != sizeof(bool))
			gShowPoofDialog = true;
		
		close(fileRef);

		if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(loc)) {
			MoveTo(loc);
			return;
		}
	}
	
	// 	if prefs dont yet exist or the window is not onscreen,
	//		center the window
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);
}

bool
TWindow::QuitRequested()
{
	char path[B_PATH_NAME_LENGTH];

	BPoint loc = Frame().LeftTop();

	int fileRef = SettingsFileRef(kSettingsFile, path, true);
	if (fileRef >= 0) {
		write(fileRef, &loc, sizeof(BPoint));
		
		bool flag = fMainBG->ApplyCustomNow();
		write(fileRef, &flag, sizeof(bool));
		write(fileRef, &gShowPoofDialog, sizeof(bool));
		
		close(fileRef);
	}				

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
TWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws)
{
	BWindow::WorkspacesChanged(old_ws, new_ws);
}

void
TWindow::WorkspaceActivated(int32 ws, bool state)
{
	BWindow::WorkspaceActivated(ws, state);
	
	//	update the controls with the new workspace settings
	fMainBG->WorkspaceActivated(ws, state);
}

//	l/r arrow keys - refresh rate
//	shift key + arrow keys change inc/dec size
//	control key + arrow keys change position
void
TWindow::DoKeyboardControl(int8 key, ulong mods)
{
	bool			do_size = false;
	char			old_h_pos, old_v_pos;
	char			old_h_size, old_v_size;
	screen_desc 	desc;
	
	BScreen(this).private_screen()->get_screen_desc(&desc);

	if ((key >= B_LEFT_ARROW) && (key <= B_DOWN_ARROW)) {
		if ((mods & B_SHIFT_KEY) || (mods & B_CONTROL_KEY)) {
			if (mods & B_SHIFT_KEY)
				do_size = true;
			old_h_pos = desc.h_pos; old_v_pos = desc.v_pos;
			old_h_size = desc.width; old_v_size = desc.height;
			switch (key) {
				case B_LEFT_ARROW:
					if (do_size)
						desc.width = max(0, desc.width - 10);
					else
						desc.h_pos = max(0, desc.h_pos - 10);
					break;

				case B_RIGHT_ARROW:
					if (do_size)
						desc.width = min(100, desc.width + 10);
					else
						desc.h_pos = min(100, desc.h_pos + 10);
					break;

				case B_UP_ARROW:
					if (do_size)
						desc.height = max(0, desc.height - 10);
					else
						desc.v_pos = max(0, desc.v_pos - 10);
					break;

				case B_DOWN_ARROW:
					if (do_size)
						desc.height = min(100, desc.height + 10);
					else
						desc.v_pos = min(100, desc.v_pos + 10);
					break;
			}
			if ((old_h_pos != desc.h_pos) ||
				(old_v_pos != desc.v_pos) ||
				(old_h_size != desc.width) ||
				(old_v_size != desc.height))
				SetCRTPosition(desc.h_pos, desc.v_pos,
							   desc.width, desc.height);
		} else {
			float min, max;
			float rate = RefreshRate();

			RateLimits(&min, &max);
			if (min == max)
				return;

			if (key == B_LEFT_ARROW) {
				rate = rate - 0.1;
				if (rate < min)
					return;
			} else if (key == B_RIGHT_ARROW) {
				rate = rate + 0.1;
				if (rate > max)
					return;
			} else
				return;

			SetRefreshRate(rate);						// set the monitor
			fMainBG->SetMonitorRefreshRate(rate);		// set the UI
			fMainBG->UpdateControls(false);
		}
	}
}

// ****************************************************************************** //

TBox::TBox(BRect frame, bool applyCustomNow)
	: BBox(frame, "box", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS)
{
	fApplyCustomNow = applyCustomNow;
	fAllWorkspaces = false;
	fCanRevert = false;
	GetMonitorSettings();
	AddParts();	
	GetSettings(&fSavedSettings);
}

TBox::~TBox()
{
}

void
TBox::AttachedToWindow()
{
	fTargetMenu->SetTargetForItems(this);
	fResolutionMenu->SetTargetForItems(this);
	fColorsMenu->SetTargetForItems(this);
	fRefreshRateMenu->SetTargetForItems(this);
	fDecorMenu->SetTargetForItems(this);
	
	fApplyBtn->SetTarget(this);
	
	fRevertBtn->SetTarget(this);
	fDefaultsBtn->SetTarget(this);
	
#if SHOW_PICTURE_BTN
	fHelpBtn->SetTarget(this);
#endif
#if SHOW_CONFIG_WS
	fWorkspaceInfoBtn->SetTarget(this);
#endif
}

void
TBox::Draw(BRect r)
{
	BBox::Draw(r);

	//	horizontal btn separator	
	if (!fDesktopColorCntl->IsHidden()) {
		SetHighColor(kMediumGray);
		StrokeLine(BPoint(12, fDefaultsBtn->Frame().top-11),
			BPoint(Bounds().Width()-12, fDefaultsBtn->Frame().top-11));
		SetHighColor(kWhite);
		StrokeLine(BPoint(12, fDefaultsBtn->Frame().top-10),
			BPoint(Bounds().Width()-12, fDefaultsBtn->Frame().top-10));
	}

#if SHOW_CONFIG_WS
	//	vertical btn separator		
	SetHighColor(kMediumGray);
	StrokeLine(	BPoint(	fWorkspaceInfoBtn->Frame().right+10,
						fWorkspaceInfoBtn->Frame().top),
				BPoint(	fWorkspaceInfoBtn->Frame().right+10,
						fWorkspaceInfoBtn->Frame().bottom));
	SetHighColor(kWhite);
	StrokeLine(	BPoint(	fWorkspaceInfoBtn->Frame().right+11,
						fWorkspaceInfoBtn->Frame().top),
				BPoint(	fWorkspaceInfoBtn->Frame().right+11,
						fWorkspaceInfoBtn->Frame().bottom));
#endif
}

void
TBox::KeyDown(const char *bytes, int32 numbytes)
{
	BBox::KeyDown(bytes, numbytes);
}

void
TBox::MessageReceived(BMessage* m)
{
	switch (m->what) {
		case 'blah':
			{
				float w = Window()->Bounds().Width();
				float h = Window()->Bounds().Height();
				if (h == 292) {
					h = 202;
					fDesktopColorCntl->Hide();
				} else {
					h = 292;
					fDesktopColorCntl->Show();
				}
				Window()->ResizeTo(w, h);
			}
			break;
			
#if SHOW_PICTURE_BTN
		case msg_show_help:
			ShowHelp();
			break;
#endif
			
		case msg_revert:					// cmd-shift R
			Revert();
			break;
		case msg_defaults:					// cmd-shift D
			Defaults();
			break;			
		
		//	individual default settings	
		case msg_default_resolution:		//	command D
			SetRevertSettings();
			SetDefaultResolution();
			SetRevert(true);
			break;
		case msg_default_refreshrate:		// command R
			SetRevertSettings();
			SetDefaultRefreshRate();
			SetRevert(true);
			break;
		case msg_default_position:			//	command P
			SetDefaultCRTPosition();
			break;

		//	saved settings	
		case msg_save_settings:				// cmd S
			SaveSettings();
			break;
		case msg_use_settings:				// cmd U, current workspace
			UseSettings(fSavedSettings);
			break;
		case msg_use_all:					// cmd-shift U, all workspaces
			UseSettings(fSavedSettings, true);
			break;
		
		//	workspace
		case msg_ws_all:					// all or current ws
			{
				int32 value=0;
				m->FindInt32("workspace", &value);
				if (value) {
					fAllWorkspaces = true;
					UpdateControls(true);
					SetRevert(false);		// nothing to revert, set it back explicitly
				} else {
					fAllWorkspaces = false;
					fTargetWorkspace = current_workspace();
					UpdateControls(		(fColors != BitsPerPixel())
									|| 	(fRefreshRate != RefreshRate())
									|| 	(fResolution != Resolution()) );
				}
			}
			break;
#if SHOW_CONFIG_WS
		case msg_ws_set:					// cmd I
			ShowWorkspaceConfig();
			break;
		case msg_config_cancel:
		case msg_config_okay:
			break;
#endif

		//	show the custom refresh rate dialog
		case msg_refresh_set:
			ShowRefreshRateConfig();
			break;			
		// 	cancel button was pressed in the refresh rate dialog
		case msg_refresh_cancel:
			UpdateControls(false);
			SetRevert(false);
			break;
		//	OK button was pressed in the refresh rate dialog
		case msg_refresh_done:
			//	if the apply now is not set the actual rr will not match
			//	thus it is the intended refresh rate
			m->FindFloat("rate", &fRefreshRate);
			{
				bool test = CompareFloats(fRefreshRate, RefreshRate());
				SetRevert(true);
				UpdateControls(!test);
			}
			break;
		case msg_refresh_apply:
			//	apply btn was pressed in dialog,
			//	keep everything in sync
			m->FindFloat("rate", &fRefreshRate);
			{
				bool test = CompareFloats(fRefreshRate, RefreshRate());
				SetRevert(true);
				UpdateControls(!test);
			}
			break;
		case msg_refresh_apply_now:
			fApplyCustomNow = !fApplyCustomNow;
			break;

		//	sent from resolution btn
		case msg_choose_resolution:
			{
				int32 value;
				m->FindInt32("resolution", &value);
				SetMonitorResolution((resolution)value);
			}
			break;
			
		//	sent from color btn
		case msg_choose_color:
			{
				int32 value;
				color_space colors;
				m->FindInt32("color", &value);
				
				switch (value) {
					case 0:		colors = B_COLOR_8_BIT; 	break;
					case 1: 	colors = B_RGB15;			break;
					case 2: 	colors = B_RGB16;			break;
					case 3: 	colors = B_RGB32;			break;
					default: 	colors = B_NO_COLOR_SPACE;	break;
				}
				
				SetMonitorColors(colors);
			}
			break;
		
		//	sent from refresh rate btn
		case msg_choose_refresh_rate:
			{
				int32 value;
				float rate = fRefreshRate;
				m->FindInt32("rate", &value);
				
				switch (value) {
					case 0: 	rate = 56.0; break;
					case 1: 	rate = 60.0; break;
					case 2: 	rate = 70.0; break;
					case 3: 	rate = 72.0; break;
					case 4: 	rate = 75.0; break;
#if _CONSTRAIN_REFRESH_RATE_
					case 5:		ShowRefreshRateConfig(); break;
#else
					case 5: 	rate = 90.0; break;
					case 6:		ShowRefreshRateConfig(); break;
#endif
					default:	rate = 0.0;		break;
				}
			
				SetMonitorRefreshRate(rate);
			}
			break;
			
		case msg_choose_desktop_color:
			//	message sent from the color control, desktop color has
			//	already changed tell the monitor thing to update its color
			DesktopColorChange();
			break;
	
		case msg_choose_decor:
			{
				const char* decor;
				if (m->FindString("decor", &decor) == B_OK) {
					fDecorName = decor;
					set_window_decor(decor);
					SetRevert(true);
				}
			}
			break;
			
		case msg_cmdline_set_workspace:
			// grab the components from the message
			{
				int32 temp=0;
				if (m->FindInt32("workspace", &temp) == 0) {
					fTargetWorkspace = temp;
					if (temp == -1)
						fAllWorkspaces = true;
				}
				if (m->FindInt32("resolution", &temp) == 0)
					fResolution = (resolution)temp;
				if (m->FindInt32("colors", &temp) == 0)
					fColors = (color_space)temp;

				float r;
				if (m->FindFloat("refresh", &r) == 0)
					fRefreshRate = r;
				
#if 0
				//	ignore these for now	
				bool test;				
				if (m->FindBool("dontconfirm", &test) == 0)
					bool dont_confirm = test;
				if (m->FindBool("nogui", &test) == 0)
					bool no_gui = test;
#endif
			}
		
		case msg_set_monitor:
			SetMonitorForWorkspace();
			break;
			
		default:
			BBox::MessageReceived(m);
			break;
	}
}

void
TBox::WorkspaceActivated(int32 , bool state)
{
	if (state) {
		GetMonitorSettings();
		UpdateControls(false);
		UpdateDesktopColor();
		
		SetRevert(false);	// current settings, no revert
	}
}

void
TBox::AddParts()
{		
	AddMiniScreen(BPoint(12, 19));
	AddScreenControls();
	AddDesktopColorSelector();
	AddButtons();

	SetRevert(false);
	
	//	put the controls in sync with the monitor settings
	UpdateControls(false);
}

//

const int32 resolution_count = 6;
const char* const resolution_list[] = {
	"640 x 480",
	"800 x 600",
	"1024 x 768",
	"1152 x 864",		// bug #11325
	"1280 x 1024",
	"1600 x 1200"
};

const int32 colors_count = 4;
const char* const colors_list[] = {
	"8 Bits/Pixel",
	"15 Bits/Pixel",
	"16 Bits/Pixel",
	"32 Bits/Pixel"
};

//	!!
//	since we cannot really tell what the max refresh rate is
//	without actually changing the resolution, which is inconvenient
//	we will limit the refresh rates, in the popup to some known
//	possible rates, the max rate is left out from this menu
//	it will be accessible via the other menu, after we set the resolution
//
#if _CONSTRAIN_REFRESH_RATE_
const int32 refresh_rate_count = 6;				
#else
const int32 refresh_rate_count = 7;
#endif
const char* const refresh_rate_list[] = {
	"56 Hz",
	"60 Hz",
	"70 Hz",
	"72 Hz",
	"75 Hz",
#if _CONSTRAIN_REFRESH_RATE_ == 0
	"90 Hz",
#endif
	"Other..."
};

void
TBox::AddMiniScreen(BPoint loc)
{
	int32 w = 1920/16;
	int32 h = 1440/16;
	BRect r( loc.x, loc.y, w + 16 + loc.x + 6, h + 16 + loc.y + 31);
	
	fMiniScreen = new TScreenThing( r, fResolution, fDesktopColor);
	AddChild(fMiniScreen);
}

void
TBox::AddScreenControls()
{
	BRect r(fMiniScreen->Frame());	
	BMessage* msg=NULL;
	BMenuItem* mitem=NULL;

	r.top -= 11;		//	offset top to accomodate the labelview of the bbox
	r.left = r.right + 11;
	r.right = Bounds().Width() - 12;
	fScreenBox = new BBox(r, "screen controls", B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(fScreenBox);
	
	//	workspace target menu
	fTargetMenu = new BPopUpMenu("ws menu");
	msg = new BMessage(msg_ws_all);
	msg->AddInt32("workspace", 1);
	mitem = new BMenuItem("All Workspaces", msg);
	fTargetMenu->AddItem(mitem);
	msg = new BMessage(msg_ws_all);
	msg->AddInt32("workspace", 0);
	mitem = new BMenuItem("Current Workspace", msg);
	mitem->SetMarked(true);
	fTargetMenu->AddItem(mitem);

	r.top = 0; r.bottom = r.top + 18;
	r.left = 0; r.right = r.left + StringWidth("Current Workspace") + 20;
	fTargetBtn = new BMenuField(r, "ws btn", "", fTargetMenu, true);
	fTargetBtn->SetFontSize(10);
	(fTargetBtn->MenuBar())->SetFontSize(10);
	(fTargetBtn->Menu())->SetFontSize(10);
	fTargetBtn->SetDivider(0);
	
	fScreenBox->SetLabel(fTargetBtn);

	//		Resolution Menu
	fResolutionMenu = new BPopUpMenu("resolution");
	for (int32 i=0 ; i<resolution_count ; i++) {
		msg = new BMessage(msg_choose_resolution);
		msg->AddInt32("resolution", i);
		mitem = new BMenuItem(resolution_list[i], msg);
		fResolutionMenu->AddItem(mitem);
	}

	r.top = 30; r.bottom = r.top + 18;
	r.left = 9;
	r.right = r. left + StringWidth("Refresh Rate:") + StringWidth("0000 x 0000") + 14;
	fResolutionBtn = new BMenuField(r, "", "Resolution:", fResolutionMenu, true);
	fResolutionBtn->SetFontSize(10);
	fResolutionBtn->SetAlignment(B_ALIGN_RIGHT);
	(fResolutionBtn->MenuBar())->SetFontSize(10);
	(fResolutionBtn->Menu())->SetFontSize(10);
	fResolutionBtn->SetDivider(StringWidth("Refresh Rate:"));
	
	//		Bit Depth / Colors Button
	fColorsMenu = new BPopUpMenu("colors");
	for (int32 i=0 ; i<colors_count ; i++) {
		msg = new BMessage(msg_choose_color);
		msg->AddInt32("color", i);
		mitem = new BMenuItem(colors_list[i], msg);
		fColorsMenu->AddItem(mitem);
	}

	r.top = r.bottom + 10;
	r.bottom = r.top + 18;
	fColorsBtn = new BMenuField(r, "", "Colors:", fColorsMenu, true);
	fColorsBtn->SetFontSize(10);
	fColorsBtn->SetAlignment(B_ALIGN_RIGHT);
	(fColorsBtn->MenuBar())->SetFontSize(10);
	(fColorsBtn->Menu())->SetFontSize(10);
	fColorsBtn->SetDivider(StringWidth("Refresh Rate:"));

	//		Refresh Rate Button
	fRefreshRateMenu = new BPopUpMenu("Other");
	for (int32 i=0 ; i<refresh_rate_count ; i++) {
		msg = new BMessage(msg_choose_refresh_rate);
		msg->AddInt32("rate", i);
		mitem = new BMenuItem(refresh_rate_list[i], msg);
		fRefreshRateMenu->AddItem(mitem);		
	}
	
	//	!! disable 'Other...' menu item, for now
#if _CONSTRAIN_REFRESH_RATE_
	mitem = fRefreshRateMenu->ItemAt(refresh_rate_count-1);
#endif

	r.top = r.bottom + 10;
	r.bottom = r.top + 18;
	fRefreshRateBtn = new BMenuField(r, "", "Refresh Rate:",
		fRefreshRateMenu, true);
	fRefreshRateBtn->SetFontSize(10);
	fRefreshRateBtn->SetAlignment(B_ALIGN_RIGHT);
	(fRefreshRateBtn->MenuBar())->SetFontSize(10);
	(fRefreshRateBtn->Menu())->SetFontSize(10);
	fRefreshRateBtn->SetDivider(StringWidth("Refresh Rate:"));

	//		Set Button
	r.bottom = fScreenBox->Bounds().Height() - 10;
	r.top = r.bottom - (FontHeight(this, true) + 10);
	r.left = fRefreshRateBtn->Divider() + fRefreshRateBtn->Frame().left;
	r.right = r.left + 75;
	fApplyBtn = new BButton(r, "apply", "Apply", new BMessage(msg_set_monitor));
	fApplyBtn->SetFontSize(10);
	
	fScreenBox->AddChild(fResolutionBtn);
	fScreenBox->AddChild(fColorsBtn);
	fScreenBox->AddChild(fRefreshRateBtn);
	
	fScreenBox->AddChild(fApplyBtn);
}

void
TBox::AddDesktopColorSelector()
{
	BPoint pt(fMiniScreen->Frame().left, fMiniScreen->Frame().bottom + 11);
	fDesktopColorCntl = new TDesktopColorControl(pt,
		new BMessage(msg_choose_desktop_color), this);
	AddChild(fDesktopColorCntl);
	
	fDesktopColorCntl->Hide();
}

const char* const kRevertStr = "Revert";
const char* const kDefaultsStr = "Defaults";
#if SHOW_CONFIG_WS
const char* const kWorkspaceInfoStr = "Workspaces...";
#endif

static void get_decor_items(BList* into, directory_which which, const char* leaf)
{
	BPath path;
	BDirectory dir;
	entry_ref ref;
	
	if (find_directory(which, &path, false) != B_OK)
		return;
	if (path.Append(leaf) != B_OK)
		return;
	if (dir.SetTo(path.Path()) != B_OK)
		return;
	
	while (dir.GetNextRef(&ref) == B_OK) {
		BMessage* msg = new BMessage(msg_choose_decor);
		msg->AddString("decor", ref.name);
		BMenuItem* it = new BMenuItem(ref.name, msg);
		if (it)
			into->AddItem(it);
	}
}

static int decorcmp(const void* it1, const void* it2)
{
	BMenuItem* m1 = *(BMenuItem**)it1;
	BMenuItem* m2 = *(BMenuItem**)it2;
	if (m1 && m2) return strcmp(m1->Label(), m2->Label());
	else if (m1) return -1;
	return 1;
}

void
TBox::AddButtons()
{
	BRect r;
	
	r.bottom = Bounds().Height() - 12;
	r.top = r.bottom - (FontHeight(this, true) + 10);
#if SHOW_CONFIG_WS
	r.left = 11; r.right = r.left + 100;
	fWorkspaceInfoBtn = new BButton(r, "ws", kWorkspaceInfoStr, new BMessage(msg_ws_set),
		B_FOLLOW_BOTTOM);
	AddChild(fWorkspaceInfoBtn);
	r.left = r.right + 10 + 2 + 10;
	r.right = r.left + 75;
#else
	r.left = 11; r.right = r.left + 75;
#endif
	fDefaultsBtn = new BButton(r, "defaults", kDefaultsStr,
		new BMessage(msg_defaults), B_FOLLOW_BOTTOM);
	AddChild(fDefaultsBtn);

	r.left = r.right + 10;
	r.right = r.left + 75;
	fRevertBtn = new BButton(r, "revert", kRevertStr, new BMessage(msg_revert),
		B_FOLLOW_BOTTOM);
	AddChild(fRevertBtn);
	
#if SHOW_PICTURE_BTN
	r.right = Bounds().Width() - 15;
	r.left = r.right - 15;
	r.bottom = Bounds().bottom - 17;
	r.top = r.bottom - 15;
	fHelpBtn = new TPictureButton(r, new BMessage(msg_show_help));
	AddChild(fHelpBtn);
#endif

	r.bottom = r.top - 12;
	r.top = r.bottom - (FontHeight(this, true) + 10);
	r.left = fScreenBox->Frame().left;
	r.right = fScreenBox->Frame().right;
	
	//		Decor Name Button
	fDecorMenu = new BPopUpMenu("Decor");
	BList decorItems;
	get_decor_items(&decorItems, B_USER_CONFIG_DIRECTORY, "decors");
	get_decor_items(&decorItems, B_BEOS_ETC_DIRECTORY, "decors");
	decorItems.SortItems(decorcmp);
	BMessage* msg = new BMessage(msg_choose_decor);
	msg->AddString("decor", "DEFAULT");
	fDecorMenu->AddItem(new BMenuItem("Default", msg));
	if (decorItems.CountItems() > 0)
		fDecorMenu->AddSeparatorItem();
	fDecorMenu->AddList(&decorItems, 2);
	
	fDecorBtn = new BMenuField(r, "", "Decor:", fDecorMenu, true);
	fDecorBtn->SetFontSize(10);
	fDecorBtn->SetAlignment(B_ALIGN_RIGHT);
	(fDecorBtn->MenuBar())->SetFontSize(10);
	(fDecorBtn->Menu())->SetFontSize(10);
	fDecorBtn->SetDivider(StringWidth("Decor:"));
	AddChild(fDecorBtn);
}

void
TBox::UpdateControls(bool canSet)
{
	if (canSet)
		SetRevertSettings();
	SetRevert(canSet);				//	bug #11413
	
	fApplyBtn->SetEnabled(canSet);
	
	bool enableMenuItem=false;
	uint32 colorSpace = MonitorColorSpace();
	BMenuItem *mitem;

	// disable the resolutions that are not available for the current bit depth setting
	enableMenuItem = (PartsToScreenSpace( fColors, 640, 480) & colorSpace);
	mitem = fResolutionMenu->ItemAt(0);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (fResolution == 0 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}
	enableMenuItem = (PartsToScreenSpace( fColors, 800, 600) & colorSpace);
	mitem = fResolutionMenu->ItemAt(1);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (fResolution == 1 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}
	enableMenuItem = (PartsToScreenSpace( fColors, 1024, 768) & colorSpace);
	mitem = fResolutionMenu->ItemAt(2);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (fResolution == 2 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}
	enableMenuItem = (PartsToScreenSpace( fColors, 1152, 900) & colorSpace);
	mitem = fResolutionMenu->ItemAt(3);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (fResolution == 3 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}
	enableMenuItem = (PartsToScreenSpace( fColors, 1280, 1024) & colorSpace);
	mitem = fResolutionMenu->ItemAt(4);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (fResolution == 4 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}
	enableMenuItem = (PartsToScreenSpace( fColors, 1600, 1200) & colorSpace);
	mitem = fResolutionMenu->ItemAt(5);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (fResolution == 5 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}
	
	// 	disable the bit depths that are unavailable for this resolution
	//	check the current one
	float w, h;	
	DimensionsFor(fResolution, &w, &h);
	
	enableMenuItem = (PartsToScreenSpace( B_COLOR_8_BIT, w, h) & colorSpace);
	mitem = fColorsMenu->ItemAt(0);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (ColorCount(fColors) == 8 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}

	enableMenuItem = PartsToScreenSpace( B_RGB15, w, h) & colorSpace;
	mitem = fColorsMenu->ItemAt(1);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (ColorCount(fColors) == 15 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}

	enableMenuItem = PartsToScreenSpace( B_RGB16, w, h) & colorSpace;
	mitem = fColorsMenu->ItemAt(2);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (ColorCount(fColors) == 16 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}

	enableMenuItem = PartsToScreenSpace( B_RGB32, w, h) & colorSpace;
	mitem = fColorsMenu->ItemAt(3);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (ColorCount(fColors) == 32 && !mitem->IsMarked())
			mitem->SetMarked(true);
	}

	//	Refresh Rate
	float min, max;
	bool rateSet=false;
	RateLimits(&min, &max);
//	int32 refreshRate = round(fRefreshRate);

	enableMenuItem = (min <= 56.0 && max >= 56.0);
	mitem = fRefreshRateMenu->ItemAt(0);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (CompareFloats(56.0, fRefreshRate) && !mitem->IsMarked())
			mitem->SetMarked(true);
		if (CompareFloats(56.0, fRefreshRate))
			rateSet = true;		
	}
	
	enableMenuItem = (min <= 60.0 && max >= 60.0);
	mitem = fRefreshRateMenu->ItemAt(1);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (CompareFloats(60.0, fRefreshRate) && !mitem->IsMarked())
			mitem->SetMarked(true);
		if (CompareFloats(60.0, fRefreshRate))
			rateSet = true;		
	}
	
	enableMenuItem = (min <= 70.0 && max >= 70.0);
	mitem = fRefreshRateMenu->ItemAt(2);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (CompareFloats(70.0, fRefreshRate) && !mitem->IsMarked())
			mitem->SetMarked(true);
		if (CompareFloats(70.0, fRefreshRate))
			rateSet = true;		
	}
	
	enableMenuItem = (min <= 72.0 && max >= 72.0);
	mitem = fRefreshRateMenu->ItemAt(3);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (CompareFloats(72.0, fRefreshRate) && !mitem->IsMarked())
			mitem->SetMarked(true);
		if (CompareFloats(72.0, fRefreshRate))
			rateSet = true;		
	}
	
	enableMenuItem = (min <= 75.0 && max >= 75.0);
	mitem = fRefreshRateMenu->ItemAt(4);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (CompareFloats(75.0, fRefreshRate) && !mitem->IsMarked())
			mitem->SetMarked(true);
		if (CompareFloats(75.0, fRefreshRate))
			rateSet = true;		
	}

#if _CONSTRAIN_REFRESH_RATE_ == 0
	enableMenuItem = (min <= 90.0 && max >= 90.0);
	mitem = fRefreshRateMenu->ItemAt(5);
	if (mitem) {
		if (mitem->IsEnabled() != enableMenuItem)
			mitem->SetEnabled(enableMenuItem);
		if (CompareFloats(90.0, fRefreshRate) && !mitem->IsMarked())
			mitem->SetMarked(true);
		if (CompareFloats(90.0, fRefreshRate))
			rateSet = true;		
	}
#endif
	
	//	if the current refresh rate is not one of the preset ones
	//	then modify the last item 'other' so that it displays
	//	the current rate
	if (!rateSet) {
#if _CONSTRAIN_REFRESH_RATE_
		mitem = fRefreshRateMenu->ItemAt(5);
#else
		mitem = fRefreshRateMenu->ItemAt(6);
#endif
		if (mitem) {			
			char str[32];
			
			//	this is pretty convoluted
			//	first set the label then set the menu item
			sprintf(str, "%.1f Hz", fRefreshRate);
			mitem->SetLabel(str);
			mitem->SetMarked(true);

			//	the current rate and the requested may not match
			//	but the req rate should have been set via
			//	the msg from the rr config dialog
			sprintf(str, "%.1f Hz/Other...", fRefreshRate);
			mitem->SetLabel(str);
		}
	} else {
		mitem = fRefreshRateMenu->ItemAt(5);
		if (mitem)
			mitem->SetLabel("Other...");
	}
	
	// 	Mini Screen thing
	fMiniScreen->SetResolution(fResolution);
	fMiniScreen->SetColor(fDesktopColor);
	
	mitem = fDecorMenu->FindMarked();
	const char* decor = NULL;
	if (mitem && mitem->Message())
		mitem->Message()->FindString("decor", &decor);
	if (!decor || fDecorName != decor) {
		if (mitem)
			mitem->SetMarked(false);
		for (int32 i=0; i<fDecorMenu->CountItems(); i++) {
			if ((mitem = fDecorMenu->ItemAt(i)) != NULL && mitem->Message() != NULL) {
				if (mitem->Message()->FindString("decor", &decor) == B_OK) {
					if (fDecorName == decor) {
						mitem->SetMarked(true);
						break;
					}
				}
			}
		}
	}
}

void
TBox::UpdateDesktopColor()
{
	//	wont have the msg sent back to tbox, just updates the colorcontrol
	fDesktopColorCntl->SyncColor(fDesktopColor);
}

//	only gets hit on app launch
//		and on workspace change
void
TBox::GetMonitorSettings()
{
	fTargetWorkspace = current_workspace();
	fLastWorkspaceCount = fInitialWorkspaceCount = count_workspaces();
	
	fInitialResolution = fResolution = fLastResolution = Resolution();
	fInitialColors = fColors = fLastColors = BitsPerPixel();
	fInitialRefreshRate = fRefreshRate = fLastRefreshRate = RefreshRate();
	fInitialDesktopColor = fDesktopColor = fLastDesktopColor = DesktopColor();
	
	get_window_decor(&fInitialDecorName);
	fDecorName = fLastDecorName = fInitialDecorName;
}

void
TBox::DesktopColorChange()
{
	// 	inform other parts of the app that the desktop color has changed
	//	the desktop color control sets the desktop color	
	fDesktopColor = fDesktopColorCntl->ValueAsColor();
	fMiniScreen->SetColor(fDesktopColor);
	
	SetRevert(true);
}

//	sent from dimension menufield selection
void
TBox::SetMonitorColors(color_space colors)
{
	if (colors == fColors)
		return;

	fColors = colors;
	UpdateControls(		(fColors != BitsPerPixel())
					|| 	(fRefreshRate != RefreshRate())
					|| 	(fResolution != Resolution()) );
}

void
TBox::SetMonitorResolution(resolution r)
{
	if (r == fResolution)
		return;
		
	fResolution = r;
	UpdateControls(		(fColors != BitsPerPixel())
					|| 	(fRefreshRate != RefreshRate())
					|| 	(fResolution != Resolution()) );	// enable apply button
}

void
TBox::SetMonitorRefreshRate(float f)
{
	if (f == fRefreshRate)
		return;

	fRefreshRate = f;
	UpdateControls(		(fColors != BitsPerPixel())
					|| 	(fRefreshRate != RefreshRate())
					|| 	(fResolution != Resolution()) );
}

void
TBox::SetMonitorWithRefreshRate(float rate, bool confirmChanges)
{
	bool all = fAllWorkspaces;
	
	fAllWorkspaces = false;
	fRefreshRate = rate;
	SetMonitorForWorkspace(confirmChanges);
	fAllWorkspaces = all;
}

void
TBox::SetRevertSettings()
{
	//	get the current settings so that we can revert them
	fLastResolution = Resolution();
	fLastColors = BitsPerPixel();
	fLastRefreshRate = RefreshRate();
	fLastWorkspaceCount = count_workspaces();
	get_window_decor(&fLastDecorName);
}

void
TBox::RevertSettings()
{
	fResolution = fLastResolution;
	fColors = fLastColors;
	fRefreshRate = fLastRefreshRate;	
	
	fDecorName = fLastDecorName;
	set_window_decor(fDecorName.String());
	
	SetRevert(false);	
}

void
TBox::SetMonitorForWorkspace(bool confirmChanges)
{
	if (!fAllWorkspaces)
		UpdateControls(false);

	int32 ws = (fAllWorkspaces) ? -1 : fTargetWorkspace;
	
	SetRevertSettings();
			
	if (_ConfigureWorkspace(Window(), confirmChanges, true,
			fResolution, fColors, fRefreshRate, ws)) {

		UpdateControls(false);
		SetRevert(true);	
		
	} else {	//	auto revert the settings
		RevertSettings();
		UpdateControls(false);
	}	

}

void
TBox::SetMonitorForWorkspace(monitor_settings s, bool confirmChanges)
{
	SetRevertSettings();
	
	SetMonitorResolution(s.dimensions);
	SetMonitorColors(s.colorSpace);
	SetMonitorRefreshRate(s.refreshRate);

	if (s.workspace != -1)
		UpdateControls(false);

	if (_ConfigureWorkspace(Window(), confirmChanges, true,
			s.dimensions, s.colorSpace, s.refreshRate, s.workspace)) {

		if (s.workspace != -1)
			UpdateControls(false);
		SetRevert(true);	
		
	} else {
		RevertSettings();	
		if (s.workspace != -1)
			UpdateControls(false);
	}
	
}

//	Default methods

void
TBox::SetDefaultResolution()
{
	fResolution = k640x480Resolution;
	fColors = B_COLOR_8_BIT;
	UpdateControls(false);
	// requesting default resolution does not physically change it,
	//	just changes the settings, user needs to press Apply
//	_Configure(Window(), false, false, fResolution, fColors, fRefreshRate);
}

void
TBox::SetDefaultRefreshRate()
{
	fRefreshRate = 60.0;
	UpdateControls(false);
	// requesting default refresh rate does not physically change it,
	//	just changes the settings, user needs to press Apply
//	_Configure(Window(), false, false, fResolution, fColors, fRefreshRate);
}

void
TBox::SetDefaultCRTPosition()
{
	SetCRTPosition(50, 50, 50, 50);
}

void
TBox::ShowRefreshRateConfig()
{
	//	set the menu item correctly for refresh rate
	UpdateControls(false);

//	if (fAllWorkspaces) {
//		BAlert* a = new BAlert("","A custom refresh rate can only be "
//			"applied to the current workspace.",
//			"Cancel", "Continue", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT );
			
//		if (a->Go() == 0)
//			return;
//	}
		
	//	can't do this for all workspaces
//	bool oldAllWS = fAllWorkspaces;
//	fAllWorkspaces = false;

//	if (fColors != BitsPerPixel() || fResolution != Resolution()) {
//		BAlert* a = new BAlert("","The resolution or colors setting has "
//			"changed.  The new settings will need to be applied before "
//			"selecting a custom refresh rate.",
//			"Apply", "Cancel", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT );
//			
//		if (a->Go() != 0) {
//			//	rate has not changed, update controls to show
//			//	actual rate instead of Other menu item
//			UpdateControls(false);
//			return;
//		} else {
//			SetRevertSettings();
//			if (!_Configure(Window(), true, true, fResolution, fColors,
//				fRefreshRate)) {
//				//	if the user did not confirm the changes
//				//	then revert the controls as well
//				RevertSettings();
//				UpdateControls(false);
//			}
//		}
//	}
//	fAllWorkspaces = oldAllWS;
	
	BPoint pt(Window()->Frame().LeftTop());
	
	pt.x += Window()->Frame().Width() - kRefreshRateWindWidth/2;
	pt.y += Window()->Frame().Height()/2 - kRefreshRateWindHeight/2;

	//	grab the current settings so that
	//	revert works correctly	
	SetRevertSettings();
	TConfigRefreshRateWind* w = new TConfigRefreshRateWind( pt, Window(),
		false, fRefreshRate);
	w->Show();
}

#if SHOW_CONFIG_WS
//	Workspace info methods

void
TBox::ShowWorkspaceConfig()
{
	BPoint pt(fWorkspaceInfoBtn->Frame().LeftTop());
	pt.y += fWorkspaceInfoBtn->Frame().Height()/2 - kCWHeight;
	pt.x += fWorkspaceInfoBtn->Frame().Height()/2;
	TConfigWorkspaceWind* w = new TConfigWorkspaceWind(
									Window()->ConvertToScreen(pt),
									Window());

	w->Show();
}
#endif

// 	Defaults and Revert controls

bool
TBox::CanRevert()
{
	return fCanRevert;
}

void
TBox::SetRevert(bool r)
{
	fCanRevert = r;
	if (fRevertBtn->IsEnabled() != r)
		fRevertBtn->SetEnabled(fCanRevert);
}

//	reverts all settings to the screen settings and desktop color
//	that were set on launch
void
TBox::Revert()
{
	if (!CanRevert())
		return;
	
	//	reset all the settings to the initial settings - from launch of app	
	fResolution = fLastResolution = fInitialResolution;
	fColors = fLastColors = fInitialColors;
	fRefreshRate = fLastRefreshRate = fInitialRefreshRate;
	fDecorName = fLastDecorName = fInitialDecorName;
	set_window_decor(fDecorName.String());
	
#if 0
	fDesktopColor = fLastDesktopColor = fInitialDesktopColor;
	rgb_color c = DesktopColor();
	if (c.red != fDesktopColor.red || c.green != fDesktopColor.green
		|| c.blue != fDesktopColor.blue || c.alpha != fDesktopColor.alpha)
		SetDesktopColor(fDesktopColor);
#endif
	
	// update the controls with the new settings so that we don't get a flash
	//	when they change
	UpdateControls(false);

	// invoke the new settings, confirm is off and protect is on
	_Configure( Window(), false, false,
				fResolution, fColors, fRefreshRate, true);
	SetRevert(false);	
	UpdateControls(false);
	UpdateDesktopColor();
}

//	sets the screen settings to the factory defaults
void
TBox::Defaults()
{
#if 0
	// for this workspace only
	rgb_color c, dc = DesktopColor();
	c.red = 51; c.green = 102; c.blue = 152; c.alpha = 255;

	if (c.red != dc.red || c.green != dc.green
		|| c.blue != dc.blue || c.alpha != dc.alpha) {
		SetDesktopColor(c);
		fDesktopColor = c;
	}
#endif	
	SetRevertSettings();
	SetDefaultRefreshRate();
	SetDefaultResolution();	
	fDecorName = "DEFAULT";
	set_window_decor("DEFAULT");
	
	UpdateControls(true);
	UpdateDesktopColor();

	SetRevert(true);
}

//	save the current screen settings to the Screen_date file
void
TBox::SaveSettings()
{
	fSavedSettings.workspace = current_workspace();
	fSavedSettings.dimensions = Resolution();
	fSavedSettings.colorSpace = BitsPerPixel();
	fSavedSettings.refreshRate = RefreshRate();
	fSavedSettings.desktopColor = DesktopColor();
	
	char path[B_PATH_NAME_LENGTH];

	int fileRef = SettingsFileRef(kSettingsFile, path, true);
	if (fileRef >= 0) {
		lseek(fileRef, sizeof(BPoint), 0);
		lseek(fileRef, sizeof(bool), 1);
		write(fileRef, &fSavedSettings, sizeof(monitor_settings));
		close(fileRef);
	}
}

void
TBox::UseSettings(monitor_settings settings, bool all)
{
	//	set the screen settings to the saved settings
	//	for this workspace
	settings.workspace = current_workspace();
	SetMonitorForWorkspace(settings, true);
	
	if (all)
		SetDesktopColor(settings.desktopColor);
}

void
TBox::GetSettings(monitor_settings *settings)
{
	char path[B_PATH_NAME_LENGTH];
	
	//	get the current workspaces settings, kind of a default
	settings->workspace = current_workspace();
	settings->dimensions = Resolution();
	settings->colorSpace = BitsPerPixel();
	settings->refreshRate = RefreshRate();
	settings->desktopColor = DesktopColor();
	
	//	now read the real saved settings from the file
	int fileRef = SettingsFileRef(kSettingsFile, path, false);
	if (fileRef >= 0) {
		struct stat info;
		if (stat(path, &info) != -1) {		
			if (info.st_size == (sizeof(BPoint) + sizeof(monitor_settings))) {
				lseek(fileRef, sizeof(BPoint), 0);
				lseek(fileRef, sizeof(bool), 1);
				read(fileRef, settings, sizeof(monitor_settings));		
				close(fileRef);
			}
		}
	}
}

#if SHOW_PICTURE_BTN
void
TBox::ShowHelp()
{
	bool found=false;
	BWindow* tw=NULL;
	int32 wc = be_app->CountWindows();
	for (int32 i=0 ; i<wc ; i++){
		tw = be_app->WindowAt(i);
		if (tw) {
			if (strcmp(tw->Name(), "Screen Help") == 0) {
				found = true;
				break;
			}
		}
	}
	if (found) {
		tw->Activate();
		return;
	}
	
	BRect r(0,0,375,240);
	BWindow* w = new BWindow(r, "Screen Help", B_DOCUMENT_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE);
	
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	BRect r2(r);
	r2.InsetBy(4,4);
	BTextView* text = new BTextView(r, "text", r2, B_FOLLOW_ALL, B_WILL_DRAW);
	text->SetViewColor(216, 216, 216, 255);
	text->MakeSelectable(false);
	text->MakeEditable(false);
	
	BScrollView* scroller = new BScrollView("", text, B_FOLLOW_ALL, 0, true, true);
	w->AddChild(scroller);
	
	text->Insert("Cmd-D  sets the workspace to the default resolution\n");
	text->Insert("Cmd-R  sets the workspace to the default refresh rate\n");
	text->Insert("Cmd-P  sets the workspace to the default position\n");
	text->Insert("\n\n");
	text->Insert("Cmd-X  shows the workspace configuration dialog\n");
	text->Insert("Cmd-C  shows the custom refresh rate dialog\n");
	text->Insert("\n\n");
	text->Insert("Cmd-Shift-D  sets the workspace to the system default settings\n");
	text->Insert("Cmd-Shift-R  reverts the workspace to the initial settings\n");
	text->Insert("\n\n");
	text->Insert("Cmd-S  saves the current settings\n");
	text->Insert("Cmd-U  sets the workspace to the previously saved settings\n");
	text->Insert("Cmd-Shift-U  sets all workspaces to the previously saved settings\n");
	text->Insert("\n\n");
	text->Insert("Ctrl-Alt-Shift-F12  emergency revert to system defaults\n");
	text->Insert("                    does not require Screen preferences to be open\n");
	
	CenterWindowOnScreen(w);	
	w->Show();
}
#endif
// ************************************************************************** //

TDesktopColorControl::TDesktopColorControl(BPoint pt, BMessage* msg,
	TBox* sibling)
	: BColorControl(pt, B_CELLS_32x8, 8, "desktop color", msg)
{
	fDontChange = false;
	fDontPassToSibling = false;
	fSibling = sibling;
}

TDesktopColorControl::~TDesktopColorControl()
{
}

void
TDesktopColorControl::AttachedToWindow()
{
	BColorControl::AttachedToWindow();
	
	fDontPassToSibling = true;
	fDontChange = true;
	
	rgb_color dc = DesktopColor();
	int32 c = (dc.red << 24) + (dc.green << 16) + (dc.blue << 8);
	SetValue(c);
	
	fDontPassToSibling = false;
	fDontChange = false;
}

void 
TDesktopColorControl::SetValue(int32 v)
{
	BColorControl::SetValue(v);
	
	if (fDontChange)
		return;
		
	SetDesktopColor(ValueAsColor());
	
	if (fDontPassToSibling)
		return;
		
	if (fSibling)
		fSibling->DesktopColorChange();
}

void
TDesktopColorControl::SetColor(rgb_color dc)
{
	fDontPassToSibling = true;
	int32 c = (dc.red << 24) + (dc.green << 16) + (dc.blue << 8);
	SetValue(c);
	fDontPassToSibling = false;
}

void
TDesktopColorControl::SyncColor(rgb_color dc)
{
	fDontChange = true;
	int32 c = (dc.red << 24) + (dc.green << 16) + (dc.blue << 8);
	SetValue(c);
	fDontChange = false;
}

//****************************************************************************************

TScreenThing::TScreenThing(BRect frame, resolution r, rgb_color c)
	: BBox(frame, "monitor thing", B_FOLLOW_NONE, B_WILL_DRAW, B_FANCY_BORDER)
{
	SetFont(be_plain_font);
	fResolution = r;
	fColor = c;
}

TScreenThing::~TScreenThing()
{
}

void
TScreenThing::AttachedToWindow()
{
	BBox::AttachedToWindow();
#if SHOW_CONFIG_WS == 0
	BPopUpMenu* fWSMenu = new BPopUpMenu("workspace  count");

	char 		str[32];
	BMessage 	*msg=NULL;
	int32		fWorkspaceCount = count_workspaces();
	int32		max_ws = (fWorkspaceCount >= kMaxWorkspaceCount)
					? fWorkspaceCount : kMaxWorkspaceCount;
	BMenuItem*	mi;
	for (int32 i=1 ; i<=max_ws ; i++) {
		msg = new BMessage(msg_config_index);
		msg->AddInt32("count", i);
		sprintf(str, "%ld", i);
		mi = new BMenuItem(str, msg);
		mi->SetTarget(this);
		fWSMenu->AddItem(mi);		
	}
	
	//	check the currect workspace count
	mi = fWSMenu->ItemAt(fWorkspaceCount-1);
	if (mi)
		mi->SetMarked(true);

	BRect rect;
	rect.bottom = Bounds().bottom - 10;
	rect.top = rect.bottom - 20;
	rect.left = 8;
	rect.right = Bounds().Width() - 7;
	BMenuField* fWSBtn = new BMenuField(rect, "", "Workspace count:", fWSMenu, true);
	
	AddChild(fWSBtn);
	fWSBtn->SetAlignment(B_ALIGN_RIGHT);
	fWSBtn->SetDivider(StringWidth("Workspace count:")+5);
#endif
}

void
TScreenThing::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
#if SHOW_CONFIG_WS == 0
		case msg_config_index:
			{
				int32 wsc;
				msg->FindInt32("count", &wsc);
				if (wsc != count_workspaces())
					set_workspace_count(wsc);
			}
			break;
#endif		
		default:
			BBox::MessageReceived(msg);
			break;
	}
}

void
TScreenThing::Draw(BRect r)
{	
	BBox::Draw(r);

	DrawMonitor();
}

const char* const kBGPrefsSig = "application/x-vnd.Be-BACK";
void
TScreenThing::MouseDown(BPoint)
{
	BMessage argv(B_ARGV_RECEIVED);
	argv.AddInt32("argc", 0);

	if (be_roster->IsRunning(kBGPrefsSig)) {
		team_id tid = be_roster->TeamFor(kBGPrefsSig);
		be_roster->ActivateApp(tid);
	} else
		be_roster->Launch(kBGPrefsSig, &argv);
}

//enum Mode {
//	kAtOffset,
//	kCentered,			// only works on Desktop (maybe I'll make it work for normal windows too)
//	kScaledToFit,		// only works on Desktop
//	kTiled
//};
//#include <Path.h>
//#include <Node.h>
//#include <FindDirectory.h>
//#include <fs_attr.h>
//
//// attribute name
//const char *kBackgroundImageInfo 			= "be:bgndimginfo";
//
//// BMessage entries
//const char *kBackgroundImageInfoPath 		= "be:bgndimginfopath";			// string path
//const char *kBackgroundImageInfoMode 		= "be:bgndimginfomode"; 		// int32
//const char *kBackgroundImageInfoOffset 		= "be:bgndimginfooffset";		// BPoint
//const char *kBackgroundImageInfoEraseText	= "be:bgndimginfoerasetext";	// bool
//const char *kBackgroundImageInfoWorkspaces 	= "be:bgndimginfoworkspaces";	// uint32
//
//static bool
//HasDesktopPicture(int32 workspace, char* imagepath, int32* mode, BPoint* offset, bool* erase)
//{
//	BPath path;
//	status_t ret = find_directory(B_DESKTOP_DIRECTORY, &path);
//	
//	if(ret == B_OK) {
//		BNode n(path.Path());
//	
//		if((ret = n.InitCheck()) == B_OK) {
//			
//			attr_info info;
//			if((ret = n.GetAttrInfo(kBackgroundImageInfo, &info)) == B_OK) {
//				if (info.size) {
//					void* buf;
//					ssize_t s;
//					BMessage msg;
//					uint32 mask;
//				
//					if((buf = malloc(info.size)) != 0) {
//						if((s = n.ReadAttr(kBackgroundImageInfo, B_MESSAGE_TYPE, 0, buf, info.size))
//							== info.size)
//							ret = msg.Unflatten((char *)buf);
//						free(buf);
//
//
//						if (msg.FindInt32(kBackgroundImageInfoWorkspaces, workspace, (int32 *)&mask)
//							== B_OK)						{
//							msg.FindString(kBackgroundImageInfoPath, workspace, &imagepath);
//							msg.FindInt32(kBackgroundImageInfoMode, workspace, mode);
//							msg.FindPoint(kBackgroundImageInfoOffset, workspace, offset);
//							msg.FindBool(kBackgroundImageInfoEraseText, workspace, erase);
//							
//							return true;
//						}
//					}
//				}
//			}
//		}
//	}
//	
//	return false;
//}

void
TScreenThing::DrawMonitor()
{
	float s_width, s_height;
	DimensionsFor(fResolution, &s_width, &s_height);
	
	float	w = Bounds().Width();
	float 	h = Bounds().Height();
	float	actualWidth, actualHeight;
	
	BRect	r(Bounds());
	rgb_color c;

	r.InsetBy(4,4);		
	SetHighColor(ViewColor());
	FillRect(r);
	
	actualWidth = s_width/16;
	actualHeight = s_height / 16;
	r.left = w / 2 - actualWidth / 2;
	r.right = r.left + actualWidth;
	r.top = (h / 2 - actualHeight / 2);
#if SHOW_CONFIG_WS == 0
	r.top -= 14;
#endif
	r.bottom = r.top + actualHeight;

	// gray monitor border
	SetHighColor(160, 160, 160);
	FillRect(r);

	// desktop color
	r.InsetBy(4, 4);
//	{
//		const char* imagePath;
//		int32 mode;
//		BPoint offset;
//		bool erase;
//
//		int32 thisWS = current_workspace();
//		if (HasDesktopPicture(thisWS, imagePath, &mode, &offset, &erase)) {
//			if (fCurrentWorkspace != thisWS)
//				LoadDesktopPicture();
//			DrawDesktopPicture();
//		} else {
			SetHighColor(fColor);
			FillRect(r);
//		}
//	}

	BeginLineArray(13);
	//	red power light
	AddLine(BPoint(r.left - 1, r.bottom + 3), BPoint(r.left, r.bottom + 3),
		SetRGBColor(228,0,0));

	// black borders
	c = SetRGBColor(0, 0, 0);
	r.InsetBy(-5, -5);
	AddLine(BPoint(r.left + 2, r.top), BPoint(r.right - 2, r.top), c);
	AddLine(BPoint(r.right, r.top + 2), BPoint(r.right, r.bottom - 2), c);
	AddLine(BPoint(r.right - 2, r.bottom), BPoint(r.left + 2, r.bottom), c);
	AddLine(BPoint(r.left, r.bottom - 2), BPoint(r.left, r.top + 2), c);
	
	AddLine(BPoint(r.left + 1, r.top + 1), BPoint(r.left + 1, r.top + 1), c);
	AddLine(BPoint(r.right - 1, r.top + 1), BPoint(r.right - 1, r.top + 1), c);
	AddLine(BPoint(r.right - 1, r.bottom - 1),
			BPoint(r.right - 1, r.bottom - 1), c);
	AddLine(BPoint(r.left + 1, r.bottom - 1),
			BPoint(r.left + 1, r.bottom - 1), c);

	AddLine(BPoint(r.left + 5, r.top + 4), BPoint(r.right - 5, r.top + 4), c);
	AddLine(BPoint(r.right - 4, r.top + 5),
			BPoint(r.right - 4, r.bottom - 5), c);
	AddLine(BPoint(r.right - 5, r.bottom - 4), BPoint(r.left + 5, r.bottom - 4), c);
	AddLine(BPoint(r.left + 4, r.bottom - 5), BPoint(r.left + 4, r.top + 5), c);

	EndLineArray();

	Sync();
}

void
TScreenThing::DrawMiniWindow(BRect frame, BPoint loc, float size)
{
	//	draw a mini window 20 x 20
	float w, h;
	DimensionsFor(fResolution, &w, & h);

	w = ((1600 * (640 / w)) / 16) * size;
	h = ((1200 * (480 / h)) / 16) * size;
	
	BRect wrect(frame.left + loc.x, frame.top + loc.y,
				frame.left + loc.x + w, frame.top + loc.y + h);
	
	int32 cnt = 5 - fResolution;
	rgb_color c = SetRGBColor(255, 203, 0);
	
	BeginLineArray(cnt + 1);
	
	//	add yellow tab
	AddLine(BPoint(wrect.left, wrect.top),
			BPoint(wrect.left + (w/2), wrect.top), c);

	float y = wrect.top+1;
	for (int32 i=0 ; i<cnt ; i++) {
		AddLine(BPoint(wrect.left, y), BPoint(wrect.left + (w/2), y), c);
		y++;
	}
	
	EndLineArray();	
	
	//	add a border
	wrect.top += (5 - fResolution) + 1;
	SetHighColor(100, 100, 100, 255);
	StrokeRect(wrect);
	
	// 	fill the faux window
	wrect.InsetBy(1,1);
	SetHighColor(255, 255, 255, 255);
	FillRect(wrect);
}

void
TScreenThing::SetResolution(resolution r)
{
	if (fResolution == r)
		return;
	
	fResolution = r;
	Draw(Bounds());
}

void
TScreenThing::SetColor(rgb_color c)
{
	if (fColor.red == c.red && fColor.green == c.green
		&& fColor.blue == c.blue && fColor.alpha == c.alpha)
		return;
		
	fColor = c;
	Draw(Bounds());
}

// ************************************************************************** //

const int32 kConfirmWindowWidth	= 300;
const int32 kConfirmWindowHeight = 130;
const char* const kConfirmText = "Do you wish to keep these settings?";
const int32 kSettingsConfirmed = 'cnfm';
const int32 kConfirmTimeout = 7;

TPulseBox::TPulseBox(BRect frame)
	: BView(frame, "", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
{
	fMsgType = B_WARNING_ALERT;
	GetIcon();
	fTimeRemaining = kConfirmTimeout;
}

TPulseBox::~TPulseBox()
{
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

void
TPulseBox::Draw(BRect u)
{
	BView::Draw(u);

	PushState();
		
	float h = FontHeight(this, true);

	BRect r(Bounds());
		
	//	draw gray box
	r.right = r.left + 30;
	SetHighColor(184, 184, 184, 255);
	SetLowColor(ViewColor());
	FillRect(r);
	
	r.left = r.right;
	r.right = Bounds().Width();
	SetHighColor(ViewColor());
	SetLowColor(ViewColor());
	FillRect(r);

	//	draw icon
	r.Set(18, 6, 49, 37);
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fIconBits, BRect(0,0,31,31), r);
	SetDrawingMode(B_OP_COPY);
	
	//	draw the msg
	SetHighColor(0, 0, 0, 255);
	SetFont(be_bold_font);
	
	MovePenTo(60, 20);
	DrawString(kConfirmText);
	
	char status[32];
	sprintf(status, "Settings will revert in %ld seconds.", fTimeRemaining);
	MovePenTo(60, 20 + h + 5);
	DrawString(status);
	
	PopState();
}

void
TPulseBox::Pulse()
{
	fTimeRemaining--;
	Draw(Bounds());
	
	if (fTimeRemaining <= 0)
		Window()->PostMessage('exit');
}

void
TPulseBox::GetIcon()
{
	if (fMsgType != B_EMPTY_ALERT) {
		BPath path;
		if (find_directory (B_BEOS_SERVERS_DIRECTORY, &path) == B_OK) {
			path.Append ("app_server");
			BFile		file(path.Path(), O_RDONLY);
			BResources	rfile;

			if (rfile.SetTo(&file) == B_NO_ERROR) {
				size_t	size;
				char	*name = "";
				switch(fMsgType) {
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
			// couldn't find icon so make this an B_EMPTY_ALERT
			fMsgType = B_EMPTY_ALERT;
		}
	} else
		fIconBits = NULL;
}

TConfirmWindow::TConfirmWindow()
	:BWindow( BRect(0, 0, kConfirmWindowWidth, kConfirmWindowHeight),
		"Confirm Changes", B_MODAL_WINDOW_LOOK, B_MODAL_ALL_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_ASYNCHRONOUS_CONTROLS)
{	
	fOK = false;

	BRect r(Bounds());

	fBG = new TPulseBox(r);
	fBG->SetFont(be_plain_font);
	AddChild(fBG);
	
	float h = FontHeight(fBG, true);		
	r.right = Bounds().Width() - 10;
	r.left = r.right - 75;
	r.top = 47 + h;
	r.bottom = r.top + 20;	
	fAcceptBtn = new BButton(r, "okay", "Keep", new BMessage('okay'));
	fBG->AddChild(fAcceptBtn);
	
	r.right = r.left - 10;
	r.left = r.right - 75;
	fCancelBtn = new BButton(r, "Cancel", "Revert", new BMessage('exit'));
	fBG->AddChild(fCancelBtn);
	
	ResizeTo(kConfirmWindowWidth, fAcceptBtn->Frame().bottom + 10);
	
	CenterWindowOnScreen(this);
	
	SetPulseRate(1000000);
	
	AddShortcut('R', B_COMMAND_KEY, new BMessage('exit'));
	AddShortcut('K', B_COMMAND_KEY, new BMessage('okay'));

	Show();
}

void
TConfirmWindow::MessageReceived(BMessage* m)
{
	switch (m->what) {
		case kSettingsConfirmed:
			fOK = true;
			break;
		case 'exit':
			fAlertVal = 1;
			delete_sem(fAlertSem);
			fAlertSem = -1;
			break;
		case 'okay':
			fAlertVal = 0;
			delete_sem(fAlertSem);
			fAlertSem = -1;
			break;
		default:
			BWindow::MessageReceived(m);
			break;
	}
}

int32
TConfirmWindow::Go()
{
	long		value;
	thread_id	this_tid = find_thread(NULL);
	BLooper		*loop;
	BWindow		*wind = NULL;

	fAlertSem = create_sem(0, "AlertSem");
	loop = BLooper::LooperForThread(this_tid);
	if (loop)
		wind = cast_as(loop, BWindow);

	Show();

	long err;

	if (wind) {
		// A window is being blocked. We'll keep the window updated
		// by calling UpdateIfNeeded.
		while (1) {
			while ((err = acquire_sem_etc(fAlertSem, 1, B_TIMEOUT, 50000)) == B_INTERRUPTED)
				;
			if (err == B_BAD_SEM_ID)
				break;
			wind->UpdateIfNeeded();
		}
	} else {
		do {
			err = acquire_sem(fAlertSem);
		} while (err == B_INTERRUPTED);
	}

	// synchronous call to close the alert window. Remember that this will
	// 'delete' the object that we're in. That's why the return value is
	// saved on the stack.
	value = fAlertVal;

	if (Lock()) {
		Quit();
	}

	return value;
}

// ************************************************************************** //

TRefreshSlider::TRefreshSlider(BRect frame, BMessage* message,
	int32 min, int32 max, float bottom, float top,
	bool applyNow, float rate)
	: BSlider(frame, "slider", "Refresh Rate", message,
		min, max, B_BLOCK_THUMB),
			fApplyNow(applyNow), fRefreshRate(rate)
{
	char str1[16], str2[16];
	
	fMin = min;	fMax = max;
	fTop = top; fBottom = bottom;
	
	sprintf(str1, "%.1f", bottom);
	sprintf(str2, "%.1f", top);
	SetLimitLabels(str1, str2);
	
	fStr = (char*)malloc(32);

	fKeyBuffer[0] = fKeyBuffer[1] = fKeyBuffer[2] = 0;	
}

TRefreshSlider::~TRefreshSlider()
{
	free(fStr);
}

void
TRefreshSlider::AttachedToWindow()
{
	BSlider::AttachedToWindow();
	float t = (fRefreshRate - fBottom) * 10;
	SetValue(myround(t));
}

void 
TRefreshSlider::KeyDown(const char *bytes, int32 numBytes)
{
	//	do a simple type ahead
	if (isdigit(bytes[0])){	
		if (fLastTime > system_time()) {
			fKeyBuffer[0] = fKeyBuffer[1];			
			fKeyBuffer[1] = bytes[0];
		} else {	// start over
			fKeyBuffer[0] = '0';
			fKeyBuffer[1] = bytes[0];
		}
		fKeyBuffer[2] = 0;

		int32 v = (atoi(fKeyBuffer) - (int32)fBottom) * 10;
		if (v >= fMin && v <= fMax) {
			SetValue(v);
			Invoke(Message());
		}
		fLastTime = system_time() + 500000;
		return;
	}

	BSlider::KeyDown(bytes, numBytes);
}

status_t
TRefreshSlider::Invoke(BMessage *msg)
{
	status_t err = BSlider::Invoke(msg);
	if (fApplyNow) {
		
		float rate = Value() + (fBottom *10);
		rate /= 10;
	
		_Configure(Window(), false, false, Resolution(), BitsPerPixel(), rate, true);
	}		
	return err;
}

void
TRefreshSlider::SetValue(int32 v)
{
	if (v == Value() || v < fMin || v > fMax)
		return;
		
	BSlider::SetValue(v);	
}

char*
TRefreshSlider::UpdateText() const
{
	sprintf(fStr, "%.1f Hz", Rate());

	return fStr; 
}

float
TRefreshSlider::Rate() const
{
	float v = Value() + (fBottom *10);
	v /= 10;
	
	return v;
}

void
TRefreshSlider::SetApplyNow(bool s)
{
	fApplyNow = s;
}
		
// ************************************************************************** //

const char* const kInfo = "Type or use the left and right arrow keys.";

TConfigRefreshRateWind::TConfigRefreshRateWind(BPoint loc, BWindow* owner,
	bool applyNow, float rate)
	: BWindow( BRect(loc.x, loc.y,
					 loc.x+kRefreshRateWindWidth, loc.y+kRefreshRateWindHeight),
		"Refresh Rate",
		B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE),
			fApplyNow(applyNow)
{
	if (!BScreen(B_MAIN_SCREEN_ID).Frame().Contains(Frame()))
		CenterWindowOnScreen(this);

	fOriginalRefreshRate = rate;
	fOwner = owner;

	BRect r(Bounds());
	r.InsetBy(-1, -1);
	fBG = new BBox( r, "bg", B_FOLLOW_ALL);
	fBG->SetFont(be_plain_font);
	AddChild(fBG);
	
	r.left += 10; r.right = Bounds().Width() - 30;
	r.top = 10; r.bottom = 26;
	BStringView* text = new BStringView(r, "", kInfo);
	fBG->AddChild(text);
	
	RateLimits(&fMin, &fMax);
	fMax = (fMax <= 90.0) ? fMax : 90;
	
	int32 top = (int32)(fMax - fMin) * 10;
	int32 bottom = 0;
	
	r.left = 10;
	r.right = Bounds().Width() - 6;
	r.top = 35; r.bottom = r.top + 20;
	fRefreshRateSlider = new TRefreshSlider( r,
		new BMessage(msg_refresh_update),
		bottom, top, fMin, fMax, fApplyNow, rate);
	fRefreshRateSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fRefreshRateSlider->SetHashMarkCount(10);
	fBG->AddChild(fRefreshRateSlider);
	
	float h = FontHeight(fBG, true);
	
//	r.Set(	10, Bounds().Height() - 21 - h,
//			10 + fBG->StringWidth("Apply Immediately") + 25,
//			Bounds().Height() - 21);
//	fApplyNowBtn = new BCheckBox(r, "applyNow", "Apply Immediately",
//		new BMessage(msg_refresh_apply_now));
//	fApplyNowBtn->SetValue(applyNow);
//	fBG->AddChild(fApplyNowBtn);
	
	//	 some way of closing this window
	r.Set(	Bounds().Width() - 75 - 13,
			Bounds().Height() - 25 - h,
		  	Bounds().Width() - 13,
			Bounds().Height() - 25);
	fDoneBtn = new BButton(r, "done", "Done", new BMessage(msg_refresh_done),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	
	r.right = r.left - 14;
	r.left = r.right - 75;
	fCancelBtn = new BButton(r, "cancel", "Cancel", new BMessage(msg_refresh_cancel),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	fBG->AddChild(fCancelBtn);
	
	fBG->AddChild(fDoneBtn);
		
	SetDefaultButton(fDoneBtn);
	
	fRefreshRateSlider->MakeFocus(true);
}


TConfigRefreshRateWind::~TConfigRefreshRateWind()
{
}

void
TConfigRefreshRateWind::FrameResized(float w, float h)
{
	BWindow::FrameResized(w,h);
}

void
TConfigRefreshRateWind::MessageReceived(BMessage* msg)
{
	BMessage* m=NULL;
	switch (msg->what) {
		case msg_refresh_update:
//			fApplyBtn->SetEnabled(
//				!CompareFloats(fRefreshRateSlider->Rate(), RefreshRate()));
			break;
			
		case msg_refresh_cancel:
			SetRefreshRate(fOriginalRefreshRate);
			if (fOwner)
				fOwner->PostMessage(msg_refresh_cancel);
			PostMessage(B_QUIT_REQUESTED);
			break;
		case msg_refresh_done:
			//	tell the owning window that the custom refresh dialog is going away
			if (fOwner) {
				m = new BMessage(msg_refresh_done);
				m->AddFloat("rate", fRefreshRateSlider->Rate());
				fOwner->PostMessage(m);
			}
			PostMessage(B_QUIT_REQUESTED);
			break;
//		case msg_refresh_apply:
//			SetRefreshRate(fRefreshRateSlider->Rate());
//			if (fOwner) {
//				m = new BMessage(msg_refresh_apply);
//				m->AddFloat("rate", fRefreshRateSlider->Rate());
//				fOwner->PostMessage(m);
//			}
//			break;
//				
		case msg_refresh_apply_now:
			fApplyNow = !fApplyNow;
			fRefreshRateSlider->SetApplyNow(fApplyNow);
			fDoneBtn->SetEnabled(
				!CompareFloats(fRefreshRateSlider->Rate(), RefreshRate()));
			if (fOwner)
				fOwner->PostMessage(msg_refresh_apply_now);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool
TConfigRefreshRateWind::QuitRequested()
{
	return true;
}
	
	
// ************************************************************************** //

#if SHOW_CONFIG_WS
TConfigWorkspaceWind::TConfigWorkspaceWind(BPoint loc, BWindow* owner)
	: BWindow( BRect(loc.x, loc.y, loc.x+kCWWidth, loc.y+kCWHeight),
		"Workspace Count",
		B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE)
{
	if (!BScreen(B_MAIN_SCREEN_ID).Frame().Contains(Frame()))
		CenterWindowOnScreen(this);

	fOwner = owner;
	fWorkspaceCount = fOriginalWSCount = count_workspaces();

	BRect r(Bounds());
	r.InsetBy(-1, -1);
	fBG = new BBox( r, "bg", B_FOLLOW_ALL);
	fBG->SetFont(be_plain_font);
	AddChild(fBG);
	
	//	build the popup menu
	fWSMenu = new BPopUpMenu("workspace  count");

	char 		str[32];
	BMessage 	*msg=NULL;
	int32		max_ws = (fWorkspaceCount >= kMaxWorkspaceCount)
					? fWorkspaceCount : kMaxWorkspaceCount;
	for (int32 i=1 ; i<=max_ws ; i++) {
		msg = new BMessage(msg_config_index);
		msg->AddInt32("count", i);
		sprintf(str, "%i", i);
		fWSMenu->AddItem(new BMenuItem(str, msg));		
	}
	
	//	check the currect workspace count
	BMenuItem* mi = fWSMenu->ItemAt(fWorkspaceCount-1);
	if (mi)
		mi->SetMarked(true);

	r.Set(	Bounds().Width()-9-fBG->StringWidth("Workspace count")-50,
			15,
			Bounds().Width() - 9,
			35);
	fWSBtn = new BMenuField(r, "", "Workspace count:", fWSMenu, true);
	
	fBG->AddChild(fWSBtn);
	fWSBtn->SetAlignment(B_ALIGN_RIGHT);
	fWSBtn->SetDivider(fBG->StringWidth("Workspace count:")+5);

	//	 some way of closing this window
	float h = FontHeight(fBG, true);
	r.Set(	Bounds().Width() - 75 - 13,
			Bounds().Height() - 26 - h,
		  	Bounds().Width() - 13,
			Bounds().Height() - 26);
	fOkayBtn = new BButton(r, "okay", "OK", new BMessage(msg_config_okay),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	fBG->AddChild(fOkayBtn);
	
	r.right = r.left - 14;
	r.left = r.right - 75;
	fCancelBtn = new BButton(r, "Cancel", "Cancel",
		new BMessage(msg_config_cancel),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	fBG->AddChild(fCancelBtn);
	
	SetDefaultButton(fOkayBtn);
}


TConfigWorkspaceWind::~TConfigWorkspaceWind()
{
}

void
TConfigWorkspaceWind::FrameResized(float w, float h)
{
	BWindow::FrameResized(w,h);
}

void
TConfigWorkspaceWind::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case msg_config_index:
			msg->FindInt32("count", &fWorkspaceCount);
			if (fWorkspaceCount != count_workspaces())
				set_workspace_count(fWorkspaceCount);
			break;

		case msg_config_okay:
			//	tell the owning window that the workspace count was modified
			if (fOwner)
				fOwner->PostMessage(msg_config_okay);
			PostMessage(B_QUIT_REQUESTED);
			break;
			
		case msg_config_cancel:
			set_workspace_count(fOriginalWSCount);
			if (fOwner)
				fOwner->PostMessage(msg_config_cancel);
			PostMessage(B_QUIT_REQUESTED);
			break;
			
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool
TConfigWorkspaceWind::QuitRequested()
{
	return true;
}
#endif

#if SHOW_PICTURE_BTN
const unsigned char kQuestionMarkBits[] = {
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x15,0x15,0x15,0x15,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x0c,0x00,0x00,0x00,0x0c,0x15,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x15,0x0c,0x00,0x00,0x00,0x00,0x00,0x0c,0x15,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x15,0x0c,0x00,0x00,0x0c,0x0c,0x0c,0x00,0x00,0x0c,0x15,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x15,0x00,0x00,0x0c,0x15,0x15,0x15,0x0c,0x00,0x00,0x15,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x15,0x00,0x00,0x15,0x1b,0x1b,0x15,0x0c,0x00,0x00,0x15,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x15,0x15,0x1b,0x1b,0x1b,0x15,0x0c,0x00,0x00,0x15,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x0c,0x00,0x00,0x0c,0x15,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x0c,0x00,0x00,0x0c,0x15,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x0c,0x00,0x00,0x0c,0x15,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x00,0x00,0x0c,0x15,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x00,0x00,0x15,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x00,0x00,0x15,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x00,0x00,0x15,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x15,0x15,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b
};

TPictureButton::TPictureButton(BRect frame, BMessage* msg)
	: BControl(frame, "", "", msg, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fBits = new BBitmap( Bounds(), B_COLOR_8_BIT, false);
	fBits->SetBits(kQuestionMarkBits, (16*16),
		0, B_COLOR_8_BIT);
}

TPictureButton::~TPictureButton()
{
	delete fBits;
}

void 
TPictureButton::Draw(BRect)
{
	DrawBitmap(fBits);

	if (Value() != 0) {
		InvertRect(Bounds());
	}
}

status_t	
TPictureButton::Invoke(BMessage *msg)
{
	Sync();
	snooze(50000);

	status_t e = BControl::Invoke(msg);

	// BButton never hold their state. So after Invoking reset the value
	SetValue(0);

	return e;
}

void
TPictureButton::MouseDown(BPoint where)
{
	BRect		bound;
	ulong		buttons;

	if (!IsEnabled())
		return;

	bound = Bounds();
	SetValue(1);

	if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		return;
	}

	do {
		snooze(40000);
		GetMouse(&where, &buttons);
		if (bound.Contains(where) != Value()) {
			SetValue(Value() ^ 0x01);
		}
	} while(buttons);

	if (Value()) {
		Invoke();		// Invoke will reset the value to 0
	}
}

void 
TPictureButton::MouseUp(BPoint pt)
{
	if (IsTracking()) {
		if (Bounds().Contains(pt)) {
			Invoke();		// Invoke will reset the value to 0
		}
		SetTracking(false);
	}
}

void 
TPictureButton::MouseMoved(BPoint pt, uint32 , const BMessage *)
{
	if (IsTracking()) {
		if (Bounds().Contains(pt) != Value()) {
			SetValue(Value() ^ 0x01);
		}
	}
}
#endif
