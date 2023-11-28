
#include "ToolView.h"
#include <stdio.h>

#include <PopUpMenu.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <string.h>
#include <MessageRunner.h>

const float stdHeight = 34;

ToolView::ToolView() :
	BView(BRect(0,0,50,50), "Toolbar", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
	fFramework(NULL),
	fBounds(Bounds()),
	fScaleMenu(NULL),
	fLastButton(-1),
	fRunner(NULL),
	fInitialShowing(false),
	fPageCount(0),
	fCurrentPage(0),
	fScale(1.0)
{
	for (int ix = 0; ix < iArtCount; ix++) {
		fArt[ix] = NULL;
		fDownArt[ix] = NULL;
	}
	
	BPoint offset(6, 0);

	BRect buttonRect(0, 0, 17, 18);
	BRect displayRect(0, 0, 88, 18);
	
	fZones[iFirstPage] = buttonRect;
	fZones[iFirstPage].OffsetTo(9,7);
	
	fZones[iPrevPage] = buttonRect;
	fZones[iPrevPage].OffsetTo(fZones[iFirstPage].RightTop() + offset);
	
	fZones[iCurrentPage] = BRect(0, 0, 88, 18);
	fZones[iCurrentPage].OffsetTo(fZones[iPrevPage].RightTop() + offset);
	
	fZones[iNextPage] = buttonRect;
	fZones[iNextPage].OffsetTo(fZones[iCurrentPage].RightTop() + offset);

	fZones[iLastPage] = fZones[iNextPage];
	fZones[iLastPage].OffsetTo(fZones[iNextPage].RightTop() + offset);
	
	fZones[iScale] = BRect(0, 0, 44, 18);
	fZones[iScale].OffsetTo(fZones[iLastPage].RightTop() + offset);

#if PDF_PRINTING > 0
	fZones[iPrint] = BRect(0, 0, 44, 18);
	fZones[iPrint].OffsetTo(fZones[iScale].RightTop() + offset);
#endif

	SetViewColor(151, 155, 142);
	fScaleMenu = new BPopUpMenu("ScaleMenu");
	BMessage msg(SET_SCALE);

	
	msg.AddFloat("scale", 2.0);
	fScaleMenu->AddItem(new BMenuItem("200%", new BMessage(msg)));

	msg.ReplaceFloat("scale", 1.5);
	fScaleMenu->AddItem(new BMenuItem("150%", new BMessage(msg)));
	
	msg.ReplaceFloat("scale", 1.25);
	fScaleMenu->AddItem(new BMenuItem("125%", new BMessage(msg)));

	msg.ReplaceFloat("scale", 1.0);
	fScaleMenu->AddItem(new BMenuItem("100%", new BMessage(msg)));
	
	msg.ReplaceFloat("scale", .75);
	fScaleMenu->AddItem(new BMenuItem("75%", new BMessage(msg)));

	msg.ReplaceFloat("scale", .5);
	fScaleMenu->AddItem(new BMenuItem("50%", new BMessage(msg)));

	msg.ReplaceFloat("scale", .25);
	fScaleMenu->AddItem(new BMenuItem("25%", new BMessage(msg)));

//	msg.AddFloat("scale", 9.74);
//	fScaleMenu->AddItem(new BMenuItem("974%", new BMessage(msg)));

	fScaleMenu->AddItem(new BSeparatorItem());
	
	msg.what = SCALE_TO_FIT;
	msg.AddInt32("flags", SCALE_TO_WIDTH);
	fScaleMenu->AddItem(new BMenuItem("Scale To Width", new BMessage(msg)));
	
	msg.ReplaceInt32("flags", SCALE_TO_WIDTH | SCALE_TO_HEIGHT);
	fScaleMenu->AddItem(new BMenuItem("View Whole Page", new BMessage(msg)));


	fPageNum = new BTextControl(fZones[iCurrentPage], "PageNum", "Go to:", "", new BMessage('pnum'));
	fPageNum->SetDivider(44);
	fPageNum->SetAlignment(B_ALIGN_CENTER, B_ALIGN_CENTER);
	fPageNum->Hide();
	AddChild(fPageNum);
}


ToolView::~ToolView()
{
	delete fScaleMenu;
	
	for (int32 ix = 0; ix < iArtCount; ix++) {
		if (fArt[ix]) {
			fArt[ix]->Release();
			fArt[ix] = NULL;
		}
		if (fDownArt[ix]) {
			fDownArt[ix]->Release();
			fDownArt[ix] = NULL;
		}
	}
}

void 
ToolView::SetFramework(DocFramework *framework)
{
	fFramework = framework;
}


void 
ToolView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	
		case 'pnum':  {
			int32 val;
			sscanf(fPageNum->Text(), "%ld", &val);
			if (fFramework) {
				DocView *doc = fFramework->Document();
				if (doc)
					fCurrentPage = doc->GoToPage(val);
				fPageNum->Hide();
			}
			break;
		}
		
		case 'unhd': {
			// unhide if not already showing
			if (fFramework && fFramework->IsHidden()) {
				fFramework->Show();
				fInitialShowing = true;
			}

			delete fRunner; fRunner = NULL;
			break;
		}
		
		default:
			BView::MessageReceived(msg);
	}
}

