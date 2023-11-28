//******************************************************************************
//
//	File:		CheckBox.cpp
//
//	Description:	CheckBox class.
//
//	Written by:	Peter Potrebic 
//
//	Copyright 1992-96 Be Incorporated
//
//******************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _FONT_H
#include "Font.h"
#endif
#ifndef _WINDOW_H
#include "Window.h"
#endif
#ifndef _CHECK_BOX_H
#include "CheckBox.h"
#endif
#ifndef _INTERFACE_MISC_H
#include "interface_misc.h"
#endif
#ifndef _MENU_PRIVATE_H
#include "MenuPrivate.h"
#endif

#include <message_strings.h>
#include <math.h>

static const rgb_color kWhiteColor = { 255, 255, 255, 255 };

#define BOTTOM_MARGIN	3
#define TOP_MARGIN		3

static const BRect kInvalRect( 0, 0, 18, 18 );

/*------------------------------------------------------------*/

BCheckBox::BCheckBox(BRect r, const char *name, const char *text,
				BMessage *message, uint32 resizeMask, uint32 flags) :
	BControl(r, name, text, message, resizeMask, flags)
{
	fChanged = 0;
	fState = 0;
	
	font_height	info;
	GetFontHeight(&info);

	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate( true );
	
	// need extra space in addition to the size of text
	float h = info.ascent + info.descent +
		(BOTTOM_MARGIN + TOP_MARGIN);
	if (Bounds().Height() < h)
		ResizeTo(Bounds().Width(), h);
}

/*------------------------------------------------------------*/

BCheckBox::BCheckBox(BMessage *data)
	: BControl(data)
{
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate( true );
	fChanged = 0;
	fState = 0;
}

/*------------------------------------------------------------*/

BCheckBox::~BCheckBox()
{
}

/*------------------------------------------------------------*/

BArchivable *BCheckBox::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BCheckBox"))
		return NULL;
	return new BCheckBox(data);
}

/*------------------------------------------------------------*/

status_t BCheckBox::Archive(BMessage *data, bool deep) const
{
	return BControl::Archive(data, deep);
//+	return data->AddString(B_CLASS_NAME_ENTRY, "BCheckBox");
}

/*------------------------------------------------------------*/

void	BCheckBox::AttachedToWindow()
{
//+	PRINT(("BCheckBox::AttachedToWindow(%s), window=%x, Parent=%x\n",
//+		Label(), Window(), Parent()));

	// inherit view color from parent
	BControl::AttachedToWindow();
	
	// don't start out animating
	fChanged = 0;
	
	/*
	// Disable double buffering if parent is set to draw on children
	// Double buffering does not work with this yet
	// Remember to remove it when it does
	if( Parent() && (Parent()->Flags() & B_DRAW_ON_CHILDREN) )
		SetDoubleBuffering(0);
	
	SetViewColor( B_TRANSPARENT_COLOR );
	*/
	
	
#define AUTO_RESIZE_CONTROL 0
#if AUTO_RESIZE_CONTROL
	font_height	info;
	GetFontHeight(&info);
//+	PRINT(("size=%.2f, a=%.2f, d=%.2f, l=%.2f\n", info.size, info.ascent, info.descent, info.leading));

	// need extra space in addition to the size of text
	float h = info.ascent + info.descent +
		(BOTTOM_MARGIN + TOP_MARGIN);
	if (Bounds().Height() < h)
		ResizeTo(Bounds().Width(), h);
#endif
}

/*------------------------------------------------------------*/

static const float kApproachRate = 0.6;
#define VERTEX_COUNT		11
#define REFRESH_INTERVAL	5000

static const BPoint kCheckVerticies[VERTEX_COUNT] =
{
	BPoint( 10.0, -8.0 ),
	BPoint( 10.0, -6.0 ),
	BPoint( 1.0, 3.0 ),
	BPoint( -1.0, 3.0 ),
	BPoint( -4.0, -2.0 ),
	BPoint( -4.0, -3.0 ),
	BPoint( -2.0, -5.0 ),
	BPoint( -1.0, -5.0 ),
	BPoint( 1.0, -2.0 ),
	BPoint( 8.0, -9.0 ),
	BPoint( 9.0, -9.0 ),
};

static const BPoint kBoxVerticies[VERTEX_COUNT] =
{
	BPoint( -2.0, -6.0 ),
	BPoint( -2.0, -6.0 ),
	BPoint( 5.0, -6.0 ),
	BPoint( 6.0, -5.0 ),
	BPoint( 6.0, 2.0 ),
	BPoint( 6.0, 2.0 ),
	BPoint( 6.0, 2.0 ),
	BPoint( 5.0, 3.0 ),
	BPoint( -2.0, 3.0 ),
	BPoint( -3.0, 2.0 ),
	BPoint( -3.0, -5.0 ),
};

