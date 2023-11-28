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
#ifdef __MBHUFTBL_H__
#pragma message("***** MBHUFTBL.H Included Multiple Times")
#endif
#endif

#ifndef __MBHUFTBL_H__
#define __MBHUFTBL_H__

/* This is a set of PB style descriptor tables to be used as static
   table representations.  During initialization time the PB tables
   defined below in the PBHuffTblSt arrays are converted to generic
   Huffman tables and put in the HuffTblSt arrays.  The generic
   Huffman table represention is what the encoder uses.
*/

/* Macro block PB Huffman tables.  These have a maximum bit length of
   13 bits.
 */
#define NUM_PB_MB_HUFF_TABLES 7
/* 7 static tables plus one default table */
static const PBHuffTblSt PBMBHuffTables[NUM_PB_MB_HUFF_TABLES+1] = {
	{ 8, { 0, 4, 5, 4, 4, 4, 6, 6,}},
	{12, { 0, 2, 2, 3, 3, 3, 3, 5, 3, 2, 2, 2,}},
	{12, { 0, 2, 3, 4, 3, 3, 3, 3, 4, 3, 2, 2,}},
	{12, { 0, 3, 4, 4, 3, 3, 3, 3, 3, 2, 2, 2,}},
	{13, { 0, 4, 4, 3, 3, 3, 3, 2, 3, 3, 2, 1, 1,}},
	{ 9, { 0, 4, 4, 4, 4, 3, 3, 3, 2,}},
	{10, { 0, 4, 4, 4, 4, 3, 3, 2, 2, 2,}},
	{12, { 0, 4, 4, 4, 3, 3, 2, 3, 2, 2, 2, 2,}},
};

#endif /* __MBHUFTBL_H__ */
