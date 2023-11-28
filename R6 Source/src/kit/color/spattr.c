/*********************************************************************/
/*
	Contains:	This module contains functions for tag management.

				Created by lsh, September 15, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPATTR.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spattr.c_v  $
		$Revision:   2.1  $
		$Date:   04 Apr 1994 09:35:30  $
		$Author:   lsh  $

	SCCS Revision:
		@(#)spattr.c	1.57	12/22/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993 - 1997               ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"
#include <string.h>
#include <stdio.h>

static SpStatus_t UInt32ToHexTxt( KpUInt32_t	Value, 
			KpInt32_p 	BufSize,  
			KpChar_p 	Buffer);

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Free memory allocated to TagValue by SpTagGetXXX.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 4, 1993
 *------------------------------------------------------------------*/
static void SpTagFreeTextDesc (
				SpTextDesc_t	FAR *TextDesc)
{
	SpFree (TextDesc->IsoStr);
	SpFree (TextDesc->UniStr);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Free memory allocated to TagValue by SpTagGetXXX.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 4, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagFree (
				SpTagValue_t	FAR *Value)
{
	SpTagType_t		TagType;
	KpUInt32_t		Index;
	SpProfileSeqDescRecord_t	FAR *ProfSeqRecord;

	SpTagGetIdType (Value->TagId, &TagType);

	/* if this is an unknown tag id, set the tag type from the
	   tag type signature */	
	if (TagType == Sp_AT_Unknown)
		TagType = Value->TagType;

	switch (TagType) {
	case Sp_AT_Enum:
		break;

	case Sp_AT_CrdInfo:
		if (Value->Data.CrdData.ProdDesc.count > 0)
			SpFree (Value->Data.CrdData.ProdDesc.CRD_String);
		for (Index = 0; Index < 4; Index++)
		{
			if (Value->Data.CrdData.CrdInfo[Index].count > 0)
				SpFree (Value->Data.CrdData.CrdInfo[Index].CRD_String);
		}
		break;

	case Sp_AT_Curve:
		SpFree (Value->Data.Curve.Data);
		break;

	case Sp_AT_Data:
		SpFree (Value->Data.Data.Bytes);
		break;

	case Sp_AT_DateTime:
		break;

	case Sp_AT_Lut:
		return SpLutFree (&Value->Data.Lut);

	case Sp_AT_Measurement:
		break;

	case Sp_AT_NamedColors:
		SpFree (Value->Data.NamedColors.Colors);
		break;
	case Sp_AT_NamedColors2:
		SpFree (Value->Data.NamedColors2.Colors);
		break;

	case Sp_AT_ProfileSeqDesc:
		for (Index = 0, ProfSeqRecord = Value->Data.ProfileSeqDesc.Records;
				Index < Value->Data.ProfileSeqDesc.Count;
						Index++, ProfSeqRecord++) {
			SpTagFreeTextDesc (&ProfSeqRecord->DeviceManufacturerDesc);
			SpTagFreeTextDesc (&ProfSeqRecord->DeviceModelDesc);
		}
		SpFree (Value->Data.ProfileSeqDesc.Records);
		break;

	case Sp_AT_SF15d16:
		SpFree (Value->Data.SF15d16s.Values);
		break;

	case Sp_AT_Screening:
		SpFree (Value->Data.Screening.Screens);
		break;

	case Sp_AT_Signature:
		break;

	case Sp_AT_Text:
		SpFree (Value->Data.Text);
		break;

	case Sp_AT_TextDesc:
		SpTagFreeTextDesc (&Value->Data.TextDesc);
		break;

	case Sp_AT_Ucrbg:
		SpFree (Value->Data.Ucrbg.Ucr);
		SpFree (Value->Data.Ucrbg.bg);
		SpFree (Value->Data.Ucrbg.Desc);
		break;

	case Sp_AT_UF16d16:
		SpFree (Value->Data.UF16d16s.Values);
		break;

	case Sp_AT_UInt16:
		SpFree (Value->Data.UInt16s.Values);
		break;

	case Sp_AT_UInt32:
		SpFree (Value->Data.UInt32s.Values);
		break;

	case Sp_AT_UInt64:
		SpFree (Value->Data.UInt64s.Values);
		break;

	case Sp_AT_UInt8:
		SpFree (Value->Data.UInt8s.Values);
		break;

	case Sp_AT_Viewing:
		break;

	case Sp_AT_XYZ:
		break;

	case Sp_AT_Unknown:
		SpFree (Value->Data.Binary.Values);
		break;

	default:
		return SpStatBadTagType;
	}

	return SpStatSuccess;
}

/*--------------------------*/
/* define tag ID type table */
/*--------------------------*/

#define SPTAGID(nm, t) {SpTag##nm, Sp_AT_##t}

typedef struct {
	SpTagId_t		TagId;
	SpTagType_t		TagType;
} SpTagIdTbl_t;

static SpTagIdTbl_t TagIdTbl [] = {

	SPTAGID (Unknown,		Unknown),

/* Kodak private tags */
	SPTAGID (KDeviceBits,		UInt32),
	SPTAGID (KCompressedLUT,	Unknown),
	SPTAGID (KDeviceSerialNum,	TextDesc),
	SPTAGID (KDeviceSettings,	TextDesc),
	SPTAGID (KDeviceUnit,		TextDesc),
	SPTAGID (KDMax,			SF15d16),
	SPTAGID (KEffectType,		Enum),
	SPTAGID (KIllum,		Enum),
	SPTAGID (KInterpretation,	Enum),
	SPTAGID (KLinearizationType,	TextDesc),
	SPTAGID (KLinearized,		Enum),
	SPTAGID (KMedium,		Enum),
	SPTAGID (KMediumDesc,		TextDesc),
	SPTAGID (KMediumProduct,	Enum),
	SPTAGID (KMediumSense,		Enum),
	SPTAGID (KDotGain25,		SF15d16),
	SPTAGID (KDotGain50,		SF15d16),
	SPTAGID (KDotGain75,		SF15d16),
	SPTAGID (KPhosphor,		Enum),
	SPTAGID (KPrtBlackShape,	Enum),
	SPTAGID (KPrtBlackStartDelay,	Enum),
	SPTAGID (KSenseInvertible,	Enum),
	SPTAGID (KVersion,		Text),
	SPTAGID (KDensityType,		TextDesc),
	SPTAGID (KProfileHistory,	Text),
	SPTAGID (KPCDFilmTerm,		Text),

/* Kodak private tags for device to device profiles */
	SPTAGID (KXchDeviceBits,	UInt32),
	SPTAGID (KXchDeviceSerialNum,	TextDesc),
	SPTAGID (KXchDeviceSettings,	TextDesc),
	SPTAGID (KXchDeviceUnit,	TextDesc),
	SPTAGID (KXchGamma,		SF15d16),
	SPTAGID (KXchIllum,		Enum),
	SPTAGID (KXchInterpretation,	Enum),
	SPTAGID (KXchLinearizationType,	TextDesc),
	SPTAGID (KXchLinearized,	Enum),
	SPTAGID (KXchMedium,		Enum),
	SPTAGID (KXchMediumDesc,	TextDesc),
	SPTAGID (KXchMediumProduct,	Enum),
	SPTAGID (KXchMediumSense,	Enum),
	SPTAGID (KXchPhosphor,		Enum),
	SPTAGID (KXchSenseInvertible,	Enum),
	SPTAGID (KXchDotGain25,		SF15d16),
	SPTAGID (KXchDotGain50,		SF15d16),
	SPTAGID (KXchDotGain75,		SF15d16),

/* Kodak private tags for chaining rules */
	SPTAGID (KChainAToB0,		UInt8),
	SPTAGID (KChainBToA0,		UInt8),
	SPTAGID (KChainPreview0,	UInt8),

	SPTAGID (KChainAToB1,		UInt8),
	SPTAGID (KChainBToA1,		UInt8),
	SPTAGID (KChainPreview1,	UInt8),

	SPTAGID (KChainAToB2,		UInt8),
	SPTAGID (KChainBToA2,		UInt8),
	SPTAGID (KChainPreview2,	UInt8),

	SPTAGID (KChainGamut,		UInt8),

/* Kodak private tags for multi colorant xforms */
	SPTAGID (KInkName0,		Text),
	SPTAGID (KInkName1,		Text),
	SPTAGID (KInkName2,		Text),
	SPTAGID (KInkName3,		Text),
	SPTAGID (KInkName4,		Text),
	SPTAGID (KInkName5,		Text),
	SPTAGID (KInkName6,		Text),
	SPTAGID (KInkName7,		Text),
	SPTAGID (KInkDensities,		SF15d16),

/* Public tags */
	SPTAGID (AToB0,			Lut),
	SPTAGID (AToB1,			Lut),
	SPTAGID (AToB2,			Lut),
	SPTAGID (BlueColorant,		XYZ),
	SPTAGID (BlueTRC,		Curve),
	SPTAGID (BToA0,			Lut),
	SPTAGID (BToA1,			Lut),
	SPTAGID (BToA2,			Lut),
	SPTAGID (CalibrationDateTime,	DateTime),
	SPTAGID (CharTarget,		Text),
	SPTAGID (CopyRight,		Text),
	SPTAGID (DeviceMfgDesc,		TextDesc),
	SPTAGID (DeviceModelDesc,	TextDesc),
	SPTAGID (Gamut,			Lut),
	SPTAGID (GrayTRC,		Curve),
	SPTAGID (GreenColorant,		XYZ),
	SPTAGID (GreenTRC,		Curve),
	SPTAGID (Luminance,		XYZ),
	SPTAGID (Measurement,		Measurement),
	SPTAGID (MediaBlackPnt,		XYZ),
	SPTAGID (MediaWhitePnt,		XYZ),
	SPTAGID (NamedColor,		NamedColors),
	SPTAGID (Preview0,		Lut),
	SPTAGID (Preview1,		Lut),
	SPTAGID (Preview2,		Lut),
	SPTAGID (ProfileDesc,		TextDesc),
	SPTAGID (ProfileSeqDesc,	ProfileSeqDesc),
	SPTAGID (PS2CRD0,		Data),
	SPTAGID (PS2CRD1,		Data),
	SPTAGID (PS2CRD2,		Data),
	SPTAGID (PS2CRD3,		Data),
	SPTAGID (PS2CSA,		Data),
	SPTAGID (PS2RenderIntent,	Data),
	SPTAGID (RedColorant,		XYZ),
	SPTAGID (RedTRC,		Curve),
	SPTAGID (ScreeningDesc,		TextDesc),
	SPTAGID (Screening,		Screening),
	SPTAGID (Technology,		Signature),
	SPTAGID (Ucrbg,			Ucrbg),
	SPTAGID (ViewingCondDesc,	TextDesc),
	SPTAGID (ViewingConditions,	Viewing),
	SPTAGID (NamedColor2,		NamedColors2),
	SPTAGID (CrdInfo,		CrdInfo),

	SPTAGID (ENDLIST,				Unknown),
};


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert an tag ID to a type.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 29, 1993
 *------------------------------------------------------------------*/
void SPAPI SpTagGetIdType (
				SpTagId_t	TagId,
				SpTagType_t	FAR *TagType)
{
	SpTagIdTbl_t	*Entry;
	SpTagIdTbl_t	*RetEntry = TagIdTbl;

/* look for ID */
	for (Entry = TagIdTbl; SpTagENDLIST != Entry->TagId; Entry++) {
		if (TagId == Entry->TagId)
		{
			RetEntry = Entry;
			break;
		}
	}

/* give caller the type */
/* Note:  If the type was not found, the first entry in the     */
/* table, Sp_AT_Unknown,  is returned                           */
/*    This is the correct action since we want to pass the      */
/* information along without any interpretations                */
	*TagType = RetEntry->TagType;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Check for header needed to interperate data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	August 15, 1994
 *------------------------------------------------------------------*/
static KpBool_t SpTagNeedHeader (
				SpTagId_t	TagId)
{
	KpBool_t BoolRes;

	BoolRes = (KpBool_t) ((TagId == SpTagNamedColor) ? KPTRUE : KPFALSE);

#if defined (PSFUNC)
	if (TagId == SpTagPS2CSA)
	   BoolRes = KPTRUE;
#endif

	return (BoolRes);
}


/*--------------------------------------------------------------------
* DESCRIPTION
 *      Test if Tag Exists - lifted from SpRawTagDataGet
 *
 * AUTHOR
 *      doro
 *
 * DATE CREATED
 *      August 10, 1996
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagExists (
				SpProfile_t	Profile,
				SpTagId_t	TagId,
				KpBool_t	*TagExists)
{
SpStatus_t	status = SpStatSuccess;
KpInt32_t	index;
SpProfileData_t	FAR *profileData;
SpTagDirEntry_t	FAR *tagArray;
 
const KpInt32_t SpNotFound = -1;

	/* convert profile handle to pointer to locked memory */
	profileData = SpProfileLock (Profile);
	if (NULL == profileData)
		return SpStatBadProfile;
 
	/* Verify the Tag Array if Loaded.  Search Function
	   Only loads the Header */
	if (profileData->TagArray == NULL)
	{
		status = SpProfilePopTagArray(profileData);
		if (status != SpStatSuccess)
			return status;
	}
 
	/* locate the tag data */
	tagArray = (SpTagDirEntry_t FAR *) lockBuffer(profileData->TagArray);
	index = SpTagFindById (tagArray, TagId, profileData->TotalCount);

	*TagExists = (KpBool_t)((SpNotFound == index) ? KPFALSE : KPTRUE);

	/* unlock handles */
	unlockBuffer (profileData->TagArray);
	SpProfileUnlock (Profile);
	return status;
 
} /* SpTagExists */


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get public tag given its ID.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagGetById (
				SpProfile_t		Profile,
				SpTagId_t		TagId,
				SpTagValue_t	FAR *Value)
{
	SpStatus_t	Status;
	KpUInt32_t	TagDataSize;
	void		KPHUGE * TagData;
	SpHeader_t	Hdr;
	SpHeader_t	FAR *Header;
	void		KPHUGE *	tagDataH;
#if defined (PSFUNC)
	SpTransRender_t	render;
#endif

/* get the profile header, if needed */
	if (SpTagNeedHeader (TagId)) {
		Status = SpProfileGetHeader (Profile, &Hdr);
		if (SpStatSuccess != Status)
			return Status;

		Header = &Hdr;
	}
	else
		Header = NULL;

/* get the tag data */
	Status = SpRawTagDataGet (Profile, TagId, &TagDataSize, &tagDataH);
#if defined (PSFUNC)
	if (SpStatSuccess != Status)
	{
		if (TagId == SpTagPS2CRD0)
		{
			Status = SpPSCRDCreate(Profile, 
					SpTransRenderPerceptual,
					&tagDataH, &TagDataSize);

		} else if (TagId == SpTagPS2CRD1)
		{
			Status = SpPSCRDCreate(Profile, 
					SpTransRenderColormetric,
					&tagDataH, &TagDataSize);

		} else if (TagId == SpTagPS2CRD2)
		{
			Status = SpPSCRDCreate(Profile, 
					SpTransRenderSaturation,
					&tagDataH, &TagDataSize);

		} else if (TagId == SpTagPS2CRD3)
		{
			Status = SpStatNotImp;

		} else if (TagId == SpTagPS2CSA)
		{
			/* Definitions of header rendering intent are different
			   from those for specifying xform rendering intent
 			   by 1.  See sprofile.h */
			Status = SpPSCSACreate(Profile, 
					Hdr.RenderingIntent +1,
					&tagDataH, &TagDataSize);
		}
	}
#endif

	if (SpStatSuccess != Status)
		return Status;

/* convert the tag data to a public form */
	TagData = (void KPHUGE FAR *) lockBuffer (tagDataH) ;
	Status = SpTagToPublic (Header, TagId, TagDataSize, TagData, Value);

/* "free" local copy */
	SpRawTagDataFree (Profile, TagId, TagData);
    unlockBuffer (tagDataH);

	return Status;
}


#if !defined (SP_NO_TAGSET)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Set tag value.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagSet (
				SpProfile_t		Profile,
				SpTagValue_t	FAR *Value)
{
	SpStatus_t		Status;
	SpHeader_t		Hdr;
	SpHeader_t		FAR *Header;
	SpTagType_t		TagType;
	void			KPHUGE *Buf;
	KpUInt32_t		BufSize;

/* Validate type for 'known' tags */
	SpTagGetIdType (Value->TagId, &TagType);
	if ((TagType != Sp_AT_Unknown) && (TagType != Value->TagType))
		return SpStatBadTagType;

/* get the profile header, if needed */
	if (SpTagNeedHeader (Value->TagId)) {
		Status = SpProfileGetHeader (Profile, &Hdr);
		if (SpStatSuccess != Status)
			return Status;

		Header = &Hdr;
	}
	else
		Header = NULL;

/* make new tag value reference */
	Status = SpTagFromPublic (Header, Value, &BufSize, &Buf);
	if (SpStatSuccess != Status)
		return Status;

/* now save the data to the profile */
	Status = SpRawTagDataSet (Profile, Value->TagId, BufSize, Buf);

/* free the buffer containing the InterColor formatted data */
	SpFree (Buf);

	return Status;
}
#endif



