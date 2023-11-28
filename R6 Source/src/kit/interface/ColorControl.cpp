/* ++++++++++

   FILE:  ColorControl.cpp
   DATE:  Wed May  8 01:01:54 PDT 1996

   Written By: Peter Potrebic and Robert Polic

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */
#include <Debug.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _REGION_H
#include <Region.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _COLOR_CONTROL_H
#include <ColorControl.h>
#endif
#ifndef _FONT_H
#include <Font.h>
#endif
#ifndef _SCREEN_H
#include <Screen.h>
#endif
#ifndef _TEXT_CONTROL_H
#include <TextControl.h>
#endif
#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#ifndef _MENU_PRIVATE_H
#include <MenuPrivate.h>
#endif
#include <StopWatch.h>

#include <math.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <TextView.h>
#include <string.h>

/*------------------------------------------------------------*/

#define TEXT_ENTRY_AREA_WIDTH	45
#define OLD_TEXT_MARGIN			70 // Used to ensure the control is the same size
#define TEXT_MARGIN				2
#define LABEL_WIDTH				11.0
#define	CMD_CHANGE_COLOR		'ccol'
#define	MIN_CELL_SIZE			6
#define REFRESH_INTERVAL		30000
#define COLOR_AREA_H_FRAME		2
#define COLOR_AREA_V_FRAME		2

#define	LEFT_PAD	5
#define	RIGHT_PAD	5
#define FIELD_HEIGHT		16
static rgb_color long_as_rgb(long color);

/*------------------------------------------------------------*/

BColorControl::BColorControl(BPoint start, color_control_layout layout,
	float size, const char *name, BMessage *msg, bool use_offscreen)
	: BControl(BColorControl::CalcFrame(start, layout, size),
		name, "", msg, B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE)
{
	InitData(layout, size, use_offscreen, NULL);
}

/*------------------------------------------------------------*/

void BColorControl::InitData(color_control_layout layout, float size,
	bool use_offscreen, BMessage *data)
{
	BRect 		r(Bounds());
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate( true );
	
	fColumns = (long) layout;
	fRows = 256 / fColumns;
	fTState = NULL;

	if (size < MIN_CELL_SIZE)
		size = MIN_CELL_SIZE;
	fCellSize = size;
	
	fModeFlags = 0;
	fRound = 1.0;

	fFastSet = false;

	fLastMode = BScreen(Window()).ColorSpace();
	
	// We just use double buffering now...
	if (use_offscreen)
		use_offscreen = false;
	
	if (use_offscreen) {
		BRect bb = r;
		bb.left -= TEXT_ENTRY_AREA_WIDTH;
		fBitmap = new BBitmap(bb, B_RGB_32_BIT, true);

		fOffscreenView = new BView(bb, "off_view", 0, 0);
		fBitmap->Lock();
		fBitmap->AddChild(fOffscreenView);
		fBitmap->Unlock();
	} else {
		fBitmap = NULL;
		fOffscreenView = NULL;
	}

	fFocused = false;

	/*
	 Need this 'fCachedIndex' thang because of duplicates in the 8-bit
	 color table. Because of duplicates you can't simply store the rgb_color.
	 That would mess up the drawing, keyboard navigation, and mouse
	 tracking code because it would confuse multiple cells.
	*/
	fCachedIndex = -1;
	fRetainCache = false;

	if (data) {
		long val;
		data->FindInt32(S_VALUE, &val);
		SetValue(val);
		fRedText = (BTextControl *) FindView("_red");
		fGreenText = (BTextControl *) FindView("_green");
		fBlueText = (BTextControl *) FindView("_blue");
	} else {
		BMessage	*msg;
		BTextView	*text;

		// positioning doesn't matter here. LayoutView() handles that

		r.Set(0,0,TEXT_ENTRY_AREA_WIDTH-TEXT_MARGIN,FIELD_HEIGHT-1);
		float	spacing = 21;
		BFont	font;
		uint32	mode;
		
		msg = new BMessage(CMD_CHANGE_COLOR);
		fRedText = new BTextControl(r, "_red", "R:", "0", msg);
		fRedText->SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
		text = fRedText->TextView();
		for (long i = 0; i < 256; i++)
			text->DisallowChar(i);
		for (long i = '0'; i <= '9'; i++)
			text->AllowChar(i);
		text->AllowChar(B_BACKSPACE);
		text->SetMaxBytes(3);
		text->GetFontAndColor( &font, &mode, NULL );
		font.SetSize( 10.0 );
		text->SetFontAndColor( &font, mode, NULL );
		fRedText->SetFont( &font );

		r.OffsetBy(0, spacing);

		msg = new BMessage(CMD_CHANGE_COLOR);
		fGreenText = new BTextControl(r, "_green", "G:", "0", msg);
		fGreenText->SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
		text = fGreenText->TextView();
		for (long i = 0; i < 256; i++)
			text->DisallowChar(i);
		for (long i = '0'; i <= '9'; i++)
			text->AllowChar(i);
		text->AllowChar(B_BACKSPACE);
		text->SetMaxBytes(3);
		text->GetFontAndColor( &font, &mode, NULL );
		font.SetSize( 10.0 );
		text->SetFontAndColor( &font, mode, NULL );
		fGreenText->SetFont( &font );

		r.OffsetBy(0, spacing);

		msg = new BMessage(CMD_CHANGE_COLOR);
		fBlueText = new BTextControl(r, "_blue", "B:", "0", msg);
		fBlueText->SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
		text = fBlueText->TextView();
		for (long i = 0; i < 256; i++)
			text->DisallowChar(i);
		for (long i = '0'; i <= '9'; i++)
			text->AllowChar(i);
		text->AllowChar(B_BACKSPACE);
		text->SetMaxBytes(3);
		text->GetFontAndColor( &font, &mode, NULL );
		font.SetSize( 10.0 );
		text->SetFontAndColor( &font, mode, NULL );
		fBlueText->SetFont( &font );

		LayoutView(false);

		AddChild(fRedText);
		AddChild(fGreenText);
		AddChild(fBlueText);
		
		fRedText->ResizeTo( TEXT_ENTRY_AREA_WIDTH-TEXT_MARGIN, FIELD_HEIGHT );
		text = fRedText->TextView();
		float textWidth = text->Bounds().Width();
		text->ResizeTo( textWidth-2, FIELD_HEIGHT-5 );
		
		fGreenText->ResizeTo( TEXT_ENTRY_AREA_WIDTH-TEXT_MARGIN, FIELD_HEIGHT );
		text = fGreenText->TextView();
		text->ResizeTo( textWidth-2, FIELD_HEIGHT-5 );
		
		fBlueText->ResizeTo( TEXT_ENTRY_AREA_WIDTH-TEXT_MARGIN, FIELD_HEIGHT );
		text = fBlueText->TextView();
		text->ResizeTo( textWidth-2, FIELD_HEIGHT-5 );
		
		
		SetValue(0);
	}
}

/*------------------------------------------------------------*/

BRect BColorControl::CalcFrame(BPoint start, color_control_layout layout,
	int32 size)
{
	if (size < MIN_CELL_SIZE)
		size = MIN_CELL_SIZE;

	long	cols = (long) layout;
	long	rows = 256 / cols;

	long width = (cols * size) + (2 * COLOR_AREA_H_FRAME) +
		OLD_TEXT_MARGIN;
	long height = (rows * size) + (2 * COLOR_AREA_V_FRAME);

	return BRect(start, start + BPoint(width, height));
}

