/*****************************************************************************

     $Source: /net/bally/be/rcs/src/kit/interface/MenuPrivate.h,v $

     $Revision: 1.11 $

     $Author: peter $

     $Date: 1996/11/06 21:42:00 $

     Copyright (c) 1994 by Be Incorporated.  All Rights Reserved.

*****************************************************************************/

#ifndef _MENU_PRIVATE_H
#define _MENU_PRIVATE_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#ifndef _INTERFACE_DEFS_H
#include "InterfaceDefs.h"
#endif

#ifndef _INTERFACE_MISC_H
#include <interface_misc.h>
#endif

enum {
	IDLE_MENU,
	NOW_TRACKING,
	POP_TRACKING,
	SELECT_NEXT,
	SELECT_PREV,
	EXIT_TRACKING
};

enum {
	SHOW_MENU = 0,
	DONT_SHOW_MENU,
	KEEP_SAME
};

#define _NEW_STYLE_BM_ 1

#if _NEW_STYLE_BM_
#define _LONG_BUTTON_W_		22
#define _SHORT_BUTTON_W_	17
#define _ALT_STR_W_			12
#define _OPTION_STR_W_		12
#define _CONTROL_STR_W_		12
#define _CMD_SYM_STR_W_		12
#define _SHIFT_STR_W_		16

#define _BM_SPACING_		1
#define _BM_HEIGHT_			11
#define _BM_SHORT_W_		17
#define _BM_ALT_W_			17
#define _BM_CONTROL_W_		17
#define _BM_CMD_SYM_W_		17
#define _BM_OPTION_W_		17
#define _BM_SHIFT_W_		22
#else
#define _BM_SPACING_		2
#define _BM_SHORT_W_		9
#define _BM_ALT_W_			9
#define _BM_CONTROL_W_		9
#define _BM_SHIFT_W_		9
#define _BM_OPTION_W_		9
#endif

#define SCRL_HEIGHT	10

const BRect standard_pad(14, 1, 20, 1);
const BRect menu_bar_pad(8, 2, 8, 2);

const float cLIGHTEN_1	= 0.590;	// 216 --> 232.0 (232)
const float cLIGHTEN_2	= 0.385;	// 216 --> 240.0 (240)

const float cDARKEN_1	= 1.147;	// 216 --> 184.2 (184)
const float cDARKEN_2	= 1.295;	// 216 --> 152.3 (152)
const float cDARKEN_3	= 1.407;	// 216 --> 128.1 (128)
const float cDARKEN_4	= 1.555;	// 216 -->  96.1  (96)


	/*
	 Do controls "blend" with the background color, or to they always
	 draw gray.
	*/
#define _COLORED_CONTROLS_ 0

	/*
	 Do disabled controls (RadioButton, CheckBox, TextControl) fill their
	 areas with light gray when they are disabled.
	*/
#define _FILL_DISABLED_CONTROLS_ 1

#define DISABLED_C			cDARKEN_3
#define HILITE_BKG_C		cDARKEN_2
#define DISABLED_FILL_C		cLIGHTEN_2

#define DISABLED_MARK_C		cLIGHTEN_2

#define kEmptyMenuString "<empty>"
/* ---------------------------------------------------------------- */
float pt_distance(BPoint pt, BRect rect);
float pt_distance(BPoint pt1, BPoint pt2);

// snake menu defines
const int32 kSnakeGap = 6;
const int32 kHorizontalMenuGap = kSnakeGap + 4;
const int32 kVerticalMenuGap = kSnakeGap + 5;

const int32 kItemSelectionRadius = 2;
const int32 kSnakeBendRadius = 3;

#endif
