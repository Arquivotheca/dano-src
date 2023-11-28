/*
		MenuWindow.cpp

*/

#include <Autolock.h>
#include <Debug.h>
#include <MenuItem.h>
#include <Menu.h>
#include <Message.h>
#include <Picture.h>
#include <Region.h>
#include <View.h>
#include <interface_misc.h>

#include "MenuPrivate.h"
#include "MenuWindow.h"

const window_feel _PRIVATE_MENU_WINDOW_FEEL_ = (window_feel)1025;

BMenuWindow::BMenuWindow(const char *title, bool snakeMenu)
	:	BWindow(BRect(0,0,150,18), title ? title : "_unnamed_menu_wd_", 
			snakeMenu ? B_NO_BORDER_WINDOW_LOOK : B_BORDERED_WINDOW_LOOK,
			_PRIVATE_MENU_WINDOW_FEEL_,
			B_NOT_RESIZABLE | B_NOT_MOVABLE | B_NOT_CLOSABLE),
		fMenu(NULL),
		fUp(NULL),
		fDown(NULL)
{
	fFrame = new BMenuFrame(this, snakeMenu);
}


BMenuWindow::~BMenuWindow()
{
	if (fMenu)
		fMenu->RemoveSelf();
}


void BMenuWindow::SetMenu(BMenu *menu)
{
	fMenu = menu;
	if (!fMenu)
		return;
	
	BFont font;
	fMenu->GetFont(&font);
	fFrame->SetFont(&font);

	font_height	fh;
	font.GetHeight(&fh);
	fFrame->fEmptyH = ceil(fh.ascent + fh.descent + fh.leading);
}


void BMenuWindow::RemoveScrollerViews()
{
	BView *parent = ChildAt(0);
	ASSERT(parent);

	if (fUp) {
		parent->RemoveChild(fUp);
		delete fUp;
		fUp = NULL;
	}
		
	if (fDown) {
		parent->RemoveChild(fDown);
		delete fDown;
		fDown = NULL;
	}
}


void BMenuWindow::AddScrollerViews()
{
	BRect bounds(Bounds());

	BRect rect(bounds);
	if (fFrame->fSnakeMenu)
		rect.top = kSnakeGap;
	rect.bottom = rect.top + SCRL_HEIGHT;
	
	fUp = new BMenuScroller(rect, true);
	if (fMenu) {
		//fUp->SetViewColor(fMenu->ViewColor());
		fUp->SetLowColor(fMenu->LowColor());
		fUp->SetHighColor(fMenu->HighColor());
	}
	
	BView *parent = ChildAt(0);
	ASSERT(parent);
	parent->AddChild(fUp);

	rect = bounds;
	if (fFrame->fSnakeMenu)
		rect.bottom -= kSnakeGap;
	rect.top = rect.bottom - SCRL_HEIGHT;
	fDown = new BMenuScroller(rect, false);
	if (fMenu) {
		//fDown->SetViewColor(fMenu->ViewColor());
		fDown->SetLowColor(fMenu->LowColor());
		fDown->SetHighColor(fMenu->HighColor());
	}
	
	parent->AddChild(fDown);

	UpdateScrollers();
}


