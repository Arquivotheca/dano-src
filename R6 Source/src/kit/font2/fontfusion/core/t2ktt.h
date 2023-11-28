/*
 * T2KTT.H
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

#ifndef __T2K_TT__
#define __T2K_TT__

/* #include "fscdefs.h" */
/* #include "fontscal.h" */
/* #include "fontmath.h" */
#include "fnt.h"

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

typedef struct {
	/* public */	
	

	/* semi private */
	
	/* private */
	tsiMemObject *mem;
	long xPixelsPerEm, yPixelsPerEm;
	int16 *ocvt;
	long numCVTs;
	F26Dot6 *ptr32;
	fnt_GlobalGraphicStateType globalGS;
	uint16 UPEM;
	int16	maxTwilightPoints;
	uint32 pgmLength[MAXPREPROGRAMS];	/* each program length is in here */
	
	int16 spZeroWord, epZeroWord;
	fnt_ElementType elements[2];
} T2KTTClass;

/*
 *
 */
T2KTTClass *New_T2KTTClass( tsiMemObject *mem, InputStream *in, /* sfntClass */ void *font );


void SetScale_T2KTTClass( T2KTTClass *t, long xPixelsPerEm, long yPixelsPerEm );

void GridOutline_T2KTTClass( T2KTTClass *t, GlyphClass *glyph );


/*
 *
 */
void Delete_T2KTTClass( T2KTTClass *t );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif /* __T2K_TT__ */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/t2ktt.h 1.4 1999/10/18 17:07:56 jfatal release $
 *                                                                           *
 *     $Log: t2ktt.h $
 *     Revision 1.4  1999/10/18 17:07:56  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.3  1999/09/30 15:12:24  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:58:39  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
