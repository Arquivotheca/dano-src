//******************************************************************************
//
//	File:		StatusBar.cpp
//
//	Written by:	Peter Potrebic
//
//	Copyright 1996, Be Incorporated
//
//******************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <Debug.h>
#include <Font.h>
#include <Message.h>
#include <Region.h>
#include <StatusBar.h>
#include <Window.h>
#include <String.h>

#ifndef _MENU_PRIVATE_H
#include <MenuPrivate.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif

#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif

#define TOP_MARGIN		1
#define BOTTOM_MARGIN	1
#define MIDDLE_MARGIN	2

/*------------------------------------------------------------*/

BStatusBar::BStatusBar(BRect frame, const char *name, const char *label,
	const char *aux_label)
	: BView(frame, name, B_FOLLOW_LEFT + B_FOLLOW_TOP,
			B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	InitObject(label, aux_label);

	SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);

	const rgb_color blue = ui_color(B_MENU_SELECTION_BACKGROUND_COLOR);
	fBarColor = blue;
}

/*------------------------------------------------------------*/

void BStatusBar::InitObject(const char *label, const char *aux_label)
{
	fLabel = label;
	fTrailingLabel = aux_label;
	fMax = 100;
	fCurrent = 0;

	fBarHeight = -1;
	fLabelWidth = 0;
	fTrailingLabelWidth = 0;
	fTextWidth = 0;
	fTrailingTextWidth = 0;
}

/*------------------------------------------------------------*/

BStatusBar::~BStatusBar()
{
}

/* ---------------------------------------------------------------- */

BStatusBar::BStatusBar(BMessage *data)
	: BView(data)
{
	const	char *p1, *p2;
	data->FindString(S_LABEL, &p1);
	data->FindString(S_TRAILING_LABEL, &p2);
	InitObject(p1, p2);
	data->FindString(S_TEXT, &p1);
	if (p1)
		SetText(p1);
	data->FindString(S_TRAILING_TEXT, &p1);
	if (p1)
		SetTrailingText(p1);

	long	l;
	float	f;
	data->FindInt32(S_BAR_COLOR, &l);
	SetBarColor(_long_to_color_(l));

	data->FindFloat(S_MAX, &f);
	SetMaxValue(f);

	data->FindFloat(S_VALUE, &fCurrent);
	
	if( data->FindFloat(S_HEIGHT, &fBarHeight) == B_OK ) {
		fCustomBarHeight = true;
	} else {
		fCustomBarHeight = false;
	}
}

/* ---------------------------------------------------------------- */

status_t BStatusBar::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	data->AddFloat(S_HEIGHT, BarHeight());
	data->AddInt32(S_BAR_COLOR, _color_to_long_(BarColor()));
	data->AddFloat(S_VALUE, CurrentValue());
	data->AddFloat(S_MAX, MaxValue());

	if (Text())
		data->AddString(S_TEXT, Text());
	if (TrailingText())
		data->AddString(S_TRAILING_TEXT, TrailingText());
	if (Label())
		data->AddString(S_LABEL, Label());
	if (TrailingLabel())
		data->AddString(S_TRAILING_LABEL, TrailingLabel());

	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BStatusBar::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BStatusBar"))
		return NULL;
	return new BStatusBar(data);
}

/*------------------------------------------------------------*/

void BStatusBar::AttachedToWindow()
{
	// inherit view color from parent
	if (Parent()) {
		SetColorsFromParent();
	}
	else
	{
		SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
		SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	}

	// Set double buffering
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);


	fLabelWidth = StringWidth(Label());
	fTrailingLabelWidth = StringWidth(TrailingLabel());
	fTextWidth = StringWidth(Text());
	fTrailingTextWidth = StringWidth(TrailingText());
	
	Resize();

	BView::AttachedToWindow();
}

void BStatusBar::Resize()
{
	// resize to optimal height

	ASSERT(Window());

	float pwidth=0, pheight=0;
	GetPreferredSize(&pwidth, &pheight);
	
	if (Bounds().Height() != pheight)
		ResizeTo(Bounds().Width(), pheight);
}

/*------------------------------------------------------------*/

