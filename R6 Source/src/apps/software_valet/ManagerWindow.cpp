#include <MenuBar.h>
#include <View.h>
#include <ScrollView.h>
#include <MenuItem.h>
#include <Button.h>
#include <Box.h>
#include <Alert.h>

#include "ManagerWindow.h"
#include "ValetApp.h"
#include "UpdateDialog.h"
#include "UpdateWindow.h"

#include "MyDebug.h"
#include "ManagerListView.h"

#include "PackageDB.h"
#include "PackageItem.h"

#include "UninstallWindow.h"
#include "SettingsManager.h"
#include "MultiScrollBar.h"
#include "TriangleTab.h"
#include "RegisterWindow.h"
//#include "Troll.h"
#include "SUrlView.h"
#include "BackupPanel.h"
#include "GetUpdate.h"

#include "Util.h"

#include <ctype.h>

#if 0
// testing transparency stuff to reduce resize flicker
class NStringView : public BStringView
{
public:
		NStringView(BRect bounds,
				const char *name, 
				const char *text,
				uint32 resizeFlags =
					B_FOLLOW_LEFT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW)
			: BStringView(bounds,name,text,resizeFlags,flags)
		{
			SetViewColor(B_TRANSPARENT_32_BIT);
			SetLowColor(220,220,255);
			//SetViewColor(220,220,255);
		}
		virtual void	Draw(BRect fr) {
			SetLowColor(220,220,255);
			//SetHighColor(220,220,255);
			FillRect(fr,B_SOLID_LOW);
			//SetHighColor(0,0,0);
			BStringView::Draw(fr);
		}
};

class NiceBox : public BBox
{
public:
	NiceBox(BRect bounds,
		const char *name = NULL,
		uint32 resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP,
		uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS |
											B_NAVIGABLE_JUMP,
		border_style border = B_FANCY_BORDER)
		:	BBox(bounds,name,resizeFlags, flags, border)
	{
		SetViewColor(B_TRANSPARENT_32_BIT);
	}
	
	virtual void	Draw(BRect fr) {
			SetHighColor(220,220,255);
			FillRect(fr);
			SetHighColor(0,0,0);
			SetLowColor(220,220,255);
			BBox::Draw(fr);
		}
};

		BBView::BBView(BRect frame,
				const char *name,
				uint32 resizeMask,
				uint32 flags)
		:	BView( frame, name, resizeMask, flags)
		{
			SetViewColor(B_TRANSPARENT_32_BIT);
			//SetViewColor(220,220,255);
			SetLowColor(220,220,255);
		}
		
void BBView::Draw(BRect fr)
{
	FillRect(fr,B_SOLID_LOW);
	BView::Draw(fr);
}
#endif


long	gSubCount = 0;


inline bool NullString(const char *s)
{
	return !(s && *s);
}


ManagerWindow::ManagerWindow(SettingsManager *_set)
	:	BWindow(BRect(0,0,450,400),"Software Manager",
				B_TITLED_WINDOW,
				0 /*NULL*/),
		settings(_set)
{
	Lock();
	TRACE();
	PositionWindow(this,settings->data.FindPoint("general/manwindow"),0.4,0.4);
	TRACE();
	
	BRect r = Bounds();
	r.bottom = r.top + 12;
	BMenuBar *bar = new BMenuBar(r,"menubar");
	bar->AddItem(new BMenu("Manage"));
	bar->AddItem(new BMenu("Display"));
	bar->AddItem(new BMenu("View"));
	bar->AddItem(new BMenu("Special"));
	AddChild(bar);
	//bar->SetViewColor(B_TRANSPARENT_32_BIT);
	TRACE();
	
	ResizeBy(0,bar->Frame().Height());
	TRACE();
	
	r = Bounds();
	r.top += bar->Frame().Height() + 1;
	TRACE();
	
	AddChild(new ManagerView(r));
	TRACE();

	SetPreferredHandler(FindView("manageview"));
	TRACE();
		
	Unlock();
	Show();
}


bool ManagerWindow::QuitRequested()
{
	if (gSubCount > 0) {
		doError("Please finish registrations before closing the Manager window\n");
		return false;
	}
	settings->data.ReplacePoint("general/manwindow",Frame().LeftTop());
	return BWindow::QuitRequested();
}

void ManagerWindow::DispatchMessage(BMessage *m, BHandler *h)
{
	switch(m->what) {
		case PKG_DISPLAY: {
			PRINT(("manager window dispatching display message\n"));
			h = FindView("manageview");
			break;
		}
		//case B_REFS_RECEIVED: {
		//	h = FindView("listing");
		//	break;
		//}
	}
	BWindow::DispatchMessage(m,h);
}

