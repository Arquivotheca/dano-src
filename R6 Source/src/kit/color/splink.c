/*********************************************************************/
/*
	Contains:	This module contains the Device Link functions

			Separated out of spcvrt.c August 19, 1997

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:

	SCCS Revision:
		@(#)splink.c	1.3  12/4/97

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
#include "sprofprv.h"
#include <stdio.h>
#include <string.h>



/***************************************************************************
 * FUNCTION NAME
 *      SpProfileMakeDeviceLink
 *
 * DESCRIPTION
 *      This function generates a device link profile from the
 * information supplied in the SpDevLinkPB structure pointed to by
 * pDevLinkPB.  The profile generated is returned in the location 
 * pointed to by pProfile.
 *
***************************************************************************/
SpStatus_t SPAPI SpProfileMakeDeviceLink(SpCallerId_t	callerId,
				   SpDevLinkPB_p	pDevLinkDesc,
				   SpProfile_t		FAR *pProfile)
{
 
SpStatus_t	spStatus;
KpInt32_t	eachXform, badXform;
SpXform_t	*aXform;
KpBool_t	madeXform = KPFALSE;
 
	/*	Create a new empty profile      */
	spStatus = SpProfileCreate (callerId, pProfile);
	if (SpStatSuccess != spStatus) {
		return spStatus;
	}
 
	/*	Fill in the header information  */
	spStatus = SpProfileSetLinkHeader (*pProfile, pDevLinkDesc);
	if (SpStatSuccess != spStatus) {
		SpProfileFree (pProfile);
		return spStatus;
	}
 
	if (pDevLinkDesc->xform == NULL)
	{
		if (pDevLinkDesc->numProfiles < 2)
		{
			SpProfileFree (pProfile);
			return SpStatIncompatibleArguments;
		}

		aXform = SpMalloc(sizeof(SpXform_t) * pDevLinkDesc->numProfiles);

		if (aXform == NULL)
		{
			SpProfileFree (pProfile);
			return SpStatMemory;
		}

		for (eachXform = 0; 
		     eachXform < pDevLinkDesc->numProfiles;
		     eachXform++)
		{
			aXform[eachXform] = NULL;
			spStatus = SpXformGet(
				pDevLinkDesc->pProfileList[eachXform].profile,
				pDevLinkDesc->pProfileList[eachXform].whichRender,
				pDevLinkDesc->pProfileList[eachXform].whichTransform,
				&aXform[eachXform]);

			if ((SpStatSuccess != spStatus) &&
			    (SpStatXformIsPerceptual != spStatus) &&
			    (SpStatXformIsColormetric != spStatus) &&
			    (SpStatXformIsSaturation != spStatus))
			{
				for (eachXform--; eachXform >= 0; eachXform--)
					SpXformFree (&aXform[eachXform]);
				SpFree ((void *) aXform);
				SpProfileFree (pProfile);
				return spStatus;
			}
		}
		spStatus = SpCombineXforms(pDevLinkDesc->numProfiles,
					   aXform, &pDevLinkDesc->xform, 
					   &badXform, NULL, NULL);
		madeXform = KPTRUE;

		for (eachXform = 0; 
		     	    eachXform < pDevLinkDesc->numProfiles;
		     	    eachXform++)
			SpXformFree (&aXform[eachXform]);

		SpFree ((void *) aXform);

		if (SpStatSuccess != spStatus) {
			SpXformFree (&pDevLinkDesc->xform);
			SpProfileFree (pProfile);
			return spStatus;
		}
	}

	/*	Add the transform to the profile        */
	spStatus = SpXformSet(	*pProfile,
				pDevLinkDesc->lutSize,
				SpTransRenderPerceptual,
				SpTransTypeIn,
				pDevLinkDesc->xform);

	/*	Only free the device link transform if it was made by this function	*/
	if (madeXform) {
		SpXformFree (&pDevLinkDesc->xform);
	}

	if (SpStatSuccess != spStatus) {
		SpProfileFree (pProfile);
		return spStatus;
	}

	/*	Add the Profile Description Tag */
	spStatus = SpProfileSetLinkDesc(*pProfile, pDevLinkDesc);
	if (SpStatSuccess != spStatus) {
		SpProfileFree (pProfile);
		return spStatus;
	}

	/*	Add the Profile Sequence Description Tag */
	spStatus = SpProfileSetLinkSeqDesc(*pProfile, pDevLinkDesc);
	if (SpStatSuccess != spStatus) {
		SpProfileFree (pProfile);
		return spStatus;
	}
 
 
	return SpStatSuccess;
} /* SpProfileMakeDeviceLink */



