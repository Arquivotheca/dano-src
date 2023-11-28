//******************************************************************************
//
//	File:		RadioButton.cpp
//
//	Description:	Radio button class.
//
//	Written by:	Peter Potrebic & Benoit Schillings
//
//	Copyright 1992-96, Be Incorporated
//
//******************************************************************************

#include <Debug.h>

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef	_RADIO_BUTTON_H
#include "RadioButton.h"
#endif
#ifndef _FONT_H
#include <Font.h>
#endif
#ifndef _BOX_H
#include <Box.h>
#endif
#ifndef	_WINDOW_H
#include "Window.h"
#endif
#ifndef	_INTERFACE_MISC_H
#include <interface_misc.h>
#endif

#include <message_strings.h>
#ifndef _MENU_PRIVATE_H
#include "MenuPrivate.h"
#endif

static const rgb_color kWhiteColor = { 255, 255, 255, 255 };

#define BOTTOM_MARGIN	2
#define TOP_MARGIN		3

#include <string.h>

static const BRect kInvalRect( 0, 0, 18, 18 );
/*------------------------------------------------------------*/

BRadioButton::BRadioButton(BRect r, const char *name, const char *text, 
			   BMessage *message, uint32 resizeMask, uint32 flags) :
	BControl(r, name, text, message, resizeMask, flags)
{
	fChanged = 0;
	fState = 0;
	
	fGroupID = B_DEFAULT_RADIO_GROUP;

	font_height	info;
	GetFontHeight(&info);
	
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate( true );
	
	// need extra space in addition to the size of text
	float h = info.ascent + info.descent +
		(BOTTOM_MARGIN + TOP_MARGIN + info.descent);
	if (Bounds().Height() < h)
		ResizeTo(Bounds().Width(), h);
}

/*------------------------------------------------------------*/

BRadioButton::BRadioButton(BMessage *data)
	: BControl(data)
{
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate( true );
	
	fChanged = 0;
	fState = 0;
	
	fGroupID = B_DEFAULT_RADIO_GROUP;
}

/*------------------------------------------------------------*/

BRadioButton::~BRadioButton()
{
}

/*------------------------------------------------------------*/

BArchivable *BRadioButton::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BRadioButton"))
		return NULL;
	return new BRadioButton(data);
}

/*------------------------------------------------------------*/

status_t BRadioButton::Archive(BMessage *data, bool deep) const
{
	BControl::Archive(data, deep);
//+	data->AddString(B_CLASS_NAME_ENTRY, "BRadioButton");
	return 0;
}

/*------------------------------------------------------------*/

void	BRadioButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
	
	// don't start out animating
	fChanged = 0;
	
#define AUTO_RESIZE_CONTROL 0
#if AUTO_RESIZE_CONTROL
	font_height	info;
	GetFontHeight(&info);
//+	PRINT(("a=%.2f, d=%.2f, l=%.2f\n", info.ascent, info.descent, info.leading));

	// need extra space in addition to the size of text
	float h = info.ascent + info.descent +
		(BOTTOM_MARGIN + TOP_MARGIN + info.descent);
	if (Bounds().Height() < h)
		ResizeTo(Bounds().Width(), h);
#endif
}

/*------------------------------------------------------------*/
enum {
	top_i = 0,
	right_i = 1,
	bottom_i = 2,
	left_i = 3
};

#define _NU_DRAW_
//+#define _CIRCLE_DRAW_
//+#define _DIAMOND_DRAW_
//+#define _NU_DRAW_

#ifdef _NU_DRAW_

static const float kApproachRate = 0.7;
static const float kButtonRadius = 5.0;
#define VERTEX_COUNT		8
#define REFRESH_INTERVAL	5000

static const BPoint kCheckVerticies[VERTEX_COUNT] =
{
	BPoint( 8.0, 0.0 ),
	BPoint( 8.0, 0.0 ),
	BPoint( 6.0, 2.0 ),
	BPoint( -1.0, 2.0 ),
	BPoint( -2.0, 1.0 ),
	BPoint( -2.0, -1.0 ),
	BPoint( -1.0, -2.0 ),
	BPoint( 6.0, -2.0 )
};

