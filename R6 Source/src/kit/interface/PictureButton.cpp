/*****************************************************************************

     File:			PictureButton.cpp

     Description:	BControl subclass for buttons that display themselves
					with picture bitmaps

     Written By:	Peter Potrebic

     Copyright (c) 1994-97 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#include <string.h>

#include <ClassInfo.h>
#include <Debug.h>
#include <OS.h>
#include <PictureButton.h>
#include <Window.h>
#include <archive_defs.h>
#include <interface_misc.h>

/*------------------------------------------------------------*/

BPictureButton::BPictureButton(BRect r, const char* name,
		   BPicture *off, BPicture *on,
		   BMessage *msg, uint32 behavior, uint32 resizeMask, uint32 flags)
   :BControl(r, name, B_EMPTY_STRING, msg, resizeMask, flags)
{
	InitData();

	fEnabledOff = new BPicture(*off);
	fEnabledOn = new BPicture(*on);

	fBehavior = behavior;
}

/*------------------------------------------------------------*/

void BPictureButton::InitData()
{
	fEnabledOff = NULL;
	fEnabledOn = NULL;
	fDisabledOff = NULL;
	fDisabledOn = NULL;

	fOutlined = FALSE;
	fBehavior = B_ONE_STATE_BUTTON;
}

/*------------------------------------------------------------*/

BPictureButton::BPictureButton(BMessage *data)
	: BControl(data)
{
	BMessage	archive;
	BArchivable		*obj;

	InitData();

	if (data->FindMessage(S_ENABLED_ON, &archive) == B_OK) {
		obj = instantiate_object(&archive);
		if (obj)
			fEnabledOn = cast_as(obj, BPicture);
	}
	if (data->FindMessage(S_ENABLED_OFF, &archive) == B_OK) {
		obj = instantiate_object(&archive);
		if (obj)
			fEnabledOff = cast_as(obj, BPicture);
	}

	if (data->FindMessage(S_DISABLED_ON, &archive) == B_OK) {
		obj = instantiate_object(&archive);
		if (obj)
			fDisabledOn = cast_as(obj, BPicture);
	}
	if (data->FindMessage(S_DISABLED_OFF, &archive) == B_OK) {
		obj = instantiate_object(&archive);
		if (obj)
			fDisabledOff = cast_as(obj, BPicture);
	}

	if (data->HasInt32(S_BEHAVIOR)) {
		long l;
		data->FindInt32(S_BEHAVIOR, &l);
		SetBehavior(l);
	}
}

/*------------------------------------------------------------*/

BArchivable *BPictureButton::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BPictureButton"))
		return NULL;
	return new BPictureButton(data);
}

/*------------------------------------------------------------*/

status_t BPictureButton::Archive(BMessage *data, bool deep) const
{
	BControl::Archive(data, deep);
//+	data->AddString(B_CLASS_NAME_ENTRY, "BPictureButton");

	long		err;

	if (deep) {
		if (fEnabledOn) {
			BMessage	archive;
			err = fEnabledOn->Archive(&archive);
			if (!err)
				data->AddMessage(S_ENABLED_ON, &archive);
		}

		if (fEnabledOff) {
			BMessage	archive;
			err = fEnabledOff->Archive(&archive);
			if (!err)
				data->AddMessage(S_ENABLED_OFF, &archive);
		}

		if (fDisabledOn) {
			BMessage	archive;
			err = fDisabledOn->Archive(&archive);
			if (!err)
				data->AddMessage(S_DISABLED_ON, &archive);
		}

		if (fDisabledOff) {
			BMessage	archive;
			err = fDisabledOff->Archive(&archive);
			if (!err)
				data->AddMessage(S_DISABLED_OFF, &archive);
		}
	}

	data->AddInt32(S_BEHAVIOR, Behavior());
	return 0;
}

/*------------------------------------------------------------*/

BPictureButton::~BPictureButton()
{
	delete(fEnabledOff);
	delete(fEnabledOn);
	if (fDisabledOff)
		delete(fDisabledOff);
	if (fDisabledOn)
		delete(fDisabledOn);
}

