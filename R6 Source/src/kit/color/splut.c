/*******************************************************************/
/*
	Contains:	This module contains functions for lut manipulation.

				Created by lsh, November 5, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1996 by Eastman Kodak Company, 
                        all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPLUT.C  $
		$Logfile:   O:\pc_src\dll\stdprof\splut.c_v  $
		$Revision:   2.2  $
		$Date:   07 Apr 1994 13:24:22  $
		$Author:   lsh  $

	SCCS Revision:
		@(#)splut.c	1.16	07/23/96

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1996                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"
#include "attrcipg.h"

/* Prototypes */
static void SpInitMatrix (
				KpF15d16_t FAR Matrix3x3 [9]);

static KpUInt32_t SpLut8SizeOfInputTable (
				SpLut8Bit_t	FAR *Lut8);

static KpUInt32_t SpLut8SizeOfOutputTable (
				SpLut8Bit_t	FAR *Lut8);
				
static KpUInt32_t SpLut8SizeOfClut (
				SpLut8Bit_t	FAR *Lut8);
				
static SpStatus_t SpLut8ToPublic (
				char	KPHUGE *Buf,
				SpLut_t	FAR *Lut);
				
static SpStatus_t SpLut8FromPublic (
				SpLut_t		FAR *Lut,
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize);
				
static KpUInt32_t SpLut16SizeOfInputTable (
				SpLut16Bit_t	FAR *Lut16);

static KpUInt32_t SpLut16SizeOfOutputTable (
				SpLut16Bit_t	FAR *Lut16);

static KpUInt32_t SpLut16SizeOfClut (
				SpLut16Bit_t	FAR *Lut16);

static SpStatus_t SpLut16ToPublic (
				char		KPHUGE *Buf,
				SpLut_t		FAR *Lut);
				
static SpStatus_t SpLut16FromPublic (
				SpLut_t		FAR *Lut,
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize);
				


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Initialize matrix.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	December 25, 1993
 *------------------------------------------------------------------*/
