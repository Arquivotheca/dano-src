// IGroupsListView.h
#ifndef _IGROUPSLISTVIEW_H_
#define _IGROUPSLISTVIEW_H_

#include "SimpleListView.h"
#include "IGroupList.h"

#define CHECKBOX_LIST 0

class IGroupsListView : public SimpleListView
{
public:
	IGroupsListView(	BRect r,
						GroupList *newList,
						BHandler *_fTarget);

#if (CHECKBOX_LIST)
virtual void		MouseDown(BPoint where);
#endif

virtual void		AttachedToWindow();
virtual void		DrawItem(BRect updt,
							long item,
							BRect *itemFrame = NULL);
virtual void		SelectionSet();
virtual void		Invoke(long index);
// virtual void		MessageReceived(BMessage *msg);
virtual bool		IsItemDisabled(long index);

GroupList	*groupList;
private:
	BHandler		*fTarget;

#if (CHECKBOX_LIST)
	static	bool	picsInited;
	BPicture		*cOffPict;
	BPicture		*cOnPict;
#endif
};

#endif
