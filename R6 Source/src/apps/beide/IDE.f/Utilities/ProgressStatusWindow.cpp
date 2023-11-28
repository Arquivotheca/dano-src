// ---------------------------------------------------------------------------
/*
	ProgressStatusWindow.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
				
	Pieces liberally taken from Zip-o-Matic by Pavel Cisler

*/
// ---------------------------------------------------------------------------

#include "ProgressStatusWindow.h"
#include "BarberPoleView.h"
#include "MThread.h"
#include "IDEConstants.h"
#include "Utils.h"

#include <Application.h>
#include <Button.h>
#include <String.h>
#include <StringView.h>
#include <Region.h>
#include <interface_misc.h>
#include <Autolock.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
// Message and String constants
// ---------------------------------------------------------------------------

const uint32 kStopWorker = 'Stop';
const uint32 kTextHeight = 18;
const BPoint kButtonSize(70, 20);

// ---------------------------------------------------------------------------
// ProgressStatusWindow member functions
// ---------------------------------------------------------------------------

ProgressStatusWindow::ProgressStatusWindow(BPoint offset,
										   const char* taskTitle,
										   MThread* workerThread)
					 : BWindow(BRect(0, 0, 230, 110).OffsetToSelf(offset), 
					 		   "BeIDE Status",
					 		   B_FLOATING_WINDOW_LOOK,
					 		   B_NORMAL_WINDOW_FEEL, 
					 		   B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
					   fWorkerThread(workerThread)
{
	BWindow::SetPulseRate(200000);
	fPanel = new ProgressStatusView(Bounds(), taskTitle);
	AddChild(fPanel);
}

// ---------------------------------------------------------------------------

ProgressStatusWindow::~ProgressStatusWindow()
{
}

// ---------------------------------------------------------------------------

void
ProgressStatusWindow::TaskStarted()
{
	BAutolock lock(this);
	this->Show();
	fPanel->Start();
}

// ---------------------------------------------------------------------------

void
ProgressStatusWindow::StatusUpdate(const char* status)
{
	BAutolock lock(this);
	fPanel->SetStatus(status);
}

// ---------------------------------------------------------------------------

void 
ProgressStatusWindow::TaskDone()
{
	// before we do anything, clear our worker thread
	// so that a cancel coming in doesn't call into deleted space
	// hide the window and stop any progress indicator
	
	if (this->Lock()) {
		fWorkerThread = NULL;
		fPanel->Stop();
		this->Hide();
		// unlock happens automatically as window is deleted in quit
		this->Quit();
	}
}

// ---------------------------------------------------------------------------

void
ProgressStatusWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kStopWorker:
			fPanel->Stop();
			fWorkerThread->Cancel();
			break;
			
		default:
			BWindow::MessageReceived(message);
	}
}

// ---------------------------------------------------------------------------
// ProgressStatusView
// ---------------------------------------------------------------------------

const unsigned char *bitmaps[] = {
	kBPBitmapBits1,
	kBPBitmapBits2,
	kBPBitmapBits3,
	kBPBitmapBits4,
	kBPBitmapBits5,
	kBPBitmapBits6,
	kBPBitmapBits7
};

// ---------------------------------------------------------------------------

ProgressStatusView::ProgressStatusView(BRect frame, const char* taskTitle)
				   : BBox(frame, "", B_FOLLOW_ALL, B_WILL_DRAW, B_PLAIN_BORDER)
{
	BRect rect(0, 0, kBarberPoleBitmapWidth - 1, kBarberPoleBitmapHeight - 1);
	rect.InsetBy(-2, -2);
	rect.OffsetTo(15, 13);
	fBarberPole = new BarberPoleView(rect, "barberPole", bitmaps, 7, kBPDropFilesBitmapBits);
	AddChild(fBarberPole);

	rect.OffsetBy(0, 15);
	rect.bottom = rect.top + kTextHeight;
	fTitleText = new BStringView(rect, "titleText", taskTitle);
	AddChild(fTitleText);
	rect.OffsetBy(0, 18);
	fProgressText = new FlickerFreeStringView(rect, "progressText", "");
	AddChild(fProgressText);

	rect.OffsetBy(0, 25);
	AddChild(new SeparatorLine(rect.LeftTop(), rect.Width(), false));
	BRect bounds(Bounds());
	rect = bounds;
	rect.InsetBy(12, 12);
	rect.SetLeftTop(rect.RightBottom() - kButtonSize);
	fStopButton = new BButton(rect, "Stop", "Stop", new BMessage(kStopWorker));
	AddChild(fStopButton);

	ProgressStatusView::Stop();
}

