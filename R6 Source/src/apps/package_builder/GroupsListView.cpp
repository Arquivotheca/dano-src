#include <Be.h>
// GroupsListView.cpp

#include "GroupsListView.h"
#include "GroupList.h"
#include "PackMessages.h"
#include "GroupsWindow.h"
#include "STextField.h"
// #include "GDescriptionView.h"

#include "Util.h"
#include "MyDebug.h"

GroupsListView::GroupsListView(BRect r,GroupList *newList)
	: FListView(r,"grouplisting",B_FOLLOW_ALL,B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
	SetMultipleSelect(FALSE);
	
	SetList((RList<ListItem *> *)(newList->viewList));
	groupList = newList;
	groupList->DeSelectAll();
	curSelection = -1;
}

GroupsListView::~GroupsListView()
{
}

void GroupsListView::AttachedToWindow()
{
	FListView::AttachedToWindow();
	
	theWindow = (GroupsWindow *)Window();
	
	if (CountItems() > 0) {
		Select(0);
		lowSelection = highSelection = 0;
		SelectionSet();
	}
}

void GroupsListView::DrawItem(BRect updt, long item,BRect *iFrame)
{
	updt;
	// locked in the Draw method
	IndexItem *i = groupList->viewList->ItemAt(item);
	
	BRect r;
	
	if (!iFrame)
		r = ItemFrame(item);
	else
		r = *iFrame;
	
	FillRect(r, B_SOLID_LOW);
	if (i->index == -1) {
		float y = r.bottom - ItemHeight()/2.0;
		SetHighColor(200,128,128);
		SetPenSize(2.0);
		StrokeLine( BPoint(r.left,y),BPoint(r.right,y));
		SetHighColor(0,0,0);
	}
	else {
		MovePenTo(10.0,r.bottom-BaselineOffset());
		DrawString(groupList->masterList->ItemAt(i->index)->name);
	}
	
	if (i->selected == TRUE)
		InvertRect(r);
}

void GroupsListView::Draw(BRect up)
{
	groupList->lock.Lock();
	FListView::Draw(up);
	groupList->lock.Unlock();
}

void GroupsListView::SelectionSet()
{
	if (lowSelection == highSelection) {
		BMessage selMsg(M_ITEMS_SELECTED);

		selMsg.AddInt32("selection",lowSelection);
		Looper()->PostMessage(&selMsg,this);
	}
}

void GroupsListView::NameGroup()
{
	if (curSelection != -1) {
		BTextControl *gn = (BTextControl *)(Window()->FindView("groupname"));
		
		groupList->SetName(curSelection,gn->Text());
		Update();
	
		BMessage msg(M_NAME_GROUP);
		msg.AddString("groupname",gn->Text());
		msg.AddInt32("viewindex",curSelection);
	
		theWindow->settingsViewMessenger.SendMessage(&msg);
	}
	theWindow->SetDirty(FALSE);
}

void GroupsListView::SetDescription()
{
	if (curSelection != -1) {
		STextField *tv = (STextField *)(Window()->FindView("description"));
		
		// locking
		groupList->SetDescription(curSelection,tv->Text());
	
	}
	theWindow->SetDirty(FALSE);
	theWindow->SetParentDirty();
}

void GroupsListView::SetHelp()
{
	if (curSelection >= 0) {
		STextField 	*tf = (STextField *)(Window()->FindView("grpshelp"));
		BControl	*gc = (BControl *)(Window()->FindView("dogrphelp"));
		
		groupList->SetHelp(curSelection,gc->Value() ? tf->Text() : NULL);
	}
	theWindow->SetDirty(FALSE);
	theWindow->SetParentDirty();
}

