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

/*************************************************************************
 *	DYNCLUT.C -- Builds dynamic CLUT tables for Active Palette			 *
 *		ComputeDynamicClut	prototype in yrgb8.h						 *
 *																		 *
 *************************************************************************/
#include <stdlib.h> /* srand, rand */
#include "datatype.h"
#include "pia_main.h"
#include "yrgb8.h"

/*
 * lsqrt() returns the integer square root of its argument. Argument
 * range is 0..((65536*4)-1). Note that the returned value is the
 * same as you would get with (U32)sqrt((double)dist), ie, any fractional
 * part is truncated. The argument range can be increased by increasing
 * the initial value of max.
 */

static U32
lsqrt(U32 dist)
{
	U32 min =   0;
	U32 max = 512;	/* lqsrt(65536*4) */
	U32 try;

	while ((max - min) > 1)
	{
		try = (min + max) / 2;

		if ((try * try) > dist) {
			max = try;
		} else {
			min = try;
		}
	}

	return min;
}

/*************************************************************************
 * ComputeDynamicClut -- compute the tables for a specified ActivePalette
 * Returns: PIA_S_OK if everything is successful
 *			gpu8ClutTables contains 4 contiguous entries in memory:
 *
 *			ClutTable: 	65536 one-byte entries
 *						Each entry is the closest palette entry, as indexed
 *						by a 16-bit value: yyyyyyyyvvvvuuuu, dithered
 *
 *			TableU:		256 4-byte entries
 *						Each entry is 0000uuuu:0000uuuu:0000uuuu:0000uuuu,
 *						each uuuu is a 4 bit dithered u value for the
 *						index, which is a u value in the range 16-240
 *
 *			TableV:		256 4-byte entries
 *						Same as TableU, except the values are arranged
 *						vvvv0000:vvvv0000:vvvv0000:vvvv0000. 
 *
 *			YClamping:	256 2-byte entries for clamping Y values
 *			
 *************************************************************************/

