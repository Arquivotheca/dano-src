// Patch InfoWind.cpp
#include "PatchInfoWind.h"
#include "MyDebug.h"
#include "Util.h"
#include <Path.h>
#include <Button.h>
#include <Control.h>
#include <CheckBox.h>
#include <StringView.h>
#include <ScrollBar.h>
#include <ScrollView.h>


// #include "SimpleListView.h"
#include "IArchivePatchItem.h"

PatchInfoWind::PatchInfoWind( RList<ArchivePatchItem *> *_patchList,
							  bool	*_continueInstall,
							  bool  *_deleteOriginals)
	: BWindow(BRect(0,0,360,270),
			"Patching Status",
			B_TITLED_WINDOW,
			0/*NULL*/),
	  patchList(_patchList),
	  continueInstall(_continueInstall),
	  deleteOriginals(_deleteOriginals)
{
	Lock();
	
	*deleteOriginals = TRUE;

	PositionWindow(this,0.5,1.0/3.0);

	AddChild(new PatchInfoView(Bounds(),patchList));
	
	Unlock();
	// make sure we don't die before the caller has a chance to sync with us
}

void PatchInfoWind::Go()
{
	long exitVal;
	Show();
	wait_for_thread(Thread(),&exitVal);
}

void PatchInfoWind::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_DELETE_ORIGINALS:
			BControl *c;
			msg->FindPointer("source",(void**)&c);
			*deleteOriginals = c->Value();
	}
}

bool PatchInfoWind::QuitRequested()
{
	BMessage *msg = CurrentMessage();
	
	if (msg->HasPointer("source")) {
		BControl *src;
		msg->FindPointer("source",(void**)&src);
		if (src == FindView("continuebutton")) {
			*continueInstall = TRUE;
			return BWindow::QuitRequested();
		}
	}

	*continueInstall = FALSE;
	return BWindow::QuitRequested();
}

////
PatchInfoView::PatchInfoView(BRect frame, RList<ArchivePatchItem *> *patchList)
	: BView(frame,"patchinfo",B_FOLLOW_ALL,B_WILL_DRAW)
{
	SetViewColor(light_gray_background);

	BRect r = Bounds();
	BRect ir;
	r.InsetBy(16,12);
	ir = r;
	///////////////////////////////////////////////////////
	r.left = r.right - 75;
	r.top = r.bottom - 26;

	AddChild(new BButton(r,"continuebutton","Continue",new BMessage(B_QUIT_REQUESTED),
							B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT));
	r.OffsetBy(-110,0);
	AddChild(new BButton(r,"cancelbutton","Cancel",new BMessage(B_QUIT_REQUESTED),
							B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT));
	
	///////////////////////////////////////////////////////
	r.bottom = r.top - 12;
	r.top = r.bottom - 14;
	r.left = ir.left;
	r.right = r.left + 200;
	
	BCheckBox *cb = new BCheckBox(r,"deleteoriginals","Delete Original Files",
									new BMessage(M_DELETE_ORIGINALS));
	AddChild(cb);
	cb->SetValue(TRUE);
	///////////////////////////////////////////////////////
	r = ir;	
	r.bottom = r.top + 16;
	
	AddChild(new BStringView(r,"label","Patches will be applied to the following:"));
	
	r.top = r.bottom + 12;
	r.bottom = ir.bottom - 60;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	
	RList<StringItem *> *itemList = new RList<StringItem *>;

	long count = patchList->CountItems();
	for (long i = 0; i < count; i++) {
		ArchivePatchItem *pi = patchList->ItemAt(i);
		
		// doublecheck validity
		BEntry pat(&pi->fileRef);
		if (pat.InitCheck() == B_NO_ERROR) {
			BPath	ppath;
			pat.GetPath(&ppath);
			//pi->PathForFile(&pat,pathBuf,1024);
			
			StringItem *newName = new StringItem(ppath.Path());
			itemList->AddItem(newName);
		}
	}
	
	PatchListView *lyst = new PatchListView(r,itemList);
	
	AddChild(new BScrollView("scroller",lyst,B_FOLLOW_ALL,0,FALSE,TRUE));	
}

void PatchInfoView::AllAttached()
{
	BButton *def = (BButton *)FindView("continuebutton");
	def->MakeDefault(TRUE);
	
	BView::AllAttached();
		
	BControl *cb = (BControl *)FindView("deleteoriginals");
	cb->SetTarget(Window());
}


////
PatchListView::PatchListView(BRect r,RList<StringItem *> *_itemList)
	: SimpleListView(r,"patchlisting",B_FOLLOW_ALL,
					 B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  patchList(_itemList)
{
	SetMultipleSelect(FALSE);
	// SetDragReorder(FALSE);

	SetItemList((RList<ListItem *> *)patchList);
}

PatchListView::~PatchListView()
{
	for (long i = patchList->CountItems()-1; i >= 0; i--) {
		delete(patchList->ItemAt(i));
	}
	delete patchList;
}

void PatchListView::DrawItem(BRect updateRect,
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

	
	// PRINT(("drawing index %d\n",index));	
	FillRect(iFrame, B_SOLID_LOW);
	MovePenTo(6.0,iFrame.bottom-BaselineOffset()+1);
	
	DrawString(li->string);
}

void PatchListView::Invoke(long index)
{
	index;
}

void PatchListView::SelectionSet()
{
}

void PatchListView::MouseDown(BPoint w)
{
	w;
}