void BMenuWindow::UpdateScrollers()
{
	if (!fMenu)
		return;

	BRect bounds(fMenu->Bounds());

	if (fUp)
		fUp->Enable(bounds.LeftTop() != B_ORIGIN);
	
	if (fDown) {
		BMenuItem *last = fMenu->ItemAt(fMenu->CountItems() - 1);
		BRect ib = last->Frame();
		fDown->Enable(ib.bottom > bounds.bottom);
	}
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

BMenuFrame::BMenuFrame(BMenuWindow *window, bool snakeMenu)
	:	BView(BRect(0, 0, 1, 1), NULL, B_FOLLOW_ALL,
			(snakeMenu ? B_DRAW_ON_CHILDREN : 0) | B_FRAME_EVENTS|B_WILL_DRAW),
		fWindow(window),
		fSnakeMenu(snakeMenu),
		fLastWidth(0), fLastHeight(0)
{
	BRect bounds(window->Bounds());
	ResizeTo(bounds.Width(), bounds.Height());
	BAutolock lock(window);
	ASSERT(lock.IsLocked());
	window->AddChild(this);

	if (fSnakeMenu)
		Clip();
}

void 
BMenuFrame::WindowActivated(bool)
{
	if (fSnakeMenu)
		Clip();
}

void 
BMenuFrame::FrameResized(float new_width, float new_height)
{
	if (fSnakeMenu) {
		bool changed = false;
		if (new_width != fLastWidth) {
			Invalidate(BRect(fLastWidth-4, 0, fLastWidth, fLastHeight));
			changed = true;
		}
		if (new_height != fLastHeight) {
			Invalidate(BRect(0, fLastHeight-4, fLastWidth, fLastHeight));
			fLastWidth = new_width;
			changed = true;
		}
		if (changed) {
			fLastWidth = new_width;
			fLastHeight = new_height;
			Clip();
		}
	}
}

void
BMenuFrame::Clip()
{
	ASSERT(fSnakeMenu);
	ASSERT(fWindow);

	BRect rect(fWindow->Bounds());
	rect.InsetBy(kSnakeGap, kSnakeGap);

	BPicture clipPicture;
	BeginPicture(&clipPicture);
	FillRoundRect(rect, 3, 3);

	if (fWindow->fMenu) 
		fWindow->fMenu->ClipBackgroundSnake(this);

	EndPicture();
	fWindow->ClipWindowToPicture(&clipPicture, BPoint(0, 0), 0);
}

void 
BMenuFrame::Draw(BRect updateRect, bool clipDrawing, bool clipWindowShape)
{
	BRect bounds(Bounds());
	
	const rgb_color background	= fWindow->fMenu
								? fWindow->fMenu->ViewColor()
								: ui_color(B_MENU_BACKGROUND_COLOR);
	
	if (fSnakeMenu) {
		if (clipWindowShape) {
			Clip();
		}
	
		BRegion region;
		if (clipDrawing) {
			updateRect.left = 0;
			region.Set(updateRect);
			ConstrainClippingRegion(&region);
		}
	
		// draw black frame around menu
		BRect rect(bounds);
		rect.InsetBy(kSnakeGap, kSnakeGap);
		StrokeRoundRect(rect, 3, 3);
	
		// draw light shadow around left top
		rect.InsetBy(1, 1);
		BeginLineArray(3);
		rgb_color shine = background.blend_copy(ui_color(B_SHINE_COLOR), 186);
		if (!fWindow->UpScroller()) {
			AddLine(BPoint(rect.left + 1, rect.top), BPoint(rect.right - 1, rect.top), shine);
			AddLine(BPoint(rect.left, rect.top + 1), BPoint(rect.left, rect.bottom - 1), shine);
			// cheap trick to draw rounded corner
			AddLine(BPoint(rect.left + 2, rect.top), BPoint(rect.left, rect.top + 2), shine);
		} else {
			BRect f(fWindow->UpScroller()->Frame());
			AddLine(BPoint(rect.left, f.bottom + 1), BPoint(rect.left, rect.bottom - 1), shine);
		}
		
		EndLineArray();
	
		if (fWindow && fWindow->fMenu)
			fWindow->fMenu->DrawBackgroundSnake(this);
	} else {
		SetHighColor(tint_color(background, cLIGHTEN_2));
		StrokeLine(bounds.LeftBottom(), bounds.LeftTop());
		StrokeLine(bounds.RightTop());
	
		SetHighColor(tint_color(background, cDARKEN_2));
		StrokeLine(bounds.RightBottom());
		StrokeLine(bounds.LeftBottom());
	
		SetHighColor(background);
		StrokeLine(bounds.LeftBottom(), bounds.LeftBottom());
	}
	if (fWindow && fWindow->fMenu && fWindow->fMenu->CountItems() == 0) {
		MovePenTo(12, fEmptyH - 1);
		if (fSnakeMenu)
			MovePenBy(kSnakeGap, kSnakeGap);
		SetHighColor(tint_color(background, DISABLED_C));
		SetLowColor(background);
		DrawString(kEmptyMenuString);
	}
}

void 
BMenuFrame::Draw(BRect updateRect)
{
	if (!fSnakeMenu)
		Draw(updateRect, false, false);
}

void 
BMenuFrame::DrawAfterChildren(BRect updateRect)
{
	if (fSnakeMenu)
		Draw(updateRect, false, false);
}

void BMenuFrame::AttachedToWindow()
{
	SetViewUIColor(B_UI_MENU_BACKGROUND_COLOR);
}

BMenuScroller::BMenuScroller(BRect frame, bool up)
	: BView (frame, NULL, B_FOLLOW_LEFT_RIGHT + B_FOLLOW_TOP, B_WILL_DRAW)
{
	fUp = up;
	fEnabled = false;
	SetViewColor(B_TRANSPARENT_COLOR);
}

void BMenuScroller::Draw(BRect)
{
	BRect	bounds(Bounds());
	BRect	r(bounds);
	BPoint	p1, p2, p3;

	const rgb_color background = LowColor();
	const rgb_color text = HighColor();
	
	if (fUp) {
		// up scroller - at top of menu - drawing separator at bottom
		p1 = bounds.LeftBottom();
		p2 = bounds.RightBottom();
		r.bottom -= 1;
	} else {
		// down scroller - at bottom of menu - drawing separator at top
		p1 = bounds.LeftTop();
		p2 = bounds.RightTop();
		r.top += 1;
	}

	SetHighColor(tint_color(background, cDARKEN_1));
	FillRect(r);

	SetHighColor(tint_color(background, cDARKEN_2));
	StrokeLine(p1, p2);

	float mid = (bounds.right - bounds.left) / 2;
	p1.x = mid - 5;

	p2.x = mid + 5;

	p3.x = mid;

	if (fUp) {
		p1.y = 7;
		p2.y = 7;
		p3.y = 2;
	} else {
		p1.y = 3;
		p2.y = 3;
		p3.y = 8;
	}

	if (fEnabled)
		SetHighColor(text);
	else
		SetHighColor(tint_color(background, cDARKEN_2));

	FillTriangle(p1,p2,p3);
}


void BMenuScroller::AttachedToWindow()
{
	//SetViewUIColor(B_UI_MENU_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_MENU_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_MENU_ITEM_TEXT_COLOR);
}



void BMenuScroller::Enable(bool enable)
{
	bool prev = fEnabled;

	fEnabled = enable;

	if ((prev != enable) && Window())
		Invalidate();
		//Draw(BRect(0,0,0,0));
}
