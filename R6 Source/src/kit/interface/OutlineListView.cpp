// (c) 1997 Be Incorporated
//

#include <OutlineListView.h>

#include <Debug.h>
#include <OS.h>
#include <StopWatch.h>
#include <View.h>
#include <Window.h>
#include <archive_defs.h>
#include <stdio.h>

BOutlineListView::BOutlineListView(BRect frame, const char *name, 
	list_view_type type, uint32 resizeMask, uint32 flags)
	:	BListView(frame, name, type, resizeMask, flags)
{
}

BOutlineListView::~BOutlineListView()
{
}

BOutlineListView::BOutlineListView(BMessage *data)
	:	BListView(data)
{
	int32 index = 0;
	BMessage archive;
	while(data->FindMessage(S_FULL_LIST_ITEMS, index++, &archive) == B_OK) {
		BArchivable	*obj = instantiate_object(&archive);
		BListItem *item = dynamic_cast<BListItem *>(obj);
		if (item) {
			AddItem(item);
		} else {
			delete obj;
		}
	}
}

status_t 
BOutlineListView::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);
		// skip ListView

	data->AddInt32(S_LIST_VIEW_TYPE, fListType);
	int32 index = 0;
	BListItem *item;
	if (deep) {
		while((item = (BListItem *)fullList.ItemAt(index++)) != NULL) {
			BMessage archive;
			status_t err = item->Archive(&archive);
			if (!err)
				data->AddMessage(S_FULL_LIST_ITEMS, &archive);
		}
	}

	BMessage *msg = Message();
	if (msg)
		data->AddMessage(S_MESSAGE, msg);

	if (fSelectMessage) 
		data->AddMessage(S_ALT_MESSAGE, fSelectMessage);

	return B_NO_ERROR;
}

BArchivable *
BOutlineListView::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BOutlineListView"))
		return NULL;
	return new BOutlineListView(data);
}

struct TrackItemRectParams {
	BListItem *item;
	BRect itemRect;
	BOutlineListView *listView;
	bool collapsed;
};

typedef void (*TrackFunction)(void *);

void
BOutlineListView::TrackInLatchItem(void *castToParams)
{
	TrackItemRectParams *params = (TrackItemRectParams *)castToParams;
	params->listView->DrawLatch(params->itemRect, params->item->OutlineLevel(),
		params->collapsed, true, true);
}

void
BOutlineListView::TrackOutLatchItem(void *castToParams)
{
	TrackItemRectParams *params = (TrackItemRectParams *)castToParams;
	params->listView->DrawLatch(params->itemRect, params->item->OutlineLevel(),
		params->collapsed, true, false);
}

static bool
TrackMouse(BView *view, BRect rect, TrackFunction trackInFunction, 
	TrackFunction trackOutFunction, void *params)
{
	bool in = false;
	for (;;) {
		BPoint where;
		ulong buttons;

		view->GetMouse(&where, &buttons, true);
		if (rect.Contains(where) != in) {
			if (in)
				trackOutFunction(params);
			else
				trackInFunction(params);
			in = !in;
		}
		if (!buttons)
			break;
		snooze(40000);
	}
	
	// don't quit in the tracked out draw/whatever state
	// so that callers do not have to any cleanup work in the
	// mistracked case; call trackInFor them
	if (!in)
		trackInFunction(params);
	
	return in;
}

void 
BOutlineListView::MouseDown(BPoint where)
{
	int32 index = BListView::IndexOf(where);
	
	if (index >= 0) {
		BRect frame = ItemFrame(index);
		BListItem *item = ItemAt(index);
		BRect latchRect = LatchRect(frame, item->OutlineLevel());
		
		if (latchRect.Contains(where) && item->HasSubitems()) {
			
			TrackItemRectParams params;
			params.item = item;
			params.itemRect = frame;
			params.collapsed = !item->IsExpanded();
			params.listView = this;
			
			bool result = TrackMouse(this, latchRect, TrackInLatchItem, 
				TrackOutLatchItem, &params);
			
			Invalidate(frame);
			if (result) {
				if (item->IsExpanded())
					Collapse(item);
				else
					Expand(item);
			}


			// no matter what we ended up doing as a result of tracking
			// the latch, we are done dealing with the list for this
			// mouse down event
			return;
		}
	}

	BListView::MouseDown(where);
}