static void SpInitMatrix (KpF15d16_t FAR Matrix3x3 [9])
{
	int	i;

	Matrix3x3 [0] = KpF15d16FromDouble (0.0);
	for (i = 1; i < 9; i++)
		Matrix3x3 [i] = Matrix3x3 [0];

	Matrix3x3 [0] =
	Matrix3x3 [4] =
	Matrix3x3 [8] = KpF15d16FromDouble (1.0);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Compute size of InputTable for 8 bit lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static KpUInt32_t SpLut8SizeOfInputTable (
				SpLut8Bit_t	FAR *Lut8)
{
	return 256 * Lut8->InputChannels;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Compute size of OutputTable for 8 bit lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static KpUInt32_t SpLut8SizeOfOutputTable (
				SpLut8Bit_t	FAR *Lut8)
{
	return 256 * Lut8->OutputChannels;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Compute size of clut for 8 bit lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static KpUInt32_t SpLut8SizeOfClut (
				SpLut8Bit_t	FAR *Lut8)
{
	KpUInt32_t	Size;
	KpUInt32_t	i;

	Size = Lut8->LUTDimensions;
	for (i = 1; i < Lut8->InputChannels; i++)
		Size *= Lut8->LUTDimensions;

	return Size * Lut8->OutputChannels;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Create empty 8 Bit Lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpLut8Create (
				KpUInt16_t	InChannels,
				KpUInt16_t	OutChannels,
				KpUInt16_t	LUTDimensions,
				SpLut_t		FAR *Lut)
{
	KpUInt32_t	Size;
	SpStatus_t	Status;
	SpLut8Bit_t	FAR *Lut8;

/* validate parameters */
	if ((InChannels < 1) || (8 < InChannels))
		return SpStatOutOfRange;

	if ((OutChannels < 1) || (8 < OutChannels))
		return SpStatOutOfRange;

	if ((LUTDimensions < 2) || (SPMAXLUTSIZE < LUTDimensions))
		return SpStatOutOfRange;

/* initialize all structure members */
	Lut->LutType = SpTypeLut8;
	Lut8 = &Lut->L.Lut8;
	Lut8->InputChannels = (char) InChannels;
	Lut8->OutputChannels = (char) OutChannels;
	Lut8->LUTDimensions = (char) LUTDimensions;
	SpInitMatrix (Lut8->Matrix3x3);
	Lut8->InputTable = NULL;
	Lut8->CLUT = NULL;
	Lut8->OutputTable = NULL;

	Status = SpStatSuccess;

/* input table */
	Size = SpLut8SizeOfInputTable (Lut8);
	Lut8->InputTable = SpMalloc (Size);
	if (NULL == Lut8->InputTable)
		Status = SpStatMemory;

/* CLUT */
	Size = SpLut8SizeOfClut (Lut8);
	Lut8->CLUT = SpMalloc (Size);
	if (NULL == Lut8->CLUT)
		Status = SpStatMemory;

/* output table */
	Size = SpLut8SizeOfOutputTable (Lut8);
	Lut8->OutputTable = SpMalloc (Size);
	if (NULL == Lut8->OutputTable)
		Status = SpStatMemory;

/* check for error */
	if (SpStatSuccess != Status) {
		SpFree (Lut8->CLUT);
		Lut8->CLUT = NULL;
		SpFree (Lut8->InputTable);
		Lut8->InputTable = NULL;
		SpFree (Lut8->OutputTable);
		Lut8->OutputTable = NULL;
	}

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert lut8 to public format.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static SpStatus_t SpLut8ToPublic (
				char	KPHUGE *Buf,
				SpLut_t	FAR *Lut)
{
	SpLut8Bit_t	FAR *Lut8;
	SpStatus_t	Status;

	Lut8 = &Lut->L.Lut8;

/* get fixed size elements */
	Lut8->InputChannels = *Buf++;
	Lut8->OutputChannels = *Buf++;
	Lut8->LUTDimensions = *Buf++;

/* varify pad byte is zero */
	if (0 != *Buf++)
		return SpStatOutOfRange;

/* allocate arrays */
	Status = SpLut8Create (Lut8->InputChannels, Lut8->OutputChannels,
								Lut8->LUTDimensions, Lut);
	if (SpStatSuccess != Status)
		return Status;

/* get variable length stuff */
	SpGetF15d16 (&Buf, Lut8->Matrix3x3, 9);
	SpGetBytes (&Buf, Lut8->InputTable, SpLut8SizeOfInputTable (Lut8));
	SpGetBytes (&Buf, Lut8->CLUT, SpLut8SizeOfClut (Lut8));
	SpGetBytes (&Buf, Lut8->OutputTable, SpLut8SizeOfOutputTable (Lut8));

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert 8bit lut from public format.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static SpStatus_t SpLut8FromPublic (
				SpLut_t		FAR *Lut,
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize)
{
	SpLut8Bit_t	FAR *Lut8;
	char		KPHUGE *Buf;
	KpUInt32_t	ClutSize;
	KpUInt32_t	InputTableSize;
	KpUInt32_t	OutputTableSize;

	Lut8 = &Lut->L.Lut8;

/* space for fixed size components */
	*BufferSize = 12 + 9 * sizeof (KpF15d16_t);

/* input table */
	InputTableSize = SpLut8SizeOfInputTable (Lut8);
	*BufferSize += InputTableSize;

/* CLUT */
	ClutSize = SpLut8SizeOfClut (Lut8);
	*BufferSize += ClutSize;

/* output table */
	OutputTableSize = SpLut8SizeOfOutputTable (Lut8);
	*BufferSize += OutputTableSize;

/* allocate a buffer */
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;

/* fill the buffer */
	*Buffer = Buf;
	SpPutUInt32 (&Buf, SpTypeLut8);
	SpPutUInt32 (&Buf, 0L);
	*Buf++ = Lut8->InputChannels;
	*Buf++ = Lut8->OutputChannels;
	*Buf++ = Lut8->LUTDimensions;
	*Buf++ = '\0';
	SpPutF15d16 (&Buf, Lut8->Matrix3x3, 9);
	SpPutBytes (&Buf, InputTableSize, Lut8->InputTable);
	SpPutBytes (&Buf, ClutSize, Lut8->CLUT);
	SpPutBytes (&Buf, OutputTableSize, Lut8->OutputTable);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Compute size of InputTable for 16 bit lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static KpUInt32_t SpLut16SizeOfInputTable (
				SpLut16Bit_t	FAR *Lut16)
{
	return ((KpUInt32_t)sizeof (KpUInt16_t) * 
                        Lut16->InputTableEntries * 
                        Lut16->InputChannels);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Compute size of OutputTable for 16 bit lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static KpUInt32_t SpLut16SizeOfOutputTable (
				SpLut16Bit_t	FAR *Lut16)
{
	return ((KpUInt32_t)sizeof (KpUInt16_t) * 
                      Lut16->OutputTableEntries * 
                      Lut16->OutputChannels);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Compute size of clut for 16 bit lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static KpUInt32_t SpLut16SizeOfClut (
				SpLut16Bit_t	FAR *Lut16)
{
	KpUInt32_t	Size;
	KpUInt32_t	i;

	Size = Lut16->LUTDimensions;
	for (i = 1; i < Lut16->InputChannels; i++)
		Size *= Lut16->LUTDimensions;

	return (Size * Lut16->OutputChannels * 
                       (KpUInt32_t)sizeof (KpUInt16_t));
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Create empty 16 Bit Lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpLut16Create (
				KpUInt16_t	InChannels,
				KpUInt16_t	InputTableEntries,
				KpUInt16_t	OutChannels,
				KpUInt16_t	OutputTableEntries,
				KpUInt16_t	LUTDimensions,
				SpLut_t		FAR *Lut)
{
	SpLut16Bit_t	FAR *Lut16;
	KpUInt32_t		Size;
	SpStatus_t		Status;

/* validate parameters */
	if ((InChannels < 1) || (8 < InChannels))
		return SpStatOutOfRange;

	if ((OutChannels < 1) || (8 < OutChannels))
		return SpStatOutOfRange;

	if ((LUTDimensions < 2) || (SPMAXLUTSIZE < LUTDimensions))
		return SpStatOutOfRange;

	if ((InputTableEntries < 1) || (SPMAXTABLESIZE < InputTableEntries))
		return SpStatOutOfRange;

	if ((OutputTableEntries < 1) || (SPMAXTABLESIZE < OutputTableEntries))
		return SpStatOutOfRange;

/* initialize all structure members */
	Lut->LutType = SpTypeLut16;
	Lut16 = &Lut->L.Lut16;
	Lut16->InputChannels = (char) InChannels;
	Lut16->OutputChannels = (char) OutChannels;
	Lut16->LUTDimensions = (char) LUTDimensions;
	SpInitMatrix (Lut16->Matrix3x3);
	Lut16->InputTableEntries = InputTableEntries;
	Lut16->InputTable = NULL;
	Lut16->CLUT = NULL;
	Lut16->OutputTableEntries = OutputTableEntries;
	Lut16->OutputTable = NULL;

	Status = SpStatSuccess;

/* input table */
	Size = SpLut16SizeOfInputTable (Lut16);
	Lut16->InputTable = SpMalloc (Size);
	if (NULL == Lut16->InputTable)
		Status = SpStatMemory;

/* CLUT */
	Size = SpLut16SizeOfClut (Lut16);
	Lut16->CLUT = SpMalloc (Size);
	if (NULL == Lut16->CLUT)
		Status = SpStatMemory;

/* output table */
	Size = SpLut16SizeOfOutputTable (Lut16);
	Lut16->OutputTable = SpMalloc (Size);
	if (NULL == Lut16->OutputTable)
		Status = SpStatMemory;

/* check for error */
	if (SpStatSuccess != Status) {
		SpFree (Lut16->CLUT);
		Lut16->CLUT = NULL;
		SpFree (Lut16->InputTable);
		Lut16->InputTable = NULL;
		SpFree (Lut16->OutputTable);
		Lut16->OutputTable = NULL;
	}

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert 16bit lut to public format.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static SpStatus_t SpLut16ToPublic (
				char		KPHUGE *Buf,
				SpLut_t		FAR *Lut)
{
	SpLut16Bit_t	FAR *Lut16;
	KpUInt32_t		InputTableSize;
	KpUInt32_t		OutputTableSize;
	KpUInt32_t		ClutSize;
	SpStatus_t		Status;
	KpF15d16_t		Matrix3x3 [9];
	int			i;

	Lut16 = &Lut->L.Lut16;

/* get fixed size elements */
	Lut16->InputChannels = *Buf++;
	Lut16->OutputChannels = *Buf++;
	Lut16->LUTDimensions = *Buf++;

/* varify pad byte is zero */
	if (0 != *Buf++)
		return SpStatOutOfRange;

/* get the matrix */
	SpGetF15d16 (&Buf, Matrix3x3, 9);

/* get number of table entries */
	Lut16->InputTableEntries = SpGetUInt16 (&Buf);
	Lut16->OutputTableEntries = SpGetUInt16 (&Buf);

/* allocate arrays */
	Status = SpLut16Create (Lut16->InputChannels, Lut16->InputTableEntries,
							Lut16->OutputChannels, Lut16->OutputTableEntries,
							Lut16->LUTDimensions, Lut);
	if (SpStatSuccess != Status)
		return Status;

/* No store Matrix since Create initializes it */
	for (i = 0; i < 9; i++)
		Lut16->Matrix3x3[i] = Matrix3x3[i];

/* get arrays */
	InputTableSize = SpLut16SizeOfInputTable (Lut16);
	SpGetUInt16s (&Buf, Lut16->InputTable, InputTableSize / 2);

	ClutSize = SpLut16SizeOfClut (Lut16);
	SpGetUInt16s (&Buf, Lut16->CLUT, ClutSize / 2);

	OutputTableSize = SpLut16SizeOfOutputTable (Lut16);
	SpGetUInt16s (&Buf, Lut16->OutputTable, OutputTableSize / 2);

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert 16bit lut from public format.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
static SpStatus_t SpLut16FromPublic (
				SpLut_t		FAR *Lut,
				void		KPHUGE * FAR *Buffer,
				KpUInt32_t	FAR *BufferSize)
{
	SpLut16Bit_t	FAR *Lut16;
	char			KPHUGE	*Buf;
	KpUInt32_t		ClutSize;
	KpUInt32_t		InputTableSize;
	KpUInt32_t		MatrixSize;
	KpUInt32_t		OutputTableSize;

	Lut16 = &Lut->L.Lut16;

/* space for fixed size components */
	*BufferSize = 12 + 2 + 2;

/* matrix */
	MatrixSize = 9 * sizeof (KpF15d16_t);
	*BufferSize += MatrixSize;

/* input table */
	InputTableSize = SpLut16SizeOfInputTable (Lut16);
	*BufferSize += InputTableSize;

/* CLUT */
	ClutSize = SpLut16SizeOfClut (Lut16);
	*BufferSize += ClutSize;

/* output table */
	OutputTableSize = SpLut16SizeOfOutputTable (Lut16);
	*BufferSize += OutputTableSize;

/* allocate a buffer */
	Buf = SpMalloc (*BufferSize);
	if (NULL == Buf)
		return SpStatMemory;

/* fill the buffer */
	*Buffer = Buf;
	SpPutUInt32 (&Buf, SpTypeLut16);
	SpPutUInt32 (&Buf, 0L);
	*Buf++ = Lut16->InputChannels;
	*Buf++ = Lut16->OutputChannels;
	*Buf++ = Lut16->LUTDimensions;
	*Buf++ = '\0';
	SpPutF15d16 (&Buf, Lut16->Matrix3x3, 9);
	SpPutUInt16 (&Buf, Lut16->InputTableEntries);
	SpPutUInt16 (&Buf, Lut16->OutputTableEntries);
	SpPutUInt16s (&Buf, Lut16->InputTable, InputTableSize / 2);
	SpPutUInt16s (&Buf, Lut16->CLUT, ClutSize / 2);
	SpPutUInt16s (&Buf, Lut16->OutputTable, OutputTableSize / 2);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert lut to public format.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
SpStatus_t SpLutToPublic (
				char	KPHUGE *Buf,
				SpLut_t	FAR *Lut)
{

/* get type of lut */
	switch (Lut->LutType) {
	case SpTypeLut8:
		return SpLut8ToPublic (Buf, Lut);

	case SpTypeLut16:
		return SpLut16ToPublic (Buf, Lut);
	}

	return SpStatBadLutType;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert lut from public format.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 5, 1993
 *------------------------------------------------------------------*/
SpStatus_t SpLutFromPublic (
				SpLut_t		FAR *Lut,
				KpUInt32_t	FAR *BufferSize,
				void		KPHUGE * FAR *Buffer)
{

/* determine type of lut */
	switch (Lut->LutType) {
	case SpTypeLut8:
		return SpLut8FromPublic (Lut, Buffer, BufferSize);

	case SpTypeLut16:
		return SpLut16FromPublic (Lut, Buffer, BufferSize);
	}

	return SpStatBadLutType;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Free a lut.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	April 21, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpLutFree (
				SpLut_t		FAR *Lut)
{
	switch (Lut->LutType) {
	case SpTypeLut8:
		SpFree (Lut->L.Lut8.InputTable);
		SpFree (Lut->L.Lut8.CLUT);
		SpFree (Lut->L.Lut8.OutputTable);
		break;

	case SpTypeLut16:
		SpFree (Lut->L.Lut16.InputTable);
		SpFree (Lut->L.Lut16.CLUT);
		SpFree (Lut->L.Lut16.OutputTable);
		break;

	default:
		return SpStatBadTagType;
	}

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
* DESCRIPTION
 *      Create a Transform with a PT from a Lut
 *
 * AUTHOR
 *      doro
 *
 * DATE CREATED
 *      December 27, 1995
 *------------------------------------------------------------------*/

SpStatus_t SPAPI SpXformFromLut(SpLut_t		Lut,
				SpTransRender_t	WhichRender,
				SpTransType_t	WhichTransform,
				SpSig_t		SpaceIn,
				SpSig_t		SpaceOut,
				KpF15d16XYZ_t	HdrWhite,
				KpF15d16XYZ_t	MedWhite,
				KpUInt32_t	ChainIn,
				KpUInt32_t	ChainOut,
				SpXform_t	FAR *Xform)
{
void	KPHUGE	*Buf;
KpUInt32_t	BufSize;

SpStatus_t	Status;

SpXformData_t   FAR *XformData;

	/* Allocate the Space */
	Status = SpXformAllocate (Xform);
	if (SpStatSuccess != Status)
	{
		*Xform = NULL;
		return Status;
	}

	/* Set Up Private Structure to Populate */
	XformData = lockBuffer ((KcmHandle)*Xform);
	if (NULL == XformData)
	{
		SpXformFree (Xform);
		*Xform = NULL;
		return SpStatBadXform;
	}

	/* Signature is set in XfromAllocate */

	/* Check Lut Type for Legal Value */
	if (Lut.LutType == SpTypeLut8) {
		XformData->LutType        = SpTypeLut8;
		XformData->LutSize        = 8;
	} else {
		XformData->LutType        = SpTypeLut16;
		XformData->LutSize        = 16;
		Lut.LutType               = SpTypeLut16;
	}

	/* Get the Buffer and Size to checking to the Color Processor */
	Status = SpLutFromPublic(&Lut, &BufSize, &Buf);
	if (SpStatSuccess != Status)
	{
		SpFree (XformData);
		return Status;
	}

	/* Get the PTRefNum for the Checked in Lut */
	Status = SpXformLoadImp(Buf, BufSize, &XformData->PTRefNum);

	SpFree(Buf);

	/* Initialize the PT Color Spaces */
	if (SpStatSuccess == Status)
		Status = SpXformInitColorSpace (XformData->PTRefNum, 
						KCM_SPACE_IN, SpaceIn);
	if (SpStatSuccess == Status)
		Status = SpXformInitColorSpace (XformData->PTRefNum, 
						KCM_SPACE_OUT,
						SpaceOut);

	/* Initialize the PT Chaining Values */
	if (SpStatSuccess == Status)
		Status = SpSetKcmAttrInt(XformData->PTRefNum,
					 KCM_OUT_CHAIN_CLASS_2,
					 ChainOut);
	if (SpStatSuccess == Status)
		Status = SpSetKcmAttrInt(XformData->PTRefNum,
					 KCM_IN_CHAIN_CLASS_2,
					 ChainIn);

	/* If something Failed, Free the Xform Space 
	   and report the error */
	if (SpStatSuccess != Status)
	{
		SpXformFree (Xform);
		*Xform = NULL;
		return Status;
	}

	/* Fill in the rest of the information */
	XformData->WhichRender    = WhichRender;
	XformData->WhichTransform = WhichTransform;
	XformData->SpaceIn        = SpaceIn;
	XformData->SpaceOut       = SpaceOut;
	XformData->ChainIn        = ChainIn;
	XformData->ChainOut       = ChainOut;
	XformData->HdrWtPoint.X   = HdrWhite.X;
	XformData->HdrWtPoint.Y   = HdrWhite.Y;
	XformData->HdrWtPoint.Z   = HdrWhite.Z;
	XformData->MedWtPoint.X   = MedWhite.X;
	XformData->MedWtPoint.Y   = MedWhite.Y;
	XformData->MedWtPoint.Z   = MedWhite.Z;
	if (HdrWhite.X + HdrWhite.Y + HdrWhite.Z > 0.0)
		XformData->HdrWPValid = KPTRUE;
	if (MedWhite.X + MedWhite.Y + MedWhite.Z > 0.0)
		XformData->MedWPValid = KPTRUE;

	unlockBuffer ((KcmHandle)*Xform);
	return SpStatSuccess;
}


