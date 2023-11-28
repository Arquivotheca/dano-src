#ifndef COLORS_H
#define COLORS_H

#include <InterfaceDefs.h>
	
enum UIColor {
	C_DARKEST_COLOR,
	C_LIGHTEST_COLOR,
	C_BACKGROUND_COLOR,
	C_DEPRESSED_COLOR,
	C_BEVEL_HIGHEST,
	C_BEVEL_HIGH,
	C_BEVEL_MID,
	C_BEVEL_LOW,
	C_DROP_TARGET_TEXT_FOREGROUND,
	C_NORMAL_TEXT_BACKGROUND,
	C_NORMAL_TEXT_FOREGROUND,
	C_SELECTED_TEXT_BACKGROUND,
	C_SELECTED_TEXT_FOREGROUND,
	C_HEADER_TEXT_FOREGROUND
};

const rgb_color colors[] = {
	{ 0, 0, 0, 255 },			// Darkest
	{ 255, 255, 255, 255 },		// Lightest
	{ 215, 215, 215, 255 },		// Background

	{ 205, 205, 205, 255},		// Depressed
	{ 189, 186, 189, 255 },		// Bevel Highest
	{ 156, 154, 156, 255 },		// Bevel High
	{ 148, 150, 148, 255 },		// Bevel Mid
	{ 99, 101, 99, 255 },		// Bevel Low

	{ 255, 0, 0, 255 },			// Drop Target text fore
	{ 255, 255, 255, 255 },		// normal text back
	{ 0, 0, 0, 255 },			// normal text fore
	{ 125, 190, 255, 255 },		// selected text back
	{ 0, 0, 0, 255 },			// selected text fore
	{ 255, 64, 64, 255 }		// header text fore
};


//
//	The GetUIColor macro is nice because it allows
// 	you to quickly change how the colors are stored.
//

#define GetUIColor(WHICH) (colors[WHICH])

#endif