void 
BOutlineListView::KeyDown(const char *bytes, int32 numBytes)
{
	int32 curSelected = fFirstSelected;
	BListItem *item = NULL;

	BMessage *msg = Window()->CurrentMessage();
	uint32 mods;
	msg->FindInt32("modifiers", (long*)&mods);
	bool control = (mods & B_CONTROL_KEY) != 0;
	int32 level;

	switch(bytes[0]) {
		case B_LEFT_ARROW:
			if (curSelected >= 0)
				item = ItemAt(curSelected);

			if (!item)
				break;

			if (control) {
				
				if (!item->HasSubitems())
					item = Superitem(item);
					
				if (!item)
					break;
					
				Collapse(item);
			}
			else {
				item = Superitem(item);

				if (!item)
					break;

				if (item->IsEnabled()) {
					Select(IndexOf(item));
					ScrollToSelection();
					break;
				}
			}
			break;
		case B_RIGHT_ARROW:
			if (curSelected >= 0)
				item = ItemAt(curSelected);

			if (!item || !item->HasSubitems())
				break;

			if (control)
				Expand(item);
			else if (item->IsExpanded()) {
				Select(BListView::IndexOf(item) + 1);
				ScrollToSelection();
			}
			break;
		case B_UP_ARROW:
			if (!control) {
				BListView::KeyDown(bytes, numBytes);
				break;
			}
			
			if (curSelected >= 0)
				level = ItemAt(curSelected)->OutlineLevel();
			else
				break;

			// select the previous item with the same hierarchy level
			for (int32 index = curSelected - 1; index >= 0; index--) {
				BListItem *item = ItemAt(index);
				if ((int32)item->OutlineLevel() <= level && item->IsEnabled()) {
					Select(index);
					ScrollToSelection();
					break;
				}
			}
					
			break;
			
		case B_DOWN_ARROW:
			{
				if (!control) {
					BListView::KeyDown(bytes, numBytes);
					break;
				}
	
				if (curSelected >= 0)
					level = ItemAt(curSelected)->OutlineLevel();
				else
					break;
	
				// select the next item with the same hierarchy level
				int32 count = CountItems();
				for (int32 index = curSelected + 1; index < count; index++) {
					BListItem *item = ItemAt(index);
					if ((int32)item->OutlineLevel() <= level && item->IsEnabled()) {
						Select(index);
						ScrollToSelection();
						break;
					}
				}
	
			}
			break;
	
		default:
			BListView::KeyDown(bytes, numBytes);
			break;
	}
}

BListItem *
BOutlineListView::FullListItemAt(int32 fullListIndex) const
{
	return (BListItem *)fullList.ItemAt(fullListIndex);
}

int32		
BOutlineListView::FullListIndexOf(BListItem *item) const
{
	return fullList.IndexOf(item);
}

int32		
BOutlineListView::FullListIndexOf(BPoint point) const
{
	int32 index = IndexOf(point);
	return FullListIndex(index);
}

BListItem *
BOutlineListView::FullListFirstItem() const
{
	return (BListItem *)fullList.FirstItem();
}

BListItem *
BOutlineListView::FullListLastItem() const
{
	return (BListItem *)fullList.LastItem();
}

bool		
BOutlineListView::FullListHasItem(BListItem *item) const
{
	return fullList.HasItem(item);
}

int32		
BOutlineListView::FullListCountItems() const
{
	return fullList.CountItems();
}

void		
BOutlineListView::MakeEmpty()
{
	fullList.MakeEmpty();
	BListView::MakeEmpty();
}

bool		
BOutlineListView::FullListIsEmpty() const
{
	return fullList.IsEmpty();
}

void		
BOutlineListView::FullListDoForEach(bool (*func)(BListItem *))
{
	int32 count = FullListCountItems();
	for (int32 i = 0; i < count; i++)
		// if func returns true then abort the iteration
		if ((*func)((BListItem *) fullList.ItemAt(i)))
			break;
}

void		
BOutlineListView::FullListDoForEach(bool (*func)(BListItem *, void *),
	void *params)
{
	int32 count = FullListCountItems();
	for (int32 i = 0; i < count; i++)
		// if func returns true then abort the iteration
		if ((*func)((BListItem *) fullList.ItemAt(i), params))
			break;
}

int32		
BOutlineListView::FullListIndex(int32 index) const
{
	BListItem *item = BListView::ItemAt(index);
	if (!item)
		return -1;
	return fullList.IndexOf(item);
}

int32		
BOutlineListView::ListViewIndex(int32 index) const
{
	BListItem *item = (BListItem *)fullList.ItemAt(index);
	if (!item)
		return -1;
	return BListView::IndexOf(item);
}

int32
BOutlineListView::FullListCurrentSelection(int32 index) const
{
	return FullListIndex(CurrentSelection(index));
}

BListItem *
BOutlineListView::Superitem(const BListItem *forItem)
{
	int32 count = FullListCountItems();
	int32 level = forItem->OutlineLevel();

	if (level <= 0)
		// item already has the outermost level, bail
		return NULL;
		
	BListItem *result = NULL;
	
	for (int32 index = 0; index < count; index++) {
		BListItem *item = FullListItemAt(index);
		if (item == forItem) {
			// we have reached forItem, terminate search
			
			// since level of forItem was more than 0, we must have result 
			// at this point otherwise we would have terminated early
			ASSERT(result);
			return result;
		}
		if ((int32)item->OutlineLevel() == level - 1)
			result = item;
	}
	return result;
}

bool 
BOutlineListView::AddItem(BListItem *item)
{
	return AddItem(item, FullListCountItems());
}

bool 
BOutlineListView::AddUnder(BListItem *item, BListItem *underItem)
{
	int32 itemIndex = fullList.IndexOf((void *)underItem);
	// add after the underItem into the main list
	bool result = fullList.AddItem(item, itemIndex + 1);
	// set the level according to the superitem
	item->fLevel = underItem->OutlineLevel() + 1;
	// update the underItem
	underItem->fHasSubitems = true;
	// figure out if item will be visible after insertion or not
	if (!underItem->IsItemVisible() || !underItem->IsExpanded())
		item->SetItemVisible(false);
	else {
		item->SetItemVisible(true);
		// item visible add to actual list view 
		int32 listViewItemIndex = BListView::IndexOf(underItem);
		BListView::AddItem(item, listViewItemIndex + 1);

		// invalidate the triangle of the item above
		BRect latchRect(LatchRect(ItemFrame(listViewItemIndex),
			underItem->OutlineLevel()));
		Invalidate(latchRect);
	}
	return result;
}

