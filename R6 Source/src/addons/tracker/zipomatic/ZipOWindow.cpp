
#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Debug.h>
#include <Path.h>
#include <Region.h>
#include <String.h>
#include <StringView.h>

#include <interface_misc.h>

#include "ZipOWindow.h"
#include "ZipOMatic.h"
#include "BarberPoleView.h"
#include "Bitmaps.h"

const BPoint kButtonSize(70, 20);
const uint32 kStop = 'stop';
const uint32 kTextHeight = 18;

const unsigned char *bitmaps[] = {
	kBitmapBits1,
	kBitmapBits2,
	kBitmapBits3,
	kBitmapBits4,
	kBitmapBits5,
	kBitmapBits6,
	kBitmapBits7
};

ZipOView::ZipOView(BRect frame, bool oneShotOnly)
	:	BBox(frame, "", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED, B_PLAIN_BORDER),
		engine(),
		shouldQuitWhenDone(oneShotOnly)
{
	BRect rect(0, 0, kBitmapWidth - 1, kBitmapHeight - 1);
	rect.InsetBy(-2, -2);
	rect.OffsetTo(15, 13);
	barberPole = new BarberPoleView(rect, "barberPole", bitmaps, 7, kDropFilesBitmapBits);
	AddChild(barberPole);

	rect.OffsetBy(0, 15);
	rect.bottom = rect.top + kTextHeight;
	titleText = new BStringView(rect, "titleText", "");
	AddChild(titleText);
	rect.OffsetBy(0, 18);
	progressText = new FlickerFreeStringView(rect, "progressText", "");
	AddChild(progressText);

	rect.OffsetBy(0, 25);
	AddChild(new SeparatorLine(rect.LeftTop(), rect.Width(), false));
	BRect bounds(Bounds());
	rect = bounds;
	rect.InsetBy(12, 12);
	rect.SetLeftTop(rect.RightBottom() - kButtonSize);
	stop = new BButton(rect, "Stop", "Stop", new BMessage(kStop));
	AddChild(stop);

	Done();
}

ZipOView::~ZipOView()
{
}

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

void 
ZipOView::Pulse()
{
	if (engine.GetCurrentStatus(stateString, progressString)) {
		BString newLabel(stateString);
		float width = progressText->Frame().Width();
		width -= progressText->StringWidth(stateString.String());
		if (width > 0) {
			BString tmp;
			TruncToWidth(tmp, progressString.String(), this, width,
				B_TRUNCATE_BEGINNING);
			
			newLabel += tmp;
		}
		progressText->SetText(newLabel.String());
	}
}


void 
ZipOView::AttachedToWindow()
{
	stop->SetTarget(this);
	engine.SetOwner(this);
		// have to set up ownership here because we are using
		// a BMessenger and we cannot build one in the constructor
}

void 
ZipOView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
			if (engine.State() == ZipOEngine::kReady
				|| engine.State() == ZipOEngine::kDone
				|| engine.State() == ZipOEngine::kError
				|| engine.State() == ZipOEngine::kStopped) 
				Start(message);

			break;

		case kStop:
			StopIfNeeded();
			break;

		case kDoneOK:
			Done();
			if (shouldQuitWhenDone)
				Window()->PostMessage(B_QUIT_REQUESTED);
			break;

		case kDoneError:
			Done();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}

bool 
ZipOView::StopIfNeeded()
{
	if (engine.State() != ZipOEngine::kBusy && engine.State() != ZipOEngine::kPaused)
		return true;

	engine.Pause();
	barberPole->SetPaused();
	if (engine.AskToStop()) {
		Stop();
		return true;
	}
	engine.Resume();
	barberPole->SetProgressing();
	return false;
}


void 
ZipOView::Stop()
{
	barberPole->SetWaitingForDrop();
	engine.Stop();
	Done();
}

void 
ZipOView::Done()
{
	barberPole->SetWaitingForDrop();
	stop->SetEnabled(false);
	titleText->SetText("");
}


void 
ZipOView::Start(BMessage *message)
{
	progressText->SetText("");

	barberPole->SetProgressing();
	stop->SetEnabled(true);
	engine.SetTo(message);
	engine.Start();

	BString text;
	text << "Creating archive: " << engine.ArchivingAs();
	titleText->SetText(text.String());
}


BRect kZipORect(0, 0, 230, 110);
ZipOWindow::ZipOWindow(BPoint offset, bool oneShotOnly)
	:	BWindow(kZipORect.OffsetToSelf(offset), "Zip-O-Matic", B_TITLED_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_NOT_RESIZABLE),
		quitting(true)
{
	SetPulseRate(200000);
	panel = new ZipOView(Bounds(), oneShotOnly);
	AddChild(panel);
}

bool 
ZipOWindow::QuitRequested()
{
	if (!panel->StopIfNeeded()
		&& (panel->Engine()->State() == ZipOEngine::kBusy
			|| panel->Engine()->State() == ZipOEngine::kPaused))
			// user canceled quit
		return false;

	quitting = true;
	be_app->PostMessage(kWindowClosed);
	return true;
}

void 
ZipOWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
			PostMessage(message, panel);
			break;				
		default:
			_inherited::MessageReceived(message);
	}
}

OffscreenBitmap::OffscreenBitmap(BRect frame)
	:	bitmap(0)
{
	NewBitmap(frame);
}

OffscreenBitmap::OffscreenBitmap()
	:	bitmap(0)
{
}

OffscreenBitmap::~OffscreenBitmap()
{
	delete bitmap;
}

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

BView *
OffscreenBitmap::BeginUsing(BRect frame)
{
	if (!bitmap || bitmap->Bounds() != frame)
		NewBitmap(frame);
	bitmap->Lock();
	return View();
}

void
OffscreenBitmap::DoneUsing()
{
	bitmap->Unlock();
}

BBitmap *
OffscreenBitmap::Bitmap() const
{
	ASSERT(bitmap);
	ASSERT(bitmap->IsLocked());
	return bitmap;
}

BView *
OffscreenBitmap::View() const
{
	ASSERT(bitmap);
	return bitmap->ChildAt(0);
}

FlickerFreeStringView::FlickerFreeStringView(BRect bounds, const char *name,
	const char *text, uint32 resizeFlags, uint32 flags)
	:	BStringView(bounds, name, text, resizeFlags, flags),
		bitmap(0)
{
}

FlickerFreeStringView::~FlickerFreeStringView()
{
	delete bitmap;
}

void 
FlickerFreeStringView::Draw(BRect frame)
{
	Draw(frame, false);
}


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

void 
FlickerFreeStringView::SetViewColor(rgb_color color)
{
	if (viewColor != color) {
		viewColor = color;
		Invalidate();
	}
	BStringView::SetViewColor(B_TRANSPARENT_32_BIT);
}

void 
FlickerFreeStringView::SetLowColor(rgb_color color)
{
	if (lowColor != color) {
		lowColor = color;
		Invalidate();
	}
	BStringView::SetLowColor(B_TRANSPARENT_32_BIT);
}

static BRect
LineBounds(BPoint where, float length, bool vertical)
{
	BRect result;
	result.SetLeftTop(where);
	result.SetRightBottom(where + BPoint(2, 2));
	if (vertical)
		result.bottom = result.top + length;
	else
		result.right = result.left + length;
	
	return result;
}

SeparatorLine::SeparatorLine(BPoint where, float length, bool vertical, const char *name)
	: 	BView(LineBounds(where, length, vertical), name,
			B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

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

