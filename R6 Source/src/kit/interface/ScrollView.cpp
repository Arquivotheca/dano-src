//******************************************************************************
//
//	File:		ScrollView.cpp
//
//	Description:	scrollable container view class implementation
//
//	Written By:	Peter Potrebic
//
//	Copyright 1992-96, Be Incorporated
//
//******************************************************************************
#include <Debug.h>

#ifndef _SCROLL_VIEW_H
#include <ScrollView.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#ifndef _INTERFACE_MISC_H
#include "interface_misc.h"
#endif
#ifndef _SCROLL_VIEW_MISC_H
#include "scroll_view_misc.h"
#endif
#include <ClassInfo.h>
#include <Message.h>

/*---------------------------------------------------------------*/

BScrollView::BScrollView(const char *name, BView *a_view, uint32 resizeMask,
	uint32 flags, bool h_scroll, bool v_scroll, border_style border)
	: BView(CalcFrame(a_view, h_scroll, v_scroll, border),
		name, resizeMask, ModFlags(flags, border))
{
	fBorder = border;
	BRect viewRect = a_view->Bounds();

	fHSB = NULL;
	fVSB = NULL;
	fHighlighted = false;

	// fPrevHeight/Width are shorts to minimize the amount of reserved space
	// used in the object.
	fPrevWidth = (uint16)viewRect.Width();
	fPrevHeight = (uint16)viewRect.Height();

	fTarget = a_view;
	a_view->TargetedByScrollView(this);

	a_view->MoveTo(0,0);

	long border_size = 0;
	if (fBorder == B_PLAIN_BORDER)
		border_size = 1;
	else if (fBorder == B_FANCY_BORDER) {
		border_size = 2;
		SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	}

	if (fBorder != B_NO_BORDER)
		a_view->MoveBy(border_size, border_size);

	AddChild(a_view);

	BRect tempRect = Bounds();
	tempRect.OffsetTo(BPoint(0, 0));
	
	const long tipAdjust = ((fBorder == B_NO_BORDER) ? 0 : (border_size));
	const long edgeAdjust = ((fBorder == B_NO_BORDER) ? 0 : (border_size-1));
	
	BRect bound;
	if (v_scroll) {
		bound.top = tipAdjust;
		bound.right = tempRect.right - edgeAdjust;
		bound.bottom = tempRect.bottom - tipAdjust;
		if (h_scroll)
			bound.bottom -= B_H_SCROLL_BAR_HEIGHT;
		bound.left = bound.right - B_V_SCROLL_BAR_WIDTH;
		
		fVSB = new TScrollBar(bound, "_VSB_", a_view, 0, 1000, B_VERTICAL);
		AddChild(fVSB);
	}

	if (h_scroll) {
		bound.bottom = tempRect.bottom - edgeAdjust;
		bound.top = bound.bottom - B_H_SCROLL_BAR_HEIGHT;
		bound.right = tempRect.right - tipAdjust;
		if (v_scroll)
			bound.right -= B_V_SCROLL_BAR_WIDTH;
		bound.left = 0 + tipAdjust;

		fHSB = new TScrollBar(bound, "_HSB_", a_view, 0, 1000, B_HORIZONTAL);
		AddChild(fHSB);
	}
}

/*---------------------------------------------------------------*/

BScrollView::~BScrollView()
{
}

/* ---------------------------------------------------------------- */

BScrollView::BScrollView(BMessage *data)
	: BView(data)
{
	long	l;
	// don't need to archive highlighted state
	fHighlighted = false;

	if (data->HasInt32(S_STYLE)) {
		data->FindInt32(S_STYLE, &l);
		fBorder = (border_style) l;
	} else
		fBorder = B_FANCY_BORDER;

	// the inherited constructor will rehydrate the target view and
	// the scroller. Just need to reset the pointers.

	// The target view should be the first view, if there was a target
	bool	has_target;
	if (data->FindBool("_no_target_", &has_target) != B_OK)
		has_target = true;

	BView	*view;
	
	if (has_target) {
		view = ChildAt(0);
		fTarget = view;
	} else {
		fTarget = NULL;
	}

	// The scroll bars are optional. See if they exist.
	TScrollBar	*sb;
	int			i = has_target ? 1 : 0;

	fHSB = fVSB = NULL;
	while ((view = ChildAt(i++)) != 0) {
		sb = cast_as(view, TScrollBar);
		if (sb && (sb->Orientation() == B_VERTICAL)) {
			sb->SetTarget(fTarget);
			fVSB = sb;
		} else if (sb && (sb->Orientation() == B_HORIZONTAL)) {
			sb->SetTarget(fTarget);
			fHSB = sb;
		}

		if (fVSB && fHSB)
			break;
	}
}

