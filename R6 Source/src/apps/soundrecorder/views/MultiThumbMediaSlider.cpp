
#include <Region.h>
#include <stdio.h>

#include "Bitmaps.h"
#include "DrawingTidbits.h"
#include "MediaFile.h"
#include "MultiThumbMediaSlider.h"

//	number strip is :0123456789
//	the colon is kColorWidth wide
//	all other numbers are kNumberWidth in width
const int32 kColonWidth = 3;
const int32 kNumberWidth = 4;
const int32 kNumberStripWidth = 44;
const int32 kNumberStripHeight = 5;
const color_space kNumberStripColorSpace = B_COLOR_8_BIT;

const int32 kLabelSideGutter = 3;
const int32 kNumberSeparation = 1;
const int32 kLabelWidth = (2 * kLabelSideGutter) + (8 * kNumberWidth)
	+ (4 * kNumberSeparation) + (3 * kColonWidth) - 1;
const int32 kLabelHeight = 13;

const color_space kMediaThumbColorSpace = B_COLOR_8_BIT;

const int32 kMarkerWidth = 9;
const int32 kMarkerHeight = 18;
const int32 kMediaThumbWidth = 9;
const int32 kMediaThumbHeight = 14;
const int32 kCapWidth = 9;
const int32 kCapHeight = 17;


TimeThumb::TimeThumb(TMultiThumbSlider *owner, BMessage *mdMsg, BMessage *stdMsg,
	BMessage *modMsg)
	:	TThumb(owner, mdMsg, stdMsg, modMsg),
		fBits(0)
{
}

TimeThumb::~TimeThumb()
{
	delete fBits;
}

bigtime_t 
TimeThumb::TimeValue() const
{
	return timeValue;
}

void 
TimeThumb::SetTimeValue(bigtime_t time)
{
	timeValue = time;
}

TLeftThumb::TLeftThumb(TMultiThumbSlider *owner, BMessage *mdMsg, BMessage *stdMsg,
	BMessage *modMsg)
	:	TimeThumb(owner, mdMsg, stdMsg, modMsg)
{
	fBits = new BBitmap(BRect(0, 0, kMediaThumbWidth-1, kMediaThumbHeight-1),
		kMediaThumbColorSpace, false, false);
	fBits->SetBits((char*)kMediaLeftThumb, fBits->BitsLength(), 0,
		kMediaThumbColorSpace);
}

void 
TLeftThumb::Draw(BView *osv, BRect barFrame)
{
	BPoint point(Location().x - kMediaThumbWidth, barFrame.top+2);
	
	osv->PushState();
	osv->SetDrawingMode(B_OP_OVER);
	osv->DrawBitmap(fBits, point);
	osv->PopState();
}

BRect
TLeftThumb::Frame() const
{
	BPoint loc(Location());
	BRect barFrame(const_cast<TLeftThumb *>(this)->Owner()->BarFrame());
	BRect frame(loc.x-kMediaThumbWidth, barFrame.top, loc.x + 1, barFrame.bottom);
		// adding one here because of the drop shadow that should be considered
		// thumb frame
	
	return frame;
}

bool
TLeftThumb::HitTest(BPoint point) const
{
	BRect frame(Frame());
	if (!frame.Contains(point))
		return false;
		
	uchar *bits = (uchar *)fBits->Bits();
		
	bits += (int32)((point.y * fBits->BytesPerRow()) + (point.x - frame.left));
		
	return *bits != 0xff;
}

TCenterThumb::TCenterThumb(TMultiThumbSlider *owner, BMessage *mdMsg, BMessage *stdMsg,
	BMessage *modMsg)
	:	TimeThumb(owner, mdMsg, stdMsg, modMsg)
{
	fBits = new BBitmap(BRect(0, 0, kMarkerWidth-1, kMarkerHeight-1),
		kMediaThumbColorSpace, false, false);
	fBits->SetBits((char*)kMarker, fBits->BitsLength(), 0, kMediaThumbColorSpace);
}