/*static const BPoint kBoxVerticies[VERTEX_COUNT] =
{
	BPoint( 5.0, -2.0 ),
	BPoint( 5.0, 2.0 ),
	BPoint( 2.0, 5.0 ),
	BPoint( -2.0, 5.0 ),
	BPoint( -5.0, 2.0 ),
	BPoint( -5.0, -2.0 ),
	BPoint( -2.0, -5.0 ),
	BPoint( 2.0, -5.0 )
};*/

static const BPoint kCheck1Verticies[VERTEX_COUNT] =
{
	BPoint( 1.0, 0.0 ),
	BPoint( 1.0, 0.0 ),
	BPoint( 0.0, 1.0 ),
	BPoint( 0.0, 1.0 ),
	BPoint( -1.0, 0.0 ),
	BPoint( -1.0, 0.0 ),
	BPoint( 0.0, -1.0 ),
	BPoint( 0.0, -1.0 ),
};

static const BPoint kCheck2Verticies[VERTEX_COUNT] =
{
	BPoint( 2.0, -1.0 ),
	BPoint( 2.0, 1.0 ),
	BPoint( 1.0, 2.0 ),
	BPoint( -1.0, 2.0 ),
	BPoint( -2.0, 1.0 ),
	BPoint( -2.0, -1.0 ),
	BPoint( -1.0, -2.0 ),
	BPoint( 1.0, -2.0 ),
};

void	BRadioButton::Draw(BRect)
{
	BRect			bounds;
	BPoint			origin;
	BPoint			offset( 2, -2 );
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
	
	// Draw Circle
	// Draw Drop Shadow
	if( enabled && !value && !IsPressed() )
	{
		rgb_color shadow = darken;
		BPoint o = origin + offset + BPoint( 2, 2 );
		SetDrawingMode( B_OP_ALPHA );
		for( uint8 dropShadow = 2; dropShadow > 0; dropShadow--, o+=BPoint(-1,-1) )
		{
			shadow.alpha = darken.alpha >> (dropShadow-1);
			SetHighColor( shadow );
			FillEllipse( o, kButtonRadius, kButtonRadius );
		}
	}
	
	SetDrawingMode( borderColor.alpha == 255 ? B_OP_COPY : B_OP_ALPHA );
	SetHighColor( boxContentColor );
	BPoint o = origin + offset;
	float radius = IsPressed() && !value ? kButtonRadius-1.0 : kButtonRadius;
	FillEllipse( o, radius, radius );
	if( focus && enabled && !IsPressed() )
		SetPenSize( 2.0 );
	SetHighColor( borderColor );
	StrokeEllipse( o, radius, radius );
	SetPenSize( 1.0 );
	/*scale_points( IsPressed() && !value ? 0.75 : 1.0, verticies, kBoxVerticies, VERTEX_COUNT );
	translate_points( origin+offset, verticies, verticies, VERTEX_COUNT );
	draw_poly( this, verticies, VERTEX_COUNT, boxContentColor, borderColor, darken,
		focus && enabled && !value && !IsPressed() ? 2.0 : 1.0,
		enabled && !value && !IsPressed() ? 2 : 0 );
	*/
				
	state = float(fState)/255.0;
	const BPoint *srcPoly, *morphPoly;
	if( fChanged )
	{
		if( fChanged == 1 )
		{
			fChanged = 2;;
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
					focus ? 2.0 : 1.0, 2 );
	}
	else if( value )
	{
		scale_points( IsPressed() ? 0.75 : 1.0, verticies, kCheckVerticies, VERTEX_COUNT );
		translate_points( origin + offset, verticies, verticies, VERTEX_COUNT );
		draw_poly( this, verticies, VERTEX_COUNT, checkContentColor, borderColor, darken,
			focus ? 2.0 : 1.0, enabled && !IsPressed() ? 2 : 0 );
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
		InvalidateAtTime(NextFocusTime(), BRect( 0, 0, 18, bounds.bottom));
	}
	
	SetHighColor(init_high);
}

#endif