/* ---------------------------------------------------------------- */

status_t BScrollView::Archive(BMessage *data, bool deep) const
{
	// The inherited Archive() method will archive the target view
	// along with the scrollbars (if any)
	BView::Archive(data, deep);
	if (Border() != B_FANCY_BORDER)
		data->AddInt32(S_STYLE, Border());

	// archive the fact that target is missing
	if (!fTarget)
		data->AddBool("_no_target_", true);

	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BScrollView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BScrollView"))
		return NULL;
	return new BScrollView(data);
}

/*---------------------------------------------------------------*/

BRect	BScrollView::CalcFrame(BView *child, bool h_scroll, bool v_scroll,
				border_style border)
{
	/*
	 This is a static function. It is called before the constructor of
	 this class is called. For that reason I made it static so that it
	 couldn't possibly access any uninitialized data.
	*/

	if (child->Parent())
		debugger("the target view already is a child of some other view");

	BRect rect = child->Frame();

	// Need extra pixel to separate the target view from scrollbar. Otherwise
	// the views would overlap!
	if (h_scroll)
		rect.bottom++;
	if (v_scroll)
		rect.right++;

	if (border != B_NO_BORDER) {
		/*
		 A border enlarges the view by the border size. If there is a
		 corresponding scroller then we'll use the 1 pixel border of
		 the scroller as part of this view's border
		*/
		long bsize = 0;
		if (border == B_PLAIN_BORDER)
			bsize = 1;
		else if (border == B_FANCY_BORDER)
			bsize = 2;

		rect.bottom += (bsize - (h_scroll ? 1 : 0));
		rect.right += (bsize - (v_scroll ? 1 : 0));
		rect.top -= bsize;
		rect.left -= bsize;
	}
	if (v_scroll)
		rect.right += B_V_SCROLL_BAR_WIDTH;
	if (h_scroll)
		rect.bottom += B_H_SCROLL_BAR_HEIGHT;

	return(rect);
}

/*---------------------------------------------------------------*/

BView *BScrollView::Target() const
{
	return fTarget;
}


/*---------------------------------------------------------------*/

void BScrollView::SetTarget(BView *new_target)
{
	if (new_target == fTarget)
		return;

	if (fTarget) {
		fTarget->TargetedByScrollView(NULL);
		if (fTarget->Parent() == this)
			this->RemoveChild(fTarget);
	}

	fTarget = new_target;

	if (fHSB) {
		fHSB->SetTarget(fTarget);
	}
	if (fVSB) {
		fVSB->SetTarget(fTarget);
	}

	if (fTarget) {
		// This target must be the first view in the list of children
		long border_size = 0;
		if (fBorder == B_PLAIN_BORDER)
			border_size = 1;
		else if (fBorder == B_FANCY_BORDER) {
			border_size = 2;
			SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
		}
		fTarget->MoveTo(border_size, border_size);

		fTarget->TargetedByScrollView(this);
		AddChild(fTarget, ChildAt(0));
	}
}

/*---------------------------------------------------------------*/

status_t	BScrollView::ModFlags(int32 original, border_style border)
{
	if (border != B_NO_BORDER)
		return(original | B_WILL_DRAW);
	else
		return(original & ~B_WILL_DRAW);
}


/*------------------------------------------------------------*/