void 
TCenterThumb::Draw(BView *osv, BRect barFrame)
{
	BPoint point(Location().x - kMarkerWidth/2, barFrame.top);

	osv->PushState();
	osv->SetDrawingMode(B_OP_OVER);
	osv->DrawBitmap(fBits, point);	
	osv->PopState();
}

BRect
TCenterThumb::Frame() const
{
	BPoint loc(Location());
	BRect barFrame(const_cast<TCenterThumb*>(this)->Owner()->BarFrame());
	BRect frame(loc.x-kMarkerWidth/2, barFrame.top, loc.x+kMarkerWidth/2, barFrame.bottom + 1);
	
	return frame;
}

bool
TCenterThumb::HitTest(BPoint point) const
{
	BRect frame(Frame());	
	
	if (!frame.Contains(point))
		return false;
		
	TCenterThumb* self = const_cast<TCenterThumb*>(this);
	
	if (frame.Contains(self->Owner()->ThumbAt(kLeftThumb)->Frame())
		|| frame.Contains(self->Owner()->ThumbAt(kRightThumb)->Frame()) ) {
		
		uchar *bits = (uchar *)fBits->Bits();
		
		bits += (int32)((point.y * fBits->BytesPerRow()) + (point.x - frame.left));
		
		if (*bits == 0xff)
			return false;
	}

	return true;
}

TRightThumb::TRightThumb(TMultiThumbSlider *owner, BMessage *mdMsg, BMessage *stdMsg,
	BMessage *modMsg)
	:	TimeThumb(owner, mdMsg, stdMsg, modMsg)
{
	fBits = new BBitmap(BRect(0 ,0 , kMediaThumbWidth-1, kMediaThumbHeight-1),
		kMediaThumbColorSpace, false, false);
	fBits->SetBits((char *)kMediaRightThumb, fBits->BitsLength(), 0,
		kMediaThumbColorSpace);
}

void 
TRightThumb::Draw(BView* osv, BRect barFrame)
{
	BPoint point(Location().x, barFrame.top+2);
	
	osv->PushState();
	osv->SetDrawingMode(B_OP_OVER);
	osv->DrawBitmap(fBits, point);
	osv->PopState();
}

BRect
TRightThumb::Frame() const
{
	BPoint loc(Location());
	BRect barFrame(const_cast<TRightThumb*>(this)->Owner()->BarFrame());
	BRect frame(loc.x, barFrame.top, loc.x+kMediaThumbWidth, barFrame.bottom);
	
	return frame;
}

bool
TRightThumb::HitTest(BPoint point) const
{
	BRect frame(Frame());
	
	if (!frame.Contains(point))
		return false;
		
	uchar *bits = (uchar *)fBits->Bits();
	
	bits += (int32)((point.y * fBits->BytesPerRow()) + (point.x - frame.left));
	
	return *bits != 0xff;
}

//

TMultiThumbMediaSlider::TMultiThumbMediaSlider(BRect frame,
	const char *name, const char *label, int32 minValue, int32 maxValue,
	uint32 resizingMode, uint32 flags)
	:	TMultiThumbSlider(frame, name, label, minValue, maxValue, resizingMode, flags)
{
	SetFont(be_fixed_font);
	
	fLeftCap = new BBitmap(BRect(0,0,kCapWidth-1, kCapHeight-1),
		kMediaThumbColorSpace, false, false);
	fLeftCap->SetBits((char*)kLeftBarCap, fLeftCap->BitsLength(), 0,
		kMediaThumbColorSpace);
			
	fRightCap = new BBitmap(BRect(0,0,kCapWidth-1, kCapHeight-1),
		kMediaThumbColorSpace, false, false);
	fRightCap->SetBits((char*)kRightBarCap, fRightCap->BitsLength(), 0,
		kMediaThumbColorSpace);

	fDigitBits = new BBitmap(BRect(0, 0, kNumberStripWidth - 1, kNumberStripHeight - 1),
		B_COLOR_8_BIT);
	fDigitBits->SetBits(kNumberStrip, kNumberStripWidth*kNumberStripHeight,
		0, B_COLOR_8_BIT);
	
	fInvertedDigitBits = new BBitmap(BRect(0, 0, kNumberStripWidth - 1, kNumberStripHeight - 1),
		B_COLOR_8_BIT);
	fInvertedDigitBits->SetBits(kNumberStrip, kNumberStripWidth*kNumberStripHeight,
		0, B_COLOR_8_BIT);
	
	fDropHereBits = new BBitmap(BRect(0, 0, kDropHereWidth - 1, kDropHereHeight - 1),
		B_COLOR_8_BIT);
	fDropHereBits->SetBits(kDropHereBits, kDropHereWidth * kDropHereHeight,
		0, B_COLOR_8_BIT);

	ReplaceColor(fInvertedDigitBits, kBlack, kWhite);
	
	fLabelPlacement = kLabelNone;
	fThumbCollisionDetection = true;
}


