/*
 * Syshead.h
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

 /*
  * Here is an example of what an embedded client can do instead
  * of including the regular ANSI headers.
  */
#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#ifdef YOUR_COMPANY
#include "headers.h"
#define assert(h) Assert(h)
#include "memorymanager.h"

/* Moved to CONFIG.H ...
 *    #define malloc(n)			AllocateTaggedMemoryNilAllowed(n,"t2k")
 *     #define free(p)				FreeTaggedMemory(p,"t2k")
 *     #define realloc(ptr, size)	ReallocateTaggedMemoryNilAllowed(ptr, size, "t2k");
 */
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#endif

#ifdef __cplusplus
}
#endif  /* __cplusplus */
/*********************** R E V I S I O N   H I S T O R Y **********************
 *  
 *     $Header: R:/src/FontFusion/Source/Core/rcs/syshead.h 1.6 2000/10/06 16:04:03 reggers Exp $
 *                                                                           *
 *     $Log: syshead.h $
 *     Revision 1.6  2000/10/06 16:04:03  reggers
 *     Removed unneeded #include of math.h
 *     Revision 1.5  1999/10/18 17:01:09  jfatal
 *     Changed all include file names to lower case.
 *     Revision 1.4  1999/09/30 15:11:56  jfatal
 *     Added correct Copyright notice.
 *     Revision 1.3  1999/07/22 15:16:09  sampo
 *     Put 'C++ wrapper" around whole file.
 *     Revision 1.2  1999/05/17 15:57:42  reggers
 *     Inital Revision
 *                                                                           *
******************************************************************************/
