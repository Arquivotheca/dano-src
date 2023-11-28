//========================================================================
//	MListView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	JW

#include <Beep.h>
#include <List.h>
#include <stdlib.h>

#include "MListView.h"
#include "MHiliteColor.h"
#include <Message.h>
#include <Window.h>
#include <ScrollBar.h>

inline int32 Min(int32 x, int32 y) { return x < y ? x : y; }
inline int32 Max(int32 x, int32 y) { return x > y ? x : y; }

//	No overlap when scrolling by page
//const float OVERLAP = 0.0;
const float OVERLAP = 16.0;
//	Some pixels for scrolling
const float SCROLL_MARGIN = 10.0;



class StSetter
{
		bool &					fTheRef;
		bool					fOldValue;
public:
inline							StSetter(bool & theBool, bool value = TRUE) :
									fTheRef(theBool)
								{
									fOldValue = theBool;
									theBool = value;
								}
inline							~StSetter()
								{
									fTheRef = fOldValue;
								}
};



MListView::MListView(
	BRect area,
	const char * name,
	uint32 resize,
	uint32 flags) :
	BView(
		area,
		name,
		resize,
		flags)
{
	fFontHeight.ascent = 10;
	fFontHeight.descent = 3;
	fFontHeight.leading = 3;

	fRowHeight = 16.0;
	fList = new BList;
	fOwnsList = TRUE;
	fAttached = FALSE;
	fAllowMulti = TRUE;
	fDragSelection = TRUE;
	fDragNDropCells = false;
	fEnabled = true;
	fClickCount = 0;
	fClickTime = 0;
	fClickButtons = 0xffffffff;
}


MListView::~MListView()
{
	DeleteList();
}


bool
MListView::RowData::Delete(
	void * data)
{
	delete (RowData *)data;
	return FALSE;
}


void
MListView::Draw(
	BRect area)
{
	if (!fList)
		return;
	
	int32 cur = GetPosRow(area.top);
	int32 stop = GetPosRow(area.bottom);

	if (cur >= CountRows())
		return;

	BRect bounds = Bounds();
	BRect rowRect = bounds;
	BRect sectRect;

	float pos = GetRowPos(cur) - 1.0;
	
	while (cur <= stop)
	{
		RowData * data = GetRowData(cur);
		rowRect.top = pos + 1.0;
		pos = data->fEnd;
		rowRect.bottom = pos;
		rowRect.left = 0.0;
		rowRect.right = bounds.right;
		sectRect = rowRect & area;

		if (data->fSelected)
		{
			HiliteRow(cur, rowRect);
		}
		else
		{
			DrawRow(cur, fList->ItemAt(cur), rowRect, sectRect);
		}
		cur++;
	}
}


