// ===========================================================================
//	DownloadManager.cpp
//  Copyright 1999 by Be Incorporated.
// ===========================================================================

#include <MenuBar.h>
#include <Roster.h>

#include "DownloadManagerDefs.h"

static DownloadWindow *sDownloadWindow = NULL;
static ConnectionManager *sDownloadConnectionMgr = NULL;

DownloadScrollView::DownloadScrollView(BRect rect)
	: BScrollView("", mTargetView = new ActivatedView(BRect(rect.left, rect.top, rect.right - B_V_SCROLL_BAR_WIDTH, rect.bottom - B_H_SCROLL_BAR_HEIGHT),"",
				 B_FOLLOW_ALL, B_WILL_DRAW), B_FOLLOW_ALL,
				B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED, false, true, B_FANCY_BORDER), mNumberDownloads(0)
{
	mTargetView->SetViewColor(kWhite, kWhite, kWhite);
	int32 maxConnections = gPreferences.FindInt32("MaxDownloadConnections");
	
	if (!sDownloadConnectionMgr)
		sDownloadConnectionMgr = new ConnectionManager();
	sDownloadConnectionMgr->SetMaxConnections(maxConnections);
	
	BMessage downloadArchive, archiveItem;
	gPreferences.FindMessage("PreviousDownloads", &downloadArchive);
	int index = 0;
	while (downloadArchive.FindMessage("item", index++, &archiveItem) == B_OK) {
		DownloadItemView *item = new DownloadItemView(&archiveItem);
		if( item->FileExists() )
			AddItem(item);
	}		
}

DownloadScrollView::~DownloadScrollView()
{
	BMessage downloadArchive;
	
	for (int i = 0; i < CountItems(); i++) {
		DownloadItemView *item = ItemAt(i);
		BMessage archiveItem;
		item->Archive(&archiveItem);
		downloadArchive.AddMessage("item", &archiveItem);
	}
	
	gPreferences.ReplaceMessage("PreviousDownloads", &downloadArchive);
}

void
DownloadScrollView::AddItem(BView *item)
{
	if(!Window()->IsActive())
		Window()->Activate();
	float itemHeight = item->Frame().Height();
	item->ResizeTo(mTargetView->Frame().Width(), itemHeight);
	int vPos = 0;
	int32 index = mItemList.CountItems();
	if (index > 0)
		vPos = index * kItemHeight;
	item->MoveTo(0, vPos);
	Window()->BeginViewTransaction();
	mTargetView->AddChild(item);
	Window()->EndViewTransaction();
	mItemList.AddItem(item);
	UpdateScrollBar();

	// Scroll to the end of the list so that the new item is visible.
	BScrollBar *bar = ScrollBar(B_VERTICAL);
	float min = 0;
	float max = 0;
	bar->GetRange(&min, &max);
	bar->SetValue(max);
}


void
DownloadScrollView::RemoveItem(void *item)
{
	int32 index = mItemList.IndexOf(item);
	if (index < 0)
		return;

	Window()->Lock();
	mItemList.RemoveItem(item);
	mTargetView->RemoveChild(static_cast<DownloadItemView*>(item));
	if(static_cast<DownloadItemView*>(item)->Lock())
		delete item;

	if(mItemList.CountItems() == 0)
		Window()->QuitRequested();
		
	for (int i = 0; i < mItemList.CountItems(); i++) {
		BView *view = (BView *)mItemList.ItemAt(i);
		view->MoveTo(BPoint(0, i*kItemHeight) - mTargetView->Bounds().LeftTop());
	}
	UpdateScrollBar();
	Window()->Unlock();
}


void
DownloadScrollView::RemoveItem(int32 index)
{
	RemoveItem((BView *)mItemList.ItemAt(index));
}

void 
DownloadScrollView::RemoveAll()
{
	for(int i = mItemList.CountItems() - 1; i >= 0; --i){
		DownloadItemView *view = ItemAt(i);
		view->StopDownload();
		view->Trash();
		BMessage deleteMessage(msg_DeleteDownloadItem);
		deleteMessage.AddPointer("itemPtr", this);
		Window()->PostMessage(&deleteMessage);
//		RemoveItem(view);
	}
}

DownloadItemView*
DownloadScrollView::ItemAt(int32 index)
{
	return static_cast<DownloadItemView *>(mItemList.ItemAt(index));
}

int32
DownloadScrollView::CountItems()
{
	return mItemList.CountItems();
}

void
DownloadScrollView::FrameResized(float new_width, float new_height)
{
	BScrollView::FrameResized(new_width, new_height);
	for(int i=0; i < CountItems(); ++i){
		DownloadItemView *item = ItemAt(i);
		item->NewParentWidth(new_width);
	}
	Invalidate();
	UpdateScrollBar();
}

void
DownloadScrollView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_SIMPLE_DATA:{
			BString URL(msg->FindString("be:url"));
			if (URL.Length() > 0){
				DownloadManager::DownloadFile(URL.String(), true);
				break;
			}

			if(msg->WasDropped() && !msg->FindBool("be:netpositive:fromdl")){
				ulong type;
				long count = 0;
				msg->GetInfo("refs", &type, &count);
				NetPositive *np = (NetPositive *)be_app;
				entry_ref ref;
				for (long i = 0; i < count; i++) {
					msg->FindRef("refs", i, &ref);
					URL.SetTo("");
					if (np->RefToURL(ref,URL) && URL.Length() > 0)
						DownloadManager::DownloadFile(URL.String(), true);
				}
			}
			break;
		}
		case msg_NewBrowserWindow:
			((NetPositive *)be_app)->NewWindow();
			break;
		case msg_StopAll:
			StopAll();
			break;
		case msg_SetSimultaneous:{
			int32 maxConnections;
			if (msg->FindInt32("num", &maxConnections) == B_OK) {
				gPreferences.ReplaceInt32("MaxDownloadConnections", maxConnections);
				sDownloadConnectionMgr->SetMaxConnections(maxConnections);
			}
			break;
		}
		case msg_UnselectAllButOne:{
			void *pTheOne;
			if(msg->FindPointer("theOne", &pTheOne)  != B_OK)
				break;
			for(int i=0; i<CountItems(); ++i){
				DownloadItemView *pdlView = ItemAt(i);
				if(!pdlView)
					break;
				if(pdlView != static_cast<DownloadItemView*>(pTheOne)){
					pdlView->SetSelected(false);
				}
				else{
					pdlView->SetSelected(true);
				}
			}
			break;
		}
		case B_NODE_MONITOR:
		{
			int32 opcode = msg->FindInt32("opcode");
			if(opcode == B_ENTRY_MOVED){
				ino_t node;
				if (msg->FindInt64("node", &node) != B_OK)
					break;
				for (int i = 0; i < mItemList.CountItems(); i++) {
					DownloadItemView *view = ItemAt(i);
					if(view->GetNodeRef().node == node){
						view->MessageReceived(msg);
						break;
					}
				}				
			}
		}
		case msg_DragAllSelected:{
			BPoint point;
			int32 indexOfCaller = -1;
			void *viewThatCalled;
			if(msg->FindPoint("where", &point) != B_OK || msg->FindPointer("callingView", &viewThatCalled))
				break;
			for(int i=0; i<mItemList.CountItems(); ++i){
				DownloadItemView *view = ItemAt(i);
				if(view == static_cast<DownloadItemView *>(viewThatCalled)){
					indexOfCaller = i;
					break;
				}
			}
			if(indexOfCaller == -1)
				break;
			DragAllSelected(point, indexOfCaller);
			break;	
		}
		case msg_LaunchAllSelected:
			LaunchAllSelected();
			break;
		case msg_DeleteAllSelected:
			DeleteAllSelected();
			break;
		default:
			BScrollView::MessageReceived(msg);
			break;
	}
}

void 
DownloadScrollView::AttachedToWindow()
{
	BView::AttachedToWindow();
	MakeFocus();
	UpdateScrollBar();
}

