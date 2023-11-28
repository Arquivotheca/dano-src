// SimpleListView.cpp
#include <Window.h>
#include <ScrollView.h>
#include <Looper.h>
#include <Message.h>
#include <ScrollBar.h>
#include "SimpleListView.h"

#define DEBUG 0
#include <Debug.h>

//  SimpleListView class
//  (C) 1996-1997 by Michael Klingbeil

#define DRAG_REORDERING 1
#define msg_DRAG_LISTITEM 'DLsI'

SimpleListView::SimpleListView( BRect frame,
				const char *name,
				ulong resizeMask,
				ulong flags)
	: BView(frame,name,resizeMask,flags | B_FRAME_EVENTS)
{
	// currently displayed list
	fList = NULL;
	fScrollView = NULL;
	fDrawFocus = true;
	
	// nothing is selected
	fLowSelection = fHighSelection = -1;
	selPoint = selMark = -1;
	
	SetMultipleSelect(TRUE);
	SetDragReorder(TRUE);
	SetBaselineOffset(3);
	SetItemHeight(16);
}

SimpleListView::~SimpleListView()
{
}

void SimpleListView::Draw(BRect up)
{
	PRINT(("entering draw method update region \n"));

	long start = (long)(up.top/ItemHeight());
	long end = (long)(up.bottom/ItemHeight());
	
	// pin start and end values
	if (start < 0)
		return;
	end = min_c(CountItems()-1,end);
	
	// cache values for faster drawing
	BRect fastFrame = ItemFrame(0);
	long iHeight = (long)(ItemHeight());
	fastFrame.top = start*ItemHeight();
	fastFrame.bottom = fastFrame.top + iHeight-1;
	while(start <= end) {
		PRINT(("drawing item %d\n",start));
		DrawItem(up,start,&fastFrame);
		fastFrame.top += iHeight;
		fastFrame.bottom += iHeight;
		start++;
	}

	/***	// stuff to draw focus line
	if (IsFocus()) {
		BRect r = Bounds();
		r.right = r.left + 1;
		BRect i = up & r;
		FillRect(i,B_SOLID_HIGH);
	}
	***/
}

void SimpleListView::MakeFocus(bool state)
{
	BView::MakeFocus(state);
	
	if (fDrawFocus && fScrollView) {
		fScrollView->SetBorderHighlighted(state);
	}
}

void SimpleListView::WindowActivated(bool state)
{
	BView::WindowActivated(state);
	
	if (fDrawFocus && fScrollView && IsFocus()) {
		fScrollView->SetBorderHighlighted(state);
	}
}

void SimpleListView::TargetedByScrollView(BScrollView *view)
{
	fScrollView = view;
}