TMultiThumbMediaSlider::~TMultiThumbMediaSlider()
{
	delete fLeftCap;
	delete fRightCap;
	delete fDigitBits;
	delete fInvertedDigitBits;
	delete fDropHereBits;
}

void
TMultiThumbMediaSlider::AttachedToWindow()
{
	TMultiThumbSlider::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_32_BIT);
}

void
TMultiThumbMediaSlider::GetPreferredSize(float *width, float *height)
{
	TMultiThumbSlider::GetPreferredSize(width, height);
	
	if (fLabelPlacement == kLabelTop || fLabelPlacement == kLabelBottom) 
		*height += LabelHeight();
}

void
TMultiThumbMediaSlider::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);
	ResizeTo(width, height);
}

void
TMultiThumbMediaSlider::MouseUp(BPoint point)
{
	TMultiThumbSlider::MouseUp(point);
	
	//	need to redraw the whole slider
	//	in case the labels are showing, they might go away
	DrawSlider(Bounds());
}

void
TMultiThumbMediaSlider::SetValueFor(int32 value, int32 thumbIndex, bool postUpdates,
	 bool lazy)
{
	TThumb *thumb = ThumbAt(thumbIndex);
	if (lazy && thumb->Value() == value)
		return;

	BPoint oldLocation(thumb->Location());
	
	TMultiThumbSlider::SetValueFor(value, thumbIndex, postUpdates, lazy);
	
	if (thumbIndex == kMainThumb) 
		// make sure fValue gets set properly
		BControl::SetValueNoUpdate(value);

	// force an update on the label
	BPoint newLocation(thumb->Location());
	BRect barFrame(BarFrame());
	BRect oldLabelFrame(TrackingLabelRect(thumbIndex, barFrame, oldLocation));
	oldLabelFrame.bottom += 1;
	Invalidate(oldLabelFrame);
	Invalidate(TrackingLabelRect(thumbIndex, barFrame, newLocation));

	if (ThumbCount() != 3 || !ThumbCollisionDetection())
		// slider not fully set up yet or collision detection off
		return;
	
	switch(thumbIndex) {
		case kLeftThumb:
			if (value > ThumbAt(kMainThumb)->Value()) {
				TMultiThumbSlider::SetValueFor(value, kMainThumb, postUpdates);
				BControl::SetValue(value);
				if (postUpdates)
					Invoke(ModificationMessageFor(kMainThumb));
			}
	
			value++;
			if (value > ThumbAt(kRightThumb)->Value()) {
				TMultiThumbSlider::SetValueFor(value, kRightThumb, postUpdates);
				if (postUpdates)
					Invoke(MessageFor(kRightThumb));
			}
			break;

		case kMainThumb:
			if (value < ThumbAt(kLeftThumb)->Value()) {
				TMultiThumbSlider::SetValueFor(value, kLeftThumb, postUpdates);
				if (postUpdates)
					Invoke(MessageFor(kLeftThumb));
			} else if (value > ThumbAt(kRightThumb)->Value()) {
				TMultiThumbSlider::SetValueFor(value, kRightThumb, postUpdates);
				if (postUpdates)
					Invoke(MessageFor(kRightThumb));
			}
			break;

		case kRightThumb:
			if (value < ThumbAt(kMainThumb)->Value()) {
				TMultiThumbSlider::SetValueFor(value, kMainThumb, postUpdates);
				BControl::SetValue(value);
				if (postUpdates)
					Invoke(ModificationMessageFor(kMainThumb));
			}
			
			if (value < ThumbAt(kLeftThumb)->Value()) {
				TMultiThumbSlider::SetValueFor(value, kLeftThumb, postUpdates);
				if (postUpdates)
					Invoke(MessageFor(kLeftThumb));
			}
			break;
	}
}

