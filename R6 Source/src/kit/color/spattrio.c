/*********************************************************************/
/*
	Contains:	This module contains functions for converting attributes
				between internal and external format.

				Created by lsh, September 15, 1993

	** Note:	"internal" form = ICC format
				"public"   form = Sprofile library format

				Example: SpTagFromPublic() converts from the
				Sprofile library format to the ICC v3.0 format.

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPATTRIO.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spattrio.c_v  $
		$Revision:   2.1  $
		$Date:   04 Apr 1994 09:35:34  $
		$Author:   lsh  $

	SCCS Revision:
	 @(#)spattrio.c	1.67	12/22/97

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


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert a Type Signature to a Type Id.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 21, 1994
 *------------------------------------------------------------------*/
static SpTagType_t SpAttrTypeSigToTypeId (
				SpSig_t	TagTypeSig)
{
	switch (TagTypeSig) {
	case SpTypeCurve:
		return Sp_AT_Curve;

	case SpTypeData:
		return Sp_AT_Data;

	case SpTypeDateTime:
		return Sp_AT_DateTime;

	case SpTypeLut16:
		return Sp_AT_Lut;

	case SpTypeLut8:
		return Sp_AT_Lut;

	case SpTypeMeasurement:
		return Sp_AT_Measurement;

#if !defined (SP_NO_OBSOLETE)
	case SpTypeNamedColors:
		return Sp_AT_NamedColors;
#endif

	case SpTypeNamedColors2:
		return Sp_AT_NamedColors2;

	case SpTypeProfileSeqDesc:
		return Sp_AT_ProfileSeqDesc;

	case SpTypeSF15d16:
		return Sp_AT_SF15d16;

	case SpTypeScreening:
		return Sp_AT_Screening;

	case SpTypeSignature:
		return Sp_AT_Signature;

	case SpTypeText:
		return Sp_AT_Text;

	case SpTypeTextDesc:
		return Sp_AT_TextDesc;

	case SpTypeUF16d16:
		return Sp_AT_UF16d16;

	case SpTypeUcrbg:
		return Sp_AT_Ucrbg;

	case SpTypeUInt16:
		return Sp_AT_UInt16;

	case SpTypeUInt32:
		return Sp_AT_UInt32;

	case SpTypeUInt64:
		return Sp_AT_UInt64;

	case SpTypeUInt8:
		return Sp_AT_UInt8;

	case SpTypeViewing:
		return Sp_AT_Viewing;

	case SpTypeXYZ:
		return Sp_AT_XYZ;

	case SpTypeCrdInfo:
		return Sp_AT_CrdInfo;
	}

	return Sp_AT_Unknown;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert array of 16 bit values from internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 17, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpArray16ToPublic (
				KpUInt32_t	BufferSize,
				char		KPHUGE *Buf,
				KpUInt32_t	FAR *Count,
				void		FAR * FAR *Values)
{
	KpUInt32_t	Index, Limit;
	KpUInt16_t	FAR *UInt16Ptr;

	if (BufferSize < 10)
		return SpStatBadTagData;

	Limit = (BufferSize - 8) / 2;
	UInt16Ptr = (KpUInt16_t *)SpMalloc ((KpInt32_t)sizeof (KpUInt16_t) * Limit);
	if (NULL == UInt16Ptr)
		return SpStatMemory;

	*Count = Limit;
	*Values = UInt16Ptr;

	for (Index = 0; Index < Limit; Index++)
		*UInt16Ptr++ = SpGetUInt16 (&Buf);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert array of 32 bit values from internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 17, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpArray32ToPublic (
				KpUInt32_t	BufferSize,
				char		KPHUGE *Buf,
				KpUInt32_t	FAR *Count,
				void		FAR * FAR *Values)
{
	KpUInt32_t	Index, Limit;
	KpUInt32_t	FAR *UInt32Ptr;

	if (BufferSize < 12)
		return SpStatBadTagData;

	Limit = (BufferSize - 8) / 4;
	UInt32Ptr = (KpUInt32_t *)SpMalloc ((KpInt32_t)sizeof (KpUInt32_t) * Limit);
	if (NULL == UInt32Ptr)
		return SpStatMemory;

	*Count = Limit;
	*Values = UInt32Ptr;

	for (Index = 0; Index < Limit; Index++)
		*UInt32Ptr++ = SpGetUInt32 (&Buf);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert array of 64 bit values from internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 2, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpArray64ToPublic (
				KpUInt32_t	BufferSize,
				char		KPHUGE *Buf,
				KpUInt32_t	FAR *Count,
				void		FAR * FAR *Values)
{
	KpUInt32_t	Index, Limit;
	SpUInt64_t	FAR *UInt64Ptr;

	if (BufferSize < 16)
		return SpStatBadTagData;

	Limit = (BufferSize - 8) / 8;
	UInt64Ptr = SpMalloc ((KpInt32_t)sizeof (SpUInt64_t) * Limit);
	if (NULL == UInt64Ptr)
		return SpStatMemory;

	*Count = Limit;
	*Values = UInt64Ptr;

	for (Index = 0; Index < Limit; Index++, UInt64Ptr++) {
		UInt64Ptr->hi = SpGetUInt32 (&Buf);
		UInt64Ptr->lo = SpGetUInt32 (&Buf);
	}

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert array of 8 bit values from internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 17, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpArray8ToPublic (
				KpUInt32_t	BufferSize,
				char		KPHUGE *Buf,
				KpUInt32_t	FAR *Count,
				void		FAR * FAR *Values)
{
	KpUInt32_t	Limit;
	char		FAR *UInt8Ptr;

	if (BufferSize < 9)
		return SpStatBadTagData;

	Limit = BufferSize - 8;
	UInt8Ptr = SpMalloc ((KpInt32_t)sizeof (char) * Limit);
	if (NULL == UInt8Ptr)
		return SpStatMemory;

	*Count = Limit;
	*Values = UInt8Ptr;

	SpGetBytes (&Buf, UInt8Ptr, Limit);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert TextDesc from internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 21, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpTextDescToPublic (
				SpStatus_t		Status,
				char			KPHUGE * FAR *Buf, 
				KpInt32_t		BufferSize,
				SpTextDesc_t	FAR *TextDesc)
{
	KpUInt32_t	Index, Limit;
	KpUInt16_t	KPHUGE *UInt16Ptr;
	char		KPHUGE *BytePtr;
	char		KPHUGE *BufSave;
	KpUInt16_t	ByteOrder;
	KpInt32_t	BytesUsed, TagSize;
	char		*BufEnd;
	unsigned char charCount;
	union {
		KpUInt16_t	Value;
		char		Bytes [2];
	} Swapper;
	
	if (Status != SpStatSuccess)
		return(Status);

/* initialize pointers */
	TextDesc->IsoStr = NULL;
	TextDesc->UniStr = NULL;
	TextDesc->MacCount = 0;
	TextDesc->MacScriptCode = 0;
	BufEnd = *Buf + BufferSize;
        for (Index = 0; Index < sizeof(TextDesc->MacStr); Index++)
	   TextDesc->MacStr[Index] = '\0';

	BytesUsed = 0;
	TagSize = 4; /* Min Tag Size for Ascii string count only */

	if (TagSize > BufferSize)
		return SpStatBadProfile;


/* get the IsoLatin1 string */
	/* Get the 7 bit ASCII profile description count */
	BytesUsed += 4;
	Limit = SpGetUInt32 (Buf);
	if (0 != Limit) {
		BytesUsed += (KpInt32_t)Limit;
		if ((TagSize += (KpInt32_t)Limit) > BufferSize)
			goto BadProfile;

		TextDesc->IsoStr = SpMalloc (Limit + 1);
		if (NULL == TextDesc->IsoStr)
			return SpStatMemory;
		
		/* Get the 7 bit ASCII profile description: Limit Bytes */
		/* Null Terminator should be counted */
		SpGetBytes (Buf, TextDesc->IsoStr, Limit);
		/* Make sure Null terminated */
		if (TextDesc->IsoStr [(int) Limit -1] != '\0')
			TextDesc->IsoStr [(int) Limit] = '\0';
	}

/* get the Unicode string */
	/* Get Unicode Language Code */
	TagSize += 8;         /* Min Tag Size after Type, Reserved 
				 and Ascii String */
	BytesUsed += 4;
	TextDesc->UniLangCode = SpGetUInt32 (Buf);

	/* Get Unicode profile description count */
	BytesUsed += 4;
	Limit = SpGetUInt32 (Buf);

	if (0 != Limit) {
		BytesUsed += (KpInt32_t)(Limit * 2);
		if ((TagSize += (KpInt32_t)(Limit * 2)) > BufferSize)
		{
			*Buf = BufEnd;
			return SpStatSuccess;
		}

		TextDesc->UniStr = SpMalloc (2 * (Limit + 1));
		if (NULL == TextDesc->UniStr) {
			SpFree (TextDesc->IsoStr);
			TextDesc->IsoStr = NULL;
			return SpStatMemory;
		}

		UInt16Ptr = TextDesc->UniStr;
		BufSave = *Buf;

		/* Get unicode byte order  */
		SpGetBytes (Buf, &Swapper.Bytes[0], 2);
		ByteOrder = Swapper.Value;
		switch (ByteOrder) {
		case 0xFEFF:
			Limit--;
			break;

		case 0xFFFE:
			Limit--;
			break;

		default:
			ByteOrder = 0xFEFF;
			*Buf = BufSave;
			break;
		}
		BytePtr = *Buf;

		/* Get unicode localizable description: Limit*2 Bytes 
		   or Limit words */
		for (Index = 0; Index < Limit; Index++) {
			if (0xFEFF == ByteOrder) {
				Swapper.Bytes [0] = *BytePtr++;
				Swapper.Bytes [1] = *BytePtr++;
			}
			else {
				Swapper.Bytes [1] = *BytePtr++;
				Swapper.Bytes [0] = *BytePtr++;
			}
			*UInt16Ptr++ = Swapper.Value;
		}
		/* If no null terminator, add one */
		if ((Swapper.Bytes[0] != Swapper.Bytes[1]) ||
		    (Swapper.Bytes[0] != 0))
			*UInt16Ptr++ = 0;

		*Buf = BytePtr;
	}

	TagSize += 70;  /* Add in the Mac size */
	if (TagSize > BufferSize)
	{
		*Buf = BufEnd;
		return SpStatSuccess;
	}

/* get the Mac string */
	/* Get script code: 2 Bytes */
	TextDesc->MacScriptCode = SpGetUInt16 (Buf);
	BytePtr = *Buf;

	/* Get localizable profile description count: 1 Byte */
	charCount = *BytePtr++;
	memcpy(&TextDesc->MacCount, &charCount, 1);
	if (sizeof (TextDesc->MacStr) < TextDesc->MacCount)
	{
		TextDesc->MacScriptCode = 0;
		TextDesc->MacCount = 0;
		*Buf = BufEnd;
		return SpStatSuccess;
	}

	*Buf = BytePtr;
	if (charCount != 0)
	{
		/* Get localizable profile description */
		SpGetBytes (Buf, TextDesc->MacStr, TextDesc->MacCount);
	}
	/* Move past MacStr in buffer */
	*Buf += sizeof(TextDesc->MacStr) - TextDesc->MacCount;
	return SpStatSuccess;

BadProfile:
	if (NULL != TextDesc->IsoStr)
		SpFree (TextDesc->IsoStr);
	TextDesc->IsoStr = NULL;
	if (NULL != TextDesc->UniStr)
		SpFree (TextDesc->UniStr);
	TextDesc->UniStr = NULL;
	
	return SpStatBadProfile;
}




/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Helper for SpProfileSeqDescToPublic.
 *
 * AUTHOR
 * 	gbp
 *
 * DATE CREATED
 *	26 May, 1995
 *------------------------------------------------------------------*/
static SpStatus_t SpTextDescTypeToPublic (
				char		KPHUGE **Buf, 
				KpInt32_t	BufferSize,
				SpTextDesc_t	FAR *TextDesc)
{
KpUInt32_t	dummy;
SpStatus_t	Status = SpStatSuccess;
KpInt32_t	BytesLeft;


	BytesLeft = 0;

	/* Min Text Type Size */
	if (90 > BufferSize)
		return SpStatBadProfile;


	dummy = SpGetUInt32 (Buf);	/* get type */
	if (SpTypeTextDesc != dummy) {
		Status = SpStatBadTagData;
	}

	dummy = SpGetUInt32 (Buf);	/* skip reserved */

	BytesLeft = BufferSize - 8;

	Status = SpTextDescToPublic (Status, Buf, BytesLeft, TextDesc);	/* get the text description */

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert ProfileSeqDesc from internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 21, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpProfileSeqDescToPublic (
				KpUInt32_t			BufferSize,
				char				KPHUGE *Buf,
				SpProfileSeqDesc_t	FAR *ProfileSeqDesc)
{
	SpStatus_t					Status;
	KpUInt32_t					Index;
	SpProfileSeqDescRecord_t	KPHUGE *Record;
	KpUInt32_t					BytesUsed, BytesLeft, ptrDiff;
	char						KPHUGE *BufCurrent;

	if (BufferSize < 12)
		return SpStatBadTagData;

	BytesUsed = 
	BytesLeft =
	ptrDiff	  = 0;


/* get number of description records */
	BytesUsed += 4;
	ProfileSeqDesc->Count = SpGetUInt32 (&Buf);
	ProfileSeqDesc->Records = NULL;

/* done if 0 records */
	if (0 == ProfileSeqDesc->Count)
		return SpStatSuccess;

/* allocate space for the records */
	ProfileSeqDesc->Records =
		SpMalloc (ProfileSeqDesc->Count * (KpInt32_t)sizeof (*ProfileSeqDesc->Records));
	if (NULL == ProfileSeqDesc->Records)
		return SpStatMemory;

/* get the individual records */
	Status = SpStatSuccess;
	for (Index = 0, Record = ProfileSeqDesc->Records;
			Index < ProfileSeqDesc->Count;
					Index++, Record++)
	{
		/* Min Size of ProfileDescr Structure */
		if ((BytesUsed + 200) > BufferSize)
		{
			SpFree(ProfileSeqDesc->Records);
			ProfileSeqDesc->Records = NULL;
			return SpStatBadTagData;
		}
		BytesUsed += 20;
		Record->DeviceManufacturer = SpGetUInt32 (&Buf);
		Record->DeviceModel = SpGetUInt32 (&Buf);
		Record->DeviceAttributes.hi = SpGetUInt32 (&Buf);
		Record->DeviceAttributes.lo = SpGetUInt32 (&Buf);
		Record->Technology = SpGetUInt32 (&Buf);

		/* Save current location so we know how big next 
		   Part is */
		BufCurrent = Buf;

		BytesLeft = BufferSize - BytesUsed;
		Status = SpTextDescTypeToPublic (&Buf, BytesLeft, &Record->DeviceManufacturerDesc);
		if (SpStatSuccess != Status)
			return (Status);

		ptrDiff = (KpInt32_t) (Buf - BufCurrent);
		if ((BytesUsed += ptrDiff) > BufferSize)
		{
			SpFree(ProfileSeqDesc->Records);
			ProfileSeqDesc->Records = NULL;
			return SpStatBadProfile;
		}

		/* Save again for size of next */
		BufCurrent = Buf;
		BytesLeft = BufferSize - BytesUsed;

		Status = SpTextDescTypeToPublic (&Buf, BytesLeft, &Record->DeviceModelDesc);
		if (SpStatSuccess != Status)
			return (Status);

		ptrDiff = (KpInt32_t) (Buf - BufCurrent);
		if ((BytesUsed += ptrDiff) > BufferSize)
		{
			SpFree(ProfileSeqDesc->Records);
			ProfileSeqDesc->Records = NULL;
			return SpStatBadProfile;
		}
	}

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a named color string. Maximum of 32 bytes, including '\0' terminator.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	Sept 16, 1996
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColor2GetName (
				KpUInt32_t		FAR *BufferSize,
				char			KPHUGE * FAR *Buf,
				char			FAR *Name)
{
	char		KPHUGE *BytePtr;
	int			Count;
	KpUInt32_t	Size;

	for (BytePtr = *Buf, Count = 0, Size = *BufferSize;
			(Size > 0) && (Count < 31) && ('\0' != *BytePtr);
					Count++, Size--) {
		*Name++ = *BytePtr++;
	}

	if ('\0' != *BytePtr)
		return SpStatBadTagData;

	*Name = *BytePtr++;
	*Buf += 32;
	*BufferSize -= 32;

	return SpStatSuccess;
}


#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a named color string. Maximum of 33 bytes, including '\0' terminator.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 7, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColorGetName (
				KpUInt32_t		FAR *BufferSize,
				char			KPHUGE * FAR *Buf,
				char			FAR *Name)
{
	char		KPHUGE *BytePtr;
	int			Count;
	KpUInt32_t	Size;

	for (BytePtr = *Buf, Count = 0, Size = *BufferSize;
			(Size > 0) && (Count < 31) && ('\0' != *BytePtr);
					Count++, Size--) {
		*Name++ = *BytePtr++;
	}

	if ('\0' != *BytePtr)
		return SpStatBadTagData;

	*Name = *BytePtr++;
	*Buf = BytePtr;
	*BufferSize = Size;

	return SpStatSuccess;
}

#endif

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a NamedColor2 record.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	Sept 16, 1996
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColor2GetRecord (
				KpUInt32_t		FAR *BufferSize,
				char			KPHUGE * FAR *Buf,
				int			DevCoords,
				SpNamedColor2_t	FAR *Color)
{
	SpStatus_t	Status;
	int		Channel;
	char		KPHUGE *BytePtr;

	Status = SpNamedColor2GetName (BufferSize, Buf, Color->Name);
	if (SpStatSuccess != Status)
		return Status;

	BytePtr = *Buf;
	for (Channel = 0; Channel < 3; Channel++)
		Color->Values [Channel] = SpGetUInt16(&BytePtr);

	for (Channel = 0; Channel < DevCoords; Channel++)
		Color->dValues [Channel] = SpGetUInt16(&BytePtr);

	*Buf = BytePtr;

	return SpStatSuccess;
}


#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a named color record.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 7, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColorGetRecord (
				KpUInt32_t		FAR *BufferSize,
				char			KPHUGE * FAR *Buf,
				int			DevCoords,
				SpNamedColor_t	FAR *Color)
{
	SpStatus_t	Status;
	int			Channel;
	char			KPHUGE *BytePtr;

	Status = SpNamedColorGetName (BufferSize, Buf, Color->Name);
	if (SpStatSuccess != Status)
		return Status;

	BytePtr = *Buf;
	/* Old Named Color only had Bytes of data */
	for (Channel = 0; Channel < DevCoords; Channel++)
		Color->Values [Channel] = *BytePtr++;

	*Buf = BytePtr;

	return SpStatSuccess;
}
#endif

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert NamedColor2 from internal form.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	Sept 16,1996
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColors2ToPublic (
				KpUInt32_t		BufferSize,
				char			KPHUGE *Buf,
				SpNamedColors2_t	FAR *NamedColors)
{
	SpStatus_t		Status;
	KpUInt32_t		Index;
	SpNamedColor2_t		FAR *NamedColor;

	if (BufferSize < 18)
		return SpStatBadTagData;

/* get named color 'header' data */
	NamedColors->VendorFlags = SpGetUInt32 (&Buf);
	NamedColors->Count = SpGetUInt32 (&Buf);
	NamedColors->DeviceChannels = SpGetUInt32 (&Buf);
	BufferSize -= 20; /* remove byte already used */

	NamedColors->Colors = NULL;

	Status = SpNamedColor2GetName (&BufferSize, &Buf, NamedColors->Prefix);
	if (SpStatSuccess != Status)
		return Status;

	Status = SpNamedColor2GetName (&BufferSize, &Buf, NamedColors->Suffix);
	if (SpStatSuccess != Status)
		return Status;

/* allocate and initialize named colors */
	NamedColor = SpMalloc (NamedColors->Count * 
			      (KpInt32_t)sizeof(SpNamedColor2_t));
	if (NULL == NamedColor)
		return SpStatMemory;

	NamedColors->Colors = NamedColor;
	KpMemSet (NamedColor, 0, NamedColors->Count * 
                                 (KpInt32_t)sizeof (*NamedColor));

/* now get each of the named color records */
	for (Index = 0; Index < NamedColors->Count; Index++, NamedColor++) {
		Status = SpNamedColor2GetRecord(&BufferSize, &Buf,
						NamedColors->DeviceChannels,
						NamedColor);
		if (SpStatSuccess != Status) {
			SpFree (NamedColors->Colors);
			NamedColors->Colors = NULL;
			return Status;
		}
	}

	return Status;
}


#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert named colors from internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 6, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColorsToPublic (
				SpHeader_t		FAR *Header,
				KpUInt32_t		BufferSize,
				char			KPHUGE *Buf,
				SpNamedColors_t	FAR *NamedColors)
{
	SpStatus_t		Status;
	KpUInt32_t		Index;
	SpNamedColor_t	FAR *NamedColor;

	if (BufferSize < 18)
		return SpStatBadTagData;

/* get named color 'header' data */
	NamedColors->VendorFlags = SpGetUInt32 (&Buf);
	NamedColors->Count = SpGetUInt32 (&Buf);
	NamedColors->DeviceChannels =
		SpGetChannelsFromColorSpace (Header->DataColorSpace);
	BufferSize -= 16;

	NamedColors->Colors = NULL;

	Status = SpNamedColorGetName (&BufferSize, &Buf, NamedColors->Prefix);
	if (SpStatSuccess != Status)
		return Status;

	Status = SpNamedColorGetName (&BufferSize, &Buf, NamedColors->Suffix);
	if (SpStatSuccess != Status)
		return Status;

/* allocate and initialize named colors */
	NamedColor = SpMalloc (NamedColors->Count * (KpInt32_t)sizeof(SpNamedColor_t));
	if (NULL == NamedColor)
		return SpStatMemory;

	NamedColors->Colors = NamedColor;
	KpMemSet (NamedColor, 0, NamedColors->Count * (KpInt32_t)sizeof (*NamedColor));

/* now get each of the named color records */
	for (Index = 0; Index < NamedColors->Count; Index++, NamedColor++) {
		Status = SpNamedColorGetRecord (&BufferSize, &Buf,
						NamedColors->DeviceChannels,
						NamedColor);
		if (SpStatSuccess != Status) {
			SpFree (NamedColors->Colors);
			NamedColors->Colors = NULL;
			return Status;
		}
	}

	return Status;
}

#endif

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert Ucrbg from internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 21, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpUcrbgToPublic (
				KpUInt32_t	BufferSize,
				char		KPHUGE *Buf,
				SpUcrbg_t	FAR *Ucrbg)
{
	KpUInt32_t	Index, Limit;
	KpUInt16_t	FAR *UInt16Ptr;

	if (BufferSize < 17)
		return SpStatBadTagData;

/* get Ucr data */
	Limit = 
	Ucrbg->UcrCount = SpGetUInt32 (&Buf);
	UInt16Ptr = (KpUInt16_t *)SpMalloc ((KpInt32_t)sizeof (KpUInt16_t) * Limit);
	if (NULL == UInt16Ptr)
		return SpStatMemory;

	Ucrbg->Ucr = UInt16Ptr;
	for (Index = 0; Index < Limit; Index++)
		*UInt16Ptr++ = SpGetUInt16 (&Buf);

/* get bg data */
	Limit = 
	Ucrbg->bgCount = SpGetUInt32 (&Buf);
	UInt16Ptr = (KpUInt16_t *)SpMalloc ((KpInt32_t)sizeof (KpUInt16_t) * Limit);
	if (NULL == UInt16Ptr) {
		SpFree (Ucrbg->Ucr);
		Ucrbg->Ucr = NULL;
		return SpStatMemory;
	}

	Ucrbg->bg = UInt16Ptr;
	for (Index = 0; Index < Limit; Index++)
		*UInt16Ptr++ = SpGetUInt16 (&Buf);

/* get the description */
	Limit = BufferSize - (8 + 4 + 2*Ucrbg->UcrCount + 4 + 2*Ucrbg->bgCount);
	Ucrbg->Desc = SpMalloc ((KpInt32_t)sizeof (KpUInt16_t) * Limit);
	if (NULL == Ucrbg->Desc) {
		SpFree (Ucrbg->Ucr);
		Ucrbg->Ucr = NULL;
		SpFree (Ucrbg->bg);
		Ucrbg->bg = NULL;
		return SpStatMemory;
	}
	SpGetBytes (&Buf, Ucrbg->Desc, Limit);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a Crd String
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 7, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpCrdGetInfo (
				KpUInt32_t		FAR *BufferSize,
				char			KPHUGE * FAR *Buf,
				SpCrdItem_t		FAR *CrdInfo)
{
        char            KPHUGE *BytePtr;
        KpUInt32_t             Count;
        KpUInt32_t      Size;
        char		*Name;


	CrdInfo->count = SpGetUInt32 (Buf);
	Size = *BufferSize;
	BytePtr = *Buf;
	if (CrdInfo->count > 0)
	{
		Name = CrdInfo->CRD_String = 
			SpMalloc(CrdInfo->count);
 
		if (NULL == CrdInfo->CRD_String)
		   return SpStatMemory;

		for (Count = 0;
		     (Size > 0) && (Count < CrdInfo->count) && 
		     ('\0' != *BytePtr);
		     Count++, Size--) {
			*Name++ = *BytePtr++;
		}
 
		Count++;
		Size--;
		if (('\0' != *BytePtr) || (Count != CrdInfo->count))
			return SpStatBadTagData;

		*Name = *BytePtr++;
	}
	*Buf = BytePtr;
	*BufferSize = Size;

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get a Crd Info Record
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 7, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpCrdInfoToPublic (
				KpUInt32_t		BufferSize,
				char			KPHUGE * FAR *Buf,
				SpCrdInfo_t		FAR *CrdBuf)
{
	SpStatus_t	Status;
	int		i;

	Status = SpCrdGetInfo(&BufferSize, Buf, 
			&CrdBuf->ProdDesc);

	for (i = 0; i < 4 && Status == SpStatSuccess; i++)
	{
		Status = SpCrdGetInfo(&BufferSize, Buf,
				 &CrdBuf->CrdInfo[i]);
	}
	return Status;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert attribute value from more compact internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagToPublic (
				SpHeader_t		FAR *Header,
				SpTagId_t		TagId,
				KpUInt32_t		TagDataSize,
				void			KPHUGE *TagData,
				SpTagValue_t	FAR *Value)
{
	char			KPHUGE *Buf;
	char			KPHUGE *BufSave;
	KpUInt32_t		Index, Limit;
	KpUInt16_t		FAR *UInt16Ptr;
	SpData_t		FAR *Data;
	SpTextDesc_t	FAR *TextDesc;
	SpScreen_t		FAR *Screen;
	SpSig_t			TypeSig;
	KpUInt32_t		Reserved;
	SpStatus_t		Status;

#if defined (SP_NO_OBSOLETE)
	if (Header) {}
#endif

/* set tag id and type in callers structure */
	Value->TagId = TagId;
	SpTagGetIdType (TagId, &Value->TagType);

/* get tag data type and reserved bytes */
	if (TagDataSize < 8)
		return SpStatBadTagData;

	Buf = TagData;
	TypeSig = SpGetUInt32 (&Buf);
	Reserved = SpGetUInt32 (&Buf);

/* if this is an unknown tag id, set the data type from the type signature */
	if (Value->TagType == Sp_AT_Unknown) {
		Value->TagType = SpAttrTypeSigToTypeId (TypeSig);
	}

/* do the TagType specific conversions */
	KpMemSet (&Value->Data, 0, (KpInt32_t)sizeof (Value->Data));
	switch (Value->TagType) {
	case Sp_AT_Enum:
		if (TypeSig != SpTypeUInt32)
			return SpStatBadTagData;

		if (TagDataSize != 12)
			return SpStatBadTagData;

		Value->Data.TagEnum = (SpTagEnum_t) SpGetUInt32 (&Buf);
		return SpStatSuccess;

	case Sp_AT_Curve:
		if (TypeSig != SpTypeCurve)
			return SpStatBadTagData;

		if (TagDataSize < 12)
			return SpStatBadTagData;

		Limit = SpGetUInt32 (&Buf);
		if ((TagDataSize - 12) / 2 < Limit)
			return SpStatBadTagData;

		if (0 == Limit) {
			Value->Data.Curve.Count = Limit;
			Value->Data.Curve.Data = NULL;
			return SpStatSuccess;
		}

		UInt16Ptr = (KpUInt16_t *)SpMalloc (Limit * (KpInt32_t)sizeof (*UInt16Ptr));
		if (NULL == UInt16Ptr)
			return SpStatMemory;

		Value->Data.Curve.Count = Limit;
		Value->Data.Curve.Data = UInt16Ptr;

		for (Index = 0; Index < Limit; Index++)
			*UInt16Ptr++ = SpGetUInt16 (&Buf);

		return SpStatSuccess;

	case Sp_AT_Data:
		if (TypeSig != SpTypeData)
			return SpStatBadTagData;

		if (TagDataSize < 12)
			return SpStatBadTagData;

		Data = &Value->Data.Data;
		Data->DataFlag = SpGetUInt32 (&Buf);
		Data->Count = TagDataSize - 12;
		Data->Bytes = NULL;
		if (0 != Data->Count) {
			Data->Bytes = SpMalloc (Data->Count);
			if (NULL == Data->Bytes)
				return SpStatMemory;

			SpGetBytes (&Buf, Data->Bytes, Data->Count);
		}
		return SpStatSuccess;

	case Sp_AT_DateTime:
		if (TypeSig != SpTypeDateTime)
			return SpStatBadTagData;

		if (TagDataSize != 20)
			return SpStatBadTagData;

		Value->Data.DateTime.Year = SpGetUInt16 (&Buf);
		Value->Data.DateTime.Month = SpGetUInt16 (&Buf);
		Value->Data.DateTime.Day = SpGetUInt16 (&Buf);
		Value->Data.DateTime.Hour = SpGetUInt16 (&Buf);
		Value->Data.DateTime.Minute = SpGetUInt16 (&Buf);
		Value->Data.DateTime.Second = SpGetUInt16 (&Buf);
		return SpStatSuccess;

	case Sp_AT_Lut:
		Value->Data.Lut.LutType = TypeSig;
		return SpLutToPublic (Buf, &Value->Data.Lut);

	case Sp_AT_Measurement:
		if (TypeSig != SpTypeMeasurement)
			return SpStatBadTagData;

		if (TagDataSize != 36)
			return SpStatBadTagData;

		Value->Data.Measurement.StdObserver = SpGetUInt32 (&Buf);
		SpGetF15d16XYZ (&Buf, &Value->Data.Measurement.Backing);
		Value->Data.Measurement.Geometry = SpGetUInt32 (&Buf);
		SpGetF15d16 (&Buf, &Value->Data.Measurement.Flare, 1);
		Value->Data.Measurement.IllumType = SpGetUInt32 (&Buf);
		return SpStatSuccess;

#if !defined (SP_NO_OBSOLETE)
	case Sp_AT_NamedColors:
		if (TypeSig != SpTypeNamedColors)
			return SpStatBadTagData;

		return SpNamedColorsToPublic (Header, TagDataSize,
						Buf, &Value->Data.NamedColors);
#endif

	case Sp_AT_NamedColors2:
		if (TypeSig != SpTypeNamedColors2)
			return SpStatBadTagData;

		return SpNamedColors2ToPublic (TagDataSize,
						Buf, 
						&Value->Data.NamedColors2);

	case Sp_AT_ProfileSeqDesc:
		if (TypeSig != SpTypeProfileSeqDesc)
			return SpStatBadTagData;

		return SpProfileSeqDescToPublic (TagDataSize, Buf,
						&Value->Data.ProfileSeqDesc);

	case Sp_AT_SF15d16:
		if (TypeSig != SpTypeSF15d16)
			return SpStatBadTagData;

		return SpArray32ToPublic (TagDataSize, Buf, &Value->Data.SF15d16s.Count,
							(void FAR * FAR *) &Value->Data.SF15d16s.Values);

	case Sp_AT_Screening:
		if (TypeSig != SpTypeScreening)
			return SpStatBadTagData;

		if (TagDataSize < 16)
			return SpStatBadTagData;

		Value->Data.Screening.Flag = SpGetUInt32 (&Buf);
		Limit = SpGetUInt32 (&Buf);

		if (TagDataSize != 16 + 12 * Limit)
			return SpStatBadTagData;

		Screen = SpMalloc (Limit * (KpInt32_t)sizeof (*Screen));
		if (NULL == Screen)
			return SpStatMemory;

		Value->Data.Screening.Channels = Limit;
		Value->Data.Screening.Screens = Screen;
		for (Index = 0;
				Index < Limit;
						Index++, Screen++) {
			SpGetF15d16 (&Buf, &Screen->Frequency, 1);
			SpGetF15d16 (&Buf, &Screen->Angle, 1);
			Screen->Spot = SpGetUInt32 (&Buf);
		}
		return SpStatSuccess;

	case Sp_AT_Signature:
		if (TypeSig != SpTypeSignature)
			return SpStatBadTagData;

		if (TagDataSize != 12)
			return SpStatBadTagData;

		Value->Data.Signature = SpGetUInt32 (&Buf);
		return SpStatSuccess;

	case Sp_AT_Text:
		if (TypeSig != SpTypeText)
			return SpStatBadTagData;

		Limit = TagDataSize - 8;
		Value->Data.Text = SpMalloc (Limit + 1);
		if (NULL == Value->Data.Text)
			return SpStatMemory;

		SpGetBytes (&Buf, Value->Data.Text, Limit);
		Value->Data.Text [ (int) Limit] = '\0';
		return SpStatSuccess;

	case Sp_AT_TextDesc:
		if (TypeSig != SpTypeTextDesc)
			return SpStatBadTagData;

		TextDesc = &Value->Data.TextDesc;
		Status = SpStatSuccess;

		Status = SpTextDescToPublic(Status, &Buf, (TagDataSize - 8), TextDesc);
		return Status;

	case Sp_AT_UF16d16:
		if (TypeSig != SpTypeUF16d16)
			return SpStatBadTagData;

		return SpArray32ToPublic (TagDataSize, Buf, &Value->Data.UF16d16s.Count,
							(void FAR * FAR *) &Value->Data.UF16d16s.Values);

	case Sp_AT_Ucrbg:
		if (TypeSig != SpTypeUcrbg)
			return SpStatBadTagData;

		return SpUcrbgToPublic (TagDataSize, Buf, &Value->Data.Ucrbg);

	case Sp_AT_UInt16:
		if (TypeSig != SpTypeUInt16)
			return SpStatBadTagData;

		return SpArray16ToPublic (TagDataSize, Buf, &Value->Data.UInt16s.Count,
							(void FAR * FAR *) &Value->Data.UInt16s.Values);

	case Sp_AT_UInt32:
		if (TypeSig != SpTypeUInt32)
			return SpStatBadTagData;

		return SpArray32ToPublic (TagDataSize, Buf, &Value->Data.UInt32s.Count,
							(void FAR * FAR *) &Value->Data.UInt32s.Values);

	case Sp_AT_UInt64:
		if (TypeSig != SpTypeUInt64)
			return SpStatBadTagData;

		return SpArray64ToPublic (TagDataSize, Buf, &Value->Data.UInt64s.Count,
							(void FAR * FAR *) &Value->Data.UInt64s.Values);

	case Sp_AT_UInt8:
		if (TypeSig != SpTypeUInt8)
			return SpStatBadTagData;

		return SpArray8ToPublic (TagDataSize, Buf, &Value->Data.UInt8s.Count,
							(void FAR * FAR *) &Value->Data.UInt8s.Values);

	case Sp_AT_Viewing:
		if (TypeSig != SpTypeViewing)
			return SpStatBadTagData;

		if (TagDataSize != 36)
			return SpStatBadTagData;

		SpGetF15d16XYZ (&Buf, &Value->Data.Viewing.Illuminant);
		SpGetF15d16XYZ (&Buf, &Value->Data.Viewing.Surround);
		Value->Data.Viewing.IllumType = SpGetUInt32 (&Buf);
		return SpStatSuccess;

	case Sp_AT_XYZ:
		if (TypeSig != SpTypeXYZ)
			return SpStatBadTagData;

		if (TagDataSize != 20)
			return SpStatBadTagData;

		SpGetF15d16XYZ (&Buf, &Value->Data.XYZ);
		return SpStatSuccess;
		
	case Sp_AT_CrdInfo:
		if (TypeSig != SpTypeCrdInfo)
			return SpStatBadTagData;

		return SpCrdInfoToPublic(TagDataSize, &Buf,
				&Value->Data.CrdData);

	case Sp_AT_Unknown:
		Value->Data.Binary.Size = TagDataSize;
		BufSave = (char KPHUGE *) SpMalloc (TagDataSize);
		if (NULL == BufSave)
			return SpStatMemory;

		KpMemCpy (BufSave, (void *)((KpInt32_t)Buf - 8), TagDataSize);
		Value->Data.Binary.Values = BufSave;
		return SpStatSuccess;
	}

	KpMemSet (&Value->Data, 0, (KpInt32_t)sizeof (Value->Data));
	return SpStatBadTagType;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert profile header from more compact internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 17, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpHeaderToPublic (
				char		KPHUGE *Ptr,
				KpUInt32_t	BufferSize,
				SpHeader_t	FAR *Header)
{
	KpUInt32_t	ProfileSize;

/* validate buffer is atleast HEADER_SIZE bytes long 
   SpGetHeaderFromDisc only has header in buffer */
	if (BufferSize < HEADER_SIZE)
		return SpStatBadBuffer;

	ProfileSize = SpGetUInt32 (&Ptr);

/* make sure the profile is not larger then 16Meg! */
	if ((ProfileSize < HEADER_SIZE) || (0xFFFFFFL < ProfileSize))
		return SpStatOutOfRange;

	Header->CMMType = SpGetUInt32 (&Ptr);
	Header->ProfileVersion = SpGetUInt32 (&Ptr);

/* reject profiles earlier then ColorSync 2.0 */
	if (Header->ProfileVersion < 0x01000000L)
		return SpStatOutOfRange;

	Header->DeviceClass = SpGetUInt32 (&Ptr);
	Header->DataColorSpace = SpGetUInt32 (&Ptr);
	Header->InterchangeColorSpace = SpGetUInt32 (&Ptr);

	Header->DateTime.Year = SpGetUInt16 (&Ptr);
	Header->DateTime.Month = SpGetUInt16 (&Ptr);
	Header->DateTime.Day = SpGetUInt16 (&Ptr);
	Header->DateTime.Hour = SpGetUInt16 (&Ptr);
	Header->DateTime.Minute = SpGetUInt16 (&Ptr);
	Header->DateTime.Second = SpGetUInt16 (&Ptr);

	if (SpProfileSig != SpGetUInt32 (&Ptr))
		return SpStatBadProfile;

	Header->Platform = SpGetUInt32 (&Ptr);
	Header->Flags = SpGetUInt32 (&Ptr);
	Header->DeviceManufacturer = SpGetUInt32 (&Ptr);
	Header->DeviceModel = SpGetUInt32 (&Ptr);
	Header->DeviceAttributes.hi = SpGetUInt32 (&Ptr);
	Header->DeviceAttributes.lo = SpGetUInt32 (&Ptr);
	Header->RenderingIntent = SpGetUInt32 (&Ptr);
	Header->Illuminant.X = SpGetUInt32 (&Ptr);
	Header->Illuminant.Y = SpGetUInt32 (&Ptr);
	Header->Illuminant.Z = SpGetUInt32 (&Ptr);
	Header->Originator = SpGetUInt32 (&Ptr);

	return SpStatSuccess;
}


#if !defined (SP_NO_TAGSET)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert array of 16 bit values to internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 17, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpArray16FromPublic (
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize,
				SpSig_t		TypeSig,
				KpUInt32_t	Reserved,
				KpUInt32_t	Count,
				void		FAR *Values)
{
	char		KPHUGE *Buf;
	KpUInt32_t	Index;
	KpUInt16_t	FAR *UInt16Ptr;

	*BufferSize = 2 * Count + 8;
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;

	*Buffer = Buf;
	SpPutUInt32 (&Buf, (KpUInt32_t) TypeSig);
	SpPutUInt32 (&Buf, Reserved);
	UInt16Ptr = Values;
	for (Index = 0; Index < Count; Index++)
		SpPutUInt16 (&Buf, *UInt16Ptr++);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert array of 32 bit values to internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 17, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpArray32FromPublic (
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize,
				SpSig_t		TypeSig,
				KpUInt32_t	Reserved,
				KpUInt32_t	Count,
				void		FAR *Values)
{
	char		KPHUGE *Buf;
	KpUInt32_t	Index;
	KpUInt32_t	FAR *UInt32Ptr;

	*BufferSize = 4 * Count + 8;
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;

	*Buffer = Buf;
	SpPutUInt32 (&Buf, (KpUInt32_t) TypeSig);
	SpPutUInt32 (&Buf, Reserved);
	UInt32Ptr = Values;
	for (Index = 0; Index < Count; Index++)
		SpPutUInt32 (&Buf, *UInt32Ptr++);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert array of 64 bit values to internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 2, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpArray64FromPublic (
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize,
				SpSig_t		TypeSig,
				KpUInt32_t	Reserved,
				KpUInt32_t	Count,
				void		FAR *Values)
{
	char		KPHUGE *Buf;
	KpUInt32_t	Index;
	SpUInt64_t	FAR *UInt64Ptr;

	*BufferSize = 8 * Count + 8;
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;

	*Buffer = Buf;
	SpPutUInt32 (&Buf, (KpUInt32_t) TypeSig);
	SpPutUInt32 (&Buf, Reserved);
	UInt64Ptr = Values;
	for (Index = 0; Index < Count; Index++) {
		SpPutUInt32 (&Buf, UInt64Ptr->hi);
		SpPutUInt32 (&Buf, UInt64Ptr->lo);
		UInt64Ptr++;
	}

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert array of 8 bit values to internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 17, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpArray8FromPublic (
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize,
				SpSig_t		TypeSig,
				KpUInt32_t	Reserved,
				KpUInt32_t	Count,
				void		FAR *Values)
{
	char		KPHUGE *Buf;

	*BufferSize = Count + 8;
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;

	*Buffer = Buf;
	SpPutUInt32 (&Buf, (KpUInt32_t) TypeSig);
	SpPutUInt32 (&Buf, Reserved);
	SpPutBytes (&Buf, Count, Values);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Put a named color record.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	Sept 16, 1996
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColor2PutRecord (
				char			KPHUGE * FAR *Buf,
				int			DevCoord,
				SpNamedColor2_t	FAR *Color)
{
	int		Channel;
	char		KPHUGE *BytePtr;


	SpPutBytes (Buf, 32, Color->Name);

	/* Prepare to move beyond 32 character Name */
	BytePtr = *Buf;

	for (Channel = 0; Channel < 3; Channel++)
		SpPutUInt16(&BytePtr, Color->Values [Channel]);
	for (Channel = 0; Channel < DevCoord; Channel++)
		SpPutUInt16(&BytePtr, Color->dValues [Channel]);

	*Buf = BytePtr;

	return SpStatSuccess;
}


#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Put a named color record.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 7, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColorPutRecord (
				char			KPHUGE * FAR *Buf,
				int			DevCoord,
				SpNamedColor_t	FAR *Color)
{
	int		Channel;
	char		KPHUGE *BytePtr;

	SpPutBytes (Buf, (KpUInt32_t)strlen (Color->Name) + 1, Color->Name);

	BytePtr = *Buf;

	for (Channel = 0; Channel < DevCoord; Channel++)
		*BytePtr++ = Color->Values [Channel];

	*Buf = (char *)BytePtr;

	return SpStatSuccess;
}
#endif


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert NamedColors2 to internal form.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	Sept 16, 1996
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColors2FromPublic (
				void			KPHUGE * FAR *Buffer,
				KpUInt32_t		FAR *BufferSize,
				KpUInt32_t		Reserved,
				SpNamedColors2_t	FAR *NamedColors)
{
	char			KPHUGE *Buf;
	SpNamedColor2_t		FAR *Color;
	KpUInt32_t		Index;
	SpStatus_t		Status;

/* determine size of buffer needed */
	*BufferSize = 8;       /* ncl2 and reserved */
	*BufferSize += 12;     /* Vender Flag, Count, Channels */
	*BufferSize += 32;     /* Prefix */
	*BufferSize += 32;     /* Suffix */
	*BufferSize += (NamedColors->Count * 
			(32  +                    /* Named Color */
			(2 * 3) +		  /* PCS Coords */
			(2 * NamedColors->DeviceChannels)));

/* allocate the buffer */
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;
        KpMemSet (Buf, 0, *BufferSize);

	*Buffer = Buf;

/* now fill the buffer */
	/*Force conversion to new Named Color */
	SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeNamedColors2);
	SpPutUInt32 (&Buf, Reserved);
	SpPutUInt32 (&Buf, NamedColors->VendorFlags);
	SpPutUInt32 (&Buf, NamedColors->Count);
	SpPutUInt32 (&Buf, NamedColors->DeviceChannels);
	SpPutBytes (&Buf, (KpUInt32_t)strlen (NamedColors->Prefix) + 1, 
				NamedColors->Prefix);

	/* Move Past Prefix */
	Buf = *Buffer;
	Buf += (20 + 32);

	SpPutBytes (&Buf, (KpUInt32_t)strlen (NamedColors->Suffix) + 1, 
				NamedColors->Suffix);

	/* Move Past Suffix */
	Buf = *Buffer;
	Buf += (20 + 64);

	Status = SpStatSuccess;
	for (Index = 0, Color = NamedColors->Colors;
	    (Index < NamedColors->Count) && (SpStatSuccess == Status);
	     Index++, Color++) {
		Status = SpNamedColor2PutRecord (&Buf, 
						NamedColors->DeviceChannels,
						Color);
	}

	return Status;
}


#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert named colors to internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 6, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpNamedColorsFromPublic (
				SpHeader_t		FAR *Header,
				void			KPHUGE * FAR *Buffer,
				KpUInt32_t		FAR *BufferSize,
				KpUInt32_t		Reserved,
				SpNamedColors_t	FAR *NamedColors)
{
	char			KPHUGE *Buf;
	SpNamedColor_t	FAR *Color;
	KpUInt32_t		Index;
	SpStatus_t		Status;

	if (NULL == Header)
		return SpStatBadTagData;

/* check callers number of channels against data color space channels */
	if (NamedColors->DeviceChannels != 
		SpGetChannelsFromColorSpace (Header->DataColorSpace))
		return SpStatBadTagData;

/* determine size of buffer needed */
	*BufferSize = 8;       /* ncl2 and reserved */
	*BufferSize += 8;      /* Vender Flag, Count */
	*BufferSize += strlen (NamedColors->Prefix) + 1;
	*BufferSize += strlen (NamedColors->Suffix) + 1;
	for (Index = 0, Color = NamedColors->Colors;
			Index < NamedColors->Count;
			Index++, Color++) {
		*BufferSize += strlen (Color->Name) + 1;
	}
	*BufferSize += NamedColors->Count *
			NamedColors->DeviceChannels;

/* allocate the buffer */
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;
        KpMemSet (Buf, 0, *BufferSize);

	*Buffer = Buf;

/* now fill the buffer */
	/*Force conversion to new Named Color */
	SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeNamedColors);
	SpPutUInt32 (&Buf, Reserved);
	SpPutUInt32 (&Buf, NamedColors->VendorFlags);
	SpPutUInt32 (&Buf, NamedColors->Count);

	SpPutBytes (&Buf, (KpUInt32_t)strlen (NamedColors->Prefix) + 1, 
				NamedColors->Prefix);

	SpPutBytes (&Buf, (KpUInt32_t)strlen (NamedColors->Suffix) + 1, 
				NamedColors->Suffix);

	Status = SpStatSuccess;
	for (Index = 0, Color = NamedColors->Colors;
	    (Index < NamedColors->Count) && (SpStatSuccess == Status);
	     Index++, Color++) {
		Status = SpNamedColorPutRecord (&Buf, 
						NamedColors->DeviceChannels,
						Color);
	}

	return Status;
}
#endif	/* !SP_NO_OBSOLETE */


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get sizes of various TextDesc strings.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 22, 1994
 *------------------------------------------------------------------*/
static void SpTextDescStrSizes (
			SpTextDesc_t	FAR *TextDesc,
			KpUInt32_t		FAR *IsoCount,
			KpUInt32_t		FAR *MacCount,
			KpUInt32_t		FAR *UniCount)
{
	KpUInt32_t	Len;
	KpUInt16_t	FAR *UInt16Ptr;

	if (sizeof (TextDesc->MacStr) < TextDesc->MacCount)
		*MacCount = (KpUInt32_t)sizeof (TextDesc->MacStr);
	else
		*MacCount = TextDesc->MacCount;

	if (NULL != TextDesc->IsoStr)
		*IsoCount = (KpUInt32_t)strlen (TextDesc->IsoStr) + 1;
	else
		*IsoCount = 0;

	Len = 0;
	if (NULL != TextDesc->UniStr) {
		for (UInt16Ptr = TextDesc->UniStr;
				*UInt16Ptr;
						UInt16Ptr++)
			Len++;
		Len += 2; /* 1 for terminator and 1 for byte order */
	}
	*UniCount = Len;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Fill buffer with TextDesc.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 22, 1994
 *------------------------------------------------------------------*/
static void SpTextDescFromPublic (
				char			KPHUGE * FAR *Buf,
				SpTextDesc_t	FAR *TextDesc)
{
	KpUInt32_t	IsoCount;
	KpUInt32_t	MacCount;
	KpUInt32_t	UniCount;
	KpUInt32_t	Index;
	KpUInt32_t	nullSize, i;
	KpUInt16_t	FAR *UInt16Ptr;
	char		KPHUGE	*BytePtr;
	char		tempBuf[67];

	SpTextDescStrSizes (TextDesc, &IsoCount, &MacCount, &UniCount);

	SpPutUInt32 (Buf, IsoCount);
	if (NULL != TextDesc->IsoStr)
		SpPutBytes (Buf, IsoCount, TextDesc->IsoStr);

	SpPutUInt32 (Buf, TextDesc->UniLangCode);
	SpPutUInt32 (Buf, UniCount);
	if (NULL != TextDesc->UniStr) {
		SpPutUInt16 (Buf, 0xFEFF);
		for (Index = 0, UInt16Ptr = TextDesc->UniStr;
				Index < UniCount - 1;
						Index++)
			SpPutUInt16 (Buf, *UInt16Ptr++);
	}

	SpPutUInt16 (Buf, TextDesc->MacScriptCode);
	BytePtr = *Buf;
	*BytePtr++ = TextDesc->MacCount;
	*Buf = BytePtr;

	/* add MacStr to buffer  */
	SpPutBytes (Buf, TextDesc->MacCount, TextDesc->MacStr);

	/* null out remainder of MacStr buffer  */
	nullSize = (KpUInt32_t)sizeof(TextDesc->MacStr) - TextDesc->MacCount;
	for (i = 0; i < nullSize; i++) {
		tempBuf[i] = 0;
	}
	SpPutBytes(Buf, nullSize, tempBuf);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get size of buffer needed to hold a text description.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 22, 1994
 *------------------------------------------------------------------*/
static KpUInt32_t SpTextDescSize (
				SpTextDesc_t	FAR *TextDesc)
{
	KpUInt32_t	IsoCount;
	KpUInt32_t	MacCount;
	KpUInt32_t	UniCount;

	SpTextDescStrSizes (TextDesc, &IsoCount, &MacCount, &UniCount);

	return	  4								/* Iso string size */
			+ IsoCount						/* Iso string */
			+ 4								/* Unicode language code */
			+ 4								/* Unicode string size */
			+ 2 * UniCount					/* Unicode string */
			+ 2								/* Mac script code */
			+ 1								/* Mac string length */
			+ (KpUInt32_t)sizeof (TextDesc->MacStr);	/* Mac string buffer */
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Helper for SpProfileSeqDescFromPublic.
 *
 * AUTHOR
 * 	gbp
 *
 * DATE CREATED
 *	26 May 1995
 *------------------------------------------------------------------*/
static void SpTextDescTagFromPublic (
				char		KPHUGE **Buf,
				SpTextDesc_t	FAR *TextDesc)
{
	SpPutUInt32 (Buf, SpTypeTextDesc);		/* Write type id	*/
	SpPutUInt32 (Buf, 0L);					/* Write reserved	*/
	SpTextDescFromPublic (Buf, TextDesc);	/* Write tag		*/
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert ProfileSeqDesc to internal form (ICC form).
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 21, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpProfileSeqDescFromPublic (
				void				KPHUGE * FAR *Buffer,
				KpUInt32_t			FAR *BufferSize,
				SpSig_t				TypeSig,
				KpUInt32_t			Reserved,
				SpProfileSeqDesc_t	FAR *ProfileSeqDesc)
{
	KpUInt32_t					Index;
	char						KPHUGE *Buf;
	SpProfileSeqDescRecord_t	KPHUGE *Record;

/* determine size of ICC formatted buffer needed */
	*BufferSize = 8 + 4;
	for (Index = 0, Record = ProfileSeqDesc->Records;
			Index < ProfileSeqDesc->Count;
					Index++, Record++) {
		*BufferSize += 20;
		*BufferSize += 8 + SpTextDescSize (&Record->DeviceManufacturerDesc);
		*BufferSize += 8 + SpTextDescSize (&Record->DeviceModelDesc);
	}

/* allocate the buffer */
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;

	*Buffer = Buf;

/* now fill the buffer */
	SpPutUInt32 (&Buf, (KpUInt32_t) TypeSig);
	SpPutUInt32 (&Buf, Reserved);

	SpPutUInt32 (&Buf, ProfileSeqDesc->Count);
	for (Index = 0, Record = ProfileSeqDesc->Records;
			Index < ProfileSeqDesc->Count;
					Index++, Record++) {
		SpPutUInt32 (&Buf, Record->DeviceManufacturer);
		SpPutUInt32 (&Buf, Record->DeviceModel);
		SpPutUInt32 (&Buf, Record->DeviceAttributes.hi);
		SpPutUInt32 (&Buf, Record->DeviceAttributes.lo);
		SpPutUInt32 (&Buf, Record->Technology);
		SpTextDescTagFromPublic (&Buf, &Record->DeviceManufacturerDesc);
		SpTextDescTagFromPublic (&Buf, &Record->DeviceModelDesc);
	}

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert Ucrbg to internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 21, 1994
 *------------------------------------------------------------------*/
static SpStatus_t SpUcrbgFromPublic (
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize,
				SpSig_t		TypeSig,
				KpUInt32_t	Reserved,
				SpUcrbg_t	FAR *Ucrbg)
{
	char			KPHUGE *Buf;
	KpUInt32_t		Index;
	KpUInt16_t		FAR *UInt16Ptr;

/* determine size of buffer needed */
	*BufferSize = 8;
	*BufferSize += 4;
	*BufferSize += 2 * Ucrbg->UcrCount;
	*BufferSize += 4;
	*BufferSize += 2 * Ucrbg->bgCount;
	*BufferSize += strlen (Ucrbg->Desc) + 1;

/* allocate the buffer */
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;

	*Buffer = Buf;

/* now fill the buffer */
	SpPutUInt32 (&Buf, (KpUInt32_t) TypeSig);
	SpPutUInt32 (&Buf, Reserved);

	SpPutUInt32 (&Buf, Ucrbg->UcrCount);
	UInt16Ptr = Ucrbg->Ucr;
	for (Index = 0; Index < Ucrbg->UcrCount; Index++)
		SpPutUInt16 (&Buf, *UInt16Ptr++);

	SpPutUInt32 (&Buf, Ucrbg->bgCount);
	UInt16Ptr = Ucrbg->bg;
	for (Index = 0; Index < Ucrbg->bgCount; Index++)
		SpPutUInt16 (&Buf, *UInt16Ptr++);

	SpPutBytes (&Buf, (KpUInt32_t)strlen (Ucrbg->Desc) + 1, Ucrbg->Desc);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert CrdInfo to internal form.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	Sept 16, 1996
 *------------------------------------------------------------------*/
static SpStatus_t SpCrdInfoFromPublic (
			void		KPHUGE * FAR *Buffer,
			KpUInt32_t	FAR *BufferSize,
			KpUInt32_t	Reserved,
			SpCrdInfo_t	FAR *CrdInfo)
{
	char			KPHUGE *Buf;
	KpUInt32_t		Index;
	SpStatus_t		Status = SpStatSuccess;

/* determine size of buffer needed */
	*BufferSize = 8;       /* crdi and reserved */
	*BufferSize += 20;     /* 5 Counts */
	*BufferSize += CrdInfo->ProdDesc.count;     /* Product String */
	for (Index = 0; Index < 4; Index++)
		*BufferSize += CrdInfo->CrdInfo[Index].count;     /* Crd */

/* allocate the buffer */
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;
        KpMemSet (Buf, 0, *BufferSize);

	*Buffer = Buf;

/* now fill the buffer */
	/*Force conversion to new Named Color */
	SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeCrdInfo);
	SpPutUInt32 (&Buf, Reserved);
	SpPutUInt32 (&Buf, CrdInfo->ProdDesc.count);
	SpPutBytes (&Buf, (KpUInt32_t)CrdInfo->ProdDesc.count,
				CrdInfo->ProdDesc.CRD_String);

	for (Index = 0; Index < 4; Index++) 
	{
		SpPutUInt32 (&Buf, CrdInfo->CrdInfo[Index].count);
		SpPutBytes (&Buf, (KpUInt32_t)CrdInfo->CrdInfo[Index].count,
				CrdInfo->CrdInfo[Index].CRD_String);
	}

	return Status;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert attribute value to more compact internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagFromPublic (
				SpHeader_t		FAR *Header,
				SpTagValue_t	FAR *Value,
				KpUInt32_t		FAR *BufferSize,
				void			KPHUGE * FAR *Buffer)
{
	char			KPHUGE *Buf;
	KpUInt32_t		Index, Limit;
	SpData_t		FAR *Data;
	KpUInt16_t		FAR *UInt16Ptr;
	SpScreen_t		FAR *Screen;

#if defined (SP_NO_OBSOLETE)
	if (Header) {}		/* Unused parameter */
#endif

	*Buffer = NULL;
	*BufferSize = 0;

	switch (Value->TagType) {
	case Sp_AT_Unknown:
		*BufferSize = Value->Data.Binary.Size;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutBytes (&Buf, *BufferSize, Value->Data.Binary.Values);
		return SpStatSuccess;

	case Sp_AT_Enum:
		*BufferSize = 4 + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeUInt32);
		SpPutUInt32 (&Buf, 0L);
		SpPutUInt32 (&Buf, (KpUInt32_t) Value->Data.TagEnum);
		return SpStatSuccess;

	case Sp_AT_Curve:
		Limit = Value->Data.Curve.Count;
		*BufferSize = 4 + 2 * Limit + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeCurve);
		SpPutUInt32 (&Buf, 0L);
		SpPutUInt32 (&Buf, Limit);
		UInt16Ptr = Value->Data.Curve.Data;
		while (Limit--)
			SpPutUInt16 (&Buf, *UInt16Ptr++);

		return SpStatSuccess;

	case Sp_AT_Data:
		Data = &Value->Data.Data;
		*BufferSize = 4 + Data->Count + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeData);
		SpPutUInt32 (&Buf, 0L);
		SpPutUInt32 (&Buf, Data->DataFlag);
		SpPutBytes (&Buf, Data->Count, Data->Bytes);
		return SpStatSuccess;

	case Sp_AT_DateTime:
		*BufferSize = 12 + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeDateTime);
		SpPutUInt32 (&Buf, 0L);
		SpPutUInt16 (&Buf, Value->Data.DateTime.Year);
		SpPutUInt16 (&Buf, Value->Data.DateTime.Month);
		SpPutUInt16 (&Buf, Value->Data.DateTime.Day);
		SpPutUInt16 (&Buf, Value->Data.DateTime.Hour);
		SpPutUInt16 (&Buf, Value->Data.DateTime.Minute);
		SpPutUInt16 (&Buf, Value->Data.DateTime.Second);
		return SpStatSuccess;

	case Sp_AT_Lut:
		return SpLutFromPublic (&Value->Data.Lut, BufferSize, Buffer);

	case Sp_AT_Measurement:
		*BufferSize = 28 + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeMeasurement);
		SpPutUInt32 (&Buf, 0L);
		SpPutUInt32 (&Buf, Value->Data.Measurement.StdObserver);
		SpPutF15d16XYZ (&Buf, &Value->Data.Measurement.Backing);
		SpPutUInt32 (&Buf, Value->Data.Measurement.Geometry);
		SpPutF15d16 (&Buf, &Value->Data.Measurement.Flare, 1);
		SpPutUInt32 (&Buf, Value->Data.Measurement.IllumType);
		return SpStatSuccess;

#if !defined (SP_NO_OBSOLETE)
	case Sp_AT_NamedColors:
		return SpNamedColorsFromPublic (Header, Buffer, BufferSize, 0L,
						&Value->Data.NamedColors);
#endif

	case Sp_AT_NamedColors2:
		return SpNamedColors2FromPublic (Buffer, BufferSize, 0L,
						&Value->Data.NamedColors2);

	case Sp_AT_ProfileSeqDesc:
		return SpProfileSeqDescFromPublic (Buffer, BufferSize,
						SpTypeProfileSeqDesc, 0L,
						&Value->Data.ProfileSeqDesc);

	case Sp_AT_SF15d16:
		return SpArray32FromPublic (Buffer, BufferSize,
						SpTypeSF15d16, 0L,
						Value->Data.SF15d16s.Count,
						Value->Data.SF15d16s.Values);

	case Sp_AT_Screening:
		*BufferSize = 8 + 12 * Value->Data.Screening.Channels + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeScreening);
		SpPutUInt32 (&Buf, 0L);
		SpPutUInt32 (&Buf, Value->Data.Screening.Flag);
		SpPutUInt32 (&Buf, Value->Data.Screening.Channels);
		for (Index = 0, Screen = Value->Data.Screening.Screens;
				Index < Value->Data.Screening.Channels;
						Index++, Screen++) {
			SpPutF15d16 (&Buf, &Screen->Frequency, 1);
			SpPutF15d16 (&Buf, &Screen->Angle, 1);
			SpPutUInt32 (&Buf, Screen->Spot);
		}
		return SpStatSuccess;

	case Sp_AT_Signature:
		*BufferSize = 4 + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeSignature);
		SpPutUInt32 (&Buf, 0L);
		SpPutUInt32 (&Buf, Value->Data.Signature);
		return SpStatSuccess;

	case Sp_AT_Text:
		Limit = (KpUInt32_t)strlen (Value->Data.Text) + 1;
		*BufferSize = Limit + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeText);
		SpPutUInt32 (&Buf, 0L);
		SpPutBytes (&Buf, Limit, Value->Data.Text);
		return SpStatSuccess;

	case Sp_AT_TextDesc:
		*BufferSize = 8 + SpTextDescSize (&Value->Data.TextDesc);
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpTextDescTagFromPublic (&Buf, &Value->Data.TextDesc);
		return SpStatSuccess;

	case Sp_AT_UF16d16:
		return SpArray32FromPublic (Buffer, BufferSize,
									SpTypeUF16d16, 0L,
									Value->Data.UF16d16s.Count,
									Value->Data.UF16d16s.Values);

	case Sp_AT_Ucrbg:
		return SpUcrbgFromPublic (Buffer, BufferSize,
									SpTypeUcrbg, 0L,
									&Value->Data.Ucrbg);

	case Sp_AT_UInt16:
		return SpArray16FromPublic (Buffer, BufferSize,
									SpTypeUInt16, 0L,
									Value->Data.UInt16s.Count,
									Value->Data.UInt16s.Values);

	case Sp_AT_UInt32:
		return SpArray32FromPublic (Buffer, BufferSize,
									SpTypeUInt32, 0L,
									Value->Data.UInt32s.Count,
									Value->Data.UInt32s.Values);

	case Sp_AT_UInt64:
		return SpArray64FromPublic (Buffer, BufferSize,
									SpTypeUInt64, 0L,
									Value->Data.UInt64s.Count,
									Value->Data.UInt64s.Values);

	case Sp_AT_UInt8:
		return SpArray8FromPublic (Buffer, BufferSize,
									SpTypeUInt8, 0L,
									Value->Data.UInt8s.Count,
									Value->Data.UInt8s.Values);

	case Sp_AT_Viewing:
		*BufferSize = 28 + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeViewing);
		SpPutUInt32 (&Buf, 0L);
		SpPutF15d16XYZ (&Buf, &Value->Data.Viewing.Illuminant);
		SpPutF15d16XYZ (&Buf, &Value->Data.Viewing.Surround);
		SpPutUInt32 (&Buf, Value->Data.Viewing.IllumType);
		return SpStatSuccess;

	case Sp_AT_XYZ:
		*BufferSize = 12 + 8;
		Buf = SpMalloc (*BufferSize);
		if (NULL == Buf)
			return SpStatMemory;

		*Buffer = Buf;
		SpPutUInt32 (&Buf, (KpUInt32_t) SpTypeXYZ);
		SpPutUInt32 (&Buf, 0L);
		SpPutF15d16XYZ (&Buf, &Value->Data.XYZ);
		return SpStatSuccess;

	case Sp_AT_CrdInfo:
		return SpCrdInfoFromPublic(Buffer, BufferSize,
					0L, &Value->Data.CrdData);

	}

	return SpStatBadTagType;
}
#endif /* !SP_NO_TAGSET */


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert profile header to more compact internal form.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	August 16, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpHeaderFromPublic (
				SpHeader_t	FAR *Header,
				KpUInt32_t	BufferSize,
				char		KPHUGE *Buffer)
{
	char		KPHUGE *Ptr;

/* validate buffer is atleast HEADER_SIZE bytes long */
	if (BufferSize < HEADER_SIZE)
		return SpStatBadBuffer;

/* clear the buffer */
	KpMemSet (Buffer, 0, 128);

	Ptr = Buffer;
	SpPutUInt32 (&Ptr, 128L);

	SpPutUInt32 (&Ptr, Header->CMMType);
	SpPutUInt32 (&Ptr, Header->ProfileVersion);
	SpPutUInt32 (&Ptr, Header->DeviceClass);
	SpPutUInt32 (&Ptr, Header->DataColorSpace);
	SpPutUInt32 (&Ptr, Header->InterchangeColorSpace);

	SpPutUInt16 (&Ptr, Header->DateTime.Year);
	SpPutUInt16 (&Ptr, Header->DateTime.Month);
	SpPutUInt16 (&Ptr, Header->DateTime.Day);
	SpPutUInt16 (&Ptr, Header->DateTime.Hour);
	SpPutUInt16 (&Ptr, Header->DateTime.Minute);
	SpPutUInt16 (&Ptr, Header->DateTime.Second);

	SpPutUInt32 (&Ptr, SpProfileSig);
	SpPutUInt32 (&Ptr, Header->Platform);
	SpPutUInt32 (&Ptr, Header->Flags);
	SpPutUInt32 (&Ptr, Header->DeviceManufacturer);
	SpPutUInt32 (&Ptr, Header->DeviceModel);
	SpPutUInt32 (&Ptr, Header->DeviceAttributes.hi);
	SpPutUInt32 (&Ptr, Header->DeviceAttributes.lo);
	SpPutUInt32 (&Ptr, Header->RenderingIntent);
	SpPutUInt32 (&Ptr, Header->Illuminant.X);
	SpPutUInt32 (&Ptr, Header->Illuminant.Y);
	SpPutUInt32 (&Ptr, Header->Illuminant.Z);
	SpPutUInt32 (&Ptr, Header->Originator);

	return SpStatSuccess;
}
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	 Finds tags that share the same data. The IDs of tags that share the 
 *	 the same data as the input ID are filled in caller provided space.
 *	 Also, the number of matches found is returned.
 *
 * AUTHOR
 * 	shaque
 *
 * DATE CREATED
 *	Oct 10, 1997
 *------------------------------------------------------------------*/


 SpStatus_t SPAPI SpProfileGetSharedTags(SpProfile_t Profile,
								  SpTagId_t		TagId,
								  SpTagId_t		*Matched_TagIds,
								  KpInt32_t		*num_matched_tags)
{

	SpProfileData_t		FAR	*ProfileData;
	SpTagDirEntry_t	FAR *tagArray;
	KpInt32_t			index, i, num_of_matches = 0;
	KpGenericPtr_t	FAR tagData1, FAR tagData2;
	KpUInt32_t			*temp_tagId_buffer;


	*num_matched_tags = 0;
	/* convert profile handle to pointer to locked memory */
	ProfileData = SpProfileLock(Profile);
	if (NULL == ProfileData)
		return (SpStatBadProfile);

	/* Check if Profile found via Search function */
	if (ProfileData->TagArray == NULL)
	/* If so, it needs the Tag Array Loaded */
		SpProfilePopTagArray(ProfileData);

	tagArray = (SpTagDirEntry_t FAR *) 
				lockBuffer (ProfileData->TagArray);

	temp_tagId_buffer =(KpUInt32_t *)allocBufferPtr(ProfileData->TotalCount * sizeof(KpUInt32_t));
	if (temp_tagId_buffer ==NULL)
	{
		unlockBuffer(ProfileData->TagArray);
		SpProfileUnlock(Profile);
		return(SpStatMemory);
	}
 
	for (index = 0; index < ProfileData->TotalCount; index++)
	{
		if (tagArray[index].TagId == (TagId))
			break;
	}
	
	/* Reached the end of the tag array*/
	if (index >= ProfileData->TotalCount)
	{
		*num_matched_tags = 0;
		unlockBuffer(ProfileData->TagArray);
		SpProfileUnlock(Profile);
		freeBufferPtr(temp_tagId_buffer);
		return(SpStatSuccess);
	}

	for(i = index+1; i < ProfileData->TotalCount; i++)
	{
		if(tagArray[index].TagDataSize == tagArray[i].TagDataSize)
		{
			/* compare the size of the data */
			tagData1 = lockBuffer(tagArray[index].TagData);
			tagData2 = lockBuffer(tagArray[i].TagData);

			if (0 == KpMemCmp (tagData1, tagData2,
							tagArray[index].TagDataSize)) 
			{
				temp_tagId_buffer[num_of_matches++] = (KpUInt32_t)tagArray[i].TagId;
			}
			unlockBuffer(tagArray[index].TagData);
			unlockBuffer(tagArray[i].TagData);
		}
	}

	/* If matches were found, fill in the tagID array with the matched IDs */
	if (num_of_matches > 0)
		{
			for(i=0; i<num_of_matches; i++)
				Matched_TagIds[i] = (SpTagId_t)temp_tagId_buffer[i];
		}

	unlockBuffer(ProfileData->TagArray);
	SpProfileUnlock(Profile);
	freeBufferPtr(temp_tagId_buffer);
	*num_matched_tags = num_of_matches;
	return(SpStatSuccess);
}

 /*--------------------------------------------------------------------
 * DESCRIPTION
 *	 Gets the number of tags in a Profile.
 * AUTHOR
 * 	shaque
 *
 * DATE CREATED
 *	Oct 10, 1997
 *------------------------------------------------------------------*/

 SpStatus_t SPAPI SpProfileGetTagCount(
				SpProfile_t		Profile,
				KpInt32_t		*tagCount)
{
	SpProfileData_t		FAR *ProfileData;
	KpInt32_t			numValidTags = 0, index;
	SpTagDirEntry_t		FAR	*tagArray;

/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData) {
		return SpStatBadProfile;
	}

/* lock the tag array */
	tagArray = (SpTagDirEntry_t FAR *) lockBuffer (ProfileData->TagArray);

/* search for valid tags */
	for (index = 0; index < ProfileData->TotalCount; index++) {
		if (tagArray[index].TagDataSize != -1) {
			numValidTags++;
		}
	}

	*tagCount = numValidTags;

/* unlock tag array handle */
	unlockBuffer (ProfileData->TagArray);

/* unlock the profile */
	SpProfileUnlock (Profile);

	return SpStatSuccess;
}