PIA_RETURN_STATUS ComputeDynamicClut()
{
	const I32 YSTEP=  16;
	const I32 YGAP =   1;
	const I32 UVSTEP= 16;


	/* parameters for generating the U and V dither magnitude and bias */
	const I32 MAG_PAL_SAMPLES = 32;	/* # random palette samples to check */
	const I32 BIAS_PAL_SAMPLES= 128;/* number of RGB samples to check */

	PTR_COLOR pCol;  		/* YUV values for RGB colors  */
	PTR_COLOR pColIndex;	/* Pointer to index into pCol */
	PU8  pu8T;
/*	U8   au8YSlice[YSIZ][MAX_COL];  pointer to reduce stack use */
	PU8  pu8YSlice;
	PU8  pu8Y;
	PI32 piYCnt;
	PU32 puDiff; 	
	PU32 puD;  		
	PU32 puDelta; 	
	PU32 puDe; 		
	I32 i,j,yseg,y,u,v,ind,mini,addr1,addr2,yo,uo,vo,ycount,yi;
	U32 uD, uMin;	/* since 3*256^2 > 64K */

    I32 Y, U, V;
    I32 U_0, U_1, U_2, U_3;
    I32 V_0, V_1, V_2, V_3;
       
    /* Umag and Vmag max is (128 * sqrt(3) * MAG_NUM_NEAREST) = ~1330 */
    I32 Umag, Vmag;
    /* dist max is 256*256*3 > 65K */
    U32 uDist;
    U32 uCloseDist[MAG_NUM_NEAREST];
    I32 palindex;
    I32 R, G, B;
    I32 k, p, tmp, iu, iv;
    /* Ubias and Vbias max is (128 * 4 * BIAS_PAL_SAMPLES) = 65536 */
    /* even the worst palette (all black except the reserved colors) */
    /* would not achieve this. */
    I32  Ubias, Vbias;
    U32  uUDither, uVDither;
    PU32 puTableU, puTableV;

	PU32 puSquares;

	if ((gpalActivePalette.u16NumberOfEntries < 1) ||
		(gpalActivePalette.u16NumberOfEntries > 256)) {
		return PIA_S_ERROR;
	}

	puSquares = HiveGlobalAllocPtr(sizeof(U32)*512, TRUE);
	if (!puSquares)
		return PIA_S_OUT_OF_MEMORY;
	for (i=-256; i<256; i++) 				
		puSquares[256+i] = (U32) ((I32)i*i);		

	/* Allocate the memory needed */
	pCol = (PTR_COLOR) HiveGlobalAllocPtr(
					sizeof(COLOR)*gpalActivePalette.u16NumberOfEntries, TRUE);
 	piYCnt  = (PI32) HiveGlobalAllocPtr(sizeof(I32)*YSIZ, TRUE);
	puDiff  = (PU32) HiveGlobalAllocPtr(sizeof(U32)*258, TRUE);
	puDelta = (PU32) HiveGlobalAllocPtr(sizeof(U32)*258, TRUE);
	pu8YSlice = (PU8) HiveGlobalAllocPtr(sizeof(U8)*YSIZ*MAX_COL, TRUE);

	if (!piYCnt || !puDiff || !puDelta || !pu8YSlice)
		return (PIA_S_OUT_OF_MEMORY);

	/* Initialize the variables */
	pColIndex = pCol;
    for (i = 0; i < gpalActivePalette.u16NumberOfEntries; i++, pColIndex++) {
    	B = gpalActivePalette.pbgrTable[i].u8B;
    	G = gpalActivePalette.pbgrTable[i].u8G;
    	R = gpalActivePalette.pbgrTable[i].u8R;
    	pColIndex->y = YFROM(R, G, B);
		pColIndex->u = UFROM(R, G, B);
 		pColIndex->v = VFROM(R, G, B);

/* 		pColIndex->v = VFROM(R, G, B); */ /* Original code */
/* R and G shouldn't *need* to be grouped together... but for some reason,
 * they aren't unless parenthized this way, and in floating-point math, it does
 * matter.  This compiler bug is fixed in MSVC 2.1 */
/*		pColIndex->v = (I32)((( 0.439 * R) + (-0.368 * G)) + (-0.071 * B) + 128.); */
    }


	for (i=0; i<gpalActivePalette.u16NumberOfEntries; i++) {
		yseg = pCol[i].y >> 4;
		pu8YSlice[yseg*MAX_COL + piYCnt[yseg]++] = (U8) i;
	}


	/* Do exhaustive search on all U,V points and a coarse grid in Y */
	for (u=0; u<256; u+=UVSTEP)	{
		for (v=0; v<256; v+=UVSTEP)	{
			ind = TBLIDX(0,u,v);
			pu8T = gpu8ClutTables+ind;
			for (y=0; y<256; y+=YSTEP) {
				uMin = 0x0FFFFFFF;
				for (i=0, pColIndex=pCol; 
					 i<gpalActivePalette.u16NumberOfEntries; 
					 i++, pColIndex++) {
					uD = (3*puSquares[256+y - pColIndex->y])>>1;
					if (uD > uMin) continue;				

					uD += puSquares[256+u - pColIndex->u];
					if (uD > uMin) continue;

					uD += puSquares[256+v - pColIndex->v];
					if (uD < uMin) {
						uMin = uD;
						mini = i;
					}
				}
				*pu8T = (U8) mini;
#ifdef USE_844
				pu8T += YSTEP<<8;
#else
				pu8T += YSTEP;
#endif /* USE_844 */
			}
		}
	}


	/*  Go through points not yet done, and search
	**  (1) The closest point to the prev and next Y in coarse grid
	**  (2) All the points in this Y slice
	**
	** Also, take advantage of the fact that we can do distance computation
	** incrementally.  Keep all N errors in an array, and update each
	** time we change Y.
	*/

	for (u=0; u<256; u+=UVSTEP)	{
		for (v=0; v<256; v+=UVSTEP)	{
			for (y=YGAP; y<256; y+=YSTEP) {
				yseg = y >> 4;
				ycount = piYCnt[yseg] + 2;
					/* +2 is 'cause we add 2 Y endpoints */

				pu8Y = &(pu8YSlice[yseg*MAX_COL]);
				
				addr1 = TBLIDX(yseg*16,u,v);
				pu8Y[ycount-2] = gpu8ClutTables[addr1];

				addr2 = TBLIDX((yseg+(yseg < (YSIZ-1)))*16,u,v);
				pu8Y[ycount-1] = gpu8ClutTables[addr2];

				puD  = puDiff;
				puDe = puDelta;
				for (i=0; i<ycount; i++, pu8Y++, puD++, puDe++) {
					j = *pu8Y; /* pu8YSlice[yseg][i]; */
					pColIndex = pCol+j;
					yo = pColIndex->y;
					uo = pColIndex->u;
					vo = pColIndex->v;
					*puD = 3*puSquares[256+y-yo] + 2*(puSquares[256+u-uo] + 
						   							  puSquares[256+v-vo]);
					*puDe = 3*(((y-yo)<<1) + 1);
				}

				ind = TBLIDX(y,u,v);
				pu8T = gpu8ClutTables+ind;
				for (yi=0; yi<YSTEP-1; yi += YGAP) {
					uMin = 0x0FFFFFFF;
					pu8Y = &(pu8YSlice[yseg*MAX_COL]);
					puD  = puDiff;
					puDe = puDelta;

					for (i=0; i<ycount; i++, pu8Y++, puD++, puDe++)
					{
						if (*puD < uMin)
						{
							uMin = *puD;
							mini = *pu8Y; /* pu8YSlice[yseg][i]; */
						}
						*puD += *puDe;
						*puDe += 6;
					}
					*pu8T = (U8) mini;
#ifdef USE_844
					pu8T += YGAP<<8;
#else
					pu8T++;
#endif /* USE_844 */
				}
			}
		}
	}


    /* now do U and V dither tables and shift lookup table*/
    /* NOTE: All Y, U, V values are 8 bits */

	Umag = Vmag = 0;
	Ubias = Vbias = 0;

	/* use srand(0) and rand() to generate a repeatable series of */
	/* pseudo-random numbers */
	srand((unsigned)1);
	
	for (p = 0; p < MAG_PAL_SAMPLES; ++p) {
	   for (i = 0; i < MAG_NUM_NEAREST; ++i) {
	      uCloseDist[i] = 0x7FFFL;
	   }
	    
	   palindex = RANDOM(235) + 10;	/* random palette index, unreserved colors */
	   pColIndex = &pCol[palindex];
	   Y = pColIndex->y;
	   U = pColIndex->u;
	   V = pColIndex->v;
	    
	   for (i = 0, pColIndex=pCol; i < 255; ++i, pColIndex++) {
	      if (i != palindex) {
	         uDist = puSquares[256+(Y - pColIndex->y)] +
	                 puSquares[256+(U - pColIndex->u)] +
	                 puSquares[256+(V - pColIndex->v)];
	       
	         /* keep a sorted list of the nearest MAG_NUM_NEAREST entries */
	         for (j = 0; j < MAG_NUM_NEAREST; ++j) {
	            if (uDist < uCloseDist[j]) {
	               /* insert new entry; shift others down */
	               for (k = (MAG_NUM_NEAREST-1); k > j; k--)
	                  uCloseDist[k] = uCloseDist[k-1];
	               uCloseDist[j] = uDist;
	               break; /* out of for j loop */
	            }
	         } /* for j */
	      } /* if i */
	   } /* for i */
	   
	   /* now calculate Umag as the average of (U - U[1-6]) */
	   /* calculate Vmag in the same way */
	   
	   for (i = 0; i < MAG_NUM_NEAREST; ++i)
	   {
	      /* there are (MAG_PAL_SAMPLES * MAG_NUM_NEAREST) lsqrt() */
	      /* calls in this method */
	      Umag += lsqrt(uCloseDist[i]);	/* (I32)sqrt((double)uCloseDist[i]); */
	   }
	} /* for p */

	Umag /= (MAG_NUM_NEAREST * MAG_PAL_SAMPLES);
	Vmag = Umag;
	
	for (p = 0; p < BIAS_PAL_SAMPLES; ++p) {

		/* now calculate the average bias (use random RGB points) */
		R = RANDOM(255);
		G = RANDOM(255);
		B = RANDOM(255);
	   
		Y = YFROM(R, G, B);
		U = UFROM(R, G, B);
		V = VFROM(R, G, B);
	   
		for (uD = 0; uD < 4; uD++)	{
			U_0 = U + (gaDither[uD].uUDither*Umag)/3;
			V_0 = V + (gaDither[uD].uVDither*Vmag)/3;
	      
			/* Clamp values */
			if (U_0 > 255) U_0 = 255;
			if (V_0 > 255) V_0 = 255;
					
			/* (Y, U_0, V_0) is the dithered YUV for the RGB point */
			/* colptr points to the closest palette entry to the dithered */
			/* RGB */
			pColIndex = &pCol[gpu8ClutTables[TBLIDX(Y, U_0, V_0)]]; 
      
			Ubias += (U - pColIndex->u);
			Vbias += (V - pColIndex->v);
		}
	} /* for p */
	
	Ubias = (Ubias+BIAS_PAL_SAMPLES*2)/(BIAS_PAL_SAMPLES * 4);
	Vbias = (Vbias+BIAS_PAL_SAMPLES*2)/(BIAS_PAL_SAMPLES * 4);

    U_0 = (2*(I32)Umag/3); V_0 = (1*(I32)Vmag/3);
    U_1 = (1*(I32)Umag/3); V_1 = (2*(I32)Vmag/3);
    U_2 = (0*(I32)Umag/3); V_2 = (3*(I32)Vmag/3);
    U_3 = (3*(I32)Umag/3); V_3 = (0*(I32)Vmag/3);

#if defined USE_844
    puTableU = (PU32)&gpu8ClutTables[CLUT_TABLE_SIZE];
#else
    puTableU = (PU32)&gpu8ClutTables[16384];
#endif /* USE_844 */
    puTableV = puTableU + (TABLE_U_SIZE/sizeof(*puTableU));
       
    iu = Ubias /* + (UVSTEP>>1) */;
    iv = Vbias /* + (UVSTEP>>1) */;

    for (i = 0; i < 256; i++, iu++, iv++) {
#if defined USE_844 || defined USE_448
		/* dither: vvvv0000, 0000uuuu */
		tmp = iu + U_0; uUDither  = CLAMP8(tmp); uUDither <<= 8;
		tmp = iu + U_1; uUDither |= CLAMP8(tmp); uUDither <<= 8;
		tmp = iu      ; uUDither |= CLAMP8(tmp); uUDither <<= 8; /* U_2 == 0 */
		tmp = iu + U_3; uUDither |= CLAMP8(tmp);
          
		tmp = iv + V_0; uVDither  = CLAMP8(tmp); uVDither <<= 8;
		tmp = iv + V_1; uVDither |= CLAMP8(tmp); uVDither <<= 8;
		tmp = iv + V_2; uVDither |= CLAMP8(tmp); uVDither <<= 8;
		tmp = iv      ; uVDither |= CLAMP8(tmp);                /* V_3 == 0 */
#if defined USE_448
		*puTableU++ = (uUDither << 0) & 0xF0F0F0F0L;
		*puTableV++ = (uVDither >> 4) & 0x0F0F0F0FL;
#else
		*puTableU++ = (uUDither >> 4) & 0x0F0F0F0FL;
		*puTableV++ = (uVDither << 0) & 0xF0F0F0F0L;
#endif /* USE_448 */
#else
		/* dither: 00000uuu, 00vvv000 */
		tmp = iu + U_0; uUDither  = CLAMP8(tmp); uUDither <<= 8;
		tmp = iu + U_1; uUDither |= CLAMP8(tmp); uUDither <<= 8;
		tmp = iu      ; uUDither |= CLAMP8(tmp); uUDither <<= 8; /* U_2 == 0 */
		tmp = iu + U_3; uUDither |= CLAMP8(tmp);
		*puTableU++ = (uUDither >> 4) & 0x07070707L;
          
		tmp = iv + V_0; uVDither  = CLAMP8(tmp); uVDither <<= 8;
		tmp = iv + V_1; uVDither |= CLAMP8(tmp); uVDither <<= 8;
		tmp = iv + V_2; uVDither |= CLAMP8(tmp); uVDither <<= 8;
		tmp = iv      ; uVDither |= CLAMP8(tmp);                /* V_3 == 0 */
		*puTableV++ = (uVDither >> 1) & 0x38383838L;
#endif /* USE_844 */
    }

	/* Entries 0-16 of TableU and V should be the same, as should entries
	 * 240-255.  Force them. This is because the input pels can overshoot
	 * the expected CCIR601 ranges. */
#if defined USE_844
    puTableU = (PU32)&gpu8ClutTables[CLUT_TABLE_SIZE];
#else
    puTableU = (PU32)&gpu8ClutTables[16384];
#endif /* USE_844 */
    puTableV = puTableU + (TABLE_U_SIZE/sizeof(*puTableU));
	for (i=0; i<16; i++) {
		puTableU[i] = puTableU[16];
		puTableV[i] = puTableV[16];
	}
	for (i=241; i<256; i++) {
		puTableU[i] = puTableU[240];
		puTableV[i] = puTableV[240];
	}

#if defined USE_844
	/* likewise, build the Y clamping into the ClutTable.
	 * Since Y dithering ranges from MIN_Y_DITHER to MAX_Y_DITHER,
	 * we fill in these entries manually.
	 */
	for (u = 0; u < 256; u += UVSTEP) {
		for (v = 0; v < 256; v += UVSTEP) {
			pu8T = gpu8ClutTables + TBLIDX(16, u, v);
			mini = *pu8T;

			for (y = MIN_Y_DITHER; y < 16; ++y) {
				pu8T -= (1 << 8);
				*pu8T = (U8)mini;
			}

			pu8T = gpu8ClutTables + TBLIDX(235, u, v);
			mini = *pu8T;

			for (y = 236; y < 256+MAX_Y_DITHER; ++y) {
				pu8T += (1 << 8);
				*pu8T = (U8)mini;
			}
		} /* for v... */
	} /* for u... */
#else
ERROR - The active palette color converter is only set up for USE_844 currently.
        To use 448, the Y pels must be clamped before dithering - expensive on P5.
#endif /* USE_844 */


	/* free memory allocated */
	HiveGlobalFreePtr(pCol);
	HiveGlobalFreePtr(piYCnt);
	HiveGlobalFreePtr(puDiff);
	HiveGlobalFreePtr(puDelta);
	HiveGlobalFreePtr(pu8YSlice);
	HiveGlobalFreePtr(puSquares);

	return (PIA_S_OK);
}
