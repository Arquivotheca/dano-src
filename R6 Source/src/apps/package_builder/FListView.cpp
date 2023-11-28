#include <Be.h>
// FListView.cpp

#include "FListView.h"
#include "ColLabelView.h"
#include "RList.h"
#include "MyDebug.h"

// #include <Debug.h>

FListView::FListView( BRect frame,
				const char *name,
				ulong resizeMask,
				ulong flags)
	: BView(frame,name,resizeMask,flags)
{
	fColumns = new RList<ColumnInfo *>;
// currently displayed list
	fList = NULL;
	dataWidth = 0;
	
// nothing is selected
	lowSelection = highSelection = -1;
	
	SetMultipleSelect(TRUE);
	SetShowColumns(FALSE);
}

FListView::~FListView()
{
	for(long i = fColumns->CountItems()-1; i >= 0;i--)
	{
		delete fColumns->ItemAt(i);
	}
	delete fColumns;
}

void FListView::Draw(BRect up)
{
	//PRINT(("entering draw method update region "));
	long start = up.top/ItemHeight();
	long end = up.bottom/ItemHeight();
	
	// pin star and end values
	if (start < 0)
		return;
	end = min_c(CountItems()-1,end);
	
	// cache values for fast drawing
	
	BRect fastFrame = Bounds();
	long iHeight = ItemHeight();
	fastFrame.top = start*ItemHeight();
	fastFrame.bottom = fastFrame.top + iHeight-1;
	
//	PRINT(("Draw method start %d  end %d\n",start,end));
	
	while(start <= end) {
		DrawItem(up,start++,&fastFrame);
		fastFrame.top += iHeight;
		fastFrame.bottom += iHeight;
	}
		
	/***
	if (IsFocus()) {
		BRect r = Bounds();
		r.left = 0;
		r.right = r.left + 1;
		BRect i = up & r;
		FillRect(i,B_SOLID_HIGH);
	}***/
}

void FListView::MakeFocus(bool state)
{
	BView::MakeFocus(state);
	BView *v = Parent();
	if (v && typeid(*v) == typeid(BScrollView)) {
		BScrollView *sv = (BScrollView *)v;
		sv->SetBorderHighlighted(state);
	}
	/***
	BRect r = Bounds();
	r.left = 0;
	r.right = r.left + 1;
	FillRect(r,state ? B_SOLID_HIGH : B_SOLID_LOW);
	if (!state) {
		// blacken left edges
		long start = r.top/ItemHeight();
		long end = r.bottom/ItemHeight();
		end = min_c(CountItems()-1,end);
		while(start <= end) {
			if (ItemAt(start)->selected == TRUE) {
				BRect fr = ItemFrame(start);
				fr.right = fr.left + 1;
				InvertRect(fr);
			}
			start++;
		}
	}
	***/
}

void FListView::WindowActivated(bool state)
{
	if (IsFocus()) {
		BView *v = Parent();
		if (v && typeid(*v) == typeid(BScrollView)) {
			BScrollView *sv = (BScrollView *)v;
			sv->SetBorderHighlighted(state);
		}
	}
}

