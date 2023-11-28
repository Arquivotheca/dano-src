// ===========================================================================
//	Utils.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Utils.h"
#include "MessageWindow.h"

#include <malloc.h>
#include <string.h>
#include <Debug.h>
#include <String.h>
#include <ctype.h>

/*
#if DEBUG
void debugASSERT(long mustBeNonZero, char *assertion, char *file, int line)
{
	if (!mustBeNonZero) {
		char str[1024];
		sprintf(str,"ASSERT FAILED: %s false in '%s' line %d",assertion,file,line);
		debugSTR(str);
	}
}
#endif

void debugSTR(char *str)
{
	//pprint(str);
	debugger(str);
}
*/


// ===========================================================================
//	String utils

//	Simple util to allocate a new string

char *SetStr(char *oldStr, const char *newStr)
{
	if (oldStr) free(oldStr);
	if (!newStr) return 0;
	if ((bool)(oldStr = (char *)malloc(strlen(newStr) + 1)))
		strcpy(oldStr,newStr);
	return oldStr;
}


// ===========================================================================
// ===========================================================================
//	Strings

void CropTrailingSpace(BString& string)
{
	int32 len = string.Length();
	if (len == 0)
		return;
	while (len && isspace(string[len - 1]))
		len--;

	string.Truncate(len);
}

void CropLeadingSpace(BString& string)
{
	if (string.Length() == 0)
		return;
	while (isspace(string[0]))
		string.Remove(0, 1);
}

// ===========================================================================
// ===========================================================================
//	Simple linkable class

CLinkable::CLinkable() : fNext(NULL), fPrevious(NULL)
{
}

CLinkable::~CLinkable()
{
	Remove();
}

//	Add new item to the end of the chain
//	Gets to be slow after a while

void CLinkable::Add(CLinkable *item)
{
	if (item == 0) return;
	Last()->AddAfter(item);
}

//	Add item after me

void CLinkable::AddAfter(CLinkable *item)
{
	if (item == 0) return;

	CLinkable *oldNext = fNext;
	CLinkable *first = item->First();

	fNext = first;
	first->fPrevious = this;
	if (oldNext) {
		CLinkable *last = item->Last();
		last->fNext = oldNext;
		oldNext->fPrevious = last;
	}
}

//	Get item at index

CLinkable *CLinkable::At(long index)
{
	CLinkable *item = First();
	long i;

	for (i = 0; i < index && item; i++)
		item = item->Next();
	return item;
}

//	Number of items in list

long CLinkable::Count() const
{
	long count = 0;
	CLinkable *item;
	for (item = First(); item; item = item->Next())
		count++;
	return count;
}

//	Delete everything in list

void CLinkable::DeleteAll()
{
	CLinkable *item;
	CLinkable *nextItem;

	for (item = First(); item; item = nextItem) {
		nextItem = item->Next();
		delete(item);
	}
}

//	Get the first item in this list

CLinkable *CLinkable::First() const
{
	CLinkable *item = (CLinkable *)this;

//	for (; item->Previous(); item = item->Previous());
	while (item->Previous())
		item = item->Previous();
	return item;
}

//	Get the last item in this list

CLinkable *CLinkable::Last() const
{
	CLinkable *item = (CLinkable *)this;

//	for (; item->Next(); item = item->Next());
	while (item->Next())
		item = item->Next();
	return item;
}

//	Remove from list

void CLinkable::Remove()
{
	if (fNext)
		fNext->fPrevious = fPrevious;
	if (fPrevious)
		fPrevious->fNext = fNext;
	fPrevious = fNext = 0;
}

//======================================================================================
//======================================================================================
//	CLinkedList object manages a list of CLinkable items


CLinkedList::CLinkedList() : mFirst(NULL),mLast(NULL)
{
}

CLinkedList::~CLinkedList()
{
	DeleteAll();
}

//	Add an item to the end of list by default
//	....much faster on huge documents

void CLinkedList::Add(CLinkable* item)
{
	if (mLast) {
		mLast->AddAfter(item);
		mLast = item;
	} else
		mFirst = mLast = item;
}