#if !defined (SP_NO_TEXTFUNCTIONS)
/***************************************************************************
 * FUNCTION NAME
 *      SpTagGetString
 *
 * DESCRIPTION
 *      This function converts the tag value pointed to by TagValue to a string.
 *      The resulting string is copied to the buffer pointed to by Buffer.  The
 *      size of the buffer is specified in the location pointed to by BufSize.
 *      The size of the string placed in the buffer is returned in BufSize.
 *
 ***************************************************************************/
SpStatus_t SPAPI SpTagGetString(SpTagValue_t *TagValue, 
			  KpInt32_p	BufSize,
			  KpChar_p	Buffer)
{
SpStatus_t	spStatus;

	switch (TagValue->TagType) {
		case Sp_AT_Text:
			spStatus = TextToString(TagValue, BufSize, Buffer);
			break;

		case Sp_AT_TextDesc:
			spStatus = TextDescToString(TagValue, BufSize, Buffer);
			break;

		case Sp_AT_Enum:
		case Sp_AT_CrdInfo:
		case Sp_AT_Curve:
		case Sp_AT_Data:
		case Sp_AT_DateTime:
		case Sp_AT_Lut:
		case Sp_AT_Measurement:
		case Sp_AT_NamedColors:
		case Sp_AT_ProfileSeqDesc:
		case Sp_AT_SF15d16:
		case Sp_AT_Screening:
		case Sp_AT_Signature:
		case Sp_AT_UF16d16:
		case Sp_AT_Ucrbg:
		case Sp_AT_UInt16:
		case Sp_AT_UInt32:
		case Sp_AT_UInt64:
		case Sp_AT_UInt8:
		case Sp_AT_Viewing:
		case Sp_AT_XYZ:
		case Sp_AT_NamedColors2:
			spStatus = SpStatNotImp;
			break;

		default:
			spStatus = SpStatBadTagType;
			break;
	}
	return spStatus;
} /* SpTagGetString */