void
DownloadScrollView::Pulse()
{
	int32 count = 0;
	for (int i = 0; i < mItemList.CountItems(); i++) {
		DownloadItemView *view = ItemAt(i);
		if(view->Status() == kdDownloading)
			++count;
	}
	if(count == mNumberDownloads)
		return;

	mNumberDownloads = count;		
	if(count == 0){
		BMessage msg(DOWNLOADS_STOPPED);
		be_app->PostMessage(&msg);
		return;
	}
	
	BMessage mess(DOWNLOADS_STARTED);
	be_app->PostMessage(&mess); 
	return;		
}

void 
DownloadScrollView::KeyDown(const char *bytes, int32 numBytes)
{
	if(numBytes == 1){
		switch (bytes[0]){
			case B_DELETE:
				DeleteAllSelected();
				return;
			case B_UP_ARROW:
				for (int i = 0; i < mItemList.CountItems(); i++) {
					DownloadItemView *view = ItemAt(i);
					if(view->IsSelected()){
						if(i > 0)
							view = ItemAt(i-1);
						BMessage msg(msg_UnselectAllButOne);
						msg.AddPointer("theOne", view);
						MessageReceived(&msg);
						return;
					}
				}
				if(mItemList.CountItems() > 0){
					BView *view = ItemAt(0);
					BMessage msg(msg_UnselectAllButOne);
					msg.AddPointer("theOne", view);
					MessageReceived(&msg);
					return;
				}
				break;					
			case B_DOWN_ARROW:
				for (int i = mItemList.CountItems() - 1; i >= 0; --i){
					DownloadItemView *view = ItemAt(i);
					if(view->IsSelected()){
						if(i < mItemList.CountItems() - 1)
							view = ItemAt(i+1);
						BMessage msg(msg_UnselectAllButOne);
						msg.AddPointer("theOne", view);
						MessageReceived(&msg);
						return;
					}
				}
				if(mItemList.CountItems() > 0){
					BView *view = ItemAt(CountItems() - 1);
					BMessage msg(msg_UnselectAllButOne);
					msg.AddPointer("theOne", view);
					MessageReceived(&msg);
					return;
				}
				break;					
					
		}
	}
	BScrollView::KeyDown(bytes, numBytes);
}

void 
DownloadScrollView::DragAllSelected(BPoint where, int32 indexOfCaller)
{
	BMessage dragMessage(B_SIMPLE_DATA);
	dragMessage.AddInt32("be:actions", B_MOVE_TARGET);
	dragMessage.AddBool("be:netpositive:fromdl", true);
	int32 firstSelected = -1;
	int32 lastSelected = -1;
	for(int i=0; i< mItemList.CountItems(); ++i){
		DownloadItemView *view = ItemAt(i);
		if(view->IsSelected()){
			if(firstSelected == -1)
				firstSelected = i;
			lastSelected = i;	
			entry_ref entryRef = view->GetEntryRef();
			dragMessage.AddRef("refs", &entryRef);
		}
	}

	if(firstSelected == -1) //shouldn't really happen
		return;

	BBitmap *sizingMap = ItemAt(0)->NewDragBitmap();
	BRect rect(sizingMap->Bounds());
	delete sizingMap;
	rect.bottom = kItemHeight * (lastSelected - firstSelected + 1);
	BBitmap *dragBitmap = new BBitmap(rect, B_RGBA32, true);
	dragBitmap->Lock();
	BView *view = new BView(dragBitmap->Bounds(), "", B_FOLLOW_NONE, 0);
	dragBitmap->AddChild(view);
	view->SetOrigin(0, 0);
	BRect clipRect(view->Bounds());
	BRegion newClip;
	newClip.Set(clipRect);
	view->ConstrainClippingRegion(&newClip);
	// Transparent draw magic
	view->SetHighColor(0, 0, 0, 0);
	view->FillRect(view->Bounds());
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetHighColor(0, 0, 0, 192);
	for(int i=0; i< mItemList.CountItems(); ++i){
		DownloadItemView *dlview = ItemAt(i);
		if(dlview->IsSelected()){
			BBitmap *iconToAdd = dlview->NewDragBitmap();
			view->DrawBitmap(iconToAdd, BPoint(0,(i - firstSelected) * kItemHeight));
			delete iconToAdd;
			if(i == lastSelected)
				break;
		}
	}
	view->Sync();
	dragBitmap->Unlock();
		
	BRect iconBounds = dragBitmap->Bounds();
	iconBounds.OffsetTo(kIconLeft, kIconDown - ((indexOfCaller - firstSelected) * kItemHeight) );
	DragMessage(&dragMessage, dragBitmap, B_OP_ALPHA, where - iconBounds.LeftTop(), this);
}

void 
DownloadScrollView::LaunchAllSelected()
{
	for(int i=0; i< mItemList.CountItems(); ++i){
		DownloadItemView *view = ItemAt(i);
		if(view->IsSelected()){
			view->Launch();
		}
	}
	
}

void 
DownloadScrollView::DeleteAllSelected()
{
	for(int i=mItemList.CountItems() - 1; i >= 0; --i){
		DownloadItemView *view = ItemAt(i);
		if(view->IsSelected()){
			view->StopDownload();
			view->Trash();
			BMessage deleteMessage(msg_DeleteDownloadItem);
			deleteMessage.AddPointer("itemPtr", this);
			Window()->PostMessage(&deleteMessage);
//			RemoveItem(view);
		}
	}
}

void 
DownloadScrollView::RetryAllStopped()
{
	for(int i=0; i < mItemList.CountItems(); ++i){
		DownloadItemView *view = ItemAt(i);
		int status = view->Status();
		switch (status){
			case kdUninitialized:
			case kdStopped:
			case kdError:
			case kdWaiting:{
				BMessage retryPlease(msg_RetryDownloadItem);
				view->MessageReceived(&retryPlease);
				break;
			}	
			default:
				break;
		}					
	}
}

void 
DownloadScrollView::StopAll()
{
	for (int i = 0; i < mItemList.CountItems(); i++) {
		DownloadItemView *view = ItemAt(i);
		view->StopDownload();
	}
	
	for (int i = 0; i < mFilePanelList.CountItems(); i++) {
		BFilePanel *panel = (BFilePanel *)mFilePanelList.ItemAt(i);
		panel->Window()->PostMessage(B_QUIT_REQUESTED);
	}
}

void 
DownloadScrollView::SelectAll()
{
	for (int i = 0; i < mItemList.CountItems(); i++) {
		DownloadItemView *view = ItemAt(i);
		view->SetSelected(true);
	}
}

bool 
DownloadScrollView::IsStillDownloading()
{
	for (int i = 0; i < mItemList.CountItems(); i++) {
		DownloadItemView *item = ItemAt(i);
		if (item->Status() == kdDownloading)
			return true;
	}
	
	if (mFilePanelList.CountItems() > 0)
		return true;
		
	return false;
}

void 
DownloadScrollView::AddFilePanel(BFilePanel *panel)
{
	mFilePanelList.AddItem(panel);
}

void
DownloadScrollView::RemoveFilePanel(BFilePanel *panel)
{
	mFilePanelList.RemoveItem(panel);
}

void
DownloadScrollView::UpdateScrollBar()
{
	float targetHeight = 0;
	int32 index = mItemList.CountItems();
	if (index > 0)
		targetHeight = (Target()->Frame()).bottom;
		
	int vPos = 0;
	float smallStep = 0;
	int32 numItems = mItemList.CountItems();
	
	if (numItems > 0) {
		smallStep = kItemHeight;
		vPos = numItems * kItemHeight;
	}
		
	BScrollBar *scrollBar = ScrollBar(B_VERTICAL);
	scrollBar->SetRange(0, vPos > targetHeight ? vPos - targetHeight : 0);
	scrollBar->SetSteps(smallStep, targetHeight - smallStep);
}

ActivatedView::ActivatedView(BRect frame, const char *name, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
}

