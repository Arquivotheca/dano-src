/*
 * @(#)evalth1g.c	1.11 97/12/22
*	(c) Copyright 1996-1997 Eastman Kodak Company.
*	All rights reserved.
*
*	tetrahedral evaluation functions
*
*	Author:			George Pawle
*
*	Create Date:	12/20/96
*
*******************************************************************************/

#include "kcpcache.h"
#include "interp.h"
#include "attrib.h"

#if defined (KCP_EVAL_TH1)

#define MAX_8_BITS (0xff)
#define MAX_12_BITS (0xfff)
#define MAX_16_BITS (0xffff)

#define FRAC_BITS_8_TO_12 (12)
#define FRAC_ONE_8_TO_12 (1 << FRAC_BITS_8_TO_12)
#define FRAC_HALF_8_TO_12 (FRAC_ONE_8_TO_12 >> 1)
#define MAX_8_TO_12 (MAX_8_BITS << FRAC_BITS_8_TO_12)
#define FRAC_MASK_8_TO_12 (FRAC_ONE_8_TO_12 -1)
#define FRAC_ONE_8_TO_12 (1 << FRAC_BITS_8_TO_12)

#define FRAC_BITS_8_TO_16 (8)
#define FRAC_ONE_8_TO_16 (1 << FRAC_BITS_8_TO_16)
#define FRAC_HALF_8_TO_16 (FRAC_ONE_8_TO_16 >> 1)
#define MAX_8_TO_16 (MAX_8_BITS << FRAC_BITS_8_TO_16)
#define FRAC_MASK_8_TO_16 (FRAC_ONE_8_TO_16 -1)
#define FRAC_ONE_8_TO_16 (1 << FRAC_BITS_8_TO_16)

#define FRAC_BITS_12_TO_16 (4)
#define FRAC_ONE_12_TO_16 (1 << FRAC_BITS_12_TO_16)
#define FRAC_HALF_12_TO_16 (FRAC_ONE_12_TO_16 >> 1)
#define MAX_12_TO_16 (MAX_12_BITS << FRAC_BITS_12_TO_16)
#define FRAC_MASK_12_TO_16 (FRAC_ONE_12_TO_16 -1)
#define FRAC_ONE_12_TO_16 (1 << FRAC_BITS_12_TO_16)

void cpeval (fut_ptr_t, KpInt32_t, KpInt32_t, fut_generic_ptr_t FAR*, fut_generic_ptr_t, KpInt32_t);

static KpInt32_t BoseSort1[] = {0};
static KpInt32_t BoseSort2[] = {1, 0, 1};
static KpInt32_t BoseSort3[] = {3, 0, 1, 0, 2, 1, 2};
static KpInt32_t BoseSort4[] = {5, 0, 1, 2, 3, 0, 2, 1, 3, 1, 2};
static KpInt32_t BoseSort5[] = {9, 0, 1, 2, 3, 0, 2, 1, 3, 1, 2, 0, 4, 1, 4, 2, 4, 3, 4};
static KpInt32_t BoseSort6[] = {12, 0, 1, 2, 3, 0, 2, 1, 3, 1, 2, 4, 5, 0, 4, 1, 5, 1, 4, 2, 4, 3, 5, 3, 4};
static KpInt32_t BoseSort7[] = {16, 0, 1, 2, 3, 0, 2, 1, 3, 1, 2, 4, 5, 4, 6, 5, 6, 0, 4, 1, 5, 1, 4, 2, 6, 3, 6, 2, 4, 3, 5, 3, 4};
static KpInt32_t BoseSort8[] = {19, 0, 1, 2, 3, 0, 2, 1, 3, 1, 2, 4, 5, 6, 7, 4, 6, 5, 7, 5, 6, 0, 4, 1, 5, 1, 4, 2, 6, 3, 7, 3, 6, 2, 4, 3, 5, 3, 4};

/******************************************************************************/

