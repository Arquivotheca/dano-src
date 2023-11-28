// FListView.h

#include "RList.h"

#ifndef _FLISTVIEW_H
#define _FLISTVIEW_H

class ListItem
{
public:
	ListItem() { selected = FALSE; }
	bool selected;
	virtual int Kind() { return 0; }
};

class StringItem : public ListItem
{
public:
	char *string;	
};

struct ColumnInfo {
	long tag;
	float width;
	const char *title;
};


class ColLabelView;


class FListView : public BView
{
public:
	FListView( BRect frame,
				const char *name,
				ulong resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
				ulong flags = B_WILL_DRAW | B_FRAME_EVENTS);
	virtual ~FListView();
		
	virtual void		Draw(BRect up);
	virtual void		MouseDown(BPoint where);
	virtual void		KeyDown(const char *, long);
	virtual void		MakeFocus(bool state = TRUE);
	virtual void		WindowActivated(bool state);
// fix index stuff here!!!
	virtual void		ReorderItem(long oldIndex, long newIndex);

// add a column to the column list
	virtual void		AddColumn(long tag, float width,const char *title);
//virtual void			AddColumnAtIndex(long tag, float width,const char *title,long index);
// return number of columns
	virtual long		ColumnCount();

// bogus
virtual ColLabelView	*SetUpColumnView();
		void			SetMultipleSelect(bool flag);
		void			SetShowColumns(bool flag);
		void			SetList(RList<ListItem *> *items);
		void			RemoveList();
		// call this when the list has been modified
		void			Update();
virtual	void			SelectionSet();

		void			ScrollListTo(float val);
		
virtual void			Invoke(long index);
virtual	void 			AttachedToWindow();
//virtual void			SetFontName(const char *name);
//virtual void			SetFontSize(float pointSize);
//virtual void			SetFontShear(float degrees);
virtual	void			FrameResized(float new_width, float new_height);

		long			lowSelection;
		long			highSelection;
		long			IndexOf(ListItem *item);
virtual void			InvalidateItem(long index);
inline	ListItem		*ItemAt(long index);
inline  long			CountItems();

protected:
		void			Select(long index);
		void			DeSelect(long index);
		void			ToggleSelect(long index);
		bool			IsItemSelected(long index);

virtual float			ItemHeight();

inline	bool			AddItem(ListItem *item);
inline	ListItem		*RemoveItem(long index);

RList<ListItem *> 		*fList;
RList<ColumnInfo *>		*fColumns;

virtual void			DrawItem(BRect updateRect, long index, BRect *iFrame = NULL);
virtual void			HighlightItem(bool on, long index);

BRect					ItemFrame(long index);
inline	float			BaselineOffset();
void					FixupScrollBar();
void					FixupHScrollBar();

private:
	bool				fShowColumns;
	BScrollBar			*fScroll;
	BScrollBar			*hScroll;
	long				selectionIndex;
	float				iHeight;
	float				dataWidth;
	bool				fMultipleSelect;
};

inline long FListView::CountItems()
{
	if (!fList) return 0;
	return fList->CountItems();
}

inline bool FListView::AddItem(ListItem *i)
{
	if (!fList) return FALSE;
	return fList->AddItem(i);
}

inline ListItem *FListView::ItemAt(long index)
{
	if (!fList) return NULL;
	return fList->ItemAt(index);
}

inline ListItem *FListView::RemoveItem(long index)
{
	if (!fList) return NULL;
	return fList->RemoveIndex(index);
}

inline float FListView::BaselineOffset()
{
	return 3.0;
}

#endif