#ifdef _BITMAP_DRAW_
void	BRadioButton::Draw(BRect)
{
	BRect		bounds = Bounds();
	BRect		r;
	font_height	finfo;
	rgb_color	low = LowColor();
	rgb_color	init_high = HighColor();
	rgb_color	high = init_high;
	rgb_color	base;
	rgb_color	mark_color = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	rgb_color	white = {255, 255, 255, 255};
	float		text_baseline;
	bool		enabled = IsEnabled();
	bool		is_on = Value() != 0;
	
	// NOTE: These are no longer defined.
	BBitmap		*bm = sBitmaps[is_on][enabled];

	GetFontHeight(&finfo);

#if _COLORED_CONTROLS_
	if (low.red == 255 && low.green == 255 && low.blue == 255)
		base = ui_color(B_PANEL_BACKGROUND_COLOR);
	else
		base = low;
#else
	base = ui_color(B_PANEL_BACKGROUND_COLOR);
#endif

	if (!enabled) {
		mark_color = shift_color(mark_color, DISABLED_MARK_C);
		high = shift_color(ui_color(B_PANEL_BACKGROUND_COLOR), DISABLED_C);
	}

	text_baseline = bounds.top + TOP_MARGIN + (int)finfo.ascent;

	r.left = 0;
	r.bottom = text_baseline + 2;
	r.top = (text_baseline - ((int) finfo.ascent) - 2);
	r.right = (r.left + ((int) finfo.ascent) + 4);

	// center the radio bitmap inside this rest 'r'
	BRect	br = bm->Bounds();
	r.top = r.top + ((r.Height() - br.Height()) / 2);
	r.left = r.left + ((r.Width() - br.Width()) / 2);
	r.bottom = r.top + br.Height();
	r.right = r.left + br.Width();

	SetDrawingMode(B_OP_OVER);
	DrawBitmapAsync(bm, r.LeftTop());
	SetDrawingMode(B_OP_COPY);

	if (IsPressed()) {
		SetHighColor(shift_color(base, cDARKEN_4));
//+		StrokeArc(r, 45, 180);
//+		StrokeArc(r, 225, 180);
		StrokeEllipse(r);
	}

#if 0
	if (IsPressed()) {
		SetHighColor(shift_color(base, cDARKEN_4));
		StrokeArc(r, 45, 180);
		StrokeArc(r, 225, 180);
	} else {
		if (enabled)
			SetHighColor(shift_color(base, cDARKEN_1));
		else
			SetHighColor(base);
		StrokeArc(r, 45, 180);
		if (enabled)
			SetHighColor(white);
		else
			SetHighColor(shift_color(base, cLIGHTEN_2));
		StrokeArc(r, 225, 180);
	}

	r.InsetBy(1,1);

	if (enabled)
		SetHighColor(shift_color(base, cDARKEN_4));
	else
		SetHighColor(shift_color(base, cDARKEN_2));
	StrokeArc(r, 45, 180);
	SetHighColor(base);
	StrokeArc(r, 225, 180);

	r.InsetBy(1,1);
	if (Value()) {
		if (enabled) {
			rgb_color	dark;
			dark = shift_color(mark_color, cDARKEN_2);
			SetHighColor(dark);
			PRINT(("dark=(%d,%d,%d)\n",
				dark.red, dark.green, dark.blue));
			StrokeEllipse(r);
			r.InsetBy(1,1);
		}
		PRINT(("mark=(%d,%d,%d)\n",
			mark_color.red, mark_color.green, mark_color.blue));
		SetHighColor(mark_color);
		FillEllipse(r);
		SetHighColor(shift_color(mark_color, cDARKEN_2));
		StrokeArc(r, 225, 180);

		if (enabled)
			r.InsetBy(-1,-1);
		float d = r.Width()/4.0;
		r.InsetBy(d,d);
		SetHighColor(white);
		StrokeArc(r, 90, 90);
	} else {
#if _FILL_DISABLED_CONTROLS_
		if (!enabled)
			SetHighColor(shift_color(base, DISABLED_FILL_C));
		else
#endif
			SetHighColor(white);
		FillEllipse(r);
	}
#endif

	// --------------------- //
	// --------------------- //

	SetHighColor(high);

	MovePenTo(BPoint((int) finfo.ascent + 10, text_baseline));

	if (Label() && Label()[0]) {
		DrawString(Label());
#if 1
		BPoint	cur = PenLocation();
		bool	foc = IsFocus() && Window()->IsActive();
		if (foc)
			SetHighColor(mark_color);
		else
			SetHighColor(ViewColor());
		float f = text_baseline + finfo.descent + 0;
		StrokeLine(BPoint((int) finfo.ascent + 9, f), BPoint(cur.x, f));
		if (foc)
			SetHighColor(white);
		f++;
		StrokeLine(BPoint((int) finfo.ascent + 9, f), BPoint(cur.x, f));
#endif
	};

	SetHighColor(init_high);
}
#endif