void
	evalTh1gen (	imagePtr_p	inp,
				KpInt32_p	inStride,
				imagePtr_p	outp,
				KpInt32_p	outStride,
				KpInt32_t	n,
				th1Cache_p	th1Cache)
{
imagePtr_t	inData[FUT_NICHAN], outData[FUT_NOCHAN];
KpUInt32_t	imask, omask, chanIMask, data[FUT_NICHAN], sPosition;
KpInt32_t	i, i1, iChan, numOutputs, numInputs, indexFrac, cell, index, tResult;
KpInt32_t	oTblData, interpolant, next, dimSize, i2, numCompares, index1, index2, temp;
KpInt32_t	hVert[FUT_NICHAN], hFrac[FUT_NICHAN], previousVertex, thisVertex;
KpInt32_p	BoseSortP;
KpUInt8_p	vertexP;
fut_ptr_t			fut;
fut_itbl_ptr_t		theITbl, FAR* iTblsP;
fut_chan_ptr_t		chan[FUT_NOCHAN], theChan;
fut_gtbl_ptr_t		theGTbl;
fut_gtbldat_ptr_t	gTbl[FUT_NOCHAN];
fut_otbldat_ptr_t	oTbl[FUT_NOCHAN];				
	
	for (i1 = 0; i1 < FUT_NICHAN; i1++) {
		inData[i1].p8 = inp[i1].p8;			/* copy addresses - do not update supplied lists! */
	}

	imask = (KpInt32_t)FUT_IMASK(th1Cache->iomask);
	omask = (KpInt32_t)FUT_OMASK(th1Cache->iomask);
	fut = th1Cache->cachedFut;

	for (i = 0, numOutputs = 0; i < FUT_NOCHAN; i++) {
		if (omask & FUT_BIT(i)) {
			theChan = fut->chan[i];
			chan[numOutputs] = theChan;
			gTbl[numOutputs] = theChan->gtbl->tbl;	/* get the grid and output tables */
			oTbl[numOutputs] = theChan->otbl->tbl;
			outData[numOutputs].p8 = outp[numOutputs].p8;	/* copy addresses - do not update supplied lists! */
			numOutputs++;
		}
	}

	for (i = n; i > 0; i--) {
		for (i1 = 0; i1 < FUT_NICHAN; i1++) {
			if (imask & FUT_BIT(i1)) {
				if (th1Cache->dataSizeI == KCM_UBYTE) {
					data[i1] = *inData[i1].p8; 		/* get 8 bit input data */
				}
				else {
					data[i1] = *inData[i1].p16; 	/* get 12/16 bit input data */
				}
				
				inData[i1].p8 += inStride[i1];
			}
		}

		for (iChan = 0; iChan < numOutputs; iChan++) {	/* evaluate each channel */
			theChan = chan[iChan];
			iTblsP = theChan->itbl;
			theGTbl = theChan->gtbl;

			chanIMask = theChan->imask;			/* inputs needed for this chan */
			cell = 0;
			
			for (i1 = 0, numInputs = 0; chanIMask != 0; chanIMask >>= 1, i1++) {
				if ((chanIMask & 1) != 0) {
					theITbl = iTblsP[i1];

					switch (th1Cache->dataSizeI) {
					case KCM_UBYTE:
						indexFrac = theITbl->tbl[data[i1]];				/* pass input data through input tables */
						break;

					case KCM_USHORT_12:
						if (theITbl->tblFlag != 0) {
							indexFrac = theITbl->tbl2[data[i1]];		/* pass input data through input tables */
						}
						else {	/* no 12 bit iTbl, interpolate 8 to 12 */
							sPosition = (data[i1] * MAX_8_TO_12) / MAX_12_BITS;
							index = sPosition >> FRAC_BITS_8_TO_12;
							interpolant = sPosition & FRAC_MASK_8_TO_12;
							
							indexFrac = theITbl->tbl[index];		/* pass input data through input tables */
							next = theITbl->tbl[index +1];			/* next table data */
					        indexFrac += (((next - indexFrac) * interpolant) + FRAC_HALF_8_TO_12) >> FRAC_BITS_8_TO_12;
						}
						break;

					case KCM_USHORT:
						if (theITbl->tblFlag != 0) {	/* 12 bit iTbl, interpolate 12 to 16 */
							sPosition = (data[i1] * MAX_12_TO_16) / MAX_16_BITS;
							index = sPosition >> FRAC_BITS_12_TO_16;
							interpolant = sPosition & FRAC_MASK_12_TO_16;
							
							indexFrac = theITbl->tbl2[index];		/* pass input data through input tables */
							next = theITbl->tbl2[index +1];			/* next table data */
					        indexFrac += (((next - indexFrac) * interpolant) + FRAC_HALF_12_TO_16) >> FRAC_BITS_12_TO_16;
						}
						else {	/* no 12 bit iTbl, interpolate 8 to 16 */
							sPosition = (data[i1] * MAX_8_TO_16) / MAX_16_BITS;
							index = sPosition >> FRAC_BITS_8_TO_16;
							interpolant = sPosition & FRAC_MASK_8_TO_16;
							
							indexFrac = theITbl->tbl[index];		/* pass input data through input tables */
							next = theITbl->tbl[index +1];			/* next table data */
					        indexFrac += (((next - indexFrac) * interpolant) + FRAC_HALF_8_TO_16) >> FRAC_BITS_8_TO_16;
						}
						break;
					}

					hFrac[numInputs] = indexFrac & ((1 << FUT_INP_FRACBITS) -1);	/* get fut fractional value */
					index = indexFrac >> FUT_INP_FRACBITS;	/* get fut index value */
					dimSize = theGTbl->size[numInputs];		/* size of this dimension */
					hVert[i1] = dimSize;					/* save for offset calcs */
					cell *= dimSize;						/* build cell index */
					cell += index;							/* add in this index */
					
					numInputs++;
				}
			}

			/* build offsets for each dimension */
			index = 2;
			for (i1 = numInputs-1; i1 >= 0; i1--) {
				dimSize = hVert[i1];
				hVert[i1] = index;
				index *= dimSize;
			}

			switch (numInputs) {	/* evaluate as needed */
			case 1:
				BoseSortP = BoseSort1;
				break;
				
			case 2:
				BoseSortP = BoseSort2;
				break;
				
			case 3:
				BoseSortP = BoseSort3;
				break;
				
			case 4:
				BoseSortP = BoseSort4;
				break;
				
			case 5:
				BoseSortP = BoseSort5;
				break;
				
			case 6:
				BoseSortP = BoseSort6;
				break;
				
			case 7:
				BoseSortP = BoseSort7;
				break;
				
			case 8:
				BoseSortP = BoseSort8;
				break;
				
			default:
				break;
				
			}

			/* find the hyperhedron in which the interpolation point is located */
			numCompares = *BoseSortP++;		/* first element is # of compares */
			
			for (i2 = 0; i2 < numCompares; i2++) {
				index1 = *BoseSortP++;
				index2 = *BoseSortP++;
				
				/* sort into largest to smallest based upon interpolants */
				temp = hFrac[index1];
				if (temp < hFrac[index2]) {
					hFrac[index1] = hFrac[index2];	/* swap interpolants */
					hFrac[index2] = temp;

					temp = hVert[index1];			/* swap vertices */
					hVert[index1] = hVert[index2];
					hVert[index2] = temp;
				}
			}
				
			/* hyperhedral interpolation */		
			vertexP = (KpUInt8_p)(gTbl[iChan] + cell);

			previousVertex = (KpInt32_t)*(ecGridData_p)(vertexP);
			tResult = previousVertex << 16;

			for (i1 = 0; i1 < numInputs; i1++) {
				vertexP += hVert[i1];
				thisVertex = (KpInt32_t)*(ecGridData_p)(vertexP);
				tResult += (hFrac[i1] * (thisVertex - previousVertex));
				previousVertex = thisVertex;
			}

			tResult += (1 << 15);		/* round */
			tResult >>= 16;				/* get integer part */

			if (oTbl[iChan] == NULL) {	/* apply to output table */
				oTblData = tResult;
			}
			else {
				oTblData = oTbl[iChan][tResult];
			}
			
			switch (th1Cache->dataSizeO) {
			case KCM_UBYTE:
				*outData[iChan].p8 = (KpUInt8_t) FUT_OTBL_NINT(oTblData);
				break;
				
			case KCM_USHORT_12:
				KCP_SCALE_OTBL_DATA(oTblData, KCP_MAX_12BIT)

				*outData[iChan].p16 = (KpUInt16_t) oTblData;
				break;
				
			case KCM_USHORT:
				KCP_SCALE_OTBL_DATA(oTblData, KCP_MAX_16BIT)

				*outData[iChan].p16 = (KpUInt16_t) oTblData;
				break;
			}
			
			outData[iChan].p8 += outStride[iChan];	/* next location */
		}
	}
}

void
	cpeval (fut_ptr_t fut, KpInt32_t evalMask, KpInt32_t dattype, fut_generic_ptr_t FAR* in, fut_generic_ptr_t out, KpInt32_t n)	/* use the CP interpolator */
{
KpInt32_t	stride8[] = {1, 1, 1, 1, 1, 1, 1, 1};
KpInt32_t	stride16[] = {2, 2, 2, 2, 2, 2, 2, 2};
KpInt32_p	theStride;
th1Cache_t	theCache;

	theCache.iomask = evalMask;
	theCache.cachedFut = fut;

	if (dattype == 0) {
		theCache.dataSizeI = KCM_UBYTE;
		theCache.dataSizeO = KCM_UBYTE;
		theStride = stride8;
	}
	else {
		theCache.dataSizeI = KCM_USHORT_12;
		theCache.dataSizeO = KCM_USHORT_12;
		theStride = stride16;
	}
	
	evalTh1gen ((imagePtr_p) in, theStride, (imagePtr_p)&out, theStride, n, &theCache);
}

#endif /* #if defined (KCP_EVAL_TH1) */

