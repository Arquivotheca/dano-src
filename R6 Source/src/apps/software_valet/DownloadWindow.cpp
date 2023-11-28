// DownloadWindow.cpp

#include "DownloadWindow.h"
#include "SettingsManager.h"
#include "DoIcons.h"
#include "VHTUtils.h"
#include "ProgressBar.h"
#include <errno.h>
#include <ctype.h>
#include "AutoPtr.h"
#include "Util.h"
#include "Log.h"
#include "ValetApp.h"

#define DEBUG 0
#include <Debug.h>
#include <View.h>
#include <Window.h>
#include <Application.h>
#include <ListView.h>
#include <Button.h>
#include <ScrollBar.h>
#include <Directory.h>
#include <ScrollView.h>
#include <Path.h>
#include <NodeInfo.h>
#include <fs_attr.h>
#include <stdlib.h>
#include <string.h>
//#include "MyDebug.h"

extern SettingsManager *gSettings;
status_t readline(FILE *f, char *buf, int bufsize);
extern const char *kUPkgSig;
extern const char *kPPkgSig;


/*
DownloadWindow --
User interface for viewing and controlling downloads
*/
DownloadWindow::DownloadWindow()
	:	BWindow(BRect(0,0,470,160),"Download Manager",
				B_TITLED_WINDOW,
				0 /*NULL*/)
{
	Lock();
	PositionWindow(this,gSettings->data.FindPoint("general/dlwindow"),0.4,0.4);
	
	BRect r = Bounds();
	
	BView *v = new DownloadView(r);
	AddChild(v);

	SetPreferredHandler(v);
	
	SetSizeLimits(360,490,120,8192);
		
	Unlock();
	Show();
	((ValetApp *)be_app)->downloadManager->PostMessage('GetA');
}

