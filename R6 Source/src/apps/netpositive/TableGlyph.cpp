// ===========================================================================
//	TableGlyph.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "TableGlyph.h"
#include "HTMLTags.h"
#include "BeDrawPort.h"
#include "MessageWindow.h"

#include <View.h>
#include <Window.h>
#include <malloc.h>
#include <stdio.h>

// Fast table layout has some problems right now.  I'll turn it on later.
#define USE_FAST_TABLE 1

void *CellGlyph::sFreeList;
// ===========================================================================
// Cells are just pageGlyphs with some additional info about their positioning

CellGlyph::CellGlyph(Document* htmlDoc) :
	PageGlyph(htmlDoc)
{
	mRow = mCol = 0;
	mRowSpan = mColSpan = 1;
	mVAlign = AV_MIDDLE;
	mNoWrap = 0;
	mKnownWidth = 0;
	mKnownHeight = 0;
	mIsWidthPercentage = false;
	mIsHeightPercentage = false;
	mBGColor = -1;
}

#ifdef DEBUGMENU
void CellGlyph::PrintStr(BString& print)
{
	PageGlyph::PrintStr(print);
	char str[128];
	if (mColSpan != 1 || mRowSpan != 1)
		sprintf(str," [%d:%d,%d:%d]",mCol,mColSpan,mRow,mRowSpan);
	else
		sprintf(str," [%d,%d]",mCol,mRow);
	print += str;
}
#endif

bool CellGlyph::IsCell()
{
	return true;
}

void CellGlyph::SetAttribute(long attributeID, long value, bool isPercentage)
{
	switch (attributeID) {
		case A_ROWSPAN:	mRowSpan = value;	break;
		case A_COLSPAN:	mColSpan = value;	break;
		
		case A_ALIGN:
			if (value == AV_MIDDLE)
				value = AV_CENTER;
			mAlign = value;
			break;

		case A_VALIGN:	
			// VALIGN=CENTER is incorrect HTML.  They really meant VALIGN=MIDDLE.
			if (value == AV_CENTER)
				value = AV_MIDDLE;
			mVAlign = value;
			break;

		case A_NOWRAP:	mNoWrap = 1;		break;

		case A_HEIGHT:					// may be a percentage, must know it was req
			mKnownHeight = value;
			mIsHeightPercentage = isPercentage;
			if (mIsHeightPercentage)
				mKnownHeight = 0;		// Dont handle height percentages yet
			break;

		case A_WIDTH:					// may be a percentage, must know it was req
			if(value >= 0){
				mKnownWidth = value;
				mIsWidthPercentage = isPercentage;
			}
			break;
		case A_BGCOLOR:	mBGColor = value;	break;
	}
}

void CellGlyph::SetColSpan(short span)
{
	mColSpan = span;
}

void CellGlyph::SetRowSpan(short span)
{
	mRowSpan = span;
}

bool CellGlyph::IsKnownWidthPercentage()
{
	return mIsWidthPercentage;
}

bool CellGlyph::IsKnownHeightPercentage()
{
	return mIsHeightPercentage;
}

void CellGlyph::SetRowAndCol(short row,short col)
{
	mRow = row;
	mCol = col;
}

float CellGlyph::GetKnownWidth()
{
	return mKnownWidth;
}

void CellGlyph::SetKnownWidth(float width, bool isPercentage)
{
	mKnownWidth = width;
	mIsWidthPercentage = isPercentage;
}

float CellGlyph::GetKnownHeight()
{
	return mKnownHeight;
}

void CellGlyph::ResetLayout()
{
	mWidth = 0;
	mHeight = 0;
	PageGlyph::ResetLayout();
}

long CellGlyph::GetBGColor()
{
	if (mBGColor != -1)
		return mBGColor;
	else {
		Glyph *g = GetParent();
		if (g->IsTable())
			return ((TableGlyph *)g)->GetBGColor();
	}
	return 0;
}

void CellGlyph::SetBGColor(long color)
{
	mBGColor = color;
}

//	VAlignCell a cell glyph vertically repositions its contents according to vAlign

void CellGlyph::VAlignCell(float height)
{
	float offset = 0;
	switch (mVAlign) {
		case AV_TOP:	offset = 0;							break;
		case AV_MIDDLE:	offset = (height - GetHeight())/2;	break;
		case AV_BOTTOM:	offset = height - GetHeight();		break;
	}
	Glyph *g;
	for (g = GetChildren(); g; g = (Glyph *)g->Next())
		g->SetTop(g->GetTop() + offset);
	SetHeight(height);
}

//	Draw detail about the cell

#ifdef DEBUGMENU
void CellGlyph::DrawInfo(DrawPort *drawPort)
{
	BRect r;
	GetBounds(&r);
	
	BString info;
	char str[256];
	sprintf(str,"Cell (%d,%d)",mCol,mRow);
	info = str;
	if (mColSpan > 1 || mRowSpan > 1) {
		sprintf(str," [%dx%d]",mColSpan,mRowSpan);
		info += str;
	}
	
	if (mKnownWidth) {
		if (mIsWidthPercentage)
			sprintf(str," %f%%",mKnownWidth);
		else {
			if (mWidth != mKnownWidth)
				sprintf(str," %f{%f}",mKnownWidth,mWidth);
			else
				sprintf(str," %f",mKnownWidth);
		}
		info += str;
	}
	float width = drawPort->TextWidth(info.String(),info.Length());
	
	Style s = {1,0,0,0,0,0,0,0,0};
	drawPort->SetStyle(s);
	
	r.bottom = r.top + 16;
	r.right = r.left + width + 2;
	drawPort->EraseRect(&r);
	drawPort->DrawText(r.left + 1,r.top + 1,info.String(),info.Length(),0);

	GetBounds(&r);
	drawPort->SetColor(0x00FF0000);
	drawPort->FrameRect(&r);
	drawPort->SetGray(0);
}
#endif