//////////////////////////////////////////////////
#pragma mark -----Manager View-----

char *ManagerView::btnnames[] = {"regbtn","updtbtn","uninstbtn"};

ManagerView::ManagerView(BRect fr)
	:	BView(fr,"manageview",B_FOLLOW_ALL,B_WILL_DRAW),
		curSel(NULL),
		pDB(NULL),
		mulSel(FALSE)
{
	TRACE();

	SetViewColor(220,220,255);
	TRACE();

	btns = new BButton *[5];
	
	TRACE();	
	pDB = new PackageDB();
		TRACE();

	//updateOption = UPDATE_checkDl;
}

ManagerView::~ManagerView()
{
	// delete listing
	for (long i = fItems.CountItems()-1; i >= 0; i--) {
		delete fItems.ItemAt(i);
	}

	delete pDB;
	delete[] btns;
}

status_t	ManagerView::ReadAllPackages()
{
	status_t	res = B_OK;
	lv->ItemList()->MakeEmpty();
	
	for (int i = fItems.CountItems()-1; i >=0; i--)
		delete fItems.RemoveIndex(i);
		
///// PACKAGE REGISTRY TESTING

	if (pDB == NULL) {
		PRINT(("pDB is null %Xd\n",pDB));
		res = B_ERROR;
	}
	else {
		//PRINT(("&pDB->fDirEntry = %Xd\n",&(pDB->fDirEntry)));
			
		pDB->Rewind();
		long pkerr;
		PackageItem *readItem;
		
		do {
			readItem = new PackageItem();
			pkerr = pDB->GetNextPackage(readItem,PackageDB::ALL_DATA);
			if (pkerr >= B_NO_ERROR)
				fItems.AddItem(readItem);
		} while (pkerr >= B_NO_ERROR);
		delete readItem;
	}
	return res;
/////////
}

void	ManagerView::UpdateViewList(bool filterDups)
{
	lv->SelectNoItems();
	lv->SelectionSet();
	lv->ItemList()->MakeEmpty();
	
	int count = fItems.CountItems();
	for (int i = 0; i < count; i++) {
		PackageItem *nit = fItems.ItemAt(i);
		if (filterDups) {
			int ix;
			for (ix = lv->CountItems()-1; ix >= 0; ix--) {
				PackageItem *it = (PackageItem *)lv->ItemAt(ix);
				if (strcasecmp(nit->data.FindString("package"),
							it->data.FindString("package")) == 0)
				{
					if (nit->data.FindInt32("releasedate") >
						it->data.FindInt32("releasedate"))
					{
						lv->RemoveItem(ix);
						lv->ItemList()->AddItem(nit,ix);	
					}
					break;
				}
			}
			if (ix < 0)
				lv->AddItem(nit);
		}
		else
			lv->AddItem(nit);
	}
	lv->Update();
}