/***************************************************************************
 * FUNCTION NAME
 *      SpProfileSetLinkHeader
 *
 * DESCRIPTION
 *      This function sets the fields of the header of the device link
 * profile pointed to by pProfile.
 *

***************************************************************************/
SpStatus_t SpProfileSetLinkHeader(SpProfile_t	pProfile,
				  SpDevLinkPB_p	pDevLinkDesc)
{
SpHeader_t		header, tempHeader;
SpProfListEntry_p	pCurProfEntry;
SpStatus_t		spStatus;

	/*	Get the header from the profile */
	spStatus = SpProfileGetHeader(pProfile, &header);
	if (SpStatSuccess != spStatus) {
		return spStatus;
	}
 
	/*	Set the class information to device link */
	header.DeviceClass = SpProfileClassLink;

	/* Set the device color spaces from the first profile
	   in the list */
	pCurProfEntry = pDevLinkDesc->pProfileList;
	spStatus = SpProfileGetHeader (pCurProfEntry->profile,
					&tempHeader);
	if (SpStatSuccess != spStatus) {
		return spStatus;
	}
	if (SpTransTypeOut == pCurProfEntry->whichTransform) {
		header.DataColorSpace = 
			tempHeader.InterchangeColorSpace;
	}
	else {
		header.DataColorSpace = 
			tempHeader.DataColorSpace;
	}

	/* Set the connection color spaces from the last
	   profile in the list */
	pCurProfEntry = pDevLinkDesc->pProfileList +
				(pDevLinkDesc->numProfiles - 1);
	spStatus = SpProfileGetHeader(pCurProfEntry->profile,
					&tempHeader);
	if (SpStatSuccess != spStatus) {
		return spStatus;
	}
	if (SpTransTypeOut == pCurProfEntry->whichTransform) {
		header.InterchangeColorSpace = 
			tempHeader.DataColorSpace;
	} else {
		header.InterchangeColorSpace = 
			tempHeader.InterchangeColorSpace;
	}

	/* Set the Device Manufacturer to "KODK" 
	   and the Device Model to "unkn" */
	header.DeviceManufacturer = SpSigMfgKodak;
	header.DeviceModel = SpSigNone;
	header.Originator  = SpSigOrgKodak;

	/*	Write the updated header back to the profile */
	spStatus = SpProfileSetHeader(pProfile, &header);

	return spStatus;
 
} /* SpProfileSetLinkHeader */



