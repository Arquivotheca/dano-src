
#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Debug.h>
#include <Region.h>
#include <String.h>
#include <ScrollView.h>
#include <TextView.h>
#include <Window.h>

#include <ctype.h>
#include <stdio.h>

#include "HexDumpEditor.h"

const int32 kMargin = 3;
const int32 kLineSpacing = 0;
const uint32 kBytesPerLine = 16;
const int32 kHexDigitsLeftMargin = 20;
const int32 kAsciiDigitsLeftMargin = 20;
const int32 kSelectionExtraMargin = 1;

const bigtime_t kCursorBlinkTime = 500000;

static inline void mix_color_func(rgb_color* target, const rgb_color other, uint8 amount)
{
	target->red = (uint8)( ((int16(other.red)-int16(target->red))*amount)/255
								+ target->red );
	target->green = (uint8)( ((int16(other.green)-int16(target->green))*amount)/255
								+ target->green );
	target->blue = (uint8)( ((int16(other.blue)-int16(target->blue))*amount)/255
								+ target->blue );
	target->alpha = (uint8)( ((int16(other.alpha)-int16(target->alpha))*amount)/255
								+ target->alpha );
}

static rgb_color my_mix_color(rgb_color color1, rgb_color color2, uint8 amount)
{
	mix_color_func(&color1, color2, amount);
	return color1;
}


// offscreen bitmap utility class

OffscreenBitmap::OffscreenBitmap()
	:	bitmap(NULL),
		target(NULL),
		destinationFrame(0, 0, 0, 0)
{
}


OffscreenBitmap::OffscreenBitmap(BView *target, BRect frame)
{
	SetToCommon(target, frame);
}

OffscreenBitmap::~OffscreenBitmap()
{
	delete bitmap;
}

BView * 
OffscreenBitmap::BeginBlitting(BView *newTarget, BRect frame,
	bool copyTargetViewState, bool erase)
{
	if (!bitmap
		|| target != newTarget
		|| frame.OffsetToCopy(0, 0) != destinationFrame.OffsetToCopy(0, 0)) {
		// different view or different bitmap size, just allocate a new bitmap
		delete bitmap;
		SetToCommon(newTarget, frame);
	} else {
		// same bitmap, just make sure it's locked and the origin is
		// set up right
		if (!bitmap->IsLocked() && !bitmap->Lock())
			throw (status_t)B_ERROR;
		destinationFrame = frame;
		if (copyTargetViewState)
			SetUpState(newTarget, frame);
		else
			OffscreenView()->SetOrigin(-frame.left, -frame.top);

		if (erase) {
			if (!copyTargetViewState) {
				// We need to at least copy the low color.
				OffscreenView()->SetLowColor(newTarget->LowColor());
			}
			OffscreenView()->FillRect(frame, B_SOLID_LOW);
		}
	}
	return OffscreenView();
}

void 
OffscreenBitmap::SetToCommon(BView *newTarget, BRect frame)
{
	target = newTarget;
	destinationFrame = frame;

	BRect bounds(frame);
	bounds.OffsetTo(0, 0);
	bitmap = new BBitmap(bounds, B_COLOR_8_BIT, true);
	if (!bitmap->Lock())
		throw (status_t)B_ERROR;

	BView *view = new BView(bitmap->Bounds(), "", B_FOLLOW_NONE, 0);
	bitmap->AddChild(view);
	SetUpState(newTarget, frame);
}

void 
OffscreenBitmap::SetUpState(BView *newTarget, BRect frame)
{
	BView *view = bitmap->ChildAt(0);

	// make sure the bitmap matches the target view drawing modes
	view->SetViewColor(newTarget->ViewColor());
	view->SetLowColor(newTarget->LowColor());
	view->SetHighColor(newTarget->HighColor());
	view->SetDrawingMode(newTarget->DrawingMode());
	BFont font;
	newTarget->GetFont(&font);
	view->SetFont(&font);

	view->SetOrigin(-frame.left, -frame.top);

#if 0
	BRegion newClip;
	newClip.Set(frame);
	view->ConstrainClippingRegion(&newClip);
#endif
}


BView *
OffscreenBitmap::OffscreenView() const
{
	ASSERT(bitmap);
	return bitmap->ChildAt(0);
}

void 
OffscreenBitmap::DoneBlitting()
{
	// bitmap is composed, output it to the target view
	bitmap->ChildAt(0)->Sync();
	target->SetDrawingMode(B_OP_COPY);
	target->DrawBitmap(bitmap, bitmap->Bounds(), destinationFrame);
	bitmap->Unlock();
}

// #pragma mark -

// addon proxy code

float 
HexDumpAddon::QuickQuality(const BResourceItem *) const
{
	// we can do pretty much anything
	
	return 0.01;
}

float 
HexDumpAddon::PreciseQuality(const BResourceItem *item) const
{
	return QuickQuality(item);
}

BMiniItemEditor *
HexDumpAddon::MakeMiniEditor(const BResourceAddonArgs &, BResourceHandle,
	const BMessage *)
{
	// no mini editor yet
	return NULL;
}

BFullItemEditor *
HexDumpAddon::MakeFullEditor(const BResourceAddonArgs &args, BResourceHandle handle,
	const BMessage *)
{
	return new HexDumpFullEditor(args, handle);
}

status_t 
HexDumpAddon::HandleDrop(const BMessage *)
{
	return B_ERROR;
}

// #pragma mark -

const uint32 kScrollViewMargin = 2;

HexDumpFullEditor::HexDumpFullEditor(const BResourceAddonArgs &args,
	BResourceHandle primaryItem)
	:	BFullItemEditor(args, primaryItem),
		BView(BRect(0, 0, 50, 50), "HexEditor", B_FOLLOW_ALL_SIDES, B_NAVIGABLE_JUMP)
{
	SetChangeTarget(this);
	
	BRect frame(Bounds());
	frame.left += kScrollViewMargin;
	frame.top += kScrollViewMargin;
	frame.right -= B_V_SCROLL_BAR_WIDTH + kScrollViewMargin;
	frame.bottom -= B_H_SCROLL_BAR_HEIGHT + kScrollViewMargin;
	hexDumpView = new HexDumpView(frame, "HexDumpView", this);

	AddChild(new BScrollView("StringScroll", hexDumpView,
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW, true, true, B_FANCY_BORDER));

	const BResourceCollection *collection = ReadLock();
	if (collection) {
		const BResourceItem *item = collection->ReadItem(PrimaryItem());
		if( item->Size() > 0 ) {
			memcpy(data.LockBuffer(item->Size()), item->Data(), item->Size());
			data.UnlockBuffer(item->Size());
		} else {
			data = "";
		}
		ReadUnlock(collection);
	}
}

