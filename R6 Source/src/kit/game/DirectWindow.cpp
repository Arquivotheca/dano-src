//******************************************************************************
//
//	File:		DirectWindow.cpp
//
//	Description:	BDirectWindow class.
//			        Application Windows objects for direct screen access.
//
//	Written by:	Pierre Raynaud-Richard
//
//	Revision history
//
//	Copyright 1998, Be Incorporated, All Rights Reserved.
//
//******************************************************************************


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <Debug.h>

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif

#ifndef _DIRECT_WINDOW_H
#include <DirectWindow.h>
#endif

#include <IRegion.h>
#include <Screen.h>
#include <interface_misc.h>
#include <direct_window_priv.h>

#ifndef _DIRECT_DRIVER1_H
#include "DirectDriver1.h"
#endif

/*-------------------------------------------------------------*/

// Private secret perform code for BDirectWindow
enum {
	B_ADD_HOOK		= 0x5fec7893,
	B_REMOVE_HOOK	= 0x5fec7894,
	B_GET_DIRECT_INFO = 0x5fec7895
};

typedef void (direct_connected_hook)(direct_buffer_info *info, BDirectWindow *wnd);

struct hook_item {
	struct hook_item		*next;
	struct hook_item		*prev;
	direct_connected_hook	*hook;
};

struct priv_ext {
	struct hook_item		*first_hook;
};

/*-------------------------------------------------------------*/

BDirectWindow::BDirectWindow(	BRect		frame,
								const char	*title, 
								window_type	type,
								uint32		flags,
								uint32		workspace)
	:BWindow(frame, title, type, flags, workspace)
{
	InitData();
}

/*-------------------------------------------------------------*/

BDirectWindow::BDirectWindow(	BRect 		frame,
								const char 	*title, 
								window_look	look,
								window_feel feel,
								uint32		flags,
								uint32 		workspace)
	:BWindow(frame, title, look, feel, flags, workspace)
{
	InitData();
}

/*-------------------------------------------------------------*/

BDirectWindow::~BDirectWindow ()
{
	DisposeData();
}

/*-------------------------------------------------------------*/

BArchivable *BDirectWindow::Instantiate(BMessage *data)
{
	return NULL;
}

/*-------------------------------------------------------------*/

status_t BDirectWindow::Archive(BMessage *data, bool deep) const
{
	return BWindow::Archive(data, deep);
}

/*-------------------------------------------------------------*/

enum {
	B_APP_SERVER_LINK = 	0x0001,
	B_DIRECT_DEAMON =		0x0002,
	B_CLIPING_AREA = 		0x0004,
	B_DIRECT_BENAPHORE =	0x0008
};

void BDirectWindow::InitData()
{
	status_t		err;
	dw_init_info	init;

	// init private states
	direct_driver = NULL;
	direct_driver_type = 0;
	direct_driver_token = 0;
	direct_driver_ready = false;
	full_screen_enable = false;
	connection_enable = false;
	dw_init_status = 0;
	direct_lock_count = 0;
	direct_lock_owner = 0;
	direct_lock_stack = NULL;
	in_direct_connect = false;
	extension = NULL;
	
	// create the direct benaphore
	direct_lock = 0;		
	direct_sem = create_sem(0, "direct sem");
	if (direct_sem < 0)
		return;
	dw_init_status |= B_DIRECT_BENAPHORE;
	
	// create the special direct window connection
	if (!Lock())
		return;
		
	a_session->swrite_l(GR_INIT_DIRECTWINDOW);
	a_session->swrite_l(server_token);
	Flush();
	a_session->sread(sizeof(dw_init_info), &init);
	a_session->sread(4, &err);
	Unlock();
	
	if (err != B_NO_ERROR)
		return;

	source_clipping_area = init.clipping_area;
	disable_sem = init.disable_sem;
	disable_sem_ack = init.disable_sem_ack;
	
	dw_init_status |= B_APP_SERVER_LINK;	

	// map the cliping region area
	cloned_clipping_area = clone_area("Clone direct area",
									(void**)&info_area_size,
									B_ANY_ADDRESS,
									B_READ_AREA,
									source_clipping_area);
	if (cloned_clipping_area < 0)
		return;
	buffer_desc = (direct_buffer_info*)info_area_size;
	dw_init_status |= B_CLIPING_AREA;	

	// spawn the direct deamon thread
	direct_deamon_id = spawn_thread(BDirectWindow::DirectDeamonFunc,
									"direct deamon",
									B_DISPLAY_PRIORITY,
									(void*)this);
	if (direct_deamon_id < 0)
		return;
	// launch the direct deamon thread
	deamon_killer = false;
	connection_enable = false;
	if (resume_thread(direct_deamon_id) != B_NO_ERROR) {
		kill_thread(direct_deamon_id);
		return;
	}
	dw_init_status |= B_DIRECT_DEAMON;
	return;
}

