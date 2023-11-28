//******************************************************************************
//
//	File:			Button.cpp
//
//	Description:	Implementation for BButton class.
//
//	Written by:		Peter Potrebic and Steve Horowitz
//
//	Copyright 1992-96, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _FONT_H
#include <Font.h>
#endif
#ifndef _GLYPH_H
#include <Glyph.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
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

#include <Screen.h>
#include <string.h>

#include <StreamIO.h>

/*------------------------------------------------------------*/

BButton::BButton(BRect r, const char *name, const char *text,
				BMessage *message, uint32 resizeMask, uint32 flags) :
	BControl(r, name, text, message, resizeMask, flags)
{
	fDrawAsDefault = false;
	fCachedWidth = -1;
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate( true );
	
	font_height	info;
	GetFontHeight(&info);

	// need 12 pixels in addition to the size of text
	// allow for increased sized of default button.
	float h = 12 + (fDrawAsDefault ? 6 : 0) + 
			  ceil(info.ascent + info.descent);
	if (Bounds().Height() < h)
		ResizeTo(Bounds().Width(), h);
}

/*------------------------------------------------------------*/

BButton::BButton(BMessage *data)
	: BControl(data)
{
	fDrawAsDefault = false;
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate( true );
	
	data->FindBool(S_DEFAULT, &fDrawAsDefault);
	fCachedWidth = -1;
}

/*------------------------------------------------------------*/

BButton::~BButton()
{
}

/*------------------------------------------------------------*/

BArchivable *BButton::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BButton"))
		return NULL;
	return new BButton(data);
}

/*------------------------------------------------------------*/

status_t BButton::Archive(BMessage *data, bool deep) const
{
	BControl::Archive(data, deep);
//+	data->AddString(B_CLASS_NAME_ENTRY, "BButton");

	if (IsDefault())
		data->AddBool(S_DEFAULT, IsDefault());
	return 0;
}

/*------------------------------------------------------------*/

void	BButton::AttachedToWindow()
{
	BControl::AttachedToWindow();

	if (fDrawAsDefault) {
		// now that we're attached we can do it for real
		Window()->SetDefaultButton(this);
	}
	
#define AUTO_RESIZE_CONTROL 0
#if AUTO_RESIZE_CONTROL
	font_height	info;
	GetFontHeight(&info);
//+	PRINT(("font %s: size=%.1f, a=%.1f, d=%.1f, l=%.1f\n",
//+		info.name, info.size, info.ascent, info.descent, info.leading));
//+	PRINT_OBJECT(Bounds());

	// need 12 pixels in addition to the size of text
	// allow for increased sized of default button.
	float h = 12 + (fDrawAsDefault ? 6 : 0) + 
			  ceil(info.ascent + info.descent);
	if (Bounds().Height() < h) {
		ResizeTo(Bounds().Width(), h);
	}
#endif
}

/*------------------------------------------------------------*/

bool	BButton::IsDefault() const
{
	return fDrawAsDefault;
}

/*------------------------------------------------------------*/

void	BButton::MakeDefault(bool state)
{
	/*
	 You might notice that this and it's cousin, BWindow::SetDefaultButton
	 are rather complicated. We're just setting up a default button, why
	 does it have to be so complicated? Well, we wanted to be able to
	 set up the default button by calling a method on either the Window
	 or the Button. We didn't want to be Button centric or Window centric.
	 Supporting this notion makes the func more complicated.
	*/

	BButton	*cur;

	BWindow *window = Window();

	cur = window ? window->DefaultButton() : NULL;

	if (state == false) {
		if (!fDrawAsDefault)
			return;

		fDrawAsDefault = false;

		ResizeBy(-12, -4);
		MoveBy(6, 2);

		if (cur == this)
			window->SetDefaultButton(NULL);
	} else {
		if (fDrawAsDefault && (cur == this))
			return;

		fDrawAsDefault = true;

		ResizeBy(12, 4);
		MoveBy(-6, -2);

		if (window && (cur != this))
			window->SetDefaultButton(this);
	}
}