const void *
HexDumpFullEditor::Data() const
{
	return data.String();
}

size_t 
HexDumpFullEditor::Size() const
{
	return data.Length();
}

void 
HexDumpFullEditor::PickUpDataChange(const BResourceCollection *collection,
	BResourceHandle &item, uint32 changes)
{
	if (item != PrimaryItem() || changes == 0 )
		return;
	
	BAutolock lock(Looper());
	if (!lock.IsLocked())
		return;
	
	// Retrieve resource item, and copy it into the text editor
	// if its data has changed.
	const BResourceItem *resourceItem = collection->ReadItem(PrimaryItem());
	if ((changes & B_RES_DATA_CHANGED) != 0) {
		if (resourceItem->Size() > 0) {
			memcpy(data.LockBuffer(resourceItem->Size()),
				resourceItem->Data(), resourceItem->Size());
			data.UnlockBuffer(resourceItem->Size());
		} else {
			data = "";
		}
		// force update on the hex edit view
		hexDumpView->DataChanged();
	}
}

status_t 
HexDumpFullEditor::Retarget(BResourceHandle handle)
{
	SetPrimaryItem(handle);
	const BResourceCollection *collection = ReadLock();
	if (collection) {
		PickUpDataChange(collection, PrimaryItem(), B_RES_ALL_CHANGED);
		ReadUnlock(collection);
	}
	return B_OK;
}

void 
HexDumpFullEditor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_RESOURCE_DATA_CHANGED:
			{
				const BResourceCollection *collection = ReadLock();
				if (collection) {
					for (;;) {
						uint32 changes;
						BResourceHandle handle = collection->GetNextChange(this, &changes);
						if (!handle.IsValid())
							break;
						PickUpDataChange(collection, handle, changes);
					}
					ReadUnlock(collection);
				}
			}
			break;
		
		default:
			BView::MessageReceived(message);
			break;
	}
}

void 
HexDumpFullEditor::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

void 
HexDumpFullEditor::MakeFocus(bool on)
{
	if( on && hexDumpView ) hexDumpView->MakeFocus(on);
	else BView::MakeFocus(on);
}


void 
HexDumpFullEditor::GetPreferredSize(float *width, float *height)
{
	hexDumpView->GetPreferredSize(*width, *height);
	*width += 2 * kScrollViewMargin + B_V_SCROLL_BAR_WIDTH + 1;
	*height += 2 * kScrollViewMargin + B_H_SCROLL_BAR_HEIGHT + 1;
}

void 
HexDumpFullEditor::SetData(const char *newData, size_t size, bool update)
{
	if (size > 0) {
		memcpy(data.LockBuffer(size), newData, size);
		data.UnlockBuffer(size);
	} else {
		data = "";
	}
	if (update)
		hexDumpView->DataChanged();
}

void 
HexDumpFullEditor::Insert(uint32 offset, const char *newData, size_t size,
	const char *undoName, bool update)
{
	if (offset > (uint32)data.Length() || size == 0)
		return;
	
	// Go through a BString Insert here because char *insert would not
	// deal with data that contains nulls. This is obviously not optimal
	// for large inserts but it should be used at most to insert a pasted
	// clipboard
	BString tmp;
	if (size > 0) {
		memcpy(tmp.LockBuffer(size), newData, size);
		tmp.UnlockBuffer(size);
	}
	
	data.Insert(tmp, offset);

	if (update)
		hexDumpView->DataChanged();

	// Lock resources for writing.  This action is shown
	// to the user as "Edit Text".
	BResourceCollection *collection = WriteLock(undoName);
	
	if (collection) {
		BResourceItem *item = collection->WriteItem(PrimaryItem(), this);
		item->InsertData(offset, newData, size);
		
		// Finished writing data.
		WriteUnlock(collection);
	}
}

void 
HexDumpFullEditor::DeleteRange(uint32 from, uint32 to, const char *undoName, bool update)
{
	if (to > (uint32)data.Length() || to <= from)
		return;

	data.Remove(from, to - from);
	if (update)
		hexDumpView->DataChanged();

	BResourceCollection *collection = WriteLock(undoName);
	
	if (collection) {
		BResourceItem *item = collection->WriteItem(PrimaryItem(), this);
		item->DeleteData(from, to - from);
		
		// Finished writing data.
		WriteUnlock(collection);
	}
}

void 
HexDumpFullEditor::ModifyCharAt(uint32 offset, uchar ch, const char *undoName, bool update)
{
	if (offset >= (uint32)data.Length())
		return;

	data[offset] = ch;
	if (update)
		hexDumpView->DataChanged();

	BResourceCollection *collection = WriteLock(undoName);
	
	if (collection) {
		BResourceItem *item = collection->WriteItem(PrimaryItem(), this);
		item->DeleteData(offset, 1);
		item->InsertData(offset, &ch, 1);
		
		// Finished writing data.
		WriteUnlock(collection);
	}
}

// #pragma mark -

HexDumpView::HexDumpView(BRect frame, const char *name, HexDumpFullEditor *parent)
	:	BView(frame, name, B_FOLLOW_ALL,
			B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS | B_NAVIGABLE),
		parent(parent),
		pulse(NULL),
		hexDigitChunkSize(2),
		selectionStart(0),
		selectionEnd(0),
		trackingMouse(false),
		hexColumnFocused(true),
		needScrollBarTweak(true),
		cursorOn(false),
		expectingSecondHexDigit(false)
{
}

HexDumpView::~HexDumpView()
{
	StopPulse();
}

void 
HexDumpView::UpdateSizes()
{
	// recalculate cached sizes
	
	GetFontHeight(&fontHeight);
	
	fixedCharacterWidth = StringWidth(" ");
	offsetWidth = 8 * fixedCharacterWidth;
	hexRunWidth = (2 * kBytesPerLine
		+ kBytesPerLine / hexDigitChunkSize - 1) * fixedCharacterWidth;
	asciiRunWidth = kBytesPerLine * fixedCharacterWidth;
}

