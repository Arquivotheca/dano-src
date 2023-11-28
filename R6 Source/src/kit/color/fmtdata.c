/*********************************************************************/
/*
	File:		fmtdata.c	@(#)fmtdata.c	1.6	12/22/97

	Contains:	This module contains most of the internals for
				doevalsw.c's switch statement and was split out
				so it could switch on color space as well.
					Created by MSM, January 15, 1996

	Written by:	The KCMS Team

 *********************************************************************
 *    COPYRIGHT (c) 1992-1996 Eastman Kodak Company
 *    As  an  unpublished  work pursuant to Title 17 of the United
 *    States Code.  All rights reserved.
 *********************************************************************
 */

#include "kcms_sys.h"

#include "attrib.h"
#include "fut.h"
#include "fut_util.h"
#include "kcmptlib.h"
#include "kcptmgr.h"
#include "kcpmgru.h"
#include "kcpfut.h"
#include "kcpcache.h"


void
	pass8in  (KpInt32_t nInputs, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpInt32_t	i1, i2;

	for (i1 = 0; i1 < nInputs; i1++){
		for (i2 = 0; i2 < np; i2++){
			*oLineData[i1].p8++ = *iLineData[i1].p8;
			iLineData[i1].p8 += inPelStride[i1];
		}
	}
}
					
void
	format555to8 (KpInt32_t	nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theTmp8, theData16;
KpInt32_t	i1;

	if (nChans) {}
		
	for (i1 = 0; i1 < np; i1++) {
		theData16 = *iLineData[0].p16;
		iLineData[0].p8 += inPelStride[0];
						
		theTmp8 = KCP_EXTRACT_COMPONENT(theData16, 5, 10);
		*oLineData[0].p8++ = (KpUInt8_t)KCP_CONVERT_UP(theTmp8, 5, 8);
						
		theTmp8 = KCP_EXTRACT_COMPONENT(theData16, 5, 5);
		*oLineData[1].p8++ = (KpUInt8_t) KCP_CONVERT_UP(theTmp8, 5, 8);
					
		theTmp8 = KCP_EXTRACT_COMPONENT(theData16, 5, 0);
		*oLineData[2].p8++ = (KpUInt8_t) KCP_CONVERT_UP(theTmp8, 5, 8);
	}
}
					

void
	format565to8 (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theTmp8, theData16;
KpInt32_t	i1;
	
	if (nChans) {}
		
	for (i1 = 0; i1 < np; i1++) {
		theData16 = *iLineData[0].p16;
		iLineData[0].p8 += inPelStride[0];
						
		theTmp8 = (KpUInt8_t)KCP_EXTRACT_COMPONENT(theData16, 5, 11);
		*oLineData[0].p8++ = (KpUInt8_t)KCP_CONVERT_UP(theTmp8, 5, 8);
			
		theTmp8 = (KpUInt8_t)KCP_EXTRACT_COMPONENT(theData16, 6, 5);
		*oLineData[1].p8++ = (KpUInt8_t)KCP_CONVERT_UP(theTmp8, 6, 8);
						
		theTmp8 = (KpUInt8_t)KCP_EXTRACT_COMPONENT(theData16, 5, 0);
		*oLineData[2].p8++ = (KpUInt8_t)KCP_CONVERT_UP(theTmp8, 5, 8);
	}
}


void
	format8to12 (KpInt32_t	nInputs, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16;
KpInt32_t	i1, i2;

	for (i1 = 0; i1 < nInputs; i1++){
		for (i2 = 0; i2 < np; i2++){
			theData16 = *iLineData[i1].p8;
			iLineData[i1].p8 += inPelStride[i1];
			*oLineData[i1].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theData16, 8, 12);
		}
	}
}


void
	pass16in (KpInt32_t	nInputs, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpInt32_t	i1, i2;

	for (i1 = 0; i1 < nInputs; i1++){
		for (i2 = 0; i2 < np; i2++){
			*oLineData[i1].p16++ = *iLineData[i1].p16;
			iLineData[i1].p8 += inPelStride[i1];
		}
	}
}


void
	format16to12 (KpInt32_t	nInputs, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16;
KpInt32_t	i1, i2;

	for (i1 = 0; i1 < nInputs; i1++) {
		for (i2 = 0; i2 < np; i2++) {
			theData16 = *iLineData[i1].p16;
			iLineData[i1].p8 += inPelStride[i1];
			*oLineData[i1].p16++ = (KpUInt16_t)KCP_CONVERT_DOWN(theData16, 16, 12);
		}
	}
}				