void	BScrollView::AttachedToWindow()
{
	/*
	 The ScrollView class makes an assumption in the constructor
	 that isn't always correct:

	 If there is only one scroll bar then size that scrollbar
	 to fill the length (height or width) of the scroll view.

	 However, if that scroll_view sits in the bottom right
	 corner of a B_DOCUMENT_WINDOW (this window has the built-in
	 resize box) then the scrollbar will overlap with the resize box.
	 By overriding AttachedToWindow() we can account for this case.
	*/

	BWindow	*window = Window();
	if (window->Look() == B_DOCUMENT_WINDOW_LOOK) {
		BRect	resizeb = window->Bounds();
		resizeb.top = resizeb.bottom - B_H_SCROLL_BAR_HEIGHT;
		resizeb.left = resizeb.right - B_V_SCROLL_BAR_WIDTH;
		if (fVSB && !fHSB) {
			BRect	vr = fVSB->Bounds();
			fVSB->ConvertToScreen(&vr);
			window->ConvertFromScreen(&vr);
			vr.bottom--;
			if (vr.Intersects(resizeb)) {
				fVSB->ResizeBy(0, resizeb.top - vr.bottom);
			}
		} else if (fHSB && !fVSB) {
			BRect	vr = fHSB->Bounds();
			fHSB->ConvertToScreen(&vr);
			window->ConvertFromScreen(&vr);
			vr.right--;		// to account for the ++ in CalcFrame
			if (vr.Intersects(resizeb)) {
				fHSB->ResizeBy(resizeb.left - vr.right, 0);
			}
		}
	}

	// Set the default draging color to gray.
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));	//152,152,152,255
	
	// We want to receive frame events
	if ((Flags() & B_FULL_UPDATE_ON_RESIZE) == 0)
		SetFlags(Flags() | B_FRAME_EVENTS);
}

/*------------------------------------------------------------*/

void	BScrollView::Draw(BRect)
{
	BRect	r = Bounds();

	if (fBorder == B_PLAIN_BORDER) {
		StrokeRect(r);
	} else if (fBorder == B_FANCY_BORDER) {

		bool foc = fHighlighted && Window()->IsActive();

		if (foc)
			SetHighColor(NextFocusColor());
		else
			SetHighColor(ui_color(B_SHINE_COLOR));
		StrokeLine(r.RightTop() + BPoint(0,1), r.RightBottom());
		StrokeLine(r.RightBottom(), r.LeftBottom() - BPoint(1,0));

		if (!foc)
			SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT)); //184, 184, 184, 255

		StrokeLine(r.LeftTop(), r.RightTop());
		StrokeLine(r.LeftTop(), r.LeftBottom());

		r.InsetBy(1,1);
		SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));	//152,152,152,255
		StrokeLine(r.LeftTop(), r.RightTop());
		StrokeLine(r.LeftTop(), r.LeftBottom());
		if (!fHSB)
			StrokeLine(r.LeftBottom(), r.RightBottom());
		if (!fVSB)
			StrokeLine(r.RightBottom(), r.RightTop());
		if (foc && !IsInvalidatePending())
			InvalidateAtTime(NextFocusTime());
	}
}

/*------------------------------------------------------------*/

void	BScrollView::SetBorder(border_style border)
{
	// ??? This never worked -- need to readjust view sizes/positions
	fBorder = border;
	if (LockLooper()) {
		Invalidate();
		if (fBorder == B_FANCY_BORDER)
			SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
		else
			SetDoubleBuffering(0);
		UnlockLooper();
	}
}

/*------------------------------------------------------------*/

BScrollBar*	BScrollView::ScrollBar(orientation flag) const
{
	if (flag == B_HORIZONTAL)
		return(fHSB);
	else
		return(fVSB);
}

/*------------------------------------------------------------*/

status_t BScrollView::SetBorderHighlighted(bool state)
{
	if (fBorder != B_FANCY_BORDER)
		return B_ERROR;

	if (state != fHighlighted) {
		fHighlighted = state;
		if (LockLooper()) {
			Draw(Bounds());
			UnlockLooper();
		}
	}

	return B_OK;
}

/*------------------------------------------------------------*/

bool BScrollView::IsBorderHighlighted() const
{
	return fHighlighted;
}

/*------------------------------------------------------------*/

border_style BScrollView::Border() const
	{ return fBorder; };

/*---------------------------------------------------------------*/