/*------------------------------------------------------------*/
#ifdef _CIRCLE_DRAW_
void	BRadioButton::Draw(BRect)
{
	BRect		bounds;
	BRect		r;
	font_height	finfo;
	rgb_color	low = LowColor();
	rgb_color	init_high = HighColor();
	rgb_color	high = init_high;
	rgb_color	base;
	rgb_color	mark_color = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	rgb_color	white = {255, 255, 255, 255};
	float		text_baseline;
	bool		enabled;

	GetFontHeight(&finfo);

	bounds = Bounds();
	enabled = IsEnabled();

#if _COLORED_CONTROLS_
	if (low.red == 255 && low.green == 255 && low.blue == 255)
		base = ui_color(B_PANEL_BACKGROUND_COLOR);
	else
		base = low;
#else
	base = ui_color(B_PANEL_BACKGROUND_COLOR);
#endif

	if (!enabled) {
		mark_color = shift_color(mark_color, DISABLED_MARK_C);
		high = shift_color(ui_color(B_PANEL_BACKGROUND_COLOR), DISABLED_C);
	}

	text_baseline = bounds.top + TOP_MARGIN + (int)finfo.ascent;

	r.left = 0;
	r.bottom = text_baseline + 2;
	r.top = (text_baseline - ((int) finfo.ascent) - 2);
	r.right = (r.left + ((int) finfo.ascent) + 4);

	if (IsPressed()) {
		SetHighColor(shift_color(base, cDARKEN_4));
		StrokeArc(r, 45, 180);
		StrokeArc(r, 225, 180);
	} else {
		if (enabled)
			SetHighColor(shift_color(base, cDARKEN_1));
		else
			SetHighColor(base);
		StrokeArc(r, 45, 180);
		if (enabled)
			SetHighColor(white);
		else
			SetHighColor(shift_color(base, cLIGHTEN_2));
		StrokeArc(r, 225, 180);
	}

	r.InsetBy(1,1);

	if (enabled)
		SetHighColor(shift_color(base, cDARKEN_4));
	else
		SetHighColor(shift_color(base, cDARKEN_2));
	StrokeArc(r, 45, 180);
	SetHighColor(base);
	StrokeArc(r, 225, 180);

	r.InsetBy(1,1);
	if (Value()) {
		if (enabled) {
			rgb_color	dark;
			dark = shift_color(mark_color, cDARKEN_2);
			SetHighColor(dark);
			PRINT(("dark=(%d,%d,%d)\n",
				dark.red, dark.green, dark.blue));
			StrokeEllipse(r);
			r.InsetBy(1,1);
		}
		PRINT(("mark=(%d,%d,%d)\n",
			mark_color.red, mark_color.green, mark_color.blue));
		SetHighColor(mark_color);
		FillEllipse(r);
		SetHighColor(shift_color(mark_color, cDARKEN_2));
		StrokeArc(r, 225, 180);

		if (enabled)
			r.InsetBy(-1,-1);
		float d = r.Width()/4.0;
		r.InsetBy(d,d);
		SetHighColor(white);
		StrokeArc(r, 90, 90);
	} else {
#if _FILL_DISABLED_CONTROLS_
		if (!enabled)
			SetHighColor(shift_color(base, DISABLED_FILL_C));
		else
#endif
			SetHighColor(white);
		FillEllipse(r);
	}

	// --------------------- //
	// --------------------- //

	SetHighColor(high);

	MovePenTo(BPoint((int) finfo.ascent + 10, text_baseline));

	if (Label())
		DrawString(Label());

#if 1
	BPoint	cur = PenLocation();
	bool	foc = IsFocus() && Window()->IsActive();
	if (foc)
		SetHighColor(mark_color);
	else
		SetHighColor(ViewColor());
	float f = text_baseline + finfo.descent + 0;
	StrokeLine(BPoint((int) finfo.ascent + 9, f), BPoint(cur.x, f));
	if (foc)
		SetHighColor(white);
	f++;
	StrokeLine(BPoint((int) finfo.ascent + 9, f), BPoint(cur.x, f));
#endif

	SetHighColor(init_high);
}
#endif

/*------------------------------------------------------------*/