/*------------------------------------------------------------*/

BColorControl::BColorControl(BMessage *data)
	: BControl(data)
{
	long	lay;
	bool	use_offscreen;
	float	size;

	data->FindInt32(S_LAYOUT, &lay);
	data->FindBool(S_OFFSCREEN, &use_offscreen);
	data->FindFloat(S_CELL_SIZE, &size);

	InitData((color_control_layout) lay, size, use_offscreen, data);
}

/*------------------------------------------------------------*/

BArchivable *BColorControl::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BColorControl"))
		return NULL;
	return new BColorControl(data);
}

/*------------------------------------------------------------*/

status_t BColorControl::Archive(BMessage *data, bool deep) const
{
	status_t	err;
	if ((err = BControl::Archive(data, deep)) != B_OK)
		return err;
	if ((err = data->AddInt32(S_LAYOUT, fColumns)) != B_OK)
		return err;
	if ((err = data->AddFloat(S_CELL_SIZE, fCellSize)) != B_OK)
		return err;
	return data->AddBool(S_OFFSCREEN, fOffscreenView != NULL);
}

/*------------------------------------------------------------*/

BColorControl::~BColorControl()
{
	delete fBitmap;
}

/*------------------------------------------------------------*/

void BColorControl::LayoutView(bool calc_frame)
{
	color_control_layout	layout = (color_control_layout) fColumns;

	BRect	frame = Frame();

	if (calc_frame) {
		BRect	new_frame = CalcFrame(frame.LeftTop(), layout, fCellSize);
//+		PRINT_OBJECT(frame);
//+		PRINT_OBJECT(new_frame);
		ResizeTo(new_frame.Width(), new_frame.Height());
	}

	BRect		r(Bounds());
	//float		section = r.Height() / 3;

	// try to horizontally align these fields with corresponding color
	// bar in 32 bit mode.

	//float spacing = (section > FIELD_HEIGHT) ? section : FIELD_HEIGHT;

	// start from the top and move down.
	//if (spacing > FIELD_HEIGHT)
	//	r.InsetBy(COLOR_AREA_H_FRAME, COLOR_AREA_V_FRAME);
	
	//r.left = r.right - TEXT_ENTRY_AREA_WIDTH + TEXT_MARGIN;
	r.right = r.left + TEXT_ENTRY_AREA_WIDTH - TEXT_MARGIN;
	//r.bottom = r.top + FIELD_HEIGHT;
	
	/*if (spacing > FIELD_HEIGHT) {
		// horizontally center the field relative to color bar
		r.top += section-1;
		r.bottom += section-1;
		r.OffsetBy(0, ((spacing - FIELD_HEIGHT)/2));
	} else {*/
		// not enough room to position relative to color bar.
		// so just space then out evenly
	//	spacing = Bounds().Height() / 3;
	//}
	fRedText->MoveTo(r.LeftTop());
	fRedText->ResizeTo(r.Width(), fRedText->Bounds().Height());
	fRedText->SetDivider(LABEL_WIDTH);

	//r.OffsetBy(0, spacing);

//+	PRINT(("green ")); PRINT_OBJECT(r);
	fGreenText->MoveTo(BPoint( r.left, ceil((r.Height()-FIELD_HEIGHT)/2.0)+r.top ) );
	fGreenText->ResizeTo(r.Width(), fGreenText->Bounds().Height());
	fGreenText->SetDivider(LABEL_WIDTH);

	//r.OffsetBy(0, spacing);

//+	PRINT(("blue ")); PRINT_OBJECT(r);
	fBlueText->MoveTo(BPoint( r.left, r.bottom-FIELD_HEIGHT ));
	fBlueText->ResizeTo(r.Width(), fBlueText->Bounds().Height());
	fBlueText->SetDivider(LABEL_WIDTH);
}

/*------------------------------------------------------------*/

void BColorControl::MessageReceived(BMessage *msg)
{
	if( msg->WasDropped() )
	{
		rgb_color		*color;
		ssize_t			size;
		if( msg->FindData( "RGBColor", B_RGB_COLOR_TYPE, (const void **)&color, &size ) == B_OK )
		{
			SetValue( *color );
			Invalidate();
			return;
		}
	}
	
	switch (msg->what) {
		case CMD_CHANGE_COLOR:
			{
			const char	*str;
			rgb_color	rgb = {255, 255, 255, 255};
			str = fRedText->Text();
			rgb.red = atoi(str);
			str = fGreenText->Text();
			rgb.green = atoi(str);
			str = fBlueText->Text();
			rgb.blue = atoi(str);
//+			PRINT(("setting value to (%d,%d,%d)\n",
//+				rgb.red, rgb.green, rgb.blue));
			SetValue(rgb);
			Invoke();
			break;
			}
		default:
			BControl::MessageReceived(msg);
	}
}

/*------------------------------------------------------------*/

void BColorControl::SetValue(int32 color)
{
	if (!fRetainCache)
		fCachedIndex = -1;
	fRetainCache = false;

	if (fBitmap) {
		if (fBitmap->Lock()) {
			if (!fFastSet)
				UpdateOffscreen();
			fBitmap->Unlock();
		}
	}
	
	rgb_color	old_rgb = ValueAsColor();
	rgb_color	new_rgb = long_as_rgb(color);
	char		buf[10];

//+	PRINT(("SetValue(%d,%d,%d)\n", new_rgb.red, new_rgb.green, new_rgb.blue));
	if (old_rgb.red != new_rgb.red) {
		sprintf(buf, "%d", new_rgb.red);
		fRedText->SetText(buf);
	}
	if (old_rgb.green != new_rgb.green) {
		sprintf(buf, "%d", new_rgb.green);
		fGreenText->SetText(buf);
	}
	if (old_rgb.blue != new_rgb.blue) {
		sprintf(buf, "%d", new_rgb.blue);
		fBlueText->SetText(buf);
	}
	/*if (LockLooper()) {
		BWindow *w = Window();
		ASSERT(w);
		w->UpdateIfNeeded();
		UnlockLooper();
	}*/
	if( fTState )
		BControl::SetValueNoUpdate(color);
	else
		BControl::SetValue(color);
}

/*------------------------------------------------------------*/

rgb_color BColorControl::ValueAsColor()
{
	return long_as_rgb(Value());
}

/*------------------------------------------------------------*/

void BColorControl::AttachedToWindow()
{
	BControl::AttachedToWindow();

	fRedText->SetTarget(this);
	fBlueText->SetTarget(this);
	fGreenText->SetTarget(this);
	
	if (fBitmap) {
		// draw the image for the first time.
		UpdateOffscreen();
	}
}

/*------------------------------------------------------------*/

void BColorControl::UpdateOffscreen()
{
	ASSERT(fBitmap);
	if (fBitmap->Lock()) {
		UpdateOffscreen(fOffscreenView->Bounds());
		fBitmap->Unlock();
	}
}

/*------------------------------------------------------------*/

void BColorControl::UpdateOffscreen(BRect update)
{
	ASSERT(fBitmap);
	if (fBitmap->Lock()) {
		fOffscreenView->SetViewColor( ViewColor() );
		fOffscreenView->SetLowColor( ViewColor() );
		BRect b = fOffscreenView->Bounds();
		update = update & b;
		fOffscreenView->FillRect(update, B_SOLID_LOW);
		DrawColorArea(fOffscreenView, update);
		fOffscreenView->Sync();
		fBitmap->Unlock();
	}
}

