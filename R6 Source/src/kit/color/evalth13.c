/*
 * @(#)evalth13.c	1.4 97/12/22
 *
*	(c) Copyright 1996-1997 Eastman Kodak Company.
*	All rights reserved.
*
*	tetrahedral evaluation functions
*
*	Author:			George Pawle
*
*	Create Date:	12/20/96
*
*	Note: This implementation supports number of inputs equal to 3 only.
*	It supports number of outputs equal to 1 through 8. 
*	Please refer to function evalGrid3() for an example.
*
*
*******************************************************************************/

#include "kcpcache.h"

#if defined (KCP_EVAL_TH1)

/******************************************************************************/

void
	evalTh1i3o1d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8;
KpInt32_t	outStride0 = outStride[0]; 
KpUInt8_t	prevRes0 = 0;

	TH1_3D_INITD8

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];
		}

		TH1_STORE_DATA8(0);		/* store the data */
	}
}


/******************************************************************************/

void
	evalTh1i3o1d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt16_p	outp0 = outp[0].p16;
KpInt32_t	outStride0 = outStride[0]; 
KpUInt16_t	prevRes0 = 0;

	TH1_3D_INITD16

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD16
		
		if ((ColorPart1 != prevPart1) || (data2 != prevPart2)) {

			TH1_3D_FINDTETRAD16
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];
		}

		TH1_STORE_DATA16(0);		/* store the data */
	}
}


/******************************************************************************/

void
	evalTh1i3o2d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0;

	TH1_3D_INITD8

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
	}
}


/******************************************************************************/

void
	evalTh1i3o2d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)

{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0;

	TH1_3D_INITD16

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD16
		
		if ((ColorPart1 != prevPart1) || (data2 != prevPart2)) {

			TH1_3D_FINDTETRAD16
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
	}
}


/******************************************************************************/

void
	evalTh1i3o3d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0;

	TH1_3D_INITD8

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
	}
}


/******************************************************************************/

void
	evalTh1iB24oB24 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8;
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0;

	TH1_3D_INITD8

	if (outStride) {}

	for (i = n; i > 0; i--) {
		data0 = *inp0++; 					/* get channel 0 input data */
		data1 = *inp0++; 					/* get channel 1 input data */
		data2 = *inp0++; 					/* get channel 2 input data */

		thisColor = (data0 << 16) | (data1 << 8) | (data2);	/* calc this color */

		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
		}

		*outp0++ = prevRes0;	/* use result from previous color evaluation */
		*outp0++ = prevRes1;
		*outp0++ = prevRes2;
	}
}


/******************************************************************************/

void
	evalTh1iL24oL24 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp2 = outp[2].p8;
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0;

	TH1_3D_INITD8

	if (outStride) {}
	
	for (i = n; i > 0; i--) {
		data2 = inp2[0]; 					/* get channel 2 input data */
		data1 = inp2[1]; 					/* get channel 1 input data */
		data0 = inp2[2]; 					/* get channel 0 input data */

		inp2 += 3;

		thisColor = (data0 << 16) | (data1 << 8) | (data2);	/* calc this color */
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
		}
		
		outp2[0] = (KpUInt8_t)(prevRes2);		/* use result from previous color evaluation */
		outp2[1] = (KpUInt8_t)(prevRes1);
		outp2[2] = (KpUInt8_t)(prevRes0);

		outp2 += 3;
	}
}


#if defined (KPMAC)
/**************************** Quick Draw Formats **********************************/