#ifdef _DIAMOND_DRAW_
void	BRadioButton::Draw(BRect update_rect)
{
	BRect		bounds;
	BRect		r;
	font_height	finfo;
	rgb_color	low = LowColor();
	rgb_color	init_high = HighColor();
	rgb_color	high = init_high;
	rgb_color	base = low;
	rgb_color	mark_color = {200, 0, 50, 255};
	long		ascent;
	float		text_baseline;
	bool		enabled;
	float		cshift = 1.0;

	GetFontHeight(&finfo);
	bounds = Bounds();
	enabled = IsEnabled();

	if (low.red == 255 && low.green == 255 && low.blue == 255)
		base = ui_color(B_PANEL_BACKGROUND_COLOR);
	else
		base = low;

	if (!enabled) {
		mark_color = shift_color(mark_color, 0.5);
		high = shift_color(high, 0.5);
		SetHighColor(high);
		cshift = 0.6;
	}

	text_baseline = bounds.top + TOP_MARGIN + finfo.descent + finfo.ascent;

	ascent = ((finfo.ascent)/2.0);
	ascent = ascent * 2;

	BPoint	dTop;
	BPoint	dRight;
	BPoint	dBottom;
	BPoint	dLeft;
	BPoint	d[4];

	dLeft.x = 0;
	dLeft.y = bounds.top + TOP_MARGIN + finfo.descent + (ascent/2.0);

	dRight.x = dLeft.x + (2 * finfo.descent) + ascent;
	dRight.y = dLeft.y;

	dTop.x = dLeft.x + finfo.descent + (ascent/2.0);
	dTop.y = bounds.top + TOP_MARGIN;

	dBottom.x = dTop.x;
	dBottom.y = dTop.y + (2 * finfo.descent) + ascent;

	BeginLineArray(8);
	d[top_i] = dTop;
	d[right_i] = dRight;
	d[bottom_i] = dBottom;
	d[left_i] = dLeft;

	if (IsPressed()) {
		base = shift_color(base, 1.4);
	}

	AddLine(d[bottom_i], d[left_i], shift_color(base, (cshift + .35)));
	AddLine(d[left_i], d[top_i], shift_color(base, (cshift + .35)));
	AddLine(d[top_i], d[right_i], shift_color(base, (cshift + .5)));
	AddLine(d[right_i], d[bottom_i], shift_color(base, (cshift + .5)));

	d[top_i].y++;
	d[right_i].x--;
	d[bottom_i].y--;
	d[left_i].x++;
	AddLine(d[bottom_i], d[left_i], shift_color(base, (cshift + .2)));
	AddLine(d[left_i], d[top_i], shift_color(base, (cshift + .2)));
	AddLine(d[top_i], d[right_i], shift_color(base, (cshift + .35)));
	AddLine(d[right_i], d[bottom_i], shift_color(base, (cshift + .35)));

	EndLineArray();

	d[top_i].y++;
	d[right_i].x--;
	d[bottom_i].y--;
	d[left_i].x++;

#if 0
	// don't fill inside of diamond with the background color
	if (Value())
		SetHighColor(mark_color);
	else
		SetHighColor(255,255,255,0);
	FillPolygon(d, 4);
#else
	// fill inside of diamond with the background color
	if (Value())
		SetHighColor(mark_color);
	else {
		rgb_color w = {255, 255, 255, 255};
		BeginLineArray(4);
		AddLine(d[bottom_i], d[left_i], w);
		AddLine(d[left_i], d[top_i], w);
		AddLine(d[top_i], d[right_i], low);
		AddLine(d[right_i], d[bottom_i], low);
		EndLineArray();

		d[top_i].y++;
		d[right_i].x--;
		d[bottom_i].y--;
		d[left_i].x++;
		SetHighColor(low);
	}
	FillPolygon(d, 4);
#endif

	SetHighColor(high);

	// --------------------- //
	// --------------------- //

	r.top = dTop.y;
	r.right = dRight.x;
	r.bottom = dBottom.y;
	r.left = dLeft.x;

	MovePenTo(BPoint(r.right + 6, text_baseline));

	if (Label())
		DrawString(Label());

	BPoint cur = PenLocation();
	if (IsFocus())
		SetHighColor(mark_color);
	else
		SetHighColor(ViewColor());
	float f = text_baseline + finfo.descent + 1;
	StrokeLine(BPoint(r.right + 5, f), BPoint(cur.x, f));

	SetHighColor(init_high);
}
#endif

