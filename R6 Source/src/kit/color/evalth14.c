/*
 * @(#)evalth14.c	1.10 97/12/22
*
*	4 input evaluation functions using tetrahedral interpolation
*
*	(c) Copyright 1997 Eastman Kodak Co.
*	All rights reserved.
*
*	Author:			George Pawle
*
*	Create Date:	12/22/96
*
*	Note: This implementation supports number of inputs equal to 4 only.
*	It supports number of outputs equal to 1, 2, 3, 4.
*
*******************************************************************************/
/*********************************************************************
*    COPYRIGHT (c) 1996-1997 Eastman Kodak Company
*    As  an  unpublished  work pursuant to Title 17 of the United
*    States Code.  All rights reserved.
*********************************************************************
*/

#include "kcpcache.h"

/******************************************************************************/

/**************************************************************
 * evalTh1i4o1 ---- 8 BIT
 *  Evaluation routine for evaluating 4 channel to 1 channel.
 */

void
	evalTh1i4o1d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8;
KpInt32_t	outStride0 = outStride[0]; 
KpUInt8_t	prevRes0 = 0;

	TH1_4D_INITD8

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA8
		
		if (thisColor != prevColor) {

			TH1_4D_FINDTETRAD8
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];
		}

		TH1_STORE_DATA8(0);		/* store the data */
	}
}

/**************************************************************
 * evalTh1i4o1 ---- 16 bit
 *  Evaluation routine for evaluating 4 channel to 1 channel.
 */

void
	evalTh1i4o1d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt16_p	outp0 = outp[0].p16;
KpInt32_t	outStride0 = outStride[0]; 
KpUInt16_t	prevRes0 = 0;

	TH1_4D_INITD16

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA16
		
		if ((thisColor1 != prevColor1) || (thisColor2 != prevColor2)) {

			TH1_4D_FINDTETRAD16
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];
		}

		TH1_STORE_DATA16(0);		/* store the data */
	}
}

/**************************************************************
 * evalTh1i4o2 --- 8 bit
 **************************************************************/

void
	evalTh1i4o2d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0;

	TH1_4D_INITD8

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA8
		
		if (thisColor != prevColor) {

			TH1_4D_FINDTETRAD8
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
	}
}

/**************************************************************
 * evalTh1i4o2  ----  16 bit
 **************************************************************/

void
	evalTh1i4o2d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0;

	TH1_4D_INITD16

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA16
		
		if ((thisColor1 != prevColor1) || (thisColor2 != prevColor2)) {

			TH1_4D_FINDTETRAD16
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
	}
}

/**************************************************************
 * evalTh1i4o3   -----   8 bit
 **************************************************************/
 
void
	evalTh1i4o3d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0;

	TH1_4D_INITD8

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA8
		
		if (thisColor != prevColor) {

			TH1_4D_FINDTETRAD8
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_4D_TETRAINTERP_AND_OLUT(2)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
	}
}


#if defined (KPMAC)

typedef union QDBuff_s {
	KpUInt8_t	cbuf[4];
	KpUInt32_t	lword;
} QDBuff_t;

/**************************************************************
 * evalTh1i4o3QD
 **************************************************************/
void
	evalTh1i4o3QD (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt32_p	outpL;
QDBuff_t	QDoBuf;
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0;

	TH1_4D_INITD8

	if (outStride) {}

	outpL = (KpUInt32_p)(outp[0].p8 -1);

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA8
		
		if (thisColor != prevColor) {

			TH1_4D_FINDTETRAD8
					
			TH1_4D_TETRAINTERP				/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_4D_TETRAINTERP_AND_OLUT(2)
		}

		QDoBuf.lword = *outpL;				/* preserve alpha channel */

		QDoBuf.cbuf[1] = prevRes0;			/* use result from previous color evaluation */
		QDoBuf.cbuf[2] = prevRes1;
		QDoBuf.cbuf[3] = prevRes2;
		
		*outpL++ = QDoBuf.lword;
	}
}

#endif 	/* if defined KPMAC */


/**************************************************************
 * evalTh1i4o3   ---- 16 bit
 **************************************************************/
 
