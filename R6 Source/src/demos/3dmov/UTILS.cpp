// ===========================================================================
//	Utils.cpp
// 	й1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "UTILS.H"

// ===========================================================================
//	Handy rectangle class

OrdRect::OrdRect()
{
}

OrdRect::OrdRect(int l,int t,int r,int b)
{
	top = t;
	left = l;
	bottom = b;
	right = r;
}

void OrdRect::Set(int l,int t,int r,int b)
{
	top = t;
	left = l;
	bottom = b;
	right = r;
}

Ord OrdRect::Width()
{
	return right - left;
}

Ord OrdRect::Height()
{
	return bottom - top;
}

bool OrdRect::Empty()
{
	return Width() == 0 && Height() == 0;
}

void OrdRect::OffsetBy(int x, int y)
{
	top += y;
	bottom += y;
	left += x;
	right += x;
}

void OrdRect::OffsetTo(int x, int y)
{
	OffsetBy(x - left, y - top);
}

void OrdRect::InsetBy(int x, int y)
{
	top += y;
	bottom -= y;
	left += x;
	right -= x;
}

bool OrdRect::Intersects(OrdRect &r)
{
	if (right < r.left) return false;
	if (bottom < r.top) return false;
	if (left > r.right) return false;
	if (top > r.bottom)  return false;
	return true;
}

bool OrdRect::ContainsPoint(int x, int y)
{
	if (x < left) return false;
	if (y < top) return false;
	if (x > right) return false;
	if (y > bottom)  return false;
	return true;
}

// ===========================================================================
//	String utils

//	Convert a string to upper case

void upstr(char *str)
{
	while (*str) {
		char c = toupper(*str);
		*str++ = c;
	}
}

//	Convert a string to lower case

void lostr(char *str)
{
	while (*str) {
		char c = tolower(*str);
		*str++ = c;
	}
}

//	Simple util to allocate a new string

char *SetStr(char *oldStr, char *newStr)
{
	if (oldStr) FREE(oldStr);
	if (!newStr) return 0;
	if (oldStr = (char *)MALLOC(strlen(newStr) + 1))
		strcpy(oldStr,newStr);
	return oldStr;
}


//	Handy string utils

char*  SkipChars(const char* str, const char* skip)
{
	NP_ASSERT(str);
	NP_ASSERT(skip);
	while (str[0] && strchr(skip, str[0]))
		str++;
	return (char *)str;
}

char*  SkipStr(const char* str, const char* skip)
{
	NP_ASSERT(str);
	NP_ASSERT(skip);
	
	int i = strlen(skip);
	if (strncmp(skip,str,i) == 0)
		return (char *)(str + i);
	return NULL;
}

// ===========================================================================
// ===========================================================================
//	Root object for NetPositive Objects

#ifndef NPOBJECT

void NPObject::DumpAll()
{
	pprint("No Object info (Compiled without NPOBJECT)");
}

void NPMemory::DumpAll()
{
	pprint("No Memory info (Compiled without NPOBJECT)");
}

#else

// ===========================================================================
//	Maintain a list of all NPObject objects

ObjHeader*	NPObject::mList = NULL;
long		NPObject::mCount = 0;

#define OBJHDRSIZE (sizeof(ObjHeader) - 4)

NPObject::~NPObject()
{
}

//	Dump list of objects

void NPObject::DumpAll()
{
	ObjHeader* obj = mList;
	pprint("\nDumpObjects (%d Objects)",mCount);
	int i = 0;
	long size = 0;
	while (obj) {
		char *name = GetObjName(obj);
		char *info = GetObjInfo(obj);
		if (info)
			pprint("0x%8x %s %s",(long )((char*)obj + OBJHDRSIZE),name,info);
		else
			pprint("0x%8x %s",(long )((char*)obj + OBJHDRSIZE),name);
		size += obj->size;
		obj = obj->next;
		i++;
	}
	NP_ASSERT(mCount == i);
	pprint("%d Objects (%d bytes)",mCount,size);
}

//	Return a info string that identifies object in dump

const char* NPObject::GetInfo()
{
	return NULL;
}

//	Return the class name of the object

const char* NPObject::GetType()
{
	return GetObjName((ObjHeader*)((char*)this - OBJHDRSIZE));
}

//	Check that is object is alive and well

Boolean NPObject::Valid(void *obj)
{
	if (obj == NULL)
		return false;
		
	ObjHeader* o = (ObjHeader*)((char *)obj - OBJHDRSIZE);
	return (o->signature == 'HERE' && o->size > 0);
}

