/*********************************************************************/
/*
	Contains:	This module contains the profile functions.

				Separated out of sprofile.c
				August 19, 1997

	Written by:	The Kodak CMS MS Windows Team

	Copyright:	(C) 1993-1998 by Eastman Kodak Company, 
			all rights reserved.

	Macintosh
	Change History (most recent first):

	Windows Revision Level:

	SCCS Revision:
	@(#)spsearch.c	1.8	1/19/98

	To Do:
*/
/*********************************************************************/


/**********************************************************************
 **********************************************************************
 **********************************************************************
 *** COPYRIGHT (c) Eastman Kodak Company, 1993-1998                 ***
 *** As  an unpublished  work pursuant to Title 17 of the United    ***
 *** States Code.  All rights reserved.                             ***
 **********************************************************************
 **********************************************************************
 **********************************************************************/

#include "sprof-pr.h"
#include <string.h>



/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Order the list of profiles by newest creation date
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	December 15, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileOrderList(
			SpProfile_t	*profileList,
			KpInt32_t	foundCount)
{
   KpInt32_t	Date, *DateList;
   KpInt32_t	Time, *TimeList;
   SpHeader_t	PfHeader;
   KpHandle_t	DateHandle, TimeHandle;
   int		i, j, z;
   SpStatus_t	Status;
   SpProfile_t	profileToMove;

   if (foundCount < 2)
      return SpStatSuccess;

   if ((DateHandle = allocBufferHandle(foundCount*sizeof(KpInt32_t))) ==
       (KpHandle_t)NULL)
      return SpStatBadBuffer;

   if ((DateList = (KpInt32_t *)lockBuffer(DateHandle)) ==
       (KpInt32_t *)NULL)
   {
      freeBuffer(DateHandle);
      return SpStatBadBuffer;
   }

   if ((TimeHandle = allocBufferHandle(foundCount*sizeof(KpInt32_t))) ==
       (KpHandle_t)NULL)
   {
      freeBuffer(DateHandle);
      return SpStatBadBuffer;
   }
   if ((TimeList = (KpInt32_t *)lockBuffer(TimeHandle)) ==
       (KpInt32_t *)NULL)
   {
      freeBuffer(DateHandle);
      freeBuffer(TimeHandle);
      return SpStatBadBuffer;
   }

   Status = SpProfileGetHeader (profileList[0], &PfHeader);
   Date = 366*PfHeader.DateTime.Year +
           31*PfHeader.DateTime.Month +
              PfHeader.DateTime.Day;
   Time = 3600*PfHeader.DateTime.Hour +
            60*PfHeader.DateTime.Minute +
               PfHeader.DateTime.Second;

   /* Save First Profile's date */
   DateList[0] = Date;
   TimeList[0] = Time;
 
   for (i = 1; i < foundCount; i++)
   {
      /* Get testable version of Time and Date */
      Status = SpProfileGetHeader (profileList[i], &PfHeader);
      Date = 366*PfHeader.DateTime.Year +
              31*PfHeader.DateTime.Month +
                 PfHeader.DateTime.Day;
      Time = 3600*PfHeader.DateTime.Hour +
               60*PfHeader.DateTime.Minute +
                  PfHeader.DateTime.Second;
 
      /* Save in case no move required */
      DateList[i] = Date;
      TimeList[i] = Time;
 
      /* Loop thru those before */
      for (j = 0; j < i; j++)
      {
         /* Test for created before a previous entry */
         if ((Date  > DateList[j]) ||
            ((Date == DateList[j]) && (Time > TimeList[j])))
         {
            /* Save the profile o finterest */
            profileToMove = profileList[i];
            /* Move the older profiles back */
            for (z = i; z > j; z--)
            {
               DateList[z]    = DateList[z-1];
               TimeList[z]    = TimeList[z-1];
               profileList[z] = profileList[z-1];
            }
            /* Put the Profile of interest into its spot */
            DateList[j]    = Date;
            TimeList[j]    = Time;
            profileList[j] = profileToMove;
            /* Check the next one on the complete list (i) */
            break;
         }
      }
   }
 
   freeBuffer(DateHandle);
   freeBuffer(TimeHandle);
   return SpStatSuccess;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Test Date Value against Header values.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	October 23, 1995
 *------------------------------------------------------------------*/
KpInt32_t TestHeaderDate(SpDateTime_t *ValA, SpDateTime_t *ValB)
{
   KpInt32_t DateA, DateB;
   KpInt32_t TimeA, TimeB;

   DateA = 366*ValA->Year + 31*ValA->Month + ValA->Day;
   DateB = 366*ValB->Year + 31*ValB->Month + ValB->Day;

   if (DateA < DateB)
      return BEFORE;
   if (DateA > DateB)
      return AFTER;

   TimeA = 3600*ValA->Hour + 60*ValA->Minute + ValA->Second;
   if (TimeA == 0)
      return SAME;

   TimeB = 3600*ValB->Hour;
   if ((ValA->Minute > 0) && (ValA->Second > 0))
      TimeB += 60*ValB->Minute;
   if (ValA->Second > 0)
      TimeB += ValB->Second;

   if (TimeA < TimeB)
      return BEFORE;
   if (TimeA > TimeB)
      return AFTER;

   return SAME;
}

/*--------------------------------------------------------------------
 * DESCRIPTION
 *	Test profile header with search criteria and report status
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	October 23, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileCheck(
                                SpSearch_p              SearchValue,
                                SpHeader_t              FAR *Header)
{
KpInt32_t Passed[SPSEARCH_LIST];
KpInt32_t Tested[SPSEARCH_LIST];
int       i;

if (SearchValue != (SpSearch_t *)NULL)
{
   for (i = 0; i < SPSEARCH_LIST; i++)
   {
      /* preset all entries in PassedEvalArray to Not Tested and failed . */
      Passed[i]  = 0;
      Tested[i]  = 0;
   }
   
   /* Loop thru the parameters, test the value based upon the Field
    *   Set that it Passed if it did.  Set that it was tested always
    */
   for (i = 0; i < SearchValue->critCount; i++)
   {
      switch (SearchValue->criterion[i].SearchElement) {
         case SPSEARCH_PREFERREDCMM       :
            if (SearchValue->criterion[i].SearchValue.Signature ==
                                    (KpInt32_t) Header->CMMType)
               Passed[SPSEARCH_PREFERREDCMM] = 1;
            Tested[SPSEARCH_PREFERREDCMM]    = 1;
            break;
    
         case SPSEARCH_VERSION            :
            if (SearchValue->criterion[i].SearchValue.Value     ==
                                     Header->ProfileVersion)
               Passed[SPSEARCH_VERSION]      = 1;
            Tested[SPSEARCH_VERSION]         = 1;
            break;
    
         case SPSEARCH_PROFILECLASS       :
            if (SearchValue->criterion[i].SearchValue.Signature ==
                                 Header->DeviceClass)
               Passed[SPSEARCH_PROFILECLASS] = 1;
            Tested[SPSEARCH_PROFILECLASS]    = 1;
            break;
    
         case SPSEARCH_DEVICECOLORSPACE   :
            if (SearchValue->criterion[i].SearchValue.Signature ==
                              Header->DataColorSpace)
               Passed[SPSEARCH_DEVICECOLORSPACE] = 1;
            Tested[SPSEARCH_DEVICECOLORSPACE]    = 1;
            break;
    
         case SPSEARCH_CONNECTIONSPACE    :
            if (SearchValue->criterion[i].SearchValue.Signature ==
                       Header->InterchangeColorSpace)
               Passed[SPSEARCH_CONNECTIONSPACE] = 1;
            Tested[SPSEARCH_CONNECTIONSPACE]    = 1;
            break;
    
         /* Only use ONDATE value for all date testing since more
          * than one value can be given for an attribute */
         case SPSEARCH_BEFOREDATE         :
            if (BEFORE == TestHeaderDate(
                            &Header->DateTime,
                            &SearchValue->criterion[i].SearchValue.Date))
               Passed[SPSEARCH_ONDATE]          = 1;
            Tested[SPSEARCH_ONDATE]             = 1;
            break;
    
         case SPSEARCH_ONDATE             :
            if (SAME   == TestHeaderDate(
                            &Header->DateTime,
                            &SearchValue->criterion[i].SearchValue.Date))
               Passed[SPSEARCH_ONDATE]          = 1;
            Tested[SPSEARCH_ONDATE]             = 1;
            break;
    
         case SPSEARCH_AFTERDATE          :
            if (AFTER  == TestHeaderDate(
                            &Header->DateTime,
                            &SearchValue->criterion[i].SearchValue.Date))
               Passed[SPSEARCH_ONDATE]          = 1;
            Tested[SPSEARCH_ONDATE]             = 1;
            break;
    
         case SPSEARCH_PLATFORM           :
            if (SearchValue->criterion[i].SearchValue.Signature ==
                                    Header->Platform)
               Passed[SPSEARCH_PLATFORM] = 1;
            Tested[SPSEARCH_PLATFORM]    = 1;
            break;
   
         case SPSEARCH_PROFILEFLAGS       :
            if (SearchValue->criterion[i].SearchValue.Value     ==
                                       Header->Flags)
               Passed[SPSEARCH_PROFILEFLAGS] = 1;
            Tested[SPSEARCH_PROFILEFLAGS]    = 1;
            break;
    
         case SPSEARCH_DEVICEMFG          :
            if (SearchValue->criterion[i].SearchValue.Signature ==
                          Header->DeviceManufacturer)
               Passed[SPSEARCH_DEVICEMFG] = 1;
            Tested[SPSEARCH_DEVICEMFG]    = 1;
            break;
    
         case SPSEARCH_DEVICEMODEL        :
            if (SearchValue->criterion[i].SearchValue.Signature ==
                                 Header->DeviceModel)
               Passed[SPSEARCH_DEVICEMODEL] = 1;
            Tested[SPSEARCH_DEVICEMODEL]    = 1;
            break;
    
         case SPSEARCH_DEVICEATTRIBUTESHI :
            if (SearchValue->criterion[i].SearchValue.Value     ==
                         Header->DeviceAttributes.hi)
               Passed[SPSEARCH_DEVICEATTRIBUTESHI] = 1;
            Tested[SPSEARCH_DEVICEATTRIBUTESHI]    = 1;
            break;
    
         case SPSEARCH_DEVICEATTRIBUTESLO :
            if (SearchValue->criterion[i].SearchValue.Value     ==
                         Header->DeviceAttributes.lo)
               Passed[SPSEARCH_DEVICEATTRIBUTESLO] = 1;
            Tested[SPSEARCH_DEVICEATTRIBUTESLO]    = 1;
            break;
    
         case SPSEARCH_RENDERINGINTENT    :
            if (SearchValue->criterion[i].SearchValue.Signature ==
                             (KpInt32_t) Header->RenderingIntent)
               Passed[SPSEARCH_RENDERINGINTENT] = 1;
            Tested[SPSEARCH_RENDERINGINTENT]    = 1;
            break;
    
         case SPSEARCH_ILLUMINANT         :
            if ((SearchValue->criterion[i].SearchValue.XYZ.X    ==
                                Header->Illuminant.X)
            &&  (SearchValue->criterion[i].SearchValue.XYZ.Y    ==
                                Header->Illuminant.Y)
            &&  (SearchValue->criterion[i].SearchValue.XYZ.Z    ==
                                Header->Illuminant.Z))
               Passed[SPSEARCH_ILLUMINANT] = 1;
            Tested[SPSEARCH_ILLUMINANT]    = 1;
            break;

	case SPSEARCH_ORIGINATOR         :
           if (SearchValue->criterion[i].SearchValue.Signature ==
                                Header->Originator)
               Passed[SPSEARCH_ORIGINATOR] = 1;
           Tested[SPSEARCH_ORIGINATOR]    = 1;
           break;

        case SPSEARCH_TIMEORDER:
        default:
           /* No special processing */
	   break;
      }
   }
   
   /* Loop thru enumeration types, Checking Passed vs Tested
    * for return value
    */
   for (i = 0; i < SPSEARCH_LIST; i++)
   {
      if (Tested[i] && !Passed[i])
         return SpStatBadProfile;
   }
}
return SpStatSuccess;

}