/*-------------------------------------------------------------*/

void BDirectWindow::DisposeData()
{
	long		retval;
	uint32		result;

	// wait to lock with connection_enable == false
	while (true) {
		if (LockDirect()) {
			if (!connection_enable)
				break;
			UnlockDirect();
		}
		snooze(50000);
	}
	
	if (direct_driver)
		delete direct_driver;
	
	if (dw_init_status & B_DIRECT_DEAMON) {
		deamon_killer = true;
		release_sem(disable_sem);
		wait_for_thread(direct_deamon_id, &retval);
	}
	
	if (dw_init_status & B_CLIPING_AREA)
		delete_area(cloned_clipping_area);
		
	if (dw_init_status & B_DIRECT_BENAPHORE)
		delete_sem(direct_sem);
		
	// free the private hook list and struct if any
	if (extension != NULL) {
		hook_item	*it, *next;
		
		it = extension->first_hook;
		while (it) {
			next = it->next;
			delete it;
			it = next;
		}
		delete extension;
	}
}

/*-------------------------------------------------------------*/

int32 BDirectWindow::DirectDeamonFunc(void *arg)
{
	hook_item		*it;
	BDirectWindow	*dw;
	
	dw = (BDirectWindow*)arg;
	while (true) {
		// time to quit ?
		if (dw->deamon_killer)
			break;
		// wait for the next direct access transaction
		while (acquire_sem(dw->disable_sem) == B_INTERRUPTED)
			;
		if (dw->deamon_killer)
			break;
			
		while (!dw->LockDirect())
			if (dw->deamon_killer)
				break;
		if ((dw->buffer_desc->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_START) {
			dw->direct_driver_ready = false;
			dw->connection_enable = true;
		}

		dw->in_direct_connect = true;
		/* call the hook functions first, if any */
		if (dw->extension != NULL) {
			it = dw->extension->first_hook;
			while (it) {
				(*it->hook)(dw->buffer_desc, dw);
				it = it->next;
			}
		}
		dw->DirectConnected(dw->buffer_desc);
		dw->in_direct_connect = false;
		
		if ((dw->buffer_desc->buffer_state & B_DIRECT_MODE_MASK) == B_DIRECT_STOP)
			dw->connection_enable = false;
		dw->UnlockDirect();
	
		// acknowledge the readiness to the server
		release_sem(dw->disable_sem_ack);
	}
	return 0;
}

/*-------------------------------------------------------------*/

void BDirectWindow::DirectConnected(direct_buffer_info *info)
{
}

/*-------------------------------------------------------------*/

status_t BDirectWindow::GetClippingRegion(BRegion *region, BPoint *point) const
{
	int				i;
	int32			dh, dv;
	BDirectWindow	*t = const_cast<BDirectWindow *>(this);

	// refused to be called by a thread that locking the window
	if (t->IsLocked())
		return B_ERROR;
		
	if (!LockDirect())
		return B_ERROR;
		
	if (!in_direct_connect) {
		UnlockDirect();
		return B_ERROR;
	}
		
	// set the offset
	if (point == NULL) {
		dh = 0;
		dv = 0;
	}
	else {
		dh = (int32)point->x;
		dv = (int32)point->y;
	}
		
	// set the region
	const int32 N = buffer_desc->clip_list_count;
	IRegion* reg = IKAccess::RegionData(region);
	clipping_rect* rects = reg->CreateRects(N, N);
	reg->SetRectCount(N);
	reg->Bounds().left = (buffer_desc->clip_bounds.left - dh);
	reg->Bounds().top = (buffer_desc->clip_bounds.top - dv);
	reg->Bounds().right = (buffer_desc->clip_bounds.right - dh);
	reg->Bounds().bottom = (buffer_desc->clip_bounds.bottom - dv);
	for (i=0; i<N; i++) {
		rects[i].left = (buffer_desc->clip_list[i].left - dh);
		rects[i].top = (buffer_desc->clip_list[i].top - dv);
		rects[i].right = (buffer_desc->clip_list[i].right - dh);
		rects[i].bottom = (buffer_desc->clip_list[i].bottom - dv);
	}
	UnlockDirect();
	return B_NO_ERROR;
}
	
/*-------------------------------------------------------------*/

status_t BDirectWindow::SetFullScreen(bool enable)
{
	uint32			status;
	status_t		err;

	if (Lock()) {
		full_screen_enable = enable;
		a_session->swrite_l(GR_SET_FULLSCREEN);
		a_session->swrite_l(server_token);
		a_session->swrite_l(enable);
		Flush();
		a_session->sread(4, &status);
		a_session->sread(4, &err);
		full_screen_enable = status;
		Unlock();
		return err;
	}
	return B_ERROR;
}

/*-------------------------------------------------------------*/

bool BDirectWindow::IsFullScreen() const
{
	return full_screen_enable;
}

/*-------------------------------------------------------------*/

bool BDirectWindow::SupportsWindowMode(screen_id id)
{
	_BAppServerLink_	link;
	direct_window_info	dw_info;

	link.session->swrite_l(GR_DIRECT_WINDOW_INFO);
	link.session->swrite_l((uint32)id.id);
	link.session->flush();
	link.session->sread(sizeof(direct_window_info), &dw_info);
	return ((dw_info.flags & B_SUPPORTS_WINDOW_MODE) != 0);
}

/*-------------------------------------------------------------*/
/*
status_t BDirectWindow::GetDriverHook(driver_hook_token token, graphics_card_hook *hook) const
{
	status_t		result;
	BDirectWindow	*t = const_cast<BDirectWindow *>(this);

	// refused to be called by a thread that locking the window
	if (t->IsLocked())
		return B_ERROR;
	
	// check a violation of full screen priviledge
	if (!LockDirect())
		return B_ERROR;
		
	if (!full_screen_enable) {
		UnlockDirect();
		return B_ERROR;
	}
	
	// check availability of the direct driver
	if (DriverSetup() == B_NO_ERROR) {
		UnlockDirect();
		return B_ERROR;
	}
		
	// synchronise the state of the driver if needed
	if (!direct_driver_ready) {
		if (direct_driver->Sync(full_screen_enable) != B_NO_ERROR) {
			UnlockDirect();
			return B_ERROR;
		}
		t->direct_driver_ready = true;
	}
	
	// call the direct driver (finally...)
	result = direct_driver->GetHook(token, hook);
	UnlockDirect();
	return result;
}
*/
/*-------------------------------------------------------------*/
/*
status_t BDirectWindow::GetDriverCookie(void **cookie) const
{
	status_t		result;
	BDirectWindow	*t = const_cast<BDirectWindow *>(this);

	// refused to be called by a thread that locking the window
	if (t->IsLocked())
		return B_ERROR;
		
	// check availability of the direct driver
	if (!LockDirect())
		return B_ERROR;
		
	if (DriverSetup() != B_NO_ERROR) {
		UnlockDirect();
		return B_ERROR;
	}
		
	// call the direct driver (finally...)
	result = direct_driver->GetCookie(cookie);
	UnlockDirect();
	return result;
}
*/
/*-------------------------------------------------------------*/

bool BDirectWindow::LockDirect() const
{
	char			*stack;
	status_t		err;
	thread_id		owner = 0;
	BDirectWindow	*t = const_cast<BDirectWindow *>(this);

	return true;
	if (t->direct_lock_count > 0) {
		stack = ((char*)&stack) - (((uint32)&stack)&(B_PAGE_SIZE-1));
		if (stack == t->direct_lock_stack) {
			t->direct_lock_count++;
			return true;
		}
		owner = find_thread(NULL);
		if (owner == t->direct_lock_owner) {
			t->direct_lock_stack = stack;
			t->direct_lock_count++;
			return true;
		}
	}

	if (atomic_add(&t->direct_lock, 1) > 0)
		do {
			do err = acquire_sem_etc(t->direct_sem, 1, B_TIMEOUT, 50000);
			while (err == B_INTERRUPTED);
			if (err == B_TIMED_OUT) {
				if (atomic_add(&t->direct_lock, -1) == 0)
					while (acquire_sem(t->direct_sem) == B_INTERRUPTED)
						;
				return false;
			}
		}
		while (err != B_NO_ERROR);

	t->direct_lock_count++;
	if (owner == 0)
		owner = find_thread(NULL);
	t->direct_lock_owner = owner;
	stack = ((char*)&stack) - (((uint32)&stack)&(B_PAGE_SIZE-1));
	t->direct_lock_stack = stack;
	return true;
}

/*-------------------------------------------------------------*/

void BDirectWindow::UnlockDirect() const
{
	BDirectWindow	*t = const_cast<BDirectWindow *>(this);

	return;
	t->direct_lock_count--;
	if (t->direct_lock_count > 0)
		return;
		
	t->direct_lock_owner = 0;
	t->direct_lock_stack = NULL;

	if (atomic_add(&t->direct_lock, -1) > 1)
		release_sem(t->direct_sem);
}

/*-------------------------------------------------------------*/

status_t BDirectWindow::DriverSetup() const
{
	BDirectWindow	*t = const_cast<BDirectWindow *>(this);
	
	// check that the connection is enable
	if (!connection_enable)
		return B_ERROR;
	
	// release any improper version of the direct driver
	if (direct_driver != NULL)
		if ((buffer_desc->_dd_type_ != direct_driver_type) ||
			(buffer_desc->_dd_token_ != direct_driver_token)) {
			delete direct_driver;
			t->direct_driver = NULL;
		}
		
	// load a proper version of the direct driver if needed
	if (direct_driver == NULL) {
		t->direct_driver_type = buffer_desc->_dd_type_;
		t->direct_driver_token = buffer_desc->_dd_token_;
		
		switch(direct_driver_type) {
		case 1 :
			t->direct_driver = new BDirectDriver1(direct_driver_token);
			break;
		default:
			return B_ERROR;
		}
	}
	
	// check-in completed
	return B_NO_ERROR;
}

/*-------------------------------------------------------------*/
/* Virtual glue reserved for potential extension 			   */
/*-------------------------------------------------------------*/

void BDirectWindow::Quit() {
	BWindow::Quit();
}

void BDirectWindow::DispatchMessage(BMessage *message, BHandler *handler) {
	BWindow::DispatchMessage(message, handler);
}

void BDirectWindow::MessageReceived(BMessage *message) {
	BWindow::MessageReceived(message);
}

void BDirectWindow::FrameMoved(BPoint new_position) {
	BWindow::FrameMoved(new_position);
}

void BDirectWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws) {
	BWindow::WorkspacesChanged(old_ws, new_ws);
}

