//******************************************************************************
//
//	File:		List.cpp
//
//	Description:	BList class.
//			Implementation for a basic list class.
//	
//	Written by:	Steve Horowitz
//
//	Copyright 1992-93, Be Incorporated, All Rights Reserved.
//
//	Notes:		o Supports only pointers as list elements
//			o Assumes no NULL pointers are stored in list since
//			  this is the return value if something fails
//
//******************************************************************************

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <List.h>

#define ITEM_SIZE sizeof(void*)

//------------------------------------------------------------
BList::BList(int32 initialAllocSize)
{
	fItemCount = 0;
	fSingleItem = NULL;
	Alloc(initialAllocSize);
}

//------------------------------------------------------------
BList::BList(const BList& list)
{
	fObjectList = NULL;
	(*this) = list;
}

//------------------------------------------------------------
BList &BList::operator=(const BList& list)
{
	if (this != &list) {
		Cleanup();
		fPhysicalSize = list.fPhysicalSize;
		fItemCount = list.fItemCount;
		fSingleItem = list.fSingleItem;
		fInitialSize = list.fInitialSize;
		if (list.fObjectList == &list.fSingleItem) {
			fObjectList = &fSingleItem;
		} else if (list.fObjectList) {
			fObjectList = (void**)malloc(fPhysicalSize);
			memcpy(fObjectList, list.fObjectList, fPhysicalSize);
		}
	}
	return *this;
}

//------------------------------------------------------------
BList::~BList()
{
	Cleanup();
}

//------------------------------------------------------------
bool BList::AddItem(void* item, int32 index)
{
	if (index < 0 || index > fItemCount)
		return(false);

	if (fPhysicalSize < (fItemCount + 1) * ITEM_SIZE)
		Grow(fItemCount+1);

	// make room for new object
	if (index != fItemCount)
		memmove(fObjectList + index + 1,		// dest
	        	fObjectList + index,			// src
	        	(fItemCount - index) * ITEM_SIZE);	// count

	fObjectList[index] = item;
	fItemCount++;
	return(true);
}

//------------------------------------------------------------
bool BList::AddList(BList *newItems)
{
	return AddList(newItems, CountItems());
}

//------------------------------------------------------------
bool BList::AddList(BList *newItems, int32 index)
{
	int32	new_count;

	if (index < 0 || index > fItemCount)
		return(false);

	new_count = newItems->CountItems();

	if (fPhysicalSize < (fItemCount + new_count) * ITEM_SIZE)
		Grow(fItemCount + new_count);

	// make hole for new list
	if (index != fItemCount)
		memmove(fObjectList + index + new_count,	// dest
	        	fObjectList + index,				// src
	        	(fItemCount - index) * ITEM_SIZE);	// count

	// copy new list into proper location
	memcpy(fObjectList + index,
			newItems->Items(),
			new_count * ITEM_SIZE);
		
	fItemCount += new_count;
	return(true);
}

//------------------------------------------------------------
bool BList::RemoveItem(void* item)
{
	return(RemoveItem(IndexOf(item)) != NULL);
}

//------------------------------------------------------------
void* BList::RemoveItem(int32 index)
{
	void* item;

	if (index < 0 || index >= fItemCount)
		return(NULL);

	item = fObjectList[index];

	if (RemoveItems(index, 1) == false) return (NULL);
	else return(item);
}

//------------------------------------------------------------

bool BList::RemoveItems(int32 index, int32 count)
{
	return RemoveItems(index, count, NULL, NULL);
}

//------------------------------------------------------------

bool BList::RemoveItems(int32 index, int32 count,
						bool (*func)(void *, void *), void *dataPtr)
{
	if (index < 0 || index >= fItemCount)
		return(false);

	//make sure we don't go past the end of the list
	if (index + count > fItemCount) count = fItemCount - index;
	
	const int32 new_count = fItemCount - count;
	
	if (func) {
		for (int32 i=index; i<(index+count); i++) {
			// if func returns true then abort the removal
			// yes, this is weird, but that's how DoForEach()
			// does it
			if ((*func)(fObjectList[i], dataPtr))
				return false;
		}
	}
	
	// compact list to get rid of 'Removed' items
	memcpy(fObjectList + index,					// dest
	       fObjectList + index + count,			// src
	       (new_count - index) * ITEM_SIZE);	// count

	// resize array if new size brings it below threshold
	Shrink(new_count);
	
	fItemCount = new_count;

	return(true);
}

//------------------------------------------------------------
void BList::Alloc(int32 initialAllocSize)
{
	if (initialAllocSize <= 1) {
		// Store initial item inlined into class.
		fPhysicalSize = ITEM_SIZE;
		fObjectList = &fSingleItem;
		fInitialSize = 1;
	} else {
		// Allocate an array to store items.
		fPhysicalSize = initialAllocSize * ITEM_SIZE;
		fObjectList = (void**)malloc(fPhysicalSize);
		fInitialSize = initialAllocSize;
	}
}

//------------------------------------------------------------
void BList::Cleanup()
{
	if (fObjectList != &fSingleItem) free(fObjectList);
	fObjectList = NULL;
	fPhysicalSize = 0;
	fItemCount = 0;
}

//------------------------------------------------------------
void BList::MakeEmpty()
{
	Cleanup();
	Alloc(fInitialSize);
}

//------------------------------------------------------------
void BList::DoForEach(bool (*func)(void*))
{
	for (int32 i = 0; i < fItemCount; i++)
		// if func returns true then abort the iteration
		if ((*func)(fObjectList[i]))
			break;
}