/***************************************************************************
 * FUNCTION NAME
 *      TextToString
 *
 * DESCRIPTION
 *      This function converts tags of type Sp_AT_Text to a string
 ***************************************************************************/
SpStatus_t TextToString(SpTagValue_t	*TagValue, 
			KpInt32_p	BufSize,
			KpChar_p	Buffer)
{
KpInt32_t	length;
SpStatus_t	spStatus = SpStatSuccess;

	if (*BufSize < 1)
		return SpStatBufferTooSmall;

	length = strlen(TagValue->Data.Text);

	if (length >= *BufSize) {
		length = *BufSize -1;
		spStatus = SpStatBufferTooSmall;
	}

	KpMemSet(Buffer, 0, *BufSize);

	strncpy(Buffer, TagValue->Data.Text, length);
	Buffer += length;
	Buffer = 0;
	*BufSize = length;

	return spStatus;
} /* TextToString */

/***************************************************************************
 * FUNCTION NAME
 *      TextDescToString
 *
 * DESCRIPTION
 *      This function converts tags of type Sp_AT_TextDesc to a string
 ***************************************************************************/
SpStatus_t TextDescToString(SpTagValue_t	*TagValue,
			    KpInt32_p		BufSize,
			    KpChar_p		Buffer)
{
KpInt32_t	length;
SpTextDesc_t	*TextDesc;
SpStatus_t	spStatus = SpStatSuccess;

	if (*BufSize < 1)
		return SpStatBufferTooSmall;

	TextDesc = &TagValue->Data.TextDesc;
	length = strlen(TextDesc->IsoStr);
	if (length >= *BufSize) {
		length = *BufSize -1;
		spStatus = SpStatBufferTooSmall;
	}

	KpMemSet(Buffer, 0, *BufSize);

	strncpy(Buffer, TextDesc->IsoStr, length);
	Buffer += length;
	Buffer = 0;
	*BufSize = length;

	return spStatus;
} /* TextDescToString */



