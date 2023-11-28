/*****************************************************************************/
/*
** "BAEFile.cpp"
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
**	8/18/99		Changed CleanResource & DeleteResource to do the right thing
**				if an error happens.
**	8/28/99		Added the begining of BAEFileResourceGroup
*/
/*****************************************************************************/

#include "BAE.h"
#include "BAE_API.h"
#include "X_API.h"
#include "X_Formats.h"

#include "BAEFile.h"

// BAE file class. Use to open/read/write standard data files

// open a file given a file name, accessMode and err
BAEFile::BAEFile(BAEPathName pFile, BAE_FILE_ACCESS accessMode, BAEResult *pError)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	mMode = accessMode;
	mFileReference = NULL;
	mFileName = (void *)XNewPtr(sizeof(XFILENAME));
	if (mFileName)
	{
		if (pFile == NULL)
		{
			if (XGetTempXFILENAME((XFILENAME *)mFileName))
			{
				err = BAE_BAD_FILE;
			}
		}
		if (err == BAE_NO_ERROR)
		{
			XConvertNativeFileToXFILENAME(pFile, (XFILENAME *)mFileName);
			switch (accessMode)
			{
				case BAE_FILE_READ:
					mFileReference = (void *)XFileOpenForRead((XFILENAME *)mFileName);
					break;
				case BAE_FILE_READ_WRITE_CREATE:
					mFileReference = (void *)XFileOpenForWrite((XFILENAME *)mFileName, TRUE);
					break;
				case BAE_FILE_READ_WRITE:
					mFileReference = (void *)XFileOpenForWrite((XFILENAME *)mFileName, FALSE);
					break;
			}
			if (mFileReference == NULL)
			{
				err = BAE_FILE_NOT_FOUND;
			}
		}
	}
	else
	{
		err = BAE_MEMORY_ERR;
	}

	if (pError)
	{
		Close();
		*pError = err;
	}
}


BAEFile::~BAEFile()
{
	Close();
}


BAEResult BAEFile::Close(void)
{
	if (mFileReference)
	{
		XFileClose((XFILE)mFileReference);
		mFileReference = NULL;
	}
	XDisposePtr((XPTR)mFileName);
	mFileName = NULL;
	return BAE_NO_ERROR;
}