/*------------------------------------------------------------*/

void	BPictureButton::KeyDown(const char *bytes, int32 numBytes)
{
	uchar key = bytes[0];

	switch (key) {
		case B_ENTER:
		case B_SPACE:
			if (IsEnabled()) {
				if (fBehavior == B_ONE_STATE_BUTTON) {
					SetValue(1);
					Invoke();
				} else {
					SetValue(!Value());
					Sync();
					Invoke();
				}
			}
			break;
		default:
			BControl::KeyDown(bytes, numBytes);
			break;
	}
}

/*------------------------------------------------------------*/

void	BPictureButton::MouseDown(BPoint where)
{
	BRect	bounds;
	ulong	buttons;

	// make sure button is not disabled
	if (!IsEnabled())
		return;

	bounds = Bounds();

	if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
		if (fBehavior == B_ONE_STATE_BUTTON) {
			SetValue(1);
		} else {
			fOutlined = TRUE;
			SetValue(!Value());
		}
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		return;
	}

	if (fBehavior == B_ONE_STATE_BUTTON) {
		SetValue(1);

		do {
			snooze(30000);
			GetMouse(&where, &buttons);
			if (bounds.Contains(where) != Value()) {
				SetValue(!Value());
			}
		} while(buttons);

		if (Value()) {
			Invoke();
		}
	} else {
		fOutlined = TRUE;
//+		PRINT(("Start value=%d\n", Value()));
		SetValue(!Value());

		do {
			snooze(30000);
			GetMouse(&where, &buttons);
			if (bounds.Contains(where) != fOutlined) {
				fOutlined = !fOutlined;
//+				PRINT(("Change value=%d (%d)\n", Value(), fOutlined));
				SetValue(!Value());
			}
		} while(buttons);

//+		PRINT(("End value=%d (%d)\n", Value(), fOutlined));
		if (fOutlined) {
			fOutlined = FALSE;
			// Sticky state 
			Invoke();
		}
	}
}

/*------------------------------------------------------------*/

void	BPictureButton::Draw(BRect)
{
	bool	enabled;
	bool	foc = IsFocus() && Window()->IsActive();
	enabled = IsEnabled();

	rgb_color c = HighColor();

//+	PRINT(("Draw: value=%d, outlined=%d\n", Value(), fOutlined));

	if (!foc) {
		BRect b = Bounds();
		SetHighColor(ViewColor());
		StrokeRect(b);
		SetHighColor(c);
	}

	if (enabled) {
		if ((Value() == 1)) {
			DrawPicture(fEnabledOn, B_ORIGIN);
		} else {
			DrawPicture(fEnabledOff, B_ORIGIN);
		}
	} else {
		if (!fDisabledOff || (!fDisabledOn && (fBehavior == B_TWO_STATE_BUTTON)))
			debugger("Need to set the 'disabled' pictures for this BPictureButton");
		if ((Value() == 1)) {
			DrawPicture(fDisabledOn, B_ORIGIN);
		} else {
			DrawPicture(fDisabledOff, B_ORIGIN);
		}
	}

	if (foc) {
		BRect b = Bounds();
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(b);
		SetHighColor(c);
	}
}

/*------------------------------------------------------------*/

void	BPictureButton::Redraw()
{
	if (LockLooper()) {
		Draw(Bounds());
		Flush();
		UnlockLooper();
	}
}

/*------------------------------------------------------------*/

void BPictureButton::SetBehavior(uint32 behavior)
{
	fBehavior = behavior;
}

/*------------------------------------------------------------*/

uint32 BPictureButton::Behavior() const
{
	return fBehavior;
}

/*------------------------------------------------------------*/

void BPictureButton::SetEnabledOn(BPicture *on) 
{
	ASSERT(on);
	delete fEnabledOn;
	fEnabledOn = new BPicture(*on);
}

/*------------------------------------------------------------*/

void BPictureButton::SetEnabledOff(BPicture *off) 
{
	ASSERT(off);
	delete fEnabledOff;
	fEnabledOff = new BPicture(*off);
}

/*------------------------------------------------------------*/

