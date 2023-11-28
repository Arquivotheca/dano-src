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
#ifdef __ENCODER_H__
#pragma message("***** ENCODER.H Included Multiple Times")
#endif
#endif

#ifndef __ENCODER_H__
#define __ENCODER_H__

/*
 * Main header file for the Indeo 5 encoder. It also documents (a bit) the
 * operation of the encoder.  The constants and structures here are widely
 * used by encoder routines, but are not part of the BS.
 * 
 * To produce a BS in little-endian (LE) format, the encoder ensures
 * that the data part of the BS is ALWAYS in LE format: ALL access of
 * multibyte fields MUST go thru endian-correct routines or macros. Info
 * in the header of the BS is accessed so much that this approach would be
 * error-prone, as well as tedious. Therefore the header is accessed in the
 * native mode of the processor. When the BS is to be written to disk, it's
 * header is made endian-correct; it is restored to native mode after
 * writing.
 */

/*
 * This struct contains info that will be needed by some contexts, and
 * that is CONSTANT for THAT context.  It is initialized by EncNtryScOpen,
 * but copies may be modified down the chain.
 * This struct is passed to the Open routines.
 */
typedef struct _EncConstInfoSt {
   MatrixSt Pic;	/* prototype for storage allocation */
   I32 BandId;		/* counts from 0 for each color */
   I32 Color;		/* see ivi5bs.h */
   I32 iNumRows;	/* vertical size of picture */
   I32 iNumCols;	/* horizontal size of picture */
} EncConstInfoSt, *PEncConstInfoSt;

typedef const EncConstInfoSt *PCEncConstInfoSt;

#endif