void	BScrollView::FrameResized(float new_width, float new_height)
{
	// ??? This func and BScrollView::FrameResized are exactly the same
	// perhaps this could be factored out.
	// The code in BBox::FrameResized isn't all that different either.

	// if the frame changed size then we need to force an extra update
	// event to get the frame area to draw correctly.
	BRect	b = Bounds();

	float	prev_width = (float) fPrevWidth;
	float	prev_height = (float) fPrevHeight;

	// can't trust new_width and new_height because Update events are
	// handled out of turn. This means that the Update event corresponding
	// to this FrameResized event might have already been handled. This
	// gets the redraw code out of synch.

	new_width = b.Width();
	new_height = b.Height();

	if (new_width > prev_width) {
		// need to redraw the ALL the area between the new right side and the
		// previous location of the right border
		BRect	inval = b;
		inval.right -= 1;
		inval.left = inval.right - (new_width - prev_width);
		Invalidate(inval);
	} else if (new_width < prev_width) {
		// need to force update in the area that contains the new right border
		BRect inval = b;
		inval.left = inval.right - 1;
		Invalidate(inval);
	}
	if (new_height > prev_height) {
		// need to redraw the ALL the area between the new bottom and the
		// previous location of the bottom border
		BRect	inval = b;
		inval.bottom -= 1;
		inval.top = inval.bottom - (new_height - prev_height);
		Invalidate(inval);
	} else if (new_height < prev_height) {
		// need to redraw the area the should contain the new bottom border
		BRect inval = b;
		inval.top = inval.bottom - 1;
		Invalidate(inval);
	}

	fPrevWidth = (uint16) new_width;
	fPrevHeight = (uint16) new_height;
}

/*------------------------------------------------------------*/

BScrollView &BScrollView::operator=(const BScrollView &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BScrollView::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}

/*----------------------------------------------------------------*/

status_t BScrollView::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BScrollView::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BScrollView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BScrollView::AllDetached()
{
	BView::AllDetached();
}

/* ---------------------------------------------------------------- */

void BScrollView::AllAttached()
{
	BView::AllAttached();
}

/* ---------------------------------------------------------------- */

void BScrollView::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
}

/* ---------------------------------------------------------------- */

void BScrollView::WindowActivated(bool state)
{
	BView::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BScrollView::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

/* ---------------------------------------------------------------- */

void BScrollView::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

/*---------------------------------------------------------------*/

void	BScrollView::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void BScrollView::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BScrollView::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	
	if (fTarget) fTarget->GetPreferredSize(width, height);
	else *width = *height = 0;
	
	BScrollBar* hsb = ScrollBar(B_HORIZONTAL);
	BScrollBar* vsb = ScrollBar(B_VERTICAL);
	
	// Need extra pixel to separate the target view from scrollbar. Otherwise
	// the views would overlap!
	if (vsb) (*width)++;
	if (hsb) (*height)++;

	switch (Border()) {
		case B_NO_BORDER:
			break;
			
		case B_PLAIN_BORDER:
			(*width) += 2 - (vsb ? 1 : 0);
			(*height) += 2 - (hsb ? 1 : 0);
			break;
			
		case B_FANCY_BORDER:
			(*width) += 4 - (vsb ? 1 : 0);
			(*height) += 4 - (hsb ? 1 : 0);
			break;
	}
	
	if (vsb) (*width) += B_V_SCROLL_BAR_WIDTH;
	if (hsb) (*height) += B_H_SCROLL_BAR_HEIGHT;
}

/*---------------------------------------------------------------*/

void BScrollView::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::MakeFocus(state);
}

/*---------------------------------------------------------------*/

status_t BScrollView::GetSupportedSuites(BMessage *data)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	return BView::GetSupportedSuites(data);
}

/* ---------------------------------------------------------------- */

void BScrollView::_ReservedScrollView1() {}
void BScrollView::_ReservedScrollView2() {}
void BScrollView::_ReservedScrollView3() {}
void BScrollView::_ReservedScrollView4() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/*---------------------------------------------------------------*/


TScrollBar::TScrollBar(BRect frame, const char* name, BView* target,
					   float min, float max, orientation direction)
  :	BScrollBar(frame, name, target, min, max, direction),
	fRulerView(NULL)
{
}


/* ---------------------------------------------------------------- */

TScrollBar::~TScrollBar()
{
}


/* ---------------------------------------------------------------- */

void TScrollBar::ValueChanged(float newValue)
{
	BScrollBar::ValueChanged(newValue);
	if (fRulerView)
	{
		BMessage	msg(kVALUE_MESSAGE);

		msg.AddFloat("value", newValue);
		Looper()->PostMessage(&msg, fRulerView);
	}
}

/* ---------------------------------------------------------------- */

void TScrollBar::SetRulerView(BView* view)
{
	fRulerView = view;
}
