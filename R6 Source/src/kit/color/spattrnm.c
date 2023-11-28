/*********************************************************************/
/*
	Contains:	This module contains tables for attribute translation.

				Created by lsh, September 16, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, 
                            all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPATTRNM.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spattrnm.c_v  $
		$Revision:   2.1  $
		$Date:   04 Apr 1994 09:35:46  $
		$Author:   lsh  $

	SCCS Revision:
		@(#)spattrnm.c	1.17 4/3/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** PROPRIETARY NOTICE:     The  software  information   contained ***
 *** herein is the  sole property of  Eastman Kodak Company  and is ***
 *** provided to Eastman Kodak users under license for use on their ***
 *** designated  equipment  only.  Reproduction of  this matter  in ***
 *** whole  or in part  is forbidden  without the  express  written ***
 *** consent of Eastman Kodak Company.                              ***
 ***                                                                ***
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1997                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"
#include <stdio.h>
#include <string.h>

#if defined (KPWIN32)
extern HANDLE SpHInst;
#endif


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Tables of enumeration values to strings.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/

#define SPEnumEntry(nm) {#nm, Sp##nm}

typedef struct {
	char			*Name;
	SpTagEnum_t		TagEnum;
} SpTagEnumTbl_t;

static SpTagEnumTbl_t TagEffectTypeTbl [] = {
	SPEnumEntry (EffectInput),
	SPEnumEntry (EffectOutput),
	SPEnumEntry (EffectAdapt),
	SPEnumEntry (EffectTone),
	SPEnumEntry (EffectGreyBalance),
	SPEnumEntry (EffectSelColor),
	SPEnumEntry (EffectNegToPos),
	SPEnumEntry (EffectMPA),
};

static SpTagEnumTbl_t TagIllumTbl [] = {
	SPEnumEntry (KIllumD50Flourescent),
	SPEnumEntry (KIllumD65Flourescent),
	SPEnumEntry (KIllumTungsten),
	SPEnumEntry (KIllumD50),
	SPEnumEntry (KIllumD65),
	SPEnumEntry (KIllumD93),
	SPEnumEntry (KIllumF2),
	SPEnumEntry (KIllumIncandescent),
};

static SpTagEnumTbl_t TagInterpretationTbl [] = {
	SPEnumEntry (InterpPerceptual),
	SPEnumEntry (InterpColormetric),
	SPEnumEntry (InterpPhotoCD),
	SPEnumEntry (InterpNegative),
};

static SpTagEnumTbl_t TagMediumTbl [] = {
	SPEnumEntry (MediumReflective),
	SPEnumEntry (MediumTransmissive),
};

static SpTagEnumTbl_t TagMediumSenseTbl [] = {
	SPEnumEntry (MediumSensePositive),
	SPEnumEntry (MediumSenseNegative),
};

static SpTagEnumTbl_t TagSenseInvertibleTbl [] = {
	SPEnumEntry (SenseIsInvertible),
	SPEnumEntry (SenseNotInvertible),
};

static SpTagEnumTbl_t TagMediumProductTbl [] = {
	SPEnumEntry (MediumKodaChrome),
	SPEnumEntry (MediumEktaColor),
	SPEnumEntry (MediumEktaChrome),
	SPEnumEntry (MediumFujiChrome),
	SPEnumEntry (MediumEktaTherm),
	SPEnumEntry (MediumPaper),
	SPEnumEntry (MediumPhotoPrint),
	SPEnumEntry (MediumTransparency),
	SPEnumEntry (MediumColorNegative),
};

static SpTagEnumTbl_t TagLinearizedTbl [] = {
	SPEnumEntry (IsLinearized),
	SPEnumEntry (NotLinearized),
};

static SpTagEnumTbl_t TagPhosphorTbl [] = {
	SPEnumEntry (PhosphorP22),
	SPEnumEntry (PhosphorEBU),
	SPEnumEntry (PhosphorSMPTE_C),
};

static SpTagEnumTbl_t TagPrtBlackShapeTbl [] = {
	SPEnumEntry (BlackShapeAggressive),
	SPEnumEntry (BlackShapeNormal),
};

static SpTagEnumTbl_t TagPrtBlackStartDelayTbl [] = {
	SPEnumEntry (BlackDelayShort),
	SPEnumEntry (BlackDelayMedium),
	SPEnumEntry (BlackDelayLong),
};


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Table of attribute Ids to enumeration tables.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
#define SpEffectBase		12200
#define SpIllumBase		12220
#define SpInterpBase		12240
#define SpMediumBase		12260
#define SpMedSenseBase		12270
#define SpInvertBase		12280
#define SpMedProdBase		12290
#define SpLineBase		12310
#define SpPhosphBase		12320
#define SpShapeBase		12330
#define SpStartBase		12340
#define SpStartTagIDs		12400
#define SpStartTagTypes		12700

#define SPTagId2EnumEntry(nm, tbl, indx) {SpTag##nm, tbl, SPNumElem(tbl), indx}

typedef struct {
	SpTagId_t	TagId;
	SpTagEnumTbl_t	*EnumTbl;
	int		Size;
	int		Base;
} SpTagId2EnumTbl_t;

static SpTagId2EnumTbl_t TagId2EnumTbl [] = {
	SPTagId2EnumEntry (KEffectType,		TagEffectTypeTbl,	SpEffectBase),
	SPTagId2EnumEntry (KIllum,		TagIllumTbl,		SpIllumBase),
	SPTagId2EnumEntry (KInterpretation,	TagInterpretationTbl,	SpInterpBase),
	SPTagId2EnumEntry (KLinearized,		TagLinearizedTbl,	SpLineBase),
	SPTagId2EnumEntry (KMedium,		TagMediumTbl,		SpMediumBase),
	SPTagId2EnumEntry (KMediumProduct,	TagMediumProductTbl,	SpMedProdBase),
	SPTagId2EnumEntry (KMediumSense,	TagMediumSenseTbl,	SpMedSenseBase),
	SPTagId2EnumEntry (KPhosphor,		TagPhosphorTbl,		SpPhosphBase),
	SPTagId2EnumEntry (KPrtBlackShape,	TagPrtBlackShapeTbl,	SpShapeBase),
	SPTagId2EnumEntry (KPrtBlackStartDelay,	TagPrtBlackStartDelayTbl, SpStartBase),
	SPTagId2EnumEntry (KSenseInvertible,	TagSenseInvertibleTbl,	SpInvertBase),

	SPTagId2EnumEntry (KXchIllum,		TagIllumTbl,		SpIllumBase),
	SPTagId2EnumEntry (KXchInterpretation,	TagInterpretationTbl,	SpInterpBase),
	SPTagId2EnumEntry (KXchLinearized,	TagLinearizedTbl,	SpLineBase),
	SPTagId2EnumEntry (KXchMedium,		TagMediumTbl,		SpMediumBase),
	SPTagId2EnumEntry (KXchMediumProduct,	TagMediumProductTbl,	SpMedProdBase),
	SPTagId2EnumEntry (KXchMediumSense,	TagMediumSenseTbl,	SpMedSenseBase),
	SPTagId2EnumEntry (KXchPhosphor,	TagPhosphorTbl,		SpPhosphBase),
	SPTagId2EnumEntry (KXchSenseInvertible,	TagSenseInvertibleTbl,	SpInvertBase),
};


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Find Enumeration table given a TagId.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 22, 1993
 *------------------------------------------------------------------*/