void FListView::MouseDown(BPoint where)
{
	// get_click_speed(&double)

	long curItem;
	long prevItem;
	ulong buttons;
	BMessage *current = Window()->CurrentMessage();
	// select the item
	
	prevItem = where.y / ItemHeight();

	if (prevItem >= CountItems()) {
		MakeFocus(true);
		return;
	}
		
	uint32 modifiers = current->FindInt32("modifiers");
	
	if ((modifiers & B_COMMAND_KEY) && fMultipleSelect) {
		ToggleSelect(prevItem);
		
		if (IsItemSelected(prevItem)) {
			// the item has been selected
			if (lowSelection < 0) {
				// nothing was selected
				lowSelection = highSelection = prevItem;
			}
			else if (prevItem < lowSelection)
				lowSelection = prevItem;
			else if (prevItem > highSelection)
				highSelection = prevItem;
		}
		else {
			// the item has been deselected
			if (lowSelection == highSelection && lowSelection == prevItem)
				lowSelection = highSelection = -1;
			else if (highSelection == prevItem) {
				// scan back
				while( --highSelection > lowSelection && !IsItemSelected(highSelection))
						;
					
			}
			else if (lowSelection == prevItem ){
				// scan forward
				while( ++lowSelection < highSelection && !IsItemSelected(lowSelection))
					;
			}
		}
	}
	else if ((modifiers & B_SHIFT_KEY) && fMultipleSelect) {
		if (lowSelection < 0) {
			Select(prevItem);
			lowSelection = highSelection = prevItem;
		}
		else if (prevItem < lowSelection) {
			// select range from prevItem up to highSelection
			for (long i = prevItem; i < highSelection; i++)
				Select(i);
			lowSelection = prevItem;
		}
		else if (prevItem >= highSelection) {
			// select range from lowSelection up to prevItem
			for (long i = lowSelection; i <= prevItem; i++)
				Select(i);
			highSelection = prevItem;
		}
		else {
			//PRINT(("bracketed\n"));
			// previtem is bracketed
			// select range from lowSelection up to prevItem
			for (long i = lowSelection; i <= prevItem; i++) {
				//PRINT(("selecting %d\n",i));
				Select(i);
			}
			// deselect range from prevItem+1 to highSelection
			for (long i = prevItem+1; i <= highSelection; i++) {
				//PRINT(("deselecting %d\n",i));
				DeSelect(i);
			}
			highSelection = prevItem;
		}
		snooze(20 * 1000.0);
		GetMouse(&where, &buttons, TRUE);
		if (buttons) {
			// do select auto scrolling
			long max = CountItems();
			float minScroll, maxScroll;
			fScroll->GetRange(&minScroll,&maxScroll);
			float smallStep, bigStep;
			fScroll->GetSteps(&smallStep,&bigStep);
			long pivot;
			if (prevItem == highSelection)
				pivot = lowSelection;
			else
				pivot = highSelection;
			while(buttons)	{
				GetMouse(&where, &buttons, TRUE);
				curItem = where.y / ItemHeight();
				
				if (where.y < Bounds().top &&
					fScroll->Value() > minScroll) {
					// scroll up
					fScroll->SetValue(fScroll->Value()-smallStep);
					//
					Window()->UpdateIfNeeded();
				}
				else if (where.y > Bounds().bottom && 
						fScroll->Value() < maxScroll) {
					// scroll down
					fScroll->SetValue(fScroll->Value()+smallStep);
					Window()->UpdateIfNeeded();
				}
				if (curItem < 0)
					curItem = 0;
				else if (curItem >= max)
					curItem = max-1;
				if (prevItem != curItem) {
					// only bother if in range
					
					// autoscroll here
					
					if (curItem > pivot) {
						if (prevItem < curItem) {
							if (prevItem < pivot) {
								// deal with previous item less than pivot
								while(prevItem < pivot)
									DeSelect(prevItem++);
							}
							while(prevItem < curItem)
								Select(++prevItem);
						}
						else if (prevItem > curItem)
							while(prevItem > curItem)
								DeSelect(prevItem--);
					}
					else {  // curItem <= pivot
						if (prevItem > curItem) {
							if (prevItem > pivot) {
								while(prevItem > pivot)
									DeSelect(prevItem--);
							}
							while(prevItem > curItem)
								Select(--prevItem);
						}
						else if (prevItem < curItem)
							while (prevItem < curItem)
								DeSelect(prevItem++);
					}
				}
				snooze(20 * 1000.0);
			}
			if (pivot < prevItem) {
				lowSelection = pivot;
				highSelection = prevItem;
			}
			else {
				lowSelection = prevItem;
				highSelection = pivot;
			}
		}
	
	}
	else {
		// no modifiers
		// deselect all but clicked item
		if (lowSelection >= 0) {
			for (long i = lowSelection; i <= highSelection; i++)
			{
				// there is a crash in here!!!
				if (i != prevItem) DeSelect(i);
			}
		}
		// select clicked item
		Select(prevItem);
		// handle double-click
		if (lowSelection == highSelection &&
			lowSelection == prevItem &&
			current->FindInt32("clicks") == 2) {
				
			while(buttons)	{
				GetMouse(&where, &buttons, TRUE);
				snooze(40*1000);
			}
			if ( ((int)(where.y / ItemHeight())) == prevItem)
				Invoke(prevItem);
		}
		// check for drag (reorders items)
		else {
			lowSelection = highSelection = prevItem;
		
			// check for drag
			snooze(200 * 1000.0);
			GetMouse(&where, &buttons, TRUE);
			if (buttons) {
				long max = CountItems();
				float minScroll;
				float maxScroll;
				fScroll->GetRange(&minScroll,&maxScroll);
				float smallStep, bigStep;
				fScroll->GetSteps(&smallStep,&bigStep);
				long lastItem = prevItem;
				// for the moment, don't worry about moving to
				// position 0
				while(buttons)	{
					GetMouse(&where, &buttons, TRUE);
					curItem = where.y / ItemHeight();
					
					if (where.y < Bounds().top &&
						fScroll->Value() > minScroll) {
						fScroll->SetValue(fScroll->Value()-smallStep);
						Window()->UpdateIfNeeded();
					}
					else if (where.y > Bounds().bottom && 
							fScroll->Value() < maxScroll) {
						fScroll->SetValue(fScroll->Value()+smallStep);
						Window()->UpdateIfNeeded();
					}
					
					if (where.y < 0) {
						curItem = -1;
						//PRINT(("curItem is -1 and lastItem is %d\n",lastItem));
					} 
					else if (curItem >= max)
						curItem = max-1;
					if (lastItem < curItem) {
						while(lastItem < curItem) {
							if (lastItem != prevItem) {
								SetPenSize(2.0);
								SetHighColor(255,255,255);
								float v = (lastItem+1)*ItemHeight()-1.0;
								StrokeLine(BPoint(2,v),BPoint(Bounds().right,v));
							}
							lastItem++;
						}
						if (curItem != prevItem) {
							SetHighColor(180,180,180);
							float v = (curItem+1)*ItemHeight()-1.0;
							StrokeLine(BPoint(2,v),BPoint(Bounds().right,v));
						}
						SetHighColor(0,0,0);
					}
					else if (lastItem > curItem) {
						while(lastItem > curItem) {
							if (lastItem != prevItem) {
								SetPenSize(2.0);
								SetHighColor(255,255,255);
								float v = (lastItem+1)*ItemHeight()-1.0;
								StrokeLine(BPoint(2,v),BPoint(Bounds().right,v));
							}
							lastItem--;
						}
						if (curItem != prevItem) {
							SetHighColor(180,180,180);
							float v = (curItem+1)*ItemHeight()-1.0;
							StrokeLine(BPoint(2,v),BPoint(Bounds().right,v));
						}
						SetHighColor(0,0,0);
					}
					snooze(30 * 1000.0);
				}
				if (prevItem != curItem) {
					ReorderItem(prevItem,curItem);
					SelectionSet();
					MakeFocus(true);
					return;
				}
			}
		}
	}
	//PRINT(("low is %d     high is %d\n",lowSelection,highSelection));
	MakeFocus(true);
	SelectionSet();
}

