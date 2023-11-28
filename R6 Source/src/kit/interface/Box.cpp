//*****************************************************************************
//
//	File:		Box.cpp
//
//	Description:	BBox class.
//
//	Written by:	Peter Potrebic
//
//	Copyright 1992-96, Be Incorporated
//
//*****************************************************************************

#include <Debug.h>
#include <stdlib.h>
#include <string.h>

#include <interface_misc.h>
#include <archive_defs.h>
#include <Control.h>
#include <Font.h>
#include <MenuPrivate.h>
#include <Picture.h>
#include <Window.h>
#include <Region.h>
#include <StreamIO.h>

#include "Box.h"

namespace BPrivate {
	const float kTextOffset = 3.0;
	struct _private_t
	{
		BRect fLabelBoundingBox;
	};
} using namespace BPrivate;
#define m (*static_cast<_private_t *>(m_private))

/*---------------------------------------------------------------*/

BBox::BBox(BRect bounds, const char *name, uint32 flags, uint32 drawFlags,
	border_style style)
	: BView(bounds, name, flags, drawFlags)
{
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
	InitObject();
	fStyle = style;
}

/*---------------------------------------------------------------*/

BBox::~BBox(void)
{
	SetLabel((BView *)NULL);
	delete static_cast<_private_t *>(m_private);
}

/* ---------------------------------------------------------------- */

BBox::BBox(BMessage *data)
	: BView(data)
{
	const char	*str;
	InitObject(data);
	if (data->FindString(S_LABEL, &str) == B_OK) {
		SetLabel(str);
	}

	// compatibility for old bug in R2
	bool	plain;
	if (data->FindBool(S_STYLE, &plain) == B_OK) {
		fStyle = plain ? B_PLAIN_BORDER : B_NO_BORDER;
	}
	// end of fix

	long	style;
	if (data->FindInt32(S_STYLE, &style) == B_OK) {
		fStyle = (border_style) style;
	}

	bool	has_view;
	if (data->FindBool("_lblview", &has_view) == B_OK) {
		ASSERT(has_view);
		fLabelView = dynamic_cast<BView*>(ChildAt(0));
	}	
}

/* ---------------------------------------------------------------- */

void BBox::InitObject(BMessage *data)
{
	m_private = (void *)(new _private_t);
	fLabel = NULL;
	fLabelView = NULL;
	fBounds = Bounds();
	fStyle = B_FANCY_BORDER;
	
	BFont aFont(*be_bold_font);
	uint32 mask = B_FONT_SPACING;
	//aFont.SetSpacing(B_STRING_SPACING);
	
	if (!data || !data->HasString(S_FONT_FAMILY_STYLE))
		mask |= B_FONT_FAMILY_AND_STYLE;
	if (!data || !data->HasFloat(S_FONT_FLOATS))
		mask |= B_FONT_SIZE;

	SetFont(&aFont, mask);
	
	if ((Flags() & B_FULL_UPDATE_ON_RESIZE) == 0)
		SetFlags(Flags() | B_FRAME_EVENTS);

	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
}

/* ---------------------------------------------------------------- */

status_t BBox::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	if (fLabel)
		data->AddString(S_LABEL, fLabel);
	if (fLabelView) {
		data->AddBool("_lblview", true);
	}

	if (fStyle != B_FANCY_BORDER) {
		data->AddInt32(S_STYLE, fStyle);
	}
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BBox::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BBox"))
		return NULL;
	return new BBox(data);
}

/*---------------------------------------------------------------*/

void BBox::FrameResized(float new_width, float new_height)
{
	// if the frame changed size then we need to force an extra update
	// event to get the frame area to draw correctly.
	const BRect b = Bounds();

	// can't trust new_width and new_height because Update events are
	// handled out of turn. This means that the Update event corresponding
	// to this FrameResized event might have already been handled. This
	// gets the redraw code out of synch.

	new_width = b.Width();
	new_height = b.Height();

	if (fStyle != B_NO_BORDER) {
		if (new_width > fBounds.Width()) {
			// need to redraw the ALL the area between the new right side and the
			// previous location of the right border
			BRect inval = b;
			inval.right -= 1;
			inval.left = fBounds.right - 2;
			Invalidate(inval);
		} else if (new_width < fBounds.Width()) {
			// need to force update in the area that contains the new right border
			BRect inval = b;
			inval.left = inval.right - 2;
			Invalidate(inval);
		}
		if (new_height > fBounds.Height()) {
			// need to redraw the ALL the area between the new bottom and the
			// previous location of the bottom border
			BRect inval = b;
			inval.bottom -= 1;
			inval.top = fBounds.bottom - 2;
			Invalidate(inval);
		} else if (new_height < fBounds.Height()) {
			// need to redraw the area that should contain the new bottom border
			BRect inval = b;
			inval.top = inval.bottom - 2;
			Invalidate(inval);
		}
	}
	
	fBounds = b;
}

