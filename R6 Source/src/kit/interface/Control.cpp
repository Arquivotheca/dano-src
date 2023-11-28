//******************************************************************************
//
//	File:		Control.cpp
//
//	Description:	base control view class imp[ementation
//
//	Written by:	Peter Potrebic
//
//	Copyright 1993-96, Be Incorporated
//
//******************************************************************************
 
#ifndef _DEBUG_H
#include <Debug.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef	_CONTROL_H
#include "Control.h"
#endif
#ifndef _FONT_H
#include "Font.h"
#endif
#ifndef	_WINDOW_H
#include "Window.h"
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#include <PropertyInfo.h>

#include <fbc.h>

/*------------------------------------------------------------*/

BControl::BControl(BRect r, const char *name, const char* text,
			BMessage *message, uint32 resizeMask, uint32 flags) :
	BView(r, name, resizeMask, flags), BInvoker()
{
	InitData();

	SetLabel(text);
	SetMessage(message);
}

/*------------------------------------------------------------*/

BControl::BControl(BMessage *data)
	: BView(data), BInvoker()
{
	InitData(data);

	if (data->HasMessage(S_MESSAGE)) {
		BMessage	*msg = new BMessage();
		data->FindMessage(S_MESSAGE, msg);
		SetMessage(msg);
	}

	const char	*str;
	long		l;
	bool		b;
	if (data->HasString(S_LABEL)) {
		data->FindString(S_LABEL, &str);
		SetLabel(str);
	}

	if (data->HasInt32(S_VALUE)) {
		data->FindInt32(S_VALUE, &l);
		SetValue(l);
	}

	if (data->HasBool(S_DISABLED)) {
		data->FindBool(S_DISABLED, &b);
		SetEnabled(!b);
	}

	// backwards compatibility -- BControl used to set this if
	// the control was navigable but not enabled
	if (data->HasBool(S_WANTS_NAVIGATION)) {
		data->FindBool(S_WANTS_NAVIGATION, &b);
		if (b) BView::SetFlags(Flags()|B_NAVIGABLE);
	}
}

/*------------------------------------------------------------*/

BArchivable *BControl::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BControl"))
		return NULL;
	return new BControl(data);
}

/*------------------------------------------------------------*/

status_t BControl::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	BMessage *msg = Message();
	if (msg) {
		data->AddMessage(S_MESSAGE, msg);
	}

	if (fLabel)
		data->AddString(S_LABEL, fLabel);

	if (fValue != 0)
		data->AddInt32(S_VALUE, fValue);

	if (!fEnabled)
		data->AddBool(S_DISABLED, TRUE);

	// can't save reference to the controls target ???
	return 0;
}

/*------------------------------------------------------------*/

void BControl::InitData(BMessage *data)
{
	fValue = 0;
	fLabel = NULL;
	fEnabled = TRUE;
	fFocusChanging = FALSE;
	fTracking = false;
	fInvalidate = false;
	fPressed = false;
	
	if (!data || !data->HasString(S_FONT_FAMILY_STYLE))
		SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
	BFont f;
	//f.SetSpacing(B_STRING_SPACING);
	BView::SetFont(&f, B_FONT_SPACING);
}

/*------------------------------------------------------------*/

BControl::~BControl()
{
	if (fLabel)
		free(fLabel);
	SetMessage((BMessage *) NULL);
}

/*------------------------------------------------------------*/

void	BControl::SetEnabled(bool on)
{
	if (fEnabled != on) {
		fEnabled = on;
		if (Window()) {
			UpdateView();
		}
	}
}

/*------------------------------------------------------------*/

bool	BControl::IsEnabled() const
{
	return fEnabled;
}

/*------------------------------------------------------------*/

void	BControl::SetPressed(bool on)
{
	if (fPressed != on) {
		fPressed = on;
		if (Window()) {
			UpdateView();
		}
	}
}

/*------------------------------------------------------------*/

bool	BControl::IsPressed() const
{
	return fPressed;
}

/*------------------------------------------------------------*/

void	BControl::AttachedToWindow()
{
	SetColorsFromParent();

	// if target/looper wasn't set then default to window
	if (!Messenger().IsValid()) 
		SetTarget(Window());
}


/* -------------------------------------------------------------------- */

void BControl::WindowActivated(bool active)
{
	BView::WindowActivated(active);
	if (IsFocus()) {
		UpdateView();
	}
}