/***************************************************************************
 * FUNCTION NAME
 *      SpProfileSetLinkDesc
*
 * DESCRIPTION
 *      This function creates the Profile Description tag for the
 * device link profile pointed to by pProfile.  The information needed 
 * to form the description string is contained in the SpDevLinkPB 
 * structure pointed to by pDevLinkDesc.  The description string 
 * is formed by taking the manufacture, model and device color space 
 * from the first profile in the profile list and taking the 
 * manufacturer, model and connection color space from the last profile 
 * in the list.
 *
***************************************************************************/
SpStatus_t SpProfileSetLinkDesc(SpProfile_t	pProfile,
				SpDevLinkPB_p	pDevLinkDesc)
{
KpTChar_t	manufacturer1[SpMaxTextDesc], model1[SpMaxTextDesc];
KpTChar_t	manufacturer2[SpMaxTextDesc], model2[SpMaxTextDesc];
KpTChar_p	pDescStr;
KpInt32_t	length;
SpProfListEntry_p	pCurProfEntry;
SpTagValue_t	tagValue;
SpStatus_t	spStatus;

	/* Get the manufacturer information from the first profile */
	pCurProfEntry = pDevLinkDesc->pProfileList;
	strcpy (manufacturer1, "Unknown");
	spStatus = SpTagGetById(pCurProfEntry->profile,
				SpTagDeviceMfgDesc,
				&tagValue);

	if (SpStatSuccess == spStatus) {
		length = sizeof (manufacturer1);
		SpTagGetString(&tagValue, &length,  manufacturer1);
		SpTagFree(&tagValue);
	}

	/*	Get the model information from the first profile */
	strcpy(model1, "Unknown");
	spStatus = SpTagGetById(pCurProfEntry->profile,
				SpTagDeviceModelDesc,
				&tagValue);
	if (SpStatSuccess == spStatus) {
		length = sizeof (model1);
		SpTagGetString(&tagValue, &length, model1);
		SpTagFree(&tagValue);
	}
 
	/* Get the manufacturer information from the last profile */
	pCurProfEntry = pDevLinkDesc->pProfileList +
				(pDevLinkDesc->numProfiles - 1);
	strcpy(manufacturer2, "Unknown");
	spStatus = SpTagGetById(pCurProfEntry->profile,
				SpTagDeviceMfgDesc,
				&tagValue);
	if (SpStatSuccess == spStatus) {
		length = sizeof (manufacturer2);
		SpTagGetString(&tagValue, &length, manufacturer2);
		SpTagFree(&tagValue);
	}
 
	/* Get the model information from the last profile */
	strcpy(model2, "Unknown");
	spStatus = SpTagGetById(pCurProfEntry->profile,
				SpTagDeviceModelDesc,
				&tagValue);
	if (SpStatSuccess == spStatus) {
		length = sizeof (model2);
		SpTagGetString(&tagValue, &length, model2);
		SpTagFree(&tagValue);
	}
 
 
	/* Allocate memory for the description string */
	length = strlen(manufacturer1) +
			strlen(model1) + 
			strlen(manufacturer2) +
			strlen(model2) + 7;

	pDescStr = (KpTChar_p)allocBufferPtr(length+1);
	if (NULL == pDescStr) {
		return SpStatMemory;
	}
 
	/* Form the description string */
	/* sprintf(pDescStr, "%s %s %s to %s %s %s",
		manufacturer1, model1, colorSpace1,
		manufacturer2, model2, colorSpace2); */
	strcpy(pDescStr, manufacturer1);
	strcat(pDescStr, " ");
	strcat(pDescStr, model1);
	strcat(pDescStr, " to ");
	strcat(pDescStr, manufacturer2);
	strcat(pDescStr, " ");
	strcat(pDescStr, model2);

	/* Convert the string to a text description tag and
	   add it to the profile */
	spStatus = SpStringToTextDesc(pDescStr,
				&tagValue.Data.TextDesc);
	freeBufferPtr(pDescStr);
	if (SpStatSuccess != spStatus) {
		return spStatus;
	}

	tagValue.TagId = SpTagProfileDesc;
	tagValue.TagType = Sp_AT_TextDesc;
	spStatus = SpTagSet (pProfile, &tagValue);
	SpFreeTextDesc (&tagValue.Data.TextDesc);

	return spStatus;
 
} /* SpProfileSetLinkDesc */

