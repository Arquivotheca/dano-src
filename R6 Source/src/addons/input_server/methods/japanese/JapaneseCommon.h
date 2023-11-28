#ifndef _JAPANESECOMMON_H
#define _JAPANESECOMMON_H


#define J_DROP_BOX_NAME	"Japanese Drop Box"	
#define J_MODE_MESSENGER_NAME	"modeMessenger"
#define J_MESSENGER 	"method"
#define J_YOMI 			"yomi"
#define J_HYOKI 		"hyoki"
#define J_HINSHI		"hinshi"
#define J_KUTOUTEN		"kutouten"
#define J_SPACE			"space"
#define J_THRESHOLD		"threshold"

#define J_SETTINGS_PALETTE_WINDOW		"PALETTE_WINDOW"
#define J_SETTINGS_PALETTE_WINDOW_LOC	"PALETTE_WINDOW_LOC"
#define J_SETTINGS_KUTOUTEN_TYPE		"PUNCTUATION_TYPE"
#define J_SETTINGS_SPACE_TYPE			"SPACE_TYPE"
#define J_SETTINGS_THRESHOLD_VALUE		"CANDIDATE_WINDOW_THRESHOLD"

#define J_PREFS_SIG						"application/x-vnd.Be-JPrefs"


const uint32 J_GRABBED_DROP_BOX 				= 'Jgdb';
const uint32 J_SET_MODE_MESSENGER 				= 'Jsmm';
const uint32 J_ADD_TO_DICTIONARY				= 'Jadd';
const uint32 J_CHANGE_KUTOUTEN_TYPE				= 'Jckt';
const uint32 J_CHANGE_SPACE_TYPE				= 'Jcsp';
const uint32 J_CHANGE_HENKAN_WINDOW_THRESHOLD	= 'Jchw';
const uint32 J_SET_KOUHO_LISTVIEW				= 'Jklv';
const uint32 J_SET_KOUHO_LIST_SELECTION			= 'Jslt';
const uint32 J_FAKED_KEYPRESS					= 'Jfkp';
const uint32 J_GET_INPUT_MODE					= 'Jgim';


const bool		J_DEFAULT_PALETTE_WINDOW			= true;
const int32		J_DEFAULT_PALETTE_WINDOW_LOC		= -10000;
const int32		J_DEFAULT_KUTOUTEN_TYPE				= 0;
const bool		J_DEFAULT_SPACE_TYPE				= false;
const int32		J_DEFAULT_HENKAN_WINDOW_THRESHOLD 	= 3;


#endif