void SimpleListView::MouseDown(BPoint where)
{
	MakeFocus(TRUE);
	
	ulong buttons;
	bool togSelecting = TRUE;
	long firstSelection = 0;
	BMessage *curMessage = Looper()->CurrentMessage();
	long modifiers = curMessage->FindInt32("modifiers");
	long clicks = curMessage->FindInt32("clicks");
	long max = CountItems();
	// fix scroll bar shit!!!
	float minScroll, maxScroll;
	float smallStep, bigStep;
	
	BScrollBar *vScroll = ScrollBar(B_VERTICAL);
	vScroll->GetRange(&minScroll,&maxScroll);
	vScroll->GetSteps(&smallStep,&bigStep);

	long curItem = (long)(where.y / ItemHeight());
	if (curItem >= CountItems()) {
		// perhaps deselect all?
		return;
	}
	
	if (IsItemDisabled(curItem))
		return;
	
	// check for double click, is this right???
	if (clicks == 2 && curItem == LowSelection()) {
		do {
		// wait for mouse to go up
			snooze(20 * 1000);
			GetMouse(&where, &buttons, TRUE);
		} while (buttons);
		long endItem = (long)(where.y / ItemHeight());
		if (endItem == curItem)
			Invoke(curItem);		
		return;
	}
	// first handle the initial click (drag behavior is different)	
	if ((modifiers & B_SHIFT_KEY) && fMultipleSelect) {
		// should discontiguous selections be cleared first???
		// if they aren't, low and hi are messed up
		
		if (curItem < selPoint) {
			// click before point
			if (selMark > selPoint) {
				// add to selection on opposite side
				for (long i = selPoint-1; i >= curItem; i--)
					Select(i);
				// reverse
				selPoint = selMark;
			}
			else {
				for (long i = selMark-1; i >= curItem; i--)
					Select(i);
				for (long i = selMark; i < curItem; i++)
					DeSelect(i);
			}
		}
		else {
			if (selMark < selPoint) {
				for (long i = selPoint+1; i <= curItem; i++)
					Select(i);
				selPoint = selMark;
			}
			else {
				for (long i = selMark+1; i <= curItem; i++)
					Select(i);
				for (long i = selMark; i > curItem; i--)
					DeSelect(i);
			}
		}
		selMark = curItem;
	}
	else if ((modifiers & B_COMMAND_KEY) && fMultipleSelect) {
		ToggleSelect(curItem);
		if (ItemSelected(curItem)) {
			togSelecting = TRUE;
		}
		else {
			togSelecting = FALSE;
		}
		selPoint = selMark = curItem;
		// fix low selection + high selection at end of routine
	}
	else {
		// deselect existing items
		if (fLowSelection >= 0 && fHighSelection >= 0) {
			for(long i = fLowSelection; i <= fHighSelection; i++) {
				if (i != curItem)
					DeSelect(i);
			}
		}
		selMark = selPoint = curItem;
		fLowSelection = fHighSelection = curItem;
		Select(curItem);
		
		if (!fDragReorder) {		
			SelectionSet();
			return;
		}
		// low selection + high selection are fine
		firstSelection = curItem;
		
		// initiate for possible drag reordering operation
		SetPenSize(1.0);
	}

	SetHighColor(0,0,0);
	SetLowColor(255,255,255);
	bool lineDrawn = FALSE;
	do {
		// handle drags
		snooze(20 * 1000);
		bool scrolling = FALSE;
		GetMouse(&where, &buttons, TRUE);
		curItem = (long)(where.y / ItemHeight());
		BRect b = Bounds();
		
		// handle auto scrolling
		if (where.y < b.top && b.top > minScroll) {
			ScrollBy(0,-smallStep);
			Window()->UpdateIfNeeded();
		}
		else if (where.y > b.bottom &&  b.top < maxScroll) {
			// scroll down
			ScrollBy(0,smallStep);
			Window()->UpdateIfNeeded();
		}
		// pin at min and max
		
		if (curItem < 0)
			curItem = 0;
		else if (curItem >= max)
			curItem = max-1;				
		
		if (modifiers & B_SHIFT_KEY) {
			// from selPoint to curItem should be the selection range
			if (curItem > selMark) {
				// cursor moved down
				if (selMark > selPoint) {
					// expand selection from selMark to curItem (new curItem)
					for (long i = selMark+1; i <= curItem; i++) {
						Select(i);
					}
				}
				else {
					// selMark <= selPoint
					// contract selection from selMark to min(curItem, pivot)
					long range = min_c(curItem,selPoint);
					for (long i = selMark; i < range; i++)
						DeSelect(i);
					// expand selection from range to curItem;
					for (long i = range+1; i <= curItem; i++)
						Select(i);
				}
			}
			else if (curItem < selMark) {
				// cursor moved up
				if (selMark < selPoint) {
					// expand selection from selMark up to curItem
					for (long i = selMark-1; i >= curItem; i--)
						Select(i);
				}
				else {
					// contract selection 
					long range = max_c(curItem,selPoint);
					for (long i = selMark; i > range; i--)
						DeSelect(i);
					for (long i = range-1; i >= curItem; i--)
						Select(i);
				
				}	
			}
			selMark = curItem;
		}
		else if (modifiers & B_COMMAND_KEY) {
			// assertion, lowSel and highSel are properly set
			if (curItem > selMark) {
				for (long i = selMark+1; i <= curItem; i++) {
					if (togSelecting)
						Select(i);
					else
						DeSelect(i);
				}
			}
			else {
				for (long i = selMark-1; i >= curItem; i--) {
					if (togSelecting)
						Select(i);
					else
						DeSelect(i);		
				}
			}
			selMark = curItem;
		}
		
#if DRAG_REORDERING
		else {
			if (where.y < 0) {
				// allows us to drag to the top of the list
				curItem = -1;
				//PRINT(("curItem is -1 and lastItem is %d\n",lastItem));
			} 

			if (selMark != curItem /* &&
				curItem != firstSelection */) {
				float v;
				BRect b = Bounds();				
				
				SetDrawingMode(B_OP_INVERT);
				
				// erase previous (if any)
				if (selMark != firstSelection) {
					v = (selMark+1)*ItemHeight()-1.0;
					
					if (v-1 >= b.top && v <= b.bottom && lineDrawn) {
						StrokeRect(BRect(0,v,b.right,v+1),B_MIXED_COLORS);
					}
				}
				// draw current
				if (curItem != firstSelection) {
					v = (curItem+1)*ItemHeight()-1.0;
					
					if (v-1 >= b.top && v <= b.bottom) {
						StrokeRect(BRect(0,v,b.right,v+1),B_MIXED_COLORS);	
						lineDrawn = TRUE;
					}
					else
						lineDrawn = FALSE;
				}
				
				SetDrawingMode(B_OP_COPY);				
				selMark = curItem;
			}
		}
#endif
	} while (buttons);
	
	if (((modifiers & B_COMMAND_KEY) || (modifiers & B_SHIFT_KEY)) &&
			fMultipleSelect )
	{
		// potential discontiguous selection
		fLowSelection = -1;
		fHighSelection = -1;
		for (long i = 0; i < max; i++) {
			if (ItemSelected(i)) {
				fLowSelection = i;
				break;	
			}
		}
		if (fLowSelection == -1) {
			selPoint = selMark = fLowSelection;
		}
		else {
			for (long i = max-1; i >= fLowSelection; i--) {
				if (ItemSelected(i)) {
					fHighSelection = i;
					break;
				}
			}
			if (!togSelecting)
				// selections were being untoggled, reset the point and mark
				selPoint = selMark = fLowSelection;
		}		
	}
#if DRAG_REORDERING
	else {
		if (selMark != firstSelection) {

			if (selMark != firstSelection - 1) {
				ReorderItem(firstSelection,selMark);
				
				// selMark is the new position
				if (selMark < firstSelection) 
					selMark++;
				fLowSelection = fHighSelection = selPoint = selMark;
			}
			
			// erase separator line
			BRect b = Bounds();
			float v = (selMark+1)*ItemHeight()-1.0;
			if (v-1 >= b.top && v <= b.bottom && lineDrawn) {
				StrokeRect(BRect(0,v,b.right,v+1),B_MIXED_COLORS);
			}			
		}
	}
#endif
	PRINT(("lowSelection is %d, highSelection is %d\n",fLowSelection,fHighSelection));
	SelectionSet();
}