/*------------------------------------------------------------*/

void	BControl::MakeFocus(bool state)
{
	if (state == IsFocus())
		return;

	if (state && !IsEnabled() && IsNavigating()) {
		// if this control is disabled and the user is trying to
		// tab-navigate into it, don't take focus.
		return;
	}
	
	BView::MakeFocus(state);
	if (Window()) {
		fFocusChanging = TRUE;
		UpdateView();
		fFocusChanging = FALSE;
		Flush();
	}
}

/*------------------------------------------------------------*/

void	BControl::KeyDown(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_ENTER:
		case B_SPACE:
			if (IsEnabled()) {
#if _R5_COMPATIBLE_
				// A gross kludge since BControl didn't used to
				// implement KeyDown().
				if (Window()) Window()->fKeyIntercept = this;
#endif
				SetPressed(true);
				if (Window()) Window()->UpdateIfNeeded();
			}
			break;
		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}

/* ---------------------------------------------------------------- */

void	BControl::KeyUp(const char *bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_ENTER:
		case B_SPACE:
#if _R5_COMPATIBLE_
			if (Window()) Window()->fKeyIntercept = NULL;
#endif
			if (IsEnabled() && IsPressed()) {
				fPressed = false;
				SetValue(!Value());
				if (Window()) Window()->UpdateIfNeeded();
				Invoke();
			}
			break;
		default:
			BView::KeyUp(bytes, numBytes);
			break;
	}
}

/* ---------------------------------------------------------------- */

status_t BControl::Invoke(BMessage *msg)
{
//+	PRINT(("invoking %s\n", Name()));
	
	bool notify = false;
	uint32 kind = InvokeKind(&notify);
	
	BMessage clone(kind);
	status_t err = B_BAD_VALUE;
	
	if (!msg && !notify) {
		// If no message is supplied, pull it from the BInvoker.
		// However, ONLY do so if this is not an InvokeNotify()
		// context -- otherwise, this is not the default invocation
		// message, so we don't want it to get in the way here.
		// For example, a control may call InvokeNotify() with their
		// "modification" message...  if that message isn't set,
		// we still want to send notification to any watchers, but
		// -don't- want to send a message through the invoker.
		msg = Message();
	}
	if (!msg) {
		// If not being watched, there is nothing to do.
		if( !IsWatched() ) return err;
	} else {
		clone = *msg;
	}

	clone.SetWhen(system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("be:value",fValue);
	clone.AddMessenger(B_NOTIFICATION_SENDER, BMessenger(this));
	if( msg ) err = BInvoker::Invoke(&clone);
	
	// Also send invocation to any observers of this handler.
	SendNotices(kind, &clone);
	
	return err;
}

/* ---------------------------------------------------------------- */

void	BControl::SetLabel(const char *text)
{
	if (fLabel && text && (strcmp(fLabel, text) == 0))
		return;

	if (fLabel) {
		free(fLabel);
		fLabel = NULL;
	}
	if (text)
		fLabel = strdup(text);
	Invalidate();
}

/*------------------------------------------------------------*/

const char*	BControl::Label() const
{
	return(fLabel);
}

/*------------------------------------------------------------*/

void	BControl::SetValue(int32 value)
{
	if (fValue != value) {
		fValue = value;
		if (Window()) {
			UpdateView();
		}
	}
}

void 
BControl::SetValueNoUpdate(int32 value)
{
	fValue = value;
}

/*------------------------------------------------------------*/

int32	BControl::Value() const
{
	return fValue;
}

/*------------------------------------------------------------*/

bool	BControl::IsFocusChanging() const
{
	return fFocusChanging;
}

/*-------------------------------------------------------------*/

void
BControl::GetPreferredSize(
	float	*width,
	float	*height)
{	
	BView::GetPreferredSize(width, height);
}

/*-------------------------------------------------------------*/

void
BControl::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}

/*-------------------------------------------------------------*/

