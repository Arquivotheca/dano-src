/*******************************************************************************
*                   Copyright (c) 1995-1997 Gemplus Development
*
* Name        : GemCARD.h
*
* Description : General definition for GEMPLUS programs using Card Identifier.
*
* Release     : 4.31.002
*
* Last Modif  : 30/03/98: V4.31.002 Add JAVACARD definition. 
*               13/10/97: V4.31.001 
*               06/02/97: V4.30.001 Add GCL8K and GCR680_ASIC cards definitions
*               16/07/96: V4.30.000 Use OROS cards definitions.
*               27/10/95: V4.10.001
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
   _GEMCARD_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GEMCARD_H
#define _GEMCARD_H

/*------------------------------------------------------------------------------
Card section:
------------------------------------------------------------------------------*/

#define  ISOCARD                 0x02
#define  COSCARD                 0x02
#define  JAVACARD                0x02
#define  FASTISOCARD             0x12

#define  GFM                     0x06

#define  GPM103                  0x07
#define  GPM256                  0x03
#define  GPM271                  0x0E
#define  GPM276                  0x0D
#define  GPM416                  0x04  /* 0V on the pad fuse C4 of IFD.       */
#define  GPM416R                 0x14  /* 5V on the pad fuse C4 of IFD.       */
#define  GPM896                  0x04  /* Simulate a fuse blown on ICC.       */
#define  GPM896R                 0x14  /* To have the real comportment.       */
#define  GPM2K                   0x09
#define  GPM8K                   0x08

#define  GAM                     0x0A
#define  GAM144                  0x0A
#define  GAM226                  0x0F

#define  GSM1K                   0xF4
#define  GSM4K                   0xF6


#define  GCL8K                   0x02
#define  GCR680_ASIC             0x02

#endif