void
MListView::MouseDown(
	BPoint where)
{
	if (! IsEnabled())
		return;

	StSetter set(fInClick);
	MakeFocus();

	int32 			row = GetPosRow(where.y);
	bigtime_t 		clickSpeed = 0;
	bigtime_t		nowTime = system_time();
	get_click_speed(&clickSpeed);
	BMessage* msg = Window()->CurrentMessage();
	uint32 modKeys = msg->FindInt32("modifiers");
	uint32 buttons = msg->FindInt32("buttons");

	if (clickSpeed > (nowTime - fClickTime) && IsRowSelected(row) && buttons == fClickButtons) 
	{
		if (fClickCount > 1) 
		{
			// triple click
			fClickCount = 0;
			fClickTime = 0;
			fClickButtons = buttons;
		}
		else 
		{
			// double click
			fClickCount = 2;
			fClickTime = nowTime;
			fClickButtons = buttons;
		}
	}
	else 
	{
		// new click
		fClickCount = 1;
		fClickTime = nowTime;
		fClickButtons = buttons;
	}

	if (where.y < 0.0 || where.y >= GetMaxYPixel())
	{
		if (CountRows())
			SelectRow(0, FALSE, FALSE);
		return;
	}

	if (fClickCount == 2 && IsRowSelected(row))
	{
		InvokeSelection();
	}
	else
	{

		if (ClickHook(where, row, modKeys, buttons))
			return;

		bool 	fToSelect = TRUE;
		bool 	fKeepOld = fAllowMulti && IsRowSelected(row);
		bool 	fExtendSelect = FALSE;
		bool	doDragNDrop = false;
//		uint32	modKeys = modifiers();
		
		if (fAllowMulti && (modKeys & B_COMMAND_KEY) != 0L)
		{
			fToSelect = !IsRowSelected(row);
			fKeepOld = TRUE;
		}
		else 
		if (fAllowMulti && (modKeys & B_SHIFT_KEY) != 0L)
		{
			fExtendSelect = TRUE;
			fToSelect = TRUE;
		}
		else
		{
			fExtendSelect = fAllowMulti && ! fDragNDropCells;	//	drag-select?
		}

		int32		anchorRow;
		
		if (fExtendSelect)
		{
			int32		lastRow;

			if (0 == FirstSelected(anchorRow))
				anchorRow = lastRow = row;
			else
			if (anchorRow > row)
			{
				anchorRow = row;
				lastRow = CountRows();
				PreviousSelected(lastRow);
			}
			else
				lastRow = row;
			
			SelectRows(anchorRow, lastRow, fKeepOld, fToSelect);
		}
		else
		{
			SelectRow(row, fKeepOld, fToSelect);
		}

		if (fDragNDropCells)
		{
			if (WaitMouseUp(where))
			{
				InitiateDrag(where, row);
				StartAutoScroll();
			}
		}
		else
		if (fDragSelection)
		{
			while (TRUE)
			{
				bool		scrolled = false;
				uint32 		btn = 0;
				BPoint		pt;
				Flush();
				snooze(30000);
				GetMouse(&pt, &btn);
				if (!btn)
				{
					break;
				}
				
				//	Autoscroll
				if (pt.y > Bounds().bottom)
				{
					scrolled = BoundedScroll(0, fRowHeight);
				}
				else if (pt.y < Bounds().top)
				{
					scrolled = BoundedScroll(0, -fRowHeight);
				}
				if (scrolled)
				{
					AdjustScrollbar();
					Window()->UpdateIfNeeded();		// GetMouse calls this but here is faster
				}

				int32 newRow = GetPosRow(pt.y);
				if (newRow != row)
				{
					if (fExtendSelect)	//	Shift-drag
					{
						int32 mi = anchorRow < newRow ? anchorRow : newRow;
						int32 mx = anchorRow > newRow ? anchorRow : newRow;
						SelectRows(mi, mx, FALSE, fToSelect);
					}
					else if (!fKeepOld)
					{
						//	Do this since it's faster than de-selecting all each time
						SelectRow(row, TRUE, FALSE);
						SelectRow(newRow, TRUE, fToSelect);
					}
					else
					{
						SelectRow(newRow, TRUE, fToSelect);
					}
					row = newRow;
				}
			}
		}
	}
}


void
MListView::StartAutoScroll()
{
	SetFlags(Flags() | B_PULSE_NEEDED);				// turn on pulse
	fAutoScrolling = true;
	Window()->SetPulseRate(100000);
}


void
MListView::Pulse()
{
	if (fAutoScrolling)
	{
		BPoint		newLoc;
		uint32		buttons;
	
		GetMouse(&newLoc, &buttons);

		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0)
		{
			fAutoScrolling = false;
			SetFlags(Flags() & ~B_PULSE_NEEDED);	// turn off pulse
		}
		else
		{
			float		delta = 0.0;
			BRect		r = Bounds();
			
			r.bottom = r.top;
			r.top -= B_H_SCROLL_BAR_HEIGHT;
			
			if (r.Contains(newLoc))
				delta = -1;
			else
			{
				r.top = Bounds().bottom;
				r.bottom = r.top + B_H_SCROLL_BAR_HEIGHT;
				
				if (r.Contains(newLoc))
					delta = 1.0;
			}
			
			if (delta != 0.0)
			{
				delta *= fRowHeight;
				BoundedScroll(0.0, delta);
			}
		}
	}
}


