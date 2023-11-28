//******************************************************************************
//
//	File:		WindowScreen.cpp
//
//	Description:	BWindowScreen class.
//			        Application Windows objects for direct screen access.
//
//	Written by:	Pierre Raynaud-Richard
//
//	Revision history
//
//	Copyright 1996, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef _APP_DEFS_PRIVATE_H
#include <AppDefsPrivate.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _MESSAGES_H
#include <messages.h>
#endif
#ifndef _TOKEN_H
#include <token.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _INTERFACE_DEFS_H
#include <InterfaceDefs.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _SESSION_H
#include <session.h>
#endif

#ifndef _WINDOW_SCREEN_H
#include <WindowScreen.h>
#endif
#include <Screen.h>
#include <input_server_private.h>

#if 0
#define	dprintf(a) dprintf_real a
#define dump_display_mode(a) dump_display_mode_real a
#include <stdarg.h>
#include <stdio.h>

#if 1
extern "C" void _kdprintf_(const char *, ...);
static void dprintf_real(char *fmt, ...) {
	va_list ap;
	char buffer[1024];
	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);
	_kdprintf_(buffer);
}
#else
static void dprintf_real(char *fmt, ...) {
	va_list ap;
	char buffer[1024];
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fflush(stderr);
	va_end(ap);
}
#endif
void dump_display_mode_real(display_mode *dm) {
	dprintf(("  timing: %u  %d %d %d %d  ",
		dm->timing.pixel_clock,
		dm->timing.h_display,
		dm->timing.h_sync_start,
		dm->timing.h_sync_end,
		dm->timing.h_total));
	dprintf(("%d %d %d %d  0x%08x\n",
		dm->timing.v_display,
		dm->timing.v_sync_start,
		dm->timing.v_sync_end,
		dm->timing.v_total,
		dm->timing.flags));
	dprintf(("  colorspace: 0x%08x\n", dm->space));
	dprintf(("  virtual: %d %d\n", dm->virtual_width, dm->virtual_height));
	dprintf(("  offset: %d %d\n", dm->h_display_start, dm->v_display_start));
}
#else
#define	dprintf(a)
#define dump_display_mode(a)
#endif

typedef struct {
	uint32			count;
	display_mode	*list;
} mode_list_info;

typedef struct {
	int32			x,y;
	uint32			pixelFormat;
	float			rate;
} vague_display_mode;

static status_t mode2parms(uint32 mode, uint32 *cs, int32 *width, int32 *height);

static fill_rectangle			fill_rect_global = 0;
static screen_to_screen_blit	blit_rect_global = 0;
static screen_to_screen_transparent_blit	trans_blit_rect_global = 0;
static screen_to_screen_scaled_filtered_blit	scaled_filtered_blit_rect_global = 0;
static wait_engine_idle			wait_idle_global = 0;
static engine_token				*et_global = 0;
static acquire_engine			acquire_engine_global = 0;
static release_engine			release_engine_global = 0;
static move_display_area		move_display_global = 0;
static graphics_card_info		card_info_global;

/*-------------------------------------------------------------*/
static status_t _get_frame_buffer_config_(uint32 screen, frame_buffer_config *fbc) {
	status_t			result;
	_BAppServerLink_	link;
	
	dprintf(("_get_frame_buffer_config_(%d) begins...\n", screen));
	link.session->swrite_l(GR_GET_FRAME_BUFFER_CONFIG);
	link.session->swrite_l(screen);
	link.session->flush();
	link.session->sread(4, &result);
	if (result == B_OK) {
		link.session->sread(sizeof(frame_buffer_config), fbc);
		dprintf(("  fbc.fb: 0x%08x\n  fbc.fb_dma: 0x%08x\n  fbc.row_bytes: %d (0x%08x)\n",
				 fbc->frame_buffer, fbc->frame_buffer_dma, fbc->bytes_per_row, fbc->bytes_per_row));
	}
	dprintf(("_get_frame_buffer_config_() returns %d (0x%08x)\n ->%s\n",
			 result, result, strerror(result)));
	return result;
}

static long _lock_direct_screen_(long index, long state, long token) {
	long				retval;
	_BAppServerLink_	link;

	dprintf(("_lock_direct_screen_(index:%d, state:%d, token:%d) begins...\n", index, state, token));
	link.session->swrite_l(GR_LOCK_DIRECT_SCREEN);
	link.session->swrite_l(index);
	link.session->swrite_l(state);
	link.session->swrite_l(token);
	link.session->flush();
	link.session->sread(4, &retval);
	dprintf(("_lock_direct_screen_() returns %d (0x%08x)\n  ->%s\n", retval, retval, strerror(retval)));
	return retval;
}

static status_t _propose_display_mode_(long index,
									   display_mode *target,
									   display_mode *low,
									   display_mode *high) {
	long				error;
	_BAppServerLink_	link;

	dprintf(("_propose_display_mode_() begins...\n"));
	link.session->swrite_l(GR_CTRL_GRAPHICS_CARD);
	link.session->swrite_l(index);
	link.session->swrite_l(B_PROPOSE_DISPLAY_MODE);
	link.session->swrite(sizeof(display_mode), target);
	link.session->swrite(sizeof(display_mode), low);
	link.session->swrite(sizeof(display_mode), high);
	link.session->flush();
	link.session->sread(4, &error);
	link.session->sread(sizeof(display_mode), target);
	dprintf(("_propose_display_mode_() returns %d (0x%08x)\n ->%s\n", error, error, strerror(error)));
	dump_display_mode((target));
	return error;
}

