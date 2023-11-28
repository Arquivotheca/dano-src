// IGroupsListView.cpp
#include "IGroupsListView.h"
#include "IGroupList.h"
#include "InstallMessages.h"
#include "MyDebug.h"

#include <Looper.h>
#include <Message.h>

#if (CHECKBOX_LIST)
bool IGroupsListView::picsInited = FALSE;
#endif

IGroupsListView::IGroupsListView(BRect r,
								GroupList *_groupList,
								BHandler *_fTarget)
	:	SimpleListView(r,"grouplisting",
						B_FOLLOW_ALL,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  groupList(_groupList),
	  fTarget(_fTarget)
{
	SetItemList((RList<ListItem *> *)(groupList->viewList));
	
	SetMultipleSelect(TRUE);
	SetDragReorder(FALSE);
	
	SelectNoItems();
}


void IGroupsListView::AttachedToWindow()
{
	SimpleListView::AttachedToWindow();
	
	SetFont(be_plain_font);
	// SetFontSize(12);
	
#if (CHECKBOX_LIST)
	if (!picsInited) {
		BCheckBox	*cb = new BCheckBox(BRect(0,0,13,13),
										B_EMPTY_STRING,
										B_EMPTY_STRING,
										new BMessage('????'));
		AddChild(cb);
		
		cb->BeginPicture(new BPicture());
		cb->Draw(cb->Bounds());
		cOffPict = cb->EndPicture();
		
		cb->SetValue(B_CONTROL_ON);
		cb->BeginPicture(new BPicture());
		cb->Draw(cb->Bounds());
		cOnPict = cb->EndPicture();
		
		RemoveChild(cb);
		
		picsInited = TRUE;
	}
	SetItemHeight(16);
	SetBaselineOffset(3.0);
#else
	SetItemHeight(14);
#endif
}

void IGroupsListView::DrawItem(BRect updt, long item, BRect *iFrame)
{
	updt;
	IndexItem *i = (IndexItem *)ItemAt(item);
	
	BRect r;
	if (iFrame)
		r = *iFrame;
	else
		r = ItemFrame(item);
	
	FillRect(r, B_SOLID_LOW);

	if (i->index == -1) {
		// separator item
		float y = r.bottom - ItemHeight()/2.0;
		SetHighColor(200,128,128);
		SetPenSize(2.0);
		StrokeLine( BPoint(r.left,y),BPoint(r.right,y));
		SetHighColor(0,0,0);
	}
	else {
	
#if (CHECKBOX_LIST)
		MovePenTo(5.0,r.top);
		if (i->selected == TRUE)
			DrawPicture(cOnPict);
		else
			DrawPicture(cOffPict);

		SetHighColor(0,0,0);		
		MovePenTo(24.0,r.bottom-BaselineOffset());
		DrawString(groupList->masterList->ItemAt(i->index)->name);
#else	
		MovePenTo(10.0,r.bottom-BaselineOffset());
		DrawString(groupList->masterList->ItemAt(i->index)->name);
		if (i->selected == TRUE)
			InvertRect(r);
		
#endif		
		// for debugging		
		// StrokeLine(r.LeftBottom(),r.RightBottom());
	}
}

void IGroupsListView::Invoke(long index)
{
	index;
}

#if (CHECKBOX_LIST)
void IGroupsListView::MouseDown(BPoint where)
{
	BMessage *msg = Looper()->CurrentMessage();
	
	long mod = msg->FindLong("modifiers");

	mod = mod | B_COMMAND_KEY;
	mod = mod & ~B_SHIFT_KEY;
	msg->ReplaceLong("modifiers",mod);
	SimpleListView::MouseDown(where);
}
#endif	


bool IGroupsListView::IsItemDisabled(long index)
{
	IndexItem *i = (IndexItem *)ItemAt(index);
	if (i->index == -1)
		return TRUE;
		
	return FALSE;
}

void IGroupsListView::SelectionSet()
{
	ulong theGroups = 0;
	BMessage selMsg(M_ITEMS_SELECTED);
	if (LowSelection() >= 0) {		
		// check for single selection
		if (LowSelection() == HighSelection()) {
			IndexItem *ii = groupList->viewList->ItemAt(LowSelection());
			if (ii->index != -1) {
				// do all the info shit
				GroupItem *gi = groupList->masterList->ItemAt(ii->index);
				
				selMsg.AddString("name",gi->name);
				selMsg.AddString("description",gi->description);
				selMsg.AddData("helptext",
								B_POINTER_TYPE,
								&(gi->helpText),sizeof(char *));
				if (gi->helpText)
					PRINT(("HELPTXT %s\n",gi->helpText));
			}
		}
		else {
			selMsg.AddString("name",B_EMPTY_STRING);
			selMsg.AddString("description",B_EMPTY_STRING);
		}
		// set mask for multiple selections
		for (long i = LowSelection(); i <= HighSelection(); i++) {
			IndexItem *ii = groupList->viewList->ItemAt(i);
			if (ii->index != -1) {
				theGroups |= 1 << (ii->index);
			}
		}
	}
	else {
		selMsg.AddString("name",B_EMPTY_STRING);
		selMsg.AddString("description",B_EMPTY_STRING);
	}
	selMsg.AddInt32("groups",theGroups);
	
	fTarget->Looper()->PostMessage(&selMsg,fTarget);
}

/*
void IGroupsListView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {

		default:
			break;
	}
}
*/
