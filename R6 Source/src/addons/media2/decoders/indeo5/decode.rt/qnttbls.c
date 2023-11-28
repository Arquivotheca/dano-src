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

/* 
 * qnttbls.c
 *
 * Handle tables associated with quantization.  This includes a list of 
 * the transforms, the scan orders, and the information necessary for 
 * the quantization tables.
 *
 * Functions:
 *  InitQuantTables	 Initialize a set of quantization tables
 *
 */


#include <stdio.h>
#include <string.h>

#include "datatype.h"
#ifdef DEBUG
#include "pia_main.h"
#endif

#include "qnttbls.h"
#include "common.h"

const U8 xform_to_scan[5] = 
	{ 	SCAN_ZIGZAG88,
		SCAN_COLUMN88,
		SCAN_ROW88,
		SCAN_ROW88,
		SCAN_ZIGZAG44,
	};		

U8 ubScan[4][64]  = { /* 4 tables with 64 entries each */
	{ /*  SCAN_ZIGZAG88 : 
	   * zig zag order in 8x8 blocks, starting from 0 and 1.
	   */ 
 		 0, 1, 8,16, 9, 2, 3,10,17,24,32,25,18,11, 4, 5,
		12,19,26,33,40,48,41,34,27,20,13, 6, 7,14,21,28, 	
		35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
 		58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
	},
	{ /*  SCAN_COLUMN88 :
	   * scan along the columns in 8x8 blocks
	   */ 
 		 0, 8,16,24,32,40,48,56, 1, 9,17,25,33,41,49,57,
 		 2,10,18,26,34,42,50,58, 3,11,19,27,35,43,51,59,
 		 4,12,20,28,36,44,52,60, 5,13,21,29,37,45,53,61,
 		 6,14,22,30,38,46,54,62, 7,15,23,31,39,47,55,63
	},
	{ /* SCAN_ROW88 :
	   * scan along the rows in 8x8 blocks
	   */ 
 		 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
 		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
 		32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
 		48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
	},
	 { /* SCAN_ZIGZAG44 :
		* scan along the zig zag order in 4x4 blocks, starting from 0 and 1
		*/ 
 		 0, 1, 4, 8, 5, 2, 3, 6, 9,12,13,10, 7,11,14,15,
 		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 		 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	},
};
	

