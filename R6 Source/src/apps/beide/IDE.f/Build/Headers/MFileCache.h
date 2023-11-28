//========================================================================
//	MFileCache.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MFILECACHE_H
#define _MFILECACHE_H

#include "pool_alloc.h"
#include "MList.h"
#include "MAreaFileList.h"
#include "MFileGetter.h"
#include "BeIDEComm.h"

#include <Locker.h>

struct AreaFileRec
{
	int32		offset;
	int32		usecount;
	char		name[256];
	bool		systemtree;
};


class MFileCache
{
public:
 								MFileCache(
 									int32			inSize,
 									MFileGetter&	inGetter); 
 								~MFileCache();

		status_t				GetFile(	
									FileHandle&		ioFileHandle,
									const char *	inFileName,
									bool			inSystemTree);

		void					DoneWithFile(
									const FileHandle&	ioFileHandle);

private:
	
		area_id					fArea;
		int32					fAreaSize;
		void*					fAreaBase;
		MFileGetter&			fGetter;
		mem_pool_obj			fPoolObject;
		MAreaFileList			fFileList;
		MList<int32>				fLRUList;
		BLocker					fLock;
		

		SharedFileRec*			FileHeader(
									int32		inOffset);

		void					BuildHeap(
									int32	inSize);
		void					KillHeap();
		SharedFileRec*			GetBlock(
									int32		inSize);

		void					Lock();
		void					Unlock();
};

inline void MFileCache::Lock()
{
	fLock.Lock();
}
inline void MFileCache::Unlock()
{
	fLock.Unlock();
}

#endif