static long _move_display_area_(long index, uint16 x, uint16 y) {
	long				error;
	_BAppServerLink_	link;

	dprintf(("_move_display_area_() begins...\n"));
	link.session->swrite_l(GR_CTRL_GRAPHICS_CARD);
	link.session->swrite_l(index);
	link.session->swrite_l(B_MOVE_DISPLAY);
	link.session->swrite(sizeof(uint16), &x);
	link.session->swrite(sizeof(uint16), &y);
	link.session->flush();
	link.session->sread(4, &error);
	dprintf(("_move_display_area_() returns %d (0x%08x)\n ->%s\n", error, error, strerror(error)));
	return error;
}

static void _set_color_map_(long index, long from, long to, rgb_color *list) {
	long				error;
	_BAppServerLink_	link;

	dprintf(("_set_color_map_()\n"));
	link.session->swrite_l(GR_SET_COLOR_MAP);
	link.session->swrite_l(index);
	link.session->swrite_l(from);
	link.session->swrite_l(to);
	link.session->swrite((to+1-from)*sizeof(rgb_color), list);
	link.session->flush();
}

static long _set_standard_space_(long index, display_mode *space) {
	_BAppServerLink_ link;
	long             error;

	dprintf(("_set_standard_space_() begins...\n"));
	link.session->swrite_l(GR_SET_STANDARD_SPACE);
	link.session->swrite_l(index);
	link.session->swrite(sizeof(display_mode), space);
	link.session->flush();

	link.session->sread(4, &error);
	dprintf(("_set_standard_space_() returns %d (0x%08x)\n  ->%s\n", error, error, strerror(error)));
	return error;
}

static void _activate_workspace_(int index) {
	_BAppServerLink_ link;

	dprintf(("_activate_workspace_(%d)\n", index));
	link.session->swrite_l(GR_SELECT_WS_ASYNC);
	link.session->swrite_l(index);
	link.session->flush();
}

void set_mouse_position(int32 x, int32 y) {
	BMessage reply;
	BMessage command(IS_SET_MOUSE_POSITION);
	command.AddPoint(IS_WHERE, BPoint(x, y));

	_control_input_server_(&command, &reply);
}

/*-------------------------------------------------------------*/

BRect BWindowScreen::CalcFrame(int32 index, int32 space, display_mode *dmode)
{
	int         s_width, s_height;

	// Chicken-and-egg: can't use BScreen's BWindow constructor,
	// as we use CalcFrame to figure the rect to pass to BWindow's
	// constructor.  Must use screen_ids here.
	BScreen		screen( B_MAIN_SCREEN_ID );

	// The previous code ignored index and space, so this code does too.
	// It's just a fancy way to get the frame rectange and possibly the
	//  current display mode.

	// get the current mode if given a place to put it
	if (dmode) screen.GetMode(dmode);

	// set specific space
	return screen.Frame();
}

#define	DISABLE_VIEW_DRAWING 0x10000

BWindowScreen::BWindowScreen (const char *title, uint32 space, status_t *error, bool debug_enable)
	:BWindow(CalcFrame(0, space, 0L), title, B_TITLED_WINDOW,
			B_NOT_MOVABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE |
			B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | DISABLE_VIEW_DRAWING)
{
	*error = InitData(space, (debug_enable?B_ENABLE_DEBUGGER:0));
}

BWindowScreen::BWindowScreen (const char *title, uint32 space, uint32 attributes, status_t *error)
	:BWindow(CalcFrame(0, space, 0L), title, B_TITLED_WINDOW,
			B_NOT_MOVABLE | B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE |
			((attributes&B_ENABLE_VIEW_DRAWING)?0:DISABLE_VIEW_DRAWING))
{
	*error = InitData(space, attributes);
}

status_t BWindowScreen::InitData(uint32 space, uint32 attributes) {
	uint32      work1, work2;
	BScreen		screen(this);
	status_t	err = B_ERROR;

	dprintf(("BWindowScreen::InitData(space: %d, attr: %d) begins...\n", space, attributes));
// Init the debugging system
	debug_state = ((attributes & B_ENABLE_DEBUGGER) != 0);
	debug_list_count = 0;
	debug_list = 0L;
	debug_first = true;
	debug_sem = create_sem(1, "WindowScreen debug sem");
	
// Place to put old and new display_mode
	old_space = (display_mode *)calloc(1, sizeof(display_mode));
	if (!old_space) {
		dprintf(("  calloc() failed for old_space\n"));
		goto error_exit;
	}
	new_space = (display_mode *)calloc(1, sizeof(display_mode));
	if (!new_space) {
		dprintf(("  calloc() failed for new_space\n"));
		goto free_old_space;
	}

// Get ourselves a copy of the mode list
	err = screen.GetModeList(&mode_list, &mode_count);
	if (err != B_OK) {
		dprintf(("  GetModeList() failed %d (0x%08x)\n   ->%s\n", err, err, strerror(err)));
		goto free_new_space;
	}

	_attributes = attributes;
	
// Set default values
	screen_index = 0;
	lock_state = false;
	addon_state = 0;
	direct_enable = false;
	window_state = false;
	space_mode = 1;

// Convert the old space convention to the new graphic mode description, if possible
	err = GetModeFromSpace(space, new_space);
	if (err != B_OK) {
		dprintf(("  GetModeFromSpace() failed %d (0x%08x)\n   ->%s\n", err, err, strerror(err)));
		goto free_mode_list;
	}

// Calculate the index of the WindowScreen window 	
	work1 = Workspaces();
	space0 = 0;
	while ((work1 & 1) == 0) {
		work1 >>= 1;
		space0++;
	}
// Calculate the workspace masks of the WindowScreen window...
	work1 = Workspaces();
	SetWorkspaces(B_CURRENT_WORKSPACE);
// ... and the one of the current workspace
	work2 = Workspaces();
// If they're identical and we want to debug, then we need to change
// the target workspace of the WindowScreen (and update the ws index).
	if ((work1 == work2) && debug_state) {
		if (work1 == 1) {
			work1<<=1;
			space0++;
		}
		else {
			work1>>=1;
			space0--;
		}
	}

// Make the window a fullscreen window (but it's not visible yet)
	err = SetFullscreen(true);
	if (err != B_OK) {
		dprintf(("  SetFullScreen(true) failed %d (0x%08x)\n   ->%s\n", err, err, strerror(err)));
		lock_state = -1;
		goto free_mode_list;
	}

// If the target ws is the current one, then the workspace-connection is already made...
	if (work1 == work2)
		work_state = true;
// In the other case, we set the proper ws visibility and schedule an asynchronous ws switch
// (the workspace connection still needs to be made).
	else {
		work_state = false;
		SetWorkspaces(work1);
		activate_workspace(space0);
	}

// Calculate the index of the debug workspace.
	debug_workspace = 0;
	while ((work2 & 1) == 0) {
		work2 >>= 1;
		debug_workspace++;
	}

// Initialize default colormap for 8 bits (the screen colormap)
	memcpy(colorList, screen.ColorMap()->color_list, sizeof(rgb_color)*256);

// Initialize default card_info 
	GetCardInfo();
	format_info.bits_per_pixel = card_info.bits_per_pixel;
	format_info.bytes_per_row = card_info.bytes_per_row;
	format_info.width = card_info.width;
	format_info.height = card_info.height;
	format_info.display_width = card_info.width;
	format_info.display_height = card_info.height;
	format_info.display_x = 0;
	format_info.display_y = 0;
	
// Create activate semaphore
	activate_sem = create_sem(0, "WindowScreen start lock");
	activate_state = false;
	goto error_exit;

free_mode_list:
	free(mode_list);
	mode_list = 0;
	mode_count = 0;
free_new_space:
	free(new_space);
	new_space = 0;
free_old_space:
	free(old_space);
	old_space = 0;
error_exit:
	dprintf(("BWindowScreen::InitData() returns %d (0x%08x)\n ->%s\n", err, err, strerror(err)));
	return err;
}

