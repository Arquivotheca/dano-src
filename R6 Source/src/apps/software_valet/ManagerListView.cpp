// ManagerListView.cpp

#include "ManagerListView.h"

#include "PackageDB.h"
#include "PackageItem.h"

#include "DoIcons.h"
#include "MyDebug.h"

#include <ScrollBar.h>
#include <MenuItem.h>

ManagerListView::ManagerListView(BRect r,
								BHandler *_fTarget)
	:	SimpleListView(r,"listing",
						B_FOLLOW_ALL,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  fTarget(_fTarget),
	  IHeight(35),
	  fCl(NULL)
{
	SetItemList((RList<ListItem *> *)new RList<PackageItem *>);
	
	SetMultipleSelect(TRUE);
	SetDragReorder(TRUE);
	
	// groupList = newList;
	SelectNoItems();
	SetItemHeight(IHeight);
	SetBaselineOffset(2.0);
	// groupList->DeSelectAll();
	
	BRect offRect(0,0,800,ItemHeight()-1);
	fOffBitmap = new BBitmap(offRect,B_COLOR_8_BIT,true);
	fOffView = new BView(offRect,B_EMPTY_STRING,B_FOLLOW_ALL,B_WILL_DRAW);
	fOffBitmap->AddChild(fOffView);
	fOffBitmap->Lock();
	fOffView->SetFontSize(9);
	fOffBitmap->Unlock();
}

ManagerListView::~ManagerListView()
{
	delete ItemList();
	
	// for 
	for (long i = ColList->CountItems()-1; i >= 0; i--) {
		delete ColList->ItemAt(i);
	}
	delete ColList;
	
	delete fOffBitmap;
}

void ManagerListView::AttachedToWindow()
{
	SimpleListView::AttachedToWindow();
	
	SetFont(be_plain_font);
	SetFontSize(9);
	SetItemHeight(IHeight);
	
	/////////// setup columns /////////////
	
	ColList = new RList<ColumnInfo *>;
	
	ColList->AddItem(new ColumnInfo(ICONTAG,40,40,""));
	ColList->AddItem(new ColumnInfo(NAMETAG,108,60,"Name"));
	ColList->AddItem(new ColumnInfo(VERSIONTAG,65,50,"Version"));
	ColList->AddItem(new ColumnInfo(SIZETAG,55,40,"Size"));
	ColList->AddItem(new ColumnInfo(REGTAG,65,60,"Registered"));
	ColList->AddItem(new ColumnInfo(DESCTAG,260,60,"Description"));
	
	///////////////////////////////////////
	BRect clabelR = Parent()->Frame();
	
	clabelR.right -= 1;
	clabelR.bottom = clabelR.top-1;
	clabelR.top = clabelR.bottom - 18;
	
	fCl = new ColLabelView(clabelR,
							"collabel",
							B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
							B_WILL_DRAW /* | B_FULL_UPDATE_ON_RESIZE */);
	Parent()->Parent()->AddChild(fCl);
	fCl->SetColumnList(ColList);
	fCl->SetTarget(this);
	
	MakeFocus(TRUE);
	SetViewColor(B_TRANSPARENT_32_BIT);
}

BView *ManagerListView::LabelView()
{
	return fCl;
}

void ManagerListView::FixHScroll()
{
	BScrollBar *hScroll = ScrollBar(B_HORIZONTAL);
	if (hScroll) {
		float viewWidth = Bounds().Width();
		float dataWidth = 800;
		if (dataWidth < viewWidth)
			dataWidth = viewWidth;
	
		hScroll->SetRange(0,dataWidth - viewWidth);
		hScroll->SetProportion((float)viewWidth/(float)dataWidth);
		// hScroll->SetValue(Bounds().left);
		hScroll->SetSteps(5.0,viewWidth);
	}
}

void ManagerListView::FrameResized(float w, float h)
{
	SimpleListView::FrameResized(w,h);
	FixHScroll();
}

void ManagerListView::Draw(BRect up)
{
	long start = (long)(up.top/ItemHeight());
	long end = (long)(up.bottom/ItemHeight());
	
	// pin start and end values
	if (start < 0)
		return;
	
	end = min_c(CountItems()-1,end);
	
	// cache values for faster drawing
	BRect fastFrame = ItemFrame(0);
	long iHeight = (long)ItemHeight();
	fastFrame.top = start*ItemHeight();
	fastFrame.bottom = fastFrame.top + iHeight-1;
	while(start <= end) {
		DrawItem(up,start,&fastFrame);
		fastFrame.top += iHeight;
		fastFrame.bottom += iHeight;
		start++;
	}
	
	// if top of "last" frame 
	if (fastFrame.top <= up.bottom) {
		up.top = fastFrame.top;
		FillRect(up, B_SOLID_LOW);
	}
}

void ManagerListView::DrawItem(BRect updt, long item, BRect *iFrame)
{
	updt;
	
	const long tOff = 16;
	
	PackageItem *it = (PackageItem *)ItemAt(item);
	
	BRect r;
	if (iFrame)
		r = *iFrame;
	else
		r = ItemFrame(item);	
	
	fOffBitmap->Lock();
	fOffView->FillRect(fOffView->Bounds(), B_SOLID_LOW);
	
	//BView *fOffView = this;
	//FillRect(r,B_SOLID_LOW);
	
	BRect rr = r;
	rr.OffsetTo(0,0);

	// draw icon
	long left = (long)r.left;
	long top = 0;
	long count = ColList->CountItems();
	for (long i = 0; i < count; i++) {
		ColumnInfo *ci = ColList->ItemAt(i);
		if (ci->hidden)
			continue;
		long ileft = left + 4;
		long cmax = (long)(ci->width - 8.0);
		switch(ci->tag) {
			case ICONTAG:
			{
				fOffView->DrawBitmapAsync(gPkgIcon,BPoint(ileft, top + 1));
				break;
			}
			case NAMETAG:
			{
				DrawStringClipped(fOffView, it->data.FindString("package"),
					BPoint(ileft, top + tOff), cmax);
				DrawStringClipped(fOffView, it->data.FindString("developer"),
					BPoint(ileft, top + 28), cmax);
				break;
			}
			case VERSIONTAG:
			{
				DrawStringClipped(fOffView, it->data.FindString("version"),
					BPoint(ileft, top + tOff), cmax);
					
				int32 softtype = it->data.FindInt32("softtype");
				static char *type[] = {"Commercial","Trialware","Freeware","Shareware","Beta",""};
				if (softtype < 0 || softtype > 4) softtype = 5;
				DrawStringClipped(fOffView, type[softtype],
					BPoint(ileft, top + 28), cmax);
				break;
			}
			case SIZETAG:
			{
				char buf[40];
				int64 installedSize;
				it->data.FindInt64("installsize",&installedSize);
				if (installedSize == 0) { // should -- instead of 0 bytes
					sprintf(buf, "--");
				} else if (installedSize < 1024) {
					sprintf(buf,"%Ld bytes",installedSize);
				} else if (installedSize < 1024*1024) {
					sprintf(buf,"%d KB",(long)(((float)installedSize/1024.0) + 0.5));
				} else {
					sprintf(buf,"%.2f MB",(float)installedSize/(1024.0*1024.0));
				}
				fOffView->DrawString(buf, BPoint(ileft, top + tOff));
				
				break;
			}
			case DESCTAG:
			{
				DrawStringClipped(fOffView, it->data.FindString("description"),
					BPoint(ileft, top + tOff), ci->width);
				break;
			}
			case REGTAG:
			{
				int32 registered = it->data.FindInt32("registered");
				fOffView->DrawString((registered == PackageItem::REGISTERED_YES) ? "Yes" : 
						(registered == PackageItem::REGISTERED_NO) ? "No" :
						"Waiting",BPoint(ileft, top + tOff));
				break;
			}
			default:
			{
				break;
			}
		}
		left += (long)ci->width;
	}

	/*
	float colOne = r.left + 40;
	float colTwo = colOne + 80;
	float margin = 12;
	
	// DrawString(i->fileName,BPoint(r.left + 40, r.top + 18));
	DrawStringClipped("Version 1.1",BPoint(colOne,r.top + 30),colTwo-colOne-margin);
	DrawString("Size: 2.1 MB",BPoint(colTwo, r.top + 18));
	DrawString("Dumbass Software, Inc.",BPoint(colTwo,r.top + 30));
	*/
	
	//SetHighColor(200,80,80);
	//StrokeLine(r.LeftBottom(), r.RightBottom());
	//SetHighColor(0,0,0);
	
	fOffView->SetHighColor(200,80,80);
	fOffView->StrokeLine(rr.LeftBottom(), rr.RightBottom());
	fOffView->SetHighColor(0,0,0);
	
	if (it->selected) {
		fOffView->SetDrawingMode(B_OP_SUBTRACT);
		fOffView->SetHighColor(60,60,60,0);
		fOffView->FillRect(rr);
		fOffView->SetDrawingMode(B_OP_COPY);
		fOffView->SetHighColor(0,0,0);
	}
	fOffView->Sync();
	
	DrawBitmap(fOffBitmap, r.LeftTop());
	fOffBitmap->Unlock();
}


void DrawStringClipped(BView *v, const char *str,BPoint where,float len)
{
	const char ellipses[] = "...";
	float elWidth = v->StringWidth(ellipses);

	v->MovePenTo(where);

	float cur = where.x;
	float limit = max_c(cur,cur + len - elWidth);
	while(*str && cur < limit ) {
		const char *start = str;
		str++;
		// skip to next multibyte boundary
		while ((*str & 0xc0) == 0x80)
			str++;
		v->DrawString(start,str-start);
		cur = v->PenLocation().x;
	}
	if (*str && cur >= limit)
		v->DrawString(ellipses);
}


void ManagerListView::HighlightItem(bool on, long index,BRect *iFrame)
{
	if (on) {
		SetDrawingMode(B_OP_SUBTRACT);
		SetHighColor(60,60,60);
		FillRect(iFrame ? *iFrame : ItemFrame(index));
		SetDrawingMode(B_OP_COPY);
		SetHighColor(0,0,0);
	}
}

void ManagerListView::SelectionSet()
{
	BMessage *selMsg = new BMessage(M_ITEMS_SELECTED);
	selMsg->AddBool("multiple",FALSE);
	if (LowSelection() >= 0) {		
		if (LowSelection() == HighSelection()) {
			// single item selected
			selMsg->AddPointer("item",ItemAt(LowSelection()));
		}
		else {
			// multiple items selected
			selMsg->ReplaceBool("multiple",TRUE);
		}
	}
	else {
		// nothing selected
	}
	
	fTarget->Looper()->PostMessage(selMsg,fTarget);
}

void ManagerListView::KeyDown(const char *bytes, int32 numBytes)
{
	switch(*bytes) {
		case B_RIGHT_ARROW: {
			BScrollBar *h = ScrollBar(B_HORIZONTAL);
			if (h)
				h->SetValue(h->Value() + 32);
			break;
		}
		case B_LEFT_ARROW: {
			BScrollBar *h = ScrollBar(B_HORIZONTAL);
			if (h)
				h->SetValue(h->Value() - 32);
			break;
		}
		default:
			SimpleListView::KeyDown(bytes, numBytes);
			break;
	}
}

void ManagerListView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		// update the display for an existing package
		case M_COLUMN_DISPLAY: {
			int32 index = msg->FindInt32("index");
			bool ok = false;
			for (int i = ColList->CountItems()-1; i >= 0; i--) {
				ColumnInfo *ci = ColList->ItemAt(i);
				if (!ci->hidden && i != index) {
					ok = true;
					break;
				}
			}
			if (ok) {
				ColumnInfo *ci = ColList->ItemAt(index);
				if (ci)
					ci->hidden = !ci->hidden;
				BMenuItem *mi;
				msg->FindPointer("source",(void **)&mi);
				if (mi)
					mi->SetMarked(!ci->hidden);
				Invalidate();
				fCl->Invalidate();
			}
			break;
		}
		case B_REFS_RECEIVED: {
			entry_ref	ref;
			msg->FindRef("refs",&ref);
			int32 count = CountItems();
			for (long i = 0; i < count; i++) {
				PackageItem	*it = (PackageItem *)ItemAt(i);
				if (it->fileRef == ref) {
					// read in 
					//BEntry ent(&r);
					//db.ReadPackage(it,&ent);
					//InvalidateItem(i);
					SelectNoItems();
					SelectItem(i);
					Invalidate();
					ScrollTo(BPoint(i*ItemHeight(),0));
					SelectionSet();
				}
			}
			break;
		}
		case M_ITEM_MODIFIED:
			type_code	type;
			int32		count;

			msg->GetInfo("item",&type,&count);
			for (int i = 0; i < count; i++) {
				PackageItem *item;
				msg->FindPointer("item",i,(void **)&item);
				InvalidateItem(IndexOf(item));
				if (item->selected) {
					// to update display
					SelectionSet();
				}
			}
			break;
		case M_SELECT_ALL: {
			SelectAllItems();
			SelectionSet();
			Invalidate();
			break;
		}
		default:
			break;
	}
}

void ManagerListView::Invoke(long index)
{
	index;
}
