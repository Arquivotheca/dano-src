/*********************************************************************/
/*	ShaperMatrix.c

	These routines convert a pt into a set of shaper tables followed by
	a matrix.
	
	They are based on the MATLAB routine pt2shmtx.m developed by Doug Walker

	The call is:
	
		SpStatus_t ComputeShaperMatrix (
				PTRefNum_t	pt,
				double		FAR *shaper [3],
				double		ColorMatrix [3] [3]);
	
	where
	
		pt = the input pt to be converted
		
		shaper[i][j] = the shaper table for color i and index j
		ColorMatrix[i][j] = the matrix for RGB index i and XYZ index j
		
	The results should approimate
	
	RCStoXYZ(pt([R,G,B])) = Sum(k) ( ShapedRGB[k] * ColorMatrix[k,:])
		where ShapedRGB = [ shaper[0][R], shaper[1][B], shaper[2][G] ]
		
	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPXFMTAG.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spxfmtag.c_v  $
		$Revision:   1.5  $
		$Date:   22 Nov 1994 11:51:23  $
		$Author:   lsh  $

	SCCS Revision:
	@(#)spxfmtag.c	1.23	11/22/97
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
 *** COPYRIGHT (c) Eastman Kodak Company, 1993 -1997                ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Generate Colorant and Response Curve tags for the specified PT.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	April 15, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformCreateMatTagsFromPT (
				SpProfile_t		Profile,
				PTRefNum_t		RefNum)
{
	SpStatus_t		Status;

	if (SpNoFPU == SpIsFPUpresent()) {
/*		DebugStr("\p software FPU");		*/
		Status = SpXformCreateMatTagsFromPTnoFPU (Profile, RefNum);
	} 
	else {
/*		DebugStr("\p hardware FPU");		*/
		Status = SpXformCreateMatTagsFromPTFPU (Profile, RefNum);
	} 
	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Generate Colorant and Response Curve tags for the specified
 *	Xform data.  This data is assumed to be a PT.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	April 15, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformCreateMatTags (
				SpProfile_t		Profile,
				KpInt32_t		DataSize,
				KpLargeBuffer_t	Data)
{
	SpStatus_t		Status;
	PTRefNum_t		RefNum;

	Status = SpXformLoadImp (Data, DataSize, &RefNum);
	if (SpStatSuccess != Status)
		return Status;

	Status = SpXformCreateMatTagsFromPT (Profile, RefNum);
	PTCheckOut (RefNum);

	return Status;
}

/*---------------------------------------------------------------------
	DESCRIPTION
	This function transforms a pel array (in place) using a pt
	The array is assumed to be in plane-sequential order
	The data type is assumed to be 12-bit (KCM_USHORT_12)

	INPUTS	
	PTRefNum_t	pt			---	reference # of the pt to be transformed
	KpUInt16_t	FAR *Pels	---	pointer to the data
	int		nPels			---	number of pixels in data stream

	OUTPUTS
	KpUInt16_t	FAR *Pels	--- pointer to data that has been xformed

	RETURNS
	SpStatus_t - SpStatSuccess if successful, otherwise an sprofile error
 
   AUTHOR
   mtobin
-------------------------------------------------------------------------*/
SpStatus_t Transform12BPels (
				PTRefNum_t	pt,
				KpUInt16_t	FAR *Pels,
				int		nPels)
{
	opRefNum_t		opRefNum;
	PTEvalDTPB_t	dtpb;
	PTErr_t			PTStat;
	KpInt32_t		i;
	PTCompDef_t		comps [3];
	SpStatus_t		Status;

	for (i = 0; i < 3; i++) {
		comps [i].pelStride = 3 * sizeof(*Pels);
		comps [i].lineStride = nPels * 3 * sizeof (*Pels);
		comps [i].addr = (void FAR *) (Pels+i);
	}

/* build color processor image structure */
	dtpb.nPels = nPels;
	dtpb.nLines = 1;
	dtpb.nInputs = 3;
	dtpb.input = comps;
	dtpb.dataTypeI = KCM_USHORT_12;
	dtpb.nOutputs = 3;
	dtpb.output = comps;  /* evaluate in place */
	dtpb.dataTypeO = KCM_USHORT_12;

/* do the transformation */
	PTStat = PTEvalDT (pt, &dtpb, KCP_EVAL_DEFAULT, 0, 1, &opRefNum, NULL);

/* translate the return value to an Sp error */
	Status = SpStatusFromPTErr(PTStat);
	
	return Status;
}

/* ---------------------------------------------------------------------- */
SpStatus_t TransformPels (
				PTRefNum_t	pt,
				KpUInt8_t	FAR *Pels,
				int			nPels)
{
/*
 * Transform the pel array using the pt
 * The array is assumes to be in plane-sequential order
 */

	PTEvalPB_t		dtpb;
	PTCompDef_t		comps [3];
	opRefNum_t		opRefNum;
	int				i;
	PTErr_t			pterr;

	for (i = 0; i < 3; i++) {
		comps [i].pelStride = 3 * sizeof(*Pels);
		comps [i].lineStride = nPels * 3 * sizeof (*Pels);
		comps [i].addr = (void FAR *) (Pels+i);
	}

	dtpb.nPels = nPels;
	dtpb.nLines = 1;
	dtpb.nOutputs =
	dtpb.nInputs = 3;
	dtpb.output =
	dtpb.input = comps;

	pterr = PTEval (pt, &dtpb, KCP_EVAL_DEFAULT, 0, 1, &opRefNum, NULL);

	return (SpStatusFromPTErr(pterr));
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Generate Colorant and Response Curve tags for the specified Xform.
 *
 * AUTHOR
 * 	lsh (modified by doro)
 *
 * DATE CREATED
 *	August 5, 1997
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformCreateMatTagsFromXform (
				SpProfile_t		Profile,
				SpXform_t		Xform)
{
	SpStatus_t		Status;

	if (SpNoFPU == SpIsFPUpresent()) {
/*		DebugStr("\p software FPU");		*/
		Status = SpXformCreateMatTagsFromXformnoFPU (Profile, Xform);
	} 
	else {
/*		DebugStr("\p hardware FPU");		*/
		Status = SpXformCreateMatTagsFromXformFPU (Profile, Xform);
	} 
	return Status;
}

