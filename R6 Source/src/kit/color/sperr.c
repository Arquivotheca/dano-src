/*********************************************************************/
/*
	Contains:	This module contains the error number to text function.

				Created by lsh, September 20, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPERR.C  $
		$Logfile:   O:\pc_src\dll\stdprof\sperr.c_v  $
		$Revision:   2.0  $
		$Date:   21 Mar 1994 14:59:50  $
		$Author:   lsh  $

	SCCS Revision:
		@(#)sperr.c	1.29 06/12/96

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
 *** COPYRIGHT (c) Eastman Kodak Company, 1993                      ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"
#include <string.h>

char	SpErrorHdr[] = {"Profile Processor Error: "};

#define SPStatEntry(nm, Str) {SpStat##nm, Str}

typedef struct {
	SpStatus_t	Status;
	char		*Text;
} SpStatTbl_t;

/* The entry NotImp must be the last entry in this table */
/* The code uses this entry as an end of table marker    */

SpStatTbl_t	StatusToTxtTbl [] = {
	SPStatEntry (Success,			"Success"),
	SPStatEntry (BadCallerId,		"Bad caller Id"),
	SPStatEntry (BadLutType,		"Bad LUT type"),
	SPStatEntry (BadProfile,		"Bad profile"),
	SPStatEntry (BadTagData,		"Bad tag data"),
	SPStatEntry (BadTagType,		"Bad tag type"),
	SPStatEntry (BadTagId,			"Bad tag ID"),
	SPStatEntry (BadXform,			"Bad transform"),
	SPStatEntry (XformNotActive,	"Transform not active"),
	SPStatEntry (BufferTooSmall,	"Buffer too small"),
	SPStatEntry (Failure,			"General failure"),
	SPStatEntry (FileNotFound,		"File not found"),
	SPStatEntry (FileReadError,		"File read error"),
	SPStatEntry (FileWriteError,	"File write error"),
	SPStatEntry (IncompatibleArguments,	"Incompatible arguments"),
	SPStatEntry (Memory,			"Memory allocation"),
	SPStatEntry (NoFileName,		"No file name"),
	SPStatEntry (NotFound,			"Not found"),
	SPStatEntry (OutOfRange,		"Value out of range"),
	SPStatEntry (TagNotFound,		"Tag not found"),
	SPStatEntry (BadBuffer,			"Bad buffer"),
	SPStatEntry (BadProfileDir,		"Bad profile directory entry"),
	SPStatEntry (ReqTagMissing,		"Required tag missing"),
	SPStatEntry (Unsupported,		"Unsupported feature"),
	SPStatEntry (NoComponent,		"No KODAK PRECISION ProfileAPI Component"),
	SPStatEntry (NoCompMemory,		"No component memory"),
	SPStatEntry (BadCompRequest,	"Bad ProfileAPI component request"),
	SPStatEntry (BadCompInstance,	"Bad ProfileAPI component instance"),
	SPStatEntry (CPComp,	"Kodak Precision Color Processor error or missing"),
	SPStatEntry (ResFileErr,		"Resource file error or missing"),
	SPStatEntry (KcmFailure,	"Kodak Precision API error or missing"),
	SPStatEntry (Abort,		"Operation Aborted"),
	SPStatEntry (XformIsPerceptual,		"Xform Returned Is Perceptual"),
	SPStatEntry (XformIsColormetric,	"Xform Returned Is Colormetric"),
	SPStatEntry (XformIsSaturation,		"Xform Returned Is Saturation"),
	SPStatEntry (NotImp,			"Not implemented")
};


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Append a string to a buffer.  If the entire string will fit, 1 is
 *	returned, else 0 is returned.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
KpBool_t SpStrAppend (
				size_t	BufferSize,
				char	FAR *Buffer,
				char	FAR *Str)
{
	size_t	len1, len2;

	len1 = strlen (Buffer);
	len2 = strlen (Str);

	if (len1 + len2 < BufferSize) {
		strcat (Buffer, Str);
		return KPTRUE;
	}

	return KPFALSE;
}


#define SpStatusHdr	498

