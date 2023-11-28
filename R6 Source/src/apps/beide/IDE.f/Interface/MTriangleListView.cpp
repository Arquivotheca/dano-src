//==================================================================
//	MTriangleListView.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//==================================================================
//	BDS

#include "MTriangleListView.h"
#include "MSectionLine.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "stDrawingMode.h"

#include <Bitmap.h>

BRect		MTriangleListView::sTriangleRect;

const float kSectionLeft = 12.0;
const float kNameLeft = 18.0;
const bigtime_t kAnimationDelay = 100000; // 100 ms

// ---------------------------------------------------------------------------
//		MTriangleListView
// ---------------------------------------------------------------------------
//	Constructor

MTriangleListView::MTriangleListView(
	BRect			inFrame,
	const char*		inName)
	: MDLOGListView(
		inFrame,
		inName)
{
	sTriangleRect = MSectionLine::ExpandedBitmap()->Bounds();
	// offseting the triangle left bits of color on left as we
	// updated after an expand/collapse click
	// [old code was...] sTriangleRect.OffsetBy(1.0, 0.0);
	// however, we do want to make it easier to click on - so 
	// make our hit testing (and invalidating) rectangle just 
	// a little larger
	sTriangleRect.InsetBy(-1.0, 0.0);
	
	SetMultiSelect(false);
}

// ---------------------------------------------------------------------------
//		~MTriangleListView
// ---------------------------------------------------------------------------
//	Destructor

MTriangleListView::~MTriangleListView()
{
}

// ---------------------------------------------------------------------------
//		DrawRow
// ---------------------------------------------------------------------------
//	Subclasses should call this function and then draw the string for the name
//	at the current pen position.

void
MTriangleListView::DrawRow(
	int32 	inRow,
	void * 	/*inData*/,
	BRect	inArea,
	BRect	/*inIntersection*/)
{
	float 		left = kNameLeft;

	if (fTriangleList.Collapsable(inRow))
	{
		// Draw the triangle control
		if (fTriangleList.Expanded(inRow))
			DrawExpanded(inArea);
		else
			DrawContracted(inArea);

		left = kSectionLeft;
	}
	
	// Draw the title
	MovePenTo(left, inArea.bottom - GetCachedFontHeight().descent);
	//Subclasses should call this function and then draw the string for the name
}

// ---------------------------------------------------------------------------
//		ClickHook
// ---------------------------------------------------------------------------
//	Opens and closes the triangle for rows with triangles on them.