BWindowScreen::~BWindowScreen ()
{
	dprintf(("BWindowScreen::~BWindowScreen()\n"));
// End the WindowScreen exclusive access.
	Disconnect();
// Close direct add-on
	if (addon_state == 1) {
		// uninit the accelerant (Disable for now. Trey, why did you do that ?) 
	//	uninit_accelerant ua = (uninit_accelerant)(m_gah)(B_UNINIT_ACCELERANT, NULL);
	//	if (ua) (ua)();
		unload_add_on(addon_image);
	}
	SetFullscreen(false);
	delete_sem(activate_sem);
	delete_sem(debug_sem);
	if (debug_state)
		_activate_workspace_(debug_workspace);
// Free our cached display_mode(s)
	if (new_space) free(new_space);
	if (old_space) free(old_space);
	if (mode_list) free(mode_list);
}

int32 BWindowScreen::SetFullscreen(int32 enable) {
	uint32			status;
	status_t		err;

	dprintf(("BWindowScreen::SetFullscreen(%d) begins...\n",enable));
	a_session->swrite_l(GR_SET_FULLSCREEN);
	a_session->swrite_l(server_token);
	a_session->swrite_l(enable);
	a_session->flush();
	a_session->sread(4, &status);
	a_session->sread(4, &err);
	dprintf(("BWindowScreen::SetFullscreen() returns %d (0x%08x)\n  ->%s\n", err, err, strerror(err)));
	return err;
}

void BWindowScreen::Quit() {
	dprintf(("BWindowScreen::Quit()\n"));
	Disconnect();
	BWindow::Quit();
}

void BWindowScreen::Disconnect() {
	dprintf(("BWindowScreen::Disconnect()\n"));
	if (lock_state == true) {
		if (debug_state)
			debug_first = true;
		SetActiveState(false);
	}
}

void BWindowScreen::WindowActivated(bool active)
{
	dprintf(("BWindowScreen::WindowActivated(%s)\n", active?"true":"false"));
	window_state = active;
	if (active && !lock_state && work_state)
		SetActiveState(true);
}

void BWindowScreen::WorkspaceActivated(int32 workspace, bool active)
{
	dprintf(("BWindowScreen::WorkspaceActivated(ws: %d, active: %s)\n", workspace, active?"true":"false"));
	work_state = active;
	if (active) {
		if (!lock_state) {
			if (window_state)
				SetActiveState(true);
			else {
				if (!IsHidden()) {
					Activate();
					WindowActivated(true);
				}
			}
		}
	}
	else if (lock_state)
		SetActiveState(false);
}

void BWindowScreen::ScreenConnected(bool active)
{
	dprintf(("BWindowScreen::ScreenConnected(%s)\n", active?"true":"false"));
}

void BWindowScreen::Hide()
{
	dprintf(("BWindowScreen::Hide()\n"));
	Disconnect();
	BWindow::Hide();
}

void BWindowScreen::Show()
{
	dprintf(("BWindowScreen::Show()\n"));
	BWindow::Show();
	if (!activate_state) {
		release_sem(activate_sem);
		activate_state = true;
	}
}

