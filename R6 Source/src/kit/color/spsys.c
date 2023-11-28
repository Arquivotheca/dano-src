/*********************************************************************/
/*
	Contains:	This module contains system specific functions.

				Created by lsh, September 14, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPSYS.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spsys.c_v  $
		$Revision:   2.1  $
		$Date:   07 Apr 1994 13:24:04  $
		$Author:   lsh  $

	SCCS Revision:
		@(#)spsys.c	1.10 12/22/97

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Allocate a block of memory.  The allocated block can be bigger
 *	than 64K.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 14, 1993
 *------------------------------------------------------------------*/
void FAR *SpMalloc (KpInt32_t Size)
{
	return allocBufferPtr (Size);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Free a block of memory allocated with SpMalloc.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 14, 1993
 *------------------------------------------------------------------*/
void SpFree (void FAR *Ptr)
{
	if (NULL != Ptr)
		freeBufferPtr (Ptr);
}


#if defined (KPMAC)
/*--------------------------------------------------------------------
 * DESCRIPTION	(Macintosh Version)
 *	Converts a SpIOFileChar_t structure to an ioFileChar structure.
 *
 * AUTHOR
 * 	mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpCvrtIOFileData (
				SpIOFileChar_t	*SpProps,
				ioFileChar		*KcmProps)
{
	int i;
   
	for (i = 0; i < 5; i++)
	{
	  KcmProps->fileType[i]    = SpProps->fileType[i];
	  KcmProps->creatorType[i] = SpProps->creatorType[i];
	}
	KcmProps->vRefNum = SpProps->vRefNum;

	return SpStatSuccess;
}
SpStatus_t SPAPI SpCvrtIOFileProps(
                                SpIOFileChar_t *SpProps,
                                SpFileProps_t  *KcmProps)
{
	int i;
   
	for (i = 0; i < 5; i++)
	{
	  KcmProps->fileType[i]    = SpProps->fileType[i];
	  KcmProps->creatorType[i] = SpProps->creatorType[i];
	}
	KcmProps->vRefNum = SpProps->vRefNum;
	KcmProps->dirID   = 0;

	return SpStatSuccess;
}
SpStatus_t SPAPI SpCvrtSpFileData (
				SpFileProps_t	*SpProps,
				ioFileChar		*KcmProps)
{
	int i;
	
	for (i= 0; i < 5; i++)
	{
	   KcmProps->fileType[i]    = SpProps->fileType[i];
	   KcmProps->creatorType[i] = SpProps->creatorType[i];
	}
	KcmProps->vRefNum = SpProps->vRefNum;

	return SpStatSuccess;
}
SpStatus_t SPAPI SpCvrtSpFileProps(
				SpFileProps_t		*SpProps,
				KpFileProps_t		*KcmProps)
{
	int i;
	
	for (i = 0; i < 5; i++)
	{
	   KcmProps->fileType[i]    = SpProps->fileType[i];
	   KcmProps->creatorType[i] = SpProps->creatorType[i];
	}
	KcmProps->vRefNum = SpProps->vRefNum;
	KcmProps->dirID   = SpProps->dirID;

	return SpStatSuccess;
}

#else
/*--------------------------------------------------------------------
 * DESCRIPTION	(non-Macintosh Version)
 *	Converts a SpIOFileChar_t structure to an ioFileChar structure.
 *
 * AUTHOR
 * 	mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpCvrtIOFileData (
				SpIOFileChar_t	*SpProps,
				ioFileChar		*KcmProps)
{
	if (SpProps) {}
	if (KcmProps) {}

	return SpStatSuccess;
}
SpStatus_t SPAPI SpCvrtIOFileProps(
                                SpIOFileChar_t *SpProps,
                                SpFileProps_t  *KcmProps)
{
	if (SpProps) {}
	if (KcmProps) {}

	return SpStatSuccess;
}
SpStatus_t SPAPI SpCvrtSpFileData (
				SpFileProps_t	*SpProps,
				ioFileChar		*KcmProps)
{
	if (SpProps) {}
	if (KcmProps) {}

	return SpStatSuccess;
}
SpStatus_t SPAPI SpCvrtSpFileProps(
				SpFileProps_t		*SpProps,
				KpFileProps_t		*KcmProps)
{
	if (SpProps) {}
	if (KcmProps) {}

	return SpStatSuccess;
}
#endif


#if defined(KPMAC)
#include "spcback.h"

KpInt32_t  theAppA5;
KpInt32_t  theAppA4;


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * SPsaveCallbackA4A5 (Macintosh version only)
 *
 * DESCRIPTION
 * This function saves the hosts A4 and A5 values
 * for later use
--------------------------------------------------------------------*/
void SPsaveCallbackA4A5(KpInt32_t theA4, KpInt32_t theA5)
{
	/*
	 * Update the current application's global data structure
	 */
	theAppA4 = theA4;
	theAppA5 = theA5;

}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * SPretrieveCallbackA4A5	(Macintosh version only)
 *
 * DESCRIPTION
 * This function returns the callback A4 and A5 values previously
 * stored using saveCallbackA4A5().
--------------------------------------------------------------------*/
void SPretrieveCallbackA4A5( KpInt32_t  *theA4, KpInt32_t  *theA5)
{


*theA4 = theAppA4;
*theA5 = theAppA5;

}
#endif


