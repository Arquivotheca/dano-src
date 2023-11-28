/*

	File:           kpfile.c

	Contains:
		This module contains an operating system interface to doing PT
		file I/O.  It contains a section for each operating system, each
		separated by a #if.  The routines which must be supported are:

	int       KpFileOpen          ( char *fileName, char *mode,
					KpFileProps_t *props, KpFileId *id );
	int       KpFileClose         ( KpFileId id );
	int       KpFileRead          ( KpFileId id, KpLargeBuffer_t buf,
					KpInt32_t *numBytes );
	int       KpFileWrite         ( KpFileId id, KpLargeBuffer_t buf,
					KpInt32_t numBytes );
	int       KpFilePosition      ( KpFileId id, FileStart, KpInt32_t loc );
	int       KpFileTell          ( KpFileId id, KpInt32_t FAR *position );
	int       KpFileDelete        ( char *fileName, KpFileProps_t *prop );
	int       KpFileSize          ( char *fileName, KpFileProps_t *props,
					KpInt32_t *size );
	dirStatus KpFileExists        ( char *fileName, KpFileProps_t *props ,
					KpBool_t * doesExist);
	int       KpFileRename        ( KpFileProps_t *props, char *oldName, 
					char *newName );
	dirStatus KpFileDirCount      ( char *dirName, KpFileProps_t *props,
					KpInt32_t *numFiles );
	KpBoot_t  KpFileFind	      ( KpfileDirEntry_t FAR * fileSearch,
					void FAR * user_ptr,
					KpfileCallBack_t user_fcn );
	void	  KpFileStripPath     ( KpChar_p filePlusPath,
					KpChar_p theFile );	

        In addition, there is the following operating system independent
		routine:
				int KpFileCopy( KpFileProps_t * props, 
						char FAR *existingFile, 
				                char FAR *newFile );

		Each routine returns a 1 (KCMS_IO_SUCCESS) when successful and a 
		0 (KCMS_IO_ERROR) otherwise.  In addition, KpFileRead() returns 
		the actual number of bytes read through its input/output 
		parameter numBytes.
		
		WARNING: THIS FUNCTIONS IN THIS FILE ARE NOT SUPPORT WIN16 OR DOS!!!
	
		 These routines were taken from the io_file.c
		 and modified for the KpFilexxx fucntions. 
		 
		Created May 1, 1991 by Peter Tracy as io.c
		Modified for the PC starting October 11, 1991 by Robert Cushman
		Modified to KpFilexxx starting November 6, 1995 by Anne Rourke / Dave Oro.

	Written by:     Midnight KCMS Team

	Copyright:      (c) 1991-1997 by Eastman Kodak Company, all rights reserved.


	To Do:

	Revision:	
		@(#)kpfile.c	1.40	9/24/97

*/

/*
  PROPRIETARY NOTICE :   The  software  information  contained
  herein  is  the  sole  property  of  Eastman Kodak Company  and   is
  provided  to Eastman Kodak Company users  under license for use  on  
  their designated  equipment  only.  Reproduction of this matter in
  whole or in part is forbidden without  the  express  written
  consent of Eastman Kodak Company

  COPYRIGHT (c) 1991-1997 Eastman Kodak Company
  As  an  unpublished  work pursuant to Title 17 of the United
  States Code.  All rights reserved.
*/


#include "kcms_sys.h"
#include <string.h>

#define WAIT_LENGTH		25
#define COPY_BUFFER_SIZE	4096		/* size of buffer to use when copying */
#define KpWin32ioSize		(32*1024)	/* max bytes to read or write */


#if defined(KPUNIX)

/******************************************

		*** SUN MICROSYSTEMS SECTION ***

*******************************************/