void
	evalTh1iQDoQD (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt32_p	inpL, outpL;
KpUInt32_t	alphaData, prevResult = 0;

	TH1_3D_INITD8

	if (inStride == outStride) {}
	
	inpL = (KpUInt32_p)(inp[0].p8 -1);
	outpL = (KpUInt32_p)(outp[0].p8 -1);

	for (i = n; i > 0; i--) {
		thisColor = *inpL++;
		alphaData = thisColor & 0xff000000;		/* get alpha channel input data */
		data0 = (thisColor >> 16) & 0xff;		/* get channel 0 input data */
		data1 = (thisColor >> 8) & 0xff;		/* get channel 1 input data */
		data2 = (thisColor) & 0xff;				/* get channel 2 input data */

		thisColor &= 0xffffff;					/* clear off alpha channel */

		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP					/* channel 0 */
			
			prevResult = (KpUInt32_t)outLut0[(0*4096)+tResult];

			baseP += sizeof (ecGridData_t);
			TH1_3D_TETRAINTERP					/* channel 1 */
			
			prevResult <<= 8;
			prevResult |= (KpUInt32_t)outLut0[(1*4096)+tResult];
			
			baseP += sizeof (ecGridData_t);
			TH1_3D_TETRAINTERP					/* channel 2 */
			
			prevResult <<= 8;
			prevResult |= (KpUInt32_t)outLut0[(2*4096)+tResult];
		}

		*outpL++ = prevResult | alphaData;
	}
}


/******************************************************************************/

void
	evalTh1iQDo3 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt32_p	inpL;
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8;
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2]; 

	TH1_3D_INITD8
		
	if (inStride) {}
	
	inpL = (KpUInt32_p)(inp[0].p8 -1);

	for (i = n; i > 0; i--) {
		thisColor = *inpL++;
		data0 = (thisColor >> 16) & 0xff;		/* get channel 0 input data */
		data1 = (thisColor >> 8) & 0xff;		/* get channel 1 input data */
		data2 = (thisColor) & 0xff;				/* get channel 2 input data */
		
		thisColor &= 0xffffff;					/* clear off alpha channel */

		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
	}
}


/******************************************************************************/

void
	evalTh1i3oQD (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt32_p	outpL;
KpUInt32_t	prevResult = 0; 

	TH1_3D_INITD8

	if (outStride) {}
	
	outpL = (KpUInt32_p)(outp[0].p8 -1);

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8

		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP					/* channel 0 */
			
			prevResult = (KpUInt32_t)outLut0[(0*4096)+tResult];
			prevResult |= 0xff00;				/* insert white into alpha channel */

			baseP += sizeof (ecGridData_t);
			TH1_3D_TETRAINTERP					/* channel 1 */
			
			prevResult <<= 8;
			prevResult |= (KpUInt32_t)outLut0[(1*4096)+tResult];
			
			baseP += sizeof (ecGridData_t);
			TH1_3D_TETRAINTERP					/* channel 2 */
			
			prevResult <<= 8;
			prevResult |= (KpUInt32_t)outLut0[(2*4096)+tResult];
		}

		*outpL++ = prevResult;
	}
}
#endif 	/* KPMAC */


/*********************************************************************************************/

void
	evalTh1i3o3d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)

{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16, outp2 = outp[2].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0;

	TH1_3D_INITD16

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD16
		
		if ((ColorPart1 != prevPart1) || (data2 != prevPart2)) {

			TH1_3D_FINDTETRAD16
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
		TH1_STORE_DATA16(2);
	}
}


/******************************************************************************/

void
	evalTh1i3o4d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8, outp3 = outp[3].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0;

	TH1_3D_INITD8

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
		TH1_STORE_DATA8(3);
	}
}


/******************************************************************************/

void
	evalTh1i3o4d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)

{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16, outp2 = outp[2].p16, outp3 = outp[3].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0;

	TH1_3D_INITD16

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD16
		
		if ((ColorPart1 != prevPart1) || (data2 != prevPart2)) {

			TH1_3D_FINDTETRAD16
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
		TH1_STORE_DATA16(2);
		TH1_STORE_DATA16(3);
	}
}


/******************************************************************************/

void
	evalTh1i3o5d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8, outp3 = outp[3].p8;
KpUInt8_p	outp4 = outp[4].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpInt32_t	outStride4 = outStride[4]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0, prevRes4 = 0;

	TH1_3D_INITD8

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
			TH1_3D_TETRAINTERP_AND_OLUT(4)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
		TH1_STORE_DATA8(3);
		TH1_STORE_DATA8(4);
	}
}