void BDirectWindow::WorkspaceActivated(int32 ws, bool state) {
	BWindow::WorkspaceActivated(ws, state);
}

void BDirectWindow::FrameResized(float new_width, float new_height) {
	BWindow::FrameResized(new_width, new_height);
}

void BDirectWindow::Minimize(bool minimize) {
	BWindow::Minimize(minimize);
}

void BDirectWindow::Zoom(	BPoint rec_position,
							float rec_width,
							float rec_height) {
	BWindow::Zoom(rec_position, rec_width, rec_height);
}

void BDirectWindow::ScreenChanged(BRect screen_size, color_space depth) {
	BWindow::ScreenChanged(screen_size, depth);
}

void BDirectWindow::MenusBeginning() {
	BWindow::MenusBeginning();
}

void BDirectWindow::MenusEnded() {
	BWindow::MenusEnded();
}

void BDirectWindow::WindowActivated(bool state) {
	BWindow::WindowActivated(state);
}

void BDirectWindow::Show() {
	BWindow::Show();
}

void BDirectWindow::Hide() {
	BWindow::Hide();
}

BHandler *BDirectWindow::ResolveSpecifier(	BMessage *msg,
									int32 index,
									BMessage *specifier,
									int32 form,
									const char *property) {
	return BWindow::ResolveSpecifier(msg, index, specifier, form, property);
}

