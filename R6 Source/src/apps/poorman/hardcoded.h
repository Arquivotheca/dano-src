/* emacs: -*- C++ -*- */
/*
 * Copyright (c) 1996 Be, Inc.	All Rights Reserved 
 *
 * Hardcoded strings, window dimensions, etc. 
 * One day this will not be necessary (hopefully)
 */
#ifndef _HARDCODED_H
#define _HARDCODED_H

#include <Message.h>
#include <ScrollBar.h>

static const char SIGNATURE[] = "application/x-vnd.Be-PMWS";
static const char WIND_TITLE[] =  "PoorMan";
static const char START_LABEL[] = "Start Server";
static const char STOP_LABEL[] = "Stop Server";

static const char START_LOG[] = "Start Logging";
static const char STOP_LOG[] = "Stop Logging";

#define PROGRAM_NAME "PoorMan"

static const char CANT_START_SERVER[] = "Can't start server.";


static const char DONT_HAVE_DIR[] = "You do not have a `%s' directory.";
static const char CANT_CREATE_DIR[] = "Cannot create directory `%s'.";
static const char CANT_CREATE_FILE[] = "Cannot create file `%s'.";
static const char NEED_HTML_DIRECTORY[] = "You must select a Web directory before starting the server.";
static const char HIT_TEXT[] = "Hits: %d";
static const char DIR_TEXT[] = "Directory: %s";
static const char ZERO_HIT_TEXT[] = "Hits: 0";
static const char STOPPED_TEXT[] = "Status: Stopped";
static const char RUNNING_TEXT[] = "Status: Running";
static const char OK[] = "OK";
static const char CANCEL[] = "Cancel";


static const int WIND_XLEFT = 82;
static const int WIND_YTOP = 30;
static const int WIND_XRIGHT = 400;
static const int WIND_YBOTTOM = 350;//130;


static const int STEXT_XLEFT = 5;
static const int STEXT_YTOP = 5;
static const int STEXT_XRIGHT = 120;
static const int STEXT_YBOTTOM = 18;

static const int HTEXT_XLEFT = 159;
static const int HTEXT_YTOP = 5;
static const int HTEXT_XRIGHT = 314;
static const int HTEXT_YBOTTOM = 18;

static const int DTEXT_XLEFT = 5;
static const int DTEXT_YTOP = 19;
static const int DTEXT_XRIGHT = 290;
static const int DTEXT_YBOTTOM = 32;

static const int BOX_XLEFT = 5;
static const int BOX_YTOP = 36;
static const int BOX_XRIGHT = 314;
static const int BOX_YBOTTOM = 295;

static const int LOG_XLEFT = 2;
static const int LOG_YTOP = 1;
static const int LOG_XRIGHT = 307 - (int)B_V_SCROLL_BAR_WIDTH;
static const int LOG_YBOTTOM = 257;

static const int SCROLL_XLEFT = 308 -(int)B_V_SCROLL_BAR_WIDTH;
static const int SCROLL_YTOP = 1;
static const int SCROLL_XRIGHT = 308;
static const int SCROLL_YBOTTOM = 257;

static const int GOT_DIR = 4;

static const int FONT_SIZE = 10;

// Miscellaneous internal messages

static const uint32 GET_PM_WINDOW = 'gPMw';

// Menu item messages

static const uint32 MENU_FILE_SAVE_CONSOLE = 'MFsc';
static const uint32 MENU_FILE_SAVE_CONSOLE_SEL = 'MFss';

static const uint32 MENU_EDIT_PREFS =	'MEpr';

static const uint32 MENU_WEB_SERVER_RUN = 'MWsr';
static const uint32 MENU_WEB_CLEAR_HITS = 'MWch';

static const uint32 MENU_WEB_CLEAR_CONSOLE= 'MWcc';

static const uint32 MENU_WEB_CLEAR_FILE = 'MWcf';

#endif /* _HARDCODED_H */

