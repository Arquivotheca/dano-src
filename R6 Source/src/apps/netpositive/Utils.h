// ===========================================================================
//	Utils.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __UTILS__
#define __UTILS__

#define ONE_SECOND   1000000
#define TEN_SECONDS 10000000

#include <SupportDefs.h>
#include <String.h>

#if !defined(MIN) && defined(R4_COMPATIBLE)
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#if !defined(MAX) && defined(R4_COMPATIBLE)
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

//===========================================================================
//	Common utils

char *SetStr(char *oldStr, const char *newStr);	//	Simple util to allocate a new string
												//	First string is memory based, second isn't
void CropTrailingSpace(BString& string);
void CropLeadingSpace(BString& string);

//void* PoolAlloc(void*& freeList, size_t allocationSize);
//void PoolFree(void*& freeList, void* ptr);

#if __INTEL__

#if _NO_INLINE_ASM
inline int FastConvertFloatToIntRound( float f )
{
	f += .5;
	return int(f);
}

inline int FastConvertFloatToIntTruncPositive( float f )
{
	return int(f);
}

#else
// gcc's built-in float-to-integer conversions suck.
// Here are some faster ones.
// Jason Sams is a god.
inline int FastConvertFloatToIntRound( float f )
{
    int t;
    __asm__ __volatile__ (
        "flds (%0) \n"
        "fistpl (%1) \n"
        : :"r"(&f),"r"(&t):"memory" );
    return t;

}

inline int FastConvertFloatToIntTruncPositive( float f )
{
    int t;
    static float offset = 0.5;
    __asm__ __volatile__ (
        "flds (%0) \n"
        "fsubs (%2) \n" 
        "fistpl (%1) \n"
        : :"r"(&f),"r"(&t),"r"(&offset):"memory" );
    return t;

}
#endif // _NO_INLINE_ASM

#endif

#ifdef DEBUGMENU
#include <Locker.h>

class TLocker : public BLocker {
public:				
			TLocker();
			TLocker(const char *name);
bool		Lock();
void		Unlock();
status_t	LockWithTimeout(bigtime_t timeout);

void		DumpLockingCallStack();

protected:
unsigned long	mCallStack[8];
};
#else
#define TLocker BLocker
#endif

// ===========================================================================
//	Infinite array class

template<class TYPE, int SIZE>
class CArray {
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


//===========================================================================
//	Simple linkable class

class CLinkable {
public:
							CLinkable();
					virtual	~CLinkable();
public:

	CLinkable*				At(long index);
	long					Count() const;
	CLinkable*				First() const;
	CLinkable*				Last() const;
	CLinkable*				Next() const {return fNext;}
	CLinkable*				Previous() const {return fPrevious;}

	void					AddAfter(CLinkable *item);
	void					DeleteAll();

	void					Add(CLinkable *item);
	void					Remove();
protected:

	CLinkable*	fNext;
	CLinkable*	fPrevious;
};

#define AddOrSet(_list,_item) { if (_list) _list->Add(_item); else _list = (_item);}

//======================================================================================
//	CLinkList object manages a list of CLinkable items

class CLinkedList {
public:
				CLinkedList();
virtual			~CLinkedList();
				
	void		Add(CLinkable* item);		// Add at tail of list
	void		AddAfter(CLinkable* item, CLinkable* afterThis);
		
	void		Delete(CLinkable* item);
	void		Remove(CLinkable* item);
	void		DeleteAll();
				
	CLinkable*	First() const;
	CLinkable*	Last();

protected:
	CLinkable*	mFirst;
	CLinkable*	mLast;
};


//===========================================================================
//	Simple data bucket class, use instead of handles

class CBucket {
public:
						CBucket();
						CBucket(long chunkSize);
				virtual	~CBucket();

				bool	AddData(const void *data, long count);
				long	GetCount();
				unsigned char*	GetData();
				void	SetChunkSize(long chunkSize);
				void	Reset();
				void	Trim();
protected:
		unsigned char *mData;
		long	mDataSpace;
		long	mDataCount;
		long	mChunkSize;
};


//===========================================================================
//	Inherit this class to do reference counting.

class Counted {
public:
						Counted();
				void	Reference();
				int32	Dereference();

// Make the destructor private so that you can't directly delete an instance.
protected:
		virtual 		~Counted() {}
		int32			refcount;
};

#endif
