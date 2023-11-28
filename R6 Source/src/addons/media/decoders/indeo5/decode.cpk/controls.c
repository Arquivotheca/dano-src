/*
 *
 *              INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *      This software is supplied under the terms of a license
 *      agreement or nondisclosure agreement with Intel Corporation
 *      and may not be copied or disclosed except in accordance
 *      with the terms of that agreement.
 *
 *      Copyright (C) 1994-1997 Intel Corp.
 *      All Rights Reserved.
 */

/*
 * CONTROLS.C -- Contrast, Brightness, and Saturation controls
 * 		called from HIVE in response to custom messages
 *
 * The following functions are implemented:
 *		DecodeChangeBrightness
 *		DecodeChangeContrast
 *		DecodeChangeSaturation
 *		DecodeResetBrightnessAndContrast
 *		DecodeResetSaturation
 *		DecodeUpdateLuma
 *		DecodeUpdateChroma
 *
 * The prototypes are in controls.h
 */

#include "datatype.h"
#include "pia_main.h"
#include "cpk_blk.h"
#include "xpardec.h"
#include "rtdec.h"
#include "hivedec.h"
#include "pia_dec.h"
#include "controls.h"


/*
 * Function: void ModifyContrastOrSaturation(U16, PU8);
 *
 * Description:
 *		This function is a helper function called by DecodeChangeContrast
 *		and Decode ChangeSaturation.
 *		Modifying the Contrast or Saturation tables is the same, so this
 * 		function does either.
 *			0 = 1/SCALE					yeilds all 0's
 *			1 = 2/SCALE
 *			n = (n+1)/SCALE
 * 			SCALE - 1 = 1				yields no change
 *			65535 = 65536/SCALE         yields all 255's
 *											(except 0 for 0)
 * if the response is too coarse, SCALE can be increased
 * if the response is too fine, SCALE can be decreased
 */
#define SCALE 256

void
ModifyContrastOrSaturation(U16 iDelta, PU8 puTable){

	I32 i;
	I32 temp, changeFactor;

	changeFactor = (I32) ((U32)iDelta) + 1;	/* 1 to 65536 */

	for(i = 0; i < 256; i++) {
		temp = i;				/* Begin with original default value */
		temp -= 128; 			/* shift table value about 0 */
		temp *= SCALE;			/* gain extra precision */
		if (temp == 0)
			temp += SCALE / 2;  /* Middle is 127 1/2 */
		temp *= changeFactor;
		temp /= SCALE;
		temp /= SCALE;
		temp += 128;

		if (temp < 0)			/* and clamp */
			puTable[i] = 0;
		else if (temp < 256)
 			puTable[i] = (U8) temp;
		else
			puTable[i] = 255;
	}
}


/*
 * Function: PIA_RETURN_STATUS DecodeChangeBrightness
 *
 * Description:
 *		Change the brightness table.  If the pointer to the table is
 *      nul, use the global table, else the table is for the instance
 *      only.  Add the new iDelta to each entry, and then clip to keep
 *      the entries within range.
 */

PIA_RETURN_STATUS
DecodeChangeBrightness(PTR_DEC_INST pInst, I32 iDelta)
{
	I32 temp;
	I32  i;
	PU8 pLumaTable, pBrightTable, pContrastTable;

	if (pInst->pDecodePrivate == NULL)
		return PIA_S_ERROR;  /* sanity check */

  	pLumaTable     = pInst->pDecodePrivate->puLumaTable;
	pBrightTable   = pInst->pDecodePrivate->puBrightTable;
	pContrastTable = pInst->pDecodePrivate->puContrastTable;

	/* Compute the brightness value, and then compose with the contrast value.*/
	for(i = 0; i < 256; i++) {
		temp = iDelta + i;			  /* original value is i adjust by iDelta */

		if (temp < 0) {				  /* clip to between 0 and 255 */
			pBrightTable[i] = 0;
		} else if (temp < 256) {
			pBrightTable[i] = (U8) temp;
		} else { /* >= 256 */
			pBrightTable[i] = 255;
		}

		pLumaTable[i] =	pContrastTable[pBrightTable[i]];
	}
	pInst->pDecodePrivate->bUpdateLuminance = TRUE;

	return PIA_S_OK;
}

