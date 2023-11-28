/*
	HDefines
*/

#ifndef HDEFINES_H
#define HDEFINES_H

#if __BEOS__
#include <GraphicsDefs.h>

const rgb_color
	kViewColor = { 216, 216, 216, 255 },
	kShadow = { 184, 184, 184, 255 },
	kDarkShadow  = { 150, 150, 150, 255 },
	kVeryDark = { 100, 100, 100, 255 },
	kWhite = { 255, 255, 255, 255 },
	kBlack = { 0, 0, 0, 255 },
	kRed = { 255, 0, 0, 255 };

const ulong
	msg_OK				= 'ok  ',
	msg_Cancel			= 'cncl',
	msg_FieldChanged	= 'chng';

#else // !__BEOS__

#include "BeCompat.h"

const rgb_color
	kViewColor = rgb_color(219, 219, 219, 0),
	kShadow = rgb_color(184, 184, 184, 0),
	kDarkShadow  = rgb_color(152, 152, 152, 0),
	kVeryDark = rgb_color(90, 90, 90, 0),
	kWhite = rgb_color(255, 255, 255, 0),
	kBlack = rgb_color(0, 0, 0, 0);
#endif

#endif