/* ---------------------------------------------------------------- */

void BBox::AllAttached()
{
	BView::AllAttached();
	SetUpLabelView();
}

/* ---------------------------------------------------------------- */

void BBox::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetUpColors();
}

/* ---------------------------------------------------------------- */

void BBox::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BBox::AllDetached()
{
	BView::AllDetached();
}

/*---------------------------------------------------------------*/

void BBox::GetHeaderInfo(font_height* out_finfo, float* out_bottom) const
{
	const char *theText = Label();
	const bool hasTextLabel = (theText && *theText);
	
	if (hasTextLabel || fLabelView) {
		if (hasTextLabel)
			GetFontHeight(out_finfo);
		*out_bottom = ceil((hasTextLabel)
							? (out_finfo->ascent+out_finfo->descent)
							: fLabelView->Bounds().Height()+1);
	} else {
		*out_bottom = 0;
	}
}

/*---------------------------------------------------------------*/

void BBox::Draw(BRect)
{
	PushState();
	
//+	PRINT(("DRAW ")); PRINT_OBJECT(Bounds());
	BRect b = Bounds();
	const char *theText = Label();
	const bool hasTextLabel = (theText && *theText);
	const bool hasLabel = hasTextLabel || fLabelView;
	const rgb_color kBlackFill = { 0, 0, 0, 64 };
	const rgb_color kBlackBorder = { 0, 0, 0, 64 };
	const bool fillTop = (fStyle == B_FANCY_BORDER && hasLabel);
	BPoint labelPosition(B_ORIGIN);
	const BRect boundingBox = m.fLabelBoundingBox;
	
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	
	BRect labelRect;
	if (hasLabel) {
		labelPosition = B_ORIGIN - boundingBox.LeftTop();
		labelRect = ((hasTextLabel) ? (boundingBox) : (fLabelView->Bounds()));
		labelRect.OffsetTo(6, 0);
		if (hasTextLabel) labelRect.right += kTextOffset*2;
		b.top = labelRect.Height() * 0.5;
	}
	
	// The following seems a little overkill, however it is necessary if we want the
	// BBox to print correctly (we must deal with surfaces and not pixels)
	// Anyway, it is quite fast to draw
	if (fillTop)
	{
		// The height of the filled region is 5, thus we need to start up by 2 pixels
		b.top -= 2;

		// Build the region in which we don't want to draw, store it into a BPicture
		BRect r = b;
		r.top += 5;
		BPicture pic;
		BeginPicture(&pic);
		FillRoundRect(r, 3.0, 3.0);
		if (hasLabel)
			FillRect(labelRect);
		EndPicture();
		
		// Clip this region (we want to draw every where excepted here)
		ClipToInversePicture(&pic);

		// This fill rect, will draw filled part of the BBox (upper) minus the label area
		SetHighColor(kBlackFill);
		FillRoundRect(b, 3.0, 3.0);

		// Now Reverse the clipping region: We want to draw everywhere inside the BBox
		// excepted the filled upper region
		ConstrainClippingRegion(NULL);
		BeginPicture(&pic);
		FillRoundRect(r, 3.0, 3.0);
		EndPicture();
		ClipToPicture(&pic);
	}
	
	if ((hasLabel) && (!fillTop))
	{ // if we have a label and no fill region, we must clip the label, because the clipping region has not been set, yet.
		BRegion region;
		GetClippingRegion(&region);
		region.Exclude(labelRect);
		ConstrainClippingRegion(&region);
	}

	// Draw the thin line delimiting the BBox
	if (fStyle != B_NO_BORDER)
	{
		SetHighColor(kBlackBorder);
		if (Parent()) {
			// Embedded within another view
			StrokeRoundRect(b, 3.0, 3.0);
		} else {
			// Used as top-level view -- don't want border to "pull
			// out" from window.
			StrokeRect(b);
		}		
	}

	// Eventually draw the label
	if (hasTextLabel) {
		ConstrainClippingRegion(NULL);
		SetDrawingMode(B_OP_OVER);
		SetHighColor(make_color(0, 0, 0, 255));
		DrawString(theText, BPoint(labelRect.left + kTextOffset + labelPosition.x, labelRect.top + labelPosition.y));
	}
	
	PopState();
}