bool
MTriangleListView::ClickHook(
	BPoint	inWhere,
	int32	inRow,
	uint32 /* modifiers */,
	uint32 /* buttons */)
{
	bool			result = false;
	
	if (fTriangleList.Collapsable(inRow))
	{
		BRect			triangleRect = sTriangleRect;
		BRect 			frame;

		GetRowRect(inRow, &frame);

		triangleRect.OffsetTo(sTriangleRect.left, frame.top);

		if (triangleRect.Contains(inWhere))
		{
			if (fTriangleList.Expanded(inRow))
			{
				Contract(frame);
				ContractRow(inRow);
			}
			else
			{
				Expand(frame);		
				ExpandRow(inRow);
			}

			Invalidate(triangleRect);
			result = true;
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		InsertCollapsableRow
// ---------------------------------------------------------------------------
//	Insert a row that has a triangle control.

void
MTriangleListView::InsertCollapsableRow(
	int32	inVisibleIndex,
	void*	inData,
	float	inHeight)
{
	ASSERT(inVisibleIndex >= 0 && inVisibleIndex <= CountRows());
	
	// Insert the row into the triangle list
	int32	wideOpenIndex;
	
	if (inVisibleIndex == CountRows())		// special case for inserting at the end
		wideOpenIndex = fTriangleList.Rows();
	else
		wideOpenIndex = fTriangleList.GetWideOpenIndex(inVisibleIndex);

	fTriangleList.InsertParentRow(wideOpenIndex, inData);

	// Insert the row into the view
	MListView::InsertRow(inVisibleIndex, inData, inHeight);
}

// ---------------------------------------------------------------------------
//		InsertRow
// ---------------------------------------------------------------------------
//	Insert a row that cannot be collapsed.

void
MTriangleListView::InsertRow(
	int32	inVisibleIndex,
	void*	inData,
	float	inHeight)
{
	ASSERT(inVisibleIndex >= 0 && inVisibleIndex <= CountRows());
	
	// Insert the row into the triangle list
	int32	wideOpenIndex;
	
	if (inVisibleIndex == CountRows())		// special case for inserting at the end
		wideOpenIndex = fTriangleList.Rows();
	else
		wideOpenIndex = fTriangleList.GetWideOpenIndex(inVisibleIndex);

	fTriangleList.InsertChildRow(wideOpenIndex, inData);

	// Insert the row into the view
	MListView::InsertRow(inVisibleIndex, inData, inHeight);
}

// ---------------------------------------------------------------------------
//		InsertRow
// ---------------------------------------------------------------------------
//	Insert a row that cannot be collapsed.

void
MTriangleListView::InsertRowWideOpen(
	int32	inWideOpenIndex,
	void*	inData,
	float	inHeight)
{
	ASSERT(inWideOpenIndex >= 0 && inWideOpenIndex <= fTriangleList.Rows());

	// Insert the row into the triangle list
	fTriangleList.InsertChildRow(inWideOpenIndex, inData);

	int32		visibleIndex = fTriangleList.GetVisibleIndex(inWideOpenIndex);

	// Insert the row into the view
	if (visibleIndex >= 0)
		MListView::InsertRow(visibleIndex, inData, inHeight);
}

// ---------------------------------------------------------------------------
//		RemoveRows
// ---------------------------------------------------------------------------
//	Remove rows that cannot be collapsed.

void
MTriangleListView::RemoveRows(
	int32	inVisibleIndexFrom,
	int32 	inNumRows)
{
	ASSERT(inVisibleIndexFrom >= 0 && inVisibleIndexFrom + inNumRows <= CountRows());
	
	// Remove the rows from the triangle list
	fTriangleList.RemoveRows(inVisibleIndexFrom, inNumRows);

	// Remove the rows from the view
	MListView::RemoveRows(inVisibleIndexFrom, inNumRows);
}

// ---------------------------------------------------------------------------
//		RemoveRowsWideOpen
// ---------------------------------------------------------------------------
//	Remove rows that cannot be collapsed.

void
MTriangleListView::RemoveRowsWideOpen(
	int32	inWideOpenIndexFrom,
	int32 	inNumRows)
{
	ASSERT(inWideOpenIndexFrom >= 0 && inWideOpenIndexFrom + inNumRows <= fTriangleList.Rows());
	
	int32		visibleIndex = fTriangleList.GetVisibleIndex(inWideOpenIndexFrom);
	// Remove the rows from the triangle list
	fTriangleList.RemoveRowsWideOpen(inWideOpenIndexFrom, inNumRows);

	// Remove the rows from the view if they were visible
	if (visibleIndex >= 0)
		MListView::RemoveRows(visibleIndex, inNumRows);
}

// ---------------------------------------------------------------------------
//		Contract
// ---------------------------------------------------------------------------

void
MTriangleListView::Contract(
	BRect inFrame)
{
	Sync();
	DrawIntermediate(inFrame); 
	Sync();
	snooze(kAnimationDelay);
	DrawContracted(inFrame); 
	Sync();
}

// ---------------------------------------------------------------------------
//		Expand
// ---------------------------------------------------------------------------

void
MTriangleListView::Expand(
	BRect inFrame)
{
	Sync();
	DrawIntermediate(inFrame);
	Sync();
	snooze(kAnimationDelay);
	DrawExpanded(inFrame);
	Sync();
}

// ---------------------------------------------------------------------------
//		ContractRow
// ---------------------------------------------------------------------------

void
MTriangleListView::ContractRow(
	int32	inVisibleIndex)
{
	int32		rowsHidden = fTriangleList.ContractRow(inVisibleIndex);

	MListView::RemoveRows(inVisibleIndex + 1, rowsHidden);
}

// ---------------------------------------------------------------------------
//		ExpandRow
// ---------------------------------------------------------------------------

void
MTriangleListView::ExpandRow(
	int32	inVisibleIndex)
{
	int32		rowsRevealed = fTriangleList.ExpandRow(inVisibleIndex);
	
	if (rowsRevealed > 0)
	{
		int32		wideOpenIndex = fTriangleList.GetWideOpenIndex(inVisibleIndex) + 1;
		int32		lastIndex = inVisibleIndex + rowsRevealed;
		
		for (int32 i = inVisibleIndex + 1; i <= lastIndex; i++)
		{
			void*		rowdata = fTriangleList.RowData(wideOpenIndex++);
			MListView::InsertRow(i, rowdata);
		}
	}
}

// ---------------------------------------------------------------------------
//		DrawContracted
// ---------------------------------------------------------------------------

void
MTriangleListView::DrawContracted(
	BRect inFrame)
{
	stDrawingMode		mode(this, B_OP_OVER);
	DrawBitmap(MSectionLine::ContractedBitmap(), inFrame.LeftTop());
}

// ---------------------------------------------------------------------------
//		DrawExpanded
// ---------------------------------------------------------------------------

void
MTriangleListView::DrawExpanded(
	BRect inFrame)
{
	stDrawingMode		mode(this, B_OP_OVER);
	DrawBitmap(MSectionLine::ExpandedBitmap(), inFrame.LeftTop());
}

// ---------------------------------------------------------------------------
//		DrawIntermediate
// ---------------------------------------------------------------------------

void
MTriangleListView::DrawIntermediate(
	BRect inFrame)
{
	stDrawingMode		mode(this, B_OP_OVER);
	DrawBitmap(MSectionLine::IntermediateBitmap(), inFrame.LeftTop());
}

// ---------------------------------------------------------------------------
//		GetWideOpenIndex
// ---------------------------------------------------------------------------

int32
MTriangleListView::GetWideOpenIndex(
	int32 inVisibleIndex) const
{	
	return fTriangleList.GetWideOpenIndex(inVisibleIndex);	
}

// ---------------------------------------------------------------------------
//		GetVisibleIndex
// ---------------------------------------------------------------------------

int32
MTriangleListView::GetVisibleIndex(
	int32 inWideOpenIndex) const
{
	return fTriangleList.GetVisibleIndex(inWideOpenIndex);	
}

// ---------------------------------------------------------------------------
//		RowData
// ---------------------------------------------------------------------------

void*
MTriangleListView::RowData(
	int32 inWideOpenIndex) const
{
	return fTriangleList.RowData(inWideOpenIndex);	
}

// ---------------------------------------------------------------------------
//		WideOpenIndexOf
// ---------------------------------------------------------------------------

int32
MTriangleListView::WideOpenIndexOf(
	void* inData) const
{	
	return fTriangleList.WideOpenIndexOf(inData);	
}

// ---------------------------------------------------------------------------
//		VisibleIndexOf
// ---------------------------------------------------------------------------

int32
MTriangleListView::VisibleIndexOf(
	void* inData) const
{	
	return fTriangleList.GetVisibleIndex(fTriangleList.WideOpenIndexOf(inData));	
}

// ---------------------------------------------------------------------------
//		WideOpenRowCount
// ---------------------------------------------------------------------------

int32
MTriangleListView::WideOpenRowCount() const
{
	return fTriangleList.Rows();	
}

