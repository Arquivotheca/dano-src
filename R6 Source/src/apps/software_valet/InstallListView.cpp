// InstallListView.cpp

#include "InstallListView.h"
#include "PackageItem.h"
#include "DoIcons.h"
#include "SettingsManager.h"

#include <NodeMonitor.h>
#include <Directory.h>
#include <Looper.h>
#include <NodeInfo.h>

extern SettingsManager *gSettings;

#include "MyDebug.h"

bool IsPackage(entry_ref *ref);

extern const char *kDPkgSig;
extern const char *kUPkgSig;


InstallListView::InstallListView(BRect r,
								BHandler *_fTarget)
	:	SimpleListView(r,"listing",
						B_FOLLOW_ALL,
						B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  fTarget(_fTarget),
	  IHeight(34)
{
	SetItemList((RList<ListItem *> *)new RList<UPackageItem *>);
	
	SetMultipleSelect(TRUE);
	SetDragReorder(TRUE);
	
	SetItemHeight(IHeight);
	SetBaselineOffset(2.0);
	// groupList->DeSelectAll();
}

InstallListView::~InstallListView()
{
	// cancel all watches ???
	stop_watching(this);
	
	for (int i = fWatchList.CountItems()-1; i >= 0; i--) {
		delete fWatchList.ItemAt(i);
	}
	// delete query;
}

void InstallListView::AttachedToWindow()
{
	SimpleListView::AttachedToWindow();
	
	SetFont(be_plain_font);
	SetFontSize(9);
	SetItemHeight(IHeight);
		
	// nMonitor.SetMessenger(BMessenger(this));
	// get from prefs!!!, update dynamically???
	BDirectory	pDir(gSettings->data.FindString("download/path"));
	entry_ref	ref;
	while (pDir.GetNextRef(&ref) >= B_NO_ERROR) {
		if (IsPackage(&ref)) {
			AddItem(new UPackageItem(ref));
		}
		else {
			WatchEntry(&ref,true);
		}
	}
	
	/**
	query = new BQuery();
	
	BVolume	vol;
	BVolumeRoster rost;
	rost.GetBootVolume(&vol);
	query->SetVolume(&vol);
	query->PushAttr("BEOS:TYPE");
	query->PushString(kUPkgSig);
	query->PushOp(B_EQ);
	query->SetTarget(BMessenger(this));
	query->Fetch();
	***/
	
	node_ref	nref;
	pDir.GetNodeRef(&nref);
	
	watch_node(&nref,B_WATCH_DIRECTORY,BMessenger(this));
	directoryID = nref.node;
	
	SelectNoItems();
	Update();
	MakeFocus(TRUE);
}

void InstallListView::DrawItem(BRect updt, long item, BRect *iFrame)
{
	updt;
	
	UPackageItem *i = (UPackageItem *)ItemAt(item);
	
	BRect r;
	if (iFrame)
		r = *iFrame;
	else
		r = ItemFrame(item);	
	// FillRect(r, B_SOLID_LOW);

	DrawBitmapAsync(gPkgIcon,BPoint(r.left + 4, r.top + 1));

	DrawString(i->fileRef.name,BPoint(r.left + 40, r.top + 20));
	//DrawString("Version 1.1",BPoint(r.left + 40,r.top + 24));
	//DrawString("Dumbass Software, Inc.",BPoint(r.left + 40,r.top + 36));
	
	SetHighColor(200,80,80);
	StrokeLine(r.LeftBottom(),r.RightBottom());
	SetHighColor(0,0,0);
	if (i->selected) {
		PRINT(("item is selected!\n"));
	
		HighlightItem(TRUE,item,&r);	
	}
}


void InstallListView::HighlightItem(bool on, long index,BRect *iFrame)
{
	if (on) {
		SetDrawingMode(B_OP_SUBTRACT);
		SetHighColor(60,60,60);
		FillRect(iFrame ? *iFrame : ItemFrame(index));
		SetDrawingMode(B_OP_COPY);
		SetHighColor(0,0,0);
	}
}


void InstallListView::SelectionSet()
{
	BMessage *selMsg = new BMessage(M_ITEMS_SELECTED);
	if (LowSelection() >= 0) {		
		if (LowSelection() == HighSelection()) {
			// single item selected
		}
		else {
			// multiple items selected	
		}
		selMsg->AddBool("selected",true);
	}
	else {
		// nothing selected
		selMsg->AddBool("selected",false);
	}
	
	fTarget->Looper()->PostMessage(selMsg,fTarget);
}

void InstallListView::MessageReceived(BMessage *msg)
{
#if DEBUG
			printf("got some message\n");
			msg->PrintToStream();
#endif
	switch(msg->what) {
		case 'Open':
			if (LowSelection() >= 0)
				Invoke(LowSelection());
			break;
		case B_NODE_MONITOR: {

			long opcode = msg->FindInt32("opcode");
#if DEBUG
			printf("opcode is %d\n",opcode);
#endif
			switch(opcode) {
				case B_STAT_CHANGED:
				case B_ATTR_CHANGED: {
					node_ref cref;
					msg->FindInt32("device",&cref.device);
					msg->FindInt64("node",&cref.node);
					
					for (int i = fWatchList.CountItems()-1; i >= 0; i--) {
						entry_ref	*ref = fWatchList.ItemAt(i);
						BNode	n(ref);
						node_ref	ex;
						n.GetNodeRef(&ex);
						if (ex == cref) {
							//
							if (IsPackage(ref)) {
								watch_node(&ex,	B_STOP_WATCHING,BMessenger(this));
								fWatchList.RemoveIndex(i);
								InsertEntry(*ref, ex.node);
							}
							break;
						}
					}
					break;
				}
				case B_ENTRY_CREATED: {
					entry_ref	ref;
					GetMsgEntry(msg,ref,"directory");
					ino_t	node;
					msg->FindInt64("node",&node);
					// adding possibly with watch
					InsertEntry(ref, node);
					break;
				}
				case B_ENTRY_REMOVED: {
					node_ref rmed;
					msg->FindInt32("device",&rmed.device);
					msg->FindInt64("node",&rmed.node);
					
					// might not be in our list
					RemoveNode(&rmed);
					// but be sure to stop watching
					
					ino_t	dir;
					msg->FindInt64("directory",&dir);
					
					entry_ref	r(rmed.device, dir, msg->FindString("name"));
					
					// stop watching and remove from list (if present)
					WatchEntry(&r,false);
					break;
				}
				case B_ENTRY_MOVED: {
					// entry moved into or out of this directory
					dev_t fileDev;
					ino_t fromDir;
					ino_t toDir;
					
					msg->FindInt64("from directory",&fromDir);
					msg->FindInt64("to directory",&toDir);
					fileDev = msg->FindInt32("device");

					if (toDir == directoryID) {
						PRINT(("moving to directory\n"));
						
						entry_ref r(fileDev,toDir,msg->FindString("name"));
						
						// adding with possible watch
						ino_t	node;
						msg->FindInt64("node",&node);
						InsertEntry(r,node);
					}
					else if (fromDir == directoryID) {
						PRINT(("moving from directory\n"));
						// removing
						node_ref rmed;
						rmed.device = fileDev;
						msg->FindInt64("node",&rmed.node);
						RemoveNode(&rmed);
						
						// be sure to stop watching
						watch_node(&rmed,B_STOP_WATCHING,BMessenger(this));
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			SimpleListView::MessageReceived(msg);
			break;
	}
}

void InstallListView::RemoveNode(node_ref *ref)
{
	bool removed = FALSE;
	long count = CountItems();
	for (long i = 0; i < count; i++) {
		UPackageItem *it = (UPackageItem *)ItemAt(i);
		if (ref->node == it->inode) {
			RemoveItem(i);
			delete it;
			Update();
			break;
		}
	}
}

status_t InstallListView::GetMsgEntry(BMessage *msg, entry_ref &ref,const char *opcode)
{
	status_t err;
	err = msg->FindInt32("device",&ref.device);
	err = msg->FindInt64(opcode,&ref.directory);
	if (err == B_NO_ERROR) {
		const char *name;
		msg->FindString("name",&name);
		ref.set_name(name);
		return err;
	}
	return err;
}

void InstallListView::InsertEntry(entry_ref &ref, ino_t node)
{
	// check still downloading...
	if (IsPackage(&ref)) {
		// this is a valid package
		int i;
		for (i = CountItems()-1; i >= 0; i--) {
			UPackageItem *it = (UPackageItem *)ItemAt(i);
			if (it->inode == node) {
				RemoveItem(i);
				delete it;
				break;
			}
		}
		for (i = CountItems()-1; i >= 0; i--) {
			UPackageItem *it = (UPackageItem *)ItemAt(i);
			int cmp = strcmp(ref.name, it->fileRef.name);
			if (cmp > 0) {
				break;
			}
		}
		i++;	// adjust location
		ItemList()->AddItem(new UPackageItem(ref),i);
		Update();
	}
	else {
		// add to the watch list
		WatchEntry(&ref,true);
		// to see if this becomes a valid package
	}
}

bool InstallListView::StillDownloading(entry_ref *ref)
{
	bool v = FALSE;
		
	BNode n(ref);
	
	if (n.InitCheck() == B_NO_ERROR &&
			n.Lock() == B_NO_ERROR) {
		n.ReadAttr("STILL_DOWNLOADING", B_BOOL_TYPE,
			0, &v, sizeof(v));
		n.Unlock();	
	}
	return v;
}

status_t InstallListView::WatchEntry(entry_ref *ref, bool start)
{
	status_t err;
	BNode n(ref);
	node_ref node;
	if ((err = n.InitCheck()) < B_NO_ERROR)
		return err;
		
	n.GetNodeRef(&node);
	
	if (!start) {
		for (int i = fWatchList.CountItems()-1; i >= 0; i--) {
			if (*fWatchList.ItemAt(i) == *ref) {
				delete fWatchList.RemoveIndex(i);
			}
		}
		return watch_node(&node,B_STOP_WATCHING,BMessenger(this));
	}
	else {
		int i;
		for (i = fWatchList.CountItems()-1; i >= 0; i--) {
			if (*fWatchList.ItemAt(i) == *ref)
				break;
		}
		if (i < 0) {
			fWatchList.AddItem(new entry_ref(*ref));
			return watch_node(&node,B_WATCH_ATTR | B_WATCH_STAT,BMessenger(this));
		}
	}
	return B_NO_ERROR;
}

void InstallListView::Invoke(long index)
{
	UPackageItem *item = (UPackageItem *)ItemAt(index);
	
	BMessage *m = new BMessage(B_REFS_RECEIVED);
	m->AddRef("refs",&item->fileRef);
	fTarget->Looper()->PostMessage(m,fTarget);
}


bool IsPackage(entry_ref *ref)
{
	BNode	node(ref);
	BNodeInfo	ninf(&node);
	
	char type[B_MIME_TYPE_LENGTH];
	ninf.GetType(type);
	
	if (!strcmp(type, kUPkgSig) ||
		!strcmp(type, kDPkgSig)) {
		BFile		file(ref,O_RDONLY);
		// BNodeInfo	ninf(&node);
		int32	code;
		
		ssize_t	res = file.Read(&code,sizeof(code));
		if (res == sizeof(code)) {
			return code == 'AlB\032';
		}
	}
	return false;
}
