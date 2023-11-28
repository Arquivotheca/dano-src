//******************************************************************************
//
//	File:		ListView.cpp
//
//	Written by:	Peter Potrebic
//
//	Copyright 1996, Be Incorporated
//
//******************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#include <string.h>
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _LIST_VIEW_H
#include "ListView.h"
#endif
#ifndef _FONT_H
#include "Font.h"
#endif
#ifndef _WINDOW_H
#include "Window.h"
#endif
#ifndef _SCROLL_VIEW_H
#include "ScrollView.h"
#endif
#ifndef _SCROLL_BAR_H
#include "ScrollBar.h"
#endif
#ifndef _LIST_H
#include <List.h>
#endif
#ifndef _AUTO_LOCK_H
#include <Autolock.h>
#endif
#ifndef _LOOPER_H
#include <Looper.h>
#endif
#include <PropertyInfo.h>
#include <MessageRunner.h>


#ifndef _ARCHIVE_DEFS_H
#include <archive_defs.h>
#endif
#ifndef _OUTLINE_LIST_VIEW_H
#include <OutlineListView.h>
#endif
#ifndef _FBC_H
#include <fbc.h>
#endif

#define S_FOCUS_INDICATOR_NAME	"_f_ind_"

#define TICK_TIME	50000

class track_data {
public:
	~track_data()
		{ delete runner; }

	bool			changed;
	bool			extend;
	bool			disjoint;
	bool			turn_on;
	bigtime_t		snooze_time;
	BMessageRunner	*runner;
	bigtime_t		last_mm_time;
	BPoint			last_mm_pt;
	bigtime_t		initial_mm_time;
	BPoint			initial_mm_pt;
	int32			clicked_index;
	bool			initiate_drag;
	bool			delayed_select;
	bool			initially_selected;
};


/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

BListView::BListView(BRect frame, const char *name, list_view_type list_type,
	uint32 resizeMask, uint32 flags)
	:	BView(frame, name, resizeMask, flags),
		fList(32)
{
	InitObject(list_type);
}

/* ---------------------------------------------------------------- */

void BListView::InitObject(list_view_type list_type)
{
	SetDoubleBuffering(B_UPDATE_RESIZED|B_UPDATE_INVALIDATED);

	fListType = list_type;
	fFirstSelected = -1;
	fLastSelected = -1;
	fAnchorIndex = -1;
	fSelectMessage = NULL;
	fScrollView = NULL;
	fTrack = NULL;

	BRect r = Bounds();
	fWidth = r.Width();
	r.right = r.left + 1;
	
	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
}

/* ---------------------------------------------------------------- */

BListView::BListView(BMessage *data)
	: BView(data), BInvoker()
{
	long l;
	data->FindInt32(S_LIST_VIEW_TYPE, &l);
	InitObject((list_view_type) l);

	long		i = 0;
	BArchivable	*obj;
	BMessage	archive;
	while(data->FindMessage(S_LIST_ITEMS, i++, &archive) == B_OK) {
		obj = instantiate_object(&archive);
		if (!obj)
			continue;
		BListItem	*item = dynamic_cast<BListItem*>(obj);
		if (item)
			AddItem(item);
	}

	if (data->HasMessage(S_MESSAGE)) {
		BMessage	*msg = new BMessage();
		data->FindMessage(S_MESSAGE, msg);
		SetInvocationMessage(msg);
	}
	if (data->HasMessage(S_ALT_MESSAGE)) {
		BMessage	*msg = new BMessage();
		data->FindMessage(S_ALT_MESSAGE, msg);
		SetSelectionMessage(msg);
	}
}

/* ---------------------------------------------------------------- */

status_t BListView::Archive(BMessage *data, bool deep) const
{
	// WARNING: BOutlineListView skips over this implementation of
	// Archive(), directly calling BView::Archive().
	
	BView::Archive(data, deep);

	// ??? can't archive a reference to the message 'target' 
	
	data->AddInt32(S_LIST_VIEW_TYPE, fListType);

	if (deep) {
		BListItem	*item;
		long		i = 0;
		while ((item = ItemAt(i++)) != 0) {
			BMessage	archive;
			long		err;
			err = item->Archive(&archive, true);
			if (!err)
				data->AddMessage(S_LIST_ITEMS, &archive);
		}
	}

	BMessage *msg = Message();
	if (msg) {
		data->AddMessage(S_MESSAGE, msg);
	}
	if (fSelectMessage) {
		data->AddMessage(S_ALT_MESSAGE, fSelectMessage);
	}
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BListView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BListView"))
		return NULL;
	return new BListView(data);
}

/* ---------------------------------------------------------------- */

BListView::~BListView()
{
	SetSelectionMessage(NULL);
	if (fSelectMessage) {
		delete fSelectMessage;
		fSelectMessage = NULL;
	}
}

/* ---------------------------------------------------------------- */

void BListView::SetListType(list_view_type list_type)
{
	if ((list_type == B_SINGLE_SELECTION_LIST) &&
		(fListType == B_MULTIPLE_SELECTION_LIST))
			DeselectAll();
	fListType = list_type;
}

/* ---------------------------------------------------------------- */

list_view_type BListView::ListType() const
{
	return fListType;
}

/* ---------------------------------------------------------------- */

void BListView::MakeFocus(bool state)
{
	if (state == IsFocus())
		return;

	BView::MakeFocus(state);

	if (fScrollView)
		fScrollView->SetBorderHighlighted(state);
}

/* ---------------------------------------------------------------- */

inline int32 BListView::RangeCheck(int32 index)
{
	// If we're dealing with a bogus index then set it to -1.

	if ((index < 0) || (index >= CountItems()))
		index = -1;
	return index;
}

/* ---------------------------------------------------------------- */

int32 BListView::IndexOf(BPoint point) const
{
	BListItem	*cell;
	long		i = 0;
	float		cur_y = 0;

	if (point.y < 0)
		return -1;

	while ((cell = ItemAt(i++)) != 0) {
		cur_y += ceil(cell->Height());
		if (cur_y >= point.y)
			return (i-1);
		cur_y++;
	}

	return -1;
}

/* ---------------------------------------------------------------- */

void BListView::DrawItem(BListItem *item, BRect itemRect, bool complete)
{
	item->DrawItem(this, itemRect, complete);
}

/* ---------------------------------------------------------------- */

void BListView::Draw(BRect updateRect)
{
	BRect	bounds;
	BRect	cellRect;
	long	i;

	bounds = Bounds();

	if (fWidth != bounds.right) {
		fWidth = bounds.right;
		FixupScrollBar();
	}

//+	PRINT_OBJECT(updateRect);
//+	PRINT(("    ")); PRINT_OBJECT(bounds);
	i = IndexOf(updateRect.LeftTop());		// start drawing here
//+	PRINT(("i = %d\n", i));

	cellRect = ItemFrame(i);				// a little tricky here to get
	cellRect.bottom = cellRect.top - 1;		// start in the loop.
//+	PRINT_OBJECT(cellRect);

	while (cellRect.top <= updateRect.bottom) {
		BListItem	*cell = ItemAt(i);
		if (!cell)
			break;
		float		height = ceil(cell->Height());

		cellRect.top = cellRect.bottom + 1;
		cellRect.bottom = cellRect.top + height;
		DrawItem(cell, cellRect);
		i++;
	}
}