#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/dirent.h> 

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileOpen  (Unix Version)
 *
 * DESCRIPTION
 * This function opens a file for either reading or writing.
 *
 * AUTHOR
 * Peter Tracy(originally part of FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 *--------------------------------------------------------------------*/
int KpFileOpen (
			char		*filename,
			char        	*mode,
			KpFileProps_t	*fileProps,
			KpFileId	*fdp)
{
	int		status;
	KpBool_t	FileExists;
	dirStatus	StatusRet;
	int		Counter;
	
	if (fileProps) {}

	switch (*mode) {
	    case 'r':
			*fdp = (KpFileId)open (filename, O_RDONLY, 0);
			/* Test for Lock in place */
			if (*fdp == (KpFileId)(-1))
			{
				StatusRet = KpFileExists(filename,
						 				 fileProps,
						 				 &FileExists);
				if (FileExists == KPTRUE)
				{
					for (Counter = WAIT_LENGTH; 
					     ((Counter > 5) && (*fdp == (KpFileId)(-1)));
					     Counter--)
					{
					KpSleep(Counter, 1);
					*fdp = (KpFileId)open(filename,
							      O_RDONLY, 0);
					}
				}
			}
			break;
	    case 'w':
			*fdp = (KpFileId)open (filename, O_WRONLY|O_CREAT|O_EXCL, 0664);
			/* The O_EXCL will replace a link with an 
			   empty file */
			/* Test for Lock in place */
			for (Counter = WAIT_LENGTH;
			     ((Counter > 5) && (*fdp == (KpFileId)(-1)));
			     Counter--)
			{
				StatusRet = KpFileExists(filename,
						 				 fileProps,
						 				 &FileExists);
				if (FileExists == KPTRUE)
				{
					KpSleep(Counter, 1);
					*fdp = (KpFileId)open(filename,
							      O_WRONLY|O_CREAT, 0664);
				} else
				{
					*fdp = (KpFileId)open(filename,
							      O_WRONLY|O_CREAT|O_EXCL, 0664);
				}
			}
			break;
	    case 'e':
			*fdp = (KpFileId)open (filename, O_WRONLY|O_CREAT|O_EXCL, 02664);
			/* The O_EXCL will replace a link with an 
			   empty file and 2664 means locked rw-rw-r-- */
			/* Test for Lock in place */
			for (Counter = WAIT_LENGTH;
			     ((Counter > 5) && (*fdp == (KpFileId)(-1)));
			     Counter--)
			{
				StatusRet = KpFileExists(filename,
						 				 fileProps,
						 				 &FileExists);
				if (FileExists == KPTRUE)
				{
					KpSleep(Counter, 1);
					*fdp = (KpFileId)open(filename,
							      O_WRONLY|O_CREAT, 02664);
				} else
				{
					*fdp = (KpFileId)open(filename,
							      O_WRONLY|O_CREAT|O_EXCL, 02664);
				}
			}
			break;
	    default:
			*fdp = (KpFileId)(-1);
			break;
	}
	status = (*fdp == (KpFileId)(-1))?  KCMS_IO_ERROR : KCMS_IO_SUCCESS;

	return status;
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileClose (Unix Version)
 *
 * DESCRIPTION
 * This function closes a file.
 *
 * AUTHOR
 * Peter Tracy(originally part of FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 *--------------------------------------------------------------------*/
int KpFileClose (
			KpFileId	fd)
{
	int		status;
	KpInt32_t	sysRet;

	sysRet = fchmod (fd, 0664);

	status = close(fd);

	return ( (status==(-1))? KCMS_IO_ERROR :KCMS_IO_SUCCESS );
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileSize  (Unix Version)
 *
 * DESCRIPTION
 * This function returns the size of a file.
 *
 * AUTHOR
 * Scott Kohnle
 *
 * DATE CREATED
 * March 21, 1994
 *
 *--------------------------------------------------------------------*/
int KpFileSize (
			char		*fileName,
			KpFileProps_t	*fileProps,
			KpInt32_t	*size)
{
	KpInt32_t pos;
	int fd;
	KpBool_t FileExists;
	dirStatus	StatusRet;
	int		Counter;

	if (fileProps) {}
	
	fd = open (fileName, O_RDONLY, 0);
	if (fd == -1) 
	{
		StatusRet = KpFileExists(fileName,
				 				 fileProps,
				 				 &FileExists);
		if (FileExists == KPTRUE)
		{
			for (Counter = WAIT_LENGTH;
			     ((Counter > 5) && (fd == -1));
			     Counter--)
			{
				KpSleep(Counter, 1);
				fd = open (fileName, O_RDONLY, 0);
			}
			if (fd == -1) 
				return KCMS_IO_ERROR;
		} else
			return KCMS_IO_ERROR;
	}

	/* seek to end of file */
	pos = (KpInt32_t)lseek (fd, 0, SEEK_END);
	close(fd);

	if (pos == -1)
		return KCMS_IO_ERROR;

	*size = pos;
	return KCMS_IO_SUCCESS;
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileExists (Unix Version)
 *
 * DESCRIPTION
 * This function checks to see if a file exists.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Dave Oro
 *
 * DATE CREATED
 * October 31, 1995
--------------------------------------------------------------------*/
dirStatus FAR KpFileExists (
         char        	FAR *path, 
         KpFileProps_t	FAR *fileProps, 
         KpBool_t	FAR *doesExist )
{
int      fd_stat;
struct stat buf;
 
   fd_stat = stat (path, &buf); 

   if (fd_stat == -1)
   {
      *doesExist = KPFALSE;
      return DIR_ERROR;
   } else
   {
      *doesExist = KPTRUE;
      return DIR_OK;
   }
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileDelete (Unix Version)
 *
 * DESCRIPTION
 * This function deletes a file given its name.
 *
 * AUTHOR
 * Scott Kohnle
 *
 * DATE CREATED
 * March 21, 1994
 *
 *
--------------------------------------------------------------------*/

int FAR KpFileDelete (
			char		FAR *fileName,
			KpFileProps_t	FAR *fileProps )
{
	fileProps = fileProps;	/* avoid warning */

	return (0 == remove(fileName)) ? KCMS_IO_SUCCESS : KCMS_IO_ERROR;
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileRead (Unix Version)
 *
 * DESCRIPTION
 * This function reads from the file.
 *
 * AUTHOR
 * Peter Tracy(derived from FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 *--------------------------------------------------------------------*/

int KpFileRead (
			KpFileId		fd,
			KpLargeBuffer_t	buf,
			KpInt32_t		*nbytes)
{
	KpInt32_t num_read;

	num_read = (KpInt32_t)read (fd, (char *)buf, *nbytes);
	*nbytes = num_read;
	return ( (num_read<=0)? KCMS_IO_ERROR:KCMS_IO_SUCCESS );
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileWrite (Unix Version)
 *
 * DESCRIPTION
 * This function writes to the file.
 *
 * AUTHOR
 * Peter Tracy(derived from FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 *--------------------------------------------------------------------*/

int KpFileWrite (
			KpFileId		fd,
			KpLargeBuffer_t	buf,
			KpInt32_t		nbytes)
{
	KpInt32_t num_write;

	num_write = (KpInt32_t)write (fd, buf, nbytes);

	return ( (num_write == nbytes)?  KCMS_IO_SUCCESS : KCMS_IO_ERROR );
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFilePosition (Unix Version)
 *
 * DESCRIPTION
 * This function positions the file.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * May 2, 1991
 *
 *--------------------------------------------------------------------*/
int KpFilePosition (
			KpFileId	fd,
			KpFileStart	start,
			KpInt32_t	position)
{
	int     origin;           /* where to start seek from */
	off_t   status;           /* status of lseek */

	/*
	 *  Set up orgin of seek depending
	 *  on input parameter
	 */
	if( start == FROM_START )
		origin = SEEK_SET;
	else if( start == FROM_END )
		origin = SEEK_END;
	else
		origin = SEEK_CUR;

	/*
	 *  Now actually do the seek and check the status
	 */
	status = lseek(fd,(off_t)position,origin);
	if( status == -1L )
		return   KCMS_IO_ERROR;

	return   KCMS_IO_SUCCESS;
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileTell (Unix Version)
 *
 * DESCRIPTION
 * This function returns the current byte position of the file.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Larry Hamilton
 *
 * DATE CREATED
 * April 6, 1994
 *
--------------------------------------------------------------------*/
int FAR KpFileTell (
			KpFileId	fd,
			KpInt32_t	FAR *position)
{
int         status;
KpInt32_t   fPosition;

	fPosition = (KpInt32_t)lseek (fd, 0, 1);
	if (-1 == fPosition) {
		status = KCMS_IO_ERROR;
		*position = 0;
	}
	else {
		status = KCMS_IO_SUCCESS;
		*position = fPosition;
	}

	return   status;
}



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileRename (Unix Version)
 *
 * DESCRIPTION
 * This function renames a file.
 *
 * RETURN
 * A return int of 1 means it worked OK.
 * A return int of 0 means an error occured.
 *
 * AUTHOR
 * mjb
 *
 * DATE CREATED
 * 6/28/94
 *
--------------------------------------------------------------------*/
int FAR KpFileRename (
			KpFileProps_t *props,
			char	  FAR *oldName,
			char	  FAR *newName)
{
	if (props) {}
	return (0 == rename (oldName, newName)) ? KCMS_IO_SUCCESS : KCMS_IO_ERROR;
}

/*--------------------------------------------------------------------
 * FUNCTION
 *	KpFileStripPath (Unix Version)
 *
 * DESCRIPTION 
 * This function strips off the path of a file. 
 *
 * RETURN VALUE
 *  None.
 * 
 * AUTHOR
 * sek
 *
 * DATE CREATED
 * April 30, 1995
 *-------------------------------------------------------------------*/

void KpFileStripPath (KpChar_p filePlusPath, KpChar_p theFile)
{
	char *fPtr;
	KpInt16_t i;

	/* clear theFile buffer */
	theFile[0] = '\0';

	/* find the last occurrence of the slash */
	fPtr = strrchr (filePlusPath, '//');
	if (fPtr == NULL) {
		/* no path, so just copy the file name */
		fPtr = &filePlusPath[0];
	}
	else
		*fPtr++;

	/* get the file name */
	i = 0;
	while (*fPtr != '\0')
	{
		theFile[i++] = *fPtr++;
	}
	theFile[i] = '\0';
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileFind(Unix Version)
 *
 * DESCRIPTION
 * This function finds files in a directory.
 * It calls a callback fuinction with the file or directory name in
 * the fileName item of the fileSearch parameter.  This structure
 * also contains an flag stating if it is the first or last call (no
 * filename included) or a call with fileName filled (First or Next).
 *
 * This will be used for the SpProfileSearch routine as follows:
 *
 * Call this routine with the pointer to an array containing a
 * pointer to the CallerID, a pointer to the SearchCriteria, a
 * pointer to the SpProfile Array, a pointer to the Max number of 
 * SpProfiles and a pointer to the current number of SpProfiles 
 * found with a callback that tests the profile against the search 
 * criteria and puts any passed profile in the array in the next
 * available space and increments the number contained not 
 * exceeding the space available.
 *
 * adapted from io version by
 * Dave  Oro
 *
 * DATE CREATED
 * October 31, 1995
 *-------------------------------------------------------------------*/
 KpBool_t KpFileFind(KpfileDirEntry_t  FAR *fileSearch,
                void              FAR *user_ptr,
                KpfileCallBack_t       user_fcn)
{
   bool         returnStatus = KPTRUE;
   char         PathName[256];
   int          Result;
   KpBool_t     fileFound = KPFALSE;
   dirent_t    *dpb;
   DIR         *FD;
   struct stat  FBuf;
   KpUInt32_t   wanted, dontWant;
   /* addresses passed to tell if Directory or Not */
   KpBool_t     Directory = KPTRUE;
   KpBool_t     NotDirectory = KPFALSE;
 
   if ((fileSearch == NULL)                                 ||
       (fileSearch->structSize != sizeof(KpfileDirEntry_t)) ||
       (user_fcn   == NULL)){
      return KPFALSE;
     }
 
   dontWant       = fileSearch->nwAttr & ATTR_MASK;
   wanted         = fileSearch->wAttr  & ATTR_MASK;
 
   strcpy(fileSearch->osfPrivate.base_Dir, fileSearch->fileName);
   if ((FD = opendir(fileSearch->fileName)) != NULL)
   {
      fileFound = KPTRUE;
      if(fileSearch->subDirSearch == KPFALSE) {
      	fileSearch->opStatus = IOFILE_STARTED;
      	fileFound = (user_fcn)(fileSearch, user_ptr);
      	fileSearch->opStatus = IOFILE_FIRST;
      }
      else
      	fileSearch->opStatus = IOFILE_NEXT;
      	
      while ( fileFound == KPTRUE)
      {
         if ((dpb = readdir(FD)) == (dirent_t *)NULL)
         {
         	if (fileSearch->subDirSearch == KPFALSE) {
           	   fileSearch->opStatus = IOFILE_FINISHED;
           	   (user_fcn)(fileSearch, user_ptr);
           	}
            fileFound = KPFALSE;
         } else
         {
            /* Do Not use preference files */
            if (dpb->d_name[0] != '.')
            {
               /* Create full path of file */
               strcpy(PathName, fileSearch->osfPrivate.base_Dir);
               strcat(PathName, "/");
               strcat(PathName, dpb->d_name);
               /* Check if File or sub-directory */
               Result = stat(PathName, &FBuf);
               if (S_ISREG(FBuf.st_mode))             /* is a file */
               {
                  if (!(wanted & ATTR_DIRECTORY))     /* SubDir only */
                  {
                     strcpy(fileSearch->fileName, PathName);
                     fileSearch->osfPrivate.userPrivate = (void *)&NotDirectory;
                     fileFound = (user_fcn)(fileSearch, user_ptr);
                     fileSearch->opStatus = IOFILE_NEXT;
					 if (!fileFound && fileSearch->subDirSearch == KPFALSE) {
           				fileSearch->opStatus = IOFILE_FINISHED;
						(user_fcn) (fileSearch, user_ptr);
					 }
                  }
               } else                                 /* Is a Subdir */
               {
                  if (!(dontWant & ATTR_DIRECTORY))  /* Not No SubDirs */
                  {
                     strcpy(fileSearch->fileName, dpb->d_name);
                     fileSearch->osfPrivate.userPrivate = (void *)&Directory;
                     fileFound = (user_fcn)(fileSearch, user_ptr);
                     fileSearch->opStatus = IOFILE_NEXT;
					 if (!fileFound && fileSearch->subDirSearch == KPFALSE) {
           				fileSearch->opStatus = IOFILE_FINISHED;
						(user_fcn) (fileSearch, user_ptr);
					 }
                  }
               }
            }
         }
      }
      closedir(FD);
   }
   return returnStatus;
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileDirCount (Unix Version)
 *
 * DESCRIPTION
 * This function returns the number of files in a directory
 *
 * RETURN
 * A return int of DIR_OK means it worked OK.
 * A return int of DIR_NO_FILE means an error occured.
 *
 * AUTHOR
 * Dave Oro
 *
 * DATE CREATED
 * October 31, 1995
--------------------------------------------------------------------*/
dirStatus FAR KpFileDirCount (
         char           FAR *dirName,
         KpFileProps_t  FAR *props,
         KpInt32_t      FAR *numFiles)
{
DIR         *FD;
dirent_t    *dpb;
int          Count, Result;
struct stat  DBuf, FBuf;
char         PathName[256];

   if (props) {}		/* avoid compiler warning */
   
   Count = 0;
   *numFiles = 0;
 
   Result = stat(dirName, &DBuf);

   if (S_ISDIR(DBuf.st_mode))
   {
      if ((FD = opendir(dirName)) != NULL)
      {
         while ((dpb = readdir(FD)) != NULL)
         {
            strcpy(PathName, dirName);
            strcat(PathName, "/");
            strcat(PathName, dpb->d_name);
            Result = stat(PathName, &FBuf);
            if (S_ISREG(FBuf.st_mode))
            {
               if (dpb->d_name[0] != '.')
                  Count++;
            }
         }
         closedir(FD);
      }
      *numFiles = Count;
      return DIR_OK;
   }
   return DIR_NO_FILE;
}



#elif defined(KPMAC) || defined (KPMSMAC)
/******************************************

		*** MACINTOSH SECTION ***

*******************************************/
#if defined (KPMSMAC)
#include <macname1.h>
#endif
#if !defined(KPTHINK)
#include <Memory.h>
#include <String.h>
#endif
#if !defined(KPTHINK) && !defined(KPMSMAC)
#include <Strings.h>
#endif

#include <Files.h>
#include <Errors.h>
#include <Folders.h>
#include <ToolUtils.h>
#include <string.h>
#if defined (KPMSMAC)
#include <macname2.h>
#endif
//#define lstrcat strcat
//#define lstrcpy strcpy
//#define lstrlen strlen

#define MAX_FILENAME_LENGTH 255		/* Maximum length of a path name */
#define	BIT4				(1<<4)	/* for testing bit number 4 on */

static void pstrinsert(char_p dst, StringPtr src);
static void pstrcat (StringPtr dst, StringPtr src);

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpGetBlessed
 *
 * DESCRIPTION
 * Get the location of the currently blessed System Folder
 * If result = noErr, then foundVRefNum and foundDirID
 * contain the location of the system folder. This code works
 * under both System 6 and System 7 because the glue code in MPW
 * and other up-to-date development environments implements
 * FindFolder even it FindFolder isn't available.
 *
 * Notice that this function returns the real vRefNum and real dirID of the System
 * Folder instead of a working directory number to it. Those numbers will not
 * change unless the current System Folder is unblessed and a new folder is
 * blessed to make it the System Folder, or unless the System is rebooted with a
 * new startup disk. If you have code that depends on getting passed a working
 * directory number for the System Folder, then you'll need to use the OpenWD
 * function to open a working directory yourself. Like this:
 *  
 * If you really must have a working directory number instead of the
 * real vRefNum and dirID. (use 'ERIK' for procID for System 6 compatibility)
 *  
 * Note that working directories opened by an application are closed when that
 * application quits, so you shouldn't be passing working directory numbers
 * between applications.
 * 
 * AUTHOR
 * Mark Micalizzi, from DEVSUPPORT
 *
 * DATE CREATED
 * August 25, 1994
 *
 *
--------------------------------------------------------------------*/
int KpGetBlessed (short	*foundVRefNum, long	*foundDirID)
{
	OSErr	errnum;				/* error return value */

	errnum = FindFolder(kOnSystemDisk, kSystemFolderType, kDontCreateFolder,
                        foundVRefNum, foundDirID);

	if (errnum == noErr) {
		return KCMS_IO_SUCCESS;
	}
	else
		return KCMS_IO_ERROR;
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileOpen (Macintosh version)
 *
 * DESCRIPTION
 * This function opens a file for either reading or writing.
 *
 * AUTHOR
 * Peter Tracy(originally part of FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 *
--------------------------------------------------------------------*/
int KpFileOpen(	char		*fileName,
		char		*mode,
		KpFileProps_t	*fileProps,
		KpFileId	*fdp)
{
	OSErr      errCode;
	short      refNum, vRefNum;
	long       dirID;
	Str255     pFile;
	OSType     kFileType;
	OSType     kCreator;
	SignedByte	permission;

	union {               /* union to convert types */
		OSType          type;
		char            chars[4];
	}  data;

	data.chars[0] = fileProps->fileType[0];
	data.chars[1] = fileProps->fileType[1];
	data.chars[2] = fileProps->fileType[2];
	data.chars[3] = fileProps->fileType[3];
	kFileType = data.type;

	data.chars[0] = fileProps->creatorType[0];
	data.chars[1] = fileProps->creatorType[1];
	data.chars[2] = fileProps->creatorType[2];
	data.chars[3] = fileProps->creatorType[3];
	kCreator = data.type;

	vRefNum = fileProps->vRefNum;
	dirID = fileProps->dirID;

	pFile[0] = '\0';
	strcpy((char *)pFile,fileName);
	c2pstr((char *)pFile);

	switch (*mode)
	{
		case 'r':
			permission = fsRdPerm;
			break;
		
		case 'w':
		case 'e':
			errCode = HCreate(vRefNum, dirID, pFile,kCreator,kFileType);
			if (errCode != noErr  &&  errCode != dupFNErr)
			{
				return  KCMS_IO_ERROR;
			}
			permission = fsRdWrShPerm;
			break;
		
		default:
			*fdp = -1;
			return   KCMS_IO_ERROR;
	}
	
	
	errCode = HOpenDF(vRefNum, dirID, pFile, permission, &refNum);
	if (errCode != noErr)
	{
		return  KCMS_IO_ERROR;
	}
	*fdp = (int) refNum;

	return KCMS_IO_SUCCESS;
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileSize (Macintosh version)
 *
 * DESCRIPTION
 * This function returns the size of a file.
 *
 * RETURN
 * A return int of 1 means that size contains the file length.
 * A return int of 0 means an error occured.
 *
 * AUTHOR
 * Bonnie Hill
 *
 * DATE CREATED
 * March 4, 1992
 *
 *
--------------------------------------------------------------------*/
int KpFileSize (
			char		*fileName,
			KpFileProps_t	*fileProps,
			KpInt32_t	*size)
{
	Boolean			async=false;		/* no asynchronous call to ROM */
	OSErr			ioErr;				/* status of ROM call */
	HParamBlockRec	parm;				/* parameters for call to ROM */
	int				status;
	Str255     		pPath;

	pPath[0] = '\0';
	strcpy((char *)pPath,fileName);
	c2pstr((char *)pPath);
	
	/*
	 *   Set up and call the Macintosh ROM
	 */
	 
	parm.fileParam.ioCompletion	= nil;
	parm.fileParam.ioVRefNum	= fileProps->vRefNum;
	parm.fileParam.ioDirID		= fileProps->dirID;
	parm.fileParam.ioNamePtr	= (unsigned char *)pPath;
	parm.fileParam.ioFDirIndex	= -1;
	ioErr = PBHGetFInfo(&parm,async);

	if (ioErr == noErr) {
		*size = parm.fileParam.ioFlLgLen;
		status = KCMS_IO_SUCCESS;
	} else {
		*size = 0;
		status = KCMS_IO_ERROR;
	}
	
	return (status);
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileExists (Macintosh version)
 *
 * DESCRIPTION
 * This function tells if a file exists.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * June 24, 1991
 *
--------------------------------------------------------------------*/
dirStatus	KpFileExists(
				char 		FAR *path, 
				KpFileProps_t	FAR *fileProps, 
				KpBool_t	FAR *doesExist )
{
	Boolean			 async=false;		/* no asynchronous call to ROM */
	char			 pFile[256];
	OSErr			 ioErr;				/* status of ROM call */
	HParamBlockRec	 parm;				/* parameters for call to ROM */

	/* This function only handles strings <= 255 */
	if (strlen(path) > 255)
		return DIR_PARAM_ERROR;
		
	strcpy(pFile,path);
	c2pstr(pFile);

	/*
	 *   Set up and call the Macintosh ROM
	 */
	parm.fileParam.ioCompletion	= nil;
	parm.fileParam.ioVRefNum	= fileProps->vRefNum;
	parm.fileParam.ioDirID		= fileProps->dirID;
	parm.fileParam.ioFDirIndex	= -1;
	parm.fileParam.ioFVersNum	= 0;
	parm.fileParam.ioNamePtr	= (unsigned char *)pFile;
	ioErr = PBHGetFInfo(&parm,async);

	/*
	 *   Return the existence status and
	 *   operation status based on the
	 *   return from PBHGetFInfo()
	 */
	if( ioErr == noErr )
		{
		*doesExist = KPTRUE;
		return   DIR_OK;
		}
	else if( ioErr == fnfErr )
		{
		*doesExist = KPFALSE;
		return   DIR_OK;
		}
	else
		{
		*doesExist = KPFALSE;
		return   DIR_ERROR;
		}
}



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileRename (Macintosh specific)
 *
 * DESCRIPTION
 * This function renames files in the same directory.  oldName and newName
 * can both contain full paths in which case fileProps is ignored.  
 * If fileProps is used to specify the directory, then oldName and newName
 * are just the file names.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * June 24, 1991
 *
--------------------------------------------------------------------*/
int	KpFileRename(
			KpFileProps_t	*fileProps, 
			char		*oldName, 
			char		*newName )
{
	OSErr	 err;
	short	 vRefNum;			/* reference number of new file */
	long	 dirID;				/* directory Id of new file */
	Str255	 pOld;				/* Pascal form of old file name */
	Str255	 pNew;				/* Pascal form of new file name */

	/*
	 *   Get volume reference numbers and
	 *   corresponding portion of file names
	 */
	vRefNum = fileProps->vRefNum;
	dirID   = fileProps->dirID;

	/*
	 *   Get file name portions into
	 *   Pascal strings
	 */
	strcpy((char_p)pOld,oldName);
	c2pstr((char_p)pOld);
	strcpy((char_p)pNew,newName);
	c2pstr((char_p)pNew);

	/*
	 *   Do the actual renaming and return
	 *   the appropriate status value.
	 */
	err = HRename(vRefNum, dirID, pOld, pNew);

	if( err != noErr )
		{
		return KCMS_IO_ERROR;
		}
	else
		{
		return   KCMS_IO_SUCCESS;
		}
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileDelete		(Macintosh Version)
 *
 * DESCRIPTION
 * This function deletes a file given its name.
 *
 * AUTHOR
 * Bonnie Hill
 *
 * DATE CREATED
 * March 5, 1992
 *
 *
--------------------------------------------------------------------*/

int KpFileDelete( char *fileName, KpFileProps_t *fileProps )
{
	OSErr  errCode;
	Str255  pFile;

	pFile[0] = '\0';
	strcpy((char *)pFile,fileName);
	c2pstr((char *)pFile);
	errCode = HDelete(fileProps->vRefNum, fileProps->dirID, pFile);
	if (errCode != noErr) return KCMS_IO_ERROR;
	else return KCMS_IO_SUCCESS;
}



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileClose (Macintosh Version)
 *
 * DESCRIPTION
 * This function closes a file.
 *
 * AUTHOR
 * Peter Tracy(originally part of FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 *
--------------------------------------------------------------------*/
int KpFileClose (KpFileId fd)
{
	OSErr   errCode;
	short   refNum;

	refNum = (short)fd;

	errCode = FSClose(refNum);

	return ( (errCode==noErr)?  KCMS_IO_SUCCESS : KCMS_IO_ERROR );
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileRead (Macintosh Version)
 *
 * DESCRIPTION
 * This function reads from the file.  It returns KCMS_IO_SUCCESS
 * if any data is read.  nbytes is updated to reflect the actual  
 * number of bytes read into the buffer.
 *
 * AUTHOR
 * Peter Tracy(derived from FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 *
--------------------------------------------------------------------*/

int KpFileRead (KpFileId fd, KpLargeBuffer_t buf, KpInt32_t *nbytes)
{
	OSErr errCode;
	short refNum;
	long count;


	count = *nbytes;
	refNum = (short) fd;

	errCode = FSRead(refNum,&count,buf);
	if (errCode != noErr  &&  errCode != eofErr )
	{
		return(KCMS_IO_ERROR);
	}

	*nbytes = ( (count <= 0 )?  0 : count);

	return(KCMS_IO_SUCCESS);
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileWrite (Macintosh Version)
 *
 * DESCRIPTION
 * This function writes to the file.
 *
 * AUTHOR
 * Peter Tracy(derived from FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 *
--------------------------------------------------------------------*/

int KpFileWrite (KpFileId fd, KpLargeBuffer_t buf, KpInt32_t nbytes)
{
	OSErr errCode;
	short refNum;
	long count;

	count = nbytes;
	refNum = (short) fd;

	errCode = FSWrite(refNum,&count,buf);
	if (errCode != noErr)
		return(KCMS_IO_ERROR);
	else if (count != nbytes)
		return(KCMS_IO_ERROR);

	return(KCMS_IO_SUCCESS);
}



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFilePosition (Macintosh MPW Version)
 *
 * DESCRIPTION
 * This function positions the file.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * June 13, 1991
 *
--------------------------------------------------------------------*/
int KpFilePosition (
					KpFileId fd, 
					KpFileStart start, 
					KpInt32_t position)
{
	short   posMode;       /* where to start seek from */
	short   refNum;
	OSErr   status;

	/*
	 *  Set up orgin of seek depending
	 *  on input parameter
	 */
	if( start == FROM_START )
		posMode = fsFromStart;
	else if( start == FROM_END )
		posMode = fsFromLEOF;
	else
		posMode = fsFromMark;

	/*
	 *  Now actually do the seek and check the status
	 */
	refNum = (short)fd;
	status = SetFPos( refNum, posMode, position );

	return   ( (status == noErr)?  KCMS_IO_SUCCESS : KCMS_IO_ERROR );
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileTell (Macintosh Version)
 *
 * DESCRIPTION
 * This function returns the current byte position of the file.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Matthew L. Benati
 *
 * DATE CREATED
 * October 27, 1994
 *
--------------------------------------------------------------------*/
int FAR KpFileTell (
			KpFileId	fd,
			KpInt32_t	FAR *position)
{
	int		status;
	long		fPosition;
	OSErr	error;

	error = GetFPos(fd, &fPosition);
	if (error == noErr)
	{
		status = KCMS_IO_SUCCESS;
		*position = fPosition;
	}
	else
	{
		status = KCMS_IO_ERROR;
		*position = 0;
	}

	return   status;
}

/*--------------------------------------------------------------------
 * FUNCTION
 *	KpFileStripPath (Macintosh Version)
 *
 * DESCRIPTION 
 * This function strips off the path of a file. 
 *
 * RETURN VALUE
 *  None.
 * 
 * AUTHOR
 * sek
 *
 * DATE CREATED
 * April 30, 1995
 *-------------------------------------------------------------------*/

void KpFileStripPath (
						KpChar_p filePlusPath, 
						KpChar_p theFile)
{
	char *fPtr;
	KpInt16_t i;

	/* clear theFile buffer */
	theFile[0] = '\0';

	/* find the last occurrence of the : */
	fPtr = strrchr (filePlusPath, ':');
	if (fPtr == NULL) {
		/* no path, so just copy the file name */
		fPtr = &filePlusPath[0];
	}
	else
		*fPtr++;

	/* get the file name */
	i = 0;
	while (*fPtr != '\0')
	{
		theFile[i++] = *fPtr++;
	}
	theFile[i] = '\0';
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileFind(Macintosh specific(MPW))
 *
 * DESCRIPTION
 * This function finds files in a directory based on a specified 
 * search criteria. For each file matching the criteria, it calls 
 * back the user supplied function with the file information block 
 * satisfying the criteria and a user supplied void type pointer.
 *
 * The user supplied function must return KPTRUE or KPFALSE depending
 * whether the function wishes the search to continue or not.
 * The function will be called at least once!
 * A status field in the file structure block pass indicates the
 * status of the call.
 *
 * opStatus = IOFILE_STARTED    - inital call, No file yet,
 *                                just for initialization.
 * opStatus = IOFILE_FIRST      - first file found.
 * opStatus = IOFILE_NEXT       - Ongoing subsequent file found.
 * opStatus = IOFILE_ERROR      - Search or access error.
                                  Includes first file not found.
 * opStatus = IOFILE_FINISHED   - No more files found. User
 *                                should clean up whatever.
 *
 * osfPrivate structure in the iofileDirEntry_t structure:
 *      typedef struct osfPrivate_tbl   {
 *                              ## Mac volume reference number ##
 *              KpUInt16_t      vRefNum;
 *                              ## Mac File Type ##
 *              KpUInt32_t      fileType;
 *                              ## Mac Creator ##
 *              KpUInt32_t      fileCreator;
 *                              ## Mac Directory ID ##
 *              KpUInt32_t      dirID;
 *                              ## Mac parameters for call to ROM ##
 *              CInfoPBRec      parm;
 *  } osfPrivate_t;
 *
 * The search criteria is somewhat OS implementation dependent.
 * Here, the fileType and/or fileCreator may be set to desired
 * values (i.e. 'PT  ' and 'KEPS') or to zero for don't care.
 *
 * In order to provide for subdirectory searches, the dirID
 * field must be set to zero when originally called. It is used
 * to pass the dirID of the subdirectory by the callback if
 * recursive searches are desired.
 *
 * The wAttr and nwAttr fields in the iofileDirEntry_t struct
 * should be set as follows:
 *
 *      wAttr = 0, nwAttr = ATTR_DIRECTORY  --->
 *                  for passing files, that match type
 *                  and/or creator from the current folder,
 *                  to the callback.
 *
 *      wAttr = ATTR_DIRECTORY, nwAttr = 0  --->
 *                  for passing only folders in the current
 *                  folder and subfolders to the callback
 *                  (no files). [recursive]
 *
 *      wAttr = 0, nwAttr = 0               --->
 *                  for passing both files, that match type
 *                  and/or creator, and folders to the
 *                  callback.  [recursive]
 *
 * If nwAttr = 0, all searches will be recursive, and the
 * callback should expect to receive folders and set the dirID
 * field properly.
 *
 * The return is somewhat nebulous. It returns true if a search
 * went to completion.
 * It returns false if it didn't. The user function should
 * probably setup in the user supplied pointer, a variable to
 * indicate whatever status information it wishes to pass back
 * from the iofileFind call.
 *
 * Recursive callback code sample:
 *
 *      callback(KpfileDirEntry_t FAR * fileSearch,
 *               void             FAR * user_ptr)
 *{
 *      ...
 *      ## handle to allocated recursive file search struct ##
 *      KcmHandle               hRecursive;
 *      KpfileDirEntry_t       *recursiveFileSearch;
 *                                ## for accessing dir parms##
 *      DirInfo                *dParm =
 *         (DirInfo *)&fileSearch->osfPrivate.parm;
 *                               ## for accessing file parms##
 *      HFileInfo              *fParm =
 *         (HFileInfo *)&fileSearch->osfPrivate.parm;
 * ...
 *  case IOFILE_NEXT :
 *
 *     if (fParm->ioFlAttrib & BIT4)  ##it's a directory,
 *                                      lets traverse it##
 *     {
 *        ##do a recursive search down subdirectories
 *         (must allocate a new KpfileDirEntry_t struct ##
 *
 *        if( (hRecursive=allocBufferHandle(
 *                sizeof(KpfileDirEntry_t))) == (KcmHandle)NULL )
 *        {
 *           fileSearch->opStatus = IOFILE_ERROR;
 *           return   KPFALSE;
 *        }
 *
 *        if( (recursiveFileSearch=
 *            (KpfileDirEntry_t *)lockBuffer(hRecursive))
 *            == (KpfileDirEntry_t *)NULL )
 *        {
 *           freeBuffer(hRecursive);
 *           fileSearch->opStatus = IOFILE_ERROR;
 *           return   KPFALSE;
 *        }
 *        ## copy search parameters from original ##
 *        *recursiveFileSearch = *fileSearch;
 *        ##setting subdir ID ##
 *        recursiveFileSearch->osfPrivate.dirID = dParm->ioDrDirID;
 *        ##tell search this is recursive call ##
 *        recursiveFileSearch->subDirSearch = KPTRUE;  
 *        ## search subdir ##
 *        KpFileFind(recursiveFileSearch, user_ptr, callback );
 *        ##free up recursive search buffer after search##
 *        freeBuffer(hRecursive);
 *	}
 *
 * adapted from Windows version by
 * Stan Pulchtopek 
 *
 * DATE CREATED
 * July 1, 1994
 * Modified Kp version Nov 8, 1995.  DaveOro/ Anne Rourke
 *
--------------------------------------------------------------------*/
KpBool_t KpFileFind( KpfileDirEntry_t FAR * fileSearch,
					 void FAR * user_ptr,
					 KpfileCallBack_t user_fcn )
{
	Boolean			async=false;		/* no asynchronous call to ROM */
	int				fileNum=1;			/* next file number to find */
	OSErr			ioErr;				/* status of ROM call */
  	char        	*file;
	KpBool_t		fileFound = KPFALSE;
	KpBool_t		fileTypeSet, fileCreatorSet;
	CInfoPBRec		parm;				/* parameters for call to ROM */
	HFileInfo		*fParm = (HFileInfo *)&parm;
	DirInfo			*dParm = (DirInfo *)&parm;
	KpUInt32_t		wanted, dontWant;
	KpBool_t        returnStatus = KPTRUE;
	KpFileProps_t	props;
	KpFileProps_t	newProps;
	
   if ((fileSearch == NULL)                                 ||
       (fileSearch->structSize != sizeof(KpfileDirEntry_t)) ||
       (user_fcn   == NULL))
		return KPFALSE;

	dontWant       = fileSearch->nwAttr & ATTR_MASK;
	wanted         = fileSearch->wAttr & ATTR_MASK;
	fileTypeSet    = (fileSearch->osfPrivate.fileType != 0) ? 
	                 KPTRUE : KPFALSE;
	fileCreatorSet = (fileSearch->osfPrivate.fileCreator != 0) ? 
	                 KPTRUE : KPFALSE;

	/* convert any directory name into vRefNum and dirID */	
	props.vRefNum = fileSearch->osfPrivate.vRefNum;
	props.dirID = fileSearch->osfPrivate.dirID;
	
	ioErr = KpGetDirRefNum(&props, fileSearch->fileName, 
					strlen (fileSearch->fileName), &newProps);
	if (ioErr == KCMS_IO_SUCCESS) {

		/* check if the specified directory exists */
		file = fileSearch->fileName;
  	  	file[0] = '\0';
   	 	c2pstr(file);

		fParm->ioCompletion = nil;
		fParm->ioVRefNum    = newProps.vRefNum;
		fParm->ioDirID   = newProps.dirID;
		fParm->ioFDirIndex  = -1;
		fParm->ioNamePtr    = (StringPtr)file;
		ioErr = PBGetCatInfo(&parm,async);
		if (( ioErr == noErr ) &&
		   	 ( fParm->ioFlAttrib & ATTR_DIRECTORY ))
		{
			fileFound = KPTRUE;
			fileSearch->osfPrivate.vRefNum = fParm->ioVRefNum;
			fileSearch->osfPrivate.dirID   = fParm->ioDirID;
	
			if (fileSearch->subDirSearch == KPFALSE)
			{	/* First Entry into this routine - indicate start to the user */
				fileSearch->opStatus = IOFILE_STARTED;
				fileFound = (user_fcn)(fileSearch, user_ptr);
				if (!fileFound) {
					returnStatus = KPFALSE;
				}
				fileSearch->opStatus = IOFILE_FIRST;
			}
			else {
				/* This is a recursive entry into this routine */
				fileSearch->opStatus = IOFILE_NEXT;
			}
		}

	}
	else	/* directory not found */
		returnStatus = KPFALSE;
		

	/*  Loop until all files are processed   */
	while (fileFound == KPTRUE)
	{
  	  	file = fileSearch->fileName;
  	  	file[0] = '\0';
   	 	c2pstr(file);

		fParm->ioCompletion = nil;
		fParm->ioVRefNum    = fileSearch->osfPrivate.vRefNum;
		fParm->ioDirID   = fileSearch->osfPrivate.dirID;
		fParm->ioFDirIndex  = fileNum++;
		fParm->ioNamePtr    = (StringPtr)file;
		ioErr = PBGetCatInfo(&parm,async);
		if( ioErr == noErr )
		{
			/* file was found */
			if( fParm->ioFlAttrib & ATTR_DIRECTORY )	/* it's a folder */
			{
				if (dontWant & ATTR_DIRECTORY)
					continue;		/* skip over subdirectories */
				else				  /* Want Folders */
				{
					p2cstr((StringPtr)file);
					fParm->ioNamePtr    = nil;
					fileSearch->osfPrivate.parm = parm;
					fileFound = (user_fcn)(fileSearch, user_ptr);
					fileSearch->opStatus = IOFILE_NEXT;	/* for next call if another file is found */
					if (!fileFound && fileSearch->subDirSearch == KPFALSE) {
           				fileSearch->opStatus = IOFILE_FINISHED;
						(user_fcn) (fileSearch, user_ptr);
					}
				}
					
			}
			else										/* it's a file */
			{
				if (!(wanted & ATTR_DIRECTORY))
				{
					if ( (!fileTypeSet     ||
						  fParm->ioFlFndrInfo.fdType ==
						  fileSearch->osfPrivate.fileType) &&
						 (!fileCreatorSet  ||
						  fParm->ioFlFndrInfo.fdCreator ==
						  fileSearch->osfPrivate.fileCreator) )
					{
						p2cstr((StringPtr)file);
						fParm->ioNamePtr    = nil;
						fileSearch->osfPrivate.parm = parm;
						fileFound = (user_fcn)(fileSearch, user_ptr);
						fileSearch->opStatus = IOFILE_NEXT;	/* for next call if another file is found */
						if (!fileFound && fileSearch->subDirSearch == KPFALSE) {
           					fileSearch->opStatus = IOFILE_FINISHED;
							(user_fcn) (fileSearch, user_ptr);
						}
					}
				}
			}
		}
		else if (ioErr == fnfErr)
		{
			/* no more entries in the directory */
			if (fileSearch->subDirSearch == KPFALSE)			 /* at top of recursive loop */
			{
				fileSearch->opStatus = IOFILE_FINISHED;
				fileFound = (user_fcn)(fileSearch, user_ptr);	/* allow for successful cleanup */
			}
			fileFound = KPFALSE; 								/* to get out of loop */
		}
		else
		{
			 /* Bad Status we did not expect */
			fileSearch->opStatus = IOFILE_ERROR;
			fileFound = (user_fcn)(fileSearch, user_ptr);	/* allow for error cleanup */
			fileFound = KPFALSE;							/* to get out of loop */
			returnStatus = KPFALSE;
		}
	} /* end of while loop */

	fileSearch->fileName[0] = '\0';
	return (returnStatus);

}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpGetVolAndName(Macintosh specific(MPW))
 *
 * DESCRIPTION
 *
 * Given a directory or file, this function returns a volume reference 
 * number, directory ID and file name which meets the macintosh
 * restriction of < 255 characters in a file name.  
 * If a directory is passed to this function, it should end in a colon.
 * Otherwise, the resulting newProps will be for the parent directory
 * of the one specified.  This routine sets the file parameter to the start 
 * of the file name in fileName.  newProps contains the vRefNum and 
 * dirID of the directory containing file.
 *
 *  The legal inputs to this routine are:
 *    fileName = full path name
 *    fileName = partial path name, oldProps->vRefNum = working dir
 *    fileName = partial path name, oldProps->vRefNum = vRefnum,
 *                                  oldProps->dirID = dirID,
 *    fileName = file name, oldProps->vRefNum = working dir
 *    fileName = file name, oldProps->vRefNum = vRefnum,
 *                                  oldProps->dirID = dirID,
 * 
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * May 28, 1991
 * Modified for Kpfunction 11/2/95.  ACR
 *
--------------------------------------------------------------------*/

int KpGetVolAndName( 
			char		*fileName, 
			KpFileProps_t	*oldProps, 
			char		**file,
			KpFileProps_t	*newProps )
{
	int		 fileErr;
	char	 *nextColon;			/* pointer to next colon in file name */
	char     *startChr;			/* start of partial path name */
	KpInt32_t	dirLength;



	/* Call KpGetDirRefNum with just the directory part of the string.
	 * This will return in newProps the vRefNum and dirID of this directory.
	 */
	startChr = fileName;
	nextColon = strchr(fileName,':');
	
	/* find the last colon in the string */
	while( nextColon != (char*)NULL )
	{
		startChr = nextColon;
		nextColon = strchr(startChr+1,':');
	}
	
	/* convert the directory name to KpFileProps_t
	 * include the trailing colon to include the last directory 
	 */
	 dirLength = startChr - fileName +1;
	 if (startChr == fileName)
	 	dirLength = 0;
	 
	fileErr = KpGetDirRefNum(oldProps, fileName,(int)(dirLength), newProps);
	if( fileErr == KCMS_IO_ERROR )
		return   fileErr;

	/* return file pointing to the first char after the last colon */
	if (dirLength == 0)
		*file = startChr;			
	else
		*file = startChr+1;			

	
	return   KCMS_IO_SUCCESS;
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpGetDirRefNum(Macintosh specific(MPW))
 *
 * DESCRIPTION
 * This function is passed a reference to a directory and returns
 * the KpFileProps_t (vRefNum and dirID) for that directory.
 * The directory reference may be specified in one of the following 
 * manners:
 *    rPath = full path name
 *    rPath = partial path name, props->vRefNum = working dir
 *    rPath = partial path name, props->vRefNum = vRefnum,
 *                               props->dirID = dirID,
 *
 *
 * If rPath specifies a directory, the directory must end in a colon.
 * If rPath specifies a partial directory, the directory must start 
 * with a colon.
 *
 * The length allows the caller to point to any ':' in the string
 * and the vRefNum and dirId of the path up to that point is returned.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * May 28, 1991
 * Modified for Kpfunction 11/2/95.  ACR
--------------------------------------------------------------------*/

int KpGetDirRefNum(KpFileProps_t	*props, 
			char		*rPath, 
			int		length , 
			KpFileProps_t	*newProps)
{
	Boolean			 async=false;	/* don't do I/O asynchronously */
	CInfoPBRec		 catParms;		/* parameters for PBGetCatInfo */
	HParamBlockRec	 vParm;		/* parameters for PBHGetVInfo */
	OSErr			 err;			/* status of file ROM call */
	Str255			 pFile;			/* Pascal version of file name */
	char	         *startChr;			/* start of partial path name */
	char	         *lastChr;			/* end of partial path name */
	char	         *startSearchChr;	/* address to start looking for colon */
	char	         *nextColon;			/* pointer to next colon in file name */

		
	/* Set myProps to props values to start */
	strcpy(newProps->fileType, props->fileType);
	strcpy(newProps->creatorType, props->creatorType);
	newProps->vRefNum = props->vRefNum;
	newProps->dirID = props->dirID;

	if (length == 0)  
		return  KCMS_IO_SUCCESS;
	
	/* Check if this is a partial path or full path */
	/* Starting with ':' indicates a partial path */
	/* Otherwise, we have to check if it is */
	if (rPath[0] != ':')  {
		/*
		 *   Get the drive name including the training colon
		 */
		pFile[0] = '\0';
		strncpy((char *)pFile,rPath,length);
		pFile[length] = '\0';
		nextColon = strchr((char_p)pFile,':');
		if (nextColon != 0) {
		   if ((nextColon - (char_p)pFile) > length)
			return KCMS_IO_ERROR;	
		   pFile[(int)(nextColon-(char_p)pFile + 1)] = '\0';
		   c2pstr((char *)pFile);


		   vParm.volumeParam.ioCompletion = nil;
		   vParm.volumeParam.ioVRefNum	= -32768;
		   vParm.volumeParam.ioVolIndex	= -1;
		   vParm.volumeParam.ioNamePtr	= pFile;
		   err = PBHGetVInfo(&vParm, async);
		   if (err == noErr) {
			/* the drive exists, this is a full path name */
			newProps->vRefNum = vParm.volumeParam.ioVRefNum;
			newProps->dirID = 0;
		   }
		}
	}
	/*
	 *   Get the file name
	 *   into a Pascal string
	 */
	pFile[0] = '\0';
	strncpy((char *)pFile,rPath,length);
	pFile[length] = '\0';
	c2pstr((char *)pFile);
	
	/*
	 *   If the file name length is less than
	 *   the maximum, can convert in one call.
	 */
	 
	if( length <= MAX_FILENAME_LENGTH )
	{
		/*
		 *   Set up and make the call to the ROM to
	 	 *   get information about this directory
	 	 */
		catParms.hFileInfo.ioCompletion	= nil;
		catParms.hFileInfo.ioFDirIndex	= 0;
		catParms.hFileInfo.ioVRefNum	= newProps->vRefNum;
		catParms.hFileInfo.ioNamePtr	= pFile;
		catParms.hFileInfo.ioDirID	= newProps->dirID;
		if( (err=PBGetCatInfo(&catParms,async)) != noErr )
		{
			return KCMS_IO_ERROR;
		}
	} 
	else {

	/*
	 *   The search is done by finding each subdirectory
	 *   specification(by looking for colons) and getting
	 *   a volume reference number for each one until the
	 *   file name part is found(no more colons)
	 *
	 *   Start by looking for the first colon in the file
	 *   not counting the first character.
	 */

		lastChr = rPath + length;
		
		startChr = rPath;
		startSearchChr = (rPath[0]==':')?  rPath+1:rPath;
		nextColon = strchr(startSearchChr,':');

		if (nextColon > lastChr)
			return KCMS_IO_ERROR;
			
		if( rPath[0] != ':' )
		{				/* full path name */
			nextColon = strchr(nextColon+1,':');
			if (nextColon > lastChr)
				nextColon = strchr(startSearchChr,':');
		}

		catParms.hFileInfo.ioCompletion	= nil;
		catParms.hFileInfo.ioFDirIndex	= 0;
		catParms.hFileInfo.ioVRefNum	= newProps->vRefNum;
		catParms.hFileInfo.ioDirID	= newProps->dirID;
		
		while( (nextColon != (char*)NULL) && (nextColon <= lastChr) )
		{
			/*
			 *   Set up and make the call to the ROM to
			 *   get information about this directory
			 */
			strncpy((char *)pFile,startChr,(int)(nextColon-startChr+1));
			pFile[(int)(nextColon-startChr)] = '\0';
			c2pstr((char *)pFile);
			
			catParms.hFileInfo.ioNamePtr	= pFile;
			if( (err=PBGetCatInfo(&catParms,async)) != noErr )
			{
				return KCMS_IO_ERROR;
			}

			/*
			 *   Look for next colon
		 	*/
			startChr = nextColon;
			nextColon = strchr(startChr+1,':');
		}
	}

	newProps->vRefNum = catParms.hFileInfo.ioVRefNum;
	newProps->dirID = catParms.hFileInfo.ioDirID;
	
	return  KCMS_IO_SUCCESS;
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpPathNameFromDirID (Macintosh specific)
 *
 * DESCRIPTION
 * This function returns a full path name given a volume reference 
 * number and a dirID.
 * This function can handle any size string but it will return an
 * error and a null full path name if the full path name string
 * is greater that the bufSize.
 *
 * AUTHOR
 * taken from Think Reference
 *
 * DATE CREATED
 * Moved to kcms_sys library 11/6/95.  ACR
--------------------------------------------------------------------*/

int KpPathNameFromDirID(
						short		vRefNum,
						long		dirID, 
						char_p		fullPathName,
						KpUInt32_t	bufSize)
{
	CInfoPBRec		block;
	Str255		directoryName;
	OSErr			err;
	
	fullPathName[0] = '\0';
	block.dirInfo.ioDrParID = dirID;
	block.dirInfo.ioNamePtr = directoryName;
	
	do {
		block.dirInfo.ioVRefNum = vRefNum;
		block.dirInfo.ioFDirIndex = -1;			/* ignore current name - use ioDrDirID */
		block.dirInfo.ioDrDirID = block.dirInfo.ioDrParID;
		err = PBGetCatInfo(&block, KPFALSE);
		pstrcat(directoryName, (StringPtr)"\p:"); /* add colon */
		if ((strlen(fullPathName) + directoryName[0]) > bufSize)
		{
			/* string too long */
			fullPathName[0] = '\0';
			return KCMS_IO_ERROR;
		}
		pstrinsert(fullPathName, directoryName); /* insert into return string */
	} while (block.dirInfo.ioDrDirID != 2);
	
	return KCMS_IO_SUCCESS;
}

static void pstrcat (StringPtr dst, StringPtr src)
{
	/* copy string in */
	BlockMove(src + 1, dst + *dst + 1, *src);
	/* adjust length byte */
	*dst += *src;
}

static void pstrinsert(char_p dst, StringPtr src)
{
	/* make room for new string */
	BlockMove(dst , dst + *src, strlen(dst)+1);
	/* copy new string in */
	BlockMove(src + 1, dst, *src);
}

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileDirCount (MAC Version)
 *
 * DESCRIPTION
 * This function returns the number of files in a folder
 *
 * RETURN
 * A return int of DIR_OK means it worked OK.
 * A return int of DIR_NO_FILE means an error occured.
 *
 * AUTHOR
 * Dave Oro
 *
 * DATE CREATED
 * October 31, 1995
--------------------------------------------------------------------*/
dirStatus FAR KpFileDirCount (
         char           FAR *dirName,
         KpFileProps_t  FAR *props,
         KpInt32_t      FAR *numFiles)
{

int		Count, fileNum;
int		fileFound = 1;
CInfoPBRec	cipbr;
HFileInfo	*fpb   = (HFileInfo *) &cipbr;
DirInfo		*dpb   = (DirInfo *)   &cipbr;
Boolean		async=false;
OSErr		theErr;
long		theDirID;
short		theVRefNum;
char		*file;
char		Direct[MAX_PATH];

   Count = 0;
   *numFiles = Count;
 
   theVRefNum        = props->vRefNum;
   theDirID          = props->dirID;


   if (dirName != NULL)
   {
      strcpy(Direct, dirName);
      file              = Direct;
      c2pstr(file);
      fpb->ioNamePtr    = (StringPtr)file;
      fpb->ioCompletion = NULL;
      fpb->ioVRefNum    = props->vRefNum;
      fpb->ioDirID      = props->dirID;
      fpb->ioFDirIndex  = 0;

      theErr = PBGetCatInfo(&cipbr, async);
      if (theErr == noErr)   /* Need file name string */
      {
         theDirID   = dpb->ioDrDirID;	/* dirID of dirID + dirName */
         theVRefNum = dpb->ioVRefNum;	/* VRefNum of VRefNum + dirName */
      }
   }

   fpb->ioNamePtr    = (StringPtr)NULL;
   fpb->ioCompletion = NULL;
   fpb->ioVRefNum    = theVRefNum;
   fpb->ioDirID      = theDirID;
   fpb->ioFDirIndex  = 0;

   theErr = PBGetCatInfo(&cipbr, async);
 
   if (theErr == noErr)
   {
      if (dpb->ioFlAttrib & 16)
      {
         fileNum = 1;
         while (fileFound)
         {
            fpb->ioDirID       = theDirID;
            fpb->ioVRefNum     = theVRefNum;
            fpb->ioCompletion  = NULL;
            fpb->ioFDirIndex   = fileNum++;
            fpb->ioNamePtr     = (StringPtr)NULL;
            theErr = PBGetCatInfo(&cipbr, async);
            if (theErr == noErr)
            {
               if (!(dpb->ioFlAttrib & ATTR_DIRECTORY))
                  Count++;
             } else if (theErr == fnfErr)
                fileFound = 0;
         }
         *numFiles = Count;
         return DIR_OK;
      }
   }
   return DIR_NO_FILE;
}


#elif defined(KPWIN32)

/******************************************

		*** MICROSOFT C SECTION ***

*******************************************/
#include <fcntl.h>
#include <io.h>

//#if !defined(KPMSMAC)
#include <conio.h>
#include <dos.h>
//#endif

#include <string.h>
#include <sys\stat.h>

/*
 * What follows is the implementation for DOS.  Since the PC
 * needs special attention to deal with large arrays and/or
 * numbers > 64K, the buffers for reading/writing are declared
 * as unsigned char _huge * and the number of bytes to read/write
 * are declared as long.  To make this work for all memory
 * models, the I/O operations must be broken up into a series
 * of smaller operations by copying into a smaller buffer in
 * the memory model compiled for.
 *
 *          Peter Tracy     25 March 1991(for FUTs) and May 1, 1991
 */


#define MAX_TEMP_BUFFER (30*1024)          /* should be less than 32768 */

/*--------------------------------------------------------------------
 * FUNCTION
 *	KpFileStripPath (MICROSOFT C Version)
 *
 * DESCRIPTION 
 * This function strips off the path of a file. 
 *
 * RETURN VALUE
 *  None.
 * 
 * AUTHOR
 * sek
 *
 * DATE CREATED
 * April 30, 1995
 *-------------------------------------------------------------------*/

void KpFileStripPath (KpChar_p filePlusPath, KpChar_p theFile)
{
	char *fPtr;
	KpInt16_t i;

	/* clear theFile buffer */
	theFile[0] = '\0';

	/* find the last occurrence of the backslash */
	fPtr = strrchr (filePlusPath, '\\');
	if (fPtr == NULL) {
		/* find the occurrence of the ':' */
		fPtr = strrchr (filePlusPath, ':');
		if (fPtr == NULL) {
			/* no path, so just copy the file name */
			fPtr = &filePlusPath[0];
		}
		else
			*fPtr++;
	}
	else
		*fPtr++;

	/* get the file name */
	i = 0;
	while (*fPtr != '\0')
	{
		theFile[i++] = *fPtr++;
	}
	theFile[i] = '\0';
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileRead (MS Windows Version (32 bit))
 *
 * DESCRIPTION
 * This function reads from the file.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy(derived from FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 * Modified
 * Dec 12, 1991 lsh
 *
--------------------------------------------------------------------*/

int FAR KpFileRead (
			KpFileId		fd,
			KpLargeBuffer_t	buf,
			KpInt32_t		FAR *numBytes)
{
	int		ThisRead;		/* number of bytes read in last read */
	UINT	BytesRead;
	UINT	BytesToRead;
	char	*Ptr;

#if 1
	Ptr = buf;
	BytesToRead = (UINT) *numBytes;
	BytesRead = 0;

/* read chunks of data */
	while (BytesToRead > KpWin32ioSize) {
		ThisRead = _lread (fd, Ptr, KpWin32ioSize);
		if (ThisRead < 0)
			return KCMS_IO_ERROR;

		BytesRead += ThisRead;
		if (ThisRead != KpWin32ioSize) {
			*numBytes = BytesRead;
			if (BytesRead == 0)
				return KCMS_IO_ERROR;
			return KCMS_IO_SUCCESS;
		}

		BytesToRead -= ThisRead;
		Ptr += ThisRead;
	}

/* read remaining data */
	if (0 < BytesToRead) {
		ThisRead = _lread (fd, Ptr, BytesToRead);
		if (ThisRead < 0)
			return KCMS_IO_ERROR;

		BytesRead += ThisRead;
		if ((UINT) ThisRead != BytesToRead) {
			*numBytes = BytesRead;
			if (BytesRead == 0)
				return KCMS_IO_ERROR;
			return KCMS_IO_SUCCESS;
		}

		BytesToRead -= ThisRead;
	}

	*numBytes = BytesRead;
	if (BytesRead)
		return KCMS_IO_SUCCESS;
	return KCMS_IO_ERROR;

#else
/* would use this if network reads worked!! */
	ThisRead = _lread (fd, buf, (UINT) *numBytes);
	if (ThisRead != *numBytes) {
		if (ThisRead <= 0) {
			*numBytes = ThisRead;
			return KCMS_IO_ERROR;
		}
	}
	*numBytes = ThisRead;
	return KCMS_IO_SUCCESS;
#endif
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileWrite (MS Windows Version (32 bit))
 *
 * DESCRIPTION
 * This function writes to the file.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy(derived from FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 * Modified
 * Dec 12, 1991 lsh
 *
--------------------------------------------------------------------*/
int FAR KpFileWrite (
			KpFileId		fd,
			KpLargeBuffer_t	buf,
			KpInt32_t		numBytes)
{
	int		ThisWrite;		/* number of bytes written in last write */
	char	*Ptr;

#if 1
	Ptr = buf;

/* write chunks of data */
	while (numBytes > KpWin32ioSize) {
		ThisWrite = _lwrite (fd, Ptr, KpWin32ioSize);
		if (ThisWrite != KpWin32ioSize)
			return KCMS_IO_ERROR;

		numBytes -= ThisWrite;
		Ptr += ThisWrite;
	}

/* write remaining data */
	if (0 < numBytes) {
		ThisWrite = _lwrite (fd, Ptr, (UINT) numBytes);
		if (ThisWrite != numBytes)
			return KCMS_IO_ERROR;

		numBytes -= ThisWrite;
	}

	return 1;
#else
/* would use this if network reads worked!! */
	return ((UINT) numBytes != _lwrite (fd, buf, (UINT) numBytes)) ? KCMS_IO_ERROR : 1;
#endif
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileRename (MS Windows Version (32 bit))
 *
 * DESCRIPTION
 * This function renames a file.
 *
 * RETURN
 * A return int of 1 means it worked OK.
 * A return int of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * Jan 3, 1992
 *
--------------------------------------------------------------------*/
int FAR KpFileRename ( KpFileProps_t	*props,
			char		FAR *oldName,
			char		FAR *newName)
{
	if (props) {}

	return MoveFile (oldName, newName) ? KCMS_IO_SUCCESS : KCMS_IO_ERROR;
}
//#if !defined (KPMSMAC)
/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileFind(Win32 PC Specific)
 *
 * DESCRIPTION
 * This function finds files in a directory based on a specified search
 * criteria. For each file matching the criteria, it calls back the user
 * supplied function with the file information block satisfying the criteria
 * and a user supplied void type pointer.
 *
 * The user supplied function must return KPTRUE or KPFALSE depending whether
 * the function wishes the search to continue or not. The function will be
 * called at least once!
 *
 * A status field in the file structure block pass indicates
 * the status of the call.
 *
 * opStatus = IOFILE_STARTED	- inital call, No file yet,
 *									just for initialization.
 * opStatus = IOFILE_FIRST		- first file found.
 * opStatus = IOFILE_NEXT		- Ongoing subsequent file found.
 * opStatus = IOFILE_ERROR		- Search or access error.
 *									Includes first file not found.
 * opStatus = IOFILE_FINISHED	- No more files found.
 *									User should clean up whatever.
 *
 * The search criteria is somewhat OS implementation dependent. For WIN32
 * the field filter in the osfFilePrivate structure specifies a string
 * used to filter filenames.  The string may contain wild card characters.
 * For instance to allow only profiles to be passed to the callback 
 * function, the string "*.pf" is set in the filter field.  If the filter
 * field is a zero length string, the filter "*.*" is used.  
 *
 * The return is somewhat nebulous.
 *		It returns true if a search went to completion.
 *
 *		It returns false if it didn't. The user function should probably
 *		setup in the user supplied pointer, a variable to indicate whatever
 *		staus information it wishes to pass back from the KpFileFind call.
 *
 * AUTHOR
 * Norton and later Larry.
 *
 * DATE CREATED
 * June 15, 1994
 *
--------------------------------------------------------------------*/
KpBool_t KpFileFind (
			KpfileDirEntry_t	FAR *fileSearch,
			void				FAR *user_ptr,
			KpfileCallBack_t	user_fcn)
{
Kp_FindOS_Data_p pFindData;
HANDLE           hFindFile;
KpBool_t         fileFound = KPTRUE;
size_t           size;
KpBool_t         returnStatus = KPFALSE;
KpBool_t         wantFiles, wantDirs;
KpBool_t		 cbAborted = KPFALSE;

   /* Validate arguments */
   if ((fileSearch == NULL)                                 ||
       (fileSearch->structSize != sizeof(KpfileDirEntry_t)) ||
       (user_fcn   == NULL))
      return returnStatus;

	/* Allocate the find data structure	*/
	pFindData = (Kp_FindOS_Data_p) allocBufferPtr (sizeof(Kp_FindOS_Data));
	if (NULL == pFindData)
		return returnStatus;

	/* Determine what we are looking for */
	if  (0 == fileSearch->wAttr) {
		wantFiles = KPTRUE;
		if ( 0 == fileSearch->nwAttr) {
			wantDirs = KPTRUE;
		}
		else  {
			wantDirs = KPFALSE;
		}
	}
	else {
		wantFiles = KPFALSE;
		wantDirs = KPTRUE;
	}

	/*	It is assumed that a directory name is passed in 
		fileSearch->fileName the first time through.
		
		NOTE: we might want to test that name passed in is in fact a 
		directory before doing this - mjb									*/  

	/* Save the starting directory */
	strcpy(fileSearch->osfPrivate.base_Dir, fileSearch->fileName);

	/*	If a filter string was not supplied, set the filter string to \*.*
		so all of the files and directorys under the specified directory 
		can be found.														*/  
	if (0 == strlen (fileSearch->osfPrivate.filter)) {
		strcpy (fileSearch->osfPrivate.filter, "*.*");
	}
	
	/*	append the filter string to the directory name	*/
	if ((strlen (fileSearch->fileName) + 
		 strlen (fileSearch->osfPrivate.filter)) >= MAX_PATH) {
		freeBufferPtr (pFindData);
		return KPFALSE;
	}
	strcat (fileSearch->fileName, "\\");
	strcat (fileSearch->fileName, fileSearch->osfPrivate.filter);

	/* Find the first file or directory under that directory */
	hFindFile = FindFirstFile (fileSearch->fileName, pFindData);
	if (hFindFile != INVALID_HANDLE_VALUE) {

		/* Set up the opStatus */
		if (fileSearch->subDirSearch == KPFALSE)
		{	/* First Entry into this routine - indicate start to the user */
			fileSearch->opStatus = IOFILE_STARTED;
			fileFound = (user_fcn)(fileSearch, user_ptr);
			if (!fileFound) {
				freeBufferPtr (pFindData);
				return returnStatus;
			}
			fileSearch->opStatus = IOFILE_FIRST;
		}
		else {
			/* This is a recursive entry into this routine */
			fileSearch->opStatus = IOFILE_NEXT;
		}
	
		while (fileFound == KPTRUE) {

			/* A file or directory was found, save its name */
			size = strlen (fileSearch->osfPrivate.base_Dir) +
				   strlen (pFindData->cFileName);
			if (size >= MAX_PATH) {
				freeBufferPtr (pFindData);
				return returnStatus;
			}
			strcpy (fileSearch->fileName, fileSearch->osfPrivate.base_Dir);
			strcat (fileSearch->fileName, "\\"); 
			strcat (fileSearch->fileName, pFindData->cFileName);

			/*	Pass the Find Data down to the call back */
			fileSearch->osfPrivate.pFindData = pFindData;

			/* Was a directory found */
			if (pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

				/* Filter out "." and ".." directory entries	*/
				if (pFindData->cFileName[0] != '.') {
				
					if (wantDirs) {
						fileFound = (user_fcn)(fileSearch, user_ptr);
						if (!fileFound) {
							cbAborted = KPTRUE;
						}
						fileSearch->opStatus = IOFILE_NEXT;
					}
				}
			}

			/* Was a file was found */
			else {
				if (wantFiles) {
					fileFound = (user_fcn)(fileSearch, user_ptr);
					if (!fileFound) {
						cbAborted = KPTRUE;
					}
					fileSearch->opStatus = IOFILE_NEXT;
				}
			}

			/*	Is there another directory or file */
			if (fileFound == KPTRUE) {
				fileFound = (KpBool_t)FindNextFile(hFindFile, pFindData);
			}

		} /* while */ 

		/* all done - if no error only report finished at top level */
		if (cbAborted || GetLastError() == ERROR_NO_MORE_FILES) {
			if  (fileSearch->subDirSearch == KPFALSE) {
				fileSearch->opStatus = IOFILE_FINISHED;
				(user_fcn) (fileSearch, user_ptr);
			}
			returnStatus = KPTRUE;
			fileFound = KPFALSE;
		}
		else {
			fileSearch->opStatus = IOFILE_ERROR;
			(user_fcn) (fileSearch, user_ptr);
			returnStatus = KPFALSE;
			fileFound = KPFALSE;
		}
		FindClose (hFindFile);
	}
	

	freeBufferPtr ((KpGenericPtr_t)pFindData);
	return returnStatus;
}
//#endif

/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileOpen (MS Windows Version)
 *
 * DESCRIPTION
 * This function opens a file for either reading or writing.  If the
 * file name is NULL and the file properties is not NULL, the
 * OFSTRUCT data structure is used to reopen the file.  This is, of
 * course, Windows specific.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * A return of 2 means a sharing violation occured. This is only possible
 *		with the modes "R", "R+", "W", and "W+"
 *
 * AUTHOR
 * Peter Tracy(originally part of FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
--------------------------------------------------------------------*/
int FAR KpFileOpen (
			char		FAR *filename,
			char		FAR *mode,
			KpFileProps_t	FAR *fileProps,
			KpFileId	FAR *fdp)
{
	UINT		openFlag;
	OFSTRUCT	ofInfo;
	LPOFSTRUCT	ofData;
	KpBool_t	FileExists;
	dirStatus	StatusRet;
	int		Counter;

	openFlag = ((filename == NULL) && (fileProps != NULL)) ? OF_REOPEN : 0;

	ofData = (fileProps == NULL) ? &ofInfo : (LPOFSTRUCT) fileProps;

	switch (*mode) {
	case 'r':
		openFlag |= OF_READ;
		openFlag |= OF_SHARE_DENY_NONE;
		*fdp = OpenFile (filename, ofData, openFlag);
		/* Test for Lock in place */
		if (*fdp == HFILE_ERROR)
		{
			StatusRet = KpFileExists(filename,
					 				 fileProps,
					 				 &FileExists);
			if (FileExists == KPTRUE)
			{
				for (Counter = WAIT_LENGTH;
				     ((Counter > 5) && (*fdp == HFILE_ERROR));
				     Counter--)
				{
					KpSleep(Counter, 1);
					*fdp = OpenFile(filename,
						ofData, openFlag);
				}
			}
		}
		return (*fdp == (KpFileId) HFILE_ERROR)
					? KCMS_IO_ERROR : KCMS_IO_SUCCESS;

	case 'w':
		openFlag |= OF_READWRITE;
		openFlag |= OF_SHARE_DENY_NONE;
		*fdp = OpenFile (filename, ofData, openFlag);
		/* Test for Lock in place */
		for (Counter = WAIT_LENGTH;
		     ((Counter > 5) && (*fdp == HFILE_ERROR));
		     Counter--)
		{
			StatusRet = KpFileExists(filename,
					 				 fileProps,
					 				 &FileExists);
			if (FileExists == KPTRUE)
			{
				KpSleep(Counter, 1);
				*fdp = OpenFile(filename,
						ofData, openFlag);
			} else
			{
				openFlag |= OF_CREATE;
				*fdp = OpenFile(filename, 
						ofData, openFlag);
			}
		}
		return (*fdp == (KpFileId) HFILE_ERROR)
					? KCMS_IO_ERROR : KCMS_IO_SUCCESS;

	case 'e':
		openFlag |= OF_READWRITE;
		openFlag |= OF_SHARE_EXCLUSIVE;
		*fdp = OpenFile (filename, ofData, openFlag);
		for (Counter = WAIT_LENGTH;
		     ((Counter > 5) && (HFILE_ERROR == *fdp));
		     Counter--)
		{
			StatusRet = KpFileExists(filename,
					 				 fileProps,
					 				 &FileExists);
			if (FileExists == KPTRUE)
			{
				KpSleep(Counter, 1);
				*fdp = OpenFile(filename,
						ofData, openFlag);
			} else
			{
				openFlag |= OF_CREATE;
				*fdp = OpenFile(filename, 
						ofData, openFlag);
			}
		}
		return (*fdp == (KpFileId) HFILE_ERROR)
					? KCMS_IO_ERROR : KCMS_IO_SUCCESS;

	case 'R':

	/* set the mode bits */
		if ('+' == mode [1])
			openFlag |= OF_READWRITE | OF_SHARE_EXCLUSIVE;
		else
			openFlag |= OF_READ | OF_SHARE_DENY_WRITE;

	/* open the file */
		*fdp = OpenFile (filename, ofData, openFlag);

	/* check for success */
		if (HFILE_ERROR != *fdp)
			return KCMS_IO_SUCCESS;

	/* check for sharing violation */
		if (0x0020 == ofData->nErrCode)
			return KCMS_IO_ERR_SHARE;

	/* some other error, return failure */
		return KCMS_IO_ERROR;

	case 'W':
	/* set the mode bits */
		if ('+' == mode [1])
			openFlag |= OF_READWRITE | OF_SHARE_EXCLUSIVE;
		else
			openFlag |= OF_WRITE | OF_SHARE_EXCLUSIVE;

	/* open the file */
		*fdp = OpenFile (filename, ofData, openFlag);

	/* check for success */
		if (HFILE_ERROR != *fdp)
			return KCMS_IO_SUCCESS;

	/* check for sharing violation */
		if (0x0020 == ofData->nErrCode)
			return KCMS_IO_ERR_SHARE;

	/* try to create the file */
		*fdp = OpenFile (filename, ofData, OF_CREATE | openFlag);
	
	/* check for successfully creation of the file */
		if (HFILE_ERROR != *fdp) {

		/* close and open the file so the sharing options get set */
			_lclose (*fdp);
			*fdp = OpenFile (filename, ofData, openFlag);
		}

		if (HFILE_ERROR == *fdp) {
			if (0x0020 == ofData->nErrCode)
				return KCMS_IO_ERR_SHARE;
			else
				return KCMS_IO_ERROR;
		}
		return KCMS_IO_SUCCESS;
	}

/* invalid mode, return error */
	*fdp = (KpFileId) HFILE_ERROR;
	return KCMS_IO_ERROR;
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileClose (MS Windows Version)
 *
 * DESCRIPTION
 * This function closes a file.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy(originally part of FUT library)
 *
 * DATE CREATED
 * May 1, 1991
 *
 * Modified
 * Dec 12, 1991 lsh
 *
--------------------------------------------------------------------*/
int FAR KpFileClose (
			KpFileId	fd)
{
	return (HFILE_ERROR == _lclose ((HFILE) fd))
				? KCMS_IO_ERROR : KCMS_IO_SUCCESS;
}



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFilePosition (MS Windows Version)
 *
 * DESCRIPTION
 * This function positions the file.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * May 2, 1991
 *
--------------------------------------------------------------------*/
int FAR KpFilePosition (
			KpFileId	fd,
			KpFileStart	start,
			KpInt32_t	position)
{
	int	origin;                /* where to start seek from */

/* Set up orgin of seek depending on input parameter */
	switch (start) {
	case FROM_START:
		origin = 0;
		break;

	case FROM_END:
		origin = 2;
		break;

	default:
		origin = 1;
		break;
	}

/* Now actually do the seek and check the status */
	return (HFILE_ERROR == _llseek (fd, position, origin))
				? KCMS_IO_ERROR : KCMS_IO_SUCCESS;
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileTell (MS Windows Version)
 *
 * DESCRIPTION
 * This function returns the current byte position of the file.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * Dec 31, 1991
 *
--------------------------------------------------------------------*/
int FAR KpFileTell (
			KpFileId	fd,
			KpInt32_t	FAR *position)
{
	int		status;
	long	fPosition;

	fPosition = _llseek (fd, 0, 1);
	if (HFILE_ERROR == fPosition) {
		status = KCMS_IO_ERROR;
		*position = 0;
	}
	else {
		status = 1;
		*position = fPosition;
	}

	return status;
}


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileDelete(MS Windows Version)
 *
 * DESCRIPTION
 * This function deletes a file given its name.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * May 9, 1991
 *
 * Modified
 * Oct 18, 1991 RLC
 * Dec 12, 1991 lsh
 *
--------------------------------------------------------------------*/

int FAR KpFileDelete (
			char		FAR *fileName,
			KpFileProps_t	FAR *fileProps)
{
	KpFileId	ret;

	ret = OpenFile (fileName, (LPOFSTRUCT) fileProps, OF_DELETE);
	return (HFILE_ERROR == ret) ? KCMS_IO_ERROR : KCMS_IO_SUCCESS;
}



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileSize (MS Windows Version)
 *
 * DESCRIPTION
 * This function gets the size of a file.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Robert Cushman
 *
 * DATE CREATED
 * October 18, 1991
 *
 * Modified
 * Dec 12, 1991 lsh
 *
--------------------------------------------------------------------*/
int FAR KpFileSize (
				char		FAR *file,
				KpFileProps_t	FAR *fileProps,
				KpInt32_t	FAR *size)
{
	KpFileId	fd;
	UINT		openFlag;
	OFSTRUCT	props;
	long		pos;
	KpBool_t FileExists;
	dirStatus	StatusRet;
	int		Counter;

	fileProps = fileProps;

	openFlag = ((file == NULL) && (fileProps != NULL)) ? OF_REOPEN : 0;
	openFlag |= OF_READ;
	openFlag |= OF_SHARE_DENY_NONE;

	fd = OpenFile (file, &props, openFlag);
	if (HFILE_ERROR == fd)
	{
		StatusRet = KpFileExists(file,
				 				 fileProps,
				 				 &FileExists);
		if (FileExists == KPTRUE)
		{
			for (Counter = WAIT_LENGTH;
			     ((Counter > 5) && (HFILE_ERROR == fd));
			     Counter--)
			{
				KpSleep(Counter, 1);
				fd = OpenFile (file, &props, openFlag);
			}
			if (HFILE_ERROR == fd)
				return KCMS_IO_ERROR;
		} else
			return KCMS_IO_ERROR;
	}

/*
 * Seek to end of file and get size since
 * _llseek returns current position
 */
	pos = _llseek (fd, 0, 2);
	_lclose (fd);
	if (HFILE_ERROR == pos)
		return KCMS_IO_ERROR;

	*size = pos;
	return KCMS_IO_SUCCESS;
}



/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileExists (MS Windows Version)
 *
 * DESCRIPTION
 * This function checks to see if a file exists.
 *
 * RETURN
 * A return of 1 means it worked OK.
 * A return of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * Jan 3, 1992
 *
--------------------------------------------------------------------*/
dirStatus FAR KpFileExists (
			char		FAR *file,
			KpFileProps_t	FAR *fileProps,
			KpBool_t	FAR *exists)
{
	int		fd_stat;
	dirStatus	retStatus;
	struct stat	buf;

	fileProps = fileProps;

	fd_stat = stat (file, &buf);
	if (fd_stat == -1)
	{
		*exists =KPFALSE;
		retStatus = DIR_ERROR;
	}
	else
	{
		*exists = KPTRUE;
		retStatus = DIR_OK;
	}

	return (retStatus);
}
//#if !defined (KPMSMAC)
/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileDirCount (MS Windows Version)
 *
 * DESCRIPTION
 * This function counts the files in a directory.
 *
 * RETURN
 * A return int of 1 means it worked OK.
 * A return int of 0 means an error occured.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * Jan 3, 1992
 *
--------------------------------------------------------------------*/
dirStatus FAR KpFileDirCount (
			char		FAR *dirName,
			KpFileProps_t	FAR *props,
			KpInt32_t	FAR *numFiles)
{
Kp_FindOS_Data_p pFindData;
HANDLE           hFindFile;
KpBool_t         fileFound = KPTRUE;
KpInt32_t        Count;
dirStatus        returnStatus = DIR_NO_FILE;
KpChar_t		 searchStr [MAX_PATH];
KpChar_p		 pChar;

	props = props;

	(*numFiles) = 0;

	pFindData = (Kp_FindOS_Data_p) allocBufferPtr (sizeof(Kp_FindOS_Data));
	if (NULL == pFindData)
		return (returnStatus);

	/*	Need to append \*.* to the end of dirName so FindFile will look for
		files in the directory instead of the directory itself.  Need to
		copy dirName into a local buffer because the size of the buffer
		dirName points to is unknown										*/
	if ((strlen (dirName) + 5) > MAX_PATH) {
		freeBufferPtr (pFindData);
		return KCMS_FAIL;
	}
	strcpy (searchStr, dirName);
	pChar = strrchr (searchStr, '\\');
	if (NULL != pChar++) {
		if ('\\' == *pChar) {
			*pChar = 0;
		}
	}	
	strcat (searchStr, "\\*.*");
	hFindFile = FindFirstFile (searchStr, pFindData);
	if (hFindFile != INVALID_HANDLE_VALUE) {
		/* First file was found OK */
		Count = 1;
		returnStatus = DIR_OK;
		while (fileFound)
		{

			fileFound = (KpBool_t)FindNextFile(hFindFile, pFindData);
			if (fileFound) {
				Count++;
			}
			else {
				if (GetLastError() != ERROR_NO_MORE_FILES)
					returnStatus = DIR_ERROR;
			}
		
		}
		FindClose (hFindFile);
        *numFiles = Count;
	}
	freeBufferPtr (pFindData);

	return (returnStatus);
}
//#endif


#else

   **** OPERATING SYSTEM NOT DEFINED ****

#endif


/*--------------------------------------------------------------------
 * FUNCTION NAME
 * KpFileCopy ( OS independent )
 *
 * DESCRIPTION
 * This function copies files.
 *
 * AUTHOR
 * Peter Tracy
 *
 * DATE CREATED
 * June 24, 1991
 *
--------------------------------------------------------------------*/

int FAR KpFileCopy (KpFileProps_t	*props, 
		    char 		FAR *existingFile, 
		    char 		FAR *newFile)
{
	char		FAR *buffer;	/* buffer for copying data */
	KpFileId	inId;			/* file ID for existing file */
	KpInt32_t	numRead;		/* no. of bytes read in */
	KpInt32_t	Size;			/* file size of input file */
	KpFileId	outId;			/* file Id for new file */
	int			status;			/* file I/O status */

/*
 *   Set things up by first opening up the files
 *   and allocating space for a copy buffer
 */
	status = KpFileOpen (existingFile, "r", props, &inId);
	if (KCMS_IO_SUCCESS != status)
		return status;

	status = KpFileOpen (newFile, "w", props, &outId);
	if (KCMS_IO_SUCCESS != status) {
		KpFileClose (inId);
		return status;
	}

	status = KpFileSize(existingFile, props, &Size);
	if (KCMS_IO_SUCCESS != status) {
		KpFileClose (inId);
		KpFileClose (outId);
		return status;
	}
	buffer = allocBufferPtr (COPY_BUFFER_SIZE);
	if (buffer == NULL) {
		KpFileClose (inId);
		KpFileClose (outId);
		return KCMS_IO_ERROR;
	}

/*
 *   This is the actual copying loop
 */
	numRead = COPY_BUFFER_SIZE;
	status = KCMS_IO_SUCCESS;
	while ((Size > 0) && (status == KCMS_IO_SUCCESS)) {
		status = KpFileRead (inId, buffer, &numRead);
		Size -= numRead;
		if ((status == KCMS_IO_SUCCESS) && (numRead > 0))
			status = KpFileWrite (outId, buffer, numRead);
	}

/*
 *   Now clean up and exit this routine
 */
	KpFileClose (inId);
	KpFileClose (outId);
	freeBufferPtr (buffer);

	return status;
}