#if defined (KPWIN)
extern	HANDLE	SpHInst;
/*--------------------------------------------------------------------
 * DESCRIPTION	(Windows version)
 *	Convert SpStat_t value to textual error message.
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpGetErrorText (
				SpStatus_t	StatusVal,
				size_t		BufferSize,
				char		FAR *Buffer)
{
	KpChar_t	FAR ErrMsg[256];
	WORD		res_code;

/* Adjust resource code value for success and not implemented */
	if (-1 == StatusVal)
		res_code = (WORD)499;
	else if (0 == StatusVal)
		res_code = (WORD)500;
	else
		res_code = (WORD)(StatusVal);

/* Build error message */
	*Buffer = '\0';
	if (0 == LoadString(SpHInst, SpStatusHdr, (LPSTR)ErrMsg, 255))
	{
		KpItoa (SpStatusHdr, ErrMsg);
	}

	if (!SpStrAppend (BufferSize, Buffer, ErrMsg))
		return SpStatBufferTooSmall;

	if (0 == LoadString(SpHInst, res_code, (LPSTR)ErrMsg, 255))
	{
		KpItoa (StatusVal, ErrMsg);
	}

/* Give error text to caller */
	if (!SpStrAppend (BufferSize, Buffer, ErrMsg))
		return SpStatBufferTooSmall;

	return SpStatSuccess;
}
#elif defined (KPMAC)
/* This define is duplicated in msperr.r */
#define SpStatusMsgs	499

/*--------------------------------------------------------------------
 * DESCRIPTION	(Macintosh version)
 *	Convert SpStat_t value to textual error message.
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpGetErrorText (
				SpStatus_t	StatusVal,
				size_t		BufferSize,
				char		FAR *Buffer)
{
	KpChar_t	FAR ErrMsg[256];
	KpInt32_t	res_code;
	SpStatTbl_t	FAR *TblEntry;
	char		Work [20];

/* Adjust resource code values */
	if (-1 == StatusVal)
		res_code = (KpInt32_t)1;
	else if (0 == StatusVal)
		res_code = (KpInt32_t)2;
	else
		res_code = (KpInt32_t)(StatusVal-SpStatusMsgs+1);

/* Build error message */
	*Buffer = '\0';
	GetIndString ((StringPtr)ErrMsg, SpStatusHdr, 1);
	if (0 == ErrMsg[0])
	{
		strcpy(ErrMsg, SpErrorHdr);
	}
	else
	{
		p2cstr ((StringPtr)ErrMsg);
	}

	if (!SpStrAppend (BufferSize, Buffer, ErrMsg))
		return SpStatBufferTooSmall;

	GetIndString ((StringPtr)ErrMsg, SpStatusMsgs, res_code);
	if (0 == ErrMsg[0])
	{
		/* look for match in table */
		for (TblEntry = StatusToTxtTbl;
		     SpStatNotImp != TblEntry->Status;
		     TblEntry++) {

			/* if match found, exit loop */
			if (StatusVal == TblEntry->Status)
				break;
		}


		if (StatusVal == TblEntry->Status)
			strcpy(ErrMsg, TblEntry->Text);
		else {
			KpItoa (StatusVal, Work);
			strcpy(ErrMsg, Work);
		}
	}
	else
	{
		p2cstr ((StringPtr)ErrMsg);
	}

/* Give error text to caller */
	if (!SpStrAppend (BufferSize, Buffer, ErrMsg))
		return SpStatBufferTooSmall;

	return SpStatSuccess;
}
#else
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert SpStat_t value to textual error message.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpGetErrorText (
				SpStatus_t	StatusVal,
				size_t		BufferSize,
				char		FAR *Buffer)
{
	SpStatTbl_t	FAR *TblEntry;
	char		Work [20];
	char		FAR *ErrMsg;

/* look for match in table */
	for (TblEntry = StatusToTxtTbl;
			SpStatNotImp != TblEntry->Status;
					TblEntry++) {

	/* if match found, exit loop */
		if (StatusVal == TblEntry->Status)
			break;
	}

/* build error message */
	*Buffer = '\0';
	if (!SpStrAppend (BufferSize, Buffer, SpErrorHdr))
		return SpStatBufferTooSmall;

	if (StatusVal == TblEntry->Status)
		ErrMsg = TblEntry->Text;
	else {
		KpItoa (StatusVal, Work);
		ErrMsg = Work;
	}

/* give error text to caller */
	if (!SpStrAppend (BufferSize, Buffer, ErrMsg))
		return SpStatBufferTooSmall;

	return SpStatSuccess;
}
#endif