status_t BWindowScreen::SetActiveState(int32 state)
{
	int         err;

	dprintf(("BWindowScreen::SetActiveState(%s) begins...\n", state==true?"true":"false"));
	if (state) {
// Hide the cursor (and makes sure it's really hidden)
		be_app->HideCursor();
		be_app->IsCursorHidden();
// Try to acquire exckusive ownership of the screen
		err = SetLockState(true);
		if (err == B_NO_ERROR) {
	// If it succeeds, try to set the required graphic mode
			err = AssertDisplayMode(new_space);
			if (err == B_NO_ERROR) {
		// wait for the Show if he has not been done yet.
				if (!activate_state)
					while (acquire_sem(activate_sem) == B_INTERRUPTED) {;}
		// Reset the colormap.
				SetColorList(colorList);
		// Restart after a suspension in debug mode
				if (debug_state && !debug_first) {
					SuspensionHook(true);
					Resume();
				}
		// Or just call ScreenConnected
				else {				
					debug_first = false;
					ScreenConnected(true);
				}
			}
	// The graphic mode setting failed, so we unlock the screen.
			else
				SetLockState(false);
		}
// If there was an error, then the screen is not locked and we need to show the
// cursor again.
		if (err != B_NO_ERROR)
			be_app->ShowCursor();
	}
	else {	
// If in debug mode, we do a suspension
		if (debug_state && !debug_first) {
			Suspend();
			SuspensionHook(false);
		}
// In standard case, we just call ScreenConnected and let the app suspend itself
		else
			ScreenConnected(false);
// Unlock exclusive access to the screen, and if it succeeds, show the cursor back
// and reset the system colormap.
		err = SetLockState(false);
		if (err == B_NO_ERROR) {
			be_app->ShowCursor();
			if (activate_state)
				_set_color_map_(screen_index, 0, 255, (rgb_color*)system_colors()->color_list); 
		}
	}
	dprintf(("BWindowScreen::SetActiveState(%s) ends with error:%d.\n",
			 state==true?"true":"false", err));
	return err;
}

status_t BWindowScreen::SetLockState(int32 state)
{
	int                  i;
	long                 err;
	area_info            ainfo;

	dprintf(("BWindowScreen::SetLockState(%d) begins...\n", state));

	if (addon_state == 1) {
		if (state == false) {
			(m_wei)();
			acquire_engine_global = 0;
			release_engine_global = 0;
			wait_idle_global = 0;
			move_display_global = 0;
			fill_rect_global = 0;
			blit_rect_global = 0;
			trans_blit_rect_global = 0;
			scaled_filtered_blit_rect_global = 0;
			et_global = 0;
		}
	}

// send the lock/unlock command
	err = _lock_direct_screen_(screen_index,state,server_token);
// if the command is succesful
	if (err == B_NO_ERROR) {
		lock_state = state;
#if 0	// doesn't seem to be used any place
		// enable direct memory access
		if (!direct_enable)
			direct_enable = true;
#endif
		// only handle acceleration stuff if we're taking control of the screen
		if (lock_state == true) {
		// enable direct add-on access if not already done...
			if (addon_state == 0) {
				if (InitClone() == B_OK) {
					addon_state = 1;
					m_wei = (wait_engine_idle)(m_gah)(B_WAIT_ENGINE_IDLE, NULL);
					m_re = (release_engine)(m_gah)(B_RELEASE_ENGINE, NULL);
					m_ae = (acquire_engine)(m_gah)(B_ACQUIRE_ENGINE, NULL);
					m_md = (move_display_area)(m_gah)(B_MOVE_DISPLAY, NULL);
				} else addon_state = -1;
			}
			if (addon_state == 1) {
				if (state == true) {
					acquire_engine_global = m_ae;
					release_engine_global = m_re;
					wait_idle_global = m_wei;
					move_display_global = m_md;
					fill_rect_global = fill_rect;
					blit_rect_global = blit_rect;
					trans_blit_rect_global = trans_blit_rect;
					scaled_filtered_blit_rect_global = scaled_filtered_blit_rect;
					et_global = et;
					(m_wei)();
				}
			}
		}
	}
	dprintf(("BWindowScreen::SetLockState(%d) returns %d (0x%08x)\n ->%s\n", state, err, err, strerror(err)));
	return err;
}

uint16 bitsPerPixel(uint32 a_color_space) {
	switch (a_color_space & 0x0fff) {
		case B_CMAP8: return 8;
		case B_RGB15_LITTLE:
		case B_RGB15_BIG: return 15;
		case B_RGB16_LITTLE:
		case B_RGB16_BIG: return 16;
		case B_RGB32_LITTLE:
		case B_RGB32_BIG: return 32;
	}
	return 0;
}

void BWindowScreen::GetCardInfo()
{
	BScreen scr(this);
	display_mode dmode;
	frame_buffer_config fbc;
	
	dprintf(("BWindowScreen::GetCardInfo() begins...\n"));
	card_info.version = 2;
	card_info.id = 0;
	scr.GetMode(&dmode);
	dump_display_mode((&dmode));
	card_info.bits_per_pixel = bitsPerPixel(dmode.space);
	card_info.width = dmode.virtual_width;	// should these be virtual or displayed?
	card_info.height = dmode.virtual_height;
	if (dmode.space & 0x1000)
		strncpy(card_info.rgba_order, "argb", 4);
	else
		strncpy(card_info.rgba_order, "bgra", 4);
	card_info.flags = 0;
	if (dmode.flags & B_SCROLL) card_info.flags |= B_FRAME_BUFFER_CONTROL;
	if (dmode.flags & B_PARALLEL_ACCESS) card_info.flags |= B_PARALLEL_BUFFER_ACCESS;
	// what about B_CRT_CONTROL?
	_get_frame_buffer_config_(scr.ID().id, &fbc);
	card_info.frame_buffer = fbc.frame_buffer;
	card_info.bytes_per_row = fbc.bytes_per_row;
	card_info_global = card_info;
	dprintf(("BWindowScreen::GetCardInfo() ends.\n"));

}