void
HexDumpView::TweakScrollBars(bool force)
{
	if (!needScrollBarTweak && !force)
		return;
	
	const BRect bounds(Bounds());
	float dataWidth, dataHeight;
	GetContentSize(dataWidth, dataHeight);
	
	BScrollBar* sb = ScrollBar(B_VERTICAL);
	if (sb) {
		float visHeight = bounds.Height();
		float prop = visHeight / dataHeight;
		if (dataHeight <= visHeight) {
			dataHeight = 0;
			prop = 1;
		} else {
			dataHeight -= visHeight;
		}
		sb->SetRange(0, dataHeight);
		sb->SetProportion(prop);
		if (dataHeight >= 1) {
			if (visHeight > LineHeight()) visHeight -= LineHeight();
			sb->SetSteps(LineHeight(), visHeight);
		}
	}
	
	sb = ScrollBar(B_HORIZONTAL);
	if (sb) {
		float visWidth = bounds.Width();
		float prop = visWidth / dataWidth;
		if (dataWidth <= visWidth) {
			dataWidth = 0;
			prop = 1;
		} else {
			dataWidth -= visWidth;
		}
		sb->SetRange(0, dataWidth);
		sb->SetProportion(prop);
		if (dataWidth >= 1) {
			if (visWidth > fixedCharacterWidth*2) visWidth -= fixedCharacterWidth*2;
			sb->SetSteps(fixedCharacterWidth*2, visWidth);
		}
	}
	
	needScrollBarTweak = false;
}

void 
HexDumpView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetFont(be_fixed_font);
	UpdateSizes();
	DataChanged();
	TweakScrollBars(true);
	SetViewColor(B_TRANSPARENT_COLOR);
}

void 
HexDumpView::DetachedFromWindow()
{
	BView::AttachedToWindow();
	StopPulse();
}

void 
HexDumpView::GetPreferredSize(float &width, float &height)
{
	GetContentSize(width, height);
	float minPreferedHeight = 2 * kMargin + 16 * LineHeight();
	float maxPreferedHeight = 2 * kMargin + 24 * LineHeight();
	if (height < minPreferedHeight)
		height = minPreferedHeight;
	
	if (height > maxPreferedHeight)
		height = maxPreferedHeight;
}

void 
HexDumpView::GetContentSize(float &width, float &height)
{
	height = 2 * kMargin + ((parent->Size()+kBytesPerLine) / kBytesPerLine)
							* LineHeight();
	width = 2 * kMargin + kHexDigitsLeftMargin + offsetWidth
		+ kAsciiDigitsLeftMargin + hexRunWidth + asciiRunWidth;
}

void 
HexDumpView::DataChanged()
{
//	float width, height;
//	GetContentSize(width, height);
//	ResizeTo(width, height);

	if (selectionStart > parent->Size())
		selectionStart = parent->Size();

	if (selectionEnd > parent->Size())
		selectionEnd = parent->Size();

	needScrollBarTweak = true;
	Invalidate();
}

float 
HexDumpView::LineHeight() const
{
	return ceil(fontHeight.ascent + fontHeight.descent + fontHeight.leading);
}

int32
HexDumpView::VisibleLines() const
{
	// Note -- round down, to only include fully visible lines.
	return int32(Bounds().Height()/LineHeight());
}

int32 
HexDumpView::PointToOffset(BPoint point, bool centerDivider) const
{
	// handles points in the ascii and hex columns separately

	// start with offset based on horizontal location
	int32 result = (int32)((point.y - kMargin) / LineHeight());
	result *= kBytesPerLine;

	int32 vDelta;
	if (InAsciiColumn(point)) {
		// return offset based on click in ascii column
		vDelta = (int32)(point.x - kMargin - kHexDigitsLeftMargin - offsetWidth
			- hexRunWidth - kAsciiDigitsLeftMargin);

		if (centerDivider)
			vDelta += (int32)(fixedCharacterWidth / 2);
			
		vDelta /= (int32)fixedCharacterWidth;
	} else {
		// return offset based on click in hex column
		vDelta = (int32)(point.x - kMargin - kHexDigitsLeftMargin - offsetWidth);
		vDelta /= (int32)fixedCharacterWidth;

		if (centerDivider)
			vDelta++;
	
		// the hex digits are drawn in a group of <hexDigitChunkSize> separated by a space
		int32 chunkAndABit = 2 * hexDigitChunkSize + 1;
		vDelta = (vDelta / chunkAndABit) * hexDigitChunkSize + (vDelta % chunkAndABit) / 2;
	}
	
	// pin the vertical delta to valid values
	if (vDelta < 0)
		vDelta = 0;
	
	if (vDelta > (int32)kBytesPerLine)
		vDelta = kBytesPerLine;

	// add vertical delta to what we already had based on horizontal offset
	result += vDelta;

	// pin the result valid values
	if (result < 0)
		result = 0;
	if (result > (int32)parent->Size())
		result = parent->Size();
		
	return result;
}

BPoint 
HexDumpView::OffsetToHexPoint(int32 offset) const
{
	// converts offset to a point in the hex column

	BPoint result(kMargin + kHexDigitsLeftMargin + offsetWidth,
		kMargin + ceil(fontHeight.ascent));
	result.y += LineHeight() * (offset / kBytesPerLine);
	offset %= kBytesPerLine;
	
	// the hex digits are drawn in a group of <hexDigitChunkSize> separated by a space
	int32 verticalCharacterOffset =  2 * (offset % hexDigitChunkSize);
	verticalCharacterOffset += (offset / hexDigitChunkSize) * (2 * hexDigitChunkSize + 1);
	result.x += verticalCharacterOffset * fixedCharacterWidth;
	
	return result;
}

BPoint 
HexDumpView::OffsetToAsciiPoint(int32 offset) const
{
	// converts offset to a point in the ascii column

	BPoint result(kMargin + kHexDigitsLeftMargin + offsetWidth
		+ hexRunWidth + kAsciiDigitsLeftMargin,
		kMargin + ceil(fontHeight.ascent));

	result.y += LineHeight() * (offset / kBytesPerLine);
	offset %= kBytesPerLine;

	result.x += offset * fixedCharacterWidth;
	
	return result;
}

void
HexDumpView::DoMakeOffsetVisible(int32 offset, bool hexArea)
{
	BPoint where = hexArea
				 ? OffsetToHexPoint(offset)
				 : OffsetToAsciiPoint(offset);
	where.y -= ceil(fontHeight.ascent);
	
	const float charW = fixedCharacterWidth * (hexArea ? 2 : 1);
	
	const BRect b(Bounds());
	BPoint scrollPos(b.LeftTop());
	if (b.top > where.y) {
		scrollPos.y += (where.y-b.top);
	} else if (b.bottom < (where.y+LineHeight())) {
		scrollPos.y += (where.y+LineHeight()-b.bottom);
	}
	if (b.left > where.x) {
		scrollPos.x += (where.x-b.left);
	} else if (b.right < (where.x+charW)) {
		scrollPos.x += (where.x+charW-b.right);
	}
	
	ScrollTo(scrollPos);
}

bool 
HexDumpView::InHexColumn(BPoint point, bool exactHitOnly) const
{
	if (!exactHitOnly)
		return point.x <= kMargin + offsetWidth + kHexDigitsLeftMargin
			+ hexRunWidth + kMargin;

	// true if point in hex column
	return point.x >= kMargin + offsetWidth + kHexDigitsLeftMargin
		&& point.x <= kMargin + offsetWidth + kHexDigitsLeftMargin + hexRunWidth;
}

