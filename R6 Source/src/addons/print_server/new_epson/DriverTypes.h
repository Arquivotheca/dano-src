//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef DRIVER_TYPES_H
#define DRIVER_TYPES_H

#include <SupportDefs.h>

typedef struct stPF
{
	char	*PageName;
	uint32	left_margin;
	uint32	printable_width;
	uint32	right_margin;
	uint32	top_margin;
	uint32	printable_height;	
	uint32	bottom_margin;

	float Width(void) const {
		return (left_margin + printable_width + right_margin);
	}

	float Height(void) const {
		return (top_margin + printable_height + bottom_margin);
	}

	bool operator == (struct stPF& p) const {
	 	return ((Width() == p.Width()) && (Height() == p.Height()));
	}
} tPageFormat;

typedef struct
{
	char		*ResName;
	uint32		x_dpi;
	uint32		y_dpi;
} tPrinterRes;

typedef struct
{
	char		*ColorModeName;
	int32		color_mode;
} tColorMode;

typedef struct
{
	char		*WeaveModeName;
	int32		weave_mode;
} tWeaveMode;

typedef struct
{
	char		*SpeedModeName;
	int32		speed_mode;
} tSpeedMode;

typedef struct
{
	char		*MicroDotName;
	int32		micro_dot;
} tMicroDot;

typedef struct
{
	char		*PaperName;
} tPrinterPaper;

typedef struct
{
	char 			*OrientationName;
	uint32			orientation;
}tOrientation;


// -----------------------------------------------

typedef enum
{
	LF	= 0x0A,
	FF	= 0x0C,
	CR 	= 0x0D,
	ESC	= 0x1B
} t_ascii_codes;

#endif
