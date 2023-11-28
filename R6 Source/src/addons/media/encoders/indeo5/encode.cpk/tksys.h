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
*               Copyright (C) 1994-1997 Intel Corp.                       *
*                         All Rights Reserved.                          *
*                                                                       *
************************************************************************/

#ifdef INCLUDE_NESTING_CHECK
#ifdef __TKSYS_H__
#pragma message("***** TKSYS.H Included Multiple Times")
#endif
#endif

#ifndef __TKSYS_H__	 
#define __TKSYS_H__

/*
 * tksys.h
 * 
 * Header file for tksys.c, malloc and free routines.
 * The use of macros to refer to the functions enables the ability to
 * turn on memory monitoring.
 */

#define SOURCE_ID __FILE__, __LINE__, __DATE__, __TIME__ 

/** FUNCTION PROTOTYPES **/

#ifdef MEMDEBUG

extern PDbl SysMallocFunc(const PChr, I32, U32, jmp_buf);
#define SysMalloc(x,y) SysMallocFunc(__FILE__, __LINE__, (x), (y))

#else

extern PDbl SysMallocFunc(U32, jmp_buf);
#define SysMalloc(x,y) SysMallocFunc((x),(y))

#endif

#ifdef MEMDEBUG

extern void	SysFreeFunc(const PChr, I32, PU8, jmp_buf);
#define SysFree(x,y) SysFreeFunc(__FILE__, __LINE__, (x), (y))

#else

extern void	SysFreeFunc(PU8, jmp_buf);
#define SysFree(x,y) SysFreeFunc((x),(y))

#endif /* MEMDEBUG */

#endif