// ---------------------------------------------------------------------------

ProgressStatusView::~ProgressStatusView()
{
}

// ---------------------------------------------------------------------------

static void
TruncToWidth(BString &result, const char *source, const BView *view,
	float maxWidth, uint32 mode)
{
	int32 length = strlen(source);
	if (length) {
		char *tmp = result.LockBuffer(length + 3);
		BFont font;
		const_cast<BView *>(view)->GetFont(&font);
		font.GetTruncatedStrings(&source, 1, mode, maxWidth, &tmp);
		result.UnlockBuffer();		
	} else
		result = "";
}

// ---------------------------------------------------------------------------

void 
ProgressStatusView::SetStatus(const char* statusString)
{
	if (statusString) {
		BString newStatus;
		float width = fProgressText->Frame().Width();
		TruncToWidth(newStatus, statusString, this, width, B_TRUNCATE_END);
		fProgressText->SetText(newStatus.String());
	}
}

// ---------------------------------------------------------------------------

void 
ProgressStatusView::AttachedToWindow()
{
	fStopButton->SetTarget(this->Window());
}

// ---------------------------------------------------------------------------

void 
ProgressStatusView::Stop()
{
	fBarberPole->SetPaused();
	fStopButton->SetEnabled(false);
	this->SetStatus("");
}

// ---------------------------------------------------------------------------

void 
ProgressStatusView::Start()
{
	this->SetStatus("");
	fBarberPole->SetProgressing();
	fStopButton->SetEnabled(true);
}

// ---------------------------------------------------------------------------
// OffscreenBitmap helper class
// ---------------------------------------------------------------------------

OffscreenBitmap::OffscreenBitmap(BRect frame)
	:	bitmap(0)
{
	NewBitmap(frame);
}

// ---------------------------------------------------------------------------

OffscreenBitmap::OffscreenBitmap()
	:	bitmap(0)
{
}

// ---------------------------------------------------------------------------

OffscreenBitmap::~OffscreenBitmap()
{
	delete bitmap;
}

// ---------------------------------------------------------------------------

void
OffscreenBitmap::NewBitmap(BRect bounds)
{
	delete bitmap;
	bitmap = new BBitmap(bounds, B_COLOR_8_BIT, true);
	if (bitmap->Lock()) {
		BView *view = new BView(bitmap->Bounds(), "", B_FOLLOW_NONE, 0);
		bitmap->AddChild(view);

		BRect clipRect = view->Bounds();
		BRegion newClip;
		newClip.Set(clipRect);
		view->ConstrainClippingRegion(&newClip);

		bitmap->Unlock();
	} else {
		delete bitmap;
		bitmap = NULL;
	}
}

// ---------------------------------------------------------------------------

BView *
OffscreenBitmap::BeginUsing(BRect frame)
{
	if (!bitmap || bitmap->Bounds() != frame)
		NewBitmap(frame);
	bitmap->Lock();
	return View();
}

// ---------------------------------------------------------------------------

void
OffscreenBitmap::DoneUsing()
{
	bitmap->Unlock();
}

// ---------------------------------------------------------------------------

BBitmap *
OffscreenBitmap::Bitmap() const
{
	ASSERT(bitmap);
	ASSERT(bitmap->IsLocked());
	return bitmap;
}

// ---------------------------------------------------------------------------

BView *
OffscreenBitmap::View() const
{
	ASSERT(bitmap);
	return bitmap->ChildAt(0);
}

// ---------------------------------------------------------------------------
// FlickerFreeStringView helper class
// ---------------------------------------------------------------------------

FlickerFreeStringView::FlickerFreeStringView(BRect bounds, const char *name,
	const char *text, uint32 resizeFlags, uint32 flags)
	:	BStringView(bounds, name, text, resizeFlags, flags),
		bitmap(0)
{
}

// ---------------------------------------------------------------------------

FlickerFreeStringView::~FlickerFreeStringView()
{
	delete bitmap;
}

// ---------------------------------------------------------------------------

void 
FlickerFreeStringView::Draw(BRect frame)
{
	Draw(frame, false);
}

// ---------------------------------------------------------------------------

