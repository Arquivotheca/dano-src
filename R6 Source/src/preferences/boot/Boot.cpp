// **************************************************************************
//	
//	Boot.cpp
//
//	Written by: Robert Polic, lots of rehacking by Robert Chinn
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
// **************************************************************************

#include <Debug.h>

#ifndef BOOT_H
#include "Boot.h"
#endif
#ifndef _SCSIPROBE_DRIVER_H
#include <scsiprobe_driver.h>
#endif
#ifndef _IDE_CALLS_H
#include <ide_calls.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <nvram.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fs_info.h>
#include <priv_syscalls.h>

#include <Alert.h>
#include <Beep.h>
#include <Directory.h>
#include <Drivers.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <ListItem.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StorageDefs.h>
#include <Volume.h>

#include <interface_misc.h>

const int32 kButtonWidth = 75;			// same as B_WIDTH_AS_USUAL

#ifdef _BUILD_INTEL
#define DEFAULT_PATH		"/%s/home/config/settings"
#define FLAG_NAME			"Is_Default_Volume"
#endif					
const char* const kPrefsFileName = "Boot_settings";

enum
{
	msg_easy_mode = 1,
	msg_expert_mode,
	msg_scsi,
	msg_ide,
	msg_scsi_bus,
	msg_scsi_id,
	msg_scsi_lun,
	msg_ide_bus,
	msg_ide_device,
	msg_single_click,
	msg_double_click,
	msg_scsi_partition,
	msg_ide_partition,
	msg_toggle_mode
};

static void
ExtractSCSIInfo (const char *deviceName, int *bus, int *id, int *lun)
{
	sscanf (deviceName, "/dev/disk/scsi/%d/%d/%d/", bus, id, lun);
}

static void
ExtractIDEInfo (const char *deviceName, int *device, int *bus, int *id, int *lun)
{
	char devicestring[6]; // strlen("atapi")+1
	char idstring[7]; // strlen("master")+1

	if(sscanf(deviceName, "/dev/disk/ide/%5[a-z]/%d/%6[a-z]/%d/", devicestring, bus, idstring, lun) == 4) {
		if(strcmp("ata", devicestring) == 0) {
			*device = bootdev_ata;
		}
		else if(strcmp("ata", devicestring) == 0) {
			*device = bootdev_atapi;
		}

		if(strcmp("master", idstring) == 0) {
			*id = 0;
		}
		else if(strcmp("slave", idstring) == 0) {
			*id = 1;
		}
	}
}
	
static void
GetDeviceName(fs_info info, char* dev_name, int32 *partition, int32 *session)
{
	*session = *partition = 0;
	if (!strstr(info.device_name, "/raw")) {
		int32 fd;
		if ((fd = open(info.device_name, 0)) >= 0) {
			partition_info	p_info;
			
			if (ioctl(fd, B_GET_PARTITION_INFO, &p_info) == B_NO_ERROR) {
				strcpy(dev_name, (char*)p_info.device);
				*session = p_info.session;
				*partition = p_info.partition;
			} else
				strcpy(dev_name, (char*)&info.device_name);
				
			close(fd);
		}
	} else
		strcpy(dev_name, (char*)&info.device_name);
}

// **************************************************************************

int main(void)
{	
	TApp app;
	app.Run();
	return B_NO_ERROR;
}

// **************************************************************************

TApp::TApp()
	:BApplication("application/x-vnd.Be-BOOT")
{
	TWindow* w = new TWindow();
	w->Show();
}

TApp::~TApp()
{
}

void
TApp::MessageReceived(BMessage* m)
{
	switch(m->what) {
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		default:
			BApplication::MessageReceived(m);
			break;
	}
}

void
TApp::AboutRequested()
{
	(new BAlert("", "Set your boot volume here.", "OK"))->Go();
}

// **************************************************************************

#define WIND_WIDTH			250
#define WIND_HEIGHT			248

TWindow::TWindow()
	:BWindow(BRect(0, 0, WIND_WIDTH, WIND_HEIGHT), "Boot",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	short mode;
	GetPrefs(&mode);
	
	BRect r(Bounds());
	r.InsetBy(-1, -1);	
	fMainView = new TBootView(r, mode);
	AddChild(fMainView);

	watch_node(NULL, B_WATCH_MOUNT, this);

	AddShortcut('I', B_COMMAND_KEY, new BMessage(B_ABOUT_REQUESTED));
#ifndef _BUILD_INTEL
	AddShortcut('T', B_COMMAND_KEY, new BMessage(msg_toggle_mode), fMainView);
#endif
}

TWindow::~TWindow(void)
{
	stop_watching(this);
}

void
TWindow::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case B_ABOUT_REQUESTED:
			be_app->MessageReceived(msg);
			break;

		case msg_single_click:
		case msg_double_click:
			fMainView->MessageReceived(msg);
			break;

#ifndef _BUILD_INTEL
		case msg_expert_mode:
		case msg_scsi:
		case msg_ide:
		case msg_scsi_bus:
		case msg_scsi_id:
		case msg_scsi_lun:
		case msg_ide_bus:
		case msg_ide_device:
		case msg_scsi_partition:
		case msg_ide_partition:
#endif
		case msg_easy_mode:
		case B_SIMPLE_DATA:
		case B_NODE_MONITOR:
			fMainView->MessageReceived(msg);
			break;

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

bool
TWindow::QuitRequested(void)
{
	SetPrefs(fMainView->Mode());
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

void
TWindow::GetPrefs(short *mode)
{
	short newMode = msg_easy_mode;
	*mode = newMode;
	BPath path;
	BRect	screenFrame;
	BPoint 	pt;
	
	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		long ref;
		BPoint loc;
				
		path.Append (kPrefsFileName);
		if ((ref = open(path.Path(), O_RDWR)) >= 0) {
			loc.x = -1; loc.y = -1;
		
			lseek (ref, 0, SEEK_SET);
			read(ref, &loc, sizeof(BPoint));
			read(ref, &newMode, sizeof(short));
			close(ref);
			
			if (BScreen(B_MAIN_SCREEN_ID).Frame().Contains(loc)) {
				MoveTo(loc);
				goto DONE;
			}			
		} else {
			// go back up 1 level
			find_directory (B_USER_SETTINGS_DIRECTORY, &path);
			BDirectory dir;
			dir.SetTo(path.Path());
			BFile prefsFile;
			dir.CreateFile(kPrefsFileName, &prefsFile);
		}
	}
	
	// 	if prefs dont yet exist or the window is not onscreen, center the window

	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());

	pt.x = screenFrame.Width()/2 - Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		MoveTo(pt);

DONE:
	*mode = newMode;
}

void
TWindow::SetPrefs(short mode)
{
	BPath path;
	BPoint loc = Frame().LeftTop();

	if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) == B_OK) {
		long ref;
		
		path.Append (kPrefsFileName);
		if ((ref = creat(path.Path(), O_RDWR)) >= 0) {

			lseek (ref, 0, SEEK_SET);
			write (ref, &loc, sizeof(BPoint));
			write (ref, &mode, sizeof(short));
			close(ref);
			
		}
	}
}

// **************************************************************************

#ifdef _BUILD_INTEL
const char* const kEasyLabel = "Select a boot volume:";
#endif

TBootView::TBootView(BRect rect, short PPC_ONLY(mode))
	: BBox(rect, "", B_FOLLOW_ALL, B_WILL_DRAW)
{
#ifndef _BUILD_INTEL
	fMode = mode;
#else
	fMode = msg_easy_mode;
#endif

#ifndef _BUILD_INTEL
	BMenuItem	*mitem;
	BPopUpMenu	*popup = new BPopUpMenu("");
	popup->AddItem(mitem = new BMenuItem("Easy", new BMessage(msg_easy_mode)));
	if (fMode == msg_easy_mode)
		mitem->SetMarked(true);
	popup->AddItem(mitem = new BMenuItem("Expert", new BMessage(msg_expert_mode)));
	if (fMode == msg_expert_mode)
		mitem->SetMarked(true);

	BRect r(Bounds().Width()-StringWidth("Expert")-20-10,9, Bounds().Width()-10, 27);	
	fModeMenu = new BMenuField(r, "", NULL, popup, true);
	AddChild(fModeMenu);
	
#else
	BRect r(10, 12, StringWidth(kEasyLabel), 28);
	fLabel = new BStringView(r, "", kEasyLabel);
	AddChild(fLabel);
#endif

	BuildEasy();
#ifndef _BUILD_INTEL
	BuildExpert();
#endif
}