void SimpleListView::ScrollTo(BPoint where)
{
	BScrollBar *sb = ScrollBar(B_VERTICAL);
	if (sb) {
		float min, max;
		sb->GetRange(&min,&max);
		if (where.y < min)
			where.y = min;
		else if (where.y > max)
			where.y = max;
	}
	BView::ScrollTo(where);
}

void SimpleListView::KeyDown(const char *bytes, int32 byteCount)
{
	//PRINT(("list view got key down\n"));
	long modifiers = Looper()->CurrentMessage()->FindInt32("modifiers");
	switch (*bytes) {
		case B_DOWN_ARROW: {
			bool multiple = (modifiers & B_SHIFT_KEY) && fMultipleSelect;

			long last = CountItems()-1;
			if (selMark >= last)
				return;
			
			long nextItem = selMark;
			
			// find the next non-disabled item
			while(nextItem < last && IsItemDisabled(++nextItem))
				;

			if (nextItem == selMark || IsItemDisabled(nextItem)) {
				// nothing new found
				return;
			}
			if (multiple) {			
				if (selMark < selPoint) {
					// selection contracted
					DeSelect(selMark);
				}
				else {
					// selection expanded
					Select(nextItem);
				}
				selMark = nextItem;
				
				// potential discontiguous selection
				fLowSelection = 0;
				while(fLowSelection <= last && !ItemSelected(fLowSelection))
					fLowSelection++;
				fHighSelection = last;
				while(fHighSelection >= 0 && !ItemSelected(fHighSelection))
					fHighSelection--;
			}
			else {
				if (fLowSelection >= 0 && fHighSelection >= 0) {
					// some items are selected
					// deSelect all but the current item
					for (long i = fLowSelection; i <= fHighSelection; i++)
						if (i != selMark) DeSelect(i);
				}
				if (selMark >= 0)
					DeSelect(selMark);
			
				PRINT(("nextItem is %d\n",nextItem));		
				Select(nextItem);
				selPoint = selMark = fLowSelection = fHighSelection = nextItem;
			}
			if ((selMark+1)*ItemHeight() > Bounds().bottom) {
				// scroll down
				ScrollBy(0, ItemHeight());
			}
			SelectionSet();
			break;
		}
		case B_UP_ARROW: {
			bool multiple = (modifiers & B_SHIFT_KEY) && fMultipleSelect;
			
			long last = CountItems()-1;
			if (selMark == 0 || last < 0)
				return;
				
			// we have at least one item in our list
			// deal with selMark is negative (no items selected)
			if (selMark < 0) {
				if (!IsItemDisabled(0)) {
					selMark = selPoint = 0;
					fLowSelection = fHighSelection = 0;
					Select(selMark);
				}
				return;
			}

			long nextItem = selMark;
			
			// find the next non-disabled item
			while(nextItem > 0 && IsItemDisabled(--nextItem))
				;

			if (nextItem == selMark || IsItemDisabled(nextItem)) {
				// nothing new found
				return;
			}
			if (multiple) {			
				if (selMark > selPoint) {
					// selection contracted
					DeSelect(selMark);
				}
				else {
					// selection expanded
					Select(nextItem);					
				}
				selMark = nextItem;
				
				// potential discontiguous selection
				fLowSelection = 0;
				while(fLowSelection <= last && !ItemSelected(fLowSelection))
					fLowSelection++;
				fHighSelection = last;
				while(fHighSelection >= 0 && !ItemSelected(fHighSelection))
					fHighSelection--;
			}
			else {
				if (fLowSelection >= 0 && fHighSelection >= 0) {
					// some items are selected
					// deSelect all but the current item
					for (long i = fLowSelection; i <= fHighSelection; i++)
						if (i != selMark) DeSelect(i);
				}

				if (selMark >= 0)
					DeSelect(selMark);
				Select(nextItem);
				selPoint = selMark = fLowSelection = fHighSelection = nextItem;
			}
			if ((selMark-1)*ItemHeight() < Bounds().top) {
				// scroll up if necessary
				ScrollBy(0, -ItemHeight());
			}
			SelectionSet();
			break;
		}
		case B_RETURN:
			if ((fLowSelection == fHighSelection) && ( fLowSelection != -1))
				Invoke(fLowSelection);
			break;
		case B_PAGE_DOWN:
			//PRINT(("got pg down"));
			ScrollBy(0, Bounds().Height() - ItemHeight());
			break;
		case B_PAGE_UP:
			//PRINT(("got pg up"));
			ScrollBy(0, - (Bounds().Height() - ItemHeight()));
			break;
		default:
			BView::KeyDown(bytes, byteCount);
	}
}