/***************************************************************************
 * FUNCTION NAME
 *      SpProfileSetLinkSeqDesc
 *
 * DESCRIPTION
 *      This function creates the Profile Sequence Description tag for
 * the device link profile pointed to by pProfile.  The information
 * needed to form the sequence description is contained in the
 * SpDevLinkPB structure pointed to by pDevLinkDesc.  A sequence record 
 * is created for each profile in the profile list.  The sequence records
 * are used to make the sequence description.
 *
***************************************************************************/
SpStatus_t SpProfileSetLinkSeqDesc(SpProfile_t		pProfile,
				   SpDevLinkPB_p	pDevLinkDesc)
{
KpInt32_t			index, numProfiles,numRecords;
SpProfListEntry_p		pCurProfEntry;
SpProfileSeqDescRecord_t	*pSeqRecords, *pCurrentRecord;
SpTagValue_t			tagValue;
SpStatus_t			spStatus;

	/* Allocate the memory for the profile sequence records */
	numProfiles = pDevLinkDesc->numProfiles;
	pSeqRecords = (SpProfileSeqDescRecord_t *)
			allocBufferPtr(sizeof(SpProfileSeqDescRecord_t) *
			numProfiles);
	if (NULL == pSeqRecords) {
		return SpStatMemory;
	}
 
	/* For each profile in the profile list make a 
	   profile sequence record */
	pCurrentRecord = pSeqRecords;
	pCurProfEntry = pDevLinkDesc->pProfileList;
	numRecords = 0;
	for (index = 0; index < pDevLinkDesc->numProfiles; index++) {
		spStatus = SpProfileCreateSeqRecord(pCurProfEntry->profile,
				pCurrentRecord);
		if (SpStatSuccess != spStatus) {
			goto getOut;
		}
		numRecords++;
		pCurrentRecord++;
		pCurProfEntry++;
	}

	/* Create the profile sequence description tag and 
	   add it to the profile */
	tagValue.TagId = SpTagProfileSeqDesc;
	tagValue.TagType = Sp_AT_ProfileSeqDesc;
	tagValue.Data.ProfileSeqDesc.Count = numRecords;
	tagValue.Data.ProfileSeqDesc.Records = pSeqRecords;
	spStatus = SpTagSet (pProfile, &tagValue);
 
	/* Free all of the sequence records */
getOut:
	pCurrentRecord = pSeqRecords;
	for (index = 0; index < numRecords; index++) {
		SpProfileFreeSeqRecord (pCurrentRecord);
		pCurrentRecord++;
	}
	freeBufferPtr (pSeqRecords);
	return spStatus;
} /* SpProfileSetLinkSeqDesc */


/***************************************************************************
 * FUNCTION NAME
 *      SpProfileCreateSeqRecord
 *
 * DESCRIPTION
 *      This function creates the Profile Sequence record for the
 * profile pointed to by pProfile.  The Profile Sequence record to 
 * build is pointed to by pSeqRecord.
 *
***************************************************************************/
SpStatus_t SpProfileCreateSeqRecord(SpProfile_t		pProfile,
			SpProfileSeqDescRecord_t	*pSeqRecord)
{
SpHeader_t	header;
SpTagValue_t	tagValue;
SpStatus_t	spStatus;

	/* Get the profiles header */
	spStatus = SpProfileGetHeader (pProfile, &header);
	if (SpStatSuccess != spStatus) {
		return spStatus;
	}
 
	/* Copy the manufacturer, model, attributes and technology
	   fields into the sequence record */
	pSeqRecord->DeviceManufacturer = header.DeviceManufacturer;
	pSeqRecord->DeviceModel = header.DeviceModel;
	pSeqRecord->DeviceAttributes.hi = header.DeviceAttributes.hi;
	pSeqRecord->DeviceAttributes.lo = header.DeviceAttributes.lo;

	/* Get the technology tag*/
	spStatus = SpTagGetById(pProfile, 
				SpTagTechnology,
				&tagValue);
	if (SpStatSuccess != spStatus) 
		pSeqRecord->Technology = SpSigNone;
	else 
		pSeqRecord->Technology = tagValue.Data.Signature;

	/* Get the manufacturer description */
	spStatus = SpTagGetById(pProfile, 
				SpTagDeviceMfgDesc,
				&tagValue);
	if (SpStatSuccess != spStatus) {
		spStatus = SpStringToTextDesc("", &tagValue.Data.TextDesc);
		if (SpStatSuccess != spStatus) {
			return spStatus;
		}
	}
	pSeqRecord->DeviceManufacturerDesc = tagValue.Data.TextDesc;

	/* Get the model description */
	spStatus = SpTagGetById(pProfile, 
				SpTagDeviceModelDesc,
				&tagValue);
	if (SpStatSuccess != spStatus) {
		spStatus = SpStringToTextDesc("", &tagValue.Data.TextDesc);
		if (SpStatSuccess != spStatus) {
			return spStatus;
		}
	}
	pSeqRecord->DeviceModelDesc = tagValue.Data.TextDesc;

	return SpStatSuccess;
} /* SpProfileCreateSeqRecord */