/*------------------------------------------------------------*/

status_t	BButton::Execute()
{
	if (IsEnabled()) {
		SetValue(1);
		if (Window()) Window()->UpdateIfNeeded();
		Invoke();		// Invoke will reset the value to 0
		return B_OK;
	}
	return B_ERROR;
}

/*------------------------------------------------------------*/

void	BButton::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_ENTER:
		case B_SPACE:
			// Make sure value starts out at 0; BControl will
			// change it to 1.
			SetValueNoUpdate(0);
			BControl::KeyDown(bytes, numBytes);
			break;
		default:
			BControl::KeyDown(bytes, numBytes);
			break;
	}
}

void	BButton::Draw(BRect)
{
	BRect		bounds;
	BRect		rect;
	BPoint		offset;
	font_height	fontInfo;
	const char	*label;
	float		str_width;
	rgb_color	contentColor = ui_color(B_CONTROL_BACKGROUND_COLOR);
	rgb_color	defaultRectColor = ui_color(B_CONTROL_HIGHLIGHT_COLOR);
	rgb_color	textColor = ui_color(B_CONTROL_TEXT_COLOR);
	rgb_color	frameColor = ui_color(B_CONTROL_BORDER_COLOR);
	rgb_color	finalContentColor;
	rgb_color	finalFrameColor;
	bool		pressing;
	bool		enabled;
	bool		focus;
	
	focus = IsFocus() && Window()->IsActive();
	pressing = Value() != 0 || IsPressed();
	enabled = IsEnabled();
	label = Label();
	bounds = Bounds();
	bounds.right-=2;
	bounds.top+=2;
	bounds.bottom-=2;
	
	if (fCachedWidth < 0) {
		str_width = label ? StringWidth(label) : 0;
		fCachedWidth = str_width;
	} else
		str_width = fCachedWidth;
	GetFontHeight( &fontInfo );
	
	if( enabled ) 
	{
		finalContentColor = contentColor;
		finalFrameColor = frameColor;
		SetDrawingMode( B_OP_COPY );
	}
	else
	{
		finalContentColor = contentColor.disable_copy(B_TRANSPARENT_COLOR);
		finalFrameColor = frameColor.disable_copy(B_TRANSPARENT_COLOR);
		textColor.disable(B_TRANSPARENT_COLOR);
		defaultRectColor = finalFrameColor;
		SetDrawingMode( B_OP_ALPHA );
	}
	
	// Set up button glyph.
	BButtonBorderGlyph glyph;
	glyph.SetBackgroundColor(B_TRANSPARENT_COLOR);
	if (focus)	glyph.SetBorderColor(NextFocusColor());
	else		glyph.SetBorderColor(frameColor);
	glyph.SetFillColor(contentColor);
	glyph.SetLabelColor(frameColor);
	glyph.SetPressed(pressing);
	glyph.SetActive(false);
	glyph.SetFocused(focus);
	glyph.SetEnabled(enabled);
	
	if( fDrawAsDefault )
	{
		rect = bounds;
		rect.InsetBy( 6, 2 );
		glyph.SetFrame(rect);
		
		if (!enabled) {
			// Remove actual button from clipping region, so that we
			// don't composite this underneath it.
			glyph.InverseClipView(this);
		}
		
		// Draw Default Thingy
		rect = bounds;
		rect.right = bounds.left+18;
		SetHighColor( defaultRectColor );
		FillRoundRect( rect, 3.0, 3.0 );
		if (enabled) {
			SetHighColor( finalFrameColor );
			StrokeRoundRect( rect, 3.0, 3.0 );
		}

		rect = bounds;
		rect.left = bounds.right-18;
		SetHighColor( defaultRectColor );
		FillRoundRect( rect, 3.0, 3.0 );
		if (enabled) {
			SetHighColor( finalFrameColor );
			StrokeRoundRect( rect, 3.0, 3.0 );
		}

		ConstrainClippingRegion(NULL);
	
	} else {
		glyph.SetFrame(bounds);
	}
	
	// Draw Button
	PushState();
	glyph.Draw(this);
	PopState();
	
	// Draw button label
	if (pressing)
		SetHighColor( textColor.mix_copy(finalContentColor, 100) );
	else
		SetHighColor( textColor );
	rect = glyph.Frame();
	offset.x = (rect.Width()-str_width)/2 + rect.left;
	offset.y = (rect.Height()-fontInfo.ascent)/2 + rect.top + fontInfo.ascent-1;
	SetLowColor( finalContentColor );
	DrawString( label, offset );
	
	if (focus && !IsInvalidatePending())
		InvalidateAtTime(NextFocusTime());
}