/*------------------------------------------------------------*/

void	BRadioButton::MouseDown(BPoint)
{
	BRect		bound;
	ulong		buttons;
	BPoint		pt;

	if (!IsEnabled())
		return;

	bound = Bounds();
	SetPressed(true);
	Window()->UpdateIfNeeded();

	if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
//+		TRACE();
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		return;
	}

	do {
		snooze(40000);
		GetMouse(&pt, &buttons);
		if (bound.Contains(pt) != IsPressed()) {
			SetPressed(!IsPressed());
			Window()->UpdateIfNeeded();
		}
	} while(buttons);

	if (IsPressed()) {
		SetPressed(false);
		if( !Value() )
		{
			SetValue(true);
		}
		else
			Invalidate( kInvalRect );
		Window()->UpdateIfNeeded();
		Invoke();
	} else {
		Invalidate( kInvalRect );
	}
}

/*------------------------------------------------------------*/

status_t BRadioButton::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}

/*------------------------------------------------------------*/

void
BRadioButton::GetPreferredSize(
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

/*------------------------------------------------------------*/

void
BRadioButton::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}

/*-------------------------------------------------------------*/

void BRadioButton::SetValue(int32 value)
{
	if( Window() && (bool)value != (bool)Value() ) {
		fChanged = true;
	}

	// Put radio button logic here. In this way it works interactively,
	// through MouseDown, and programatically if user code directly calls
	// SetValue()
	BControl::SetValue(value);

	// if this button is turning 'on', then turn off any siblings
	if (value != 0) {
		BView			*sib = NULL;
		BView			*parent;
		BBox			*box;
		BRadioButton	*rb;

		// walk through all "children" of "parent" and turn off all
		// radio buttons. Children include BBoxes that have RadioButton labels.

		parent = Parent();
		if (parent) {
			// If we're clicking on a RadioButton that is a BBox label
			// then we'll use the BBox's parent view as the "parent"
			box = dynamic_cast<BBox*>(parent);
			if (box && (box->LabelView() == this)) {
				parent = box->Parent();
			}
			
			// now we have the real "parent". If this parent happens to be
			// a BBox with a label, we want to ignore/skip that label
			if (!parent)
				sib = Window()->ChildAt(0);
			else if ((box = dynamic_cast<BBox*>(parent)) != NULL && box->LabelView())
				sib = parent->ChildAt(1);
			else
				sib = parent->ChildAt(0);

		} else if (Window()) {
			sib = Window()->ChildAt(0);
		}

		while (sib) {
			if ((sib != this) && ((rb = dynamic_cast<BRadioButton*>(sib)) != 0) &&
				(
				 (!rb->GroupID() && !fGroupID) ||
				 (
				  (rb->GroupID() && fGroupID) && !strcmp(rb->GroupID(),fGroupID)
				 )
				)) {
				if( rb->Value() )
				{
					rb->fChanged = fChanged;
					rb->SetValue(0);
				}
			}
			else if (((box = dynamic_cast<BBox*>(sib)) != 0) &&
				((rb = dynamic_cast<BRadioButton*>(box->LabelView())) != 0)
				&& (rb != this) &&
				(
				 (!rb->GroupID() && !fGroupID) ||
				 (
				  (rb->GroupID() && fGroupID) && !strcmp(rb->GroupID(),fGroupID)
				 )
				)) {
				if( rb->Value() )
				{
					rb->fChanged = fChanged;
					rb->SetValue(0);
				}
			}

			sib = sib->NextSibling();
		}
	}
}

/*------------------------------------------------------------*/

static float make_difference(float largePri, float largeSec,
							 float smallPri, float smallSec)
{
	if( largePri < smallPri || largeSec < smallSec ) return -1;
	
	float diff = floor( largeSec - smallSec + .5 );
	diff += (largePri-smallPri)/1000000;
	
	return diff;
}

enum direction {
	kUp, kDown, kLeft, kRight
};

