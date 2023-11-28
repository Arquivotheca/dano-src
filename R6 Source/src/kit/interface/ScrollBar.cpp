//******************************************************************************
//
//	File:		ScrollBar.cpp
//
//	Description:	ScrollBar control implementation.
//
//	Written by:	Peter Potrebic and Benoit Schillings
//
//	Copyright 1992-2001, Be Incorporated
//
//  !!! Note to the reader.  This class does patently goofy things with the
//		fTargetName field.  The intention is that you can set fTargetName
//		before the view is attached to the Window.  Later, when any funtion
//		that needs the target is called, the ScrollBar will look for the Target
//		name - if set - and re-target the scroll bar at that time.  This 'lazy
//		target' evaluation allows LoadFromResource to work.  The exact
//		algorithm used to figure out the target is:
//
//		  If targetName is set
//			If we can find the View, set target to view, free fTargetName and
//				set it to NULL.
//			else
//				return to user.
//		  If targetName is NOT set
//			If there is a target
//				Use it.
//			else
//				assume that it will be set at some point and return to user.
//
//		If this seems strange and twisted to you, you are right - pass Go,
//			collect $200.  If it doesn't, put your analyst on danger money.
//		
//	???
//	!!! Note that right now if the Steps are set before the
//	    ScrollBar view is added to the window, the Steps
//	    will not be sent.
//******************************************************************************

#include <stdlib.h>
#include <string.h>
#include <Debug.h>

#include <token.h>
#include <messages.h>
#include <session.h>

#include <interface_misc.h>
#include <archive_defs.h>

#include <Application.h>
#include <Window.h>
#include <ScrollBar.h>

/*---------------------------------------------------------------*/

#define	VSCROLL_RULE	B_FOLLOW_TOP_BOTTOM | B_FOLLOW_RIGHT
#define	HSCROLL_RULE	B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM

namespace BPrivate
{
static int32 cast_float_to_int(float f)
{
	// deals with unusual values properly, unlike builtin cast
	if (f < (float)LONG_MIN)
		// -INFINITY
		return LONG_MIN;
	
	if (f > (float)LONG_MAX)
		// INFINITY
		return LONG_MAX;
	
   if (f < (float)LONG_MAX && f > (float)LONG_MIN)
 		// normal value
		return (int32)f;

	TRESPASS();
	// we are dealing with Nan
	return 0;
}
} using namespace BPrivate;

/*---------------------------------------------------------------*/

BScrollBar::BScrollBar(BRect view_bound, const char *name, BView *target,
	float min_v, float max_v, orientation orient)
	: BView (view_bound, name, 
		(orient == B_HORIZONTAL) ? HSCROLL_RULE : VSCROLL_RULE,
		B_WILL_DRAW)
{
	InitObject(min_v, max_v, orient, target);
}

/* ---------------------------------------------------------------- */

void BScrollBar::InitObject(float min_v, float max_v, orientation orient,
	BView *target)
{
	if (min_v < max_v) {
		fMin = min_v;
		fMax = max_v;
	} else {
		// bogus min/max values were passed in.
		fMax = 1000;
		fMin = 0;
	}

	fValue = 0;
	fProportion = 0.0;
	fOrientation = orient;
	fTargetName = NULL;
	fTarget = NULL;
	SetTarget(target);
	
	// Goofy defaults - they really don't mean anything.
	// initialize the default "steps" - must be kept in sync with app_server
	// which really defines the defaults.
	fSmallStep = 1;
	fLargeStep = 10;
}

/* ---------------------------------------------------------------- */

BScrollBar::BScrollBar(BMessage *data)
	: BView(data)
{
	float	i1, i2;
	int32	oo;

	data->FindFloat(S_RANGE, &i1);
	data->FindFloat(S_RANGE, 1, &i2);
	data->FindInt32(S_ORIENTATION, &oo);
	InitObject(i1, i2, (orientation) oo, NULL);

	// Someone else (most likely the BScrollView) will set the target view

	SetRange(i1, i2);
	data->FindFloat(S_STEPS, &i1);
	data->FindFloat(S_STEPS, 1, &i2);
	SetSteps(i1, i2);
	data->FindFloat(S_VALUE, &i1);
	SetValue(i1);

	float f;
	data->FindFloat(S_PROPORTION, &f);
	SetProportion(f);
}

/* ---------------------------------------------------------------- */

