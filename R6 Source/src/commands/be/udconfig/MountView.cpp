//
// Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Message.h>
#include <Invoker.h>
#include <Button.h>
#include <ListView.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <private/storage/DeviceMap.h>
#include <ListItem.h>
#include <ScrollView.h>
#include <Box.h>
#include <StringView.h>
#include <Alert.h>
#include <Application.h>

#include "MountView.h"
#include "VolumeItem.h"

#define DISK_ROOT	"/dev/disk"

#define MSG_SCAN_DONE		'sCAN'
#define MSG_MOUNT_DONE		'mONT'
#define MSG_BUTTON_MOUNT	'bMNT'
#define MSG_BUTTON_RESCAN	'bSCN'

#define MSG_VOLUME_LIST_INVOKED		'lINV'
#define MSG_VOLUME_LIST_SELECTED	'lSEL'

#define DEFAULT_BUTTON_OFFSET	4


MountView::MountView(BRect frame, const char *name, uint32 resizingMode, uint32 flags, const char *fname, BInvoker *invoker) :
	BView(frame, name, resizingMode, flags),
	fEm(be_bold_font->StringWidth("m")),
	fErr(B_NO_INIT),
	fFileName(strdup(fname)),
	fVolumeName(NULL),
	fDoneInvoker(invoker),
	fScanThread((thread_id)B_BAD_THREAD_ID),
	fVolumeListView(NULL),
	fMountButton(NULL),
	fRescanButton(NULL)
{

	BRect r(Bounds());
	AddFileName(r);
	r.top += fEm/2;
	AddButtons(r);
	r.bottom -= fEm/2;
	AddVolumeList(r);

	fErr = ScanVolumes();

	int vlen;
	char *c = strchr(fFileName+1, '/');
	if(c) {
		vlen = c - fFileName;
	}
	else {
		vlen = strlen(fFileName);
	}
	fVolumeName = (char*)malloc(vlen+1);
	memcpy(fVolumeName, fFileName, vlen);
	fVolumeName[vlen] = '\0';
}

MountView::~MountView()
{
	free(fFileName);
	free(fVolumeName);
	delete fDoneInvoker;
}

status_t
MountView::InitCheck()
{
	return fErr;
}

void 
MountView::AttachedToWindow()
{
	if(Parent()) {
		SetViewColor(Parent()->ViewColor());
	}

	fVolumeListView->SetTarget(this);
	fMountButton->SetTarget(this);
	fRescanButton->SetTarget(this);
}

void 
MountView::AllAttached()
{
}

void 
MountView::FrameResized(float width, float height)
{
}

void 
MountView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	case MSG_SCAN_DONE:
		if(EvaluateFile()) {
			return;
		}
		UpdateVolumeList();
		fScanThread = (thread_id)B_BAD_THREAD_ID;
		fRescanButton->SetEnabled(true);
		break;
	case MSG_MOUNT_DONE:
		if(EvaluateFile()) {
			return;
		}
		fMountThread = (thread_id)B_BAD_THREAD_ID;
		ScanVolumes();
		break;
	case MSG_VOLUME_LIST_INVOKED:
	case MSG_BUTTON_MOUNT:
		MountVolumes();
		break;
	case MSG_BUTTON_RESCAN:
		ScanVolumes();
		break;
	case MSG_VOLUME_LIST_SELECTED:
		HandleSelection();
		break;
	default:
		BView::MessageReceived(msg);
	}
}