//	override new and delete to log objects

void* NPObject::operator new(size_t size)
{
	NPMemory::HeapCheck();
	NP_ASSERT(size > 0);
	void *obj = ::operator new(size + OBJHDRSIZE);
	RememberObject((ObjHeader*)obj,size);
	return (char *)obj + OBJHDRSIZE;
}

void NPObject::operator delete(void* obj)
{
	NPMemory::HeapCheck();
	if (obj == NULL)
		return;
		
//	Check for object validity before deleting it

	NP_ASSERT(Valid(obj));
	
	ObjHeader* o = (ObjHeader*)((char *)obj - OBJHDRSIZE);
	ForgetObject(o);
	::operator delete(o);
}

//	Get name of object from RTTI info, if present

const char *NPObject::GetObjName(ObjHeader* obj)
{
	if (obj->name != 0)
		if (*obj->name != 0)
			if (**obj->name)
				return **obj->name;	// Found a name string
	return "Unknown";
}

//	Get info from object

const char *NPObject::GetObjInfo(ObjHeader* obj)
{
	NPObject *o = (NPObject *)((char *)obj + OBJHDRSIZE);
	return o->GetInfo();
}

//	Add object to list

void NPObject::RememberObject(ObjHeader* obj, long size)
{
	obj->prev = NULL;
	if (mList)
		mList->prev = obj;
	obj->next = mList;
	obj->size = size;
	obj->signature = 'HERE';
	mList = obj;						// Place at start of list
	mCount++;
}

//	Delete object from list

void NPObject::ForgetObject(ObjHeader* obj)
{
	if (mList == obj)
		mList = obj->next;
	if (obj->next)
		obj->next->prev = obj->prev;	// Not last in list
	if (obj->prev)
		obj->prev->next = obj->next;	// Not first in list
	
	memset((char *)obj + OBJHDRSIZE,'G',obj->size);
	obj->signature = 'GONE';
	--mCount;
}

// ===========================================================================
//	Debug versions of malloc, realloc and free


#define MEMHDRSIZE sizeof(MemHeader)

MemHeader*	NPMemory::mList = NULL;
long		NPMemory::mCount = 0;

void NPMemory::DumpAll()
{
	MemHeader* mem = mList;
	long size = 0;
	pprint("\nDumpMemory, %d Blocks",mCount);
	int i = 0;

	while (mem) {
		pprint("0x%8x %s line %d:%d",(long)((char*)mem + MEMHDRSIZE),mem->file,mem->line,mem->size);
		size += mem->size;
		mem = mem->next;
		i++;
	}
	
	NP_ASSERT(mCount == i);
	pprint("%d Blocks (%d bytes)",mCount,size);
}

Boolean NPMemory::Valid(void *mem)
{
	if (mem == NULL)
		return false;
		
	MemHeader* m = (MemHeader*)((char *)mem - MEMHDRSIZE);
	return (m->signature == 'HERE' && m->size > 0);
}

void NPMemory::ForgetMem(MemHeader* mem)
{
	if (mList == mem) {
		if (mem->next)
			NP_ASSERT(NPMemory::Valid((char*)mem->next + MEMHDRSIZE));
		mList = mem->next;
	}
	if (mem->next)
		mem->next->prev = mem->prev;	// Not last in list
	if (mem->prev)
		mem->prev->next = mem->next;	// Not first in list
	
	memset((char *)mem + MEMHDRSIZE,'G',mem->size);
	mem->signature = 'GONE';
	--mCount;
}

void NPMemory::RememberMem(MemHeader* mem, long size, char* file, int line)
{
	mem->prev = NULL;
	if (mList)
		mList->prev = mem;
	mem->next = mList;
	mem->size = size;
	strcpy(mem->file,file);
	mem->line = line;
	mem->signature = 'HERE';
	mList = mem;						// Place at start of list
	mCount++;
}

//	Check heap

void NPMemory::HeapCheck()
{
	return;//ееееееееееееееееееее
	
	if (gPlatform) gPlatform->MemoryLock(true);	
	MemHeader* m = mList;
	while (m) {
		NP_ASSERT(NPMemory::Valid((char*)m + MEMHDRSIZE));
		m = m->next;
	}
	if (gPlatform) gPlatform->MemoryLock(false);	
}

