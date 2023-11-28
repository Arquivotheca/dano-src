// DestListView.h

#include "FListView.h"

#include "DestinationList.h"

#ifndef _DESTLISTVIEW_H
#define _DESTLISTVIEW_H

class PackWindow;

class DestListView : public FListView
{
public:
				DestListView(BRect r,DestList *newList,PackWindow *pw);
				~DestListView();
				
virtual void	Draw(BRect update);
virtual void 	DrawItem(BRect update, long index,BRect *iFrame = NULL);
virtual void 	SelectionSet();
virtual void	AttachedToWindow();
	// display name
	
		void	NameDest();	
	
virtual void 	ReorderItem(long oldIndex, long newIndex);
	// fix up assoc lists
	
	// deletion
virtual void 	MessageReceived(BMessage *msg);

	DestList*	destList;
	long		curSelection;
	PackWindow	*pw;
};
#endif