// ===========================================================================

typedef struct {
	float	min;
	float	max;
	float	known;
	float measuredmin;
	float measuredmax;
} PageWidth;

TableGlyph::TableGlyph(Document* htmlDoc) :
	PageGlyph(htmlDoc)
{
	mBreakType = kHardAlign;

	mColumns = mRows = 0;
	mCurrentCol = 0;

	mCellSpacing = 2;
	mCellPadding = 1;
	mCellBorder = 0;
	mBorder = 0;
	mVAlign = AV_MIDDLE;
	mAlign = AV_BASELINE;

	mCaption = 0;
	mInRow = 0;
	mInCell = NULL;
	mParentTable = 0;

	mKnownWidth = 0;
	mIsWidthPercentage = 0;

	mMinUsedWidth = 0;
	mMaxUsedWidth = 0;
		
	mBGColor = -1;
	
	mLayoutComplete = false;
	
	mOldRowHeights = 0;
}

TableGlyph::~TableGlyph()
{
	if (mOldRowHeights)
		free(mOldRowHeights);
}

#ifdef DEBUGMENU
void TableGlyph::PrintStr(BString& print)
{
	PageGlyph::PrintStr(print);
	
	char str[128];
	sprintf(str," [%d,%d]",mColumns,mRows);
	print += str;
	
	if (mKnownWidth) {
		if (mIsWidthPercentage)
			sprintf(str," %f%%",mKnownWidth);
		else
			sprintf(str," %f",mKnownWidth);
		print += str;
	}
}
#endif

bool TableGlyph::IsTable()
{
	return true;
}

bool TableGlyph::Floating()
{
	return (mAlign == AV_LEFT || mAlign == AV_RIGHT);
}

//	Only add html data inside cells

void TableGlyph::AddChild(Glyph* child)
{
	if (child->IsCell())
		PageGlyph::AddChild(child);
	else {
//		NP_ASSERT(GetParent());
		GetParent()->AddChild(child);
		//CellGlyph *g = NewCell(false);	// Can only add cells to tables...
		//OpenCell(g,false);
		//g->AddChild(child);				// So open one, ad stuff the lost html in
	}
}

TableGlyph *TableGlyph::GetParentTable()
{
	return mParentTable;
}

void TableGlyph::SetParentTable(TableGlyph *parentTable)
{
	mParentTable = parentTable;
}

void TableGlyph::SetAttribute(long attributeID, long value, bool isPercentage)
{
	switch (attributeID)
	{
		case	A_BORDER:
			mBorder = (value == -1) ? 1 : value;
			if (mBorder && !mCellBorder)
				mCellBorder = 1;
			break;
		case	A_CELLBORDER:
			mCellBorder = (value == -1) ? 1 : value;
			break;
		case	A_CELLSPACING:	mCellSpacing = value;	break;	// Space between cells
		case	A_CELLPADDING:	mCellPadding = value;	break;	// Space between cell walls and content
		
		case A_ALIGN:
			if (value == AV_MIDDLE)
				value = AV_CENTER;
			mAlign = value;
			break;					// Not in table tag, in TR tag
		
		case	A_VALIGN:
			// VALIGN=CENTER is incorrect HTML.  They really meant VALIGN=MIDDLE.
			if (value == AV_CENTER)
				value = AV_MIDDLE;
			mVAlign = value;
			break;

		case A_WIDTH:	// may be a percentage, must know it was req
			if (value <= 0)
				break;	
			mKnownWidth = value;
			mIsWidthPercentage = isPercentage;
			break;
			
		case A_BGCOLOR:	mBGColor = value;	break;
	}
}

void TableGlyph::SaveAttributes()
{
	mOldBorder = mBorder;
	mOldCellBorder = mCellBorder;
	mOldCellSpacing = mCellSpacing;
	mOldCellPadding = mCellPadding;
	mOldAlign = mAlign;
	mOldVAlign = mVAlign;
	mOldKnownWidth = mKnownWidth;
	mOldKnownPercentage = mIsWidthPercentage;
	mOldBGColor = mBGColor;
}

void TableGlyph::RestoreAttributes()
{
	mBorder = mOldBorder;
	mCellBorder = mOldCellBorder;
	mCellSpacing = mOldCellSpacing;
	mCellPadding = mOldCellPadding;
	mAlign = mOldAlign;
	mVAlign = mOldVAlign;
	mKnownWidth = mOldKnownWidth;
	mIsWidthPercentage = mOldKnownPercentage;
	mBGColor = mOldBGColor;
}

//	Layout a table. Create cells as the table is being drawn

void TableGlyph::OpenTable()
{
	mColumns = mRows = 0;
	mCurrentCol = 0;
	PageGlyph::Open();
}

void TableGlyph::OpenRow()
{
	if (mInRow) CloseRow();
	
	SaveAttributes();
	mInRow = true;
	mCurrentCol = 0;
	mRowAlign = AV_BASELINE;
	mVAlign = AV_MIDDLE;
}

//	Let the table instantiate the cell

CellGlyph *TableGlyph::NewCell(bool isHeading)
{
	CellGlyph *cell = new CellGlyph(mHTMLDoc);
	cell->SetBGColor(mBGColor);
	cell->SetAttribute(A_VALIGN,mVAlign,0);			// Inherit alignment from table
	if (isHeading)
		cell->SetAttribute(A_ALIGN,AV_CENTER,0);	// And are centered by default
	else
		cell->SetAttribute(A_ALIGN,mRowAlign,0);
	return cell;
}

//	Open a new cell, position it to avoid rowspans from above
//	Return the new cell so builder can make it a target
//	Does TD and TH tags

