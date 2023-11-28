//*********************************************************************
//	
//	ScrollBar.cpp
//
//	Written by: Robert Polic
//	Nearly completely rewritten by:  Robert Chinn
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//*********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include <interface_misc.h>

#include <Alert.h>
#include <Debug.h>
#include <Box.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>
#include <Screen.h>

#include "ScrollBar.h"
#include "scroll_bits.h"

//*********************************************************************

int main()
{	
	TScrollBarApplication().Run();
	return B_NO_ERROR;
}

//*********************************************************************

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

#if 0
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
#endif

static void
SetSystemSettings(bool arrows, bool proportional,
	short thumbStyle, float thumbWidth)
{
	scroll_bar_info	info;
	
	info.double_arrows = arrows;
	info.proportional = proportional;
	info.knob = thumbStyle;
	info.min_knob_size = (int32)thumbWidth;
	
	set_scroll_bar_info(&info);
}

static void
GetSystemSettings(bool *doubleArrows, bool *proportionalThumb,
	short *thumbStyle, float *thumbSize)
{
	scroll_bar_info	info;

	get_scroll_bar_info(&info);
	
	*doubleArrows = info.double_arrows;
	*proportionalThumb = info.proportional;
	*thumbStyle = info.knob;
	*thumbSize = info.min_knob_size;	
}

//*********************************************************************

TScrollBarApplication::TScrollBarApplication()
		  :BApplication("application/x-vnd.Be-SCRL")
{
	TScrollBarWindow* window = new TScrollBarWindow(BRect(0, 0, 180, 160),
		"Scroll Bar");

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("ScrollBar_data");

	int	ref;
	BPoint loc;
	if ((ref = open(path.Path(), 0)) >= 0) {
		read(ref, &loc, sizeof(BPoint));
		close(ref);
		if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(loc)) {
			window->MoveTo(loc);
			goto SHOW;			
		}
	}
	
	{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - window->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - window->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		window->MoveTo(pt);
	}
SHOW:
	window->Show();
}

void
TScrollBarApplication::MessageReceived(BMessage* m)
{
	switch(m->what) {
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		default:
			BApplication::MessageReceived(m);
			break;
	}
}

void
TScrollBarApplication::AboutRequested()
{
	(new BAlert("", "ScrollBar configuration", "OK"))->Go();
}

//*********************************************************************

const int32 kBoxWidth = 158;
const int32 kBoxHeight = 112;

