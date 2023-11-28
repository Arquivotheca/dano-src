/******************************************************************************
**
**	File:		Alert.cpp
**
**	Description:	BAlert class.
**
**	Written by: Peter Potrebic
**
**	Copyright 1993-97, Be Incorporated, All Rights Reserved.
**
******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Debug.h>

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _TEXT_VIEW_H
#include <TextView.h>
#endif
#ifndef _ROSTER_H
#include <Roster.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _RESOURCES_H
#include <Resources.h>
#endif
#ifndef _PATH_H
#include <Path.h>
#endif
#ifndef _FIND_DIRECTORY_H
#include <FindDirectory.h>
#endif
#ifndef _AP_DEFS_PRIVATE_H
#include <AppDefsPrivate.h>
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#include <support/Beep.h>

#ifndef _MESSAGE_FILTER_H
#include <MessageFilter.h>
#endif
#if !defined(_SCREEN_H)
#include <Screen.h>
#endif /* _SCREEN_H */

#if _SUPPORTS_FEATURE_HTML_ALERTS
#include <www/TellBrowser.h>
#endif

#include <ClassInfo.h>

#define X_GUTTER		10
#define EXTRA_X			45
#define Y_GUTTER		6
#define WIDTH			310
//#define BUTTON_HEIGHT	23
#define BUTTON_WIDTH	75
#define BUTTON_SPACING	9

/* ------------------------------------------------------------------------- */

class _BAlertFilter_ : public BMessageFilter {
public:
						_BAlertFilter_(BAlert *alert);
virtual	filter_result	Filter(BMessage *message, BHandler **target);

		BAlert			*fAlert;
};

/* ------------------------------------------------------------------------- */

_BAlertFilter_::_BAlertFilter_(BAlert *alert)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
	fAlert = alert;
}

/* ------------------------------------------------------------------------- */