/*
 * Function: PIA_RETURN_STATUS DecodeChangeContrast
 *
 * Description:
 *		Select whether this is a global or instance modification based
 *		upon whether the pointer to the table is NULL or not.  The actual
 *		work is done in ModifyContrastOrSaturation().
 */

PIA_RETURN_STATUS
DecodeChangeContrast(PTR_DEC_INST pInst, I32 iDelta)
{
	U32  i;
	U16 u16ChangeFactor;
	PU8 pLumaTable, pBrightTable, pContrastTable;

	if (pInst->pDecodePrivate == NULL)
		return PIA_S_ERROR;  /* sanity check */

  	pLumaTable     = pInst->pDecodePrivate->puLumaTable;
	pBrightTable   = pInst->pDecodePrivate->puBrightTable;
	pContrastTable = pInst->pDecodePrivate->puContrastTable;

	/* The iDelta is 0, then just reset the table.  Else, put the change factor
	 * into the range expected between 0 and 65535 with 255 being the nochange
	 * point.
	 */
	if (iDelta == 0)
		DecodeResetContrast(pInst);
	else {
		if (iDelta < -255)
			u16ChangeFactor = 0;
		else if (iDelta > 255)
			u16ChangeFactor = 65535;
		else if (iDelta < 0)
			u16ChangeFactor = (U16) (iDelta + 255);
		else 
			u16ChangeFactor = (U16) (iDelta * iDelta) + 255;

		ModifyContrastOrSaturation(u16ChangeFactor, &pContrastTable[0]);

		/* Compose with the brightness table to achieve the final luma table*/
		for (i=0; i<256; i++)
			pLumaTable[i] = pBrightTable[pContrastTable[i]];

		pInst->pDecodePrivate->bUpdateLuminance = TRUE;
	}

	return PIA_S_OK;
}

/*
 * Function: PIA_RETURN_STATUS DecodeChangeSaturation
 *
 * Description:
 *		Select whether this is a global or instance modification based
 *		upon whether the pointer to the table is NULL or not.  The actual
 *		work is done in ModifyContrastOrSaturation().
 */

PIA_RETURN_STATUS
DecodeChangeSaturation(PTR_DEC_INST pInst, I32 iDelta)
{
	U16 u16ChangeFactor;

	if (pInst->pDecodePrivate == NULL)
		return PIA_S_ERROR;  /* sanity check */

	/* The iDelta is 0, then just reset the table.  Else, put the change factor
	 * into the range expected, between 0 and 65535, with 255 being the no-change
	 * point.
	 */
	if (iDelta == 0)
		DecodeResetSaturation(pInst);
	else {
		if (iDelta < -255)
			u16ChangeFactor = 0;
		else if (iDelta > 255)
			u16ChangeFactor = 65535;
		else if (iDelta < 0)
			u16ChangeFactor = (U16) (iDelta + 255);
		else
			u16ChangeFactor = (U16) (iDelta * iDelta) + 255;
		ModifyContrastOrSaturation(u16ChangeFactor,
									&pInst->pDecodePrivate->puChromaTable[0]);

		pInst->pDecodePrivate->bUpdateChrominance = TRUE;
	}

	return PIA_S_OK;
}

/*
 * Function: PIA_RETURN_STATUS DecodeResetBrightness
 *
 * Description:
 * 		Sets the brightness table to the identity table, then sets
 *		the luminance table to the contrast table.  Compare the luminance
 *		table to the identity, and reset the flag to use them if
 *		appropriate (it is the identity table)
 */