inline float
MListView::DiffSquared(float a, float b)
{
	return a > b ? (a - b) * (a - b) : (b - a) * (b - a);
}

const float kDragDist = 3.0;

bool
MListView::WaitMouseUp(
	BPoint inPoint)
{
	while (true)
	{
		BPoint		newLoc;
		uint32		buttons;
	
		GetMouse(&newLoc, &buttons);
		
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0)
			return false;
		else
		if (DiffSquared(inPoint.x, newLoc.x) + DiffSquared(inPoint.y, newLoc.y) > kDragDist * kDragDist)
			return true;
		else
			snooze(30000);
	}
}


void
MListView::FrameResized(
	float newWidth,
	float newHeight)
{
	BView::FrameResized(newWidth, newHeight);
	AdjustScrollbar();
}


void
MListView::KeyDown(
	const char *bytes, 
	int32 		numBytes)
{
	switch (bytes[0])
	{
	case 10:	//	Invoke ALL selected
	case 13:
		InvokeSelection();
		break;
	case B_UP_ARROW:
		SelectPrev(fAllowMulti && ((modifiers() & B_SHIFT_KEY) != 0));
		break;
	case B_DOWN_ARROW:
		SelectNext(fAllowMulti && ((modifiers() & B_SHIFT_KEY) != 0));
		break;
	case B_PAGE_UP:
		PageUp();
		break;
	case B_PAGE_DOWN:
		PageDown();
		break;
	case B_HOME:
		ScrollTo(0.0, 0.0);
		AdjustScrollbar();
		break;
	case B_END:
		if (BoundedScroll(0, GetMaxYPixel()-Bounds().Height()-1.0))
			AdjustScrollbar();
		break;
	default:
			BView::KeyDown(bytes, numBytes);
		break;
	}
}


void
MListView::InvokeSelection()
{
	int32 row;
	if (FirstSelected(row))
		do
			InvokeRow(row);
		while (NextSelected(row));
}


void
MListView::SelectPrev(
	bool	inKeepOld)
{
	int32 row = CountRows();

	FirstSelected(row);

	if (row > 0)
	{
		SelectRow(row-1, inKeepOld);
		ScrollRowIntoView(row - 1);
	}
}


void
MListView::SelectNext(
	bool	inKeepOld)
{
	int32 row = -1;
	while (NextSelected(row))
		;
	if (row < CountRows()-1)
	{
		SelectRow(row+1, inKeepOld);
		ScrollRowIntoView(row + 1);
	}
}


void
MListView::PageUp()
{
	if (BoundedScroll(0, fRowHeight - Bounds().Height()))
		AdjustScrollbar();
}


void
MListView::PageDown()
{
	if (BoundedScroll(0, Bounds().Height() - fRowHeight))
		AdjustScrollbar();
}


void
MListView::AttachedToWindow()
{
	BView::AttachedToWindow();
	fAttached = TRUE;
	UpdateFontHeight();
	SetDefaultRowHeight();
	AdjustScrollbar();
}


// BView version isn't virtual
void
MListView::SetFont(
	const BFont *	font, 
	uint32 			mask)
{
	BView::SetFont(font, mask);
	UpdateFontHeight();
}


void
MListView::SetFontSize(
	float size)
{
	BView::SetFontSize(size);
	UpdateFontHeight();
}

void
MListView::UpdateFontHeight()
{
	GetFontHeight(&fFontHeight);
	fFontHeight.ascent = ceil(fFontHeight.ascent);
	fFontHeight.descent = ceil(fFontHeight.descent);
	fFontHeight.leading = ceil(fFontHeight.leading);
}


