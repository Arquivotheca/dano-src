//*****************************************************************************
//
//	File:		PopUpMenu.cpp
//
//	Written by:	Peter Potrebic
//
//	Copyright 1994-97, Be Incorporated
//
//*****************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _POP_UP_MENU_H
#include "PopUpMenu.h"
#endif
#ifndef _MENU_BAR_H
#include "MenuBar.h"
#endif
#ifndef _MENU_ITEM_H
#include "MenuItem.h"
#endif
#ifndef _VIEW_H
#include "View.h"
#endif
#ifndef _WINDOW_H
#include "Window.h"
#endif
#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#include <Application.h>
#include <ClassInfo.h>

/* ---------------------------------------------------------------- */

BPopUpMenu::BPopUpMenu(const char *title, bool radioMode, bool autoRename,
	menu_layout layout)
	: BMenu(title, layout)
{
	if (radioMode)
		SetRadioMode(TRUE);
	if (autoRename)
		SetLabelFromMarked(TRUE);
	
	fUseWhere = FALSE;
	fTrackThread = -1;
	fAutoDestruct = false;
}

/* ---------------------------------------------------------------- */

BPopUpMenu::~BPopUpMenu()
{
	if (fTrackThread) {
		int32		retval;
		status_t	err;
		do {
			err = wait_for_thread(fTrackThread, &retval);
		} while (err == B_INTERRUPTED);
	}
}

/* ---------------------------------------------------------------- */

BPopUpMenu::BPopUpMenu(BMessage *data)
	: BMenu(data)
{
	fUseWhere = FALSE;
	fTrackThread = -1;
	fAutoDestruct = false;
}

/* ---------------------------------------------------------------- */

status_t BPopUpMenu::Archive(BMessage *data, bool deep) const
{
	return BMenu::Archive(data, deep);
}

/* ---------------------------------------------------------------- */

BArchivable *BPopUpMenu::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BPopUpMenu"))
		return NULL;
	return new BPopUpMenu(data);
}

/* ---------------------------------------------------------------- */

void BPopUpMenu::SetAsyncAutoDestruct(bool state)
{
	fAutoDestruct = state;
}

/* ---------------------------------------------------------------- */

bool BPopUpMenu::AsyncAutoDestruct() const
{
	return fAutoDestruct;
}

/* ---------------------------------------------------------------- */

BPoint BPopUpMenu::ScreenLocation()
{
	if (fUseWhere)
		return fWhere;

	ASSERT((Supermenu()));
	ASSERT((Superitem()));
	
	BMenuItem	*parentItem = Superitem();
	BMenu		*parent = Supermenu();
	BMenuItem	*curItem;
	BRect		rect;
	BPoint		pt;
	
	const char *title = parentItem->Label();
	curItem = FindItem(title);
	
//	PRINT(("looking for \"%s\" (%d)\n", title, curItem != NULL));
	
	rect = parentItem->Frame();
	pt = rect.LeftTop();
	parent->ConvertToScreen(&pt);

	float frLeft, frTop, frRight, frBottom;
	GetFrameMargins(&frLeft, &frTop, &frRight, &frBottom);
	pt.x -= frLeft;
	pt.y -= frTop;
	
	if (curItem) {
		rect = curItem->Frame();
		pt.y -= rect.top;
	}
	
	return pt;
}

/* ---------------------------------------------------------------- */

BMenuItem *BPopUpMenu::Go(BPoint where, bool autoInvoke, bool is_sticky,
	bool async)
{
	return _go(where, autoInvoke, is_sticky, NULL, async);
}

/* ---------------------------------------------------------------- */

BMenuItem *BPopUpMenu::Go(BPoint where, bool autoInvoke, bool is_sticky,
	 BRect stick, bool async)
{
	return _go(where, autoInvoke, is_sticky, &stick, async);
}

/* ---------------------------------------------------------------- */

struct pop_data {
	BPopUpMenu	*menu;
	BPoint		where;
	bool		async;
	bool		auto_invoke;
	bool		is_sticky;
	bool		use_stick_rect;
	BRect		stick_rect;
	sem_id		block_sem;
	BWindow		*block_window;
	BMenuItem	*selected_item;
};

/* ---------------------------------------------------------------- */

BMenuItem *BPopUpMenu::_go(BPoint where, bool auto_invoke, bool is_sticky,
	 BRect *stick, bool async)
{
	BMenuItem	*item = NULL;
	BWindow		*window = NULL;
	sem_id		sem = B_NO_MORE_SEMS;

	// for async calls the spawned thread will delete this object.
	pop_data *data = (pop_data *) malloc(sizeof(pop_data));

	// Determine if this thread is a window
	// thread. If so then call UpdateIfNeeded to keep window updated.
	BLooper	*l = BLooper::LooperForThread(find_thread(NULL));
	if (l)
		window = cast_as(l, BWindow);

	data->block_window = NULL;
	data->block_sem = B_NO_MORE_SEMS;

	if (window) {
		// set this special semaphore that will prevent the owning window
		// from closing while the popup is being displayed.
		// Only need to do this when async is TRUE. Otherwise the window
		// is blocked waiting for this call to return.
		sem = data->block_sem = create_sem(0, "window close lock");
		if (async) {
			data->block_window = window;
			_set_menu_sem_(window, data->block_sem);
		}
	}

	data->async = async;
	data->selected_item = NULL;
	data->menu = this;
	data->where = where;
	data->auto_invoke = auto_invoke;
	data->is_sticky = is_sticky;
	data->use_stick_rect = (stick != NULL);
	if (stick)
		data->stick_rect = *stick;
	thread_id tid = spawn_thread(BPopUpMenu::entry, "popup",
		B_DISPLAY_PRIORITY, data);
	fTrackThread = tid;
	if (tid >= 0)
		resume_thread(tid);

	long err;
	if (!async) {
		// synchronous call.
		if (window) {
			// we have a window so let's keep it updated.
			while (1) {
				err = acquire_sem_etc(sem, 1, B_TIMEOUT, 50000);
				if (err == B_BAD_SEM_ID)
					break;
				window->UpdateIfNeeded();
			}
			// we're done. Fall through and call wait_for_thread to
			// get the return value.
		} 
		int32 retval = 0;
		do {
			err = wait_for_thread(tid, &retval);
		} while (err == B_INTERRUPTED);
		item = data->selected_item;
		free(data);
	}

	return item;
}