void BWindowScreen::SetColorList(rgb_color *list, int32 first_index, int32 last_index)
{
	dprintf(("BWindowScreen::SetColorList()\n"));
	int             i;
	indexed_color   col;       
	
	if ((first_index < 0) || (last_index > 255) || (first_index > last_index)) return;
	Lock();
	if (!activate_state)
		for (i=first_index;i<=last_index;i++)
			colorList[i] = list[i-first_index];
// use direct addon if available
	else if (addon_state == 1) {
		uint8 colors[256*3];
		int j = 0;
		dprintf(("  using the add-on\n"));
		for (i=first_index;i<=last_index;i++) {
			colorList[i] = list[i-first_index];
			colors[j++] = colorList[i].red;
			colors[j++] = colorList[i].green;
			colors[j++] = colorList[i].blue;
		}
		set_indexed_colors sic = (set_indexed_colors)(m_gah)(B_SET_INDEXED_COLORS, NULL);
		dprintf(("  sic: 0x%08x\n", sic));
		if (sic) (sic)((last_index - first_index) + 1,first_index,colors,0);
		BScreen().WaitForRetrace();
	}
// else, use the app_server addon
	else {
		dprintf(("  using the app_server\n"));
		for (i=first_index;i<=last_index;i++)
			colorList[i] = list[i-first_index];
		_set_color_map_(screen_index, first_index, last_index, colorList+first_index); 
		BScreen().WaitForRetrace();
	}
	Unlock();
}

void BWindowScreen::ScreenChanged(BRect screen_size, color_space depth)
{
	dprintf(("BWindowScreen::ScreenChanged()\n"));
}

status_t 
BWindowScreen::InitClone()
{
	_BAppServerLink_ link;
	status_t result = B_ERROR;
	int32 size;
	void *clone_info;

	dprintf(("BWindowScreen::InitClone() begins...\n"));
	// note failure by default
	addon_image = -1;

	// get path to accelerant from app_server
	link.session->swrite_l(GR_GET_ACCELERANT_IMAGE_PATH);
	link.session->swrite_l(screen_index);
	link.session->flush();
	link.session->sread(sizeof(result), &result);
	if (result != B_OK) goto bail_out;
	link.session->sread(sizeof(size), &size);
	link.session->sread(size, info.addon_path);
	info.addon_path[size] = 0;
	dprintf(("info.addon_path ->%s<-\n", info.addon_path));
	
	// get the clone info from the app_server
	link.session->swrite_l(GR_GET_ACCELERANT_CLONE_INFO);
	link.session->swrite_l(screen_index);
	link.session->flush();
	link.session->sread(sizeof(result), &result);
	if (result != B_OK) goto bail_out;

	link.session->sread(sizeof(size), &size);
	clone_info = (void *)malloc(size);
	if (clone_info)
		// grab the data
		link.session->sread(size, clone_info);
	else {
		char buffer[256];
		while (size > 0) {
			// read out what's left
			link.session->sread(sizeof(size) > sizeof(buffer) ? sizeof(buffer) : size, buffer);
			size -= 256;
		}
		result = B_NO_MEMORY;
		goto bail_out;
	}
	dprintf(("got %d bytes of clone data\n", size));

	// load the accelerant
	addon_image = load_add_on(info.addon_path);
	if (addon_image < 0) {
		result = B_ERROR;
		goto free_clone_info;
	}

	result = get_image_symbol(addon_image, B_ACCELERANT_ENTRY_POINT,B_SYMBOL_TYPE_ANY,(void **)&m_gah);
	if (result != B_OK) goto unload_image;
	
	clone_accelerant ca;
	ca = (clone_accelerant)(m_gah)(B_CLONE_ACCELERANT, NULL);
	if (!ca) goto unload_image;

	result = ca(clone_info);
	if (result != B_OK) goto unload_image;
	goto free_clone_info;	

unload_image:
	dprintf(("BWindowScreen::InitClone() unloading add-on\n", result, result));
	unload_add_on(addon_image);
	addon_image = -1;
free_clone_info:
	dprintf(("BWindowScreen::InitClone() freeing clone info\n", result, result));
	free(clone_info);
bail_out:
	dprintf(("BWindowScreen::InitClone() returns %d (0x%08x)\n  ->%s\n", result, result, strerror(result)));
	return result;
}

status_t 
BWindowScreen::GetModeFromSpace(uint32 old_space, display_mode *dmode) {
	// convert the old-style space to one of the supported modes
	int32 width;
	int32 height;
	uint32 cs;
	status_t err = mode2parms(old_space, &cs, &width, &height);
	if (err != B_OK) return err;
	float rate = 60.1;	// what rate to use?
	int32 target_area = width * height;
	int32 best_area_diff = target_area / 5;  // match has to be better than 80%
	int32 area_diff, test_area;
	
	float best_rate_diff = rate;
	float rate_diff, test_rate;

	dprintf(("BWindowScreen::GetModeFromSpace(%d) begins...\n", old_space));

	display_mode *dm = mode_list;
	int best_mode = -1;
	
	for (int index = 0; index < mode_count; index++, dm++) {
		uint32 space = dm->space;
		// is the mode pixel format compatible?
		if ((cs & 0x0FFF) == (space & 0x0FFF)) {
			test_area = dm->timing.h_display * dm->timing.v_display;
			area_diff = test_area - target_area;
			if (area_diff < 0) area_diff = -area_diff;
			
			if (area_diff < best_area_diff) {
				// a new winner!
				best_area_diff = area_diff;
				test_rate = (dm->timing.pixel_clock * 1000.0) / (double)(dm->timing.h_total * dm->timing.v_total);
				best_rate_diff = fabs(rate - test_rate);
				best_mode = index;
			} else if (area_diff == best_area_diff) {
				test_rate = (dm->timing.pixel_clock * 1000.0) / (double)(dm->timing.h_total * dm->timing.v_total);
				rate_diff = fabs(test_rate - rate);
				if (rate_diff < best_rate_diff) {
					// a new winner!
					best_mode = index;
					best_rate_diff = rate_diff;
				}
			}
		}
	}
	
	if (best_mode == -1) {
		dprintf(("BWindowScreen::GetModeFromSpace() FAILED\n"));
		return B_ERROR;
	}

	*dmode = mode_list[best_mode];
	dprintf(("BWindowScreen::GetModeFromSpace() found a mode!\n"));
	dump_display_mode((dmode));
	return B_OK;
}