bool 
HexDumpView::InAsciiColumn(BPoint point, bool exactHitOnly) const
{
	if (!exactHitOnly)
		return !InHexColumn(point, false);

	// true if point in ascii column
	return point.x >= kMargin + offsetWidth + kHexDigitsLeftMargin
		+ hexRunWidth + kAsciiDigitsLeftMargin
		&& point.x <= kMargin + offsetWidth + kHexDigitsLeftMargin
		+ hexRunWidth + kAsciiDigitsLeftMargin + asciiRunWidth;
}

void 
HexDumpView::Pulse()
{
	// blink the cursor
	if (selectionStart == selectionEnd && system_time() >= nextCursorTime
		&& !trackingMouse) {
		if( IsFocus() ) InvertCursor();
		else HideCursor();
	}
}

void 
HexDumpView::ShowCursor()
{
	if (!cursorOn && selectionStart == selectionEnd) {
		if( IsFocus() ) InvertCursor();
	}
}

void 
HexDumpView::HideCursor()
{
	if (cursorOn)
		InvertCursor();

}

void 
HexDumpView::InvertCursor()
{
	cursorOn = !cursorOn;

	// restart the timer
	nextCursorTime = system_time() + kCursorBlinkTime;

	BPoint where(OffsetToHexPoint(selectionStart));
	if (expectingSecondHexDigit)
		// if inserting a hex digit, place the cursor in the
		// middle of the hex number
		where += BPoint(fixedCharacterWidth, 0);

	// draw the two cursors, one in hex column, one in ascii
	DrawCursorAt(where, !hexColumnFocused);	
	DrawCursorAt(OffsetToAsciiPoint(selectionStart), hexColumnFocused);
}

void 
HexDumpView::DrawCursorAt(BPoint point, bool dimmed)
{
	point.x--;
	if (!dimmed) {
		// full cursor, just invert
		BRect rect(point, point);
		rect.top -= ceil(fontHeight.ascent);
		rect.bottom++;
		InvertRect(rect);
	} else {
		// dimmed cursor, draw and invalidate to turn on and off
		BRect rect(point, point);
		rect.top -= ceil(fontHeight.ascent);
		rect.bottom++;
		if (cursorOn) {
			rgb_color highColor = HighColor();
			rgb_color fg, bg;
			GetViewColors(&fg, &bg, false);
			SetHighColor(my_mix_color(fg, bg, 128));
			FillRect(rect, B_SOLID_HIGH);
			SetHighColor(highColor);
		} else
			FillRect(rect, B_SOLID_LOW);
	}
}

void 
HexDumpView::Draw(BRect updateRect)
{
	DrawRange(PointToOffset(updateRect.LeftTop() - BPoint(0, LineHeight()), false),
		PointToOffset(updateRect.RightBottom() + BPoint(0, LineHeight()), true),
		true, true);
	TweakScrollBars();
}

void 
HexDumpView::DrawRange(int32 fromOffset, int32 toOffset, bool directDraw,
	bool eraseToEnd)
{
	// clip offsets to line start/end
	fromOffset = (fromOffset / kBytesPerLine) * kBytesPerLine;

	// draw one line more to properly undraw the outline selection
	toOffset = (1 + toOffset / kBytesPerLine) * kBytesPerLine;

	HideCursor();

	float verticalSpacing = LineHeight();
	
	// figure out where we are drawing
	BPoint where(kMargin, kMargin + ceil(fontHeight.ascent));
	where.y += (fromOffset / kBytesPerLine) * verticalSpacing;
	
	const uchar *bytes = (const uchar *)parent->Data();

	// figure out how much we are drawing
	ssize_t bytesLeft = toOffset + kBytesPerLine;
	if (bytesLeft > (ssize_t)parent->Size())
		bytesLeft = parent->Size();
	bytesLeft -= fromOffset;

	int32 offset = fromOffset;
	do {
		// ToDo:
		// don't draw/bail if drawing before/after visible rect
	
		SetViewColors(this, false);
		BRect rect;
		rect.left = Bounds().left;
		rect.right = Bounds().right;
		rect.bottom = where.y + ceil(fontHeight.descent + (fontHeight.leading));
		rect.top = rect.bottom - LineHeight();
		rect.bottom -= 1;
		if (offset == 0 && rect.top > 0) {
			FillRect(BRect(rect.left, 0, rect.right, rect.top-1), B_SOLID_LOW);
		}
		if (directDraw) {
			// direct draw - use an offscreen bitmap
		
			DrawLine(offscreen.BeginBlitting(this, rect, false), where, offset,
				bytes + offset, bytesLeft);
			offscreen.DoneBlitting();
		} else {
			FillRect(rect, B_SOLID_LOW);
			DrawLine(this, where, offset, bytes + offset, bytesLeft);
		}

		bytesLeft -= kBytesPerLine;
		offset += kBytesPerLine;
		where.y += verticalSpacing;
	} while( bytesLeft > 0 );

	if (eraseToEnd || toOffset >= (int32)(parent->Size() - kBytesPerLine)) {
		// erase any artifact left over after deleting text or after
		// unselecting an outline selection at the end of the text
		BRect rect(Bounds());
		int32 last = parent->Size()-1;
		if (last < 0) last = 0;
		rect.top = OffsetToHexPoint(last).y + fontHeight.descent;
	
		if (!eraseToEnd)
			rect.bottom = rect.top + LineHeight();
		
		SetViewColors(this, false);
		FillRect(rect, B_SOLID_LOW);
		
		// draw the the outline selection again - top/bottom parts of it may
		// still be missing and it should be pretty cheap to draw anyhow			
		bool active = Window()->IsActive() && IsFocus();
		if (!active || hexColumnFocused)
			DrawAsciiOutlineSelection(this);
	
		if (!active || !hexColumnFocused)
			DrawHexOutlineSelection(this);

		ConstrainClippingRegion(NULL);
	}
	

	ShowCursor();
}

void 
HexDumpView::DrawLine(BView *view, BPoint where, int32 offset, const void *data,
	size_t size)
{
	rgb_color fg, bg;
	GetViewColors(&fg, &bg, false);

	// draw offset value with a shade of gray
	view->SetHighColor(my_mix_color(fg, bg, 64+32));
	DrawOffsetValue(view, where, offset);

	view->SetHighColor(fg);
	
	// draw hex column
	where.x += kHexDigitsLeftMargin + offsetWidth;
	DrawDataCommon(view, &HexDumpView::DrawHexRun, where, data, size, offset,
		hexColumnFocused);

	// draw ascii column
	where.x += kAsciiDigitsLeftMargin + hexRunWidth;
	DrawDataCommon(view, &HexDumpView::DrawAsciiRun, where, data, size, offset,
		!hexColumnFocused);

	// draw outline selection on top of the non-focused column
	if (hexColumnFocused)
		DrawAsciiOutlineSelection(view);

	if (!hexColumnFocused)
		DrawHexOutlineSelection(view);
}