TBootView::~TBootView()
{
}

void
TBootView::AttachedToWindow()
{
	BBox::AttachedToWindow();
}

void
TBootView::AllAttached()
{
	BBox::AllAttached();
#ifndef _BUILD_INTEL
	switch (fMode) {
		case msg_easy_mode:
			fExpertView->Hide();
			break;
		case msg_expert_mode:
			fEasyView->Hide();
			break;
	}
#endif

	fEasyView->RefreshList();
}

void
TBootView::Draw(BRect where)
{
	BBox::Draw(where);
}

void
TBootView::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case msg_single_click:
			fEasyView->ChooseVolume();
			break;
		case msg_double_click:
			fEasyView->SetBootVolume();
			break;

#ifndef _BUILD_INTEL
		case msg_toggle_mode:
			{
				BMenuItem* mitem;
				if (fMode == msg_easy_mode) {
					mitem = fModeMenu->Menu()->ItemAt(1);
					fMode = msg_expert_mode;
				} else {
					mitem = fModeMenu->Menu()->ItemAt(0);
					fMode = msg_easy_mode;
				}
				mitem->SetMarked(true);
				SwitchMode(fMode);
			}
			break;
		case msg_easy_mode:
		case msg_expert_mode:
			SwitchMode(msg->what);
			break;

		case msg_scsi_bus:
		case msg_scsi_id:
		case msg_scsi_lun:
		case msg_ide_bus:
		case msg_ide_device:
		case msg_scsi_partition:
		case msg_ide_partition:
			fEasyView->NeedToUpdate(true);
			fExpertView->SetBootVolume();
			break;
			
		case msg_scsi:
		case msg_ide:
			fExpertView->MessageReceived(msg);
			break;
#endif	// ppc only

		case B_SIMPLE_DATA:
			HandleDrop(msg);
			break;

		case B_NODE_MONITOR:
			if (fMode == msg_easy_mode) {
				fEasyView->NeedToUpdate(true);
				fEasyView->RefreshList();
			}
			break;

		default:
			BBox::MessageReceived(msg);
			break;
	}
}

void
TBootView::HandleDrop(BMessage *msg)
{
	entry_ref		ref;
	if (msg->FindRef("refs", &ref) != B_NO_ERROR)
		return;
		
	BEntry *entry = new BEntry(&ref);
	struct stat stat;
	entry->GetStat(&stat);
	
	int32 cookie = 0;
	fs_info info;
	while (_kstatfs_(-1, &cookie, -1, NULL, &info) == B_NO_ERROR) {
		if (info.dev == stat.st_dev) {
			char deviceName[B_FILE_NAME_LENGTH];
			int32 partition, session;

			GetDeviceName(info, deviceName, &partition, &session);
			
			switch (fMode) {
				case msg_easy_mode:
					fEasyView->SelectVolume(deviceName);	
					fEasyView->UpdateStatus();
					break;
#ifndef _BUILD_INTEL
				case msg_expert_mode:
					fExpertView->SelectVolume(deviceName, partition, session);
				break;
#endif
			}
		}
	}
}

//	Easy Mode controls

void
TBootView::BuildEasy()
{
#ifndef _BUILD_INTEL
	BRect r(11, fModeMenu->Frame().bottom + 8,
		Bounds().Width()-11, Bounds().Height()-2);
#else	// intel
	BRect r(11, fLabel->Frame().bottom + 8,
		Bounds().Width()-11, Bounds().Height()-2);
#endif

	fEasyView = new TEasyView(r);
	AddChild(fEasyView);
}

void
TBootView::ShowEasy(void)
{
	if (fEasyView->IsHidden())
		fEasyView->Show();
	fEasyView->Invalidate();
}

void
TBootView::HideEasy(void)
{
	if (!fEasyView->IsHidden())
		fEasyView->Hide();
}

//	Expert Mode controls

#ifndef _BUILD_INTEL

void
TBootView::BuildExpert(void)
{
	BRect r(10, fModeMenu->Frame().bottom + 3,
		Bounds().Width()-11, Bounds().Height()-12);
	BootInfo bootInfo;
	
	fExpertView = new TExpertView(r, bootInfo.Partition());
	AddChild(fExpertView);
}

void
TBootView::ShowExpert()
{
	if (fExpertView->IsHidden())	
		fExpertView->Show();
	fExpertView->SyncControls();
}

void
TBootView::HideExpert(void)
{
	if (!fExpertView->IsHidden())	
		fExpertView->Hide();
}

void
TBootView::SwitchMode(short mode)
{
	switch (mode) {
		case msg_easy_mode:
			HideExpert();
			ShowEasy();			// list is refreshed and controls are updated
			fMode = msg_easy_mode;
			break;

		case msg_expert_mode:
			HideEasy();
			ShowExpert();
			fMode = msg_expert_mode;
			break;
	}
}

#endif		// ppc only

short
TBootView::Mode() const
{
	return fMode;
}

bool
TBootView::EasyMode() const
{
	return fMode == msg_easy_mode;
}

// **************************************************************************

const int32 kExclamationMarkWidth = 12;
const int32 kExclamationMarkHeight = 11;

const unsigned char kExclamationMark [] = {
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x00,0xfa,0xfa,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x00,0xfa,0xfa,0x00,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0xfa,0x00,0x00,0xfa,0x00,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0xfa,0x00,0x00,0xfa,0x00,0xff,0xff,0xff,
	0xff,0xff,0x00,0xfa,0xfa,0x00,0x00,0xfa,0xfa,0x00,0xff,0xff,
	0xff,0xff,0x00,0xfa,0xfa,0x00,0x00,0xfa,0xfa,0x00,0xff,0xff,
	0xff,0x00,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0x00,0xff,
	0xff,0x00,0xfa,0xfa,0xfa,0x00,0x00,0xfa,0xfa,0xfa,0x00,0xff,
	0x00,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

TEasyView::TEasyView(BRect rect)
	: BView(rect, "easy", B_FOLLOW_ALL, B_WILL_DRAW)
{
	fNeedUpdate = true;
	SetViewColor(general_info.background_color);
	BRect r(Bounds());

	r.right = Bounds().Width()+1;
	r.left = r.right - kButtonWidth;
	r.bottom = Bounds().Height() - 11;
	r.top = r.bottom - 20;
	fSetBtn = new BButton(r, "set", "Set", new BMessage(msg_double_click));

	r = Bounds();
	r.top += 2;
	r.left += 2;
	r.right -= (B_V_SCROLL_BAR_WIDTH + 2);
	r.bottom = fSetBtn->Frame().top - 8;
	fVolumeList = new TVolumeList(r);
	fVolumeList->SetSelectionMessage(new BMessage(msg_single_click));
	fVolumeList->SetInvocationMessage(new BMessage(msg_double_click));

	BScrollView* sv = new BScrollView("scroll", fVolumeList, B_FOLLOW_ALL, 0,
		false, true);
	AddChild(sv);
	
	AddChild(fSetBtn);
	
	fExclamationMark = new BBitmap(BRect(0, 0, kExclamationMarkWidth-1, kExclamationMarkHeight-1), B_COLOR_8_BIT);
	fExclamationMark->SetBits(kExclamationMark, (kExclamationMarkWidth*kExclamationMarkHeight), 0, B_COLOR_8_BIT);
	fShowExclamationMark = false;
}

TEasyView::~TEasyView()
{
	fVolumeList->EmptyList();
}

void
TEasyView::Draw(BRect ur)
{
	BView::Draw(ur);

	PushState();

	SetHighColor(ViewColor());
	SetLowColor(ViewColor());

	BRect irect(10, Bounds().Height()-27, fSetBtn->Frame().left-5, Bounds().Height()-8);
	FillRect(irect);

	if (fShowExclamationMark) {
		// 	draw the exclamation mark
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fExclamationMark, BPoint(11, Bounds().Height()-23));

		SetHighColor(0,0,0, 255);
		MovePenTo(30, Bounds().Height()-18);
		DrawString("Changes will take effect");
		
		MovePenTo(30, Bounds().Height()-8);
		DrawString("on restart");
	}
	PopState();
}