/* 6 tables of two tables (Inter/Intra) with 64 entries each */
const U8 au8BaseTables[NUM_QUANT_TABLES][2][64] = { 
	/* Q_NB_SL88: ynbsl88.qnt */
{ {19, 29, 31, 35, 37, 39, 41, 45,
   29, 31, 33, 35, 37, 39, 43, 47,
   31, 33, 35, 36, 38, 41, 45, 49,
   35, 35, 36, 37, 39, 43, 47, 51,
   37, 37, 38, 39, 41, 45, 49, 53,
   39, 39, 41, 43, 45, 47, 51, 55,
   41, 43, 45, 47, 49, 51, 53, 57,
   45, 47, 49, 51, 53, 55, 57, 59},

  { 13, 23, 27, 33, 35, 37, 39, 45,
 	23, 25, 31, 33, 35, 39, 43, 53,
 	27, 31, 31, 34, 37, 42, 51, 57,
 	33, 33, 34, 37, 41, 49, 54, 61,
 	35, 35, 37, 41, 47, 51, 57, 71,
 	37, 39, 42, 49, 51, 55, 67, 83,
 	39, 43, 51, 54, 57, 67, 77,101,
	45, 53, 57, 61, 71, 83,101,127}
},
/* Q_B0_SL88: yb0sl88.qnt */
{ {19, 29, 31, 35, 37, 39, 41, 45,
   29, 31, 33, 35, 37, 39, 43, 47,
   31, 33, 35, 36, 38, 41, 45, 49,
   35, 35, 36, 37, 39, 43, 47, 51,
   37, 37, 38, 39, 41, 45, 49, 53,
   39, 39, 41, 43, 45, 47, 51, 55,
   41, 43, 45, 47, 49, 51, 53, 57,
   45, 47, 49, 51, 53, 55, 57, 59},

  {19, 29, 31, 35, 37, 39, 41, 45,
   29, 31, 33, 35, 37, 39, 43, 47,
   31, 33, 35, 36, 38, 41, 45, 49,
   35, 35, 36, 37, 39, 43, 47, 51,
   37, 37, 38, 39, 41, 45, 49, 53,
   39, 39, 41, 43, 45, 47, 51, 55,
   41, 43, 45, 47, 49, 51, 53, 57,
   45, 47, 49, 51, 53, 55, 57, 59}

},

/* Q_B1_SL18: yb1sl18.qnt */
{ 

  { 39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97},

  { 39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97,
	39,  85, 121, 106, 111,  97, 107,  97}
},
/* Q_B2_SL81: yb2sl81.qnt */
{ 

  {  39,  39,  39,  39,  39,  39,  39,  39,
	 85,  85,  85,  85,  85,  85,  85,  85,
	121, 121, 121, 121, 121, 121, 121, 121,
	106, 106, 106, 106, 106, 106, 106, 106,
	111, 111, 111, 111, 111, 111, 111, 111,
	 97,  97,  97,  97,  97,  97,  97,  97,
	107, 107, 107, 107, 107, 107, 107, 107,
	 97,  97,  97,  97,  97,  97,  97,  97},

  {  39,  39,  39,  39,  39,  39,  39,  39,
	 85,  85,  85,  85,  85,  85,  85,  85,
	121, 121, 121, 121, 121, 121, 121, 121,
	106, 106, 106, 106, 106, 106, 106, 106,
	111, 111, 111, 111, 111, 111, 111, 111,
	 97,  97,  97,  97,  97,  97,  97,  97,
	107, 107, 107, 107, 107, 107, 107, 107,
	 97,  97,  97,  97,  97,  97,  97,  97}

},
/* Q_B3_88: yb3none.qnt */

{ { 47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47},

  { 47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47,
    47,  47,  47,  47,  47,  47,  47,  47}
},
/* Q_NB_SL44: uvsl44.qnt */
{ {15, 31, 37, 41,
   31, 37, 41, 43,
   37, 41, 43, 47,
   41, 43, 47, 51,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},

  {15, 31, 37, 41,
   31, 37, 41, 47,
   37, 41, 47, 61,
   41, 47, 61, 73,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
}

};

/* The tables below are in fixed point, the units are tenths */
const au16QuantScale[NUM_QUANT_TABLES][2][24] = { 
{
	/* Scale array for Q_NB_SL88 inter */
	{  11,  17,  19,  20,  21,  22,  24,  26,  27,  29,  32,  34, 
	   35,  37,  40,  42,  46,  50,  53,  57,  61,  65,  68,  74 }, 

	/* Scale array for Q_NB_SL88 intra */
	{  11,  14,  16,  18,  20,  22,  23,  24,  26,  28,  30,  32, 
	   34,  36,  39,  40,  42,  45,  47,  49,  52,  55,  57,  60 }, 
},
{
	/* Scale array for Q_B0_SL88 inter */
	{   7,  20,  22,  24,  27,  30,  34,  37,  41, 45, 49,  53, 
	   58,  63,  68,  74,  80,  86,  92, 99, 106, 113, 120, 126 }, 

	/* Scale array for Q_B0_SL88 intra */
	{   1,  16,  18,  20,  22,  24,  27,  30,  34,  37,  40, 44, 
		48,  52,  56,  61,  66,  71,  76,  82, 88,  94,  101, 108}, 
},
{
	/* Scale array for Q_B1_SL18 inter */
	{  21,  37,  40,  45,  48,  52,  58,  61,  66, 72,   76,  81,
	   86,  91,  96,  101, 107, 112, 118, 124, 130, 136, 143, 151 }, 

	/* Scale array for Q_B1_SL18 intra */
	{  19,  34,  39,  42,  45,  51,  54,  60,  65, 69,  73,  78, 
	   83,  88,  93,  99, 105, 111, 117, 124, 130, 136, 142,  149 }, 
},
{
	/* Scale array for Q_B2_SL81 inter */
	{  19,  31,  32,  34,  37,  40,  43,  45,  48,  51, 54,  57,
	   60,  63,  66,  69,  72,  75,  78,  82,  86,  90, 94,  98 }, 

	/* Scale array for Q_B2_SL81 intra */
	{  19,  31,  33,  36,  39,  41,  45,  47,  52, 55,  58,  61,
	   64,  68,  72,  76,  79,  82,  86,  90,  94,  98, 102, 107 }, 
},
{
	/* Scale array for Q_B3_NONE88 inter */
	{  60,  82,  88,  93,  99, 104, 104, 109, 115, 120, 124, 128,
	  132, 137, 142, 147, 152, 157, 163, 169, 173, 177, 181, 186 }, 

	/* Scale array for Q_B3_NONE88 intra */
	{  49,  66,  71,  71,  77,  82,  88,  88,  93,  99, 103, 107, 
	   111, 115, 120, 124, 128, 132, 137, 142, 147, 152, 157, 164 }, 
},
{
	/*  Scale array for Q_NB_SL44 inter */
	{  11,  13,  13,  14,  17,  17,  18,  19,  20,  21,  22,  23, 
	   24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35 }, 
	/* Scale array for Q_NB_SL44 intra */
	{   1,  11,  11,  13,  13,  13,  14,  15,  16,  17,  19,  20,
	   21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32 }, 

}
};


/*
 * Initialize a set of the quantization tables.  Given a Base and Scale,
 * those specify a set of quantization tables (all that's needed for 
 * inter/intra, each quant level, and each element) that is stored in
 * au16Quant[2][24][64].
 * u8Set is a number from 0x1 to 0x06.
*/
void InitQuantTables(U8 u8Set, U16 au16Quant[2][24][64]) {

	I32 i, j, k, q; 

#ifdef DEBUG
	if (u8Set > NUM_QUANT_TABLES) {
		HivePrintString(
	"InitQuantTables called with an invalid set---not initializing tables.\n");
		return;
	}
#endif 

	if (u8Set < 5) { /* Y plane set? */
		for (i = 0; i < 2; i++) {			/* For Inter/Intra */
			for (j=0; j<24; j++) {
				for (k = 0; k < 64; k++) {	/* Set each element */
					q = (au8BaseTables[u8Set][i][k] *
					     au16QuantScale[u8Set][i][j]) >> 8;
					if (q < 1)   q = 1;
					if (q > 255) q = 255;
					au16Quant[i][j][k] = (U16) q;
				}
			}
		}
	} else { /* UV plane set (6) */
		for (i = 0; i < 2; i++) {			/* For Inter/Intra */
			for (j=0; j<24; j++) {
				for (k = 0; k < 16; k++) {	/* Set each element */
					q = (au8BaseTables[u8Set][i][k] *
					     au16QuantScale[u8Set][i][j]) >> 8;
					if (q < 1)   q = 1;
					if (q > 255) q = 255;
					au16Quant[i][j][k] = (U16) q;
				} /* for k */
			} /* for j */
		} /* for i */
	} /* end if Y plane set else UV plane set */
}