/************************************************************
 * FUNCTION NAME
 *      SpProfileGetHeaderString
 *
 * DESCRIPTION
 *      This function converts a specified profile header field 
 *      value into a string.  *BufSize contains the size of the 
 *      Buffer into which the string will be written. The actual 
 *      string size is returned in BufSize.
 *
 ************************************************************/
SpStatus_t SPAPI SpProfileGetHeaderString(SpSearchType_t	hdrItem,
				    SpHeader_t		*hdr,
				    KpInt32_p		BufSize,
				    KpChar_p		Buffer)
{
SpStatus_t	spStat;
KpInt32_t	value;
KpF15d16XYZ_t	aF15d16XYZ_Value;
KpUInt32_t	uintValue;

switch (hdrItem) {

	case SPSEARCH_PROFILECLASS:
		value = hdr->DeviceClass;
		spStat = SignatureToTxt(value, BufSize, Buffer);
		break;
	case SPSEARCH_DEVICECOLORSPACE:
		value = hdr->DataColorSpace;
		spStat = SignatureToTxt(value, BufSize, Buffer);
		break;
	case SPSEARCH_CONNECTIONSPACE:
		value = hdr->InterchangeColorSpace;
		spStat = SignatureToTxt(value, BufSize, Buffer);
		break;
	case SPSEARCH_PREFERREDCMM:
		value = hdr->CMMType;
		spStat = SignatureToTxt(value, BufSize, Buffer);
		break;
	case SPSEARCH_PLATFORM:
		value = hdr->Platform;
		spStat = SignatureToTxt(value, BufSize, Buffer);
		break;
	case SPSEARCH_DEVICEMFG:
		value = hdr->DeviceManufacturer;
		spStat = SignatureToTxt(value, BufSize, Buffer);
		break;
	case SPSEARCH_DEVICEMODEL:
		value = hdr->DeviceModel;
		spStat = SignatureToTxt(value, BufSize, Buffer);
		break;
	case SPSEARCH_PROFILEFLAGS:
		uintValue = hdr->Flags;
		spStat = UInt32ToHexTxt(uintValue, BufSize, Buffer);
		break;
	case SPSEARCH_DEVICEATTRIBUTESHI:
		uintValue = hdr->DeviceAttributes.hi;
		spStat = UInt32ToHexTxt(uintValue, BufSize, Buffer);
		break;
	case SPSEARCH_DEVICEATTRIBUTESLO:
		uintValue = hdr->DeviceAttributes.lo;
		spStat = UInt32ToHexTxt(uintValue, BufSize, Buffer);
		break;
	case SPSEARCH_RENDERINGINTENT:
		uintValue = hdr->RenderingIntent;
		spStat = UInt32ToTxt(uintValue, BufSize, Buffer);
		break;
	case SPSEARCH_ILLUMINANT:
		aF15d16XYZ_Value = hdr->Illuminant;
		spStat = F15d16XYZToTxt(aF15d16XYZ_Value, BufSize, Buffer);
		break;
	case SPSEARCH_VERSION:
		uintValue = hdr->ProfileVersion;
		spStat = UInt32ToHexTxt(uintValue, BufSize, Buffer);
		break;
	case SPSEARCH_ORIGINATOR:
		value = hdr->Originator;
		spStat = SignatureToTxt(value, BufSize, Buffer);
		break;

	default:
		spStat = SpStatUnsupported;
		break;
	}
return spStat;
}



