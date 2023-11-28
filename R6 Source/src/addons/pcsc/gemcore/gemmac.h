/*******************************************************************************
*                   Copyright (c) 1995 Gemplus Development
*
* Name        : GemMac.h
*
* Description : General macro definitions for GEMPLUS programs
*
* Release     : 4.31.001
*
* Last Modif  : 13/10/97: V4.31.001
*               01/12/95: V4.10.001 - Update for 4.10 Version.
*               22/08/94: V4.00
*               06/07/94: V4.00
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/


/*------------------------------------------------------------------------------
Name definition:
   _GEMMAC_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GEMMAC_H
#define _GEMMAC_H


/*------------------------------------------------------------------------------
General C macros.
------------------------------------------------------------------------------*/
#ifndef LOWORD
#define LOWORD(l)   ((WORD)(l))
#endif
#ifndef HIWORD
#define HIWORD(l)   ((WORD)(((WORD32)(l)) >> 16))
#endif
#ifndef LOBYTE
#define LOBYTE(w)   ((BYTE)(w))
#endif
#ifndef HIBYTE
#define HIBYTE(w)   ((BYTE)(((WORD16)(w)) >> 8))
#endif

#ifndef MK_FP
#define MK_FP(seg,ofs) ((void far *)((((WORD32)(seg))<<16)|(ofs)))
#endif
#define PEEKW(seg,ofs) *((unsigned short far *)MK_FP ((seg),(ofs)))


/*------------------------------------------------------------------------------
Macros used to read a port address under DOS environment.
------------------------------------------------------------------------------*/
#define ComPort(i) PEEKW(0x40 ,2 * ((i) - 1))
#define LptPort(i) PEEKW(0x40,8 + 2*((i) - 1))

#endif 
