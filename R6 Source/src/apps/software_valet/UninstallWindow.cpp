
#include "UninstallWindow.h"
#include "LabelView.h"

#include "UninstallTree.h"
#include "UninstallThread.h"

#include "PackageItem.h"
#include "PackageDB.h"
#include "PackData.h"

#include "DoIcons.h"
#include "Util.h"

#include "MyDebug.h"
#include <ScrollView.h>
#include <StatusBar.h>
#include <Button.h>
#include <Query.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Alert.h>


/*****************************************************/

UninstallWindow::UninstallWindow(const char *pkname, BMessenger &listView)
	:	BWindow(BRect(0,0,300,400),B_EMPTY_STRING,
					B_TITLED_WINDOW,
					0/*NULL*/,0/*NULL*/)
{
	Lock();
	PositionWindow(this,0.4,0.2);
	
	long len = strlen(pkname);
	char *buf = new char[len+2+10+1];
	sprintf(buf,"Uninstall \"%s\"",pkname);
	SetTitle(buf);
	delete buf;
	
	UninstallView *uv = new UninstallView(Bounds(),listView);
	AddChild(uv);
	AddShortcut('A',B_COMMAND_KEY,new BMessage(T_SELECT_ALL),FindView("uninsttree"));
	

	
	uv->FindFiles(pkname);
	
	Show();
	Unlock();
	

}

bool UninstallWindow::QuitRequested()
{
	// don't quit until done!!
	bool canQuit =
		(((UninstallView *)FindView("uninstview"))->uninstThread == NULL);
	
	return canQuit;
}


/*****************************************************/
#pragma mark -----uninstall view-----


UninstallView::UninstallView(BRect frame, BMessenger &listView)
	:	BView(frame,"uninstview",B_FOLLOW_ALL,B_WILL_DRAW),
		uninstThread(NULL),
		it(NULL),
		flistView(listView)
{
}

UninstallView::~UninstallView()
{
	delete it;
}