static const BPoint kCheck1Verticies[VERTEX_COUNT] =
{
	BPoint( 0.0, -1.0 ),
	BPoint( 0.0, -1.0 ),
	BPoint( -1.0, 0.0 ),
	BPoint( -2.0, 0.0 ),
	BPoint( -4.0, -2.0 ),
	BPoint( -4.0, -3.0 ),
	BPoint( -2.0, -5.0 ),
	BPoint( -1.0, -5.0 ),
	BPoint( 0.0, -4.0 ),
	BPoint( 0.0, -4.0 ),
	BPoint( 0.0, -4.0 ),
};

static const BPoint kCheck2Verticies[VERTEX_COUNT] =
{
	BPoint( 5.0, -2.0 ),
	BPoint( 5.0, -1.0 ),
	BPoint( 1.0, 3.0 ),
	BPoint( -1.0, 3.0 ),
	BPoint( -4.0, -2.0 ),
	BPoint( -4.0, -3.0 ),
	BPoint( -2.0, -5.0 ),
	BPoint( -1.0, -5.0 ),
	BPoint( 1.0, -2.0 ),
	BPoint( 3.0, -4.0 ),
	BPoint( 3.0, -4.0 ),
};

void	BCheckBox::Draw(BRect)
{
	BRect			bounds;
	BPoint			origin;
	BPoint			offset( 0, -2 );
	BPoint			verticies[VERTEX_COUNT];
	int32			value;
	rgb_color		boxContentColor, checkContentColor;
	rgb_color		parentColor;
	rgb_color		borderColor = ui_color(B_CONTROL_BORDER_COLOR);
	rgb_color		darken = { 0, 0, 0, 60 };
	rgb_color		init_high = HighColor();
	BView			*parent;
	bool			enabled;
	bool			focus;
	float			state;
	font_height		finfo;
	float			text_baseline;
	bool			parentDraw = true;
	
	value = Value();
	bounds = Bounds();
	parent = Parent();
	enabled = IsEnabled();
	GetFontHeight( &finfo );
	text_baseline = bounds.top + TOP_MARGIN + finfo.ascent;
	origin.x = 4.0;
	origin.y = text_baseline - 2.0;
	focus = IsFocus() && Window()->IsActive();
	
	if (focus) borderColor = NextFocusColor();
	
	// Setup Colors
	if( parent )
	{
		parentColor = parent->ViewColor();
		if( parentColor == B_TRANSPARENT_COLOR )
			parentColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	}
	else
		parentColor.set_to( 255, 255, 255 );
	
	// Should we erase the background
	/*if( !parent || (parent && !(parent->Flags() & B_DRAW_ON_CHILDREN)) )
	{
		parentDraw = false;
		SetDrawingMode( B_OP_COPY );
		SetHighColor( parentColor );
		FillRect( Bounds() );
	}*/
	
	if( enabled )
	{
		checkContentColor = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
		boxContentColor = ui_color(B_CONTROL_BACKGROUND_COLOR);
	}
	else
	{
		checkContentColor = parentColor;
		boxContentColor = parentColor;
		borderColor.disable(B_TRANSPARENT_COLOR);
		SetDrawingMode( B_OP_ALPHA );
	}
	
	// Draw Box
	scale_points( IsPressed() && !value ? 0.75 : 1.0, verticies, kBoxVerticies, VERTEX_COUNT );
	translate_points( origin, verticies, verticies, VERTEX_COUNT );
	draw_poly( this, verticies, VERTEX_COUNT, boxContentColor, borderColor, darken,
		1.0, enabled && !value && !IsPressed() ? 2 : 0 );
	if( focus && enabled && !IsPressed() )
	{
		BRect focusRect( verticies[8].x, verticies[0].y+1, verticies[3].x-1, verticies[5].y );
		SetHighColor( borderColor );
		StrokeRect( focusRect );
	}
	
	state = float(fState)/255.0;
	const BPoint *srcPoly, *morphPoly;
	if( fChanged )
	{
		if( fChanged == 1 )
		{
			fChanged = 2;
			state = 1.0;
		}
		
		if( fChanged == 2 )
		{
			morphPoly = !value ? kCheckVerticies : kCheck1Verticies;
			srcPoly = kCheck2Verticies;
		}
		else
		{
			morphPoly = kCheck2Verticies;
			srcPoly = value ? kCheckVerticies : kCheck1Verticies;
		}
		morph_points( state, verticies, srcPoly, morphPoly, VERTEX_COUNT );
		translate_points( origin + offset, verticies, verticies, VERTEX_COUNT );
		draw_poly( this, verticies, VERTEX_COUNT, checkContentColor, borderColor, darken,
			focus && enabled && !IsPressed() ? 2.0 : 1.0, 2 );
	}
	else if( value )
	{
		scale_points( IsPressed() ? 0.75 : 1.0, verticies, kCheckVerticies, VERTEX_COUNT );
		translate_points( origin + offset, verticies, verticies, VERTEX_COUNT );
		draw_poly( this, verticies, VERTEX_COUNT, checkContentColor, borderColor, darken,
			focus && enabled && !IsPressed() ? 2.0 : 1.0, enabled && !IsPressed() ? 2 : 0 );
	}
	
	if (Label())
	{
		rgb_color text_col(init_high);
		
		if( parentDraw ) {
			if( !enabled ) {
				SetDrawingMode( B_OP_ALPHA );
				text_col.disable(B_TRANSPARENT_COLOR);
			} else {
				SetDrawingMode( B_OP_OVER );
			}
		} else {
			SetDrawingMode( B_OP_COPY );
			if( !enabled ) text_col.disable(parentColor);
		}
		
		SetHighColor( text_col );
		MovePenTo(BPoint(finfo.ascent + 10, text_baseline));
		DrawString(Label());
	}
	
	float goal = 0.0;
	if( state != goal )
	{
		// State should approach goal
		float delta = goal - state;
		if( fabs( delta ) < 0.05 )
		{
			if( fChanged == 2 )
			{
				fChanged = 3;
				state = 1.0 - kApproachRate;
			}
			else if( fChanged >= 3 )
			{
				fChanged = 0;
				state = goal;
			}
			else
				state = goal;
		}
		else
			state += delta * (fChanged ? kApproachRate : 1.0);
		
		// Store state in uint8
		fState = uint8(state * 255.0);
		
		DelayedInvalidate( REFRESH_INTERVAL, BRect( 0, 0, 18, bounds.bottom) );
	} else if (focus && !IsInvalidatePending()) {
		InvalidateAtTime( NextFocusTime(), BRect( 0, 0, 18, bounds.bottom) );
	}
	
	SetHighColor(init_high);
}