bool
BOutlineListView::AddItem(BListItem *newItem, int32 fullListIndex)
{
	// update the hasSubitems flag of the previous item
	int32 itemCount = FullListCountItems();
	
	if (fullListIndex < 0)
		fullListIndex = 0;
	else if (fullListIndex > itemCount)
		// can add one past last item
		fullListIndex = itemCount;

	int32 afterIndex = -1;
	BListItem *afterItem = 0;
	int32 level = newItem->OutlineLevel();
	bool visible = true;
	
	// if visible, find afterIndex
	// update superitems fHasSubitems field

	for (int32 index = fullListIndex - 1; index >= 0; index--) {
		afterItem = FullListItemAt(index);

		if (level > (int32)afterItem->OutlineLevel()) {
			// we hit our superitem
			// and we are becoming it's first subitem			
			afterItem->fHasSubitems = true;

			// make sure afterItem is not expanded if superitem isn't either
			if (!afterItem->IsItemVisible() || !afterItem->IsExpanded()) {
				visible = false;
				break;
			}

			if (visible && afterIndex < 0)
				afterIndex = IndexOf(afterItem);
			break;
		}
		if (visible && afterIndex < 0)
			afterIndex = IndexOf(afterItem);
	}

	// add to the master list
	bool result = fullList.AddItem(newItem, fullListIndex);

	// if expanded, add to the view right after the
	// the previous visible item
	newItem->SetItemVisible(visible);
	if (visible) {
		BListView::AddItem(newItem, afterIndex + 1);

		if (afterItem && Window()) {
			BRect latchRect(LatchRect(ItemFrame(afterIndex),
				afterItem->OutlineLevel()));
			Invalidate(latchRect);
		}
	}

	return result;
}

// Finds what the superitem would be for an item at the specified index and
// outline level.
BListItem *
BOutlineListView::SuperitemForIndex(int32 fullListIndex, int32 level)
{
	BListItem *ret = NULL;
	if (level > 0) {
		BListItem *temp;
		while (fullListIndex >= 0) {
			temp = FullListItemAt(fullListIndex);
			if ((int32)temp->OutlineLevel() == level - 1) {
				ret = temp;
				break;
			}
			fullListIndex--;
		}
	}
	return ret;
}

int32
BOutlineListView::FindPreviousVisibleIndex(int32 fullListIndex)
{
	int32 ret = -1;
	int32 i;
	BListItem *item;
	fullListIndex--;
	while (fullListIndex >= 0) {
		item = FullListItemAt(fullListIndex);
		i = fList.IndexOf(item);
		if (i >= 0) {
			ret = i;
			break;
		}
		fullListIndex--;	
	}
	return ret;
}

bool
BOutlineListView::AddList(BList *newItems)
{
	return AddList(newItems, FullListCountItems());
}

bool
BOutlineListView::AddList(BList *newItems, int32 fullListIndex)
{
	int32 newItemCount = newItems->CountItems();
	if (newItemCount <= 0)
		return false;

	int32 itemCount = FullListCountItems();
	if (fullListIndex < 0) {
		fullListIndex = 0;
	} else if (fullListIndex > itemCount) {
		fullListIndex = itemCount;
	}

	int32 listViewIndex = FindPreviousVisibleIndex(fullListIndex);
	int32 lowestInvalidateIndex = fList.CountItems();
	if (listViewIndex >= 0) {
		listViewIndex++;
		lowestInvalidateIndex = listViewIndex;
	} else {
		listViewIndex = 0;
	}	

	if (itemCount > 0) {
		BListItem *firstNewItem = (BListItem *)newItems->FirstItem();
		ASSERT(firstNewItem != NULL);
		// update the item immediately before the insertion if this insertion
		// reparents that item's children to one of the inserted items
		if (fullListIndex > 0) {
			BListItem *beforeItem = FullListItemAt(fullListIndex - 1);
			ASSERT(beforeItem != NULL);
			if (beforeItem->OutlineLevel() >= firstNewItem->OutlineLevel()) {
				beforeItem->fHasSubitems = false;
				int32 beforeViewIndex = ListViewIndex(fullListIndex - 1);
				if (beforeItem->IsItemVisible() && beforeViewIndex >= 0
					&& beforeViewIndex < lowestInvalidateIndex)
				{
					lowestInvalidateIndex = beforeViewIndex;
				}
			}
		}
		// update the last inserted item's HasSubitems state, if necessary
		BListItem *lastNewItem = (BListItem *)newItems->LastItem();
		BListItem *afterItem = ItemAt(fullListIndex);
		if ((afterItem != NULL) && (lastNewItem != NULL)) {
			if (lastNewItem->OutlineLevel() == afterItem->OutlineLevel() - 1) {
				lastNewItem->fHasSubitems = true;
			}
		}
	}
	
	BFont font;
	if (Window())
		GetFont(&font);

	int32 showingCount = 0;
	int32 insertIndex = fullListIndex;
	// insert new list into full list
	fullList.AddList(newItems, insertIndex);
	
	for (int32 index = 0; index < newItemCount; index++, insertIndex++) {
		BListItem *item = (BListItem *)newItems->ItemAt(index);
		BListItem *superItem = SuperitemForIndex(insertIndex, item->OutlineLevel());
		bool showing = true;
		if (superItem != NULL) {
			bool superItemInvalidate = false;
			if (!superItem->fHasSubitems) {
				superItem->fHasSubitems = true;
				superItemInvalidate = true;	
			}
			// update lowestInvalidateIndex flag, if needed
			if (superItemInvalidate) {
				int32 superItemIndex = fList.IndexOf(superItem);
				if (superItemIndex >= 0 && superItemIndex < lowestInvalidateIndex) {
					lowestInvalidateIndex = superItemIndex;
				}
			}
			showing = (superItem->IsItemVisible() && superItem->IsExpanded());
		} else { // item has no superitem	
			if (listViewIndex < lowestInvalidateIndex) {
				lowestInvalidateIndex = listViewIndex;
			}	
		}
		item->Update(this, &font);
		item->SetItemVisible(showing);
		if (showing) {
			showingCount++;
			// update selection fields
			if (fFirstSelected != -1 && listViewIndex <= fFirstSelected) {
				fFirstSelected++;
			}
			if (fLastSelected != -1 && listViewIndex <= fLastSelected) {
				fLastSelected++;
			}
			fList.AddItem(item, listViewIndex++); // add to visible list			
		}
	}
	
	if (Window() && (showingCount > 0)) {
		FixupScrollBar();
		InvalidateFrom((lowestInvalidateIndex > 0) ? lowestInvalidateIndex : 0);
	}

	return true;
}