void 
ToolView::MouseDown(BPoint where)
{
	// check to see where it intersects
	if (!fFramework)
		return;
		
	DocView *doc = fFramework->Document();
	if (!doc)
		return;
	bool invalid = false;
	if (fZones[iCurrentPage].Contains(where)) {
		char buf[16];
		sprintf(buf, "%ld", fCurrentPage);
		SetHighColor(151, 155, 142);
		FillRect(fZones[iCurrentPage]);
		fPageNum->SetText(buf);
		fPageNum->Show();
		fPageNum->MakeFocus(true);
		fLastButton = iCurrentPage;
	}
	else if (fLastButton == iCurrentPage) {
		fPageNum->Hide();
		fLastButton = -1;
	
	}
	
	if (fZones[iFirstPage].Contains(where)) {
		uint32 newPage = doc->GoToFirstPage();
		if (newPage != fCurrentPage) {
			fCurrentPage = newPage;
			invalid = true;
			if (fDownArt[iFirstPage])
				fDownArt[iFirstPage]->Draw(this, fZones[iFirstPage]);
			fLastButton = iFirstPage;
		}
	}
	else if (fZones[iPrevPage].Contains(where)) {
		uint32 newPage = doc->GoToPreviousPage();
		if (newPage != fCurrentPage) {
			fCurrentPage = newPage;
			invalid = true;
			if(fDownArt[iPrevPage])
				fDownArt[iPrevPage]->Draw(this, fZones[iPrevPage]);
			fLastButton = iPrevPage;
		}
	}
	else if (fZones[iNextPage].Contains(where)) {
		uint32 newPage = doc->GoToNextPage();
		if (newPage != fCurrentPage) {
			fCurrentPage = newPage;
			invalid = true;
			if (fDownArt[iNextPage])
				fDownArt[iNextPage]->Draw(this, fZones[iNextPage]);
			fLastButton = iNextPage;
		}
	}
	else if (fZones[iLastPage].Contains(where)) {
		uint32 newPage = doc->GoToLastPage();
		if (newPage != fCurrentPage) {
			fCurrentPage = newPage;
			invalid = true;
			if(fDownArt[iLastPage])
				fDownArt[iLastPage]->Draw(this, fZones[iLastPage]);
			fLastButton = iLastPage;
		}
	}
	else if (fZones[iScale].Contains(where)) {
		// show the scale menu
		BMenuItem *item = fScaleMenu->Go(ConvertToScreen(fZones[iScale].LeftTop()));
		if (item) {
			bool invalidScale = false;
			BMessage *msg = item->Message();
			if (msg->what == SET_SCALE) {
				float scale = 1.0;
				float x = 1.0;
				float y = 1.0;
				doc->GetScale(&x, &y);
				msg->FindFloat("scale", &scale);
				if (scale != y) {
					doc->SetScale(scale, scale);
					invalidScale = true;
				}
			}
			else if (msg->what == SCALE_TO_FIT) {
				uint32 flags = SCALE_TO_WIDTH;
				msg->FindInt32("flags", (int32 *)&flags);
				doc->ScaleToFit(doc->Bounds(), flags);
				invalidScale = true;
			}
			if (invalidScale)
				Invalidate(fZones[iScale]);
		}
	}
#if PDF_PRINTING > 0
	else if (fZones[iPrint].Contains(where)) {
		if (!doc->IsPrinting())
			doc->Print();
	}
#endif
	if (invalid) {
		Invalidate(fZones[iCurrentPage] | fZones[fLastButton]);
	}
}