void 
ActivatedView::MouseDown(BPoint where)
{
	Window()->Activate();
	BView::MouseDown(where);
}

DownloadItemView::DownloadItemView(UResourceImp *resource, const char *name, const char *downloadPath, int32 status, bool openWhenDone, bool wasMoved)
	: BView(BRect(0,0,400,kItemHeight - 1), "", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_PULSE_NEEDED),
	  	mName(name), mStatus(status), mResource(resource), mDone(0), mOf(0), mBPS(0), mTotalTime(0),
		mLastUpdate(0), mLastClick(0), mOpenWhenDone(openWhenDone), mBigIcon(NULL), mSmallIcon(NULL), mFileIconPhase(-1), mUpdateIcon(false), 
		mFileExists(true), wasDragged(wasMoved), mUpdate(false), mIsSelected(false), mButtonLeft(10000)
{
	// DownloadItemView doesn't add a reference to the resource passed into its constructor.  It inherits the reference from the caller.
	mStartTime = system_time();
	mURL = resource->GetURL();

	BEntry entry(downloadPath);
	entry.GetRef(&mEntryRef);
	entry.GetNodeRef(&mNodeRef);
	Init();
}


DownloadItemView::DownloadItemView(BMessage *msg)
	: BView(BRect(0,0,400,kItemHeight - 1), "", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW | B_PULSE_NEEDED),
		mResource(NULL), mTotalTime(0), mLastUpdate(-1), mLastClick(0), mBigIcon(NULL), mSmallIcon(NULL),
		mFileIconPhase(-1), mUpdateIcon(false), wasDragged(false), mUpdate(false), mIsSelected(false), mButtonLeft(10000)
{
	// Note that we call a normal BView constructor, not the archive constructor.  We don't store BView-specific
	// information in the archive because we don't care.
	mName = msg->FindString("name");
	mStatus = msg->FindInt32("status");
	mDone = msg->FindInt32("done");
	mOf = msg->FindInt32("of");
	mFileIconPhase = msg->FindInt32("fileIconPhase");
	mStartTime = system_time();
	mURL = msg->FindString("url");
	BString downloadPath = msg->FindString("downloadPath");
	BEntry entry(downloadPath.String());
	entry.GetRef(&mEntryRef);
	entry.GetNodeRef(&mNodeRef);
	mFileExists = entry.Exists();

	if(msg->FindBool("wasDragged", &wasDragged) != B_OK)
		wasDragged = false;
	if(msg->FindBool("openWhenDone", &mOpenWhenDone) != B_OK)
		mOpenWhenDone = false;

	Init();
}


status_t
DownloadItemView::Archive(BMessage *msg, bool /*deep*/) const
{
	msg->AddString("name", mName.String());
	msg->AddInt32("status", mStatus);
	msg->AddInt32("done", mDone);
	msg->AddInt32("of", mOf);
	msg->AddString("url", mURL.String());
	BEntry entry(&mEntryRef, false);
	BPath path;
	entry.GetPath(&path);
	msg->AddString("downloadPath", path.Path());
	msg->AddBool("wasDragged", wasDragged);
	msg->AddBool("openWhenDone", mOpenWhenDone);
	msg->AddInt32("fileIconPhase", mFileIconPhase);
	return B_OK;
	// Don't call BView's archive.  We don't care.
}

DownloadItemView::~DownloadItemView()
{
	if(mFileExists){
		node_ref nodeRef;
		BEntry entry(&mEntryRef);
		entry.GetNodeRef(&nodeRef);
		watch_node(&nodeRef, B_STOP_WATCHING, this);
	}
	delete mBigIcon;
	delete mSmallIcon;
}

void
DownloadItemView::MouseDown(BPoint where)
{
	Window()->Activate();
	//handle the button case first
	BRect buttonBounds = mStopIcon->Bounds();
	buttonBounds.OffsetTo(mButtonLeft, kStopButtonDown);
	if( buttonBounds.Contains(where) ){
		mButtonDown = true;
		Window()->Lock();
		Invalidate();
		Window()->Unlock();
		BPoint cursor;
		uint32 buttons;
		while(true){
			GetMouse(&cursor, &buttons);		
			if( !(buttons & B_PRIMARY_MOUSE_BUTTON) && buttonBounds.Contains(where)){
				mButtonDown = false;
				Invalidate();
				BMessage buttPressed(msg_DLButtonPressed);
				MessageReceived(&buttPressed);
				return;
			}
			else if( !(buttons & B_PRIMARY_MOUSE_BUTTON) ){ //was released outside the button rect
				mButtonDown = false;
				Invalidate();
				return;
			}
		}		
	}		

	BRect iconBounds(0,0,31,31); //default to 32x32 in case there is no mBigIcon
	if(mBigIcon)
		iconBounds = mBigIcon->Bounds();
	iconBounds.OffsetTo(kIconLeft, kIconDown);

	if (mFileExists && iconBounds.Contains(where)) {
		bigtime_t oldClick = mLastClick;
		bigtime_t clickSpeed = 0;
		mLastClick = system_time();
		get_click_speed(&clickSpeed);
		//double click handling
		if(mLastClick - oldClick < clickSpeed && mStatus == kdComplete && !(modifiers() & B_SHIFT_KEY)){
			BMessage msg(msg_LaunchAllSelected);
			Parent()->MessageReceived(&msg);
			return;
		}

		//selection handling on the icon
		int32 mods = modifiers();
		if(mods & B_SHIFT_KEY){
			mIsSelected = !mIsSelected;
			Invalidate();
		}
		else{
			if(!mIsSelected){
				BMessage msg(msg_UnselectAllButOne);
				msg.AddPointer("theOne", this);
				Parent()->MessageReceived(&msg);
			}
		}

		//now drag if necessary and not yet dragged
		if(!wasDragged && mIsSelected)
		{
			BPoint dragCursor;
			uint32 dragButtons;
			bigtime_t now = system_time();
			bigtime_t later = now + clickSpeed;
			while(now < later){
				GetMouse(&dragCursor, &dragButtons);
				if( !(dragButtons & B_PRIMARY_MOUSE_BUTTON))
					break;
				if(dragCursor != where){
					BMessage msg(msg_DragAllSelected);
					msg.AddPoint("where", where);
					msg.AddPointer("callingView", this);
					Parent()->MessageReceived(&msg);
					break;
				}
				now = system_time();
			}
		}
	
		return;
	}

	//selection handling off of the icon
	int32 mods = modifiers();
	if(mods & B_SHIFT_KEY){
		mIsSelected = !mIsSelected;
		Invalidate();
	}
	else{
		BMessage msg(msg_UnselectAllButOne);
		msg.AddPointer("theOne", this);
		Parent()->MessageReceived(&msg);
	}

	BView::MouseDown(where);
}

