/*********************************************************************/
/*
	Contains:	This module contains more transform functions.

				Created by lsh, June 21, 1994

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, 
                            all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile$
		$Logfile$
		$Revision$
		$Date$
		$Author$

	SCCS Revision:
		@(#)spxfblob.c	1.7  11/17/97

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
#include <string.h>


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get size of data block needed to hold a transform.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	Jne 20, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformToBlobGetDataSize (
				SpXform_t	Xform,
				KpInt32_t	FAR *BufferSize)
{
	SpXformData_t	FAR *XformData;
	KpInt32_t		DataSize;
	PTErr_t			PTStat;

	XformData = SpXformLock (Xform);
	if (NULL == XformData)
		return SpStatBadXform;

	PTStat = PTGetSizeF (XformData->PTRefNum, PTTYPE_FUTF, &DataSize);
	if (KCP_SUCCESS != PTStat) {
		SpXformUnlock (Xform);
		return SpStatusFromPTErr(PTStat);
	}

	*BufferSize = (KpInt32_t)sizeof (*XformData) + DataSize;

	SpXformUnlock (Xform);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Get transform into a block of data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	Jne 20, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformToBlobGetData (
				SpXform_t		Xform,
				KpInt32_t		BufferSize,
				SpHugeBuffer_t	Buffer)
{
	SpStatus_t		Status;
	SpXformData_t	FAR *XformData;
	KpInt32_t		CheckSize;
	PTErr_t			PTStat;

	Status = SpXformToBlobGetDataSize (Xform, &CheckSize);
	if (SpStatSuccess != Status)
		return Status;

	if (BufferSize < CheckSize)
		return SpStatBufferTooSmall;

	XformData = SpXformLock (Xform);
	if (NULL == XformData)
		return SpStatBadXform;

	KpMemCpy (Buffer, XformData, sizeof (*XformData));
	Buffer = ((char KPHUGE *) Buffer) + sizeof (*XformData);
	BufferSize -= sizeof (*XformData);

	PTStat = PTGetPTF (XformData->PTRefNum, PTTYPE_FUTF,
							BufferSize, (PTAddr_t) Buffer);
	if (KCP_SUCCESS != PTStat) {
		SpXformUnlock (Xform);
		return SpStatusFromPTErr(PTStat);
	}

	SpXformUnlock (Xform);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Create a transform from a block of data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 20, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpXformFromBlob (
				KpInt32_t		BufferSize,
				SpHugeBuffer_t	Buffer,
				SpXform_t		FAR *Xform)
{
	SpStatus_t		Status;
	SpXformData_t	FAR *XformData;

/* create an empty Xform */
	Status = SpXformAllocate (Xform);
	if (SpStatSuccess != Status)
		return Status;

/* lock down the Xform */
	XformData = SpXformLock (*Xform);
	if (NULL == XformData)
		return SpStatBadXform;

/* fill in Xform structure from data block */
	KpMemCpy (XformData, Buffer, sizeof (*XformData));
	Buffer = ((char KPHUGE *) Buffer) + sizeof (*XformData);
	BufferSize -= sizeof (*XformData);

/* give transform to the color processor */
	Status = SpXformLoadImp (Buffer, BufferSize, &XformData->PTRefNum);
	if (SpStatSuccess != Status) {
		SpFree (XformData);
		*Xform = NULL;
		return Status;
	}

	SpXformUnlock (*Xform);
	return SpStatSuccess;
}

