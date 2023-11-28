/*********************************************************************/
/*
	Contains:	This module contains the transform functions.

				Created by lsh, September 20, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, 
                        all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPXFORM.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spxform.c_v  $
		$Revision:   2.3  $
		$Date:   07 Apr 1994 13:24:28  $
		$Author:   lsh  $

	SCCS Revision:
		@(#)spxform.c	1.69	12/22/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1997                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"
#include <string.h>
#include "attrcipg.h"


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert Xform handle to pointer to locked Xform data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 25, 1993
 *------------------------------------------------------------------*/
SpXformData_t FAR *SpXformLock (SpXform_t Xform)
{
	SpXformData_t	FAR *XformData;

	XformData = lockBuffer ((KcmHandle) Xform);
	if (NULL != XformData) {
		if (SpXformDataSig != XformData->Signature) {
			unlockBuffer ((KcmHandle) Xform);
			return NULL;
		}
	}
	return XformData;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Unlock xform data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 25, 1993
 *------------------------------------------------------------------*/
void SpXformUnlock (SpXform_t Xform)
{
	unlockBuffer ((KcmHandle) Xform);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Allocate an Xform data block.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 20, 1994
 *------------------------------------------------------------------*/
SpStatus_t SpXformAllocate (
				SpXform_t	FAR *Xform)
{
	SpXformData_t	FAR *XformData;

/* allocate a Transform block */
	XformData = SpMalloc (sizeof (*XformData));
	if (NULL == XformData)
		return SpStatMemory;

/* clear entire data block */
	KpMemSet (XformData, 0, sizeof (*XformData));

/* mark memory with a signature */
	XformData->Signature   = SpXformDataSig;
	XformData->HdrWPValid  = KPFALSE;
	XformData->MedWPValid  = KPFALSE;

	*Xform = (SpXform_t FAR) getHandleFromPtr (XformData);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get PT reference number.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 25, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformGetRefNum (SpXform_t Xform, PTRefNum_t FAR *RefNum)
{
	SpXformData_t	FAR *XformData;

	XformData = SpXformLock (Xform);
	if (NULL == XformData)
		return SpStatBadXform;

	*RefNum = XformData->PTRefNum;
	SpXformUnlock (Xform);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Table to Convert WhichRender and WhichTrans to TagId.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 21, 1993
 *------------------------------------------------------------------*/
typedef struct {
	SpTransRender_t	Render;
	SpTransType_t	Transform;
	SpTagId_t		TagId;
} LutTagTable_t;

static LutTagTable_t LutTagTable [] = {
	{SpTransRenderPerceptual,	SpTransTypeIn,		SpTagAToB0},
	{SpTransRenderPerceptual,	SpTransTypeOut,		SpTagBToA0},
	{SpTransRenderPerceptual,	SpTransTypeSim,		SpTagPreview0},
	{SpTransRenderPerceptual,	SpTransTypeGamut,	SpTagGamut},

	{SpTransRenderColormetric,	SpTransTypeIn,		SpTagAToB1},
	{SpTransRenderColormetric,	SpTransTypeOut,		SpTagBToA1},
	{SpTransRenderColormetric,	SpTransTypeSim,		SpTagPreview1},
	{SpTransRenderColormetric,	SpTransTypeGamut,	SpTagGamut},

	{SpTransRenderSaturation,	SpTransTypeIn,		SpTagAToB2},
	{SpTransRenderSaturation,	SpTransTypeOut,		SpTagBToA2},
	{SpTransRenderSaturation,	SpTransTypeSim,		SpTagPreview2},
	{SpTransRenderSaturation,	SpTransTypeGamut,	SpTagGamut},

	{SpTransRenderAbsColormetric,	SpTransTypeIn,		SpTagAToB1},
	{SpTransRenderAbsColormetric,	SpTransTypeOut,		SpTagBToA1},
	{SpTransRenderAbsColormetric,	SpTransTypeSim,		SpTagPreview1},
	{SpTransRenderAbsColormetric,	SpTransTypeGamut,	SpTagGamut},
};


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert WhichRender and WhichTrans to TagId.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 21, 1993
 *------------------------------------------------------------------*/
static SpStatus_t SpRenderAndTransToTagId (
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				SpTagId_t		FAR *TagId)
{
	int		i;

	for (i = 0; i < SPNumElem (LutTagTable); i++) {
		if (LutTagTable [i].Render != WhichRender)
			continue;

		if (LutTagTable [i].Transform != WhichTransform)
			continue;

		*TagId = LutTagTable [i].TagId;
		return SpStatSuccess;
	}

	return SpStatOutOfRange;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Table to Lut TagId to TagId for Chain class.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	July 6, 1994
 *------------------------------------------------------------------*/
typedef struct {
	SpTagId_t		LutId;
	SpTagId_t		ChainId;
} LutChainTagTable_t;

#define SpLutChainEntry(nm) {SpTag##nm, SpTagKChain##nm}

static LutChainTagTable_t LutChainTagTable [] = {
	SpLutChainEntry (AToB0),
	SpLutChainEntry (BToA0),
	SpLutChainEntry (Preview0),

	SpLutChainEntry (AToB1),
	SpLutChainEntry (BToA1),
	SpLutChainEntry (Preview1),

	SpLutChainEntry (AToB2),
	SpLutChainEntry (BToA2),
	SpLutChainEntry (Preview2),

	SpLutChainEntry (Gamut),
};


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert Lut TagId to TagId for Chain class.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	July 6, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpConvertLutIdToChainId (
				SpTagId_t	LutId,
				SpTagId_t	FAR *ChainId)
{
	int		i;

	for (i = 0; i < SPNumElem (LutChainTagTable); i++) {
		if (LutChainTagTable [i].LutId != LutId)
			continue;

		*ChainId = LutChainTagTable [i].ChainId;
		return SpStatSuccess;
	}

	return SpStatOutOfRange;
}


#if !defined (SP_READONLY_PROFILES)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Set data for implied LUT tag.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 21, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformSetData (
				SpProfile_t		Profile,
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				KpInt32_t		DataSize,
				SpHugeBuffer_t	Data)
{
	SpStatus_t	Status;
	SpTagId_t	LutId;

	Status = SpRenderAndTransToTagId (WhichRender, WhichTransform, &LutId);
	if (SpStatSuccess != Status)
		return Status;

	return SpRawTagDataSet (Profile, LutId, DataSize, Data);
}
#endif	/* !SP_READONLY_PROFILES */


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get size of data for implied LUT tag.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 21, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformGetDataSize (
				SpProfile_t		Profile,
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				KpInt32_t		FAR *DataSize)
{
	SpStatus_t		Status;
	SpTagId_t		TagId;

	Status = SpRenderAndTransToTagId (WhichRender, WhichTransform, &TagId);
	if (SpStatSuccess != Status)
		return Status;

/* get the tag data */
	Status = SpRawTagDataGetSize (Profile, TagId, (KpUInt32_t FAR *) DataSize);

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get data for the implied LUT tag.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 25, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformGetData (
				SpProfile_t		Profile,
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				KpInt32_t		DataSize,
				SpHugeBuffer_t	Data)
{
	SpStatus_t	Status;
	SpTagId_t	TagId;
	KpUInt32_t	TagDataSize;
	void		KPHUGE * TagData;
	void		KPHUGE * tagDataH;

	Status = SpRenderAndTransToTagId (WhichRender, WhichTransform, &TagId);
	if (SpStatSuccess != Status)
		return Status;

/* get the tag data */
	Status = SpRawTagDataGet (Profile, TagId, &TagDataSize, &tagDataH);
	if (SpStatSuccess != Status)
		return Status;

/* check that caller's buffer can hold the data */
	if (TagDataSize > (KpUInt32_t) DataSize)
		return SpStatBufferTooSmall;

/* copy to caller's buffer */
	TagData = (void KPHUGE FAR *) lockBuffer (tagDataH) ;
	KpMemCpy (Data, TagData, TagDataSize);

/* free local copy */
	SpRawTagDataFree (Profile, TagId, TagData);
    unlockBuffer (tagDataH);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get the color space from an Xform in the Color Processor.
 *	This is translated to a color space for a Profile Transform.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 7, 1993
 *------------------------------------------------------------------*/
SpStatus_t SpColorSpaceKp2Sp (
				KpInt32_t	KpColorSpace,
				KpInt32_t	FAR *SpColorSpace)
{
	switch (KpColorSpace) {
	case KCM_CIE_XYZ:
		*SpColorSpace = SpSpaceXYZ;
		break;

	case KCM_CIE_LAB:
		*SpColorSpace = SpSpaceLAB;
		break;

	case KCM_CIE_LUV:
		*SpColorSpace = SpSpaceluv;
		break;

	case KCM_CIE_YXY:
		*SpColorSpace = SpSpaceYxy;
		break;

	case KCM_RGB:
		*SpColorSpace = SpSpaceRGB;
		break;

	case KCM_CIE_GRAY:
		*SpColorSpace = SpSpaceGRAY;
		break;

	case KCM_CIE_HSV:
		*SpColorSpace = SpSpaceHSV;
		break;

	case KCM_CIE_HLS:
		*SpColorSpace = SpSpaceHLS;
		break;

	case KCM_CMYK:
		*SpColorSpace = SpSpaceCMYK;
		break;

	case KCM_CMY:
		*SpColorSpace = SpSpaceCMY;
		break;

	case KCM_PHOTO_CD_YCC:
		*SpColorSpace = SpSpaceYCbCr;
		break;

	case KCM_RCS:
		*SpColorSpace = SpSpaceRCS;
		break;

	case KCM_MONO:
		*SpColorSpace = SpSpaceGRAY;
		break;

	case KCM_2_COLOR:
		*SpColorSpace = SpSpace2CLR;
		break;

	case KCM_3_COLOR:
		*SpColorSpace = SpSpace3CLR;
		break;

	case KCM_4_COLOR:
		*SpColorSpace = SpSpace4CLR;
		break;

	case KCM_5_COLOR:
	case KCM_HI_FI_5_COLOR:
		*SpColorSpace = SpSpace5CLR;
		break;

	case KCM_6_COLOR:
	case KCM_HI_FI_6_COLOR:
		*SpColorSpace = SpSpace6CLR;
		break;

	case KCM_7_COLOR:
	case KCM_HI_FI_7_COLOR:
		*SpColorSpace = SpSpace7CLR;
		break;

	case KCM_8_COLOR:
	case KCM_HI_FI_8_COLOR:
		*SpColorSpace = SpSpace8CLR;
		break;

	case KCM_9_COLOR:
		*SpColorSpace = SpSpace9CLR;
		break;

	case KCM_A_COLOR:
		*SpColorSpace = SpSpaceACLR;
		break;

	case KCM_B_COLOR:
		*SpColorSpace = SpSpaceBCLR;
		break;

	case KCM_C_COLOR:
		*SpColorSpace = SpSpaceCCLR;
		break;

	case KCM_D_COLOR:
		*SpColorSpace = SpSpaceDCLR;
		break;

	case KCM_E_COLOR:
		*SpColorSpace = SpSpaceECLR;
		break;

	case KCM_F_COLOR:
		*SpColorSpace = SpSpaceFCLR;
		break;

	default:
		*SpColorSpace = 0;
		return SpStatOutOfRange;
	}

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert a color space from Profile to Precision.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 24, 1993
 *------------------------------------------------------------------*/
SpStatus_t SpColorSpaceSp2Kp (
				KpInt32_t	SpColorSpace,
				KpInt32_t	FAR *KpColorSpace)
{
	switch (SpColorSpace) {
	case SpSpaceXYZ:
		*KpColorSpace = KCM_CIE_XYZ;
		break;

	case SpSpaceLAB:
		*KpColorSpace = KCM_CIE_LAB;
		break;

	case SpSpaceluv:
		*KpColorSpace = KCM_CIE_LUV;
		break;

	case SpSpaceYxy:
		*KpColorSpace = KCM_CIE_YXY;
		break;

	case SpSpaceRGB:
		*KpColorSpace = KCM_RGB;
		break;

	case SpSpaceGRAY:
		*KpColorSpace = KCM_CIE_GRAY;
		break;

	case SpSpaceHSV:
		*KpColorSpace = KCM_CIE_HSV;
		break;

	case SpSpaceHLS:
		*KpColorSpace = KCM_CIE_HLS;
		break;

	case SpSpaceCMY:
		*KpColorSpace = KCM_CMY;
		break;

	case SpSpaceCMYK:
		*KpColorSpace = KCM_CMYK;
		break;

	case SpSpaceYCbCr:
		*KpColorSpace = KCM_PHOTO_CD_YCC;
		break;

 	case SpSpaceRCS:
		*KpColorSpace = KCM_RCS;
		break;

 	case SpSpace2CLR:
		*KpColorSpace = KCM_2_COLOR;
		break;

 	case SpSpace3CLR:
		*KpColorSpace = KCM_3_COLOR;
		break;

 	case SpSpace4CLR:
		*KpColorSpace = KCM_4_COLOR;
		break;

	case SpSpace5CLR:
 	case SpSpaceMCH5:
		*KpColorSpace = KCM_5_COLOR;
		break;

	case SpSpace6CLR:
 	case SpSpaceMCH6:
		*KpColorSpace = KCM_6_COLOR;
		break;

	case SpSpace7CLR:
 	case SpSpaceMCH7:
		*KpColorSpace = KCM_7_COLOR;
		break;

	case SpSpace8CLR:
 	case SpSpaceMCH8:
		*KpColorSpace = KCM_8_COLOR;
		break;

	case SpSpace9CLR:
		*KpColorSpace = KCM_9_COLOR;
		break;

	case SpSpaceACLR:
		*KpColorSpace = KCM_A_COLOR;
		break;

	case SpSpaceBCLR:
		*KpColorSpace = KCM_B_COLOR;
		break;

	case SpSpaceCCLR:
		*KpColorSpace = KCM_C_COLOR;
		break;

	case SpSpaceDCLR:
		*KpColorSpace = KCM_D_COLOR;
		break;

	case SpSpaceECLR:
		*KpColorSpace = KCM_E_COLOR;
		break;

	case SpSpaceFCLR:
		*KpColorSpace = KCM_F_COLOR;
		break;

	case SpSpaceGAMUT:
	default:
		*KpColorSpace = KCM_ICC_UNKNOWN;
		return SpStatOutOfRange;
	}

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Set the ICC color space attribute in the CP
 *
 * AUTHOR
 *      acr	
 *
 * DATE CREATED
 *	August 23, 1996
 *------------------------------------------------------------------*/
static SpStatus_t SpSetColorSpaceICC2Kp (
		PTRefNum_t RefNum, 
		KpInt32_t  AttrNum, 
		KpInt32_t  SpColorSpace)
{
	PTErr_t		PTErr;
	char		attrStr[5];

	/* unknown color space given to CP  */
	/* set the signature string in the CP */
	strncpy(attrStr, (char_p)&SpColorSpace, 4);
	attrStr[4] = 0;

	if (AttrNum == KCM_SPACE_IN) 
		PTErr = PTSetAttribute (RefNum, KCM_ICC_COLORSPACE_IN, attrStr);
	else  
		PTErr = PTSetAttribute (RefNum, KCM_ICC_COLORSPACE_OUT, attrStr);

	return SpStatusFromPTErr(PTErr);

}
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get the ICC color space attribute in the CP
 *
 * AUTHOR
 *      acr	
 *
 * DATE CREATED
 *	August 23, 1996
 *------------------------------------------------------------------*/
static SpStatus_t SpGetColorSpaceKp2ICC(
		PTRefNum_t RefNum, 
		KpInt32_t  AttrNum, 
		KpInt32_t  *AttrValue)
{
PTErr_t		PTErr;
KpInt32_t	strSize;
char		attrStr[5];

	/* unknown color space from CP  */
	/* read the signature string form the CP */

	strSize= sizeof(attrStr);
	if (AttrNum == KCM_SPACE_IN) 
		PTErr = PTGetAttribute (RefNum, KCM_ICC_COLORSPACE_IN, &strSize, attrStr);
	else  
		PTErr = PTGetAttribute (RefNum, KCM_ICC_COLORSPACE_OUT, &strSize, attrStr);

	if (PTErr != KCP_SUCCESS)
		return SpStatusFromPTErr(PTErr);

	strncpy((char *)AttrValue, attrStr, 4);

	return (SpStatSuccess);
	
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Set the color space attribute for a Color Processor Transform.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 24, 1993
 *------------------------------------------------------------------*/
SpStatus_t SpXformSetColorSpace (
				PTRefNum_t	RefNum,
				KpInt32_t	AttrNum,
				KpInt32_t	SpColorSpace)
{
	KpInt32_t	AttrValue;
	SpStatus_t	status;

	status = SpColorSpaceSp2Kp (SpColorSpace, &AttrValue);
	if (status == SpStatOutOfRange)
		/* must be unknown color space - set ICC strings to CP*/
		SpSetColorSpaceICC2Kp (RefNum, AttrNum, SpColorSpace);

	return SpSetKcmAttrInt (RefNum, AttrNum, AttrValue);
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Initialize the color space attribute for a Color Processor Transform.
 *	If the attribute is not set, set it to the supplied default.
 *
 * AUTHOR
 * 	gbp
 *
 * DATE CREATED
 *	27 June 1995
 *------------------------------------------------------------------*/
SpStatus_t SpXformInitColorSpace (
				PTRefNum_t	RefNum,
				KpInt32_t	AttrNum,
				KpInt32_t	SpColorSpace)
{
	SpStatus_t	status;
	KpInt32_t	AttrIntValue;
	PTErr_t		PTErr;
	char		AttrAscValue [10];
	KpInt32_t	AttrSize;

	AttrSize = sizeof (AttrAscValue);

	PTErr = PTGetAttribute (RefNum, AttrNum, &AttrSize, AttrAscValue);
	if (KCP_SUCCESS == PTErr) {
		status = SpStatSuccess;	/* already set. leave it */
	}
	else {		/* not set, use default */
		status = SpColorSpaceSp2Kp (SpColorSpace, &AttrIntValue);
		if (status == SpStatOutOfRange)
			/* must be unknown color space - set ICC strings to CP*/
			SpSetColorSpaceICC2Kp (RefNum, AttrNum, SpColorSpace);

		status = SpSetKcmAttrInt (RefNum, AttrNum, AttrIntValue);
	}

	return status;	
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get the color space from an Xform in the Color Processor.
 *	This is translated to a color space for a Profile Transform.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 7, 1993
 *------------------------------------------------------------------*/
SpStatus_t SpXformGetColorSpace (
				PTRefNum_t	RefNum,
				KpInt32_t	AttrNum,
				KpInt32_t	FAR *ColorSpace)
{
SpStatus_t	status;


	status = SpColorSpaceKp2Sp (SpGetKcmAttrInt (RefNum, AttrNum), 
			ColorSpace);
	if (status == SpStatOutOfRange)
		/* must be unknown color space - read ICC strings from CP*/
		SpGetColorSpaceKp2ICC (RefNum, AttrNum, ColorSpace);

	return (SpStatSuccess);
}



/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Checkin and activate a transform.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	February 5, 1994
 *------------------------------------------------------------------*/
SpStatus_t SpXformLoadImp (
				KpLargeBuffer_t	Data,
				KpInt32_t		Size,
				PTRefNum_t		FAR *RefNum)
{
	PTErr_t		PTStat;

/* give the Transform to the color processor */
	PTStat = PTCheckIn (RefNum, (PTAddr_t) Data);
	if (KCP_SUCCESS == PTStat) {
		PTStat = PTActivate (*RefNum, Size, (PTAddr_t) Data);
		if (KCP_SUCCESS != PTStat)
			PTCheckOut (*RefNum);
	}

	return SpStatusFromPTErr(PTStat);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Create a transform from a block of data containing an InterColor Lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 21, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformFromBuffer (
				KpInt32_t		BufferSize,
				SpHugeBuffer_t	Buffer,
				SpSig_t			SpaceIn,
				SpSig_t			SpaceOut,
				SpXform_t		FAR *Xform)
{
	SpStatus_t		Status;
	PTRefNum_t		RefNum;

	*Xform = NULL;

/* give transform to the color processor */
	Status = SpXformLoadImp (Buffer, BufferSize, &RefNum);
	if (SpStatSuccess != Status)
		return Status;

/* set the color space attributes */
	Status = SpXformInitColorSpace (RefNum, KCM_SPACE_IN, SpaceIn);
	if (SpStatSuccess == Status)
		Status = SpXformInitColorSpace(
				RefNum, KCM_SPACE_OUT, SpaceOut);

/* build an SpXform_t from the RefNum */
	if (SpStatSuccess == Status)
		Status = SpXformFromPTRefNumImp (RefNum, Xform);

/* free PT if SpXform_t not created */
	if (SpStatSuccess != Status)
		PTCheckOut (RefNum);

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get the Associated Xform Data
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	August 10, 1996
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformGetParms (
				SpXform_t		Xform,
				SpTransRender_t		*WhichRender,
				SpTransType_t		*WhichTransform,
				KpF15d16XYZ_t		*HdrWhite,
				KpF15d16XYZ_t		*MedWhite,
				KpUInt32_t		*ChainIn,
				KpUInt32_t		*ChainOut)
{
	SpXformData_t		*XformData;

	XformData = SpXformLock(Xform);
	if (XformData == NULL)
		return(SpStatBadXform);

/* get the transform supplied data */
	*WhichRender	= XformData->WhichRender;
	*WhichTransform	= XformData->WhichTransform;

	if (XformData->HdrWPValid == KPTRUE)
	{
		HdrWhite->X	= XformData->HdrWtPoint.X;
		HdrWhite->Y	= XformData->HdrWtPoint.Y;
		HdrWhite->Z	= XformData->HdrWtPoint.Z;
	}

	if (XformData->MedWPValid == KPTRUE)
	{
		MedWhite->X	= XformData->MedWtPoint.X;
		MedWhite->Y	= XformData->MedWtPoint.Y;
		MedWhite->Z	= XformData->MedWtPoint.Z;
	}

	*ChainIn	= XformData->ChainIn;
	*ChainOut	= XformData->ChainOut;

	SpXformUnlock (Xform);
	return SpStatSuccess;

}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Fill in the rest of the xform info
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	August 10, 1996
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformSetParms (
				SpXform_t		Xform,
				SpTransRender_t		WhichRender,
				SpTransType_t		WhichTransform,
				KpF15d16XYZ_t		HdrWhite,
				KpF15d16XYZ_t		MedWhite,
				KpUInt32_t		ChainIn,
				KpUInt32_t		ChainOut)
{
	SpXformData_t		*XformData;

	XformData = SpXformLock(Xform);
	if (XformData == NULL)
		return(SpStatBadXform);

/* give transform the supplied data */
	XformData->WhichRender    = WhichRender;
	XformData->WhichTransform = WhichTransform;

	XformData->HdrWtPoint.X   = HdrWhite.X;
	XformData->HdrWtPoint.Y   = HdrWhite.Y;
	XformData->HdrWtPoint.Z   = HdrWhite.Z;
	if (HdrWhite.X + HdrWhite.Y + HdrWhite.Z > 0.0)
		XformData->HdrWPValid = KPTRUE;
	XformData->MedWtPoint.X   = MedWhite.X;
	XformData->MedWtPoint.Y   = MedWhite.Y;
	XformData->MedWtPoint.Z   = MedWhite.Z;
	if (MedWhite.X + MedWhite.Y + MedWhite.Z > 0.0)
		XformData->MedWPValid = KPTRUE;

	XformData->ChainIn  = ChainIn;
	XformData->ChainOut = ChainOut;

	SpXformUnlock (Xform);
	return SpStatSuccess;

}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a transform from a profile.
 *	Implementation.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 21, 1993
 *------------------------------------------------------------------*/
static SpStatus_t SpXformGetImp (
				SpProfile_t		Profile,
				SpHeader_t		FAR *Header,
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				SpXform_t		FAR *Xform)
{
	SpStatus_t		Status;
	SpTagId_t		LutId;
	SpTagId_t		ChainId;
	KpUInt32_t		TagDataSize;
	void			KPHUGE * TagData;
	SpXformData_t	FAR *XformData;
	KpInt32_t		ColorSpaceIn;
	KpInt32_t		ColorSpaceOut;
	char			KPHUGE *Buf;
	SpTagValue_t	TagValue;
	void			KPHUGE *tagDataH;

/* clear callers return variable */
	*Xform = NULL;

/*********************************************/
/* check for request for any rendering style */
/*********************************************/
	if (SpTransRenderAny == WhichRender) {
		Status = SpXformGetImp (Profile, Header,
					SpTransRenderPerceptual,
					WhichTransform, Xform);
		if (SpStatSuccess != Status)
		{
			Status = SpXformGetImp (Profile, Header,
						SpTransRenderColormetric,
						WhichTransform, Xform);
			if (SpStatSuccess != Status)
			{
				Status = SpXformGetImp (Profile, Header,
						SpTransRenderSaturation,
						WhichTransform, Xform);
				if (Status == SpStatSuccess)
					Status = SpStatXformIsSaturation;
			} else
				Status = SpStatXformIsColormetric;
		} else
			Status = SpStatXformIsPerceptual;

		return Status;
	}

/* convert to tag id */
	Status = SpRenderAndTransToTagId (WhichRender, WhichTransform, &LutId);
	if (SpStatSuccess != Status)
		return Status;

/* get the tag data */
	Status = SpRawTagDataGet (Profile, LutId, &TagDataSize, &tagDataH);
	if (SpStatSuccess != Status)
		return Status;


/* get the color spaces for the transform */
	switch (WhichTransform) {
	case SpTransTypeIn:
		ColorSpaceIn  = Header->DataColorSpace;
		ColorSpaceOut = Header->InterchangeColorSpace;
		break;

	case SpTransTypeOut:
		ColorSpaceIn  = Header->InterchangeColorSpace;
		ColorSpaceOut = Header->DataColorSpace;
		break;

	case SpTransTypeGamut:
		ColorSpaceIn  = Header->InterchangeColorSpace;
		ColorSpaceOut = SpSpaceGAMUT;
		break;

	case SpTransTypeSim:
	default:
		ColorSpaceIn  =
		ColorSpaceOut = Header->InterchangeColorSpace;
		break;
	}

	TagData = (void KPHUGE FAR *) lockBuffer (tagDataH) ;
	Status = SpXformFromBuffer (TagDataSize, TagData,
				ColorSpaceIn, ColorSpaceOut, 
				Xform);

	if (SpStatSuccess != Status) {
		SpRawTagDataFree (Profile, LutId, TagData);
   		unlockBuffer (tagDataH);
		return Status;
	}

/* remember which Lut was found */
	XformData = SpXformLock (*Xform);
	if (NULL == XformData) {
		SpRawTagDataFree (Profile, LutId, TagData);
   		unlockBuffer (tagDataH);
		return SpStatBadXform;
	}

	XformData->HdrWtPoint.X = Header->Illuminant.X;
	XformData->HdrWtPoint.Y = Header->Illuminant.Y;
	XformData->HdrWtPoint.Z = Header->Illuminant.Z;
	XformData->HdrWPValid   = KPTRUE; 

	Status = SpTagGetById (Profile, SpTagMediaWhitePnt, &TagValue);

	if (SpStatSuccess == Status)
	{
		XformData->MedWtPoint.X = TagValue.Data.XYZ.X;
		XformData->MedWtPoint.Y = TagValue.Data.XYZ.Y;
		XformData->MedWtPoint.Z = TagValue.Data.XYZ.Z;
		XformData->MedWPValid   = KPTRUE;
	} else {
		XformData->MedWtPoint.X = 0;
		XformData->MedWtPoint.Y = 0;
		XformData->MedWtPoint.Z = 0;
		XformData->MedWPValid   = KPFALSE;
	}

	Buf = TagData;
	XformData->LutType = SpGetUInt32 (&Buf);
	XformData->WhichRender = WhichRender;
	XformData->WhichTransform = WhichTransform;
	SpRawTagDataFree (Profile, LutId, TagData);
    unlockBuffer (tagDataH);

/* get the chaining values */
	Status = SpConvertLutIdToChainId (LutId, &ChainId);
	if (SpStatSuccess == Status) {
		Status = SpTagGetById (Profile, ChainId, &TagValue);
		if (SpStatSuccess == Status) {
			if (2 == TagValue.Data.UInt8s.Count) {
				XformData->ChainIn = TagValue.Data.UInt8s.Values [0];
				XformData->ChainOut = TagValue.Data.UInt8s.Values [1];
				if ((0 != XformData->ChainIn) && (0 != XformData->ChainOut)) {
					SpSetKcmAttrInt (XformData->PTRefNum,
									KCM_IN_CHAIN_CLASS_2, XformData->ChainIn);
					SpSetKcmAttrInt (XformData->PTRefNum,
									KCM_OUT_CHAIN_CLASS_2, XformData->ChainOut);
				}
			}
			SpTagFree (&TagValue);
		}
		else if (Status == SpStatMemory) {
			SpXformUnlock (*Xform);
			return Status;
		}
	}
	else {
		XformData->ChainIn = 0;
		XformData->ChainOut = 0;
	}

	SpXformUnlock (*Xform);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a transform from a profile.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 29, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformGet (
				SpProfile_t		Profile,
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				SpXform_t		FAR *Xform)
{
	SpStatus_t	Status;
	SpHeader_t	Header;
	SpXformData_t	*XformData;

	*Xform = NULL;

	Status = SpProfileGetHeader (Profile, &Header);
	if (SpStatSuccess != Status)
		return Status;

/* get the transform */
	Status = SpXformGetImp (Profile, &Header, WhichRender,
							WhichTransform, Xform);

	if ((SpStatSuccess != Status) && 
	    (WhichRender != SpTransRenderAny) &&
	    (WhichTransform != SpTransTypeSim))
		Status = SpXformGetImp (Profile, &Header, SpTransRenderAny,
								WhichTransform, Xform);


	if (SpStatSuccess != Status) {

		/*	If the error indicates we got back a different transform then
			we asked and we asked for any transform then return success
			The Below Statuses are only returned if the rendering
			intent requested was SpTransRenderAny */
		if ((SpStatXformIsPerceptual == Status) ||
			(SpStatXformIsColormetric == Status) ||
			(SpStatXformIsSaturation == Status)) {
		
			if ((SpTransRenderAny   == WhichRender) ||
			    (Header.DeviceClass != SpProfileClassOutput)) {
					Status = SpStatSuccess;
			}

			if (SpTransRenderAbsColormetric == WhichRender) {
				XformData = SpXformLock (*Xform);
				if (NULL == XformData) {
					return SpStatBadXform;
				} else {
					XformData->WhichRender = WhichRender;
					SpXformUnlock (*Xform);
					Status = SpStatSuccess;
				}
			}

		}
		else {

			/* try to generate the transform if we don't have a good status */		
			Status = SpXformGenerate (Profile, 16, WhichRender,             
									  WhichTransform, Xform);
		}

	}

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a transform from the color processor.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	April 21, 1994
 *------------------------------------------------------------------*/
SpStatus_t SpXformGetDataFromCP (
				PTRefNum_t	PTRefNum,
				KpInt32_t	LutSize,
				KpUInt32_t	FAR *DataSize,
				SpHugeBuffer_t	FAR *Data)
{
	PTErr_t		PTStat;
	KpUInt32_t	MFType;

	*Data = NULL;
	*DataSize = 0;

/* determine type of transform to get */
	switch (LutSize) {
	case 0:
		MFType = PTTYPE_FUTF;
		break;

	case 8:
		MFType = PTTYPE_MFT1;
		break;

	case 16:
		MFType = PTTYPE_MFT2;
		break;

	default:
		return SpStatOutOfRange;
	}

/* ask color processor for the size of the Transform */
	PTStat = PTGetSizeF (PTRefNum, MFType, (KpInt32_t FAR *) DataSize);
	if (KCP_SUCCESS != PTStat)
		return SpStatusFromPTErr(PTStat);

/* allocate a buffer to hold the data */
	*Data = SpMalloc (*DataSize);
	if (NULL == *Data)
		return SpStatMemory;

/* ask color processor for the Transform data */
	PTStat = PTGetPTF (PTRefNum, MFType, *DataSize, (PTAddr_t) *Data);
	if (KCP_SUCCESS != PTStat) {
		SpFree (*Data);
		return SpStatusFromPTErr(PTStat);
	}

	return SpStatSuccess;
}


#if !defined (SP_READONLY_PROFILES)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Set a transform into a profile.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 25, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformSet (
				SpProfile_t		Profile,
				KpInt32_t		LutSize,
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				SpXform_t		Xform)
{
	SpXformData_t	FAR *XformData;
	SpStatus_t		Status;
	KpUInt32_t		DataSize;
	SpHugeBuffer_t	Data;
	SpTagId_t		LutId;
	SpTagId_t		ChainId;
	SpTagValue_t	Value;
	unsigned char	Int8Data [2];

/* lock down xform data */
	XformData = SpXformLock (Xform);
	if (NULL == XformData)
		return SpStatBadXform;

	Status = SpXformGetDataFromCP (XformData->PTRefNum, LutSize,
					&DataSize, &Data);

/* save the lut data in the profile */
	if (SpStatSuccess != Status)
	{
		SpXformUnlock (Xform);
		return Status;
	}

	Status = SpXformSetData(Profile, WhichRender,
				WhichTransform, DataSize, Data);

/* save Color Space Info - if not already there  */
	if ((SpStatSuccess == Status)
		&& (XformData->SpaceIn == 0)
		&& (XformData->SpaceOut == 0))
	{	XformData->SpaceIn  = SpGetKcmAttrInt (XformData->PTRefNum, 
						KCM_SPACE_IN);
		XformData->SpaceOut = SpGetKcmAttrInt (XformData->PTRefNum, 
                                                KCM_SPACE_OUT);
	}

/* save chaining information, if any */
	if ((SpStatSuccess == Status)
			&& (0 != XformData->ChainIn)
			&& (0 != XformData->ChainOut)) {
		SpRenderAndTransToTagId (WhichRender, WhichTransform, &LutId);
		Status = SpConvertLutIdToChainId (LutId, &ChainId);
		if (SpStatSuccess == Status) {
			Value.TagId = ChainId;
			Value.TagType = Sp_AT_UInt8;
			Value.Data.UInt8s.Count = 2;
			Value.Data.UInt8s.Values = Int8Data;
			Int8Data [0] = (unsigned char) XformData->ChainIn;
			Int8Data [1] = (unsigned char) XformData->ChainOut;
			Status = SpTagSet (Profile, &Value);
		}
		else
			Status = SpStatSuccess;
	}

	SpXformUnlock (Xform);
	SpFree (Data);

	return Status;
}
#endif /* !(SP_READONLY_PROFILES) */


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Release resources used for transform.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 21, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformFree (
				SpXform_t		FAR *Xform)
{
	SpXformData_t	FAR *XformData;
	
	XformData = SpXformLock (*Xform);
	if (NULL == XformData)
		return SpStatBadXform;

	PTCheckOut (XformData->PTRefNum);

	SpXformUnlock (*Xform);
	SpFree (XformData);
	*Xform = NULL;

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get the number of channels for a given color space.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 9, 1993
 *------------------------------------------------------------------*/
KpInt32_t SpGetChannelsFromColorSpace (
				KpInt32_t	ColorSpace)
{
	switch (ColorSpace) {
	case SpSpaceGRAY:
		return 1;

	case SpSpaceXYZ:
	case SpSpaceLAB:
	case SpSpaceluv:
	case SpSpaceYxy:
	case SpSpaceRGB:
	case SpSpaceHSV:
	case SpSpaceHLS:
	case SpSpaceCMY:
	case SpSpaceYCbCr:
		return 3;

	case SpSpaceCMYK:
		return 4;

	case SpSpaceMCH5:
		return 5;

	case SpSpaceMCH6:
		return 6;

	case SpSpaceMCH7:
		return 7;

	case SpSpaceMCH8:
		return 8;
	}

	return 0;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get the number of input and output channels for a Transform.
 *
 * AUTHOR
 * 	mjb
 *
 * DATE CREATED
 *	October 17, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformGetChannels (
				SpXform_t		Xform,
				KpInt32_t		FAR *In,
				KpInt32_t		FAR *Out)
{

	PTRefNum_t	refNum;
	KpChar_t	attribute [KCM_MAX_ATTRIB_VALUE_LENGTH+1];
	KpInt32_t	inChans, outChans, temp, size, attribId, index;
	SpStatus_t	spStatus;
	PTErr_t		ptErr;

	/*	Get the PTRefNum for the transform	*/
	spStatus = SpXformGetRefNum (Xform, &refNum);
	if (SpStatSuccess != spStatus) {
		return spStatus;
	}

	/*	Start by determining the number of output channels	*/
	size = sizeof (attribute);
	ptErr = PTGetAttribute (refNum, KCM_NUM_VAR_OUT, &size, attribute);
	if (KCP_SUCCESS != ptErr) {
		return SpStatBadXform;
	}
	outChans = KpAtoi (attribute);

	/*	Check that we didn't exceed the maximum output channels	*/
	if (outChans > SpMaxComponents) {
		return SpStatBadXform;
	}

	/*	For each output channel get the number of input channels.  The 
		number of input channels must be the same for each output channel.	*/
	inChans = -1;
	for (index = 0, attribId = KCM_NUM_VAR_1_IN; index < outChans; 
			index++, attribId++) {

		size = sizeof (attribute);
		ptErr = PTGetAttribute (refNum, attribId, &size, attribute);
		if (KCP_SUCCESS != ptErr) {
			return SpStatBadXform;
		}
		temp = KpAtoi (attribute);

		if (-1 == inChans) {
			inChans = temp;
		}
		else {
			if (temp != inChans) {
				return SpStatBadXform;
			}
		}

	}

	/*	return the number of input and output channels	*/
	*In = inChans;
	*Out = outChans;
	return SpStatSuccess;

}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get the description information for a Transform.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 26, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformGetDesc (
				SpXform_t		Xform,
				SpXformDesc_t	FAR *Desc)
{
	SpXformData_t	FAR *XformData;

	XformData = SpXformLock (Xform);
	if (NULL == XformData)
		return SpStatBadXform;

	Desc->LutType = XformData->LutType;
	Desc->WhichRender = XformData->WhichRender;
	Desc->WhichTransform = XformData->WhichTransform;
	Desc->SpaceIn = XformData->SpaceIn;
	Desc->SpaceOut = XformData->SpaceOut;
	SpXformUnlock (Xform);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *      Create a Lut from an Xform
 *
 * AUTHOR
 *      doro
 *
 * DATE CREATED
 *      December 27, 1995
 *------------------------------------------------------------------*/

SpStatus_t SPAPI SpXformToLut  (SpXform_t	Xform,
				SpLut_t		*Lut,
				SpTransRender_t	*WhichRender,
				SpTransType_t	*WhichTransform,
				SpSig_t		*SpaceIn,
				SpSig_t		*SpaceOut,
				KpF15d16XYZ_t	*HdrWhite,
				KpF15d16XYZ_t	*MedWhite,
				KpUInt32_t	*ChainIn,
				KpUInt32_t	*ChainOut)
{
SpHugeBuffer_t	Buf;
char		*LutBuf;
KpUInt32_t	BufSize;

SpStatus_t	Status;

SpXformData_t   FAR *XformData;

	/* Set Up Private Structure to Populate */
	XformData = lockBuffer ((KcmHandle)Xform);
	if (NULL == XformData)
		return SpStatBadXform;

	if (XformData->LutSize == 0)
		XformData->LutSize = 16;

	Status = SpXformGetDataFromCP  (XformData->PTRefNum,
					XformData->LutSize,
					&BufSize,
					&Buf);

	if (Status != SpStatSuccess)
	{
		unlockBuffer ((KcmHandle)Xform);
		return Status;
	}

	if (XformData->LutSize == 8)
		Lut->LutType = SpTypeLut8;
	else
		Lut->LutType = SpTypeLut16;

	/* Move past the tag and Reserved */
	LutBuf = (char *)Buf;
	LutBuf += 8;
	Status = SpLutToPublic(LutBuf, Lut);

	/* Clear White Points */
	HdrWhite->X = MedWhite->X =
	HdrWhite->Y = MedWhite->Y =
	HdrWhite->Z = MedWhite->Z = 0;

	if (Status == SpStatSuccess)
	{
		*WhichRender     = XformData->WhichRender;
		*WhichTransform  = XformData->WhichTransform;
		*SpaceIn         = XformData->SpaceIn;
		*SpaceOut        = XformData->SpaceOut;
		if (XformData->HdrWPValid) {
			HdrWhite->X       = XformData->HdrWtPoint.X;
			HdrWhite->Y       = XformData->HdrWtPoint.Y;
			HdrWhite->Z       = XformData->HdrWtPoint.Z;
		}
		if (XformData->MedWPValid) {
			MedWhite->X      = XformData->MedWtPoint.X;
			MedWhite->Y      = XformData->MedWtPoint.Y;
			MedWhite->Z      = XformData->MedWtPoint.Z;
		}
		*ChainIn         = XformData->ChainIn;
		*ChainOut        = XformData->ChainOut;
	}
	SpFree(Buf);
	unlockBuffer ((KcmHandle)Xform);
	return Status;
}

/***************************************************************************
 * FUNCTION NAME
 *      SpXformToPT
 *
 * DESCRIPTION
 *      This function copies the PT associated with the PTRefNum in
 *      the Xform to the Address given.  It will be in the format
 *      requested by the caller.  The SpXform is NOT freed by this
 *      function.
 *
 *
 
***************************************************************************/
SpStatus_t SPAPI SpXformToPT(SpXform_t	spXform,
				KpUInt32_t	LutSize,
				KpUInt32_t	Datasize,
				SpHugeBuffer_t pXformData)
{
 
PTRefNum_t	refNum;
SpStatus_t	spStatus;
KpUInt32_t	PTDatasize;

PTErr_t		PTStat;
KpUInt32_t	MFType;

	/*      Get the Color Processor PTRefNum of the SpXform */
	spStatus = SpXformGetRefNum (spXform, &refNum);
	if (SpStatSuccess != spStatus)
		return spStatus;

/* determine type of transform to get */
	switch (LutSize) {
	case 0:
		MFType = PTTYPE_FUTF;
		break;

	case 8:
		MFType = PTTYPE_MFT1;
		break;

	case 16:
		MFType = PTTYPE_MFT2;
		break;

	default:
		return SpStatOutOfRange;
	}

/* ask color processor for the size of the Transform */
	PTStat = PTGetSizeF (refNum, MFType, (KpInt32_t FAR *)&PTDatasize);
	if (KCP_SUCCESS != PTStat)
		return SpStatusFromPTErr(PTStat);

	if (PTDatasize > Datasize)
		return SpStatBufferTooSmall;

/* ask color processor for the Transform data */
	PTStat = PTGetPTF(refNum, MFType, 
			Datasize, (PTAddr_t) pXformData);

	return SpStatusFromPTErr(PTStat);

}

SpStatus_t SPAPI SpXformGetPTSize(SpXform_t   spXform,
				KpUInt32_t	LutSize,
				KpUInt32_t	*Datasize)
{
PTRefNum_t	refNum;
SpStatus_t	spStatus;
PTErr_t		PTStat;
KpUInt32_t	MFType;

	/*      Get the Color Processor PTRefNum of the SpXform */
	spStatus = SpXformGetRefNum (spXform, &refNum);
	if (SpStatSuccess != spStatus)
		return spStatus;

/* determine type of transform to get */
	switch (LutSize) {
	case 0:
		MFType = PTTYPE_FUTF;
		break;

	case 8:
		MFType = PTTYPE_MFT1;
		break;

	case 16:
		MFType = PTTYPE_MFT2;
		break;

	default:
		return SpStatOutOfRange;
	}

	PTStat = PTGetSizeF (refNum, MFType, 
			(KpInt32_t FAR *) Datasize);
 
	return SpStatusFromPTErr(PTStat);
}
ÿ