#if defined (KPWIN32)

	/*	Constant stings used to find the default profile 
	 *	data base path in the registry.
	 */
	static const char prefKPCMS []       = "kpcms.ini";
	static const char sectICCProfiles [] = "IniFileMapping\\kpcms.ini\\ICC Profiles";
	static const char keyDefPath []      = "Default Path";
	static const char win95ProfileDB []  = "\\Color";

/*--------------------------------------------------------------------
 * DESCRIPTION	(private)
 *	Get the default profile data base path for windows.  The path
 *	is returned in the buffer pointed to by pDataBasePath.  The 
 *	size of the buffer in bytes is specified by size.
 *
 * AUTHOR
 * 	M. Biamonte
 *
 * DATE CREATED
 *	November 16, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI GetWinDefaultDB (KpTChar_p pDataBasePath, KpInt32_t size)
{

	KpInt32_t			version, retVal;
	KpOsType_t			osType;
	KpChar_t			*p;
	HANDLE				hFindFile;
	WIN32_FIND_DATA		findData;
	char 				defProfileDB [260];
	SpFileProps_t		props;

	*pDataBasePath = '\0';
	KpGetSystemInfo (&osType, &version);
	switch (osType)
	{
		case KPOSTYPE_WIN32S:
		/* Create the {Windows System Directory}\Color
			   directory to use as the default */
 			retVal = GetSystemDirectory (defProfileDB, sizeof (defProfileDB));
			if (0 == retVal) {
				return SpStatFileNotFound;
			}

			/* Check for room to add  \\Color */
			if (retVal >= (KpInt32_t) (size - sizeof (win95ProfileDB))) {
				return SpStatBufferTooSmall;
			} 
 			strcat (defProfileDB, win95ProfileDB);

			retVal = KpReadStringPreferenceEx (
					(KpChar_p) prefKPCMS, 
					KPLOCAL_MACHINE,
					&props, 0, 0, 
					(KpChar_p) sectICCProfiles,
					(KpChar_p) keyDefPath, 
					(KpChar_p) defProfileDB,
					pDataBasePath, 
					(KpUInt32_p)&size);
			if (KCMS_SUCCESS != retVal) {
				if (KCMS_MORE_DATA == retVal) {
					return SpStatBufferTooSmall;
				}
				else {
					return SpStatFileNotFound;
				}
			}

			/* if trailing file separator - remove it */
			p = strrchr (pDataBasePath, '\\');
			if (p != NULL) {
				if (*(p+1) == '\0') {
					*p = '\0';
				}
			}
			break;


 		case KPOSTYPE_WINNT:
			/* Create the {Windows System Directory}\Color
			   directory to use as the default */
 			retVal = GetSystemDirectory (defProfileDB, sizeof (defProfileDB));
			if (0 == retVal) {
				return SpStatFileNotFound;
			}

			/* Check for room to add  \\Color */
			if (retVal >= (KpInt32_t) (size - sizeof (win95ProfileDB))) {
				return SpStatBufferTooSmall;
			} 
 			strcat (defProfileDB, win95ProfileDB);

			/* Read default DB from registry.  If one is not present, set registry to
			   default of windows\system\color.  Note - this doesn't use the
			   KpReadStringPreferenceEx call so that the ini file stuff is skipped.  */

			retVal = KpReadRegistryString ( KPLOCAL_MACHINE, 
					(KpChar_p) sectICCProfiles,
					(KpChar_p) keyDefPath, 
					pDataBasePath, 
					(KpUInt32_p)&size);

			if (KCMS_SUCCESS != retVal) {
				if (KCMS_MORE_DATA == retVal) {
					return SpStatBufferTooSmall;
				}
				else {
					/* write registry entry */
					retVal = KpWriteRegistryString (KPLOCAL_MACHINE, (KpChar_p) sectICCProfiles,
									(KpChar_p) keyDefPath, defProfileDB);
					if (KCMS_SUCCESS != retVal)
						return SpStatFileNotFound;
					strncpy (pDataBasePath, defProfileDB, size);
				}
			}

			/* if trailing file separator - remove it */
			p = strrchr (pDataBasePath, '\\');
			if (p != NULL) {
				if (*(p+1) == '\0') {
					*p = '\0';
				}
			}
			break;

		case KPOSTYPE_WIN95:
 			retVal = GetSystemDirectory (pDataBasePath, size);
			if (0 == retVal) {
				return SpStatFileNotFound;
			}
			/* Check for room to add  \\Color */
			if (retVal >= (KpInt32_t) (size - sizeof (win95ProfileDB))) {
				size = retVal + sizeof (win95ProfileDB);
				return SpStatBufferTooSmall;
			} 
 			strcat (pDataBasePath, win95ProfileDB);
			break;

		default:
			return SpStatFileNotFound;
	}

	/*	Check if path exists	*/
	hFindFile = FindFirstFile (pDataBasePath, &findData);

	if (INVALID_HANDLE_VALUE == hFindFile) {
		FindClose(hFindFile);
		return (SpStatFileNotFound);
	}
	FindClose(hFindFile);
	if (0 == (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {		
		return (SpStatFileNotFound);
	}

	return SpStatSuccess;

}

#endif /* if defined KPWIN32 */

/*--------------------------------------------------------------------
 * Routine to return the number of default databases for searching profiles
 *   on a platform 
 *
 * AUTHOR
 * 	Anne Rourke
 *
 * DATE CREATED
 *	April 24, 1997
 *------------------------------------------------------------------*/
static KpInt32_t GetDefaultDBCount(void)
{
#if defined (KPSUN)
  return 2;
#else
  return 1;
#endif
}

/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Return the default directory name(s) where the profiles
 *	were loaded.  The argument Entries points to the first element in
 *	an array of SpDataBaseEntry structures.  The number of entries in
 *	the array is specified in the location pointed to by numEntries.
 *	The size of the dirName field in each entry in the array is specified 
 *	in the location pointed to by entrySize.  If the dirName field is not
 *	large enough to hold the default directory names the size required 
 *	is returned in the location pointed to by entrySize. 
 *
 * AUTHOR
 * 	Marleen_Clark & doro & M. Biamonte
 *
 * DATE CREATED
 *	October 16, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileGetDefaultDB(
				KpInt32_t		numEntries,
				KpInt32_t		FileNameSize,
				SpDataBaseEntry_p	Entries)
{
KpInt32_t	numDBs;
#if defined (KPSGI) || defined(KPSGIALL) || defined (KPSUN)
KpBool_t	DoesExist;
dirStatus	Dstat;
#endif

#if defined (KPMAC)
OSErr           theErr;
long            theDirID;
short           theVRefNum;
char		fname[MAX_PATH];
CInfoPBRec	parm; /* parameters for call to ROM */
HFileInfo	*fParm = (HFileInfo *)&parm;
DirInfo		*dParm = (DirInfo *)&parm;
Boolean		async=false;
#endif

#if defined (KPWIN32)
KpInt32_t		retVal;
#endif

	numDBs = GetDefaultDBCount();

   if (numEntries < numDBs)
      return SpStatBufferTooSmall;

   numDBs = 0;

#if defined (KPSUN)
   if (FileNameSize < 40)  /* Testing against the longer 
                                   of the following two 
                                   directory paths */
      return SpStatBufferTooSmall;

   strcpy(Entries[numDBs].dirName, 
         "/usr/openwin/share/etc/devdata/profiles");
   Dstat = KpFileExists(Entries[numDBs].dirName,
                        &Entries[numDBs].fileProps,
                        &DoesExist);
   if (DoesExist == KPTRUE)
      numDBs++;

   strcpy(Entries[numDBs].dirName,
         "/etc/openwin/devdata/profiles");
   Dstat = KpFileExists(Entries[numDBs].dirName,
                        &Entries[numDBs].fileProps,
                        &DoesExist);
   if (DoesExist == KPTRUE)
      numDBs++;
      
     /* Set filenames of unused DBs to NULL */
   while (numDBs < numEntries)
   	 Entries[numDBs++].dirName[0] = '\0';

#else

#if defined (KPSGI) || defined(KPSGIALL)

   if (FileNameSize < 18)   /* Testing against the string size */
      return SpStatBufferTooSmall;

   strcpy(Entries[0].dirName,
         "/var/cms/profiles");

   Dstat = KpFileExists(Entries[numDBs].dirName, 
                        &Entries[numDBs].fileProps,
                        &DoesExist);
   if (DoesExist == KPTRUE)
      numDBs++;

     /* Set filenames of unused DBs to NULL */
   while (numDBs < numEntries)
   	 Entries[numDBs++].dirName[0] = '\0';

#elif defined (KPMAC)
   theErr = FindFolder(kOnSystemDisk,     kPreferencesFolderType,
                       kDontCreateFolder, &theVRefNum, &theDirID);

   if (theErr != noErr)
      return SpStatFileNotFound;

   /* Get the folder name */
   GetIndString((StringPtr)fname, COLORSYNCSTR, 1);
   if (fname[0] == 0)  /* test for empty pascal string */
      return SpStatFileNotFound;

   if (fname[0] > (unsigned long)FileNameSize)
      return SpStatBufferTooSmall;

   fParm->ioCompletion = nil;
   fParm->ioVRefNum    = theVRefNum;
   fParm->ioDirID      = theDirID;
   fParm->ioFDirIndex  = 0;
   fParm->ioNamePtr    = (StringPtr)fname;
   theErr = PBGetCatInfo(&parm,async);
   if (theErr == noErr )
   {
      strcpy(Entries[0].dirName, "");
      Entries[0].fileProps.dirID   = dParm->ioDrDirID;
      Entries[0].fileProps.vRefNum = fParm->ioVRefNum;
      numDBs++;
   } else
      return SpStatFileNotFound;

     /* Set filenames and Props of unused DBs to NULL */
   while (numDBs < numEntries) {
   	 Entries[numDBs].dirName[0] = '\0';
	 Entries[numDBs].fileProps.dirID   = 0;
     Entries[numDBs++].fileProps.vRefNum = 0;   	
   } 

#elif defined (KPWIN32)

	retVal = GetWinDefaultDB (Entries[0].dirName, FileNameSize);
	if (retVal == SpStatSuccess) {
		numDBs++;
	}
	else {
		return retVal;
	}
	
     /* Set filenames of unused DBs to NULL */
   while (numDBs < numEntries)
   	 Entries[numDBs++].dirName[0] = '\0';
#else

   return SpStatFileNotFound;

#endif

#endif
   return SpStatSuccess;
}

KpBool_t TestFileCB(KpfileDirEntry_t FAR *fileSearch,
                KpInt32_t            *Limits)
{

SpCallerId_t		CallerId = (SpCallerId_t)Limits[0];
SpSearch_t		*SearchCriterion = (SpSearch_t *)Limits[1];
SpProfile_t		*profileList = (SpProfile_t *)Limits[2];
SpProfile_t		*profPtr;
KpInt32_t		listSize = Limits[3];
KpInt32_t		CurrentIndex = Limits[4];

SpHeader_t		Header;
SpStatus_t		Status;
SpFileProps_t	FileProp;
SpProfileData_t FAR	*ProfData;
KpBool_t		returnStatus = KPTRUE;	/* assume we will continue */
char			*filename;
#if defined (KPWIN32) 
#endif
#if defined (KPMAC)
int			i;
#endif

if ((fileSearch->opStatus == IOFILE_FINISHED) ||
    (fileSearch->opStatus == IOFILE_STARTED) ||
	(fileSearch->opStatus == IOFILE_ERROR))
{
   return returnStatus;
}

#if defined (KPMAC)
   FileProp.vRefNum = fileSearch->osfPrivate.vRefNum;
   FileProp.dirID   = fileSearch->osfPrivate.dirID;
#endif

   Status = SpProfileLoadHeader(fileSearch->fileName,
                                    &FileProp, &Header);
   if (Status == SpStatSuccess)
   {
      Status = SpProfileCheck(SearchCriterion, &Header);
      if (Status == SpStatSuccess)
      {
         profPtr = &profileList[CurrentIndex];

         Status = SpProfileAlloc(CallerId,
                                 profPtr,
                                 &ProfData);
         if (Status == SpStatSuccess)
         {
            if ((Status = SpProfileSetHeader(*profPtr,
                                             &Header)) ==
                 SpStatSuccess)
            {
               ProfData->FileName = allocBufferHandle(
                  strlen(fileSearch->fileName) + 1);
               if (ProfData->FileName == 0)
               {
                  SpProfileFree(profPtr);
                  return KPFALSE;
               }
               filename = (char *)lockBuffer(ProfData->FileName);
               if (filename == 0)
               {
                  freeBuffer(ProfData->FileName);
                  SpProfileFree(profPtr);
                  return KPFALSE;
               }
               strcpy(filename, fileSearch->fileName);
	           unlockBuffer (ProfData->FileName);
               ProfData->TotalCount = 0;
               freeBuffer(ProfData->TagArray);
               ProfData->TagArray   = NULL;
#if defined (KPMAC)
               ProfData->Props.vRefNum     = fileSearch->osfPrivate.vRefNum;
               ProfData->Props.dirID       = fileSearch->osfPrivate.dirID;
               for (i= 0; i < 5; i++)
               {
                  ProfData->Props.fileType[i]    = NULL;
                  ProfData->Props.creatorType[i] = NULL;
               }
#endif

               CurrentIndex++;
               Limits[4] = CurrentIndex;
               if (CurrentIndex >= listSize)
                  returnStatus = KPFALSE;
            } else 
            {
               Limits[5] = (KpInt32_t)Status;
               returnStatus = KPFALSE;
            } /* if SpProfileSetHeader */
            SpProfileUnlock(*profPtr);
         } else
         {
            Limits[5] = (KpInt32_t)Status;
            returnStatus = KPFALSE;
         } /* if SpProfileAlloc */
      } /* if SpProfileCheck */
   } /* if SpProfileLoadHeader */
   return returnStatus;
}

/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Search thru the profiles for those that match the criteria
 *	were loaded.
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	October 16, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSearch(
				SpCallerId_t	CallerId,
				SpDataBase_t	*DataBaseList,
                                SpSearch_t	*SearchCriterion,
                                SpProfile_t	*profileList,
                                KpInt32_t	listSize,
                                KpInt32_t	*foundCount)
{
/* Sun returns 2 default diretories so set up for this incase
 * there is a null pointer passed for directories
 */
char 			Dir1[MAX_PATH], Dir2[MAX_PATH];
SpDataBaseEntry_t	List[2];		/* Max num default DBs on any platform */
SpDataBase_t		DataBase;
SpDataBase_p		DB;

SpStatus_t		status = SpStatSuccess;
KpBool_t		Fstatus;
KpHandle_t		Handle;
KpInt32_t 		*Limits, dirNameSize;

int			i;
KpfileDirEntry_t	fileSearch;

	
	status = SpCallerIdValidate (CallerId);
	if (SpStatSuccess != status)
		return status;
		
   /* Null DB entry means get default */
   if (DataBaseList == (SpDataBase_t *)NULL)
   {
      List[0].dirName     = (char *)Dir1;
      List[1].dirName     = (char *)Dir2;
      DataBase.numEntries =  GetDefaultDBCount();
      DataBase.Entries    = &List[0];
	  dirNameSize = sizeof (Dir1);
      status = SpProfileGetDefaultDB(DataBase.numEntries, dirNameSize, 
                                    DataBase.Entries);
      if (status != SpStatSuccess)
         return status;
      DB = &DataBase;
   } else
      DB = DataBaseList;

   /* Allocate space to point to the search criteria,
    * point to the SpProfile array, give the max profiles 
    * to find, and give the total found so far
    */
   if ((Handle = allocBufferHandle(6*sizeof(KpInt32_t))) !=
       (KpHandle_t)NULL)
   {
      if ((Limits = (KpInt32_t *)lockBuffer(Handle)) !=
          (KpInt32_t *)NULL)
      {
         Limits[0] = (KpInt32_t)CallerId;
         Limits[1] = (KpInt32_t)SearchCriterion;
         Limits[2] = (KpInt32_t)profileList;
         Limits[3] = listSize; /* for Maximum Profiles */
         Limits[4] = 0;        /* for Current number found */
         Limits[5] = (KpInt32_t) SpStatSuccess;   /* Init Good Status */

         /* Loop thru DB entries */
         for (i = 0; i < DB->numEntries && 
                     Limits[4] < Limits[3] &&
                     Limits[5] == (KpInt32_t)SpStatSuccess; ++i)
         {
            fileSearch.structSize   = sizeof(KpfileDirEntry_t);
            fileSearch.nwAttr       = ATTR_DIRECTORY;
            fileSearch.wAttr        = 0;
            strcpy(fileSearch.fileName, DB->Entries[i].dirName);
            fileSearch.subDirSearch = KPFALSE; 

#if defined (KPMAC)
	    fileSearch.osfPrivate.vRefNum      = 
               DB->Entries[i].fileProps.vRefNum;
	    fileSearch.osfPrivate.fileType     = 0;
	    fileSearch.osfPrivate.fileCreator  = 0;
	    fileSearch.osfPrivate.dirID        = 
               DB->Entries[i].fileProps.dirID;

#elif defined (KPWIN32)
		fileSearch.osfPrivate.filter[0] = '\0';
#endif

            /* The Call Back routine tests the individual
             * files reported for the test criteria and if
             * more profiles are found than there is space
             * for, it returns to stop the FileFind routine 
             * and this search loop.
             */
            Fstatus = KpFileFind(&fileSearch,  
                                 Limits, 
                                 (KpfileCallBack_t)TestFileCB);
         }
         if (Limits[5] != (KpInt32_t)SpStatSuccess)
         {
            status = (SpStatus_t)Limits[5];
            *foundCount = 0;
         } else
         {
            *foundCount = Limits[4];
            if ((*foundCount > 0) &&
                (SearchCriterion != (SpSearch_t *)NULL))
            {
               for (i = 0; i < SearchCriterion->critCount; i++)
               {
                  if (SearchCriterion->criterion[i].SearchElement == 
                      SPSEARCH_TIMEORDER) 
                  {
                     status = SpProfileOrderList(profileList, *foundCount);
                     break;
                  }
               }
            }
         }
      }
      else
      	status = SpStatMemory;
      	
      freeBuffer(Handle);
   }
   else
   	status = SpStatMemory;
   	
   return status;
}


/*--------------------------------------------------------------------
 * DESCRIPTION	(Public)
 *	Return the List of Profiles with those that pass the new
 *	search criteria in the front.  foundCount is the number
 *      of Profiles that were found that passed the search criteria
 *
 * AUTHOR
 * 	doro
 *
 * DATE CREATED
 *	November 20, 1995
 *------------------------------------------------------------------*/
SpStatus_t SPAPI SpProfileSearchRefine(
                                SpSearch_t	*SearchCriterion,
                                SpProfile_t	*profileList,
                                KpInt32_t	listSize,
                                KpInt32_t	*foundCount)
{

int			i, j;
SpProfileData_t		*Pf;
SpProfile_t		ThisProfile;
SpStatus_t		status;

   *foundCount = 0;

   /* Loop thru those profiles given and test header information
    * against search criteria
    */
   for (i = 0; i < listSize; i++)
   {
      Pf = SpProfileLock(profileList[i]);
      if (Pf == NULL)
         return SpStatBadProfile;

      if ((status = SpProfileCheck(SearchCriterion, &Pf->Header)) ==
          SpStatSuccess)
      {
         if (*foundCount == i)
            (*foundCount)++;
         else
         {
            ThisProfile = profileList[i];
            for (j = i; j > *foundCount;  j--)
            {
               profileList[j] = profileList[j - 1];
            }
            profileList[(*foundCount)++] = ThisProfile;
         }
      }
      SpProfileUnlock(profileList[i]);
   }
   if ((*foundCount > 0) &&
       (SearchCriterion != (SpSearch_t *)NULL))
   {
      for (i = 0; i < SearchCriterion->critCount; i++)
      {
         if (SearchCriterion->criterion[i].SearchElement == 
             SPSEARCH_TIMEORDER) 
         {
            status = SpProfileOrderList(profileList, *foundCount);
            break;
         }
      }
   }

   return SpStatSuccess;
}