void
TMultiThumbMediaSlider::AddThumbs(int32 centerValue, int32 leftValue, int32 rightValue,
	BMessage* mainMdMsg, BMessage* mainMsg, BMessage* mainModMsg,
	BMessage* leftMdMsg, BMessage* leftMsg, BMessage* leftModMsg,
	BMessage* rightMdMsg, BMessage* rightMsg, BMessage* rightModMsg)
{
	AddThumb(new TCenterThumb(this, mainMdMsg, mainMsg, mainModMsg), centerValue);
	AddThumb(new TLeftThumb(this, leftMdMsg, leftMsg, leftModMsg), leftValue);
	AddThumb(new TRightThumb(this, rightMdMsg, rightMsg, rightModMsg), rightValue);
}

BRect
TMultiThumbMediaSlider::LeftUsedRect(BRect barFrame) const
{
	if (!ThumbAt(kLeftThumb))
		return BRect(0, 0, 0, 0);
	
	BRect rect(barFrame);	
	rect.right = ThumbAt(kLeftThumb)->Location().x;
	
	return rect;
}

BRect
TMultiThumbMediaSlider::RightUsedRect(BRect barFrame) const
{
	if (!ThumbAt(kRightThumb))
		return BRect(0, 0, 0, 0);
		
	BRect rect(barFrame);	
	rect.left = ThumbAt(kRightThumb)->Location().x;
	
	return rect;
}

BRect
TMultiThumbMediaSlider::FillRect(BRect barFrame) const
{
	if (!ThumbAt(kMainThumb))
		return BRect(0, 0, 0, 0);
		
	BRect rect(barFrame);	
	rect.left = LeftUsedRect(barFrame).right;
	rect.right = RightUsedRect(barFrame).left - 1;
	
	return rect;
}