BListItem *
BOutlineListView::RemoveOne(int32 fullListIndex)
{
	BListItem *result = (BListItem *)fullList.RemoveItem(fullListIndex);
	if (result && result->IsItemVisible()) {
		// do not call BListView::RemoveItem(BListItem *) since
		// that would cause recursion through the virtual
		// calls
		ASSERT(IndexOf(result) >= 0);
		BListView::RemoveItem(IndexOf(result));
	}
	return result;
}

BListItem *
BOutlineListView::RemoveCommon(int32 fullListIndex)
{
	BListItem *result = RemoveOne(fullListIndex);
	if (result) {
		if (result->fHasSubitems) {
			// we removed a superitem, remove all it's subitems
			int32 level = result->OutlineLevel();
			BListItem *item;
			for (;;) {
				// XXX: memory leak here?  the subitems are never deleted?
				item = (BListItem *)fullList.ItemAt(fullListIndex);
				if (!item || (int32)item->OutlineLevel() <= level)
					break;
				if (!RemoveOne(fullListIndex))
					break;
			}
		}
		BListItem *prev = FullListItemAt(fullListIndex - 1);
		if (prev && prev->OutlineLevel() == result->OutlineLevel() - 1) {
			// prev is result's superitem
			BListItem *next = FullListItemAt(fullListIndex);
			if (next == NULL || next->OutlineLevel() != result->OutlineLevel()) {
				// we removed this superitem's last subitem, turn off its fHasSubitems flag
				prev->fHasSubitems = false;
				if (prev->IsItemVisible()) {
					InvalidateItem(fList.IndexOf(prev));
				}
			}
		}
	}
	return result;
}

void
BOutlineListView::FullListSortItems(int (*compareFunc)(const BListItem *,
	const BListItem *))
{
	SortItemsUnder(0, false, compareFunc);
}

#define BRAINDEAD_SORT
//#define HEAP_SORT

#ifdef HEAP_SORT
static void
heapup(BOutlineListView *list, BListItem *underItem, uint32 n, uint32 i,
	int (*compareFunc)(const BListItem *, const BListItem *))
{
	for (;;) {
		int32 j = i * 2 + 1;
		if (j >= n)
			break;
		int32 k = j + 1;
		BListItem *jp = list->ItemUnderAt(underItem, true, j);
		BListItem *kp = list->ItemUnderAt(underItem, true, k);
		ASSERT(jp && kp);
		if (k < n && (compareFunc)(jp, kp) < 0) {
			j = k;
			kp = jp;
		}

		BListItem *ip = list->ItemUnderAt(underItem, true, i);
		ASSERT(ip);
		
		if ((compareFunc)(ip, jp) >= 0)
			break;

		list->SwapItems(list->FullListIndexOf(ip), list->FullListIndexOf(jp));
		i = j;
	}
}
#endif