void *NPMemory::Malloc(long size, char *file, int line)
{
	HeapCheck();
	
	if (size == 0) {
		pprint("NPMemory::Malloc: Requested Zero bytes at '%s' line %d",file,line);
		return NULL;
	}
	
	if (gPlatform) gPlatform->MemoryLock(true);	
	char *mem = (char *)malloc(size + MEMHDRSIZE);
	NP_ASSERT(mem && "NPMemory::Malloc failed");
	RememberMem((MemHeader*)mem,size,file,line);
	if (gPlatform) gPlatform->MemoryLock(false);	

	HeapCheck();
	return mem + MEMHDRSIZE;
}

void NPMemory::Free(void* mem, char *file, int line)
{
	HeapCheck();
	
//	Check for mem validity before freeing it

	if (!Valid(mem)) {
		char str[128];
		sprintf(str,"NPMemory::FREE: Bad free at '%s' line '%d'",file,line);
		NP_DEBUGSTR(str);
		return;
	}
	
	if (gPlatform) gPlatform->MemoryLock(true);	
	MemHeader* m = (MemHeader*)((char*)mem - MEMHDRSIZE);
	ForgetMem(m);
	free(m);
	if (gPlatform) gPlatform->MemoryLock(false);	

	HeapCheck();
}

void *NPMemory::Realloc(void* mem, long size, char *file, int line)
{
	HeapCheck();

	if (mem == NULL)
		return Malloc(size,file,line);
	
	if (gPlatform) gPlatform->MemoryLock(true);	
	MemHeader* m = (MemHeader*)((char*)mem - MEMHDRSIZE);
	MemHeader* prev = m->prev;
	MemHeader* next = m->next;
	
	m = (MemHeader*)realloc(m,size + MEMHDRSIZE);
	NP_ASSERT(m);
	m->size = size;
	if (prev)
		prev->next = m;
	else
		mList = m;
	if (next)
		next->prev = m;
	if (gPlatform) gPlatform->MemoryLock(false);	

	HeapCheck();
	return (char *)m + MEMHDRSIZE;
}

#endif


// ===========================================================================
// ===========================================================================
//	Strings

CString::CString() : mStr(NULL), mLength(0)
{
}

CString::CString(const char* str)
{
	if (str) {
		mLength = strlen(str);
		mStr = (char*)MALLOC(mLength + 1);
		NP_ASSERT(mStr);
		strcpy(mStr,str);
	} else {
		mStr = NULL;
		mLength = 0;
	}
}

CString::CString(const CString& s)
{
	if (s.mStr) {
		mLength = strlen(s.mStr);
		mStr = (char*)MALLOC(mLength + 1);
		NP_ASSERT(mStr);
		strcpy(mStr,s.mStr);
	} else {
		mLength = 0;
		mStr = NULL;
	}
}

CString& CString::operator=(const CString& s)
{
	if (mStr) {
		FREE(mStr);
		mStr = NULL;
	}
	mLength = 0;
	if (s.mStr != NULL) {
		mLength = strlen(s.mStr);
		mStr = (char *)MALLOC(mLength + 1);
		NP_ASSERT(mStr);
		strcpy(mStr,s.mStr);
	}
	return *this;
}

int CString::Length()
{
	return mLength;
}

void CString::Lowercase()
{
	if (mStr == NULL)
		return;
	char *s = mStr;
	while (s[0]) {
		s[0] = tolower(s[0]);
		s++;
	}
}

void CString::Uppercase()
{
	if (mStr == NULL)
		return;
	char *s = mStr;
	while (s[0]) {
		s[0] = toupper(s[0]);
		s++;
	}
}

CString& CString::operator+=(const CString& str)
{
	if (str.mLength == 0)
		return *this;
	mLength += str.mLength;
	char *s = (char *)MALLOC(mLength + 1);
	NP_ASSERT(s);
	if (mStr) {
		strcpy(s,mStr);
		strcat(s,str.mStr);
		FREE(mStr);
	} else
		strcpy(s,str.mStr);
	mStr = s;
	return *this;
}

CString& CString::operator=(const char* str)
{
	if (mStr) {
		FREE(mStr);
		mStr = NULL;
	}
	mLength = 0;
	if (str == NULL || str[0] == 0)
		return *this;
		
	mLength = strlen(str);
	mStr = (char *)MALLOC(mLength + 1);
	NP_ASSERT(mStr);
	strcpy(mStr,str);
	return *this;
}