void 
TMultiThumbMediaSlider::DrawBar(BView *osv, BRect barFrame)
{
	osv->SetHighColor(Parent()->ViewColor());
	osv->FillRect(osv->Bounds());
	osv->PushState();
	
	osv->SetHighColor(kUnusedColor);
	
	if (IsEnabled()) {
		bool fill = false;
		
		if (ThumbAt(kLeftThumb) && ThumbAt(kRightThumb))
			fill = true;
		BRect fillRect(barFrame);
		if (fill) {
			BRect usedRect = LeftUsedRect(barFrame);
			osv->FillRect(usedRect);
			fillRect.left = usedRect.right;
			
			usedRect = RightUsedRect(barFrame);
			osv->FillRect(usedRect);
			fillRect.right = usedRect.left - 1;
			
			osv->SetHighColor(kFillGreen);
			osv->FillRect(fillRect);
		}
			
		// draw the trim
		osv->BeginLineArray(7);
		
		if (fill) {
			osv->AddLine(BPoint(fillRect.left, fillRect.top),
				BPoint(fillRect.left, fillRect.bottom), kDarkGreen);	
			osv->AddLine(BPoint(fillRect.left+1, fillRect.top),
				BPoint(fillRect.left+1, fillRect.bottom), kDarkGreen);	
			osv->AddLine(BPoint(fillRect.left, fillRect.top+2),
				BPoint(fillRect.right, fillRect.top+2), kDarkGreen);	
			osv->AddLine(BPoint(fillRect.left, fillRect.top+3),
				BPoint(fillRect.right, fillRect.top+3), kDarkGreen);	
		}
	} else {
		BRect clipRect(barFrame);
		clipRect.InsetBy(-3, 0);
		BRegion region;
		region.Set(clipRect);
		osv->ConstrainClippingRegion(&region);
		osv->SetDrawingMode(B_OP_OVER);

		BRect bitBounds(fDropHereBits->Bounds());
		float offset = (bitBounds.Width() - barFrame.Width()) / 2;
		osv->DrawBitmap(fDropHereBits, BPoint(barFrame.left - offset + 4, barFrame.top + 2));
		osv->ConstrainClippingRegion(0);

		osv->SetDrawingMode(B_OP_COPY);
		osv->BeginLineArray(3);
	}
	
	rgb_color white = kWhiteColor;
	rgb_color ltgray = kLtGrayColor;
	rgb_color black = kBlackColor;

	osv->AddLine(BPoint(barFrame.left, barFrame.top),
		BPoint(barFrame.right, barFrame.top), ltgray);
	osv->AddLine(BPoint(barFrame.left, barFrame.top+1),
		BPoint(barFrame.right, barFrame.top+1), black);
	osv->AddLine(BPoint(barFrame.left, barFrame.bottom),
		BPoint(barFrame.right, barFrame.bottom), white);

	osv->EndLineArray();

	osv->SetDrawingMode(B_OP_OVER);

	if (IsEnabled()) {
		osv->DrawBitmap(fLeftCap, BPoint(0 , barFrame.top));
		osv->DrawBitmap(fRightCap, BPoint(barFrame.right,barFrame.top));
	} else {
		BBitmap tmp1(fLeftCap);
		ReplaceColor(&tmp1, 0x13, 0x0c);
		osv->DrawBitmap(&tmp1, BPoint(0 , barFrame.top));
		
		BBitmap tmp2(fRightCap);
		ReplaceColor(&tmp2, 0x13, 0x0c);
		osv->DrawBitmap(&tmp2, BPoint(barFrame.right,barFrame.top));
	}
	osv->PopState();
}


BRect
TMultiThumbMediaSlider::BarFrame() const
{
	BRect barRect(Bounds());
	
	//	offset left and right half the amount of the thumb
	barRect.left += kMediaThumbWidth;
	barRect.right -= kMediaThumbWidth;
	
	if (fLabelPlacement == kLabelBottom || fLabelPlacement == kLabelTop)
		barRect.bottom--;
	else
		barRect.bottom -= 2;
	
	if ((fLabelPlacement == kLabelBottom)) 
		barRect.bottom -= LabelHeight();
	else if (fLabelPlacement == kLabelTop) 
		barRect.top += LabelHeight() + 1;
		
	return barRect;
}

const int32 kHashMarkSpacing = 6;

void
TMultiThumbMediaSlider::DrawHashMarks(BView *osv, BRect barFrame, BRect )
{
	if (!IsEnabled() || barFrame.Width() <= kHashMarkSpacing)
		return;

	osv->PushState();

	int32 count = (int32)(barFrame.Width() / 4);
	BPoint pt1, pt2;
	pt1.x = barFrame.left + kHashMarkSpacing / 2;
	pt1.y = barFrame.top + 6;
	pt2.x = pt1.x;
	pt2.y = pt1.y + 6;
	
	osv->BeginLineArray(count * 2);
	rgb_color highHashColor;
	rgb_color lowHashColor;
	BRect fillRect(FillRect(barFrame));
	while(true) {		
		if (pt1.x >= fillRect.left && pt1.x <= fillRect.right) {
			SetLowColor(kFillGreen);
			highHashColor.red = 144;
			highHashColor.green = 186;
			highHashColor.blue = 136;
			lowHashColor.red = 189;
			lowHashColor.green = 244;
			lowHashColor.blue = 178;
		} else {
			SetLowColor(kUnusedColor);
			highHashColor.red = highHashColor.green = highHashColor.blue = 128;
			lowHashColor.red = lowHashColor.green = lowHashColor.blue = 179;
		}
		
		osv->AddLine(pt1, pt2, highHashColor);
		pt1.x++;
		pt2.x++;
		osv->AddLine(pt1, pt2, lowHashColor);
		pt1.x += kHashMarkSpacing - 1;
		pt2.x += kHashMarkSpacing - 1;
		if (pt1.x > barFrame.right)
			break;
	}
	osv->EndLineArray();
	osv->PopState();
}

