/*********************************************************************/
/*
	Contains:	This module contains a conversion function to convert
				Color Processor Transforms (RefNums) to Standard
				profile transforms (SpXform_t).

				Created by lsh, Feb. 5, 1994

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1995 by Eastman Kodak Company, 
                            all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPXFROMR.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spxfromr.c_v  $
		$Revision:   2.2  $
		$Date:   07 Apr 1994 13:24:24  $
		$Author:   lsh  $

	SCCS Revision:
		@(#)spxfromr.c	1.28   11/14/95

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
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1995                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"
#include <stdio.h>

static int 			getOtblMax(int16*);


/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function checks to determine if the PT can be inverted
 *
 * AUTHOR
 * PGT
 *
 * DATE CREATED
 * May 11, 1994
 *--------------------------------------------------------------------*/
static KpBool_t InvertOk (
				PTRefNum_t		RefNum,
				KcmAttribute	senseAttrib)
{
	KpInt32_t			sense;
	KcmAttribute	invertAttrib;

/* See if the attribute is the right type */
	switch (senseAttrib) {
	case KCM_MEDIUM_SENSE_IN:
		invertAttrib = KCM_SENSE_INVERTIBLE_IN;
		break;

	case KCM_MEDIUM_SENSE_OUT:
		invertAttrib = KCM_SENSE_INVERTIBLE_OUT;
		break;

	default:
		return KPFALSE;
	}

/*
 * If the sense attribute is not in this PT, then this PT cannot be
 * inverted for that sense because it is a color space for which
 * inverting is not appropriate (e.g., RCS)
 */
	sense = SpGetKcmAttrInt (RefNum, senseAttrib);
	if (KCM_UNKNOWN == sense)
		return KPFALSE;

/*
 * Make sure that the PT is allowed to be inverted:
 *  if the attribute KCM_SENSE_INVERTIBLE==KCM_IS_INVERTIBLE
 *  if KCM_SENSE_INVERTIBLE is missing and KCM_CLASS==KCM_OUTPUT_CLASS
 */
	switch (SpGetKcmAttrInt (RefNum, invertAttrib)) {
	case KCM_IS_NOT_INVERTIBLE:
		return KPFALSE;

	case KCM_IS_INVERTIBLE:
		return KPTRUE;
	}

	if (KCM_OUTPUT_CLASS == SpGetKcmAttrInt (RefNum, KCM_CLASS))
		return KPTRUE;

	return KPFALSE;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function determines if a table handle has been
 * processed before.
 *
 * AUTHOR
 * PGT
 *
 * DATE CREATED
 * May 12, 1994
 *--------------------------------------------------------------------*/
static KpBool_t UniqueTable (
				KcmHandle	tbl,
				KcmHandle	FAR *tblList,
				int			numTbls)
{
	int	i;

	for (i = 0; i < numTbls; ++i) {
		if (tbl == tblList [i])
			return   KPFALSE;
	}

	return KPTRUE;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function inverts the output tables of a PT
 *
 * AUTHOR
 * PGT
 *
 * DATE CREATED
 * May 11, 1994
 *--------------------------------------------------------------------*/
static KpBool_t InvertOutputTables (
				PTRefNum_t	refNum,
				KpInt32_t	numOutChan)
{
	int			i, k;
	KpBool_t		status;
	PTErr_t		cpStatus;
	KcmHandle	otblHandle;
	KpInt16_t	FAR *otbl;
	KcmHandle	FAR *tblList;
	int			numTbls;
	int			otblMax;

/*
 * Allocate an array for holding previously processed
 * table handles to avoid doing a table twice.
 */
	tblList = (KcmHandle *)allocBufferPtr (numOutChan * 
                                (KpInt32_t)sizeof(*tblList));
	if (tblList == NULL)
		return KPFALSE;
	
/*
 * Get the Color Processor reference number for this PT
 */
	status = KPTRUE;
	numTbls = 0;
	for (i = 0; (i < numOutChan) && status; ++i) {
		cpStatus = PTGetOtbl (refNum, i, &otblHandle);
		if (cpStatus == KCP_NO_OUTTABLE)
			;
		else if (cpStatus != KCP_SUCCESS)
			status = KPFALSE;
		else if (UniqueTable (otblHandle, tblList, numTbls)) {
			otbl = lockBuffer (otblHandle);
			if (otbl != NULL) {
				otblMax = getOtblMax(otbl);
				for( k=0; k<4096; ++k )
					otbl[k] = (int16)(otblMax - otbl[k]);
				unlockBuffer (otblHandle);
				tblList [numTbls++] = otblHandle;
			}
			else
				status = KPFALSE;
		}
	}

	if (tblList != NULL)
		freeBufferPtr (tblList);

	return   status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function inverts the input tables of a PT
 *
 * AUTHOR
 * PGT
 *
 * DATE CREATED
 * May 11, 1994
 *--------------------------------------------------------------------*/
static KpBool_t InvertInputTables (
				PTRefNum_t	refNum,
				KpInt32_t	numOutChan,
				KpInt32_t	FAR *numInVar)
{
	int			i, j, k;
	KpBool_t		status;
	PTErr_t		cpStatus;
	KcmHandle	itblHandle;
	KpInt32_t	FAR *itbl;
	KpInt32_t	itblTemp;
	KcmHandle	FAR *tblList;
	int			maxTbls;
	int			numTbls;

/*
 * Find out the maximum number of tables this PT may have
 * and allocate some memory to hold handle values
 */
	for (i = 0, maxTbls = 0; i < numOutChan; ++i)
		maxTbls += (int) numInVar [i];

	numTbls = 0;
	tblList = (KcmHandle*) allocBufferPtr (maxTbls * 
                                (KpInt32_t)sizeof(*tblList));
	if (NULL == tblList)
		return   KPFALSE;

/*
 * Get the Color Processor reference number for this PT
 */
	status = KPTRUE;
	for (i = 0, numTbls = 0; (i < numOutChan) && status; ++i) {
		for (j = 0; j < numInVar[i] && status; ++j) {
			cpStatus = PTGetItbl (refNum, i, j, &itblHandle);
			if (cpStatus == KCP_NO_INTABLE)	/* no input tbl, ignore */
				;
			else if (cpStatus != KCP_SUCCESS)
				status = KPFALSE;
			else if (UniqueTable (itblHandle, tblList, numTbls)) {
				tblList [numTbls++] = itblHandle;
				itbl = lockBuffer (itblHandle);
				if (itbl != NULL) {
					for (k = 0; k < 128; ++k) {
						itblTemp     = itbl [k];
						itbl [k]     = itbl [255-k];
						itbl [255-k] = itblTemp;
					}
					itbl [256] = itbl [255];
					unlockBuffer (itblHandle);
				}
			}
		}
	}

	if (tblList != NULL)
		freeBufferPtr (tblList);

	return   status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function obtains the number of input and output
 * channels of a PT
 *
 * AUTHOR
 * PGT
 *
 * DATE CREATED
 * May 11, 1994
 *--------------------------------------------------------------------*/
static void getNumChans (
				PTRefNum_t	RefNum,
				KpInt32_t	FAR *numOutChan,
				KpInt32_t	FAR *numInVar)
{
	KcmAttribute	attrib;
	int				i;

/*
 * first get number of output variables and then the
 * number of input variables for each output variable
 */
	*numOutChan = SpGetKcmAttrInt (RefNum, KCM_NUM_VAR_OUT);
	for (i = 1; i <= *numOutChan; ++i) {
		switch (i) {
		case 1:
			attrib = KCM_NUM_VAR_1_IN;
			break;
		case 2:
			attrib = KCM_NUM_VAR_2_IN;
			break;
		case 3:
			attrib = KCM_NUM_VAR_3_IN;
			break;
		case 4:
			attrib = KCM_NUM_VAR_4_IN;
			break;
		case 5:
			attrib = KCM_NUM_VAR_5_IN;
			break;
		case 6:
			attrib = KCM_NUM_VAR_6_IN;
			break;
		case 7:
			attrib = KCM_NUM_VAR_7_IN;
			break;
		case 8:
			attrib = KCM_NUM_VAR_8_IN;
			break;
		default:
			attrib = KCM_NUM_VAR_1_IN;
			break;
		}
		numInVar [i-1] = SpGetKcmAttrInt (RefNum, attrib);
	}
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 * This function inverts the sense of either the input or
 * output side of a transform, creating a new transform. The
 * attribute senseAttrib can be either KCM_MEDIUM_SENSE_IN or
 * KCM_MEDIUM_SENSE_OUT
 *
 * AUTHOR
 * PGT
 *
 * DATE CREATED
 * May 11, 1994
 *
 *--------------------------------------------------------------------*/
static KpBool_t InvertXformSense (
				PTRefNum_t		RefNum,
				KcmAttribute	senseAttrib)
{
	KpInt32_t		numInVar [8];
	KpInt32_t		numOutChan;

/* Check for proper parameters */
	if (!InvertOk (RefNum, senseAttrib))
		return KPFALSE;

/* Find out the number of channels in the PT */
	getNumChans (RefNum, &numOutChan, numInVar);

	switch (senseAttrib) {
	case KCM_MEDIUM_SENSE_IN:
		return InvertInputTables (RefNum, numOutChan, numInVar);

	case KCM_MEDIUM_SENSE_OUT:
		return InvertOutputTables (RefNum, numOutChan);
	}

	return   KPFALSE;
}


/***************************************************************************
 * FUNCTION NAME
 *	SpXformInvert
 *
 * DESCRIPTION
 *	This function inverts the transform specified by Xform.  If invertInp
 *	is true the input tables are inverted and if invertOut is true then
 *	the output tables are inverted.  
 *
 *	This function should be in the profile processor.  It was added here
 *	temporarily because the profile processor code was frozen at the time
 *	this function was created.
 *
 ***************************************************************************/
SpStatus_t SPAPI SpXformInvert (
				SpXform_t Xform, 
				KpBool_t invertInp, 
				KpBool_t invertOut)
{

	PTRefNum_t		refNum;
	KpBool_t		ok;
	SpStatus_t		spStatus;

	/*	Get the PTRefNum for the transform	*/
	spStatus = SpXformGetRefNum (Xform, &refNum);
	if (SpStatSuccess != spStatus) {
		return spStatus;
	}

	/*	Invert the input tables	*/
	if (invertInp) {

		/*	Set the KCM_MEDIUM_SENSE_IN attribute to KCM_POSITIVE, and the
			KCM_SENSE_INVERTIBLE_IN attribute to KCM_IS_INVERTIBLE			*/
		spStatus = SpSetKcmAttrInt (refNum, KCM_SENSE_INVERTIBLE_IN,
									KCM_IS_INVERTIBLE);  
		if (SpStatSuccess != spStatus) {
			return spStatus;
		}
		spStatus = SpSetKcmAttrInt (refNum, KCM_MEDIUM_SENSE_IN,
									KCM_POSITIVE);  
		if (SpStatSuccess != spStatus) {
			return spStatus;
		}
		ok = InvertXformSense (refNum, KCM_MEDIUM_SENSE_IN);
		if (!ok) {
			return SpStatFailure;
		}
	}

	/*	Invert the output tables */
	if (invertOut) {

		/*	Set the KCM_MEDIUM_SENSE_OUT attribute to KCM_POSITIVE, and the
			KCM_SENSE_INVERTIBLE_OUT attribute to KCM_IS_INVERTIBLE			*/
		spStatus = SpSetKcmAttrInt (refNum, KCM_SENSE_INVERTIBLE_OUT,
									KCM_IS_INVERTIBLE);  
		if (SpStatSuccess != spStatus) {
			return spStatus;
		}
		spStatus = SpSetKcmAttrInt (refNum, KCM_MEDIUM_SENSE_OUT,
									KCM_POSITIVE);  
		if (SpStatSuccess != spStatus) {
			return spStatus;
		}
		ok = InvertXformSense (refNum, KCM_MEDIUM_SENSE_OUT);
		if (!ok) {
			return SpStatFailure;
		}
	}
	return SpStatSuccess;

}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Construct Xform given a PT reference number.
 *	FOR PT2PF building ONLY !!
 *
 * AUTHOR
 * 	lcc
 *
 * DATE CREATED
 *	February 9, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformFromPTRefNumCombine (
				SpConnectType_t	ConnectType,
				SpParadigm_t	ParadigmType,
				PTRefNum_t		FAR *RefNum,
				SpXform_t		FAR *Xform)
{
	KpInt32_t	SpaceIn, SpaceOut;
	KpInt32_t	SenseIn, SenseOut;
	KpInt32_t	Class, Render;
	int			Index;
	KpInt32_t	FailXform;
	SpStatus_t	Status;
	PTRefNum_t	CvrtInRefNum, CvrtOutRefNum;
	PTRefNum_t	RefNumList [3];
	PTRefNum_t	NewRefNum;

	Class = SpGetKcmAttrInt (*RefNum, KCM_CLASS);
	SpaceIn = SpGetKcmAttrInt (*RefNum, KCM_SPACE_IN);
	SpaceOut = SpGetKcmAttrInt (*RefNum, KCM_SPACE_OUT);
	SenseIn = SpGetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_IN);
	SenseOut = SpGetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_OUT);

/* setup to fix color spaces */
	Index = 0;
	Status = SpStatSuccess;

	KpEnterCriticalSection (&SpCacheCritFlag);

/* setup to fix input color space */
	if (KCM_RCS == SpaceIn) {
		Render = SpGetKcmAttrInt (*RefNum, KCM_COMPRESSION_OUT);
		if (Render == KCM_UNKNOWN) Render = KCM_PERCEPTUAL;
		Status = SpXformBuildCnvrt (KPTRUE, Render, ConnectType, ParadigmType, &CvrtInRefNum);
		if (SpStatSuccess == Status) {
			RefNumList [Index++] = CvrtInRefNum;
			RefNumList [Index++] = *RefNum;
		}
	}
	else
		RefNumList [Index++] = *RefNum;

/* setup to fix output color space */
	if (KCM_RCS == SpaceOut) {
		Render = KCM_PERCEPTUAL;
		if (SpStatSuccess == Status)
			Status = SpXformBuildCnvrt (KPFALSE, Render, ConnectType, ParadigmType, &CvrtOutRefNum);

		if (SpStatSuccess == Status)
			RefNumList [Index++] = CvrtOutRefNum;
	}

/* fix the color spaces */
	if ((SpStatSuccess == Status) && (1 != Index)) {
		Status = SpConnectSequenceCombine (ConnectType, Index, RefNumList,
					&NewRefNum, &FailXform, NULL, NULL);
		PTCheckOut (*RefNum);
		*RefNum = NewRefNum;
	}

	KpLeaveCriticalSection (&SpCacheCritFlag);

	if (SpStatSuccess != Status)
		return Status;

/* Invert input, if negative */
	if ((KCM_RCS != SpaceIn) && (KCM_NEGATIVE == SenseIn)) {
		if (!InvertXformSense (*RefNum, KCM_MEDIUM_SENSE_IN)) {
			PTCheckOut (*RefNum);
			return SpStatFailure;
		}
		Status = SpSetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_IN, KCM_POSITIVE);
		if (SpStatSuccess != Status) {
			PTCheckOut (*RefNum);
			return Status;
		}
	}

/* Invert output, if negative */
	if ((KCM_RCS != SpaceOut) && (KCM_NEGATIVE == SenseOut)) {
		if (!InvertXformSense (*RefNum, KCM_MEDIUM_SENSE_OUT)) {
			PTCheckOut (*RefNum);
			return SpStatFailure;
		}
		Status = SpSetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_OUT, KCM_POSITIVE);
		if (SpStatSuccess != Status) {
			PTCheckOut (*RefNum);
			return Status;
		}
	}

	Status = SpSetKcmAttrInt (*RefNum, KCM_CLASS, Class);
	Status = SpXformFromPTRefNumImp (*RefNum, Xform);
	*RefNum = 0;

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Construct Xform given a PT reference number.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 25, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformFromPTRefNumEx (
				SpConnectType_t	ConnectType,
				SpParadigm_t	ParadigmType,
				PTRefNum_t		FAR *RefNum,
				SpXform_t		FAR *Xform)
{
	KpInt32_t	SpaceIn, SpaceOut;
	KpInt32_t	SenseIn, SenseOut;
	KpInt32_t	Class, Render;
	int			Index;
	KpInt32_t	FailXform;
	SpStatus_t	Status;
	PTRefNum_t	CvrtInRefNum, CvrtOutRefNum;
	PTRefNum_t	RefNumList [3];
	PTRefNum_t	NewRefNum;

	Class = SpGetKcmAttrInt (*RefNum, KCM_CLASS);
	SpaceIn = SpGetKcmAttrInt (*RefNum, KCM_SPACE_IN);
	SpaceOut = SpGetKcmAttrInt (*RefNum, KCM_SPACE_OUT);
	SenseIn = SpGetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_IN);
	SenseOut = SpGetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_OUT);

/* setup to fix color spaces */
	Index = 0;
	Status = SpStatSuccess;

	KpEnterCriticalSection (&SpCacheCritFlag);

/* setup to fix input color space */
	if (KCM_RCS == SpaceIn) {
		Render = SpGetKcmAttrInt (*RefNum, KCM_COMPRESSION_OUT);
		if (Render == KCM_UNKNOWN) Render = KCM_PERCEPTUAL;
		Status = SpXformBuildCnvrt (KPTRUE, Render, ConnectType, ParadigmType, &CvrtInRefNum);
		if (SpStatSuccess == Status) {
			RefNumList [Index++] = CvrtInRefNum;
			RefNumList [Index++] = *RefNum;
		}
	}
	else
		RefNumList [Index++] = *RefNum;

/* setup to fix output color space */
	if (KCM_RCS == SpaceOut) {
		Render = KCM_PERCEPTUAL;
		if (SpStatSuccess == Status)
			Status = SpXformBuildCnvrt (KPFALSE, Render, 
				ConnectType, ParadigmType, &CvrtOutRefNum);

		if (SpStatSuccess == Status)
			RefNumList [Index++] = CvrtOutRefNum;
	}

/* fix the color spaces */
	if ((SpStatSuccess == Status) && (1 != Index)) {
		Status = SpConnectSequenceImp (ConnectType, Index, RefNumList,
									&NewRefNum, &FailXform, NULL, NULL);
		PTCheckOut (*RefNum);
		*RefNum = NewRefNum;
	}

	KpLeaveCriticalSection (&SpCacheCritFlag);

	if (SpStatSuccess != Status)
		return Status;

/* Invert input, if negative */
	if ((KCM_RCS != SpaceIn) && (KCM_NEGATIVE == SenseIn)) {
		if (!InvertXformSense (*RefNum, KCM_MEDIUM_SENSE_IN)) {
			PTCheckOut (*RefNum);
			return SpStatFailure;
		}
		Status = SpSetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_IN, KCM_POSITIVE);
		if (SpStatSuccess != Status) {
			PTCheckOut (*RefNum);
			return Status;
		}
	}

