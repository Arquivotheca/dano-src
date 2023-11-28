// cine_functions.h by Simon Clarke

#ifndef __CINE_FUNCTIONS_H
#define __CINE_FUNCTIONS_H

#include <SupportDefs.h>

short cpDecompress(uchar *data, uchar *baseAddr,
				   long rowuchars, long width, long height,
				   long *codebook, long depth);

#endif