// 	if the following items exist, then the volume is bootable:
//beos/system/kernel_XXXX (kernel_intel, kernel_joe, kernel_mac)
//beos/system/add-ons/kernel/file_systems/bfs
//beos/system/lib/libroot.so
//beos/system/lib/libnet.so
//beos/bin/sh
static bool
VolumeIsBootable(const char* volumeName)
{	
	//	check for the kernel
	system_info	 sys;
	get_system_info(&sys);
	char platform[32];
	if (sys.platform_type == B_BEBOX_PLATFORM)
		strcpy(platform, "joe");
	else if (sys.platform_type == B_MAC_PLATFORM)
		strcpy(platform, "mac");
	else
		strcpy(platform, "intel");

	char file[B_FILE_NAME_LENGTH];
	sprintf(file, "kernel_%s", platform);
	
	char path[B_PATH_NAME_LENGTH];
	sprintf(path, "/%s/beos/system/", volumeName);
	BDirectory dir;
	dir.SetTo(path);	
	if (!dir.Contains(file)) {
//		printf("failed on kernel\n");
		return false;
	}
		
	// check for the filesystem addons
	sprintf(path, "/%s/beos/system/add-ons/kernel/file_systems/", volumeName);
	dir.SetTo(path);	
	if (!dir.Contains("bfs")) {
//		printf("failed on fs\n");
		return false;
	}

	//beos/system/lib/libroot.so
	sprintf(path, "/%s/beos/system/lib/", volumeName);
	dir.SetTo(path);	
	if (!dir.Contains("libroot.so")) {
//		printf("failed on libroot.so\n");
		return false;
	}

	//beos/system/lib/libnet.so
	sprintf(path, "/%s/beos/system/lib/", volumeName);
	dir.SetTo(path);	
	if (!dir.Contains("libnet.so")) {
//		printf("failed on libnet.so\n");
		return false;
	}

	//beos/bin/sh
	sprintf(path, "/%s/beos/bin/", volumeName);
	dir.SetTo(path);	
	if (!dir.Contains("sh")) {
//		printf("failed on sh\n");
		return false;
	}
	
	return true;
}