void 
TMultiThumbMediaSlider::DrawThumb(BView *osv, BRect barFrame, BRect updateRect)
{
	if (!IsEnabled() || ThumbCount() != 3)
		// slider not fully set up yet
		return;
			
	//	draw in this order so that the layering is correct
	DrawLabel(osv, barFrame);
	
	TThumb *thumb = ThumbAt(kLeftThumb);
	if (thumb->Frame().Intersects(updateRect))
		thumb->Draw(osv, barFrame);
	
	thumb = ThumbAt(kRightThumb);
	if (thumb->Frame().Intersects(updateRect))
		thumb->Draw(osv, barFrame);

	thumb = ThumbAt(kMainThumb);
	if (thumb->Frame().Intersects(updateRect))
		thumb->Draw(osv, barFrame);
}

void
TMultiThumbMediaSlider::SetLabelPlacement(label_placement placement)
{
	if (fLabelPlacement == placement)
		return;

	fLabelPlacement = placement;
	ResizeToPreferred();
}

label_placement
TMultiThumbMediaSlider::LabelPlacement() const
{
	return fLabelPlacement;
}

float
TMultiThumbMediaSlider::LabelHeight() const
{
	if (fLabelPlacement == kLabelCenter || fLabelPlacement == kLabelCenterWhileTracking)
		return kLabelHeight + 1;
		
	return kLabelHeight - 3;
	
}

BRect 
TMultiThumbMediaSlider::TrackingLabelRect(int32 thumbIndex, BRect barFrame, BPoint point) const
{
	BRect result;	
	float labelHeight = LabelHeight();

	barFrame.bottom += 1;

	if (fLabelPlacement == kLabelBottom)
		result.Set(point.x - kLabelWidth / 2, barFrame.bottom,
			point.x + kLabelWidth / 2, barFrame.bottom + labelHeight);
	else if (fLabelPlacement == kLabelCenter
			|| fLabelPlacement == kLabelCenterWhileTracking) {
		float top = barFrame.Height() / 2 - labelHeight / 2 - 1;
		//	place the tracking label relative to the thumb
		//	left - to the right of the thumb
		//	center - centered on thumb
		//	right - to left of thumb
		if (thumbIndex == kLeftThumb)
			result.Set(point.x, top, point.x + kLabelWidth, top+labelHeight);
		else if (thumbIndex == kRightThumb)
			result.Set(point.x - kLabelWidth, top, point.x, top + labelHeight);
		else
			result.Set(point.x - kLabelWidth / 2, top, point.x + kLabelWidth / 2,
				top + labelHeight);
	} else
		result.Set(point.x - kLabelWidth / 2, barFrame.top - labelHeight,
			point.x + kLabelWidth / 2, barFrame.top);

	//	offset the rect when it bumps into the left or right edges
	if (result.left < barFrame.left)
		result.OffsetTo(barFrame.left, result.top);
	else if (result.right > barFrame.right)
		result.OffsetTo(barFrame.right - kLabelWidth, result.top);
	
	return result;
}


const rgb_color kLabelFillBlue = { 48, 48, 241, 255 };
const rgb_color kLabelTopBorderBlue = { 146, 146, 214, 255 };
const rgb_color kLabelBottomBorderBlue = { 0, 0, 140, 255 };

