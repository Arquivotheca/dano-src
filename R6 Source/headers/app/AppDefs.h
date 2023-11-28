/******************************************************************************
/
/	File:			AppDefs.h
/
/	Description:	Message codes and the global cursors.
/
/	Copyright 1993-98, Be Incorporated
/
*******************************************************************************/

#ifndef _APP_DEFS_H
#define _APP_DEFS_H

#include <BeBuild.h>

/*---------------------------------------------------------*/
/*----- Global Cursors ------------------------------------*/

// Old-style cursors
extern const unsigned char B_HAND_CURSOR[];
extern const unsigned char B_I_BEAM_CURSOR[];

// New-style cursors
#ifdef  __cplusplus
class BCursor;
extern const BCursor *B_CURSOR_SYSTEM_DEFAULT;
extern const BCursor *B_CURSOR_I_BEAM;
#endif

/*---------------------------------------------------------------*/
/*----- System Message Codes ------------------------------------*/

enum {
	B_ABOUT_REQUESTED			= '_ABR',
	B_WINDOW_ACTIVATED			= '_ACT',
	B_APP_ACTIVATED				= '_ACT',	/* Same as B_WINDOW_ACTIVATED */
	B_ARGV_RECEIVED 			= '_ARG',
	B_QUIT_REQUESTED 			= '_QRQ',
	B_CLOSE_REQUESTED 			= '_QRQ',	/* Obsolete; use B_QUIT_REQUESTED */
	B_CANCEL					= '_CNC',
	B_KEY_DOWN 					= '_KYD',
	B_KEY_UP 					= '_KYU',
	B_INVALIDATE				= '_IVL',
	B_UNMAPPED_KEY_DOWN 		= '_UKD',
	B_UNMAPPED_KEY_UP 			= '_UKU',
	B_MODIFIERS_CHANGED			= '_MCH',
	B_MINIMIZE					= '_WMN',
	B_MOUSE_DOWN 				= '_MDN',
	B_MOUSE_MOVED 				= '_MMV',
	B_MOUSE_ENTER_EXIT			= '_MEX',
	B_MOUSE_UP 					= '_MUP',
	B_MOUSE_WHEEL_CHANGED		= '_MWC',
	B_OPEN_IN_WORKSPACE			= '_OWS',
	B_PRINTER_CHANGED			= '_PCH',
	B_PULSE 					= '_PUL',
	B_READY_TO_RUN 				= '_RTR',
	B_REFS_RECEIVED 			= '_RRC',
	B_RELEASE_OVERLAY_LOCK		= '_ROV',
	B_ACQUIRE_OVERLAY_LOCK		= '_AOV',
	B_REQUEST_TOOL_INFO			= '_RQT',
	B_SCREEN_CHANGED 			= '_SCH',
	B_UI_SETTINGS_CHANGED		= '_UIC',
	B_VALUE_CHANGED 			= '_VCH',
	B_VIEW_MOVED 				= '_VMV',
	B_VIEW_RESIZED 				= '_VRS',
	B_WINDOW_MOVED 				= '_WMV',
	B_WINDOW_RESIZED 			= '_WRS',
	B_WORKSPACES_CHANGED		= '_WCG',
	B_WORKSPACE_ACTIVATED		= '_WAC',
	B_ZOOM						= '_WZM',
	B_PIPESTDOUT_REQUESTED		= '_PSR',
	B_PIPESTDOUT_ACKNOWLEDGE	= '_PSA',
	B_PIPESTDOUT_RESET			= '_PSC',
	_APP_MENU_					= '_AMN',
	_BROWSER_MENUS_				= '_BRM',
	_MENU_EVENT_ 				= '_MEV',
	_PING_						= '_PBL',
	_QUIT_ 						= '_QIT',
	_VOLUME_MOUNTED_ 			= '_NVL',
	_VOLUME_UNMOUNTED_			= '_VRM',
	_MESSAGE_DROPPED_ 			= '_MDP',
	_DISPOSE_DRAG_ 				= '_DPD',
	_MENUS_DONE_				= '_MND',
	_SHOW_DRAG_HANDLES_			= '_SDH',
	_EVENTS_PENDING_ 			= '_EVP',
	_UPDATE_ 					= '_UPD',
	_UPDATE_IF_NEEDED_			= '_UPN',
	_PRINTER_INFO_				= '_PIN',
	_SETUP_PRINTER_				= '_SUP',
	_SELECT_PRINTER_			= '_PSL',
	