/***************************************************************************
 * FUNCTION NAME
 *      SpProfileFreeSeqRecord
 *
 * DESCRIPTION
 *      This function frees the memory associated with the Profile
Sequence
 *      record pointed to by pSeqRecord.
 *
***************************************************************************/
SpStatus_t SpProfileFreeSeqRecord(SpProfileSeqDescRecord_t *pSeqRecord)
{
	SpFreeTextDesc(&pSeqRecord->DeviceManufacturerDesc);
	SpFreeTextDesc(&pSeqRecord->DeviceModelDesc);
	return SpStatSuccess;
} /* SpProfileFreeSeqRecord */
/***************************************************************************
 * FUNCTION NAME
 *      SpFreeTextDesc
 *
 * DESCRIPTION
 *      This function frees the memory that was allocated for the Iso
 * and Unicode strings in the SpTextDesc structure pointed to pTextDesc.
 *
***************************************************************************/
void SpFreeTextDesc(SpTextDesc_t *pTextDesc)
{
	if (NULL != pTextDesc->IsoStr) {
		freeBufferPtr (pTextDesc->IsoStr);
	}
	if (NULL != pTextDesc->UniStr) {
		freeBufferPtr (pTextDesc->UniStr);
	}
	return;
} /* SpFreeTextDesc */

/***************************************************************************
 * FUNCTION NAME
 *      SpStringToTextDesc
 *
 * DESCRIPTION
 *      This function converts stings to tags of type Sp_AT_TextDesc
 * to a string.  It allocates memory for the iso, unicode and mac 
 * strings which must be freed.  SpFreeTextDesc can be called to free 
 * that memory.
***************************************************************************/
SpStatus_t SpStringToTextDesc(KpChar_p pString,
				SpTextDesc_t *pTextDesc)
{
KpInt32_t	length, index, macScriptLen = 67;
KpChar_p	pSrcStr, pDestStr;
KpUInt16_t	data16;
KpUInt16_p	pDestStr16;

	/* Allocate memory for the stings */
	length = strlen (pString);
	pTextDesc->IsoStr = (char *) allocBufferPtr (length+1);
	pTextDesc->UniStr = (unsigned short *) allocBufferPtr((length+2)*2);
	if ((NULL == pTextDesc->IsoStr) ||
	    (NULL == pTextDesc->UniStr)) {
		SpFreeTextDesc (pTextDesc);
		return SpStatMemory;
	}
 
	/* copy the string to the isoString */
	pSrcStr = pString;
	pDestStr = pTextDesc->IsoStr;
	strcpy (pDestStr, pSrcStr);

	/* convert the string to unicode */
	pSrcStr = pString;
	pDestStr16 = pTextDesc->UniStr;
	pTextDesc->UniLangCode = 0;
	for (index = 0; index < length; index++) {
		data16 = *pSrcStr++;
		*pDestStr16++ = data16;
	}
	data16 = 0;
	*pDestStr16++ = data16;
 
	/* convert the string to macintosh script */
	pSrcStr = pString;
	pDestStr = pTextDesc->MacStr;
	pTextDesc->MacScriptCode = 0;
	if (length >= macScriptLen) {
		length = macScriptLen - 1;
	}
	pTextDesc->MacCount = (char) (length+1);
	strncpy (pDestStr, pSrcStr, length+1);
	pDestStr += length;
	*pDestStr = 0;
 
	return SpStatSuccess;
} /* SpStringToTextDesc */


/***************************************************************************
 * FUNCTION NAME
 *      SpGetStringFromSig
 *
 * DESCRIPTION
 *      This function converts the signature passed in signature to a
null
 *      terminated string.  The string is returned in the buffer
pointed to
 *      by pString.  It is assumed that the buffer is large enough to
hold
 *      the four characters in the signature and the terminating NULL.
*

***************************************************************************/
void SpGetStringFromSig(KpInt32_t	signature,
			KpTChar_p	pString)
{
KpChar_p	pSig;
KpInt32_t	index;

	pSig = (KpChar_p) &signature;

	/* The signature was written in big endian format,
	   if this is a little endian machine the bytes of the
	   signature need to be swapped. */
#if defined (KPLSBFIRST)
	Kp_swab32 (pSig, 1);
#endif
 
	for (index = sizeof (KpInt32_t); index--;) {
		*pString++ = *pSig++;
	}
	*pString = 0;
	return;
} /* SpGetStringFromSig */