void
MListView::SyncList()
{
	if (!fList || !fList->CountItems())
	{
		fRowData.DoForEach(RowData::Delete);
		fRowData.MakeEmpty();
	}
	else
	{
		while (fRowData.CountItems() > fList->CountItems())
		{
			delete (RowData *)fRowData.LastItem();
			fRowData.RemoveItem(fRowData.CountItems()-1);
		}
		RowData * end = (RowData *)fRowData.LastItem();
		RowData nilData = { 0.0, 0 };
		if (!end)
			end = &nilData;
		while (fRowData.CountItems() < fList->CountItems())
		{
			RowData * data = new RowData;
			data->fEnd = end->fEnd + fRowHeight;
			data->fSelected = FALSE;
			fRowData.AddItem(data);
			end = data;
		}
	}
	AdjustScrollbar();
}


void
MListView::SetList(
	BList * list,
	bool ownsList)
{
	if (list != fList)
		DeleteList();
	fList = list;
	fOwnsList = ownsList;
	SyncList();
}


float
MListView::GetMaxYPixel() const
{
	if (!CountRows())
		return 0.0;
	return ((RowData *)fRowData.LastItem())->fEnd+1.0;
}


void
MListView::SetDefaultRowHeight(
	float height)
{
	if (height < 1.0)
		fRowHeight = ceil(fFontHeight.ascent + fFontHeight.descent + fFontHeight.leading);
	else
		fRowHeight = ceil(height);
}


void
MListView::InsertRow(
	int32 asNo,
	void * data,
	float height)
{
	if (asNo < 0)
	{
		DEBUGGER("InsertRow row number negative!\n");
		asNo = 0;
	}

	ASSERT(fList != NULL);
	if (!fList)
		return;

	if (height < 1.0)
		height = fRowHeight;
	if (asNo > fList->CountItems())
	{
		DEBUGGER("InsertRow row number too large!\n");
		asNo = fList->CountItems();
	}
	RowData * newData = new RowData;
	newData->fEnd = asNo>0 ? GetRowEnd(asNo-1) : -1.0;
	newData->fSelected = FALSE;
	fRowData.AddItem(newData, asNo);
	fList->AddItem(data, asNo);
	AdjustRowsFrom(asNo, height, TRUE);
}


void
MListView::DoDeleteRows(
	int32 fromNo,
	int32 numRows,
	bool callDelete)
{
	ASSERT(fList != NULL);
	if (!fList)
		return;

	if (numRows <= 0)
		return;

	if (fromNo < 0 || fromNo >= CountRows())
	{
		DEBUGGER("DeleteRows row number out of range!\n");
		return;
	}

	if (fromNo + numRows > CountRows())
		numRows = CountRows() - fromNo;

	float 		delPos = GetRowPos(fromNo);
	float 		delHeight = GetRowPos(fromNo + numRows) - delPos;
	int32		row = fromNo + numRows;

	while (--row >= fromNo)
	{
		delete (RowData *) fRowData.ItemAt(row);
		if (callDelete)
			DeleteItem(fList->ItemAt(row));
	}

	fList->RemoveItems(fromNo, numRows);
	fRowData.RemoveItems(fromNo, numRows);

	AdjustRowsFrom(fromNo, -delHeight, TRUE);
	if (Bounds().bottom >= GetMaxYPixel())
		if (BoundedScroll(0, 0))
			AdjustScrollbar();
}


void
MListView::SetRowHeight(
	int32 row,
	float height)
{
	if (height < 1.0)
		height = fRowHeight;

	if (row < 0 || row >= CountRows())
	{
		DEBUGGER("SetRowHeight row number invalid!\n");
		return;
	}

	float top = GetRowPos(row);
	RowData * data = GetRowData(row);
	float delta = (top + height) - data->fEnd;
	AdjustRowsFrom(row, delta, TRUE);
}