	/* This code is returned when reading from a port that does not
	   contain a valid message.  The resulting message has the raw
	   port data stored in "be:port_data" (B_RAW_TYPE). */
	B_RAW_PORT_DATA				= 'RAWP'
	
	/* Media Kit reserves all reserved codes starting in '_TR' */
};


/*---------------------------------------------------------*/
/*----- Other Commands ------------------------------------*/

enum {
	B_SET_PROPERTY				= 'PSET',
	B_GET_PROPERTY				= 'PGET',
	B_CREATE_PROPERTY			= 'PCRT',
	B_DELETE_PROPERTY			= 'PDEL',
	B_COUNT_PROPERTIES			= 'PCNT',
	B_EXECUTE_PROPERTY			= 'PEXE',
	B_GET_SUPPORTED_SUITES		= 'SUIT',
	B_UNDO						= 'UNDO',
	B_REDO						= 'REDO',
	B_CUT 						= 'CCUT',
	B_COPY 						= 'COPY',
	B_PASTE 					= 'PSTE',
	B_CLEAR						= 'CLER',
	B_SELECT_ALL				= 'SALL',
	B_SAVE_REQUESTED 			= 'SAVE',
	B_MESSAGE_NOT_UNDERSTOOD	= 'MNOT',
	B_NO_REPLY 					= 'NONE',
	B_REPLY 					= 'RPLY',
	B_SIMPLE_DATA				= 'DATA',
	B_MIME_DATA					= 'MIME',
	B_ARCHIVED_OBJECT			= 'ARCV',
	B_UPDATE_STATUS_BAR			= 'SBUP',
	B_RESET_STATUS_BAR			= 'SBRS',
	B_NODE_MONITOR				= 'NDMN',
	B_QUERY_UPDATE				= 'QUPD',
	B_ENDORSABLE				= 'ENDO',
	B_COPY_TARGET				= 'DDCP',
	B_MOVE_TARGET				= 'DDMV',
	B_TRASH_TARGET				= 'DDRM',
	B_LINK_TARGET				= 'DDLN',
	B_INPUT_DEVICES_CHANGED		= 'IDCH',
	B_INPUT_METHOD_EVENT		= 'IMEV',
	B_WINDOW_MOVE_TO			= 'WDMT',
	B_WINDOW_MOVE_BY			= 'WDMB',
	B_SILENT_RELAUNCH			= 'AREL',
	B_OBSERVER_NOTICE_CHANGE 	= 'NTCH',
	B_CONTROL_INVOKED			= 'CIVK',
	B_CONTROL_MODIFIED			= 'CMOD'

	/* Media Kit reserves all reserved codes starting in 'TRI' */
};

/*-------------------------------------------------------------*/
/*----- Some standard message protocol fields -----------------*/

	/* This field can be supplied in B_MOUSE_DOWN, B_MOUSE_MOVED, and        */
	/* B_MOUSE_UP message to indicate whether the cursor should be down.  If */
	/* not supplied, B_CURSOR_NEEDED is assumed.                             */
enum {
	B_CURSOR_NEEDED			= 0,	/* Allow cursor to be shown */
	B_CURSOR_NOT_NEEDED		= 1,	/* Don't allow cursor to be shown */
	B_CURSOR_MAYBE_NEEDED 	= 2		/* No change from last message */
};
#define B_MOUSE_CURSOR_NEEDED "be:cursor_needed"

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _APP_DEFS_H */