/************************************************************
 * FUNCTION NAME
 *	SignatureToTxt
 *
 * DESCRIPTION
 *	This function converts a SpSig_t value into a string. 
 *      *BufSize contains the size of the Buffer into which 
 *      the string will be written. The actual string size is 
 *      returned in BufSize.
 *
 ************************************************************/
SpStatus_t SignatureToTxt(KpInt32_t	value, 
			  KpInt32_p	BufSize,
			  KpChar_p	Buffer)
{
SpStatus_t	spStat = SpStatSuccess;
char		tempStr[5];
KpInt32_t	length = 4;
	
	if (*BufSize < 1)
		return SpStatBufferTooSmall;

	tempStr[0] = (char) (value >> 24);
	tempStr[1] = (char) (value >> 16);
	tempStr[2] = (char) (value >> 8);
	tempStr[3] = (char) (value);
	tempStr[4] = '\0';

	if (length >= *BufSize)
	{
		length = *BufSize - 1;
		spStat = SpStatBufferTooSmall;
	}
			
	strncpy(Buffer, tempStr, length);
	Buffer += length;
	*Buffer = 0;
	*BufSize = length;

	return spStat;
}

/************************************************************
 * FUNCTION NAME
 *	F15d16ToTxt
 *
 * DESCRIPTION
 *	This function converts a KpF15d16_t value into a string. 
 * *BufSize contains the size of the Buffer into which the string 
 * will be written. The actual string size is returned in BufSize.
 *
 ************************************************************/
 SpStatus_t F15d16ToTxt(KpF15d16_t	value, 
			KpInt32_p	BufSize,  
			KpChar_p 	Buffer)
{
char		DefStr[] = "0.000000";
char		OneStr[] = "1.000000";
KpUInt32_t	Ivalue;
KpInt32_t	length;
SpStatus_t	spStat = SpStatSuccess;
KpTChar_t	tempString[32];
KpInt32_t	DecStart, NumChars;

	if (*BufSize < 1)
		return SpStatBufferTooSmall;

	/* value must be divided by 2**16 to become a double 
	 * then round up for trunctation to integer and multiple
	 * to get the decimal part into the integer range. */
	Ivalue = (KpUInt32_t)((KpF15d16ToDouble(value) + 0.0000005) 
                               * 1000000); 
	/* Make room for null terminator */
	*BufSize -= 1;
	/* Max characters are 8 plus null terminator */
	if (*BufSize > 8)
		*BufSize = 8;

	/* Can exceed due to round up */
	if (Ivalue >= 1000000)
	{
		strncpy(Buffer, OneStr, *BufSize);
		Buffer += *BufSize;
		*Buffer = 0;
	} else
	{
		/* Fill in leading zeros */
		strncpy(Buffer, DefStr, *BufSize);

		/* Do not want sign */
		Ultoa (Ivalue, tempString, 10);
		length = strlen(tempString);

		/* Find first non-zero char location */
		DecStart = 8 - length;
		/* Move past 0. and leading 0's */
		if (DecStart < *BufSize)
		{
			Buffer += DecStart;
			/* find number of non-zero chars 
			   that fit into buffer */
			NumChars = *BufSize - DecStart;
			strncpy(Buffer, tempString, NumChars);
			Buffer += NumChars;
		} else
			Buffer += *BufSize;

		*Buffer = 0;
	}

	return spStat;	
}