void BStatusBar::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_UPDATE_STATUS_BAR:
			{
			float d;
			const char *text = NULL;
			const char *aux_text = NULL;

			msg->FindFloat("delta", &d);

			if (msg->HasString("text"))
				msg->FindString("text", &text);

			if (msg->HasString("trailing_text"))
				msg->FindString("trailing_text", &aux_text);

			Update(d, text, aux_text);
			break;
			}
		case B_RESET_STATUS_BAR:
			{
			const char *label = NULL;
			const char *aux_label = NULL;

			if (msg->HasString("label"))
				msg->FindString("label", &label);

			if (msg->HasString("trailing_label"))
				msg->FindString("trailing_label", &aux_label);

			Reset(label, aux_label);
			break;
			}
		default:
			BView::MessageReceived(msg);
	}
}

/*------------------------------------------------------------*/

void BStatusBar::Reset(const char *label, const char *trailing_label)
{
	SetText("");
	SetTrailingText("");
	SetAndInvalidate(&fLabel, label,
					 &fLabelWidth, LabelPos(), false);
	SetAndInvalidate(&fTrailingLabel, trailing_label,
					 &fTrailingLabelWidth, TrailingLabelPos(), true);
	
	fCurrent = 0;
	fMax = 100;

	Invalidate();
	Window()->UpdateIfNeeded();
}

/*------------------------------------------------------------*/

void BStatusBar::Update(float delta, const char *text, const char *trailing_text)
{
	SetTo(fCurrent + delta, text, trailing_text);
}

/*------------------------------------------------------------*/

void BStatusBar::SetTo(float value, const char *text, const char *trailing_text)
{
	bool changed = false;

	if (value < 0.0f) value = 0.0f;
	else if (value > fMax) value = fMax;
	
	font_height fh;
	if (Window() && (text || trailing_text || fCurrent != value)) {
		GetFontHeight(&fh);
	}
	
	if (fCurrent != value) {
		BRect bar = BarRect(Bounds(), fh);
		float oldPos = BarPos(bar, fCurrent);
		fCurrent = value;
		float newPos = BarPos(bar, fCurrent);
		
		if (oldPos != newPos) {
			if (oldPos > newPos) {
				float tmp = oldPos;
				oldPos = newPos;
				newPos = tmp;
			}
			Invalidate(BRect(bar.left+oldPos-4, bar.top,
							 bar.left+newPos+4, bar.bottom));
			changed = true;
		}
	}

//+	PRINT(("Update: del=%8.2f, cur=%d, fMax=%d, text=%8s, trailing_text=%8s\n",
//+		delta, (int) fCurrent, (int) fMax,
//+		text ? text : "null", trailing_text ? trailing_text : "null"));

	if (text) {
		if (SetAndInvalidate(&fText, text,
							 &fTextWidth, TextPos(), false, &fh)) {
			changed = true;
		}
	}
	if (trailing_text) {
		if (SetAndInvalidate(&fTrailingText, trailing_text,
							 &fTrailingTextWidth, TrailingTextPos(), true, &fh)) {
			changed = true;
		}
	}

//+	PRINT(("calling _Draw(%d)\n", !full));
	if (changed) Window()->UpdateIfNeeded();
}

/*------------------------------------------------------------*/

bool BStatusBar::SetAndInvalidate(BString* into, const char* text,
								  float* width, float pos, bool trailing,
								  const font_height* fh)
{
	if (!text) text = "";
	if (*into == text) return false;
	float old_width = *width;
	*into = text;
	
	if (!Window()) return true;
	
	*width = StringWidth(into->String());
	if (*width > old_width) {
		if (trailing) pos -= (*width-old_width);
		old_width = *width;
	}
	
	InvalidateLabel(pos, old_width, fh);
	
	return true;
}

/*------------------------------------------------------------*/

void BStatusBar::InvalidateLabel(float left, float width,
								 const font_height* fh)
{
	// note: this will leave garbage if the glyphs of the current
	// font extend outside of the bounding area.
	font_height		fheight;
	if (!fh) {
		GetFontHeight(&fheight);
		fh = &fheight;
	}
	float baseline = TextBaseline(*fh);
	Invalidate(BRect(left, baseline-fh->ascent-fh->leading,
					 left+width, baseline+fh->descent));
}

/*------------------------------------------------------------*/

void BStatusBar::Draw(BRect updateRect)
{
//+	PRINT(("Update event\n"));
	_Draw(updateRect);
}


/*
	<label> <text>                      <trailing text> <trailing label>
   ---------------------------------------------------------------------
  |                                                                     |
   ---------------------------------------------------------------------

*/

/*------------------------------------------------------------*/

float BStatusBar::LabelPos() const
{
	return 2.0f;
}