status_t 
BWindowScreen::AssertDisplayMode(display_mode *dmode)
{
	status_t err;

	dprintf(("BWindowScreen::AssertDisplayMode() begins...\n"));
	if ((err = _set_standard_space_(screen_index, dmode)) == B_NO_ERROR) {
		*new_space = *dmode;
		space_mode = 1;
	}
	GetCardInfo();
	format_info.bits_per_pixel = card_info.bits_per_pixel;
	format_info.bytes_per_row = card_info.bytes_per_row;
	format_info.width = card_info.width;
	format_info.height = card_info.height;
	format_info.display_width = card_info.width;
	format_info.display_height = card_info.height;
	format_info.display_x = 0;
	format_info.display_y = 0;
	dprintf(("BWindowScreen::AssertDisplayMode() returns %d (0x%08x)\n  ->%s\n", err, err, strerror(err)));
	return err;
}

status_t BWindowScreen::SetSpace(uint32 space)
{
	status_t err;
	display_mode dmode;

	dprintf(("BWindowScreen::SetSpace(0x%08x) begins...\n", space));
	err = GetModeFromSpace(space, &dmode);
	if (err == B_OK) err = AssertDisplayMode(&dmode);
	dprintf(("BWindowScreen::SetSpace() returns %d (0x%08x)\n  ->%s\n", err, err, strerror(err)));
	return err;
}

bool BWindowScreen::CanControlFrameBuffer()
{
	bool result = (((addon_state == 1) || (addon_state == 0)) &&
			((card_info.flags & B_FRAME_BUFFER_CONTROL) != 0));
	dprintf(("BWindowScreen::CanControlFrameBuffer() returns %s\n", result?"true":"false"));
	return result;
}

status_t BWindowScreen::SetFrameBuffer(int32 width, int32 height) {
	dprintf(("BWindowScreen::SetFrameBuffer(%d, %d)\n", width, height));
	display_mode target, low, high;
	target = *new_space;
	target.virtual_width = width;
	target.virtual_height = height;
	low = target;
	high = target;
	high.virtual_height = 0xffff; // max out on height
	status_t result = _propose_display_mode_(screen_index, &target, &low, &high);
	if (result == B_OK) {
		result = AssertDisplayMode(&target);
	}
	return result;
}

status_t BWindowScreen::MoveDisplayArea(int32 x, int32 y)
{
	status_t result = B_ERROR;
	dprintf(("BWindowScreen::MoveDisplayArea(%d,%d)\n", x, y));
	if (addon_state == 1 && move_display_global) {
		result = (move_display_global)((uint16)x, (uint16)y);
	}
	if (result != B_OK)
		result = _move_display_area_(screen_index, x, y);
	if (result == B_OK) {
		new_space->h_display_start = format_info.display_x = x;
		new_space->v_display_start = format_info.display_y = y;
	}
	return result;
}

void BWindowScreen::RegisterThread(thread_id id) {
	dprintf(("BWindowScreen::RegisterThread(%d)\n", id));
	while (acquire_sem(debug_sem) == B_INTERRUPTED) {;}
	debug_list_count++;
	debug_list = (thread_id*)realloc(debug_list, sizeof(thread_id)*debug_list_count);
	debug_list[debug_list_count-1] = id;
	release_sem(debug_sem);
}

void BWindowScreen::SuspensionHook(bool active) {
	dprintf(("BWindowScreen::SuspensionHook(%s)\n", active?"true":"false"));
}

void BWindowScreen::Suspend() {
	int        i, j, size, err;
	void       *base;

	dprintf(("BWindowScreen::Suspend()\n"));
	while (acquire_sem(debug_sem) == B_INTERRUPTED) {;}
	for (i=0; i<debug_list_count; i++)
		for (j=0; j<10; j++) {
			if (suspend_thread(debug_list[i]) != B_BAD_THREAD_STATE) break;
			snooze(10000);
		}
	size = CardInfo()->bytes_per_row * CardInfo()->height;
	debug_buffer = (char*)malloc(size);
	memcpy(debug_buffer, CardInfo()->frame_buffer, size);
}

void BWindowScreen::Resume() {
	int        i, size;
	void       *base;	

	dprintf(("BWindowScreen::Resume()\n"));
	size = CardInfo()->bytes_per_row * CardInfo()->height;
	memcpy(CardInfo()->frame_buffer, debug_buffer, size);
	free(debug_buffer);
	for (i=0; i<debug_list_count; i++)
		resume_thread(debug_list[i]);
	release_sem(debug_sem);
}

void BWindowScreen::Suspend(char *label) {
	bool   state;

	dprintf(("BWindowScreen::Suspend(%s)\n", label));
	if (debug_state) {
		fprintf(stderr, "## Debugger(\"%s\").", label);
// need to be updated to take the Ctrl/Alt inversion into account.
		fprintf(stderr, " Press Alt-F%d or Cmd-F%d to resume.\n", space0+1, space0+1);
		state = IsLocked();
		if (state) Unlock();
		_activate_workspace_(debug_workspace);
		suspend_thread(find_thread(0L));
		if (state) Lock();
	}
}

frame_buffer_info *BWindowScreen::FrameBufferInfo() {
	dprintf(("BWindowScreen::FrameBufferInfo()\n"));
    return &format_info;    
}

graphics_card_info *BWindowScreen::CardInfo() {
	dprintf(("BWindowScreen::CardInfo()\n"));
    return &card_info;    
}

static int32 draw_rect_8(int32 left, int32 top, int32 right, int32 bottom, uint8 color) {
	if ((left>right)||(left>=card_info_global.width)||(right<0)||(top>bottom)||(top>=card_info_global.height)||(bottom<0)) {
		return B_BAD_VALUE;
	}
	fill_rect_params frp;
	uint32 c = color;
	c |= c << 8;
	c |= c << 16;
	frp.left = (uint16)left;
	frp.top = (uint16)top;
	frp.right = (uint16)right;
	frp.bottom = (uint16)bottom;
	(acquire_engine_global)(B_2D_ACCELERATION, 0xffffffff, 0, &et_global);
	(fill_rect_global)(et_global, c, &frp, 1);
	(release_engine_global)(et_global, 0);
	return B_OK;
}