void
	evalTh1i4o3d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16, outp2 = outp[2].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0;

	TH1_4D_INITD16

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA16
		
		if ((thisColor1 != prevColor1) || (thisColor2 != prevColor2)) {

			TH1_4D_FINDTETRAD16
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_4D_TETRAINTERP_AND_OLUT(2)
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
		TH1_STORE_DATA16(2);
	}
}


/**************************************************************
 * evalTh1i4o4   ----  8 bit
 *  Evaluation routine for evaluating 4 channel to 4 channels.
 **************************************************************/
void
	evalTh1i4o4d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8, outp3 = outp[3].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0;

	TH1_4D_INITD8

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA8
		
		if (thisColor != prevColor) {

			TH1_4D_FINDTETRAD8
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_4D_TETRAINTERP_AND_OLUT(2)
			TH1_4D_TETRAINTERP_AND_OLUT(3)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
		TH1_STORE_DATA8(3);
	}
}


/**************************************************************
 * evalTh1i4oB32
 **************************************************************/

void
	evalTh1iB32oB32 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8;
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0;

	TH1_4D_INITD8

	if (inStride || outStride) {}

	thisColor = ~inp0[3];		/* make sure cache is not valid */

	for (i = n; i > 0; i--) {
		data0 = *inp0++; 					/* get channel 0 input data */
		data1 = *inp0++; 					/* get channel 1 input data */
		data2 = *inp0++; 					/* get channel 2 input data */
		data3 = *inp0++; 					/* get channel 3 input data */
		
		thisColor = (data0 << 24) | (data1 << 16) | (data2 << 8) | (data3);	/* calc this color */
		
		if (thisColor != prevColor) {

			TH1_4D_FINDTETRAD8
					
			TH1_4D_TETRAINTERP				/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_4D_TETRAINTERP_AND_OLUT(2)
			TH1_4D_TETRAINTERP_AND_OLUT(3)
		}

		*outp0++ = prevRes0;				/* use result from previous color evaluation */
		*outp0++ = prevRes1;
		*outp0++ = prevRes2;
		*outp0++ = prevRes3;
	}
}

/**************************************************************
 * evalTh1i4oL32
 *  Evaluation routine for evaluating 4 channel to 4 channels,
 **************************************************************/
void
	evalTh1iL32oL32 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp3 = outp[3].p8;
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0;

	TH1_4D_INITD8

	if (inStride || outStride) {}
	
	thisColor = ~inp0[0];		/* make sure cache is not valid */

	for (i = n; i > 0; i--) {
		data3 = inp3[0]; 					/* get channel 3 input data */
		data2 = inp3[1]; 					/* get channel 2 input data */
		data1 = inp3[2]; 					/* get channel 1 input data */
		data0 = inp3[3]; 					/* get channel 0 input data */

		inp3 += 4;

		thisColor = (data0 << 24) | (data1 << 16) | (data2 << 8) | (data3);	/* calc this color */
		
		if (thisColor != prevColor) {

			TH1_4D_FINDTETRAD8
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_4D_TETRAINTERP_AND_OLUT(2)
			TH1_4D_TETRAINTERP_AND_OLUT(3)
		}

		outp3[0] = prevRes3;		/* use result from previous color evaluation */
		outp3[1] = prevRes2;
		outp3[2] = prevRes1;
		outp3[3] = prevRes0;

		outp3 += 4;
	}

}

/**************************************************************
 * evalTh1i4o4   ---- 16 bit
 *  Evaluation routine for evaluating 4 channel to 4 channels.
 **************************************************************/
void
	evalTh1i4o4d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16, outp2 = outp[2].p16, outp3 = outp[3].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0;

	TH1_4D_INITD16

	for (i = n; i > 0; i--) {
		TH1_4D_GETDATA16
		
		if ((thisColor1 != prevColor1) || (thisColor2 != prevColor2)) {

			TH1_4D_FINDTETRAD16
					
			TH1_4D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_4D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_4D_TETRAINTERP_AND_OLUT(2)
			TH1_4D_TETRAINTERP_AND_OLUT(3)
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
		TH1_STORE_DATA16(2);
		TH1_STORE_DATA16(3);
	}
}

