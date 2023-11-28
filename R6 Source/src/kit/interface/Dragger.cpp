//*************************************************************************
//
//	File:		Dragger.cpp
//
//	Written By:	Peter Potrebic
//
//	Copyright 1997, Be Incorporated
//
//*************************************************************************

#include <Debug.h>
#include <string.h>

#include <Dragger.h>
#include <AutoLock.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Alert.h>
#include <Bitmap.h>
#include <Shelf.h>

#ifndef _BEEP_H
#include <Beep.h>
#endif

#include <AppDefsPrivate.h>
#include <roster_private.h>
#include <archive_defs.h>
#include <interface_misc.h>

/*------------------------------------------------------------*/

bool	BDragger::sVisible = false;
bool	BDragger::sInited = false;
BLocker	BDragger::sLock;
BList	BDragger::sList(5);

/*------------------------------------------------------------*/

#define TANN	131

unsigned char dragger_data[] = {
	0xff,	0xff,	0x00,	0x00,	0x00,	0xff,	0xff,	0xff,
	0xff,	0xff,	0x00,	TANN,	TANN,	0x00,	0xff,	0xff,
	0x00,	0x00,	0x00,	0x00,	TANN,	TANN,	0x00,	0x00,
	0x00,	TANN,	0x00,	0x00,	TANN,	TANN,	0x00,	0x00,
	0x00,	TANN,	TANN,	TANN,	TANN,	TANN,	0x00,	0x00,
	0xff,	0x00,	TANN,	TANN,	TANN,	TANN,	0x00,	0x00,
	0xff,	0xff,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0x00,	0x00
};

/*------------------------------------------------------------*/

void _toggle_handles_(bool visible)
{
	if (!BDragger::sLock.Lock())
		return;

	BDragger::sVisible = visible;

	BDragger	*dw;
	long		i = 0;

	BMessage msg(_SHOW_DRAG_HANDLES_);

	while ((dw = (BDragger*) BDragger::sList.ItemAt(i++)) != 0) {
		BWindow *w = dw->Window();
		(void) w->PostMessage(&msg, dw);	// ignore error
		// it's OK to ignore error above. If for some reason the Post
		// fails it just means that the corresponding replicant won't
		// toggle it's visible state.
	}

	BDragger::sLock.Unlock();
}

/* -------------------------------------------------------------------- */
BDragger::BDragger(BRect bounds, BView *target,
	uint32 rmask, uint32 flags)
	: BView(bounds, "_dragger_", rmask, flags)
{
	fTarget = target;
	fRelation = TARGET_UNKNOWN;
	fShelf = NULL;
	fTransition = false;
	fIsZombie = false;
	fErrCount = 0;
	fPopUp = NULL;
	fBitmap = new BBitmap(BRect(0,0,7,7), B_COLOR_8_BIT);
	fBitmap->SetBits(dragger_data, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);
}

/*------------------------------------------------------------*/

BDragger::BDragger(BMessage *data)
	: BView(data)
{
	fTarget = NULL;
	data->FindInt32("_rel", (long*) &fRelation);
	fTransition = false;
	fIsZombie = false;
	fErrCount = 0;
	fBitmap = new BBitmap(BRect(0,0,7,7), B_COLOR_8_BIT);
	fBitmap->SetBits(dragger_data, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);

	fPopUp = NULL;

	BMessage	menu_archive;
	if (data->FindMessage("_popup", &menu_archive) == B_OK) {
		BArchivable *a = instantiate_object(&menu_archive);
		if (a)
			fPopUp = dynamic_cast<BPopUpMenu*>(a);
	}
}

/* -------------------------------------------------------------------- */

void BDragger::ListManage(bool add)
{
	if (!BDragger::sLock.Lock())
		return;

	// call AreDraggersDrawn to ensure that the sVisible flag is initialized
	bool	vis = AreDraggersDrawn();

	if (add) {
		sList.AddItem(this);
		bool allow_drag = true;
		if (fShelf)
			allow_drag = fShelf->AllowsDragging();
		if ((!vis || !allow_drag) && (fRelation != TARGET_IS_CHILD))
			Hide();
	} else {
		sList.RemoveItem(this);
	}

	BDragger::sLock.Unlock();
}

