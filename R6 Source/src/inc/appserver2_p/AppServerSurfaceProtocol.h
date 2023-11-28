
#ifndef	_APPSERVER2_SURFACEPROTOCOL_H_
#define	_APPSERVER2_SURFACEPROTOCOL_H_

namespace B {
namespace AppServer2 {

enum SurfaceProtocol {
	GR_MOVE_WINDOW = 0x580,
	GR_MOVETO_WINDOW,
	GR_SIZE_WINDOW,
	GR_SIZETO_WINDOW,
	GR_SELECT_WINDOW,
	GR_CLOSE_WINDOW,
	GR_WGET_TITLE,
	GR_WSET_TITLE,
	GR_WGET_BOX,
	GR_WIS_FRONT,
	GR_SEND_TO_BACK,
	GR_WGET_BOUND,
	GR_HIDE,
	GR_SHOW,
	GR_WINDOW_LIMITS,
	GR_IS_ACTIVE,
	GR_MINIMIZE,
	GR_MAXIMIZE,
	GR_SHOW_SYNC,
	GR_SET_WINDOW_FLAGS,
	GR_SEND_BEHIND,
	GR_ADD_SUBWINDOW,
	GR_REMOVE_SUBWINDOW,
	GR_SET_WINDOW_ALIGNMENT,
	GR_GET_WINDOW_ALIGNMENT,
	GR_GET_BASE_POINTER,
	GR_PICK_VIEW,
	GR_SET_WINDOW_PICTURE,
	GR_WGET_DECOR_STATE,
	GR_WSET_DECOR_STATE,
	GR_ADD_WINDOW_CHILD,
	GR_REMOVE_WINDOW_CHILD,
	
	GR_SET_VIEW_COLOR = 0x703,

	GR_INVAL = 0x840,
	GR_INVAL_REGION = 0x848,

	E_START_DRAW_WIND = 0x8000,
	E_END_DRAW_WIND,
	E_DRAW_VIEW,
	E_END_DRAW_VIEW,
	E_DRAW_VIEWS,
	E_START_DRAW_VIEW
};


struct update_info {
	enum {
		ufPostDraw = 0x00000001
	};

	int32	view_token;
	uint32	flags;
	BRect	current_bound;
	BRect	update_rect;
};

/*	The following are "window manager" specific, and have nothing to do with
	the new concept of the app_server pourely as a rendering surface manager.
	But currently the app_server is also the window manager, so we'll put these
	here for now. */

enum window_type {
	B_UNTYPED_WINDOW	= 0,
	B_TITLED_WINDOW 	= 1,
	B_MODAL_WINDOW 		= 3,
	B_DOCUMENT_WINDOW	= 11,
	B_BORDERED_WINDOW	= 20,
	B_FLOATING_WINDOW	= 21
};

/*----------------------------------------------------------------*/

enum window_look {
	B_BORDERED_WINDOW_LOOK	= 20,
	B_NO_BORDER_WINDOW_LOOK	= 19,
	B_TITLED_WINDOW_LOOK	= 1,	
	B_DOCUMENT_WINDOW_LOOK	= 11,
	B_MODAL_WINDOW_LOOK		= 3,
	B_FLOATING_WINDOW_LOOK	= 7
};

/*----------------------------------------------------------------*/

enum window_feel {
	B_NORMAL_WINDOW_FEEL			= 0,	
	B_MODAL_SUBSET_WINDOW_FEEL		= 2,
	B_MODAL_APP_WINDOW_FEEL			= 1,
	B_MODAL_ALL_WINDOW_FEEL			= 3,
	B_FLOATING_SUBSET_WINDOW_FEEL	= 5,
	B_FLOATING_APP_WINDOW_FEEL		= 4,
	B_FLOATING_ALL_WINDOW_FEEL		= 6
};

/*----------------------------------------------------------------*/

enum window_alignment {
	B_BYTE_ALIGNMENT	= 0,
	B_PIXEL_ALIGNMENT	= 1
};

/*----------------------------------------------------------------*/

enum {
	B_NOT_MOVABLE				= 0x00000001,
	B_NOT_CLOSABLE				= 0x00000020,
	B_NOT_ZOOMABLE				= 0x00000040,
	B_NOT_MINIMIZABLE			= 0x00004000,
	B_NOT_RESIZABLE				= 0x00000002,
	B_NOT_H_RESIZABLE			= 0x00000004,
	B_NOT_V_RESIZABLE			= 0x00000008,
	B_AVOID_FRONT				= 0x00000080,
	B_AVOID_FOCUS				= 0x00002000,
	B_WILL_ACCEPT_FIRST_CLICK	= 0x00000010,
	B_OUTLINE_RESIZE			= 0x00001000,
	B_NO_WORKSPACE_ACTIVATION	= 0x00000100,
	B_NOT_ANCHORED_ON_ACTIVATE	= 0x00020000,
	B_ASYNCHRONOUS_CONTROLS		= 0x00080000,
	B_QUIT_ON_WINDOW_CLOSE		= 0x00100000,
	B_VIEWS_CAN_OVERLAP			= 0x00200000
};

#define B_CURRENT_WORKSPACE	0
#define B_ALL_WORKSPACES	0xffffffff

} }	// namespace B::AppServer2

using namespace B::AppServer2;

#endif	/* _APPSERVER2_SURFACEPROTOCOL_H_ */