void CLinkedList::AddAfter(CLinkable* item, CLinkable* afterThis)
{
	afterThis->AddAfter(item);
	if (afterThis == mLast)			// Adding after last item
		mLast = item;
}

void CLinkedList::Delete(CLinkable* item)
{
	if (item == NULL)
		return;
	if (item == mFirst)
		mFirst = item->Next();
	if (item == mLast)
		mLast = item->Previous();
	delete item;
}

void CLinkedList::Remove(CLinkable* item)
{
	if (item == NULL)
		return;
	if (item == mFirst)
		mFirst = item->Next();
	if (item == mLast)
		mLast = item->Previous();
	item->Remove();
}

void CLinkedList::DeleteAll()
{
	if (mFirst) {
		mFirst->DeleteAll();
		mFirst = mLast = NULL;
	}
}
				
CLinkable*	CLinkedList::First() const
{
	return mFirst;
}

CLinkable*	CLinkedList::Last()
{
	return mLast;
}


//===========================================================================
//	Simple data bucket class, use instead of handles

CBucket::CBucket()
{
	mData = 0;
	mDataSpace = mDataCount = 0;
	mChunkSize = 16*1024L;			// Default allocation chunk for buckets are 16k
}

CBucket::CBucket(long chunkSize)
{
	mData = 0;
	mDataSpace = mDataCount = 0;
	mChunkSize = chunkSize;
}

CBucket::~CBucket()
{
	if (mData)
		free(mData);
}

bool	CBucket::AddData(const void *data, long count)	// Returns true if add was successful
{
//	NP_ASSERT(count >= 0);
	if (count <= 0 || !data)
		return true;
		
	if (mDataCount + count > mDataSpace) {
		while (mDataCount + count > mDataSpace)		// Reallocate and copy when required
			mDataSpace += mChunkSize;
		uchar *newData = (uchar *)realloc(mData,mDataSpace);
		if (newData == NULL) {
			pprintBig("CBucket::AddData: Failed to allocate %d bytes",mDataSpace);
			return false;
		}
		mData = newData;	// Check to see if it moved?
	}
	if (!mData)
		return false;
	memcpy(mData + mDataCount,data,count); // Can't use memcpy on huge
	mDataCount += count;
	return true;
}

long CBucket::GetCount()
{
	return mDataCount;
}

uchar* CBucket::GetData()
{
	return mData;
}

void CBucket::SetChunkSize(long chunkSize)
{
	mChunkSize = chunkSize;
}

void CBucket::Reset()
{
	if (mData)
		free(mData);
	mData = 0;
	mDataSpace = mDataCount = 0;
}

//	Once a bucket is complete, trim it so we don't waste memory

void CBucket::Trim()
{
	if (mData && mDataSpace != mDataCount) {
		mData = (uchar *)realloc(mData,mDataCount);
		mDataSpace = mDataCount;
	}
}

Counted::Counted()
{
	refcount = 1;
}

void Counted::Reference()
{
	atomic_add(&refcount, 1);
}

int32 Counted::Dereference()
{
	int32 newCount = atomic_add(&refcount, -1) - 1;
	if (newCount == 0)
		delete this;
	return newCount;
}