void
TMultiThumbMediaSlider::DrawLabel(BView *osv, BRect barFrame)
{
	if (!IsEnabled() || fLabelPlacement == kLabelNone || ThumbCount() < 1)
		return;

	bool drawTracking;
	bool drawFixed;
	BPoint constantLoc(ThumbAt(kMainThumb)->Location());
	int32 trackingThumbIndex = CurrentThumb();
	
	switch (trackingThumbIndex) {
		case kMainThumb:
			drawTracking = true;
			drawFixed = false;
			break;
	
		case kLeftThumb:
		case kRightThumb:
			drawTracking = true;
			drawFixed = true;
			break;
	
		default:
			drawTracking = false;
			drawFixed = true;
			break;
	}
	
	
	BRect trackingRect;
	if (drawTracking)
		trackingRect = TrackingLabelRect(trackingThumbIndex, barFrame,
			ThumbAt(trackingThumbIndex)->Location());
	
	BRect fixedRect;
	if (drawFixed) 
		fixedRect = TrackingLabelRect(kMainThumb, barFrame, constantLoc);

#if 0	
	if (drawFixed && drawTracking) {
		// should there be label collision detection?
	}
#endif

	if (!drawFixed && !drawTracking)
		return;

	bigtime_t pos = dynamic_cast<TimeThumb *>(ThumbAt(kMainThumb))->TimeValue();
	bigtime_t otherPos = 0;
	
	if (trackingThumbIndex >= 0)
		otherPos = dynamic_cast<TimeThumb *>(ThumbAt(trackingThumbIndex))->TimeValue();
		
	if (drawFixed && fLabelPlacement != kLabelCenterWhileTracking) {
		// 	if the tracking label is over the fixed label
		//	only draw the fixed label while the position is not
		// 	in the fixed label - won't leak out the other side
 		if (!drawTracking
			|| (!fixedRect.Contains(trackingRect.LeftTop()) || trackingThumbIndex != kLeftThumb)
			&& (!fixedRect.Contains(trackingRect.RightTop()) || trackingThumbIndex != kRightThumb)) {
			
			DrawLabelBox(osv, fixedRect, Color(216, 216, 216), kWhiteColor, kBorderGray);
			DrawText(osv, fixedRect, false, pos, ViewColor());
		}
	}

	if (drawTracking) {
		DrawLabelBox(osv, trackingRect, kLabelFillBlue,
			kLabelTopBorderBlue, kLabelBottomBorderBlue);
		DrawText(osv, trackingRect, true, otherPos, kLabelFillBlue);
	}
}

void
TMultiThumbMediaSlider::DrawLabelBox(BView *osv, BRect rect, rgb_color fill,
	rgb_color top, rgb_color bottom)
{
	osv->PushState();
	osv->SetHighColor(fill);
	osv->FillRect(rect);
	osv->SetHighColor(bottom);
//	BPoint start, end;
//	start.x = rect.left + (rect.Width()/2) - 5;
//	start.y = end.y = rect.bottom - 1;
//	end.x = rect.left + rect.Width()/2 + 5;
//	osv->StrokeLine(start, end);
	osv->StrokeLine(rect.LeftBottom(), rect.RightBottom());
	osv->StrokeLine(rect.RightBottom(), rect.RightTop());
	osv->SetHighColor(top);
	osv->StrokeLine(rect.LeftBottom(), rect.LeftTop());
	if (fLabelPlacement == kLabelTop)
		osv->StrokeLine(rect.LeftTop(), rect.RightTop());
	osv->PopState();
}