TScrollBarWindow::TScrollBarWindow(BRect rect, char *title)
	:BWindow(rect, title, B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	GetSystemSettings(&fDoubleArrows, &fProportionalThumb,
		&fThumbStyle, &fThumbWidth);
	
	BRect frame(Bounds());
	frame.InsetBy(-1, -1);
	fBG = new BBox( frame, "", B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(fBG);
	
	frame.Set(12, 8, 12+kBoxWidth, 8+kBoxHeight);
	fArrowSelector = new TSBSelector( frame, "", "Arrow Style",
		arrow_selector);
	fBG->AddChild(fArrowSelector);

	frame.OffsetBy(11+kBoxWidth, 0);
//	fThumbSelector = new TSBSelector(frame, "", "Knob Type",
//		thumb_type_selector);
//	fBG->AddChild(fThumbSelector);

//	frame.OffsetBy(-(11+kBoxWidth), 4+kBoxHeight);
//	frame.bottom -= 3;
//	fKnobSelector = new TSBSelector(frame, "", "Minimum Knob Size",
//		thumb_size_selector);
//	fBG->AddChild(fKnobSelector);

//	frame.OffsetBy(11+kBoxWidth, 0);
//	fKnobSizeSelector = new TSBSelector(frame, "", "Knob Style",
//		thumb_style_selector);
//	fBG->AddChild(fKnobSizeSelector);

	frame.top = fArrowSelector->Frame().bottom + 1;
	frame.bottom = Bounds().Height()+1;
	frame.left = 1; frame.right = Bounds().Width()+1;
	fBtnBar = new TButtonBar(frame, true, true, BB_BORDER_NONE);
	fBG->AddChild(fBtnBar);
	
	fBtnBar->CanRevert(false);
	SetPulseRate(1000000);
	
	AddShortcut('I', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
	AddShortcut('R', B_COMMAND_KEY, new BMessage(msg_revert));
	AddShortcut('D', B_COMMAND_KEY, new BMessage(msg_defaults));
}

TScrollBarWindow::~TScrollBarWindow()
{
}

void TScrollBarWindow::MessageReceived(BMessage* theMessage)
{
	bool needToUpdate = false;
	switch(theMessage->what) {
		case B_ABOUT_REQUESTED:
			be_app->MessageReceived(theMessage);
			break;
			
		case msg_selector_change:
			needToUpdate = true;
			fBtnBar->CanRevert(true);
			break;
			
		case msg_defaults:
			SetSystemSettings(true, true, 2, 15);
			needToUpdate = true;
			fBtnBar->CanRevert(true);
			break;
		case msg_revert:
			SetSystemSettings(fDoubleArrows, fProportionalThumb,
				fThumbStyle, fThumbWidth);
			needToUpdate = true;
			fBtnBar->CanRevert(false);
			break;

		default:
			BWindow::MessageReceived(theMessage);
	}
	
	if (needToUpdate) {
		fArrowSelector->SyncWithSystem();
//		fThumbSelector->SyncWithSystem();
//		fKnobSelector->SyncWithSystem();
//		fKnobSizeSelector->SyncWithSystem();
	}
}

bool TScrollBarWindow::QuitRequested()
{
	int		ref;
	BPoint	loc;

	loc = Frame().LeftTop();

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	path.Append("ScrollBar_data");

	if ((ref = creat(path.Path(), 0777)) >= 0) {
		write(ref, &loc, sizeof(BPoint));
		close(ref);
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

//*********************************************************************

TSBSelector::TSBSelector(BRect frame, const char* name, const char* label,
	selector_type type)
	: TBox(frame, name, label, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE | B_PULSE_NEEDED),
		fType(type)
{
	BRect bounds(0, 0, 15, 12);
	fBits = new BBitmap(bounds, B_COLOR_8_BIT);

	bounds.Set(0, 0, 128, 24);
	fOffView = new BView(bounds, "", B_FOLLOW_ALL, B_WILL_DRAW);
	
	fSizeBits = new BBitmap(bounds, B_RGBA32, true);
	fSizeBits->AddChild(fOffView);
}

TSBSelector::~TSBSelector()
{
	delete fBits;
	delete fSizeBits;
}

void
TSBSelector::AttachedToWindow()
{
	GetScrollBarInfo();
	CacheSelectorFrames();
	fSizerColor.red = 0; fSizerColor.green = 255;
	fSizerColor.blue = 0; fSizerColor.alpha = 255;
	fTime = time(NULL);
}

void
TSBSelector::Draw(BRect where)
{
	TBox::Draw(where);
	
	switch(fType) {
		case arrow_selector:
			DrawArrowStyleSelector();
			break;

		case thumb_type_selector:
			DrawKnobTypeSelector();
			break;
		
		case thumb_style_selector:
			DrawKnobStyleSelector();
			break;

		case thumb_size_selector:
			DrawKnobSizeSelector();
			break;
	}
	UpdateSelection();		// updates and syncs all selection frames
}

//	arrow key navigation for each selector
bool
TSBSelector::ChangeSelection(bool direction)
{
	if (fType == arrow_selector) {
		fDoubleArrows = !fDoubleArrows;
		return true;
	} else if (fType == thumb_type_selector) {
		fProportionalThumb = !fProportionalThumb;
		return true;
	} else if (fType == thumb_style_selector){
		if (direction)
			fThumbStyle++;
		else
			fThumbStyle--;
			
		if (fThumbStyle < 0)
			fThumbStyle = 2;
		if (fThumbStyle > 2)
			fThumbStyle = 0;
			
		return true;
	} else
		return false;
}

//	arrow key navigation for the size selector
bool
TSBSelector::ChangeSize(bool direction)
{
	if (fType != thumb_size_selector)
		return false;
		
	float width = fThumbWidth;
	
	if (direction)
		width++;
	else
		width--;
		
	if (width < kMinThumbWidth)
		width = kMinThumbWidth;
	if (width > kMaxThumbWidth)
		width = kMaxThumbWidth;
	
	if (width != fThumbWidth) {
		fThumbWidth = width;
		DrawKnobSizeSelector();
		return true;
	} else
		return false;
}

void
TSBSelector::KeyDown(const char *key, int32 count)
{
	bool itemChanged=false;
	switch (key[0]) {
		case B_UP_ARROW:
			itemChanged = ChangeSelection(false);
			break;
		case B_DOWN_ARROW:
			itemChanged = ChangeSelection(true);
			break;

		case B_LEFT_ARROW:
			itemChanged = ChangeSize(false);
			break;
		case B_RIGHT_ARROW:
			itemChanged = ChangeSize(true);
			break;

		default:
			BView::KeyDown(key, count);
	}
	if (itemChanged) {
		SetScrollBarInfo();
		Invoke();
	}
}

sb_selector
TSBSelector::HitTest(BPoint where, BRect *frame)
{
	if (fType == arrow_selector) {
		*frame = fDoubleArrowFrame;
		if ((frame->Contains(where)) && (!fDoubleArrows))
			return sb_double;
	
		*frame = fSingleArrowFrame;
		if ((frame->Contains(where)) && (fDoubleArrows))
			return sb_single;
			
		return sb_none;
	} else if (fType == thumb_type_selector) {
		*frame = fProportionalThumbFrame;
		if ((frame->Contains(where)) && (!fProportionalThumb))
			return sb_proportional;
	
		*frame = fFixedThumbFrame;
		if ((frame->Contains(where)) && (fProportionalThumb))
			return sb_fixed;

		return sb_none;
	} else if (fType == thumb_style_selector) {
		*frame = fSimpleKnobStyleFrame;
		if ((frame->Contains(where)) && (fThumbStyle != 0))
			return sb_simple;
	
		*frame = fSquareKnobStyleFrame;
		if ((frame->Contains(where)) && (fThumbStyle != 1))
			return sb_square;
	
		*frame = fBarKnobStyleFrame;
		if ((frame->Contains(where)) && (fThumbStyle != 2))
			return sb_bar;

		return sb_none;
	} else if (fType == thumb_size_selector) {
		*frame = fKnobSizeFrame;
		if (frame->Contains(where))
			return sb_size;

		return sb_none;
	} else
		return sb_none;
}

void
TSBSelector::UpdateSelection()
{
	if (fType == arrow_selector) {
		if (fDoubleArrows){
			FrameItem(fDoubleArrowFrame, 1);
			FrameItem(fSingleArrowFrame, 0);
		} else {
			FrameItem(fDoubleArrowFrame, 0);
			FrameItem(fSingleArrowFrame, 1);
		}
	}
	
	if (fType == thumb_type_selector) {
		if (fProportionalThumb){
			FrameItem(fProportionalThumbFrame, 1);
			FrameItem(fFixedThumbFrame, 0);
		} else {
			FrameItem(fProportionalThumbFrame, 0);
			FrameItem(fFixedThumbFrame, 1);
		}
	}

	if (fType == thumb_style_selector) {
		if (fThumbStyle == 0){
			FrameItem(fSimpleKnobStyleFrame, 1);
			FrameItem(fSquareKnobStyleFrame, 0);
			FrameItem(fBarKnobStyleFrame, 0);
		} else if (fThumbStyle == 1) {
			FrameItem(fSquareKnobStyleFrame, 1);
			FrameItem(fSimpleKnobStyleFrame, 0);
			FrameItem(fBarKnobStyleFrame, 0);
		} else {
			FrameItem(fBarKnobStyleFrame, 1);
			FrameItem(fSimpleKnobStyleFrame, 0);
			FrameItem(fSquareKnobStyleFrame, 0);
		}
	}
}

bool
TSBSelector::DoHiliteTracking(BRect frame)
{
	bool itemChanged = true;

	FrameItem(frame, -1);	//draws a gray border around selected item

	ulong	buttons;
	BPoint	loc;
	do {
		GetMouse(&loc, &buttons);
		if (frame.Contains(loc)) {
			if (!itemChanged) {
				itemChanged = true;
				FrameItem(frame, -1);	// draw gray
			}
		} else if (itemChanged) {
			itemChanged = false;
			FrameItem(frame, 0);		// draw view color
		}
		snooze(50000);
	} while(buttons);
	
	return itemChanged;
}

bool
TSBSelector::DoSizeTracking(BRect frame)
{
	// calculate the dimensions of the thumb
	frame.left = frame.left + kThumbStart;
	frame.right = frame.left + fThumbWidth;
	//	offset for the drag region
	frame.top = frame.top+15;
	frame.bottom = frame.top + 6;
	
	BRect trackingRect(frame);
	trackingRect.top -= 15;
	trackingRect.left -= 6;
	trackingRect.right = trackingRect.left + kMaxThumbWidth + 6;

	ulong	buttons;
	BPoint	loc;
	float 	width = fThumbWidth;
	do {
		GetMouse(&loc, &buttons);
		
		if (trackingRect.Contains(loc)) {
			width = loc.x - frame.left;
			if (width < kMinThumbWidth)
				width = kMinThumbWidth;
			if (width > kMaxThumbWidth)
				width = kMaxThumbWidth;
	
			if (width != fThumbWidth) {
				fThumbWidth = width;
				DrawKnobSizeSelector();
				frame.right = frame.left + width;
			}
		}
		
		snooze(500);
	} while(buttons);
	
	return true;
}

void
TSBSelector::MouseDown(BPoint where)
{
	sb_selector item;
	BRect frame;
	
	//	find the selector that is hit
	//	return the specific items frame	
	if ((item = HitTest(where, &frame)) == sb_none)
		return;
	
	//	track the mouse
	//	knob size is slightly different
	bool itemChanged=false;
	if (item == sb_size)
		itemChanged = DoSizeTracking(frame);
	else
		itemChanged = DoHiliteTracking(frame);
	
	//	update the globals if a new selector is selected	
	if (itemChanged) {
		switch (item) {
			case sb_double:
			case sb_single:
				fDoubleArrows = (item==sb_double);
				break;
			case sb_proportional:
			case sb_fixed:
				fProportionalThumb = (item==sb_proportional);
				break;
			case sb_simple:		// 0
			case sb_square:		// 1
			case sb_bar:		// 2
				fThumbStyle = item - sb_simple;
				break;
			default:
				TRESPASS();
		}
		
		SetScrollBarInfo();		
		Invoke();
	}
}

void
TSBSelector::Pulse()
{
	TBox::Pulse();
}

void
TSBSelector::Invoke()
{
	BMessage* m = new BMessage(msg_selector_change);
	m->AddInt32("who", fType);
	
	Window()->PostMessage(m);
}

void
TSBSelector::DrawArrowStyleSelector()
{
	if (fType != arrow_selector)
		return;

	BFont oldFont;
	GetFont(&oldFont);
	PushState();

	SetFont(be_plain_font);
	SetHighColor(kBlack);
	SetLowColor(ViewColor());

	//	double arrow scroller
	MovePenTo(fDoubleArrowFrame.left, fDoubleArrowFrame.top-6);
	DrawString("Double:");

	DrawScrollBar(fDoubleArrowFrame, true, fProportionalThumb,
		fThumbWidth, fThumbStyle);

	//	single arrow scroller
	MovePenTo(fSingleArrowFrame.left, fSingleArrowFrame.top-6);
	DrawString("Single:");

	DrawScrollBar(fSingleArrowFrame, false, fProportionalThumb,
		fThumbWidth, fThumbStyle);

	PopState();
	SetFont(&oldFont);
}

void
TSBSelector::DrawKnobTypeSelector()
{
	if (fType != thumb_type_selector)
		return;

	BFont oldFont;
	GetFont(&oldFont);
	PushState();

	SetFont(be_plain_font);
	SetHighColor(kBlack);
	SetLowColor(ViewColor());

	// proportional thumb
	MovePenTo(fProportionalThumbFrame.left, fProportionalThumbFrame.top-6);
	DrawString("Proportional:");

	DrawScrollBar(fProportionalThumbFrame, fDoubleArrows, true,
		fThumbWidth, fThumbStyle);

	//	fixed thumb
	MovePenTo(fFixedThumbFrame.left, fFixedThumbFrame.top - 6);
	DrawString("Fixed:");

	DrawScrollBar(fFixedThumbFrame, fDoubleArrows, false,
		fThumbWidth, fThumbStyle);

	PopState();
	SetFont(&oldFont);
}

void
TSBSelector::DrawKnobSizeSelector()
{
	if (fType != thumb_size_selector)
		return;
		
	PushState();	
	DrawSizeScrollBar(fKnobSizeFrame);
	PopState();
}

void
TSBSelector::DrawKnobStyleSelector()
{
	if (fType != thumb_style_selector)
		return;

	PushState();
	
	DrawKnob(fSimpleKnobStyleFrame, 0);
	DrawKnob(fSquareKnobStyleFrame, 1);
	DrawKnob(fBarKnobStyleFrame, 2);
	
	PopState();
}

void
TSBSelector::CacheSelectorFrames()
{
	int32 fontHeight = (int32)FontHeight(this, true);
	int32 componentHeight;
	float xLoc = (Bounds().Width() - kScrollBarWidth) / 2;
	float yLoc, space;
	
	// **********************
	//	ARROWS	
	// **********************
	componentHeight = fontHeight + 5 + kScrollBarHeight;

	//	double button scroll bar
	space = (Bounds().Height() - (2 * componentHeight)) / 3;
	yLoc = space + fontHeight + 5;
	fDoubleArrowFrame.Set(xLoc, yLoc, xLoc+kScrollBarWidth, yLoc+kScrollBarHeight);

	//	single button scroll bar
	yLoc += kScrollBarHeight + space + fontHeight + 5;
	fSingleArrowFrame.Set(xLoc, yLoc, xLoc+kScrollBarWidth, yLoc+kScrollBarHeight);

	// **********************
	//	THUMBS
	// **********************
	componentHeight = fontHeight + 5 + kScrollBarHeight;
	
	// proportional thumb
	space = (Bounds().Height() - (2 * componentHeight)) / 3;
	yLoc = space + fontHeight;

	yLoc += 5;
	fProportionalThumbFrame.Set(xLoc, yLoc, xLoc+kScrollBarWidth, yLoc+kScrollBarHeight);

	//	fixed thumb
	yLoc += kScrollBarHeight + space + fontHeight;
	yLoc += 5;
	fFixedThumbFrame.Set(xLoc, yLoc, xLoc+kScrollBarWidth, yLoc+kScrollBarHeight);

	// **********************
	//	KNOB STYLES
	// **********************
	space = (Bounds().Height() - (3 * kScrollBarHeight)) / 4;
	yLoc = space + 5;		// 5 is to offset for bbox title
	
	//	simple
	fSimpleKnobStyleFrame.Set(xLoc, yLoc, xLoc+kScrollBarWidth, yLoc+kScrollBarHeight);

	//	square
	yLoc += kScrollBarHeight + space;
	fSquareKnobStyleFrame.Set(xLoc, yLoc, xLoc+kScrollBarWidth, yLoc+kScrollBarHeight);

	//	bars
	yLoc += kScrollBarHeight + space;
	fBarKnobStyleFrame.Set(xLoc, yLoc, xLoc+kScrollBarWidth, yLoc+kScrollBarHeight);

	// **********************
	//	KNOB SIZE
	// **********************
	space = (Bounds().Height() - kScrollBarHeight) / 2;
	yLoc = space + 5;		// 5 is to offset for bbox title
	
	fKnobSizeFrame.Set(xLoc, yLoc, xLoc+kScrollBarWidth, yLoc+24);
}

//	individual component draw routines

void
TSBSelector::DrawScrollBar(BRect frame, bool doubleArrows,
	bool thumbType, short thumbWidth, short thumbStyle)
{
	PushState();
	
	BRect	destRect;

	// draw the left arrow on the left side
	fBits->SetBits(ARROW_LEFT_NORMAL, fBits->BitsLength(), 0,
		B_COLOR_8_BIT);
	
	destRect.left = frame.left;
	destRect.right = destRect.left + HOR_ARROW_WIDTH;
	destRect.top = frame.top + 1;
	destRect.bottom = destRect.top + HOR_ARROW_HEIGHT;
	DrawBitmap(fBits, fBits->Bounds(), destRect);

	fBits->SetBits(ARROW_RIGHT_NORMAL, fBits->BitsLength(), 0,
		B_COLOR_8_BIT);

	//	draw the right arrow, if double, on the left side
	if (doubleArrows) {
		destRect.left = destRect.right;
		destRect.right += HOR_ARROW_WIDTH;
		DrawBitmap(fBits, fBits->Bounds(), destRect);
	}
	
	//	build a rect for the area between the arrows
	BRect centerRect(destRect);
	centerRect.left = destRect.right + 1;

	destRect.right = frame.right;
	destRect.left = destRect.right - HOR_ARROW_WIDTH;
	DrawBitmap(fBits, fBits->Bounds(), destRect);

	if (doubleArrows) {
		fBits->SetBits(ARROW_LEFT_NORMAL, fBits->BitsLength(), 0,
			B_COLOR_8_BIT);
		destRect.right = destRect.left;
		destRect.left -= HOR_ARROW_WIDTH;
		DrawBitmap(fBits, fBits->Bounds(), destRect);
	}

	centerRect.right = destRect.left - 1;

	SetLowColor(ViewColor());
	SetHighColor(kFillGray);
	FillRect(centerRect);
	
	BeginLineArray(7);

	//	top and left	
	AddLine(BPoint(centerRect.left, centerRect.top),
		BPoint(centerRect.left, centerRect.bottom), kShadeGray);
	AddLine(BPoint(centerRect.left + 1, centerRect.top),
		BPoint(centerRect.left + 1, centerRect.bottom), kShadeGray);
	AddLine(BPoint(centerRect.left, centerRect.top),
		BPoint(centerRect.right, centerRect.top), kShadeGray);
	AddLine(BPoint(centerRect.left, centerRect.top + 1),
		BPoint(centerRect.right, centerRect.top + 1), kShadeGray);

	AddLine(BPoint(centerRect.left, centerRect.bottom),
		BPoint(centerRect.right, centerRect.bottom), ViewColor());

	AddLine(BPoint(frame.left, frame.top),
		BPoint(frame.right, frame.top), kHiliteGray);
	AddLine(BPoint(frame.left, frame.bottom),
		BPoint(frame.right, frame.bottom), kHiliteGray);

	EndLineArray();
	
	float x;
	if (thumbType)
		x = .75;
	else
		x = 0;

	float thumb = (float)(centerRect.right - centerRect.left) * x;
	if (thumb < thumbWidth)
		thumb = thumbWidth;
	float offset = (((centerRect.right - centerRect.left) - thumb) / 2);
	centerRect.left = centerRect.left + offset;
	centerRect.right = centerRect.left + thumb;

	DrawThumb(centerRect, thumbStyle);
	
	PopState();
}

void
TSBSelector::DrawKnob(BRect frame, short thumbStyle)
{
	PushState();
	
	SetLowColor(ViewColor());

	BeginLineArray(15);
	
	SetHighColor(152, 152, 152);
	AddLine(BPoint(frame.left + 1, frame.top),
		BPoint(frame.right - 2, frame.top), kHiliteGray);
	AddLine(BPoint(frame.left + 1, frame.bottom),
		BPoint(frame.right - 2, frame.bottom), kHiliteGray);

	AddLine(BPoint(frame.left + 1, frame.top + 1),
		BPoint(frame.right - 2, frame.top + 1), kShadeGray);
	AddLine(BPoint(frame.left + 1, frame.top + 2),
		BPoint(frame.right - 2, frame.top + 2), kShadeGray);

	AddLine(BPoint(frame.left + 1, frame.top + 3),
		BPoint(frame.right - 2, frame.top + 3), kFillGray);
	AddLine(BPoint(frame.left + 2, frame.top + 4),
		BPoint(frame.right - 1, frame.top + 4), kFillGray);
	AddLine(BPoint(frame.left + 2, frame.top + 5),
		BPoint(frame.right - 1, frame.top + 5), kFillGray);
	AddLine(BPoint(frame.left + 3, frame.top + 6),
		BPoint(frame.right, frame.top + 6), kFillGray);
	AddLine(BPoint(frame.left + 2, frame.top + 7),
		BPoint(frame.right - 1, frame.top + 7), kFillGray);
	AddLine(BPoint(frame.left + 2, frame.top + 8),
		BPoint(frame.right - 1, frame.top + 8), kFillGray);
	AddLine(BPoint(frame.left + 1, frame.top + 9),
		BPoint(frame.right - 2, frame.top + 9), kFillGray);
	AddLine(BPoint(frame.left, frame.top + 10),
		BPoint(frame.right - 3, frame.top + 10), kFillGray);
	AddLine(BPoint(frame.left, frame.top + 11),
		BPoint(frame.right - 3, frame.top + 11), kFillGray);
	AddLine(BPoint(frame.left, frame.top + 12),
		BPoint(frame.right - 3, frame.top + 12), kFillGray);

	AddLine(BPoint(frame.left + 1, frame.top + 13),
		BPoint(frame.right - 2, frame.top + 13), ViewColor());
	
	EndLineArray();
	
	frame.left += 10;
	frame.right -= 10;
	frame.top += 1;
	frame.bottom -= 1;
	DrawThumb(frame, thumbStyle);
	
	PopState();
}

void
TSBSelector::DrawThumb(BRect frame, short thumbStyle)
{
	PushState();
	BeginLineArray(18);
	SetLowColor(kScrollBarColor);

	SetHighColor(kScrollBarColor);
	FillRect(frame);
	
	AddLine(BPoint(frame.right - 1, frame.top),
		BPoint(frame.right - 1, frame.bottom), kShadeGray);
	AddLine(BPoint(frame.left + 1, frame.bottom),
		BPoint(frame.right - 1, frame.bottom), kShadeGray);	

	AddLine(BPoint(frame.left + 1, frame.top),
		BPoint(frame.left + 1, frame.bottom - 2), kWhite);
	AddLine(BPoint(frame.left + 1, frame.top),
		BPoint(frame.right - 2, frame.top), kWhite);

	rgb_color c = {96, 96, 96, 255};
	AddLine(BPoint(frame.right, frame.top),
		BPoint(frame.right, frame.bottom), c);

	AddLine(BPoint(frame.left, frame.top),
		BPoint(frame.left, frame.bottom), kHiliteGray);

	float knob = frame.left + ((frame.right - frame.left) / 2) - 1;
	float top = frame.top + 4;

	switch (thumbStyle) {
		case 0:
			break;

		case 1:		// 12
			if ((frame.right - frame.left) > 12) {
				AddLine(BPoint(knob, top),
					BPoint(knob, top + 3), kWhite);
				AddLine(BPoint(knob + 1, top),
					BPoint(knob + 3, top), kWhite);
					
				AddLine(BPoint(knob, top + 4),
					BPoint(knob + 4, top + 4), kShadeGray);
				AddLine(BPoint(knob + 4, top),
					BPoint(knob + 4, top + 3), kShadeGray);

				if ((frame.right - frame.left) > 25) {
					AddLine(BPoint(knob - 7, top),
						BPoint(knob - 7, top + 3), kWhite);
					AddLine(BPoint(knob - 6, top),
						BPoint(knob - 4, top), kWhite);
					AddLine(BPoint(knob + 7, top),
						BPoint(knob + 7, top + 3), kWhite);
					AddLine(BPoint(knob + 8, top),
						BPoint(knob + 10, top), kWhite);

					AddLine(BPoint(knob - 7, top + 4),
						BPoint(knob - 3, top + 4), kShadeGray);
					AddLine(BPoint(knob - 3, top),
						BPoint(knob - 3, top + 3), kShadeGray);
					AddLine(BPoint(knob + 7, top + 4),
						BPoint(knob + 11, top + 4), kShadeGray);
					AddLine(BPoint(knob + 11, top),
						BPoint(knob + 11, top + 3), kShadeGray);
				}
			}
			break;

		case 2:		// 12
			top -= 1;
			knob += 1;
			if ((frame.right - frame.left) > 10) {
				AddLine(BPoint(knob, top),
					BPoint(knob + 1, top), kWhite);
				AddLine(BPoint(knob, top + 1),
					BPoint(knob, top + 6), kWhite);
				AddLine(BPoint(knob + 2, top),
					BPoint(knob + 2, top + 6), kShadeGray);
				AddLine(BPoint(knob + 1, top + 6),
					BPoint(knob + 1, top + 6), kShadeGray);

				if ((frame.right - frame.left) > 20) {
					AddLine(BPoint(knob - 4, top),
						BPoint(knob - 3, top), kWhite);
					AddLine(BPoint(knob - 4, top + 1),
						BPoint(knob - 4, top + 6), kWhite);
					AddLine(BPoint(knob + 4, top),
						BPoint(knob + 6, top), kWhite);
					AddLine(BPoint(knob + 4, top + 1),
						BPoint(knob + 4, top + 6), kWhite);

					AddLine(BPoint(knob - 2, top),
						BPoint(knob - 2, top + 6), kShadeGray);
					AddLine(BPoint(knob - 3, top + 6),
						BPoint(knob - 3, top + 6), kShadeGray);
					AddLine(BPoint(knob + 6, top),
						BPoint(knob + 6, top + 6), kShadeGray);
					AddLine(BPoint(knob + 5, top + 6),
						BPoint(knob + 5, top + 6), kShadeGray);
				}
			}
			break;
	}
	
	EndLineArray();
	PopState();
}


rgb_color
TSBSelector::CurrentSizeColor() const
{
//	rgb_color color = {10, 200, 50, 255};

	return fSizerColor;
}

void
TSBSelector::DrawSizeScrollBar(BRect destFrame)
{
	BRect rect(fSizeBits->Bounds());

	fSizeBits->Lock();
	fOffView->PushState();
	fOffView->SetDrawingMode(B_OP_COPY);
	fOffView->SetLowColor(ViewColor());
	fOffView->SetHighColor(ViewColor());
	fOffView->FillRect(rect);

	fOffView->BeginLineArray(14);
	fOffView->AddLine(BPoint(rect.left + 1, rect.top),
		BPoint(rect.right - 2, rect.top), kHiliteGray);
	fOffView->AddLine(BPoint(rect.left + 1, rect.top + 14),
		BPoint(rect.right - 2, rect.top + 14), kHiliteGray);

	fOffView->AddLine(BPoint(rect.left + 1, rect.top + 1),
		BPoint(rect.right - 2, rect.top + 1), kShadeGray);
	fOffView->AddLine(BPoint(rect.left + 1, rect.top + 2),
		BPoint(rect.right - 2, rect.top + 2), kShadeGray);

	fOffView->AddLine(BPoint(rect.left + 1, rect.top + 3),
		BPoint(rect.right - 2, rect.top + 3), kFillGray);
	fOffView->AddLine(BPoint(rect.left + 2, rect.top + 4),
		BPoint(rect.right - 1, rect.top + 4), kFillGray);
	fOffView->AddLine(BPoint(rect.left + 2, rect.top + 5),
		BPoint(rect.right - 1, rect.top + 5), kFillGray);
	fOffView->AddLine(BPoint(rect.left + 3, rect.top + 6),
		BPoint(rect.right, rect.top + 6), kFillGray);
	fOffView->AddLine(BPoint(rect.left + 2, rect.top + 7),
		BPoint(rect.right - 1, rect.top + 7), kFillGray);
	fOffView->AddLine(BPoint(rect.left + 2, rect.top + 8),
		BPoint(rect.right - 1, rect.top + 8), kFillGray);
	fOffView->AddLine(BPoint(rect.left + 1, rect.top + 9),
		BPoint(rect.right - 2, rect.top + 9), kFillGray);
	fOffView->AddLine(BPoint(rect.left, rect.top + 10),
		BPoint(rect.right - 3, rect.top + 10), kFillGray);
	fOffView->AddLine(BPoint(rect.left, rect.top + 11),
		BPoint(rect.right - 3, rect.top + 11), kFillGray);
	fOffView->AddLine(BPoint(rect.left, rect.top + 12),
		BPoint(rect.right - 3, rect.top + 12), kFillGray);

	fOffView->AddLine(BPoint(rect.left + 1, rect.top + 13),
		BPoint(rect.right - 2, rect.top + 13), ViewColor());
		
	fOffView->EndLineArray();

	// draw thumb...
	BRect r1;
	r1.left = rect.left + kThumbStart;
	r1.right = r1.left + fThumbWidth;
	r1.top = rect.top + 1;
	r1.bottom = r1.top + HOR_ARROW_HEIGHT;

	fOffView->SetHighColor(kScrollBarColor);
	fOffView->FillRect(r1);

	fOffView->BeginLineArray(13);
	fOffView->AddLine(BPoint(r1.right - 1, r1.top),
		BPoint(r1.right - 1, r1.bottom), kShadeGray);
	fOffView->AddLine(BPoint(r1.left + 1, r1.bottom),
		BPoint(r1.right - 1, r1.bottom), kShadeGray);	

	fOffView->AddLine(BPoint(r1.left + 1, r1.top),
		BPoint(r1.left + 1, r1.bottom - 2), kWhite);
	fOffView->AddLine(BPoint(r1.left + 1, r1.top),
		BPoint(r1.right - 2, r1.top), kWhite);

	rgb_color c = {96, 96, 96, 255};
	fOffView->AddLine(BPoint(r1.right, r1.top),
		BPoint(r1.right, r1.bottom), c);

	fOffView->AddLine(BPoint(r1.left, r1.top),
		BPoint(r1.left, r1.bottom), kHiliteGray);

	// draw sizing gadget...
	rgb_color gray = {80, 80, 80, 255};
	//	border
	fOffView->AddLine(BPoint(r1.right, r1.bottom + 2),
		BPoint(r1.right - 5, r1.bottom + 7), gray);
	fOffView->AddLine(BPoint(r1.right - 5, r1.bottom + 7),
		BPoint(r1.right + 5, r1.bottom + 7), gray);
	fOffView->AddLine(BPoint(r1.right + 5, r1.bottom + 7),
		BPoint(r1.right, r1.bottom + 2), gray);

	//	green fill
	rgb_color sizeColor = CurrentSizeColor();
	fOffView->AddLine(BPoint(r1.right, r1.bottom + 3),
		BPoint(r1.right, r1.bottom + 3), sizeColor);
	fOffView->AddLine(BPoint(r1.right - 1, r1.bottom + 4),
		BPoint(r1.right + 1, r1.bottom + 4), sizeColor);
	fOffView->AddLine(BPoint(r1.right - 2, r1.bottom + 5),
		BPoint(r1.right + 2, r1.bottom + 5), sizeColor);
	fOffView->AddLine(BPoint(r1.right - 3, r1.bottom + 6),
		BPoint(r1.right + 3, r1.bottom + 6), sizeColor);

	fOffView->EndLineArray();
	
	fOffView->PopState();
	fOffView->Sync();
	fSizeBits->Unlock();
	
	DrawBitmap(fSizeBits, rect, destFrame);	
}

//	draws a black border around selected item
void
TSBSelector::FrameItem(BRect rect, short select)
{
	PushState();
	
	SetLowColor(ViewColor());
	if (select == 1)
		SetHighColor(kBlack);
	else if (select == 0)
		SetHighColor(ViewColor());
	else
		SetHighColor(kSelectionGray);

	rect.InsetBy(-3, -3);
	StrokeRect(rect);
	rect.InsetBy(1, 1);
	StrokeRect(rect);
	rect.InsetBy(2, 2);
	
	PopState();
}

void
TSBSelector::GetScrollBarInfo()
{
	GetSystemSettings(&fDoubleArrows, &fProportionalThumb, &fThumbStyle, &fThumbWidth);
	fProportionalThumb = true;
	fThumbStyle = 2;
	fThumbWidth = 15;
}

void
TSBSelector::SetScrollBarInfo()
{
	char			data[256];
	short			loop;
	int				ref;

	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);

	path.Append("ScrollBar_settings");
	if ((ref = open(path.Path(), O_RDONLY | O_CREAT, 0777)) < 0) {
		for (loop = 0; loop < 4; loop++)
			write(ref, &data, sizeof(data));
	}
	if (ref >= 0)
		close(ref);

	SetSystemSettings(fDoubleArrows, fProportionalThumb,
		fThumbStyle, fThumbWidth);	
}

void
TSBSelector::SetScrollBarInfo(bool arrows, bool proportional,
	short thumbStyle, short thumbWidth)
{
	fDoubleArrows = arrows;
	fProportionalThumb = proportional;
	fThumbStyle = thumbStyle;
	fThumbWidth = thumbWidth;
	
	SetScrollBarInfo();
}

void
TSBSelector::SyncWithSystem()
{
	GetScrollBarInfo();
	Invalidate();
}

//*********************************************************************

const int32 kButtonXLoc = 10;

//	this is a simple generic object that draws buttons and lines
//	based on UI conformance rules
TButtonBar::TButtonBar(BRect frame, bool defaultsBtn, bool revertBtn,
	bb_border_type borderType)
	: BView(frame, "button bar", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW),
	fBorderType(borderType),
	fHasDefaultsBtn(defaultsBtn), fHasRevertBtn(revertBtn),
	fHasOtherBtn(false)
{
	BRect rect;
	
	rect.bottom = Bounds().Height()-10;
	rect.top = rect.bottom - (FontHeight(this, true) + 11);
	rect.left = kButtonXLoc;
	if (fHasDefaultsBtn) {
		rect.right = rect.left + 75;
		fDefaultsBtn = new BButton(rect, "defaults", "Defaults",
			new BMessage(msg_defaults));
		AddChild(fDefaultsBtn);
	} else
		fDefaultsBtn=NULL;
	
	if (fHasRevertBtn) {
		if (fHasDefaultsBtn)
			rect.left = fDefaultsBtn->Frame().right + 10;
		else
			rect.left = kButtonXLoc;
		rect.right = rect.left + 75;
		fRevertBtn = new BButton(rect, "revert", "Revert",
			new BMessage(msg_revert));
		AddChild(fRevertBtn);
	} else
		fRevertBtn=NULL;
		
	fOtherBtn=NULL;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


TButtonBar::~TButtonBar()
{
}

void  TButtonBar::Draw(BRect)
{
	PushState();	
	if (fHasOtherBtn) {
		BPoint top, bottom;
		
		top.x = fOtherBtn->Frame().right + 10;
		top.y = fOtherBtn->Frame().top;
		bottom.x = top.x;
		bottom.y = fOtherBtn->Frame().bottom;
		
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
		SetLowColor(ViewColor());
		StrokeLine(top, bottom);
		SetHighColor(kWhite);
		top.x++;
		bottom.x++;
		StrokeLine(top, bottom);
	}
	PopState();
}

void 
TButtonBar::AddButton(const char* title, BMessage* m)
{
	BRect rect;

	rect.bottom = Bounds().Height() - 10;
	rect.top = rect.bottom - (FontHeight(this, true) + 10);
	rect.left = kButtonXLoc;
	rect.right = rect.left + StringWidth(title) + 20;
	fOtherBtn = new BButton(rect, "other", title, m);
	
	float w = 22+fOtherBtn->Bounds().Width();
	if (fHasDefaultsBtn) {
		RemoveChild(fDefaultsBtn);
		if (fHasRevertBtn)
			RemoveChild(fRevertBtn);
		AddChild(fOtherBtn) /*, fDefaultsBtn)*/;
		AddChild(fDefaultsBtn);
		fDefaultsBtn->MoveBy(w, 0);
		if (fHasRevertBtn) {
			AddChild(fRevertBtn);
			fRevertBtn->MoveBy(w,0);
		}
	} else if (fHasRevertBtn) {
		RemoveChild(fRevertBtn);
		AddChild(fOtherBtn) /*, fRevertBtn)*/;
		AddChild(fRevertBtn);
		fRevertBtn->MoveBy(w,0);
	} else
		AddChild(fOtherBtn);
		
	fHasOtherBtn = (fOtherBtn != NULL);
}

void
TButtonBar::CanRevert(bool state)
{
	fRevertBtn->SetEnabled(state);
}

void
TButtonBar::CanDefault(bool state)
{
	fDefaultsBtn->SetEnabled(state);
}

void
TButtonBar::DisableControls()
{
	fRevertBtn->SetEnabled(false);
	fDefaultsBtn->SetEnabled(false);
}

//*********************************************************************

TBox::TBox(BRect frame, const char *name, const char* label,
			uint32 resizeMask, uint32 flags, border_style style)
	: BBox(frame, name, resizeMask, flags, style)
{
	SetLabel(label);
}

TBox::~TBox()
{
}

void
TBox::WindowActivated(bool state)
{
	BBox::WindowActivated(state);
	
	//	redraw and refresh things like the focus mark
	if (IsFocus())
		Invalidate();
}

#define L_EDGE	5
#define WHITE_SPACE_PAD	5

void 
TBox::Draw(BRect updateRect)
{
	SetFont(be_bold_font);
	BBox::Draw(updateRect);

	PushState();

	BPoint penLoc = Bounds().LeftTop();
	penLoc.x += L_EDGE + WHITE_SPACE_PAD;
	penLoc.y += FontHeight(this, true);
	BPoint endLoc = penLoc;
	endLoc.x += StringWidth(Label());
	if (IsFocus() && Window()->IsActive())
		SetHighColor(keyboard_navigation_color());
	else
		SetHighColor(ViewColor());
			
	SetLowColor(ViewColor());
	StrokeLine(penLoc, endLoc);

	PopState();
}

void 
TBox::MakeFocus(bool state)
{
	BBox::MakeFocus(state);
	Invalidate();
}