#ifdef R4_COMPATIBLE
// In R4, BString::Truncate had a nasty bug that caused corruption if you called Truncate with
// a length greater than the string length.  Let's paste in the corrected version here and tie
// it to the JavaScript version so we can build a JavaScript NetPositive build that will run on
// R4.
BString &
BString::Truncate(int32 newLength, bool lazy)
{
	_AssertNotUsingAsCString();
	if (_privateData && newLength < Length()) {
		if (!newLength) {
			free(_privateData - sizeof(int32));
			_privateData = 0;
			return *this;
		}
		if (!lazy) {
			if (newLength != Length()) {
				_privateData = (char *)realloc(_privateData ? _privateData - sizeof(int32)
					: 0, newLength + 1 + sizeof(int32));	
				if (!_privateData) 
					return *this;
				_privateData += sizeof(int32);
			}
		}
		_SetLength(newLength);
		_privateData[Length()] = '\0';
	}

	return *this;
}
#endif
/*
	Oops -- This isn't thread-safe.  Once it is, we should be able to use it.
	
void* PoolAlloc(void*& freeList, size_t allocationSize)
{
	if (freeList) {
		void* value = freeList;
		freeList = *((void **)freeList);
		return value;
	}
	const int numPerBlock = B_PAGE_SIZE / allocationSize;
//printf("Allocating a page to hold %ld blocks of %ld bytes\n", numPerBlock, allocationSize);
	freeList = malloc(allocationSize * numPerBlock);
	void *pointer = freeList;
	for (int x = 0; x < numPerBlock; x++) {
		if (x == numPerBlock - 1)
			*((void**)pointer) = 0;
		else
			*((void**)pointer) = ((char *)pointer) + allocationSize;
		pointer = (void *)(((char *)pointer) + allocationSize);
	}
	void* value = freeList;
	freeList = *((void **)freeList);
	return value;
}

void PoolFree(void*& freeList, void* ptr)
{
	*((void**)ptr) = freeList;
	freeList = ptr;
}
*/


/*
#define bogus_addr(x)  ((x) < 0x80000000)

static unsigned long
GetCallerAddress(int level)
{
#if __INTEL__
	ulong fp = 0, nfp, ret = 0;

	level += 2;
	
	fp = (ulong)get_stack_frame();
	if (bogus_addr(fp))
		return 0;
	nfp = *(ulong *)fp;
	while (nfp && --level > 0) {
		if (bogus_addr(fp))
			return 0;
		nfp = *(ulong *)fp;
		ret = *(ulong *)(fp + 4);
		if (bogus_addr(ret))
			break;
		fp = nfp;
	}

	return ret;
#else
	return 0;
#endif
}


*/

#ifdef DEBUGMENU

extern "C" void __pure_virtual()
{
	printf("Pure virtual method called\n");
	debugger("Pure virtual method called");
}


TLocker::TLocker()
{
}

TLocker::TLocker(const char *name)
	: BLocker(name)
{
}

bool TLocker::Lock()
{
	bool wasLocked = IsLocked();
	
	bool retval = BLocker::Lock();

	if (retval && !wasLocked) {
		for (int i = 0; i < 8; i++)
//			mCallStack[i] = GetCallerAddress(i);
			mCallStack[i] = 0;
	}
	return retval;
}

status_t TLocker::LockWithTimeout(bigtime_t timeout)
{
	bool wasLocked = IsLocked();
	status_t retval = BLocker::LockWithTimeout(timeout);
	if (retval == B_OK && !wasLocked) {
		for (int i = 0; i < 8; i++)
//			mCallStack[i] = GetCallerAddress(i);
			mCallStack[i] = 0;
	} else if (retval == B_TIMED_OUT) {
		printf("LockWithTimeout timed out in thread %ld.\n", find_thread(NULL));
		printf("TLocker 0x%x is held by thread %ld, call stack 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
			(unsigned int)this, LockingThread(), mCallStack[0], mCallStack[1], mCallStack[2], mCallStack[3],
			mCallStack[4], mCallStack[5], mCallStack[6], mCallStack[7]);
	}
	return retval;
}

void TLocker::DumpLockingCallStack()
{
	if (IsLocked())
		printf("TLocker 0x%x is held by thread %ld, call stack 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n",
			(unsigned int)this, LockingThread(), mCallStack[0], mCallStack[1], mCallStack[2], mCallStack[3],
			mCallStack[4], mCallStack[5], mCallStack[6], mCallStack[7]);
	else
		printf("TLocker 0x%x not locked\n", (unsigned int)this);
}

void TLocker::Unlock()
{
	if (!IsLocked())
		debugger("Locker is not locked");
		
	if (LockingThread() != find_thread(NULL)) {
		printf("TLocker not being unlocked from locking thread\n");
		DumpLockingCallStack();
		debugger("TLocker not being unlocked from locking thread");
	}
	BLocker::Unlock();
}
#endif
