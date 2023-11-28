//========================================================================
//	MSourceFileList.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	This list keeps sourcefile objects in alphabetical order and provides
//	a binary search to add or find objects.
//	BDS

#include <ctype.h>
#include <string.h>

#include "MSourceFileList.h"
#include "MSourceFile.h"
#include "MWEditUtils.h"

// ---------------------------------------------------------------------------
//		MSourceFileList
// ---------------------------------------------------------------------------
//	Constructor

MSourceFileList::MSourceFileList(int32 itemsPerBlock)
				: MList<MSourceFile*>(itemsPerBlock)
{
}

// ---------------------------------------------------------------------------
//		AddItem
// ---------------------------------------------------------------------------
//	Add the item to the list in sort order.  The item is added to the list
//	whether there is an identical item in the list or not.

bool
MSourceFileList::AddItem(MSourceFile* inSourceFile)
{
	int32	index;

	FindItem(inSourceFile->GetFileName(), inSourceFile->IsInSystemTree(), index);

	return MList<MSourceFile*>::AddItem(inSourceFile, index);
}

// ---------------------------------------------------------------------------
//		AddItem
// ---------------------------------------------------------------------------

bool
MSourceFileList::AddItem(MSourceFile* inSourceFile, int32 inAtIndex)
{
	return MList<MSourceFile*>::AddItem(inSourceFile, inAtIndex);
}

// ---------------------------------------------------------------------------
//		FindItem
// ---------------------------------------------------------------------------
//	Use a binary search to find this file in the list.

bool
MSourceFileList::FindItem(const char* inFileName, 
						  bool inSystemTree, 
						  int32& outIndex) const
{
	int32			top = CountItems() - 1;
	int32			bottom = 0;
	int32			middle;
	int32			comparison;
	
	while (top >= bottom)
	{
		middle = (bottom + top) / 2;
		
		MSourceFile*	rec = (MSourceFile*) ItemAtFast(middle);
		
		comparison = CompareStrings(inFileName, rec->GetFileName());

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
			if (inSystemTree == rec->IsInSystemTree())
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

// ---------------------------------------------------------------------------
// class MSourceFileSet
// ---------------------------------------------------------------------------

MSourceFileSet::MSourceFileSet(int32 itemsPerBlock = 50)
			   : MList<MSourceFile*>(itemsPerBlock)
{
}

// ---------------------------------------------------------------------------

bool
MSourceFileSet::AddItem(MSourceFile* inSourceFile)
{
	int32 index;
	this->FindItem(inSourceFile, index);
	return MList<MSourceFile*>::AddItem(inSourceFile, index);
}

// ---------------------------------------------------------------------------

bool
MSourceFileSet::AddItem(MSourceFile* inSourceFile, int32 inAtIndex)
{
	return MList<MSourceFile*>::AddItem(inSourceFile, inAtIndex);
}

// ---------------------------------------------------------------------------

int32
CompareEntryRef(MSourceFile& left, MSourceFile& right)
{
	// compare the left MSourceFile to the right
	// I can't just compare entry_ref because I want the 
	// name to be the first key not the device & directory.
	// I also can't compare full paths because I want to 
	// the leaf names need to be next to each other.
	// Look at the leaf name, followed by directory and volume
	// (first do a no-case compare so that we get proper sorting)
	// (but then check case compare so we don't get ByteOrder.h matching byteorder.h)
	
	int32 diff = CompareStringsNoCase(left.GetFileName(), right.GetFileName());	
	if (diff == 0) {
		diff = CompareStrings(left.GetFileName(), right.GetFileName());
	}
	
	// return now if names are different
	if (diff != 0) {
		return diff;
	}
	
	// names match - check the directories		
	entry_ref leftRef;
	entry_ref rightRef;
	left.GetRef(leftRef);
	right.GetRef(rightRef);
	
	if (leftRef.directory < rightRef.directory) {
		diff = -1;
	}
	else if (leftRef.directory > rightRef.directory) {
		diff = 1;
	}
	else {
		diff = 0;
	}
	
	if (diff != 0) {
		return diff;
	}
	
	// directories match - check volumes	
	if (leftRef.device < rightRef.device) {
		diff = -1;
	}
	else if (leftRef.device > rightRef.device) {
		diff = 1;
	}
	else {
		diff = 0;
	}				
	
	return diff;	
}

// ---------------------------------------------------------------------------

bool
MSourceFileSet::FindItem(MSourceFile* inFile, int32& outIndex) const
{
	//	Use a binary search to find this file in the list.
	int32 top = CountItems() - 1;
	int32 bottom = 0;
	int32 middle;
	int32 comparison;
	
	while (top >= bottom) {
		middle = (bottom + top) / 2;
		
		MSourceFile* rec = (MSourceFile*) ItemAtFast(middle);
		comparison = CompareEntryRef(*inFile, *rec);

		if (comparison < 0 ) {
			top = middle - 1;
		}
		else if (comparison > 0) {
			bottom = middle + 1;
		}
		else {
			outIndex = middle;
			return true;			// found; early exit
		}
	}
	
	// Not found
	outIndex = bottom;
	return false;
}
