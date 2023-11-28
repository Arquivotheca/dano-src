//==================================================================
//	MTriangleList.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	This class is a data structure that holds the data for
//	a triangle list view.

#include "MTriangleList.h"
#include "IDEConstants.h"

#include <Debug.h>

struct ItemRec
{
	int32	wideOpenIndex;
	int32	visibleIndex;
	void*	data;
	bool	parentRow;
	bool	expanded;
	
			ItemRec(
				int32	inWideOpenIndex,
				int32	inVisibleIndex,
				void*	inData,
				bool	inIsParent,
				bool	inIsExpanded)
			: wideOpenIndex(inWideOpenIndex), 
			visibleIndex(inVisibleIndex),
			data(inData), parentRow(inIsParent),
			expanded(inIsExpanded){}
};

// ---------------------------------------------------------------------------
//		MTriangleList
// ---------------------------------------------------------------------------

MTriangleList::MTriangleList()
{
}

// ---------------------------------------------------------------------------
//		~MTriangleList
// ---------------------------------------------------------------------------

MTriangleList::~MTriangleList()
{
	ItemRec*	rec;
	int32		i = 0;

	while (fItemList.GetNthItem(rec, i++))
		delete rec;
}

// ---------------------------------------------------------------------------
//		GetWideOpenIndex
// ---------------------------------------------------------------------------

int32
MTriangleList::GetWideOpenIndex(
	int32 inVisibleIndex) const
{
	int32		wideOpenIndex = kTInvalidIndex;
	int32		index = 0;
	ItemRec*	rec;
	
	// This could be a binary search ????
	while (fItemList.GetNthItem(rec, index))
	{
		if (rec->visibleIndex == inVisibleIndex)
		{
			wideOpenIndex = index;
			break;
		}
		index++;
	}
	
	return wideOpenIndex;	
}

// ---------------------------------------------------------------------------
//		GetVisibleIndex
// ---------------------------------------------------------------------------

int32
MTriangleList::GetVisibleIndex(
	int32 inWideOpenIndex) const
{
	int32		visibleIndex = kTInvalidIndex;
	ItemRec*	rec;
	
	if (fItemList.GetNthItem(rec, inWideOpenIndex))
		visibleIndex = rec->visibleIndex;
	
	return visibleIndex;
}

// ---------------------------------------------------------------------------
//		GetWideOpenIndex
// ---------------------------------------------------------------------------

int32
MTriangleList::WideOpenIndexOf(
	void*	inData) const
{
	int32		wideOpenIndex = kTInvalidIndex;
	int32		index = 0;
	ItemRec*	rec;
	
	// This could be a binary search ????
	while (fItemList.GetNthItem(rec, index))
	{
		if (rec->data == inData)
		{
			wideOpenIndex = index;
			break;
		}
		index++;
	}
	
	return wideOpenIndex;	
}

// ---------------------------------------------------------------------------
//		InsertParentRow
// ---------------------------------------------------------------------------

void
MTriangleList::InsertParentRow(
	int32 	inWideOpenIndex,
	void*	inData)
{
	InsertRow(inWideOpenIndex, inData, true);
}

// ---------------------------------------------------------------------------
//		InsertChildRow
// ---------------------------------------------------------------------------

void
MTriangleList::InsertChildRow(
	int32 	inWideOpenIndex,
	void*	inData)
{
	InsertRow(inWideOpenIndex, inData, false);
}

// ---------------------------------------------------------------------------
//		InsertRow
// ---------------------------------------------------------------------------
//	Insert a row at the specified wide open index.  If the row is a parent
//	row then it is inserted as a visible row.  If the row is a child row
//	then whether it is visible or not depends on the state of its parent
//	row.
//	This doesn't deal with adding child rows without parent rows or adding
//	child rows before any parent rows.

void
MTriangleList::InsertRow(
	int32 	inWideOpenIndex,
	void*	inData,
	bool	inParent)
{
	if (inWideOpenIndex < 0)
		inWideOpenIndex = 0;
	if (inWideOpenIndex > fItemList.CountItems())
		inWideOpenIndex = fItemList.CountItems();

	// Calculate the visible index
	bool		isVisible = inParent;
	int32		i;
	int32		visibleIndex = kTInvalidIndex;
	ItemRec*	rec;
	
	if (! isVisible)
	{
		// Find the parent row
		i = inWideOpenIndex - 1;
		while (fItemList.GetNthItem(rec, i--))
		{
			if (rec->parentRow)
			{
				isVisible = rec->expanded;
				break;
			}
		}
	}

	if (isVisible)
	{
		i = 0;
		visibleIndex = 0;

		while (fItemList.GetNthItem(rec, i) && i < inWideOpenIndex)
		{
			if (rec->visibleIndex >= 0)
				visibleIndex++;
			i++;
		}
	}
	
	// Build the new record
	rec = new ItemRec(inWideOpenIndex, visibleIndex, inData, inParent, true);

	fItemList.AddItem(rec, inWideOpenIndex);

	AdjustIndexes(inWideOpenIndex + 1, 1, isVisible);
}

// ---------------------------------------------------------------------------
//		AdjustIndexes
// ---------------------------------------------------------------------------
//	Adjust the wideopen and visible indexes of all the rows starting with
//	the specified row.  Done after inserting or removing rows.

void
MTriangleList::AdjustIndexes(
	int32	inWideOpenIndexFrom,
	int32 	inDelta,
	bool	inVisible)	// default parameter, is the row visible?
{
	ItemRec*	rec;
	
	int32		i = inWideOpenIndexFrom;

	while (fItemList.GetNthItem(rec, i++))
	{
		rec->wideOpenIndex += inDelta;
		if (inVisible && rec->visibleIndex >= 0)
			rec->visibleIndex += inDelta;
	}
}

