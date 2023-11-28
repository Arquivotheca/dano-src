/*
 * strkconv.h
 * Font Fusion Copyright (c) 2000 all rights reserved by Bitstream Inc.
 * http://www.bitstream.com/
 * http://www.typesolutions.com/
 * Author: Sampo Kaasila
 *
 * This software is the property of Bitstream Inc. and it is furnished
 * under a license and may be used and copied only in accordance with the
 * terms of such license and with the inclusion of the above copyright notice.
 * This software or any other copies thereof may not be provided or otherwise
 * made available to any other person or entity except as allowed under license.
 * No title to and no ownership of the software or intellectual property
 * contained herein is hereby transferred. This information in this software
 * is subject to change without notice
 */

#ifndef __FF_STRKCONV__
#define __FF_STRKCONV__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifdef ENABLE_STRKCONV

typedef struct {
	/* private */

	/* public */	
	unsigned char *baseAddr;
	int internal_baseAddr;
	long left, right, top, bottom;
	F26Dot6 fTop26Dot6, fLeft26Dot6;
	long rowBytes;
	
	/* private */
	tsiMemObject *mem;

	short *startPoint;
	short *endPoint;
	short numberOfContours;

	long *x;
	long *y;
	char *onCurve;
	
	long xmin, xmax;
	long ymin, ymax;
} ffStrkConv;


/*
 * The stroke-converter constructor
 */
ffStrkConv *ff_NewStrkConv( tsiMemObject *mem, short numberOfContours, short *startPtr, short *endPtr,
							long *xPtr, long *yPtr, char *onCurvePtr );
							
/*
 * This uses approximations to gain SPEED at low sizes, with the idea that the approximations will not be visible at low sizes.
 * Useful sizes may be sizes up to about 36 ppem.
 *
 */
void MakeStrkBits( ffStrkConv *t, char omitBitMap, FF_GetCacheMemoryPtr funcptr, void *theCache, int bitRange255, uint8 *remapBits, long xRadius, long yRadius  );

							
/*
 * The stroke-converter destructor
 */
void ff_DeleteStrkConv( ffStrkConv *t );

#endif /*  ENABLE_STRKCONV */


#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __FF_STRKCONV__ */
