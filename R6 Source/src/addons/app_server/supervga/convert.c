/*----------------------------------------------------------

	convert.c
	
	by Pierre Raynaud-Richard.

	Copyright (c) 1998 by Be Incorporated.
	All Rights Reserved.
	
----------------------------------------------------------*/

#include "supervga.h"

void convert_line(int count, unsigned long *gray, unsigned char *from, unsigned char *to) {
	int				i;
	unsigned long	banks;
	
	for (i=count; i>0; i--) {
		banks = (gray[from[0]]<<7) |
				(gray[from[1]]<<6) |
				(gray[from[2]]<<5) |
				(gray[from[3]]<<4) |
				(gray[from[4]]<<3) |
				(gray[from[5]]<<2) |
				(gray[from[6]]<<1) |
				 gray[from[7]];
		to[BAND*0] = banks;
		to[BAND*4] = banks>>8;
		to[BAND*8] = banks>>16;
		to[BAND*12] = banks>>24;
		to++;
		from += 8;
	}	
}