/*---------------------------------------------------------------*/

BInsets BBox::FrameInset() const
{
	font_height finfo;
	float bottom;
	GetHeaderInfo(&finfo, &bottom);
	
	if (fStyle != B_NO_BORDER) {
		return BInsets(2, 2+bottom, 2, 2);
	}
	
	return BInsets();
}

/*---------------------------------------------------------------*/

void BBox::ClearAnyLabel()
{
	if (fLabel) {
		ASSERT(!fLabelView);
		free(fLabel);
		fLabel = NULL;
	} else if (fLabelView) {
		ASSERT(!fLabel);
		fLabelView->RemoveSelf();
		delete fLabelView;
		fLabelView = NULL;
	}
}

/*---------------------------------------------------------------*/

void BBox::SetUpColors()
{
	SetColorsFromParent();
}

/*---------------------------------------------------------------*/

void BBox::SetUpLabelView()
{
	if (fLabelView) {
		fLabelView->SetColorsFromParent();
	}
}

/*---------------------------------------------------------------*/

BView *BBox::LabelView() const
{
	return fLabelView;
}

/*---------------------------------------------------------------*/

status_t BBox::SetLabel(BView *view)
{
	ClearAnyLabel();
	if (view) {
		fLabelView = view;

		view->ResizeToPreferred();
		view->MoveTo(kTextOffset, 1);

		AddChild(view, ChildAt(0));		// add this as first child
		SetUpLabelView();
	}

	if (LockLooper()) {
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
	
	return B_OK;
}

/*---------------------------------------------------------------*/

const char *BBox::Label() const
{
	return fLabel;
}

/*---------------------------------------------------------------*/

void BBox::SetLabel(const char *label)
{
	ClearAnyLabel();
	if (label) {
		fLabel = strdup(label);
		m.fLabelBoundingBox.Set(0,0,-1,-1);
		if ((fLabel) && (*fLabel)) {
			BFont font;
			GetFont(&font);
			escapement_delta ed;
			ed.nonspace = 0;
			ed.space = 0;
			font.GetBoundingBoxesForStrings((const char **)&fLabel, 1, B_SCREEN_METRIC, &ed, &m.fLabelBoundingBox);
		}
	}

	if (LockLooper()) {
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

/*---------------------------------------------------------------*/

void BBox::SetBorder(border_style style)
{
	fStyle = style;
	if (LockLooper()) {
		SetUpLabelView();
		Invalidate();
		Window()->UpdateIfNeeded();
		UnlockLooper();
	}
}

/*---------------------------------------------------------------*/

border_style BBox::Border() const
{
	return fStyle;
}

/*-------------------------------------------------------------*/

BHandler *BBox::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}

/*----------------------------------------------------------------*/

status_t BBox::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BBox::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
}

/* ---------------------------------------------------------------- */

void BBox::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BBox::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

/* ---------------------------------------------------------------- */

void BBox::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

/* ---------------------------------------------------------------- */

void BBox::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

/*---------------------------------------------------------------*/

void	BBox::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void
BBox::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BBox::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here in the future must consider the implications of that fact. Only
	 a concern on PPC, as Intel compatibility was broken in R4.
	*/
	BView::GetPreferredSize(width, height);
}


/*---------------------------------------------------------------*/

void	BBox::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here in the future must consider the implications of that fact. Only
	 a concern on PPC, as Intel compatibility was broken in R4.
	*/
	BView::MakeFocus(state);
}

/*---------------------------------------------------------------*/

status_t	BBox::GetSupportedSuites(BMessage *data)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here in the future must consider the implications of that fact. Only
	 a concern on PPC, as Intel compatibility was broken in R4.
	*/
	return BView::GetSupportedSuites(data);
}

/*---------------------------------------------------------------*/

BBox &BBox::operator=(const BBox &) {return *this;}

/* ---------------------------------------------------------------- */

void BBox::_ReservedBox1() {}
void BBox::_ReservedBox2() {}

/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
/*---------------------------------------------------------------*/