/*------------------------------------------------------------*/

void BColorControl::Draw(BRect update)
{
	if (fBitmap) {
		if (fBitmap->Lock()) {
			BRect b(Bounds());
			b.left += TEXT_ENTRY_AREA_WIDTH;
			if (b.Intersects(update)) {
				UpdateOffscreen(update);
				DrawBitmap(fBitmap, BPoint(TEXT_ENTRY_AREA_WIDTH, 0) );
			}
			fBitmap->Unlock();
		}
	} else {
		BRect r(Bounds());
		r.left += TEXT_ENTRY_AREA_WIDTH;
		if (r.Intersects(update))
			DrawColorArea(this, update);
	}
}

// Floating Point Color Struct
/*------------------------------------------------------------*/

namespace BPrivate {

struct frgb_color
{
	frgb_color( void ) {  };
	frgb_color( const rgb_color &c ) { red=float(c.red); green=float(c.green); blue=float(c.blue); };
	inline frgb_color &operator=( const rgb_color &c ) { red=float(c.red); green=float(c.green); red=float(c.blue); return *this; };
	inline rgb_color RGBValue( void ) { rgb_color c2 = { uint8(red), uint8(green), uint8(blue), 255 }; return c2; };
	inline frgb_color &operator+=( const frgb_color &c ) { red+=c.red; green+=c.green; blue+=c.blue; return *this; };
	float red;
	float green;
	float blue;
};

static inline frgb_color operator-( const frgb_color &c1, const frgb_color &c2 ) { frgb_color c3; c3.red = c1.red-c2.red; c3.green = c1.green-c2.green; c3.blue = c1.blue-c2.blue; return c3; };
static inline frgb_color operator/( const frgb_color &c1, float f ) { frgb_color c3; c3.red = c1.red/f; c3.green = c1.green/f; c3.blue = c1.blue/f; return c3; };

#define POINTER_VERTEX_COUNT 	7

static const BPoint kPointerVerticies[POINTER_VERTEX_COUNT] =
{
	BPoint( 0.0,-1.0 ),
	BPoint( -3.0,-4.0 ),
	BPoint( -3.0,-5.0 ),
	BPoint( -2.0,-6.0 ),
	BPoint( 2.0,-6.0 ),
	BPoint( 3.0,-4.0 ),
	BPoint( 3.0,-4.0 )
};

}

using namespace BPrivate;

/*------------------------------------------------------------*/

void BColorControl::DrawColorSlider(BRect where, BView *target, rgb_color c1, rgb_color c2, bool focused)
{
	frgb_color color( c1 );
	frgb_color cinc = (frgb_color( c2 )-color)/(where.right - where.left-1);
	BPoint a( where.left+1, where.top+1 ), b( where.left+1, where.bottom-1 );
	
	target->SetDrawingMode( B_OP_COPY );
	target->SetPenSize( 1.0 );
	
	// Draw Color Ramp
	target->BeginLineArray( where.IntegerWidth() );
	for( ; a.x < where.right; a.x++, b.x++, color += cinc )
		target->AddLine( a, b, color.RGBValue() );
	target->EndLineArray();
	
	// Draw Border
	rgb_color borderColor = { 0, 0, 0, 255 };
	if( focused ) borderColor = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
	if( !IsEnabled() ) borderColor.disable(ViewColor());
	target->SetHighColor( borderColor );
	if( focused )
	{
		// Fill in corner pixels missed by StrokeRoundRect()... What a hack!
		target->BeginLineArray( 4 );
		target->AddLine( BPoint( where.left+2, where.top+2 ), BPoint( where.left+2, where.top+2 ), borderColor );
		target->AddLine( BPoint( where.left+2, where.bottom-2 ), BPoint( where.left+2, where.bottom-2 ), borderColor );
		target->AddLine( BPoint( where.right-2, where.top+2 ), BPoint( where.right-2, where.top+2 ), borderColor );
		target->AddLine( BPoint( where.right-2, where.bottom-2 ), BPoint( where.right-2, where.bottom-2 ), borderColor );
		target->EndLineArray();
		target->SetPenSize( 2.0 );
	}
	target->StrokeRoundRect( where, 3.0, 3.0 );
	target->SetPenSize( 1.0 );
	
	// Draw Drop Shadow
	target->SetDrawingMode( B_OP_ALPHA );
	rgb_color darken1 = { 0, 0, 0, 80 };
	rgb_color darken2 = { 0, 0, 0, 20 };
	if( !IsEnabled() ) {
		darken1.disable(B_TRANSPARENT_COLOR);
		darken2.disable(B_TRANSPARENT_COLOR);
	}
	target->BeginLineArray( 8 );
	target->AddLine( BPoint( where.left+2.0, where.top+1.0 ), BPoint( where.right-2.0, where.top+1.0 ), darken1 );
	target->AddLine( BPoint( where.left+1.0, where.top+2.0 ), BPoint( where.right-1.0, where.top+2.0 ), darken2 );
	target->AddLine( BPoint( where.left+1.0, where.top+2.0 ), BPoint( where.left+1.0, where.bottom-2.0 ), darken1 );
	target->AddLine( BPoint( where.left+2.0, where.top+2.0 ), BPoint( where.left+2.0,where.bottom-1.0 ), darken2 );
	
	// Clean Corners
	target->AddLine( BPoint( where.left, where.top+1 ), BPoint( where.left+1, where.top ), darken1 );
	target->AddLine( BPoint( where.right, where.top+1 ), BPoint( where.right-1, where.top ), darken1 );
	target->AddLine( BPoint( where.right, where.bottom-1 ), BPoint( where.right-1, where.bottom ), darken1 );
	target->AddLine( BPoint( where.left, where.bottom-1 ), BPoint( where.left+1, where.bottom ), darken1 );
	target->EndLineArray();
	
	target->SetDrawingMode( B_OP_COPY );
}

/*------------------------------------------------------------*/

void BColorControl::DrawThumb( BRect where, uint8 value, BView *target )
{
	const int PSIZE = 2;
	target->SetPenSize(PSIZE);
	
	rgb_color	low = LowColor();
	rgb_color	base = low;

	if (low.red == 255 && low.green == 255 && low.blue == 255)
		base = ui_color(B_PANEL_BACKGROUND_COLOR);
	else
		base = low;
	
	where.left += LEFT_PAD;
	where.right -= RIGHT_PAD;
	
	BPoint origin( where.Width()*(float(value)/255.0)+where.left, where.Height()/2.0+where.top );
	BPoint verticies[POINTER_VERTEX_COUNT];
	rgb_color contentColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color frameColor = { 0, 0, 0, 255 };
	rgb_color shadowColor = { 0, 0, 0, 60 };
	
	if( !IsEnabled() ) {
		contentColor.disable(base);
		frameColor.disable(base);
		shadowColor.disable(B_TRANSPARENT_COLOR);
	}
	
	translate_points( origin, verticies, kPointerVerticies, POINTER_VERTEX_COUNT );
	draw_poly( target, verticies, POINTER_VERTEX_COUNT, contentColor, frameColor, shadowColor, 1.0, 1 );
	reflecty_points( verticies, kPointerVerticies, POINTER_VERTEX_COUNT );
	translate_points( origin, verticies, verticies, POINTER_VERTEX_COUNT );
	draw_poly( target, verticies, POINTER_VERTEX_COUNT, contentColor, frameColor, shadowColor, 1.0, 1 );
}