void
DownloadItemView::Draw(BRect /*updateRect*/)
{
	if(!mUpdate) //bitmaps aren't initialized yet
		return;

	BView *mainView = new BView(Bounds(),"offMain",B_FOLLOW_ALL,B_WILL_DRAW);
	BBitmap *mainBitmap = new BBitmap(Bounds(),NetPositive::MainScreenColorSpace(),TRUE);
	mainBitmap->AddChild(mainView);
	mainBitmap->Lock();
	mainView->SetDrawingMode(B_OP_OVER);

	if(mIsSelected)
		mainView->SetHighColor(192,192,192);
	else
		mainView->SetHighColor(kWhite,kWhite,kWhite);
	mainView->FillRect(Bounds());		

	if (mFileExists && mBigIcon){
		BBitmap *phaseBitmap = new BBitmap(mBigIcon, true);
		float percentDone = 0;
		if(mOf > 0)
			percentDone = (float)mDone / (float)mOf;
		int32 barHeight = static_cast<int32>(29 * percentDone);
		BView *phaseView = new BView(phaseBitmap->Bounds(),"phaseView",B_FOLLOW_ALL,B_WILL_DRAW);
		phaseBitmap->AddChild(phaseView);

		phaseBitmap->Lock();
		phaseView->SetDrawingMode(B_OP_OVER);
		if(mStatus != kdComplete){ //draw status bar on icon
			phaseView->SetHighColor(0,0,0);
			phaseView->StrokeRect(BRect(1,1,6,31)); //black shade
			phaseView->SetHighColor(96,96,96);
			phaseView->StrokeRect(BRect(0,0,5,30)); //grey box around bar
			phaseView->SetHighColor(255,255,255);
			phaseView->FillRect(BRect(1,1,4,1 + 29 - barHeight)); //white top of bar
			switch (mStatus) {
				case kdDownloading:
				case kdUninitialized:
				case kdWaiting:
					phaseView->SetHighColor(0,203,0); //green - in progress
					break;
				default:	
					phaseView->SetHighColor(178,0,0); //red - stopped or error
					break;
			}
			phaseView->FillRect(BRect(1,1 + 29 - barHeight, 4, 29)); //bottom of bar
		}
		phaseView->Sync();

		if(mIsSelected || wasDragged){
			uchar* oldBits = (uchar*)phaseBitmap->Bits();
			long    len = phaseBitmap->BitsLength();
		
			BScreen screen;
			int32 shiftColor = 60;
			if(wasDragged)
				shiftColor = -120;
			for (long i = 0; i < len; i++) {
				uchar   index = *oldBits;
				if (index == B_TRANSPARENT_8_BIT)
					*oldBits = index;
				else {
					rgb_color color = system_colors()->color_list[index];
					if (color.red + color.green + color.blue != (255 * 3)) {
						color.red = min_c(255, max_c(0, color.red - shiftColor));
						color.green = min_c(255, max_c(0, color.green - shiftColor));
						color.blue = min_c(255,max_c(0, color.blue - shiftColor));
					}
		
					*oldBits = screen.IndexForColor(color);
				}
				oldBits++;
			}
		}		

		phaseBitmap->Unlock();
		mainView->DrawBitmap(phaseBitmap, BPoint(kIconLeft, kIconDown));
		delete phaseBitmap;
	}	
	mainView->SetHighColor(0,0,0);
	mainView->DrawString(mName.String(), mName.Length(), BPoint(kNameLeft, kTextBaseline));

	if (mStatus == kdDownloading) {
		BString statusStr;
		FormatProgressStr(statusStr, mDone, 0, mOf, mBPS, true, false, true, true);
		mainView->SetHighColor(0,0,0);
		mainView->DrawString(statusStr.String(), statusStr.Length(), BPoint(kStatusStringLeft, kStatusStringDown));

		//draw stop icon
		if(mButtonDown == false)
			mainView->DrawBitmap(mStopIcon, BPoint(mButtonLeft ,kStopButtonDown) );
		else
			mainView->DrawBitmap(mStopPressedIcon, BPoint(mButtonLeft, kStopButtonDown));

	} else if (mStatus == kdStopped || mStatus == kdError || mStatus == kdComplete) {
		//draw disabled stop or retry icon
		switch(mStatus){
			case kdStopped:
			case kdError:
				if(mStatus == kdStopped){
					fflush(stdout);
				}
				else{
					fflush(stdout);
				}
				if(mButtonDown == false)
					mainView->DrawBitmap(mRetryIcon, BPoint(mButtonLeft ,kStopButtonDown) );
				else
					mainView->DrawBitmap(mRetryPressedIcon, BPoint(mButtonLeft ,kStopButtonDown) );
				break;
			default: //this should be kdComplete
				break;
		}				
		BString statusStr;
		FormatProgressStr(statusStr, mDone, 0, mOf, mBPS, false, false, false, false);
		mainView->SetHighColor(0,0,0);
		mainView->DrawString(statusStr.String(), statusStr.Length(), BPoint(kStatusStringLeft, kStatusStringDown));
	} else if (mStatus == kdWaiting) {
		char str[255];
		if (mOf > 0) {
			BString sizeStr;
			ByteSizeStr(mOf, sizeStr);
			sprintf(str, kDownloadWaitingSize, sizeStr.String());
		} else {
			strcpy(str, kDownloadWaiting);
		}
		if(mButtonDown == false)
			mainView->DrawBitmap(mStopIcon, BPoint(mButtonLeft ,kStopButtonDown) );
		else
			mainView->DrawBitmap(mStopPressedIcon, BPoint(mButtonLeft,kStopButtonDown) );

		mainView->SetHighColor(0,0,0);
		mainView->DrawString(str, strlen(str), BPoint(kStatusStringLeft, kStatusStringDown));
	}
	
	SetDrawingMode(B_OP_OVER);
	mainView->Sync();
	DrawBitmap(mainBitmap, Bounds(), Bounds());
	mainBitmap->Unlock();
	delete mainBitmap;
}

void
DownloadItemView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case msg_DLButtonPressed:
			switch (mStatus){
				case kdDownloading:
					StopDownload();
					break;
				case kdStopped:
				case kdError:
				case kdWaiting:
					BMessage retryPlease(msg_RetryDownloadItem);
					MessageReceived(&retryPlease);
					break;
				//this should be kdComplete so do nothing
			}
			break;
		case msg_StopDownloadItem:
			StopDownload();
			break;
		case msg_RetryDownloadItem: {
			if(mResource)
				break;
			off_t size;
			BFile file(&mEntryRef, B_READ_WRITE);
			if (file.GetSize(&size) != B_OK || size < (off_t)mDone) {
				// Create and zorch the file.
				file.SetSize(0);
				size = 0;
			}
							
			mDone = size;
			BString error;
			BEntry entry(&mEntryRef, false);
			BPath path;
			entry.GetPath(&path);
			mResource = GetUResource(sDownloadConnectionMgr, mURL.String(), 0, error, false, path.Path(), NULL, NULL, true, mDone, false, true);
			UResourceCache::DontCache(mResource);
			mStatus = kdWaiting;
			if (mResource && mResource->LockWithTimeout(1000) == B_OK){
				long done = 0;
				long of = 0;
				mResource->GetProgress(&done, &of);
				mDone = done == 0 ? mDone : done;
				mOf = of == 0 ? mOf : of;
				mResource->Unlock();
				mBPS = 1;
			}
			UpdateStatus();
			break;
		}
		case B_MESSAGE_NOT_UNDERSTOOD:
			wasDragged = false;
			break;
		case B_NODE_MONITOR:
		{
			int32 opcode = msg->FindInt32("opcode");
			if(opcode == B_ENTRY_MOVED){
				ino_t fromDirectoryNode;
				ino_t toDirectoryNode;
				dev_t device;
				ino_t node;
				const char *tmpTo;
				if (msg->FindInt32("device", &device) != B_OK ||
					msg->FindInt64("from directory", &fromDirectoryNode) != B_OK ||
					msg->FindInt64("to directory", &toDirectoryNode) != B_OK ||
					msg->FindInt64("node", &node) != B_OK ||
					msg->FindString("name", &tmpTo) != B_OK)
					break;
				
				if(mNodeRef.node != node) //this is someone elses drag message
					break;
										
				wasDragged = true;
				mEntryRef.device = device;
				mEntryRef.directory = toDirectoryNode;
				mEntryRef.set_name(tmpTo);
				
				BPath dirPath;
				BEntry dirEntry;
				BEntry fileEntry(&mEntryRef);
				fileEntry.GetParent(&dirEntry);
				dirEntry.GetPath(&dirPath);
				BPath trashPath;
				find_directory(B_TRASH_DIRECTORY, &trashPath);
				if(mStatus == kdComplete || dirPath == trashPath){
					if(dirPath == trashPath)
						StopDownload();
					BMessage deleteMessage(msg_DeleteDownloadItem);
					deleteMessage.AddPointer("itemPtr", this);
					Window()->PostMessage(&deleteMessage);
					return;
				}
			}
			break;
		}
		case B_SIMPLE_DATA:
			Window()->MessageReceived(msg);
			break;
			
		default:
			BView::MessageReceived(msg);
			break;
	}
}

