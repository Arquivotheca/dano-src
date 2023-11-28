/*----------------------------------------------------------

	supervga.h
	
	by Pierre Raynaud-Richard.

	Copyright (c) 1998 by Be Incorporated.
	All Rights Reserved.
	
----------------------------------------------------------*/

#define		COLOR_MODE		0
#define		GRAY_MODE		1
#define		COUNT			8
#define		BAND			20

void convert_line(int count, unsigned long *gray, unsigned char *from, unsigned char *to);