void 
HexDumpView::DrawOffsetValue(BView *view, BPoint where, int32 offset)
{
	char string[9];
	for (int32 index = 7; index >= 0; index--) {
		string[index] = "0123456789ABCDEF"[offset % 0x10];
		offset /= 0x10;
	}
	string[8] = '\0';
	view->DrawString(string, where);
}

void
HexDumpView::GetViewColors(rgb_color* foreground, rgb_color* background,
						   bool selected) const
{
	// ToDo:
	// should use the system defined selection color here
	static const rgb_color black = {0, 0, 0, 255};
	static const rgb_color white = {255, 255, 255, 255};
	static const rgb_color gray = {192, 192, 192, 155};
	
	const bool active = Window() && Window()->IsActive() && IsFocus();
	if (selected) {
		*foreground = active ? white : black;
		*background = active ? black : gray;
	} else {
		*foreground = black;
		*background = white;
	}
}

void
HexDumpView::SetViewColors(BView* into, bool selected) const
{
	rgb_color f, b;
	GetViewColors(&f, &b, selected);
	into->SetHighColor(f);
	into->SetLowColor(b);
}

void 
HexDumpView::DrawRunCommon(BView *view, BPoint where, const char *string, int32 length,
	float width, bool selected, bool erase)
{
	SetViewColors(view, selected);
	if (selected || erase) {
		// draw white background or selection
		BRect eraseRect;
		eraseRect.left = where.x - kSelectionExtraMargin;
		eraseRect.right = width - 2 + kSelectionExtraMargin;
		if (selected && string[length - 1] == ' ')
			eraseRect.right -= fixedCharacterWidth - 1;

		eraseRect.bottom = where.y + ceil(fontHeight.descent + fontHeight.leading);
		eraseRect.top = eraseRect.bottom - LineHeight();
		eraseRect.bottom -= 1;
		view->FillRect(eraseRect, B_SOLID_LOW);
	}
	view->DrawString(string, where);
}

float
HexDumpView::DrawHexRun(BView *view, BPoint where, const void *data, size_t size,
	int32 absoluteOffset, bool selected, bool erase)
{
	// hex run composer
	ASSERT(size <= kBytesPerLine);
	ASSERT(size > 0);

	const int32 len = 2 * kBytesPerLine + kBytesPerLine / hexDigitChunkSize;
	
	BString string;
	
	if( len > 0 ) {
		char *run = string.LockBuffer(len);
		
		for (uint32 index = 0; index < size; index++) {
			uchar ch = ((const uchar *)data)[index];
			*run++ = "0123456789ABCDEF"[ch / 0x10];
			*run++ = "0123456789ABCDEF"[ch % 0x10];
			absoluteOffset++;
			if ((absoluteOffset % hexDigitChunkSize) == 0)
				*run++ = ' ';
		}
		*run = '\0';
		string.UnlockBuffer();
	}
	
	float result = where.x + fixedCharacterWidth * string.Length();
	DrawRunCommon(view, where, string.String(), string.Length(), result, selected, erase);
	return result;
}

void 
HexDumpView::DrawOutlineSelectionCommon(BView *view, float left, float right,
	float notchLeft, float notchRight)
{
	float top = OffsetToHexPoint(selectionStart).y - ceil(fontHeight.ascent) + 1;
	float bottom = OffsetToHexPoint(selectionEnd).y
		+ ceil(fontHeight.descent + fontHeight.leading);
	
	bool leftTopNotch = (selectionStart % kBytesPerLine) != 0;
	bool rightBottomNotch = (selectionEnd % kBytesPerLine) != 0;

	if (!rightBottomNotch)
		bottom -= LineHeight();

	BRect notchOutline(OffsetToHexPoint(selectionStart), OffsetToHexPoint(selectionEnd));
	notchOutline.left = notchLeft - kSelectionExtraMargin;
	notchOutline.bottom += ceil(fontHeight.descent + fontHeight.leading)
		- LineHeight();
	notchOutline.right = notchRight - kSelectionExtraMargin;
	notchOutline.top += ceil(fontHeight.descent + fontHeight.leading);

	rgb_color color, dummy;
	GetViewColors(&dummy, &color, true);

	int32 lineCount = 4;
	if (leftTopNotch)
		lineCount += 2;
	else {
		notchOutline.top = top;
		notchOutline.left = left;
	}

	if (rightBottomNotch)
		lineCount += 2;
	else {
		notchOutline.right = right;
		notchOutline.bottom = bottom;
	}

	bool drawTwoRects = false;

	if ((selectionStart / kBytesPerLine) == ((selectionEnd - 1)/ kBytesPerLine)) {
		// single line selection only
		lineCount = 4;
		left = notchOutline.left;
		right = notchOutline.right;
		notchOutline.bottom = bottom;
		notchOutline.top = top;
	} else if (selectionEnd < selectionStart + kBytesPerLine)
		// two line selection, two discontiguous rects
		drawTwoRects = true;

	view->BeginLineArray(lineCount);
	view->AddLine(BPoint(notchOutline.left, top), BPoint(right, top), color);
	view->AddLine(BPoint(right, top),  BPoint(right, notchOutline.bottom), color);
	view->AddLine(BPoint(left, notchOutline.top), BPoint(left, bottom), color);
	view->AddLine(BPoint(left, bottom), BPoint(notchOutline.right, bottom), color);
	
	if (drawTwoRects) {
		view->AddLine(BPoint(notchOutline.left, top),
			BPoint(notchOutline.left, notchOutline.top), color);
		view->AddLine(BPoint(notchOutline.left, notchOutline.top),
			BPoint(right, notchOutline.top), color);

		view->AddLine(BPoint(left, notchOutline.top),
			BPoint(notchOutline.right, notchOutline.top), color);
		view->AddLine(BPoint(notchOutline.right, notchOutline.top),
			BPoint(notchOutline.right, bottom), color);
	} else {
		if (leftTopNotch) {
			view->AddLine(BPoint(left, notchOutline.top),
				BPoint(notchOutline.left, notchOutline.top), color);
			view->AddLine(BPoint(notchOutline.left, notchOutline.top),
				BPoint(notchOutline.left, top), color);
		}
		if (rightBottomNotch) {
			view->AddLine(BPoint(right, notchOutline.bottom),
				BPoint(notchOutline.right, notchOutline.bottom), color);
			view->AddLine(BPoint(notchOutline.right, notchOutline.bottom),
				BPoint(notchOutline.right, bottom), color);
		}
	}
	view->EndLineArray();
}