/******************************************************************************/

void
	evalTh1i3o5d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16, outp2 = outp[2].p16, outp3 = outp[3].p16;
KpUInt16_p	outp4 = outp[4].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpInt32_t	outStride4 = outStride[4]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0, prevRes4 = 0;

	TH1_3D_INITD16

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD16
		
		if ((ColorPart1 != prevPart1) || (data2 != prevPart2)) {

			TH1_3D_FINDTETRAD16
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
			TH1_3D_TETRAINTERP_AND_OLUT(4)
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
		TH1_STORE_DATA16(2);
		TH1_STORE_DATA16(3);
		TH1_STORE_DATA16(4);
	}
}


/******************************************************************************/

void
	evalTh1i3o6d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8, outp3 = outp[3].p8;
KpUInt8_p	outp4 = outp[4].p8, outp5 = outp[5].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpInt32_t	outStride4 = outStride[4], outStride5 = outStride[5]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0, prevRes4 = 0, prevRes5 = 0;

	TH1_3D_INITD8

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
			TH1_3D_TETRAINTERP_AND_OLUT(4)
			TH1_3D_TETRAINTERP_AND_OLUT(5)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
		TH1_STORE_DATA8(3);
		TH1_STORE_DATA8(4);
		TH1_STORE_DATA8(5);
	}
}


/******************************************************************************/

void
	evalTh1i3o6d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)

{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16, outp2 = outp[2].p16, outp3 = outp[3].p16;
KpUInt16_p	outp4 = outp[4].p16, outp5 = outp[5].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpInt32_t	outStride4 = outStride[4], outStride5 = outStride[5]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0, prevRes4 = 0, prevRes5 = 0;

	TH1_3D_INITD16

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD16
		
		if ((ColorPart1 != prevPart1) || (data2 != prevPart2)) {

			TH1_3D_FINDTETRAD16
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
			TH1_3D_TETRAINTERP_AND_OLUT(4)
			TH1_3D_TETRAINTERP_AND_OLUT(5)
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
		TH1_STORE_DATA16(2);
		TH1_STORE_DATA16(3);
		TH1_STORE_DATA16(4);
		TH1_STORE_DATA16(5);
	}
}

/******************************************************************************/

void
	evalTh1i3o7d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8, outp3 = outp[3].p8;
KpUInt8_p	outp4 = outp[4].p8, outp5 = outp[5].p8, outp6 = outp[6].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpInt32_t	outStride4 = outStride[4], outStride5 = outStride[5], outStride6 = outStride[6]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0, prevRes4 = 0, prevRes5 = 0, prevRes6 = 0;

	TH1_3D_INITD8

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
			TH1_3D_TETRAINTERP_AND_OLUT(4)
			TH1_3D_TETRAINTERP_AND_OLUT(5)
			TH1_3D_TETRAINTERP_AND_OLUT(6)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
		TH1_STORE_DATA8(3);
		TH1_STORE_DATA8(4);
		TH1_STORE_DATA8(5);
		TH1_STORE_DATA8(6);
	}
}


/******************************************************************************/

void
	evalTh1i3o7d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)

{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16, outp2 = outp[2].p16, outp3 = outp[3].p16;
KpUInt16_p	outp4 = outp[4].p16, outp5 = outp[5].p16, outp6 = outp[6].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpInt32_t	outStride4 = outStride[4], outStride5 = outStride[5], outStride6 = outStride[6]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0, prevRes4 = 0, prevRes5 = 0, prevRes6 = 0;

	TH1_3D_INITD16

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD16
		
		if ((ColorPart1 != prevPart1) || (data2 != prevPart2)) {

			TH1_3D_FINDTETRAD16
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
			TH1_3D_TETRAINTERP_AND_OLUT(4)
			TH1_3D_TETRAINTERP_AND_OLUT(5)
			TH1_3D_TETRAINTERP_AND_OLUT(6)
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
		TH1_STORE_DATA16(2);
		TH1_STORE_DATA16(3);
		TH1_STORE_DATA16(4);
		TH1_STORE_DATA16(5);
		TH1_STORE_DATA16(6);
	}
}