void FListView::ReorderItem(long prevItem, long curItem)
{
	if (prevItem == curItem)
		return;
	
	ListItem *li = RemoveItem(prevItem);
	if (curItem < prevItem)
		curItem++;
	
	fList->AddItem(li,curItem);
	Invalidate();
	lowSelection = highSelection = curItem;
}

void FListView::ScrollListTo(float val)
{
	fScroll->SetValue(val);
}

void FListView::KeyDown(const char *byte, long byteCount)
{
	//PRINT(("list view got key down\n"));
	long count;
	switch (*byte) {
		case B_DOWN_ARROW:
			count = CountItems()-1;
			if (lowSelection < count) {
				/*** if (lowSelection != -1) { **/
				/********
				BMessage *cmsg = Looper()->CurrentMessage();
				if ((cmsg->FindLong("modifiers") & B_SHIFT_KEY) &&
						fMultipleSelect)
				{
					if (highSelection < count) {
						long sel = lowSelection;
						highSelection++;
						// make sure selection is contiguous
						while(++sel <= highSelection) {
							if (!IsItemSelected(sel))
								Select(sel);
						}
					}
				}
				else {
				*********/
				if (lowSelection != -1) {
					DeSelect(lowSelection);
						while(highSelection > lowSelection)
							DeSelect(highSelection--);
				}
				lowSelection++;
				highSelection = lowSelection;
				Select(lowSelection);
				// }
				// }
				// auto scrolling
				if ((highSelection+1)*ItemHeight() > Bounds().bottom) {
					// scroll down
					fScroll->SetValue(fScroll->Value()+ItemHeight());
					Window()->UpdateIfNeeded();	
				}
				SelectionSet();
			}
			break;
		case B_UP_ARROW:
			if (lowSelection > 0) {
				DeSelect(lowSelection);
				while(highSelection > lowSelection)
					DeSelect(highSelection--);
				lowSelection--;
				if ((lowSelection-1)*ItemHeight() < Bounds().top) {
					// scroll up
					fScroll->SetValue(fScroll->Value()-ItemHeight());
					Window()->UpdateIfNeeded();
				}
				Select(lowSelection);
				highSelection = lowSelection;
				SelectionSet();
			}
			else if (lowSelection == -1 && CountItems()) {
				Select(++lowSelection);
				highSelection = lowSelection;
				SelectionSet();
			}
			break;
		case B_RETURN:
			if ((lowSelection == highSelection) && ( lowSelection != -1))
				Invoke(lowSelection);
			break;
		case B_PAGE_DOWN:
			//PRINT(("got pg down"));
			fScroll->SetValue(fScroll->Value()+(Bounds().Height() - ItemHeight()));
			break;
		case B_PAGE_UP:
			//PRINT(("got pg up"));
			fScroll->SetValue(fScroll->Value()-(Bounds().Height() - ItemHeight()));
			break;
		default:
			BView::KeyDown(byte,byteCount);
	}
}

