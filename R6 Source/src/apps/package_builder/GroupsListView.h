// GroupsListView.h
#include "FListView.h"

#include "GroupList.h"


#ifndef _GROUPSLISTVIEW_H
#define _GROUPSLISTVIEW_H

enum {
	M_DO_GROUPSHELP = 'DGrH'
};

class GroupsWindow;

class GroupsListView : public FListView
{
public:
				GroupsListView(BRect r,GroupList *newList);
				~GroupsListView();
				
virtual void	AttachedToWindow();

virtual void	Draw(BRect update);
virtual void	DrawItem(BRect updateRect, long index, BRect *iFrame = NULL);
virtual void 	SelectionSet();
	// display name
	
virtual void 	ReorderItem(long oldIndex, long newIndex);
	// fix up assoc lists
	
	// deletion
virtual void 	MessageReceived(BMessage *msg);

void			NameGroup();
void			SetDescription();
void			SetHelp();

	GroupList*	groupList;
		long	curSelection;
private:
	
	GroupsWindow	*theWindow;
};

#endif
