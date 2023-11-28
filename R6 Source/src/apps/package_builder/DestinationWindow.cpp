#include <Be.h>
#include <stdlib.h>
// DestinationWindow.cpp
#include "DestinationWindow.h"
#include "DestView.h"
#include "PackMessages.h"
#include "DestListView.h"

#include "Util.h"
#include "MyDebug.h"


DestinationWindow::DestinationWindow(const char *title,DestList *theList,PackWindow *parWin)
	: ChildWindow(BRect(0,0,240,270),title,B_TITLED_WINDOW,
			B_NOT_ZOOMABLE /*| B_NOT_RESIZABLE */,(BWindow *)parWin)
{
	Lock();

	// stagger	
	MoveBy(10,10);
	////////////////////////////////////////////////////
	//////       Do the main view

	BRect r = Bounds();
	r.top += 14;
	DestView *dv = new DestView(r,theList,parWin);
	
	AddChild(dv);
			
	////////////////////////////////////////////////////
	scrollList = FindView("destlisting");
	if (scrollList == NULL) {
		doError("Failed to find destination list view");
		exit(-1);
	}
	
	r = Bounds();
	r.bottom = r.top + 19;
	
	BMenuBar *mbar = new BMenuBar(r,"menubar");
	
	BMenu *menu = new BMenu("Destinations"); 
	BMenuItem *item;
	
	item = new BMenuItem("Add Pathname",new BMessage(M_NEW_DEST),'P');
	item->SetTarget(scrollList);
	menu->AddItem(item);
	
	BMessage *mmsg = new BMessage(M_NEW_FINDITEM);
	mmsg->AddString("signature","application/*");
	mmsg->AddInt32("size",0);
	
	item = new BMenuItem("Add Query",mmsg);
	item->SetTarget(scrollList);
	menu->AddItem(item);
	
	menu->AddSeparatorItem();
	
	item = new BMenuItem("Delete",new BMessage(M_REMOVE_ITEMS),'D');
	item->SetTarget(scrollList);
	menu->AddItem(item);
	
	mbar->AddItem(menu);
	AddChild(mbar);
	
	SetSizeLimits(Bounds().Width(),8192.0,150,8192.0);

	windowDirty = FALSE;

	Show();
	Unlock();
}

DestinationWindow::~DestinationWindow()
{
}

bool DestinationWindow::QuitRequested()
{
/*** I don't like this hack
	if (windowDirty) {
		// set shit for currently selected item
		//
		DestListView *listView = (DestListView *)FindView("destlisting");
		listView->NameDest();
	}
*****/
	return TRUE;
}

void DestinationWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_ITEMS_UPDATED:
			PRINT(("INVALIDATING scroll list\n"));
			scrollList->Invalidate();
			break;
		default:
			ChildWindow::MessageReceived(msg);
	}				
}
