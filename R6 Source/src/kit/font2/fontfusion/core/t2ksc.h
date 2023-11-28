/*
 * T2ksc.h
 * Font Fusion Copyright (c) 1989-1999 all rights reserved by Bitstream Inc.
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

#ifndef __T2K_SC__
#define __T2K_SC__
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */




#define IS_POS_T2K_EDGE 0x01


typedef struct  {
	int32 coordinate25Dot6_flag1;
	void *next;
} T2KInterSectType;



#define T2K_SC_BASE_BUFFER_SIZE 32 /* 32 */
#define T2K_SC_FREE_BLOCK_BUFFER_SIZE 32 /* 32 */
#define T2K_SC_FIRST_MEM_BLOCK_BUFFER_SIZE 256*4 /* 256 */

typedef struct {
	/* private */
#ifdef ENABLE_MORE_TT_COMPATIBILITY
	long prevX0, prevY0;
#endif
	/* public */	
	
	long left, right, top, bottom;
	F26Dot6 fTop26Dot6, fLeft26Dot6;
	long rowBytes;
	unsigned char *baseAddr;
	int internal_baseAddr;

	
	
	/* private */
	int scanBBoxIsComputed;
	
	long xminSC, xmaxSC;
	long yminSC, ymaxSC;
	
	long outlineXMin, outlineXMax;
	long outlineYMin, outlineYMax;
	
	int couldOverflowIntegerMath;

	T2KInterSectType *yBaseBuffer[T2K_SC_BASE_BUFFER_SIZE];
	T2KInterSectType **yEdgeHead, **yBase;	/* T2KInterSectType *yEdgeHead[], The normal y-scan-line info */
	long minYIndex, maxYIndex;							/* max and min index into yEdgeHead[] */
	
	T2KInterSectType *xBaseBuffer[T2K_SC_BASE_BUFFER_SIZE];
	T2KInterSectType **xEdgeHead, **xBase;	/* Same for x */
	long minXIndex, maxXIndex;
	
	T2KInterSectType *free;			/* first free */
	T2KInterSectType *freeEnd;		/* One past the last legal position */
	
	T2KInterSectType firstMemBlockBuffer[T2K_SC_FIRST_MEM_BLOCK_BUFFER_SIZE];
	T2KInterSectType *freeMemBlocksBuffer[ T2K_SC_FREE_BLOCK_BUFFER_SIZE ];
	T2KInterSectType **freeMemBlocks; /* *freeMemBlocks[ freeMemBlockMaxCount ] */
	long freeMemBlockMaxCount;
	long freeMemBlockN;
	

	long  maxError;
	uint8 greyScaleLevel;
	char xDropOutControl;
	char yDropOutControl;
	char includeStubs; /* best quality setting is false */
	char smartDropout; /* best quality setting is true  */
	char doXEdges;
	
	int weDidXDropouts;
	int weDidYDropouts;
	
	short *startPoint;
	short *endPoint;
	short numberOfContours;

	long *x;
	long *y;
	char *onCurve;

	tsiMemObject *mem;
} tsiScanConv;

tsiScanConv *tsi_NewScanConv( tsiMemObject *mem, short numberOfContours, short *startPtr, short *endPtr,
				              long *xPtr, long *yPtr, char *onCurvePtr, uint8 greyScaleLevel,
				              char curveType, char xDropOutControl, char yDropOutControl, int smart_droput, int include_stubs, F26Dot6 oneHalfFUnit );
void MakeBits( tsiScanConv *t, char xWeightIsOne, char omitBitMap, FF_GetCacheMemoryPtr funcptr, void *theCache, int bitRange255, uint8 *remapBits  );

void tsi_DeleteScanConv( tsiScanConv *t );


#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_SC__ */

/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2ksc.h 1.6 1999/12/09 22:06:58 reggers release $
 *                                                                           *
 *     $Log: t2ksc.h $
 *     Revision 1.6  1999/12/09 22:06:58  reggers
 *     Sampo: multiple TrueType compatibility enhancements (scan converter)
 *     Revision 1.4  1999/09/30 15:12:16  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.3  1999/08/27 20:08:41  reggers
 *     Latest changes from Sampo
 *     Revision 1.2  1999/05/17 15:58:27  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