/* ---------------------------------------------------------------- */

const float kDragTreshold = 2;
const bigtime_t kDragTimeTreshold = 2000000;

void BListView::MouseDown(BPoint where)
{
	if (!IsFocus()) {
		MakeFocus(true);

		// do this to allow the 'focus' indicator to draw
		Sync();
		Window()->UpdateIfNeeded();
	}

	BMessage	*msg = Window()->CurrentMessage();
	ulong		mods;
	msg->FindInt32("modifiers", (long*) &mods);

	bool		extend = (fListType == B_MULTIPLE_SELECTION_LIST) && 
							(mods & B_OPTION_KEY) != 0;
	bool		disjoint = (fListType == B_MULTIPLE_SELECTION_LIST) && 
							(mods & B_SHIFT_KEY) != 0;
	bool		turn_on = true;
	long		clicks;
	long		i = IndexOf(where);
	BListItem	*cell = ItemAt(i);
	long		lasti = CountItems() - 1;

	msg->FindInt32("clicks", &clicks);

	// Wrap after second click.  This is so that multiple double-clicks
	// connected together will be reported as individual invokations.
	clicks = (clicks-1)%2 + 1;
	
	PRINT(("\n\nMouseDown: item = %d, ", i)); PRINT_OBJECT(where);
//+	for (long t = 0; t <= lasti; t++) {
//+		PRINT(("frame(%d): ", t));
//+		PRINT_OBJECT(ItemFrame(t));
//+	}

	// It's only a dbl-click if fast enough and clicking on the same item.
	if (clicks == 2) {
		if (i == fAnchorIndex) {
			PRINT(("dbl-click\n"));
			Invoke();
			return;
		}
	}

	if (extend) {
		long ii = (i != -1) ? i : lasti;
		if (ii < fFirstSelected)
			fAnchorIndex = fLastSelected;
		else 
			fAnchorIndex = fFirstSelected;
		if (fAnchorIndex == -1)
			fAnchorIndex = i;
	} else {
		fAnchorIndex = (i != -1) ? i : lasti;
		if (disjoint && (i != -1)) {
			ASSERT(cell);
			turn_on = !cell->IsSelected();
		}
	}

	BPoint originalPoint;
	if (msg->FindPoint("be:view_where", &originalPoint) != B_OK)
		originalPoint = where;


	fTrack = new track_data;
	fTrack->changed = false;
	fTrack->snooze_time = 20000;
	fTrack->extend = extend;
	fTrack->disjoint = disjoint;
	fTrack->turn_on = turn_on;
	fTrack->runner = NULL;
	fTrack->last_mm_time = 0;
	fTrack->last_mm_pt = B_ORIGIN;
	fTrack->initial_mm_time = system_time();
	fTrack->initial_mm_pt = originalPoint;
	fTrack->clicked_index = i;
	fTrack->initiate_drag = !extend && ItemAt(i) != 0;
	fTrack->delayed_select = false;
	fTrack->initially_selected = cell ? cell->IsSelected() : false;
	
	if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
		BMessage	msg('TICK');
		fTrack->runner = new BMessageRunner(BMessenger(this), &msg, TICK_TIME);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
		DoMouseMoved(where);
		return;
	}

	ulong buttons;
	
	do {
		if( TryInitiateDrag(where) ) return;
		DoMouseMoved(where);
		snooze(fTrack->snooze_time);
		GetMouse(&where, &buttons, true);
	} while (buttons);

	DoMouseUp(where);
}

/* ---------------------------------------------------------------- */

void BListView::PerformDelayedSelect()
{
	ASSERT(fTrack);

	if (fTrack->delayed_select) {
		// clicked on a selected item, but didn't do enough to drag...
		// perform a normal selection operation.
		// we know this wasn't a disjoint select, as that overrides
		// any initial drag state, so just select the current item.
		if( fTrack->turn_on ) {
			if (_Select(fTrack->clicked_index, fTrack->clicked_index, false))
				fTrack->changed = true;
		} else {
			if (_Deselect(fTrack->clicked_index))
				fTrack->changed = true;
		}
		fTrack->delayed_select = false;
	}
}

/* ---------------------------------------------------------------- */

bool BListView::TryInitiateDrag(BPoint where)
{
	ASSERT(fTrack);

	if (fTrack->initiate_drag) {
		BPoint delta(where);
		delta -= fTrack->initial_mm_pt;
		if (delta.x < -kDragTreshold
			|| delta.x > kDragTreshold
			|| delta.y < -kDragTreshold
			|| delta.y > kDragTreshold
			|| (system_time() - fTrack->initial_mm_time) > kDragTimeTreshold) {
			fTrack->initiate_drag = InitiateDrag(where, fTrack->clicked_index,
				fTrack->initially_selected);
			
			if (fTrack->initiate_drag) {
				delete fTrack;
				fTrack = NULL;
				return true;
			} else {
				PerformDelayedSelect();
			}
		}
	}
	
	return false;
}

/* ---------------------------------------------------------------- */

void BListView::DoMouseMoved(BPoint where)
{
#define AUTO_SCROLL_SLOP	25
	ASSERT(fTrack);
	bool		scrolled = false;
	BRect		bounds = Bounds();
	long		i;
	long		lasti;

	fTrack->last_mm_time = system_time();
	fTrack->last_mm_pt = ConvertToScreen(where);

	if ((where.x < (bounds.left - 15)) || (where.x > (bounds.right + 15)))
		goto done;
//+	PRINT(("DoMouseMoved(): "));
//+	PRINT_OBJECT(where);
//+	PRINT_OBJECT(bounds);

	i = IndexOf(where);
	lasti = CountItems() - 1;

	// auto-scrolling
	if ((bounds.top > 0) && (where.y < bounds.top) &&
		(where.y > bounds.top - AUTO_SCROLL_SLOP)) {
		BPoint pt = bounds.LeftTop();
		pt.y--;
		// scroll-up
		i = IndexOf(pt);
		BRect f = ItemFrame(i);
		PRINT(("Scroll up to item %d (%.1f)\n", i, f.top - bounds.top));
		ScrollBy(0, f.top - bounds.top);
		scrolled = true;
	} else if ((where.y > bounds.bottom) &&
		(where.y < bounds.bottom + AUTO_SCROLL_SLOP)) {
		BPoint pt = bounds.LeftBottom();
		pt.y++;
		// scroll-down
		i = IndexOf(pt);
		PRINT(("Scroll down to item %d\n", i));
		// if i==-1 then we're past the end of the list so don't scroll
		if (i != -1) {
			BRect f = ItemFrame(i);
			ScrollBy(0, f.bottom - bounds.bottom);
			scrolled = true;
		}
	}

	if (!scrolled) {
		// pin the point to inside the bounds
		if (where.y < bounds.top)
			where.y = bounds.top;
		else if (where.y > bounds.bottom)
			where.y = bounds.bottom;
	}

	i = IndexOf(where);
//+	PRINT(("i=%d, ", i)); PRINT_OBJECT(where);

	if (fTrack->extend) {
		if (i == -1)
			i = lasti;
		if (_Select(fAnchorIndex, i, false))
			fTrack->changed = true;
	} else if (fTrack->disjoint) {
		if (fTrack->turn_on) {
			if (_Select(i, true))
				fTrack->changed = true;
		} else {
			if( fTrack->initiate_drag ) {
				BListItem *item = ItemAt(i);
				fTrack->delayed_select =
					(fTrack->initiate_drag && item && item->IsSelected());
			} else {
				fTrack->delayed_select = false;
			}
			if( !fTrack->delayed_select ) {
				if (_Deselect(i))
					fTrack->changed = true;
			}
		}
		fAnchorIndex = i;
	} else {
		if (i == -1) {
			if (_DeselectAll(-1, -1))
				fTrack->changed = true;
		} else {
			if( fTrack->initiate_drag ) {
				BListItem *item = ItemAt(i);
				fTrack->delayed_select =
					(fTrack->initiate_drag && item && item->IsSelected());
			} else {
				fTrack->delayed_select = false;
			}

			// select item.  don't select others if this could be a drag.
			if (_Select(i, fTrack->delayed_select)) 
				fTrack->changed = true;
		}
		fAnchorIndex = i;
	}

done:
	if (scrolled) {
		Sync();
		Window()->UpdateIfNeeded();
//+		bounds = Bounds();
		fTrack->snooze_time = 100000;
	}
}

