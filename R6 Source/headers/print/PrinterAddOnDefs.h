// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#ifndef _PRINTER_ADDON_DEFS_H_
#define _PRINTER_ADDON_DEFS_H_

#include <string.h>
#include <SupportDefs.h>

enum orientation_type
{	B_PRINT_LAYOUT_PORTRAIT,
	B_PRINT_LAYOUT_LANDSCAPE
};

enum 
{ // Bitmask returned by Attributes()
	B_PRINT_ATTR_CENTERED		= 0x00000001,	// the margins are adjusted so that the PrintableRect() is centered
	B_PRINT_ATTR_H_MIRROR		= 0x00000002,	// horizontal mirror is to be performed
	B_PRINT_ATTR_V_MIRROR		= 0x00000004,	// vertical mirror is to be performed
	B_PRINT_ATTR_ASSEMBLED		= 0x00010000,	// copies assembled
	B_PRINT_ATTR_REVERSE		= 0x00020000	// first page is printed last
};

struct printer_status_t
{
	printer_status_t() : status(B_UNKNOWN), time_sec(-1)
	{
		memset(CMYKcmyk, 0xFF, sizeof(CMYKcmyk));
		memset(_reserved, 0, sizeof(_reserved));
	}

	enum
	{
		B_UNKNOWN = -1,
		B_ONLINE, B_OFFLINE, B_PRINTING, B_CLEANING,
		B_PAPER_JAM, B_NO_PAPER, B_NO_INK, B_ERROR,
		B_COVER_OPEN
	};
	int32 status;		// printer's status
	int32 time_sec;		// time remaining until the end of the current action (if applicable)
	uint8 CMYKcmyk[8];	// Ink levels (%), 0xFF=unknown/not applicable
	uint32 _reserved[4];
};

#endif