PIA_RETURN_STATUS
DecodeResetBrightness(PTR_DEC_INST pInst) {
	U32 i;
	PIA_Boolean bIdentity;
	PU8 pLumaTable, pBrightTable, pContrastTable;

	if (pInst->pDecodePrivate == NULL)
		return PIA_S_ERROR;  /* sanity check */

  	pLumaTable     = pInst->pDecodePrivate->puLumaTable;
	pBrightTable   = pInst->pDecodePrivate->puBrightTable;
	pContrastTable = pInst->pDecodePrivate->puContrastTable;

	bIdentity = 1;
	for(i = 0; i < 256; i++) {
  		pBrightTable[i] = (unsigned char)i;		/* reset the brightness */
		pLumaTable[i] = pContrastTable[i];
		if (pLumaTable[i] != i)
			bIdentity = 0;
	}

	if (bIdentity)
		pInst->pDecodePrivate->bUpdateLuminance = FALSE;

	return PIA_S_OK;
}

/*
 * Function: PIA_RETURN_STATUS DecodeResetContrast
 *
 * Description:
 * 		Sets the contrast table to the identity table, then sets the
 *		luminance table to the brightness table.  If the luminance table
 * 		is the identity, then reset the flag indicating the need to use
 *		the tables.
 */

PIA_RETURN_STATUS
DecodeResetContrast(PTR_DEC_INST pInst) {
	U32 i;
	PIA_Boolean bIdentity;
	PU8 pLumaTable, pBrightTable, pContrastTable;

	if (pInst->pDecodePrivate == NULL)
		return PIA_S_ERROR;  /* sanity check */

  	pLumaTable     = pInst->pDecodePrivate->puLumaTable;
	pBrightTable   = pInst->pDecodePrivate->puBrightTable;
	pContrastTable = pInst->pDecodePrivate->puContrastTable;

	bIdentity = 1;
	for(i = 0; i < 256; i++) {
  		pContrastTable[i] = (unsigned char)i;		/* reset the contrast */
		pLumaTable[i] = pBrightTable[i];
		if (pLumaTable[i] != i)
			bIdentity = 0;
	}

	if (bIdentity)
		pInst->pDecodePrivate->bUpdateLuminance = FALSE;

	return PIA_S_OK;
}

/*
 * Function: PIA_RETURN_STATUS DecodeResetSaturation
 *
 * Description:
 * 		Sets the chrominance table to the identity table,
 *		and reset the flag indicating the need to use the table.
 */

PIA_RETURN_STATUS
DecodeResetSaturation(PTR_DEC_INST pInst) {
	U32 i;

	if (pInst->pDecodePrivate == NULL)
		return PIA_S_ERROR;  /* sanity check */

	pInst->pDecodePrivate->bUpdateChrominance = FALSE;

	for(i = 0; i < 256; i++)
  		pInst->pDecodePrivate->puChromaTable[i] = (unsigned char)i;

	return PIA_S_OK;
}


/*
 * Function: void DecodeUpdateLuma( PU8 pu8Y, U32 uWidth, U32 uHeight, 
 									U32 uStride, PU8 puTable)
 *
 * Description:
 * 		Appplies the brightness and contrast table to the luminance pels
 */

void
DecodeUpdateLuma(PU8 pu8Y, U32 uWidth, U32 uHeight, U32 uStride, PU8 puTable)
{
	U32 i,j;
	U32 index;

	for (i=0; i < uHeight; i++) 
		for (j=0, index = i*uStride; j < uWidth; j++, index++) 
			pu8Y[index] = puTable[pu8Y[index]];
}


/*
 * Function: void DecodeUpdateChroma(PU8 pu8VU, U32 uWidth, U32 uHeight, 
 									 U32 uStride, PU8 puTable)
 *
 * Description:
 * 		Appplies the saturation table to the chrominance pels.
 */

void
DecodeUpdateChroma(PU8 pu8VU, U32 uWidth, U32 uHeight, U32 uStride, PU8 puTable)
{
	U32 i,j;
	U32 index;

	for (i=0; i < uHeight; i++) 
		for (j=0, index = i*uStride; j < uWidth; j++, index++) 
			pu8VU[index] = puTable[pu8VU[index]];
}