void 
TMultiThumbMediaSlider::DrawText(BView *osv, BRect rect, bool tracking, bigtime_t playTime,
	rgb_color fillColor)
{
	int seconds = playTime / 1000000;
	int minutes	= seconds / 60;
	int hours = minutes / 60;

	minutes = minutes % 60;
	seconds = seconds % 60;
	int frame = (playTime / 30000) % 30;

	BPoint loc(rect.LeftTop());
	loc.x += 3;
	loc.y += rect.Height()/2 - kNumberStripHeight/2;
	
	BBitmap *bits = tracking ? fInvertedDigitBits : fDigitBits;

	osv->PushState();

	// hours
	float offset = DrawDigit(osv, bits, hours >= 0 ? hours / 10 : 0, loc);
	loc.x += offset + 1;
	offset = DrawDigit(osv, bits, hours >= 0 ? hours % 10 : 0, loc);
		
	// minutes
	loc.x += offset;
	offset = DrawDigit(osv, bits, -1, loc);

	loc.x += offset;
	offset = DrawDigit(osv, bits, minutes >= 0 ? minutes / 10 : 0, loc);
	loc.x += offset + 1;
	offset = DrawDigit(osv, bits, minutes >= 0 ? minutes % 10 : 0, loc);
		
	// seconds
	loc.x += offset;
	offset = DrawDigit(osv, bits, -1, loc);

	loc.x += offset;
	offset = DrawDigit(osv, bits, seconds >= 0 ? seconds / 10 : 0, loc);
	loc.x += offset + 1;
	offset = DrawDigit(osv, bits, seconds >= 0 ? seconds % 10 : 0, loc);
	
	//	frame
	loc.x += offset;
	offset = DrawDigit(osv, bits, -1, loc);

	loc.x += offset;
	offset = DrawDigit(osv, bits, frame >= 0 ? frame / 10 : 0, loc);
	loc.x += offset + 1;
	offset = DrawDigit(osv, bits, frame >= 0 ? frame % 10 : 0, loc);

	osv->PopState();
}

float
TMultiThumbMediaSlider::DrawDigit(BView *osv, const BBitmap *map, int32 digit, BPoint destLoc)
{
	BRect src;
	BRect dest;
	float digitWidth;
	
	if (digit >= 0 && digit <= 9){
		src.Set(0, 0, kNumberWidth-1, kNumberStripHeight-1);
		src.OffsetBy(kColonWidth, 0);			// skip the colon
		src.OffsetBy(kNumberWidth * digit, 0);	// skip to the desired number		
		digitWidth = kNumberWidth;
	} else {
		src.Set(0, 0, kColonWidth-1, kNumberStripHeight-1);
		digitWidth = kColonWidth;
	}

	dest.Set(destLoc.x, destLoc.y,
		destLoc.x+digitWidth-1, destLoc.y+kNumberStripHeight-1);
	
	osv->SetDrawingMode(B_OP_OVER);
	osv->SetLowColor(B_TRANSPARENT_32_BIT);
	
	osv->DrawBitmap(map, src, dest);
		
	return digitWidth;
}

void 
TMultiThumbMediaSlider::UpdatePosition(float pos, int32 thumb)
{
	if (!TrackingThumb(thumb)) {
		if (pos < 0)
			pos = 0;
		else if (pos > 1.0)
			pos = 1.0;
		
		SetValueFor((int32)((MaxValue() - MinValue()) * pos), thumb, false);
	}	
}

void 
TMultiThumbMediaSlider::UpdateTime(bigtime_t time, int32 thumb)
{
	dynamic_cast<TimeThumb *>(ThumbAt(thumb))->SetTimeValue(time);
}

void 
TMultiThumbMediaSlider::UpdateMainTime(bigtime_t time)
{
	TimeThumb *thumb = dynamic_cast<TimeThumb *>(ThumbAt(kMainThumb));
	thumb->SetTimeValue(time);
	if (TrackingThumb(kMainThumb)) 
		Invalidate(TrackingLabelRect(kMainThumb, BarFrame(), thumb->Location()));
}


void
TMultiThumbMediaSlider::SetThumbCollisionDetection(bool on)
{
	fThumbCollisionDetection = on;
}

bool 
TMultiThumbMediaSlider::ThumbCollisionDetection() const
{
	return fThumbCollisionDetection;
}
