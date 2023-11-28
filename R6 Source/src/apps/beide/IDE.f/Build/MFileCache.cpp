//========================================================================
//	MFileCache.cpp
//	Copyright 1997Metrowerks Corporation, All Rights Reserved.
//========================================================================
//	Implements a cache for header and source files.  Uses the MSL pool_alloc functions to manage
//	the memory pool in the area.  There is a single lock around all functions that may change the
//	pool.
//	BDS

#include "IDEConstants.h"
#include "MFileCache.h"
#include "pool_alloc.h"

#include <Debug.h>

// These are dummy functions
// they're required by pool_alloc but they're never called
void *	__sys_alloc(mem_size) { ASSERT(false); return 0; }
void		__sys_free(void *) { ASSERT(false); }

// ---------------------------------------------------------------------------
//		MFileCache
// ---------------------------------------------------------------------------
//	Constructor

MFileCache::MFileCache(
	int32	inSize,
	MFileGetter&	inGetter)
	: fGetter(inGetter),
	fLock("filecache")
{
	fArea = -1;
	fAreaSize = inSize;

	BuildHeap(inSize);
}

// ---------------------------------------------------------------------------
//		~MFileCache
// ---------------------------------------------------------------------------
//	Destructor

MFileCache::~MFileCache()
{
	KillHeap();
}

// ---------------------------------------------------------------------------
//		FileHeader
// ---------------------------------------------------------------------------

inline SharedFileRec*
MFileCache::FileHeader(
	int32		inOffset)
{
	return (SharedFileRec*) ((char*) fAreaBase + inOffset);
}

// ---------------------------------------------------------------------------
//		GetFile
// ---------------------------------------------------------------------------
//	Return a filehandle for the specified file.  If the file is already
//	in the cache just get its offset.  If not yet in the cache
//	have the filegetter read the file into a new block.

status_t
MFileCache::GetFile(
	FileHandle&		ioFileHandle,
	const char *	inFileName,
	bool			inSystemTree)
{
	// inSystemTree here is the value requested by the #include dierective
	// and this could be incorrect.  we should use the correct value
	if (fArea <= B_ERROR)
	{
		return B_ERROR;
	}

	Lock();
	
	status_t	result;
	int32		index;
	bool		found = fFileList.FindItem(inFileName, inSystemTree, index);
	// need to save files by their actual tree and not their requested tree
	// Is it already in the cache?
	if (found)
	{
		// Find the offset
		AreaFileRec*	rec = fFileList.ItemAtFast(index);
		int32			offset = rec->offset;		
		
		// Move the most recently used file to the end of the list
		bool	gone = fLRUList.RemoveItem(offset);
		fLRUList.AddItem(offset);

		// Increment the use count
		rec->usecount++;

		// Build the response
		ioFileHandle.id = fArea;
		ioFileHandle.offset = offset;
		result = B_NO_ERROR;
	}
	else
	{
		// Need to add the file to the cache
		off_t	size = fGetter.FileSize(inFileName, inSystemTree);

		if (size <= 0)
			result = B_FILE_NOT_FOUND;
		else
		{
			// There is a potential race condition for the file size here so allocate
			// a bit more than the filegetter asked for.
			size += 24;
			int32	blockSize = size + sizeof(SharedFileRec);
			SharedFileRec*	header = GetBlock(blockSize);
			
			if (header)
			{
				void*	block = (char*) header + sizeof(SharedFileRec);
				int32	filesize = size;

				if (B_NO_ERROR == fGetter.WriteFileToBlock(block, size, inFileName, inSystemTree))
				{
					int32			offset = (uint32) header - (uint32) fAreaBase;
					
					// Add a record to the file list
					AreaFileRec*	rec = new AreaFileRec;
					
					rec->offset = offset;
					rec->usecount = 1;
					rec->systemtree = inSystemTree;
					strncpy(rec->name, inFileName, sizeof(rec->name));
					fFileList.AddItem(rec);
					
					// Add the offset to the lru list at the end
					fLRUList.AddItem(offset);

					// Increment the use count and other header fields
					header->length = filesize;
					header->systemtree = inSystemTree;
					strncpy(header->name, inFileName, sizeof(header->name));

					// Build the response
					ioFileHandle.id = fArea;
					ioFileHandle.offset = offset;
					result = B_NO_ERROR;
				}
				else
				{
					__pool_free(&fPoolObject, header);
					ioFileHandle.id = -1;
					result = B_ERROR;
				}
			}
		}	
	}
	
	Unlock();
	
	return result;
}

// ---------------------------------------------------------------------------
//		GetBlock
// ---------------------------------------------------------------------------
//	Try to get a block from the pool.  If we can't get a block on the first
//	try then purge blocks from the pool in lru order until we've either
//	found some space or purged all the blocks that aren't in use.

SharedFileRec*
MFileCache::GetBlock(
	int32		inSize)
{
	SharedFileRec*	rec = (SharedFileRec*) __pool_alloc(&fPoolObject, inSize);

	if (rec == nil && inSize < fAreaSize)
	{
		// Purge records in lru order if they're not in use
		int32	offset;
		int32	i = 0;
		
		while (rec == nil && fLRUList.GetNthItem(offset, i))
		{
			SharedFileRec*		header = FileHeader(offset);
			int32				index;
			
			if (fFileList.FindItem(header->name, header->systemtree, index))
			{
				AreaFileRec*	areaRec = fFileList.ItemAtFast(index);
				
				if (areaRec->usecount <= 0)
				{
					__pool_free(&fPoolObject, header);
					fLRUList.RemoveItemAt(i);
					fFileList.RemoveItemAt(index);
				
					rec = (SharedFileRec*) __pool_alloc(&fPoolObject, inSize);
				}
				else
					i++;	// Increment if we didn't purge this record
			}
			else
				i++;		// file wasn't found ??
		}
	}

	return rec;
}

// ---------------------------------------------------------------------------
//		DoneWithFile
// ---------------------------------------------------------------------------
//	Called when a file is no longer in use.

void
MFileCache::DoneWithFile(
	const FileHandle&		ioFileHandle)
{
	Lock();

	SharedFileRec*		header = FileHeader(ioFileHandle.offset);
	int32				index;
	
	if (fFileList.FindItem(header->name, header->systemtree, index))
	{
		// Find the offset
		AreaFileRec*	rec = fFileList.ItemAtFast(index);
		rec->usecount--;
	}

	Unlock();
}

// ---------------------------------------------------------------------------
//		BuildHeap
// ---------------------------------------------------------------------------
//	Create an area and initialize it as a pool.

void
MFileCache::BuildHeap(	
	int32	inSize)
{
	void*			address = (void*) B_PAGE_SIZE;
	const int32		size = (inSize / B_PAGE_SIZE) * B_PAGE_SIZE; 

	fArea = create_area("filecache", &address, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	fAreaBase = address;

	if (fArea > B_ERROR)
	{
		__init_pool_obj(&fPoolObject);
		__pool_preassign(&fPoolObject, address, size);
	}
}

// ---------------------------------------------------------------------------
//		KillHeap
// ---------------------------------------------------------------------------
//	Delete the area.

void
MFileCache::KillHeap()
{
	delete_area(fArea);
	fArea = -1;
}