// Note: this currently doesn't work well if buttons are not exactly
// lined up horizontally or verically.  The make_difference() function
// should be improved to handle slightly offset objects more gracefully.
static void select_next_button(BRadioButton* cur, direction dir)
{
	BView * p = cur->Parent();
	float diff = 1000000;
	BRadioButton * rbt = NULL, *r = NULL;
	const BRect curr(cur->Frame());
	if (p) for (int ix=0; ix<p->CountChildren(); ix++) {
		r = dynamic_cast<BRadioButton *>(p->ChildAt(ix));
		if (r != NULL && r != cur && r->IsEnabled() &&
			(
			 (!r->GroupID() && !cur->GroupID()) ||
			 (
			  (r->GroupID() && cur->GroupID()) && !strcmp(r->GroupID(),cur->GroupID())
			 )
			 )) {
			const BRect newr(r->Frame());
			float d = 0;
			switch( dir ) {
				case kLeft:
					d = make_difference(curr.left, curr.top,
										newr.left, newr.top);
					break;
				case kUp:
					d = make_difference(curr.top, curr.left,
										newr.top, newr.left);
					break;
				case kRight:
					d = make_difference(newr.left, newr.top,
										curr.left, curr.top);
					break;
				case kDown:
					d = make_difference(newr.top, newr.left,
										curr.top, curr.left);
					break;
			}
			if( d > 0 && d < diff ) {
				diff = d;
				rbt = r;
			}
		}
	}
	if (rbt != NULL) {
		rbt->MakeFocus(true);
		rbt->SetValue(1);
		rbt->Invoke();
	}
}

void BRadioButton::KeyDown(const char *bytes, int32 numBytes)
{
	uchar key = bytes[0];

	switch (key) {
		case B_ENTER:
		case B_SPACE:
			// you can't 'turn off' a radio button. You can only 'turn on'
			// some other button.
			if (IsEnabled() && (Value() == 0)) {
				BControl::KeyDown(bytes, numBytes);
			}
			break;
		/*	We want to allow selection using arrow keys, too	*/
		case B_DOWN_ARROW: 
			{
				select_next_button(this, kDown);
			}
			break;
		case B_UP_ARROW:
			{
				select_next_button(this, kUp);
			}
			break;
		case B_RIGHT_ARROW:
			{
				select_next_button(this, kRight);
			}
			break;
		case B_LEFT_ARROW:
			{
				select_next_button(this, kLeft);
			}
			break;
		default:
			BControl::KeyDown(bytes, numBytes);
			break;
	}
}

/*-------------------------------------------------------------*/

BRadioButton &BRadioButton::operator=(const BRadioButton &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BRadioButton::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BControl::ResolveSpecifier(msg, index, spec, form, prop);
}

/*----------------------------------------------------------------*/

status_t BRadioButton::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BRadioButton::MessageReceived(BMessage *msg)
{
	BControl::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BRadioButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BRadioButton::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BRadioButton::MouseUp(BPoint pt)
{
	if (IsTracking()) {
		BRect	bound = Bounds();
		
		SetPressed(false);
		if (!bound.Contains(pt)) {
			Invalidate( kInvalRect );
		}
		else
		{
			if( !Value() )
			{
				SetValue(true);
			}
			else
				Invalidate( kInvalRect );
			Window()->UpdateIfNeeded();
			Invoke();
		}
		SetTracking(false);
	}
}

/* ---------------------------------------------------------------- */

void BRadioButton::MouseMoved(BPoint pt, uint32 , const BMessage *)
{
	if (IsTracking()) {
		BRect	bound = Bounds();

		if (bound.Contains(pt) != IsPressed()) {
			SetPressed(!IsPressed());
			Window()->UpdateIfNeeded();
		}
	}
}

/*---------------------------------------------------------------*/

void	BRadioButton::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BRadioButton::FrameResized(float new_width, float new_height)
{
	BControl::FrameResized(new_width, new_height);
}

/*---------------------------------------------------------------*/

void BRadioButton::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void BRadioButton::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllAttached();
}

/*---------------------------------------------------------------*/

void BRadioButton::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllDetached();
}

/*---------------------------------------------------------------*/

status_t BRadioButton::GetSupportedSuites(BMessage *data)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	return BControl::GetSupportedSuites(data);
}

/*---------------------------------------------------------------*/

void BRadioButton::SetGroupID(const char *groupID)
{
	if (fGroupID) free(fGroupID);
	fGroupID = strdup(groupID);
}

const char * BRadioButton::GroupID() const
{
	return fGroupID;
}

/*---------------------------------------------------------------*/

void BRadioButton::_ReservedRadioButton1() {}
void BRadioButton::_ReservedRadioButton2() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