void 
HexDumpView::DrawHexOutlineSelection(BView *view)
{
	if (selectionStart == selectionEnd)
		return;

	float left = kMargin + kHexDigitsLeftMargin + offsetWidth;
	DrawOutlineSelectionCommon(view, left - kSelectionExtraMargin,
		left + fixedCharacterWidth
			* (kBytesPerLine / hexDigitChunkSize) * (2 * hexDigitChunkSize + 1),
		OffsetToHexPoint(selectionStart).x,
		OffsetToHexPoint(selectionEnd).x);
}

void 
HexDumpView::DrawAsciiOutlineSelection(BView *view)
{
	if (selectionStart == selectionEnd)
		return;

	float left = kMargin + kHexDigitsLeftMargin + offsetWidth
		+ hexRunWidth + kAsciiDigitsLeftMargin;
	DrawOutlineSelectionCommon(view, left - kSelectionExtraMargin,
		left + fixedCharacterWidth * kBytesPerLine,
		OffsetToAsciiPoint(selectionStart).x,
		OffsetToAsciiPoint(selectionEnd).x);
}

void
HexDumpView::DrawDataCommon(BView *view,
	float (HexDumpView::*drawFunc)(BView *, BPoint, const void *, size_t, int32, bool, bool),
	BPoint where, const void *data, size_t size, int32 offset, bool focused)
{
	if (size > kBytesPerLine)
		size = kBytesPerLine;
	if (size == 0)
		return;
		
	int32 localSelStart = (int32)selectionStart - offset;
	if (localSelStart < 0)
		localSelStart = 0;
	if (localSelStart > (int32)size)
		localSelStart = size;

	int32 localSelEnd = (int32)selectionEnd - offset;
	if (localSelEnd < 0)
		localSelEnd = 0;
	if (localSelEnd > (int32)size)
		localSelEnd = size;
	
	ASSERT(localSelStart >= 0);
	ASSERT(localSelEnd >= 0);
	ASSERT(localSelStart <= (int32)size);
	ASSERT(localSelEnd <= (int32)size);

	if (!focused)
		(this->*drawFunc)(view, where, data, size, offset, false, false);
	else {
		if (localSelStart > 0) {
			int32 runLength = localSelStart;
			where.x = (this->*drawFunc)(view, where, data, runLength, offset, false, false);
			data = (const void *)((const char *)data + runLength);
			offset += runLength;
		}
		if (localSelStart < localSelEnd) {
			int32 runLength = localSelEnd - localSelStart;
			where.x = (this->*drawFunc)(view, where, data, runLength, offset, true, false);
			data = (const void *)((const char *)data + runLength);
			offset += runLength;
		}
		if (localSelEnd < (int32)size) 
			(this->*drawFunc)(view, where, data, size - localSelEnd, offset, false, false);
	}
}

float
HexDumpView::DrawAsciiRun(BView *view, BPoint where, const void *data, size_t size,
	int32 , bool selected, bool erase)
{
	char string[kBytesPerLine + 1];
	char *run = string;
	
	if (size > kBytesPerLine)
		size = kBytesPerLine;

	for (uint32 index = 0; index < size; index++) {
		uchar ch = ((const uchar *)data)[index];
		if (isprint(ch))
			*run++ = ch;
		else
			*run++ = '.';
	}
	*run++ = '\0';
	
	float result = where.x + fixedCharacterWidth * size;
	DrawRunCommon(view, where, string, size, result, selected, erase);
	return result;
}

void 
HexDumpView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case 'puls':
			{
				if (trackingMouse) {
					const BRect bounds(Bounds());
					BPoint delta(0, 0);
					if (trackingPosition.x < bounds.left) {
						delta.x = (trackingPosition.x - bounds.left) / 2;
					} else if (trackingPosition.x > bounds.right) {
						delta.x = (trackingPosition.x - bounds.right) / 2;
					}
					if (trackingPosition.y < bounds.top) {
						delta.y = (trackingPosition.y - bounds.top)/2;
					} else if (trackingPosition.y > bounds.bottom) {
						delta.y = (trackingPosition.y - bounds.bottom)/2;
					}
					delta.x = floor(delta.x);
					delta.y = floor(delta.y);
					bool changed = false;
					BScrollBar* sb = ScrollBar(B_HORIZONTAL);
					if (sb) {
						float min, max;
						sb->GetRange(&min, &max);
						if ((bounds.left+delta.x) < min) {
							delta.x = min-bounds.left;
						}
						if ((bounds.left+delta.x) > max) {
							delta.x = max-bounds.left;
						}
						if (delta.x != 0) {
							sb->SetValue(sb->Value()+delta.x);
							trackingPosition.x += delta.x;
							changed = true;
						}
					}
					sb = ScrollBar(B_VERTICAL);
					if (sb) {
						float min, max;
						sb->GetRange(&min, &max);
						if ((bounds.top+delta.y) < min) {
							delta.y = min-bounds.top;
						}
						if ((bounds.top+delta.y) > max) {
							delta.y = max-bounds.top;
						}
						if (delta.y != 0) {
							sb->SetValue(sb->Value()+delta.y);
							trackingPosition.y += delta.y;
							changed = true;
						}
					}
					if (changed) {
						TrackMouse(trackingPosition, NULL);
					}
				}
			}
			break;
		
		default:
			_inherited::MessageReceived(message);
			break;
	}
}

void 
HexDumpView::MouseDown(BPoint where)
{
	if (!IsFocus()) {
		MakeFocus();
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS,
					  B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS |
					  	B_NO_POINTER_HISTORY);
	
	selectionAnchor = PointToOffset(where, true);
	trackingMouse = true;
	StartPulse();
	TrackMouse(where, NULL, true);
}

void 
HexDumpView::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	TrackMouse(where, message);
	
	switch (code) {
		case B_ENTERED_VIEW:
			if (!trackingMouse) {
				if (!message && Window()->IsActive())
					SetViewCursor(B_CURSOR_I_BEAM);
				else
					SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			}
			break;
			
		case B_INSIDE_VIEW:
			break;
			
		case B_EXITED_VIEW:
			if (!trackingMouse) {
				if (!message && Window()->IsActive())
					SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			}
			break;
	}
}

void 
HexDumpView::MouseUp(BPoint where)
{
	TrackMouse(where, NULL);
	StopPulse();
	trackingMouse = false;

	if (Bounds().Contains(where) && Window()->IsActive())
		// we stopped tracking a mouse down, cursor could have been a
		// pointer
		SetViewCursor(B_CURSOR_I_BEAM);
}