void BPictureButton::SetDisabledOn(BPicture *on) 
{
	ASSERT(on);
	delete fDisabledOn;
	fDisabledOn = new BPicture(*on);
}

/*------------------------------------------------------------*/

void BPictureButton::SetDisabledOff(BPicture *off) 
{
	ASSERT(off);
	delete fDisabledOff;
	fDisabledOff = new BPicture(*off);
}

/*------------------------------------------------------------*/
BPicture *BPictureButton::EnabledOn() const
	{ return fEnabledOn; };

/*------------------------------------------------------------*/
BPicture *BPictureButton::EnabledOff() const
	{ return fEnabledOff; };

/*------------------------------------------------------------*/
BPicture *BPictureButton::DisabledOn() const
	{ return fDisabledOn; };

/*------------------------------------------------------------*/
BPicture *BPictureButton::DisabledOff() const
	{ return fDisabledOff; };

/*------------------------------------------------------------*/

void	BPictureButton::SetValue(int32 val)
{
	BControl::SetValue(val);
}

/*------------------------------------------------------------*/

status_t	BPictureButton::Invoke(BMessage *msg)
{
	status_t err;
	if (fBehavior == B_ONE_STATE_BUTTON) {
		Sync();
		snooze(50000);
		err = BControl::Invoke(msg);
		SetValue(0);
	} else {
		err = BControl::Invoke(msg);
	}
	return err;
}

/*------------------------------------------------------------*/

BPictureButton &BPictureButton::operator=(const BPictureButton &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BPictureButton::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BControl::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BPictureButton::GetSupportedSuites(BMessage *data)
{
	return BControl::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BPictureButton::Perform(perform_code d, void *arg)
{
	return BControl::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BPictureButton::MessageReceived(BMessage *msg)
{
	BControl::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BPictureButton::AttachedToWindow()
{
	BControl::AttachedToWindow();
}

/* ---------------------------------------------------------------- */

void BPictureButton::DetachedFromWindow()
{
	BControl::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BPictureButton::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BPictureButton::MouseUp(BPoint pt)
{
	if (IsTracking()) {
		BRect	bounds = Bounds();
		if (fBehavior == B_ONE_STATE_BUTTON) {
			if (bounds.Contains(pt)) {
				Invoke();
//+				PRINT(("async Invoke\n"));
			}
		} else {
			if (bounds.Contains(pt) != fOutlined) {
				fOutlined = !fOutlined;
				SetValue(!Value());
			}

			if (fOutlined) {
				fOutlined = false;
				// Sticky state 
				Invoke();
//+				PRINT(("async Invoke\n"));
			}
		}
		SetTracking(false);
	}
}

/* ---------------------------------------------------------------- */

void BPictureButton::MouseMoved(BPoint pt, uint32 , const BMessage *)
{
	if (IsTracking()) {
		BRect	bounds = Bounds();
		if (fBehavior == B_ONE_STATE_BUTTON) {
			if (bounds.Contains(pt) != Value()) {
//+				PRINT(("async Change value=%d (%d)\n", Value(), fOutlined));
				SetValue(!Value());
			}
		} else {
			if (bounds.Contains(pt) != fOutlined) {
				fOutlined = !fOutlined;
//+				PRINT(("async Change value=%d (%d)\n", Value(), fOutlined));
				SetValue(!Value());
			}
		}
	}
}

/*---------------------------------------------------------------*/

void	BPictureButton::FrameMoved(BPoint new_position)
{
	BControl::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BPictureButton::FrameResized(float new_width, float new_height)
{
	BControl::FrameResized(new_width, new_height);
}

/*---------------------------------------------------------------*/

void BPictureButton::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BPictureButton::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::GetPreferredSize(width, height);
}

/*---------------------------------------------------------------*/

void BPictureButton::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void BPictureButton::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllAttached();
}

/*---------------------------------------------------------------*/

void BPictureButton::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BControl::AllDetached();
}


void BPictureButton::_ReservedPictureButton1() {}
void BPictureButton::_ReservedPictureButton2() {}
void BPictureButton::_ReservedPictureButton3() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