static int32 draw_rect_32(int32 left, int32 top, int32 right, int32 bottom, uint32 color) {
	if ((left>right)||(left>=card_info_global.width)||(right<0)||(top>bottom)||(top>=card_info_global.height)||(bottom<0)) {
		return B_BAD_VALUE;
	}
	fill_rect_params frp;
	frp.left = (uint16)left;
	frp.top = (uint16)top;
	frp.right = (uint16)right;
	frp.bottom = (uint16)bottom;
	(acquire_engine_global)(B_2D_ACCELERATION, 0xffffffff, 0, &et_global);
	(fill_rect_global)(et_global, color, &frp, 1);
	(release_engine_global)(et_global, 0);
	return B_OK;
}

static int32 draw_rect_16(int32 left, int32 top, int32 right, int32 bottom, uint16 color) {
	if ((left>right)||(left>=card_info_global.width)||(right<0)||(top>bottom)||(top>=card_info_global.height)||(bottom<0)) {
		return B_BAD_VALUE;
	}
	fill_rect_params frp;
	uint32 c = color;
	c |= c << 16;
	frp.left = (uint16)left;
	frp.top = (uint16)top;
	frp.right = (uint16)right;
	frp.bottom = (uint16)bottom;
	(acquire_engine_global)(B_2D_ACCELERATION, 0xffffffff, 0, &et_global);
	(fill_rect_global)(et_global, c, &frp, 1);
	(release_engine_global)(et_global, 0);
	return B_OK;
}

static int32 blit(int32 srcX, int32 srcY, int32 dstX, int32 dstY, int32 width, int32 height) {
	if ((width<0)||(srcX<0)||(srcX+width>=card_info_global.width)||(dstX<0)||(dstX+width>=card_info_global.width)
				||(height<0)||(srcY<0)||(srcY+height>=card_info_global.height)||(dstY<0)||(dstY+height>=card_info_global.height)) {
		return B_BAD_VALUE;
	}
	blit_params bp;
	bp.src_left = (uint16)srcX;
	bp.src_top = (uint16)srcY;
	bp.dest_left = (uint16)dstX;
	bp.dest_top = (uint16)dstY;
	bp.width = (uint16)width;
	bp.height = (uint16)height;
	dprintf(("blit(src:%d,%d  dst:%d,%d  size:%d,%d)\n", bp.src_left, bp.src_top, bp.dest_left, bp.dest_top, bp.width, bp.height));
	(acquire_engine_global)(B_2D_ACCELERATION, 0xffffffff, 0, &et_global);
	(blit_rect_global)(et_global, &bp, 1);
	(release_engine_global)(et_global, 0);
	return B_OK;
}

static int32 scaled_filtered_blit(int32 srcX, int32 srcY, int32 dstX, int32 dstY, int32 src_width, int32 src_height, int32 dst_width, int32 dst_height) {
	if ((src_width<0)||(dst_width<0)||(srcX<0)||(srcX+src_width>=card_info_global.width)||(dstX<0)||(dstX+dst_width>=card_info_global.width)
				||(src_height<0)||(dst_height<0)||(srcY<0)||(srcY+src_height>=card_info_global.height)||(dstY<0)||(dstY+dst_height>=card_info_global.height)) {
		return B_BAD_VALUE;
	}
	scaled_blit_params sbp;
	sbp.src_left = (uint16)srcX;
	sbp.src_top = (uint16)srcY;
	sbp.dest_left = (uint16)dstX;
	sbp.dest_top = (uint16)dstY;
	sbp.src_width = (uint16)src_width;
	sbp.src_height = (uint16)src_height;
	sbp.dest_width = (uint16)dst_width;
	sbp.dest_height = (uint16)dst_height;
	dprintf(("blit(src:%d,%d  dst:%d,%d  src_size:%d,%d  dst_size:%d,%d)\n", sbp.src_left, sbp.src_top, sbp.dest_left, sbp.dest_top, sbp.src_width, sbp.src_height, sbp.dest_width, sbp.dest_height));
	(acquire_engine_global)(B_2D_ACCELERATION, 0xffffffff, 0, &et_global);
	(scaled_filtered_blit_rect_global)(et_global, &sbp, 1);
	(release_engine_global)(et_global, 0);
	return B_OK;
}

static int32 trans_blit(int32 srcX, int32 srcY, int32 dstX, int32 dstY, int32 width, int32 height, uint32 trans_color) {
	if ((width<0)||(srcX<0)||(srcX+width>=card_info_global.width)||(dstX<0)||(dstX+width>=card_info_global.width)
				||(height<0)||(srcY<0)||(srcY+height>=card_info_global.height)||(dstY<0)||(dstY+height>=card_info_global.height)) {
		return B_BAD_VALUE;
	}
	blit_params bp;
	bp.src_left = (uint16)srcX;
	bp.src_top = (uint16)srcY;
	bp.dest_left = (uint16)dstX;
	bp.dest_top = (uint16)dstY;
	bp.width = (uint16)width;
	bp.height = (uint16)height;
	dprintf(("trans_blit(src:%d,%d  dst:%d,%d  size:%d,%d, trans_color: 0x%08x)\n", bp.src_left, bp.src_top, bp.dest_left, bp.dest_top, bp.width, bp.height, trans_color));
	(acquire_engine_global)(B_2D_ACCELERATION, 0xffffffff, 0, &et_global);
	(trans_blit_rect_global)(et_global, trans_color, &bp, 1);
	(release_engine_global)(et_global, 0);
	return B_OK;
}

static int32 sync(void) {
	(wait_idle_global)();
	return B_OK;
}