/* -------------------------------------------------------------------- */

BDragger::~BDragger()
{
	SetPopUp(NULL);
	delete fBitmap;
}

/*------------------------------------------------------------*/

BBitmap *BDragger::DragBitmap(BPoint *, drawing_mode *)
{
	return NULL;
}

/*------------------------------------------------------------*/

BArchivable *BDragger::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BDragger"))
		return NULL;
	return new BDragger(data);
}

/* -------------------------------------------------------------------- */

status_t BDragger::Archive(BMessage *data, bool deep) const
{
	BMessage	menu_archive;
	if (fPopUp) {
		fPopUp->Archive(&menu_archive);
		data->AddMessage("_popup", &menu_archive);
	}
	data->AddInt32("_rel", fRelation);
	return BView::Archive(data, deep);
}

/* -------------------------------------------------------------------- */

status_t BDragger::SetViewToDrag(BView *target)
{
	if (target->Window() != Window())
		return B_MISMATCHED_VALUES;

	fTarget = target;

	if (fTarget && Window())
		determine_relationship();

	return 0;
}

/* -------------------------------------------------------------------- */

status_t BDragger::determine_relationship()
{
	long err = B_NO_ERROR;

	// determine our relationship to the 'target' view
	if (fTarget) {
		// if we know the 'target', then determine the relationship
		if (fTarget == Parent())
			fRelation = TARGET_IS_PARENT;
		else if (ChildAt(0) == fTarget)
			fRelation = TARGET_IS_CHILD;
		else
			fRelation = TARGET_IS_SIBLING;
	} else {
		// is we know the 'relationship', then determine the target
		if (fRelation == TARGET_IS_PARENT)
			fTarget = Parent();
		else if (fRelation == TARGET_IS_CHILD)
			fTarget = ChildAt(0);
		else
			err = B_ERROR;
	}
	return err;
}

/* -------------------------------------------------------------------- */

void BDragger::DetachedFromWindow()
{
	ListManage(false);
}

/* -------------------------------------------------------------------- */

void BDragger::AttachedToWindow()
{
	if (fIsZombie) {
		SetViewColor(220,220,220,255);
		SetLowColor(220,220,220,255);
	} else {
		SetViewColor(B_TRANSPARENT_32_BIT);
		SetLowColor(B_TRANSPARENT_32_BIT);
	}

	determine_relationship();
	ListManage(true);
}

/* -------------------------------------------------------------------- */

void BDragger::MessageReceived(BMessage *msg)
{
//+	PRINT(("what=%.4s\n", (char*) &(msg->what)));
	if (msg->what == B_TRASH_TARGET) {
//+		PRINT(("fShelf=%x\n", fShelf));
		if (fShelf) {
			BMessage deleteView(CMD_DELETE_VIEW);
			Window()->PostMessage(&deleteView, fTarget);
		} else {
			BAlert *a = new BAlert("", "Can't delete this replicant from its original application. Life goes on.", "OK", NULL, NULL, B_WIDTH_AS_USUAL,
				B_WARNING_ALERT);
			a->Go(NULL);
		}
	} else if (msg->what == _SHOW_DRAG_HANDLES_) {
		// If relationship is TARGET_IS_SIBLING or TARGET_IS_PARENT then
		// we can Show/Hide the widget view as needed. If TAGET_IS_CHILD
		// then we can't Show/Hide because that would Show/Hide the real view.
		if (fRelation == TARGET_IS_CHILD) {
			fTransition = true;
			Draw(BRect(0,0,1,1));
			fTransition = false;
		} else {
			if (fShelf) {
				if (fShelf->AllowsDragging() && AreDraggersDrawn())
					Show();
				else
					Hide();
			} else if (AreDraggersDrawn())
				Show();
			else
				Hide();
//+			PRINT(("Show/Hide(%d), IsHidden=%d\n", AreDraggersDrawn(), IsHidden()));
		}
	} else
		BView::MessageReceived(msg);
}

/* -------------------------------------------------------------------- */