/* ---------------------------------------------------------------- */

void BListView::DoMouseUp(BPoint )
{
	ASSERT(fTrack);

	PRINT(("DoMouseUp()\n"));
	
	PerformDelayedSelect();
	
	if (fTrack->changed) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}

	delete fTrack;
	fTrack = NULL;
}

/* ---------------------------------------------------------------- */

void BListView::FrameResized(float, float)
{
	BRect	bounds = Bounds();
	if (fWidth != bounds.right) {
		fWidth = bounds.right;
	}
	FixupScrollBar();
}

/* ---------------------------------------------------------------- */

void BListView::ScrollTo(BPoint where)
{
	BView::ScrollTo(where);
	fWidth = Bounds().right;
}

/* ---------------------------------------------------------------- */

void BListView::KeyDown(const char *bytes, int32 numBytes)
{
	long		index;
	long		curSelected;
	BRect		frame;
	BRect		rect;
	BPoint		pt;
	uchar		aKey = bytes[0];
	BListItem	*item;
	BMessage	*cur = Window()->CurrentMessage();
	uint32		mods;
	bool		extend;

	cur->FindInt32("modifiers", (int32 *) &mods);
	extend = ((mods & (B_OPTION_KEY|B_SHIFT_KEY)) != 0) &&
		(fListType == B_MULTIPLE_SELECTION_LIST);

	switch(aKey) {
		case B_UP_ARROW:
			curSelected = fFirstSelected;
			index = (curSelected == -1) ? 1 : curSelected;
			while (--index >= 0) {
				if(!(item = ItemAt(index)))
					break;
				if (item->IsEnabled()) {
					Select(index, extend);
					ScrollToSelection();
					break;
				}
			}
			break;
		case B_DOWN_ARROW:
			curSelected = fLastSelected;
			index = curSelected;
			while (++index >= 0) {
				if(!(item = ItemAt(index)))
					break;
				if (item->IsEnabled()) {
					Select(index, extend);
					ScrollToSelection();
					break;
				}
			}
			break;
		case B_PAGE_UP:
		case B_PAGE_DOWN: {
			rect = Bounds();
			pt = rect.LeftTop();
			if (aKey == B_PAGE_DOWN)
				pt.y = pt.y + rect.Height();
			else
				pt.y = pt.y - rect.Height();
			if (pt.y < 0.0)
				pt.y = 0.0;
			else if (pt.y > rect.bottom)
				pt.y = rect.bottom;
			index = IndexOf(pt);
			frame = ItemFrame(index);
			ScrollTo(frame.LeftTop());
			break;
		}
		case B_HOME:
			if (extend)
				Select(0, fLastSelected, true);
			else
				Select(0);
			ScrollToSelection();
			break;
		case B_END:
			if (extend)
				Select(fFirstSelected, CountItems() - 1, true);
			else
				Select(CountItems() - 1);
			ScrollToSelection();
			break;

		case B_RETURN:
		case B_SPACE:
			curSelected = fFirstSelected;
			if (curSelected >= 0)
				Invoke();
			break;
		default:
			BView::KeyDown(bytes, numBytes);
			break;
	}
}

/* ---------------------------------------------------------------- */

bool BListView::AddItem(BListItem *item)
{
	if(!fList.AddItem(item))
		return false;
	
	if (Window()) {
		BFont font;
		GetFont(&font);
		item->Update(this, &font);
		FixupScrollBar();
		InvalidateItem(CountItems() - 1);
	}
	return true;
}

/* ---------------------------------------------------------------- */

bool BListView::AddItem(BListItem *item, int32 atIndex)
{
	if(!fList.AddItem(item, atIndex))
		return false;

	if ((fFirstSelected != -1) && (atIndex <= fFirstSelected))
		fFirstSelected++;

	if ((fLastSelected != -1) && (atIndex <= fLastSelected))
		fLastSelected++;

	if (Window()) {
		BFont font;
		GetFont(&font);
		item->Update(this, &font);
		FixupScrollBar();
		InvalidateFrom(atIndex);
	}
	return true;
}

/* ---------------------------------------------------------------- */

bool BListView::RemoveItem(BListItem *item)
{
	return RemoveItem(IndexOf(item)) != NULL;
}

/* ---------------------------------------------------------------- */

bool BListView::RemoveItems(int32 index, int32 count)
{
	index = RangeCheck(index);
	if (index < 0)
		return false;

	while(count--)
		RemoveItem(index);

	return true;
}

/* ---------------------------------------------------------------- */

BListItem *BListView::RemoveItem(int32 index)
{
	BListItem	*item;

	item = ItemAt(index);
	if (!item)
		return NULL;

	if (item->IsSelected())
		Deselect(index);
	if (item && fList.RemoveItem(index)) {

		if ((fFirstSelected != -1) && (index < fFirstSelected))
			fFirstSelected--;

		if ((fLastSelected != -1) && (index < fLastSelected))
			fLastSelected--;

		InvalidateFrom(index);
		FixupScrollBar();
	}

	return item;
}

/* ---------------------------------------------------------------- */
bool BListView::SwapItems(int32 a, int32 b)
{
	MiscData data;
	data.swap.a = a;
	data.swap.b = b;
	return DoMiscellaneous(B_SWAP_OP, &data);
}

