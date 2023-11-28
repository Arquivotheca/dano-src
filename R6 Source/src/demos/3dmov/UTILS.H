
// ===========================================================================
//	Utils.h
// 	©1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __UTILS__
#define __UTILS__

#include <SupportDefs.h>
#include <Midi.h>
#include <File.h>
#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define huge

#define BEBOX

//	Types used

#ifdef BEBOX
#define Boolean bool
#define nil 0
#define false 0
#define true 1
#define OSErr short
#define noErr 0
#define OSType long

//#include "MessageWindow.h"	// pprint
//#ifndef pprint
#define pprint printf
//#endif

typedef unsigned char Byte;
#endif


//===========================================================================
//	Common utils

typedef long Ord;		// Ordinate
typedef long Position;	// Simple now, will get more complex

// ===========================================================================
//	Handy rectangle class

class OrdRect {
public:
			OrdRect();
			OrdRect(int l,int t,int r,int b);
			
	Ord		Width();
	Ord		Height();

	void	Set(int l,int t,int r,int b);
	void	OffsetBy(int x, int y);
	void	OffsetTo(int x, int y);
	void	InsetBy(int x, int y);
	
	bool	Empty();
	bool	Intersects(OrdRect &r);
	bool	ContainsPoint(int x, int y);
	
	Ord		top;
	Ord		left;
	Ord		bottom;
	Ord		right;
};

// ===========================================================================

#ifndef ABS
#define ABS(_x) ((_x) < 0 ? -(_x) : (_x))
#endif

#ifndef MIN
#define MIN(_x,_y) (((_x) < (_y)) ? (_x) : (_y))
#endif

#ifndef MAX
#define MAX(_x,_y) (((_x) > (_y)) ? (_x) : (_y))
#endif

#if __POWERPC__
#define NP_ASSERT(_x) debugASSERT((long)(_x),#_x,__FILE__,__LINE__);
void 	debugASSERT(long mustBeNonZero, char *assertion, char *file, int line);
#else
#define NP_ASSERT(_x)
#endif

#define NP_DEBUGSTR(_x) debugSTR(_x)
void 	debugSTR(char* str);

void upstr(char *str);						//	Convert a string to upper case
void lostr(char *str);						//	Convert a string to lower case
char *SetStr(char *oldStr, char *newStr);	//	Simple util to allocate a new string
											//	First string is memory based, second isn't
											
char*  SkipChars(const char* str, const char* skip);	// Skip a list of defined chars
char*  SkipStr(const char* str, const char* skip);		// Skip a string if found

// ===========================================================================
//	Root object for NetPositive Objects, memory tracking

//#define NPOBJECT

#ifndef NPOBJECT

class NPObject {			// No RTTI, no VTables
public:
static	void	DumpAll();
static  Boolean	Valid(void *obj) { return obj != NULL; };
};

class NPMemory {
public:
static	void	DumpAll();
static  Boolean	Valid(void *mem) { return mem != NULL; };
};

#define MALLOC malloc		// No memory tracking
#define FREE free
#define REALLOC realloc
#define MEMCPY	memcpy

#else

struct ObjHeader {						//	Link objects together, extract RTTI name info
	ObjHeader*	next;
	ObjHeader*	prev;
	long		signature;
	long		size;
	char***		name;
};
typedef struct ObjHeader ObjHeader;


//	All Netpositive objects descend from NPObject in debug builds

class NPObject {
public:

virtual				~NPObject();
static	void		DumpAll();			// Dump list of NPObject objects
static	bool		Valid(void *obj);	// Check that an object is alive and well

virtual	const char*	GetInfo();			// Virtual method means VTables for all NPObjects
		const char*	GetType();

		void*	operator	new(size_t size);
		void	operator	delete(void* obj);
		
protected:

static	const char*	GetObjName(ObjHeader* obj);
static	const char*	GetObjInfo(ObjHeader* obj);

static	void		RememberObject(ObjHeader* obj,long size);
static	void		ForgetObject(ObjHeader* obj);
		
static	ObjHeader*	mList;
static	long		mCount;

};

// ===========================================================================
//	Debug versions of malloc, realloc and free

#define MALLOC(_size) 			NPMemory::Malloc(_size,__FILE__,__LINE__)
#define FREE(_mem) 				NPMemory::Free(_mem,__FILE__,__LINE__)
#define REALLOC(_mem,_size) 	NPMemory::Realloc(_mem,_size,__FILE__,__LINE__)
#define MEMCPY(_dst,_src,_size)	memcpy(_dst,_src,_size)

//	Remembers where block was allocated

struct MemHeader {
	MemHeader*	prev;
	MemHeader*	next;
	long		signature;
	long		size;
	char		file[32];
	int			line;
};
typedef struct MemHeader MemHeader;