/*------------------------------------------------------------*/

void BColorControl::DrawSwatch( BRect where, BView *target, rgb_color c )
{
	BRect clipRect = where;
	BRegion clip;
	rgb_color frameColor = { 0, 0, 0, 255 };
	rgb_color darken1 = { 0, 0, 0, 80 };
	rgb_color darken2 = { 0, 0, 0, 20 };
	rgb_color dvcolor = tint_color( target->ViewColor(), B_DARKEN_1_TINT );
	
	if( !IsEnabled() ) {
		c.disable(target->ViewColor());
		frameColor.disable(target->ViewColor());
		dvcolor.disable(target->ViewColor());
		darken1.disable(B_TRANSPARENT_COLOR);
		darken2.disable(B_TRANSPARENT_COLOR);
	}
	
	target->SetPenSize( 1.0 );
	// Draw Left Half
	clipRect.right = clipRect.left + ceil(clipRect.Width()/2.0);
	clip.Set( clipRect );
	target->ConstrainClippingRegion( &clip );
	target->SetDrawingMode( B_OP_COPY );
	target->SetHighColor( c );
	target->FillRoundRect( where, 3.0, 3.0 );
	target->SetHighColor( frameColor );
	target->StrokeRoundRect( where, 3.0, 3.0 );
	
	// Draw Right Half
	target->SetDrawingMode( B_OP_COPY );
	clipRect.left = clipRect.right+1;
	clipRect.right = where.right;
	clip.Set( clipRect );
	target->ConstrainClippingRegion( &clip );
	target->SetHighColor( c );
	target->FillEllipse( where );
	target->SetHighColor( frameColor );
	target->StrokeEllipse( where );
	target->ConstrainClippingRegion( NULL );
	
	// Draw Drop Shadow and touchup corner
	target->SetDrawingMode( B_OP_ALPHA );
	target->BeginLineArray( 6 );
	target->AddLine( BPoint( where.left+2, where.top+1 ), BPoint(clipRect.left+1, where.top+1), darken1 );
	target->AddLine( BPoint( where.left+2, where.top+2 ), BPoint(clipRect.left+3, where.top+2), darken2 );
	target->AddLine( BPoint( where.left+1, where.top+2 ), BPoint(where.left+1, where.bottom-2), darken1 );
	target->AddLine( BPoint( where.left+2, where.top+3 ), BPoint(where.left+2, where.bottom-1), darken2 );
	target->EndLineArray();
	target->SetDrawingMode( B_OP_COPY );
	target->BeginLineArray( 2 );
	target->AddLine( BPoint( where.left, where.top+1 ), BPoint(where.left+1, where.top), dvcolor );
	target->AddLine( BPoint( where.left, where.bottom-1 ), BPoint(where.left+1, where.bottom), dvcolor );
	target->EndLineArray();
}
		
/*------------------------------------------------------------*/

void BColorControl::DrawColorArea(BView *target, BRect update)
{
	BScreen screen( Window() );

	//if (fTState)
		//return;

	rgb_color	rgb = ValueAsColor();

	bool		focused = IsFocus() && Window()->IsActive();
	bool		disabled = !IsEnabled();

	if ((fFocused != IsFocus()) || (screen.ColorSpace() != fLastMode)) {
		fFocused = IsFocus();
		if (fFocused)
			fFocusedComponent = 0;
	}

	fLastMode = screen.ColorSpace();

//+	PRINT(("red=%d, green=%d, blue=%d\n", rgb.red, rgb.green, rgb.blue));

	BRect		bounds(Bounds());
	BRect		r;
	rgb_color	low = LowColor();
	rgb_color	base = low;
	rgb_color	white = {255,255,255,255};

//+	PRINT(("Draw: ")); PRINT_OBJECT(r);
	r.left += TEXT_ENTRY_AREA_WIDTH;

	if (low.red == 255 && low.green == 255 && low.blue == 255)
		base = ui_color(B_PANEL_BACKGROUND_COLOR);
	else
		base = low;

	if( disabled ) white.disable(base);
	
	// Draw Frame
	if (((fModeFlags & B_CC_8BIT_MODE) | (screen.ColorSpace() == B_COLOR_8_BIT))&&!(fModeFlags & B_CC_32BIT_MODE)) {
		r = bounds;
		r.left += TEXT_ENTRY_AREA_WIDTH;
		r.right -= OLD_TEXT_MARGIN - TEXT_ENTRY_AREA_WIDTH;
		
		rgb_color c;
	
		c = shift_color(base, cDARKEN_1);
		if( disabled ) c.disable(base);
		target->SetHighColor(c);
		if( r.Intersects( update ) )
			{
			target->StrokeLine(r.LeftBottom(), r.LeftTop());
			target->StrokeLine(r.RightTop());
			target->SetHighColor(white);
			target->StrokeLine(r.LeftBottom()+BPoint(1,0), r.RightBottom());
			target->StrokeLine(r.RightTop()+BPoint(0,1));
			r.InsetBy(1,1);
			if (focused) {
				// draw UI indication for 'active'
				c = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
				if( disabled ) c.disable(base);
				target->SetHighColor(c);
				target->StrokeRect(r);
			} else {
				c = shift_color(base, cDARKEN_4);
				if( disabled ) c.disable(base);
				target->SetHighColor(c);
				target->StrokeLine(r.LeftBottom(), r.LeftTop());
				target->StrokeLine(r.RightTop());
				target->SetHighColor(base);
				target->StrokeLine(r.LeftBottom()+BPoint(1,0), r.RightBottom());
				target->StrokeLine(r.RightTop()+BPoint(0,1));
			}
		}
	
		r = bounds;
		r.left += TEXT_ENTRY_AREA_WIDTH;
		r.right -= OLD_TEXT_MARGIN - TEXT_ENTRY_AREA_WIDTH;
		r.InsetBy(COLOR_AREA_H_FRAME, COLOR_AREA_V_FRAME);
		
		if( r.Intersects( update ) )
		{
			BRegion		*clip = new BRegion();
			BRect		color_rect;
			rgb_color	c = {255, 255, 255, 255};
			long		color_index;
			
			BScreen screen( Window() );
	
			target->GetClippingRegion(clip);
			c.red = c.green = c.blue = 136;
			c.alpha = 255;
			if( disabled ) c.disable(base);
			target->BeginLineArray(fColumns + fRows + 2);
			for (long i = 0; i <= fColumns; i += 1)
				target->AddLine(BPoint(i * fCellSize + r.left, r.top),
								BPoint(i * fCellSize + r.left, r.bottom), c);
			for (long i = 0; i <= fRows; i += 1)
				target->AddLine(BPoint(r.left, i * fCellSize + r.top),
								BPoint(r.right, i * fCellSize + r.top), c);
			target->EndLineArray();
	
			color_index = 0;
			for (long i = 0; i < fRows; i += 1)
				for (long j = 0; j < fColumns; j += 1) {
					color_rect.Set(r.left + 1 + j * fCellSize,
								   r.top + 1 + i * fCellSize,
								   r.left - 1 + fCellSize + j * fCellSize,
								   r.top - 1 + fCellSize + i * fCellSize);
					if (clip->Intersects(color_rect)) {
						if (disabled)
							target->SetHighColor(
								screen.ColorForIndex( color_index )
								.disable(base));
						else
							target->SetHighColor(
								screen.ColorForIndex( color_index ) );
						target->FillRect(color_rect);
					}
					color_index += 1;
				}
			delete clip;
	
			long ii = fCachedIndex;
			if (ii != -1)
				color_index = ii;
			else
				color_index = screen.IndexForColor( rgb );
			target->SetHighColor(white);
			color_rect.left = r.left + ((color_index % fColumns) * fCellSize);
			color_rect.right = color_rect.left + fCellSize;
			color_rect.top = r.top + ((color_index / fColumns) * fCellSize);
			color_rect.bottom = color_rect.top + fCellSize;
			target->StrokeRect(color_rect);
		}
		
		if( fModeFlags & B_CC_SHOW_SWATCH )
		{
			BRect	swatchRect = bounds;
			swatchRect.left = swatchRect.right + 4 - OLD_TEXT_MARGIN + TEXT_ENTRY_AREA_WIDTH;
			if( swatchRect.Intersects( update ) )
				DrawSwatch( swatchRect, target, screen.ColorForIndex( fCachedIndex != -1 ? fCachedIndex : fCachedIndex = screen.IndexForColor( ValueAsColor() ) ) );
		}
	
	} else {
		r = bounds;
		r.left += TEXT_ENTRY_AREA_WIDTH;
		//r.InsetBy(COLOR_AREA_H_FRAME, COLOR_AREA_V_FRAME);
		ColorRamp(r, update, target, rgb, 0xf, focused);
	}
}

