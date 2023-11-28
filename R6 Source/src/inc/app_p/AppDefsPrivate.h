/*****************************************************************************

     $Source: /net/bally/be/rcs/src/inc/app_p/AppDefsPrivate.h,v $

     $Revision: 1.24 $

     $Author: peter $

     $Date: 1997/03/09 14:53:52 $

     Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef _APP_DEFS_PRIVATE_H
#define _APP_DEFS_PRIVATE_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif

#ifndef _ROSTER_H
#include <Roster.h>
#endif
#include <message_strings.h>

struct entry_ref;

struct _drag_data_ {
	area_id			serverArea;
	area_id			area;
	uint32			areaBase;
	sem_id			sem;
	bigtime_t		timestamp;
};

enum {
	/* Browser Event codes */
	APP_STARTED			= B_SOME_APP_LAUNCHED,
	APP_SWITCH			= B_SOME_APP_ACTIVATED,
	APP_QUIT			= B_SOME_APP_QUIT,
	CLOSE_PANELS		= 'BRCP',
	RUN_OPEN_PANEL		= 'BROP',
	RUN_SAVE_PANEL		= 'BRSV',

	/* other private/internal event codes */
	ALERT_BUTTON_MSG	= 'ALTB',

	/* command for drag&drop of views */
	CMD_DELETE_VIEW		= 'JAHA',
	CMD_DUPLICATE_VIEW	= 'JAHB'
};

#define _OPEN_ITEMS_STRING_	"__BE__WAIT_FOR_OPEN_ITEMS__"

// private 'bit' in app_info.flags field
#define DEFAULT_APP_FLAGS_BIT	(0x10000000)
#define DEFAULT_APP_FLAGS		(DEFAULT_APP_FLAGS_BIT | B_MULTIPLE_LAUNCH | B_ARGV_ONLY)

// MemUtils calls
extern "C" void*	sh_malloc(size_t nbytes);
extern "C" void*	sh_realloc(void *block, size_t size);
extern "C" void		sh_free(void *block);
extern int _init_shared_heap_();

extern void _delete_menu_bitmaps_(void);

status_t _get_sig_and_flags_(const entry_ref *ref, char *mime_sig,
	uint32 *flags);

team_id _find_cur_team_id_();

void __set_window_decor(int32);

#endif