void
TEasyView::RefreshList(void)
{
	if (!fNeedUpdate)
		return;
	else
		fNeedUpdate = false;
		
	fVolumeList->EmptyList();
	
#ifndef _BUILD_INTEL
	BootInfo bootInfo;
#endif

	fs_info	info;
	int32 cookie = 0;
	bool found = false;
	while (_kstatfs_(-1, &cookie, -1, NULL, &info) == B_NO_ERROR) {
		if ((strlen(info.device_name)) && (!strcmp(info.fsh_name, "bfs"))) {

			// check to see if the volume is bootable, see above for constraints
			if (!VolumeIsBootable(info.volume_name))
				continue;
			
				
			char deviceName[B_FILE_NAME_LENGTH];
			int32 partition, session;
			GetDeviceName( info, deviceName, &partition, &session);
			
#ifdef _BUILD_INTEL
			BVolume vol;
			if ((vol.SetTo(info.dev) == B_NO_ERROR) && (!vol.IsReadOnly()) &&
				(strcmp(deviceName, "/dev/disk/floppy/raw") != 0)) {
#else
			if (strcmp(deviceName, "/dev/disk/floppy/raw") != 0) {
#endif
				BDirectory		dir;
				BEntry			entry;
				BPath			device;
				node_ref		node;
	
				node.device = info.dev;
				node.node = info.root;
				dir.SetTo(&node);
				dir.GetEntry(&entry);
				entry.GetPath(&device);
				
				fVolumeList->AddItem(new TListItem(info.dev, info.volume_name,
					deviceName, session, partition,
					(strcmp(device.Path(), "/boot") == 0)));

				//	Select the current boot volume
#ifndef _BUILD_INTEL
				if ((bootInfo.Session() == session)
				&& (bootInfo.Partition() == partition)
				&& (!found)) {
					found = bootInfo.DeviceIsBoot(deviceName);
					if (found) {
						fVolumeList->Select(fVolumeList->CountItems() - 1);
						fVolumeList->ScrollToSelection();
					}
				}
#else
				char path[1024];
				sprintf(path, DEFAULT_PATH, info.volume_name);
				if ((!found) && (dir.SetTo(path) == B_NO_ERROR) &&
					(dir.Contains(FLAG_NAME))) {
					found = true;
					fVolumeList->Select(fVolumeList->CountItems() - 1);
					fVolumeList->ScrollToSelection();
				}
#endif
			}			
		}
	}
	
	UpdateStatus();
}

//	called from HandleDrop
void
TEasyView::SelectVolume(const char* deviceName)
{
	int32 index = 0;
	TListItem* item=NULL;
	
	while ((item = fVolumeList->ItemAt(index)) != 0) {
		if (!strcmp(deviceName, item->DeviceName())) {
			fVolumeList->Select(index);
			break;
		}
		index++;
	}
}

void
TEasyView::SelectVolume(int32 bus, int32 id, int32 lun, int32 partition)
{
	int32 count = fVolumeList->CountItems();
	int32 index=0, target=-1;
	TListItem* item=NULL;
	
	while (index < count) {
		item = fVolumeList->ItemAt(index);
		if (item && item->Bus()==bus && item->ID()==id && item->Lun()==lun
		&& item->Partition()==partition) {
			target = index;
			break;
		}
	}
	
	if (target > -1) {
		fVolumeList->Select(target);
		fVolumeList->ScrollToSelection();
	}
}

void
TEasyView::SelectVolume(int32 bus, bool master, int32 partition)
{
	int32 count = fVolumeList->CountItems();
	int32 index=0, target=-1;
	TListItem* item=NULL;
	
	while (index < count) {
		item = fVolumeList->ItemAt(index);
		if (item && item->Bus()==bus && item->IsMaster()==master
		&& item->Partition()==partition) {
			target = index;
			break;
		}
	}
	
	if (target > -1) {
		fVolumeList->Select(target);
		fVolumeList->ScrollToSelection();
	}
}

//	does the actual config write calls

void
TEasyView::SetBootVolume()
{
	int32 currItem = fVolumeList->CurrentSelection();
	
	//	bail if nothing is selected (good proper english)
	if (currItem < 0 || currItem >= fVolumeList->CountItems())
		return;
	
	TListItem *selected = fVolumeList->ItemAt(currItem);
	if (!selected)
		return;
			
	//	the list needs to be updated to reflect the new changes
	//	the next boot volume will need its string changed
	fVolumeList->Invalidate();
	
	//	disable the button so that the user doesn't keep setting the same volume
	fSetBtn->SetEnabled(false);

	if (selected && !selected->Boot())
		fShowExclamationMark = true;
	else
		fShowExclamationMark = false;

#ifndef _BUILD_INTEL

	//	dont set the currently set next boot volume again
	if (selected && !selected->IsNextBoot())
		selected->MakeBootVolume();

#else
	int32 count = fVolumeList->CountItems();
	for (int32 index=0 ; index<count ; index++){
		TListItem* item = fVolumeList->ItemAt(index);

		char path[1024];
		sprintf(path, DEFAULT_PATH, item->VolumeName());
		BDirectory	dir;
		BEntry		entry;
		if (item == selected) {
			BFile		file;

			if (dir.SetTo(path) != B_NO_ERROR) {
				sprintf(path, "/%s/", item->VolumeName());
				dir.SetTo(path);
				dir.CreateDirectory("home", &dir);
				sprintf(path, "/%s/home/", item->VolumeName());
				dir.SetTo(path);
				dir.CreateDirectory("config", &dir);
				sprintf(path, "/%s/home/config/", item->VolumeName());
				dir.SetTo(path);
				dir.CreateDirectory("settings", &dir);
				sprintf(path, DEFAULT_PATH, item->VolumeName());
				dir.SetTo(path);
			}
			dir.CreateFile(FLAG_NAME, &file);
		} else if ((dir.SetTo(path) == B_NO_ERROR) &&
			(dir.FindEntry(FLAG_NAME, &entry) == B_NO_ERROR)) {
			entry.Remove();
		}
	}
#endif
	Draw(Bounds());
}

//	called from a single click message
//	updates the status text and the set btn
void
TEasyView::ChooseVolume()
{
	if (fVolumeList->SelectedVolumeName()) {
		TListItem *selected = fVolumeList->ItemAt(fVolumeList->CurrentSelection());
		bool nb = selected ? selected->IsNextBoot() : false;

		fSetBtn->SetEnabled(!nb);
	} else
		fSetBtn->SetEnabled(false);
}

void
TEasyView::UpdateStatus()
{
	int32 currItem = fVolumeList->CurrentSelection();
	TListItem *selected = fVolumeList->ItemAt(currItem);
	
	if (selected)
		fSetBtn->SetEnabled(!(selected->IsNextBoot()));
	else
		fSetBtn->SetEnabled(false);
}

// **************************************************************************

#ifndef _BUILD_INTEL
TExpertView::TExpertView(BRect frame, int32 partitionID)
	: BView( frame, "expert", B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(general_info.background_color);
	fIDEPartitionID = fSCSIPartitionID = partitionID;
	
	AddSCSIParts();
	AddIDEParts();
}

TExpertView::~TExpertView()
{
}

void
TExpertView::MessageReceived(BMessage* m)
{
	switch(m->what) {
		case msg_scsi:
			fIDEBusMenu->SetEnabled(false);
			fIDEDeviceMenu->SetEnabled(false);
			fIDEPartition->SetEnabled(false);
			
			fSCSIBusMenu->SetEnabled(true);
			fSCSIIDMenu->SetEnabled(true);
			fSCSILUNMenu->SetEnabled(true);
			fSCSIPartition->SetEnabled(true);
			break;

		case msg_ide:
			fIDEBusMenu->SetEnabled(true);
			fIDEDeviceMenu->SetEnabled(true);
			fIDEPartition->SetEnabled(true);
			
			fSCSIBusMenu->SetEnabled(false);
			fSCSIIDMenu->SetEnabled(false);
			fSCSILUNMenu->SetEnabled(false);
			fSCSIPartition->SetEnabled(false);
			break;
	}
}

void
TExpertView::ValidatePartition()
{
	char		text[256];
	const char	*str;
	
	if (fSCSIBtn->Value())
		str = fSCSIPartition->Text();
	else
		str = fIDEPartition->Text();
		
	while (*str) {
		if ((*str < '0') || (*str > '9')) {
			beep();
			if (fSCSIBtn->Value()) {
				sprintf(text, "%d", fSCSIPartitionID);
				fSCSIPartition->SetText(text);
			} else {
				sprintf(text, "%d", fIDEPartitionID);
				fIDEPartition->SetText(text);
			}
			break;
		}
		str++;
	}
	if (!*str) {
		if (fSCSIBtn->Value())
			fSCSIPartitionID = strtol(fSCSIPartition->Text(), NULL, 10);
		else
			fIDEPartitionID = strtol(fIDEPartition->Text(), NULL, 10);
	}
}

void
TExpertView::AddSCSIParts()
{
	char str[256];
	BootInfo bootInfo;

	fSCSIBox = new BBox(BRect(0, 0, Bounds().Width(), 98));
	AddChild(fSCSIBox);

	fSCSIBtn = new BRadioButton(BRect(0,0,StringWidth("SCSI")+20,16), "", "SCSI",
		new BMessage(msg_scsi));
	fSCSIBox->SetLabel(fSCSIBtn);

	fSCSIBtn->SetValue(bootInfo.IsSCSI());

	BMenuItem *mitem;
	
	fSCSIBus = new BPopUpMenu("");
	int32 device = open(B_SCSIPROBE_DRIVER, 0);
	if (device < 0) {
		fSCSIBus->AddItem(mitem = new BMenuItem("Built-In", new BMessage(msg_scsi_bus)));
		mitem->SetMarked(true);
	} else {
		short path = ioctl(device, B_SCSIPROBE_HIGHEST_PATH);
		if ((path < 0) || (path == 255)) {
			fSCSIBus->AddItem(mitem = new BMenuItem("none", new BMessage(msg_scsi_bus)));
			mitem->SetEnabled(false);
			mitem->SetMarked(true);
			fSCSIBtn->SetEnabled(false);
			fSCSIBtn->SetValue(0);
		} else { 
			for (int32 loop = 0; loop <= path; loop++) {
				scsiprobe_extended_path_inquiry	info;
				scsiprobe_path_inquiry			short_info;
				info.path = loop;
				if (ioctl(device, B_SCSIPROBE_EXTENDED_PATH_INQUIRY, &info) == B_NO_ERROR) {
					if (strcmp("unknown", info.data.cam_controller_type))
						sprintf(str, "%d: %s %s", loop, info.data.cam_controller_family,
													 info.data.cam_controller_type);
					else
						sprintf(str, "%d: %s", loop, info.data.cam_controller_family);
				} else {
					ioctl(device, B_SCSIPROBE_PATH_INQUIRY, &short_info);
					sprintf(str, "%d: %s", loop, short_info.data.cam_hba_vid);
				}
				fSCSIBus->AddItem(mitem = new BMenuItem(str, new BMessage(msg_scsi_bus)));
				if (bootInfo.Bus() == loop)
					mitem->SetMarked(true);
			}
		}
		close(device);
	}

	BRect r(88, 18, fSCSIBox->Bounds().right - 25, 36);
	fSCSIBusMenu = new BMenuField(r, "", "Bus:", fSCSIBus, true);
	fSCSIBusMenu->SetAlignment(B_ALIGN_RIGHT);
	fSCSIBusMenu->SetEnabled(bootInfo.IsSCSI());
	fSCSIBusMenu->SetDivider(fSCSIBusMenu->StringWidth("Bus:")+5);
	fSCSIBox->AddChild(fSCSIBusMenu);

	//
	
	float mb_height = fSCSIBusMenu->Bounds().Height();

	fSCSIID = new BPopUpMenu("");
	str[1] = 0;
	for (int32 loop = 0; loop < 7 ; loop++) {
		str[0] = '0' + loop;
		fSCSIID->AddItem(mitem = new BMenuItem(str, new BMessage(msg_scsi_id)));
		if ((bootInfo.Device() == 0) && (loop == 0))
			mitem->SetMarked(true);
		else if ((bootInfo.Device() == 1) && (bootInfo.ID() == loop))
			mitem->SetMarked(true);
	}

	r.top = fSCSIBusMenu->Frame().bottom + 6;
	r.bottom = r.top + 18;
	r.left += 7;
	r.right = r.left + StringWidth("ID: ") + 33;

	fSCSIIDMenu = new BMenuField(r, "", "ID:", fSCSIID, true);
	fSCSIBox->AddChild(fSCSIIDMenu);
	fSCSIIDMenu->SetAlignment(B_ALIGN_RIGHT);
	fSCSIIDMenu->SetEnabled(bootInfo.IsSCSI());
	fSCSIIDMenu->SetDivider(fSCSIIDMenu->StringWidth("ID:")+5);

	//
	
	fSCSILUN = new BPopUpMenu("");
	for (int32 loop = 0; loop < 8 ; loop++) {
		str[0] = '0' + loop;
		fSCSILUN->AddItem(mitem = new BMenuItem(str, new BMessage(msg_scsi_lun)));
		if ((bootInfo.Device() == 0) && (loop == 0))
			mitem->SetMarked(true);
		else if ((bootInfo.Device() == 1) && (bootInfo.Lun() == loop))
			mitem->SetMarked(true);
	}

	r.right = fSCSIBusMenu->Frame().right;
	r.left = r.right - StringWidth("LUN:") - 36;
	
	fSCSILUNMenu = new BMenuField(r, "", "LUN:", fSCSILUN, true);
	fSCSIBox->AddChild(fSCSILUNMenu);
	fSCSILUNMenu->SetAlignment(B_ALIGN_RIGHT);
	fSCSILUNMenu->SetEnabled(bootInfo.IsSCSI());
	fSCSILUNMenu->SetDivider(fSCSILUNMenu->StringWidth("LUN:")+5);
	
	r.left = 55;
	r.OffsetBy(0, mb_height + 8);
	r.left += 10;
	r.right = r.left + StringWidth("Partition: MMM");

	sprintf(str, "%d", fSCSIPartitionID);
	fSCSIPartition = new TArrowTextControl(r, "Partition:", str,
		new BMessage(msg_scsi_partition));
	fSCSIBox->AddChild(fSCSIPartition);
	fSCSIPartition->SetDivider(StringWidth(" Partition:"));
	fSCSIPartition->SetEnabled(bootInfo.IsSCSI());
	fSCSIPartition->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_CENTER);
}