filter_result	_BAlertFilter_::Filter(BMessage *msg, BHandler **)
{
	BButton		*b;
	long		i;
	uchar		ch = 0;

	if (msg->FindInt8("byte", (int8 *)&ch) != B_NO_ERROR)
		return B_DISPATCH_MESSAGE;

	for (i = 0; i < 3 && (b = fAlert->fButtons[i]) != 0; i++) {
		if (ch == fAlert->fKeys[i]) {
			// simluate a button click so that the button is hilighted
			b->SetValue(1);
			b->Sync();
			snooze(50000);
//+			ASSERT(b->Message());
//+			b->Message()->FindInt32("which", &(fAlert->fAlertVal));
//+
//+			ASSERT(fAlert->fAlertVal == i);

			BMessage msg(ALERT_BUTTON_MSG);
			msg.AddInt32("which", i);
			(void) fAlert->PostMessage(&msg);	// ignore error
			// it's OK to ignore error above. BAlert is built-in window, so
			// it shoulnd't be handling many messages. And if this message
			// is dropped it isn't a big deal: User hit a ke to simulate
			// a button click... it didn't work... user hits key again.

			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
}

/* ------------------------------------------------------------------------- */

class TAlertView;

class TAlertView : public BView {
public:
					TAlertView(BRect frame);
					TAlertView(BMessage *data);
	virtual			~TAlertView();

static	BArchivable	*Instantiate(BMessage *data);
virtual	long		Archive(BMessage *data, bool deep = true) const;

virtual	void		Draw(BRect rect);

		BBitmap		*fBitmap;
};

/* ------------------------------------------------------------------------- */

BAlert::~BAlert()
{
	if (fAlertSem != -1)
		delete_sem(fAlertSem);
}

/* ------------------------------------------------------------------------- */

BAlert::BAlert(const char *title, const char *text, const char *b1,
	const char *b2, const char *b3, button_width resize_mode,
	alert_type msg_type)
 : BWindow( BRect(100, 100, 100+WIDTH, 100+40), title, B_MODAL_WINDOW,
	B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	InitObject(text, b1, b2, b3, resize_mode, B_EVEN_SPACING, msg_type);
	BPoint where = AlertPosition(Frame().Width(), Frame().Height());
	MoveTo(where.x, where.y);
}

/* ------------------------------------------------------------------------- */

BAlert::BAlert(const char *title, const char *text, const char *b1,
	const char *b2, const char *b3, button_width resize_mode,
	button_spacing spacing, alert_type msg_type)
 : BWindow( BRect(100, 100,
 					100+WIDTH + ((spacing == B_OFFSET_SPACING && b3) ? 25:0),
					100+40),
 	title, B_MODAL_WINDOW, B_NOT_RESIZABLE | B_NOT_CLOSABLE)
{
	InitObject(text, b1, b2, b3, resize_mode, spacing, msg_type);
	BPoint where = AlertPosition(Frame().Width(), Frame().Height());
	MoveTo(where.x, where.y);
}

/* ---------------------------------------------------------------- */

BAlert::BAlert(BMessage *data)
	: BWindow(data)
{
	long tmp;
	Lock();

	fInvoker = NULL;
	fAlertVal = -1;
	fAlertSem = -1;
	fTextView = (BTextView *) FindView("_tv_");
	ASSERT(fTextView);
	data->FindInt32(S_ALERT_TYPE, &tmp);
	fMsgType = (alert_type) tmp;
	data->FindInt32(S_BUTTON_WIDTH, &tmp);
	fButtonWidth = (button_width) tmp;
	fButtons[0] = fButtons[1] = fButtons[2] = NULL;

	BButton *lastButton;
	lastButton = fButtons[0] = (BButton *) FindView("_b0_");
	fButtons[1] = (BButton *) FindView("_b1_");
	if (fButtons[1])
		lastButton = fButtons[1];
	fButtons[2] = (BButton *) FindView("_b2_");
	if (fButtons[2])
		lastButton = fButtons[2];

	TAlertView *master = (TAlertView *) FindView("_master_");
	ASSERT(master);
	master->fBitmap = InitIcon();

	SetDefaultButton(lastButton);

	AddCommonFilter(new _BAlertFilter_(this));

	if (data->HasInt32(S_BUTTON_KEYS)) {
		data->FindInt32(S_BUTTON_KEYS, &tmp);
		SetShortcut(0, tmp);
		data->FindInt32(S_BUTTON_KEYS, 1, &tmp);
		SetShortcut(1, tmp);
		data->FindInt32(S_BUTTON_KEYS, 2, &tmp);
		SetShortcut(2, tmp);
	}
	Unlock();
}

/* ---------------------------------------------------------------- */

status_t BAlert::Archive(BMessage *data, bool deep) const
{
	BWindow::Archive(data, deep);

	// ??? Need to collapse the text_view itself, in case the developer
	// has configured it (font, size, color, etc).

	if (fTextView->Text())
		data->AddString(S_TEXT, fTextView->Text());

	data->AddInt32(S_ALERT_TYPE, fMsgType);
	data->AddInt32(S_BUTTON_WIDTH, fButtonWidth);

	if (fKeys[0] || fKeys[1] || fKeys[2]) {
		data->AddInt32(S_BUTTON_KEYS, fKeys[0]);
		data->AddInt32(S_BUTTON_KEYS, fKeys[1]);
		data->AddInt32(S_BUTTON_KEYS, fKeys[2]);
	}
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BAlert::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BAlert"))
		return NULL;
	return new BAlert(data);
}


/* ------------------------------------------------------------------------- */

void BAlert::InitObject(const char *text, const char *b1,
	const char *b2, const char *b3, button_width resize_mode,
	button_spacing spacing, alert_type msg_type)
{
	BRect		wBounds;
	BRect		bounds;
	float		height, viewHeight;
	float		widths[3];
	float		maxWidth;
	int			nButtons = 1;
	int			i;
	BButton		*lastBut;
	BMessage	*msg;
	long		extra_x = 0;
	float		buttonHeight = 0.0;
	bool		offset_button3 = (b3 &&
								(spacing == B_OFFSET_SPACING));
	bool		offset_button2 = ((b2 && !b3) &&
								(spacing == B_OFFSET_SPACING));

	if (b1 == NULL) {
		debugger("BAlert's must have at least one button.\n");
		b1 = B_EMPTY_STRING;
	}

//+	PRINT(("New alert (msg=%d)\n", msg_type));

	fInvoker = NULL;
	fAlertSem = -1;
	fAlertVal = -1;
	fTextView = NULL;
	fMsgType = msg_type;
	fButtonWidth = resize_mode;
	fButtons[0] = fButtons[1] = fButtons[2] = NULL;

	if(b2)
		nButtons++;
	if(b3)
		nButtons++;
	
	Lock();

	/*
	 make top level view.
	*/
	TAlertView *master = new TAlertView(Bounds());
	AddChild(master);
	master->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	master->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);

	master->fBitmap = InitIcon();
	if (master->fBitmap)
		extra_x = EXTRA_X;

	/*
	 Now initialize the buttons. There final locations won't be determined
	 until after all buttons are initialized and we determine the width
	 of the buttons and the height of the text area.
	*/
	bounds.Set(0, 0, BUTTON_WIDTH, 0); 

	fKeys[0] = fKeys[1] = fKeys[2] = 0;
	widths[0] = widths[1] = widths[2] = 0;

	i = 1;

	msg = new BMessage(ALERT_BUTTON_MSG);
	msg->AddInt32("which", 0);
	lastBut = fButtons[0] = new BButton(bounds, "_b0_", b1, msg,
		B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
	master->AddChild(lastBut);
	maxWidth = widths[0] = lastBut->StringWidth(b1) + (2*X_GUTTER);
//+	PRINT(("width \"%s\" = %.1f\n", b1, widths[0]));

	if (b2) {
		msg = new BMessage(ALERT_BUTTON_MSG);
		msg->AddInt32("which", 1);
		lastBut = fButtons[1] = new BButton(bounds, "_b1_", b2, msg,
			B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
		master->AddChild(lastBut);
		widths[1] = lastBut->StringWidth(b2) + (2*X_GUTTER);
//+		PRINT(("width \"%s\" = %.1f\n", b2, widths[1]));
		if (widths[1] > maxWidth)
			maxWidth = widths[1];
	}
	if (b3) {
		msg = new BMessage(ALERT_BUTTON_MSG);
		msg->AddInt32("which", 2);
		lastBut = fButtons[2] = new BButton(bounds, "_b2_", b3, msg,
			B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
		master->AddChild(lastBut);
		widths[2] = lastBut->StringWidth(b3) + (2*X_GUTTER);
//+		PRINT(("width \"%s\" = %.1f\n", b3, widths[2]));
		if (widths[2] > maxWidth)
			maxWidth = widths[2];
	}

	SetDefaultButton(lastBut);
	buttonHeight = lastBut->Frame().Height();

	if (resize_mode == B_WIDTH_FROM_WIDEST) {
		float	windowWidth;

		wBounds = Bounds();
		windowWidth = extra_x + (2 * X_GUTTER) + (nButtons * maxWidth) +
			((nButtons-1) * BUTTON_SPACING) +
			(offset_button3 ? BUTTON_SPACING*2 : 0);
//+		PRINT(("windowWidth = %.1f, wBounds.width=%.1f\n",
//+			windowWidth, wBounds.Width()));
		if (windowWidth > wBounds.Width()) {
			ResizeTo(windowWidth, wBounds.Height());
		}
	} else {
		float	windowWidth;

		wBounds = Bounds();
		windowWidth = extra_x + (2 * X_GUTTER) +
			((nButtons - 1) * BUTTON_SPACING) +
			(offset_button3 ? BUTTON_SPACING*2 : 0);
		for (i = 0; i < nButtons; i++) {
			if (resize_mode == B_WIDTH_AS_USUAL && widths[i] < BUTTON_WIDTH)
				widths[i] = BUTTON_WIDTH;
			windowWidth += widths[i];
		}
//+		PRINT(("FROM_LABEL: windowWidth = %.1f, wBounds.width=%.1f\n",
//+			windowWidth, wBounds.Width()));
		if (windowWidth > wBounds.Width()) {
			ResizeTo(windowWidth, wBounds.Height());
		}
	}

	/*
	 Now that we've determined the 'width' of the alert we can
	 initialize the text view. After setting the text make sure
	 entire content is visible. Resize the view/window vertically
	 if needed.
	*/
	BRect	trect;

	wBounds = Bounds();
	bounds.Set(X_GUTTER + extra_x, Y_GUTTER, wBounds.right - X_GUTTER,
		Y_GUTTER + 20);
	trect = bounds;
	trect.OffsetTo(B_ORIGIN);

	fTextView = new BTextView(bounds, "_tv_", trect, 
							  be_plain_font, NULL, 
							  B_FOLLOW_LEFT + B_FOLLOW_TOP, B_WILL_DRAW);
	master->AddChild(fTextView);
	fTextView->MakeEditable(FALSE);
	fTextView->MakeSelectable(FALSE);
	fTextView->SetWordWrap(TRUE);
	fTextView->SetText(text);
	fTextView->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	fTextView->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);

	// resize textview so that all lines are visible.
	height = fTextView->CountLines() * fTextView->LineHeight();
	bounds.bottom = bounds.top + height;
	trect.bottom = trect.top + height;
	fTextView->ResizeTo(bounds.Width(), bounds.Height());
	fTextView->SetTextRect(trect);

	/*
	 Resize the height of the window based on the text view.
	*/
	bounds = fTextView->Bounds();
	viewHeight = bounds.Height();
	height = 2*Y_GUTTER + viewHeight + BUTTON_SPACING + buttonHeight;	
	wBounds = Bounds();
	ResizeTo(wBounds.Width(), height);

	// position the buttons
	wBounds = Bounds();
//+	PRINT_OBJ(wBounds);
	if (resize_mode == B_WIDTH_FROM_LABEL || resize_mode == B_WIDTH_AS_USUAL) {
		float	x;
		i = nButtons - 1;			// 'i' is index of last button
		x = wBounds.right - (X_GUTTER - 4);
		while (i >= 0) {
			float curButtonHeight = fButtons[i]->Frame().Height();
			float bTop = wBounds.bottom - Y_GUTTER - buttonHeight - 
						 ((curButtonHeight - buttonHeight) / 2);

			x -= widths[i];
			if (offset_button3 && i == 0)
				x -= (BUTTON_SPACING*2 + 5);
			if (offset_button2 && i == 0)
				x = extra_x + 10;
			fButtons[i]->MoveTo(x, bTop);
			fButtons[i]->ResizeTo(widths[i], curButtonHeight);
//+			PRINT(("button %d:", i)); PRINT_OBJECT((fButtons[i]->Frame()));
			x -= BUTTON_SPACING;
			if (i == (nButtons - 1))
				x += 3;
			i--;
		}
	} else {
		float	x;
		i = nButtons - 1;			// 'i' is index of last button
		x = wBounds.right - (X_GUTTER - 4);
		while (i >= 0) {
			float curButtonHeight = fButtons[i]->Frame().Height();
			float bTop = wBounds.bottom - Y_GUTTER - buttonHeight - 
						 ((curButtonHeight - buttonHeight) / 2);

			x -= maxWidth;
			if (offset_button3 && i == 0)
				x -= (BUTTON_SPACING*2 + 5);
			if (offset_button2 && i == 0)
				x = extra_x + 10;
			fButtons[i]->MoveTo(x, bTop);
			fButtons[i]->ResizeTo(maxWidth, curButtonHeight);
//+			PRINT(("button %d:", i)); PRINT_OBJECT((fButtons[i]->Frame()));
			x -= BUTTON_SPACING;
			if (i == (nButtons - 1))
				x += 3;
			i--;
		}
	}

	AddCommonFilter(new _BAlertFilter_(this));

	Unlock();
}

/* ------------------------------------------------------------------------- */

BBitmap *BAlert::InitIcon()
{
	BBitmap *bitmap = NULL;

#if _SUPPORTS_RESOURCES

	/*
	 Find the 'icon' for the alert
	*/
	if (fMsgType != B_EMPTY_ALERT) {
		BPath		path;
		if (find_directory (B_BEOS_SERVERS_DIRECTORY, &path) == B_OK) {
			path.Append ("app_server");
			BFile		file(path.Path(), O_RDONLY);
			BResources	rfile;

//+			PRINT(("Resource file err=%x\n", err));
			if (rfile.SetTo(&file) == B_NO_ERROR) {
				size_t	size;
				char	*name = "";
				switch(fMsgType) {
					case B_INFO_ALERT:		name = "info"; break;
					case B_IDEA_ALERT:		name = "idea"; break;
					case B_WARNING_ALERT:	name = "warn"; break;
					case B_STOP_ALERT:		name = "stop"; break;
					default:
						break;
				}
				void	*data = rfile.FindResource('ICON', name, &size);
//+				PRINT(("FindResource err=%x, data=%x\n", rfile.Error(), data));
				if (data) {
					bitmap = new BBitmap(BRect(0,0,31,31), B_COLOR_8_BIT);
					bitmap->SetBits(data, size, 0, B_COLOR_8_BIT);
					free(data);
				}
			} 
		}
	}

#endif

	if (!bitmap) {
		// couldn't find icon so make this an B_EMPTY_ALERT
		fMsgType = B_EMPTY_ALERT;
	}
		
	return bitmap;
}

/* ------------------------------------------------------------------------- */

void BAlert::FrameResized(float, float)
{
	if (fTextView) {
		BRect	rect;
		BRect	w(Bounds());

//		fTextView->ResizeTo(
//			w.Width() - ((2 * X_GUTTER) + (fMsgType ? EXTRA_X : 0)),
//			w.Height() - ((2 * Y_GUTTER) + BUTTON_SPACING + BUTTON_HEIGHT));

		float buttonHeight = fButtons[0]->Frame().Height();
		for (int32 i = 0; i < 3; i++) {
			if (fButtons[i] == NULL)
				break;
			else
				buttonHeight = fButtons[i]->Frame().Height();
		}

		fTextView->ResizeTo(
			w.Width() - ((2 * X_GUTTER) + (fMsgType ? EXTRA_X : 0)),
			w.Height() - ((2 * Y_GUTTER) + BUTTON_SPACING + buttonHeight));

		rect = fTextView->Bounds();
		rect.OffsetTo(B_ORIGIN);
		fTextView->SetTextRect(rect);
	}
}

/* ------------------------------------------------------------------------- */

char BAlert::Shortcut(int32 button_index) const
{
	if ((button_index >= 3)) 
		return 0;

	BButton *button = ButtonAt(button_index);

	if (!button)
		return 0;

	return fKeys[button_index];
}

/* ------------------------------------------------------------------------- */

void BAlert::SetShortcut(int32 button_index, char key)
{
	long	i;

	if ((button_index >= 3)) 
		return;

	BButton *button = ButtonAt(button_index);

	if (!button)
		return;

	// check if that key is currently being used by a button. If so - remove!
	for (i = 0; i < 3; i++) {
		if (fKeys[i] == key) {
			fKeys[i] = 0;
			break;
		}
	}

	fKeys[button_index] = key;
}

/* ------------------------------------------------------------------------- */

status_t BAlert::Go(BInvoker *invoker)
{
	// asychronous case
	// invoker can be NULL.

	fInvoker = invoker;
	Show();
	switch(fMsgType) {
		case B_EMPTY_ALERT:		system_beep("Alert Empty"); break;
		case B_INFO_ALERT:		system_beep("Alert Info"); break;
		case B_IDEA_ALERT:		system_beep("Alert Idea"); break;
		case B_WARNING_ALERT:	system_beep("Alert Warning"); break;
		case B_STOP_ALERT:		system_beep("Alert Stop"); break;
		default:
			break;
	}
	return B_NO_ERROR;
}

/* ------------------------------------------------------------------------- */

int32 BAlert::Go()
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
	switch(fMsgType) {
		case B_EMPTY_ALERT:		system_beep("Alert Empty"); break;
		case B_INFO_ALERT:		system_beep("Alert Info"); break;
		case B_IDEA_ALERT:		system_beep("Alert Idea"); break;
		case B_WARNING_ALERT:	system_beep("Alert Warning"); break;
		case B_STOP_ALERT:		system_beep("Alert Stop"); break;
		default:
			break;
	}

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

/* ------------------------------------------------------------------------- */

void BAlert::MessageReceived(BMessage *e)
{
	switch (e->what) {
		case ALERT_BUTTON_MSG:
			e->FindInt32("which", &fAlertVal);
//+			PRINT(("Received %d\n", fAlertVal));

			if (fAlertSem != -1) {
				// by deleting this semaphore, the Go() method will unblock.
				delete_sem(fAlertSem);
				fAlertSem = -1;
			} else if (fInvoker) {
				BMessage msg(*(fInvoker->Message()));
				msg.AddInt32("which", fAlertVal);
				fInvoker->Invoke(&msg);
				Quit();
			} else {
				// initiator of alert didn't care about response/result
				Quit();
			}

			break;
		default:
			BWindow::MessageReceived(e);
			break;
	}
}

/* ------------------------------------------------------------------------- */

TAlertView::TAlertView(BRect frame)
	: BView(frame, "_master_", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fBitmap = NULL;
}

/* ------------------------------------------------------------------------- */

TAlertView::TAlertView(BMessage *data)
	: BView(data)
{
	fBitmap = NULL;
}

/* ---------------------------------------------------------------- */

TAlertView::~TAlertView()
{
	delete (fBitmap);
}

/* ---------------------------------------------------------------- */

long TAlertView::Archive(BMessage *data, bool deep) const
{
	return BView::Archive(data, deep);
}

/* ---------------------------------------------------------------- */

BArchivable *TAlertView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "TAlertView"))
		return NULL;
	return new TAlertView(data);
}