void SimpleListView::AttachedToWindow()
{
	BView::AttachedToWindow();

	FixupScrollBar();	
	SetViewColor(255,255,255);
}

/*
void SimpleListView::SetFontName(const char *name)
{
	BView::SetFontName(name);
	
	font_info fInfo;
	GetFontInfo(&fInfo);
	SetItemHeight(fInfo.size+BaselineOffset());
}

void SimpleListView::SetFontSize(float pts)
{
	BView::SetFontSize(pts);
	
	font_info fInfo;
	GetFontInfo(&fInfo);
	SetItemHeight(iHeight = fInfo.size+BaselineOffset());
}
*/
void SimpleListView::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width,new_height);
	FixupScrollBar();
}

void SimpleListView::Update()
{
	FixupScrollBar();
/** not sure if this is good ***/
	int count = CountItems()-1;
	if (fLowSelection > count ||
		fHighSelection > count)
	{
		selPoint = selMark = fLowSelection = fHighSelection = -1;
		SelectionSet();
	}
/** not sure if this is good ***/

	Invalidate();
}

void SimpleListView::SelectItem(long index)
{
	ListItem *li = ItemAt(index);
	li->selected = TRUE;
	
	if (LowSelection() == -1)
		selPoint = selMark = fLowSelection = fHighSelection = index;
	else if (index < LowSelection())
		selMark = fLowSelection = index;
	else if (index > HighSelection())
		selMark = fHighSelection = index;
}