void
	format555to12 (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16, theTmp16;
KpInt32_t	i1;
	
	if (nChans) {}
		
	for (i1 = 0; i1 < np; i1++) {
		theData16 = *iLineData[0].p16;
		iLineData[0].p8 += inPelStride[0];
						
		theTmp16 = KCP_EXTRACT_COMPONENT(theData16, 5, 10);
		theTmp16 = KCP_CONVERT_UP(theTmp16, 5, 10);
		*oLineData[0].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp16, 10, 12);
							
		theTmp16 = KCP_EXTRACT_COMPONENT(theData16, 5, 5);
		theTmp16 = KCP_CONVERT_UP(theTmp16, 5, 10);
		*oLineData[1].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp16, 10, 12);
				
		theTmp16 = KCP_EXTRACT_COMPONENT(theData16, 5, 0);
		theTmp16 = KCP_CONVERT_UP(theTmp16, 5, 10);
		*oLineData[2].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp16, 10, 12);
	}
}
					

void
	format565to12 (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16, theTmp16;
KpInt32_t	i1;
	
	if (nChans) {}
		
	for (i1 = 0; i1 < np; i1++) {
		theData16 = *iLineData[0].p16;
		iLineData[0].p8 += inPelStride[0];
						
		theTmp16 = KCP_EXTRACT_COMPONENT(theData16, 5, 11);
		theTmp16 = KCP_CONVERT_UP(theTmp16, 5, 10);
		*oLineData[0].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp16, 10, 12);
							
		theTmp16 = KCP_EXTRACT_COMPONENT(theData16, 6, 5);
		*oLineData[1].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp16, 6, 12);
					
		theTmp16 = KCP_EXTRACT_COMPONENT(theData16, 5, 0);
		theTmp16 = KCP_CONVERT_UP(theTmp16, 5, 10);
		*oLineData[2].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp16, 10, 12);
	}
}


void
	format10to12 (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p aPtr)
{
KpInt32_t	i1;
KpUInt32_t	theData32, theTmp32;

	if (nChans) {}
		
	for (i1 = 0; i1 < np; i1++) {
		theData32 = *iLineData[0].p32;
		iLineData[0].p8 += inPelStride[0];
						
		theTmp32 = KCP_EXTRACT_COMPONENT(theData32, 10, 20);
		*aPtr[0].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp32, 10, 12);
							
		theTmp32 = KCP_EXTRACT_COMPONENT(theData32, 10, 10);
		*aPtr[1].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp32, 10, 12);
						
		theTmp32 = KCP_EXTRACT_COMPONENT(theData32, 10, 0);
		*aPtr[2].p16++ = (KpUInt16_t)KCP_CONVERT_UP(theTmp32, 10, 12);
	}
}					
					

void
	pass8out  (KpInt32_t	nOutputs, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p outPelStride, imagePtr_p oLineData)
{
KpInt32_t	i1, i2;

	for (i1 = 0; i1 < nOutputs; i1++){
		for (i2 = 0; i2 < np; i2++){
			*oLineData[i1].p8 = *iLineData[i1].p8++;
			oLineData[i1].p8 += outPelStride[i1];
		}
	}
}
					

void
	format8to555 (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData8, thePel16;
KpInt32_t	i1;
	
	if (nChans) {}
		
	for ( i1 = 0; i1 < np; i1++) {
		theData8 = *iLineData[0].p8++;
		thePel16 = KCP_CONVERT_DOWN(theData8, 8, 5) << 10;
							
		theData8 = *iLineData[1].p8++;
		thePel16 |= KCP_CONVERT_DOWN(theData8, 8, 5) << 5;

		theData8 = *iLineData[2].p8++;
		thePel16 |= KCP_CONVERT_DOWN(theData8, 8, 5);
					
		*oLineData[0].p16 = (KpUInt16_t)thePel16;
		oLineData[0].p8 += inPelStride[0];
	}
}