/* ---------------------------------------------------------------- */

BMenuItem *BPopUpMenu::start_track(BPoint where, bool auto_invoke,
	bool is_sticky, BRect *stick)
{
	/*
	 If the 'is_sticky' flag is TRUE then the popup will start off
	 in sticky tracking mode.
	 If a 'sticky' rect is specified (it should be in screen coords)
	 then, if the mouse is release in this area, the popup menu will
	 enter into 'sticky' mode, just like a std menu bar.
	*/
	BMenuItem	*item;

	extern bigtime_t	_gtrack_start_time_;
	extern bool			_gstartup_phase_;
	_gtrack_start_time_ = system_time();
	_gstartup_phase_ = true;

	fUseWhere = TRUE;
	fWhere = where;
	Show();
	snooze(50000);
	item = Track(is_sticky, stick);

	if (item && auto_invoke)
		item->Invoke();

	fUseWhere = FALSE;

	Hide();

	// in case cursor was hidden because of key navigating menus.
	be_app->ShowCursor();

	return item;
}

/* ---------------------------------------------------------------- */

int32 BPopUpMenu::entry(void *raw)
{
	pop_data	*data = (pop_data *) raw;
	BPopUpMenu	*menu = data->menu;
	BMenuItem	*item;

	if (data->use_stick_rect)
		item = menu->start_track(data->where, data->auto_invoke,
			data->is_sticky, &data->stick_rect);
	else
		item = menu->start_track(data->where, data->auto_invoke,
			data->is_sticky, NULL);

	if (data->block_sem != B_NO_MORE_SEMS) {
		delete_sem(data->block_sem);
		if (data->block_window)
			// a window was blocked (prevented from quitting).
			// remove that blockade.
			_set_menu_sem_(data->block_window, B_NO_MORE_SEMS);
	}

	bool reset_tracking = true;
	if (data->async) {
		if (menu->AsyncAutoDestruct()) {
			// Might look like a race condition here, but if there is
			// it's a bug on the person who set up the popup to hit this.
			menu->fTrackThread = -1;
			reset_tracking = false;
			delete menu;
		}
		free(data);
	} else {
		// synchronous call so caller would want the return value.
		data->selected_item = item;
	}

	if (reset_tracking)
		menu->fTrackThread = -1;

	return 0;
}

/*-------------------------------------------------------------*/

BPopUpMenu &BPopUpMenu::operator=(const BPopUpMenu &) {return *this;}

/*-------------------------------------------------------------*/

BHandler *BPopUpMenu::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BMenu::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BPopUpMenu::GetSupportedSuites(BMessage *data)
{
	return BMenu::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BPopUpMenu::Perform(perform_code d, void *arg)
{
	return BMenu::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BPopUpMenu::MessageReceived(BMessage *msg)
{
	BMenu::MessageReceived(msg);
}

/* ---------------------------------------------------------------- */

void BPopUpMenu::AttachedToWindow()
{
	BMenu::AttachedToWindow();
}

/* ---------------------------------------------------------------- */

void BPopUpMenu::DetachedFromWindow()
{
	BMenu::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BPopUpMenu::MouseDown(BPoint pt)
{
	BMenu::MouseDown(pt);
}

/* ---------------------------------------------------------------- */

void BPopUpMenu::MouseUp(BPoint pt)
{
	BMenu::MouseUp(pt);
}

/* ---------------------------------------------------------------- */

void BPopUpMenu::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	BMenu::MouseMoved(pt, code, msg);
}

/*---------------------------------------------------------------*/

void	BPopUpMenu::FrameMoved(BPoint new_position)
{
	BMenu::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BPopUpMenu::FrameResized(float new_width, float new_height)
{
	BMenu::FrameResized(new_width, new_height);
}

/*---------------------------------------------------------------*/

void BPopUpMenu::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BPopUpMenu::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::GetPreferredSize(width, height);
}

/*---------------------------------------------------------------*/

void BPopUpMenu::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void BPopUpMenu::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::AllAttached();
}

/*---------------------------------------------------------------*/

void BPopUpMenu::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::AllDetached();
}


void BPopUpMenu::_ReservedPopUpMenu1() {}
void BPopUpMenu::_ReservedPopUpMenu2() {}
void BPopUpMenu::_ReservedPopUpMenu3() {}

