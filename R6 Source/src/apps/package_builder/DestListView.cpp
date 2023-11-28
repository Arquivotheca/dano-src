#include <Be.h>
// DestListView.cpp

#include "DestListView.h"
#include "DestinationList.h"
#include "PackMessages.h"
#include "DestinationWindow.h"

#include "PackWindow.h"

#include "Util.h"
#include "MyDebug.h"


DestListView::DestListView(BRect r,DestList *newList,PackWindow *_pw)
	: FListView(r,"destlisting",B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  destList(newList),
	  pw(_pw)
{
	// convenience copy of current selection
	curSelection = -1;
	SetMultipleSelect(FALSE);

	// worry about locks!
	SetList((RList<ListItem *> *)(destList));
	
	destList->lock.Lock();
	long count = destList->CountItems();
	for (long i = 0; i < count; i++) {
		destList->ItemAt(i)->selected = FALSE;
	}
	destList->lock.Unlock();
}

void DestListView::AttachedToWindow()
{
	FListView::AttachedToWindow();
	destList->lock.Lock();

	// select first item (if any) in the list
	long count = destList->CountItems();
	if (count > 0) {
		destList->ItemAt(0)->selected = TRUE;
		lowSelection = highSelection = 0;
	}
	destList->lock.Unlock();
	SelectionSet();
}


DestListView::~DestListView()
{
}

void DestListView::DrawItem(BRect updt, long item,BRect *iFrame)
{
	updt;
	BRect r;
	// locked in the Draw method
	
	if (iFrame)
		r = *iFrame;
	else
		r = ItemFrame(item);
		
	DestItem *it = destList->ItemAt(item);
	
	BView::SetFont(be_fixed_font);
	FillRect(r, B_SOLID_LOW);
	MovePenTo(10.0,r.bottom-BaselineOffset());

	DrawString(it->path);
	
	if (it->selected == TRUE)
		InvertRect(r);
}

void DestListView::Draw(BRect up)
{
	destList->lock.Lock();
	FListView::Draw(up);
	destList->lock.Unlock();
}

void DestListView::SelectionSet()
{
	if (lowSelection == highSelection) {
		BMessage selMsg(M_ITEMS_SELECTED);
		selMsg.AddInt32("selection",lowSelection);
		Looper()->PostMessage(&selMsg,this);
	}
}

// move these to the edit view

void DestListView::NameDest()
{
	if (curSelection != -1) {
		BTextControl *tc = (BTextControl *)(Window()->FindView("pathname"));
		
		destList->lock.Lock();
		DestItem *it = destList->ItemAt(curSelection);
		free(it->path);
		it->path = strdup(tc->Text());
	
		destList->lock.Unlock();
		
		Update();
		
		BMessage msg(M_NAME_DEST);
		
		msg.AddString("destname",tc->Text());
		msg.AddInt32("index",curSelection);
	
		((DestinationWindow *)Window())->settingsViewMessenger.SendMessage(&msg);
		((DestinationWindow *)Window())->windowDirty = FALSE;
	}
}

void DestListView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_INFO_MODIFIED:
			// this message is called when a text control is modified
			// (ie typed in for the first time)
		
			((DestinationWindow *)Window())->windowDirty = TRUE;
			PRINT(("window is DIRTY\n"));
			break;
		case M_NEW_DEST: {
			// a new path destination is being created
			// window dirty updated in other window
			
			destList->lock.Lock();
			
			const char *newPath;
		
			if (msg->HasString("text"))
				newPath = msg->FindString("text");
			else {
				newPath = "newpathname";
				msg->AddString("text",newPath);
			}
			
			// try it without strdup
			
			destList->AddItem(new DestItem(newPath));
			
			if (curSelection != -1) {
				DeSelect(curSelection);
			}
			long newSel = destList->CountItems()-1;
			Select(newSel);
			curSelection = lowSelection = highSelection = newSel;
			// get the name widget selected
			SelectionSet();
			Looper()->DetachCurrentMessage();
	
			destList->lock.Unlock();
			Update();
			
			// send message to parent window as long as we were not sent
			// this message from a separate dialog box
			if (! msg->HasBool("fromdialog"))
				((DestinationWindow *)Window())->settingsViewMessenger.SendMessage(msg);
				
			break;
		}
		case M_ITEMS_SELECTED: {
			// set the index number
			curSelection = msg->FindInt32("selection");
						
			msg->AddPointer("destlist",destList);
	
			Looper()->DetachCurrentMessage();
			Looper()->PostMessage(msg,Parent());
			break;
		}
		case M_NEW_FINDITEM: {
			// a new find (query) destination is being created
			// window dirty updated in other window
			
			const char *newName;
			
			newName = msg->FindString("text");
			if (!newName)
				newName = "New Query";
			
			const char *sig = msg->FindString("signature");
			long size = msg->FindInt32("size");
			
			PRINT(("NEW sig %s  size %d\n",sig,size));
				
			destList->lock.Lock();
			destList->AddItem(new FindItem(newName,size,sig));
			
			if (curSelection != -1) {
				DeSelect(curSelection);
			}
			long newSel = destList->CountItems()-1;
			destList->lock.Unlock();
			
			Select(newSel);
			curSelection = lowSelection = highSelection = newSel;
			SelectionSet();
			

			//// send shit on to the window
			if ( ! msg->HasBool("fromapp")) {
				if (!msg->HasString("text"))
					msg->AddString("text",newName);
				msg->what = M_NEW_DEST;
				Looper()->DetachCurrentMessage();
				((DestinationWindow *)Window())->settingsViewMessenger.SendMessage(msg);
			}
			break;
		}
		case M_REMOVE_ITEMS:
			// currently only does single removals
			// window dirty updated in other window
			
			if (curSelection != -1) {
				destList->lock.Lock();
				delete destList->RemoveIndex(curSelection);
				destList->lock.Unlock();
				
				BMessage rmMsg(M_REMOVE_DEST);
				rmMsg.AddInt32("index",curSelection);

				curSelection = lowSelection = highSelection = -1;
				SelectionSet();
				Update();
				((DestinationWindow *)Window())->settingsViewMessenger.SendMessage(&rmMsg);		
			}
			break;
		case M_NAME_DEST:
			// pathname or query name modified
			// window dirty updated in other window
		
			NameDest();
			break;
		case M_SET_TYPE: {
			// set the signature code for a find item
			if (curSelection == -1)
				break;
			
			destList->lock.Lock();
			DestItem *it = destList->ItemAt(curSelection);
			FindItem *fItem = cast_as(it,FindItem);
			//FindItem *fItem = (FindItem *)it;
			
			if (!fItem)
				break;
						
			// window dirty needs updating
			pw->attribDirty = TRUE;
			
			fItem->SetSignature(msg->FindString("signature"));
			destList->lock.Unlock();
			break;
		}
		case M_SET_SIZE: {
			if (curSelection == -1)
				break;
			
			destList->lock.Lock();
			DestItem *it = destList->ItemAt(curSelection);
			FindItem *fItem = cast_as(it,FindItem);
			
			if (!fItem)
				break;	
			fItem->size = msg->FindInt32("size");
			destList->lock.Unlock();
			
			// window dirty needs updating
			pw->attribDirty = TRUE;
			
			break;
		}
		default:
			break;
	}
}

void DestListView::ReorderItem(long oldIndex, long newIndex)
{
	destList->lock.Lock();
	FListView::ReorderItem(oldIndex,newIndex);
	destList->lock.Unlock();

	// fix up the menu
	BMessage ord(M_REORDER_DEST);
	ord.AddInt32("oldindex",oldIndex);
	ord.AddInt32("newindex",newIndex);
	
	((DestinationWindow *)Window())->settingsViewMessenger.SendMessage(&ord);
}