/* Invert output, if negative */
	if ((KCM_RCS != SpaceOut) && (KCM_NEGATIVE == SenseOut)) {
		if (!InvertXformSense (*RefNum, KCM_MEDIUM_SENSE_OUT)) {
			PTCheckOut (*RefNum);
			return SpStatFailure;
		}
		Status = SpSetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_OUT, KCM_POSITIVE);
		if (SpStatSuccess != Status) {
			PTCheckOut (*RefNum);
			return Status;
		}
	}

	Status = SpSetKcmAttrInt (*RefNum, KCM_CLASS, Class);
	Status = SpXformFromPTRefNumImp (*RefNum, Xform);
	*RefNum = 0;

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Construct Xform given a PT reference number using default grid size.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 17, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformFromPTRefNum (
				PTRefNum_t		FAR *RefNum,
				SpXform_t		FAR *Xform)
{
	return SpXformFromPTRefNumEx (SpGetConnectType (), SpParadigmRel, RefNum, Xform);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Create an SpXform_t from a data block containing a PT.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	February 6, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformCreateFromDataEx (
				SpConnectType_t	ConnectType,
				KpInt32_t		Size,
				KpLargeBuffer_t	Data,
				SpXform_t		FAR *Xform)
{
	SpStatus_t	Status;
	PTRefNum_t	RefNum;

	*Xform = NULL;

	Status = SpXformLoadImp (Data, Size, &RefNum);
	if (SpStatSuccess != Status)
		return Status;

	return SpXformFromPTRefNumEx (ConnectType, SpParadigmRel, &RefNum, Xform);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Create an SpXform_t from a data block containing a PT using the
 *	default grid size.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	February 6, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformCreateFromData (
				KpInt32_t		Size,
				KpLargeBuffer_t	Data,
				SpXform_t		FAR *Xform)
{
	return SpXformCreateFromDataEx (SpGetConnectType (), Size, Data, Xform);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Construct Xform given a PT reference number.  No RCS -> LAB color
 *	space conversion is performed.
 *
 * AUTHOR
 * 	mjb
 *
 * DATE CREATED
 *	September 27, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformFromPTRefNumNC (
				PTRefNum_t		FAR *RefNum,
				SpXform_t		FAR *Xform)
{
	KpInt32_t	SpaceIn, SpaceOut;
	KpInt32_t	SenseIn, SenseOut;
	KpInt32_t	Class;
	int			Index;
	SpStatus_t	Status;

	Class = SpGetKcmAttrInt (*RefNum, KCM_CLASS);
	SpaceIn = SpGetKcmAttrInt (*RefNum, KCM_SPACE_IN);
	SpaceOut = SpGetKcmAttrInt (*RefNum, KCM_SPACE_OUT);
	SenseIn = SpGetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_IN);
	SenseOut = SpGetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_OUT);

	Index = 0;
	Status = SpStatSuccess;

/* Invert input, if negative */
	if ((KCM_RCS != SpaceIn) && (KCM_NEGATIVE == SenseIn)) {
		if (!InvertXformSense (*RefNum, KCM_MEDIUM_SENSE_IN)) {
			PTCheckOut (*RefNum);
			*RefNum = 0;
			return SpStatFailure;
		}
		Status = SpSetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_IN, KCM_POSITIVE);
		if (SpStatSuccess != Status) {
			PTCheckOut (*RefNum);
			*RefNum = 0;
			return Status;
		}
	}

/* Invert output, if negative */
	if ((KCM_RCS != SpaceOut) && (KCM_NEGATIVE == SenseOut)) {
		if (!InvertXformSense (*RefNum, KCM_MEDIUM_SENSE_OUT)) {
			PTCheckOut (*RefNum);
			return SpStatFailure;
		}
		Status = SpSetKcmAttrInt (*RefNum, KCM_MEDIUM_SENSE_OUT, KCM_POSITIVE);
		if (SpStatSuccess != Status) {
			PTCheckOut (*RefNum);
			*RefNum = 0;
			return Status;
		}
	}

	Status = SpSetKcmAttrInt (*RefNum, KCM_CLASS, Class);
	if (SpStatSuccess != Status) {
		PTCheckOut (*RefNum);
		*RefNum = 0;
		return Status;
	}
	Status = SpXformFromPTRefNumImp (*RefNum, Xform);
	*RefNum = 0;

	return Status;

}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Create an SpXform_t from a data block containing a PT using the
 *	default grid size.  There is no RCS->LAB color space conversion.
 *	Used for importing the raw PT.
 *
 * AUTHOR
 * 	mjb
 *
 * DATE CREATED
 *	September 27, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformCreateFromDataNC (
				KpInt32_t		Size,
				KpLargeBuffer_t	Data,
				SpXform_t		FAR *Xform)
{

	SpStatus_t	Status;
	PTRefNum_t	RefNum;

	*Xform = NULL;

	Status = SpXformLoadImp (Data, Size, &RefNum);
	if (SpStatSuccess != Status)
		return Status;

	return SpXformFromPTRefNumNC (&RefNum, Xform);

}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * invertOutputTables
 *
 * DESCRIPTION
 * This function inverts the output tables of a PT
 *
 * AUTHOR
 * PGT
 *
 * DATE CREATED
 * May 11, 1993
 *
 *--------------------------------------------------------------------*/
static int getOtblMax(int16 *otbl)
{
	int	i;
	int	tblMax=4080;

	for( i=0; i<4096; ++i )
		if( otbl[i] > tblMax )
			tblMax = otbl[i];

	if( tblMax > 4080 )
		tblMax = 4095;

	return  tblMax;
}