void SimpleListView::SelectItem(ListItem *li)
{
	long index = IndexOf(li);
	li->selected = TRUE;
	if (LowSelection() == -1)
		selPoint = selMark = fLowSelection = fHighSelection = index;
	else if (index < LowSelection())
		selMark = fLowSelection = index;
	else if (index > HighSelection())
		selMark = fHighSelection = index;
}

void SimpleListView::SelectAllItems()
{
	selMark = fHighSelection = CountItems()-1;
	for(long i = CountItems()-1; i >= 0; i--) {
		ListItem *li = ItemAt(i);
		li->selected = TRUE;	
	}
	selPoint = fLowSelection = 0;
}

void SimpleListView::SelectNoItems()
{
	if (fHighSelection >= 0 && fLowSelection >= 0) {
		for (long i = fHighSelection; i >= fLowSelection; i--) {
			ListItem *li = ItemAt(i);
			li->selected = FALSE;
		}
	}
	selPoint = selMark = fLowSelection = fHighSelection = -1;
}

void SimpleListView::SetMultipleSelect(bool flag)
{
	fMultipleSelect = flag;
}

void SimpleListView::SetDragReorder(bool flag)
{
	fDragReorder = flag;
}

void SimpleListView::InvalidateItem(long index)
{
	Invalidate(ItemFrame(index));
}

void SimpleListView::HighlightItem(bool on, long index,BRect *iFrame)
{
	index;
	
	if (on)
		InvertRect(*iFrame);
}

bool SimpleListView::IsItemDisabled(long index)
{
	index;
	
	return FALSE;
}

void SimpleListView::SetItemHeight(float val) {
	iHeight = val;
}

// private
void SimpleListView::ReorderItem(long prevItem, long curItem)
{
	if (prevItem == curItem)
		return;
	
	ListItem *li = RemoveItem(prevItem);
	if (curItem < prevItem)
		curItem++;
	
	fList->AddItem(li,curItem);
	Invalidate();
	// /*selPoint */ selMark = fLowSelection = fHighSelection = curItem;
}

void SimpleListView::FixupScrollBar()
{
	BScrollBar *vScroll = ScrollBar(B_VERTICAL);
	
	if (vScroll) {
		BRect b = Bounds();
		long listHeight = (long)(CountItems()*ItemHeight()) - 1;
		long viewHeight = (long)b.Height();
		
		if (listHeight < viewHeight)
			listHeight = viewHeight;
		
		vScroll->SetRange(0,listHeight - viewHeight);
		vScroll->SetProportion((float)viewHeight/(float)listHeight);
		vScroll->SetValue(b.top);
		vScroll->SetSteps(ItemHeight()/2.0,viewHeight);
	}
}

void SimpleListView::Select(long index)
{
	if (IsItemDisabled(index))
		return;
	ListItem *li = ItemAt(index);
	if (li->selected == FALSE) {
		li->selected = TRUE;
		DrawItem(Bounds(),index);
	}
}

void SimpleListView::DeSelect(long index)
{
	if (IsItemDisabled(index))
		return;
	ListItem *li = ItemAt(index);
	if (li->selected == TRUE) {
		li->selected = FALSE;
		BRect iFrame = ItemFrame(index);
		FillRect(iFrame,B_SOLID_LOW);
		DrawItem(Bounds(),index,&iFrame);
	}
}

void SimpleListView::ToggleSelect(long index)
{
	ListItem *li = ItemAt(index);
	bool sel = !li->selected;
	li->selected = sel;
	BRect iFrame = ItemFrame(index);
	
	// whiten frame
	if (!sel) {
		FillRect(iFrame,B_SOLID_LOW);
	}
	DrawItem(Bounds(),index,&iFrame);
}

BRect SimpleListView::ItemFrame(long index)
{
	BRect frame;
	
	frame.left = 0;
	frame.right = 1000;
	frame.top = index*ItemHeight();
	frame.bottom = frame.top+ItemHeight()-1;
	return frame;
}

void SimpleListView::SetBaselineOffset(float off)
{
	fBaselineOff = off;
}

long SimpleListView::IndexOf(ListItem *item)
{
	long i;
	for (i = fList->CountItems()-1; i >= 0; i--) {
		if (fList->ItemAt(i) == item)
			break;
	}
	return i;
}