void
BOutlineListView::SortItemsUnder(BListItem *underItem, bool oneLevelOnly,
	int (*compareFunc)(const BListItem *, const BListItem *))
{
	int32 count = CountItemsUnder(underItem, true);
	if (count < 2)
		return;

#ifdef BRAINDEAD_SORT

	for (int32 i = 0; i < count; i++) {
		for (int32 j = 0; j < count - 1; j++) {
			BListItem *item1 = ItemUnderAt(underItem, true, j);
			BListItem *item2 = ItemUnderAt(underItem, true, j + 1);

			if (compareFunc(item1, item2) > 0) {
				SwapItems(FullListIndexOf(item1), FullListIndexOf(item2));
			}
		}
	}

#else
 #ifdef HEAP_SORT
		PRINT(("heap sort - countItems under %d, full count %d\n",
			count, FullListCountItems()));
		for (uint32 index = count / 2 - 1; index >= 1; index--)
			heapup(this, underItem, count, index, compareFunc);

		for (int32 index = count - 1; index >= 1; index--) {
			heapup(this, underItem, index, 0, compareFunc);
			SwapItems(FullListIndexOf(ItemUnderAt(underItem, true, 0)),
				FullListIndexOf(ItemUnderAt(underItem, true, index)));
		}
	
 #else		
	int32 r = count;
	int32 l = (r / 2) + 1;
	
	BListItem *lp = ItemUnderAt(underItem, true, l - 1);
	BListItem *rp = ItemUnderAt(underItem, true, r - 1);

//	PRINT(("sort - getting item at %d, under %s, got %s\n", l - 1,
//		underItem ? dynamic_cast<BStringItem *>(underItem)->Text() : "-",
//		lp ? dynamic_cast<BStringItem *>(lp)->Text() : "-"));

	for (;;) {
		if (l > 1) {
			l--;
			lp = ItemUnderAt(underItem, true, l - 1);
		} else {
			int32 fullListIndexL = FullListIndexOf(lp);
			int32 fullListIndexR = FullListIndexOf(rp);
			
			PRINT(("-swapping %d %s %s and %d %s %s\n",
				fullListIndexL,
				dynamic_cast<BStringItem *>(FullListItemAt(fullListIndexL))->Text(),
				dynamic_cast<BStringItem *>(lp)->Text(),
				fullListIndexR,
				dynamic_cast<BStringItem *>(FullListItemAt(fullListIndexR))->Text(),
				dynamic_cast<BStringItem *>(rp)->Text()
				));
			
			SwapItems(FullListIndexOf(lp), FullListIndexOf(rp));
			
			if (--r == 1)
				break;
				
			rp = ItemUnderAt(underItem, true, r - 1);
		}
		int32 j = l;
		BListItem *jp = ItemUnderAt(underItem, true, j - 1);
		
		while (j*2 <= r) {
			int32 i = j;
			BListItem *ip = jp;

			j *= 2;
			jp = ItemUnderAt(underItem, true, j - 1);
			
			if (j < r) {
				BListItem *kp = ItemUnderAt(underItem, true, j);
				
				ASSERT(kp);
				ASSERT(jp);
				if ((compareFunc)(jp, kp) < 0) {
					j++;
					jp = kp;
				}
			}
			ASSERT(ip);
			ASSERT(jp);
			if ((compareFunc)(ip, jp) < 0) {
				int32 fullListIndexL = FullListIndexOf(ip);
				int32 fullListIndexR = FullListIndexOf(jp);
				PRINT(("swapping %d %s %s and %d %s %s\n",
					fullListIndexL,
					dynamic_cast<BStringItem *>(FullListItemAt(fullListIndexL))->Text(),
					dynamic_cast<BStringItem *>(ip)->Text(),
					fullListIndexR,
					dynamic_cast<BStringItem *>(FullListItemAt(fullListIndexR))->Text(),
					dynamic_cast<BStringItem *>(jp)->Text()
					));

				SwapItems(FullListIndexOf(ip), FullListIndexOf(jp));	
			}
			else
				break;
		}
	}

 #endif
#endif

	if (oneLevelOnly)
		return;
	
	for (int32 index = 0; index < count; index ++) {
		// descend into subhierarchies and sort them
		BListItem *item = ItemUnderAt(underItem, true, index);
		if (item->HasSubitems()) {

//			PRINT(("---------------sort - descending into %s, index %d\n",
//				dynamic_cast<BStringItem *>(item)->Text(), index));

			SortItemsUnder(item, oneLevelOnly, compareFunc);
		}
	}
}

BListItem *
BOutlineListView::EachItemUnder(BListItem *underItem, bool oneLevelOnly,
	BListItem *(*func)(BListItem *, void *), void *passThru)
{
	int32 count = FullListCountItems();
	if (!count)
		return 0;

	int32 underItemIndex;
	int32 underItemLevel;

	if (!underItem) {
		underItemIndex = -1;
		underItemLevel = -1;
	} else {
		underItemLevel = underItem->OutlineLevel();
		underItemIndex = FullListIndexOf(underItem);
	}

	for (int32 index = underItemIndex + 1; index < count; index++) {
		BListItem *item = FullListItemAt(index);
		ASSERT(item);
		int32 itemLevel = item->OutlineLevel();
		
		if (itemLevel <= underItemLevel)
			// next item after subitem with the same level, <underItem> doesn't
			// have that many subitems
			return 0;

		if (!oneLevelOnly || itemLevel == underItemLevel + 1) {
			// we are either in the recursive mode or at the first level
			// under <underItem>, where we want't to iterate
			BListItem *result = (func)(item, passThru);

			if (result)
				// terminate iteration early
				return result;
		}
		ASSERT(itemLevel > underItemLevel);
	}
	return 0;
}

