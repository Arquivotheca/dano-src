/*********************************************************************/
/*
	Contains:	This module contains the profile functions.

				Created by lsh, September 14, 1993

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1997 by Eastman Kodak Company, all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:
		$Workfile:   SPROFILE.C  $
		$Logfile:   O:\pc_src\dll\stdprof\sprofile.c_v  $
		$Revision:   2.2  $
		$Date:   07 Apr 1994 13:24:16  $
		$Author:   lsh  $

	SCCS Revision:
	@(#)sprofile.c	1.90	12/22/97

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

#define SPGLOBAL
#include "sprof-pr.h"
#include <string.h>


#if defined(KPMAC)
#include "spcback.h"
#if (!defined KPMACPPC) & (defined KPMW) 
	#include <A4Stuff.h>
#endif

/*Local Prototypes*/
SpStatus_t SpDoIter (SpTagIter_t	ProgressFunc,SpIterState_t	State,
				SpProfile_t	Profile,SpTagId_t	TagId,void	FAR *Data);
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do progress callback. - MAC version
 *
 * AUTHOR
 * 	mec
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SpDoIter (
				SpTagIter_t	ProgressFunc,
				SpIterState_t	State,
				SpProfile_t		Profile,
				SpTagId_t		TagId,
				void			FAR *Data)
{
	#if defined(KPMAC68K)
	volatile long			hostA4, hostA5;
	volatile long 			thisA4, thisA5;
	#endif
	SpStatus_t  status;
	
	if (NULL == ProgressFunc)
		return SpStatSuccess;

	#if defined(KPMAC68K)
	/* restore host's global world - we don't know if its an A4 or A5*/
	SPretrieveCallbackA4A5(&hostA4, &hostA5);
	if (hostA5 != 0)
		thisA5 = SetA5(hostA5);			
	if (hostA4 != 0)
		thisA4 = SetA4(hostA4);
	#endif	

	status = CallSPIterCallBackFunc((spIterCallBackUPP)ProgressFunc,State, Profile, TagId, Data);

	#if defined (KPMAC68K)
	/* reset our global world */
	if (hostA5 != 0)
		SetA5(thisA5);								
	if (hostA4 != 0)
		SetA4(thisA4);								
	#endif

	return status;
}

#else
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Do Tag iter callback. - non MAC version
 *
 * AUTHOR
 * 	mec
 *
 * DATE CREATED
 *	May 5, 1995
 *------------------------------------------------------------------*/
