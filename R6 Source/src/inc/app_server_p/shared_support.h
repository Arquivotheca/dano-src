
//******************************************************************************
//
//	File:		shared_support.h
//
//	Description:	Some useful classes/functions
//	
//	Written by:	George Hoffman
//
//	Copyright 1997, Be Incorporated
//
//******************************************************************************/

#ifndef SHARED_SUPPORT_H
#define SHARED_SUPPORT_H

#include <memory.h>
#include <malloc.h>

namespace BPrivate {

// Stack-based memory buffer that will dynamically select between
// using the stack or heap.
class session_buffer {
public:
	inline void* reserve(size_t req_size)
	{
		used = req_size;
		if (req_size < sizeof(stack)) {
			if (buffer) {
				free(buffer);
				buffer = NULL;
			}
			return stack;
		}
		return (buffer = realloc(buffer, req_size));
	}
	
	inline size_t size() const				{ return used; }
	inline const void* retrieve() const		{ return buffer ? buffer : stack; }
	
	inline session_buffer()	: used(0), buffer(NULL)	{ }
	inline ~session_buffer()	{ if (buffer) free(buffer); }

private:
	session_buffer(const session_buffer&);
	session_buffer& operator=(const session_buffer&);
	
	uint32 stack[64];
	size_t used;
	void* buffer;
};

// ---------------------------------------------------------------------

/* Some useful macros to manipulate bitmaps */
#define BMALLOC(num) \
  calloc((num+7)>>3,1)
#define BMTST(bm, num)  \
  (((char *) (bm))[(num) >> 3] & (0x80 >> ((num) & 0x07)))
#define BMSET(bm, num)  \
  (((char *) (bm))[(num) >> 3] |= (0x80 >> ((num) & 0x07)))
#define BMCLR(bm, num)  \
  (((char *) (bm))[(num) >> 3] &= ~(0x80 >> ((num) & 0x07))) 

struct Bitmap
{
		void *	bm;

			Bitmap(int32 numBits)
				{ bm = BMALLOC(numBits); };
            ~Bitmap()
				{ free(bm); };
inline	bool		test(int32 num)
				{ return BMTST(bm,num); };
inline	void		set(int32 num)
				{ BMSET(bm,num); };
inline	void		setExtent(int32 start, int32 len)
				{ for (int32 q=0;q<len;q++) BMSET(bm,start+q); };
inline	void		clear(int32 num)
				{ BMCLR(bm,num); };
};

template <class t>
class BArray {

	private:

		t		*items;
		int		numItems;
		int		numSlots;
		int		blockSize;

inline	int32	AssertSize(int size)
{
	if (size > numSlots) {
		numSlots = ((size+blockSize-1)/blockSize) * blockSize;
		t *tmp = (t*)realloc(items,numSlots*sizeof(t));
		if (!tmp)
			return -1;
		items = tmp;
	};
	return size;
};

	public:

inline			BArray(int _blockSize=256)
{
	blockSize = _blockSize;
	numItems = numSlots = 0;
	items = NULL;
};

inline			BArray(BArray<t> &copyFrom)
{
	blockSize = copyFrom.blockSize;
	numSlots = 0;
	items = NULL;
	AssertSize(copyFrom.numSlots);
	numItems = copyFrom.numItems;
	memcpy(items,copyFrom.items,numItems*sizeof(t));
};

inline			~BArray()
{
	if (items)
		free(items);
};

inline	t*		Items()
{
	return items;
};

inline	void		SetList(t* newList, int32 listSize)
{
	if (items)
		free(items);
	items = newList;
	numSlots = listSize;
	numItems = listSize;
};

inline	void		SetSlots(int32 slots)
{
	if (numSlots != slots) {
		numSlots = slots;
		if (numItems > numSlots)
			numItems = numSlots;
		t *tmp = (t*)realloc(items,numSlots*sizeof(t));
		if (!tmp) return;
		items = tmp;
	};
};

inline	void		SetItems(int32 count)
{
	if (AssertSize(count) < 0)
		return;
	numItems = count;
};

inline	void		Trim()
{
	SetSlots(numItems);
};

inline	int32	CountItems()
{ return numItems; };

/*
inline	bool		IsMember(t &theT)
{
	for (int32 i=0;i<numItems;i++)
		if (theT == items[i]) return true;
	return false;
};
*/

inline	void		RemoveItems(int32 index, int32 len)
{
	memmove(items+index,items+index+len,sizeof(t)*(numItems-index-len));
	numItems-=len;
};

inline	void		RemoveItem(int32 index)
{
	RemoveItems(index,1);
};

inline	int32	AddArray(BArray<t> *a)
{
	if (AssertSize(numItems + a->numItems) < 0)
		return -1;
	memcpy(items+numItems,a->items,a->numItems*sizeof(t));
	numItems = numItems + a->numItems;
	return a->numItems;
};

inline	int32	AddItem(const t &theT)
{
	if (AssertSize(numItems+1) < 0)
		return -1;
	items[numItems] = theT;
	numItems++;
	return numItems-1;
};

inline	void		MakeEmpty()
{
	numSlots = 0;
	numItems = 0;
	if (items!=NULL) {
		free(items);
		items = NULL;
	};
};

inline	t&		ItemAt(int index)
				{ return items[index]; };

inline	t&		operator[](int index)
				{ return items[index]; };
};

}
using namespace BPrivate;

#endif