void GroupsListView::MessageReceived(BMessage *msg)
{
	// this is bad, change to class variable
	// static IndexItem *realItem = NULL;
	switch(msg->what) {
		case M_INFO_MODIFIED:
			((GroupsWindow *)Window())->SetDirty(TRUE);
			break;
		case M_NEW_GROUP: {
			if (!msg->HasBool("fromdialog")) {
				if (groupList->masterList->CountItems() >= 32) {
					doError("Sorry, no more groups can be added");
					break;
				}
				const char *newName;
				
				if (msg->HasString("text"))
					newName = msg->FindString("text");
				else {
					newName = "New Group";
					msg->AddString("text",newName);
				}
				groupList->AddGroup(newName);
				if (curSelection != -1) {
					DeSelect(curSelection);
				}
				long newSel = CountItems()-1;
			
				// draw false
				Select(newSel);
				curSelection = lowSelection = highSelection = newSel;
				SelectionSet();

				// only detach if we are sending on
				Looper()->DetachCurrentMessage();
				((GroupsWindow *)Window())->settingsViewMessenger.SendMessage(msg);
			}			
			
			Update();
			break;
		}	
		case M_NEW_SEPARATOR:
			groupList->AddSeparator();
			Update();
			((GroupsWindow *)Window())->settingsViewMessenger.SendMessage(M_NEW_SEPARATOR);
			break;
		case M_REMOVE_ITEMS: {
			// this is the harder one
			if (curSelection != -1 ) {
			
				// really ugly stuff in this routine
				long masterIndex = groupList->RemoveGroup(curSelection);

				BMessage rmMsg(M_REMOVE_GROUP);
				
				// need to know if separator item
				rmMsg.AddInt32("viewindex",curSelection);
				rmMsg.AddInt32("masterindex",masterIndex);
				((GroupsWindow *)Window())->settingsViewMessenger.SendMessage(&rmMsg);
				curSelection = lowSelection = highSelection = -1;
				Update();
				SelectionSet();
			}
			break;
		}
		case M_NAME_GROUP:
			NameGroup();
			break;
		case M_SET_DESCRIPTION:
			SetDescription();
			break;
		case M_DO_GROUPSHELP: {
			SetHelp();
			
			BControl	*gc;
			msg->FindPointer("source",reinterpret_cast<void **>(&gc));
			if (gc) {
				/* grow and shrink window */
				((GroupsWindow *)Window())->SetDisplayHelp(gc->Value());
			}
			break;
		}
		case M_ITEMS_SELECTED: {
			BTextControl *gn = 	(BTextControl *)(Window()->FindView("groupname"));
			STextField	*des = 	(STextField *)(Window()->FindView("description"));
			STextField	*htx =	(STextField *)(Window()->FindView("grpshelp"));
			BControl	*ghl =	(BControl *)(Window()->FindView("dogrphelp"));
			// BStringView *label = (BStringView *)(Window()->FindView("descriptionlabel"));
		
			bool disable = FALSE;
			curSelection = msg->FindInt32("selection");
		
			if (curSelection != -1) {
				PRINT(("low selection is %d\n",curSelection));
				IndexItem *ii = groupList->viewList->ItemAt(curSelection);
				// realItem = ii;
				if (ii->index == -1) {
					// separator item (what about saving and curSelection????)
					disable = TRUE;
				}
				else {
					GroupItem *item = groupList->masterList->ItemAt(ii->index);
					gn->SetEnabled(TRUE);
					gn->SetText(item->name);
					gn->Message()->ReplaceInt32("item",curSelection);
					
					char *t = item->description;
					des->SetText(t ? t : B_EMPTY_STRING);
					des->Message()->ReplaceInt32("item",curSelection);
					des->SetEnabled(TRUE);
					
					t = item->helpText;
					htx->SetText(t ? t : B_EMPTY_STRING);

					ghl->SetEnabled(TRUE);
					ghl->SetValue(item->doHelp);
					
					((GroupsWindow *)Window())->SetDisplayHelp(item->doHelp);			
				}
			}
			else {
				// realItem = NULL;
				disable = TRUE;
			}
			
			if (disable) {
				gn->SetText(B_EMPTY_STRING);
				gn->SetEnabled(FALSE);
				des->SetText(B_EMPTY_STRING);
				des->SetEnabled(FALSE);
				ghl->SetEnabled(FALSE);
				
				((GroupsWindow *)Window())->SetDisplayHelp(FALSE);
			}
			

			break;
		}
		default:
			break;
	}
}

void GroupsListView::ReorderItem(long oldIndex, long newIndex)
{
	// don't change the order of these operations!!!
	// otherwise GroupList::ReorderItems is working with already
	// reordered data!
	groupList->ReorderItem(oldIndex,newIndex);
	
	// reoder the view list
	FListView::ReorderItem(oldIndex,newIndex);
	
	BMessage ord(M_REORDER_GROUP);
	ord.AddInt32("oldindex",oldIndex);
	ord.AddInt32("newindex",newIndex);
	
	((GroupsWindow *)Window())->settingsViewMessenger.SendMessage(&ord);
}