void BDragger::MouseDown(BPoint where)
{
	long	err;

	if (!fTarget || !AreDraggersDrawn())
		return;

	uint32	buttons;

	Window()->CurrentMessage()->FindInt32("buttons", (long*) &buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		if (fShelf && fTarget) {
			ShowPopUp(fTarget, where);
		} else {
			beep();
		}
	} else {
		BPoint		save = where;
		bigtime_t	dbl = 0;
		bigtime_t	start = system_time();
		uint32		buttons;
		bool		do_drag = false;

		get_click_speed(&dbl);

		do {
			if ((fabs(where.x - save.x) > 4) || (fabs(where.y - save.y) > 4) ) {
				do_drag = true;
				break;
			}
	
			if (fShelf && ((system_time() - start) > (2 * dbl))) {
				ShowPopUp(fTarget, where);
				break;
			}
	
			snooze(50000);
			GetMouse(&where, &buttons);
		} while (buttons);

		if (!do_drag)
			return;

		if (fIsZombie) {
			beep();
			fErrCount++;
			if (fErrCount > 2) {
				BAlert	*alert;
				alert = new BAlert("Err",
					"This is a zombied replicant. It can't be moved", "OK",
					NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go(NULL);
				fErrCount = 0;
			}
			return;
		}

		BMessage msg(B_ARCHIVED_OBJECT);
		if (fRelation == TARGET_IS_PARENT) {
			err = fTarget->Archive(&msg, true);
		} else if (fRelation == TARGET_IS_CHILD) {
			err = Archive(&msg, true);
		} else {
			// sibling views. Need to archive up both! Slightly icky case
			err = fTarget->Archive(&msg, true);
			if (!err) {
				BMessage aux(B_ARCHIVED_OBJECT);

				// put the widget view itself into a field of the archive
				err = Archive(&aux, true);
				if (!err)
					err = msg.AddMessage("__widget", &aux);
			}
		}

		if (err)
			return;

		msg.AddInt32("be:actions", B_TRASH_TARGET);

		BPoint			offset;
		drawing_mode	mode;
		BBitmap			*bm = DragBitmap(&offset, &mode);

		if (!bm) {
			BRect r = fTarget->Bounds();
			r = fTarget->ConvertToScreen(r);
			r = ConvertFromScreen(r);
			DragMessage(&msg, r, this);
		} else {
			DragMessage(&msg, bm, mode, offset);
		}
	}
}

/* -------------------------------------------------------------------- */

void BDragger::Draw(BRect)
{
	BRect r = Bounds();
	r.top = r.bottom - 7;
	r.left = r.right - 7;
	bool	draw = AreDraggersDrawn();

	if (draw) {
		if (fShelf)
			draw = fShelf->AllowsDragging();
	}

	if (draw) {
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fBitmap, r.LeftTop());
		SetDrawingMode(B_OP_COPY);
		if (fIsZombie) {
			SetPenSize(2);
			SetHighColor(203, 0, 0);
			StrokeLine(r.LeftBottom(), r.RightTop());
		}
	} else if (IsVisibilityChanging()) {
		// becoming invisible so erase
		if (Parent()) {
			// invalid the parent. This view won't draw so the parent will
			// show through!
			BRect	frame = Frame();
			Parent()->Invalidate(frame);
		} else {
			// can't do anything in this case. Just erase to white.
			SetHighColor(255,255,255);
			FillRect(r);
		}
	}
}

/* -------------------------------------------------------------------- */

void BDragger::SetZombied(bool s)
{
	fIsZombie = s;
	SetViewColor(220,220,220,255);
	SetLowColor(220,220,220,255);
}

/* -------------------------------------------------------------------- */

BView *BDragger::Target() const
{
	return fTarget;
}

/* -------------------------------------------------------------------- */

bool BDragger::InShelf() const
{
	return (fShelf != NULL);
}

/* -------------------------------------------------------------------- */

void BDragger::SetShelf(BShelf *shelf)
{
	fShelf = shelf;
}

/* ---------------------------------------------------------------- */

status_t BDragger::SetPopUp(BPopUpMenu *menu)
{
	if (fPopUp && (fPopUp != menu))
		delete fPopUp;
	fPopUp = menu;
	return B_OK;
}

/* ---------------------------------------------------------------- */

BPopUpMenu *BDragger::PopUp() const
{
	// can only have a popup when there is a target view
	// all items in the pop post messages to that target.
	if ((fPopUp == NULL) && fTarget) {
		BDragger *THIS = const_cast<BDragger*>(this);
		THIS->BuildDefaultPopUp();
	}

	return fPopUp;
}