/*------------------------------------------------------------*/

void BColorControl::MouseDown(BPoint where)
{
	ulong		buttons;
	BRect		bounds(Bounds());
	BRect		r = bounds;
	rgb_color	rgb = {255, 255, 255, 255};

//+	PRINT(("MouseDown\n"));

	if (!IsEnabled())
		return;
	GetMouse( &where, &buttons );
	if( buttons & B_SECONDARY_MOUSE_BUTTON )
	{
		BPopUpMenu	popup( "ColorControl Options", false, false );
		BMenuItem *mitem;
		BMenuItem *gradient, *color32, *color8, *swatch;
		popup.AddItem( color32 = new BMenuItem( "32 Bit Mode", NULL ) );
		popup.AddItem( color8 = new BMenuItem( "8 Bit Mode", NULL ) );
		popup.AddItem( gradient = new BMenuItem( "Live Gradient", NULL ) );
		popup.AddItem( swatch = new BMenuItem( "Show Swatch", NULL ) );
		if( fModeFlags & B_CC_LIVE_GRADIENT )
			gradient->SetMarked( true );
		if( fModeFlags & B_CC_8BIT_MODE )
			color8->SetMarked( true );
		if( fModeFlags & B_CC_32BIT_MODE )
			color32->SetMarked( true );
		if( fModeFlags & B_CC_SHOW_SWATCH )
			swatch->SetMarked( true );
		ConvertToScreen( &where );
		mitem = popup.Go( where );
		if( mitem )
		{
			if( mitem == gradient )
			{
				if( fModeFlags & B_CC_LIVE_GRADIENT )
					fModeFlags &= ~B_CC_LIVE_GRADIENT;
				else
					fModeFlags |= B_CC_LIVE_GRADIENT;
			}
			else if( mitem == color8 )
			{
				if( fModeFlags & B_CC_8BIT_MODE )
					fModeFlags &= ~B_CC_8BIT_MODE;
				else
				{
					fModeFlags &= ~B_CC_32BIT_MODE;
					fModeFlags |= B_CC_8BIT_MODE;
				}
			}
			else if( mitem == color32 )
			{
				if( fModeFlags & B_CC_32BIT_MODE )
					fModeFlags &= ~B_CC_32BIT_MODE;
				else
				{
					fModeFlags &= ~B_CC_8BIT_MODE;
					fModeFlags |= B_CC_32BIT_MODE;
				}
			}
			else if( mitem == swatch )
			{
				if( fModeFlags & B_CC_SHOW_SWATCH )
					fModeFlags &= ~B_CC_SHOW_SWATCH;
				else
					fModeFlags |= B_CC_SHOW_SWATCH;
			}
			Invalidate();
		}
		return;
	}
	
	color_space cspace = BScreen(Window()).ColorSpace();
	// Flag indicates if we are doing 8 bit or 32 bit
	bool do8bit = ((fModeFlags & B_CC_8BIT_MODE) | (cspace == B_COLOR_8_BIT))&&!(fModeFlags & B_CC_32BIT_MODE);
	
	r.left += TEXT_ENTRY_AREA_WIDTH;
	// Was the click on the swatch?
	// 		Initiate drag and drop color
	if( fModeFlags & B_CC_SHOW_SWATCH )
	{
		float left = do8bit ? r.right + 4 - OLD_TEXT_MARGIN + TEXT_ENTRY_AREA_WIDTH
			: r.right - (r.Height()*0.5);
		
		if( left < 64 )
			left = 64;
		BRect swatchRect = r;
		swatchRect.left = left;
		
		if( swatchRect.Contains(where) )
		{
			BMessage		msg( B_PASTE );
			rgb_color		color, swapColor;
			char			htmlStr[256];
			
			color = ValueAsColor();
			swapColor.alpha = color.alpha;
			swapColor.red = color.blue;
			swapColor.blue = color.red;
			swapColor.green = color.green;
			msg.AddData( "RGBColor", B_RGB_COLOR_TYPE, &color, sizeof(rgb_color) );
			sprintf( htmlStr, "#%.2lX%.2lX%.2lX", int32(swapColor.red), int32(swapColor.green), int32(swapColor.green) );
			msg.AddData( "text/plain", B_MIME_TYPE, htmlStr, strlen(htmlStr) );
			
			BBitmap		*dragMap;
			dragMap = new BBitmap( BRect( 0, 0, 31, 31 ), B_RGB32 );
			
			for( rgb_color *next = (rgb_color *)dragMap->Bits(), *last = (next + dragMap->BitsLength()/sizeof(rgb_color)); next < last; )
				*next++ = swapColor;
			DragMessage( &msg, dragMap, BPoint( 15, 15 ), NULL );
			return;
		}
		r.right = do8bit ? left-4 : left;
	}
	//r.InsetBy(COLOR_AREA_H_FRAME, COLOR_AREA_V_FRAME);

	ASSERT(!fTState);
	fTState = new track_state;

	fTState->cspace = cspace;
	fTState->active_area = r;
	fTState->orig_color = fTState->cur_color = -1;
	fTState->refresh_clock = 0;
	fTState->bar_index = fTState->prev_color = -1;
	fTState->rgb = rgb;
	
	if (do8bit) {
		r.InsetBy(COLOR_AREA_H_FRAME, COLOR_AREA_V_FRAME);
		fTState->active_area = r;
		
		// Open a connection to the screen...
		BScreen screen( Window() );

		if ((Flags() & B_NAVIGABLE) != 0 && !IsFocus())
			MakeFocus(true);
		
		SetExplicitFocus();
		
		if (fCachedIndex != -1)
			fTState->orig_color = fCachedIndex;
		else
			fTState->orig_color = screen.IndexForColor(ValueAsColor());
		fTState->prev_color = fTState->orig_color;
		fTState->r = r;
		if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
			SetTracking(true);
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS|B_NO_POINTER_HISTORY);
			DoMouseMoved(where);
			return;
		}
		do {
			GetMouse(&where, &buttons);
			DoMouseMoved(where);
			snooze(REFRESH_INTERVAL);
		} while(buttons);
	} else {
		// 32 bit
		float bar_height = r.Height() / 3.0;
		fTState->bar_index = (int32)((where.y - r.top) / bar_height)+1;
		if (fTState->bar_index) {
			if ((Flags() & B_NAVIGABLE) != 0) {
				if (!IsFocus()) MakeFocus(true);
				fFocused = true;
				if (fFocusedComponent != fTState->bar_index-1) {
					fFocusedComponent = fTState->bar_index-1;
					Invalidate();
				}
			}
			
			SetExplicitFocus();
			
			rgb = fTState->rgb = ValueAsColor();
			switch (fTState->bar_index) {
				case 1:
					fTState->orig_color = rgb.red;
					//r.top += bar_height;
					r.bottom = r.top + bar_height;
					break;
				case 2:
					fTState->orig_color = rgb.green;
					r.top += bar_height * 1;
					r.bottom = r.top + bar_height;
					break;
				case 3:
					fTState->orig_color = rgb.blue;
					r.top += bar_height * 2;
					r.bottom = r.top + bar_height;
					break;
			}
			const float slop = 20.0;
			r.InsetBy(0, -slop);

			r.left += LEFT_PAD;
			r.right -= RIGHT_PAD;
//+			PRINT_OBJECT(r);
			fTState->r = r;

			fTState->prev_color = fTState->orig_color;
			
			if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
				SetTracking(true);
				SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS|B_NO_POINTER_HISTORY);
				DoMouseMoved(where);
				return;
			}

			do {
				GetMouse(&where, &buttons);
				DoMouseMoved(where);
				snooze(50000);
			} while(buttons);
		}
	}

	DoMouseUp(where);
}