bool
MountView::EvaluateFile()
{
	struct stat st;
	if(stat(fFileName, &st) >= 0) {
		fDoneInvoker->Invoke();
		return true;
	}

	int err = stat(fVolumeName, &st);
	if(err >= 0) {
		char *buf = (char*)malloc(strlen(fFileName) + 128);
		sprintf(buf, "The file \"%s\" could not be found on the mounted volume \"%s\".", fFileName, fVolumeName);
		BAlert *a = new BAlert("File Not Found", buf, "Quit", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
		free(buf);
		a->Go();
		be_app->PostMessage(B_QUIT_REQUESTED, be_app);
		return true;
	}

	return false;
}

void
MountView::FreeDeviceList()
{
	int nitems;
	int i;

	nitems = fDeviceList.CountItems();
	for(i = 0; i < 0; i++) {
		Device *d = (Device*)fDeviceList.ItemAt(i);
		delete d;
	}
	fDeviceList.MakeEmpty();
}

void
MountView::AddDevice(const char *path)
{
	struct DevParams {
		bool changed;
	} params;

	Device *d = new Device(path);
	if(d != NULL) {
		// actually check to see if the volumes are mounted
		//
		d->FindMountedVolumes((void*)&params);
		fDeviceList.AddItem((void*)d);
	}
}

void
MountView::RecurseDir(const char *directory)
{
	BDirectory      dir;
	BEntry          entry;
	BPath           name;
	partition_info  info;
	int32           dev;
	bool            add;

	dir.SetTo(directory);
	if (dir.InitCheck() == B_NO_ERROR) {
		dir.Rewind();

		while (dir.GetNextEntry(&entry) >= 0) {
			entry.GetPath(&name);

			if (entry.IsDirectory()) {
				RecurseDir(name.Path());
			}
			else if (!strstr(name.Path(), "rescan")) {
				add = true;

				if ((dev = open(name.Path(), 0)) >= 0) {
					if (ioctl(dev, B_GET_PARTITION_INFO, &info) == B_NO_ERROR) {
						add = false;
					}
					close(dev);
				}
				if (add) {
					AddDevice(name.Path());
				}
			}
		}
	}
}

void
MountView::BuildDeviceList()
{
	FreeDeviceList();
	RecurseDir(DISK_ROOT);
}

int32
MountView::build_device_list(void *arg)
{
	MountView *mv = (MountView*)arg;
	if(mv == NULL) {
		return B_BAD_VALUE;
	}
	mv->BuildDeviceList();

	BMessenger(mv).SendMessage(new BMessage(MSG_SCAN_DONE));

	return B_OK;
}

status_t
MountView::ScanVolumes()
{
	status_t err;

	if(fScanThread >= B_OK) {
		return B_BUSY;
	}

	fRescanButton->SetEnabled(false);
	fMountButton->SetEnabled(false);

	fVolumeListView->MakeEmpty();
	fVolumeListView->AddItem(new BStringItem("Scanning Volumes..."));

	fScanThread = spawn_thread(build_device_list, "Scan Thread", B_NORMAL_PRIORITY, (void*)this);
	err = resume_thread(fScanThread);
	if(err != B_OK) {
		fScanThread = (thread_id)err;
	}
	return err;
}

void
MountView::MountThread()
{
	int nitems;
	int i;

	nitems = fMountList.CountItems();
	for(i = 0; i < nitems; i++) {
		Partition *p = (Partition*)fMountList.ItemAt(i);
		p->Mount();
	}
}

int32
MountView::mount_selected_volumes(void *mount_view)
{
	MountView *mv = (MountView*)mount_view;
	mv->MountThread();
	BMessenger(mv).SendMessage(new BMessage(MSG_MOUNT_DONE));

	return B_OK;
}

void
MountView::MountVolumes()
{
	int32 index;
	int32 cookie = 0;
	status_t err;

	if(fScanThread >= B_OK) {
		return;
	}

	fRescanButton->SetEnabled(false);
	fMountButton->SetEnabled(false);

	fMountList.MakeEmpty();

	while((index = fVolumeListView->CurrentSelection(cookie++)) >= 0) {
		VolumeItem *vi;
		
		vi = dynamic_cast<VolumeItem*>(fVolumeListView->ItemAt(index));
		if(vi == NULL) {
			continue;
		}
		fMountList.AddItem((void*)vi->GetPartition());
	}

	fVolumeListView->MakeEmpty();
	fVolumeListView->AddItem(new BStringItem("Mounting..."));

	fMountThread = spawn_thread(mount_selected_volumes, "Mount Thread", B_NORMAL_PRIORITY, (void*)this);
	err = resume_thread(fMountThread);
	if(err != B_OK) {
		fMountThread = (thread_id)err;
	}
}

void
MountView::UpdateVolumeList()
{
	char buf[512];
	char path[256];
	VolumeItem *vi;
	int nitems;
	int i;

	if(fVolumeListView == NULL) {
		return;
	}
	fVolumeListView->MakeEmpty();

	nitems = fDeviceList.CountItems();
	for(i = 0; i < nitems; i++) {
		int nsessions;
		int j;

		Device *d = (Device*)fDeviceList.ItemAt(i);
		if(d == NULL) {
			continue;
		}

		nsessions = d->CountSessions();
		for(j = 0; j < nsessions; j++) {
			int npartitions;
			int k;

			Session *s = d->SessionAt(j);
			npartitions = s->CountPartitions();
			for(k = 0; k < npartitions; k++) {
				Partition *p = s->PartitionAt(k);
				const char *fs = p->FileSystemShortName();

				if(fs == NULL || fs[0] == '\0' || strcmp(fs, "unknown") == 0) {
					continue;
				}

				vi = new VolumeItem(p);
				vi->SetEnabled(p->Mounted() != kMounted);
				fVolumeListView->AddItem(vi);
				if(strcmp(p->VolumeName(), fVolumeName+1) == 0) {
					fVolumeListView->Select(fVolumeListView->CountItems()-1, true);
				}
			}
		}
	}
}

void
MountView::AddFileName(BRect &r)
{
	BRect br = r;
	br.bottom = br.top + 4 * fEm;

	BBox *b = new BBox(br, "Box:FileName", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	AddChild(b);

	br.InsetBy(fEm,fEm);
	br.bottom -= fEm/4;

	BStringView *s = new BStringView(br, "String:FileName", fFileName, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	b->AddChild(s);
	s->SetFont(be_bold_font);

	r.top += b->Bounds().Height();
}

void
MountView::AddVolumeList(BRect &r)
{
	BRect vr(r.InsetByCopy(3, 3));
	vr.right -= B_V_SCROLL_BAR_WIDTH;

	fVolumeListView = new BListView(vr, "List:Volume", B_MULTIPLE_SELECTION_LIST,
			B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
	BScrollView *sv = new BScrollView("Scroll:Volume", fVolumeListView,
			B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	AddChild(sv);

	fVolumeListView->TargetedByScrollView(sv);
	fVolumeListView->SetFont(be_bold_font);
	fVolumeListView->SetInvocationMessage(new BMessage(MSG_VOLUME_LIST_INVOKED));
	fVolumeListView->SetSelectionMessage(new BMessage(MSG_VOLUME_LIST_SELECTED));
}

void
MountView::AddButtons(BRect &r)
{
	BRect br;
	BButton *b;
	float w, h, f;

	br = r.InsetByCopy(DEFAULT_BUTTON_OFFSET, DEFAULT_BUTTON_OFFSET);

	b = new BButton(br, "Button:Mount", "Mount Selected", new BMessage(MSG_BUTTON_MOUNT), B_FOLLOW_BOTTOM|B_FOLLOW_LEFT);
	AddChild(b);
	b->ResizeToPreferred();
	b->GetPreferredSize(&w, &h);
	b->MoveTo(br.left, br.bottom-h);
	br.left += w + fEm/2 + DEFAULT_BUTTON_OFFSET;

	b->SetEnabled(false);
	b->MakeDefault(true);
	fMountButton = b;

	b = new BButton(br, "Button:Rescan", "Rescan Devices", new BMessage(MSG_BUTTON_RESCAN), B_FOLLOW_BOTTOM|B_FOLLOW_LEFT);
	AddChild(b);
	b->ResizeTo(w, h);
	b->MoveTo(br.left, br.bottom-h);
//	br.left += w + fEm/2;

	b->SetEnabled(false);
	fRescanButton = b;

	r.bottom -= h + 2 * DEFAULT_BUTTON_OFFSET;
}

void
MountView::HandleSelection()
{
	fMountButton->SetEnabled(fVolumeListView->CurrentSelection() >= 0);
}