/* all update messages are sent to the DownloadWindow
	messages referring to a particular download contain
	an int32 "id" field which is used to look up the
	appropriate item in the display list
*/
void DownloadWindow::MessageReceived(BMessage *msg)
{
	BListView *lv = (BListView *)FindView("listview");
	switch (msg->what) {
		case 'Inst' : {
			int32 ix;
			if ((ix = lv->CurrentSelection()) >= 0) {
				DlListItem *it = (DlListItem *)lv->ItemAt(ix);
				if (it->fileRef.name) {
					BMessage i(B_REFS_RECEIVED);
					i.AddRef("refs",&it->fileRef);
					be_app->PostMessage(&i);
				}
			}
   			break;
		}
		case 'ISel': {
			// selection changed in the list view
			
			// enable and disable the buttons
			bool sel = msg->HasInt32("index");
			
			BButton *btn;
			btn = (BButton *)FindView("cancel");
			btn->SetEnabled(sel);
			
			int32 dStatus = 0;
			int32 ix;
			if ((ix = lv->CurrentSelection()) >= 0) {
				DlListItem *it = (DlListItem *)lv->ItemAt(ix);
				dStatus = it->fStatus;
			}
			btn = (BButton *)FindView("defer");
			btn->SetEnabled(dStatus == DlItem::ITEM_DOWNLOADING | 
							dStatus == DlItem::ITEM_QUEUED);
			
			btn = (BButton *)FindView("activate");
			btn->SetEnabled(dStatus == DlItem::ITEM_DEFERRED |
							dStatus == DlItem::ITEM_ERROR);
			
			btn = (BButton *)FindView("install");
			btn->SetEnabled(dStatus == DlItem::ITEM_COMPLETE);
			
			break;
		}
		case B_CANCELED:
		case 'ActV':
		case 'DefR': {
			// activate defer or cancel an item
			if (!lv) break;
			
			int32 ix;
			if ((ix = lv->CurrentSelection()) >= 0) {
				DlListItem *it = (DlListItem *)lv->ItemAt(ix);
				msg->AddInt32("id",it->fID);
				((ValetApp *)be_app)->downloadManager->PostMessage(msg);
			}
			break;
		}
		case 'Stat': {
			// item to update/add to display
		
			// (for example a download was completed)
			if (!lv) break;
		
			type_code	type;
			int32		count;
			msg->GetInfo("id",&type,&count);
			int32		i;
			for (i = 0; i < count; i++) {
				int32 rID = msg->FindInt32("id",i);
				int32 max = lv->CountItems();
				int32 ix;
				for (ix = 0; ix < max; ix++) {
					DlListItem *it = (DlListItem *)lv->ItemAt(ix);
					if (it->fID == rID) {
						// we found it
						entry_ref ref;
						
						int32 oldStatus = it->fStatus;
						it->SetTo(msg, i);
						
						if (oldStatus != it->fStatus) {
							it->DrawItem(lv,lv->ItemFrame(ix),true);
							lv->Invoke(lv->SelectionMessage());
						}
						else
							it->DrawProgress(lv,lv->ItemFrame(ix));
						//BRect			ItemFrame(int32 index);
						//lv->InvalidateItem(ix);
						break;
					}
				}
				if (ix >= max) {
					// didn't find it
					DlListItem *nit = new DlListItem(rID);
					lv->AddItem(nit);
					nit->SetTo(msg, i);
					//if (lv->CurrentSelection() < 0) {
					lv->Select(max);
					lv->ScrollToSelection();
					//}
				}		
			}
			
			break;
		}
		case 'RmvI': {
			// item to remove from the display
		
			// (for example a download was completed)
			if (!lv) break;
			
			int32 rID = msg->FindInt32("id");
			int32 count = lv->CountItems();
			for (int i = 0; i < count; i++) {
				DlListItem *it = (DlListItem *)lv->ItemAt(i);
				if (it->fID == rID) {
					lv->RemoveItem(i);
					break;
				}
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}	
}

/*
DownloadView --
container view for scrollView, listView and buttons
*/
DownloadView::DownloadView(BRect r)
	:	BView(r,"dlview",B_FOLLOW_ALL,B_WILL_DRAW | B_FRAME_EVENTS)
{
}

void DownloadView::FrameResized(float w, float h)
{
	BView::FrameResized(w,h);
	
	BListView *lv = (BListView *)FindView("listview");
	// adjust the horizontal scroll bar based on the maximum
	// width allowed for the list items
	BScrollBar *hs = lv->ScrollBar(B_HORIZONTAL);
	hs->SetRange(0,400 - lv->Bounds().Width());
	hs->SetProportion(lv->Bounds().Width() / 400.0);
}

void DownloadView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(light_gray_background);
	
	BRect r = Bounds();

	// create the list view	
	r.bottom -= 45;
	
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	
	BListView *lv = new BListView(r,"listview",
			B_MULTIPLE_SELECTION_LIST,
			B_FOLLOW_ALL);
	lv->SetSelectionMessage(new BMessage('ISel'));
	
	// place it inside a scroll view
	BScrollView *sv = new BScrollView(B_EMPTY_STRING,lv,
			B_FOLLOW_ALL, 0, true, true, B_PLAIN_BORDER);
	
	AddChild(sv);

	// add some buttons
	r.top = r.bottom + B_H_SCROLL_BAR_HEIGHT + 12;
	r.left += 8;
	
	const int kButtonWidth = 74;
	const int kButtonSep = kButtonWidth + 4;
	BRect br = r;
	br.bottom = br.top + 20;
	br.right = br.left + kButtonWidth;
	BButton *btn;
	
	BRect leftbr = br;
	
	btn = new BButton(br,"cancel","Cancel",
					new BMessage(B_CANCELED),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(btn);
	btn->SetEnabled(false);
	
	br.OffsetBy(kButtonSep,0);
	btn = new BButton(br,"defer","Defer",
					new BMessage('DefR'),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(btn);
	btn->SetEnabled(false);
	
	br.OffsetBy(kButtonSep,0);
	btn = new BButton(br,"activate","Activate",
					new BMessage('ActV'),
					B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	AddChild(btn);
	btn->SetEnabled(false);

	br.right = Bounds().right -12;
	br.left = br.right - kButtonWidth;
	btn = new BButton(br,"install","Install",
					new BMessage('Inst'),
					B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	AddChild(btn);
	btn->SetEnabled(false);
	btn->MakeDefault(true);

	// adjust the horizontal scroll bar based on the maximum
	// width allowed for the list items
	FrameResized(lv->Bounds().Width(),lv->Bounds().Height());
}


ProgressBar *DlListItem::pBar = NULL;
 
DlListItem::DlListItem(int32 id)
	:	BListItem(),
		fID(id),
		fStatus(0),
		totalBytes(0),
		bytesCompleted(0),
		bytesDownloaded(0),
		secondsElapsed(0)
{
	if (!pBar) pBar = new ProgressBar();
}

void DlListItem::SetTo(BMessage *info, int32 i)
{
	//entry_ref ref;
	if (info->HasRef("refs",i)) {
		entry_ref	ref;
		status_t err = info->FindRef("refs",i,&ref);
		PRINT(("DlListItem::SetTo fileRef.name is %s, device %d, directory %Ld at index %d\nerr is %s\n",
			ref.name,ref.device,ref.directory,i,strerror(err)));
		fileRef = ref;
		if (!fileRef.name && info->HasString("name",i)) {
		 	fileRef.set_name(info->FindString("name",i));
		}
	}
	info->FindInt32("status",i,&fStatus);
	info->FindInt64("totalbytes",i,&totalBytes);
	info->FindInt64("bytesdone",i,&bytesCompleted);
	info->FindInt64("bytesdown",i,&bytesDownloaded);
	info->FindFloat("time",i,&secondsElapsed);
}

void DrawStringClipped(BView *v, const char *str,BPoint where,float len);

void DlListItem::DrawProgress(BView *owner, BRect itemRect)
{
	const int	iconLeft = 2;
	const int	nameLeft = iconLeft + 32 + 8;
	const int	statLeft = nameLeft + 115 + 8;
	const int	barLeft = statLeft + 80 + 8;	
	const int	timeLeft = barLeft + 120 + 8;
	
	BRect r;
	r.left = barLeft;
	r.bottom = itemRect.bottom - 10;
	r.top = r.bottom - 12;
	r.right = r.left + 120;

	// progressOnly
	float curProgress;
	if (totalBytes <= 0)
		curProgress = 0.0;
	else
		curProgress = (float)bytesCompleted/(float)totalBytes;
	//if (curProgress - l
	
	pBar->Draw(owner,r,curProgress);
	
	if (bytesDownloaded > 0 && fStatus == 1) {
		
		
		r.left = timeLeft;
		r.bottom = itemRect.bottom - 10;
		r.top = itemRect.top + 8;
		r.right = r.left + 120;
		owner->FillRect(r, B_SOLID_LOW);
		
		char buf[80];
		
		BPoint	p = r.LeftBottom();
		p.y -= 2;
		
		PRINT(("sec %f, bytes done %Ld, bytes remaining %Ld\n",
				secondsElapsed,bytesDownloaded,totalBytes - bytesCompleted));
		
		time_t	eta = (time_t)(secondsElapsed * ((float)(totalBytes - bytesCompleted)/bytesDownloaded));
		if (eta > 3600) {
			int hr = eta/3600;
			int mn = (eta - hr*3600)/60;
			if (mn)
				sprintf(buf,"ETA: %d hr",hr);
			else
				sprintf(buf,"ETA: %d hr %d min",hr,mn);
		}
		else  { //if (eta > 0) {
			int mn = eta/60;
			if (mn)
				sprintf(buf,"ETA: %d min",mn);
			else
				sprintf(buf,"ETA: < 1 min");
		}
		owner->DrawString(buf,p);
		if (IsSelected()) {
			owner->SetDrawingMode(B_OP_SUBTRACT);
			owner->SetHighColor(60,60,60);
			owner->FillRect(r);
			owner->SetDrawingMode(B_OP_COPY);
			owner->SetHighColor(0,0,0);
		}
	}
}

void DlListItem::DrawItem(BView *owner, BRect itemRect, bool complete)
{
	// draw the list item
	// currently all we have is an icon and the id
	
	// we can eventually draw text, status information and
	// a progress bar
	
	const int	iconLeft = 2;
	const int	nameLeft = iconLeft + 32 + 8;
	const int	statLeft = nameLeft + 115 + 8;
	const int	barLeft = statLeft + 80 + 8;
	const int	timeLeft = barLeft + 120 + 8;
	
	if (complete) {
		owner->FillRect(itemRect,B_SOLID_LOW);
	}
	
	BPoint p = itemRect.LeftTop();
	p.x = iconLeft;
	p.y += 1;
	owner->DrawBitmapAsync(gPkgIcon, p);

	p = itemRect.LeftBottom();
	p.x = nameLeft;
	p.y -= 12;
	if (fileRef.name) {
		DrawStringClipped(owner, fileRef.name,p,115);
	}
	
	char *kStats[] = {"Unknown",
					"Downloading...",
					"Queued",
					"Deferred",
					"Completed",
					"Canceled",
					"Error"};

	p.x = statLeft;
	if (fStatus < 0 || fStatus > 6)
		fStatus = 6;
		
	owner->DrawString(kStats[fStatus],p);

	if (IsSelected()) {
		owner->SetDrawingMode(B_OP_SUBTRACT);
		owner->SetHighColor(60,60,60);
		owner->FillRect(itemRect);
		owner->SetDrawingMode(B_OP_COPY);
		owner->SetHighColor(0,0,0);
	}

	owner->StrokeLine(itemRect.LeftBottom(),itemRect.RightBottom());
	
	DrawProgress(owner,itemRect);
}

void DlListItem::Update(BView *owner, const BFont *font)
{
	// override to control the list item height
	owner; font;
	//BListItem::Update(owner, font);
	SetHeight(32);
}



//////////////////////////////////////////////////////
/*
DownloadManager --
A BLooper to manage the download queue

this looper is created when Valet starts up
it is global to the whole app

it receives messages to control the download queue

currently it supports 1 download at a time (although I think
it is pretty easy to expand it)

the items in the download manager list are separate from
those in the display window

the display window can come and go and the downloads can
continue silently
all status updates are sent to the window via a BMessenger
so if the window isn't there (ie the messenger is invalid) then
the message sends simply fail
*/

DownloadManager::DownloadManager()
	:	BLooper(),
		currentItem(0),
		fID(0),
		fPaused(false)
{
}

void DownloadManager::UpdateStatus(DlItem *it,
								entry_ref *ref)
{
	// we may know the file, name, or none of that changed
	BMessage stat('Stat');
	
	if (ref) {
		stat.AddRef("refs",ref);
		PRINT(("DownloadManager::UpdateStatus ref filename is %s\
 device %d, directory %Ld\n",ref->name,ref->device,ref->directory));
 		if (ref->name) stat.AddString("name",ref->name);
	}
	stat.AddInt32("status",it->fStatus);
	stat.AddInt64("totalbytes",it->totalBytes);
	stat.AddInt64("bytesdone",it->bytesCompleted);
	stat.AddInt64("bytesdown",it->bytesDownloaded);
	stat.AddFloat("time",it->secondsElapsed);
	stat.AddInt32("id",it->ID());
	
	statusWindow.SendMessage(&stat);
}

// check the queue for anything to download
void DownloadManager::CheckQueue()
{
	DlItem	*nit = 0;
	if (currentItem == NULL && !fPaused) {
		// if we're not busy, find a waiting item
		// and spawn it
		
		//int32 currentlyDownloading = 0;
		int32 count = list.CountItems();
		for (int i = 0; i < count; i++) {
			DlItem *it = list.ItemAt(i);
			if (it->fStatus == DlItem::ITEM_QUEUED) {
				nit = it;
				break;
			}
		}
	}
	if (nit) {
		const char *dlpath = gSettings->data.FindString("download/path");
		BDirectory	dir(dlpath);
		if (dir.InitCheck() < B_OK) {
			create_directory(dlpath, 0777);
			dir.SetTo(dlpath);
		}
		if (dir.InitCheck() < B_OK) {
			doError("Could not begin the download because the specified download directory could not be found.");
			// update the view
			nit->fStatus = DlItem::ITEM_ERROR;
		}
		else if (nit->BeginDownload(&dir) >= B_OK)
		{
			// if no error starting the download
			// then update the status of the item
			nit->fStatus = DlItem::ITEM_DOWNLOADING;			
			currentItem = nit;
		}
		else {
			// update the view		
			nit->fStatus = DlItem::ITEM_ERROR;
		}
		UpdateStatus(nit,&nit->fileRef);
	}
}

void DownloadManager::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case 'GetA' : {
			BMessage stat('Stat');
			
			// loop through all items
			int32 count = list.CountItems();
			for (int i = 0; i < count; i++) {
				DlItem	*it = list.ItemAt(i);
				
				stat.AddRef("refs",&it->fileRef);
				stat.AddInt32("status",it->fStatus);
				stat.AddInt64("totalbytes",it->totalBytes);
				stat.AddInt64("bytesdone",it->bytesCompleted);
				stat.AddInt64("bytesdown",it->bytesDownloaded);
				stat.AddFloat("time",it->secondsElapsed);
				stat.AddInt32("id",it->ID());
		
				statusWindow.SendMessage(&stat);
			}
			break;
		}
		case 'Stop' : {
			// stop the queue
			if (currentItem) currentItem->Cancel();
			currentItem->fStatus = DlItem::ITEM_QUEUED;
			fPaused = true;
			break;
		}
		case 'Star' : {
			// start the queue
			fPaused = false;
			CheckQueue();
			break;
		}
		case 'DefR':
		case B_CANCELED: {
			// find the item to cancel
			int32 id = msg->FindInt32("id");
			
			// find the unique item to cancel
			for (int i = list.CountItems()-1; i >= 0; i--) {
				DlItem	*it = list.ItemAt(i);
				if (it->ID() == id) {
					if (msg->what == B_CANCELED)
						it->fStatus = DlItem::ITEM_CANCELED;
					else
						it->fStatus = DlItem::ITEM_DEFERRED;
					
					it->Cancel();
					UpdateStatus(it,NULL);
					break;
				}
			}
			// thread will post 'Done' once it quits and the			
			// item is then deleted display/updated
			break;
		}
		case 'ActV': {
			// find the item to activate
			int32 id = msg->FindInt32("id");
			
			// find the unique item to activate
			for (int i = list.CountItems()-1; i >= 0; i--) {
				DlItem	*it = list.ItemAt(i);
				if (it->ID() == id) {
					it->fStatus = DlItem::ITEM_QUEUED;
					// tell the status window to add this item
					UpdateStatus(it,NULL);
					CheckQueue();
					break;
				}
			}
			break;
		}
		case 'Stat': {
			// an item reports a change in status
			DlItem	*oit;
			msg->FindPointer("item",(void **)&oit);
			
			UpdateStatus(oit,NULL);
			break;
		}
		case 'Done': {
			// an item reports processing is finished
			DlItem	*oit;
			msg->FindPointer("item",(void **)&oit);
			
			status_t	status = msg->FindInt32("status");
			// check the result for errors!!
			// (how to report errors here??
		
			// tell the status window to update this item
			// we use ids rather than pointers because they allow
			// downloads to complete
			// even if the user is at the same time trying to cancel
			// the download
			// the ids are unique over a long time period
			//BMessage	stat('RmvI');
			//stat.AddInt32("id",oit->ID());
			//statusWindow.SendMessage(&stat);
			UpdateStatus(oit,NULL);
			
			if (oit == currentItem)
				currentItem = 0;
			
			if (oit->fStatus != DlItem::ITEM_DEFERRED &&
				oit->fStatus != DlItem::ITEM_ERROR) {
				// remove the item from the queue
				if (oit->fStatus == DlItem::ITEM_CANCELED) {
					BEntry	entry(&oit->fileRef);
					if (entry.InitCheck() >= B_OK)
						entry.Remove();
				}
				else if ((oit->fStatus == DlItem::ITEM_COMPLETE))
				{
					// log the download
					Log	valetLog;
					
					BMessage	logMsg(Log::LOG_DOWNLOAD);
					logMsg.AddString("pkgname",oit->fileRef.name);
					
					char	buf[40];
					sprintf(buf,"Size: %Ld",oit->totalBytes);
					logMsg.AddString("strings",buf);
					logMsg.AddString("strings","Download Successful");
					valetLog.PostLogEvent(&logMsg);
					
					if (gSettings->data.FindInt32("download/flags") &
							SettingsManager::DL_AUTOLAUNCH)
					{
						// auto-install
						BMessage	rr(B_REFS_RECEIVED);
						rr.AddRef("refs",&oit->fileRef);
						be_app->PostMessage(&rr);	
					}
				}
				list.RemoveItem(oit);
				delete oit;
			}
			else {
				oit->netThread = NULL;
			}	
			CheckQueue();
			break;
		}
		case 'NwDl' : {
			PRINT(("new download!\n"));
			// prepare a new download from a vdwn file
			
			BMessage	downloadData('Url ');
			entry_ref	ref;
			msg->FindRef("refs",&ref);
			
			BEntry		ent(&ref);
			BPath		p;
			ent.GetPath(&p);
			
			FILE	*f;
			f = fopen(p.Path(), "r+");
			if (f == NULL) {
				PRINT(("couldn't open url file %s!\n",p.Path()));
				break;
			}
			
			char linebuf[128];
			char *max = linebuf+128;
			
			while (readline(f,linebuf,128) >= 0) {
				//
				char *hname = linebuf;
				char *c = hname;
				
				while(*c && *c != ':' && c < max) {
		        	*c = tolower(*c);
		        	c++;
		        }
		        *c = 0;
		        c++;
		        while (*c == ' ' && *c)
		        	c++;
		        
		        char *hval = c;
		        
		        int dlen = 0;
		        while (*c) {
		        	if (!isdigit(*c++)) {
		        		dlen = 0;
		        		break;
		        	}
		        	dlen++;
		        }
		        
		        //if (dlen > 0 && dlen < 12)
		        //	downloadData.AddInt32(hname,atol(hval));
		        //else
		        downloadData.AddString(hname,hval);
			}
			fclose(f);
			ent.Remove();
			
			PostMessage(&downloadData);
			break;
		}
		case B_REFS_RECEIVED: {
			// new download
			// add the item to the queue
			type_code	type;
			int32		count;
			
			msg->GetInfo("refs",&type,&count);
			
			for (int i = 0; i < count; i++) {
				bool		addMe = true;
				entry_ref	ref;
				msg->FindRef("refs",&ref);
				
				DlItem *nit = NULL;
				for (int i = list.CountItems()-1; i >= 0; i--) {
					DlItem *it = list.ItemAt(i);
					if (it->fileRef == ref) {
						if (it->fStatus == DlItem::ITEM_DEFERRED) {
							nit = it;
							nit->fStatus = DlItem::ITEM_QUEUED;
						}
						else {
							addMe = false;
						}
						break;
					}
				}
				if (addMe) {
					if (nit == NULL) {
						nit = new DlItem(this,fID++);
						nit->fStatus = DlItem::ITEM_QUEUED;
						list.AddItem(nit);
						nit->SetTo(&ref, msg->what == B_REFS_RECEIVED);
					}
					// tell the status window to add this item
					UpdateStatus(nit,&nit->fileRef);
				}
			}
			CheckQueue();
			be_app->PostMessage(M_DO_DOWNLOAD);
			break;
		}
		case 'Url ': {
			// new download urls are dumb (we don't do duplicate checking)
			// add the item to the queue
			PRINT(("got urls\n"));
			type_code	type;
			int32		count;
			
			msg->GetInfo("url",&type,&count);
			
			if (type != B_STRING_TYPE)
				break;
			
			DlItem *nit = NULL;
			for (int i = 0; i < count; i++) {
				BMessage	url('Url ');
				url.AddString("url",msg->FindString("url",i));
				nit = new DlItem(this,fID++);
				nit->fStatus = DlItem::ITEM_QUEUED;
				list.AddItem(nit);
				nit->SetTo(&url,false);
				UpdateStatus(nit,&nit->fileRef);
			}
			CheckQueue();
			be_app->PostMessage(M_DO_DOWNLOAD);
			break;
		}
		default:
			BLooper::MessageReceived(msg);
	}
}


DlItem::DlItem(BLooper *dmanager, int32 nID)
	:	fStatus(B_NO_ERROR),
		fID(nID),
		netThread(0),
		dLooper(dmanager),
		totalBytes(0),
		bytesCompleted(0),
		bytesDownloaded(0),
		secondsElapsed(0)
{
}

DlItem::~DlItem()
{
	delete netThread;
}


int32	DlItem::ID()
{
	return fID;
}

void DlItem::Cancel()
{
	if (netThread)
		netThread->Cancel();
}

status_t	DlItem::SetTo(BMessage *m, bool resume)
{
	if (resume)
		return B_ERROR;
	
	downloadData = *m;
	const char *connstr = downloadData.FindString("url");
	if (!connstr || !*connstr) {
		// no url
		doError("Cannot prepare download. No url specified!");
		PRINT(("no url\n"));
		return B_ERROR;
	}
		
	const char *fname;
	fname = connstr + strlen(connstr);
	fname--;
	while(*fname != '/' && fname > connstr)
	{
		fname--;
	}
	if (*fname == '/')
		fname++;
	
	fileRef.set_name(fname);
	PRINT(("filename is %s %s\n",fname,fileRef.name));
	
	return B_NO_ERROR;
}
// Download information from a .vdwn file
// eventually we should make .vdwn's queue multiple dl's
// (so a user can download a whole shopping cart)
status_t	DlItem::SetTo(entry_ref	*fRef, bool resume)
{
	downloadData.MakeEmpty();
	
	if (resume) {
		PRINT(("resumed download!\n"));
		fileRef = *fRef;
		
		status_t err;
	
		BFile f(&fileRef, O_RDWR);
		err = f.InitCheck();
		if (err != B_NO_ERROR)
			return err;
	
		BNodeInfo	nodeinf(&f);
		
		char typebuf[B_MIME_TYPE_LENGTH];
		
		err = nodeinf.GetType(typebuf);
		if (err != B_NO_ERROR)
			return err;
		
		if (!strcmp(typebuf, kPPkgSig))
		{
			attr_info	ainf;
			err = f.GetAttrInfo("download_info",&ainf);
			if (err == B_NO_ERROR) {
				// change to autoptr
				AutoPtr <char> buf(ainf.size);
				
				if ( f.ReadAttr("download_info", B_MESSAGE_TYPE,0,(void *)&buf, ainf.size) >= B_NO_ERROR)
				{
					downloadData.Unflatten(&buf);
					downloadData.AddBool("resume",true);
					return B_NO_ERROR;
				}
			}
			PRINT(("didn't get download info!\n"));
		}
		// wrong mime-type or couldn't read download_info
		return B_ERROR;
	}
	else {
		PRINT(("new download!\n"));
		// prepare a new download by
		
		BEntry		ent(fRef);
		BPath		p;
		ent.GetPath(&p);
		
		FILE	*f;
		f = fopen(p.Path(), "r+");
		if (f == NULL) {
			PRINT(("couldn't open url file %s!\n",p.Path()));
			return errno;
		}
		
		char linebuf[128];
		char *max = linebuf+128;
		
		while (readline(f,linebuf,128) >= 0) {
			//
			char *hname = linebuf;
			char *c = hname;
			
			while(*c && *c != ':' && c < max) {
	        	*c = tolower(*c);
	        	c++;
	        }
	        *c = 0;
	        c++;
	        while (*c == ' ' && *c)
	        	c++;
	        
	        char *hval = c;
	        
	        int dlen = 0;
	        while (*c) {
	        	if (!isdigit(*c++)) {
	        		dlen = 0;
	        		break;
	        	}
	        	dlen++;
	        }
	        
	        if (dlen > 0 && dlen < 12)
	        	downloadData.AddInt32(hname,atol(hval));
	        else
	        	downloadData.AddString(hname,hval);
		}
		fclose(f);
		ent.Remove();
		
		return SetTo(&downloadData,false);
	}
}

// begin new download
// download to the destination directory
status_t	DlItem::BeginDownload(BDirectory *dir)
{
	const char *connstr = downloadData.FindString("url");
	
	const char *fname = fileRef.name;

	if (!connstr || !*connstr || !fname) {
		// no url
		doError("no url");
		PRINT(("no url\n"));
		return B_ERROR;
	}
	
	if (downloadData.HasBool("resume") && downloadData.FindBool("resume")) {
		// this is a resume
		BFile	file(&fileRef,O_RDWR | O_BINARY);
		if (file.InitCheck() < B_OK) {
			doError("Couldn't open resume file.");
			return B_ERROR;
		}
			
		file.Seek(0,SEEK_END);
		
		delete netThread;
		netThread = new DownloadThread(file,this);
		return netThread->Run();
	}
	else {
		char 	name[B_FILE_NAME_LENGTH-4];
		char 	fbuf[B_FILE_NAME_LENGTH];
		BFile	file;
		
		// copy in with space for postfix
		strncpy(name, fname, B_FILE_NAME_LENGTH - 4);
		strcpy(fbuf,name);
		
		// deal with existing filenames here
		int i = 1;
		while (dir->Contains(fbuf) && i < INT_MAX) {
			sprintf(fbuf,"%s.%d",name,i);
			i++;
		}
		// couldn't create unique file
		if (i >= INT_MAX)
			return B_ERROR;
		
		// create the file
		status_t res;
		res = dir->CreateFile(fbuf, &file);
		if (res != B_NO_ERROR)
			return res;
	
		res = file.SetTo(dir,fbuf, O_RDWR | O_BINARY);
		if (res != B_NO_ERROR)
			return res;	
			
		file.RemoveAttr("download_info");
		
		ssize_t sz = downloadData.FlattenedSize();
		if (sz <= 0)
			return B_ERROR;
			
		AutoPtr <char> buf(sz);
		downloadData.Flatten(&buf, sz);
	
		file.WriteAttr("download_info", B_MESSAGE_TYPE, 0, &buf, sz);
		
		BNodeInfo	ninf(&file);
		ninf.SetType(kPPkgSig);
		
		BEntry		ent(dir,fbuf);
		ent.GetRef(&fileRef);
		
		downloadData.AddBool("resume",true);
		
		delete netThread;
		netThread = new DownloadThread(file,this);
		return netThread->Run();
	}
}

////////////////////

// Does the network meat!
DownloadThread::DownloadThread(BFile &inFile,
								DlItem *inItem)
	:	MThread("download thread"),
		file(inFile),
		fDlItem(inItem)
{
}

void	DownloadThread::LastCall(long status)
{
	// send a message with the completion
	BMessage stat('Done');
	stat.AddInt32("status",status);
	stat.AddPointer("item",fDlItem);
	fDlItem->dLooper->PostMessage(&stat);
	
	// the owner looper will delete our owner item
}

void	DownloadThread::Cancel()
{
	MThread::Cancel();
	// to cancel we must interrupt the network connection
	// the current or subsequent socket call will fail
	// and return B_CANCELED
	htconn.Interrupt();
}

long	DownloadThread::Execute()
{
	BMessage	*msg = &fDlItem->downloadData;

	status_t		res = B_NO_ERROR;
	
	const char *connstr = msg->FindString("url");
	if (!connstr || !*connstr) {
		// no url
		PRINT(("no url\n"));
		fDlItem->fStatus = DlItem::ITEM_ERROR;
		return B_ERROR;
	}
	
	if ((res = htconn.Connect(connstr)) < B_NO_ERROR) {
		//doError("Connection failed. %s",strerror(res));
		PRINT(("connect failed\n"));
		fDlItem->fStatus = DlItem::ITEM_ERROR;
		return res;
	}
	file.Seek(0,SEEK_END);
	fDlItem->bytesCompleted = file.Position();
	char qs[40];
	*qs = 0;
	if (fDlItem->bytesCompleted > 0)
		sprintf(qs, "off=%Ld", fDlItem->bytesCompleted);
	
	// getting response	
	if ((res = htconn.Get(qs)) < B_NO_ERROR) {
		//doError("Get failed. %s",strerror(res));
		fDlItem->fStatus = DlItem::ITEM_ERROR;
		return res;
	}
	
	if (res != 200)
	{
		char msg[512];	
		GetHttpResponse(htconn, msg, 512);
		doError(msg);
		
		fDlItem->fStatus = DlItem::ITEM_ERROR;
		return B_ERROR;
	}
	
	// BAlert *a = new ConnAlert(&htconn);
	// a->Go(NULL);
		
	fDlItem->totalBytes = htconn.ContentLength() + fDlItem->bytesCompleted;
	fDlItem->bytesDownloaded = 0;
	int32 length = htconn.ContentLength();
	int32 		updateAmt = 0;
	bigtime_t	lastTime = 0;
	
	// send a status message with the content length
	BMessage stat('Stat');
	stat.AddPointer("item",fDlItem);
	
	fDlItem->dLooper->PostMessage(&stat);
	
	fDlItem->secondsElapsed = 0;
	bigtime_t	startTime = system_time();
	char bigbuf[8192];
	while (length) {
		int amt = min_c(length,8192);
		int red = htconn.Read(bigbuf,amt);
		if (red <= 0) {
			if (red == 0)
				res = ECONNABORTED;
			else
				res = red;
			break;
		}
		length -= red;
		amt = red;
		red = file.Write(bigbuf,amt);
		if (red != amt) {
			PRINT(("file write error\n"));
			res = B_ERROR;
			break;		
		}
		fDlItem->bytesDownloaded += amt;
		fDlItem->bytesCompleted += amt;
		
		bigtime_t timeNow = system_time();
		updateAmt += amt;
		if ((float)updateAmt/(float)fDlItem->totalBytes > 0.02 ||
			timeNow - lastTime > 1000*1000)
		{
			updateAmt = 0;
			fDlItem->secondsElapsed = (float)(timeNow - startTime)/(float)(1000*1000);
			fDlItem->dLooper->PostMessage(&stat);
		}
		// send a status message with the amount downloaded and the time elapsed
		//PRINT(("*"));
		//#if DEBUG
			//fflush(stdout);
			//snooze(90000);
		//#endif
	}

	if (res >= B_NO_ERROR) {
		// download was completed
		fDlItem->fStatus = DlItem::ITEM_COMPLETE;

		BNodeInfo	ninf(&file);
		ninf.SetType(kUPkgSig);
		// should be
		// if (htconn.headers.szContentType)
		// 	ninf.SetType(htconn.headers.szContentType);
		// else // some sort of default mimetype???
		// 	ninf.SetType(htconn.headers.
	}
	else if (res == B_CANCELED) {
		// download paused/cancelled
		//fDlItem->fStatus = DlItem::ITEM_ERROR;
		PRINT(("download canceled\n"));
	}
	else {
		// either a read or write error
		fDlItem->fStatus = DlItem::ITEM_ERROR;
	}
	
	return res;
}
