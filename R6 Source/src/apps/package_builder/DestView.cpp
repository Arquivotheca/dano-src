#include <Be.h>
#include <stdlib.h>
// GroupsView.cpp

#include "DestView.h"
#include "DestinationList.h"
#include "PackMessages.h"
#include "DestListView.h"

#include "Util.h"
#include "MyDebug.h"

enum {
	M_INFO_MODIFIED = 'InMd'
};


DestView::DestView(BRect frame,DestList *theList,PackWindow *_pw)
	: BView(frame,"destview",B_FOLLOW_ALL,B_WILL_DRAW),
	  pw(_pw)
{
	SetViewColor(light_gray_background);
	fList = theList;
}

void DestView::AttachedToWindow()
{
	BView::AttachedToWindow();
	////////////////////////////////////////////
	BRect r = Bounds();
	r.InsetBy(10,10);
	r.bottom = r.top + 140;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	DestListView *dlist = new DestListView(r,fList,pw);
		
	r.right += B_V_SCROLL_BAR_WIDTH;
	BScrollView *scrollV =
		new BScrollView("scroller",dlist, B_FOLLOW_ALL,0,FALSE,TRUE);

	AddChild(scrollV);

	// dlist->SetFont(be_plain_font);
	////////////////////////////////////////////

	r.top = r.bottom + 20;
	r.bottom = r.top + 20;
	
	nControl =
		new BTextControl(r,"pathname","Pathname",B_EMPTY_STRING,
		new BMessage(M_NAME_DEST),B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	nControl->SetTarget(FindView("destlisting"));
	
	nControl->SetModificationMessage(new BMessage(M_INFO_MODIFIED));
	nControl->SetDivider(80);
	nControl->SetEnabled(FALSE);
	AddChild(nControl);

	///////////////////////////////////////////
	
	r.top = r.bottom + 0;
	r.bottom = r.top + 100;
	r.left = Bounds().left;
	r.right = Bounds().right;
	
	findView = new EditFindView(r,dlist);
	AddChild(findView);
}

void DestView::AllAttached()
{
	///////////////////////////////////////////
	
	BView::AllAttached();

	ShowFindView(FALSE);

	// ugly
	// Window()->ResizeBy(0,-60);
}

void DestView::ShowFindView(bool state)
{
	if (state) {
		if (findView->IsHidden()) {
			uint32 resmode = nControl->ResizingMode();
			nControl->SetResizingMode(B_FOLLOW_NONE);
			FindView("scroller")->SetResizingMode(B_FOLLOW_NONE);
			Window()->ResizeBy(0,60);
			nControl->SetResizingMode(resmode);
			FindView("scroller")->SetResizingMode(B_FOLLOW_ALL);

			PRINT(("showing find view\n"));
			findView->Show();
			findView->SetEnabled(TRUE);
			
		}
	}
	else {
		if (!findView->IsHidden()) {
			uint32 resmode = nControl->ResizingMode();
			nControl->SetResizingMode(B_FOLLOW_NONE);
			FindView("scroller")->SetResizingMode(B_FOLLOW_NONE);
			Window()->ResizeBy(0,-60);
			nControl->SetResizingMode(resmode);
			FindView("scroller")->SetResizingMode(B_FOLLOW_ALL);

			PRINT(("hiding find view\n"));
			findView->Hide();
			findView->SetEnabled(FALSE);			
		}
	}
}

void DestView::MessageReceived(BMessage *msg)
{	
	switch(msg->what) {
		case M_ITEMS_SELECTED: {
			long curSelection;
			ListItem	 *curItem;
			DestList	 *destList;
			
			msg->FindPointer("destlist",reinterpret_cast<void **>(&destList));
			BTextControl *nameCtl = (BTextControl *)FindView("pathname");
			curSelection = msg->FindInt32("selection");
			
			ASSERT(destList);
			
			if (curSelection == -1) {
				PRINT(("cur slection is -1\n"));
			
				nameCtl->SetText(B_EMPTY_STRING);
				nameCtl->SetEnabled(FALSE);
				
				findView->SetEnabled(FALSE);
				break;
			}
			
			PRINT(("low selection is %d\n",curSelection));
			curItem = destList->ItemAt(curSelection);
			
			PRINT(("classname is %s\n",class_name(curItem)));
			
			if (typeid(*curItem) == typeid(DestItem)) {
				PRINT(("Type is DestItem\n"));
				ShowFindView(FALSE);
				
				nameCtl->SetEnabled(TRUE);
				nameCtl->SetLabel("Pathname");
				nameCtl->SetText(((DestItem *)curItem)->path);
			}
			else if (typeid(*curItem) == typeid(FindItem)) {
				PRINT(("Type is FindItem\n"));
				
				FindItem *fItem = (FindItem *)curItem;
				ShowFindView(TRUE);
				findView->SetEnabled(TRUE);	
				nameCtl->SetEnabled(TRUE);
				
				nameCtl->SetLabel("Query Name");
				nameCtl->SetText(fItem->path);
				
				// post messages to findView for type,creator,size
				msg->AddPointer("finditem",fItem);
				findView->MessageReceived(msg);
			}
			else if (typeid(*curItem) == typeid(ListItem)){
				PRINT(("Type is ListItem\n"));
			}
			else {
				PRINT(("unknown typeid\n"));
			}
			break;
		}
		default: {
			BView::MessageReceived(msg);
			break;
		}
	}
}

BTextView	*GetTextView(BView *view);

BTextView	*GetTextView(BView *view)
{
	BTextView *result;
	long i = 0;	
	BView *child;
	do {
		child = view->ChildAt(i++);
		result = cast_as(child,BTextView);
	} while(child && !result);

	return result;	
}


EditFindView::EditFindView(BRect frame,DestListView *_listing)
	: BBox(frame,"editfindview",
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW, B_NO_BORDER),
	  listing(_listing)
{
	BTextView *textView;
	BRect r = Bounds();
	r.InsetBy(10,10);
	
	float halfWidth = (r.Width() - 10)/ 2.0 - 5;
	
	r.bottom = r.top + 20; 
	//r.right = r.left + halfWidth;
	
	typeView = new BTextControl(r,"sig","Signature",B_EMPTY_STRING,
		new BMessage(M_SET_TYPE),B_FOLLOW_LEFT | B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(typeView);
	typeView->SetDivider(80);
	
	BRect rr = r;
	
	/**
	r.left = r.right + 10;
	r.right = r.left + halfWidth;
	
	creaView = new BTextControl(r,"crea","Creator",B_EMPTY_STRING,
		new BMessage(M_SET_CREATOR),B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(creaView);
	**/
	
	r = rr;
	
	r.top = r.bottom + 10;
	r.bottom = r.top + 20;
	r.right = r.left + 200;
	
	sizeView = new BTextControl(r,"size","Size",B_EMPTY_STRING,
		new BMessage(M_SET_SIZE),B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(sizeView);
	sizeView->SetDivider(80);
	
	textView = GetTextView(sizeView);
	// allow only numbers in this view
	
	SetViewColor(light_gray_background);
}

/****
// resize window to 10 pixels beyond our bottom
void EditFindView::Show()
{
	if (IsHidden()) {
		Show();
		BView *fixedView = Window()->FindView("scroller");
		if (fixedView) {
			int32 saveMode = fixedView->ResizingMode();
			fixedView->SetResizeMode(B_FOLLOW_NONE);
		}	
		BPoint rightBot = ConvertToScreen(Bounds().RightBottom());
		BRect fr = Window()->Frame();
		long h = rightBot.y - fr.top + 10;
		Window()->ResizeTo(fr.Width(),h);		
		if (fixedView)
			fixedView->SetResizeMode(saveMode);
		}
	}
}
*****/

/****
// resize window to our top
void EditFindView::Hide()
{
	if (!IsHidden()) {
		BView *fixedView = Window()->FindView("scroller");
		if (fixedView) {
			int32 saveMode = fixedView->ResizingMode();
			fixedView->SetResizeMode(B_FOLLOW_NONE);
		}	
		BPoint rightTop = ConvertToScreen(Bounds().RightTop());
		BRect fr = Window()->Frame();
		long h = rightTop.y - fr.top + 10;
		Window()->ResizeTo(fr.Width(),h);		
		if (fixedView)
			fixedView->SetResizeMode(saveMode);
		}
		Hide();
	}	
}
****/

void EditFindView::AllAttached()
{
	typeView->SetTarget(this);
	sizeView->SetTarget(this);
		
	BBox::AllAttached();
	
	// Hide();
}

void EditFindView::MessageReceived(BMessage *msg)
{
	BTextControl 	*src;
	long			data;
	const char		*txt;
	switch(msg->what) {
		case 'DATA':
		case B_REFS_RECEIVED: {
			if (!msg->HasRef("refs"))
				break;

/*****				
			BFile fil(msg->FindRef("refs"));
			
			if (fil.Error() < B_NO_ERROR)
				break;

			ulong type,crea;
			fil.GetTypeAndApp(&type,&crea);
			
			char	nTxt[5];
			nTxt[4] = 0;
		
			*((long *)nTxt) = type;
			typeView->SetText(nTxt);
			*((long *)nTxt) = crea;
			creaView->SetText(nTxt);
			
			char szstr[40];
			sprintf(szstr,"%d",fil.Size());
			sizeView->SetText(szstr);
			
			BMessage *m = new BMessage(M_SET_TYPE);
			m->AddLong("type",type);
			Looper()->PostMessage(m,listing);
			
			m = new BMessage(M_SET_CREATOR);
			m->AddLong("creator",crea);
			Looper()->PostMessage(m,listing);
			
			m = new BMessage(M_SET_SIZE);
			m->AddLong("size",fil.Size());
			Looper()->PostMessage(m,listing);
*****/			
			break;
		}
		case M_ITEMS_SELECTED: {
			PRINT(("find view items selected\n"));
			
			FindItem *fItem;
			msg->FindPointer("finditem",reinterpret_cast<void **>(&fItem));
			
			typeView->SetText(fItem->Signature());
			
			char szstr[40];
			sprintf(szstr,"%d",(long)fItem->size);
			sizeView->SetText(szstr);
			break;
		}
		case M_SET_TYPE:
			PRINT(("find view got type\n"));
			if (msg->HasPointer("source")) {
				BControl *c;
				msg->FindPointer("source",reinterpret_cast<void **>(&c));
				src = cast_as(c,BTextControl);
				ASSERT(src);
				txt = src->Text();
				msg->AddString("signature",txt);
				
				Looper()->DetachCurrentMessage();
				Looper()->PostMessage(msg,listing);
			}
			break;
		case M_SET_SIZE:
			PRINT(("find view got size\n"));
			if (msg->HasPointer("source")) {
				BControl *c;
				msg->FindPointer("source",reinterpret_cast<void **>(&c));
				src = cast_as(c,BTextControl);
				txt = src->Text();	
				data = atoi(txt);
				msg->AddInt32("size",data);
				
				Looper()->DetachCurrentMessage();
				Looper()->PostMessage(msg,listing);
			}
			break;
	}
}

void EditFindView::SetEnabled(bool on)
{
	long i = 0;
	BView *child;
	do {
		child = ChildAt(i++);
		BControl *cView = cast_as(child,BControl);
		if (cView)
			cView->SetEnabled(on);
	} while(child);
}