int32 
BOutlineListView::CountItemsUnder(BListItem *underItem, bool oneLevelOnly) const
{
	int32 result = 0;
	int32 fullCount = FullListCountItems();
	if (!fullCount)
		return 0;

	if (underItem == 0 && !oneLevelOnly)
		return fullCount;
	
	int32 underItemIndex;
	int32 underItemLevel;

	if (!underItem) {
		underItemIndex = -1;
		underItemLevel = -1;
	} else {
		underItemLevel = underItem->OutlineLevel();
		underItemIndex = FullListIndexOf(underItem);
	}

	for (int32 index = underItemIndex + 1; index < fullCount; index++) {
		BListItem *item = FullListItemAt(index);
		ASSERT(item);
		int32 itemLevel = item->OutlineLevel();
		
		if (itemLevel <= underItemLevel)
			// next item after subitem with the same level, <underItem> doesn't
			// have that many subitems
			return result;

		if (!oneLevelOnly || itemLevel == underItemLevel + 1) 
			// we are either in the recursive mode or at the first level
			// under <underItem>, where we want't to iterate
			++result;

	}
	return result;
}

BListItem *
BOutlineListView::ItemUnderAt(BListItem *underItem, bool oneLevelOnly,
	int32 atIndex) const
{
	int32 count = FullListCountItems();
	if (!count)
		return 0;

	int32 underItemIndex;
	int32 underItemLevel;

	if (!underItem) {
		underItemIndex = -1;
		underItemLevel = -1;
	} else {
		underItemLevel = underItem->OutlineLevel();
		underItemIndex = FullListIndexOf(underItem);
	}

	for (int32 index = underItemIndex + 1; index < count; index++) {
		BListItem *item = FullListItemAt(index);
		ASSERT(item);
		int32 itemLevel = item->OutlineLevel();
		
		if (itemLevel <= underItemLevel)
			// next item after subitem with the same level, <underItem> doesn't
			// have that many subitems
			return 0;

		if (!oneLevelOnly || itemLevel == underItemLevel + 1) {
			// we are either in the recursive mode or at the first level
			// under <underItem>, where we want't to iterate


			if (atIndex == 0) 
				// done
				return item;

			// use atIndex as a counter
			atIndex--;
		}
		ASSERT(itemLevel > underItemLevel);
	}
	return 0;
}

bool
BOutlineListView::DoMiscellaneous(MiscCode code, MiscData * data)
{
	switch (code) {
		case B_REPLACE_OP:
			return OutlineReplaceItem(data->replace.index, data->replace.item);
		case B_MOVE_OP:
			return OutlineMoveItem(data->move.from, data->move.to);
		case B_SWAP_OP:
			return OutlineSwapItems(data->swap.a, data->swap.b);
		default:
			return BListView::DoMiscellaneous(code, data);
	}
	return false;	/*NOTREACHED*/
}


bool
BOutlineListView::OutlineReplaceItem(int32 index, BListItem *item)
{
	if (index < 0 || index >= fullList.CountItems())
		return false;
	BListItem * old = (BListItem *)fullList.ItemAt(index);
	if (!old) return false;
	/* it would mess up the list if we didnt' copy these members of */
	/* the old item into the new item */
	item->fLevel = old->fLevel;
	item->fEnabled = old->fEnabled;
	item->fSelected = old->fSelected;
	item->fExpanded = old->fExpanded;
	item->fHasSubitems = old->fHasSubitems;
	fullList.ReplaceItem(index, item);
	if (old->IsItemVisible()) {
		MiscData data;
		data.replace.index = fList.IndexOf(old);
		data.replace.item = item;
		return BListView::DoMiscellaneous(B_REPLACE_OP, &data);
	}
	return true;
}


bool
BOutlineListView::OutlineMoveItem(int32 from, int32 to)
{
	/* only allow valid full indexes */
	BListItem * aItem = (BListItem *)fullList.ItemAt(from);
	BListItem * bItem = (BListItem *)fullList.ItemAt(to);
	if (!aItem || (to < 0))
		return false;
	if (!bItem)
		to = fullList.CountItems();

	/* only allow moves on the same level */
	int srcCount = 1, srcLevel = aItem->fLevel;
	int dstLevel = bItem ? bItem->fLevel : srcLevel;
	BListItem *cItem = NULL;
	if (to > 0) {
		cItem = (BListItem *)fullList.ItemAt(to-1);
		/* can't move to end because there is no intermediate step item */
		if (srcLevel > (int32)cItem->fLevel+1)
			return false;
	}
	if (srcLevel < dstLevel)
		return false;

	/* count items in the place */
	BListItem *temp;
	while ((temp = (BListItem *)fullList.ItemAt(srcCount+from)) != NULL)
		if ((int32)temp->fLevel <= srcLevel)
			break;
		else
			srcCount++;

	if (from < to)
		CommonMoveItems(from, srcCount, to - srcCount);
	else
		CommonMoveItems(from, srcCount, to);
		

	return true;
}