void UninstallView::AttachedToWindow()
{
	BView::AttachedToWindow();	
	SetViewColor(light_gray_background);
	
	BRect r = Bounds();
	r.InsetBy(8,8);
	
	r.left += 40;
	r.bottom = r.top + 26;
	
	AddChild(new LabelView(r,"The following items were found to uninstall. \
Clicking the \"Remove\" button will delete the checked items. Any empty folders \
will be removed as well."));

	r.left -= 40;
	
	r.top = r.bottom + 12;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom = Bounds().bottom - 90;
	
	uTree = new UninstallTree(r);
	BScrollView *sv = new BScrollView("scroller",uTree,B_FOLLOW_ALL,
				0/*NULL*/, TRUE, TRUE, B_FANCY_BORDER);
	AddChild(sv);
	// Let the TreeView know about the scrollbars
	// do this automatically
	uTree->SetScrollBars(sv->ScrollBar(B_HORIZONTAL), sv->ScrollBar(B_VERTICAL));
	uTree->MakeFocus(TRUE);
	
					
	r.bottom += B_H_SCROLL_BAR_HEIGHT;
	r.right += B_V_SCROLL_BAR_WIDTH;
	r.top = r.bottom + 6;
	r.bottom += 24;
	
	BStatusBar *sb = new BStatusBar(r,"status","Removing...");
	sb->SetResizingMode(B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT);
	AddChild(sb);
	sb->SetBarHeight(10);
	sb->Hide();
	
	r.top = r.bottom + 16;
	
	r.right -= 6;
	r.bottom = r.top + 20;
	r.left = r.right - 80;
	
	BButton *btn;
	btn = new BButton(r,"rmbtn","Remove",new BMessage((ulong)U_REMOVE),
				B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(btn);
	btn->MakeDefault(TRUE);
	btn->SetFont(be_plain_font);
	btn->SetTarget(this);
	
	r.right = r.left - 36;
	r.left = r.right - 80;
	btn = new BButton(r,"cancelbtn","Cancel",new BMessage(U_CANCEL),
				B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	AddChild(btn);
	btn->SetFont(be_plain_font);
	btn->SetTarget(this);
	
	Window()->SetSizeLimits(Window()->Frame().Width(),8192,
						Window()->Frame().Height(),8192);
}

void UninstallView::FindFiles(const char *pkname)
{
	PRINT(("UninstallView::FindFiles\n"));
	PackageDB	pdb;
	it = new PackageItem();
	
	if (pdb.FindPackage(it,pkname) == B_NO_ERROR) {
		PRINT(("got package item!!\n"));
		BFile	regFile(&it->fileRef,O_RDONLY);
		if (regFile.InitCheck())
			return;
		
		regFile.Seek(16,SEEK_SET);
		PackData	pData;
		
		record_header	header;
		pData.ReadRecordHeader(&regFile,&header);
		if (header.type != LIS_TYPE) {
			PRINT(("bad list header!\n"));
			return;
		}
		char tagvalue[40];
		BQuery	query;
		BVolume	qvol;
		regFile.GetVolume(&qvol);
		
		bigtime_t	startt = system_time();
		int count = 0;
		while (1) {
			pData.ReadRecordHeader(&regFile,&header);
			if (header.type != STR_TYPE) {
				PRINT(("not string type!\n"));
				break;
			}
			pData.ReadString(&regFile,tagvalue,40);
			PRINT(("tag value is %s\n",tagvalue));
			
			query.Clear();
			query.SetVolume(&qvol);
			query.PushAttr("pkgtag");
			query.PushString(tagvalue);
			query.PushOp(B_EQ);
			
			if (!query.Fetch())
				PRINT(("fetched query\n"));
			else
				PRINT(("fetch failed\n"));
			
			count++;
			
			BEntry	foundEnt;
			while (query.GetNextEntry(&foundEnt) == B_NO_ERROR) {
				PRINT(("got entry\n"));
				BPath	path;
				BNode	node(&foundEnt);
				BNodeInfo	ninf(&node);
				BBitmap *bmap = new BBitmap(BRect(0,0,B_MINI_ICON-1,B_MINI_ICON-1),
								B_COLOR_8_BIT);
				if (ninf.GetTrackerIcon(bmap, B_MINI_ICON))
					bmap->SetBits(gGenericFileSIcon->Bits(),bmap->BitsLength(),0,B_COLOR_8_BIT);
				foundEnt.GetPath(&path);
				uTree->AddFilePath(path.Path(),bmap);
			}
		}
		startt = system_time()- startt;
		float secs = (float)startt/(float)(1000*1000);
		printf("elapsed time %f sec, %d files, %f files/sec\n",
				secs, count, (float)(count/secs));
		uTree->CollapseBig(uTree->Root());
		uTree->RecalculatePositions();
	}
	else {
		PRINT(("didn't get package item!!\n"));
	}
}


void UninstallView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case U_REMOVE: {
			if (uninstThread)
				return;
			
			BStatusBar *sb = (BStatusBar *)FindView("status");
			sb->Show();
			sb->SetMaxValue(100);
			
			uninstThread = new UninstallThread(this, uTree->Root());	
			
			break;
		}
		case U_PROGRESS: {
			BStatusBar *sb = (BStatusBar *)FindView("status");
			sb->Update(msg->FindInt32("value"));
			break;
		}
		case U_DONE: {
			BAlert *a;
			bool canceled = uninstThread->Cancelled();
			if (canceled)
				a = new BAlert(B_EMPTY_STRING,"Canceled","OK");
			else
				a = new BAlert(B_EMPTY_STRING,"Finished","OK");
				
			a->Go();
			FindView("status")->Hide();
			
			delete uninstThread;
			uninstThread = NULL;
			if (!canceled) {
				if (it) {
					BMessage	m('PRmv');
					m.AddRef("refs",&it->fileRef);
					flistView.SendMessage(&m);
				}
				
				Looper()->PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		case U_CANCEL: {
			if (uninstThread) {
				uninstThread->Cancel();
			}
			else {
				Looper()->PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void UninstallView::Draw(BRect up)
{
	BView::Draw(up);
	
	SetDrawingMode(B_OP_OVER);
	DrawBitmapAsync(gYellowWarnIcon,BPoint(8,8));
	SetDrawingMode(B_OP_COPY);
}
