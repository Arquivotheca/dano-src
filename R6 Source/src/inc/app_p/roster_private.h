/* ++++++++++
	FILE:	roster_private.h
	REVS:	$Revision: 1.21 $
	Written By: Peter Potrebic
	Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.
+++++ */

#ifndef _ROSTER_PRIVATE_H
#define _ROSTER_PRIVATE_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

#include <SupportDefs.h>

#define ROSTER_SIG			"application/x-vnd.Be-ROST"

// ---- defined in kit/apps/Roster.cpp
extern const char *ROSTER_PORT_NAME;
extern const char *ROSTER_THREAD_NAME;
extern const char *APP_SIG_PREFIX;
extern const char *BE_APP_SIG_PREFIX;
extern const char *ROSTER_MIME_SIG;
extern const char *TASK_BAR_MIME_SIG;
extern const char *KERNEL_MIME_SIG;
extern const char *APP_SERVER_MIME_SIG;

// ---- defined in servers/roster/mime.cpp
extern const char *_MIME_TABLE_;
extern const char *B_MIME_TYPE_ATTR;

#define MAIN_CLIPBOARD_NAME	"system"

// ---- defined in kit/storage/Mime.cpp
extern const char		*META_MIME_ROOT;

#define FOLDER_MIME_TYPE		"application/x-vnd.Be-directory"
#define META_FILE_MIME_TYPE		"application/x-vnd.Be-meta-mime"

enum {
	// Commands for dealing with the app management part of roster
	
	CMD_FIRST_APP_CMD		= 110,

	CMD_LAUNCH_INFO			= 110,
	CMD_ADD_APP				= 111,
	CMD_REMOVE_APP			= 112,
	CMD_IS_APP_PRE			= 113,
	CMD_COMPLETE_REG		= 114,
	CMD_REMOVE_PRE_REG		= 115,
	CMD_TEAM_FOR			= 116,
	CMD_GET_APP_LIST		= 117,
	CMD_GET_APP_INFO		= 118,
	CMD_SET_INFO			= 119,
	CMD_UPDATE_ACTIVE		= 120,
	CMD_DUMP_ROSTER			= 121,
	CMD_BROADCAST			= 122,
	CMD_ACTIVATE_APP		= 123,
	CMD_MONITOR_APPS		= 124,
	CMD_MSG_SCHEDULER		= 125,

	CMD_LAST_APP_CMD		= 149,

	// Commands for managing the clipboard(s)
	
	CMD_FIRST_CLIP_CMD		= 150,

	CMD_NEW_CLIP			= 150,
	CMD_GET_CLIP_DATA		= 151,
	CMD_SET_CLIP_DATA		= 152,
	CMD_GET_CLIP_HANDLER	= 153,
	CMD_GET_CLIP_COUNT		= 154,
	CMD_CLIP_MONITOR		= 155,

	CMD_LAST_CLIP_CMD		= 159,

	// Commands for managing the drag handles
	
	CMD_FIRST_DRAG_CMD		= 160,

	CMD_GET_DRAG_STATE		= 160,
	CMD_SET_DRAG_STATE		= 161,

	CMD_LAST_DRAG_CMD		= 169,

	// Commands for MIME stuff
	
	CMD_FIRST_MIME_CMD			= 180,

	CMD_GET_MIME_HANDLER		= 180,
	CMD_SAVE_MIME_TABLE			= 181,
	CMD_DUMP_MIME_TABLE			= 182,
	CMD_UPDATE_MIME_INFO		= 183,
	CMD_FIRST_WAKE_UP			= 184,
	CMD_NEW_MIME_TYPE			= 185,
	CMD_GUESS_MIME_TYPE			= 186,
	CMD_GET_SUPPORTING_APPS		= 187,
	CMD_MAP_EXTENSION_TO_TYPE	= 188,
	CMD_UPDATE_SUPPORTING_APPS	= 189,
	CMD_UPDATE_FILE_EXTENSION	= 190,
	CMD_MONITOR_META_MIME		= 191,
	CMD_META_MIME_CHANGED		= 192,
	CMD_SNIFFER_CONTROL			= 193,
	CMD_UPDATE_SNIFFER_RULE		= 194,

	// Commands sent to the roster by BMimeType::Set calls

	CMD_FIRST_PRIVATE_MIME				= 200,

	CMD_MIMETYPE_CREATE         		= 230,
	CMD_MIMETYPE_DELETE         		= 231,
	CMD_MIMETYPE_SET_PREFERRED_APP 		= 232,
	CMD_MIMETYPE_SET_ATTR_INFO  		= 233,
	CMD_MIMETYPE_SET_FILE_EXTENSIONS 	= 234,
	CMD_MIMETYPE_SET_SHORT_DESCR 		= 235,
	CMD_MIMETYPE_SET_LONG_DESCR 		= 236, 
	CMD_MIMETYPE_SET_ICON_FOR_TYPE 		= 237,
	CMD_MIMETYPE_SET_SUPPORTED_TYPES 	= 238,
	CMD_MIMETYPE_SET_APP_HINT   		= 239,
	CMD_MIMETYPE_SET_SNIFFER_RULE   	= 240,
                                                		
	CMD_LAST_MIME_CMD					= 299,
	
	// Commands for Shutting down the system
	
	CMD_FIRST_SHUTDOWN_CMD	= 300,
	CMD_SHUTDOWN_SYSTEM		= 301,
	CMD_REBOOT_SYSTEM		= 302,
	CMD_CLEANUP				= 303,
	CMD_SUSPEND_SYSTEM		= 304,

	CMD_LAST_SHUTDOWN_CMD	= 310,
	
	// Commands for recent docs/recent apps list
	
	CMD_GET_RECENT_DOCUMENT_LIST = 320,
	CMD_GET_RECENT_APP_LIST,
	CMD_GET_RECENT_FOLDER_LIST,
	CMD_DOCUMENT_OPENED,
	CMD_FOLDER_OPENED,

	ROSTER_REPLY			= 940
};

enum {
	CODE_TASK_ADD = 1,
	CODE_TASK_REMOVE,
	CODE_TASK_ADJUST,
	CODE_TASK_INFO
};

class BMessage;
struct version_info;
class BMimeType;
struct version_info;
struct entry_ref;

status_t _query_for_app_(BMimeType *meta, entry_ref *app_ref, version_info *);
status_t _send_to_roster_(BMessage *msg, BMessage *reply, bool target_mime = false);
bool _is_valid_roster_mess_(bool mime);

/* -- following converts old signatures (extended character constants)
 * to new mimetypes.
 */
extern char *to_new_sig(uint32 osig, char *buffer);

#endif
