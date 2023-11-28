/*****************************************************************************/
/*
** "BAEFile.h"
**
**	Cross platform file and resource file manager
**
**	\xA9 Copyright 1999 Beatnik, Inc, All Rights Reserved.
**	Written by Steve Hales
**
**	Beatnik products contain certain trade secrets and confidential and
**	proprietary information of Beatnik.  Use, reproduction, disclosure
**	and distribution by any means are prohibited, except pursuant to
**	a written license from Beatnik. Use of copyright notice is
**	precautionary and does not imply publication or disclosure.
**
**	Restricted Rights Legend:
**	Use, duplication, or disclosure by the Government is subject to
**	restrictions as set forth in subparagraph (c)(1)(ii) of The
**	Rights in Technical Data and Computer Software clause in DFARS
**	252.227-7013 or subparagraphs (c)(1) and (2) of the Commercial
**	Computer Software--Restricted Rights at 48 CFR 52.227-19, as
**	applicable.
**
** Modification History:
**
**	7/8/99		Created
**	8/28/99		Added the begining of BAEFileResourceGroup
*/
/*****************************************************************************/

/*
	Description of use:
	
*/

#ifndef BAE_FILE
#define BAE_FILE

#ifndef BAE_AUDIO
	#include "BAE.h"
#endif

enum BAE_FILE_ACCESS
{
	BAE_FILE_READ					=	1,
	BAE_FILE_READ_WRITE,
	BAE_FILE_READ_WRITE_CREATE
};


// BAE file class. Use to open/read/write standard data files
class BAEFile
{
public:
		// open a file given a file name, accessMode and err.
		// Pass NULL to the file to create a temporary file name.
		BAEFile(BAEPathName pFile, BAE_FILE_ACCESS accessMode, BAEResult *pError);
virtual	~BAEFile();

		BAEResult		Close(void);
		BAEResult		Delete(void);	// delete file

		BAE_BOOL		IsValid(void);
		BAE_BOOL		IsReadOnly(void);
		BAE_BOOL		IsWriteable(void);

		BAEResult		GetFileName(BAEPathName *pFile);

		BAEResult		GetSize(unsigned long *pCurrentSize);
		BAEResult		SetSize(unsigned long newSize);

		BAEResult		Read(void * data, unsigned long size);
		BAEResult		ReadAt(unsigned long startPosition, void * data, unsigned long size);
		BAEResult		Write(const void * data, unsigned long size);

		unsigned long	GetCurrentPosition(void);
		BAEResult		SetCurrentPositionFromStart(unsigned long newPosition);
		BAEResult		SetCurrentPositionFromEnd(unsigned long newPosition);
		BAEResult		SetCurrentPositionRelative(long deltaPosition);

private:
	BAE_FILE_ACCESS		mMode;
	void				*mFileReference;
	void				*mFileName;
};

typedef unsigned long	BAEResourceType;
typedef unsigned long	BAELongResourceID;
typedef unsigned short	BAEShortResourceID;
typedef void *			BAEResource;

// BAE resource class. Use to open/read/write BAE resource files
class BAEFileResource
{
public:
friend class BAEFileResourceGroup;
		// open a resource file given a file name, accessMode and err
		// Pass NULL to the file to create a temporary file name.
		BAEFileResource(BAEPathName pFile, BAE_FILE_ACCESS accessMode, BAEResult *pError);

		// open a resource file from a formated memory block.
		BAEFileResource(const void * data, unsigned long size, BAE_FILE_ACCESS accessMode, BAEResult *pError);
virtual	~BAEFileResource();

		BAEResult		Close(void);
		BAEResult		Delete(void);	// delete file

		BAE_BOOL		IsValid(void);
		BAE_BOOL		IsReadOnly(void);
		BAE_BOOL		IsWriteable(void);

		BAEResult		GetFileName(BAEPathName *pFile);

		// get a unique ID for a particular file to be used as a resource ID
		BAEResult 		GetUniqueID(BAEResourceType resourceType, BAELongResourceID *pReturnedID);

		BAEResult		GetResourceName(BAEResourceType theType, BAELongResourceID theID, char *cName, unsigned long cNameSize);

		// when calling any of the Get Resource functions, the memory returned is special. Call DisposeResource to
		// deallocate the memory. The only exception to this is ReadPartialResource, which fills a buffer

		// if you need to control the memory that is currently a resource, allocate the memory yourself and copy it, then use
		// this method to Dispose of it.

		// This is done this way to support reading resources out of ROM.
		BAEResult		DisposeResource(BAEResource pResource, unsigned long resourceSize);

		// all resource functions that return data
		BAEResource 	GetResource(BAEResourceType resourceType, BAELongResourceID resourceID, 
											char *cName, unsigned long cNameSize, unsigned long *pReturnedResourceSize, BAEResult *pError = NULL);
		BAEResource 	GetNamedResource(BAEResourceType resourceType, char *cName, unsigned long cNameSize, 
											unsigned long *pReturnedResourceSize, BAEResult *pError = NULL);
		BAEResource		GetIndexedResource(BAEResourceType resourceType, BAELongResourceID *pReturnedID, 
											long resourceIndex, 
											char *cName, unsigned long cNameSize, unsigned long *pReturnedResourceSize, BAEResult *pError = NULL);

		BAEResult 		ReadPartialResource(BAEResourceType resourceType, BAELongResourceID resourceID,
								char *cName, unsigned long cNameSize,
								void *data, unsigned long bytesToRead);

		// Add a resource to the most recently open resource file.
		//		resourceType is a type
		//		resourceID is an ID
		//		pResourceName is a pascal string
		//		pData is the data block to add
		//		length is the length of the data block
		BAEResult		AddResource(BAEResourceType resourceType, BAELongResourceID resourceID, 
							const char *cResourceName, const void *pData, unsigned long length);

		// Delete a resource from the most recently open resource file.
		//		resourceType is a type
		//		resourceID is an ID
		//		collectTrash if TRUE will force an update, otherwise it will happen when the file is closed
		BAEResult		DeleteResource(BAEResourceType resourceType, BAELongResourceID resourceID, BAE_BOOL collectTrash );

		// return the number of resources of a particular type.
		long			CountResourcesOfType(BAEResourceType resourceType, BAEResult *pError = NULL);

		// return the number of unqiue BAEResource types in this resource file
		long			CountTypes(BAEResult *pError = NULL);
		// return the BAEResourceType index from 0 to CountTypes();
		BAEResourceType	GetIndexedType(long resourceIndex, BAEResult *pError = NULL);

			// Force a clean/update of the most recently opened resource file
		BAEResult		CleanResource(void);

private:
		BAE_FILE_ACCESS		mMode;
		void				*mFileReference;
		void				*mFileName;
		BAEFileResource		*mGroupNext;		// non-null when grouped
};

// BAE resource group class. Use to search through many BAEFileResource files for
// resources
class BAEFileResourceGroup
{
		BAEFileResourceGroup();
virtual	~BAEFileResourceGroup();

		// Associate an BAE resource file to this group
		BAEResult			AddResourceFile(BAEFileResource *pResourceFile);

		// Disassociate an BAE resource file from this group
		BAEResult			RemoveResourceFile(BAEFileResource *pResourceFile);

		// close all BAEFileResource objects associated with this group
		BAEResult			Close(void);

private:
		BAEFileResource		*m_topResourceFile;
};

#endif	// BAE_FILE