/* ---------------------------------------------------------------- */

void BColorControl::DoMouseMoved(BPoint where)
{
	ASSERT(fTState);

	float		x;
	float		y;
	BRect		color_rect;
	BRect		r = fTState->r;
//+	rgb_color	rgb = {255, 255, 255, 255};
	//bool		focused = IsFocus();
	
	
	if (((fModeFlags & B_CC_8BIT_MODE) | (fTState->cspace == B_COLOR_8_BIT))&&!(fModeFlags & B_CC_32BIT_MODE)) {
		BScreen	screen(Window());
		x = where.x;
		y = where.y;
		if ((x >= r.left) && (x < r.right) &&
			(y >= r.top) && (y < r.bottom)) {
			x -= r.left;
			y -= r.top;
			fTState->cur_color = (((int) (y / fCellSize)) * fColumns) +
					((int) (x / fCellSize));
		} else {
			fTState->cur_color = fTState->orig_color;
		}

		if (fTState->cur_color != fTState->prev_color) {
			SetHighColor(136, 136, 136);
			color_rect.left = r.left +
				((fTState->prev_color % fColumns) * fCellSize);
			color_rect.right = color_rect.left + fCellSize;
			color_rect.top = r.top +
				((fTState->prev_color / fColumns) * fCellSize);
			color_rect.bottom = color_rect.top + fCellSize;
			StrokeRect(color_rect);
			if (fBitmap) {
				if (fBitmap->Lock()) {
					fOffscreenView->SetHighColor(136, 136, 136);
					fOffscreenView->StrokeRect(color_rect);
					fOffscreenView->Sync();
					fBitmap->Unlock();
				}
			}

			SetHighColor(255, 255, 255);
			color_rect.left = r.left +
				((fTState->cur_color % fColumns) * fCellSize);
			color_rect.right = color_rect.left + fCellSize;
			color_rect.top = r.top +
				((fTState->cur_color / fColumns) * fCellSize);
			color_rect.bottom = color_rect.top + fCellSize;
			StrokeRect(color_rect);
			fTState->prev_color = fTState->cur_color;

			if (fBitmap) {
				if (fBitmap->Lock()) {
					fOffscreenView->SetHighColor(255, 255, 255);
					fOffscreenView->StrokeRect(color_rect);
					fOffscreenView->Sync();
					fBitmap->Unlock();
				}
			}

			fTState->rgb = screen.ColorForIndex( fTState->cur_color );
			fCachedIndex = fTState->cur_color;
			fRetainCache = true;
			fFastSet = true;
			SetValue(fTState->rgb);
			fFastSet = false;
			
			if( fModeFlags & B_CC_SHOW_SWATCH )
			{
				BRect	swatchRect = Bounds();
				swatchRect.left = swatchRect.right + 4 - OLD_TEXT_MARGIN + TEXT_ENTRY_AREA_WIDTH;
				Invalidate( swatchRect );
			}
		
		}
	} else {
		y = where.y;
		x = where.x;
		if (x < r.left)
			x = r.left;
		if (x > r.right - 1)
			x = r.right - 1;
		if ((y >= r.top) && (y < r.bottom)) {
			// normalize the x coords to be zero based
			x -= r.left;
			y -= r.top;
			fTState->cur_color = (int32)(x * (256.0 / r.Width()));

			/*
			 the following deals with edge cases. For narrow
			 controls there is a rounding problem that might
			 otherwise prevent getting the 2 edge values.
			*/
			if (where.x >= r.right)
				fTState->cur_color = 255;
			else if (where.x <= r.left)
				fTState->cur_color = 0;
		} else {
			fTState->cur_color = fTState->orig_color;
		}
//+		PRINT(("x=%d, y=%d, c=%d\n", x, y, fTState->cur_color));
		if (fTState->cur_color != fTState->prev_color) {
			BPoint  sliderOrigin;
			color_rect = Bounds();
			color_rect.left += TEXT_ENTRY_AREA_WIDTH;
			
			switch (fTState->bar_index) {
				case 1:
					sliderOrigin.Set( color_rect.left, color_rect.top );
					fTState->rgb.red = fTState->cur_color;
					break;
				case 2:
					sliderOrigin.Set( color_rect.left, ceil((color_rect.Height()-FIELD_HEIGHT)/2.0 + color_rect.top) );
					fTState->rgb.green = fTState->cur_color;
					break;
				case 3:
					sliderOrigin.Set( color_rect.left, color_rect.bottom-FIELD_HEIGHT );
					fTState->rgb.blue = fTState->cur_color;
					break;
			}
			//color_rect.left = r.left +
			//	(fTState->prev_color * (r.Width() / 256) - 3);
			//color_rect.right = color_rect.left + 8;
			SetValue(fTState->rgb);
			
			bigtime_t now = system_time();
			
			if( (now > (fTState->refresh_clock + REFRESH_INTERVAL))||(!color_rect.Contains(where)) )
			{
				// If not live gradient, selectively update areas which have changed
				if( !(fModeFlags & B_CC_LIVE_GRADIENT) )
				{
					BRegion updateRegion;
					BRect updateRect;
					
					// Do we need to invalidate the swatch?
					if( fModeFlags & B_CC_SHOW_SWATCH )
					{
						BRect swatchRect = color_rect;
						swatchRect.left = swatchRect.right - (swatchRect.Height()*0.5);
						updateRegion.Include( swatchRect );
						updateRect.right = swatchRect.left-1;
					}
					else
						updateRect.right = color_rect.right-1;
					updateRect.left = color_rect.left;
					updateRect.top = sliderOrigin.y+1;
					updateRect.bottom = sliderOrigin.y+FIELD_HEIGHT-1;
					updateRegion.Include( updateRect );
					Invalidate( &updateRegion );
				}
				else
					Invalidate( color_rect );
				fTState->refresh_clock = now;
			}
			//ColorRamp(fTState->active_area, color_rect, this, fTState->rgb,
			//	1 << fTState->bar_index, focused);
			
			fTState->prev_color = fTState->cur_color;
			fCachedIndex = fTState->cur_color;
			fRetainCache = true;
		}
	}
}