bool BListView::DoSwapItems(int32 indexA, int32 indexB)
{
	/*	Trust BList to do range checking	*/
	if (!fList.SwapItems(indexA, indexB))
		return false;
	/*	Invalidate only the items we flipped	*/
	Invalidate(ItemFrame(indexA));
	Invalidate(ItemFrame(indexB));
	/*	Selection semantics are tricky	*/
	if (fAnchorIndex == indexA)
		fAnchorIndex = indexB;
	else if (fAnchorIndex == indexB)
		fAnchorIndex = indexA;
	/*	Update notion of our selected range	*/
	RescanSelection(indexA, indexB);

	return true;
}


/* ---------------------------------------------------------------- */
bool BListView::MoveItem(int32 fromIndex, int32 toIndex)
{
	MiscData data;
	data.move.from = fromIndex;
	data.move.to = toIndex;
	return DoMiscellaneous(B_MOVE_OP, &data);
}

bool BListView::DoMoveItem(int32 fromIndex, int32 toIndex)
{
	/*	Remember range we're changing	*/
	BRect from = ItemFrame(fromIndex);
	BRect to = ItemFrame(toIndex);

	/*	Trust BList to do range checking	*/
	if (!fList.MoveItem(fromIndex, toIndex))
		return false;

	/*	Update selection state	*/
	RescanSelection(fromIndex, toIndex);

	BRect inval = from | to;
	if (Bounds().Intersects(inval))
	{
		inval = inval & Bounds();	/*	don't draw more than we display	*/
#if 1
		Invalidate(inval);
#else
		/* this code doesn't work right, although it looks OK */
		BRect inval2 = inval;

		if (fromIndex < toIndex)
		{
			/*	we slide items up towards 0	*/
			inval2.bottom -= (from.IntegerHeight()+1);
			inval.top += (from.IntegerHeight()+1);
			CopyBits(inval, inval2);
			inval.top = inval2.bottom+1;
			Invalidate(inval);
		}
		else
		{
			/*	we slide items down from 0	*/
			inval2.bottom -= (to.IntegerHeight()+1);
			inval.top += (to.IntegerHeight()+1);
			CopyBits(inval2, inval);
			inval2.bottom = inval.top-1;
			Invalidate(inval);
		}
#endif
	}
	return true;
}


void BListView::RescanSelection(int32 a, int32 b)
{
	if (a > b) { int tmp = a; a = b; b = tmp; }	/* make sure a is smallest */
	if (fAnchorIndex != -1)
	{
		if (fAnchorIndex == a)
			fAnchorIndex = b;
		else if (fAnchorIndex == b)
			fAnchorIndex = a;
	}
	if (fFirstSelected < a && fLastSelected < a)
		return; /* nothing that affects us happened */
	if (fFirstSelected > b && fLastSelected > b)
		return; /* nothing that affects us happened */
	if (fFirstSelected >= a)
	{
		int pos;
		for (pos = a; pos <= b; pos++)
			if (((BListItem *)fList.ItemAt(pos))->IsSelected())
				break;
		fFirstSelected = pos;
	}
	if (fLastSelected <= b)
	{
		int pos;
		for (pos = a; pos <= b; pos++)
			if (((BListItem *)fList.ItemAt(pos))->IsSelected())
				fLastSelected = pos;
	}
}


/* ---------------------------------------------------------------- */

bool BListView::ReplaceItem(int32 index, BListItem *item)
{
	MiscData data;
	data.replace.index = index;
	data.replace.item = item;
	return DoMiscellaneous(B_REPLACE_OP, &data);
}

bool BListView::DoReplaceItem(int32 index, BListItem*item)
{
	BRect inval = ItemFrame(index);
	if (!fList.ReplaceItem(index, item))
	{
		return false;
	}
	if (ItemFrame(index) != inval)
	{ /* changed size? */
		InvalidateFrom(index);
	}
	else 
	{
		Invalidate(inval);
	}
	return true;
}


/* ---------------------------------------------------------------- */
/* This virtual function is new with version PR2.1. */
/* It used to be known as ReservedListView1 */
bool BListView::DoMiscellaneous(MiscCode code, MiscData * data)
{
	switch (code)
	{
	default:
	case B_NO_OP:
		break;
	case B_REPLACE_OP:
		return DoReplaceItem(data->replace.index, data->replace.item);
	case B_MOVE_OP:
		return DoMoveItem(data->move.from, data->move.to);
	case B_SWAP_OP:
		return DoSwapItems(data->swap.a, data->swap.b);
	}
	return false;
}


/* patch up old vtable references to jump to the right place */
/* here be magic and dragons; only wizards dare to enter! */

/* Only the PowerPC version needs backwards compatibility to PR2 */
#if _PR2_COMPATIBLE_

	#if (!defined(__powerc) || !defined(powerc)) || !defined(__MWERKS__)

		#error name mangling and calling conventions may differ

	#else
	
/* we need to know the type of the object, because BOutlineListView */
/* overrides DoMiscellaneous */
extern "C" bool DoMiscellaneous__9BListViewFQ29BListView8MiscCodePQ29BListView8MiscData(BListView *object, int32 code, void *ref);
extern "C" bool DoMiscellaneous__16BOutlineListViewFQ29BListView8MiscCodePQ29BListView8MiscData(BListView *object, int32 code, void *ref);
#pragma export on
extern "C" bool _ReservedListView1__9BListViewFv(BListView *object, int32 code, void *ref);
#pragma export reset

/* We know that "object" is using an old version of the vtable, and thus */
/* is a user-derived version of BListView or of BOutlineListView that a */
/* newer version of the API is calling the new method in. We direct the */
/* call to the appropriate virtual function depending on the class of the */
/* object's libbe.so base class. */
bool _ReservedListView1__9BListViewFv(BListView *object, int32 code, void *ref)
{
	bool (*func)(BListView *, int32, void *) = NULL;

	/* Find topmost libbe.so class that user class is deriving from */
	if (dynamic_cast<BOutlineListView *>(object) == NULL)
		func = DoMiscellaneous__9BListViewFQ29BListView8MiscCodePQ29BListView8MiscData;
	else
		func = DoMiscellaneous__16BOutlineListViewFQ29BListView8MiscCodePQ29BListView8MiscData;

	/* Patch vtable to replace reference to this function with reference */
	/* to intended function */
	/* vtable is first member of BArchivable; I checked */
	_patch_vtable_(((void**)object)[0], 
		(void *)_ReservedListView1__9BListViewFv, (void *)func);

	/* Call through to intended function */
	return (*func)(object, code, ref);
}

	#endif /* PPC Metrowerks */

#endif /* _PR2_COMPATIBLE_ */

/* ---------------------------------------------------------------- */

BListItem *BListView::ItemAt(int32 index) const
{
	return (BListItem *) fList.ItemAt(index);
}

/* ---------------------------------------------------------------- */

int32 BListView::IndexOf(BListItem *item) const
{
	return fList.IndexOf(item);
}

/* ---------------------------------------------------------------- */

BListItem *BListView::FirstItem() const
{
	return (BListItem *) fList.FirstItem();
}

/* ---------------------------------------------------------------- */

BListItem *BListView::LastItem() const
{
	return (BListItem *) fList.LastItem();
}