float BStatusBar::TextPos() const
{
	return LabelPos() + fLabelWidth;
}

float BStatusBar::TrailingTextPos() const
{
	return TrailingLabelPos() - fTrailingTextWidth;
}

float BStatusBar::TrailingLabelPos() const
{
	return Bounds().right - 2.0f - fTrailingLabelWidth;
}

float BStatusBar::TextBaseline(const font_height& fh) const
{
	return TOP_MARGIN + fh.ascent;
}

BRect BStatusBar::BarRect(const BRect& bounds, const font_height& fh) const
{
	BRect r;
	
	r.left = bounds.left;
	r.top = floor(TextBaseline(fh) + fh.descent + MIDDLE_MARGIN + .5);
	r.bottom = floor(r.top + BarHeight() + .5);
	r.right = bounds.right;
	
	return r;
}

float BStatusBar::BarPos(const BRect& bar_rect, float value) const
{
	float pos = (value / fMax) * bar_rect.Width();
	pos = (pos < 0.0) ? 0.0 : pos;
	pos = (pos > bar_rect.Width()) ? bar_rect.Width() : pos;
	return floor(pos+.5);
}

/*------------------------------------------------------------*/

void BStatusBar::_Draw(BRect update)
{
//+	PRINT(("_Draw(bar_only=%d)\n", bar_only));
//+	PRINT_OBJECT(Bounds());
	BRect			bounds;
	BRect			r;
	font_height		fheight;
	float			baseline;
	drawing_mode	saveMode = DrawingMode();
	rgb_color		saveHigh = HighColor();
	rgb_color 		black = {0, 0, 0, 255};

	GetFontHeight(&fheight);
	bounds = Bounds();
	
	baseline = TextBaseline(fheight);
	
	r.left = r.top = 0;
	r.right = bounds.right;
	r.bottom = ceil(baseline+fheight.descent);
	
	if (update.Intersects(r))
	{
		SetDrawingMode(B_OP_OVER);
		SetHighColor(black);

		if (Label()) {
			PRINT(("Drawing label '%s' at %.2f\n",
					Label(), LabelPos()));
			MovePenTo(BPoint(LabelPos(), baseline));
			DrawString(Label());
		}
		
		const float trail_pos = TrailingTextPos();
		if (Text()) {
			// main text -- truncate if it doesn't fit.
			float pos = TextPos();
			PRINT(("Drawing text '%s' at %.2f\n",
					Text(), pos));
			MovePenTo(BPoint(pos, baseline));
			if (pos+fTextWidth >= trail_pos-10) {
				BString tmp(Text());
				TruncateString(&tmp, B_TRUNCATE_END, trail_pos-pos-10);
				DrawString(tmp.String());
			} else {
				DrawString(Text());
			}
		}
		
		if (TrailingText()) {
			PRINT(("Drawing trailing text '%s' at %.2f\n",
					TrailingText(), trail_pos));
			MovePenTo(BPoint(trail_pos, baseline));
			DrawString(TrailingText());
		}
		if (TrailingLabel()) {
			PRINT(("Drawing trailing label '%s' at %.2f\n",
					TrailingLabel(), TrailingLabelPos()));
			MovePenTo(BPoint(TrailingLabelPos(), baseline));
			DrawString(TrailingLabel());
		}
	}

	r.top = r.bottom+1.0f;
	r.bottom = bounds.bottom;
	
	if (update.Intersects(BRect(0, r.top, bounds.right, r.bottom))) {
		const BRect bar = BarRect(bounds, fheight);
		const float curPos = BarPos(r, fCurrent);
		
		rgb_color blue = fBarColor;
		rgb_color d1blue = tint_color(blue, B_DARKEN_1_TINT);
		rgb_color d2blue = tint_color(blue, B_DARKEN_2_TINT);
		rgb_color grey = {156, 156, 156, 255};
		rgb_color d1grey = tint_color(grey, B_DARKEN_1_TINT);
		rgb_color d2grey = tint_color(grey, B_DARKEN_2_TINT);
	
		SetDrawingMode(B_OP_COPY);
		SetHighColor(black);
		StrokeRoundRect(bar, 4.0, 4.0, B_SOLID_HIGH);
	
		black.alpha = 128;
		SetDrawingMode(B_OP_ALPHA);
	
		BeginLineArray(4);
		AddLine(bar.LeftTop() + BPoint(1.0, 0.0), bar.LeftTop() + BPoint(0.0, 1.0), black);
		AddLine(bar.LeftBottom() + BPoint(1.0, 0.0), bar.LeftBottom() - BPoint(0.0, 1.0), black);
		AddLine(bar.RightBottom() - BPoint(1.0, 0.0), bar.RightBottom() - BPoint(0.0, 1.0), black);
		AddLine(bar.RightTop() - BPoint(1.0, 0.0), bar.RightTop() + BPoint(0.0, 1.0), black);
		EndLineArray();
	
		BRegion	clipRgn(bar);
		BRect	exRect(bar);
		exRect.right = bar.left;
		clipRgn.Exclude(exRect);
		
		exRect.left++;
		exRect.right++;
		exRect.bottom = exRect.top + 1.0;
		clipRgn.Exclude(exRect);
	
		exRect.bottom = bar.bottom;
		exRect.top = exRect.bottom - 1.0;
		clipRgn.Exclude(exRect);
	
		exRect = bar;
		exRect.left = bar.right;
		clipRgn.Exclude(exRect);
	
		exRect.left--;
		exRect.right--;
		exRect.bottom = exRect.top + 1.0;
		clipRgn.Exclude(exRect);
	
		exRect.bottom = bar.bottom;
		exRect.top = exRect.bottom - 1.0;
		clipRgn.Exclude(exRect);
	
		ConstrainClippingRegion(&clipRgn);
		
		black.alpha = 255;
		SetDrawingMode(B_OP_COPY);
	
		BeginLineArray(6);	
		AddLine(bar.LeftTop() + BPoint(1.0, 2.0), bar.LeftBottom() + BPoint(1.0, -2.0), d2blue);
		AddLine(bar.LeftTop() + BPoint(2.0, 1.0), bar.LeftTop() + BPoint(curPos - 1.0, 1.0), d2blue);
		AddLine(bar.LeftTop() + BPoint(2.0, 2.0), bar.LeftBottom() + BPoint(2.0, -1.0), d1blue);
		AddLine(bar.LeftTop() + BPoint(2.0, 2.0), bar.LeftTop() + BPoint(curPos - 1.0, 2.0), d1blue);
		if ((bar.left + curPos) < bar.right) {
			AddLine(bar.LeftTop() + BPoint(curPos, 1.0), bar.RightTop() + BPoint(-2.0, 1.0), d2grey);
			AddLine(bar.LeftTop() + BPoint(curPos, 2.0), bar.RightTop() + BPoint(-1.0, 2.0), d1grey);
		}
		EndLineArray();
		
		BRect fillRect = bar;
		fillRect.InsetBy(1.0, 1.0);
		fillRect.top += 2.0;
		fillRect.left += 2.0;
		fillRect.right = bar.left + curPos - 1.0;

		SetHighColor(blue);
		FillRect(fillRect);
		
		BeginLineArray(3);
		fillRect.right++;
		AddLine(BPoint(fillRect.right, bar.top), BPoint(fillRect.right, bar.bottom), black);
	
		fillRect.right++;
		AddLine(BPoint(fillRect.right, bar.top + 1.0), BPoint(fillRect.right, bar.bottom - 1.0), d2grey);
	
		fillRect.right++;
		AddLine(BPoint(fillRect.right, bar.top + 2.0), BPoint(fillRect.right, bar.bottom - 1.0), d1grey);
		EndLineArray();
	
		fillRect.right++;
		fillRect.left = fillRect.right;
		fillRect.right = bar.right - 1.0;
		SetHighColor(grey);
		FillRect(fillRect);
	
		ConstrainClippingRegion(NULL);
	}
	
	SetDrawingMode(saveMode);
	SetHighColor(saveHigh);
}

