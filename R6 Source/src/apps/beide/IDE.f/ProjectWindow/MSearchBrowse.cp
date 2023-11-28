/*
 *  MSearchBrowse.cp
 *
 *  Copyright © 1995 metrowerks inc.  All rights reserved.
 *
 *	Routines for supporting Find Definition when the 
 *	browser is disabled. Used in both the 1.5 IDE and
 *	in 1.4, where we don't use C++. This file has to be
 *	C++, as it needs to call C++ functions in 1.5, but it 
 *  can be called from C.
 */

// Compatibility
typedef char* Ptr;
typedef Ptr* Handle;
// End Compatibility

#include <string.h>
#include "Utils.h"
#include "MWBrowse.h"
#include "MSearchBrowse.h"
#include "Unmangle.h"

//#include "MWSearchBrowse.h"
//#include "MWResources.h"
//#include "MWResourceManager.h"
#if 0
#include "MWBrowse.h"
#include "MWStrings.h"
#endif


// From CompilerTools.h
typedef struct GList {
	Handle	data;		//	list data handle
	long	size;		//	size of actual data in list
// The next two fields aren't used so removed BDS
//	long	hndlsize;	//	real size of handle				
//	long	growsize;	//	number of bytes to add after overflow
} GList;

// To read object code name table
typedef struct Object Object;
typedef struct OLink OLink;
typedef struct HashNameNode HashNameNode;
#include "Obj.h"

	// Subset of the GList routines lifted from CompilerTools.c
	// That file is not included in either the 1.4 or 1.5 IDE

static short GetGListByte(register GList *gl)
{
	return(*(*gl->data+gl->size++));
}

static short GetGListWord(GList *gl)
{
	Ptr		ptr;
	short	n;

	ptr=*gl->data+gl->size; gl->size+=2;
	*(((char *)&n))=*ptr++; *(((char *)&n)+1)=*ptr; return(n);
}

static long GetGListLong(GList *gl)
{
	Ptr		ptr;
	long	n;

	ptr=*gl->data+gl->size; gl->size+=4;
	*(((char *)&n)+0)=*ptr++; *(((char *)&n)+1)=*ptr++;
	*(((char *)&n)+2)=*ptr++; *(((char *)&n)+3)=*ptr;
	return(n);
}

static void GetGListData(register GList *gl,Ptr where,long size)
{
	memcpy(where, *gl->data+gl->size, size); 
	gl->size += size;
//	BlockMove(*gl->data+gl->size,where,size); gl->size+=size;
}

static void ReadString(
	GList* 	stream, 
	char** 	names, 
	char 	str[], 
	short 	strSize)
{
	short strLen = GetGListWord(stream);
	if (strLen > 0)
	{
		short excess = 0;
		if (strLen > strSize) 
		{
			excess = strLen - strSize;
			strLen = strSize;
		}
		GetGListData(stream, str, strLen+1);
		if (excess)
			stream->size += excess;
	}
	else if (strLen == -1)
	{	// name table name
		ASSERT(names);
		long ID = GetGListLong(stream);
		strLen = strlen(names[ID]) + 1;
		if (strLen > strSize) strLen = strSize;
		memcpy(str, names[ID], strLen+1);
//		BlockMoveData(names[ID], str, strLen+1);
		str[strLen] = 0;
	}
	else
		str[0] = 0;
}