/*------------------------------------------------------------*/

void	BCheckBox::MouseDown(BPoint where)
{
	BRect		bound;
	ulong		buttons;

	if (!IsEnabled())
		return;

	bound = Bounds();
	SetPressed(true);
	Window()->UpdateIfNeeded();
	
	if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		return;
	}

	do {
		snooze(40000);
		GetMouse(&where, &buttons);
		if (bound.Contains(where) != IsPressed()) {
			SetPressed(!IsPressed());
			Window()->UpdateIfNeeded();
		}
	} while(buttons);

	if (IsPressed()) {
		SetPressed(false);
		SetValue(!Value());
		Window()->UpdateIfNeeded();
		Invoke();
	}
	else {
		//Draw(bound);
		//Flush();
		Invalidate( kInvalRect );
	}
}

/*------------------------------------------------------------*/

void	BCheckBox::SetValue(int32 val)
{
	if( Window() && (bool)val != (bool)Value() ) {
		fChanged = true;
	}
	BControl::SetValue(val);
}

/*------------------------------------------------------------*/

status_t	BCheckBox::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}

/*-------------------------------------------------------------*/

BCheckBox &BCheckBox::operator=(const BCheckBox &) { return *this; }

/*-------------------------------------------------------------*/

BHandler *BCheckBox::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BControl::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BCheckBox::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BCheckBox::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void
BCheckBox::GetPreferredSize(
	float	*width,
	float	*height)
{
	BFont theFont;
	GetFont(&theFont);

	font_height info;
	theFont.GetHeight(&info);

	*width =  ceil(info.ascent + 10.0 +
		(Label() ? theFont.StringWidth(Label()) : 0) + 2.0);
	*height = ceil(info.ascent + info.descent + BOTTOM_MARGIN + TOP_MARGIN);
}

/*-------------------------------------------------------------*/

void
BCheckBox::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}

/* ---------------------------------------------------------------- */

void BCheckBox::MessageReceived(BMessage *msg)
{
	BControl::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BCheckBox::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BCheckBox::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BCheckBox::KeyDown(const char *bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
}

/* ---------------------------------------------------------------- */

void BCheckBox::MouseUp(BPoint pt)
{
	if (IsTracking()) {
		//BRect	bound = Bounds();
//+		PRINT(("MouseUp(%.1f, %.1f)\n", pt.x, pt.y));
		if (Bounds().Contains(pt) != IsPressed()) {
			SetPressed(!IsPressed());
			Window()->UpdateIfNeeded();
		}
		if (IsPressed()) {
			SetPressed(false);
			SetValue(!Value());
			Window()->UpdateIfNeeded();
			Invoke();
		} else {
			Invalidate( kInvalRect );
		}
		SetTracking(false);
	}
}

/* ---------------------------------------------------------------- */

void BCheckBox::MouseMoved(BPoint pt, uint32 , const BMessage *)
{
	if (IsTracking()) {
		BRect	bound = Bounds();
		if (bound.Contains(pt) != IsPressed()) {
			SetPressed(!IsPressed());
			Window()->UpdateIfNeeded();
		}
//+		PRINT(("MouseMoved(%.1f, %.1f, code=%d)\n", pt.x, pt.y, code));
	}
}

/*---------------------------------------------------------------*/

void	BCheckBox::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BCheckBox::FrameResized(float new_width, float new_height)
{
	BControl::FrameResized(new_width, new_height);
}

/*---------------------------------------------------------------*/

void	BCheckBox::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void	BCheckBox::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllAttached();
}

/*---------------------------------------------------------------*/

void	BCheckBox::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllDetached();
}

/* ---------------------------------------------------------------- */

void BCheckBox::_ReservedCheckBox1() {}
void BCheckBox::_ReservedCheckBox2() {}
void BCheckBox::_ReservedCheckBox3() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
