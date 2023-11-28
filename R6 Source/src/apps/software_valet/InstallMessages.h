// InstallMessages.h
#ifndef _INSTALLMESSAGES_H_
#define _INSTALLMESSAGES_H_


#include <Errors.h>

enum {
	M_DO_INSTALL =			'inst',
	M_DO_LOGFILE =			'logf',
	M_VOL_SELECTED =		'VolS',
	M_CHECK_VOL =			'ChkV',
	M_FOLDER_SELECTED =		'FolS',
	M_UPDATE_PROGRESS = 	'Uprg',
	M_GENERAL_TASK =		'GeTs',
	M_EXTRACT_ITEMS = 		'Extr',
	M_CURRENT_FILENAME = 	'CFln',
	M_DONE = 				'Done',
	M_CANCEL =				'Canc',
	
	M_ITEMS_SELECTED =		'ISel'
};

enum {
	D_INSTLLER_ERROR_BASE	= B_ERRORS_END,
	D_SKIP_ITEM
};

#endif