void 
HexDumpView::TrackMouse(BPoint where, const BMessage *, bool fromMouseDown)
{
	if (trackingMouse) {
		trackingPosition = where;
		
		// constrain position to bounds of view; don't want to allow
		// selection outside of the visible area
		const BRect bounds(Bounds());
		if (where.x < bounds.left) where.x = bounds.left;
		else if (where.x > bounds.right) where.x = bounds.right;
		if (where.y < bounds.top) where.y = bounds.top;
		else if (where.y > bounds.bottom) where.y = bounds.bottom;
		
		uint32 newDragOffset = PointToOffset(where, true);
		int32 redrawStartOffset = -1;
		int32 redrawEndOffset = -1;
		
		bool clickIntoHex = InHexColumn(where);
		if (fromMouseDown && clickIntoHex != hexColumnFocused) {
			// switch focus between columns
			redrawStartOffset = selectionStart;
			if ((int32)newDragOffset < redrawStartOffset)
				redrawStartOffset = newDragOffset;
			redrawEndOffset = selectionEnd;
			if ((int32)newDragOffset > redrawEndOffset)
				redrawEndOffset = newDragOffset;
			hexColumnFocused = clickIntoHex;
			selectionStart = selectionAnchor;
			selectionEnd = selectionAnchor;

		} else if (newDragOffset < selectionAnchor && newDragOffset != selectionStart) {
			if (newDragOffset < selectionStart) {
				// growing selection start
				redrawStartOffset = newDragOffset;
				redrawEndOffset = selectionStart;
			} else {
				// shrinking selection start
				redrawStartOffset = selectionStart;
				redrawEndOffset = newDragOffset;
			}
			selectionStart = newDragOffset;
			if (selectionEnd != selectionAnchor) {
				// switched selection to before anchor since last change
				// update selection end too
				if (redrawEndOffset < (int32)selectionEnd)
					redrawEndOffset = selectionEnd;
				if (redrawEndOffset < (int32)selectionAnchor)
					redrawEndOffset = selectionAnchor;
				selectionEnd = selectionAnchor;
			}
		} else if (newDragOffset > selectionAnchor && newDragOffset != selectionEnd) {
			if (newDragOffset < selectionEnd) {
				// shrinking selection end
				redrawStartOffset = newDragOffset;
				redrawEndOffset = selectionEnd;
			} else {
				// growing selection start
				redrawStartOffset = selectionEnd;
				redrawEndOffset = newDragOffset;
			}
			selectionEnd = newDragOffset;
			if (selectionStart != selectionAnchor) {
				// switched selection to after anchor since last change
				// update selection start too
				if (redrawStartOffset > (int32)selectionStart)
					redrawStartOffset = selectionStart;
				if (redrawStartOffset > (int32)selectionAnchor)
					redrawStartOffset = selectionAnchor;
				selectionStart = selectionAnchor;
			}
		} else if (newDragOffset == selectionAnchor && newDragOffset != selectionStart) {
			if (newDragOffset < selectionStart)
				redrawStartOffset = newDragOffset;
			else
				redrawStartOffset = selectionStart;

			if (newDragOffset > selectionEnd)
				redrawEndOffset = newDragOffset;
			else
				redrawEndOffset = selectionEnd;

			selectionStart = newDragOffset;
			selectionEnd = newDragOffset;
		}

		if (redrawStartOffset >= 0) 
			DrawRange(redrawStartOffset, redrawEndOffset, true);
	}
}

void 
HexDumpView::WindowActivated(bool on)
{
	_inherited::WindowActivated(on);
	if (IsFocus())
		Activate(on);
}

void 
HexDumpView::MakeFocus(bool on)
{
	_inherited::MakeFocus(on);
	if (Window()->IsActive())
		Activate(on);
		
	if( on ) ShowCursor();
	else HideCursor();
	
//	BScrollView *scrollView = dynamic_cast<BScrollView *>(Parent());
//	if (scrollView)
//		scrollView->SetBorderHighlighted(on);
}


void 
HexDumpView::Activate(bool on)
{
	if (selectionStart != selectionEnd)
		DrawRange(selectionStart, selectionEnd, true);
	else if (on)
		ShowCursor();
	else
		HideCursor();
}

void 
HexDumpView::FrameResized(float width, float height)
{
	HideCursor();
		// don't let cursor leave dirt behind
	_inherited::FrameResized(width, height);
	TweakScrollBars(true);
}

void
HexDumpView::StartPulse()
{
	StopPulse();
	if (pulse) delete pulse;
	BMessage msg('puls');
	pulse = new BMessageRunner(BMessenger(this), &msg, 1000000/60);
}

void
HexDumpView::StopPulse()
{
	if (pulse) delete pulse;
	pulse = NULL;
	
}

void 
HexDumpView::BumpSelectionBy(int32 offset, bool startAnchor, bool endAnchor)
{
	if (startAnchor)
		endAnchor = false;

	int32 newOffset = selectionEnd;

	if (endAnchor)
		newOffset = selectionStart;

	newOffset += offset;
	
	if (newOffset < 0)
		newOffset = 0;

	if (newOffset > (int32)parent->Size())
		newOffset = parent->Size();

	Select(startAnchor ? selectionStart : newOffset,
		endAnchor ? selectionEnd : newOffset);
}

void 
HexDumpView::BumpSelectionTo(int32 offset, bool startAnchor, bool endAnchor)
{
	if (startAnchor)
		endAnchor = false;

	if (offset < 0)
		offset = 0;

	if (offset > (int32)parent->Size())
		offset = parent->Size();

	Select(startAnchor ? selectionStart : offset,
		endAnchor ? selectionEnd : offset);
}

