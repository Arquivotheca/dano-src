// GroupsView.h

#include "GroupList.h"

#ifndef _GROUPSVIEW_H
#define _GROUPSVIEW_H


class GroupsView : public BView
{
public:
	GroupsView(BRect fr,GroupList *theList);
	~GroupsView();
	
	virtual void AttachedToWindow();

private:
	GroupList *fList;
};
#endif
