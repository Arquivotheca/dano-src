/*
 * Dtypes.h
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

#ifndef __T2K_DTYPES__
#define __T2K_DTYPES__
#define int32 long
#define uint32 unsigned long
#define int16 short
#define uint16 unsigned short
#define uint8 unsigned char
#define int8 signed char


#define F26Dot6  long
#define F16Dot16 long
#ifdef Fract
#undef Fract
#endif
#define Fract long
#ifdef Fixed
#undef Fixed
#endif
#define Fixed long

#define ONE16Dot16 0x10000


#ifndef false
#define false 0
#endif



#ifndef true
#define true 1
#endif

typedef void *(*FF_GetCacheMemoryPtr)( void *theCache, uint32 length );


#endif /* _T2K_DTYPES__ */


/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/dtypes.h 1.3 1999/09/30 15:11:09 jfatal release $
 *                                                                           *
 *     $Log: dtypes.h $
 *     Revision 1.3  1999/09/30 15:11:09  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.2  1999/05/17 15:56:38  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