//------------------------------------------------------------
void BList::DoForEach(bool (*func)(void*, void*), void* dataPtr)
{
	for (int32 i = 0; i < fItemCount; i++)
		// if func returns true then abort the iteration
		if ((*func)(fObjectList[i], dataPtr))
			break;
}

//------------------------------------------------------------
int32 BList::IndexOf(void* item) const
{
	int32	i;
	void	**ptr;

	i = fItemCount;
	ptr = fObjectList;
	while(i--) {
		if (*ptr++ == item)
			return(fItemCount - i - 1);
	}
	return(-1);			
}

//------------------------------------------------------------
void BList::Grow(int32 needed)
{
	size_t newSize = (((fPhysicalSize/ITEM_SIZE)*3)/2)*ITEM_SIZE;
	if (newSize < (needed*ITEM_SIZE))
		newSize = needed*ITEM_SIZE;
	else if (newSize < (fPhysicalSize+ITEM_SIZE*2))
		newSize = fPhysicalSize+ITEM_SIZE*2;
	
	fPhysicalSize = newSize;
	
	if (fObjectList != &fSingleItem) {
		fObjectList = (void**)realloc(fObjectList, fPhysicalSize);
	} else {
		fObjectList = (void**)malloc(fPhysicalSize);
		if (fItemCount >= 1) {
			*fObjectList = fSingleItem;
		}
	}
}

//------------------------------------------------------------
void BList::Shrink(int32 needed)
{
	if (fObjectList == &fSingleItem) {
		// Already as small as it can get.
		return;
	}
	
	if (needed <= 1) {
		// Short-circuit -- if the caller needs one or less item,
		// deallocate the array and go back to a single inlined item.
		if (fObjectList) {
			if (fItemCount > 0 && fObjectList)
				fSingleItem = *fObjectList;
			free(fObjectList);
		}
		fObjectList = &fSingleItem;
		fPhysicalSize = ITEM_SIZE;
		return;
	}
	
	// Determine if memory should be deallocated.  The algorithm is
	// to only resize if the needed amount is less than two Grow()
	// worths of expansion, but only shrink by one Grow().  This
	// minimizes the number of reallocations if you are adding and
	// removing around a Grow() boundary.
	size_t padSize = ((needed*9)/4)*ITEM_SIZE;
	if (padSize < fPhysicalSize) {
		padSize = ((needed*3)/2)*ITEM_SIZE;
		if (padSize < (fInitialSize*ITEM_SIZE))
			padSize = fInitialSize*ITEM_SIZE;
		if (padSize < fPhysicalSize) {
			fPhysicalSize = padSize;
			fObjectList = (void**)realloc(fObjectList, fPhysicalSize);
		}
	}
}

//------------------------------------------------------------
void BList::SortItems(int (*cmp)(const void *, const void *))
{
	qsort(Items(), CountItems(), ITEM_SIZE, cmp);
}


//------------------------------------------------------------
bool BList::SwapItems(int32 indexA, int32 indexB)
{
	if (indexA<0 || indexA>=fItemCount) return false;
	if (indexB<0 || indexB>=fItemCount) return false;
	if (indexA == indexB) return true;
	void * data = fObjectList[indexA];
	fObjectList[indexA] = fObjectList[indexB];
	fObjectList[indexB] = data;
	return true;
}


//------------------------------------------------------------
bool BList::MoveItem(int32 fromIndex, int32 toIndex)
{
	if (fromIndex<0 || fromIndex>=fItemCount) return false;
	if (toIndex<0) return false;
	if (toIndex>=fItemCount) toIndex = fItemCount-1;
	if (fromIndex == toIndex) return true;
	void * data = fObjectList[fromIndex];
	if (fromIndex < toIndex)
	{
		/* slide items down */
		memmove(&fObjectList[fromIndex], &fObjectList[fromIndex+1], ITEM_SIZE*(toIndex-fromIndex));
	}
	else
	{
		/* slide items up */
		memmove(&fObjectList[toIndex+1], &fObjectList[toIndex], ITEM_SIZE*(fromIndex-toIndex));
	}
	fObjectList[toIndex] = data;
	return true;
}


//------------------------------------------------------------
bool BList::ReplaceItem(int32 index, void *newItem)
{
	if (index<0 || index>=fItemCount) return false;
	fObjectList[index] = newItem;
	return true;
}


//------------------------------------------------------------
void		**BList::Items() const
			{ return(fObjectList); }

//------------------------------------------------------------
void		*BList::ItemAt(int32 indx) const
			{	if (indx < 0 || indx >= fItemCount)
					return(NULL);
				else
					return(fObjectList[indx]);
			}

//------------------------------------------------------------
void		*BList::ItemAtFast(int32 indx) const
			{ return(fObjectList[indx]); }

//------------------------------------------------------------
bool		BList::AddItem(void* item)
			{ return(AddItem(item, fItemCount)); }

//------------------------------------------------------------
int32		BList::CountItems() const
			{ return(fItemCount); }

//------------------------------------------------------------
void 		*BList::FirstItem()	 const
			{
				if (fItemCount == 0)
					return(NULL);
				else
					return(fObjectList[0]);
			}

//------------------------------------------------------------
void 		*BList::LastItem() 	 const
			{
				if (fItemCount == 0)
					return(NULL);
				else
					return(fObjectList[fItemCount - 1]);
			}

//------------------------------------------------------------
bool		BList::IsEmpty() const
			{ return(fItemCount == 0); }

//------------------------------------------------------------
bool		BList::HasItem(void* item) const
			{ return(IndexOf(item) != -1); }

//------------------------------------------------------------

void BList::_ReservedList1() {}
void BList::_ReservedList2() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