status_t BDirectWindow::GetSupportedSuites(BMessage *data) {
	return BWindow::GetSupportedSuites(data);
}

void BDirectWindow::task_looper() {
	BWindow::task_looper();
}

BMessage *BDirectWindow::ConvertToMessage(void *raw, int32 code) {
	return BWindow::ConvertToMessage(raw, code);
}

/*-------------------------------------------------------------*/

BDirectWindow::BDirectWindow() {}
BDirectWindow::BDirectWindow(BDirectWindow &) {}
BDirectWindow &BDirectWindow::operator=(BDirectWindow &) { return *this; }

/*-------------------------------------------------------------*/

status_t BDirectWindow::Perform(perform_code d, void *arg)
{
	hook_item	*it, *next, *prev;

	switch (d) {
	case B_ADD_HOOK:
		// create the extension struct if needed
		LockDirect();
		if (extension == NULL) {
			extension = new priv_ext;
			extension->first_hook = NULL;
		}
		it = new hook_item;
		it->hook = (direct_connected_hook*)arg;
		it->prev = NULL;
		it->next = extension->first_hook;
		extension->first_hook = it;
		UnlockDirect();
		return B_NO_ERROR;
		
	case B_REMOVE_HOOK:
		LockDirect();
		if (extension == NULL) {
			UnlockDirect();
			return B_ERROR;
		}
		it = extension->first_hook;
		while (it) {
			if (it->hook == (direct_connected_hook*)arg) {
				next = it->next;
				prev = it->prev;
				if (prev)
					prev->next = next;
				else
					extension->first_hook = next;
				if (next)
					next->prev = prev;
				delete it;
				UnlockDirect();
				return B_NO_ERROR;
			}
			it = it->next;
		}
		UnlockDirect();
		return B_ERROR;
		
	case B_GET_DIRECT_INFO:
		{
			direct_buffer_info **i = (direct_buffer_info **)arg;
			*i = buffer_desc;
			return B_OK;
		}
	
	default:
		return inherited::Perform(d, arg);
	}
}

/*-------------------------------------------------------------*/

void BDirectWindow::_ReservedDirectWindow1() {}
void BDirectWindow::_ReservedDirectWindow2() {}
void BDirectWindow::_ReservedDirectWindow3() {}
void BDirectWindow::_ReservedDirectWindow4() {}