void
DownloadItemView::AttachedToWindow()
{
	if(mFileExists){
		node_ref nodeRef;
		BEntry entry(&mEntryRef);
		entry.GetNodeRef(&nodeRef);
		watch_node(&nodeRef, B_WATCH_NAME, this);
	}
	if(mStopIcon)
		mButtonLeft = Bounds().right - mStopIcon->Bounds().Width() - 10;
	else
		mButtonLeft = 10000; 
	mUpdate = true;
}

void
DownloadItemView::Pulse()
{
	if(!mUpdate) return;

	bigtime_t now = system_time();
	if(mLastUpdate == -1)
		mLastUpdate = now - ONE_SECOND - 1;
		
	if (now >= mLastUpdate + ONE_SECOND) {
		pulseLock.Lock();
		if(UpdateStatus()){
			Window()->Lock();
			Draw(Bounds());
			Window()->Unlock();
		}
		pulseLock.Unlock();
		mLastUpdate = now;
	}
}

void
DownloadItemView::StopDownload()
{
	if (mStatus != kdStopped && mStatus != kdError && mStatus != kdComplete) {
		mStatus = kdStopped;
		UpdateIconPhase((float)mDone/(float)mOf);
		if (mResource && mResource->LockWithTimeout(ONE_SECOND) == B_OK) {
			mResource->SetStore(NULL);
			mResource->MarkForDeath();
			mResource->Unlock();
			mResource->RefCount(-1);
			mResource = NULL;
			UpdateStatus();
			Window()->Lock();
			Invalidate();
			Window()->Unlock();
		}
	}
}

bool 
DownloadItemView::Trash()
{
	if(wasDragged)
		return false;

	//try to move to trash, but just delete if we can't
	BEntry tEntry(&mEntryRef, false);
	if(tEntry.Exists()){
		BPath trashPath;
		if(find_directory(B_TRASH_DIRECTORY, &trashPath) == B_OK){
			BDirectory tDir;
			if(tDir.SetTo(trashPath.Path()) == B_OK) {
				if(tEntry.MoveTo(&tDir, NULL, true) != B_OK)
					tEntry.Remove();
			} else {
				tEntry.Remove();
			}
		}
		else
			tEntry.Remove();
	}
	return true;
}
	
void 
DownloadItemView::SetSelected(bool isSelected)
{
	mIsSelected = isSelected;
	Invalidate();
	Window()->UpdateIfNeeded();
	//Invalidate();
}

BBitmap *
DownloadItemView::NewDragBitmap()
{
	BRect rect(0,0,32,32);
	if(mBigIcon)
		rect = mBigIcon->Bounds();
	BBitmap *dragBitmap = new BBitmap(rect, B_RGBA32, true);
	dragBitmap->Lock();
	BView *view = new BView(dragBitmap->Bounds(), "", B_FOLLOW_NONE, 0);
	dragBitmap->AddChild(view);
	view->SetOrigin(0, 0);
	BRect clipRect(view->Bounds());
	BRegion newClip;
	newClip.Set(clipRect);
	view->ConstrainClippingRegion(&newClip);

	// Transparent draw magic
	view->SetHighColor(0, 0, 0, 0);
	view->FillRect(view->Bounds());
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetHighColor(0, 0, 0, 192);	// set the level of transparency by
										// value
	view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	if(mBigIcon)
		view->DrawBitmap(mBigIcon);
	view->Sync();
	dragBitmap->Unlock();
	return dragBitmap;
}

bool 
DownloadItemView::Launch()
{
	if(mStatus != kdComplete || !mFileExists)
		return false;
		
	// find out who's responsible for this type of file
	BEntry entry(&mEntryRef, false);
	BPath path;
	entry.GetPath(&path);
	update_mime_info(path.Path(), 0, 1, 1);
	entry_ref theAppRef;
	status_t status = be_roster->FindApp(&mEntryRef, &theAppRef);
	if (status == B_NO_ERROR && mEntryRef != theAppRef) {
		// More security, refuse to run apps from the download folder, period.
		BEntry downloadDir(gPreferences.FindString("DLDirectory"));
		BEntry appDir(&theAppRef);
		appDir.GetParent(&appDir); // the download folder
		if (downloadDir == appDir)
			return false;

		thread_info tInfo;
		get_thread_info(find_thread(NULL), &tInfo);

		app_info aInfo;
		be_roster->GetRunningAppInfo(tInfo.team, &aInfo);
			if (aInfo.ref == theAppRef)
				NetPositive::NewWindowFromURL(mURL);
		else {
			BFile theFile(&mEntryRef, B_READ_ONLY);
			be_roster->Launch(&mEntryRef);
		}
		return true;
	}
	return false;
}

void 
DownloadItemView::NewParentWidth(float newWidth)
{
	if(mStopIcon){
		mButtonLeft = Bounds().left + newWidth - mStopIcon->Bounds().Width() - B_V_SCROLL_BAR_WIDTH - 10;
		Invalidate();
	}
}

bool 
DownloadItemView::Lock()
{
	return pulseLock.Lock();
}

void
DownloadItemView::Unlock()
{
	pulseLock.Unlock();
}

bool 
DownloadItemView::IsLocked()
{
	return pulseLock.IsLocked();
}

bool
DownloadItemView::UpdateStatus()
{
	if(mStatus == kdComplete || mStatus == kdError || mStatus == kdStopped)
		return false;

	bool retval = false;
	long done = mDone;
	long of = mOf;
	StoreStatus status;
	bigtime_t now = system_time();

	if (!mResource || mResource->IsDead()) {
		if(mStatus != kdError){
			mStatus = kdError;
			return true;
		}
		return false;
	}

	if (mResource->LockWithTimeout(100) != B_OK)
		return false;

	status = mResource->GetProgress(&done, &of);

	//so we should have a resource, now figure out any changes in mStatus
	switch(mStatus) {
		case kdUninitialized:
		case kdDownloading:
		case kdWaiting: {
			if (mStatus == kdWaiting && done > 0) { //stopped waiting, time to download
				mStartTime = now;
				mStatus = kdDownloading;
			}
			
			if(mStatus == kdDownloading && (done != mDone || of != mOf) ) {
				int secs = ((now - mStartTime) / ONE_SECOND);
				if (secs > 0)
					mBPS = done / secs;
				else
					mBPS = 0;
			}
			break;
		}
		default:
			if(mResource)
				mResource->Unlock();
			mDone = done == 0 ? mDone : done;
			mOf = of == 0 ? mOf : of;
			return false;
	};

	if(!mFetchedIcon && done > 512){ //get the icon, if we haven't yet and there is enough file
		mFetchedIcon = true;
		BString contentType = mResource->GetContentType();
		FetchIcon();
	}

	switch(status) {
		case kComplete:
		case kAbort:
		case kError:{
			if (!mFetchedIcon || mBigIcon == NULL){
				FetchIcon();
			}
			mStatus = (status == kComplete) ? kdComplete : kError;
			retval = true;
			UResourceImp* tempResource = mResource;
			mResource = NULL;
			tempResource->MarkForDeath();
			tempResource->Unlock();
			tempResource->RefCount(-1);
			tempResource = NULL;
			if (status == kComplete){
				UpdateIconPhase(1);
				FetchIcon();
				FinishFile();
			}
			break;
		}
		default:
			retval = (mDone != done || mOf != of);
			if (mStatus != kdWaiting){
				mStatus = kdDownloading;
			}
			break;
	};

	mDone = done == 0 ? mDone : done;
	mOf = of == 0 ? mOf : of;
	mTotalTime = now - mStartTime;
	UpdateIconPhase((float)mDone/(float)mOf);
	
	if (mResource)
		mResource->Unlock();
	return retval;
}