/* ---------------------------------------------------------------- */

bool BListView::HasItem(BListItem *item) const
{
	return fList.HasItem(item);
}

/* ---------------------------------------------------------------- */

int32 BListView::CountItems() const
{
	return fList.CountItems();
}

/* ---------------------------------------------------------------- */

void BListView::MakeEmpty()
{
	_DeselectAll(-1, -1);
	fList.MakeEmpty();
	ScrollTo(0, 0);
	Invalidate();
}

/* ---------------------------------------------------------------- */

bool BListView::IsEmpty() const
{
	return fList.IsEmpty();
}


/* ---------------------------------------------------------------- */

void BListView::DoForEach(bool (*func)(BListItem*))
{
	int32 count = fList.CountItems();
	for (long i = 0; i < count; i++)
		// if func returns true then abort the iteration
		if ((*func)((BListItem *) fList.ItemAt(i)))
			break;
}

/* ---------------------------------------------------------------- */

void BListView::DoForEach(bool (*func)(BListItem*, void*), void* dataPtr)
{
	int32 count = fList.CountItems();
	for (long i = 0; i < count; i++)
		// if func returns true then abort the iteration
		if ((*func)((BListItem *) fList.ItemAt(i), dataPtr))
			break;
}

/* ---------------------------------------------------------------- */

void BListView::AttachedToWindow()
{
	BView::AttachedToWindow();

	FontChanged();

//+	PRINT(("scroll_bar = %x\n", ScrollBar(B_VERTICAL)));

	// if target/looper wasn't set then default to window
	if (!Messenger().IsValid())
		SetTarget(Window());
	FixupScrollBar();
}

/* ---------------------------------------------------------------- */

void BListView::FontChanged()
{
	BFont font;
	GetFont(&font);

	long		i = 0;
	BListItem	*cell;
	while ((cell = ItemAt(i++)) != 0) {
		cell->Update(this, &font);
	}
}

/* ---------------------------------------------------------------- */

void BListView::SetSelectionMessage(BMessage *msg)
{
	if (fSelectMessage == msg)
		return;

	if (fSelectMessage)
		delete fSelectMessage;
	fSelectMessage = msg;
}

/* ---------------------------------------------------------------- */

void BListView::SetInvocationMessage(BMessage *msg)
{
	SetMessage(msg);
}

/* ---------------------------------------------------------------- */

uint32 BListView::SelectionCommand() const
{
	if (fSelectMessage)
		return fSelectMessage->what;
	else
		return 0;
}

/* ---------------------------------------------------------------- */

BMessage *BListView::SelectionMessage() const
{
	return fSelectMessage;
}

/* ---------------------------------------------------------------- */

uint32 BListView::InvocationCommand() const
{
	return Command();
}

/* ---------------------------------------------------------------- */

BMessage *BListView::InvocationMessage() const
{
	return Message();
}

/* ---------------------------------------------------------------- */

BRect BListView::ItemFrame(int32 index)
{
	// determine the drawing 'frame' of the given cell.

	BListItem	*cell;
	long		i = 0;
	BRect		frame(0,0, fWidth, -1);

	index = RangeCheck(index);
	if (index == -1)
		return BRect(-1, -1, -2, -2);		// Invalid rect

	for (i = 0; i <= index; i++) {
		cell = ItemAt(i);
		frame.top = frame.bottom + 1;
		frame.bottom = frame.top + ceil(cell->Height());
	}

	return frame;
}

/* ---------------------------------------------------------------- */

void BListView::InvalidateItem(int32 index)
{
	BRect bounds;
	BRect itemBounds;

	bounds = Bounds();
	itemBounds = ItemFrame(index);
	itemBounds = (itemBounds & bounds);
	if(!itemBounds.Height() || !itemBounds.Width())
		return;
	Invalidate(itemBounds);
}

/* ---------------------------------------------------------------- */

void BListView::SelectionChanged()
{
}

/* ---------------------------------------------------------------- */

status_t BListView::Invoke(BMessage *msg)
{
	bool notify = false;
	uint32 kind = InvokeKind(&notify);
	
	BMessage clone(kind);
	status_t err = B_BAD_VALUE;
	
	if (!msg && !notify)
		msg = Message();
	if (!msg) {
		// If not being watched, there is nothing to do.
		if( !IsWatched() ) return err;
	} else {
		clone = *msg;
	}

	clone.SetWhen(system_time());
	clone.AddPointer("source", this);
	clone.AddMessenger(B_NOTIFICATION_SENDER, BMessenger(this));

	if (fListType == B_SINGLE_SELECTION_LIST) {
		clone.AddInt32("index", fFirstSelected);
	} else {
		// go through and add ALL selected items to the message
//+		long i = fFirstSelected;
		long i = 0;
		while (BListItem *cell = ItemAt(i)) {
			if (cell->IsSelected())
				clone.AddInt32("index", i);
			i++;
		}
	}
	if( msg ) err = BInvoker::Invoke(&clone);
	
	// Also send invocation to any observers of this handler.
	SendNotices(kind, &clone);
	
	return err;
}

/* ---------------------------------------------------------------- */

