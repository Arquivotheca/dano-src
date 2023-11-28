//========================================================================
//	MAreaFileList.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	This list keeps AreaFileRec objects in alphabetical order and provides
//	a binary search to add or find objects.  This is used in the header
//	file caching.
//	BDS

#include <ctype.h>

#include "MAreaFileList.h"
#include "MFileCache.h"
#include "MWEditUtils.h"


// ---------------------------------------------------------------------------
//		MAreaFileList
// ---------------------------------------------------------------------------
//	Constructor

MAreaFileList::MAreaFileList(
	int32	itemsPerBlock)
	: MList<AreaFileRec*>(itemsPerBlock)
{
}

// ---------------------------------------------------------------------------
//		AddItem
// ---------------------------------------------------------------------------
//	Add the item to the list in sort order.  The item is added to the list
//	whether there is an identical item in the list or not.

bool
MAreaFileList::AddItem(
	AreaFileRec* inFileRec)
{
	int32	index;

	FindItem(inFileRec->name, inFileRec->systemtree, index);

	return MList<AreaFileRec*>::AddItem(inFileRec, index);
}

// ---------------------------------------------------------------------------
//		AddItem
// ---------------------------------------------------------------------------

bool
MAreaFileList::AddItem(
	AreaFileRec* 	inFileRec,
	int32 			inAtIndex)
{
	return MList<AreaFileRec*>::AddItem(inFileRec, inAtIndex);
}

// ---------------------------------------------------------------------------
//		FindItem
// ---------------------------------------------------------------------------
//	Use a binary search to find this file in the list.

bool
MAreaFileList::FindItem(
	const char *	inFileName,
	bool			inSystemTree,
	int32&			outIndex) const
{
	int32			top = CountItems() - 1;
	int32			bottom = 0;
	int32			middle;
	int32			comparison;
	
	while (top >= bottom)
	{
		middle = (bottom + top) / 2;
		
		AreaFileRec*	rec = (AreaFileRec*) ItemAtFast(middle);
		
		comparison = CompareStrings(inFileName, rec->name);

		if (comparison < 0 )
		{
			top = middle - 1;
		}
		else
		if (comparison > 0)
		{
			bottom = middle + 1;
		}
		else
		{
			if (inSystemTree == rec->systemtree)
			{
				outIndex = middle;
				return true;			// found; early exit
			}
			else
			{
				// Two files with the same name but different trees
				if (bottom < middle)
					bottom++;
				else
					top--;
			}
		}
	}
	
	// Not found
	outIndex = bottom;
	
	return false;
}