void ManagerView::AttachedToWindow()
{
	TRACE();

	BView::AttachedToWindow();

	TRACE();
	
	BView *cview;
	BRect r = Bounds();

	BMenuBar *mbar = (BMenuBar *)Window()->FindView("menubar");
	//r.top += mbar->Frame().Height();
	
	r.OffsetBy(0,19);
	r.bottom = r.top + 180;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;

	lv = new ManagerListView(r,this);
	
	TRACE();

	//////////////////	
	
	AddChild(cview = new BScrollView("scroller",lv,
			B_FOLLOW_ALL,0,FALSE,TRUE,B_NO_BORDER));
	cview->SetViewColor(220,220,225);

	BMenu *menu = mbar->SubmenuAt(1);
	menu->AddItem(new BMenuItem("Icon",new BMessage(M_COLUMN_DISPLAY)));
	menu->AddItem(new BMenuItem("Name",new BMessage(M_COLUMN_DISPLAY)));
	menu->AddItem(new BMenuItem("Version",new BMessage(M_COLUMN_DISPLAY)));
	menu->AddItem(new BMenuItem("Size",new BMessage(M_COLUMN_DISPLAY)));
	menu->AddItem(new BMenuItem("Registered",new BMessage(M_COLUMN_DISPLAY)));
	menu->AddItem(new BMenuItem("Description",new BMessage(M_COLUMN_DISPLAY)));	
	menu->SetTargetForItems(lv);
	for (int i = menu->CountItems()-1; i >= 0; i--)
		menu->ItemAt(i)->SetMarked(true);
		
	menu = mbar->SubmenuAt(2);
	
	menu->AddItem(new BMenuItem("Latest Versions",new BMessage(M_FILTER_DISPLAY)));
#if 0	
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Commercial",new BMessage(M_FILTER_DISPLAY)));
	menu->AddItem(new BMenuItem("Trialware",new BMessage(M_FILTER_DISPLAY)));
	menu->AddItem(new BMenuItem("Freeware",new BMessage(M_FILTER_DISPLAY)));
	menu->AddItem(new BMenuItem("Shareware",new BMessage(M_FILTER_DISPLAY)));
	menu->AddItem(new BMenuItem("Beta",new BMessage(M_FILTER_DISPLAY)));
	menu->AddItem(new BMenuItem("Other",new BMessage(M_FILTER_DISPLAY)));
#endif
	menu->SetTargetForItems(this);
	for (int i = menu->CountItems()-1; i >= 0; i--)
		menu->ItemAt(i)->SetMarked(true);
	
	/////////////////
		
	r.top = r.bottom+1;
	r.bottom = r.top + B_H_SCROLL_BAR_HEIGHT;
	
	MultiScrollBar *ms = new MultiScrollBar(r,
							"hscroll",lv,0,480,B_HORIZONTAL);
	AddChild(ms);
	ms->AddExtraTarget(lv->LabelView());
	lv->FixHScroll();
	
	ReadAllPackages();
	UpdateViewList(true);
/////////

	r.left = Bounds().left;
	r.right = Bounds().right;
	r.top = r.bottom;
	r.bottom = r.top + 200;

	TRACE();	
	
	BView *infbox = new BView(r,"infobox",B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,B_WILL_DRAW);
	AddChild(infbox);
	infbox->SetViewColor(ViewColor());
	//infbox->SetViewColor(B_TRANSPARENT_32_BIT);
	
	TRACE();	

	r = infbox->Bounds();
	r.InsetBy(8,8);

	TRACE();	
	
	const int kButtonWidth = 74;
	const int kButtonSep = kButtonWidth + 4;
	BRect br = r;
	br.bottom = br.top + 20;
	br.right = br.left + kButtonWidth;
	BButton *btn;

	TRACE();	
	
	btns[0] = btn = new BButton(br,btnnames[0],"Register...",
					new BMessage(M_REGISTER),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	mbar->SubmenuAt(0)->AddItem(new BMenuItem("Register...",
					new BMessage(M_REGISTER),'R',B_COMMAND_KEY));

	TRACE();	
	
	br.OffsetBy(kButtonSep,0);
	btns[1] = btn = new BButton(br,btnnames[1],"Update...",
					new BMessage(M_UPDATE),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	mbar->SubmenuAt(0)->AddItem(new BMenuItem("Update...",
					new BMessage(M_UPDATE),'U',B_COMMAND_KEY));

////
	menu = mbar->SubmenuAt(3);
//	menu->AddItem(new BMenuItem("Troll Now...",
//					new BMessage(M_TROLL)));
	menu->AddItem(new BMenuItem("Remove...",
					new BMessage(M_REMOVE)));
	menu->AddItem(new BMenuItem("Backup package info...",
					new BMessage('BkTo')));
//	menu->AddItem(new BMenuItem("Restore package info...",
//					new BMessage('RsFr')));
	menu->AddItem(new BMenuItem("Reset Services",
					new BMessage('RstS')));			
	menu->SetTargetForItems(this);
	
///

	TRACE();	
					
/*** no uninstall button ****/
#if 0
	br.OffsetBy(kButtonSep,0);
	btns[2] = btn = new BButton(br,btnnames[2],"Uninstall...",
					new BMessage(M_UNINSTALL),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	mbar->SubmenuAt(0)->AddItem(new BMenuItem("Uninstall...",
					new BMessage(M_UNINSTALL),'U',B_COMMAND_KEY | B_OPTION_KEY));
#else
	btns[2] = NULL;
#endif
	mbar->SubmenuAt(0)->AddSeparatorItem();

	TRACE();	
	
	BMenuItem *itt = new BMenuItem("Select All",
					new BMessage(M_SELECT_ALL),'A',B_COMMAND_KEY);
	mbar->SubmenuAt(0)->AddItem(itt);
	mbar->SubmenuAt(0)->AddSeparatorItem();

	br.right = infbox->Bounds().right - 8;
	br.left = br.right - kButtonWidth;
	btns[3] = btn = new BButton(br,"settbtn","Settings...",
					new BMessage(M_SETTINGS),
					B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	
	mbar->SubmenuAt(0)->AddItem(new BMenuItem("Settings...",
					new BMessage(M_SETTINGS)));
	
	mbar->SubmenuAt(0)->SetTargetForItems(this);
	itt->SetTarget(lv);
	
	TRACE();	
					
	br.OffsetBy(-kButtonSep,0);
	btns[4] = btn = new BButton(br,"logbtn","Display Log...",
					new BMessage(M_SHOWLOG),
					B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	BMenuItem *it = new BMenuItem("Activity Log...",
					new BMessage(M_SHOWLOG),'L',B_COMMAND_KEY);
	it->SetTarget(be_app);
	mbar->SubmenuAt(0)->AddItem(it);

	mbar->SubmenuAt(0)->AddSeparatorItem();
	it = new BMenuItem("About SoftwareValet...",
					new BMessage(B_ABOUT_REQUESTED));
	it->SetTarget(be_app);
	mbar->SubmenuAt(0)->AddItem(it);

	mbar->SubmenuAt(0)->AddSeparatorItem();
	it = new BMenuItem("Quit",
					new BMessage(B_QUIT_REQUESTED),'Q',B_COMMAND_KEY);
	it->SetTarget(be_app);
	mbar->SubmenuAt(0)->AddItem(it);

	TRACE();	

	for (long i = 0; i <= 4; i++) {
			TRACE();
		if (btns[i]) {
			infbox->AddChild(btns[i]);
				TRACE();		
			btns[i]->SetTarget(this);
				TRACE();
			btns[i]->SetFont(be_plain_font);
				TRACE();
			btns[i]->SetEnabled(FALSE);
				TRACE();
			//btns[i]->SetViewColor(light_gray_background);
			//btns[i]->SetLowColor(0,0,0);
				TRACE();
		}
	}
		TRACE();
	btns[3]->SetEnabled(TRUE);
		TRACE();
	btns[4]->SetTarget(be_app);
		TRACE();
	btns[4]->SetEnabled(TRUE);
	
	TRACE();	

	r = infbox->Bounds();
	r.InsetBy(8,8);
	r.top = br.bottom + 20;
	r.bottom = r.top + 140;
	BBox *box = new BBox(r,NULL,
					B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
					 B_FULL_UPDATE_ON_RESIZE  | B_WILL_DRAW);
//	box->SetLowColor(ViewColor());
//	box->SetViewColor(light_gray_background);
	box->SetLabel("Package Information");
	infbox->AddChild(box);

	TRACE();	

	br = r;
	br.OffsetTo(0,0);
	br.InsetBy(8,0);
	br.top += 16;
	br.bottom -= 10;

	br.right = br.left + 100;
	br.bottom = br.top + 12;
	BRect sr = br;
	
	BStringView *sv;
	
	sv = new BStringView(br,"curlabel","Currently Installed",
				B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(sv);
	//sv->SetViewColor(ViewColor());
	br.OffsetBy(0,18);

	TRACE();	

	
	char	*labels[] = {"Version:",
						 "Released:",
						 "Installed:",
						 "Registered:",
						 "Serial No:"};
	char	*names[] = {"curvers",
						 "currelease",
						 "curacquired",
						 "curregister",
						 "serialno"};
	BRect lr;			 
	br.right = br.left + 60;
	lr = br;
	lr.left = br.right + 5;
	lr.right = lr.left + 110;
	for (long i = 0; i < nel(labels); i++) {
		sv = new BStringView(br,B_EMPTY_STRING,labels[i],
					B_FOLLOW_LEFT | B_FOLLOW_TOP);
		box->AddChild(sv);
		sv->SetAlignment(B_ALIGN_RIGHT);
		sv->SetViewColor(ViewColor());
		sv->SetHighColor(label_red);
		
		sv = new BStringView(lr,names[i],B_EMPTY_STRING,
					B_FOLLOW_LEFT | B_FOLLOW_TOP);
		box->AddChild(sv);
		sv->SetViewColor(ViewColor());
		
		br.OffsetBy(0,15);
		lr.OffsetBy(0,15);
	}
	TRACE();	
	
	br = r;
	br.OffsetTo(0,0);
	br.top += 16;
	br.bottom -= 6;
	br.left += r.Width()/2.0 - 16;
	br.right -= 6;
	lr = br;
	lr.bottom = lr.top + 12;

	TRACE();	
	sv = new BStringView(lr,"latestlabel","Newer Versions",
			B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(sv);
	
	br.top = lr.bottom + 4;

	TRACE();	
	
	box->AddChild(new UpdateDisplayView(br));
	
	TRACE();
/******
	sv = new BStringView(br,"latestlabel","Newest Available",
				B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(sv);
	sv->SetViewColor(ViewColor());
	br.OffsetBy(-30,18);
	
	char	*labels_new[] = {"Version:",
						 "Released:",
						 "Size:",
						 "Downloaded:"};
	char	*names_new[] = {"newvers",
						 "newrelease",
						 "newsize",
						 "newdled"};
						 
	br.right = br.left + 80;
	lr = br;
	lr.left = br.right + 5;
	lr.right = lr.left + 140;
	for (long i = 0; i < nel(labels_new); i++) {
		sv = new BStringView(br,B_EMPTY_STRING,labels_new[i],
				B_FOLLOW_LEFT | B_FOLLOW_TOP);
		box->AddChild(sv);
		sv->SetAlignment(B_ALIGN_RIGHT);
		sv->SetViewColor(ViewColor());
		sv->SetHighColor(label_red);
		
		sv = new BStringView(lr,names_new[i],B_EMPTY_STRING,
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
		box->AddChild(sv);
		sv->SetViewColor(ViewColor());
		
		br.OffsetBy(0,15);
		lr.OffsetBy(0,15);
	}
	/// Add URL view
	sv = new BStringView(br,B_EMPTY_STRING,"URL:",
				B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(sv);
	sv->SetAlignment(B_ALIGN_RIGHT);
	sv->SetViewColor(ViewColor());
	sv->SetHighColor(label_red);
	
	sv = new SUrlView(lr,"newurl",B_EMPTY_STRING,
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	box->AddChild(sv);
	sv->SetViewColor(ViewColor());
		
	TRACE();	
*****/

	TRACE();	

#if 0	
	r.top = r.bottom + 8;
	br = r;

	br.bottom = br.top + 14;
	br.left += 4;
	br.right = br.left + 14;
	TRACE();
	BControl *tab = new TriangleTab(br,B_EMPTY_STRING,
			B_EMPTY_STRING,new BMessage(M_MOREINFO),B_FOLLOW_LEFT | B_FOLLOW_TOP);
	TRACE();
	infbox->AddChild(tab);
	TRACE();
	tab->SetTarget(this);
	TRACE();
	// tab->SetValue(B_CONTROL_OFF);
	TRACE();
	br.left = br.right + 8;
	br.right += 100;
	
	sv = new NStringView(br,"infostr","More Information",
				B_FOLLOW_LEFT | B_FOLLOW_TOP);
	infbox->AddChild(sv);
	sv->SetViewColor(ViewColor());
	TRACE();
	br = Bounds();
	
	br.InsetBy(40,10);
	br.top = br.bottom - 70;
	
	box = new BBox(br,"moreinf",
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
				/* B_FULL_UPDATE_ON_RESIZE  |*/ B_WILL_DRAW);
	box->SetLowColor(ViewColor());
	//box->SetViewColor(light_gray_background);
	box->SetLabel("Support Information");
	AddChild(box);
	
	br.OffsetTo(0,0);
	br.InsetBy(20,16);
	br.right = br.left + 100;
	br.bottom = br.top + 14;
	lr = br;
	lr.OffsetBy(105,0);
	lr.right = box->Bounds().right - 10;
	
	sv = new NStringView(br,B_EMPTY_STRING,"Email:",
			B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(sv);
	sv->SetAlignment(B_ALIGN_RIGHT);
	sv->SetViewColor(ViewColor());
	sv->SetHighColor(label_red);
		
	SUrlView *uv;
	sv = uv = new SUrlView(lr,"suppemail",B_EMPTY_STRING,
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	uv->SetAppSig("application/x-adam-client");
	box->AddChild(sv);
	sv->SetViewColor(ViewColor());
	
	br.OffsetBy(0,15);
	lr.OffsetBy(0,15);

	sv = new BStringView(br,B_EMPTY_STRING,"Phone:",
			B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(sv);
	sv->SetAlignment(B_ALIGN_RIGHT);
	sv->SetViewColor(ViewColor());
	sv->SetHighColor(label_red);
		
	sv = new BStringView(lr,"suppphone",B_EMPTY_STRING,
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	box->AddChild(sv);
	sv->SetViewColor(ViewColor());
		
	br.OffsetBy(0,15);
	lr.OffsetBy(0,15);

	/// Add URL view
	sv = new BStringView(br,B_EMPTY_STRING,"Web:",
				B_FOLLOW_LEFT | B_FOLLOW_TOP);
	box->AddChild(sv);
	sv->SetAlignment(B_ALIGN_RIGHT);
	sv->SetViewColor(ViewColor());
	sv->SetHighColor(label_red);
	
	sv = new SUrlView(lr,"suppurl",B_EMPTY_STRING,
				B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	box->AddChild(sv);
	sv->SetViewColor(ViewColor());
	box->Hide();	
#endif
	/** bugs in BeKit
	BRect winf = Window()->Frame();
	BPoint winr = ConvertToScreen(BPoint(0,0));
	Window()->SetSizeLimits(winr.x - winf.left,800,200,8192);
	**/
	
	Window()->SetSizeLimits(Bounds().Width(),8192,300,8192);
}


void formatTime(char *fbuf, ulong ftime, bool full);
void formatTime(char *fbuf, ulong ftime, bool full)
{
	struct tm *tp;
	char buf1[24];
	
	tp = localtime((long *)&ftime);
	if (full)
		strftime(buf1,24,"%b %d %Y, %I:%M%p",tp);
	else
		strftime(buf1,24,"%b %d %Y",tp);
	strcpy(fbuf,buf1);
	//sprintf(fbuf,buf1,tp->tm_mday);
}

void ManagerView::HandleSelection(PackageItem *it, bool multiple)
{
	curSel = it;
	mulSel = multiple;

	bool en = (curSel != NULL);
	for (long i = 0; i <= 1; i++)
		btns[i]->SetEnabled(en);
	
	if (mulSel) {
		btns[0]->SetEnabled(TRUE);
		btns[1]->SetEnabled(TRUE);
	}
	else if (curSel) {
		btns[0]->SetEnabled(curSel->data.FindBool("regservice"));
		btns[1]->SetEnabled(curSel->data.FindBool("upservice"));
	}
		
	BStringView *sv;
	sv = (BStringView *)FindView("curvers");
	sv->SetText(en ? it->data.FindString("version") : B_EMPTY_STRING);

	char timBuf[40];
	
	sv = (BStringView *)FindView("currelease");
	if (en)
		formatTime(timBuf,(uint32)it->data.FindInt32("releasedate"),false);
	else
		*timBuf = 0;
		
	sv->SetText(timBuf);

	sv = (BStringView *)FindView("curacquired");
	if (en)
		formatTime(timBuf,(uint32)it->data.FindInt32("installdate"),true);
	else
		*timBuf = 0;
	sv->SetText(timBuf);

	int32 registered = en ? it->data.FindInt32("registered") : 0;
	sv = (BStringView *)FindView("curregister");
	sv->SetText(en ? (	(registered == PackageItem::REGISTERED_YES) ? "Yes" : 
						(registered == PackageItem::REGISTERED_NO) ? "No" :
						"In Process") : B_EMPTY_STRING);

	sv = (BStringView *)FindView("serialno");
	
	const char *sid = en ? it->data.FindString("sid") : 0;
	if (!sid || !*sid) sid = en ? "N/A" : B_EMPTY_STRING;
	sv->SetText(sid);
	
	// update display view
	UpdateDisplayView *uv = (UpdateDisplayView *)FindView("updates");
	sv = (BStringView *)FindView("latestlabel");
	sv->SetText("Newer Versions");

	if (!it) {
		uv->SetItemList(0);
	}
	else {
		if (! it->data.FindBool("upservice")) {
			uv->SetItemList(0,"Updating not supported...");
		}
		else {
			// these updates have been viewed!!!
			uv->SetItemList(it->Updates());
			time_t uptime;
			uptime = it->updates.FindInt32("lastcheckdate");
			if (uptime) {		
				char buf[128];		
				struct tm *tp = localtime(&uptime);
				strftime(buf,128,"Newer Versions (as of %a, %b %d, %I:%M%p)",tp);
				sv->SetText(buf);
			}
		}
	}
}

void ManagerView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case 'BkTo': {
			// backup to
			if (msg->HasRef("directory")) {
				entry_ref	dirRef;
				msg->FindRef("directory",&dirRef);
				BackupRegistry(dirRef,msg->FindString("name"));
				doError("Backup Complete.");
			}
			else {
				restorePanel.SendMessage(B_QUIT_REQUESTED);
				if (!TryActivate(backupPanel)) {
					MFilePanel *pan = new BackupPanel(true,new BMessenger(this));
					backupPanel = BMessenger(pan->Window());
					pan->Show();
				}
			}
			break;
		}
		case 'RsFr': {
			// restore from
			if (msg->HasRef("refs")) {
				doError("Restoring...");
			}
			else {
				backupPanel.SendMessage(B_QUIT_REQUESTED);
				if (!TryActivate(restorePanel)) {
					MFilePanel *pan = new BackupPanel(false,new BMessenger(this));
					restorePanel = BMessenger(pan->Window());
					pan->Show();
				}
			}
			break;
		}
		case 'RstS': {
			// reset services
			if (!curSel)
				break;
				
			ReplaceBool(&curSel->data,"regservice",true);
			ReplaceBool(&curSel->data,"upservice",true);
			
			pDB->WritePackage(curSel,PackageDB::BASIC_DATA);
			
			HandleSelection(curSel,false);
			break;
		}
		case 'GetU': {
			// get update button clicked
			if (!curSel)
				break;
	
			BMessage	data;
	
			data.AddString("pid",curSel->data.FindString("pid"));
			data.AddString("sid",curSel->data.FindString("sid"));
			data.AddString("vid",msg->FindString("vid"));

			new GetUpdateDialog(&data);
			break;
		}
//		case M_TROLL: {
//			// troll for all
//			MThread *t = new TrollThread();
//			t->Run();
//			break;
//		}
		case ManagerWindow::PKG_DISPLAY: {
			// update/add to display
			PRINT(("Adding new package to manager\n"));
			PackageDB db;
			
			BMessage	cmsg(M_ITEM_MODIFIED);
			bool	addedNew = false;
			bool	changedExisting = false;
			ulong type;
			long mcount;
			long count = fItems.CountItems();
			msg->GetInfo("refs",&type,&mcount);
			for (long m = 0; m < mcount; m++) {
				entry_ref r;
				msg->FindRef("refs",m,&r);
				long i;
				for (i = 0; i < count; i++) {
					PackageItem	*it = fItems.ItemAt(i);
					
					// item found in existing list
					if (it->fileRef == r) {
						// read in all new data
						BEntry ent(&r);
						PackageItem newItem;
						if (db.ReadPackage(&newItem,&ent) >= B_OK) {
							// assign
							bool tsel = it->selected;
							*it = newItem;
							it->selected = tsel;
							cmsg.AddPointer("item",it);
							// redisplay flag
							changedExisting = true;
						}
						break;
					}
				}
				if (i == count) {
					// item not found, add new item
					BEntry	ent(&r);
					PackageItem *newItem = new PackageItem();
					if (!db.ReadPackage(newItem, &ent)) {
						// add at the front
						fItems.AddItem(newItem,0);
						// re-filter display???
						addedNew = true;
					}
					else {
						// delete existing
						delete newItem;
					}
				}
			}
			if (addedNew) {
				BMenuBar *mbar = (BMenuBar *)Window()->FindView("menubar");
				BMenu *menu = mbar->SubmenuAt(2);
				BMenuItem *it = menu->FindItem("Latest Versions");
				UpdateViewList(it->IsMarked());
			}
			else if (changedExisting) {
				// need to make this work with multiple items
			
				Looper()->PostMessage(&cmsg,FindView("listing"));
			}
			break;
		}
		case M_FILTER_DISPLAY: {		
			BMenuBar *mbar = (BMenuBar *)Window()->FindView("menubar");
			BMenu *menu = mbar->SubmenuAt(2);
			BMenuItem *it = menu->FindItem("Latest Versions");
			it->SetMarked(!it->IsMarked());
			UpdateViewList(it->IsMarked());
			break;
		};
		case M_ITEMS_SELECTED:
			if (msg->HasPointer("item")) {
				PackageItem *item;
				msg->FindPointer("item",(void **)&item);
				HandleSelection(item,FALSE);
			}
			else {
				HandleSelection(NULL,msg->FindBool("multiple"));
			}
			break;
		/***
		case M_MOREINFO:
			BControl *c;
			msg->FindPointer("source",(void **)&c);
			
			ASSERT(c);
			
			long value = c->Value();
			
			BView *infbox = FindView("infobox");
			BView *lview = FindView("scroller");
			BView *sbar = FindView("hscroll");
			infbox->SetResizingMode(B_FOLLOW_NONE);
			lview->SetResizingMode(B_FOLLOW_NONE);
			sbar->SetResizingMode(B_FOLLOW_NONE);
			long expand;
			
			if (value == B_CONTROL_ON) {
				// expand the window
				expand = 80;
				FindView("moreinf")->Show();
			}
			else {
				// collapse the window
				expand = -80;
				FindView("moreinf")->Hide();
			}
			float minh, maxh, minv, maxv;
			Window()->GetSizeLimits(&minh, &maxh, &minv, &maxv);
			Window()->SetSizeLimits(minh,maxh,minv+expand,maxv);

			Window()->ResizeBy(0,expand);
						
			infbox->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
			lview->SetResizingMode(B_FOLLOW_ALL);
			sbar->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
			break;
		***/
		case M_REGISTER:
			PRINT(("got register\n"));
			if (mulSel) {
				// multiple items are selected
				BAlert *a = new BAlert(B_EMPTY_STRING,"Would you like to update registration\
 information for all of the selected packages?","No","Yes");
				PositionWindow(a,0.5,0.25);
				int32 res = a->Go();
				if (res == 1) {
					BWindow *w = new RegisterWindow(NULL,BMessenger(lv));
				}
			}
			else if (curSel && !TryActivate(curSel->regWind)) {
				bool updateReg = FALSE;
				int32 registered = curSel->data.FindInt32("registered");
				if (registered != PackageItem::REGISTERED_NO) {
					char buf[B_FILE_NAME_LENGTH+80];
					sprintf(buf,"The package \"%s\" has already been registered. Would you like to\
 update your registration information?",curSel->data.FindString("package"));
					BAlert *a = new BAlert(B_EMPTY_STRING,buf,"Yes","No");
					PositionWindow(a,0.5,0.25);
					int32 res = a->Go();
					if (res == 0) {
						updateReg = TRUE;
					}
				}
				if (registered == PackageItem::REGISTERED_NO || updateReg) {
					BWindow *w = new RegisterWindow(curSel,BMessenger(lv));
					curSel->regWind = BMessenger(w);
				}
			}
			break;
		case M_UPDATE: {
			if (!msg->HasInt32("option")) {
				new UpdateDialog(lv);
				//UpdateDialog *alert = 
				//alert->Go(new BInvoker(new BMessage(msg->what),this));
				// alert->Show();
				break;
			}
			break;
		}
		case M_REMOVE: {
			if (lv->LowSelection() < 0)
				break;
			long res = doError("Important: This will remove the package information from the \
management list. Use this option to remove information displayed for a version no longer in use. \
Note that this will not remove any files which may still be installed.\n\n\
If you choose to remove the information you will not be able to receive updates.\n","",
 					"Remove","Don't Remove");
 			if (res != 0)
 				break;
 			bool updateView = false;
			for (int i = lv->LowSelection() ; i < lv->CountItems() ;  )
			{
				PackageItem *curItem = (PackageItem *)lv->ItemAt(i);
				if (!curItem->selected) {
					i++;
					continue;
				}
				if (curItem->uinstWind.IsValid() ||
					curItem->regWind.IsValid() )
				{
					if (lv->LowSelection() == lv->HighSelection())
						doError("You can't remove an entry while trying to register it.");
					i++;
					continue;
				}
				res = doError("Are you sure you want to remove %s?\n",
				curItem->data.FindString("package"),
				"Remove","Don't Remove");
					
				// remove from the master list
				// also remove from disk	
				
				if (res == 0)
				{
					BEntry	entry(&curItem->fileRef);
					entry.Remove();
					fItems.RemoveItem(curItem);
					lv->ItemList()->RemoveIndex(i);
					updateView = true;
					delete curItem;
				}
				else {
					i++;
				}
			}
			if (updateView)
			{
				lv->Update();
				lv->SelectNoItems();
				lv->SelectionSet();
			}
			break;
		}
#if 0
		case M_UNINSTALL: {
			if (!curSel)
				break;
		
			if (!TryActivate(curSel->uinstWind))
				curSel->uinstWind = BMessenger(
					new UninstallWindow(curSel->data.FindString("package"),
					BMessenger(lv))
					);
			
			break;
		}
		// remove item from display
		// used by uninstall
		case 'PRmv': {
			bool removed = false;
			entry_ref r;
			msg->FindRef("refs",&r);
			for (long i = fItems.CountItems()-1; i >= 0; i--) {
				PackageItem	*it = fItems.ItemAt(i);
				if (it->fileRef == r) {
					BEntry	entry(&r);
					entry.Remove();
					delete fItems.RemoveIndex(i);
					removed = true;
					break;
				}
			}
			if (removed) {
				BMessage filter(M_FILTER_DISPLAY);
				MessageReceived(&filter);
			}
			break;
		}
#endif
		case M_SETTINGS: {
			ValetApp *app = (ValetApp *)be_app;
			app->valetWindMess.SendMessage('Sett');
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}