/* ---------------------------------------------------------------- */

void BColorControl::DoMouseUp(BPoint )
{
	ASSERT(fTState);
	if (fTState->cur_color != fTState->orig_color) {
		if (fBitmap)
			UpdateOffscreen();

//+		PRINT(("DoMouseUp: Invoke\n"));
		Invoke();
	}
	delete fTState;
	fTState = NULL;
	
	BRect color_rect;
	color_rect = Bounds();
	color_rect.left += TEXT_ENTRY_AREA_WIDTH;
	Invalidate( color_rect );
}

/*------------------------------------------------------------*/

void BColorControl::SetModeFlags( uint8 flags )
{
	fModeFlags = flags;
}

/*------------------------------------------------------------*/

uint8 BColorControl::ModeFlags() const
{
	return fModeFlags;
}

/*------------------------------------------------------------*/

void BColorControl::ColorRamp(BRect r, BRect update, BView *target,
	rgb_color cur_color, int16, bool focused)
{
	/*
	 r		- the full color area (not necessarily starting at (0,0)
	 update	- the part of the color area to redraw
	*/
/*	
	//int32		i;
	//int32		start;
	//int32		end;
	//int32		color;
	//rgb_color	c = {255, 255, 255, 255};
	
//+	PRINT(("ramp bar=%.2f\n", bar_height));
//+	PRINT_OBJECT(Bounds());
//+	PRINT_OBJECT(r);
//+	PRINT_OBJECT(where);



	start = (int32)(where.left - r.left) - 1;
	if (start < 0)
		start = 0;
	end = (int32)(where.right - r.left) + 1;
	if (end > r.Width() + 1)
		end = (int32)r.Width() + 1;

//+	PRINT(("start=%d, end=%d\n\n", start, end));
	target->BeginLineArray((4 * (end - start)) + 4);
	for (i = start; i < end; i++) {
		color = (int32)(i * (256 / ((r.right - r.left) + 1)));
		if (color_flag & 1) {
			c.red = c.green = c.blue = color;
			if (disabled)
				c = shift_color(c, cLIGHTEN_1);
			target->AddLine(BPoint(r.left + i, r.top),
					BPoint(r.left + i, r.top + (bar_height - 1) +
					fRound), c);
		}
		if (color_flag & 2) {
			c.red = color;
			c.green = c.blue = 0;
			if (disabled)
				c = shift_color(c, cLIGHTEN_1);
			target->AddLine(BPoint(r.left + i, r.top),
					BPoint(r.left + i, r.top + (1 * bar_height - 1) + fRound), c);
		}
		if (color_flag & 4) {
			c.green = color;
			c.red = c.blue = 0;
			if (disabled)
				c = shift_color(c, cLIGHTEN_1);
			target->AddLine(BPoint(r.left + i, r.top + (1*bar_height + fRound)),
					BPoint(r.left + i, r.top + (2 * bar_height - 1) + fRound), c);
		}
		if (color_flag & 8) {
			c.blue = color;
			c.red = c.green = 0;
			if (disabled)
				c = shift_color(c, cLIGHTEN_1);
			target->AddLine(BPoint(r.left + i, r.top + (2*bar_height + fRound)),
					BPoint(r.left + i, r.top + (3 * bar_height - 1) + fRound), c);
		}
	}

	float half_bar = bar_height / 2.0;
	target->EndLineArray();
	target->SetHighColor(255, 255, 255);
	*/
	
	BRect		sliderRect;
	BPoint		sliderOrigin;
	//float		bar_height;
	bool		disabled = !IsEnabled();
	float		right = fModeFlags & B_CC_SHOW_SWATCH ? r.right - (r.Height()*0.5) : r.right;
	if( right < 64 )
		right = 64;
	//bar_height = r.Height() / 3.0;
	rgb_color c1, c2;
	uint8 value = 0;
	
	c1.alpha = 255;
	c2.alpha = 255;
	
	// Draw Each Slider
	for( int8 i=0; i<3; i++ )
	{
		switch( i )
		{
			// Select Colors for this slider
			case 0:
				value = cur_color.red;
				sliderOrigin.Set( r.left, r.top );
				sliderRect.Set( sliderOrigin.x, sliderOrigin.y, right, sliderOrigin.y+FIELD_HEIGHT );
				c1.red = 0; c2.red = 255;
				if( fModeFlags & B_CC_LIVE_GRADIENT )
				{
					c1.green = c2.green = cur_color.green;
					c1.blue = c2.blue = cur_color.blue;
				}
				else
					c1.green = c2.green = c1.blue = c2.blue = 0;
				break;
			case 1:
				value = cur_color.green;
				sliderOrigin.Set( r.left, ceil((r.Height()-FIELD_HEIGHT)/2.0 + r.top) );
				sliderRect.Set( sliderOrigin.x, sliderOrigin.y, right, sliderOrigin.y+FIELD_HEIGHT );
				c1.green = 0; c2.green = 255;
				if( fModeFlags & B_CC_LIVE_GRADIENT )
				{
					c1.red = c2.red = cur_color.red;
					c1.blue = c2.blue = cur_color.blue;
				}
				else
					c1.red = c2.red = c1.blue = c2.blue = 0;
				break;
			case 2:
				value = cur_color.blue;
				sliderOrigin.Set( r.left, r.bottom-FIELD_HEIGHT );
				sliderRect.Set( sliderOrigin.x, sliderOrigin.y, right, sliderOrigin.y+FIELD_HEIGHT );
				c1.blue = 0; c2.blue = 255;
				if( fModeFlags & B_CC_LIVE_GRADIENT )
				{
					c1.green = c2.green = cur_color.green;
					c1.red = c2.red = cur_color.red;
				}
				else
					c1.red = c2.red = c1.green = c2.green = 0;
				break;
		}
		if( sliderRect.Intersects( update ) )
		{
			if( disabled )
			{
				c2.disable(target->ViewColor());
				c1.disable(target->ViewColor());
			}
			DrawColorSlider( sliderRect, target, c1, c2, focused && (fFocusedComponent == i));
			DrawThumb( sliderRect, value, target );
		}
		//sliderRect.OffsetBy( 0, bar_height );
	}
	
	if( fModeFlags & B_CC_SHOW_SWATCH )
	{
		BRect	swatchRect = r;
		swatchRect.left = right + 5;
		
		if( swatchRect.Intersects( update ) )
			DrawSwatch( swatchRect, target, cur_color );
	}

	target->SetPenSize(1);
}

/*------------------------------------------------------------*/