void
DownloadItemView::FetchIcon()
{
	if (!mBigIcon)
		mBigIcon = new BBitmap(BRect(0,0,31,31), B_CMAP8);
	
	if (!mSmallIcon)
		mSmallIcon = new BBitmap(BRect(0,0,15,15), B_CMAP8);
	
	BNode node(&mEntryRef);
	BNodeInfo nodeInfo(&node);
	status_t status = nodeInfo.GetTrackerIcon(mBigIcon, B_LARGE_ICON);
	if (status != B_OK) {
		delete mBigIcon;
		mBigIcon = NULL;
	}
	status = nodeInfo.GetTrackerIcon(mSmallIcon, B_MINI_ICON);
	if (status != B_OK) {
		delete mSmallIcon;
		mSmallIcon = NULL;
	}
}

void
DownloadItemView::UpdateIconPhase(float percentDone)
{
	if (!mFileExists || !mBigIcon || !mSmallIcon){
		pprint("Couldn't get Tracker Icon for Icon Phase Update");
		return;
	}
	
	BNode node(&mEntryRef);
	BNodeInfo nodeInfo(&node);

	if(percentDone >= 1){
		mFileIconPhase = 1;
		nodeInfo.SetIcon(NULL, B_LARGE_ICON);
		nodeInfo.SetIcon(NULL, B_MINI_ICON);
		return;
	}
	
	bool stopped = false;
	if(mStatus != kdDownloading && mStatus != kdComplete)
		stopped = true;

	int32 barHeight = static_cast<int32>(29 * percentDone);
	if(barHeight <= mFileIconPhase && !stopped) //don't do unnecessary work
		return; 
	mFileIconPhase = barHeight;
		
	BBitmap *phaseBitmap = new BBitmap(mBigIcon, true);
	BBitmap *smallBitmap = new BBitmap(mSmallIcon, true);

	BView *phaseView = new BView(phaseBitmap->Bounds(),"phaseView",B_FOLLOW_ALL,B_WILL_DRAW);
	BView *smallView = new BView(smallBitmap->Bounds(),"smallphaseView",B_FOLLOW_ALL,B_WILL_DRAW);

	phaseBitmap->AddChild(phaseView);
	smallBitmap->AddChild(smallView);

	phaseBitmap->Lock();
	smallBitmap->Lock();

	phaseView->SetDrawingMode(B_OP_OVER);
	smallView->SetDrawingMode(B_OP_OVER);

	phaseView->SetHighColor(0,0,0);
	smallView->SetHighColor(0,0,0);

	phaseView->StrokeRect(BRect(1,1,6,31)); //black shade

	phaseView->SetHighColor(96,96,96);
	smallView->SetHighColor(96,96,96);

	phaseView->StrokeRect(BRect(0,0,5,30)); //grey box around bar
	smallView->StrokeRect(BRect(0,0,3,15)); //grey box around bar

	phaseView->SetHighColor(255,255,255);
	smallView->SetHighColor(255,255,255);

	phaseView->FillRect(BRect(1,1,4,1 + 29 - barHeight)); //white top of bar
	smallView->FillRect(BRect(1,1,2,14 - barHeight/2)); //white top of bar

	if(stopped){
		phaseView->SetHighColor(178,0,0); //red - stopped or error
		smallView->SetHighColor(178,0,0); //red - stopped or error
	}
	else{
		phaseView->SetHighColor(0,203,0); //green - in progress
		smallView->SetHighColor(0,203,0); //green - in progress
	}

	phaseView->FillRect(BRect(1,1 + 29 - barHeight, 4, 29)); //bottom of bar
	smallView->FillRect(BRect(1,1 + 14 - barHeight/2, 2, 14)); //bottom of bar

	phaseView->Sync();
	smallView->Sync();

	phaseBitmap->Unlock();
	smallBitmap->Unlock();

	nodeInfo.SetIcon(phaseBitmap, B_LARGE_ICON);
	nodeInfo.SetIcon(smallBitmap, B_MINI_ICON);

	delete phaseBitmap;
	delete smallBitmap;
}

void
DownloadItemView::FinishFile()
{
	BEntry entry(&mEntryRef, false);
	BPath path;
	entry.GetPath(&path);
	
	if ( (mOpenWhenDone) && (gPreferences.FindBool("AutoLaunchDownloadedFiles")) ){
		// get entry_ref of downloaded file
		entry_ref theAppRef;

		// Force mime update, to prevent Trojan executables from being executed
		// This could be accomplished by sending an empty ("") mimetype along
		// with the file, which would cause (mEntryRef != theAppRef) to be true,
		// whereas the following Launch() would re-sniff file and run it anyway.
		update_mime_info(path.Path(), 0, 1, 1);

		// find out who's responsible for this type of file
		status_t status = be_roster->FindApp(&mEntryRef, &theAppRef);
		if (status == B_NO_ERROR &&
			mEntryRef != theAppRef) {

			// More security, refuse to run apps from the download folder, period.
			// This is to prevent an attacker from sending an executable with a
			// particular supported type, followed by a file of that type.
			BEntry downloadDir(gPreferences.FindString("DLDirectory"));
			BEntry appDir(&theAppRef);
			appDir.GetParent(&appDir); // the download folder
			if (downloadDir == appDir)
				goto error_exit;
			
			thread_info tInfo;
			get_thread_info(find_thread(NULL), &tInfo);

			app_info aInfo;
			be_roster->GetRunningAppInfo(tInfo.team, &aInfo);

			if (aInfo.ref == theAppRef) {
				// if we're the handler, don't bother calling Launch
				NetPositive::NewWindowFromURL(mURL);	// Open it (should send a message....)
				//if it was dragged somewhere else, delete the view
				if(wasDragged){
					StopDownload();
					Trash();
					BMessage deleteMessage(msg_DeleteDownloadItem);
					deleteMessage.AddPointer("itemPtr", this);
					Window()->PostMessage(&deleteMessage);
//					dynamic_cast<DownloadScrollView*>(Parent())->RemoveItem(this);
				}
				return;
			}
			else {
				BFile theFile(&mEntryRef, B_READ_ONLY);
				if (theFile.InitCheck() != B_NO_ERROR) {
					// hmmm, file doesn't exist, what to do?
					(new BAlert(kDownloadErrorTitle, kDownloadErrorMessage, kOKButtonTitle))->Go();
					if(wasDragged){
						StopDownload();
						Trash();
						BMessage deleteMessage(msg_DeleteDownloadItem);
						deleteMessage.AddPointer("itemPtr", this);
						Window()->PostMessage(&deleteMessage);
					}
					return;
				}

				if (be_roster->Launch(&mEntryRef) == B_NO_ERROR){
					if(wasDragged){
						StopDownload();
						Trash();
						BMessage deleteMessage(msg_DeleteDownloadItem);
						deleteMessage.AddPointer("itemPtr", this);
						Window()->PostMessage(&deleteMessage);
					}
					return;
				}
				else
					// opening the file failed, ask Tracker to show the file's directory
					goto error_exit;
			}
		}
		else {
error_exit:;
			if(wasDragged){
				StopDownload();
				Trash();
				BMessage deleteMessage(msg_DeleteDownloadItem);
				deleteMessage.AddPointer("itemPtr", this);
				Window()->PostMessage(&deleteMessage);
			}
		}
	}
	else if(wasDragged){
		StopDownload();
		Trash();
		BMessage deleteMessage(msg_DeleteDownloadItem);
		deleteMessage.AddPointer("itemPtr", this);
		Window()->PostMessage(&deleteMessage);
	}
}

void
DownloadItemView::Init()
{
	BFont font = be_plain_font;
	font.SetSize(10);
	SetFont(&font);
	
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(kWhite, kWhite, kWhite);

	mFetchedIcon = false;
	mButtonDown = false;

	mRetryIcon = TranslateGIF("netpositive:download_retry.gif");
	mRetryPressedIcon = TranslateGIF("netpositive:download_retry-pressed.gif");
	mStopIcon = TranslateGIF("netpositive:download_stop.gif");
	mStopPressedIcon = TranslateGIF("netpositive:download_stop-pressed.gif");
	if(mFileExists && mDone > 512){
		mFetchedIcon = true;
		FetchIcon();
	}		
}