/* ---------------------------------------------------------------- */

void TAlertView::Draw(BRect)
{
	if (fBitmap) {
		BRect	bounds(Bounds());
		bounds.right = bounds.left + X_GUTTER + EXTRA_X - 25;
		
		const rgb_color kBlack = { 0, 0, 0, 255 };
		SetHighColor(ViewColor().blend_copy(kBlack, 40));
		FillRect(bounds);

		SetDrawingMode(B_OP_OVER);
		DrawBitmapAsync(fBitmap, BPoint(X_GUTTER + EXTRA_X - 25 - 12, Y_GUTTER));
		SetDrawingMode(B_OP_COPY);
	} 
}

/* ---------------------------------------------------------------- */

BButton *BAlert::ButtonAt(int32 index) const
	{ return fButtons[index]; }

/* ---------------------------------------------------------------- */

BTextView *BAlert::TextView() const
	{ return fTextView; }

/*-------------------------------------------------------------*/

BHandler *BAlert::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BWindow::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BAlert::GetSupportedSuites(BMessage *data)
{
	return BWindow::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BAlert::Perform(perform_code d, void *arg)
{
	return BWindow::Perform(d, arg);
}


/* ---------------------------------------------------------------- */

BPoint BAlert::AlertPosition(float width, float height)
{
	thread_id me = find_thread(NULL);
	BLooper * loop = BLooper::LooperForThread(me);
	BWindow * win = dynamic_cast<BWindow*>(loop);
	BRect screenFrame;
	BRect desirableRect;
	{
		BScreen screen(win);
		screenFrame = screen.Frame();
	}
	desirableRect = screenFrame;
	float mid_x = (desirableRect.left+desirableRect.right)/2.0;
	float mid_y = (desirableRect.top*3.0+desirableRect.bottom)/4.0;

	/* calculate alert position on an empty screen */

	desirableRect.left = mid_x-ceil(width/2.0);
	desirableRect.right = desirableRect.left+width;
	desirableRect.top = mid_y-ceil(height/3.0);
	desirableRect.bottom = desirableRect.top+height;

	/* adjust for window position, if there is one */

	if (win != NULL) {
		BRect winRect = win->Frame();
		float win_mid_x = (winRect.left+winRect.right)/2.0;
		float win_mid_y = (winRect.top*3.0+winRect.bottom)/4.0;

		/* move desirable rect by half distance between "ultimate" and window */

		desirableRect.OffsetBy(floor((win_mid_x-mid_x)/2.0), floor((win_mid_y-mid_y)/2.0));

		/* pin desirable rect within screen (with margin) */

#define POS_X_MARGIN 15.0
#define POS_Y_MARGIN 15.0

		if (desirableRect.right > screenFrame.right-POS_X_MARGIN) {
			desirableRect.OffsetTo(screenFrame.right-POS_X_MARGIN-width, desirableRect.top);
		}
		if (desirableRect.bottom > screenFrame.bottom-POS_Y_MARGIN) {
			desirableRect.OffsetTo(desirableRect.left, screenFrame.bottom-POS_Y_MARGIN-height);
		}
		if (desirableRect.left < screenFrame.left+POS_X_MARGIN) {
			desirableRect.OffsetTo(screenFrame.left+POS_X_MARGIN, desirableRect.top);
		}
		if (desirableRect.top < screenFrame.top+POS_X_MARGIN) {
			desirableRect.OffsetTo(desirableRect.left, screenFrame.top+POS_X_MARGIN);
		}
	}

	return desirableRect.LeftTop();
}

/* ---------------------------------------------------------------- */

void BAlert::DispatchMessage(BMessage *m, BHandler *h)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BWindow::DispatchMessage(m, h);
}

/* ---------------------------------------------------------------- */

void BAlert::Quit()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BWindow::Quit();
}

/* ---------------------------------------------------------------- */

bool BAlert::QuitRequested()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here in the future must consider the implications of that fact. Only
	 a concern on PPC, as Intel compatibility was broken in R4.
	*/
	return BWindow::QuitRequested();
}

/* ---------------------------------------------------------------- */

void BAlert::_ReservedAlert1() {}
void BAlert::_ReservedAlert2() {}
void BAlert::_ReservedAlert3() {}

/* ---------------------------------------------------------------- */

// Opera stuff

#if _SUPPORTS_FEATURE_HTML_ALERTS

status_t TellTellBrowser(BMessage*m) {
	return BMessenger(kTellBrowserSig).SendMessage(m);
}

status_t TellTellBrowser(BMessage*m,BMessage*r) {
	return BMessenger(kTellBrowserSig).SendMessage(m,r);
}

#else

// Opera stuff -- stub so that images can still load.

status_t TellTellBrowser(BMessage*) {
	return B_UNSUPPORTED;
}

status_t TellTellBrowser(BMessage*,BMessage*) {
	return B_UNSUPPORTED;
}

#endif