/*------------------------------------------------------------*/

void	BButton::MouseDown(BPoint where)
{
	BRect		bound;
	ulong		buttons;

	if (!IsEnabled())
		return;

	bound = Bounds();
	SetValue(1);
	if (Window()) Window()->UpdateIfNeeded();
	
	if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
//+		TRACE();
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		return;
	}

	do {
		snooze(40000);
		GetMouse(&where, &buttons);
		if (bound.Contains(where) != Value()) {
			SetValue(Value() ^ 0x01);
		if (Window()) Window()->UpdateIfNeeded();
		}
	} while(buttons);

	if (Value()) {
		Invoke();		// Invoke will reset the value to 0
	}
}

/*------------------------------------------------------------*/

void	BButton::SetLabel(const char *text)
{
	fCachedWidth = -1;
	BControl::SetLabel(text);
}

/*------------------------------------------------------------*/

void	BButton::SetValue(int32 val)
{
	BControl::SetValue(val);
}

/*------------------------------------------------------------*/

status_t	BButton::Invoke(BMessage *msg)
{
	status_t e = BControl::Invoke(msg);

	// BButton never hold their state. So after Invoking reset the value
	SetValue(0);
	if (Window()) Window()->UpdateIfNeeded();

	return e;
}

/*-------------------------------------------------------------*/

void
BButton::GetPreferredSize(
	float	*width,
	float	*height)
{
	BFont theFont;
	GetFont(&theFont);

	*width = ceil((Label() ? theFont.StringWidth(Label()) : 0) + 20.0);
	*width = (*width < 75.0) ? 75.0 : *width;

	font_height	info;
	theFont.GetHeight(&info);
	// need 12 pixels in addition to the size of text
	// allow for increased size of default button.
	*height = ceil(12 + (fDrawAsDefault ? 6 : 0) + 
				   info.ascent + info.descent);

}

/*-------------------------------------------------------------*/

void
BButton::ResizeToPreferred()
{
	BControl::ResizeToPreferred();
}

/*-------------------------------------------------------------*/

BButton &BButton::operator=(const BButton &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BButton::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BControl::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BButton::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BButton::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BButton::MessageReceived(BMessage *msg)
{
	BControl::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BButton::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BButton::MouseUp(BPoint pt)
{
	if (IsTracking()) {
//+		PRINT(("MouseUp(%.1f, %.1f)\n", pt.x, pt.y));
		if (Bounds().Contains(pt)) {
			PRINT(("Invoking button\n"));
			Invoke();		// Invoke will reset the value to 0
		}
		SetTracking(false);
	}
}

/* ---------------------------------------------------------------- */

void BButton::MouseMoved(BPoint pt, uint32 , const BMessage *)
{
	if (IsTracking()) {
		if (Bounds().Contains(pt) != Value()) {
			SetValue(Value() ^ 0x01);
		}
//+		PRINT(("MouseMoved(%.1f, %.1f, code=%d)\n", pt.x, pt.y, code));
	}
}

/*---------------------------------------------------------------*/

void	BButton::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BButton::FrameResized(float new_width, float new_height)
{
	BControl::FrameResized(new_width, new_height);
}

/*---------------------------------------------------------------*/

void	BButton::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void	BButton::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllAttached();
}

/*---------------------------------------------------------------*/

void	BButton::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllDetached();
}

void BButton::_ReservedButton1() {}
void BButton::_ReservedButton2() {}
void BButton::_ReservedButton3() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
