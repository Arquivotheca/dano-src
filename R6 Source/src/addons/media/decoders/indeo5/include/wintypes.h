/************************************************************************
*                                                                       *
*               INTEL CORPORATION PROPRIETARY INFORMATION               *
*                                                                       *
*    This listing is supplied under the terms of a license agreement    *
*      with INTEL Corporation and may not be copied nor disclosed       *
*        except in accordance with the terms of that agreement.         *
*                                                                       *
*************************************************************************
*                                                                       *
*               Copyright (C) 1994-1997 Intel Corp.                     *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/
/*************************************************************************
 *                                                                       *
 *              INTEL CORPORATION PROPRIETARY INFORMATION                *
 *                                                                       *
 *      This software is supplied under the terms of a license           *
 *      agreement or nondisclosure agreement with Intel Corporation      *
 *      and may not be copied or disclosed except in accordance          *
 *      with the terms of that agreement.                                *
 *                                                                       *
 *************************************************************************/
/***********************************************************
 * WINTYPES.H - win16 data types
 ***********************************************************/

#ifndef WINTYPES_H
#define WINTYPES_H

/* WINDOWS 3.x under DOS data types */
#ifndef WIN32
#define TEXT(quote) quote
#endif

#ifdef WIN32
#ifndef NEAR
#define NEAR 
#endif

#ifndef FAR
#define FAR
#endif

#ifndef HUGE
#define HUGE
#endif

#ifndef _loadds
#define _loadds
#endif

#ifndef _export
#define _export
#endif

#else
#define NEAR _near
#define FAR  _far
#define HUGE _huge
#endif

typedef unsigned char  U8;
typedef signed char    I8;
typedef unsigned short U16;
typedef signed short   I16;
typedef unsigned long  U32;
typedef signed long    I32;
typedef float          Sngl;
typedef double         Dbl;

/* pointers */
typedef U8   HUGE* PU8;        /* huge pointer to unsigned char */
typedef I8   HUGE* PI8;        /* huge pointer to signed char */
typedef U16  HUGE* PU16;       /* huge pointer to unsigned 16 bit value */
typedef I16  HUGE* PI16;       /* huge pointer to signed 16 bit value */
typedef U32  HUGE* PU32;       /* huge pointer to unsigned 32 bit value */
typedef I32  HUGE* PI32;       /* huge pointer to signed 32 bit value */
typedef Sngl HUGE* PSngl;      /* huge pointer to single precision float */
typedef Dbl  HUGE* PDbl;       /* huge pointer to double precision float */

typedef U8   FAR* FPU8;        /* huge pointer to unsigned char */
typedef I8   FAR* FPI8;        /* huge pointer to signed char */
typedef U16  FAR* FPU16;       /* huge pointer to unsigned 16 bit value */
typedef I16  FAR* FPI16;       /* huge pointer to signed 16 bit value */
typedef U32  FAR* FPU32;       /* huge pointer to unsigned 32 bit value */
typedef I32  FAR* FPI32;       /* huge pointer to signed 32 bit value */
typedef Sngl FAR* FPSngl;      /* huge pointer to single precision float */
typedef Dbl  FAR* FPDbl;       /* huge pointer to double precision float */

#define LOADDS _loadds

#endif