void
TExpertView::AddIDEParts()
{
	char str[256];
	bool have_ide = false;
	BootInfo bootInfo;
	
	BDirectory dir;
	dir.SetTo("/dev/disk/ide");
	if (dir.InitCheck() == B_NO_ERROR)
		have_ide = true;

	float mb_height = fSCSIBusMenu->Bounds().Height();

	BRect r(fSCSIBox->Frame());
	r.top = r.bottom + 8;
	r.bottom = r.top + 15 + 15 + (3 * mb_height) + 5 + 5 + 2 + 5 + 1;	// go figure
	fIDEBox = new BBox(r);
	AddChild(fIDEBox);

	fIDEBtn = new BRadioButton(BRect(0,0,StringWidth("IDE")+20,16), "IDE", "IDE",
		new BMessage(msg_ide));
	fIDEBox->SetLabel(fIDEBtn);

	if (have_ide)
		fIDEBtn->SetValue(!bootInfo.IsSCSI());
	else
		fIDEBtn->SetEnabled(false);

	system_info	 sys;
	get_system_info(&sys);

	BMenuItem *mitem=NULL;
	
	// build a simple menu of ATA/ATAPI
	
	fIDEBus = new BPopUpMenu("");
	if (sys.platform_type == B_BEBOX_PLATFORM) {
		fIDEBus->AddItem(mitem = new BMenuItem("0: Be Incorporated",
										new BMessage(msg_ide_bus)));
		mitem->SetMarked(true);
	} else {
		// scan both ATA/ATAPI looking for highest numbered bus
		short maxBus=1;
		BDirectory dir;
		dir.SetTo("/dev/disk/ide/ata");
		if (dir.InitCheck() == B_NO_ERROR) {
			dir.Rewind();
			BEntry entry;
			int32 currentNum;
			char buffer[32];
			while (dir.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
				entry.GetName(buffer);
				currentNum = atol(buffer);
				maxBus = (currentNum > maxBus) ? currentNum : maxBus;
			}
		}

		dir.SetTo("/dev/disk/ide/atapi");
		if (dir.InitCheck() == B_NO_ERROR) {
			dir.Rewind();
			BEntry entry;
			int32 currentNum;
			char buffer[32];
			while (dir.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
				entry.GetName(buffer);
				currentNum = atol(buffer);
				maxBus = (currentNum > maxBus) ? currentNum : maxBus;
			}
		}		
		
		for (int32 loop = 0; loop <= maxBus; loop++) {
			sprintf(str, "%i", loop);
			fIDEBus->AddItem(mitem = new BMenuItem(str, new BMessage(msg_ide_bus)));
			if (loop == bootInfo.Bus())
				mitem->SetMarked(true);
		}
	}
	
	r.Set(67, 17, 145, 35);
	fIDEBusMenu = new BMenuField(r, "device", "Bus:", fIDEBus, true);
	fIDEBox->AddChild(fIDEBusMenu);
	fIDEBusMenu->SetAlignment(B_ALIGN_RIGHT);
	fIDEBusMenu->SetDivider(StringWidth("Partition: "));
	fIDEBusMenu->SetEnabled(!bootInfo.IsSCSI());

	//
	
	fIDEDevice = new BPopUpMenu("");
	fIDEDevice->AddItem(mitem = new BMenuItem("Master",
		new BMessage(msg_ide_device)));
	mitem->SetMarked(true);
	fIDEDevice->AddItem(mitem = new BMenuItem("Slave",
		new BMessage(msg_ide_device)));
	if ((bootInfo.Device() == 0) && (bootInfo.ID() == 1))
		mitem->SetMarked(true);

	r.OffsetBy(0, mb_height + 8);
	r.right += 30;
	fIDEDeviceMenu = new BMenuField(r, "device", "Device:", fIDEDevice, true);
	fIDEDeviceMenu->SetEnabled(!bootInfo.IsSCSI());
	fIDEDeviceMenu->SetAlignment(B_ALIGN_RIGHT);
	fIDEDeviceMenu->SetDivider(StringWidth("Partition: "));
	fIDEBox->AddChild(fIDEDeviceMenu);

	r.OffsetBy(0, mb_height + 12);
	r.left -= 2;
	r.right = r.left + StringWidth("Partition: MMM ") - 3;
		
	sprintf(str, "%d", fIDEPartitionID);
	fIDEPartition = new TArrowTextControl(r, "Partition:", str,
		new BMessage(msg_ide_partition));
	fIDEBox->AddChild(fIDEPartition);
	fIDEPartition->SetDivider(StringWidth(" Partition:"));
	fIDEPartition->SetEnabled(!bootInfo.IsSCSI());
	fIDEPartition->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_CENTER);
}

void
TExpertView::SetBootVolume()
{
	BootInfo bootInfo(false);

	ValidatePartition();
	
	if (fSCSIBtn->Value()) {
		bootInfo.SetDevice(bootdev_scsi);
		bootInfo.SetBus(fSCSIBus->IndexOf(fSCSIBus->FindMarked()));
		bootInfo.SetID(fSCSIID->IndexOf(fSCSIID->FindMarked()));
		bootInfo.SetLun(fSCSILUN->IndexOf(fSCSILUN->FindMarked()));
		bootInfo.SetPartition(strtol(fSCSIPartition->Text(), NULL, 10));	
	} else {
		char devicename[54];
		uint32 bus = fIDEBus->IndexOf(fIDEBus->FindMarked());
		uint32 id = fIDEDevice->IndexOf(fIDEDevice->FindMarked());
		int fd;

		sprintf(devicename, "/dev/disk/ide/atapi/%u/%s/0/raw", bus, id == 0 ? "master" : "slave");
		fd = open(devicename, O_RDONLY);
		if(fd >= 0) {
			bootInfo.SetDevice(bootdev_atapi);
			close(fd);
		}
		else {
			bootInfo.SetDevice(bootdev_ata);
		}
		bootInfo.SetBus(bus);
		bootInfo.SetID(id);
		bootInfo.SetLun(0);
		bootInfo.SetPartition(strtol(fIDEPartition->Text(), NULL, 10));	
	}

	bootInfo.SetSession(0);
	bootInfo.Write();
}

void
TExpertView::SelectVolume(const char* deviceName, int32 partition, int32 session)
{
	char str[256];
	int bus,id,lun;
	
	write_config_item(CFG_boot_sid, (char *)&session);
	write_config_item(CFG_boot_pid, (char *)&partition);
		
	BMenuItem* menu;
	
	if(strncmp("/dev/disk/scsi", deviceName, 14) == 0) {
		ExtractSCSIInfo (deviceName, &bus, &id, &lun);
		EnableDisableControls(false);

		menu = fSCSIBus->ItemAt(bus);
		if (menu)
			menu->SetMarked(true);

		menu = fSCSIID->ItemAt(id);
		if (menu)
			menu->SetMarked(true);

		menu = fSCSILUN->ItemAt(lun);
		if (menu)
			menu->SetMarked(true);

		fSCSIPartitionID = partition;
		sprintf(str, "%d", fSCSIPartitionID);
		fSCSIPartition->SetText(str);
	} else {
		int device;
		ExtractIDEInfo (deviceName, &device, &bus, &id, &lun);

		EnableDisableControls(true);

		menu = fIDEBus->ItemAt(bus);
		if (menu)
			menu->SetMarked(true);

		if (id == 0)
			menu = fIDEDevice->FindItem("Master");
		else
			menu = fIDEDevice->FindItem("Slave");
		menu->SetMarked(true);

		fIDEPartitionID = partition;
		sprintf(str, "%d", fIDEPartitionID);
		fIDEPartition->SetText(str);
	}
}

bool
TExpertView::Settings(int32 *bus, int32 *id, int32 *lun, int32 *partition)
{
	if (fSCSIBtn->Value()) {
		*bus = fSCSIBus->IndexOf(fSCSIBus->FindMarked());
		*id = fSCSIID->IndexOf(fSCSIID->FindMarked());
		*lun = fSCSILUN->IndexOf(fSCSILUN->FindMarked());
		*partition = strtol(fIDEPartition->Text(), NULL, 10);
		return true;
	} else
		return false;
}

bool
TExpertView::Settings(int32 *bus, bool *master, int32 *partition)
{
	if (fIDEBtn->Value()) {
		*bus = fIDEBus->IndexOf(fIDEBus->FindMarked());
		*master = (fIDEDevice->IndexOf(fIDEDevice->FindMarked()) == 0);
		*partition = strtol(fIDEPartition->Text(), NULL, 10);
		return true;
	} else
		return false;
}