void
MListView::InvalidateRow(
	int32 row)
{
	ASSERT(row >= 0 && row < CountRows());
	BRect area;
	GetRowRect(row, &area);
	Invalidate(area);
}


bool
MListView::IsRowSelected(
	int32 row) const
{
	if (row < 0 || row >= CountRows())
	{
		DEBUGGER("IsRowSelected() called with invalid row!\n");
		return FALSE;
	}
	RowData * data = GetRowData(row);
	return data->fSelected;
}

// returns the first selected row after the specified row
// and how many rows are selected between this row and the
// end of the table

int32
MListView::FirstSelected(
	int32 & selRow) const
{
	int32 ret = INVALID_ROW;
	int32 cnt = 0;
	int32 ptr = CountRows();
	while (ptr-- > 0)
	{
		if (IsRowSelected(ptr))
		{
			ret = ptr;
			cnt++;
		}
	}
	if (cnt > 0)
		selRow = ret;
	return cnt;
}


int32
MListView::LastSelected() const
{
	int32		lastSelected = INVALID_ROW;
	int32		row = CountRows();
	
	while (row-- > 0)
	{
		if (IsRowSelected(row))
		{
			lastSelected = row;
			break;
		}
	}
	
	return lastSelected;
}


bool
MListView::NextSelected(
	int32 & selIter) const
{
	int32 l = selIter;
	while (++l < CountRows())
	{
		if (IsRowSelected(l))
		{
			selIter = l;
			return TRUE;
		}
	}
	return FALSE;
}


bool
MListView::PreviousSelected(
	int32 & selIter) const
{
	int32 l = selIter;
	while (--l >= 0)
	{
		if (IsRowSelected(l))
		{
			selIter = l;
			return TRUE;
		}
	}
	return FALSE;
}


void
MListView::SelectRow(
	int32 row,
	bool keepOld,
	bool toSelect)
{
	if (row < 0 || row >= CountRows())
	{
		DEBUGGER("SelectRow() called with invalid row!\n");
		return;
	}
	if (!keepOld || !fAllowMulti)
	{
		int32 cnt = CountRows();
		while (cnt-- > 0)
			if (cnt != row)
				DoSelectRow(cnt, FALSE);
	}
	DoSelectRow(row, toSelect);
}


void
MListView::SelectRows(
	int32 fromRow,
	int32 toRow,
	bool keepOld,
	bool toSelect)
{
	if (fromRow > toRow || fromRow < 0 || toRow >= CountRows())
	{
		DEBUGGER("SelectRows() with invalid row data!\n");
		return;
	}
	if (!keepOld)
	{
		int32 cnt = CountRows();
		while (cnt-- > 0)
		{
			if (cnt <= toRow && cnt >= fromRow)
				continue;
			DoSelectRow(cnt, FALSE);
		}
	}

	while (fromRow <= toRow)
	{
		DoSelectRow(fromRow, toSelect);
		fromRow++;
	}
}


void
MListView::SelectAll(
	bool	inSelect)
{
	int32 cnt = CountRows();
	while (cnt-- > 0)
	{
		DoSelectRow(cnt, inSelect);
	}
}


void
MListView::InvokeRow(
	int32 /* row */)
{
	//	Do nothing
}


void
MListView::DrawRow(
	int32 /* row */,
	void * data,
	BRect area,
	BRect /*intersection*/)
{
	if (!data)
		return;
	MovePenTo(3, area.bottom - fFontHeight.descent);
	DrawString((const char *)data);
}


float
MListView::GetRowHeight(
	int32 row)
{
	if (row < 0 || row >= CountRows())
	{
		DEBUGGER("GetRowHeight for invalid row!\n");
		return 0.0;
	}
	return GetRowPos(row+1) - GetRowPos(row);
}