/************************************************************
 * FUNCTION NAME
 *	F15d16sToTxt
 *
 * DESCRIPTION
 *	This function converts an array of  KpF15d16_t values 
 * into a string. The values are separated by spaces. *BufSize 
 * contains the size of the Buffer into which the string will
 * be written. The actual string size is returned in BufSize.
 *
 ************************************************************/
 SpStatus_t F15d16sToTxt(KpUInt32_t	count, 
			 KpF15d16_t	FAR *Values, 
			 KpInt32_p 	BufSize,  
			 KpChar_p 	Buffer)
{
unsigned long	i;
SpStatus_t	spStat = SpStatSuccess;
KpChar_p	Buf;
KpInt32_t	bufSize, remainingBufSize;
	
	/* init local variables */
	Buf = Buffer;
	remainingBufSize = bufSize = *BufSize;
	/* loop over each value */
	for (i = 0; i < count; i++, Values++) {
		if (0 != i)
		{
			if (remainingBufSize)
			{
				/* add space between values in string */
				strcpy (Buf, " ");
				Buf++;
				remainingBufSize--;
			}
			else
			{
				return SpStatBufferTooSmall;
			}
		}
		bufSize = remainingBufSize;
		if ((spStat = F15d16ToTxt (*Values, &bufSize,  Buf)) != SpStatSuccess)
			return spStat;
		Buf = Buf + bufSize;	/* advance buffer pointer */
		remainingBufSize = remainingBufSize - bufSize;
	}
		
	return spStat;	
}