/* ---------------------------------------------------------------- */

void BDragger::BuildDefaultPopUp()
{
	ASSERT(fTarget);

	BMessage	*msg;
	BMenuItem	*item;

	fPopUp = new BPopUpMenu("Shelf", false, false);

	msg = new BMessage(B_ABOUT_REQUESTED);
	const char *view_name = fTarget->Name();
	if (view_name)
		msg->AddString("target", fTarget->Name());
	else
		view_name = "<unknown>";
	char *buffer = (char *) malloc(strlen(view_name) + 20);
	sprintf(buffer, "About %s", view_name);
	fPopUp->AddItem(item = new BMenuItem(buffer, msg));
	free(buffer);

	fPopUp->AddItem(new BSeparatorItem());

	msg = new BMessage(CMD_DELETE_VIEW);
	fPopUp->AddItem(item = new BMenuItem("Delete", msg));
}

/* ---------------------------------------------------------------- */

void BDragger::ShowPopUp(BView *, BPoint where)
{
	BPopUpMenu	*menu;

	where = ConvertToScreen(where);

	ASSERT(fTarget);

	menu = PopUp();

//+	PRINT(("setting menu target to %s\n", class_name(fTarget)));
	menu->SetTargetForItems(fTarget);

	// make a little 'sticky' area so that menu sticks if user releases
	// the mouse in this area. (Bug #12238)
	BRect	r(where.x-2, where.y-2, where.x+2, where.y+2);

	menu->Go(where, true, false, r, true);
}

/* -------------------------------------------------------------------- */

bool BDragger::AreDraggersDrawn()
{
	BAutoLock	st(sLock);

	if (!sInited) {
		// sInited can only be true the first time this is called.
		// The sLock semaphore ensures that there isn't a race condition.
		// So let's get the 'visible' state from the roster
		BMessage	msg(CMD_GET_DRAG_STATE);
		BMessage	reply;
		BMessenger	mess(ROSTER_SIG);

		sVisible = false;
		if (mess.SendMessage(&msg, &reply) == B_OK) {
			bool		b;
			reply.FindBool("visible", &b);
			sVisible = b;
			sInited = true;
		}
	}

	return sVisible;
}

/* -------------------------------------------------------------------- */

bool BDragger::IsVisibilityChanging() const
{
	return fTransition;
}

/* -------------------------------------------------------------------- */

status_t	BDragger::ShowAllDraggers()
{
	BMessenger	mess(ROSTER_MIME_SIG, -1, NULL);
	BMessage	msg(CMD_SET_DRAG_STATE);
	msg.AddBool("visible", true);
	status_t err = mess.SendMessage(&msg);
//+	PRINT(("ShowAll, err=%x\n", err));
	return err;
}

/* -------------------------------------------------------------------- */

status_t	BDragger::HideAllDraggers()
{
	BMessenger	mess(ROSTER_MIME_SIG, -1, NULL);
	BMessage	msg(CMD_SET_DRAG_STATE);
	msg.AddBool("visible", false);
	status_t err = mess.SendMessage(&msg);
//+	PRINT(("HideAll, err=%x\n", err));
	return err;
}

/*-------------------------------------------------------------*/

BDragger &BDragger::operator=(const BDragger &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BDragger::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BDragger::GetSupportedSuites(BMessage *data)
{
	return BView::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BDragger::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BDragger::MouseUp(BPoint pt)
{
	BView::MouseUp(pt);
}

/* ---------------------------------------------------------------- */

void BDragger::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BView::MouseMoved(pt, code, msg);
}

/*---------------------------------------------------------------*/

void	BDragger::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BDragger::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
}

/* ---------------------------------------------------------------- */

void
BDragger::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BDragger::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::GetPreferredSize(width, height);
}

/*---------------------------------------------------------------*/

void	BDragger::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void	BDragger::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllAttached();
}

/*---------------------------------------------------------------*/

void	BDragger::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllDetached();
}

//+void BDragger::_ReservedDragger1() {}
void BDragger::_ReservedDragger2() {}
void BDragger::_ReservedDragger3() {}
void BDragger::_ReservedDragger4() {}

/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