float
MListView::GetRowPos(
	int32 row)
{
	if (row < 0)
	{
		DEBUGGER("GetRowPos for negative row!\n");
		return 0.0;
	}
	else if (row == 0)
		return 0.0;
	else if (row <= CountRows())
		return GetRowData(row-1)->fEnd + 1.0;
	else
	{
		DEBUGGER("GetRowPos for too large row!\n");
		return GetMaxYPixel();
	}
}


float
MListView::GetRowEnd(
	int32 row)
{
	if (row < 0)
	{
		DEBUGGER("GetRowPos for negative row!\n");
		return 0.0;
	}
	else if (row < CountRows())
		return GetRowData(row)->fEnd;
	else
		DEBUGGER("GetRowEnd for too large row!\n");
	return GetMaxYPixel();
}


int32
MListView::GetPosRow(
	float pos)
{
	int32 max = CountRows()-1;
	int32 min = 0;
	if (!max)
		return 0;
	while (max > min)
	{
		int32 cur = (max+min)/2;
		if (GetRowEnd(cur) < pos)
			min = cur+1;
		else
			max = cur;
	}
	return min;
}


void
MListView::GetRowRect(
	int32 row,
	BRect * area)
{
	if (row < 0 || row >= CountRows())
	{
		DEBUGGER("GetRowRect with bad row!\n");
		area->left = area->top = 0.0;
		area->bottom = area->right = -1.0;
		return;
	}

	*area = Bounds();

	if (row != 0)
		area->top = GetRowData(row-1)->fEnd + 1.0;
	else
		area->top = 0.0;
	area->bottom = GetRowData(row)->fEnd;
}

// ---------------------------------------------------------------------------
//		HiliteRow
// ---------------------------------------------------------------------------

void
MListView::HiliteRow(
	int32 		inRow,
	BRect 		inArea)
{
	if (inRow < 0 || inRow >= CountRows())
	{
		DEBUGGER("HiliteRow with bad row!\n");
	}
	else
	{
		BRect rowRect;

		GetRowRect(inRow, &rowRect);

		BRect sectRect = rowRect;
	
		if (sectRect.top < inArea.top)
			sectRect.top = inArea.top;
		if (sectRect.bottom > inArea.bottom)
			sectRect.bottom = inArea.bottom;
		if (sectRect.left < inArea.left)
			sectRect.left = inArea.left;
		if (sectRect.right > inArea.right)
			sectRect.right = inArea.right;
		if (sectRect.bottom > sectRect.top &&
				sectRect.right > sectRect.left)
		{
			rowRect.left = 0.0;
			rowRect.right = Bounds().right;

			DrawHilite(rowRect);
			SetDrawingMode(B_OP_OVER);	// for anti-aliasing
			DrawRow(inRow, fList->ItemAt(inRow), rowRect, sectRect);
			SetDrawingMode(B_OP_COPY);
		}
	}
}

// ---------------------------------------------------------------------------
//		UnHiliteRow
// ---------------------------------------------------------------------------

void
MListView::UnHiliteRow(
	int32 		inRow,
	BRect 		/*inArea*/)
{
	if (inRow < 0 || inRow >= CountRows())
	{
		DEBUGGER("HiliteRow with bad row!\n");
	}
	else
	{
		BRect rowRect;

		GetRowRect(inRow, &rowRect);

		rowRect.left = 0.0;
		rowRect.right = Bounds().right;

		FillRect(rowRect, B_SOLID_LOW);
		DrawRow(inRow, fList->ItemAt(inRow), rowRect, rowRect);
	}
}

void
MListView::DrawHilite(
	BRect area)
{
	const rgb_color viewColor = ViewColor();
	SetLowColor(HiliteColor());
	SetDrawingMode(B_OP_COPY);
	FillRect(area, B_SOLID_LOW);
	SetLowColor(viewColor);
}

