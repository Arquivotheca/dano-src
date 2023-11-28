
#ifndef _SUPPORT2_MESSAGECODES_H_
#define _SUPPORT2_MESSAGECODES_H_

namespace B {
namespace Support2 {

/**************************************************************************************/

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

/**************************************************************************************/

} } // namespace B::Support2

#endif	// _SUPPORT2_MESSAGECODES_H_