void TableGlyph::OpenCell(CellGlyph *cell, bool)
{
	if (mInCell)
		CloseCell();
	if (!mInRow)
		OpenRow();

//	Find out which col to put cell into

	CellGlyph *c = (CellGlyph *)GetChildren();
	
	for (; c; c = (CellGlyph *)c->Next()) {				// Forwards
		if (c->GetRow() + c->GetRowSpan() > mRows) {
			if (mCurrentCol >= c->GetCol() && mCurrentCol < (c->GetCol() + c->GetColSpan()))
				mCurrentCol = c->GetCol() + c->GetColSpan();
		}
	}
	
	if ((bool)(c = (CellGlyph *)GetChildren()))
		c = (CellGlyph *)c->Last();
		
	for (; c; c = (CellGlyph *)c->Previous()) {
		if (c->GetRow() + c->GetRowSpan() > mRows) {	// And backwards
			if (mCurrentCol >= c->GetCol() && mCurrentCol < (c->GetCol() + c->GetColSpan()))
				mCurrentCol = c->GetCol() + c->GetColSpan();
		}
	}
	
	cell->SetRowAndCol(mRows,mCurrentCol);	// Cell is located at row/col
	mColumns = MAX(mColumns,mCurrentCol+1);
	
	mCurrentCol += cell->GetColSpan();		// Advance horiontally to next col
	AddChild(cell);
	mInCell = cell;
	cell->Open();
}

bool TableGlyph::IsInCell()
{
	return mInCell != NULL;
}

bool TableGlyph::IsInRow()
{
	return mInRow != 0;
}

void TableGlyph::CloseCell()
{
	if (mInCell)
		mInCell->Close();
	mInCell = NULL;
}

void TableGlyph::CloseRow()
{
	CloseCell();
	if (mInRow) {
		mInRow = 0;
		mRows = mRows + 1;
		RestoreAttributes();
	}
}

//	Close the table

void TableGlyph::CloseTable()
{
	CloseRow();
	PageGlyph::Close();
}

//	Captions are cell glyphs too, they have a mRow and mCol of -1

void TableGlyph::OpenCaption(CellGlyph *caption)
{
	mCaption = caption;
	mCaptionAlign = caption->GetAlign();	// AV_TOP, AV_BOTTOM are alignments relative to table
	caption->SetAlign(AV_CENTER);			// But contents is always centered
	caption->SetRowAndCol(-1,-1);			// Won't get tangled in the rest of the layout
	AddChild(caption);
}

void TableGlyph::CloseCaption()
{
}

//	This could be based on the number of cells in a table, fancy heuristics etc.
//	It isn't

float TableGlyph::GetMaxCelWidth()
{
	return GetWidth()/mColumns;
}

float TableGlyph::GetWidth()
{
	if (mWidth) return mWidth;
//	return GetParent()->GetWidth();
	Glyph *p = GetParent();
	if (p->IsPage()) {
		float width = ((PageGlyph *)p)->SpaceLeftOnLine();
		if (width > 0 && width < p->GetWidth())
			return width;
	}
	return p->GetWidth();
}

float TableGlyph::GetMinUsedWidth(DrawPort *drawPort)
{
	if (mMinUsedWidth == 0) {
		DetermineWidth();
		LayoutCols(drawPort, true);
	}
	return mMinUsedWidth;
}

float TableGlyph::GetMaxUsedWidth(DrawPort *drawPort)
{
	if (mMaxUsedWidth == 0) {
		DetermineWidth();
		LayoutCols(drawPort, true);
	}
	return mMaxUsedWidth;
}

void TableGlyph::ResetLayout()
{
	mMaxUsedWidth = 0;
	mMinUsedWidth = 0;
	PageGlyph::ResetLayout();
}

// ======================================================================
//	Layout the columns

