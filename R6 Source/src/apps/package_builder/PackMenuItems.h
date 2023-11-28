#ifndef _PACKMENUITEMS_H
#define _PACKMENUITEMS_H


// PackMenuItems.h


enum {
	MENU_NEW =					0,
	MENU_OPEN,
	MENU_EDIT_GROUPS = 			3,
	MENU_EDIT_DESTINATIONS,
	MENU_EDIT_CONDITIONS,
	MENU_SAVE	=				6,
	MENU_SAVE_AS,
	MENU_CLOSE,
	MENU_PREFS =				10,
	MENU_ABOUT,
	MENU_QUIT =					13
} package_menu;

enum {
	MENU_ADD_FILES =	0,
	MENU_ADD_FOLDERS,
	MENU_ADD_SCRIPT,
	MENU_ADD_PATCH = 4,
	//MENU_TEST_PATCH,
	MENU_EXTRACT = 6,
	MENU_DELETE,
	MENU_NEW_FOLDER,
	MENU_RENAME,
	MENU_SELECT_ALL = 11,
	MENU_UPDATE
} items_menu;

enum {
	MENU_SPLASH = 0,
	MENU_INSTALLER_SETTINGS,
	MENU_BUILD_INSTALLER = 4
} installer_menu;

#endif