void
TExpertView::SyncControls()
{
	char str[32];
	BMenuItem* menu;	
	BootInfo bootInfo(true);
	
	EnableDisableControls(bootInfo.IsSCSI());
	
	if (bootInfo.IsSCSI()) {
		menu = fSCSIBus->ItemAt(bootInfo.Bus());
		if (menu)
			menu->SetMarked(true);

		menu = fSCSIID->ItemAt(bootInfo.ID());
		if (menu)
			menu->SetMarked(true);

		menu = fSCSILUN->ItemAt(bootInfo.Lun());
		if (menu)
			menu->SetMarked(true);

		fSCSIPartitionID = bootInfo.Partition();
		sprintf(str, "%d", fSCSIPartitionID);
		fSCSIPartition->SetText(str);
	} else {
		menu = fIDEBus->ItemAt(bootInfo.Bus());
		if (menu)
			menu->SetMarked(true);

		if ((bootInfo.Device() == 0) && (bootInfo.ID() == 1))
			menu = fIDEDevice->FindItem("Slave");
		else
			menu = fIDEDevice->FindItem("Master");
		if (menu)
			menu->SetMarked(true);

		fIDEPartitionID = bootInfo.Partition();
		sprintf(str, "%d", fIDEPartitionID);
		fIDEPartition->SetText(str);
	}
}

void
TExpertView::EnableDisableControls(bool scsi)
{
	if (scsi) {
		if (!fSCSIBtn->Value()) {
			fSCSIBtn->SetValue(1);
			fIDEBtn->SetValue(0);
			fIDEBusMenu->SetEnabled(false);
			fIDEDeviceMenu->SetEnabled(false);
			fIDEPartition->SetEnabled(false);
			
			fSCSIBusMenu->SetEnabled(true);
			fSCSIIDMenu->SetEnabled(true);
			fSCSILUNMenu->SetEnabled(true);
			fSCSIPartition->SetEnabled(true);
		}
	} else {
		if (!fIDEBtn->Value()) {
			fIDEBtn->SetValue(1);
			fSCSIBtn->SetValue(0);
			fIDEBusMenu->SetEnabled(true);
			fIDEDeviceMenu->SetEnabled(true);
			fIDEPartition->SetEnabled(true);
			
			fSCSIBusMenu->SetEnabled(false);
			fSCSIIDMenu->SetEnabled(false);
			fSCSILUNMenu->SetEnabled(false);
			fSCSIPartition->SetEnabled(false);
		}
	}
}
#endif		// ppc only

// **************************************************************************