void 
ToolView::MouseUp(BPoint where)
{
	if (fLastButton < 0)
		return;
		
	if (fLastButton < iArtCount) {
		Invalidate(fZones[fLastButton]);
		fLastButton = -1;
	}
}

void 
ToolView::DeliverArt(int32 id, ContentInstance *instance)
{
	if (id < 0)
		return;
	
	//printf("DeliverArt(%ld)\n", id);
	ContentInstance **array = NULL;
	
	if (id >= 10) {
		array = fDownArt;
		id -= 10;
	}
	else
		array = fArt;
	
	if (id >= 0 && id < iArtCount) {
		array[id] = instance;
		array[id]->Acquire();
		array[id]->FrameChanged(fZones[id],  fBounds.Width(), fBounds.Height());
		if (fInitialShowing && array == fArt) {
			if (LockLooper()) {
				Invalidate(fZones[id]);
				UnlockLooper();
			}
		}
	}
	
	if (!fInitialShowing && fArt[0] && fArt[1] && fArt[2]) {
		if (LockLooper()) {
			if (fFramework && fFramework->IsHidden()) {
				fFramework->Show();
				fInitialShowing = true;
			}
			UnlockLooper();
		}
	}
}

void 
ToolView::AttachedToWindow()
{
	fPageNum->SetTarget(this);
	SetViewColor(151, 155, 142);
	fRunner = new BMessageRunner(BMessenger(this), new BMessage('unhd'), 2000000, 1);
	if (fFramework) {
		DocView *doc = fFramework->Document();
		if (doc) {
			fPageCount = doc->PageCount();
			fCurrentPage = doc->CurrentPage();
			float x = 1.0;
			doc->GetScale(&x, &fScale);
		}
	}
}


void 
ToolView::FrameResized(float width, float height)
{
	fBounds.Set(0, 0, width, height);
	// set up the frames for the icons
	for (int32 ix = 0; ix < iArtCount; ix++) {
		if (fArt[ix])
			fArt[ix]->FrameChanged(fZones[ix],  fBounds.Width(), fBounds.Height());

		if (fDownArt[ix])
			fDownArt[ix]->FrameChanged(fZones[ix],  fBounds.Width(), fBounds.Height());

	}
}


void 
ToolView::DocSizeChanged(float scale)
{
	fScale = scale;
	Invalidate(fZones[iScale]);
}

void 
ToolView::DocPageChanged(uint32 pageNum)
{
	fCurrentPage = pageNum;
	Invalidate(fZones[iCurrentPage]);
}

