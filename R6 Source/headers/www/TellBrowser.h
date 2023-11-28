#ifndef _TELL_BROWSER_H
#define _TELL_BROWSER_H

// message command constats

enum {
	TB_OPEN_URL			= 'tbOU', // open "be:url" as normal web page
	TB_OPEN_TOP			= 'tbOT', // open "be:url" as a top-level page
	TB_REWIND			= 'tbRW', // jump back to first page in history and clear the history stack
	TB_OPEN_ALERT		= 'tbOA', // open "be:url" as an Alert
	TB_GO_BACKWARD		= 'tbGB', // tell browser go back a page in history
	TB_GO_FORWARD		= 'tbGF', // tell browser go forward a page in history
	TB_GO_HOME			= 'tbGH', // tell browser go to home page
	TB_GO_START			= 'tbGS', // tell browser go to start page
	TB_RELOAD			= 'tbRL', // tell browser reload page
	TB_STOP				= 'tbSL', // tell browser stop loading
	TB_PRINT			= 'tbPR', // tell browser print page
	TB_LAUNCH_BROWSER	= 'tbLH', // launch browser if not running
	TB_START_BROWSER	= 'tbSB', // launch browser
	TB_QUIT_BROWSER		= 'tbQT', // quit browser
	TB_ALERT_RESPONSE	= 'tbAR', // response from an html alert
	TB_SHOW_TOOLBAR		= 'tbHT', // use bool "be:show_toolbar" to show or hide the toolbar

	// these two can take a "be:stream" value (from miniplay.h) to work on some specific volume
	TB_SET_VOLUME		= 'tbSV', // 1 or 2 floats "be:volume" between 0 and 1 for main playback volume
								  // bool "be:mute" for setting/clearing the mute
	TB_GET_VOLUME		= 'tbGV', // returns 2 floats "be:volume" for volume, bool "be:mute" for mute

	TB_SET_ENV			= 'tbSE', // set environment variables for each string member

	TB_SET_SETTINGS     = 'tbSs', // string be:settings == settings name, other strings are
		                          // entries to change
	TB_GET_SETTINGS     = 'tbGs', // string be:settings == settings name, returns strings w/ values
		
	// set system time and date
	TB_SET_TIMEDATE     = 'tbTD', // "year", "month", "day", "hour", "minute" -- float
		
	// set/get mouse params,  float:speed 
	TB_SET_MOUSE        = 'tbSM',
	TB_GET_MOUSE        = 'tbGM',
		
	// these two come from sndcmd.cpp
	TB_CMD_REQUEST		= 'ScRq', // default message sent by sndcmd
	TB_CMD_REPLY		= 'ScRp', // reply expected by sndcmd, sent by default

	// TellBrowser upgrade process - only works with TellBrowser
	TB_UPGRADE			= 'tbUG', // upgrade system
	TB_OPEN_STATUS		= 'tbOS', // open fullscreen window with status info
	TB_UPDATE_STATUS	= 'tbUS', // update the content status
	TB_CLOSE_STATUS		= 'tbCS', // close the status window

	TB_GET_JAPANESE		= 'getJ', // read the Japanese input method settings file
	TB_SET_JAPANESE		= 'setJ', // write the Japanese input method settings file
	TB_ADD_TO_DICTIONARY= 'addJ', // add an entry to the dictionary

	TB_DUMMY_LAST		= 0       // to avoid having to add commas
};

#define kTellBrowserSig "application/x-vnd.Be-TellBrowser"

#endif
