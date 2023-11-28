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
#ifdef __BKHUFTBL_H__
#pragma message("***** BKHUFTBL.H Included Multiple Times")
#endif
#endif

#ifndef __BKHUFTBL_H__
#define __BKHUFTBL_H__

/* This file contains the set of static tables used for the Huffman 
   coding of the RV data.
*/

/* Block tables.  These tables have a maximum bit length of 13 bits. */
#define NUM_PB_BLK_HUFF_TABLES 7
static const PBHuffTblSt PBBlkHuffTables[NUM_PB_BLK_HUFF_TABLES+1] = {
	/* Global block Huffman table set available for all bands */
	{10, { 1, 2, 3, 4, 4, 7, 5, 5, 4, 1,}},
	{11, { 2, 3, 4, 4, 4, 7, 5, 4, 3, 3, 2,}},
	{12, { 2, 4, 5, 5, 5, 5, 6, 4, 4, 3, 1, 1,}},
	{13, { 3, 3, 4, 4, 5, 6, 6, 4, 4, 3, 2, 1, 1,}},
	{11, { 3, 4, 4, 5, 5, 5, 6, 5, 4, 2, 2,}},
	{13, { 3, 4, 5, 5, 5, 5, 6, 4, 3, 3, 2, 1, 1,}},
	{13, { 3, 4, 5, 5, 5, 6, 5, 4, 3, 3, 2, 1, 1,}},
	/* Default Huffman table */
	{ 9, { 3, 4, 4, 5, 5, 5, 6, 5, 5,}},
};

#endif /* __BKHUFTBL_H__ */