/************************************************************
 * FUNCTION NAME
 *	F15d16XYZToTxt
 *
 * DESCRIPTION
 *	This function converts a KpF15d16XYZ_t value into a 
 * string. *BufSize contains the size of the Buffer into 
 * which the string will be written. The actual string size 
 * is returned in BufSize.
 *
 ************************************************************/
 SpStatus_t F15d16XYZToTxt(KpF15d16XYZ_t	value, 
			   KpInt32_p 		BufSize,  
			   KpChar_p 		Buffer)
{
SpStatus_t	SpStat = SpStatSuccess;
KpChar_p	Buf;
KpInt32_t	bufSize, remainingBufSize;
KpF15d16_t	tempValue;
	
	/* init local variables */
	Buf = Buffer;
	remainingBufSize = bufSize = *BufSize;
	/* loop over each value (XYZ)*/
	tempValue = value.X;
	
	if ((SpStat = F15d16ToTxt (tempValue, &bufSize,  Buf)) != SpStatSuccess)
		return SpStat;
	remainingBufSize = remainingBufSize - bufSize;
	Buf = Buf + bufSize;

	if (remainingBufSize)
	{
		strcpy(Buf, " ");
		Buf++;
		remainingBufSize--;
	}
	else
	{
		return SpStatBufferTooSmall;
	}

	tempValue = value.Y;
	bufSize = remainingBufSize;
	if ((SpStat = F15d16ToTxt (tempValue, &bufSize,  Buf)) != SpStatSuccess)
		return SpStat;
	remainingBufSize = remainingBufSize - bufSize;
	Buf = Buf + bufSize;

	if (remainingBufSize)
	{
		strcpy(Buf, " ");
		Buf++;
		remainingBufSize--;
	}
	else
	{
		return SpStatBufferTooSmall;
	}
		
	tempValue = value.Z;
	bufSize = remainingBufSize;
	if ((SpStat = F15d16ToTxt (tempValue, &bufSize,  Buf)) != SpStatSuccess)
		return SpStat;
	remainingBufSize = remainingBufSize - bufSize;
	*BufSize = *BufSize - remainingBufSize;
	return SpStat;

}
/************************************************************
 * FUNCTION NAME
 *	Ultoa
 *
 * DESCRIPTION
 *	This function converts a UInt32 value into a string. 
 *
 ************************************************************/