void BListView::Deselect(int32 index)
{
	if (_Deselect(index)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}

/* ---------------------------------------------------------------- */

void BListView::Deselect(int32 from, int32 to)
{
	from = RangeCheck(from);
	to = RangeCheck(to);

	if (from > to) {
		int32 tmp = from;
		from = to;
		to = tmp;
	}

	if ((from == -1) || (to == -1))
		return;

	bool	changed = false;
	int32	i;

	for (i = from; i <= to; i++) {
		if (_Deselect(i)) {
			changed = true;
		}
	}
	if (changed) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}

/* ---------------------------------------------------------------- */

bool BListView::_Deselect(int32 index)
{
//+	PRINT(("Deselecting %d\n", index));
	index = RangeCheck(index);
	if (index == -1)
		return false;

	BAutolock	auto_lock(Window());	
	if (Window() && !auto_lock.IsLocked())
		return false;

	BListItem *cell = ItemAt(index);
	if (cell->IsSelected()) {
		BRect	bounds = Bounds();
		BRect	frame = ItemFrame(index);
		cell->Deselect();

		if ((index == fFirstSelected) && (index == fLastSelected))
			fFirstSelected = fLastSelected = -1;
		else if (index == fFirstSelected)
			fFirstSelected = CalcFirstSelected(index + 1);
		else if (index == fLastSelected)
			fLastSelected = CalcLastSelected(index - 1);

		if (Window() && frame.Intersects(bounds)) {
			PushState();
			DrawItem(cell, frame, true);
			PopState();
		}

		return true;
	}
	return false;
}

/* ---------------------------------------------------------------- */

int32 BListView::CalcFirstSelected(int32 after)
{
	BListItem	*cell;
	long		c = CountItems();

	for (long i = after; i < c; i++) {
		cell = ItemAt(i);
		if (cell->IsSelected())
			return i;
	}
	return -1;
}

/* ---------------------------------------------------------------- */

int32 BListView::CalcLastSelected(int32 before)
{
	BListItem	*cell;

	for (long i = before; i >= 0; i--) {
		cell = ItemAt(i);
		if (cell->IsSelected())
			return i;
	}
	return -1;
}

/* ---------------------------------------------------------------- */

void BListView::DeselectAll()
{
	if (_DeselectAll(-1, -1)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}

/* ---------------------------------------------------------------- */

void BListView::DeselectExcept(int32 except_from, int32 except_to)
{
	// deselect all item except those in the given range (these
	// items need not be selected).

	except_from = RangeCheck(except_from);
	except_to = RangeCheck(except_to);

	if (except_from > except_to) {
		long tmp = except_from;
		except_from = except_to;
		except_to = tmp;
	}

	if ((except_from == -1) || (except_to == -1))
		return;

	if (_DeselectAll(except_from, except_to)) {
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}


/* ---------------------------------------------------------------- */

bool BListView::_DeselectAll(int32 except_from, int32 except_to)
{
	if (fFirstSelected == -1)
		return false;

	BAutolock	auto_lock(Window());	
	if (Window() && !auto_lock.IsLocked())
		return false;

	BRect	bounds = Bounds();
	BRect	frame = ItemFrame(fFirstSelected);
	bool	result = false;

	if (Window()) PushState();
	
	for (long i = fFirstSelected; i <= fLastSelected; i++) {
		BListItem *cell = ItemAt(i);

		frame.bottom = frame.top + ceil(cell->Height());
		if (cell->IsSelected() && !((except_from <= i) && (i <= except_to))) {
//+			PRINT(("deselecting i=%d (all)\n", i));
			cell->Deselect();
			result = true;

			if (Window() && frame.Intersects(bounds))
				DrawItem(cell, frame, true);
		}

		frame.top = frame.bottom + 1;
	}

	if (Window()) PopState();
	
	fFirstSelected = CalcFirstSelected(fFirstSelected);
	fLastSelected = CalcLastSelected(fLastSelected);

	return result;
}

/* ---------------------------------------------------------------- */

void BListView::Select(int32 from, int32 to, bool extend)
{
	if (_Select(from, to, extend)) {
		fAnchorIndex = (from < to) ? from : to;
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}

/* ---------------------------------------------------------------- */

bool BListView::_Select(int32 from, int32 to, bool extend)
{
	BRect	bounds;
	BRect	frame;
	bool	result = false;
	
	BAutolock	auto_lock(Window());	
	if (Window() && !auto_lock.IsLocked())
		return false;

	from = RangeCheck(from);
	to = RangeCheck(to);

	if (from > to) {
		long tmp = from;
		from = to;
		to = tmp;
	}

//+	PRINT(("selecting from=%d, to=%d\n", from, to));
	if ((from == -1) || (to == -1))
		return false;

	if (!extend) {
		if (_DeselectAll(from, to))
			result = true;
//+		fFirstSelected = from;
//+		fLastSelected = to;
	}

#if 0
	if (extend) {
		if (fFirstSelected == -1) {
			ASSERT(fLastSelected == -1);
			fFirstSelected = from;
			fLastSelected = to;
		} else {
			if (from < fFirstSelected)
				fFirstSelected = from;
			if (to > fLastSelected)
				fLastSelected = to;
		}
	}
#endif

	if (Window()) PushState();
	
	bounds = Bounds();
	for (long i = from; i <= to; i++) {
		BListItem	*cell = ItemAt(i);
		ASSERT(cell);
		ASSERT(i >= 0);
		if (cell->IsEnabled() == false)
			continue;
		frame = ItemFrame(i);
		// if cell is already selected then don't bother
		if (!cell->IsSelected()) {
			cell->Select();
			result = true;
			if (Window() && frame.Intersects(bounds))
				DrawItem(cell, frame);
		}
	}

	if (Window()) PopState();
	
	if (result) {
		fFirstSelected = CalcFirstSelected(0);
		fLastSelected = CalcLastSelected(CountItems() - 1);
	}

	return result;
}

/* ---------------------------------------------------------------- */

void BListView::Select(int32 index, bool extend)
{
	if (_Select(index, extend)) {
		fAnchorIndex = index;
		SelectionChanged();
		InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
	}
}

/* ---------------------------------------------------------------- */

bool BListView::_Select(int32 index, bool extend)
{
	BRect	frame;
	bool	result = false;
	
	BAutolock	auto_lock(Window());	
	if (Window() && !auto_lock.IsLocked())
		return false;

	index = RangeCheck(index);

//+	PRINT(("selecting %d\n", index));
	if (index == -1)
		return false;

	BListItem	*cell = ItemAt(index);
	ASSERT(cell);
	ASSERT(index >= 0);

	if (!extend) {
		if (_DeselectAll(index, index))
			result = true;
		fFirstSelected = fLastSelected = index;
	}

	if (cell->IsEnabled() == false) {
		fFirstSelected = fLastSelected = -1;
		return result;
	}

	if (extend) {
		if (fFirstSelected == -1) {
			ASSERT(fLastSelected == -1);
			fFirstSelected = fLastSelected = index;
		} else {
			if (index < fFirstSelected)
				fFirstSelected = index;
			if (index > fLastSelected)
				fLastSelected = index;
		}
	}

	if (!cell->IsSelected()) {
		frame = ItemFrame(index);
		cell->Select();
		if (Window() && frame.Intersects(Bounds())) {
			PushState();
			DrawItem(cell, frame);
			PopState();
		}
		result = true;
	}

	return result;
}

/* ---------------------------------------------------------------- */

void BListView::ScrollToSelection()
{
	if (fFirstSelected == -1)
		return;

	BRect	frame = ItemFrame(fFirstSelected);
	BRect	bounds = Bounds();
	bool	change = false;
	float 	newY = 0;

	if (frame.bottom > bounds.bottom) {
		newY = frame.bottom - bounds.Height();
		change = true;
	} else if (frame.top < bounds.top) {
		newY = frame.top;
		change = true;
	}
	if (change)
		ScrollTo(bounds.left, newY);
}

/* ---------------------------------------------------------------- */

void BListView::FixupScrollBar(void)
{
	float		max;
	BRect		bounds;
	BRect		frame;
	float		prop;
	BScrollBar	*sb;
	long		count;

	if (!(sb = ScrollBar(B_VERTICAL)))
		return;

	count = CountItems();

	bounds = Bounds();
	if (count) {
		frame = ItemFrame(count - 1);
		max = frame.bottom;
		if(bounds.Height() >= max) {
			max = 0;
		} else {
			max = max - bounds.Height();
		}
		prop = bounds.Height() / (max + bounds.Height());
	} else {
		max = 0;
		prop = 1.0;
	}

	sb->SetRange(0, max);
	sb->SetProportion(prop);

	if (count) {
		// what should the steps be given that cell height is variable???
		sb->SetSteps(ceil(FirstItem()->Height()), bounds.Height());
	}
}

/* ---------------------------------------------------------------- */

void BListView::InvalidateFrom(int32 index)
{
	BRect	bounds = Bounds();
	BRect	frame = ItemFrame(index);
	float	y = frame.top;

	if(y < bounds.top) {
		Invalidate();
	} else if (y <= bounds.bottom) {
		bounds.top = y;
		Invalidate(bounds);
	}
}

/* ---------------------------------------------------------------- */

bool BListView::AddList(BList *newItems)
{
	return AddList(newItems, CountItems());
}

/* ---------------------------------------------------------------- */

bool BListView::AddList(BList *newItems, int32 index)
{
	if (fList.AddList(newItems, index) == false)
		return false;
	
	long c = newItems->CountItems();
	if ((fFirstSelected != -1) && (index <= fFirstSelected))
		fFirstSelected += c;

	if ((fLastSelected != -1) && (index <= fLastSelected))
		fLastSelected += c;

	if (Window()) {
		BFont font;
		GetFont(&font);

		BListItem	*cell;
		long		i = 0;
		while ((cell = (BListItem *) newItems->ItemAt(i++)) != 0)
			cell->Update(this, &font);
		FixupScrollBar();
		InvalidateFrom(index);
	}

	return true;
}

/* ---------------------------------------------------------------- */

void BListView::SortItems(int (*cmp)(const void *, const void *))
{
	DeselectAll();
	fList.SortItems(cmp);
	Invalidate();
}

/* ---------------------------------------------------------------- */

int32 BListView::CurrentSelection(int32 index) const
{
	long	result = -1;

	if (fFirstSelected == -1)
		return result;

	if (index == 0)
		result = fFirstSelected;
	else {
		long	n;
		long	i;
		for (n = 0, i = fFirstSelected; i <= fLastSelected; i++) {
			BListItem *cell = ItemAt(i);
			if (cell->IsSelected()) {
				if (n == index)
					return i;
				n++;
			}
		}
	}
	return result;
}

/* ---------------------------------------------------------------- */

bool BListView::IsItemSelected(int32 index) const
{
	BListItem *cell = ItemAt(index);
	return cell ? cell->IsSelected() : false;
}

/* ---------------------------------------------------------------- */

void BListView::TargetedByScrollView(BScrollView *sv)
{
	fScrollView = sv;
}

/* ---------------------------------------------------------------- */

const BListItem **BListView::Items() const
	{ return (const BListItem **) fList.Items(); }

/*-------------------------------------------------------------*/
	/*
	 A generic BListView supports the following:
	 	GET/SET		"Selection"		DIRECT form only
	 	EXECUTE		"Selection"		DIRECT form only
	*/
#if _SUPPORTS_FEATURE_SCRIPTING
enum {
	LV_ITEM_EXEC,
	LV_ITEM_COUNT,
	LV_SEL_GET,
	LV_SEL_SET,
	LV_SEL_SET_ALL,
	LV_SEL_EXEC,
	LV_SEL_COUNT
};

static property_info	prop_list[] = {
	{"Item",
		{B_EXECUTE_PROPERTY},
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, B_RANGE_SPECIFIER,
			B_REVERSE_RANGE_SPECIFIER},
		"",
		LV_ITEM_EXEC,
		{},
		{},
		{}
	},
	{"Item",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		"",
		LV_ITEM_COUNT,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"Selection",
		{B_GET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		LV_SEL_GET,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"Selection",
		// Select or deselect specified items based on bool data. 
		{B_SET_PROPERTY},
		{B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER,
			B_RANGE_SPECIFIER, B_REVERSE_RANGE_SPECIFIER},
		"",
		LV_SEL_SET,
		{ B_BOOL_TYPE },
		{},
		{}
	},
	{"Selection",
		// select or deselect entire list based on bool data
		{B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		LV_SEL_SET_ALL,
		{ B_BOOL_TYPE },
		{},
		{}
	},
	{"Selection",
		{B_EXECUTE_PROPERTY},
		{B_DIRECT_SPECIFIER},
		"",
		LV_SEL_EXEC,
		{},
		{},
		{}
	},
	{"Selection",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		"",
		LV_SEL_COUNT,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};
#endif

/* ---------------------------------------------------------------- */

void BListView::MessageReceived(BMessage *msg)
{
	bool		handled = false;
	BMessage	reply(B_REPLY);
	status_t	err = B_OK;

	int32		match_code;

	if (msg->what == B_SELECT_ALL) {
		if (fListType == B_MULTIPLE_SELECTION_LIST) {
			Select(0, CountItems()-1);
		}
	} else if (msg->what == 'TICK') {
		if (fTrack) {
			bigtime_t now = system_time();
			if (now > fTrack->last_mm_time + TICK_TIME) {
				PRINT(("Received TICK event.\n"));
//+				TRACE();
				// haven't got a MouseMoved event lately
				DoMouseMoved(ConvertFromScreen(fTrack->last_mm_pt));
			}
		}
	}
	
#if _SUPPORTS_FEATURE_SCRIPTING
	else if (msg->FindInt32("_match_code_", &match_code) == B_OK) {
		int32		cur = 0;
		BMessage	spec;
		int32		form = 0;
		const char	*property = NULL;
		int32		sindex = -1;
		int32		eindex = -1;
		int32		range = 0;
		bool		has_range = false;
		bool		has_index = false;
		msg->GetCurrentSpecifier(&cur, &spec, &form, &property);
		
		/*
		 The following 4 switch statements determine the 'index' and
		 'range' values (if they exist) of the current specifier.
		*/
		switch (form) {
			case B_INDEX_SPECIFIER:
			case B_REVERSE_INDEX_SPECIFIER:
			case B_RANGE_SPECIFIER:
			case B_REVERSE_RANGE_SPECIFIER:
				if (spec.FindInt32("index", &sindex) != B_OK)
					goto done;
				has_index = true;
				break;
		}
		switch (form) {
			case B_REVERSE_INDEX_SPECIFIER:
			case B_REVERSE_RANGE_SPECIFIER:
				sindex = CountItems() - sindex;
				break;
		}
		switch (form) {
			case B_RANGE_SPECIFIER:
			case B_REVERSE_RANGE_SPECIFIER:
				if (spec.FindInt32("range", &range) != B_OK)
					goto done;
				eindex = sindex + (range - 1);
				has_range = true;
				break;
		}
		switch (form) {
			case B_INDEX_SPECIFIER:
			case B_REVERSE_INDEX_SPECIFIER:
			case B_RANGE_SPECIFIER:
			case B_REVERSE_RANGE_SPECIFIER:
				if ((sindex < 0) || (has_range && (eindex < sindex))) {
					goto done;
				}
		}

		/*
		 Do some range checking on all values.
		*/
		if (has_index) {
			sindex = RangeCheck(sindex);
		}
		if (has_range) {
			int32	c = CountItems();
			if (eindex >= c)
				eindex = c - 1;
		}
//+		PRINT(("sindex=%d, eindex=%d\n", sindex, eindex));
		if ((has_index && (sindex == -1)) || (has_range && (eindex == -1))) {
//+			PRINT(("Bad index\n"));
			handled = true;
			err = B_BAD_INDEX;
			reply.AddString("message", "Bad index into the list");
			goto done;
		}

		/*
		 Consider a list that only wants SINGLE selections.
		*/
		if ((fListType == B_SINGLE_SELECTION_LIST) && has_range) {
			if (sindex != eindex) {
				// only selection ranges that spec 1 item are valid
//+				PRINT(("Illegal operation on SINGLE selection list\n"));
				handled = true;
				err = B_BAD_VALUE;
				reply.AddString("message",
					"Illegal operation on single selection list");
				goto done;
			} else {
				// just a single selection, so treat is as such
				has_range = false;
				eindex = -1;
			}
		}

		switch (match_code) {
			case LV_ITEM_EXEC: {
				// Select and invoke the specified items.
				handled = true;
				if (eindex >= 0) {
//+					PRINT(("LV: Execute items %d-%d\n", sindex, eindex));
					Select(sindex, eindex, false);
				} else {
//+					PRINT(("LV: Execute item %d\n", sindex));
					Select(sindex, false);
				}
				Invoke();
				err = B_OK;
				break;
			}
			case LV_ITEM_COUNT: {
				handled = true;
//+				PRINT(("LV: Count items\n"));
				reply.AddInt32("result", CountItems());
				err = B_OK;
				break;
			}
			case LV_SEL_GET: {
				int32	i = 0;
				int32	s = 0;
				handled = true;
//+				PRINT(("LV: Get selection\n"));
				while ((s = CurrentSelection(i++)) >= 0) {
					reply.AddInt32("result", s);
				}
				break;
			}
			case LV_SEL_SET: {
				// select or deselect the specified item or range of items
				bool	select;
				handled = true;
				if (msg->FindBool("data", &select) != B_OK) {
					reply.AddString("message","didn't find expected bool data");
					err = B_BAD_VALUE;
					break;
				}
				if (eindex >= 0) {
//+					PRINT(("LV: %s items %d-%d\n",
//+						select ? "Selecting" : "Deselecting", sindex, eindex));
					if (select)
						Select(sindex, eindex, true);
					else
						Deselect(sindex, eindex);
				} else {
//+					PRINT(("LV: %s item %d\n",
//+						select ? "Selecting" : "Deselecting", sindex));
					if (select) {
						if (fListType == B_SINGLE_SELECTION_LIST) {
							bool changed = false;
							// delete the current selection first
							int32 cs = CurrentSelection(0);
							if ((cs >= 0) && (cs != sindex)) {
								_Deselect(cs);
								changed = true;
							}
							Select(sindex);
							if (CurrentSelection(0) != sindex) {
								// sindex was probably disabled
								// need to force a send of the selection msg
								if (changed) {
									SelectionChanged();
									InvokeNotify(fSelectMessage, B_CONTROL_MODIFIED);
								}
							}
						} else {
							Select(sindex, true);
						}
					} else {
						Deselect(sindex);
					}
				}
				break;
			}
			case LV_SEL_SET_ALL: {
				// Select or Deselect entire list
				bool	select;
				handled = true;
				if (msg->FindBool("data", &select) != B_OK) {
					reply.AddString("message","didn't find expected bool data");
					err = B_BAD_VALUE;
					break;
				}
				if ((fListType == B_SINGLE_SELECTION_LIST) && select) {
					err = B_BAD_VALUE;
					reply.AddString("message",
						"Illegal operation on single selection list");
					break;
				}

				if (select)
					Select(0, CountItems()-1);
				else
					DeselectAll();

//+				PRINT(("LV: %s all items\n", select?"Selecting":"Deselecting"));
				break;
			}
			case LV_SEL_EXEC: {
				// Invoke the already selected items. Still calls invoke
				// if selection is empty
				handled = true;
//+				PRINT(("LV: Invoke selected items\n"));
				Invoke();
				break;
			}
			case LV_SEL_COUNT: {
				// Count the number of selected items
				int32	i = 0;
				int32	s = 0;
				handled = true;
//+				PRINT(("LV: Count selected items\n"));
				while ((s = CurrentSelection(i++)) >= 0) {
				}
				reply.AddInt32("result", i-1);
				break;
			}
		}
	}
#endif

done:
	if (handled) {
		reply.AddInt32("error", err);
		msg->SendReply(&reply);
	} else {
		BView::MessageReceived(msg);
	}
}

/*-------------------------------------------------------------*/

BHandler *BListView::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(prop))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BHandler	*target = NULL;
	BPropertyInfo	pi(prop_list);
	int32			i;

//+	PRINT(("BListView::Resolve: msg->what=%.4s, index=%d, form=0x%x, prop=%s\n",
//+		(char*) &(msg->what), index, spec->what, prop));

	if ((i = pi.FindMatch(msg, index, spec, form, prop)) >= 0) {
//+		PRINT(("prop=%s, index=%d, code=%d\n",
//+			prop, i, prop_list[i].extra_data));
		target = this;
		msg->AddInt32("_match_code_", prop_list[i].extra_data);
	} else {
		target = BView::ResolveSpecifier(msg, index, spec, form, prop);
	}

	return target;
#else
	return NULL;
#endif
}

/*---------------------------------------------------------------*/

status_t	BListView::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-list-view");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return BView::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}

/*----------------------------------------------------------------*/

status_t BListView::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

BListView &BListView::operator=(const BListView &) {return *this;}

/* ---------------------------------------------------------------- */

/* BListView::_ReservedListView1() is taken for DoMiscellaneous() -- h+ 10/22/97*/
void BListView::_ReservedListView2() {}
void BListView::_ReservedListView3() {}
void BListView::_ReservedListView4() {}

/* ---------------------------------------------------------------- */

void BListView::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

void BListView::WindowActivated(bool state)
{
	BView::WindowActivated(state);
	if (IsFocus() && fScrollView) {
		fScrollView->Invalidate();
		Window()->UpdateIfNeeded();
	}
}

/* ---------------------------------------------------------------- */

void BListView::MouseUp(BPoint pt)
{
	if (fTrack) {
		PRINT(("Async MouseUp\n"));
		DoMouseMoved(pt);
		DoMouseUp(pt);
	}
}

/*---------------------------------------------------------------*/

void	BListView::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/* ---------------------------------------------------------------- */

void BListView::MouseMoved(BPoint pt, uint32 , const BMessage *)
{
	if (fTrack) {
//+		bigtime_t	when = 0;
//+		Window()->CurrentMessage()->FindInt64("when", &when);
//+		PRINT(("Async MouseDown (%Ld)\n", when));
//+		{
//+		BStopWatch	timer("DoMouseMoved");
		if( TryInitiateDrag(pt) ) return;
		DoMouseMoved(pt);
//+		}
	}
}

/* ---------------------------------------------------------------- */

bool BListView::InitiateDrag(BPoint , int32 , bool)
{
	return false;
}

/*---------------------------------------------------------------*/

void BListView::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BListView::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::GetPreferredSize(width, height);
}

/*---------------------------------------------------------------*/

void	BListView::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllAttached();
}

/*---------------------------------------------------------------*/

void	BListView::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllDetached();
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