SpStatus_t SpDoIter (
				SpTagIter_t	ProgressFunc,
				SpIterState_t	State,
				SpProfile_t		Profile,
				SpTagId_t		TagId,
				void			FAR *Data)
{
	if (NULL != ProgressFunc)
		return ProgressFunc (State, Profile, TagId, Data);

	return SpStatSuccess;
}
#endif

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert Profile handle to pointer to locked profile data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpProfileData_t	FAR *SpProfileLock (SpProfile_t Profile)
{
	SpProfileData_t	FAR *ProfileData;

	ProfileData = lockBuffer ((KcmHandle) Profile);
	if (NULL != ProfileData) {
#if defined(KPMAC)
		/* increment the lock counter */
		ProfileData->LockCount++;
#endif
		if (SpProfileDataSig != ProfileData->Signature) {
			unlockBuffer ((KcmHandle) Profile);
			return NULL;
		}
	}
	return ProfileData;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Unlock profile data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
void SpProfileUnlock (SpProfile_t Profile)
{

#if defined(KPMAC)

	SpProfileData_t	FAR *ProfileData;

	if (NULL == Profile)
		return;

	ProfileData = *((SpProfileData_t FAR **) Profile);
	ProfileData->LockCount--;
		
	/* lockCount == 0 means that we need to fully unlock the handle */
	if(ProfileData->LockCount == 0)
#endif
		unlockBuffer ((KcmHandle) Profile);

}

#if defined (KPMAC)
/*--------------------------------------------------------------------
 * DESCRIPTION	(Macintosh Version)
 *	Copies 
 *
 * AUTHOR
 * 	mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfSetIOFileData (
				SpIOFileChar_t	*src,
				SpIOFileChar_t	*dest)
{
	strncpy (dest->fileType, src->fileType, 5);
	strncpy (dest->creatorType, src->creatorType, 5);
	dest->vRefNum = src->vRefNum;

	return SpStatSuccess;
}
SpStatus_t SPAPI SpProfSetSpFileData (
				SpFileProps_t	*src,
				SpIOFileChar_t	*dest)
{
	strncpy (dest->fileType, src->fileType, 5);
	strncpy (dest->creatorType, src->creatorType, 5);
	dest->vRefNum = src->vRefNum;

	return SpStatSuccess;
}
/*--------------------------------------------------------------------
 * DESCRIPTION	(Macintosh Version)
 *	Copies 
 *
 * AUTHOR
 * 	mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfSetSpFileProps (
				SpFileProps_t	*src,
				SpFileProps_t	*dest)
{
	strncpy (dest->fileType, src->fileType, 5);
	strncpy (dest->creatorType, src->creatorType, 5);
	dest->vRefNum = src->vRefNum;
	dest->dirID   = src->dirID;

	return SpStatSuccess;
}
SpStatus_t SPAPI SpProfSetIOFileProps (
				SpIOFileChar_t	*src,
				SpFileProps_t	*dest)
{
	strncpy (dest->fileType, src->fileType, 5);
	strncpy (dest->creatorType, src->creatorType, 5);
	dest->vRefNum = src->vRefNum;
	dest->dirID   = 0;

	return SpStatSuccess;
}
SpStatus_t SPAPI SpProfileClearProps (
				SpFileProps_t	*dest)
{
	strncpy (dest->fileType, "     ", 5);
	strncpy (dest->creatorType, "     ", 5);
	dest->vRefNum = 0;
	dest->dirID   = 0;

	return SpStatSuccess;
}
#else
/*--------------------------------------------------------------------
 * DESCRIPTION	(non-Macintosh Version)
 *	Copies 
 *
 * AUTHOR
 * 	mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfSetIOFileData (
				SpIOFileChar_t	*src,
				SpIOFileChar_t	*dest)
{
	if (src) {}
	if (dest) {}

	return SpStatSuccess;
}
SpStatus_t SPAPI SpProfSetSpFileProps(
				SpFileProps_t	*src,
				SpFileProps_t	*dest)
{
	if (src) {}
	if (dest) {}

	return SpStatSuccess;
}
SpStatus_t SPAPI SpProfileClearProps(
				SpFileProps_t	*dest)
{
	if (dest) {}

	return SpStatSuccess;
}
/*--------------------------------------------------------------------
 * DESCRIPTION	(non-Macintosh Version)
 *	Copies 
 *
 * AUTHOR
 * 	copied
 *
 * DATE CREATED
 *	November 15, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfSetIOFileProps (
				SpIOFileChar_t	*src,
				SpFileProps_t	*dest)
{
	if (src) {}
	if (dest) {}

	return SpStatSuccess;
}
SpStatus_t SPAPI SpProfSetSpFileData  (
				SpFileProps_t	*src,
				SpIOFileChar_t	*dest)
{
	if (src) {}
	if (dest) {}

	return SpStatSuccess;
}
#endif


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Delete a profile. If there is a file associated with the profile,
 *	it is also deleted.
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileDelete (
				SpProfile_t FAR *Profile)
{
	KpChar_t		*fileName;
	SpProfileData_t	FAR *ProfileData;
	KpFileProps_t		Props;

/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (*Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* lock FileName handle and return  ptr */
	fileName = (char *) lockBuffer (ProfileData->FileName);

/* Convert SpIOFileChar_t to KpFileProps_t */
	SpCvrtSpFileProps (&(ProfileData->Props), &Props);

/* delete associated file name */
	if (NULL != fileName)
		KpFileDelete (fileName, &Props);

/* unlock handles */
	unlockBuffer (ProfileData->FileName);
	SpProfileUnlock (*Profile);
	return SpProfileFree (Profile);
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Iterate over all the tags in a profile.  Call user supplied
 *	function with the Id of each tag.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	July 7, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpTagIter (
				SpProfile_t		Profile,
				SpTagIter_t		TagIterFunc,
				void			FAR *Data)
{
	KpInt32_t		index;
	SpProfileData_t		FAR *ProfileData;
	SpTagDirEntry_t	FAR *tagArray;
	SpStatus_t			Status;

/* convert profile handle to pointer to locked memory */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

	Status = SpDoIter ( TagIterFunc, SpIterInit, NULL, 0, Data);

	/* Check if Profile found via Search function */
	if (ProfileData->TagArray == NULL)
		/* If so, it needs the Tag Array Loaded */
		SpProfilePopTagArray(ProfileData);

	for (index = 0; (index < ProfileData->TotalCount) && (Status == SpStatSuccess); index++)
	{
		/* User Callback Function could unlock since the
		   Profile is available, so lock before calling */
		/* lock TagArray handle and return ptr */
		tagArray = (SpTagDirEntry_t FAR *) 
				lockBuffer (ProfileData->TagArray);

		/* call users function ONLY IF VALID ENTRY */	
		if (tagArray[index].TagDataSize != -1)
			Status = SpDoIter(TagIterFunc,
					SpIterProcessing, 
					Profile, 
					tagArray[index].TagId, 
					Data);
	}

	unlockBuffer (ProfileData->TagArray);

	SpDoIter ( TagIterFunc, SpIterTerm, NULL, 0, Data);

	SpProfileUnlock (Profile);
	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Convert profile file to internal format.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 18, 1993
 *------------------------------------------------------------------*/
SpStatus_t SpProfileLoadFromBufferImp (
				SpProfileData_t	FAR *ProfileData,
				char			KPHUGE *BaseAddr)
{
	SpStatus_t		Status;
	char			KPHUGE *Ptr;
	KpUInt32_t		ProfileSize;
	KpUInt32_t		TagDataHeader;
	KpUInt32_t		Offset;
	KpUInt32_t		Count;
	KpUInt32_t		i;
	SpTagId_t		TagId;
	KpUInt32_t		TagOffset;
	KpUInt32_t		TagSize;

/* validate that atleast the first long of the buffer is readable */
	if (Kp_IsBadHugeReadPtr (BaseAddr, sizeof (KpUInt32_t)))
		return SpStatBadBuffer;

/* get the long, this is the size of the profile data */
	Ptr = BaseAddr;
	ProfileSize = SpGetUInt32 (&Ptr);

/* validate that the entire buffer is readable */
	if (Kp_IsBadHugeReadPtr (BaseAddr, ProfileSize))
		return SpStatBadBuffer;

/* convert the header to public form */
	Status = SpHeaderToPublic (BaseAddr, ProfileSize, &ProfileData->Header);
	if (SpStatSuccess != Status)
		return Status;

	TagDataHeader = 128;
	Ptr = BaseAddr + TagDataHeader;

/* check that the offset to the Tag Data is inside the file */
	if (ProfileSize < TagDataHeader + 4)
		return SpStatOutOfRange;

/* get number of tags */
	Offset = TagDataHeader + sizeof (KpUInt32_t);
	if (ProfileSize < Offset)
		return SpStatBadProfile;

	Ptr = BaseAddr + TagDataHeader;
	Count = SpGetUInt32 (&Ptr);

/* validate that tag directory is readable */
	if (ProfileSize < Offset + Count * 3 * sizeof (KpUInt32_t))
		Status = SpStatBadProfileDir;

/* build tag directory */
	Status = SpStatSuccess;
	for (i = 0; (SpStatSuccess == Status) && (i < Count); i++) {
		TagId = (SpTagId_t) SpGetUInt32 (&Ptr);
		TagOffset = SpGetUInt32 (&Ptr);
		if (ProfileSize < TagOffset)
			return SpStatBadProfileDir;

		if (TagOffset & 3)
			return SpStatBadProfileDir;

		TagSize = SpGetUInt32 (&Ptr);
		if (ProfileSize < TagOffset + TagSize)
			return SpStatBadProfileDir;

		Status = SpTagDirEntryAdd (ProfileData, TagId,
					TagSize, BaseAddr + TagOffset);
	}

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Create profile that uses supplied buffer as the profile data.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileLoadFromBuffer (
				SpCallerId_t	CallerId,
				void			FAR *Buffer,
				SpProfile_t		FAR *Profile)
{
	SpStatus_t		Status;
	SpProfileData_t	FAR *ProfileData;
	
	Status = SpProfileAlloc (CallerId, Profile, &ProfileData);
	if (SpStatSuccess != Status)
		return Status;

/* get profile header into internal format */
	Status = SpProfileLoadFromBufferImp (ProfileData,  Buffer);

	SpProfileUnlock (*Profile);

/* free profile if an error occurred */
	if (SpStatSuccess != Status)
		SpProfileFree (Profile);

	return Status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Return the header for the profile.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 18, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileGetHeader (
				SpProfile_t		Profile,
				SpHeader_t		FAR *Header)
{
	SpProfileData_t FAR *ProfileData;
	
/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

	*Header = ProfileData->Header;
	SpProfileUnlock (Profile);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Set the header for the profile.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	March 18, 1994
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSetHeader (
				SpProfile_t		Profile,
				SpHeader_t		FAR *Header)
{
	SpProfileData_t FAR *ProfileData;
	
/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

	ProfileData->Header = *Header;
	SpProfileUnlock (Profile);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Returns KPTRUE if seems to be ICC Formatted Profile
 *      private function
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	August 9, 1996
 *------------------------------------------------------------------*/
KpBool_t SPAPI SpIsICCProfile(	char                    *Filename,
				SpFileProps_t		*Props)
{
KpUInt32_t     ProfileSize;
KpUInt32_t     ProfileCode = 0;
SpStatus_t     Status = SpStatBadProfile;
KpFileId       FD;
char          *name;
int            Read_Size_OK, Read_Code_OK = 0;
int           Read_Amount = 4;
KpFileProps_t  fileProps;
KpBool_t       IsICC = KPFALSE;
char           ReadBufferArray[8];
char           *ReadBuffer;
 
   name = Filename;
    
    
   SpCvrtSpFileProps(Props, &fileProps);
   if (KpFileOpen(name, "r", &fileProps, &FD)) /* 0 = not opened */
   {
      /* Read HEADER_SIZE into Buffer */
      ReadBuffer = ReadBufferArray;
      Read_Size_OK =  KpFileRead(FD, ReadBuffer, &Read_Amount); 
      ProfileSize = SpGetUInt32(&ReadBuffer);
    
      if (KpFilePosition(FD, FROM_START, 36))
      {
         ReadBuffer = ReadBufferArray;
         Read_Code_OK = KpFileRead(FD, ReadBuffer, &Read_Amount);
         ProfileCode = SpGetUInt32(&ReadBuffer);
      }

      /* Close File */
      KpFileClose(FD);

      if ((Read_Size_OK) && (ProfileSize > HEADER_SIZE))
      {
         if ((Read_Code_OK) && (ProfileCode == (0x61637370)))
            IsICC = KPTRUE;
      }
   }
   return (IsICC);
} 


#if !defined (SP_NO_FILEIO) || !defined (SP_NO_SEARCH)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Return the header for the profile.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	October 23, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileLoadHeader(
                                char                    *Filename,
				SpFileProps_t		*Props,
                                SpHeader_t              FAR *Header)
{
SpHugeBuffer_t BufferAddress;
SpStatus_t     Status = SpStatBadProfile;
KpFileId       FD;
char          *name;
int            Read_OK;
int           Read_Amount = HEADER_SIZE;
KpFileProps_t  fileProps;
 
   if (!SpIsICCProfile(Filename, Props))
      return (SpStatBadProfile);

   name = Filename;
    
   BufferAddress = allocBufferPtr(HEADER_SIZE);
   if (BufferAddress == NULL)
      return (SpStatMemory);
    
   SpCvrtSpFileProps(Props, &fileProps);
   if (KpFileOpen(name, "r", &fileProps, &FD)) /* 0 = not opened */
   {
      /* Read HEADER_SIZE into Buffer */
      Read_OK =  KpFileRead(FD, BufferAddress, &Read_Amount); 
    
      /* Close File */
      if (!KpFileClose(FD))
         Status = SpStatBadProfile; 
    
      if (Read_OK)
      {
         Status = SpHeaderToPublic(BufferAddress, HEADER_SIZE, Header);
      }
   }
   freeBufferPtr(BufferAddress);
   return Status;

}
#endif /* !SP_NO_FILEIO || !SP_NO_SEARCH */

#if !defined (SP_NO_FILEIO)
/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Return the Tag from the profile.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	October 23, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileLoadTag(
                                char                   *Filename,
                                SpFileProps_t          *Props,
                                SpTagId_t               TagId,
                                SpTagValue_t            FAR *Value)
{
SpHeader_t       Header;
SpHeader_t       *HeaderPtr;
SpHugeBuffer_t   BufferAddress, TagAddress;
SpStatus_t       Status = SpStatSuccess;
KpFileId         FD;
KpFileProps_t    fileProps;
char             name[MAX_PATH];
long             FilePosition = HEADER_SIZE;
int             Read_Amount  = sizeof(KpInt32_t);
int             i, TagArraySize, TagBufferSize;
SpTagRecord_t   *TagArray; 

   if (!SpIsICCProfile(Filename, Props))
      return (SpStatBadProfile);

   strcpy (name, Filename);
	
   /* Need Header in case we are requesting a Named Color */
   HeaderPtr = (SpHeader_t *)NULL;
   if (TagId == SpTagNamedColor)
   {
      HeaderPtr = &Header;
      Status = SpProfileLoadHeader(name, Props, HeaderPtr);
   }
   if (Status == SpStatSuccess)
   {
      /* Set up for failure - yes I know I'm negative */
      Status = SpStatFileNotFound;

      SpCvrtSpFileProps(Props, &fileProps);
      if (KpFileOpen(Filename, "r", &fileProps, &FD)) /* 0 = not opened */
      {
         if (KpFilePosition(FD, FROM_START, FilePosition)) 
         {

      	    Status = SpStatFileReadError;
            /* Read TagArraySize into Buffer  */
            if (KpFileRead(FD, &TagArraySize, &Read_Amount)) 
            {
#if defined (KPLSBFIRST)
               /* If we are on a little endian machine we need to do byte swap	*/
               Kp_swab32 (&TagArraySize, 1);
#endif
               TagBufferSize = TagArraySize * 3 * 4;  /* each entry is 3*4 bytes */
               BufferAddress = allocBufferPtr(TagBufferSize);
    	       if (BufferAddress != NULL)
	       {   
                  if (KpFileRead(FD, BufferAddress, &TagBufferSize)) 
                  {
#if defined (KPLSBFIRST)
                  /* If we are on a little endian machine we need to do byte swap	*/
                     Kp_swab32 (BufferAddress, TagBufferSize / sizeof (KpInt32_t));
#endif
                     TagArray = BufferAddress;
      	             Status = SpStatTagNotFound;
                     for (i = 0; i < TagArraySize; i++)
                     {
                        if (TagId == (SpTagId_t)TagArray[i].Id)
                        {
      	                   Status = SpStatMemory;
                           TagAddress = allocBufferPtr(TagArray[i].Size);
                           if (TagAddress != NULL) 
                           {
                              if (KpFilePosition(FD, FROM_START, TagArray[i].Offset))
                              {
                                 Read_Amount = (long)TagArray[i].Size;
                                 if (KpFileRead(FD, TagAddress, &Read_Amount))
                                 {
                                    Status = SpTagToPublic(HeaderPtr,   TagId, 
                                                     TagArray[i].Size,
                                                     TagAddress,  Value);
                                 }
                              }
                              freeBufferPtr(TagAddress);
                           } else
                           {
      	                      Status = SpStatMemory;
                              break;
                           }
                        }
                     }
                  }
                  freeBufferPtr(BufferAddress);
	       } else /* Buffer Address != 0 */
      	          Status = SpStatMemory;
            }
         }
         /* Close File */
         KpFileClose(FD); 
      }
   }
   return Status;

}
#endif /* !SP_NO_FILEIO */



#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION	(Private -- phase out 5/4/95)
 *	Return the name of the currently associated file.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileGetFileName (
				SpProfile_t		Profile,
				size_t			BufferSize,
				char			FAR *Buffer)
{
	char				FAR *fileName;
	SpProfileData_t		FAR *ProfileData;
	
/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* lock FileName handle and return  ptr */
	fileName = (char *) lockBuffer (ProfileData->FileName);

/* check for case of no file name */
	if (NULL == fileName) {
		*Buffer = '\0';
		SpProfileUnlock (Profile);
		return SpStatNoFileName;
	}

/* check for buffer large enough */
	if (BufferSize < strlen (fileName) + 1) {
		unlockBuffer (ProfileData->FileName);
		SpProfileUnlock (Profile);
		return SpStatBufferTooSmall;
	}

	strcpy (Buffer, fileName);
	
/* unlock handles */
	unlockBuffer (ProfileData->FileName);
	SpProfileUnlock (Profile);
	
	return SpStatSuccess;
}
#endif

/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Return the name of the currently associated file.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	November 16, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileGetName (
				SpProfile_t		Profile,
				size_t			BufferSize,
				KpChar_t		*Buffer,
				SpFileProps_t	*Props)
{
	char				FAR *fileName;
	SpProfileData_t			FAR *ProfileData;
	
/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* lock FileName handle and return  ptr */
	fileName = (char *) lockBuffer (ProfileData->FileName);

/* check for case of no file name */
	if (NULL == fileName) {
		*Buffer = '\0';
		SpProfileUnlock (Profile);
		return SpStatNoFileName;
	}

/* check for buffer large enough */
	if (BufferSize < strlen (fileName) + 1) {
		unlockBuffer (ProfileData->FileName);
		SpProfileUnlock (Profile);
		return SpStatBufferTooSmall;
	}

	strcpy (Buffer, fileName);

/* Get file properties */
	SpProfSetSpFileProps(&(ProfileData->Props), Props);
	
/* unlock handles */
	unlockBuffer (ProfileData->FileName);
	SpProfileUnlock (Profile);
	
	return SpStatSuccess;
}
#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Return the name of the currently associated file.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	September 20, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileGetDiskName (
				SpProfile_t		Profile,
				size_t			BufferSize,
				KpChar_t		*Buffer,
				SpIOFileChar_t	*Props)
{
	char				FAR *fileName;
	SpProfileData_t		FAR *ProfileData;
	
/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* lock FileName handle and return  ptr */
	fileName = (char *) lockBuffer (ProfileData->FileName);

/* check for case of no file name */
	if (NULL == fileName) {
		*Buffer = '\0';
		SpProfileUnlock (Profile);
		return SpStatNoFileName;
	}

/* check for buffer large enough */
	if (BufferSize < strlen (fileName) + 1) {
		unlockBuffer (ProfileData->FileName);
		SpProfileUnlock (Profile);
		return SpStatBufferTooSmall;
	}

	strcpy (Buffer, fileName);

/* Get file properties */
	SpProfSetSpFileData (&(ProfileData->Props), Props);
	
/* unlock handles */
	unlockBuffer (ProfileData->FileName);
	SpProfileUnlock (Profile);
	
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION	(Private -- phase out 5/4/95)
 *	Set the name of the file to associate with the profile.
 *	If the Name is NULL no file will be associated with the profile.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	Octorber 22, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSetFileName (
				SpProfile_t		Profile,
				char			FAR *FileName)
{
	char			FAR *fileName;
	SpProfileData_t	FAR *ProfileData;
	
/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* Free current FileName if one is there */
	if (ProfileData->FileName != NULL) 
		freeBuffer(ProfileData->FileName);

/* create the FileName handle */
	ProfileData->FileName = allocBufferHandle (strlen (FileName) + 1);
	if (ProfileData->FileName == NULL) 
	   return (SpStatMemory);
	
/* lock FileName handle and return ptr */
	fileName = (char *) lockBuffer (ProfileData->FileName);
	if (fileName == NULL) 
	   return (SpStatMemory);
	
/* copy text data into the newly allocated space */
	strcpy (fileName, FileName);
	
	SpProfileClearProps(&(ProfileData->Props));

/* unlock handles */
	unlockBuffer (ProfileData->FileName);
	SpProfileUnlock (Profile);
	return SpStatSuccess;
}
#endif

/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Set the name of the file to associate with the profile.
 *	If the Name is NULL no file will be associated with the profile.
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE Copied
 *	November 15, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSetName (
				SpProfile_t		Profile,
				KpChar_t		*FileName,
				SpFileProps_t	*Props)
{
	char			FAR *fileName;
	SpProfileData_t		FAR *ProfileData;
	SpStatus_t		status;
	
/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* Free current FileName if one is there */
	if (ProfileData->FileName != NULL) 
	{
		if (ProfileData->TagArray == NULL)
			status = SpProfilePopTagArray(ProfileData);

		freeBuffer(ProfileData->FileName);
	}

/* create the FileName handle */
	ProfileData->FileName = allocBufferHandle (strlen (FileName) + 1);
	if (ProfileData->FileName == NULL) 
	   return SpStatMemory;
	
/* lock FileName handle and return ptr */
	fileName = (char *) lockBuffer (ProfileData->FileName);
	if (fileName == NULL) 
	   return SpStatMemory;
	
/* copy text data into the newly allocated space */
	strcpy (fileName, FileName);

/* Copy props information into internal profile data structure */
	SpProfSetSpFileProps (Props, &(ProfileData->Props));
	
/* unlock handles */
	unlockBuffer (ProfileData->FileName);
	SpProfileUnlock (Profile);
	return SpStatSuccess;
}

#if !defined (SP_NO_OBSOLETE)
/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Set the name of the file to associate with the profile.
 *	If the Name is NULL no file will be associated with the profile.
 *
 * AUTHOR
 * 	lsh & mlb
 *
 * DATE CREATED
 *	May 4, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSetDiskName (
				SpProfile_t		Profile,
				KpChar_t		*FileName,
				SpIOFileChar_t	*Props)
{
	char			FAR *fileName;
	SpProfileData_t	FAR *ProfileData;
	
/* convert profile handle to data pointer */
	ProfileData = SpProfileLock (Profile);
	if (NULL == ProfileData)
		return SpStatBadProfile;

/* Free current FileName if one is there */
	if (ProfileData->FileName != NULL) 
		freeBuffer(ProfileData->FileName);

/* create the FileName handle */
	ProfileData->FileName = allocBufferHandle (strlen (FileName) + 1);
	if (ProfileData->FileName == NULL) 
	   return SpStatMemory;
	
/* lock FileName handle and return ptr */
	fileName = (char *) lockBuffer (ProfileData->FileName);
	if (fileName == NULL) 
	   return SpStatMemory;
	
/* copy text data into the newly allocated space */
	strcpy (fileName, FileName);

/* Copy props information into internal profile data structure */
	SpProfSetIOFileProps(Props, &(ProfileData->Props));
	
/* unlock handles */
	unlockBuffer (ProfileData->FileName);
	SpProfileUnlock (Profile);
	return SpStatSuccess;
}


/*--------------------------------------------------------------------
 * DESCRIPTION	(Private -- phase out 5/4/95)
 *	Load a profile from a file.
 *
 * AUTHOR
 * 	lsh
 *
 * DATE CREATED
 *	October 21, 1993
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileLoadFromFile (
				SpCallerId_t	CallerId,
				char			FAR *FileName,
				SpProfile_t		FAR *Profile)
{
	SpFileProps_t	Props;

#if defined (KPMAC)
/* map ths routine to current SpProfileLoadProfile routine */
	Props.vRefNum = Props.dirID = 0;
#endif

/* load the profile */
	return (SpProfileLoadProfile(CallerId, FileName, &Props, Profile));
}

/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Load a profile from a file.
 *
 * AUTHOR
 * 	mlb
 *
 * DATE CREATED
 *	May 2, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileLoadFromDisk (
				SpCallerId_t	CallerId,
				KpChar_t		*FileName,
				SpIOFileChar_t	*Props,
				SpProfile_t		FAR *Profile)
{
	SpFileProps_t	props;

/* map ths routine to current SpProfileLoadProfile routine */
	SpCvrtIOFileProps(Props, &props);

/* load the profile */
	return (SpProfileLoadProfile(CallerId, FileName, &props, Profile));

}
#endif

#if !defined (SP_NO_FILEIO)
/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Load a profile from a file.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	December 5, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileLoadProfile (
				SpCallerId_t	CallerId,
				KpChar_t	*FileName,
				SpFileProps_t	*Props,
				SpProfile_t	FAR *Profile)
{
	KpMapFile_t	MapFileCtl;
	KpFileProps_t	props;
	SpStatus_t	Status;

/* Convert file information into useable form */
	SpCvrtSpFileProps(Props, &props);

/* Map the file */
	if (NULL == KpMapFileEx (FileName, &props, "r", &MapFileCtl))
		return SpStatFileNotFound;

/* Load the profile */
	Status = SpProfileLoadFromBuffer (CallerId, MapFileCtl.Ptr, 
                                          Profile);

/* Unmap the file */
	KpUnMapFile (&MapFileCtl);

/* Remember the file name */
	if (SpStatSuccess == Status) {
		Status = SpProfileSetName (*Profile, FileName, Props);
		if (SpStatSuccess != Status)
			SpProfileFree (Profile);
	}

	return Status;
}
#endif /* !SP_NO_FILEIO */

/*--------------------------------------------------------------------
 * DESCRIPTION  (Semi-Private)
 *      Valid the handle points to memory with Sig set correctly
 *
 * AUTHOR
 *      doro
 *
 * DATE CREATED
 *      January 31, 1996
 *------------------------------------------------------------------*/
KpBool_t SPAPI SpProfileValidHandle(SpProfile_t SpProf)
{
SpProfileData_t		*Pf;
KpBool_t		Result = KPFALSE;
 
   if (SpProf != NULL)
   {

      Pf = SpProfileLock(SpProf);
      if (Pf != NULL)
      {
         if (Pf->Signature == SpProfileDataSig)
            Result = KPTRUE;
         SpProfileUnlock(SpProf);
      }
   }
   return Result;
}