void
	format8to565  (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p inPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData8, thePel16;
KpInt32_t	i1;
	
	if (nChans) {}
		
	for ( i1 = 0; i1 < np; i1++) {
		theData8 = *iLineData[0].p8++;
		thePel16 = KCP_CONVERT_DOWN(theData8, 8, 5) << 11;
							
		theData8 = *iLineData[1].p8++;
		thePel16 |= KCP_CONVERT_DOWN(theData8, 8, 6) << 5;

		theData8 = *iLineData[2].p8++;
		thePel16 |= KCP_CONVERT_DOWN(theData8, 8, 5);
							
		*oLineData[0].p16 = (KpUInt16_t) thePel16;
		oLineData[0].p8 += inPelStride[0];
	}
}


void
	format12to8 (KpInt32_t	nOutputs, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p outPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16;
KpInt32_t	i1, i2;

	for (i1 = 0; i1 < nOutputs; i1++) {
		for (i2 = 0; i2 < np; i2++) {
			theData16 = *iLineData[i1].p16++;
			*oLineData[i1].p8 = (KpUInt8_t) (KCP_CONVERT_DOWN(theData16, 12, 8));
			oLineData[i1].p8 += outPelStride[i1];
		}
	}
}


void
	pass16out (KpInt32_t	nOutputs, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p outPelStride, imagePtr_p oLineData)
{
KpInt32_t	i1, i2;

	for (i1 = 0; i1 < nOutputs; i1++){
		for (i2 = 0; i2 < np; i2++){
			*oLineData[i1].p16 = *iLineData[i1].p16++;
			oLineData[i1].p8 += outPelStride[i1];
		}
	}
}
void
	format12to16 (KpInt32_t	nOutputs, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p outPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16;
KpInt32_t	i1, i2;

	for (i1 = 0; i1 < nOutputs; i1++) {
		for (i2 = 0; i2 < np; i2++) {
			theData16 = *iLineData[i1].p16++;
			*oLineData[i1].p16 = (KpUInt16_t)KCP_CONVERT_UP(theData16, 12, 16);
			oLineData[i1].p8 += outPelStride[i1];
		}
	}
}

void
	format12to555 (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p outPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16, thePel16;
KpInt32_t	i1;

	if (nChans) {}
		
	for ( i1 = 0; i1 < np; i1++) {
		theData16 = *iLineData[0].p16++;
		thePel16 = KCP_CONVERT_DOWN(theData16, 12, 5) << 10;
							
		theData16 = *iLineData[1].p16++;
		thePel16 |= KCP_CONVERT_DOWN(theData16, 12, 5) << 5;
					
		theData16 = *iLineData[2].p16++;
		thePel16 |= KCP_CONVERT_DOWN(theData16, 12, 5);
							
		*oLineData[0].p16 = (KpUInt16_t) thePel16;
		oLineData[0].p8 += outPelStride[0];
	}
}

void
	format12to565 (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p outPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16, thePel16;
KpInt32_t	i1;

	if (nChans) {}
		
	for ( i1 = 0; i1 < np; i1++) {
		theData16 = *iLineData[0].p16++;
		thePel16 = KCP_CONVERT_DOWN(theData16, 12, 5) << 11;
							
		theData16 = *iLineData[1].p16++;
		thePel16 |= KCP_CONVERT_DOWN(theData16, 12, 6) << 5;
			
		theData16 = *iLineData[2].p16++;
		thePel16 |= KCP_CONVERT_DOWN(theData16, 12, 5);
				
		*oLineData[0].p16 = (KpUInt16_t) thePel16;
		oLineData[0].p8 += outPelStride[0];
	}
}

void
	format12to10 (KpInt32_t nChans, KpInt32_t np, imagePtr_p iLineData,
					KpInt32_p outPelStride, imagePtr_p oLineData)
{
KpUInt32_t	theData16, theData32;
KpInt32_t	i1;

	if (nChans) {}
		
	for ( i1 = 0; i1 < np; i1++) {
		theData16 = *iLineData[0].p16++;
		theData32 = KCP_CONVERT_DOWN(theData16, 12, 10) << 20;
							
		theData16 = *iLineData[1].p16++;
		theData32 |= KCP_CONVERT_DOWN(theData16, 12, 10) << 10;
						
		theData16 = *iLineData[2].p16++;
		theData32 |= KCP_CONVERT_DOWN(theData16, 12, 10);
							
		*oLineData[0].p32 = theData32;
		oLineData[0].p8 += outPelStride[0];
	}
}