void BControl::MessageReceived(BMessage *msg)
{
//+	PRINT(("what = %.4s\n", (char *) &(msg->what)));
	bool 		handled = false;
	BMessage	reply(B_REPLY);

#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err;
	
	switch (msg->what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BMessage	specifier;
			int32		form;
			const char	*prop;
			int32		cur;
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if (!err && (strcmp(prop, "Label") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddString("result", Label());
					handled = true;
				} else {
					const char *label;
					err = msg->FindString("data", &label);
					if (!err) {
						SetLabel(label);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if (!err && (strcmp(prop, "Value") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddInt32("result", Value());
					handled = true;
				} else {
					int32 value = -11;
					err = msg->FindInt32("data", &value);
					if (!err) {
						int32 old = Value();
						SetValue(value);
						if (old != value)
							Invoke();
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
			else if (!err && (strcmp(prop, "Enabled") == 0)) {
				if (msg->what == B_GET_PROPERTY) {
					reply.AddBool("result", IsEnabled());
					handled = true;
				} else {
					bool	value;
					err = msg->FindBool("data", &value);
					if (!err) {
						SetEnabled(value);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
			break;
		}
	}
#endif

	if (handled)
		msg->SendReply(&reply);
	else
		BView::MessageReceived(msg);
}

/*-------------------------------------------------------------*/
	/*
	 A generic BControl supports the following:
	 	GET/SET		"Value"		DIRECT form only
	 	GET/SET		"Label"		DIRECT form only
	 	GET/SET		"Enabled"	DIRECT form only
	*/

#if _SUPPORTS_FEATURE_SCRIPTING
static property_info	prop_list[] = {
	{"Value",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{B_INT32_TYPE},
		{},
		{}
	},
	{"Label",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{B_STRING_TYPE},
		{},
		{}
	},
	{"Enabled",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{B_BOOL_TYPE},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};
#endif

/*-------------------------------------------------------------*/

BHandler *BControl::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(prop))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BHandler	*target = NULL;
	BPropertyInfo	pi(prop_list);
	int32			i;

//+	PRINT(("BControl::Resolve: msg->what=%.4s, index=%d, form=0x%x, prop=%s\n",
//+		(char*) &(msg->what), index, spec->what, prop));

	if ((i = pi.FindMatch(msg, index, spec, form, prop)) >= 0) {
//+		PRINT(("prop=%s, i=%d\n", prop, i));
		target = this;
	} else {
		target = BView::ResolveSpecifier(msg, index, spec, form, prop);
	}

	return target;
#else
	return NULL;
#endif
}

/*-------------------------------------------------------------*/

status_t	BControl::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-control");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return BView::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}

/*----------------------------------------------------------------*/

bool BControl::IsTracking() const
{
	return fTracking;
}

/*----------------------------------------------------------------*/

void BControl::SetTracking(bool state)
{
	fTracking = state;
}

/*-------------------------------------------------------------*/

BControl &BControl::operator=(const BControl &) { return *this; }

/*----------------------------------------------------------------*/

status_t BControl::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BControl::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BControl::MouseDown(BPoint pt)
{
	BView::MouseDown(pt);
}

/* ---------------------------------------------------------------- */

void BControl::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

/* ---------------------------------------------------------------- */

void BControl::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

/* ---------------------------------------------------------------- */

void	BControl::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllAttached();
}

/*---------------------------------------------------------------*/

void	BControl::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllDetached();
}

/*---------------------------------------------------------------*/

void	BControl::SetFont(const BFont *font, uint32 mask)
{
	/*
	 This function/override did not exist in R5. So if adding functionality
	 here must consider the implications of that fact.  It is added now
	 because we don't want to lose B_STRING_SPACING if the programmer
	 changes the font.  (But losing it with old programs isn't that big a
	 deal.)
	*/
	BView::SetFont(font, mask/*&~B_FONT_SPACING*/);
}

/*---------------------------------------------------------------*/

void	BControl::SetInvalidate(bool invalidate)
{
	fInvalidate = invalidate;
}

/*---------------------------------------------------------------*/

void	BControl::UpdateView()
{
	if( fInvalidate )
		Invalidate();
	else
	{
		Draw( Bounds() );
		Flush();
	}
}

// Backwards compatibility

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT void
	#if __GNUC__
	_ReservedControl1__8BControl
	#elif __MWERKS__
	_ReservedControl1__8BControlFv
	#endif
	(BControl* This, bool on)
	{
		This->BControl::SetPressed(on);
	}
}
#endif

//void BControl::_ReservedControl1() {}
void BControl::_ReservedControl2() {}
void BControl::_ReservedControl3() {}
void BControl::_ReservedControl4() {}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/