graphics_card_hook BWindowScreen::CardHookAt(long index) {
	dprintf(("BWindowScreen::CardHookAt(%d)\n", index));
	graphics_card_hook gch = (graphics_card_hook)NULL;
	if (addon_state == 1) switch (index) {
		case 5:	// 8 bit rects
			fill_rect_global = fill_rect = (fill_rectangle)(m_gah)(B_FILL_RECTANGLE, new_space);
			if (fill_rect) gch = (graphics_card_hook)draw_rect_8;
			break;
		case 6: // 32 bit rects
			fill_rect_global = fill_rect = (fill_rectangle)(m_gah)(B_FILL_RECTANGLE, new_space);
			if (fill_rect) gch = (graphics_card_hook)draw_rect_32;
			break;
		case 13: // 16 bit rects
			fill_rect_global = fill_rect = (fill_rectangle)(m_gah)(B_FILL_RECTANGLE, new_space);
			if (fill_rect) gch = (graphics_card_hook)draw_rect_16;
			break;
		case 7: // blit
			blit_rect_global = blit_rect = (screen_to_screen_blit)(m_gah)(B_SCREEN_TO_SCREEN_BLIT, new_space);
			if (blit_rect) gch = (graphics_card_hook)blit;
			break;
		case 10: // sync
			wait_idle_global = m_wei;
			gch = (graphics_card_hook)sync;
			break;
		case 20: // trans_blit
			trans_blit_rect_global = trans_blit_rect = (screen_to_screen_transparent_blit)(m_gah)(B_SCREEN_TO_SCREEN_TRANSPARENT_BLIT, new_space);
			if (trans_blit_rect) gch = (graphics_card_hook)trans_blit;
			break;
		case 21: // scaled_filtered_blit
			scaled_filtered_blit_rect_global = scaled_filtered_blit_rect = (screen_to_screen_scaled_filtered_blit)(m_gah)(B_SCREEN_TO_SCREEN_SCALED_FILTERED_BLIT, new_space);
			if (scaled_filtered_blit_rect) gch = (graphics_card_hook)scaled_filtered_blit;
			break;
	}
	dprintf(("BWindowScreen::CardHookAt(%d) returns 0x%08x\n", index, gch));
	return gch;
}

rgb_color *BWindowScreen::ColorList() {
	dprintf(("BWindowScreen::ColorList()\n"));
    return colorList;
}

void *BWindowScreen::IOBase() {
	dprintf(("BWindowScreen::IOBase() has been deprecated\n"));
    return NULL;
}

/*---------------------------------------------------------------*/

BWindowScreen::BWindowScreen() {}
BWindowScreen::BWindowScreen(BWindowScreen &) {}
BWindowScreen &BWindowScreen::operator=(BWindowScreen &) { return *this; }

/*----------------------------------------------------------------*/

status_t BWindowScreen::Perform(perform_code d, void *arg)
{
	return inherited::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BWindowScreen::_ReservedWindowScreen1() {}
void BWindowScreen::_ReservedWindowScreen2() {}
void BWindowScreen::_ReservedWindowScreen3() {}
void BWindowScreen::_ReservedWindowScreen4() {}

/* ---------------------------------------------------------------- */



static status_t mode2parms(uint32 mode, uint32 *cs, int32 *width, int32 *height)
{
	switch (mode) {
		case B_8_BIT_640x480:
		case B_8_BIT_800x600:
		case B_8_BIT_1024x768:
		case B_8_BIT_1152x900:
		case B_8_BIT_1280x1024:
		case B_8_BIT_1600x1200:
			*cs = B_CMAP8;
			break;
		case B_15_BIT_640x480:
		case B_15_BIT_800x600:
		case B_15_BIT_1024x768:
		case B_15_BIT_1152x900:
		case B_15_BIT_1280x1024:
		case B_15_BIT_1600x1200:
			*cs = B_RGB15;
			break;
		case B_16_BIT_640x480:
		case B_16_BIT_800x600:
		case B_16_BIT_1024x768:
		case B_16_BIT_1152x900:
		case B_16_BIT_1280x1024:
		case B_16_BIT_1600x1200:
			*cs = B_RGB16;
			break;
		case B_32_BIT_640x480:
		case B_32_BIT_800x600:
		case B_32_BIT_1024x768:
		case B_32_BIT_1152x900:
		case B_32_BIT_1280x1024:
		case B_32_BIT_1600x1200:
			*cs = B_RGB32;
			break;
		default:
			return B_ERROR;
	}
	switch (mode) {
		case B_8_BIT_640x480:
		case B_15_BIT_640x480:
		case B_16_BIT_640x480:
		case B_32_BIT_640x480:
			*width = 640;
			*height = 480;
			break;
		case B_8_BIT_800x600:
		case B_15_BIT_800x600:
		case B_16_BIT_800x600:
		case B_32_BIT_800x600:
			*width = 800;
			*height = 600;
			break;
		case B_8_BIT_1024x768:
		case B_15_BIT_1024x768:
		case B_16_BIT_1024x768:
		case B_32_BIT_1024x768:
			*width = 1024;
			*height = 768;
			break;
		case B_8_BIT_1152x900:
		case B_15_BIT_1152x900:
		case B_16_BIT_1152x900:
		case B_32_BIT_1152x900:
			*width = 1152;
			*height = 900;
			break;
		case B_8_BIT_1280x1024:
		case B_15_BIT_1280x1024:
		case B_16_BIT_1280x1024:
		case B_32_BIT_1280x1024:
			*width = 1280;
			*height = 1024;
			break;
		case B_8_BIT_1600x1200:
		case B_15_BIT_1600x1200:
		case B_16_BIT_1600x1200:
		case B_32_BIT_1600x1200:
			*width = 1600;
			*height = 1200;
			break;
	}
	return B_OK;
}