// ---------------------------------------------------------------------------
//		RemoveRows
// ---------------------------------------------------------------------------
//	Remove rows that cannot be collapsed.

void
MTriangleList::RemoveRows(
	int32	inVisibleIndexFrom,
	int32 	inNumRows)
{
	if (inVisibleIndexFrom < 0)
		inVisibleIndexFrom = 0;
	if (inVisibleIndexFrom > fItemList.CountItems())
		inVisibleIndexFrom = fItemList.CountItems();

	int32		wideOpenIndex = GetWideOpenIndex(inVisibleIndexFrom);
	fItemList.RemoveItems(wideOpenIndex, inNumRows);

	AdjustIndexes(wideOpenIndex, -inNumRows);
}

// ---------------------------------------------------------------------------
//		RemoveRowsWideOpen
// ---------------------------------------------------------------------------
//	Remove rows that cannot be collapsed.

void
MTriangleList::RemoveRowsWideOpen(
	int32	inWideOpenIndexFrom,
	int32 	inNumRows)
{
	if (inWideOpenIndexFrom < 0)
		inWideOpenIndexFrom = 0;
	if (inWideOpenIndexFrom > fItemList.CountItems())
		inWideOpenIndexFrom = fItemList.CountItems();

	bool		visible = GetVisibleIndex(inWideOpenIndexFrom) >= 0;

	fItemList.RemoveItems(inWideOpenIndexFrom, inNumRows);

	AdjustIndexes(inWideOpenIndexFrom, -inNumRows, visible);
}

// ---------------------------------------------------------------------------
//		ContractRow
// ---------------------------------------------------------------------------

int32
MTriangleList::ContractRow(
	int32 inVisibleIndex)
{
	int32		childRows = 0;
	int32		wideOpenIndex = GetWideOpenIndex(inVisibleIndex);
	ASSERT(wideOpenIndex >= 0);

	if (wideOpenIndex >= 0)
	{
		ItemRec*	rec;
		fItemList.GetNthItem(rec, wideOpenIndex);

		ASSERT(rec->parentRow);	// Can't contract a child row

		rec->expanded = false;

		// Conceal all the child rows of this parent row
		while (fItemList.GetNthItem(rec, ++wideOpenIndex) &&
					! rec->parentRow)
		{
			rec->visibleIndex = kTInvalidIndex;
			childRows++;
		}
		
		// Adjust the visible row id of the rows after the parent row
		for (int32 i = wideOpenIndex; i < fItemList.CountItems(); i++)
		{
			if (fItemList.GetNthItem(rec, i))
			{
				if (rec->visibleIndex >= 0)
					rec->visibleIndex -= childRows;
			}
		}
	}
	
	return childRows;
}

// ---------------------------------------------------------------------------
//		ExpandRow
// ---------------------------------------------------------------------------

int32
MTriangleList::ExpandRow(
	int32 inVisibleIndex)
{
	int32		childRows = 0;
	int32		wideOpenIndex = GetWideOpenIndex(inVisibleIndex);
	ASSERT(wideOpenIndex >= 0);

	if (wideOpenIndex >= 0)
	{
		ItemRec*	rec;
		fItemList.GetNthItem(rec, wideOpenIndex);

		ASSERT(rec->parentRow);	// Can't expand a child row

		rec->expanded = true;

		// Reveal all the child rows of this parent row
		while (fItemList.GetNthItem(rec, ++wideOpenIndex) &&
					! rec->parentRow)
		{
			rec->visibleIndex = ++inVisibleIndex;
			childRows++;
		}
		
		// Adjust the visible row id of the rows after the parent row
		for (int32 i = wideOpenIndex; i < fItemList.CountItems(); i++)
		{
			if (fItemList.GetNthItem(rec, i))
			{
				if (rec->visibleIndex >= 0)
					rec->visibleIndex += childRows;
			}
		}
	}
	
	return childRows;
}

// ---------------------------------------------------------------------------
//		RowData
// ---------------------------------------------------------------------------

void*
MTriangleList::RowData(
	int32 inWideOpenIndex) const
{
	ItemRec*	rec;

	if (fItemList.GetNthItem(rec, inWideOpenIndex))
	{
		return rec->data;
	}
	else
	{
		ASSERT(!"invalid index for MTriangleList::RowData");
		return nil;
	}
}

// ---------------------------------------------------------------------------
//		Collapsable
// ---------------------------------------------------------------------------

bool
MTriangleList::Collapsable(
	int32	inVisibleRow) const
{
	ItemRec*	rec;
	int32		row = GetWideOpenIndex(inVisibleRow);

	if (fItemList.GetNthItem(rec, row))
	{
		return rec->parentRow;
	}
	else
	{
		ASSERT(!"invalid index for MTriangleList::Collapsable");
		return false;
	}
}

// ---------------------------------------------------------------------------
//		Expanded
// ---------------------------------------------------------------------------

bool
MTriangleList::Expanded(
	int32	inVisibleRow) const
{
	ItemRec*	rec;
	int32		row = GetWideOpenIndex(inVisibleRow);

	if (fItemList.GetNthItem(rec, row))
	{
		return rec->expanded;
	}
	else
	{
		ASSERT(!"invalid index for MTriangleList::Expanded");
		return false;
	}
}

// ---------------------------------------------------------------------------
//		Rows
// ---------------------------------------------------------------------------

int32
MTriangleList::Rows() const
{
	return fItemList.CountItems();
}