bool BColorControl::key_down8(uint32 key)
{
	long		index;
	long		new_index;
	rgb_color	rgb = {255, 255, 255, 255};
	bool		result = true;
	BScreen screen( Window() );

	switch (key) {
		case B_SPACE:
			if (IsEnabled())
				Invoke();
			break;
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			if (!IsEnabled())
				break;

			if (fCachedIndex != -1)
				index = fCachedIndex;
			else
				index = screen.IndexForColor(ValueAsColor());

			new_index = index + ((key == B_DOWN_ARROW) ? fColumns : -fColumns);

			if (new_index < 0) {
				new_index = 256 - (fColumns - index) - 1;
				if (new_index < 0)
					new_index = 255;
			} else if (new_index > 255) {
				new_index = fColumns - (256 - index) + 1;
				if (new_index >= fColumns)
					new_index = 0;
			}

			rgb = screen.ColorForIndex(new_index);
			fCachedIndex = new_index;
			fRetainCache = true;
			SetValue(rgb);
			Invoke();

			break;
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
			if (!IsEnabled())
				break;

			if (fCachedIndex != -1)
				index = fCachedIndex;
			else
				index = screen.IndexForColor(ValueAsColor());

			new_index = index + ((key == B_RIGHT_ARROW) ? 1 : -1);

			if (new_index < 0)
				new_index = 255;
			else if (new_index > 255)
				new_index = 0;

			rgb = screen.ColorForIndex(new_index);
			fCachedIndex = new_index;
			fRetainCache = true;
			SetValue(rgb);
			Invoke();
			break;
		default:
			result = false;
			break;
	}

	return (result);
}

/*------------------------------------------------------------*/

bool BColorControl::key_down32(uint32 key)
{
	ulong		mods;
	bool		result = true;

	switch (key) {
		case B_SPACE:
			if (IsEnabled())
				Invoke();
			break;
		case B_UP_ARROW:
		case B_DOWN_ARROW:
			if (!IsEnabled())
				break;

			// move between the color components
			Window()->CurrentMessage()->FindInt32("modifiers", (long*) &mods);
			if (key == B_UP_ARROW)
				fFocusedComponent = fFocusedComponent - 1;
			else
				fFocusedComponent = fFocusedComponent + 1;
			if (fFocusedComponent < 0)
				fFocusedComponent = 2;
			else if (fFocusedComponent > 2)
				fFocusedComponent = 0;
			if (fBitmap) {
				UpdateOffscreen();
			}
			Invalidate();

			break;
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
			if (!IsEnabled())
				break;

			KbAdjustColor(key);
			break;
		default:
			result = false;
			break;
	}

	return (result);
}

/*------------------------------------------------------------*/

void BColorControl::KeyDown(const char *bytes, int32 numBytes)
{
	uchar		key = bytes[0];
	bool		keyHandled = true;

	if ( ((fModeFlags & B_CC_8BIT_MODE) | (BScreen(Window()).ColorSpace() == B_COLOR_8_BIT))&&!(fModeFlags & B_CC_32BIT_MODE) )
		keyHandled = key_down8(key);
	else
		keyHandled = key_down32(key);

	if (!keyHandled)
		BControl::KeyDown(bytes, numBytes);
}

/*------------------------------------------------------------*/

void BColorControl::KbAdjustColor(ulong key)
{
#define INCR	(5)

	rgb_color	rgb = ValueAsColor();
	ulong		incr = (key == B_RIGHT_ARROW) ? INCR : -INCR;

	switch (fFocusedComponent) {
		case 0:
#if 0
			nv = rgb.red + incr;
			if (nv <= -INCR)
				nv = 255;
			else if (nv < 0)
				nv = 0;
			else if (nv >= (255 + INCR))
				nv = 0;
			else if (nv > 255)
				nv = 255;
			rgb.red = nv;
#endif
			rgb.red += incr;
			break;
		case 1:
#if 0
			nv = rgb.green + incr;
			if (nv <= -INCR)
				nv = 255;
			else if (nv < 0)
				nv = 0;
			else if (nv >= (255 + INCR))
				nv = 0;
			else if (nv > 255)
				nv = 255;
			rgb.green = nv;
#endif
			rgb.green += incr;
			break;
		case 2:
#if 0
			nv = rgb.blue + incr;
			if (nv <= -INCR)
				nv = 255;
			else if (nv < 0)
				nv = 0;
			else if (nv >= (255 + INCR))
				nv = 0;
			else if (nv > 255)
				nv = 255;
			rgb.blue = nv;
#endif
			rgb.blue += incr;
			break;
	}
	SetValue(rgb);
	Invoke();
}

/*------------------------------------------------------------*/

rgb_color long_as_rgb(long color)
{
	rgb_color	rgb;

	rgb.red = (color >> 24);
	rgb.green = (color >> 16);
	rgb.blue = (color >> 8);
	rgb.alpha = color;

	return rgb;
}


/*------------------------------------------------------------*/

void BColorControl::SetEnabled(bool state)
{
	if (state == IsEnabled()) 
		return;

	fRedText->SetEnabled(state);
	fGreenText->SetEnabled(state);
	fBlueText->SetEnabled(state);

	BControl::SetEnabled(state);
}

/*------------------------------------------------------------*/

void BColorControl::SetCellSize(float size)
{
	if (size < MIN_CELL_SIZE)
		size = MIN_CELL_SIZE;

	if (size != fCellSize) {
		fCellSize = size;
		LayoutView(true);
		Invalidate();
	}
}

/*------------------------------------------------------------*/

float BColorControl::CellSize() const
{
	return fCellSize;
}

/*------------------------------------------------------------*/

void BColorControl::SetLayout(color_control_layout layout)
{
	long	cols = (long) layout;

	if (cols != fColumns) {
		fColumns = cols;
		fRows = 256 / fColumns;
		LayoutView(true);
		Invalidate();
	}
}

/*------------------------------------------------------------*/

color_control_layout BColorControl::Layout() const
{
	return (color_control_layout) fColumns;
}

/*------------------------------------------------------------*/

void
BColorControl::GetPreferredSize(
	float	*width,
	float	*height)
{
	BRect frame = CalcFrame(Frame().LeftTop(), (color_control_layout)fColumns,
							fCellSize);
	
	*width = frame.Width();	
	*height = frame.Height();
}

/*-------------------------------------------------------------*/

void
BColorControl::ResizeToPreferred()
{
	LayoutView(true);
}

/*-------------------------------------------------------------*/

status_t	BColorControl::Invoke(BMessage *msg)
{
	return BControl::Invoke(msg);
}

/*-------------------------------------------------------------*/

BColorControl &BColorControl::operator=(const BColorControl &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BColorControl::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BControl::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BColorControl::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BColorControl::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BColorControl::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BColorControl::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BColorControl::MouseUp(BPoint pt)
{
	if (IsTracking()) {
//+		PRINT(("Async Up\n"));
		DoMouseMoved(pt);
		DoMouseUp(pt);
		SetTracking(false);
	}
}

/* ---------------------------------------------------------------- */

void BColorControl::MouseMoved(BPoint pt, uint32 , const BMessage *)
{
	if (IsTracking()) {
		DoMouseMoved(pt);
	}
}

/*---------------------------------------------------------------*/

void	BColorControl::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BColorControl::FrameResized(float new_width, float new_height)
{
	BControl::FrameResized(new_width, new_height);
}

/*---------------------------------------------------------------*/

void	BColorControl::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void	BColorControl::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllAttached();
}

/*---------------------------------------------------------------*/

void	BColorControl::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllDetached();
}

/* ---------------------------------------------------------------- */

void BColorControl::_ReservedColorControl1() {}
void BColorControl::_ReservedColorControl2() {}
void BColorControl::_ReservedColorControl3() {}
void BColorControl::_ReservedColorControl4() {}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