/****************** Column Stuff ****************************************/
// ## Hacky ##

void FListView::AddColumn(long tag, float width,const char *title)
{
	ColumnInfo *cInfo = new ColumnInfo;
	cInfo->tag = tag;
	cInfo->width = width;
	cInfo->title = title;
	fColumns->AddItem(cInfo);
	
	dataWidth += width;
	PRINT(("data width is %f cur width %f\n",dataWidth,width));
}

void FListView::SetShowColumns(bool flag)
{
	fShowColumns = flag;
}

long FListView::ColumnCount()
{
	return fColumns->CountItems();
}

/************************************************************************/

void FListView::SetMultipleSelect(bool flag)
{
	fMultipleSelect = flag;
}

void FListView::SetList(RList<ListItem *> *theList)
{
	fList = theList;
}

void FListView::Update()
{
	FixupScrollBar();
	Invalidate();
}

void FListView::Select(long index)
{
	ListItem *li = fList->ItemAt(index);
	if (li->selected == FALSE) {
		li->selected = TRUE;
		// just invert the rect
		DrawItem(Bounds(),index);
	}
}

void FListView::DeSelect(long index)
{
	ListItem *li = fList->ItemAt(index);
	if (li->selected == TRUE) {
		li->selected = FALSE;
		BRect iFrame = ItemFrame(index);
		FillRect(iFrame,B_SOLID_LOW);
		DrawItem(Bounds(),index,&iFrame);
	}
}

void FListView::ToggleSelect(long index)
{
	ListItem *li = fList->ItemAt(index);
	li->selected = !li->selected;
	BRect iFrame = ItemFrame(index);
	if (!li->selected) {
		FillRect(iFrame,B_SOLID_LOW);
	}
	DrawItem(Bounds(),index,&iFrame);
}

bool FListView::IsItemSelected(long index)
{
	return fList->ItemAt(index)->selected;
}

void FListView::Invoke(long index)
{
	index;
	// invoke the current selection
	// do messaging stuff	
}