static SpTagId2EnumTbl_t FAR *SpTagEnumFindByTagId (
				SpTagId_t	TagId)
{
	int					i;
	SpTagId2EnumTbl_t	FAR *Entry;

/* look for enumeration table */
	for (i = 0, Entry = TagId2EnumTbl;
			i < SPNumElem (TagId2EnumTbl);
					i++, Entry++) {

	/* if found, exit loop */
		if (TagId == Entry->TagId)
			return Entry;
	}

	return NULL;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert an attribute enumeration value to a text string.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagGetEnumName (
				SpTagId_t	TagId,
				SpTagEnum_t	TagEnum,
				size_t		BufferSize,
				char		FAR *Buffer)
{
	int					i;
	SpTagId2EnumTbl_t	FAR *Entry;
	SpTagEnumTbl_t		FAR *EnumEntry;

#if defined (KPMAC) || defined (KPWIN)
	KpChar_t		FAR ErrMsg[256];
#endif
#if defined (KPWIN)
	WORD			res_code;
#endif

	*Buffer = '\0';

/* look for enumeration table */
	Entry = SpTagEnumFindByTagId (TagId);
	if (NULL == Entry)
		return SpStatBadTagId;

/* look for enumeration value */
	for (i = 0, EnumEntry = Entry->EnumTbl;
			i < Entry->Size;
					i++, EnumEntry++) {

	/* if found, exit loop */
		if (EnumEntry->TagEnum == TagEnum) {

		/* give caller the name */
#if defined (KPMAC)
			GetIndString ((StringPtr)ErrMsg, Entry->Base, i+1);
			if (0 == ErrMsg[0])
			{
				if (!SpStrAppend (BufferSize, 
					Buffer, EnumEntry->Name))
					return SpStatBufferTooSmall;
			} else
			{	p2cstr ((StringPtr)ErrMsg);
				if (!SpStrAppend (BufferSize, Buffer,
						  ErrMsg))
					return SpStatBufferTooSmall;
			}
#elif defined (KPWIN)
			res_code = (WORD)(Entry->Base + i);
			if (0 == LoadString(SpHInst, res_code,
					(LPSTR)ErrMsg, 255))
				return SpStatBadTagType;
			if (!SpStrAppend (BufferSize, Buffer,
					  ErrMsg))
				return SpStatBufferTooSmall;
#else
			if (!SpStrAppend (BufferSize, Buffer, EnumEntry->Name))
				return SpStatBufferTooSmall;
#endif
			return SpStatSuccess;
		}
	}

	return SpStatOutOfRange;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get number of entries in enumeration table for given ID.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 21, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagEnumListSize (
				SpTagId_t	TagId,
				KpInt32_t	FAR *Size)
{
	SpTagId2EnumTbl_t	FAR *Entry;


	Entry = SpTagEnumFindByTagId (TagId);
	if (NULL == Entry) {
		*Size = 0;
		return SpStatBadTagId;
	}

	*Size = Entry->Size;
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get entry from enumeration table for given ID.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 21, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagEnumListGetEntry (
				SpTagId_t	TagId,
				KpInt32_t	n,
				SpTagEnum_t	FAR *TagEnum)
{
	SpTagId2EnumTbl_t	FAR *Entry;

	*TagEnum = SpEnumUnknown;

	Entry = SpTagEnumFindByTagId (TagId);
	if (NULL == Entry)
		return SpStatBadTagId;

	if (Entry->Size <= n)
		return SpStatOutOfRange;

	*TagEnum = Entry->EnumTbl [n].TagEnum;
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Table of attribute Ids to strings.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/

#define SPTagIdEntry(nm) {SpTag##nm, #nm}

typedef struct {
	SpTagId_t		TagId;
	char			*Name;
} SpTagIdTbl_t;

static SpTagIdTbl_t TagIdTbl [] = {

/* Kodak private tags */
	SPTagIdEntry (Private),
	SPTagIdEntry (KDeviceBits),
	SPTagIdEntry (KDeviceSerialNum),
	SPTagIdEntry (KDeviceSettings),
	SPTagIdEntry (KDeviceUnit),
	SPTagIdEntry (KDMax),
	SPTagIdEntry (KEffectType),
	SPTagIdEntry (KIllum),
	SPTagIdEntry (KInterpretation),
	SPTagIdEntry (KLinearizationType),
	SPTagIdEntry (KLinearized),
	SPTagIdEntry (KMedium),
	SPTagIdEntry (KMediumDesc),
	SPTagIdEntry (KMediumProduct),
	SPTagIdEntry (KMediumSense),
	SPTagIdEntry (KDotGain25),
	SPTagIdEntry (KDotGain50),
	SPTagIdEntry (KDotGain75),
	SPTagIdEntry (KPhosphor),
	SPTagIdEntry (KPrtBlackShape),
	SPTagIdEntry (KPrtBlackStartDelay),
	SPTagIdEntry (KSenseInvertible),
	SPTagIdEntry (KVersion),
	SPTagIdEntry (KDensityType),

/* Kodak private tags for device to device profiles */
	SPTagIdEntry (KXchDeviceBits),
	SPTagIdEntry (KXchDeviceSerialNum),
	SPTagIdEntry (KXchDeviceSettings),
	SPTagIdEntry (KXchDeviceUnit),
	SPTagIdEntry (KXchGamma),
	SPTagIdEntry (KXchIllum),
	SPTagIdEntry (KXchInterpretation),
	SPTagIdEntry (KXchLinearizationType),
	SPTagIdEntry (KXchLinearized),
	SPTagIdEntry (KXchMedium),
	SPTagIdEntry (KXchMediumDesc),
	SPTagIdEntry (KXchMediumProduct),
	SPTagIdEntry (KXchMediumSense),
	SPTagIdEntry (KXchPhosphor),
	SPTagIdEntry (KXchSenseInvertible),
	SPTagIdEntry (KXchDotGain25),
	SPTagIdEntry (KXchDotGain50),
	SPTagIdEntry (KXchDotGain75),

/* Kodak private tags for chaining rules */
	SPTagIdEntry (KChainAToB0),
	SPTagIdEntry (KChainBToA0),
	SPTagIdEntry (KChainPreview0),

	SPTagIdEntry (KChainAToB1),
	SPTagIdEntry (KChainBToA1),
	SPTagIdEntry (KChainPreview1),

	SPTagIdEntry (KChainAToB2),
	SPTagIdEntry (KChainBToA2),
	SPTagIdEntry (KChainPreview2),


/* Public tags */
	SPTagIdEntry (AToB0),
	SPTagIdEntry (AToB1),
	SPTagIdEntry (AToB2),
	SPTagIdEntry (BlueColorant),
	SPTagIdEntry (BlueTRC),
	SPTagIdEntry (BToA0),
	SPTagIdEntry (BToA1),
	SPTagIdEntry (BToA2),
	SPTagIdEntry (CalibrationDateTime),
	SPTagIdEntry (CharTarget),
	SPTagIdEntry (CopyRight),
	SPTagIdEntry (DeviceMfgDesc),
	SPTagIdEntry (DeviceModelDesc),
	SPTagIdEntry (Gamut),
	SPTagIdEntry (GrayTRC),
	SPTagIdEntry (GreenColorant),
	SPTagIdEntry (GreenTRC),
	SPTagIdEntry (Luminance),
	SPTagIdEntry (Measurement),
	SPTagIdEntry (MediaBlackPnt),
	SPTagIdEntry (MediaWhitePnt),
	SPTagIdEntry (NamedColor),
	SPTagIdEntry (Preview0),
	SPTagIdEntry (Preview1),
	SPTagIdEntry (Preview2),
	SPTagIdEntry (ProfileDesc),
	SPTagIdEntry (ProfileSeqDesc),
	SPTagIdEntry (PS2CRD0),
	SPTagIdEntry (PS2CRD1),
	SPTagIdEntry (PS2CRD2),
	SPTagIdEntry (PS2CRD3),
	SPTagIdEntry (PS2CSA),
	SPTagIdEntry (PS2RenderIntent),
	SPTagIdEntry (RedColorant),
	SPTagIdEntry (RedTRC),
	SPTagIdEntry (ScreeningDesc),
	SPTagIdEntry (Screening),
	SPTagIdEntry (Technology),
	SPTagIdEntry (Ucrbg),
	SPTagIdEntry (ViewingCondDesc),
	SPTagIdEntry (ViewingConditions),
	SPTagIdEntry (NamedColor2),

/* Kodak private tag for profile history tracking */
	SPTagIdEntry (KProfileHistory),

/* Kodak private tag for PhotoCD information */
	SPTagIdEntry (KPCDFilmTerm),

/* Kodak private tags for multi-channel profiles */
	SPTagIdEntry (KInkName0),
	SPTagIdEntry (KInkName1),
	SPTagIdEntry (KInkName2),
	SPTagIdEntry (KInkName3),
	SPTagIdEntry (KInkName4),
	SPTagIdEntry (KInkName5),
	SPTagIdEntry (KInkName6),
	SPTagIdEntry (KInkName7),
	SPTagIdEntry (KInkDensities),
	SPTagIdEntry (CrdInfo),
	SPTagIdEntry (KChainGamut),
	SPTagIdEntry (ENDLIST),
};


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get number of entries in ID table.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 21, 1993
 *------------------------------------------------------------------*/
KpInt32_t SPAPI SpTagIdListSize (
				void)
{
	return (SPNumElem (TagIdTbl) - 1);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get entry from ID table.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 21, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagIdListGetEntry (
				KpInt32_t	n,
				SpTagId_t	FAR *TagId)
{
	if (SpTagIdListSize () <= n) {
		*TagId = SpTagUnknown;
		return SpStatOutOfRange;
	}

	*TagId = TagIdTbl [n].TagId;
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert an attribute ID to a text string.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagGetIdName (
				SpTagId_t		TagId,
				size_t			BufferSize,
				char			FAR *Buffer)
{
	int		i, EntryValue;
	SpTagIdTbl_t	*Entry;

#if defined (KPMAC) || defined (KPWIN)
	KpChar_t		FAR ErrMsg[256];
#endif
#if defined (KPWIN)
	WORD			res_code;
#endif

	*Buffer = '\0';
	EntryValue = 0; /* Maps to private */

/* look for ID */
	for (i = 1, Entry = TagIdTbl;
	     SpTagENDLIST != Entry->TagId;
	     i++, Entry++) {
		if (TagId == Entry->TagId) {
			EntryValue = i - 1;
			break;
		}
	}

	/* give caller the name */
#if defined (KPMAC)
	GetIndString ((StringPtr)ErrMsg, SpStartTagIDs, EntryValue+1);
	if (0 == ErrMsg[0])
	{
		if (EntryValue == 0)
			Entry = TagIdTbl; /* Get it back to Private setting */
		if (!SpStrAppend (BufferSize, Buffer, Entry->Name))
			return SpStatBufferTooSmall;
	} else
	{	p2cstr ((StringPtr)ErrMsg);
		if (!SpStrAppend (BufferSize, Buffer,
				  ErrMsg))
			return SpStatBufferTooSmall;
	}
#elif defined (KPWIN)
	res_code = (WORD)(SpStartTagIDs + EntryValue);
	if (0 == LoadString(SpHInst, res_code,
			(LPSTR)ErrMsg, 255))
		return SpStatBadTagType;
	if (!SpStrAppend (BufferSize, Buffer,
			  ErrMsg))
		return SpStatBufferTooSmall;
#else
	if (EntryValue == 0)
		Entry = TagIdTbl; /* Get it back to Private setting */
	if (!SpStrAppend (BufferSize, Buffer, Entry->Name))
		return SpStatBufferTooSmall;
#endif

	return SpStatSuccess;

}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Table of attribute type enums to strings.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/

typedef struct {
	char		FAR *Name;
	SpTagType_t	TagType;
} SpTagTypeTbl_t;

#define SPTagTypeEntry(nm) {#nm, Sp_AT_##nm}

static SpTagTypeTbl_t TagTypeTbl [] = {
	SPTagTypeEntry (Unknown),
	SPTagTypeEntry (Enum),

	SPTagTypeEntry (Curve),
	SPTagTypeEntry (Data),
	SPTagTypeEntry (DateTime),
	SPTagTypeEntry (Lut),
	SPTagTypeEntry (Measurement),
	SPTagTypeEntry (NamedColors),
	SPTagTypeEntry (ProfileSeqDesc),
	SPTagTypeEntry (SF15d16),
	SPTagTypeEntry (Screening),
	SPTagTypeEntry (Signature),
	SPTagTypeEntry (Text),
	SPTagTypeEntry (TextDesc),
	SPTagTypeEntry (UF16d16),
	SPTagTypeEntry (Ucrbg),
	SPTagTypeEntry (UInt16),
	SPTagTypeEntry (UInt32),
	SPTagTypeEntry (UInt64),
	SPTagTypeEntry (UInt8),
	SPTagTypeEntry (Viewing),
	SPTagTypeEntry (XYZ),
	SPTagTypeEntry (NamedColors2),
	SPTagTypeEntry (CrdInfo),

	SPTagTypeEntry (ENDLIST),
};


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get number of entries in TagType table.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 21, 1993
 *------------------------------------------------------------------*/
KpInt32_t SPAPI SpTagTypeListSize (
				void)
{
	return (SPNumElem (TagTypeTbl) - 1);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get entry from TagType table.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 21, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagTypeListGetEntry (
				KpInt32_t	n,
				SpTagType_t	FAR *TagType)
{
	if (SpTagTypeListSize () <= n) {
		*TagType = Sp_AT_Unknown;
		return SpStatOutOfRange;
	}

	*TagType = TagTypeTbl [n].TagType;
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert an attribute type to a text string.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagGetTypeName (
				SpTagType_t	TagType,
				size_t		BufferSize,
				char		FAR *Buffer)
{
	SpTagTypeTbl_t	FAR *TypeEntry;

	int		i, EntryValue;
#if defined (KPMAC) || defined (KPWIN)
	KpChar_t	FAR ErrMsg[256];
#endif
#if defined (KPWIN)
	WORD		res_code;
#endif

	*Buffer = '\0';
	EntryValue = 0; /* Maps to Unknown */

/* look for attribute type value */
	for (TypeEntry = TagTypeTbl, i = 0;
			Sp_AT_ENDLIST != TypeEntry->TagType;
					TypeEntry++, i++) {

	/* if found, exit loop */
		if (TypeEntry->TagType == TagType) {
			EntryValue = i;
			break;
		}
	}

/* give caller the name */
#if defined (KPMAC)
	GetIndString ((StringPtr)ErrMsg, SpStartTagTypes, EntryValue+1);
	if (0 == ErrMsg[0])
	{
		if (EntryValue == 0)
			TypeEntry = TagTypeTbl; /* Get back to Unknown */
		if (!SpStrAppend (BufferSize, Buffer, TypeEntry->Name))
			return SpStatBufferTooSmall;
	} else
	{	p2cstr ((StringPtr)ErrMsg);
		if (!SpStrAppend (BufferSize, Buffer,
				  ErrMsg))
			return SpStatBufferTooSmall;
	}
#elif defined (KPWIN)
	res_code = (WORD)(SpStartTagTypes + EntryValue);
	if (0 == LoadString(SpHInst, res_code,
			(LPSTR)ErrMsg, 255))
		return SpStatBadTagType;
	if (!SpStrAppend (BufferSize, Buffer,
			  ErrMsg))
		return SpStatBufferTooSmall;
#else
	if (EntryValue == 0)
		TypeEntry = TagTypeTbl; /* Get back to Unknown */
	if (!SpStrAppend (BufferSize, Buffer, TypeEntry->Name))
		return SpStatBufferTooSmall;
#endif
	return SpStatSuccess;
}

