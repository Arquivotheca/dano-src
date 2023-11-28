//
//	HTMLconsts.h
//
#ifndef __HTMLCONSTS_H__
#define __HTMLCONSTS_H__


#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <StorageKit.h>
#include <stdio.h>
#include <string.h>

#include "TranslationConsts.h"

enum
{
	BACKSPACE = 	B_BACKSPACE,
	CR = 			'\r',	// a carriage return character, ^M
	LF = 			B_RETURN,
	ENTER = 		B_ENTER,
	SPACE = 		B_SPACE,
	TAB = 			B_TAB,
	ESCAPE = 		B_ESCAPE,
	LEFTARROW = 	B_LEFT_ARROW,
	RIGHTARROW =	B_RIGHT_ARROW,
	UPARROW = 		B_UP_ARROW,
	DOWNARROW = 	B_DOWN_ARROW,
	FORWARDDEL =	B_DELETE,
	HOME =			B_HOME,
	END =			B_END,
	PAGEUP =		B_PAGE_UP,
	PAGEDOWN =		B_PAGE_DOWN,
	COMMA = 		','
};

const rgb_color	OUR_BLACK = {0, 0, 0, 255};
const rgb_color	OUR_WHITE = {255, 255, 255, 255};

const rgb_color	OUR_LTGRAY = {238, 238, 238, 255};
const rgb_color	OUR_BACKGRAY = {216, 216, 216, 255};
const rgb_color	OUR_GRAY = {200, 200, 200, 255};
const rgb_color	OUR_SELECTGRAY = {160, 160, 160, 255};
const rgb_color	OUR_LOCKGRAY = {112, 112, 112, 255};
const rgb_color	OUR_DARKGRAY = {112, 112, 112, 255};

const rgb_color	OUR_RED = {255, 0, 0, 255};
const rgb_color	OUR_GREEN = {0, 255, 0, 255};
const rgb_color	OUR_BLUE = {0, 0, 255, 255};

#define INIT_BUFFER_SIZE				1024


// HTML Error codes
#define HTML_STATE_UNDERFLOW_ERR		10


#endif //__HTMLCONSTS_H__