status_t BScrollBar::Archive(BMessage *data, bool deep) const
{
	status_t err = BView::Archive(data, deep);
	if (err)
		return err;

	err = data->AddFloat(S_RANGE, fMin);
	if (err)
		return err;
		
	err = data->AddFloat(S_RANGE, fMax);
	if (err)
		return err;

	err = data->AddFloat(S_STEPS, fSmallStep);
	if (err)
		return err;
		
	err = data->AddFloat(S_STEPS, fLargeStep);
	if (err)
		return err;

	err = data->AddFloat(S_VALUE, fValue);
	if (err)
		return err;
		
	err = data->AddInt32(S_ORIENTATION, fOrientation);
	if (err)
		return err;

	err = data->AddFloat(S_PROPORTION, Proportion());
	if (err)
		return err;

	// Can't archive reference to the target view.
	return B_OK;
}

/* ---------------------------------------------------------------- */

BArchivable *BScrollBar::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BScrollBar"))
		return NULL;
	return new BScrollBar(data);
}

/*---------------------------------------------------------------*/

BScrollBar::~BScrollBar()
{
	if (fTarget != NULL)
		// detach from current target
		fTarget->UnsetScroller(this);

	if(fTargetName)
		free(fTargetName);
}

/*---------------------------------------------------------------*/

void BScrollBar::AttachedToWindow()
{
	// in case the steps was set before the scrollbar was attached
	if ((fSmallStep != 1) || (fLargeStep != 10)) {
		fSmallStep-=1; fLargeStep-=1; // force update of the server
		SetSteps(fSmallStep+1, fLargeStep+1);
	}

	// in case the range was set before the scrollbar was attached
	if ((fMin != 0) || (fMax != 1000)) {
		fMin-=1; fMax-=1; // force update of the server
		SetRange(fMin+1, fMax+1);
	}

	// in case the proportion was set before the scrollbar was attached
	if (fProportion != 0) {
		fProportion-=1; // force update of the server
		SetProportion(fProportion+1);
	}

	// in case the value was set before the scrollbar was attached
	if (fValue != 0) {
		fValue-=1; // force update of the server
		SetValue(fValue+1);
	}
	BView::AttachedToWindow();	
}

/*---------------------------------------------------------------*/

void BScrollBar::Draw(BRect)
{
}

/*---------------------------------------------------------------*/

float BScrollBar::Value() const
{
	return fValue;
}

/*---------------------------------------------------------------*/

float BScrollBar::Proportion() const
{
	return fProportion;
}

/*---------------------------------------------------------------*/

void BScrollBar::GetRange(float *minAddr, float *maxAddr) const
{
	*minAddr = fMin;
	*maxAddr = fMax;
}

/*---------------------------------------------------------------*/

void BScrollBar::GetSteps(float *small, float *large) const
{
	*small = fSmallStep;
	*large = fLargeStep;
}

/*---------------------------------------------------------------*/

void BScrollBar::SetValue(float v)
{
	if (v < fMin)		v = fMin;
	else if (v > fMax)	v = fMax;
	if (v == fValue)
		return;		
	fValue = v;
	if (owner) {
		owner->Lock();
		AppSession	*a_session = owner->a_session;
		a_session->swrite_l(GR_SCROLLBAR_SET_VALUE);
		a_session->swrite_l(server_token);
		a_session->swrite_l(v);
		a_session->flush();
		owner->Unlock();
	}
	ValueChanged(v);
}

/*---------------------------------------------------------------*/

void BScrollBar::SetProportion(float p)
{
	if (p < 0.0f)		p = 0.0f;
	else if (p > 1.0)	p = 1.0f;
	if (p == fProportion)
		return;
	fProportion = p;
	if (owner) {
		owner->Lock();
		AppSession	*a_session = owner->a_session;
		a_session->swrite_l(GR_SCROLLBAR_SET_PROPORTION);
		a_session->swrite_l(server_token);
		a_session->swrite(sizeof(float), &p);
		a_session->flush();
		owner->Unlock();
	}
}

/*---------------------------------------------------------------*/

void BScrollBar::SetRange(float vmin, float vmax)
{
	if (vmin > vmax)
		return;

	// event though the API is in floats, the app_server only
	// deals with longs.
	const int imin = cast_float_to_int(vmin);
	const int imax = cast_float_to_int(vmax);
	if ((imin == fMin) && (imax == fMax))
		return;

	fMin = imin;
	fMax = imax;
	bool newValue = false;
	if (fValue < fMin) {
		fValue = fMin;
		newValue = true;
	} else if (fValue > fMax) {
		fValue = fMax;
		newValue = true;
	}
	
	if (owner) {
		owner->Lock();
		AppSession	*a_session = owner->a_session;
		a_session->swrite_l(GR_SCROLLBAR_SET_RANGE);
		a_session->swrite_l(server_token);
		a_session->swrite_l((int32)vmin);
		a_session->swrite_l((int32)vmax);
		a_session->flush();
		owner->Unlock();
	}

	if (newValue)
		ValueChanged(fValue);
}