const unsigned char kCheckMark [] = {
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x00,0x7b,0x7b,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x00,0xda,0x7b,0x7b,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x00,0x2b,0x2c,0xda,0x7b,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0x00,0x2b,0x2f,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0x2b,0x2c,0xeb,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0x2b,0x2f,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0x00,0x2b,0x2c,0xeb,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0xff,
	0xff,0xff,0x00,0x2b,0x2f,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x7b,0x7b,0x7b,0x00,
	0xff,0x00,0x2b,0x2c,0xeb,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x7b,0x7b,0x7b,0xda,0x00,
	0xff,0x00,0x2b,0x2f,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x2b,0x2c,0xda,0x7b,0x7b,
	0x00,0x2b,0x2c,0xeb,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x2b,0x2c,0xda,0x7b,
	0x2b,0x2c,0xeb,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x2b,0x2c,0xda,0x7b,
	0x2b,0x2c,0xeb,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x2b,0x2c,0x2b,
	0x2b,0x2f,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x2b,0x2c,0x2b,
	0x2b,0x2f,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x2b,0x2c,
	0xeb,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x2b,0x2c,
	0xeb,0x2f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x2b,
	0x2f,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,
	0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

TVolumeList::TVolumeList(BRect frame)
	: BListView(frame, "list", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_LEFT + B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
{
	BRect r(0,0,31,31);
	fCheckMark = new BBitmap(r, B_COLOR_8_BIT);
	fCheckMark->SetBits(kCheckMark, (32*32), 0, B_COLOR_8_BIT);
}

TVolumeList::~TVolumeList()
{
	delete fCheckMark;
}

TListItem*
TVolumeList::ItemAt(int32 index)
{
	return (TListItem*)BListView::ItemAt(index);
}

void
TVolumeList::EmptyList()
{
	BListItem *item;
	while (CountItems() > 0) {
		item = ItemAt(CountItems()-1);
		if (item) {
			RemoveItem(item);
			delete item;
		}
	}
}

//	find the item that is marked as the current boot volume
//	this could be cached
const char*
TVolumeList::BootVolumeName()
{
	bool found=false;
	int32 index=0;
	int32 count = CountItems();
	TListItem*	item=NULL;
	
	while (index < count) {
		item = ItemAt(index);
		if (item && item->Boot()) {
			found = true;
			break;
		}
		index++;
	}
	
	if (found)
		return item->VolumeName();
	else
		return NULL;
}

const char*
TVolumeList::SelectedVolumeName()
{
	TListItem* item = ItemAt(CurrentSelection());
	if (item)
		return item->VolumeName();
	else
		return NULL;
}

void
TVolumeList::DrawCheckMark(BPoint loc)
{
	DrawBitmap(fCheckMark, loc);
}

BBitmap*
TVolumeList::CheckMark() const
{
	return fCheckMark;
}

// **************************************************************************

TListItem::TListItem(dev_t deviceNumber, char *vol, char *dev,
	int32 session, int32 partition, bool boot)
		  :BListItem()
{
	BRect	r;

	fDeviceNumber = deviceNumber;
	strcpy(fVolume, vol);
	strcpy(fDevice, dev);
	fSession = session;
	fPartition = partition;
	fBoot = boot;

	r.Set(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
	fIcon = new BBitmap(r, B_COLOR_8_BIT);
	if (get_device_icon(fDevice, fIcon->Bits(), B_LARGE_ICON) != B_NO_ERROR) {
		delete fIcon;
		fIcon = NULL;
	}
}

TListItem::~TListItem(void)
{
	delete fIcon;
}

const char* const kCurrentBootStr = "(current boot volume)";

void
TListItem::DrawItem(BView *view, BRect frame, bool)
{
	view->PushState();
	if (IsSelected())
		view->SetHighColor(180, 180, 180, 255);
	else
		view->SetHighColor(255, 255, 255);
	view->SetLowColor(view->ViewColor());
	view->FillRect(frame);
	view->SetDrawingMode(B_OP_OVER);

	if (fIcon)
		view->DrawBitmap(fIcon, BPoint(4 + frame.left, frame.top + 1));

	view->SetHighColor(0, 0, 0);
	
	font_height finfo;		
	view->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent + finfo.leading;

	int32 iconH = (int32) (frame.top + ((frame.Height())/2 + h/2) - 3);
	view->MovePenTo(4 + B_LARGE_ICON + 4, iconH);

	view->DrawString(fTruncated);

	view->MovePenBy(view->StringWidth(" "), 0);
	
	if (fBoot)
		view->DrawString(kCurrentBootStr);
	
	if (IsNextBoot()) {
		TVolumeList* vlist = dynamic_cast<TVolumeList*>(view);
		if (vlist)
			vlist->DrawCheckMark(BPoint(4 + frame.left, frame.top + 1));
	}
		
	view->PopState();
}

void
TListItem::Update(BView *view, const BFont *font)
{
	char	*source[1];
	char	*result[1];

	BListItem::Update(view, font);
	if (Height() < B_LARGE_ICON + 2)
		SetHeight(B_LARGE_ICON + 2);

	source[0] = fVolume;
	result[0] = fTruncated;
	font->GetTruncatedStrings((const char**) source, 1, B_TRUNCATE_END,
		 Width() - (4 + B_LARGE_ICON + 4), result);
}

bool
TListItem::IsSCSI()
{
	return (strncmp("/dev/disk/scsi", DeviceName(), 14) == 0);
}

bool
TListItem::IsMaster()
{
	if (!IsSCSI())
		return (strstr(DeviceName(), "master"));
	else
		return false;
}

int32
TListItem::Device()
{
	if (IsSCSI())
		return bootdev_scsi;
	else
		if(strncmp("/dev/disk/ide/ata/", DeviceName(), 18) == 0)
			return bootdev_ata;
		if(strncmp("/dev/disk/ide/atapi/", DeviceName(), 20) == 0)
			return bootdev_atapi;
		return -1;
}

int32
TListItem::Bus()
{
	int bus, id, lun;
	if (IsSCSI()) {
		ExtractSCSIInfo (DeviceName(), &bus, &id, &lun);
	} else {
		int device;
		ExtractIDEInfo (DeviceName(), &device, &bus, &id, &lun);
	}
	return bus;
}

int32
TListItem::ID()
{
	int bus, id, lun;

	if (IsSCSI()) {
		ExtractSCSIInfo (DeviceName(), &bus, &id, &lun);
	} else {
		int device;
		ExtractIDEInfo (DeviceName(), &device, &bus, &id, &lun);
	}
	return id;
}

int32
TListItem::Lun()
{
	int bus, id, lun;

	if (IsSCSI()) {
		ExtractSCSIInfo (DeviceName(), &bus, &id, &lun);
	} else {
		int device;
		ExtractIDEInfo (DeviceName(), &device, &bus, &id, &lun);
	}
	return lun;
}

bool
TListItem::IsNextBoot()
{
	bool retval=false;

#ifndef _BUILD_INTEL
	BootInfo bootInfo(true);

	retval = (	bootInfo.Device() == Device() && bootInfo.Bus() == Bus()
		&& bootInfo.ID() == ID() && bootInfo.Lun() == Lun()
		&& bootInfo.Session() == Session()
		&& bootInfo.Partition() == Partition());
#else
	BDirectory dir;
	char path[1024];
	sprintf(path, DEFAULT_PATH, fVolume);
	if ((dir.SetTo(path) == B_NO_ERROR) &&
		(dir.Contains(FLAG_NAME))) {
		retval = true;
	}
#endif	

	return retval;
}

void
TListItem::MakeBootVolume()
{
#ifndef _BUILD_INTEL
	BootInfo bootInfo(false);
	
	bootInfo.SetBus(Bus());
	bootInfo.SetID(ID());
	bootInfo.SetDevice(Device());
	bootInfo.SetLun(Lun());
	bootInfo.SetSession(Session());
	bootInfo.SetPartition(Partition());

	bootInfo.Write();
#endif
}

// **************************************************************************

#ifndef _BUILD_INTEL
BootInfo::BootInfo(bool read)
{
	if (read)
		Read();
}

void
BootInfo::Read()
{
	read_config_item(CFG_boot_dev, (char *)&fDevice);
	read_config_item(CFG_boot_bus, (char *)&fBus);
	read_config_item(CFG_boot_id, (char *)&fID);
	read_config_item(CFG_boot_lun, (char *)&fLun);
	read_config_item(CFG_boot_sid, (char *)&fSession);
	read_config_item(CFG_boot_pid, (char *)&fPartition);
}

void
BootInfo::Write()
{
	write_config_item(CFG_boot_dev, (char *)&fDevice);
	write_config_item(CFG_boot_bus, (char *)&fBus);
	write_config_item(CFG_boot_id, (char *)&fID);
	write_config_item(CFG_boot_lun, (char *)&fLun);
	write_config_item(CFG_boot_sid, (char *)&fSession);
	write_config_item(CFG_boot_pid, (char *)&fPartition);
}

void
BootInfo::Set(int32 d, int32 b, int32 i, int32 l, int32 s, int32 p)
{
	fDevice = d;
	fBus = b;
	fID = i;
	fLun = l;
	fSession = s;
	fPartition = p;
}

void
BootInfo::Get(int32 *d, int32 *b, int32 *i, int32 *l, int32 *s, int32 *p)
{
	*d = fDevice;
	*b = fBus;
	*i = fID;
	*l = fLun;
	*s = fSession;
	*p = fPartition;
}

bool
BootInfo::DeviceIsBoot(const char* deviceName)
{
	int dev;
	int bus;
	int id;
	int lun;
	
	if(IsSCSI()) {
		ExtractSCSIInfo(deviceName, &bus, &id, &lun);
		dev = bootdev_scsi;
	}
	else {
		ExtractIDEInfo(deviceName, &dev, &bus, &id, &lun);
	}
	if(dev == Device() && lun == Lun() && id == ID() && bus == Bus())
		return true;
	else
		return false;
}

bool
BootInfo::IsSCSI()
{
	return (Device() == bootdev_scsi);
}
#endif

// **************************************************************************

const int32 kIconWidth = 12;
const int32 kIconHeight = 7;
const color_space kIconColorSpace = B_COLOR_8_BIT;

const unsigned char kUpArrow [] = {
	0x1b,0x1b,0x1b,0x1b,0x1b,0x13,0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x13,0x1e,0x13,0x1b,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x13,0x1e,0x1b,0x18,0x13,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x13,0x1e,0x1b,0x1b,0x1b,0x18,0x13,0x1b,0x1b,0x1b,
	0x1b,0x13,0x1e,0x18,0x18,0x18,0x18,0x18,0x18,0x0b,0x1b,0x1b,
	0x1b,0x13,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x13,0x1b,
	0x1b,0x1b,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x1b
};

const unsigned char kDownArrow [] = {
	0x1b,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x0b,0x1b,0x1b,
	0x1b,0x13,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e,0x16,0x0b,0x13,0x1b,
	0x1b,0x1b,0x13,0x1b,0x1b,0x1b,0x1b,0x16,0x0b,0x13,0x13,0x1b,
	0x1b,0x1b,0x1b,0x13,0x1b,0x1b,0x16,0x0b,0x13,0x13,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x13,0x16,0x0b,0x13,0x13,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x0b,0x13,0x13,0x1b,0x1b,0x1b,0x1b,
	0x1b,0x1b,0x1b,0x1b,0x1b,0x1b,0x13,0x1b,0x1b,0x1b,0x1b,0x1b
};

static color_map*
ColorMap()
{
	color_map* cmap;
	
	BScreen screen(B_MAIN_SCREEN_ID);
	cmap = (color_map*)screen.ColorMap();
	
	return cmap;
}

static uint8
IndexForColor(uint8 r, uint8 g, uint8 b)
{
	BScreen screen(B_MAIN_SCREEN_ID);
	return screen.IndexForColor(r,g,b);
}

static uint8
IndexForColor(rgb_color c)
{
	return IndexForColor(c.red, c.green, c.blue);
}

static rgb_color
ColorForIndex(uint8 i)
{
	color_map *m = ColorMap();
	return m->color_list[i];
}

static uint8
GetHiliteColor(color_map* map, uint8 index)
{
	rgb_color color = map->color_list[index];
	color.red = (long)color.red * 2 / 3;
	color.green = (long)color.green * 2 / 3;
	color.blue = (long)color.blue * 2 / 3;
	
	return IndexForColor(color);
}

static void
HiliteBitmap(BBitmap* src, BBitmap* dest, color_map* cmap)
{
	uchar* srcbits = (uchar *)src->Bits();
	uchar* destbits = (uchar*)dest->Bits();
	
	if (!srcbits || !destbits) {
		PRINT(("drawhiliteicon - problem with bits\n"));
		return;
	}
	
	uint8 index;
	for (int32 i = 0; i < (kIconWidth * kIconHeight); i++) {
		index = srcbits[i];
		destbits[i] = GetHiliteColor(cmap, index);
	}
}

static void
DisableBitmap(BBitmap* src, BBitmap* dest)
{
	uchar* srcbits = (uchar *)src->Bits();
	uchar* destbits = (uchar*)dest->Bits();
	
	if (!srcbits || !destbits) {
		PRINT(("drawhiliteicon - problem with bits\n"));
		return;
	}
	
	uint8 index;
	rgb_color c;
	for (int32 i = 0; i < (kIconWidth * kIconHeight); i++) {
		index = srcbits[i];
		c = ColorForIndex(index);
		c = shift_color(c, 0.5);
		destbits[i] = IndexForColor(c);
	}
}

//
//	an arrow key filter so that we can get around the embedded,
//		private TextView in the BTextControl
//
class TKeyFilter : public BMessageFilter {
public:
					TKeyFilter(	message_delivery delivery,
								message_source source,
								filter_hook func = NULL);
							
	filter_result	Filter(BMessage *message, BHandler **target);
};

TKeyFilter::TKeyFilter(message_delivery delivery, message_source source,
	filter_hook func)
	: BMessageFilter(delivery, source, func)
{
}

filter_result 
TKeyFilter::Filter(BMessage *message, BHandler **target)
{
	if (message->what == B_KEY_DOWN) {
		int8 byte;
		message->FindInt8("byte", &byte);

		TArrowTextControl* realParent=NULL;
		BTextView* tv = dynamic_cast<BTextView*>(*target);
		if (tv) {
			BTextControl* tc = dynamic_cast<BTextControl*>(tv->Parent());
			if (tc) {
				realParent = dynamic_cast<TArrowTextControl*>(tc->Parent());
				if (!realParent->IsEnabled())
					goto DISPATCH;
			}
		}	
		switch(byte) {
			case B_UP_ARROW:
				if (realParent)
					realParent->Increment();
				break;
			case B_DOWN_ARROW:
				if (realParent)
					realParent->Decrement();
				break;
			default:
				goto DISPATCH;
				break;
		}
		return B_SKIP_MESSAGE;
	}

DISPATCH:
		return B_DISPATCH_MESSAGE;
}


const int32 kLowerBounds = 0;
const int32 kUpperBounds = 9;

//
//	a TextControl derivative
//	contains a TextControl and an up and down arrow
//	this is not a fucking spin box, whatever the fuck that is supposed to be
//
TArrowTextControl::TArrowTextControl(BRect frame, const char* label, const char* text, BMessage* m,
	BTextControl* tc)
	: BView(frame, label, B_FOLLOW_NONE, B_WILL_DRAW)
{
	BRect r(Bounds());
	
	if (tc == NULL)
		fTC = new BTextControl(r, label, label, text, m);
	else
		fTC = tc;
	AddChild(fTC);
	
	//	expand by the amount necessary for the arrows
	ResizeBy(16, 0);
	
	r.Set(0,0,11,6);
	fUpArrow = new BBitmap(r, B_COLOR_8_BIT);
	fUpArrow->SetBits(kUpArrow, (kIconWidth*kIconHeight), 0, B_COLOR_8_BIT);
	fUpFrame.Set(Bounds().Width()-14, 3, Bounds().Width()-3, 9);
	fHiliteUpArrow = new BBitmap(r, B_COLOR_8_BIT);
	fDisabledUpArrow = new BBitmap(r, B_COLOR_8_BIT);

	fDownArrow = new BBitmap(r, B_COLOR_8_BIT);
	fDownArrow->SetBits(kDownArrow, (kIconWidth*kIconHeight), 0, B_COLOR_8_BIT);
	fDownFrame.Set(Bounds().Width()-14, 10.0, Bounds().Width()-3, 16);
	fHiliteDownArrow = new BBitmap(r, B_COLOR_8_BIT);
	fDisabledDownArrow = new BBitmap(r, B_COLOR_8_BIT);
	
	fNeedToInvert = false;
	fCurrColorMap = ColorMap();			// this is kind of wasteful, its not used anywhere but here
										// but then again, there are six bitmaps to make this thing work
	HiliteBitmap(fUpArrow, fHiliteUpArrow, fCurrColorMap);
	HiliteBitmap(fDownArrow, fHiliteDownArrow, fCurrColorMap);
	
	DisableBitmap(fUpArrow, fDisabledUpArrow);
	DisableBitmap(fDownArrow, fDisabledDownArrow);
	
	TKeyFilter* filter = new TKeyFilter(B_ANY_DELIVERY, B_ANY_SOURCE);
	BTextView* tv = fTC->TextView();
	tv->AddFilter(filter);
	
	fEnabled = true;
}

TArrowTextControl::~TArrowTextControl()
{
	delete fUpArrow;
	delete fHiliteUpArrow;
	delete fDisabledUpArrow;

	delete fDownArrow;
	delete fHiliteDownArrow;
	delete fDisabledDownArrow;
}

void
TArrowTextControl::AttachedToWindow()
{
	BView::AttachedToWindow();
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}

void 
TArrowTextControl::Draw(BRect)
{
	PushState();
		
	//	draw the bounding box for the arrow controls
	BeginLineArray(6);
	
	rgb_color c = { 192, 192, 192, 255 };	
	if (!fEnabled)
		c = shift_color(c, 0.5);
	AddLine(BPoint(fTC->Frame().right, 0), BPoint(Bounds().Width()-1, 0), c);
	AddLine(BPoint(Bounds().Width()-1, 0), BPoint(Bounds().Width()-1, Bounds().Height()-1), c);
	AddLine(BPoint(Bounds().Width()-1, Bounds().Height()-1), BPoint(fTC->Frame().right, Bounds().Height()-1), c);

	c.red = 255; c.green = 255; c.blue = 255;
	if (!fEnabled)
		c = shift_color(c, 0.5);
	AddLine(BPoint(fTC->Frame().right, 1), BPoint(Bounds().Width(), 1), c);
	AddLine(BPoint(Bounds().Width(), 1), BPoint(Bounds().Width(), Bounds().Height()), c);
	AddLine(BPoint(Bounds().Width(), Bounds().Height()), BPoint(fTC->Frame().right, Bounds().Height()), c);

	EndLineArray();
	
	//	draw the arrow bitmaps
	SetDrawingMode(B_OP_OVER);
	
	if (!fEnabled)
		DrawBitmap(fDisabledUpArrow, fUpFrame);	
	else if (fNeedToInvert && Value() && Target() == 1)
		DrawBitmap(fHiliteUpArrow, fUpFrame);
	else
		DrawBitmap(fUpArrow, fUpFrame);
	
	if (!fEnabled) 		
		DrawBitmap(fDisabledDownArrow, fDownFrame);
	else if (fNeedToInvert && Value() && Target() == 2)
		DrawBitmap(fHiliteDownArrow, fDownFrame);
	else
		DrawBitmap(fDownArrow, fDownFrame);
	

	PopState();
}

void 
TArrowTextControl::MessageReceived(BMessage* m)
{
	switch(m->what) {
		default:
			BView::MessageReceived(m);
			break;
	}
}

void
TArrowTextControl::KeyDown(const char *key, int32 numBytes)
{
	if (!fEnabled)
		return;

	switch (key[0]) {
		default:
			BView::KeyDown(key,numBytes);
			break;
	}
}

void
TArrowTextControl::MouseDown(BPoint where)
{
	if (!fEnabled)
		return;
		
	BRect frame;
	
	if (fUpFrame.Contains(where)) {
		fNeedToInvert = true;
		frame = fUpFrame;
		SetTarget(1);
		SetValue(true);
	} else if (fDownFrame.Contains(where)) {
		fNeedToInvert = true;
		frame = fDownFrame;
		SetTarget(2);
		SetValue(true);
	} else {
		BView::MouseDown(where);
		SetTarget(0);
		SetValue(false);
		return;
	}
	
	BPoint loc;
	uint32 buttons;
	do {
		if (frame.Contains(loc))
			SetValue(true);

		snooze(175000);
		GetMouse(&loc, &buttons);
	} while(buttons);
	
	fNeedToInvert = false;
	SetTarget(0);
	SetValue(false);
}

void 
TArrowTextControl::SetEnabled(bool e)
{
	if (fEnabled == e)
		return;
		
	fEnabled = e;
	fTC->SetEnabled(e);
	
	Invalidate();
}

bool
TArrowTextControl::IsEnabled() const
{
	return fEnabled;
}

short
TArrowTextControl::Target() const
{
	return fTarget;
}

void
TArrowTextControl::SetTarget(short t)
{
	if (t == fTarget)
		return;
		
	fTarget = t;
}

bool
TArrowTextControl::Value() const
{
	return fValue;
}

void
TArrowTextControl::SetValue(bool v)
{
	fValue = v;
	
	if (fValue) {
		if (fTarget == 1)
			Increment();
		else if (fTarget == 2)
			Decrement();
	}
		
	Draw(Bounds());
}

void
TArrowTextControl::Increment()
{
	char value[16];
	int32 num;
	
	strcpy(value, Text());
	num = atol(value);
	
	num++;
	if (num > kUpperBounds)
		return;
	sprintf(value, "%li", num);
	
	SetText(value);
	fTC->Invoke();
}

void
TArrowTextControl::Decrement()
{
	char value[16];
	int32 num;
	
	strcpy(value, Text());
	num = atol(value);
	
	num--;
	if (num < kLowerBounds)
		return;
	sprintf(value, "%li", num);
	
	SetText(value);
	fTC->Invoke();
}

const char* 
TArrowTextControl::Text() const
{
	return fTC->Text();
}

void 
TArrowTextControl::SetText(const char* t)
{
	fTC->SetText(t);
}

void 
TArrowTextControl::SetDivider(float d)
{
	fTC->SetDivider(d);
}

void 
TArrowTextControl::SetAlignment(alignment label, alignment text)
{
	fTC->SetAlignment(label, text);
}