void CString::Set(const char* str, int count)
{
	mLength = count;
	if (mStr) {
		FREE(mStr);
		mStr = NULL;
	}
	if (count) {
		mStr = (char *)MALLOC(count + 1);
		NP_ASSERT(mStr);
		memcpy(mStr,str,count);
		mStr[count] = 0;
	}
}

void CString::CropTrailingSpace()
{
	if (mStr == NULL)
		return;
		
	int j;
	for (j = strlen(mStr); j; --j)
		if (isspace(mStr[j]))
			mStr[j] = '\0';
		else
			break;
	mLength = strlen(mStr);
	if (mLength == 0) {
		FREE(mStr);
		mStr = NULL;
	}
}
	

CString::~CString()
{
	if (mStr)
		FREE(mStr);
}

#ifdef NPOBJECT
const char*	CString::GetInfo()		// Info to help track object
{
	if (mStr == NULL)
		return "(null)";
	else
		return (const char*)mStr;
}
#endif

// ===========================================================================
//	Simple list class

CList::CList()
{
	mCount = mListDataSize = 0;
	mListData = 0;
}

CList::~CList()
{
	if (mListData)
		FREE(mListData);
}

//	Make suer there is enough room in the list

void CList::AllocateMore()
{
	if (mCount < (mListDataSize >> 2))
		return;
	mListDataSize += 1024;
	mListData = (long *)REALLOC(mListData,mListDataSize);
}

//	Return an item if one exists

long	CList::Get(long index)
{
	if (index < 0 || index >= mCount)
		return 0;
	return mListData[index];
}

//	Set an item that is already in the list

void CList::Set(long index, long item)
{
	NP_ASSERT(index >= 0 && index < mCount);
	mListData[index] = item;
}

//	Add an item to the end of the list

void CList::Add(long item)
{
	long	index = mCount++;
	AllocateMore();
	mListData[index] = item;
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

long CLinkable::Count()
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

CLinkable *CLinkable::First()
{
	CLinkable *item = (CLinkable *)this;

	for (; item->Previous(); item = item->Previous());
	return item;
}

//	Get the last item in this list

CLinkable *CLinkable::Last()
{
	CLinkable *item = (CLinkable *)this;

	for (; item->Next(); item = item->Next());
	return item;
}

CLinkable *CLinkable::Next()
{
	return fNext;
}

CLinkable *CLinkable::Previous()
{
	return fPrevious;
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

void CLinkedList::DeleteAll()
{
	if (mFirst) {
		mFirst->DeleteAll();
		mFirst = mLast = NULL;
	}
}
				
CLinkable*	CLinkedList::First()
{
	return mFirst;
}

CLinkable*	CLinkedList::Last()
{
	return mLast;
}

//======================================================================================

//===========================================================================
//	Simple data bucket class, use instead of handles

CBucket::CBucket()
{
	mData = 0;
	mDataSpace = mDataCount = 0;
	mChunkSize = 16*1024L;			// Default allocation chunk for buckets are 16k
}

CBucket::~CBucket()
{
	if (mData)
		FREE(mData);
}

Boolean	CBucket::AddData(void huge *data, long count)	// Returns true if add was successful
{
	NP_ASSERT(count >= 0);
	if (count == 0)
		return true;
		
	if (mDataCount + count > mDataSpace) {
		while (mDataCount + count > mDataSpace)		// Reallocate and copy when required
			mDataSpace += mChunkSize;
		Byte huge *newData = (Byte huge *)REALLOC(mData,mDataSpace);
		if (newData == nil) {
			pprint("CBucket::AddData: Failed to allocate %d bytes",mDataSpace);
			return false;
		}
		mData = newData;	// Check to see if it moved?
	}
	Byte huge *ff = (Byte huge *)data;
	for (long f = 0; f < count; f++)
		mData[f+mDataCount] = ff[f];
//	memcpy(mData + mDataCount,data,count); // Can't use memcpy on huge
	mDataCount += count;
	return true;
}

long CBucket::GetCount()
{
	return mDataCount;
}

Byte huge* CBucket::GetData()
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
		FREE(mData);
	mData = 0;
	mDataSpace = mDataCount = 0;
}

//	Once a bucket is complete, trim it so we don't waste memory

void CBucket::Trim()
{
	if (mData) {
		mData = (Byte huge*)REALLOC(mData,mDataCount);
		mDataSpace = mDataCount;
	}
}