bool
BOutlineListView::OutlineSwapItems(int32 a, int32 b)
{
	/* only allow valid full indicies */
	BListItem * aItem = (BListItem *)fullList.ItemAt(a);
	BListItem * bItem = (BListItem *)fullList.ItemAt(b);
	if (!aItem || !bItem)
		return false;

	/* only allow swaps on the same level */
	int32 srcCount=1, srcLevel=aItem->fLevel;
	int32 dstCount=1, dstLevel=bItem->fLevel;
	if (srcLevel != dstLevel)
		return false;

	/* count items in one place */
	BListItem *temp;
	while ((temp = (BListItem *)fullList.ItemAt(srcCount+a)) != NULL)
		if ((int32)temp->fLevel <= srcLevel)
			break;
		else
			srcCount++;

	/* then count items in the other */
	while ((temp = (BListItem *)fullList.ItemAt(dstCount+b)) != NULL)
		if ((int32)temp->fLevel <= dstLevel)
			break;
		else
			dstCount++;

	/* the swap is actually two moves */
	if (a < b) {
		CommonMoveItems(b, dstCount, a);
		// should optimize here if a and b are adjacent
		CommonMoveItems(a + dstCount, srcCount, b - srcCount + dstCount);
	} else {
		CommonMoveItems(a, srcCount, b);
		// should optimize here if a and b are adjacent
		CommonMoveItems(b + srcCount, dstCount, a + srcCount - dstCount);
	}

	return true;
}


void
BOutlineListView::CommonMoveItems(int32 from, int32 count, int32 to)
{
	/* workhorse function used by MoveItems and SwapItems */
	/* we could probably do something much fancier with CopyBits() and */
	/* figuring out what moves where in blocks, but that's left as a later optimization */
	ASSERT(from>=0 && count>0 && from+count<=fullList.CountItems()
		&& to>=0 && to <=fullList.CountItems());
	if (from == to)
		return;
		
	if (count == 1) {
		// optimization for the common case of single item moves
		BListItem *tmp = RemoveOne(from);
		AddItem(tmp, to);
	} else {
		BList middleStorage(count);

		for (int ix=0; ix < count; ix++) 
			middleStorage.AddItem(RemoveOne(from));
	
		for (int ix=0; ix < count; ix++) 
			AddItem((BListItem *)middleStorage.ItemAt(ix), to + ix);

		middleStorage.MakeEmpty();
	}
}


bool 
BOutlineListView::RemoveItem(BListItem *item)
{
	return RemoveCommon(FullListIndexOf(item)) != 0;
}

BListItem *
BOutlineListView::RemoveItem(int32 fullListIndex)
{
	return RemoveCommon(fullListIndex);
}

bool
BOutlineListView::RemoveItems(int32 fullListIndex, int32 count)
{
	bool result = false;

	for (int32 index = fullListIndex; index < fullListIndex + count; index++)
		result |= (RemoveItem(index) != NULL);
	
	return result;
}


void 
BOutlineListView::Expand(BListItem *item)
{
	ExpandOrCollapse(item, true);
}

void 
BOutlineListView::Collapse(BListItem *item)
{
	ExpandOrCollapse(item, false);
}

void 
BOutlineListView::ExpandOrCollapse(BListItem *underItem, bool expand)
{
	if (underItem->IsExpanded() == expand)
		return;

	int32 count = fullList.CountItems();
	int32 index = fullList.IndexOf((void *)underItem);
	int32 collapsedIndex = BListView::IndexOf(underItem);
	int32 level = underItem->OutlineLevel() + 1;
	
	underItem->SetExpanded(expand);
	
	bool collapsedSelection = false;
	bool invalidate = false;
	int32 listViewItemIndex = collapsedIndex + 1;
	BFont font;
	if (Window())
		GetFont(&font);
	
	int32 dontExpandLevel = -1;

	for (++index; index < count; index++) {
		BListItem *item = (BListItem *)fullList.ItemAt(index);
		if ((int32)item->OutlineLevel() < level)
			break;

		if (expand) {
			
			// check if we have to reset the dontExpandLevel
			if (dontExpandLevel < 0
				|| (int32)item->OutlineLevel() < dontExpandLevel)
				dontExpandLevel = item->IsExpanded() ?
					-1 : (int32)item->OutlineLevel() + 1;

			if (dontExpandLevel < 0 
				|| (int32)item->OutlineLevel() < dontExpandLevel) {
				invalidate = true;
				if (!fList.AddItem(item, listViewItemIndex))
					return;
				item->SetItemVisible(true);

				if (Window())
					item->Update(this, &font);
						// give the item a chance to calculate it's height
				
				if ((fFirstSelected != -1) && (listViewItemIndex <= fFirstSelected))
					fFirstSelected++;

				if ((fLastSelected != -1) && (listViewItemIndex <= fLastSelected))
					fLastSelected++;
				
				listViewItemIndex++;
			}

		} else {

			int32 removedIndex = BListView::IndexOf(item);
			if (removedIndex >= 0) {
				if (item->IsSelected()) {
					collapsedSelection = true;
					Deselect(removedIndex);
				}
				if (fList.RemoveItem(removedIndex)) {
					invalidate = true;
					item->SetItemVisible(false);
					if ((fFirstSelected != -1) && (removedIndex < fFirstSelected))
						fFirstSelected--;

					if ((fLastSelected != -1) && (removedIndex < fLastSelected))
						fLastSelected--;

				}
			}
		}
	}
	if (Window())
		if (invalidate) {
			// invalidate all the items after collapsed/expanded item
			InvalidateFrom(collapsedIndex);
			FixupScrollBar();
		} else {
			// only invalidate the underItem
			BRect frame = ItemFrame(collapsedIndex);
			Invalidate(frame);
		}

	// if we hid the currently selected item, select the superitem instead
	if (collapsedSelection)
		Select(collapsedIndex);
}

const int32 kOutlineDent = 10;