void TableGlyph::LayoutCols(DrawPort *drawPort, bool calcMinMaxOnly)
{
	//	Don't normalize the column widths to a sum of 100%.  Some sites have columns that add up to > 100%.
	//	i.e. if two cells have a width of 75%, that means that they should take up to 75% of the width if they
	//	can make use of it.

	short	i, j, cols;
	float	extra;
	CellGlyph*	c;
	PageWidth	usedWidth, widest, h, smallestColspan;
	long		smallestSpan;
	
	CArray<PageWidth,128>	colWidth;
	
	float parentWidth = 0;
	Glyph *parentGlyph = GetParent();
	while (!parentWidth && parentGlyph) {
		if (parentGlyph->IsPage())
			parentWidth = ((PageGlyph *)GetParent())->GetMarginWidth();
		parentGlyph = parentGlyph->GetParent();
	}
	float tableWidth = GetWidth();
	
	// Go sniffing through all of the cells to determine the specified width of
	// each column, and look for some specific types of problems to fix.  Specifically,
	// we're looking at the columns whose widths are specified in % and making sure that
	// they are correct:  for cases where all widths are in percent, the widths should add
	// up to 100%, and for cases where some widths are in percent and some are in pixels, 
	// making sure that the percentages give the correct result when applied against the
	// table width.  We won't touch cases where some of the columns have no width specified
	// at all, since those columns can always soak up extra pixels that are left over.

	// The cw array contains the specified widths of each column.  Zero for unspecified width,
	// positive for width in pixels, negative for width in percent.
	float *cw = (float *)malloc(mColumns * sizeof(float));
	memset(cw, 0, mColumns * sizeof(int));
	for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next()) {
		float width = c->GetKnownWidth();
		if (c->IsKnownWidthPercentage())
			width = -width;
		if (c->GetColSpan() == 1 && cw[c->GetCol()] == 0)
			cw[c->GetCol()] = width;
	}
	
	bool hasUnsizedCol = false;
	float percentageSum = 0;
	float pixelSum = 0;
	for (i = 0; i < mColumns; i++) {
		pprint("Column width of %d: %f\n",i, cw[i]);
		if (cw[i] == 0) {
			hasUnsizedCol = true;
			break;
		} else if (cw[i] < 0)
			percentageSum += -cw[i];
		else
			pixelSum += cw[i];			
	}
	free(cw);


	// Measure all cells (not the caption), measure all the cols
	// Don't use cells with columnSpan != 1 to measure widest
	
	float tableWidthLeft = tableWidth;
	float maxCellWidth;
	bool foundAbsoluteWidth;
	
	pprint("TABLE: LayoutCols 0x%x (Table Width: %f,ParentWidth: %f,Parent::GetWidth(): %f)",this, tableWidth,parentWidth,GetParent()->GetWidth());
	
	for (i = 0; i < mColumns; i++)								// Measure all the columns
	{
		maxCellWidth = 0;
		foundAbsoluteWidth = false;
		
		smallestSpan = -1;
		smallestColspan.min = smallestColspan.max = smallestColspan.known = -1;
		widest.min = widest.max = widest.known = widest.measuredmin = widest.measuredmax = -1;
		
		for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next()) {
			if (c->GetCol() == i)  {
				if ((i + c->GetColSpan()) > mColumns || c->GetColSpan() == 0)			// Limit columns span to the table width.
					c->SetColSpan(mColumns - i);

				usedWidth.known = c->GetKnownWidth();
				if (usedWidth.known && tableWidth) {
					// Deal with the case where one cell in a column specifies a pixel width and a following one specifies some
					// percentage width.  In this case, throw away the percentage width and use only the pixel width.  We don't
					// deal equally well with the case where we see the percent-sized cell first.
					if (c->IsKnownWidthPercentage() && !foundAbsoluteWidth)
						usedWidth.known = (usedWidth.known * (tableWidth - ((mBorder*2) + (mCellPadding*mColumns*2) + (mCellSpacing*(mColumns+1)))) + 50)/100;
					else
						foundAbsoluteWidth = true;
				} else
					usedWidth.known = 0;
				
				//	Layout cell with best guess at final width

#ifndef USE_FAST_TABLE
				c->SetWidth(initialWidth);						// Known size or table width						
				c->Layout(drawPort);							// Layout contents of cell
#endif
				usedWidth.min = c->GetMinUsedWidth(drawPort);	// See how small the cell can be
				usedWidth.measuredmax = c->GetMaxUsedWidth(drawPort);
				usedWidth.measuredmin = usedWidth.min;			// measuredmin may get used if min is too big
				//printf("1 CELL: (%d,%d) Measured min %f and max %f\n",i,c->GetRow(),usedWidth.measuredmin, usedWidth.measuredmax); fflush(stdout);
				
				if (usedWidth.known && usedWidth.known < usedWidth.min)	// If known size was too small for min, override it
					usedWidth.known = usedWidth.min;
				
				// If the cell specified a width, use it for the maximum, and calculate the minimum.
				usedWidth.max = usedWidth.known; //MIN(MAX(usedWidth.min,tableWidth),usedWidth.known);
				if (usedWidth.max > 0 && ((mKnownWidth && !mIsWidthPercentage) || (usedWidth.known && !c->IsKnownWidthPercentage()))) {
					if (c->IsKnownWidthPercentage() == false)
						usedWidth.min = usedWidth.max;
					else if (usedWidth.max > usedWidth.min)
						usedWidth.min = MAX(usedWidth.min,MIN(tableWidth/2, usedWidth.max));
				} else
#if USE_FAST_TABLE
					usedWidth.max = MAX(usedWidth.measuredmax, usedWidth.known);
				// If the maximum width of the cell is greater than the specified width, try to rein it in to the specified width.
				// If it turns out that 100% of the width is not used, then we'll soak up the extra later.
				// If you have a table cell = 1% width the cell will expand to be wide enough to hold its contents without wrapping
				// but will only soak up 1% of the space after that.
				// Rather than come up with a general rule which handles both cases well, I'll hack it to special-case it
				// for cells of, say, 5% width or less.
				if (usedWidth.known && (tableWidth > 0 || !c->IsKnownWidthPercentage()) && usedWidth.max > usedWidth.known &&
					(!c->IsKnownWidthPercentage() || c->GetKnownWidth() > 5))
					usedWidth.max = usedWidth.known;
				if (usedWidth.max > tableWidth && tableWidth > 0)
					usedWidth.max = tableWidth;
#else
					usedWidth.max = c->GetUsedWidth();				// See how wide the cell wants to be
#endif

				//printf("2 CELL: (%d,%d) max, min now  %f, %f\n",i,c->GetRow(),usedWidth.max, usedWidth.min);fflush(stdout);
				// max must be >= min
				
				if (usedWidth.max < usedWidth.min)
					usedWidth.max = usedWidth.min;
				else if ((usedWidth.max - usedWidth.min) <= 16)		// If min and max are close, make them the same
					usedWidth.min = usedWidth.max;
				
				if (usedWidth.min - usedWidth.measuredmin <= 16)	// If min and measured min are close, make them the same
					usedWidth.measuredmin = usedWidth.min;
					
				if (c->IsNoWrap()) {
					// If the cell specifies nowrap, this takes precedence over everything else.  We'll set the minimum
					// and even the measured width to the maximum so that the cell won't get any smaller than it needs
					// and it can't be shrunk later in the second pass to make more room for other cells.
//					usedWidth.measuredmin = usedWidth.min = usedWidth.max;
					usedWidth.min = usedWidth.max = c->GetMaxUsedWidth(drawPort);
				}
				//printf("3 CELL: (%d,%d) Layout at ?, Used [%f min, %f max]\n",i,c->GetRow(),usedWidth.min,usedWidth.max);fflush(stdout);
			
				//	If cell spans one col, record widest
				
				if (c->GetColSpan() == 1) {
					widest.min = MAX(widest.min, usedWidth.min);
					widest.max = MAX(widest.max, usedWidth.max);
					widest.measuredmin = MAX(widest.measuredmin, usedWidth.measuredmin);
					widest.measuredmax = MAX(widest.measuredmax, usedWidth.measuredmax);
					if (usedWidth.known > 0)
						widest.known = MAX(widest.known, usedWidth.known);
					
				// If colspan, record colspan with smallest min.
				
				} else if (smallestSpan == -1 || c->GetColSpan() < smallestSpan ||
						 (c->GetColSpan() == smallestSpan && usedWidth.min > smallestColspan.min)) {
					smallestSpan = c->GetColSpan();
					smallestColspan = usedWidth;
				}
			}
		}

		// If no cell found in this row, use smallest col span
		
		if (widest.min > -1)
			colWidth[i] = widest;
		else
			colWidth[i] = smallestColspan;
			
		tableWidthLeft -= maxCellWidth;

		//printf("4 COL: (%d) done.  colWidth [%f min, %f max]  used [%f min, %f max]\n", i, colWidth[i].min, colWidth[i].max, usedWidth.min, usedWidth.max);fflush(stdout);
		
