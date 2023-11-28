// FindChoiceWind.cpp

#include <Message.h>
#include <StringView.h>
#include <ScrollView.h>
#include "FindChoiceWind.h"

#include "DestinationList.h"
#include "IArchivePatchItem.h"
#include <Path.h>

#include "MyDebug.h"
#include "Util.h"

FindChoiceWind::FindChoiceWind(BEntryList *_query,
								bool *_continueInstall,
							 	entry_ref *_foundItem,
							 	FindItem	*_findItem)
	: BWindow(BRect(0,0,420,220),
			"Multiple Items Found",
			B_TITLED_WINDOW,
			0/*NULL*/),
	  query(_query),
	  continueInstall(_continueInstall),
	  foundItem(_foundItem),
	  findItem(_findItem)
{
	Lock();
	PositionWindow(this,0.5,1.0/3.0);
	SetSizeLimits(250,8192,160,8192);

	AddChild(new FindChoiceView(Bounds()));
	Unlock();
}

void FindChoiceWind::Go()
{
	long exitVal;
	
	Show();
	wait_for_thread(Thread(),&exitVal);
}

bool FindChoiceWind::QuitRequested()
{
	BMessage *msg = CurrentMessage();
	
	if (msg->HasPointer("source")) {
		BControl *src;
		msg->FindPointer("source",(void**)&src);
		if (src == cntButton) {
			*continueInstall = TRUE;
			return BWindow::QuitRequested();
		}
	}

	*continueInstall = FALSE;
	return BWindow::QuitRequested();
}

void FindChoiceWind::SelectionSet(const char *path)
{
	BEntry	pathItem(path);
	
	pathItem.GetRef(foundItem);
}

/////

FindChoiceView::FindChoiceView(BRect frame)
	: BView(frame,B_EMPTY_STRING,B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);
}

void FindChoiceView::AttachedToWindow()
{
	BView::AttachedToWindow;
	
	FindChoiceWind *wind = (FindChoiceWind *)Window();
	
	BRect r = Bounds();
	BRect ir;
	r.InsetBy(12,8);
	ir = r;
	///////////////////////////////////////////////////////
	r.right -= 4;
	r.left = r.right - 75;
	r.top = r.bottom - 26;

	BButton *btn = new BButton(r,B_EMPTY_STRING,"Continue",new BMessage(B_QUIT_REQUESTED),
							B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(btn);
	btn->MakeDefault(TRUE);
	wind->cntButton = btn;
	
	r.OffsetBy(-110,0);
	AddChild(new BButton(r,B_EMPTY_STRING,"Cancel",new BMessage(B_QUIT_REQUESTED),
							B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT));
	
	r = ir;	
	r.bottom = r.top + 16;
	
	char pathBuf[1024];
	sprintf(pathBuf,"%s returned multiple search results.",wind->findItem->path);
	BStringView *lab = new BStringView(r,B_EMPTY_STRING,pathBuf);
	AddChild(lab);
	lab->SetViewColor(light_gray_background);
	lab->SetFont(be_plain_font);
	r.OffsetBy(0,14);
	
	lab = new BStringView(r,B_EMPTY_STRING,"Select the location\
 where you would like to install:");
	AddChild(lab);
	lab->SetViewColor(light_gray_background);
	lab->SetFont(be_plain_font);
	 
	
	r.top = r.bottom + 12;
	r.bottom = ir.bottom - 48;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	RList<StringItem *> *itemList = new RList<StringItem *>;
	
	BEntryList *query = wind->query;
	query->Rewind();
	BEntry foundEntry;				
		
	while (query->GetNextEntry(&foundEntry) == B_NO_ERROR ) {
		BPath	path;	
		foundEntry.GetPath(&path);
		StringItem *newName = new StringItem(path.Path());
		itemList->AddItem(newName);
	}
	
	FindListView *lyst = new FindListView(r,itemList);
	
	AddChild(new BScrollView("scroller",lyst,B_FOLLOW_ALL,0,FALSE,TRUE));	
	
	lyst->MakeFocus(TRUE);
	lyst->SelectItem(0L);
	lyst->SelectionSet();
}


////
FindListView::FindListView(BRect r,RList<StringItem *> *_itemList)
	: SimpleListView(r,B_EMPTY_STRING,B_FOLLOW_ALL,
					 B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  foundList(_itemList)
{
	SetMultipleSelect(FALSE);
	SetDragReorder(FALSE);

	SetItemList((RList<ListItem *> *)foundList);
}

FindListView::~FindListView()
{
	for (long i = foundList->CountItems()-1; i >= 0; i--) {
		delete(foundList->ItemAt(i));
	}
	delete foundList;
}

void FindListView::DrawItem(BRect updateRect,
						long index,
						BRect *itemFrame)
{	
	updateRect;
	
	StringItem *li = (StringItem *)ItemAt(index);
	
	BRect iFrame;
	if (itemFrame)
		iFrame = *itemFrame;
	else
		iFrame = ItemFrame(index);

	MovePenTo(10.0,iFrame.bottom-BaselineOffset()+1);
	DrawString(li->string);
	if (li->selected)
		InvertRect(iFrame);
}

void FindListView::Invoke(long index)
{
	index;
}

void FindListView::SelectionSet()
{
	// do something!!
	StringItem *it;
	it = (StringItem *)ItemAt(LowSelection());
	((FindChoiceWind *)Window())->SelectionSet(it->string);
}