BAEResult BAEFile::Delete(void)
{
	XFILENAME	name;
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		name = *(XFILENAME *)mFileName;
		Close();
		if (XFileDelete(&name))
		{
			err = BAE_FILE_NOT_FOUND;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}

BAE_BOOL BAEFile::IsValid(void)
{
	return (mFileReference) ? TRUE : FALSE;
}

BAE_BOOL BAEFile::IsReadOnly(void)
{
	return (mMode == BAE_FILE_READ) ? TRUE : FALSE;
}

BAE_BOOL BAEFile::IsWriteable(void)
{
	return ((mMode == BAE_FILE_READ_WRITE) || (mMode == BAE_FILE_READ_WRITE_CREATE)) ? TRUE : FALSE;
}


BAEResult BAEFile::GetFileName(BAEPathName *pFile)
{
	BAEResult	err;
	XFILENAME	name;

	err = BAE_NO_ERROR;
	if (pFile)
	{
		if (IsValid())
		{
			if (mFileName)
			{
				void	*data;

				name = *(XFILENAME *)mFileName;
				data = &name.theFile;
				BAE_CopyFileNameNative(data, (void *)pFile);
			}
			else
			{
				err = BAE_MEMORY_ERR;
			}
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
	}
	else
	{
		err = BAE_PARAM_ERR;
	}
	return err;
}


BAEResult BAEFile::GetSize(unsigned long *pCurrentSize)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (pCurrentSize)
	{
		if (IsValid())
		{
			*pCurrentSize = (unsigned long)XFileGetLength((XFILE)mFileReference);
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
	}
	else
	{
		err = BAE_PARAM_ERR;
	}
	return err;
}

BAEResult BAEFile::SetSize(unsigned long newSize)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (XFileSetLength((XFILE)mFileReference, newSize))
		{
			err = BAE_FILE_IO_ERROR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}


BAEResult BAEFile::Read(void * data, unsigned long size)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (data && size)
	{
		if (IsValid())
		{
			if (XFileRead((XFILE)mFileReference, data, (long)size))
			{
				err = BAE_FILE_IO_ERROR;
			}
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
	}
	else
	{
		err = BAE_PARAM_ERR;
	}
	return err;
}

BAEResult BAEFile::ReadAt(unsigned long startPosition, void * data, unsigned long size)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (data && size)
	{
		if (IsValid())
		{
			SetCurrentPositionFromStart(startPosition);
			if (XFileRead((XFILE)mFileReference, data, (long)size))
			{
				err = BAE_FILE_IO_ERROR;
			}
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
	}
	else
	{
		err = BAE_PARAM_ERR;
	}
	return err;
}

BAEResult BAEFile::Write(const void * data, unsigned long size)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (data && size)
	{
		if (IsValid())
		{
			if (XFileWrite((XFILE)mFileReference, (void *)data, (long)size))
			{
				err = BAE_FILE_IO_ERROR;
			}
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
	}
	else
	{
		err = BAE_PARAM_ERR;
	}
	return err;
}


unsigned long BAEFile::GetCurrentPosition(void)
{
	unsigned long	pos;

	pos = 0;
	if (IsValid())
	{
		pos = (long)XFileGetPosition((XFILE)mFileReference);
	}
	return pos;
}

BAEResult BAEFile::SetCurrentPositionFromStart(unsigned long newPosition)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (XFileSetPosition((XFILE)mFileReference, (long)newPosition))
		{
			err = BAE_FILE_IO_ERROR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}

BAEResult BAEFile::SetCurrentPositionFromEnd(unsigned long newPosition)
{
	BAEResult		err;
	unsigned long	length;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		err = GetSize(&length);
		if (err == BAE_NO_ERROR)
		{
			err = SetCurrentPositionFromStart(length - newPosition);
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}

BAEResult BAEFile::SetCurrentPositionRelative(long deltaPosition)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (XFileSetPositionRelative((XFILE)mFileReference, deltaPosition))
		{
			err = BAE_FILE_IO_ERROR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}



// BAE resource class. Use to open/read/write BAE resource files
BAEFileResource::BAEFileResource(BAEPathName pFile, BAE_FILE_ACCESS accessMode, BAEResult *pError)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	mMode = accessMode;
	mFileReference = NULL;
	mGroupNext = NULL;
	mFileName = (void *)XNewPtr(sizeof(XFILENAME));
	if (mFileName)
	{
		if (pFile == NULL)
		{
			if (XGetTempXFILENAME((XFILENAME *)mFileName))
			{
				err = BAE_BAD_FILE;
			}
		}
		if (err == BAE_NO_ERROR)
		{
			XConvertNativeFileToXFILENAME(pFile, (XFILENAME *)mFileName);
			switch (accessMode)
			{
				case BAE_FILE_READ:
					mFileReference = (void *)XFileOpenResource((XFILENAME *)mFileName, TRUE);
					break;
				case BAE_FILE_READ_WRITE_CREATE:
				case BAE_FILE_READ_WRITE:
					mFileReference = (void *)XFileOpenResource((XFILENAME *)mFileName, FALSE);
					break;
			}
			if (mFileReference == NULL)
			{
				err = BAE_FILE_NOT_FOUND;
			}
		}
	}
	else
	{
		err = BAE_MEMORY_ERR;
	}

	if (pError)
	{
		*pError = err;
	}

	if (err)
	{
		Close();
	}
}

BAEFileResource::BAEFileResource(const void * data, unsigned long size, BAE_FILE_ACCESS accessMode, BAEResult *pError)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	mMode = accessMode;
	mFileReference = NULL;
	mGroupNext = NULL;
	if (data && size)
	{
		if (accessMode == BAE_FILE_READ)
		{
			mFileReference = (void *)XFileOpenResourceFromMemory((XPTR)data, size, TRUE);
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
	}
	else
	{
		err = BAE_PARAM_ERR;
	}

	if (pError)
	{
		Close();
		*pError = err;
	}
}

BAEFileResource::~BAEFileResource()
{
	Close();
}


BAEResult BAEFileResource::Close(void)
{
	if (mFileReference)
	{
		XFileClose((XFILE)mFileReference);
		mFileReference = NULL;
	}
	XDisposePtr((XPTR)mFileName);
	mFileName = NULL;
	return BAE_NO_ERROR;
}

BAEResult BAEFileResource::Delete(void)
{
	XFILENAME	name;
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		name = *(XFILENAME *)mFileName;
		Close();
		if (XFileDelete(&name))
		{
			err = BAE_FILE_NOT_FOUND;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}

BAE_BOOL BAEFileResource::IsValid(void)
{
	return (mFileReference) ? TRUE : FALSE;
}

BAE_BOOL BAEFileResource::IsReadOnly(void)
{
	return (mMode == BAE_FILE_READ) ? TRUE : FALSE;
}

BAE_BOOL BAEFileResource::IsWriteable(void)
{
	return ((mMode == BAE_FILE_READ_WRITE) || (mMode == BAE_FILE_READ_WRITE_CREATE)) ? TRUE : FALSE;
}


BAEResult BAEFileResource::GetFileName(BAEPathName *pFile)
{
	BAEResult	err;
	XFILENAME	name;

	err = BAE_NO_ERROR;
	if (pFile)
	{
		if (IsValid())
		{
			if (mFileName)
			{
				void	*data;

				name = *(XFILENAME *)mFileName;
				data = &name.theFile;
				BAE_CopyFileNameNative(data, (void *)pFile);
			}
			else
			{
				err = BAE_MEMORY_ERR;
			}
		}
		else
		{
			err = BAE_NOT_SETUP;
		}
	}
	else
	{
		err = BAE_PARAM_ERR;
	}
	return err;
}

// get a unique ID for a particular file to be used as a resource ID
BAEResult BAEFileResource::GetUniqueID(BAEResourceType resourceType, BAELongResourceID *pReturnedID)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (XGetUniqueFileResourceID((XFILE)mFileReference, (XResourceType)resourceType, (XLongResourceID *)pReturnedID))
		{
			err = BAE_FILE_IO_ERROR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}


BAEResult BAEFileResource::GetResourceName(BAEResourceType theType, BAELongResourceID theID, char *cName, unsigned long cNameSize)
{
	BAEResult	err;
	char		name[256];

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (cName)
		{
			cName[0] = 0;
			name[0] = 0;
			if (XGetResourceNameOnly((XFILE)mFileReference, (XResourceType)theType, (XLongResourceID)theID, name))
			{
				XPtoCstr(name);
				if (XStrLen(name) < (cNameSize + 1))
				{
					XStrCpy(cName, name);
				}
				else
				{
					err = BAE_PARAM_ERR;
				}
			}
			else
			{
				err = BAE_RESOURCE_NOT_FOUND;
			}
		}
		else
		{
			err = BAE_PARAM_ERR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}

// when calling any of the Get Resource functions, the memory returned is special. Call DisposeResource to
// deallocate the memory. The only exception to this is ReadPartialResource, which fills a buffer

// if you need to control the memory that is currently a resource, allocate the memory yourself and copy it, then use
// this method to Dispose of it.
BAEResult BAEFileResource::DisposeResource(BAEResource pResource, unsigned long resourceSize)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (pResource && resourceSize)
		{
			XDisposePtr((XPTR)pResource);
		}
		else
		{
			err = BAE_PARAM_ERR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}

// all resource functions that return data
BAEResource BAEFileResource::GetNamedResource(BAEResourceType resourceType, char *cName, unsigned long cNameSize, 
									unsigned long *pReturnedResourceSize, BAEResult *pError)
{
	BAEResult	err;
	BAEResource	data;
	XFILE		saveFile;

	data = NULL;
	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (cName && (cNameSize < 256))
		{
			cName[0] = 0;
			saveFile = XFileGetCurrentResourceFile();
			XFileUseThisResourceFile((XFILE)mFileReference);
			data = (BAEResource)XGetNamedResource((XResourceType)resourceType, (void *)cName, (long *)pReturnedResourceSize);
			if (data == NULL)
			{
				err = BAE_RESOURCE_NOT_FOUND;
			}
			XFileUseThisResourceFile(saveFile);
		}
		else
		{
			err = BAE_PARAM_ERR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	if (pError)
	{
		*pError = err;
	}
	return data;
}

BAEResource BAEFileResource::GetResource(BAEResourceType resourceType, BAELongResourceID resourceID, 
									char *cName, unsigned long cNameSize, unsigned long *pReturnedResourceSize,
									BAEResult *pError)
{
	BAEResult	err;
	BAEResource	data;
	char		name[256];

	data = NULL;
	err = BAE_NO_ERROR;
	if (IsValid())
	{
		name[0] = 0;
		if (cName)
		{
			cName[0] = 0;
		}
		data = (BAEResource)XGetFileResource((XFILE)mFileReference, (XResourceType)resourceType, (XLongResourceID)resourceID, 
												name, (long *)pReturnedResourceSize);
		if (data == NULL)
		{
			err = BAE_RESOURCE_NOT_FOUND;
		}
		else
		{
			XPtoCstr(name);
			if (cName && (XStrLen(name) < (cNameSize + 1)))
			{
				XStrCpy(cName, name);
			}
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	if (pError)
	{
		*pError = err;
	}
	return data;
}

BAEResource BAEFileResource::GetIndexedResource(BAEResourceType resourceType, BAELongResourceID *pReturnedID, 
									long resourceIndex, 
									char *cName, unsigned long cNameSize, unsigned long *pReturnedResourceSize, BAEResult *pError)
{
	BAEResult	err;
	BAEResource	data;
	char		name[256];

	data = NULL;
	err = BAE_NO_ERROR;
	if (IsValid())
	{
		name[0] = 0;
		if (cName)
		{
			cName[0] = 0;
			data = (BAEResource)XGetIndexedFileResource((XFILE)mFileReference, (XResourceType)resourceType, (XLongResourceID *)pReturnedID, resourceIndex, 
									name, (long *)pReturnedResourceSize);
			if (name[0])
			{
				XPtoCstr(name);
				if (XStrLen(name) < (cNameSize + 1))
				{
					XStrCpy(cName, name);
				}
				else
				{
					err = BAE_PARAM_ERR;
					XDisposePtr((XPTR)data);
					data = NULL;
				}
			}
		}
		else
		{
			err = BAE_PARAM_ERR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	if (pError)
	{
		*pError = err;
	}
	return data;
}


BAEResult BAEFileResource::ReadPartialResource(BAEResourceType resourceType, BAELongResourceID resourceID,
						char *cName, unsigned long cNameSize,
						void *data, unsigned long bytesToRead)
{
	BAEResult	err;
	XPTR		buffer;
	char		name[256];

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (data && bytesToRead && cName)
		{
			name[0] = 0;
			cName[0] = 0;
			if (XReadPartialFileResource((XFILE)mFileReference, (XResourceType)resourceType, (XLongResourceID)resourceID,
								name,
								&buffer, (long)bytesToRead) == 0)
			{
				XBlockMove(buffer, data, (long)bytesToRead);
				XDisposePtr(buffer);

				if (name[0])
				{
					XPtoCstr(name);
					if (cName && (XStrLen(name) < (cNameSize + 1)))
					{
						XStrCpy(cName, name);
					}
				}
			}
			else
			{
				err = BAE_FILE_IO_ERROR;
			}
		}
		else
		{
			err = BAE_PARAM_ERR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}


// Add a resource to the most recently open resource file.
//		resourceType is a type
//		resourceID is an ID
//		pResourceName is a pascal string
//		pData is the data block to add
//		length is the length of the data block
BAEResult BAEFileResource::AddResource(BAEResourceType resourceType, BAELongResourceID resourceID, 
					const char *cResourceName, const void *pData, unsigned long length)
{
	BAEResult	err;
	char		name[256];

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		name[0] = 0;
		if (cResourceName)
		{
			if (XStrLen(cResourceName) < 256)
			{
				XStrCpy(name, cResourceName);
				XCtoPstr((void *)name);
			}
			else
			{
				err = BAE_PARAM_ERR;
			}
		}
		if (err == BAE_NO_ERROR)
		{
			if (XAddFileResource((XFILE)mFileReference, (XResourceType)resourceType, (XLongResourceID)resourceID, (void *)name, (void *)pData, (long)length))
			{
				err = BAE_FILE_IO_ERROR;
			}
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}


// Delete a resource from the most recently open resource file.
//		resourceType is a type
//		resourceID is an ID
//		collectTrash if TRUE will force an update, otherwise it will happen when the file is closed
BAEResult BAEFileResource::DeleteResource(BAEResourceType resourceType, BAELongResourceID resourceID, BAE_BOOL collectTrash )
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (XDeleteFileResource((XFILE)mFileReference, (XResourceType)resourceType, (XLongResourceID)resourceID, (XBOOL)collectTrash) == FALSE)
		{
			err = BAE_FILE_IO_ERROR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}


// return the number of resources of a particular type.
long BAEFileResource::CountResourcesOfType(BAEResourceType resourceType, BAEResult *pError)
{
	BAEResult	err;
	long		count;

	count = 0;
	err = BAE_NO_ERROR;
	if (IsValid())
	{
		count = XCountFileResourcesOfType((XFILE)mFileReference, (XResourceType)resourceType);
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	if (pError)
	{
		*pError = err;
	}
	return count;
}

// return the number of unqiue BAEResource types in this resource file
long BAEFileResource::CountTypes(BAEResult *pError)
{
	BAEResult	err;
	long		count;

	count = 0;
	err = BAE_NO_ERROR;
	if (IsValid())
	{
		count = XCountTypes((XFILE)mFileReference);
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	if (pError)
	{
		*pError = err;
	}
	return count;
}

// return the BAEResourceType index from 0 to CountTypes()
BAEResourceType BAEFileResource::GetIndexedType(long resourceIndex, BAEResult *pError)
{
	BAEResult		err;
	BAEResourceType	type;

	type = 0;
	err = BAE_NO_ERROR;
	if (IsValid())
	{
		type = (BAEResourceType)XGetIndexedType((XFILE)mFileReference, resourceIndex);
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	if (pError)
	{
		*pError = err;
	}
	return type;
}

// Force a clean/update of the most recently opened resource file
BAEResult BAEFileResource::CleanResource(void)
{
	BAEResult	err;

	err = BAE_NO_ERROR;
	if (IsValid())
	{
		if (XCleanResourceFile((XFILE)mFileReference) == FALSE)
		{
			err = BAE_FILE_IO_ERROR;
		}
	}
	else
	{
		err = BAE_NOT_SETUP;
	}
	return err;
}


BAEFileResourceGroup::BAEFileResourceGroup()
{
	m_topResourceFile = NULL;
}


BAEFileResourceGroup::~BAEFileResourceGroup()
{
	BAEFileResource		*pTop, *pNext;

	Close();		// close this group

	// clear links for BAEFileResource objects
	pTop = m_topResourceFile;
	m_topResourceFile = NULL;
	while (pTop)
	{
		delete pTop;
		pNext = pTop->mGroupNext;
		pTop->mGroupNext = NULL;
		pTop = pNext;
	}
}

// close all BAEFileResource objects associated with this group
BAEResult BAEFileResourceGroup::Close(void)
{
	BAEFileResource		*pNext;
	BAEResult			err;

	err = BAE_NO_ERROR;
	pNext = m_topResourceFile;
	while (pNext)
	{
		err = pNext->Close();
		pNext = pNext->mGroupNext;
	}
	return BAE_NO_ERROR;
}