/*------------------------------------------------------------*/

void BStatusBar::SetMaxValue(float max)
{
	fMax = max;
}

/*------------------------------------------------------------*/

void BStatusBar::SetBarColor(rgb_color c)
{
	if (fBarColor != c)
	{
		fBarColor = c;
		if (LockLooper())
		{
			Invalidate();
			Window()->UpdateIfNeeded();
			UnlockLooper();
		}
	}
}

/*------------------------------------------------------------*/

void BStatusBar::SetBarHeight(float h)
{
	float old = fBarHeight;
	fBarHeight = h;
	fCustomBarHeight = fBarHeight >= 0 ? true : false;
	if ((old != h) && Window())
		Resize();
}

/*------------------------------------------------------------*/

void BStatusBar::SetText(const char *text)
{
	if (SetAndInvalidate(&fText, text,
						 &fTextWidth, TextPos(), false)) {
		if (Window()) Window()->UpdateIfNeeded();
	}
}

/*------------------------------------------------------------*/

void BStatusBar::SetTrailingText(const char *text)
{
	if (SetAndInvalidate(&fTrailingText, text,
						 &fTrailingTextWidth, TrailingTextPos(), true)) {
		if (Window()) Window()->UpdateIfNeeded();
	}
}

/*------------------------------------------------------------*/