// Call to notify the listview that the hilite color has changed.
// Redraws all the rows that are selected.

void
MListView::HiliteColorChanged()
{
	BRect		rowRect;
	int32		row = -1;

	while (NextSelected(row))
	{
		GetRowRect(row, &rowRect);
		HiliteRow(row, rowRect);
	}
}


void
MListView::AdjustScrollbar()
{
	BScrollBar *vScroll = ScrollBar(B_VERTICAL);
	BRect		bounds = Bounds();

	if (!fAttached || !vScroll)
		return;
	float size = bounds.bottom - bounds.top + 1.0;
	float toScroll = GetMaxYPixel()-size;
	if (toScroll <= 0.0)
		toScroll = 0.0;
	vScroll->SetRange(0, toScroll);

	vScroll->SetSteps(fFontHeight.ascent+fFontHeight.descent+fFontHeight.leading,size-fRowHeight);
	vScroll->SetValue(bounds.top);
	vScroll->SetProportion(size / GetMaxYPixel());
}


void
MListView::DeleteItem(
	void * data)
{
	free(data);
}


void
MListView::DeleteList()
{
	if (!fOwnsList)
		return;
	if (!fList)
		return;
	while (fList->CountItems())
		DeleteItem(fList->LastItem());
	delete fList;
	fList = NULL;
}


void
MListView::AdjustRowsFrom(
	int32 pos,
	float delta,
	bool doUpdate)
{
	int32 max = CountRows();
	float updMin = GetRowPos(pos);
	while (pos < max)
	{
		RowData * data = GetRowData(pos);
		data->fEnd += delta;
		pos++;
	}
	if (doUpdate && fAttached)
	{
		BRect area = Bounds();
		if (area.bottom >= updMin)
		{
			if (area.top < updMin)
				area.top = updMin;
			Invalidate(area);
		}
		AdjustScrollbar();
	}
}


void
MListView::DoSelectRow(
	int32 row,
	bool toSelect)
{
	ASSERT(row >= 0 && row < CountRows());
	RowData * data = GetRowData(row);
	if (data->fSelected != toSelect)
	{
		data->fSelected = toSelect;
		if (fAttached)
		{
			BRect area;
			GetRowRect(row, &area);
			if (toSelect)
				HiliteRow(row, area);
			else
				UnHiliteRow(row, area);
		}
	}
}


bool
MListView::BoundedScroll(
	float /* scrollX */,
	float scrollY)
{
	float maxY = GetMaxYPixel()-Bounds().Height()-1.0;
	if (maxY < 0.0)
		maxY = 0.0;
	float newPos = scrollY + Bounds().top;
	if (newPos < 0)
	{
		scrollY -= newPos;
	}
	if (newPos > maxY)
	{
		scrollY -= (newPos - maxY);
	}
	if (scrollY != 0.0)
	{
		ScrollBy(0, scrollY);
		return TRUE;
	}
	return FALSE;
}


void
MListView::ScrollRowIntoView(
	int32 	inRow)
{
	BRect		bounds = Bounds();
	BRect		rowRect;

	GetRowRect(inRow, &rowRect);
	
	if (! bounds.Contains(rowRect.LeftTop()) || ! bounds.Contains(rowRect.LeftBottom()))
	{
		if (rowRect.top < bounds.top)
		{
			// Row is above the bounds
			BoundedScroll(0.0, -(bounds.top - rowRect.top));
		}
		else
		if (rowRect.bottom > bounds.bottom)
		{
			// Row is below the bounds
			BoundedScroll(0.0, (rowRect.bottom - bounds.bottom));
		}
		AdjustScrollbar();
	}
}


void
MListView::SetEnabled(
	bool 	inEnabled)
{
	fEnabled = inEnabled;

	if (inEnabled)
		SetFlags(Flags() | B_NAVIGABLE);
	else
		SetFlags(Flags() & ~B_NAVIGABLE);
}