class NPMemory {
public:

static	void	DumpAll();
static	bool	Valid(void *mem);

static	void	ForgetMem(MemHeader* mem);
static	void	RememberMem(MemHeader* mem, long size, char* file, int line);

static	void	HeapCheck();
static	void*	Malloc(long size, char *file, int line);
static	void	Free(void* mem, char *file, int line);
static	void*	Realloc(void* mem, long size, char *file, int line);

protected:

static	MemHeader*	mList;
static	long		mCount;

};

#endif

// ===========================================================================
//	Infinite array class

template<class TYPE, int SIZE>
class CArray : public NPObject {
public:
			CArray() : mNext(NULL)
			{
				memset(mData,0,sizeof(mData));
			}
			
			~CArray()
			{
				delete mNext;
			}
			
	TYPE&	operator[](int index)
			{
				//NP_ASSERT(index >= 0 && index < 100000);
				if (index < SIZE)
					return mData[index];
				index -= SIZE;
				if (mNext == NULL)
					mNext = new CArray<TYPE,SIZE>;
				return (*mNext)[index];
			}
	
protected:
	TYPE				mData[SIZE];
	CArray<TYPE,SIZE>*	mNext;
};

//	Dynamic list

template<class TYPE, int SIZE>
class CDynList : public NPObject {
public:
			CDynList() : mCount(0) {};
		
	void	Add(TYPE item)
	{
		mItems[mCount] = item;
		mCount++;
	};
	
	void	Delete(TYPE item)
	{
		int i;
		for (i = 0; i < mCount; i++)
			if (item == mItems[i])
				break;
		NP_ASSERT(i < mCount);
		mCount--;
		for (;i < mCount; i++)
			mItems[i] = mItems[i+1];
	}
	
	TYPE&	Get(int index)
	{
		NP_ASSERT(index < mCount);
		return mItems[index];
	}
	
	int		Count()
	{
		return mCount;
	};
		
protected:
	CArray<TYPE,SIZE> mItems;
	int mCount;
};


// ===========================================================================
//	Dreaded CString class

class CString : public NPObject {
public:
			CString();
			CString(const char* str);
			CString(const CString& s);
			
			~CString();
			
int			Length();
void		Lowercase();
void		Uppercase();

CString&	operator+=(const CString& str);

CString&	operator=(const CString& s);
CString&	operator=(const char* str);

			operator const char*() const { return mStr; };
			
void		Set(const char* str, int count);
void		CropTrailingSpace();


#ifdef NPOBJECT
virtual	const char*	GetInfo();		// Info to help track object
#endif

protected:
	char*	mStr;
	int		mLength;
};

//===========================================================================
//	Simple list class

class CList : public NPObject {
public:
		CList();
virtual	~CList();

		virtual	long	GetCount() { return mCount; };
		virtual long	Get(long index);
		virtual	void	Set(long index, long item);
		virtual	void	Add(long item);

protected:
		virtual	void	AllocateMore();

		long	mCount;
		long*	mListData;
		long	mListDataSize;	// May be allocated in chunks
};

//===========================================================================
//	Simple linkable class

class CLinkable : public NPObject {
public:
							CLinkable();
					virtual	~CLinkable();
public:

	CLinkable*				At(long index);
	long					Count();
	CLinkable*				First();
	CLinkable*				Last();
	CLinkable*				Next();
	CLinkable*				Previous();

	void					AddAfter(CLinkable *item);
	void					DeleteAll();

	void					Add(CLinkable *item);
protected:
	void					Remove();

	CLinkable*	fNext;
	CLinkable*	fPrevious;
};

#define AddOrSet(_list,_item) { if (_list) _list->Add(_item); else _list = (_item);}

//======================================================================================
//	CLinkList object manages a list of CLinkable items

class CLinkedList : public NPObject {
public:
				CLinkedList();
virtual			~CLinkedList();
				
	void		Add(CLinkable* item);		// Add at tail of list
	void		AddAfter(CLinkable* item, CLinkable* afterThis);
		
	void		Delete(CLinkable* item);
	void		DeleteAll();
				
	CLinkable*	First();
	CLinkable*	Last();

protected:
	CLinkable*	mFirst;
	CLinkable*	mLast;
};


//===========================================================================
//	Simple data bucket class, use instead of handles

class CBucket : public NPObject {
public:
						CBucket();
				virtual	~CBucket();

				Boolean	AddData(void huge *data, long count);
				long	GetCount();
				Byte huge*	GetData();
				void	SetChunkSize(long chunkSize);
				void	Reset();
				void	Trim();
protected:
		Byte huge *mData;
		long	mDataSpace;
		long	mDataCount;
		long	mChunkSize;
};

#endif