void 
ToolView::Draw(BRect updateRect)
{
	// paint the different zones different colors

//	for (int ix = 0; ix < iArtCount; ix++) {
//		printf("%d: fArt: 0x%x fDownArt: 0x%x \n", ix, fArt[ix], fDownArt[ix]);
//	}

	if (BView::IsPrinting()) {
		SetHighColor(255,255,255);
		FillRect(Bounds(), B_SOLID_HIGH);
		return;

	}
	char buf[32];
	float strWidth = 1.0;
	float off = 0.0;
	BPoint pt;
	
	if (updateRect.Intersects(fZones[iCurrentPage])) {
		SetHighColor(255, 255, 255);
		FillRect(fZones[iCurrentPage]);
		SetHighColor(0,0,0);
		sprintf(buf, "%ld of %ld", fCurrentPage, fPageCount);
		strWidth = StringWidth(buf);
		off = (fZones[iCurrentPage].Width() - strWidth)/2;
		pt = fZones[iCurrentPage].LeftBottom() + BPoint(off, -5);
		DrawString(buf, pt);
		StrokeRect(fZones[iCurrentPage]);
	}		
	if (updateRect.Intersects(fZones[iScale])) {
		SetHighColor(255, 255, 255);
		FillRect(fZones[iScale]);
		SetHighColor(0,0,0);
		sprintf(buf, "%ld %%", (uint32)(fScale * 100));
		strWidth = StringWidth(buf);
		off = (fZones[iScale].Width() - strWidth)/2;
		pt = (fZones[iScale].LeftBottom() + BPoint(off, -5));
		DrawString(buf, pt);
		StrokeRect(fZones[iScale]);
	}
#if PDF_PRINTING > 0
	if (updateRect.Intersects(fZones[iPrint])) {
		SetHighColor(255, 255, 255);
		FillRect(fZones[iPrint]);
		SetHighColor(0,0,0);
		sprintf(buf, "Print");
		strWidth = StringWidth(buf);
		off = (fZones[iPrint].Width() - strWidth)/2;
		pt = (fZones[iPrint].LeftBottom() + BPoint(off, -5));
		DrawString(buf, pt);
		StrokeRect(fZones[iPrint]);
	}
#endif

	BPoint backTri[3];
	backTri[0].Set(4, 9);
	backTri[1].Set(14, 3);
	backTri[2].Set(14, 15);

	BPoint forTri[3];
	forTri[0].Set(13, 9);
	forTri[1].Set(3, 3);
	forTri[2].Set(3, 15);


	SetHighColor(0,0,0);
	if (updateRect.Intersects(fZones[iFirstPage])) {
		if (fLastButton == iFirstPage && fDownArt[iFirstPage])
			fDownArt[iFirstPage]->Draw(this, fZones[iFirstPage]);
		else if (fArt[iFirstPage]) {
			fArt[iFirstPage]->Draw(this, fZones[iFirstPage]);
		}
		else {
			BPoint lt(fZones[iFirstPage].LeftTop());
			FillTriangle(lt + backTri[0], lt + backTri[1], lt + backTri[2]);
			StrokeLine(lt + BPoint(3, 3), lt + BPoint(3, 15));
			StrokeRect(fZones[iFirstPage]);
		}
	}

	if (updateRect.Intersects(fZones[iPrevPage])) {
		if (fLastButton == iPrevPage && fDownArt[iPrevPage])
			fDownArt[iPrevPage]->Draw(this, fZones[iPrevPage]);
		else if (fArt[iPrevPage]) {
			fArt[iPrevPage]->Draw(this, fZones[iPrevPage]);
		}
		else {
			BPoint lt(fZones[iPrevPage].LeftTop());
			FillTriangle(lt + backTri[0], lt + backTri[1], lt + backTri[2]);
			StrokeRect(fZones[iPrevPage]);
		}
	}

	
	if (updateRect.Intersects(fZones[iNextPage])) {
		if (fLastButton == iNextPage && fDownArt[iNextPage])
			fDownArt[iNextPage]->Draw(this, fZones[iNextPage]);
		else if (fArt[iNextPage]) {
			fArt[iNextPage]->Draw(this, fZones[iNextPage]);
		}
		else {
			BPoint lt(fZones[iNextPage].LeftTop());
			FillTriangle(lt + forTri[0], lt + forTri[1], lt + forTri[2]);
			StrokeRect(fZones[iNextPage]);
		}
	}
	
	if (updateRect.Intersects(fZones[iLastPage])) {
		if (fLastButton == iLastPage && fDownArt[iLastPage])
			fDownArt[iLastPage]->Draw(this, fZones[iLastPage]);
		else if (fArt[iLastPage]) {
			fArt[iLastPage]->Draw(this, fZones[iLastPage]);
		}
		else {
			BPoint lt(fZones[iLastPage].LeftTop());
			FillTriangle(lt + forTri[0], lt + forTri[1], lt + forTri[2]);
			StrokeLine(lt + BPoint(14, 3), lt + BPoint(14, 15));
			StrokeRect(fZones[iLastPage]);
		}
	}

	
//	DocView *doc = NULL;
//	if (fFramework)
//		doc = fFramework->Document();
//		
//	if (doc) {
//	
//		const char *title = doc->Title();
//		pt = (fZones[iScale].RightBottom() + BPoint(6, -5));
//		if (title)
//			DrawString(title, pt);
//	}
	SetPenSize(2);
	BRect bounds(Bounds());
	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
}