/******************************************************************************/

void
	evalTh1i3o8d8 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)
{
KpUInt8_p	outp0 = outp[0].p8, outp1 = outp[1].p8, outp2 = outp[2].p8, outp3 = outp[3].p8;
KpUInt8_p	outp4 = outp[4].p8, outp5 = outp[5].p8, outp6 = outp[6].p8, outp7 = outp[7].p8;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpInt32_t	outStride4 = outStride[4], outStride5 = outStride[5], outStride6 = outStride[6], outStride7 = outStride[7]; 
KpUInt8_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0, prevRes4 = 0, prevRes5 = 0, prevRes6 = 0, prevRes7 = 0;

	TH1_3D_INITD8

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD8
		
		if (thisColor != prevColor) {

			TH1_3D_FINDTETRAD8
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
			TH1_3D_TETRAINTERP_AND_OLUT(4)
			TH1_3D_TETRAINTERP_AND_OLUT(5)
			TH1_3D_TETRAINTERP_AND_OLUT(6)
			TH1_3D_TETRAINTERP_AND_OLUT(7)
		}

		TH1_STORE_DATA8(0);		/* store the data */
		TH1_STORE_DATA8(1);
		TH1_STORE_DATA8(2);
		TH1_STORE_DATA8(3);
		TH1_STORE_DATA8(4);
		TH1_STORE_DATA8(5);
		TH1_STORE_DATA8(6);
		TH1_STORE_DATA8(7);
	}
}


/******************************************************************************/

void
	evalTh1i3o8d16 (imagePtr_p	inp,
					KpInt32_p	inStride,
					imagePtr_p	outp,
					KpInt32_p	outStride,
					KpInt32_t	n,
					th1Cache_p	th1Cache)

{
KpUInt16_p	outp0 = outp[0].p16, outp1 = outp[1].p16, outp2 = outp[2].p16, outp3 = outp[3].p16;
KpUInt16_p	outp4 = outp[4].p16, outp5 = outp[5].p16, outp6 = outp[6].p16, outp7 = outp[7].p16;
KpInt32_t	outStride0 = outStride[0], outStride1 = outStride[1], outStride2 = outStride[2], outStride3 = outStride[3]; 
KpInt32_t	outStride4 = outStride[4], outStride5 = outStride[5], outStride6 = outStride[6], outStride7 = outStride[7]; 
KpUInt16_t	prevRes0 = 0, prevRes1 = 0, prevRes2 = 0, prevRes3 = 0, prevRes4 = 0, prevRes5 = 0, prevRes6 = 0, prevRes7 = 0;

	TH1_3D_INITD16

	for (i = n; i > 0; i--) {
		TH1_3D_GETDATAD16
		
		if ((ColorPart1 != prevPart1) || (data2 != prevPart2)) {

			TH1_3D_FINDTETRAD16
					
			TH1_3D_TETRAINTERP		/* tetrahedral interpolation for channel 0 */
			prevRes0 = outLut0[(0*4096)+tResult];

			TH1_3D_TETRAINTERP_AND_OLUT(1)	/* and the remaining channels */
			TH1_3D_TETRAINTERP_AND_OLUT(2)
			TH1_3D_TETRAINTERP_AND_OLUT(3)
			TH1_3D_TETRAINTERP_AND_OLUT(4)
			TH1_3D_TETRAINTERP_AND_OLUT(5)
			TH1_3D_TETRAINTERP_AND_OLUT(6)
			TH1_3D_TETRAINTERP_AND_OLUT(7)
		}

		TH1_STORE_DATA16(0);		/* store the data */
		TH1_STORE_DATA16(1);
		TH1_STORE_DATA16(2);
		TH1_STORE_DATA16(3);
		TH1_STORE_DATA16(4);
		TH1_STORE_DATA16(5);
		TH1_STORE_DATA16(6);
		TH1_STORE_DATA16(7);
	}
}

#endif /* #if defined (KCP_EVAL_TH1) */

