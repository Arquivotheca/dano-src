/*
 * @(#)profilem.c	1.4 97/12/22

	Contains:	forward and inverse monochrome PT via PTNewMonoPT

	Written by:	Color Proccessor group

	Copyright:	1995, by Eastman Kodak Company (all rights reserved)

	Change History (most recent first):

*/
/*********************************************************************/

#include "csmatrix.h"
#include "kcpfut.h"
#include "monopt.h"
#include "attrib.h"

/*------------------------------------------------------------------------------
 *  PTNewMonoPT
 *------------------------------------------------------------------------------
 */

PTErr_t
	 PTNewMonoPT (	ResponseRecord_p	grayTRC,
					KpUInt32_t			gridsize,
					bool				invert,
					PTRefNum_p			thePTRefNumP)
{
PTErr_t	PTErr;
fut_ptr_t theFut;
char inColorSpaceAttr[5], outColorSpaceAttr[5], compositionAttr[5];

/* Check for valid ptrs */
	if ((grayTRC == NULL) || (thePTRefNumP == NULL)) {
		PTErr = KCP_BAD_PTR;
		goto GetOut;
	}

	if (Kp_IsBadReadPtr(grayTRC, sizeof(*grayTRC)))	{
		PTErr = KCP_BAD_PTR;
		goto GetOut;
	}

	if (Kp_IsBadWritePtr(thePTRefNumP, sizeof(*thePTRefNumP))) {
		PTErr = KCP_BAD_PTR;
		goto GetOut;
	}

	/* pass the input arguments along to the fut maker */
	if (invert == False) {
		PTErr = makeForwardXformMono
						(grayTRC, gridsize, &theFut);

		/* setup the foward color space */
		KpItoa(KCM_MONO, inColorSpaceAttr); 
		KpItoa(KCM_CIE_LAB, outColorSpaceAttr);
	}
	else {
		PTErr = makeInverseXformMono
						(grayTRC, gridsize, &theFut);

		/* setup the inverse color space */
		KpItoa(KCM_CIE_LAB, inColorSpaceAttr); 
		KpItoa(KCM_MONO, outColorSpaceAttr);
	}

	if (PTErr != KCP_SUCCESS) {
		goto GetOut;
	}

	/* convert fut to a PT */
	PTErr = fut2PT (theFut, thePTRefNumP);
	if (PTErr != KCP_SUCCESS) {
		goto GetOut;
	}

	/* set the color space attributes */
	PTErr = PTSetAttribute (*thePTRefNumP, KCM_IN_SPACE, inColorSpaceAttr);
	if (PTErr != KCP_SUCCESS) {
		goto GetOut;
	}

	PTErr = PTSetAttribute (*thePTRefNumP, KCM_OUT_SPACE, outColorSpaceAttr);
	if (PTErr != KCP_SUCCESS) {
		goto GetOut;
	}

	/* set the composition state attribute */
	KpItoa(KCM_SINGLE, compositionAttr);
	PTErr = PTSetAttribute (*thePTRefNumP, KCM_COMPOSITION_STATE, compositionAttr);

GetOut:
	return (PTErr);
}