//		pprint("TABLE: Col %d [%f min, %f max]",i,colWidth[i].min,colWidth[i].max);
	}

	//	Min and Max sizes for all cols are in the colWidth array
	//	Check to see that cells with columnSpan != 1 fit, pad cols if required.
	
	for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next())
	{
		if (c->GetColSpan() != 1)  {
			// Calculate the min used width for cells in span, and expand if necessary to fit
			// min width of spanning column.		
			long maxCount = 0;
			float minUsedWidth = 0, maxUsedWidth = 0;
			float minSpanWidth, maxSpanWidth;

			i = c->GetCol();
			cols = c->GetColSpan();
			for (j = i; j < i + cols; j++)
				minUsedWidth += colWidth[j].min + (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
				
			minSpanWidth = c->GetMinUsedWidth(drawPort) + (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
			//minSpanWidth = c->GetMinUsedWidth(drawPort);
			
//			pprint("SPAN-CELL: (%d,%d) [%d]: minUsedWidth %f, minSpanWidth %f", i, c->GetRow(), c->GetColSpan(), minUsedWidth, minSpanWidth);
			if (c->GetKnownWidth() && c->IsKnownWidthPercentage() == false)	// Absolute pixel width
				minSpanWidth = c->GetKnownWidth();

			if (minUsedWidth < minSpanWidth) {
				// Count the columns with max > min.
				for (j = i; j < i + cols; j++)
					if (colWidth[j].max > colWidth[j].min)
						maxCount++;
				
				// Take the space from columns with max > min, where possible.		
				if (maxCount > 0) {				
					extra = ((minSpanWidth - minUsedWidth) + maxCount/2) / maxCount;					
					for (j = i; j < i + cols; j++) {
						if (colWidth[j].max > colWidth[j].min) {
							colWidth[j].min += extra;
							colWidth[j].max = MAX(colWidth[j].max, colWidth[j].min); // Max must be at least min.
						}
					}
				}
				else {
					extra = ((minSpanWidth - minUsedWidth) + cols/2) / cols;					
					for (j = i; j < i + cols; j++) {
						colWidth[j].min += extra;
						colWidth[j].max = MAX(colWidth[j].max, colWidth[j].min); // Max must be at least min.
					}
				}
			}
			
			// Calculate the max used width for cells in span, and expand if necessary to fit
			// max width of spanning column.
			maxCount = 0;	
			for (j = i; j < i + cols; j++)
				if (colWidth[j].max > 0) {
					maxUsedWidth += colWidth[j].max + (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
					maxCount++;
				}
			maxSpanWidth = c->GetMaxUsedWidth(drawPort) + (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
//			pprint("SPAN-CELL: (%d,%d) [%d]: maxUsedWidth %f, maxSpanWidth %f", i, c->GetRow(), c->GetColSpan(), maxUsedWidth, maxSpanWidth);
			if (c->GetKnownWidth() && c->IsKnownWidthPercentage() == false)	// Absolute pixel width
				maxSpanWidth = c->GetKnownWidth();
			
			if (maxUsedWidth < maxSpanWidth && maxCount > 0) {
				extra = ((maxSpanWidth - maxUsedWidth) + maxCount/2) / maxCount;					
				for (j = i; j < i + cols; j++)
					if (colWidth[j].max > 0)
						colWidth[j].max += extra;
			}
			
		}
	}

	//	See if we need to widen or shrink the table to meet a width requirement
	//	Calculate the maximum and minimum table widths
	h.min = h.max = h.measuredmax = (mBorder << 2) + mCellSpacing;
	float extracellpadding = (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
	for (j = 0; j < mColumns; j++) {
		 h.min += colWidth[j].min + extracellpadding;
		 h.max += colWidth[j].max + extracellpadding;
		 h.measuredmax += colWidth[j].measuredmax + extracellpadding;
	}
//	pprint("TABLE: Table Width [%f min, %f max]",h.min,h.max);


	mMinUsedWidth = h.min;
	mMaxUsedWidth = h.max;
	
	if (mKnownWidth > 0 && !mIsWidthPercentage) {
		mMinUsedWidth = MIN(mMinUsedWidth, mKnownWidth);
		mMaxUsedWidth = MIN(mMaxUsedWidth, mKnownWidth);
	}
	
	if (calcMinMaxOnly)
		return;
		
	// If the cells will not fit horizontally, we shrink them using the following alogrithm, taken from
	// WD-tables-960123 Note that the final calculated values are placed in PageWidth::min.

	//	See if the table width is smaller than our parent, expand if required
	if (h.min > tableWidth && tableWidth < parentWidth)
		tableWidth = MIN(h.min, parentWidth);
		
	// If the table max doesn't fit but the measuredmax does, try to shrink
	// the max down to get it to fix.
	if (h.max > tableWidth && h.measuredmax <= tableWidth) {
		float extra = h.max - tableWidth;
		for (j = 0; j < mColumns; j++) {
			float shrinkage = colWidth[j].max - colWidth[j].measuredmax;
			if (shrinkage > 0) {
				float oldmax = colWidth[j].max;
				colWidth[j].max -= MIN(shrinkage, extra);
				float diff = oldmax - colWidth[j].max;
				h.max -= diff;
				extra -= diff;
			}
		}
	}

	if (h.max <= tableWidth) {
		for (j = 0; j < mColumns; j++)
			colWidth[j].min = colWidth[j].max;

		if (mKnownWidth == 0) {
		
			// No width specified and max fits, so just use it.
			
//			for (j = 0; j < mColumns; j++)
//				colWidth[j].min = colWidth[j].max;
		} else {
		
			// A width was specified, widen the cells equally to fit it.
			// Widen only the columns with non-explicit size.
			// If all cells have explict size, then widen each column in proportion to its stated width.
			
			long totalUnknownWidthCount = 0;
			float totalKnownWidth = 0;
			for (j = 0; j < mColumns; j++)
				if (colWidth[j].max > 0) {
					if (colWidth[j].known <= 0 || mColumns == 1)
						totalUnknownWidthCount++;
					else
						totalKnownWidth += colWidth[j].known;
				}
					
			if (totalUnknownWidthCount == 0 && mColumns == 1) {
				// Force the column out to the table width no matter what it wants to do.
				colWidth[0].min = colWidth[0].max = tableWidth - h.max;
			} else {		
				float leftover = (tableWidth - h.max);
				for (j = 0; j < mColumns; j++) {
					if (colWidth[j].max > 0) {
						if (totalUnknownWidthCount)
							extra = leftover / totalUnknownWidthCount;
						else
							extra = leftover * (colWidth[j].known/totalKnownWidth);
						if (!totalUnknownWidthCount || colWidth[j].known <= 0 || mColumns == 1)
							colWidth[j].min = colWidth[j].max + extra;
					}
				}
			}
		}
	} else if (h.min < tableWidth && h.min < h.max) {
	
		// Min fits but max doesn't. Expand min size to fill width using above algorithm.
		
		float W = tableWidth - h.min;
		float D = h.max - h.min;
		for (j = 0; j < mColumns; j++)
			colWidth[j].min = colWidth[j].min + (((colWidth[j].max - colWidth[j].min) * W + (D/2)) / D);
	
	} else if (h.min > tableWidth) {
	
		// 1. Shrink Known Size cells that don't have much in them
		
//		Measure the smallest possible table width

		float oversizeCount = 0;
		float mmin = (mBorder << 2) + mCellSpacing;
		for (j = 0; j < mColumns; j++) {
			if (colWidth[j].min > colWidth[j].measuredmin) {
				mmin += colWidth[j].measuredmin + (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
				oversizeCount++;
			} else {
				mmin += colWidth[j].min + (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
			}
		}
		
		if (mmin < h.min && oversizeCount) {
			float spaceNeeded = h.min - tableWidth;		// Need this much space to fit
			float spaceAvail = h.min - mmin;				// This much space is avaliable by using measuredmin
			spaceNeeded = MIN(spaceNeeded,spaceAvail);	// May not reach our goals
			
			pprint("Table does not fit (width %f, min %f, measured min %f)",tableWidth,h.min,mmin);
			pprint("Need %f pixels from %f avail",spaceNeeded,spaceAvail);
			
			for (j = 0; j < mColumns; j++)
				if (colWidth[j].min > colWidth[j].measuredmin) {
					float shrink = colWidth[j].min - colWidth[j].measuredmin;
					pprint("Col %d can shrink from %f to %f (%f)",j,colWidth[j].min ,colWidth[j].measuredmin,colWidth[j].min - colWidth[j].measuredmin);
					shrink = (spaceNeeded*shrink)/spaceAvail;			// Shrink proportionatly
					pprint("Shrinking Col %d from %f to %f",j,colWidth[j].min,colWidth[j].min - shrink);
					colWidth[j].min -= shrink;
				}
		}

		// 2. Shrink borders
		while (h.min > tableWidth && (mBorder > 1 || mCellBorder > 1))
		{
			if (mBorder > 1) {
				mBorder = mBorder - 1;
				h.min -= 2;
			}
			if (mCellBorder > 1) {
				mCellBorder = mCellBorder -1;
				h.min -= mColumns * 2;
			}
		}

		// 3. Shrink CellPadding and CellSpacing
		while (h.min > tableWidth && (mCellPadding > 1 || mCellSpacing > 1))
		{
			if (mCellPadding > 1) {
				mCellPadding = mCellPadding - 1;
				h.min -= mColumns * 2;
			}
			if (mCellSpacing > 1) {
				mCellSpacing = mCellSpacing - 1;
				h.min -= mColumns + 1;
			}
		}

		// 4. Finally, force cell sizes smaller, resulting in broken text and scaled images. We compute the
		// average size column, and then force all larger columns smaller.
		
		if (false && h.min > tableWidth)
		{
			float averageWidth = MAX(tableWidth/mColumns - (mCellPadding << 1) - mCellSpacing - (mCellBorder << 1), 0);

			// Count colums > average width.
			float oversizeCount = 0;
			for (j = 0; j < mColumns; j++)
				if (colWidth[j].min > averageWidth)
					oversizeCount++;

			if (oversizeCount) {
				extra = ((h.min - tableWidth) + oversizeCount/2)/oversizeCount;
				for (j = 0; j < mColumns; j++)
					if (colWidth[j].min > averageWidth)
						colWidth[j].min -= extra;
			}
		}
	}

	// Position cells horizontally
	
	float hPos = GetLeft() + mBorder + mCellBorder + mCellSpacing + mCellPadding;
	for (i = 0; i < mColumns; i++) {
		for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next()) {
			if (c->GetCol() == i)
				c->SetLeft(hPos);						// Set left edge
		}
		hPos += colWidth[i].min;
		for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next()) {
			if (i == (c->GetCol() + c->GetColSpan() - 1)) {
			
				float width = hPos - c->GetLeft();
				if (width != c->GetWidth()) {
//					pprint("CELL: (%d,%d) Relayout from %f to %f",c->GetCol(),c->GetRow(),c->GetWidth(),width);
					c->SetWidth(width);						// Set right edge
					if (width > 0 || c->GetWidth() > 0)
						c->Layout(drawPort);				// Layout again, centering in cell, etc
				} else {
//					pprint("CELL: (%d,%d) Relayout not needed for %f",c->GetCol(),c->GetRow(),width);
				}
					
			}
		}
		if (colWidth[i].min > 0)
			hPos += (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
	}
	hPos += mBorder - mCellBorder - mCellPadding;
	
	mMaxHPos = hPos;

	// Don't make the table narrower than 1.  Zero can mean that we haven't laid out yet.
	SetWidth(MAX(hPos - GetLeft(),1));
}

void TableGlyph::LayoutRows(DrawPort *drawPort)
{
	float v, tallest;
	float usedHeight, spanHeight;
	short i, j, rows;
	CellGlyph* c;
	CArray<float,128>	rowHeight;
	
	if (!mOldRowHeights) {
		mOldRowHeights = (float *)malloc(mRows * sizeof(float));
		for (int i = 0; i < mRows; i++)
			mOldRowHeights[i] = -1;
	}
	
	// Measure the tallest cell in each Column

	for (i = 0; i < mRows; i++)
	{
		int relayoutRow = 0; // 0 - No need to relayout the row, 1 - Found a cell that needs it, 2 - In second pass, relaying out cells.
		
		do {
			tallest = -1;
			for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next())
			{
				if (c->GetRow() == i)  {
					if (relayoutRow == 2)
						c->Layout(drawPort);
						
					if ((i + c->GetRowSpan()) > mRows || c->GetRowSpan() == 0)			// Limit columns span to the table width.
						c->SetRowSpan(mRows - i);
	
					if (c->GetRowSpan() == 1) {
/*
						if (relayoutRow == 0 && c->GetHeight() != mOldRowHeights[i] && mOldRowHeights[i] != -1) {
							// The row height is different than the last time we laid out
							// rows.  Iterate through each cell in the row and force a layout
							// of it.  This will guarantee that the cell has an opportunity
							// to relayout if the height has changed, since it may be skipped
							// over in LayoutCols if its column width hasn't changed.  This is
							// a bit inefficient, as cells whose width and height have both
							// changed will get laid out twice, but this inefficiency is hard
							// to deal with unless we combine LayoutRows and LayoutCols.
							relayoutRow = 1;
						}
*/
						tallest = MAX(tallest, MAX(c->GetHeight(),c->GetKnownHeight()));
					}
				}
			}
			if (tallest != mOldRowHeights[i] && relayoutRow == 0) {
				relayoutRow = 1;
			}
			
			rowHeight[i] = tallest;
			
			if (relayoutRow == 1)
				relayoutRow = 2;
			else
				relayoutRow = 0;
	
		} while (relayoutRow != 0);

		mOldRowHeights[i] = rowHeight[i];
	}

	// Check to see that cells with rowspan != 1 fit, pad rows if required

	for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next())
	{
		if (c->GetRowSpan() > 1)
		{
			usedHeight = 0;
			i = c->GetRow();
			rows = c->GetRowSpan();
			for (j = i; j < i + rows; j++)
				usedHeight += rowHeight[j] + (mCellPadding << 1);
			spanHeight = c->GetHeight() + (mCellPadding << 1);

			if (usedHeight < spanHeight) {
				float extra = ((spanHeight - usedHeight) + rows/2)/rows;
				for (j = i; j < i + c->GetRowSpan(); j++)
					rowHeight[j] += extra;
			}
		}
	}

	// Calculate the height of each Column, position cells vertically

	v = GetTop() + mBorder + mCellBorder + mCellSpacing + mCellPadding;
	for (i = 0; i < mRows; i++) {
		for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next()) {
			if (c->GetRow() == i)
				c->SetTop(v);						// Set top
		}
		v += rowHeight[i];
		for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next())  {
			if (i >= c->GetRow() && i < (c->GetRow() + c->GetRowSpan())) {
				c->VAlignCell(v - c->GetTop());		// Set bottom, vertically align cell
			}
		}
		v += (mCellPadding << 1) + mCellSpacing + (mCellBorder << 1);
	}
	v += mBorder - mCellBorder - mCellPadding;	// mCellBorder was already added twice above
	mHeight = v - GetTop();
}

void TableGlyph::LayoutCaption(DrawPort *drawPort)
{
	// Can't layout caption it until the end because width and heght are uncertain

	mCaption->SetWidth(GetWidth() - (mCellPadding << 1));		// Know how wide table is
	mCaption->Layout(drawPort);								// Determine height
	mCaption->VAlignCell(mCaption->GetHeight() + (mCellPadding << 1));

	if (mCaptionAlign == AV_BOTTOM)
		mCaption->SetTop(GetTop() + GetHeight());
	else {
		float top = GetTop();
		SetTop(top + mCaption->GetHeight());
		mTop = top;
		mCaption->SetTop(top);
	}
	SetHeight(GetHeight() + mCaption->GetHeight());
}

// ==========================================================================================
//	Layout is where the tricky stuff happens
//	Figure the width of the columns, position the cells in X
//	Figure the height of the rows, position the cells in Y
//	Position the caption, if any

void TableGlyph::Layout(DrawPort *drawPort)
{
	mLayoutComplete = false;

	if (GetChildren() == NULL)
		return;				// Nothing in table to layout

	DetermineWidth();

	LayoutCols(drawPort);	// Position in X
	LayoutRows(drawPort);	// Position in Y
	if (mCaption)
		LayoutCaption(drawPort);
}

void TableGlyph::DetermineWidth()
{
	float parentMarginWidth = ((PageGlyph *)GetParent())->GetMarginWidth();
	if (mKnownWidth) {					// Determine width of table
		float width = mIsWidthPercentage ? (mKnownWidth * parentMarginWidth + 50)/100 : mKnownWidth;
		if (width > parentMarginWidth && parentMarginWidth > 0) {
			pprint("Shrinking table width from %f to %f to fit parent", width, parentMarginWidth);
			width = parentMarginWidth;	//¥¥¥¥¥¥¥¥¥¥
		}
		SetWidth(width);
	} else
		SetWidth(0);					// Indicate that we don't know how big the table should be
}

bool TableGlyph::IsLayoutComplete()
{
	return mLayoutComplete;
}

void TableGlyph::LayoutComplete(DrawPort *)
{
	mLayoutComplete = true;
}

//	Draw known details about table

#ifdef DEBUGMENU
void TableGlyph::DrawInfo(DrawPort *drawPort)
{
	BRect r;
	GetBounds(&r);
	
	BString info;
	char str[256];
	sprintf(str,"Table (%dx%d)",mRows,mColumns);
	info = str;
	if (mKnownWidth) {
		if (mIsWidthPercentage)
			sprintf(str," %f%%",mKnownWidth);
		else {
			if (mWidth != mKnownWidth)
				sprintf(str," %f{%f}",mKnownWidth,mWidth);
			else
				sprintf(str," %f",mKnownWidth);
		}
		info += str;
	}
	float width = drawPort->TextWidth(info.String(),info.Length());
	
	Style s = {1,0,0,0,0,0,0,0,0};
	drawPort->SetStyle(s);
	
	r.top = r.bottom - 16;
	r.left = r.right - (width + 2);
	drawPort->EraseRect(&r);
	drawPort->DrawText(r.left+1,r.top+1,info.String(),info.Length(),0);

	GetBounds(&r);
	r.InsetBy(-1,-1);
	drawPort->FrameRect(&r);
}

bool gDrawInfo = false;
bool gDrawCell = true;
#endif

//	Draw the border of the table
//	Draw all the children, draw a border around them if required

void TableGlyph::Draw(DrawPort *drawPort)
{
//	BWindow *window = ((BeDrawPort *)drawPort)->GetView()->Window();
//	if (!window->Lock())
//		return;

	BRect clip = drawPort->GetClip();

//	Draw border around outside of table

	BRect r;
	GetBounds(&r);
	if (mBorder) {
		if (mCaption) {
			if (mCaptionAlign == AV_BOTTOM)
				r.bottom -= mCaption->GetHeight();
			else
				r.top += mCaption->GetHeight();
		}
		for (short i = mBorder; i--;) {
			drawPort->DrawAntiBevel(&r);
			r.InsetBy(1,1);
		}
	}
	
	if (mBGColor != -1) {
		drawPort->SetColor(mBGColor);
		drawPort->PaintRect(&r);
	}
	
//	Draw cells
//	Would it be simpler to do a pass of all cells that have a border?
//	Far fewer SetColor ops for the bevel, etc...

	CellGlyph* c;
	for (c = (CellGlyph *)GetChildren(); c; c = (CellGlyph *)c->Next()) {
		c->GetBounds(&r);
		bool empty = r.left >= r.right;
		
//		Don't border empty cells or captions
	
		long bgColor = c->GetBGColor();
		
		if (mCellBorder && c->GetChildren() && c != mCaption) {	
			r.InsetBy(-mCellPadding - mCellBorder,-mCellPadding - mCellBorder);
			if (r.top > clip.bottom)
				continue;
			if (r.Intersects(clip)) {
				if (!empty)
					for (short i = mCellBorder; i--;) {
						drawPort->DrawBevel(&r);
						r.InsetBy(1,1);
					}
				if (bgColor != -1) {
					drawPort->SetColor(bgColor);
					drawPort->PaintRect(&r);
				}
#ifdef DEBUGMENU
				if (gDrawInfo)
					c->DrawInfo(drawPort);
				if (gDrawCell)
#endif
					c->Draw(drawPort);
			}
		} else {
			if (r.top > clip.bottom)
				continue;
			r.InsetBy(-mCellPadding,-mCellPadding);
			if (r.Intersects(clip)) {
				if (bgColor != -1) {
					drawPort->SetColor(bgColor);
					drawPort->PaintRect(&r);
				}
#ifdef DEBUGMENU
				if (gDrawInfo)
					c->DrawInfo(drawPort);
				if (gDrawCell)
#endif
					c->Draw(drawPort);
			}
		}
	}
	
//	Draw info about the table, if req

#ifdef DEBUGMENU
	if (gDrawInfo)
		DrawInfo(drawPort);
#endif

//	window->Unlock();
}