void 
FlickerFreeStringView::Draw(BRect,  bool direct)
{
	BRect bounds(Bounds());
	if (!direct && !bitmap)
		bitmap = new OffscreenBitmap(Bounds());
	
	BView *blitView = 0;
	
	if (!direct)
		blitView = bitmap->BeginUsing(bounds);
	else
		blitView = this;

	if (Parent()) {
		viewColor = Parent()->ViewColor();
		lowColor = Parent()->ViewColor();
	}


	BFont font;
	GetFont(&font);

	if (!direct) {
		blitView->SetViewColor(viewColor);
		blitView->SetHighColor(HighColor());
		blitView->SetLowColor(lowColor);

		blitView->SetFont(&font);
	
		blitView->Sync();
	}
	blitView->FillRect(bounds, B_SOLID_LOW);


	if (Text()) {
		BPoint loc;

		font_height	height;
		GetFontHeight(&height);

		edge_info eInfo;
		switch (Alignment()) {
			case B_ALIGN_LEFT:
				{
					// If the first char has a negative left edge give it
					// some more room by shifting that much more to the right.
					font.GetEdges(Text(), 1, &eInfo);
					loc.x = bounds.left + (2 - eInfo.left);
					break;
				}
			case B_ALIGN_CENTER:
				{
					float width = StringWidth(Text());
					float center = (bounds.right - bounds.left) / 2;
					loc.x = center - (width/2);
					break;
				}
			case B_ALIGN_RIGHT:
				{
					float width = StringWidth(Text());
					loc.x = bounds.right - width - 2;
					break;
				}
		}
		loc.y = bounds.bottom - (1 + height.descent);
		blitView->MovePenTo(loc);
		blitView->DrawString(Text());
	}

	if (!direct) {
		blitView->Sync();
		SetDrawingMode(B_OP_COPY);
		DrawBitmap(bitmap->Bitmap());
		bitmap->DoneUsing();
	}
}

// ---------------------------------------------------------------------------

void 
FlickerFreeStringView::AttachedToWindow()
{
	BStringView::AttachedToWindow();
	if (Parent()) {
		viewColor = Parent()->ViewColor();
		lowColor = Parent()->ViewColor();
	}
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetLowColor(B_TRANSPARENT_32_BIT);
}

// ---------------------------------------------------------------------------

void 
FlickerFreeStringView::SetViewColor(rgb_color color)
{
	if (viewColor != color) {
		viewColor = color;
		Invalidate();
	}
	BStringView::SetViewColor(B_TRANSPARENT_32_BIT);
}

// ---------------------------------------------------------------------------

void 
FlickerFreeStringView::SetLowColor(rgb_color color)
{
	if (lowColor != color) {
		lowColor = color;
		Invalidate();
	}
	BStringView::SetLowColor(B_TRANSPARENT_32_BIT);
}

// ---------------------------------------------------------------------------

static BRect
LineBounds(BPoint where, float length, bool vertical)
{
	BRect result;
	result.SetLeftTop(where);
	result.SetRightBottom(where + BPoint(1, 1));
	if (vertical)
		result.bottom = result.top + length;
	else
		result.right = result.left + length;
	
	return result;
}

// ---------------------------------------------------------------------------
// SeparatorLine helper class
// ---------------------------------------------------------------------------

SeparatorLine::SeparatorLine(BPoint where, float length, bool vertical, const char *name)
	: 	BView(LineBounds(where, length, vertical), name,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	SetGrey(this, kLtGray);
}

// ---------------------------------------------------------------------------

const rgb_color kWhite = {255, 255, 255, 255};

void
SeparatorLine::Draw(BRect)
{
	BRect bounds(Bounds());
	rgb_color hiliteColor = shift_color(ViewColor(), 1.5);

	bool vertical = (bounds.left > bounds.right - 3);
	BeginLineArray(2);
	if (vertical) {
		AddLine(bounds.LeftTop(), bounds.LeftBottom(), hiliteColor);
		AddLine(bounds.LeftTop() + BPoint(1, 0), bounds.LeftBottom() + BPoint(1, 0), kWhite);
	} else {
		AddLine(bounds.LeftTop(), bounds.RightTop(), hiliteColor);
		AddLine(bounds.LeftTop() + BPoint(0, 1), bounds.RightTop() + BPoint(0, 1), kWhite);
	}
	EndLineArray();
}

