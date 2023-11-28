#ifndef _SIMPLELISTVIEW_H_
#define _SIMPLELISTVIEW_H_

// SimpleListView.h

#include "RList.h"
#include "SimpleListItem.h"

#include <View.h>

class SimpleListView : public BView
{
public:
	SimpleListView( BRect frame,
				const char *name,
				ulong resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				ulong flags = B_WILL_DRAW | B_FRAME_EVENTS);
	virtual ~SimpleListView();
		
virtual void			Draw(BRect up);
// draw the focus outline
virtual void			MakeFocus(bool state);
virtual void			WindowActivated(bool state);

virtual void			MouseDown(BPoint where);
virtual void			KeyDown(const char *bytes, int32 numBytes);
virtual	void 			AttachedToWindow();
virtual void			TargetedByScrollView(BScrollView *);
virtual void			ScrollTo(BPoint where);
	// ???
/*	
virtual void			SetFontName(const char *name);
virtual void			SetFontSize(float pointSize);
*/

	// implemented to update scroll bar
virtual	void			FrameResized(float new_width, float new_height);

	// set the list of items being displayed
	// (call update to redraw the view after changing this)
inline	void			SetItemList(RList<ListItem *> *items);
	// return the current list of items being displayed
inline	RList<ListItem *>	*ItemList();

	// call this when the data list has changed, fixes scroll bar
	// and redraws the view
		void			Update();
		
	// hook called when the selection has changed
virtual	void			SelectionSet() = 0;

virtual void			SelectItem(long index);
virtual void			SelectItem(ListItem *item);
virtual void			SelectAllItems();
virtual void			SelectNoItems();
inline	bool			ItemSelected(long index);
inline	bool			ItemSelected(ListItem *item);
	// allow or disallow multiple selections
	// via mouse and keybaord
	// (it is still possible to do multiple selection
	//  via calls to SelectItem(), SelectAllItems(), etc...
		void			SetMultipleSelect(bool flag);
		
	// allow or disallow the user to drag and reorder list
	// items
		void			SetDragReorder(bool flag);
		
	// hook called when an item in the list is double clicked
	// or return/enter is tapped
	// implement this by performing appropriate actions (such as sending a message)
virtual void			Invoke(long index) = 0;

	// use LowSelection() and HighSelection() to get
	// the range of the current selection, note that the selection
	// may be discontiguous, so you will need to iterate through
	// the item list from LowSelection to HighSelection to act on the
	// selected items
inline	long			LowSelection();
inline	long			HighSelection();

	// invalidate an item in the list so it gets redrawn
virtual void			InvalidateItem(long index);

	// standard operations on the current item list
		long			IndexOf(ListItem *item);		
inline	bool			AddItem(ListItem *item);
inline	ListItem		*RemoveItem(long index);
inline	ListItem		*ItemAt(long index);
inline  long			CountItems();

	void				FixupScrollBar();

protected:

	// implement this to draw each item, iFrame is passed along
	// if the item frame is already known (when called from Draw(), iFrame
	// can be precalculated to avoid a call to Bounds() which can slow down
	// drawing)
virtual void			DrawItem(BRect updateRect,
								long index,
								BRect *iFrame = NULL) = 0;

	// implement this to highlight drawn items
	// alrenatively highlighting can be done directly in the draw item method
virtual void			HighlightItem(bool on, long index, BRect *iFrame);
virtual bool			IsItemDisabled(long index);

	// the pixel height of list items
inline	float			ItemHeight();
		void			SetItemHeight(float value);
		BRect			ItemFrame(long index);

	// offset above the item rectangle to draw content at
inline	float			BaselineOffset();
		void			SetBaselineOffset(float off);

	// swap two items in the item list (for drag reordering)
void					ReorderItem(long oldIndex, long newIndex);

private:

	RList<ListItem *> 	*fList;
	
	// internal selected/deselect which redraw immediately
	// (for use in mouse polling loop)
	void				Select(long index);
	void				DeSelect(long index);
	void				ToggleSelect(long index);

	long				fLowSelection;
	long				fHighSelection;
	bool				fShowColumns;
	
	// horizontal scroll bar
	long				selectionIndex;
	float				iHeight;
	float				fBaselineOff;
	bool				fMultipleSelect;
	bool				fDragReorder;
	bool				fDrawFocus;
	BScrollView			*fScrollView;
	
	long				selPoint;
	long				selMark;
};

inline long SimpleListView::CountItems()
{
	if (!fList) return 0;
	return fList->CountItems();
}

inline float SimpleListView::ItemHeight()
{
	return iHeight;
}

inline bool SimpleListView::AddItem(ListItem *i)
{
	if (!fList) return FALSE;
	return fList->AddItem(i);
}

inline ListItem *SimpleListView::ItemAt(long index)
{
	if (!fList) return NULL;
	return fList->ItemAt(index);
}

inline ListItem *SimpleListView::RemoveItem(long index)
{
	if (!fList) return NULL;
	return fList->RemoveIndex(index);
}

inline void SimpleListView::SetItemList(RList<ListItem *> *theList)
{
	fList = theList;
}

inline RList<ListItem *> *SimpleListView::ItemList()
{
	return fList;
}

inline bool SimpleListView::ItemSelected(long index)
{
	return fList->ItemAt(index)->selected;
}

inline bool SimpleListView::ItemSelected(ListItem *li)
{
	return li->selected;
}

inline float SimpleListView::BaselineOffset()
{
	return fBaselineOff;
}

inline long  SimpleListView::LowSelection()
{
	return fLowSelection;
}

inline long  SimpleListView::HighSelection()
{
	return fHighSelection;
}

#endif