/*---------------------------------------------------------------*/

void BScrollBar::SetSteps(float small_step, float large_step)
{
	if ((fSmallStep == (int32)small_step) && (fLargeStep == (int32)large_step))
		return;
	fSmallStep = (int32)small_step;
	fLargeStep = (int32)large_step;
	if (owner) {
		owner->Lock();
		AppSession	*a_session = owner->a_session;
		a_session->swrite_l(GR_SCROLLBAR_SET_STEPS);
		a_session->swrite_l(server_token);
		a_session->swrite_l((int32) small_step);
		a_session->swrite_l((int32) large_step);
		a_session->flush();
		owner->Unlock();
	}
}

/*---------------------------------------------------------------*/

void BScrollBar::ValueChanged(float newValue)
{
//
// Check for existence of fTargetName. If it's non-NULL, resolve it and replace
// whatever value is in fTarget.
//
	if (fTargetName && Window())
		SetTarget(Window()->FindView(fTargetName));
	if (!fTarget)
		return;		// Can happen if ValueChanged called before view is attached.

	BPoint oldPt = fTarget->LeftTop();
	
	if (fOrientation == B_VERTICAL) {
		if (newValue != oldPt.y) {
//+			PRINT(("vert: oldx=%.1f, newy=%d\n", oldPt.x, newValue));
			fTarget->ScrollTo(BPoint(oldPt.x, newValue));
		}
	} else {
		if (newValue != oldPt.x) {
//+			PRINT(("hori: newx=%.1f, oldy=%.1f\n", newValue, oldPt.y));
			fTarget->ScrollTo(BPoint(newValue, oldPt.y));
		}
	}
}

/*------------------------------------------------------------*/

void BScrollBar::SetTarget(BView *target)
{
//
// Note: free and null target name since pointers always take precedence
//
	if (fTargetName) {
		free(fTargetName);
		fTargetName = NULL;
	}

	if (fTarget) {
		// detach from current target
		fTarget->UnsetScroller(this);
	}

	fTarget = target;
	if (target == 0)
		return;

	fTarget->SetScroller(this);
}

/*------------------------------------------------------------*/

void BScrollBar::SetTarget(const char *targetName)
{
//
// Given a target view name for the scrollbar, attempt to resolve the name
// to a BView*. If this works, toss the name and save only the BView*. If it
// can't be resolved, due to non-existence of the window or view, store the
// name. The presence of data in the fTargetName field will act as a flag that
// the target view needs to be resolved.
//

	BView	*theTarget;
	BWindow *theWindow;
	
	theWindow = Window();		// Check for host window
	if(theWindow && (theTarget = theWindow->FindView(targetName)) != 0)
		SetTarget(theTarget); // Note that if fTargetName is set, this will free it.
	else {
		if(fTargetName)
			free(fTargetName);
		fTargetName = strcpy((char *)malloc(strlen(targetName)+1), targetName);
	}
}

/*------------------------------------------------------------*/

orientation	BScrollBar::Orientation() const 
{
	return fOrientation;
}

/*------------------------------------------------------------*/

BView *BScrollBar::Target() const
{
	return fTarget;
}

/*-------------------------------------------------------------*/

BScrollBar &BScrollBar::operator=(const BScrollBar &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BScrollBar::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}

/*----------------------------------------------------------------*/

status_t BScrollBar::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BScrollBar::MessageReceived(BMessage *msg)
{
	BView::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BScrollBar::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BScrollBar::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
}

/* ---------------------------------------------------------------- */

void BScrollBar::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

/* ---------------------------------------------------------------- */

void BScrollBar::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

/*---------------------------------------------------------------*/

void BScrollBar::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void BScrollBar::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
}

/* ---------------------------------------------------------------- */

void BScrollBar::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BScrollBar::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	float tw = 50, th = 50;
	if( fTarget ) fTarget->GetPreferredSize(&tw, &th);
	if( fOrientation == B_HORIZONTAL ) {
		*width = tw;
		*height = B_H_SCROLL_BAR_HEIGHT;
	} else {
		*width = B_V_SCROLL_BAR_WIDTH;
		*height = th;
	}
}

/*---------------------------------------------------------------*/

void BScrollBar::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void BScrollBar::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllAttached();
}

/*---------------------------------------------------------------*/

void BScrollBar::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllDetached();
}

/*---------------------------------------------------------------*/

status_t BScrollBar::GetSupportedSuites(BMessage *data)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	return BView::GetSupportedSuites(data);
}


void BScrollBar::_ReservedScrollBar1() {}
void BScrollBar::_ReservedScrollBar2() {}
void BScrollBar::_ReservedScrollBar3() {}
void BScrollBar::_ReservedScrollBar4() {}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