BRect 
BOutlineListView::LatchRect(BRect frame, int32 level) const
{
	frame.left += level * kOutlineDent;
	frame.right = frame.left + kOutlineDent;
	
	return frame;
}

const rgb_color kLatchNormalColor = {150, 150, 150, 255};
const rgb_color kLatchHighlightColor = {0, 0, 0, 255};

void 
BOutlineListView::DrawLatch(BRect itemRect, int32 level, bool collapsed, 
	bool highlighted, bool misTracked)
{
	BRect rect(LatchRect(itemRect, level));
	rect.right = rect.left + 10;
	rect.bottom = rect.top + 10;

	const rgb_color color = HighColor();
	const rgb_color latchOutlineColor = {0, 0, 0, 255};
	const rgb_color latchMiddleColor = (highlighted && misTracked) 
		? kLatchHighlightColor : kLatchNormalColor;

	SetDrawingMode(B_OP_COPY);
	SetHighColor(latchOutlineColor);
	
	if (collapsed) {
		BeginLineArray(6);
		AddLine(BPoint(rect.left + 3, rect.top + 1), 
			BPoint(rect.left + 3, rect.bottom - 1), latchOutlineColor);
		AddLine(BPoint(rect.left + 3, rect.top + 1), 
			BPoint(rect.left + 7, rect.top + 5), latchOutlineColor);
		AddLine(BPoint(rect.left + 7, rect.top + 5), 
			BPoint(rect.left + 3, rect.bottom - 1), latchOutlineColor);
			
		AddLine(BPoint(rect.left + 4, rect.top + 3), 
			BPoint(rect.left + 4, rect.bottom - 3), latchMiddleColor);
		AddLine(BPoint(rect.left + 5, rect.top + 4), 
			BPoint(rect.left + 5, rect.bottom - 4), latchMiddleColor);
		AddLine(BPoint(rect.left + 6, rect.top + 5), 
			BPoint(rect.left + 6, rect.bottom - 5), latchMiddleColor);
		EndLineArray();
	} else {
		BeginLineArray(6);
		AddLine(BPoint(rect.left + 1, rect.top + 3), 
			BPoint(rect.right - 1, rect.top + 3), latchOutlineColor);
		AddLine(BPoint(rect.left + 1, rect.top + 3), 
			BPoint(rect.left + 5, rect.top + 7), latchOutlineColor);
		AddLine(BPoint(rect.left + 5, rect.top + 7), 
			BPoint(rect.right - 1, rect.top + 3), latchOutlineColor);

		AddLine(BPoint(rect.left + 3, rect.top + 4), 
			BPoint(rect.right - 3, rect.top + 4), latchMiddleColor);
		AddLine(BPoint(rect.left + 4, rect.top + 5), 
			BPoint(rect.right - 4, rect.top + 5), latchMiddleColor);
		AddLine(BPoint(rect.left + 5, rect.top + 6), 
			BPoint(rect.right - 5, rect.top + 6), latchMiddleColor);
		EndLineArray();
	}
	SetHighColor(color);
}

void 
BOutlineListView::DrawItem(BListItem *item, BRect cellRect, bool complete)
{
	if (item->HasSubitems()) 
		DrawLatch(cellRect, item->OutlineLevel(), !item->IsExpanded(), false,
			false);

	// get the right indent for the cell level
	BRect latchRect = LatchRect(cellRect, item->OutlineLevel());
	cellRect.left = latchRect.right + 1;

	// draw the cell
	item->DrawItem(this, cellRect, complete);
}

bool		
BOutlineListView::IsExpanded(int32 fullListIndex)
{
	BListItem *item = FullListItemAt(fullListIndex);
	return item != 0 && item->IsExpanded();
}

void
BOutlineListView::MouseUp(BPoint where)
{
	_inherited::MouseUp(where);
}

void
BOutlineListView::FrameMoved(BPoint new_position)
{
	_inherited::FrameMoved(new_position);
}

void
BOutlineListView::FrameResized(float new_width, float new_height)
{
	_inherited::FrameResized(new_width, new_height);
}

BHandler *
BOutlineListView::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return _inherited::ResolveSpecifier(msg, index, spec, form, prop);
}

status_t
BOutlineListView::GetSupportedSuites(BMessage *data)
{
	return _inherited::GetSupportedSuites(data);
}

void 
BOutlineListView::MessageReceived(BMessage *message)
{
	_inherited::MessageReceived(message);
}

status_t
BOutlineListView::Perform(perform_code d, void *arg)
{
	return _inherited::Perform(d, arg);
}

/*---------------------------------------------------------------*/

void BOutlineListView::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BListView::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BOutlineListView::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BListView::GetPreferredSize(width, height);
}

/*---------------------------------------------------------------*/

void BOutlineListView::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BListView::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void BOutlineListView::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BListView::AllAttached();
}

/*---------------------------------------------------------------*/

void BOutlineListView::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BListView::AllDetached();
}

/*---------------------------------------------------------------*/

void BOutlineListView::DetachedFromWindow()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BListView::DetachedFromWindow();
}

void
BOutlineListView::_ReservedOutlineListView1()
{
}

void
BOutlineListView::_ReservedOutlineListView2()
{
}

void
BOutlineListView::_ReservedOutlineListView3()
{
}

void
BOutlineListView::_ReservedOutlineListView4()
{
}