void 
HexDumpView::KeyDown(const char *bytes, int32 numBytes)
{
	be_app->ObscureCursor();
	
	if (numBytes != 1)
		// can't deal with this yet
		return;

	const BMessage *message = Window()->CurrentMessage();
	uint32 modifiers = 0;
	if (message)
		message->FindInt32("modifiers", (int32 *)&modifiers);

	bool shift = (modifiers & B_SHIFT_KEY) != 0;
	bool startAnchor = shift && (selectionAnchor == selectionStart);
	bool endAnchor = shift && (selectionAnchor == selectionEnd);

	switch (*bytes) {
		case B_DELETE:
			if (selectionStart == selectionEnd)
				Select(selectionStart, selectionEnd + 1);
			Delete("delete");
			return;

		case B_BACKSPACE:
			if (selectionStart == selectionEnd)
				Select(selectionStart - 1, selectionEnd);
			Delete("delete");
			return;

		case B_LEFT_ARROW:
			if (shift || selectionStart == selectionEnd)
				BumpSelectionBy(-1, startAnchor, endAnchor);
			else
				Select(selectionStart, selectionStart);
			return;

		case B_UP_ARROW:
			if (shift || selectionStart == selectionEnd)
				BumpSelectionBy(-kBytesPerLine, startAnchor, endAnchor);
			else
				Select(selectionStart, selectionStart);
			return;

		case B_RIGHT_ARROW:
			if (shift || selectionStart == selectionEnd)
				BumpSelectionBy(1, startAnchor, endAnchor);
			else
				Select(selectionEnd, selectionEnd);
			return;

		case B_DOWN_ARROW:
			if (shift || selectionStart == selectionEnd)
				BumpSelectionBy(kBytesPerLine, startAnchor, endAnchor);
			else
				Select(selectionEnd, selectionEnd);
			return;

		case B_HOME:
			if (shift || selectionStart == selectionEnd)
				BumpSelectionTo(0, startAnchor, endAnchor);
			else
				Select(0, 0);
			return;

		case B_END:
			if (shift || selectionStart == selectionEnd)
				BumpSelectionTo(parent->Size(), startAnchor, endAnchor);
			else
				Select(parent->Size(), parent->Size());
			return;

		case B_PAGE_DOWN: {
			int32 jump = VisibleLines()-1;
			if (jump < 1) jump = 1;
			if (shift || selectionStart == selectionEnd)
				BumpSelectionBy(kBytesPerLine*jump, startAnchor, endAnchor);
			else
				Select(selectionEnd, selectionEnd);
			return;
		}
		
		case B_PAGE_UP: {
			int32 jump = VisibleLines()-1;
			if (jump < 1) jump = 1;
			if (shift || selectionStart == selectionEnd)
				BumpSelectionBy(-kBytesPerLine*jump, startAnchor, endAnchor);
			else
				Select(selectionStart, selectionStart);
			return;
		}
		
		case B_RETURN:
		case B_INSERT:
		case B_FUNCTION_KEY:
			return;

		case B_ESCAPE:
		case B_TAB:
			_inherited::KeyDown(bytes, numBytes);
			return;

	}
	if (hexColumnFocused) {
		uint32 digit = 0;
		char ch = *bytes;
		if (ch >= '0' && ch <= '9')
			digit = ch - '0';
		else if (ch >= 'a' && ch <= 'f')
			digit = ch - 'a' + 10;
		else if (ch >= 'A' && ch <= 'F')
			digit = ch - 'A' + 10;
		else
			return;

		InsertHexDigit(digit, "typing");
	} else {
		if (isprint(*bytes)) {
			char tmp[2] = { *bytes, '\0'};
			Insert(tmp, 1, "typing");
		}
	}	
}

void 
HexDumpView::Select(uint32 newSelectionStart, uint32 newSelectionEnd)
{
	if (selectionStart == newSelectionStart
		&& selectionEnd == newSelectionEnd)
		return;

	if (newSelectionStart > newSelectionEnd) {
		uint32 tmp = newSelectionStart;
		newSelectionStart = newSelectionEnd;
		newSelectionEnd = tmp;
	}
		

	expectingSecondHexDigit = false;
	int32 redrawStartOffset = selectionStart;
	int32 redrawEndOffset = selectionEnd;
	if ((int32)newSelectionStart < redrawStartOffset)
		redrawStartOffset = newSelectionStart;
	if ((int32)newSelectionEnd > redrawEndOffset)
		redrawEndOffset = newSelectionEnd;

	const bool scrollToFront = (newSelectionStart != selectionStart);
	
	selectionStart = newSelectionStart;
	selectionEnd = newSelectionEnd;
	
	if (selectionStart == selectionEnd)
		selectionAnchor = selectionStart;

	MakeOffsetVisible(scrollToFront ? selectionStart : selectionEnd);
	
	DrawRange(redrawStartOffset, redrawEndOffset, true);
}

void 
HexDumpView::Insert(const char *data, size_t size, const char *undoName)
{
	int32 removedSize = selectionEnd - selectionStart;
	uint32 insertionPoint = selectionStart; 
	if (removedSize > 0) 
		parent->DeleteRange(selectionStart, selectionEnd, undoName);

	parent->Insert(selectionStart, data, size, undoName);
	
	selectionStart = insertionPoint + size;
	selectionEnd = selectionStart;
	
	if (removedSize == (int32)size) {
		// we can do a small update because the inserted text cancels
		// out the removed text - only update selection range
		DrawRange(insertionPoint, insertionPoint + removedSize, true);
	} else
		// redraw everything from selStart
		DrawRange(insertionPoint, parent->Size(), true);

	TweakScrollBars(true);
	MakeOffsetVisible(selectionStart);
}

void 
HexDumpView::Delete(const char *undoName)
{
	if (selectionStart == selectionEnd)
		return;

	parent->DeleteRange(selectionStart, selectionEnd, undoName);

	selectionEnd = selectionStart;

	DrawRange(selectionStart, parent->Size(), true, true);
	TweakScrollBars(true);
}

void 
HexDumpView::InsertHexDigit(uint32 digit, const char *undoName)
{
	uint32 redrawStart = selectionStart;
	uint32 redrawEnd = selectionEnd;
	
	if (!expectingSecondHexDigit) {
		char ch[2] = { digit * 0x10, 0};
		int32 removedSize = selectionEnd - selectionStart;
		if (removedSize > 0) 
			parent->DeleteRange(selectionStart, selectionEnd, undoName);
	
		parent->Insert(selectionStart, ch, 1, undoName);
		selectionEnd = selectionStart;
		if (removedSize != 1)
			// unless we also removed a byte this time around, redraw everything
			// after the insertion point
			redrawEnd = parent->Size();
		
		expectingSecondHexDigit = true;
		TweakScrollBars(true);
		MakeOffsetVisible(selectionStart);
	} else {
		uchar ch = ((uchar *)parent->Data())[selectionStart];
		ch += digit;
		parent->ModifyCharAt(selectionStart, ch, undoName);
		
		expectingSecondHexDigit = false;
		selectionStart++;
		selectionEnd++;
	}
	DrawRange(redrawStart, redrawEnd, true);
}

void
HexDumpView::MakeOffsetVisible(int32 offset)
{
	DoMakeOffsetVisible(offset, hexColumnFocused);
}

void
HexDumpView::MakeHexOffsetVisible(int32 offset)
{
	DoMakeOffsetVisible(offset, true);
}

void
HexDumpView::MakeAsciiOffsetVisible(int32 offset)
{
	DoMakeOffsetVisible(offset, false);
}

extern "C"
BResourceAddon *
make_nth_resourcer(int32 n, image_id , const BResourceAddonArgs &args,
	uint32 , ...)
{
	if (n == 0)
		return new HexDumpAddon(args);
	return 0;
}