DownloadWindow::DownloadWindow(BRect frame)
	: BWindow(frame, kDownloadWindowTitle, B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
	B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect rect = Bounds();
	rect.bottom = rect.top + kDownloadMenuBarHeight;
	mMenuBar = new BMenuBar(rect, "Download MenuBar");
	AddChild(mMenuBar);
	mFileMenu = BuildMenu("DOWNLOAD_FILE_MENU");
	if(mFileMenu) {
		mFileMenu->SetTargetForItems(this);
		mMenuBar->AddItem(mFileMenu);
	}

	mDownloadsMenu = BuildMenu("DOWNLOAD_DOWNLOADS_MENU");
	if(mDownloadsMenu){
		mDownloadsMenu->SetTargetForItems(this);
		mMenuBar->AddItem(mDownloadsMenu);
	}

	rect = Bounds();
	if(mMenuBar){
		rect.top = mMenuBar->Bounds().bottom + 1;
		mMenuBar->SetLowColor(kGrey,kGrey,kGrey);
	}
	mMainView = new DownloadScrollView(rect);
	AddChild(mMainView);
	
	rect = Bounds();
	rect.top = rect.bottom - B_H_SCROLL_BAR_HEIGHT;
	rect.right -= B_V_SCROLL_BAR_WIDTH - 1;
	mBottomView = new ActivatedView(rect, "DL Bottom View", B_FOLLOW_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
	AddChild(mBottomView);
	mBottomView->SetViewColor(kGrey, kGrey, kGrey);
	
	if (gPreferences.FindBool("AllowKBShortcuts")){
		AddShortcut('N', B_COMMAND_KEY, new BMessage(msg_NewBrowserWindow));
		AddShortcut('A', B_COMMAND_KEY, new BMessage(msg_SelectAll));
		AddShortcut('T', B_COMMAND_KEY, new BMessage(msg_TrashSelected));
	}

	float menuHeight = kDownloadMenuBarHeight;
	if(mMenuBar)
		menuHeight = mMenuBar->Bounds().Height();
	SetSizeLimits(kMinWindowWidth,kMaxWindowWidth,kItemHeight + menuHeight + B_H_SCROLL_BAR_HEIGHT - 1,800);

	SetPulseRate(ONE_SECOND/2);
}

DownloadWindow::~DownloadWindow()
{
	be_app->PostMessage(msg_WindowClosing);
	sDownloadWindow = NULL;
}

void
DownloadWindow::AddDownloadItem(DownloadItemView *item)
{
	mMainView->AddItem(item);
}

void
DownloadWindow::RemoveDownloadItem(DownloadItemView *item)
{
	mMainView->RemoveItem(item);
}

void
DownloadWindow::MessageReceived(BMessage *msg)
{
	switch(msg->what){
		case msg_BeginDownload: {
			BFilePanel *panel = NULL;
			msg->FindPointer("panel", (void**)&panel);
			mMainView->RemoveFilePanel(panel);
			delete panel;
			const char *url = msg->FindString("url");
			UResourceImp *resource = NULL;
			if(msg->FindPointer("resource", (void**)&resource) != B_OK){
				pprint("DownloadWindow::MessageReceived - couldn't get resource from BeginDownload message");
				resource = NULL;
			}

			bool openWhenDone = msg->FindBool("openWhenDone");
			bool wasMoved = msg->FindBool("wasMoved");
			
			entry_ref dirRef;
			if(msg->FindRef("directory", &dirRef) != B_OK){
				pprint("DownloadWindow::MessageReceived - couldn't get directory from message");
				break;
			}

			const char *name = NULL;
			msg->FindString("name", &name);
		
			BFile		file;
			BDirectory	dir;	
			if(dir.SetTo(&dirRef) != B_OK){
				pprint("DownloadWindow::MessageReceived - couldn't open directory from message");
				break;
			}
			if (dir.CreateFile(name, &file) == B_NO_ERROR) {			
				// Create a download window to copy the resource to the store
				// It may do a copy or it may set the resourceImp's store	
				BEntry entry;
				if (dir.FindEntry(name, &entry) == B_NO_ERROR) {
					BPath path;
					entry.GetPath(&path);
					DownloadManager::BeginFileDownload(resource, url, path.Path(), openWhenDone, wasMoved);
				}
			}
			
			break;
		}
		
		case B_CANCEL: {
			BFilePanel *panel = NULL;
			msg->FindPointer("source", (void**)&panel);
			mMainView->RemoveFilePanel(panel);
			
			UResourceImp *resource = NULL;
			msg->FindPointer("resource", (void**)&resource);
			if (resource) {
				resource->SetStore(NULL);
				resource->MarkForDeath();
				resource->RefCount(-1);
			}
			break;
		}
		
		case B_SIMPLE_DATA:
			mMainView->MessageReceived(msg);
			break;

		case CLOSE:
			PostMessage(B_QUIT_REQUESTED);
			break;
		
		case DO_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
				
		case msg_NewBrowserWindow:
			((NetPositive *)be_app)->NewWindow();
			break;
		case msg_AutoLaunch:
			gPreferences.ReplaceBool("AutoLaunchDownloadedFiles", !gPreferences.FindBool("AutoLaunchDownloadedFiles"));
			break;
		case msg_DeleteAll:
			mMainView->RemoveAll();
			break;
		case msg_StopAll:
			mMainView->StopAll();
			break;
		case msg_RetryAllStopped:
			mMainView->RetryAllStopped();
			break;
		case msg_SelectAll:
			mMainView->SelectAll();
			break;
		case msg_TrashSelected:
			mMainView->DeleteAllSelected();
			break;
		case msg_DeleteDownloadItem:{
			void *dlView = NULL;
			msg->FindPointer("itemPtr", &dlView);
			if(dlView)
				mMainView->RemoveItem(dlView);
			break;
		}
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

void
DownloadWindow::MenusBeginning()
{
	mDownloadsMenu->ItemAt(0)->SetMarked(gPreferences.FindBool("AutoLaunchDownloadedFiles"));
	BWindow::MenusBeginning();
	// A comment	
} // end of MenusBeginning()

bool
DownloadWindow::QuitRequested()
{
	int32 numWindows = dynamic_cast<NetPositive*>(be_app)->CountWindows();
	if(dynamic_cast<NetPositive*>(be_app)->IsQuitting() == true
		|| numWindows == 1
		|| (numWindows == 2 && IsMinimized()) ){
		
		if(mMainView->IsStillDownloading()){
			DownloadManager::ShowDownloadWindow();
			BAlert* alert = new BAlert("",kDownloadCancelMessage,kStopDownloadButtonTitle,kCancelDownloadTitle,NULL,B_WIDTH_FROM_LABEL,B_WARNING_ALERT);
			if (alert->Go() == 1)
				return false;
			mMainView->StopAll();
		}
		gPreferences.ReplaceRect("DownloadWindowRect", Frame());
		BMessage msg(msg_WindowClosing);
		msg.AddPointer("window", this);
		BMessenger mMessenger(be_app);
		mMessenger.SendMessage(&msg);
		return BWindow::QuitRequested();
	}


	Minimize(true);
	return false;
			
}

void
DownloadWindow::AddFilePanel(BFilePanel *panel)
{
	mMainView->AddFilePanel(panel);
}

void 
DownloadWindow::Zoom(BPoint /*origin*/, float /*height*/, float /*width*/)
{
	BScreen screen(this);
	BRect scrn_frame(screen.Frame());

	scrn_frame.InsetBy(5, 5);
	scrn_frame.top += 15;	   // keeps title bar of window visible

	BRect frame(Frame());

	// move frame left top on screen
	BPoint left_top(frame.LeftTop());
	left_top.ConstrainTo(scrn_frame);
	frame.OffsetTo(left_top);

	frame.right = frame.left + kWindowHSize;
	float menuHeight = kDownloadMenuBarHeight;
	if(mMenuBar)
		menuHeight = mMenuBar->Bounds().Height();
	frame.bottom = frame.top + (mMainView->CountItems() * kItemHeight) + menuHeight + B_H_SCROLL_BAR_HEIGHT - 1;

	// make sure entire window fits on screen
	frame = frame & scrn_frame;

	ResizeTo(frame.Width(), frame.Height());
	MoveTo(frame.LeftTop());
}

void
DownloadManager::DownloadFile(const char *url, bool openWhenDone, bool forceFilePanel)
{
	ShowDownloadWindow();
	BString downloadPath;
	if(!forceFilePanel){
		if(GetDefaultDownloadPath(&downloadPath, url) == B_OK)
			BeginFileDownload(NULL, url, downloadPath.String(), openWhenDone);
	}
	else{
		BMessage msg(msg_BeginDownload);
		msg.AddString("url", url);
		msg.AddBool("openWhenDone", openWhenDone);
		msg.AddBool("wasMoved", true);
		BString suggestedName;
		GetSuggestedName(&suggestedName, url);
		ShowFilePanel(suggestedName.String(), msg);
	}
}


void
DownloadManager::DownloadFile(UResourceImp *resource, ConnectionManager *srcMgr, bool openWhenDone, bool forceFilePanel)
{
	ShowDownloadWindow();

	resource->RefCount(1);
	UResourceCache::DontCache(resource);

	if (!sDownloadConnectionMgr->AdoptResource(resource, srcMgr)) {
		pprint("DownloadManager::DownloadFile failed at Adopting Resource 0x%x", resource);
		BString url = resource->GetURL();
		if (resource->LockWithTimeout(ONE_SECOND) == B_OK) {
			resource->SetStore(NULL);
			resource->MarkForDeath();
			resource->Unlock();
			resource->RefCount(-1);
			resource = NULL;
		} else {
			resource->MarkForDeath();
			resource->RefCount(-1);
			resource = NULL;
		}
		DownloadFile(url.String(), openWhenDone);
		return;
	}
	
	BString downloadPath;

	if(!forceFilePanel)
		GetDefaultDownloadPath(&downloadPath, resource->GetURL());
	if (downloadPath.Length())
		BeginFileDownload(resource, NULL, downloadPath.String(), openWhenDone);
	else {
		BMessage msg(msg_BeginDownload);
		msg.AddPointer("resource", resource);
		msg.AddBool("openWhenDone", openWhenDone);
		msg.AddBool("wasMoved", true);
		BString suggestedName;
		GetSuggestedName(&suggestedName, resource->GetURL());
		ShowFilePanel(suggestedName.String(), msg);
	}
}

void
DownloadManager::ShowDownloadWindow(bool force)
{
	if (sDownloadWindow == NULL) {
		BRect rect;
		if (gPreferences.FindRect("DownloadWindowRect", &rect) != B_OK ||
			rect.Width() != kWindowHSize || rect.Height() == 0) {
			rect.Set(0,0,kWindowHSize,200);
			BScreen screen( B_MAIN_SCREEN_ID );
			BRect screenR = screen.Frame();
			rect.OffsetTo(screenR.right - rect.Width(), screenR.bottom - rect.Height());		
		}
		sDownloadWindow = new DownloadWindow(rect);
		if (!force && !gPreferences.FindBool("AutoShowDownloadWindow"))
			sDownloadWindow->Minimize(true);
		sDownloadWindow->Show();
	} else if (force || gPreferences.FindBool("AutoShowDownloadWindow"))
		sDownloadWindow->Show();

	if (force)
		sDownloadWindow->Minimize(false);
}

BWindow*
DownloadManager::GetDownloadWindow()
{
	return sDownloadWindow;
}

bool
DownloadManager::RequestQuit()
{
	if (sDownloadWindow)
		return sDownloadWindow->QuitRequested();

	return true;
}

void
DownloadManager::BeginFileDownload(UResourceImp *resource, const char *url, const char *downloadPath, bool openWhenDone, bool wasMoved)
{
	BString urlString;
	if(url)
		urlString.SetTo(url);
		
	if (resource) {
		// If the resource is already closed in error, try again.
		if (resource->LockWithTimeout(ONE_SECOND) == B_OK) {
			if (resource->GetStatus() == kError) {
				// Get the URL from the resource, dereference it, and set it to NULL.  We'll try to re-get
				// it below.
				resource->Unlock();
				urlString = resource->GetURL();
				resource->MarkForDeath();
				resource->RefCount(-1); 
				resource = NULL;
			} else {
				resource->SetDownloadPath(downloadPath);
				resource->Unlock();
			}
		}
	} else if (urlString.Length() > 0 && !resource) {
		BString error;
		resource = GetUResource(sDownloadConnectionMgr, url, 0, error, true, downloadPath, NULL, NULL, true);
		if(!resource){
			pprint("DownloadManager::BeginFileDownload - could not get resource from URL");
			return;
		}
		UResourceCache::DontCache(resource);
	}
	
	BString newName(downloadPath);
	int slashIndex = newName.FindLast("/");
	if(slashIndex != 0)
		newName.Remove(0,slashIndex + 1);
	DownloadItemView *view = new DownloadItemView(resource, newName.String(), downloadPath, kdWaiting, openWhenDone, wasMoved);
	BAutolock lock(sDownloadWindow);
	sDownloadWindow->AddDownloadItem(view);
}

status_t
DownloadManager::GetDefaultDownloadPath(BString *result, const char *url)
{
	status_t error;	
	BString fileName;
	GetSuggestedName(&fileName, url);
	
	BString newDefaultDir = gPreferences.FindString("DLDirectory");
	BDirectory dir;
	error = dir.SetTo(newDefaultDir.String());
	if(error != B_OK){
		if(error != B_ENTRY_NOT_FOUND)
			return error;
		
		// no download dir yet, create one
		error = create_directory(newDefaultDir.String(), 0755);
		if (error != B_OK)
			return error;

		error = dir.SetTo(newDefaultDir.String());
		if (error != B_OK)
			//give up
			return error;
	}				

	BEntry entry;
	if (dir.FindEntry(fileName.String(), &entry) == B_OK){
		// we got a conflict, resolve it by appending numbers to the
		// end of the file name
		BString path;
		int32 num = 2;
		while (true) {
			path = newDefaultDir;
			path << fileName << " " << (int32)num;
			if (!BEntry(path.String()).Exists()) {
				// got a unique name, we are done
				fileName << " " << (int32)num;
				break;
			}
			num++;
		}
	}

	BFile file;
	error = dir.CreateFile(fileName.String(), &file);
	if (error != B_OK)
		return error;
		
	*result = newDefaultDir;
	*result += fileName;
	return B_OK;
}

void
DownloadManager::ShowFilePanel(const char *suggestedName, BMessage& openMessage)
{
	ShowDownloadWindow();
	
	BEntry defaultDir(gPreferences.FindString("DLDirectory"));
	entry_ref ref;
	defaultDir.GetRef(&ref);

	BFilePanel *panel = new BFilePanel(B_SAVE_PANEL, 0, &ref);
	
	sDownloadWindow->AddFilePanel(panel);

	openMessage.AddPointer("panel", panel);

	// Note.  We have to have the download window open so that the file panel will have
	// something to target.  We can't assume that our BApplication will be around because
	// we could be a replicant.
	panel->SetTarget(sDownloadWindow);
	panel->SetMessage(&openMessage);
	panel->SetSaveText(suggestedName);
	panel->Show();	
}

void
DownloadManager::GetSuggestedName(BString *result, const char *url)
{
	const char *fileName = url;
	const char *sub = NULL;
	while ((sub = strstr(fileName, "/")) != NULL)
		fileName = sub + 1;
	if (fileName[0] == '\0')
		fileName = kDefaultSavedFileName;

	//CleanName puts a string <= fileName in result
	CleanName(fileName, result->LockBuffer(strlen(fileName)) );
	result->UnlockBuffer();	
}