static Boolean SearchForName(
	const char* name, 
	Boolean 	fSearchStatics,
	GList* 		stream,
	char** 		names, 
	long* 		offset,
	Boolean* 	fIsCode)
{
	const long	MANGLED_LEN = 2048;
	char		unmangleBuf[MANGLED_LEN];
	char		simpleName[256];
	char		qualifiedName[MANGLED_LEN];
	Boolean		fFound = false;
	long		flags;
	long		localFuncID;
	
	// We're only expecting function and global records here.
	// if we find anything else we abort searching and return
	// false. To emulate old Find Definition behavior, we only
	// find symbols in main file, which has the local file
	// ID of 1.
	
	while (!fFound)
	{
		short contribFile, srcFile;
		long startOffset, endOffset;
		EBrowserItem itemType = (EBrowserItem) GetGListByte(stream);
		if (itemType == browseEnd) break;
			
		// read standard data
		contribFile = GetGListWord(stream);
		srcFile = GetGListWord(stream);
		startOffset = GetGListLong(stream);
		endOffset = GetGListLong(stream);
		GetGListLong(stream);
		
		ReadString(stream, names, simpleName, 255);
		ReadString(stream, names, qualifiedName, MANGLED_LEN-1);
		
		if (srcFile==1 && !stricmp(name, simpleName))
			fFound = true;
		else if (srcFile==1 && qualifiedName[0] != 0)
		{
			MWUnmangle(qualifiedName, unmangleBuf, MANGLED_LEN);
			if (!stricmp(name, unmangleBuf))
				fFound = true;
		}
		
		switch(itemType)
		{
			case browseFunction:
				flags = GetGListLong(stream);
				localFuncID = GetGListLong(stream);
				if (fFound)
				{
				 	if (!fSearchStatics && (flags & kStatic))
						fFound = false;	
					else
					{
						*offset = startOffset;			
						*fIsCode = true;
					}
				}
				break;
				
			case browseGlobal:
				flags = GetGListLong(stream);
				if (fFound)
				{
					if (!fSearchStatics && (flags & kStatic))
						fFound = false;	
					else
					{
						*offset = startOffset;			
						*fIsCode = false;
					}
				}
				break;
				
			default:
				*offset = -1;
				goto error;
		}
	}
	error:
	
	return *offset != -1;
}

	// SearchBrowseDataForIdentifier
	//
	// Used when browser is inactive to support the Find Definition
	// command. Searchs the browser data handle stored in the project
	// for an identifier. Returns true if found, along with the
	// source offset and whether or not this is a code symbol.
	// Only returns static code/data symbols if fSearchStatics
	// is true.
	
Boolean	SearchBrowseDataForIdentifier(short objectID, 
			const char* name, Boolean fSearchStatics, 
			long* offset, Boolean* fIsCode)			
{
	Boolean 	fFound = false;
	Handle		browseData;
	Handle  	objectData = NULL;
	ObjHeader* 	header;
	char**		names = NULL;
	
//	UseMWResourceFile(projrefnum);
//	browseData = GetMWResource(BROWSEDATARSRC, objectID);
	if (browseData)
	{
		const BrowseHeader* hdr = *(BrowseHeader**) browseData;
		if (hdr->browse_header == BROWSE_HEADER &&
			hdr->browse_version == BROWSE_VERSION)
		{
			Boolean ok = true;
			if (hdr->uses_name_table)
			{
//				objectData = GetMWResource(OBJECTDATARSRC, objectID);
				if (objectData)
				{	// read the name table
					Ptr p;
					int i;
					long count = ((ObjHeader*)*objectData)->nametable_names;
					names = new char*[count];
//					names = (char**) NewPtr(sizeof(char*) * count);

//					HLockHi(objectData);
					header = (ObjHeader*) *objectData;
					
					p = (Ptr) header + header->nametable_offset;	
				    for (i = 1; i < count; i++) 
				    {
				        p += sizeof(short);
				        names[i] = p;
				        p+= strlen(p) + 1;
				    }
				}
				else
					ok = false;
			}
			if (ok)
			{
				GList stream;
				stream.data = browseData;
				stream.size = sizeof(BrowseHeader); // skip the header
//				stream.hndlsize = GetHandleSize(browseData);
//				stream.growsize = 0;
				*offset = -1;

				fFound = SearchForName(name, fSearchStatics, &stream, names,
							offset, fIsCode); 
			}
		}
//		ReleaseMWResource(browseData);
//		if (objectData) ReleaseMWResource(objectData);
//		if (names) DisposePtr((Ptr)names);
		
		delete names;
	}	
	return fFound;
}
