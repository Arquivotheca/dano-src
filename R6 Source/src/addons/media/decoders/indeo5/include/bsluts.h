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

/****
 **** This file contains forward and reverse look-up tables used to 
 **** encode and decode the macroblock header fields in the Indeo 5 bitstream
 ****/

#ifndef __BSLUTS_H__
#define __BSLUTS_H__

#define INVALID_ENTRY 255  /* Defines invalid entries in inverse tables */

#define NUM_CBP_IDXS  15   /* Number of cbp values in the forward table */

#define NUM_MODE0_IDXS  9  /* Number of type values in the Mode 0 forward table */
#define NUM_MODE1_IDXS  6  /* Number of type values in the Mode 1 forward table */
#define NUM_MODE2_IDXS  21 /* Number of type values in the Mode 2 forward table */
#define NUM_MODE3_IDXS  14 /* Number of type values in the Mode 3 forward table */
#define NUM_MODE4_IDXS  3  /* Number of type values in the Mode 4 forward table */
#define NUM_MODE5_IDXS  2  /* Number of type values in the Mode 5 forward table */

/* The following constant arrays are defined in bsluts.c */


extern const U8 FwdCBPTable[];     

extern const U8 InvCBPTable[]; 

extern const U8 FwdTypeMode0[];

extern const U8 InvTypeMode0[];

extern const U8 FwdTypeMode1[];

extern const U8 InvTypeMode1[]; 

extern const U8 FwdTypeMode2[]; 

extern const U8 InvTypeMode2[]; 

extern const U8 FwdTypeMode3[];

extern const U8 InvTypeMode3[];

extern const U8 FwdTypeMode4[];

extern const U8 InvTypeMode4[];

extern const U8 FwdTypeMode5[];
							
extern const U8 InvTypeMode5[];
							
#endif /* __BSLUTS_H__ */