KpTChar_p Ultoa(KpUInt32_t	Value, 
		KpTChar_p	String, 
		int		Radix)
{
#if defined (KPWIN)
	return _ultoa (Value, String, Radix);
#else
char	FAR *Ptr;
int	i, j;
char	c;

	if (16 == Radix)
	{
/*		sprintf (String, "%uld", Value);  Not supported on 68*/
/* Leave until kcms_sys has a KpUItoa routine */
		Ptr = String;
		do {
			if (Value % 16 > 9)
				*Ptr++ = (char) (Value % 16 - 9 + 'A');
			else
				*Ptr++ = (char) (Value % 16 + '0');
		} while ((Value /= 16) > 0);
		*Ptr = '\0';
 
		j = (int)strlen (String) - 1;
		for (i = 0; i < j; i++, j--) {
			c = String [i];
			String [i] = String [j];
			String [j] = c;
		}

	}
	else
		KpItoa (Value, String);

	return String;
#endif
}
/************************************************************
 * FUNCTION NAME
 *	UInt32ToTxt
 *
 * DESCRIPTION
 *	This function converts a UInt32 value into a string. 
 * *BufSize contains the size of the Buffer into which the 
 * string will be written. The actual string size is returned 
 * in BufSize.
 *
 ************************************************************/
SpStatus_t UInt32ToTxt( KpUInt32_t	Value, 
			KpInt32_p 	BufSize,  
			KpChar_p 	Buffer)
{
	KpInt32_t	length;
	SpStatus_t	spStat = SpStatSuccess;
	char		tempBuf [12];

	if (*BufSize < 1)
		return SpStatBufferTooSmall;

	Ultoa (Value, tempBuf, 10);
	
	length = strlen(tempBuf);
	if (length >= *BufSize)
	{
		length = *BufSize - 1;
		spStat = SpStatBufferTooSmall;
	}
			
	strncpy(Buffer, tempBuf, length);
	Buffer += length;
	*Buffer = 0;
	*BufSize = length;

	return spStat;	
}
/************************************************************
 * FUNCTION NAME
 *	UInt32ToHexTxt
 *
 * DESCRIPTION
 *	This function converts a UInt32 value into a string. 
 *	The format oif the string is "0x012345"
 * *BufSize contains the size of the Buffer into which the 
 * string will be written. The actual string size is returned 
 * in BufSize.
 *
 ************************************************************/
static SpStatus_t UInt32ToHexTxt( KpUInt32_t	Value, 
			KpInt32_p 	BufSize,  
			KpChar_p 	Buffer)
{
	KpInt32_t	length;
	SpStatus_t	spStat = SpStatSuccess;
	char		tempBuf [12];

	if (*BufSize < 1)
		return SpStatBufferTooSmall;

	KpLtos(Value, tempBuf);

	length = strlen(tempBuf);
	if (length+3  >= *BufSize)
	{
		length = *BufSize - 3;
		spStat = SpStatBufferTooSmall;
	}
			
	if (length > 0 ) {
		strcpy (Buffer, "0x");
		Buffer += 2;
		strncpy(Buffer, tempBuf, length);
		*BufSize = length + 2;
	}
	else
	{
		length = *BufSize - 1;
		strncpy(Buffer, "0x", length);
		*BufSize = length;
	}
	Buffer += length;
	*Buffer = 0;

	return spStat;	
}
/************************************************************
/************************************************************
 * FUNCTION NAME
 *	pfUInt32sToTxt
 *
 * DESCRIPTION
 *	This function converts an array of UInt32 values into 
 * a string. The count arg specifies how many UInt32s there are. 
 * The string has a space between UInt32 values. *BufSize contains 
 * the size of the Buffer into which the string will be written. 
 * The actual string size is returned in BufSize.
 *
 ************************************************************/

SpStatus_t UInt32sToTxt(KpUInt32_t	count,
			KpUInt32_t 	FAR *Values, 
			KpInt32_p 	BufSize,  
			KpChar_p 	Buffer)
{
	unsigned long	i;
	SpStatus_t	SpStat = SpStatSuccess;
	KpChar_p	Buf;
	KpInt32_t	bufSize, remainingBufSize;
	
	/* init local variables */
	Buf = Buffer;
	remainingBufSize = bufSize = *BufSize;
	/* loop over each value */
	for (i = 0; i < count; i++, Values++) {
		if (0 != i)
		{
			if (remainingBufSize)
			{
				/* add space between values in string */
				strcpy (Buf, " ");
				Buf++;
				remainingBufSize--;
			}
			else
			{
				return SpStatBufferTooSmall;
			}
		}
		bufSize = remainingBufSize;
		if ((SpStat = UInt32ToTxt (*Values, &bufSize,  Buf)) != SpStatSuccess)
			return SpStat;
		Buf = Buf + bufSize;	/* advance buffer pointer */
		remainingBufSize = remainingBufSize - bufSize;
	}
	return SpStat;
}
#endif



