/*********************************************************************/
/*
	Contains:	This module contains the profile tag manager.

				Created by lsh, October 18, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPSAVE.C  $
		$Logfile:   O:\pc_src\dll\stdprof\spsave.c_v  $
		$Revision:   2.3  $
		$Date:   07 Apr 1994 13:24:30  $
		$Author:   lsh  $

	SCCS Revision:
	 @(#)spsave.c	1.36 12/22/97	

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
#include <stdio.h>

#include "sprof-pr.h"
#include <string.h>

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Write a 4 byte number to the file.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 20, 1993
 *------------------------------------------------------------------*/
static void SpWriteUInt32 (
				KpBool_t		FAR *fOk,
				KpFileId	fd,
				KpUInt32_t	Value)
{
	KpInt32_t	NumBytes;
	char		Bytes [4];

	if (!fOk)
		return;

	Bytes [0] = (char) (Value >> 24);
	Bytes [1] = (char) (Value >> 16);
	Bytes [2] = (char) (Value >> 8);
	Bytes [3] = (char) (Value);

	NumBytes = 4;
	if (0 == KpFileWrite (fd, Bytes, NumBytes))
		*fOk = KPFALSE;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do padding to force tag data to a 4 byte boundry.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 20, 1993
 *------------------------------------------------------------------*/
static SpStatus_t DoFilePadding (
				KpFileId	fd,
				KpInt32_t	FAR *FilePos)
{
	char			Padding [4];

/* determine where this tag will go in the file */
	KpFileTell (fd, FilePos);
	if (0 != *FilePos % 4) {

	/* clear array used for padding */
		KpMemSet (Padding, 0, sizeof (Padding));

	/* write padding bytes to force 32 bit alignment */
		if (0 == KpFileWrite (fd, Padding, 4 - (*FilePos % 4)))
			return SpStatFileWriteError;

		KpFileTell (fd, FilePos);
	}

	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Write profile header.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	Feb 14, 1994
 *------------------------------------------------------------------*/
static void SpProfileWriteHeader (
				KpBool_t			FAR *fOk,
				KpFileId		fd,
				SpProfileData_t	FAR *ProfileData)
{
	SpStatus_t	Status;
	char		Buffer [128];

	if (!*fOk)
		return;

	Status = SpHeaderFromPublic (&ProfileData->Header, sizeof (Buffer), Buffer);
	if (SpStatSuccess != Status) {
		*fOk = KPFALSE;
		return;
	}

	if (0 == KpFileWrite (fd, Buffer, sizeof (Buffer)))
		*fOk = KPFALSE;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Save the tag directory.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 15, 1994
 *------------------------------------------------------------------*/

static void SpWriteTagDir (
				KpBool_t			FAR *fOk,
				KpFileId		fd,
				KpUInt32_t		Count,
				SpTagRecord_t	FAR *TagRecords)
{
	KpUInt32_t		i;

	SpWriteUInt32 (fOk, fd, Count);
	for (i = 0;
				i < Count;
						i++, TagRecords++) {
		SpWriteUInt32 (fOk, fd, TagRecords->Id);
		SpWriteUInt32 (fOk, fd, TagRecords->Offset);
		SpWriteUInt32 (fOk, fd, TagRecords->Size);
	}
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Try to share data from previous tag.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	June 23, 1994
 *------------------------------------------------------------------*/
static KpBool_t SpTagShare (
				SpTagDirEntry_t	FAR *TagArray,
				KpUInt32_t		Index,
				SpTagRecord_t	FAR *TagRecords,
				SpTagRecord_t	FAR *TagRecord)
{
	KpUInt32_t			index;
	SpTagRecord_t	FAR *TagCurrRecord;
	KpGenericPtr_t	FAR TagData1, FAR TagData2;

/* loop through previous tags looking for duplicate data */
	for (index = 0, TagCurrRecord = TagRecords;
			index != Index;
			index++)
	{
	
		/* compare the size of the data */
		if (TagArray[index].TagDataSize == TagArray[Index].TagDataSize) {

			TagData1 = lockBuffer(TagArray[index].TagData);
			TagData2 = lockBuffer(TagArray[Index].TagData);
	
			/* compare the actual data */
			if (0 == KpMemCmp (TagData1, TagData2,
							TagArray[Index].TagDataSize)) {

				/* data is the same, share this entry */
				TagRecord->Id = (KpUInt32_t) TagArray[Index].TagId;
				TagRecord->Offset = TagCurrRecord->Offset;
				TagRecord->Size = TagCurrRecord->Size;

				unlockBuffer(TagArray[index].TagData);
				unlockBuffer(TagArray[Index].TagData);
				return KPTRUE;
			}
		
			unlockBuffer(TagArray[index].TagData);
			unlockBuffer(TagArray[Index].TagData);
		}
		
		/* increment pointer into the new profile tag table if this is a valid
			entry in the sprofile Tag Array */
		if (TagArray[index].TagDataSize != -1)
			TagCurrRecord++;
	}

	return KPFALSE;
}


#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION	(Private)
 *	Save a profile to a named file.  Optionally share the tag data.
 * 	This function has been obsoleted by SpProfileSaveProfileEx
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveToFileEx (
				SpProfile_t Profile,
				char		FAR *Name,
				KpBool_t		ShareTags)
{
	SpFileProps_t		Props;

/* Create SpFileProps_t and call SpProfileSaveProfileEx */
#if defined (KPMAC)
	strncpy (Props.fileType, "prof", 5);
	strncpy (Props.creatorType, "KCMM", 5);
	Props.vRefNum = 0;
	Props.dirID = 0;
#endif

	return (SpProfileSaveProfileEx(Profile, Name, &Props, ShareTags));
}


/*--------------------------------------------------------------------
 * DESCRIPTION	(Private)
 *	Save a profile to a named file.  Share the tag data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveToFile (
				SpProfile_t Profile,
				char		FAR *Name)
{
	return SpProfileSaveToFileEx (Profile, Name, KPTRUE);
}
#endif /* !SP_NO_OBSOLETE */


/*--------------------------------------------------------------------
 * DESCRIPTION (Public)
 *	Save the profile data to the file descriptor.  Optionally share the 
 *      tag data.
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveOutData (
				SpProfile_t	Profile,
				KpFileId	fd,
				KpBool_t	ShareTags)
{
	SpStatus_t		Status;
	SpProfileData_t	FAR *ProfileData;
	KpInt32_t		FilePos;
	KpUInt32_t		FileSize;
	SpTagRecord_t	FAR *TagRecords;
	SpTagRecord_t	FAR *TagRecord;
	SpTagDirEntry_t	FAR *tagArray;
	void 			KPHUGE *tagData;
	KpInt32_t		index;
	KpBool_t		fOk;
	KpInt32_t		activeCount = 0;
	KpUInt32_t		incrementTagRecord = 0;


/* lock down the profile data */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* determine the number of active TagArray elements */
	activeCount = SpTagGetCount (ProfileData);
	
/* allocate space for tag records */
	TagRecord =
	TagRecords = SpMalloc (activeCount * sizeof (*TagRecords));
	if (NULL == TagRecords) {
		SpProfileUnlock (Profile);
		return SpStatMemory;
	}
	
/* zero out TagRecord buffer before using */
	KpMemSet (TagRecord, 0, activeCount * sizeof (*TagRecords));

/* write the Header */
	fOk = KPTRUE;
	SpProfileWriteHeader (&fOk, fd, ProfileData);
	
/* write dummy tag directory */
	SpWriteTagDir (&fOk, fd, activeCount, TagRecords);
	Status = fOk ? SpStatSuccess : SpStatFileWriteError;

/**********************/
/* write the Tag data */
/**********************/

/* get a pointer to the head of the tag array */
	tagArray = (SpTagDirEntry_t FAR *) lockBuffer (ProfileData->TagArray);
		
	if (SpStatSuccess == Status)
	{
	
		for (index = 0; index < ProfileData->TotalCount; index++)
		{
		
		/* determine if this TagArray element holds good data */
			if (tagArray[index].TagDataSize == -1)
				continue;
			else
				incrementTagRecord = 1;
				
		/* determine where this tag will go in the file */
			Status = DoFilePadding (fd, &FilePos);
			if (SpStatSuccess != Status)
				break;

		/* try to share data from previous tags */
			if (!ShareTags || !SpTagShare (tagArray, index,
											TagRecords, TagRecord))
			{

			/* not sharing tag data, write data to file */

			/* remember where we put this tag */
				TagRecord->Id = (KpUInt32_t) tagArray[index].TagId;
				TagRecord->Offset = (KpUInt32_t) FilePos;
				TagRecord->Size = tagArray[index].TagDataSize;
			
			/* write the tag to the file */
				tagData = (void KPHUGE *) lockBuffer (tagArray[index].TagData);
				if (0 == KpFileWrite (fd, tagData, tagArray[index].TagDataSize))
				{
					Status = SpStatFileWriteError;
					unlockBuffer (tagArray[index].TagData);
					break;
				}
				unlockBuffer (tagArray[index].TagData);
			}

		/* point to the next tag directory entry */
			if (incrementTagRecord)
			{
				TagRecord++;
				incrementTagRecord = 0;
			}
		}
	}
	unlockBuffer (ProfileData->TagArray);

/*******************************/
/* update the size of the file */
/*******************************/
	if (SpStatSuccess == Status) {
		KpFileTell (fd, &FilePos);
		FileSize = (KpUInt32_t) FilePos;
		KpFilePosition (fd, FROM_START, 0);
		SpWriteUInt32 (&fOk, fd, FileSize);
	}

/********************************/
/* write the real Tag directory */
/********************************/
	if (SpStatSuccess == Status) {
		KpFilePosition (fd, FROM_START, 128);
		SpWriteTagDir (&fOk, fd, activeCount, TagRecords);
		if (!fOk)
			Status = SpStatFileWriteError;
	}

/* done with tag records, free space */
	SpFree (TagRecords);
	SpProfileUnlock (Profile);

	return Status;
}

#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION (Public)
 *	Save a profile to a named file.  Optionally share the tag data.
 * 	This function has been obsoleted by SpProfileSaveProfileEx
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveToDiskEx (
				SpProfile_t		Profile,
				KpChar_t		*Name,
				SpIOFileChar_t	*Props,
				KpBool_t		ShareTags)
{
SpFileProps_t	*PropsPtr;
#if defined (KPMAC)
SpFileProps_t	SpProps;

/* Create SpFileProps_t and call SpProfileSaveProfileEx */
	if (Props != NULL) { 
		SpCvrtIOFileProps(Props, &SpProps);
		PropsPtr = &SpProps;
	}
	else
		PropsPtr = NULL;
#else
	if (Props) {}
	PropsPtr = NULL;
#endif

	return (SpProfileSaveProfileEx(Profile, Name, PropsPtr, ShareTags));
}
#endif /* !SP_NO_OBSOLETE */

#if !defined (SP_NO_FILEIO)
/*--------------------------------------------------------------------
 * DESCRIPTION (Public)
 *	Save a profile to a named file.  Optionally share the tag data.
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveProfileEx (
				SpProfile_t	Profile,
				KpChar_t	*Name,
				SpFileProps_t	*Props,
				KpBool_t	ShareTags)
{
	SpStatus_t		Status;
	SpProfileData_t	FAR *ProfileData;
	KpFileProps_t		props;
	KpFileId		fd;

/* lock down the profile data */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* Validate profile */
	Status = SpProfileValidate (ProfileData);
	SpProfileUnlock (Profile);

	if (SpStatSuccess != Status) {
		return Status;
	}

/* Convert file information into useable form */
	SpCvrtSpFileProps(Props, &props);

/* delete existing file */
	KpFileDelete (Name, &props);

/* open file */
	if (0 == KpFileOpen (Name, "w", &props, &fd)) {
		return SpStatFileNotFound;
	}

	Status = SpProfileSaveOutData ( Profile, fd, ShareTags);

/* close the file */
	KpFileClose (fd);
	
/* check that all is well */
	if (SpStatSuccess != Status)
		KpFileDelete (Name, &props);

	return Status;
}
#endif /* !SP_NO_FILEIO */


#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION (Public)
 *	Save a profile to a named file.  Share the tag data.
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveToDisk (
				SpProfile_t		Profile,
				KpChar_t		*Name,
				SpIOFileChar_t	*Props)
{
	return SpProfileSaveToDiskEx (Profile, Name, Props, KPTRUE);
}
#endif /* !SP_NO_OBSOLETE */

#if !defined (SP_NO_FILEIO)
/*--------------------------------------------------------------------
 * DESCRIPTION (Public)
 *	Save a profile to a named file.  Share the tag data.
 *
 * AUTHOR
 * 	acr
 *
 * DATE CREATED
 *	December 12, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveProfile (
				SpProfile_t		Profile,
				KpChar_t		*Name,
				SpFileProps_t	*Props)
{
	return SpProfileSaveProfileEx (Profile, Name, Props, KPTRUE);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Save current values for a profile.  Optionally share the tag data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveEx (
				SpProfile_t		Profile,
				KpBool_t			ShareTags)
{
	char			FAR *fileName;
	SpProfileData_t FAR *ProfileData;
	SpStatus_t		Status;

/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* check for case of no file name */
	if (NULL == ProfileData->FileName) {
		SpProfileUnlock (Profile);
		return SpStatNoFileName;
	}

/* lock FileName handle and return ptr */
	fileName = (char *) lockBuffer (ProfileData->FileName);
	
/* check for case of no file name */
	if (NULL == fileName) {
		SpProfileUnlock (Profile);
		return SpStatNoFileName;
	}

/* let SpProfileSaveProfileEx do the real work */
	Status = SpProfileSaveProfileEx (Profile, fileName, &ProfileData->Props, ShareTags);

/* unlock handles */
	unlockBuffer (ProfileData->FileName);
	SpProfileUnlock (Profile);
	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Save current values for a profile.  Share the tag data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSave (
				SpProfile_t		Profile)
{
	return SpProfileSaveEx (Profile, KPTRUE);
}
#endif /* !SP_NO_FILEIO */


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	copy profile header.
 *
 * AUTHOR
 * 	ae
 *
 * DATE CREATED
 *	January 13, 1994
 *------------------------------------------------------------------*/
static void SpProfileCopyHeader (
				char			*pBuffer,
				SpProfileData_t	*ProfileData)
{
	SpStatus_t	Status;
	char		Buffer [128];

	Status = SpHeaderFromPublic (&ProfileData->Header, sizeof (Buffer), Buffer);
	if (SpStatSuccess != Status) {
		return;
	}

	memcpy (pBuffer, Buffer, sizeof (Buffer));
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do padding to force tag data to a 4 byte boundry.
 *
 * AUTHOR
 * 	ae
 *
 * DATE CREATED
 *	January 13, 1997
 *------------------------------------------------------------------*/
static SpStatus_t DoBufferPadding (
				KpChar_t	**pBuffer,
				KpUInt32_t	*BufferSize)
{

/* determine where this tag will go in the buffer */
	if (0 != *BufferSize % 4) {
	
		*pBuffer += 4 - (*BufferSize % 4);
		*BufferSize += 4 - (*BufferSize % 4);
	}

	return SpStatSuccess;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Save the tag directory to buffer .
 *
 * AUTHOR
 * 	ae
 *
 * DATE CREATED
 *	January 15, 1997
 *------------------------------------------------------------------*/

static void SpWriteTagDirToBuffer (
				char			**Buffer,
				KpUInt32_t		Count,
				SpTagRecord_t	FAR *TagRecords)
{
	KpUInt32_t		i;
	char			*Ptr;

	Ptr = *Buffer;

	SpPutUInt32 (&Ptr, Count);
	for (i = 0;
				i < Count;
						i++, TagRecords++) {
		SpPutUInt32 (&Ptr, TagRecords->Id);
		SpPutUInt32 (&Ptr, TagRecords->Offset);
		SpPutUInt32 (&Ptr, TagRecords->Size);
	}
}

/*--------------------------------------------------------------------
 * DESCRIPTION (Public)
 *	Get the profile size 
 *
 * AUTHOR
 * 	ae
 *
 * DATE CREATED
 *	january 13, 1997
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileGetProfileSize (
				SpProfile_t	Profile,
				KpUInt32_p	Size)
{
	SpProfileData_t	FAR *ProfileData;
	SpTagDirEntry_t	FAR *tagArray;
	KpInt32_t		index;
	KpInt32_t		activeCount = 0;
	KpUInt32_t		BufferPos;


/* lock down the profile data */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* determine the number of active TagArray elements */
	activeCount = SpTagGetCount (ProfileData);
	
	BufferPos = HEADER_SIZE + (activeCount * 12) + sizeof(KpUInt32_t);

/* get a pointer to the head of the tag array */
	tagArray = (SpTagDirEntry_t FAR *) lockBuffer (ProfileData->TagArray);


	for (index = 0; index < ProfileData->TotalCount; index++)
	{
		
	/* determine if this TagArray element holds good data */
		if (tagArray[index].TagDataSize == -1)
			continue;
				
	/* determine where this tag will go in the buffer */
		if (0 != BufferPos % 4) {
			BufferPos += 4 - (BufferPos % 4);
		}

		BufferPos += tagArray[index].TagDataSize;

	}
	
	unlockBuffer (ProfileData->TagArray);

	SpProfileUnlock (Profile);

	*Size = BufferPos;

	return SpStatSuccess;
}

/*--------------------------------------------------------------------
 * DESCRIPTION (Public)
 *	Save the profile data to the buffer.   
 *      
 *
 * AUTHOR
 * 	ae
 *
 * DATE CREATED
 *	January 13, 1997
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSaveToBuffer (
				SpProfile_t	Profile,
				KpChar_p	*lpBuffer,
				KpUInt32_p	inBytes)
{
	SpProfileData_t	FAR *ProfileData;
	SpTagRecord_t	FAR *TagRecords;
	SpTagRecord_t	FAR *TagRecord;
	SpTagDirEntry_t	FAR *tagArray;
	void 			KPHUGE *tagData;
	KpInt32_t		index;
	KpInt32_t		activeCount = 0;
	KpUInt32_t		incrementTagRecord = 0;
	char			*Buffer, *BytePtr;
	KpUInt32_t		BuffSize, inSize, bytes;

	Buffer = *lpBuffer;
	inSize = *inBytes;
/* lock down the profile data */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;


/* determine the number of active TagArray elements */
	activeCount = SpTagGetCount (ProfileData);
	
/* allocate space for tag records */
	bytes = activeCount * sizeof (*TagRecords);
	TagRecord =
	TagRecords = SpMalloc (bytes);
	if (NULL == TagRecords) {
		SpProfileUnlock (Profile);
		return SpStatMemory;
	}
	
/* zero out TagRecord buffer before using */
	KpMemSet (TagRecord, 0, activeCount * sizeof (*TagRecords));

/* write the Header */
	SpProfileCopyHeader (Buffer, ProfileData);

	BuffSize = HEADER_SIZE + (activeCount * 12) + sizeof(KpUInt32_t);
	Buffer += BuffSize;

/**********************/
/* write the Tag data */
/**********************/

/* get a pointer to the head of the tag array */
	tagArray = (SpTagDirEntry_t FAR *) lockBuffer (ProfileData->TagArray);
		
	for (index = 0; index < ProfileData->TotalCount; index++)
	{
		
	/* determine if this TagArray element holds good data */
		if (tagArray[index].TagDataSize == -1)
			continue;
		else
			incrementTagRecord = 1;
				
	/* determine where this tag will go in the buffer */
		DoBufferPadding (&Buffer, &BuffSize);

	/* remember where we put this tag */
			TagRecord->Id = (KpUInt32_t) tagArray[index].TagId;
			TagRecord->Offset = (KpUInt32_t) BuffSize;
			TagRecord->Size = tagArray[index].TagDataSize;

	/* write the tag to the file */
			tagData = (void KPHUGE *) lockBuffer (tagArray[index].TagData);
		
			memcpy (Buffer, tagData, tagArray[index].TagDataSize);

			BuffSize += tagArray[index].TagDataSize;
			Buffer += tagArray[index].TagDataSize;
			unlockBuffer (tagArray[index].TagData);

	/* point to the next tag directory entry */
		if (incrementTagRecord)
		{
			TagRecord++;
			incrementTagRecord = 0;
		}
	}
	
	unlockBuffer (ProfileData->TagArray);

/*******************************/
/* update the size of the buffer */
/*******************************/
	BytePtr = *lpBuffer;
/*	*Size = BuffSize; */
	SpPutUInt32(&BytePtr, BuffSize);
	
/********************************/
/* copy the Tag directory */
/********************************/
	BytePtr = *lpBuffer + HEADER_SIZE;
	SpWriteTagDirToBuffer(&BytePtr, activeCount, TagRecords);

/* done with tag records, free space */
	SpFree (TagRecords);
	SpProfileUnlock (Profile);

	return SpStatSuccess;
}