float BStatusBar::CurrentValue() const
{
	return fCurrent;
}

/*------------------------------------------------------------*/

float BStatusBar::MaxValue() const
{
	return fMax;
}

/*------------------------------------------------------------*/

float BStatusBar::BarHeight() const
{
	BStatusBar* THIS = const_cast<BStatusBar*>(this);
	
	if( THIS->fBarHeight < 0 ) {
		// if not set, GetPreferredSize() can set it for us.
		float w, h;
		THIS->GetPreferredSize(&w, &h);
	}
	return THIS->fBarHeight;
}

/*------------------------------------------------------------*/

rgb_color BStatusBar::BarColor() const
{
	return fBarColor;
}

/*------------------------------------------------------------*/

const char *BStatusBar::Text() const
{
	return fText.String();
}

/*------------------------------------------------------------*/

const char *BStatusBar::TrailingText() const
{
	return fTrailingText.String();
}

/*------------------------------------------------------------*/

const char *BStatusBar::Label() const
{
	return fLabel.String();
}

/*------------------------------------------------------------*/

const char *BStatusBar::TrailingLabel() const
{
	return fTrailingLabel.String();
}

/*-------------------------------------------------------------*/

BStatusBar &BStatusBar::operator=(const BStatusBar &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BStatusBar::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

/*----------------------------------------------------------------*/

status_t BStatusBar::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BStatusBar::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BStatusBar::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
}

/* ---------------------------------------------------------------- */

void BStatusBar::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BStatusBar::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

/* ---------------------------------------------------------------- */

void BStatusBar::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

/*---------------------------------------------------------------*/

void	BStatusBar::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BStatusBar::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
}

/*---------------------------------------------------------------*/

void BStatusBar::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BStatusBar::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	
	// The preferred height is based on the height of the font, used in the
	// label area and for the bar height, plus the space needed for the
	// margins around these.
	font_height	   fheight;
	GetFontHeight(&fheight);
	PRINT(("a=%.2f, d=%.2f, l=%.2f\n", fheight.ascent,fheight.descent,fheight.leading));

	float hh = fheight.ascent + fheight.descent;
	if (fBarHeight < 0)
		fBarHeight = fheight.ascent + fheight.descent + 6;
	PRINT(("fBarHeight=%.2f\n", fBarHeight));
	float h = hh + fBarHeight +
		(BOTTOM_MARGIN + MIDDLE_MARGIN + TOP_MARGIN);
	*height = h;
	
	// Preferred width is the space needed for the labels, with some padding
	// for the text (if it is set.)
	float w = (*Label() ? StringWidth(Label())+2 : 0)
			+ (*TrailingLabel() ? StringWidth(TrailingLabel())+2 : 0)
			+ (*Text() ? StringWidth(" WWWWW") : 0)
			+ (*TrailingText() ? StringWidth("WWW ") : 0)
			+ ( (*Label() || *Text()) && (*TrailingLabel() || *TrailingText())
				? StringWidth(" ") : 0 );
	*width = w;
}

/*---------------------------------------------------------------*/

void BStatusBar::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void BStatusBar::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllAttached();
}

/*---------------------------------------------------------------*/

void BStatusBar::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllDetached();
}

/*---------------------------------------------------------------*/

status_t BStatusBar::GetSupportedSuites(BMessage *data)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	return BView::GetSupportedSuites(data);
}

#if _R4_5_COMPATIBLE_
extern "C" {

	_EXPORT void
	#if __GNUC__
	_ReservedStatusBar1__10BStatusBar
	#elif __MWERKS__
	_ReservedStatusBar1__10BStatusBarF
	#endif
	(BStatusBar* This, float value, const char* main_text, const char* trailing_text)
	{
		This->BStatusBar::SetTo(value, main_text, trailing_text);
	}
	
}
#endif

void BStatusBar::_ReservedStatusBar2() {}
void BStatusBar::_ReservedStatusBar3() {}
void BStatusBar::_ReservedStatusBar4() {}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
