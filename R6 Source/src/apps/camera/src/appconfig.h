/*
	appconfig.h
	Message codes and global variables used in the app.
*/

#ifndef APPCONFIG_H
#define APPCONFIG_H

#include "TPreferences.h"

extern const char	*gAppName;
extern const char	*gAppSig;
extern TPreferences	*gPrefs;

// messages used in the application
enum {
	MSG_PROBE			= '0001', // probe serial ports to find the camera
	MSG_CONNECT			= '0002', // connect to camera
	MSG_SELALL			= '0003', // select all pictures
	MSG_SAVE			= '0004', // save selected pictures
	MSG_DELETE			= '0005', // delete selected pictures
	MSG_CANCEL			= '0006', // cancel
	MSG_UPDATESEL		= '0007', // selection changed - update UI
	MSG_PROBE_USB		= '0008', // probe usb ports to find the camera

	MSG_USB				= '0009',
	MSG_SERIALPORT		= '0010',
	MSG_9600			= '0011',
	MSG_19200			= '0012',
	MSG_38400			= '0013',
	MSG_57600			= '0014',
	MSG_115200			= '0015',
};

#endif
