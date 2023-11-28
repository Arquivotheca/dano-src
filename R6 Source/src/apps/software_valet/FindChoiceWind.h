#ifndef _FINDCHOICEWIND_H_
#define _FINDCHOICEWIND_H_

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <EntryList.h>
#include <Entry.h>
#include <Rect.h>

// FindChoiceWind.h

// for StringItem
#include "PatchInfoWind.h"

class FindItem;

class FindChoiceWind : public BWindow
{
public:
	FindChoiceWind(	BEntryList *_query,
					bool *_continueInstall,
				 	entry_ref *_foundItem,
				 	FindItem	*_findItem);
				 	
	virtual bool	QuitRequested();
			void	Go();
			void	SelectionSet(const char *path);
private:
	friend class	FindChoiceView;
	
	BEntryList		*query;
	bool			*continueInstall;
	
	entry_ref		*foundItem;
	FindItem		*findItem;
	BButton			*cntButton;
};


class FindChoiceView : public BView
{
public:
	FindChoiceView(BRect frame);
	virtual void AttachedToWindow();
};


////////////////////////////////////////////////////////////////

class FindListView : public SimpleListView
{
public:
	FindListView(BRect r,RList<StringItem *> *_itemList);
	~FindListView();

	virtual void DrawItem(BRect updateRect,
						long index,
						BRect *itemFrame = NULL);
	virtual void SelectionSet();
	virtual void Invoke(long index);
	
private:
	RList<StringItem *>		*foundList;
};

#endif