void FListView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	// ## HACK ##
	iHeight = 12+BaselineOffset();

	BScrollView *scrView = cast_as(Parent(),BScrollView);
	ASSERT(scrView);

	fScroll = scrView->ScrollBar(B_VERTICAL);
	hScroll = scrView->ScrollBar(B_HORIZONTAL);	
	FixupScrollBar();
	//FixupHScrollBar();
	
	if (fShowColumns) {
		// assumes in a scrolling list view!
		Parent()->Parent()->AddChild(SetUpColumnView());	
	}
	SetViewColor(255,255,255);
}

ColLabelView *FListView::SetUpColumnView()
{
	// add a view above the current one
	BRect x = Parent()->Frame();
	
	x.bottom = x.top - 3;
	x.top = x.bottom - ItemHeight()*1.2;
	x.right++;
	
	ColLabelView *clabel = new ColLabelView(x,"collabel",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FRAME_EVENTS);
		
	clabel->SetColumnList(fColumns);

	return clabel;
}

/* DR9 obsolete
void FListView::SetFontName(const char *name)
{
	BView::SetFontName(name);
	
	font_info fInfo;
	GetFontInfo(&fInfo);
	iHeight = fInfo.size+BaselineOffset();
}
*/

/* DR9 obsolete
void FListView::SetFontSize(float pts)
{
	BView::SetFontSize(pts);
	
	font_info fInfo;
	GetFontInfo(&fInfo);
	iHeight = fInfo.size+BaselineOffset();
}
*/

/* DR9 obsolete
void FListView::SetFontShear(float degrees)
{
	BView::SetFontShear(degrees);
	
	font_info fInfo;
	GetFontInfo(&fInfo);
	iHeight = fInfo.size+BaselineOffset()+1;
	// Update();
}
*/

void FListView::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
	FixupScrollBar();
	//FixupHScrollBar();
}

void FListView::FixupScrollBar()
{
	long dataHeight = CountItems()*ItemHeight()-1;
	long viewHeight = Bounds().Height();
	
	if (dataHeight < viewHeight)
		dataHeight = viewHeight;
	
	fScroll->SetRange(0,dataHeight-viewHeight);
	fScroll->SetProportion((float)viewHeight/(float)dataHeight);
	fScroll->SetValue(Bounds().top);
	fScroll->SetSteps(ItemHeight()/2.0,viewHeight);
}

void FListView::FixupHScrollBar()
{
	float dWidth = dataWidth;
	float viewWidth = Bounds().Width();
	if (dWidth < viewWidth)
		dWidth = viewWidth;

	hScroll->SetRange(0,dWidth-viewWidth);
	hScroll->SetProportion((float)viewWidth/(float)dWidth);
	hScroll->SetSteps(8.0,viewWidth);
}

void FListView::DrawItem(BRect updateRect, long index, BRect *itemFrame)
{
	updateRect;
	itemFrame;
	
	StringItem *li = (StringItem *)fList->ItemAt(index);
	BRect iFrame = ItemFrame(index);
	
	//PRINT(("drawing index %d\n",index));
	
	FillRect(iFrame, B_SOLID_LOW);
	MovePenTo(10.0,iFrame.bottom-BaselineOffset()+1);
	DrawString(li->string);
	if (li && li->selected) {
		HighlightItem(TRUE,index);
	}
}

void FListView::InvalidateItem(long index)
{
	Invalidate(ItemFrame(index));
}

void FListView::HighlightItem(bool on, long index)
{
	if (on)
		InvertRect(ItemFrame(index));
}

float FListView::ItemHeight()
{
	return iHeight;
}

BRect FListView::ItemFrame(long index)
{
	BRect frame;
	frame.left = 0;
	frame.right = 8192;
	frame.top = index*ItemHeight();
	frame.bottom = frame.top+ItemHeight()-1;
	// compensate for focus bar
	/**
	if (IsFocus())
		frame.left += 2;
	**/	
	//PRINT(("index %d has frame ",index));
	return frame;
}

long FListView::IndexOf(ListItem *item)
{
	long i;
	for (i = fList->CountItems()-1; i >= 0; i--) {
		if (fList->ItemAt(i) == item)
			break;
	}
	return i;
}

void FListView::SelectionSet()
{
}
