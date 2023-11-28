//========================================================================
//	MListView.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	JW

#ifndef _MLISTVIEW_H
#define _MLISTVIEW_H


#include <View.h>
#include <Debug.h>
#include <SupportDefs.h>
#include <List.h>

class MListView :
	public BView
{
public:
		enum {
			INVALID_ROW = -1
		};
								MListView(
									BRect area,
									const char * name = NULL,
									uint32 resize = B_FOLLOW_ALL_SIDES,
									uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED | B_NAVIGABLE);
								~MListView();

		void					AttachedToWindow();
		void					Draw(
									BRect area);
		void					MouseDown(
									BPoint where);
		void					FrameResized(
									float newWidth,
									float newHeight);
virtual	void					KeyDown(
									const char *bytes, 
									int32 		numBytes);
		void					Pulse();

		void					InvokeSelection();
		void					SelectPrev(
									bool	inKeepOld = false);
		void					SelectNext(
									bool	inKeepOld = false);
		void					PageUp();
		void					PageDown();

		void            		SetFont(
									const BFont *font, 
									uint32 mask = B_FONT_ALL);
		void					SetFontSize(
									float size);

		void					SyncList();
		BList *					GetList() const
								{
									return fList;
								}

virtual	void					SetList(
									BList * list,
									bool ownsList = TRUE);

		int32					CountRows() const
								{
									return fRowData.CountItems();
								}
		float					GetMaxYPixel() const;	//	return the bottom-most pixel Y

		void					SetDefaultRowHeight(
									float defaultHeight = 0.0);	//	< 1 means auto-calc
virtual	void					InsertRow(
									int32 asNo,
									void * data,
									float height = 0.0);	//	less than one means default
		void					DeleteRows(
									int32 fromNo,
									int32 numRows = 1)
								{
									DoDeleteRows(fromNo, numRows, TRUE);
								}
virtual	void					RemoveRows(
									int32 fromNo,
									int32 numRows = 1)
								{
									DoDeleteRows(fromNo, numRows, FALSE);
								}

		void					SetRowHeight(
									int32 row,
									float height = 0.0);//	less than one means default
		float					GetRowHeight(
									int32 row);
virtual	void					InvalidateRow(
									int32 row);
		void					UpdateFontHeight();

		void					SetMultiSelect(			//	allow more than one selected row?
									bool multi)
								{
									fAllowMulti = multi;
								}
		void					SetDragSelect(			//	allow drag selecting?
									bool drag = true)
								{
									fDragSelection = drag;
								}
		void					SetDragNDropCells(		//	allow drag real drag and drop of cells
									bool drag = true)
								{
									fDragNDropCells = drag;
								}

		font_height				GetCachedFontHeight()
								{
									return fFontHeight;
								}

		int32					ClickCount()
								{
									return fClickCount;
								}

virtual	void					SelectRow(
									int32 row,
									bool keepOld = FALSE,
									bool toSelect = TRUE);
virtual	void					SelectRows(
									int32 rowFrom,
									int32 rowTo,
									bool keepOld = FALSE,
									bool toSelect = TRUE);
virtual	void					SelectAll(
									bool	inSelect);

virtual	void					InvokeRow(
									int32 row);
		/*
		 * Selection iteration - FirstSelected will return the lowest-numbered selected row
		 * and the number of selected rows. NextSelected will step upwards, returning TRUE
		 * for each selected row it finds and returns
		 */
		bool					IsRowSelected(
									int32 row) const;
		int32					FirstSelected(		//	returns row number of first selected row
									int32 & firstRow) const;
		int32					LastSelected() const;

		bool					NextSelected(
									int32 & rowIter) const;
		bool					PreviousSelected(
									int32 & rowIter) const;

		void					ScrollRowIntoView(
									int32 	inRow);


virtual	void					SetEnabled(
									bool	inEnabled = true);
		bool					IsEnabled() const;
virtual void					HiliteColorChanged();

protected:

		font_height				fFontHeight;

virtual	void					DrawRow(
									int32 row,
									void * data,
									BRect rowRect,
									BRect intersectionRect);

		/*
		 * As a special-case, you can call GetRowPos for the N+1th row
		 * in a list of N rows
		 */
		float					GetRowPos(
									int32 row);
		float					GetRowEnd(
									int32 row);
		int32					GetPosRow(
									float yPos);

		void					GetRowRect(
									int32 row,
									BRect * area);

virtual	bool					ClickHook(
									BPoint /* where */,
									int32 /* row */,
									uint32 /* modifiers */,
									uint32 /* buttons */)
								{
									//	return true to inhibit selection
									return false;
								}

		/*
		 * Called from mousedown if fDragNDropCells is true and a drag
		 * is to be started.
		 */
virtual	void					InitiateDrag(
									BPoint /* where */,
									int32 /* row */)
								{
								}
virtual	void					StartAutoScroll();
virtual	bool					IsAutoScrolling()
								{
									return fAutoScrolling;
								}

virtual	void					HiliteRow(
									int32 row,
									BRect rowArea);
virtual	void					UnHiliteRow(
									int32 row,
									BRect rowArea);
virtual	void					DrawHilite(
									BRect area);		//	Called to actually hilite

virtual	void					AdjustScrollbar();

		/*
		 * The default item is deleted by calling free() on it
		 */
virtual	void					DeleteItem(
									void * item);
virtual	void					DoDeleteRows(
									int32 fromNo,
									int32 numRows,
									bool callDelete);

		bool					WaitMouseUp(
									BPoint inPoint);
		float					DiffSquared(float a, float b);

private:

		struct RowData {
			float					fEnd;
			bool					fSelected;
			static bool				Delete(
										void * data);
		};
		float					fRowHeight;
		BList					fRowData;
		BList *					fList;
		int32					fClickCount;
		bigtime_t				fClickTime;
		uint32					fClickButtons;
		bool					fOwnsList;
		bool					fAttached;
		bool					fAllowMulti;
		bool					fDragSelection;
		bool					fDragNDropCells;
		bool					fInClick;		//	Used to special-case drawing
		bool					fAutoScrolling;
		bool					fEnabled;

		void					DeleteList();
		void					AdjustRowsFrom(
									int32 firstRow,
									float heightDelta,
									bool doUpdate);
		void					DoSelectRow(
									int32 row,
									bool toSelect);
		RowData *				GetRowData(
									int32 row) const
								{
									ASSERT(row>=0 && row < CountRows());
									return (RowData *)fRowData.ItemAt(row);
								}
		bool					BoundedScroll(
									float deltaX,
									float deltaY);
};

inline bool MListView::IsEnabled() const
{
	return fEnabled;
}

#endif
