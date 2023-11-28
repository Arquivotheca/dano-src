#include "UpdateWindow.h"
#include "UpgradeList.h"
#include "GetUpdate.h"
#include "SettingsManager.h"
#include "SResIO.h"

#include "LabelView.h"
#include "RList.h"
#include "Util.h"

#include <Button.h>
#include <Control.h>
#include <ScrollView.h>
#include <ScrollBar.h>


#include "MyDebug.h"

extern SettingsManager *gSettings;

UpdateWindow::UpdateWindow(bool canShowOld)
	:	BWindow(BRect(0,0,280,290),
				"Software Updates Report",
				B_DOCUMENT_WINDOW,
				B_NOT_ZOOMABLE),
		curIndex(-1),
		totalCount(0)
{
	canShowOld;
	Lock();
	// build the report list
	
	BMessage	readReport;
	BMessage	*report = &readReport;
	status_t	pkerr;
	
	pkerr = B_OK;
	
	// read the report from a file
	SResIO	resIO;
	BFile	sFile(&gSettings->sEntry,O_RDWR);
	pkerr = sFile.InitCheck();
	if (pkerr >= B_OK) {
		pkerr = resIO.SetTo(&sFile, 'Rept', kNewReport);
		if (pkerr >= B_OK) {
			pkerr = report->Unflatten(&resIO);
			// remove report since it will be viewed
			if (pkerr >= B_OK) {
				PRINT(("REMOVING NEW REPORT\n"));
				resIO.RemoveResource();
			}
		}
	}

	// put in option to view the last report
	
	type_code 	type;
	
	report->GetInfo("upgradelist",&type,&totalCount);
	//if (totalCount > 0) {
		// copy into the viewed area
	//	
	//}
	
	if (totalCount <= 0) {
		doError("No newly available software updates were found. Currently available updates for \
software you have installed can be viewed in the \"Manage\" window.");
		PostMessage(B_QUIT_REQUESTED);
		return;	
	}
	
	for (int i = 0; i < totalCount; i++)
	{
		BMessage	alist;
		report->FindMessage("upgradelist",i,&alist);
		UpgradeItemList *nlist = new UpgradeItemList(&alist);
		reportList.AddItem(nlist);
		
		//report->AddString("package",it->data.FindString("package"));
		//report->AddString("sid",it->data.FindString("sid"));
		//report->AddString("pid",it->data.FindString("pid"));
		pinfo.AddString("package",report->FindString("package",i));
		pinfo.AddString("version",report->FindString("version",i));
		pinfo.AddString("sid",report->FindString("sid",i));
		pinfo.AddString("pid",report->FindString("pid",i));
	}
	PositionWindow(this,0.6,0.4);
	
	BRect r = Bounds();
	BView	*bv = new BView(r,B_EMPTY_STRING,B_FOLLOW_ALL,B_WILL_DRAW);
	bv->SetViewColor(200,200,200);
	AddChild(bv);
	
	BRect lr = r;
	lr.InsetBy(12,8);
	lr.bottom = lr.top + 28;
	
	titleView = new LabelView(lr,"SoftwareValet found the following new software updates:");
	bv->AddChild(titleView);
	
	r.top += 45;
	r.bottom = r.top + 2*100 + 4;

	upView = new UpdateDisplayView(r);
	bv->AddChild(upView);
	
	r.top = r.bottom + 12;
	r.bottom = r.top + 24;
	r.right -= 20;
	r.left = r.right - 70;
	
	BButton *btn;
	btn = new BButton(r,"next","Next",new BMessage('Next'),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
	bv->AddChild(btn);
	btn->SetEnabled(totalCount > 0);

	r.OffsetBy(-85,0);
	
	btn = new BButton(r,"prev","Previous",new BMessage('Prev'),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
	bv->AddChild(btn);
	btn->SetEnabled(false);

	r.OffsetBy(-95,-5);
	r.right += 20;
	
	countView = new BStringView(r,"count",B_EMPTY_STRING,B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	bv->AddChild(countView);
	
	PostMessage('Next');
	
	SetSizeLimits(280,320,200,8192);
	Unlock();
	Show();
}

UpdateWindow::~UpdateWindow()
{
	for (int i = reportList.CountItems()-1; i >= 0 ;i--)
		delete reportList.ItemAt(i);
}


void UpdateWindow::SetControls()
{
	char buf[256];
	sprintf(buf,"%d of %d %s",curIndex+1,totalCount,
		(totalCount == 1) ? "package":"packages");
	countView->SetText(buf);
	
	BControl *nxt = (BControl *)FindView("next");
	BControl *prv = (BControl *)FindView("prev");
	nxt->SetEnabled(curIndex < totalCount-1);
	prv->SetEnabled(curIndex > 0);
	
	if (curIndex >= 0 && curIndex < totalCount) {
		sprintf(buf,"SoftwareValet found the following new software updates to %s %s:",
				pinfo.FindString("package",curIndex),pinfo.FindString("version",curIndex));
		titleView->SetText(buf);
	}
}
void UpdateWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case 'Prev':
			if (curIndex > 0) {
				curIndex--;
				upView->SetItemList(reportList.ItemAt(curIndex));
				SetControls();
			}
			break;
		case 'Next':
			if (curIndex < totalCount-1) {
				curIndex++;
				upView->SetItemList(reportList.ItemAt(curIndex));
				SetControls();
			}	
			break;
		case 'GetU': {
			// get update button clicked
			if (curIndex < 0 || curIndex >= totalCount)
				break;
	
			BMessage	data;
			// serial number we are coming from
			data.AddString("pid",pinfo.FindString("pid",curIndex));
			data.AddString("sid",pinfo.FindString("sid",curIndex));
			// version we want to get
			data.AddString("vid",msg->FindString("vid"));

			new GetUpdateDialog(&data);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

//////////////////////////////////////////////////////////

class UpItemsView : public BView
{
public:
	UpItemsView(BRect r)
		:	BView(r,"test",B_FOLLOW_ALL,
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
			uplist(NULL),
			itemHeight((int32)r.Height()),
			updateCount(0)
	{
		SetViewColor(light_gray_background);
		if (itemHeight > 100) itemHeight = 100;
		//SetItemList(ups);
	}
	
	virtual void KeyDown(const char *bytes, int32 byteCount)
	{
		switch (*bytes) {
			case B_DOWN_ARROW:
			case B_PAGE_DOWN:
			{
				ScrollBy(0,itemHeight);
				break;
			}
			case B_UP_ARROW:
			case B_PAGE_UP:
			{
				ScrollBy(0,-itemHeight);
				break;
			}
			default:
				BView::KeyDown(bytes,byteCount);
		}
	}
	
	
	virtual void MakeFocus(bool state)
	{
		BView::MakeFocus(state);
		((BScrollView *)Parent())->SetBorderHighlighted(state && Window()->IsActive());
	}
	
	virtual	void WindowActivated(bool state)
	{
		BView::WindowActivated(state);
		((BScrollView *)Parent())->SetBorderHighlighted(state && IsFocus());
	}
	
	virtual ~UpItemsView()
	{
	}
	
	void	SetItemList(RList < UpgradeItem * > *ups, const char *msg = NULL);
	
	virtual void MouseDown(BPoint where)
	{
		BView::MouseDown(where);
		
		MakeFocus(true);
	}
	
	virtual void AttachedToWindow() {
		BView::AttachedToWindow();
		
		FrameResized(Bounds().Width(), Bounds().Height());
	}
	
	virtual void Draw(BRect r)
	{
		BView::Draw(r);
		
		int ix = (int)(r.top / itemHeight);
		
		// draw some separator lines
		float hz = ix*itemHeight;
		float pos = itemHeight - 1 + hz;
		while (pos <= r.bottom) {
			DrawHSeparator(r.left, r.right, pos, this);
			pos += itemHeight;
		}

		// draw items start to end
		int end = (int)(r.bottom / itemHeight);
		
		if (end >= updateCount)
			end = updateCount-1;
		
		pos = hz + 12;
		while (ix <= end) {
			UpgradeItem *it = uplist->ItemAt(ix);
			if (!it) break;
			
			BMessage &data = it->data;
			
			MovePenTo(10,pos);
			
			float hz = pos;
			
			SetHighColor(label_red);
			DrawString("Package:");
			MovePenTo(10,hz+=14);
			DrawString("Version:");
			MovePenTo(10,hz+=14);
			DrawString("Price:");
			MovePenTo(10,hz+=14);
			DrawString("Languages:");
			MovePenTo(10,hz+=14);
			DrawString("Platforms:");
			if (data.FindBool("competitive")) {
				MovePenTo(20,hz+=20);
				DrawString("Competitive Upgrade");
			}
			
			const int kLeftMargin = 70;
			
			SetHighColor(0,0,0);
			MovePenTo(kLeftMargin,pos);
			hz = pos;
			const char *str;
			str = data.FindString("package");
			if (str) DrawString(str);
			MovePenTo(kLeftMargin,hz+=14);
			str = data.FindString("version");
			if (str) DrawString(str);
			MovePenTo(kLeftMargin,hz+=14);

			float pr = data.FindFloat("fprice");
			if (pr > 0.0) {
				char buf[24];
				sprintf(buf,"$%.2f (US)",pr);
				DrawString(buf);
			}
			else
				DrawString("—");
			MovePenTo(kLeftMargin,hz+=14);

			type_code 	type;
			int32		count;
			
			data.GetInfo("language",&type,&count);	
			for (int i = 0; i < count ; i++) {
				DrawString(data.FindString("language",i));
				if (i < count-1)
					DrawString(", ");
			}
			
			MovePenTo(kLeftMargin,hz+=14);

			data.GetInfo("platform",&type,&count);				
			for (int i = 0; i < count ; i++) {
				DrawString(data.FindString("platform",i));
				if (i < count-1)
					DrawString(", ");
			}
			
			ix++;
			pos += itemHeight;
		}
	}
	
	void FrameResized(float w, float h)
	{
		BView::FrameResized(w, h);
		
		BRect  r = Frame();
		BScrollBar *sb = ScrollBar(B_VERTICAL);
		
		if (contentHeight < h)
			contentHeight = (int32)h;

		sb->SetRange(0,contentHeight - h);
		sb->SetProportion(h/(float)contentHeight);
		sb->SetSteps(10.0, itemHeight);

//		sb = ScrollBar(B_HORIZONTAL);
//		sb->SetRange(0,contentWidth - w);
//		sb->SetProportion(w/(float)contentWidth);
	}
	
private:
	RList<UpgradeItem *> *uplist;
	
	int32			updateCount;
	int32			itemHeight;
	
	int32			contentWidth;
	int32			contentHeight;
};

void	UpItemsView::SetItemList(RList < UpgradeItem * > *ups, const char *msg)
{
	uplist = ups;
	
	BView *child; 
	while ( child = ChildAt(0)) {
    	RemoveChild(child);
	}
	//Sync();
	
	if (uplist)
		updateCount = ups->CountItems();
	else
		updateCount = 0;

	BRect r = Frame();			
	contentWidth = (int32)r.Width();
	contentHeight = itemHeight * updateCount;
	
	if (updateCount > 0) {
		BRect n = r;
		
		n.right -= 4;
		n.top += 16;
		n.left = n.right - 70;
		n.bottom = n.top + 16;
		
		for (int i = 0; i < updateCount; i++) {
			BMessage *msg = new BMessage('GetU');
			char buf[40];
			int64 vid = -1;
			ups->ItemAt(i)->data.FindInt64("versionid",&vid);
			sprintf(buf,"%Ld",vid);
			msg->AddString("vid",buf);
			float pr = ups->ItemAt(i)->data.FindFloat("fprice");
			BButton *btn = new BButton(n,"btn",pr > 0 ? "Purchase" : "Download", msg,
						B_FOLLOW_RIGHT | B_FOLLOW_TOP,B_WILL_DRAW);
			AddChild(btn);
			n.OffsetBy(0,itemHeight);
			if (Window()) {
				BView *v = Window()->FindView("manageview");
				if (v) btn->SetTarget(v);
			}
		}
	}
	else if (uplist || msg) {
		BRect n = r;
		n.InsetBy(12,8);
		n.bottom = n.top + 14;
		
		BStringView *sv = new BStringView(n,B_EMPTY_STRING,B_EMPTY_STRING,
							B_FOLLOW_H_CENTER | B_FOLLOW_TOP);
		if (msg == NULL)
			sv->SetText("None available...");
		else
			sv->SetText(msg);
		
		AddChild(sv);
	}
	ScrollTo(0,0);
	Invalidate();
	
	if (Window())
		FrameResized(Frame().Width(), Frame().Height());
}

////////////////////////

UpdateDisplayView::UpdateDisplayView(BRect frame)
	:	BView(frame, "updates", B_FOLLOW_ALL, B_WILL_DRAW)
{
	BRect r = Bounds();
	r.InsetBy(2,2);
	r.right -= B_V_SCROLL_BAR_WIDTH;
	// r.bottom -= B_H_SCROLL_BAR_HEIGHT;
		
	BRect tr = r;
	tr.OffsetTo(0,0);
	//tr.InsetBy(4,4);
	//BTextView *tv = new BTextView(r,"textview",tr,
	//					B_FOLLOW_ALL,
	//					B_WILL_DRAW);
	vw = new UpItemsView(r);

	BScrollView *sv = new BScrollView("scroller",vw,B_FOLLOW_ALL,
								0,false,true,B_FANCY_BORDER );
	AddChild(sv);
}

void UpdateDisplayView::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}

void UpdateDisplayView::SetItemList(RList<UpgradeItem *> *ups, const char *msg)
{
	vw->SetItemList(ups,msg);
}

