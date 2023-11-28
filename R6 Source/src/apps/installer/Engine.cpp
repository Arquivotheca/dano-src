/* ++++++++++

   FILE:  Engine.cpp
   REVS:  $Revision: 1.73 $
   NAME:  steve
   DATE:  Tue Feb 06 18:31:52 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <Alert.h>
#include <Beep.h>
#include <Button.h>
#include <Debug.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <OS.h>
#include <Path.h>
#include <Roster.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <String.h>
#include <Directory.h>
#include <Path.h>

#include <fs_attr.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <fs_info.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <scsi.h>

#include "Engine.h"
#include "IView.h"
#include "LegalMessage.h"

#ifndef _STANDALONE_INSTALLER_BUILD_
#include <md4checksum.h>
#include <nvram.h>
#include "FSUtils.h"
#else
#include "InternalGlue.h"
#endif

#if __INTEL__
 #define INTEL_ONLY(x) (x)
#else
 #define INTEL_ONLY(x) 
#endif

const uint32 COPY_CANCELED = B_ERRORS_END + 1;
	// matches a cancelation error code from the tracker

const char* kInitializeString = "The disk \"%s\" you have selected needs to be initialized "
	"prior to installation. Initializing the disk will erase all the "//Macintosh or DOS "
	"files on the disk.\n"
	"Do you want to initialize this disk?";

const char* kInitializeOrJustMountString = "Would you like to initialize disk \"%s\" "
	"prior to installation?\n"
	"Initializing erases all the data on this disk.";

const char *kInitialMessage = "Choose the disk you want to install onto"
				 " from the pop-up menu. Then click \"Begin\".";

const char *kErrInstallOnSelf = "You can't install the contents of a disk onto itself. "
	"Please choose a different disk.";

const char *kErrReadOnly = "You can't install onto the selected disk because "
	"it is read-only. Please choose a different disk.";

const char *kProbablyNotEnoughSpace = "The destination disk may not have enough space. "
	"Try choosing a different disk%s.";

const char *kDontInstallOptionalItems = " or choose to not install optional items.";

const char *kDoneInstalling = "Installation was successful. "
	"Your machine will reboot after you close Installer.";

const char *kCheckOutThirdPartyStuff = "To try some of the third party applications "
	"on the BeOS CD, reinsert the CD after rebooting and explore the Third Party "
	"Software folder.";

const char *kErrNoDisksCanDriveSetup = "There are no disks to install onto. Would you "
	"like to launch DriveSetup to format a disk?";

const char *kErrNoDisks = "There are no disks  to install onto. You need to install "
	"a new disk and/or use a formatting utility to create a suitable partition for "
	"installing the BeOS.";

const char *kErrSingleMandatorySystemDisk = "There is only a single disk available "
	"for installing the BeOS. Installing onto this disk would erase your existing system "
	"software. You need to install a new disk and/or use a formatting utility to create "
	"a suitable partition for installing the BeOS.";

const char *kFirstPartitionWarning = "You have chosen to install onto the first partition "
	"on a primary disk. Installing onto this disk will erase your existing "
	"Windows, DOS or other system software.\n"
	"IT IS MOST LIKELY THAT YOU DO NOT WANT TO DO THIS.\nAre you sure you would like "
	"to proceed with the installation?";

const char *kInstallingOverBoot = "Are you sure you want to install onto the "
	"current boot disk? The installer will have to reboot your machine if "
	"you proceed.";

const char *kMustRebootNow = "Installation was successful. The installer will now "
	"reboot.";

//const off_t kInstallMinFreeSpace = (80 * 1048576);
//const off_t kOptionalInstallMinFreeSpace = (300 * 1048576);

const char *kNoMountedVolumes = "no mounted volumes";

// These are attributes that should not be overwritten in the mime database
// when copying
const char *kPreserveAttributes[] = {
	"META:ATTR_INFO",		// Extra attributes
	NULL
};

const char *kInstallerSkipAttributes[] = {
	kChecksumAttrName,		// Don't copy the checksum.
	NULL
};

// from roster_private.h, copied here so we don't have to rely on the private header file
const uint32 CMD_REBOOT_SYSTEM = 302;
#define ROSTER_SIG			"application/x-vnd.Be-ROST"

//-----------------------------------------------------

TEngine::TEngine()
	:	BLooper("InstallerCopyEngine"),
		cleanInstall(false),
//		includeOptionalItems(false),
//		includeJapaneseItems(false),
		showingDone(false),
		mergeInstall(false),
		freshInstall(false),
		defaultSourceVolume(-1),
		defaultDestinationVolume(-1)
//		m_options_running(false)
{
	SetState(IDLE);
}

TEngine::~TEngine()
{
}

bool
TEngine::QuitRequested()
{
	SetState(QUITTING);
	return inherited::QuitRequested();
}

int32
TEngine::State()
{
	int32 state;

	fStateLocker.Lock();
	state = fState;
	fStateLocker.Unlock();

	return(state);
}


void
TEngine::SetState(int32 state)
{
	fStateLocker.Lock();
	fState = state;
	fStateLocker.Unlock();
}

void
TEngine::StartMeUp()
{
	SetState(LOADING);
	fWindow = new TIWindow(this);
	(new LegalMessage())->Show();
	RescanCommon(true);
}

void TEngine::ReadInstallSizes(BVolume *vol, off_t *file_size, off_t *attr_size, off_t *num)
{
	BDirectory etc_dir;
	BFile datfile;
	BPath path;

	if (vol != NULL
		&& vol->GetRootDirectory(&etc_dir) == B_NO_ERROR
		&& path.SetTo(&etc_dir, "beos/etc/install") == B_NO_ERROR
		&& etc_dir.SetTo(path.Path()) == B_NO_ERROR
		&& datfile.SetTo(&etc_dir, "install_size", B_READ_ONLY) == B_NO_ERROR)
	{
		// Read the numbers
		datfile.Read(file_size, sizeof (off_t));
		datfile.Read(attr_size, sizeof (off_t));
		datfile.Read(num, sizeof (off_t));
		
		// Convert them to our format
		*file_size = B_LENDIAN_TO_HOST_INT64(*file_size);
		*attr_size = B_LENDIAN_TO_HOST_INT64(*attr_size);
		*num = B_LENDIAN_TO_HOST_INT64(*num);
	}
}

void
TEngine::Rescan()
{
	RescanCommon(false);
}

void
TEngine::RescanCommon(bool)
{
	char msg[300];
	fWindow->SetMessage("Scanning for disks...");
	DisableAllItems(false);
	deviceList.RescanDevices(true);
	deviceList.UpdateMountingInfo();
	
#ifdef _STANDALONE_INSTALLER_BUILD_
	// add the fs in file volumes - need to do this after the previous
	// two calls because the following is really a hack and the
	// calls would mess up the partition structure.
	//
	// This should be done properly in the DeviceMap list -- it's hacked this
	// way because we didn't wan't to touch libbe.so right before a release
	AddVirtualFileSystems(&deviceList);
#endif

	AdjustVolumeMenus();

	sprintf(msg, kInitialMessage);
	fWindow->SetMessage(msg);

	SetState(IDLE);
}

// function to change the destination popupmenu visible text to the
// short name of the selected volume
void
SetVolumePopupTitle(BMenu *menu)
{
	BMenu *bar = menu->Supermenu();
	if (bar && bar->ItemAt(0)) {
		BMenuItem *item = menu->FindMarked();
		BMessage *msg = NULL;
		if (item) {
			msg = item->Message();
			if (msg) {
				const char *name;
				status_t code = msg->FindString("short_name", &name);
				if (code == B_OK) {
					bar->ItemAt(0)->SetLabel(name);
				}
			}
		}
	}
}

void 
TEngine::MessageReceived(BMessage *message)
{
	TIView *view;
	char *str;
	
	switch (message->what) {

	case INIT_ENGINE:
		StartMeUp();
		break;

	case COMMAND_SELECTED:
		AdjustVolumeMenus();
		break;

	case LAUNCH_DRIVE_SETUP:
		if (State() == IDLE) {
			SetState(LAUNCH_DRIVE_SETUP);
			fWindow->SetMessage("Running DriveSetup... \n"
				"Close DriveSetup to continue with the installation.");
			DisableAllItems(false);
			LaunchDriveSetup();
			AdjustVolumeMenus();
			EnableAllItems(false);
		}
		break;

	case DESTINATION_VOLUME_SELECTED:
		if (fWindow->Lock()) {
			view = (TIView*)fWindow->FindView("IView");
			ASSERT(view);
			SetVolumePopupTitle(view->VolumeMenu2());
			fWindow->SetMessage(view->BuildDefaultText());
			EnableAllItems(false);
			fWindow->Unlock();
		}
		break;

	case SOURCE_VOLUME_SELECTED:
	{
		fWindow->Lock();
		view = (TIView*)fWindow->FindView("IView");
		ASSERT(view);
		UpdateSourceVolumeText(view->VolumeMenu1(), view->FromText());
		ScanSourceVolumeForOptions();
		fWindow->SetMessage(view->BuildDefaultText());
		EnableAllItems(false);
		fWindow->Unlock();
		break;
	}

	case 'BUTN':
		HandleButton();
		break;

	case OPTION_CHECKED:
		if (fWindow->Lock()) {
			view = (TIView*)fWindow->FindView("IView");
			ASSERT(view);
			view->UpdateSpaceLabel();
			fWindow->Unlock();
		}
		break;
	case OPTION_MOUSED_OVER:
		if (!showingDone) { // only update the message when we're not finished
			if (B_OK == message->FindPointer("message", (void **)&str)) {
				fWindow->SetMessage(str);		
			}
			else {
				fWindow->SetMessage(NULL);
			}
		}
		break;
	
	default:
		inherited::MessageReceived(message);
		break;
	}
}

void 
TEngine::EnableDisableCommon(bool beginButton, bool rest)
{
	fWindow->Lock();

	BButton *button = dynamic_cast<BButton *>(fWindow->FindView("Button"));
	ASSERT(button);
	if (button)
		button->SetEnabled(beginButton);

	button = dynamic_cast<BButton *>(fWindow->FindView("driveSetup"));
	ASSERT(button);
	if (button)
		button->SetEnabled(rest);

	OptionView *options = dynamic_cast<OptionView *>(fWindow->FindView("optionView"));
	ASSERT(options);
	if (options)
		options->SetEnabled(rest);
		
	BMenuField *field = dynamic_cast<BMenuField *>(fWindow->FindView("destination"));
	ASSERT(field);
	if (field)
		field->SetEnabled(rest);

	field = dynamic_cast<BMenuField *>(fWindow->FindView("source"));
	ASSERT(field);
	if (field)
		field->SetEnabled(rest);

	fWindow->Unlock();
}

void
TEngine::EnableAllItems(bool showDone)
{
	fWindow->Lock();
	EnableDisableCommon(true, true);
	showingDone = showDone;

	BButton *button = dynamic_cast<BButton *>(fWindow->FindView("Button"));
	ASSERT(button);
	if (button)
		button->SetLabel(showDone ? "Quit" : "Begin");

	fWindow->Unlock();
}

void
TEngine::DisableAllItems(bool keepStopEnabled)
{
	EnableDisableCommon(keepStopEnabled, false);
}

void 
TEngine::AdjustVolumeMenus()
{
	TIView *view = (TIView*)fWindow->FindView("IView");
	if (!view)
		return;

	BPopUpMenu *pop1 = view->VolumeMenu1();
	BPopUpMenu *pop2 = view->VolumeMenu2();

	BMenuItem *srcItem = pop1->FindMarked();
	int32 srcPartitionID(-1);

	if ((srcItem != NULL) && (srcItem->Message() != NULL)) {
		status_t code = srcItem->Message()->FindInt32("volume_id", &srcPartitionID);
		if (code != B_OK)
			srcPartitionID = -1;
	}


	BStringView *fromtext = view->FromText();
	BuildSourceMenu(pop1);
	BuildDestinationMenu(pop2);
	
	fWindow->Lock();

	SelectSourceDest(pop1, pop2, fromtext,
		defaultSourceVolume, defaultDestinationVolume);
	// show "Install from:" as text only if there are less than 2 source volumes
	if (pop1->CountItems() < 2) {
		view->ShowSourceVolumeAsText(true);
	} else {
		view->ShowSourceVolumeAsText(false);
	}

	srcItem = pop1->FindMarked();
	int32 newSrcID(-1);
	if ((srcPartitionID != -1) && (srcItem != NULL)
		&& (srcItem->Message() != NULL))
	{
		status_t code = srcItem->Message()->FindInt32("volume_id", &newSrcID);
		if (code != B_OK)
			newSrcID = -1;
	}
	
	// only reset the options if the selected source partition changes
	if ((srcPartitionID == -1) || (srcPartitionID != newSrcID))
		ScanSourceVolumeForOptions();

	fWindow->Unlock();
	EnableAllItems(false);
}


BAlert *
TEngine::CenterAlert(BAlert *alert)
{
	fWindow->Lock();
	alert->MoveTo(BPoint(fWindow->Frame().left, fWindow->Frame().top - 20));
	fWindow->Unlock();
	return alert;
}

bool
TEngine::MountSelectedDestinationIfNeeded(Partition *partition)
{
	char buffer[512];

	if (partition->Mounted() == kMounted) 
		return true;

	bool initialize = false;

	if (strcmp(partition->FileSystemShortName(), "bfs") == 0) {
		// a bfs disk, prompt to optionally initalize
		sprintf(buffer, kInitializeOrJustMountString, partition->VolumeName());	
		switch (CenterAlert(new BAlert("", buffer,
			"Initialize", "Install as is", "Stop installation",
			B_WIDTH_FROM_LABEL, B_WARNING_ALERT))->Go()) {
			case 2:
				return false;
			case 1:
				break;
			case 0:
				initialize = true;
		}
	} else {
		// not a bfs disk, either initialize or go away
		sprintf(buffer, kInitializeString, partition->VolumeName());	
		if (CenterAlert(
			new BAlert("", buffer, "Initialize", "Stop installation",
				NULL, B_WIDTH_FROM_LABEL, B_STOP_ALERT))->Go() == 1)
		{
			return false;
		}
		initialize = true;
	}

	if (initialize) {
		InitializeControlBlock *params = fWindow->InitializeParams();
		params->window = fWindow;
		params->completionMessage = DONE_INITIALIZING;

		status_t result = partition->Initialize(params);
		
		if (result != B_NO_ERROR) {
			sprintf(buffer, "There was an error (%s) while initializing volume %s.",
				strerror(result), partition->VolumeName());
			CenterAlert(new BAlert("", buffer, "Sorry"))->Go();
			return false;
		}

		if (params->cancelOrFail)
			return false;
			// initialization cancelled or unsuccessful
	}

	status_t result = partition->Mount();
	if (result == B_NO_ERROR)
		defaultDestinationVolume = partition->VolumeDeviceID();
	else {
		sprintf(buffer, "There was an error (%s) while mounting volume %s.",
			strerror(result), partition->VolumeName());
		CenterAlert(new BAlert("", buffer, "Sorry"))->Go();
		return false;
	}

	return true;
}


void
TEngine::UnmountedSourcePartitionSelected(BMessage *message)
{
	char buffer[512];

	int32 partitionId = message->FindInt32("partition_id");
	Partition *partition = deviceList.PartitionWithID(partitionId);
	ASSERT(partition);
	ASSERT(partition->Mounted() != kMounted);

	ASSERT(message->HasInt32("partition_id"));
	if (!message->HasInt32("partition_id"))
		return;

	/*
	// we don't even show non bfs volumes in the source menu,
	// don't have to do this here
	if (strcmp(partition->FileSystemShortName(), "bfs") != 0) {
		// not a bfs disk, prompt to initalize
		sprintf(buffer, "The disk %s you have selected is a "
				"non-Be OS disk. Are you sure you want to install "
				"from it?", partition->VolumeName());	
		switch (CenterAlert(new BAlert("", buffer,
			"Yes", "No", NULL,
			B_WIDTH_FROM_LABEL, B_WARNING_ALERT))->Go()) {
			case 0:
				break;
			case 1:
				return;
		}
	}
	*/

	status_t result = partition->Mount();
	if (result == B_NO_ERROR)
		defaultSourceVolume = partition->VolumeDeviceID();
	else {
		sprintf(buffer, "There was an error (%s) while mounting volume %s.",
			strerror(result), partition->VolumeName());
		CenterAlert(new BAlert("", buffer, "OK"))->Go();
	}
}


inline status_t
Execute(const char *argLine)
{
	status_t result = system(argLine);
	return result;
}


extern "C" int _kstatfs_(dev_t dev, int32 *pos, int fd, const char *path,
					struct fs_info *fs);

// volume matching rules

static bool
CheckDestinationPrimaryPartition(Partition *INTEL_ONLY(partition))
{
#if !__INTEL__
	return false;
#else
	// check if selected partition is the primary bus zeroth partition -
	// an unlikely target on windows machines
	if (!partition)
		return false;
	
	if (partition->Index() > 0)
		return false;

	if (strcmp(partition->FileSystemShortName(), "bfs") == 0)
		// we already have BeOS on there, it must be ok to reinstall
		return false;

	int fd = open(partition->GetDevice()->Name(), O_RDONLY), err;
	uint8 drive_id = 0;

	err = ioctl(fd, B_GET_BIOS_DRIVE_ID, &drive_id);
	close(fd);

	if (err < B_OK) {
		if (strcmp(partition->FileSystemShortName(), "dos") == 0)	
			return true;   /* this is safest */
	} else if (drive_id == 0x80) {
		return true;
	}

	return false;
#endif
}

struct MatchVolumeParams {
	dev_t currentDevice;
	DeviceList *deviceList;
	// add any supporting data for matching here
};
 
static bool
VolumeMatches(dev_t device, int32 , MatchVolumeParams *params)
{
	return device == params->currentDevice;
}

static bool
VolumeDiffers(dev_t device, int32, MatchVolumeParams *params)
{
	return device != params->currentDevice;
}

static bool
VolumeNonBootAndDiffers(dev_t device, int32 partitionID, MatchVolumeParams *params)
{
	Partition *partition = params->deviceList->PartitionWithID(partitionID);
	if (!partition)
		return false;

	if (CheckDestinationPrimaryPartition(partition))
		return false;

	return device != params->currentDevice
		&& strcmp(partition->MountedAt(), "/boot") != 0;
}

static bool
VolumeBeOSNonBootAndDiffers(dev_t device, int32 partitionID, MatchVolumeParams *params)
{
	Partition *partition = params->deviceList->PartitionWithID(partitionID);
	if (!partition)
		return false;

	if (CheckDestinationPrimaryPartition(partition))
		return false;

	return device != params->currentDevice
		&& strcmp(partition->MountedAt(), "/boot") != 0
		&& strcmp(partition->FileSystemShortName(), "bfs") == 0;
}

static bool
MatchAnyVolume(dev_t, int32, MatchVolumeParams *)
{
	return true;
}

inline bool
MatchPersistentVolume(BVolume &volume)
{
	return volume.IsPersistent();
}

inline bool
MatchWriteablePersistentVolume(BVolume &volume)
{
	return !volume.IsReadOnly() && volume.IsPersistent();
}

static bool
MatchWriteablePersistentVolumeWithSpace(BVolume& volume, off_t requestedSpace)
{
	return MatchWriteablePersistentVolume(volume)
		&& volume.FreeBytes() >= requestedSpace;
}

static bool
UseableAsSourcePartition(Partition *partition)
{
	if (strcmp(partition->FileSystemShortName(), "bfs") != 0)
		return false;
	
	return true;
}

static bool
UseableAsDestinationPartition(Partition *partition)
{
	ASSERT(!partition->Hidden());
		// should be weeded out by EachMounted and EachMountable
									
	if(partition->GetDevice()->ReadOnly()) 
		// can't install onto read only disks
		return false;

// In order to have backwards compatibility, we must be able to install
// on raw initialized disks. However, we must (in DriveSetup) remove the
// ability to create them in the future.

#if __INTEL__
	if (partition->GetSession()->VirtualPartitionOnly()
		&& strcmp(partition->FileSystemShortName(), "bfs") != 0)
		// cannot install onto unpartitioned disks on Intel
		return false;
#endif

	return true;
}

struct EachPartitionParams {
	BPopUpMenu *popup;
	BHandler *target;
	bool notUnique;
	bool detailedDescriptions;
};

struct UniqueNameCheckParams {
	DeviceList *deviceList;
};

static Partition *
CompareOneName(Partition *partition, void *castToPartition)
{
	Partition *checkAgainst = (Partition *)castToPartition;
	if (partition != checkAgainst
		&& partition->VolumeName()[0] != '\0'
		&& UseableAsDestinationPartition(partition)
		&& strcmp(partition->VolumeName(), checkAgainst->VolumeName()) == 0)
		return partition;
	
	return NULL;
}

static Partition *
CheckOneNameUnique(Partition *partition, void *castToParams)
{
	UniqueNameCheckParams *params = (UniqueNameCheckParams *)castToParams;
	
	if (partition->VolumeName()[0] != '\0'
		&& UseableAsDestinationPartition(partition)
		&& (params->deviceList->EachMountedPartition(CompareOneName, partition)
			|| params->deviceList->EachMountablePartition(CompareOneName, partition)))
		return partition; // not unique

	return NULL;
}

static Partition *
AddOneAsDestinationItem(Partition *partition, void *castToParams)
{
	if (!UseableAsDestinationPartition(partition))
		return NULL;

	// in addition to the previous check, do not include mounted
	// non Be disks
	if (partition->Mounted() == kMounted
		&& strcmp(partition->FileSystemShortName(), "bfs") != 0)
		return NULL;
	
	EachPartitionParams *params = (EachPartitionParams *)castToParams;
	char longName[256], shortName[256], sizestr[32];
	BMessage *message;

	uint64 bytes = partition->Blocks() * partition->LogicalBlockSize();

	if (bytes >= 1024LL * 1024 * 1024 * 1024)
		sprintf(sizestr, "%.1f TB", bytes / (1024.*1024.*1024.*1024.));
	else if (bytes >= 1024LL * 1024 * 1024)
		sprintf(sizestr, "%.1f GB", bytes / (1024.*1024.*1024.));
	else if (bytes >= 1024LL * 1024)
		sprintf(sizestr, "%.1f MB", bytes / (1024.*1024.));
	else
		sprintf(sizestr, "%.1f KB", bytes / (1024.));

	// if not Be disk, show file system
	char fsBuffer[128];
	fsBuffer[0] = '\0';
	sprintf(fsBuffer, "%s", partition->FileSystemLongName());
	
	const char *volumeName = partition->VolumeName();
	if (strcmp(partition->FileSystemShortName(), "dos") == 0
		&& strncmp(volumeName, "     ", 5) == 0)
	{
		// dos disk sometimes have spaces instead of a null string for
		// an empty name
		volumeName = "";
	}
	
	// include bus and partition in detailed description
	sprintf(longName, "%s - %s [%s] [%s partition:%li]", 
		volumeName,
		sizestr,
		fsBuffer,
		partition->GetDevice()->DisplayName(),
		partition->Index() + 1);
	// include name, size, and filesystem type in short description
	sprintf(shortName, "%s - %s [%s]", volumeName, sizestr, fsBuffer);

	char *visibleName = shortName;
	if (params->detailedDescriptions || params->notUnique
		|| volumeName[0] == '\0')
	{
		visibleName = longName;
	}
	
	message = new BMessage(DESTINATION_VOLUME_SELECTED);
	message->AddInt32("volume_id", partition->VolumeDeviceID());	
	message->AddInt32("partition_id", partition->UniqueID());
	message->AddString("volume_name", volumeName);
	message->AddString("long_name", longName);
	message->AddString("short_name", shortName);	
	params->popup->AddItem(new BMenuItem(visibleName, message));

	return NULL;
}

void
TEngine::BuildDestinationMenu(BPopUpMenu *menu)
{
	EachPartitionParams params;
	ClearItems(menu);

	UniqueNameCheckParams uniqueParams;
	uniqueParams.deviceList = &deviceList;	

	params.notUnique = 
		deviceList.EachMountedPartition(CheckOneNameUnique, &uniqueParams) != NULL
		|| deviceList.EachMountablePartition(CheckOneNameUnique, &uniqueParams) != NULL;

	params.popup = menu;
	params.target = this;
#if __INTEL__
	params.detailedDescriptions = true;
#else
	params.detailedDescriptions = (modifiers() & B_OPTION_KEY) != 0;
#endif
	deviceList.EachMountedPartition(AddOneAsDestinationItem, &params);
	deviceList.EachMountablePartition(AddOneAsDestinationItem, &params);
	deviceList.EachInitializablePartition(AddOneAsDestinationItem, &params);

	if (menu->CountItems() == 0)
		menu->AddItem(new BMenuItem("no volumes", NULL));

#if __INTEL__
// #ifndef DRIVE_SETUP_IN_OPTIONS
//	menu->AddSeparatorItem();
//	menu->AddItem(new BMenuItem("Setup partitions"B_UTF8_ELLIPSIS,
//		new BMessage(LAUNCH_DRIVE_SETUP)));
// #endif
#endif
}

static Partition *
AddOneMountedAsSourceItem(Partition *partition, void *castToParams)
{
	if (!UseableAsSourcePartition(partition))
		return NULL;

	EachPartitionParams *params = (EachPartitionParams *)castToParams;
	char buffer[256];
	BMessage *message;
	sprintf(buffer, "%s", partition->VolumeName());
	message = new BMessage(SOURCE_VOLUME_SELECTED);
	message->AddInt32("volume_id", partition->VolumeDeviceID());

	BMenuItem *item = new BMenuItem(buffer, message);
	item->SetTarget(params->target);
	params->popup->AddItem(item);

	return NULL;
}

void
TEngine::BuildSourceMenu(BPopUpMenu *menu)
{
	ClearItems(menu);
	EachPartitionParams params;
	params.popup = menu;
	params.target = this;
	deviceList.EachMountedPartition(AddOneMountedAsSourceItem, &params);
	if (menu->CountItems() == 0) {
		ASSERT(!"should not be here, should have at least /boot");
		menu->AddItem(new BMenuItem(kNoMountedVolumes, NULL));
	}

	if (menu->CountItems() == 0)
		menu->AddItem(new BMenuItem("no volumes", NULL));
}

static BMenuItem *
EachVolume(BPopUpMenu *menu, BMenuItem *(*eachFunction)(BMenuItem *, void *), 
	void *params)
{
	// select src volume
	int32 count = menu->CountItems();
	for (int32 index = 0; index < count; index++)
		if (menu->ItemAt(index)->Message()){
			BMenuItem *item = menu->ItemAt(index);			
			if (eachFunction(item, params)) 
				return item;
		}
	return 0;
}

struct MarkOneGlueParams {
	bool (*matchFunction)(dev_t, int32, MatchVolumeParams *);
	MatchVolumeParams *passThru;
};

static BMenuItem *
MarkOneGlue(BMenuItem *item, void *castToGlueParams)
{
	MarkOneGlueParams *params = (MarkOneGlueParams *)castToGlueParams;
	dev_t device = -1;
	int32 partition = -1;
	item->Message()->FindInt32("partition_id", &partition);
	if (item->Message()->FindInt32("volume_id", &device) == B_NO_ERROR
		&& (params->matchFunction)(device, partition, params->passThru))
		return item;

	return 0;
}

static bool
MarkOneVolume(BPopUpMenu *menu, bool (*matchFunction)(dev_t, int32, MatchVolumeParams *), 
	MatchVolumeParams *params)
{
	MarkOneGlueParams glueParams;
	glueParams.matchFunction = matchFunction;
	glueParams.passThru = params;
	BMenuItem *item = EachVolume(menu, MarkOneGlue, &glueParams);
	if (item) {
		item->SetMarked(true);
		return true;
	}

	return false;
}

void
TEngine::UpdateSourceVolumeText(BPopUpMenu *pop1, BStringView *sourceText)
{
	BString buffer("Install from: ");
	BMenuItem *item = pop1->FindMarked();
	ASSERT(item);
	if (item != NULL) {
		buffer << item->Label();
	}
	else {
		buffer << "[Error!]";
	}
	sourceText->SetText(buffer.String());
}

void 
TEngine::SelectSourceDest(BPopUpMenu *pop1, BPopUpMenu *pop2,
	BStringView *sourceText, dev_t initialSourceDevice, 
	dev_t initialDestDevice)
{
	// which volume are we launched from?
	app_info	a_info;
	be_app->GetAppInfo(&a_info);

	MatchVolumeParams params;
	params.currentDevice = a_info.ref.device;
	params.deviceList = &deviceList;

	// update source menu
	BMenuItem *item = pop1->ItemAt(0);
	if (strcmp(item->Label(), kNoMountedVolumes) == 0)
		item->SetMarked(true);
	else {
		bool markedOne = false;
			// respect initialDevice, if any
		if (initialSourceDevice >= 0) {
			params.currentDevice = initialSourceDevice;
			markedOne = MarkOneVolume(pop2, VolumeMatches, &params);
		}

		if (!markedOne) {
			params.currentDevice = a_info.ref.device;
			// the source should be identical to the volume we are currently
			// running on
			MarkOneVolume(pop1, VolumeMatches, &params);
		}
	}

	UpdateSourceVolumeText(pop1, sourceText);
			
	// update destination menu
	item = pop2->ItemAt(0);
	if (strcmp(item->Label(), kNoMountedVolumes) == 0)
		item->SetMarked(true);
	else {
		bool markedOne = false;
		
		// respect initialDevice, if any
		if (initialDestDevice >= 0) {
			params.currentDevice = initialDestDevice;
			markedOne = MarkOneVolume(pop2, VolumeMatches, &params);
		}
		
		// match any BeOS volume other than the source volume and not boot
		params.currentDevice = a_info.ref.device;
		if (!markedOne)
			markedOne = MarkOneVolume(pop2, VolumeBeOSNonBootAndDiffers, &params);

		// match anything different than the source volume and not boot
		if (!markedOne)
			markedOne = MarkOneVolume(pop2, VolumeNonBootAndDiffers, &params);
	
		// match anything different than the source volume
		params.currentDevice = a_info.ref.device;
		if (!markedOne)
			markedOne = MarkOneVolume(pop2, VolumeDiffers, &params);

		// match anything
		if (!markedOne)
			markedOne = MarkOneVolume(pop2, MatchAnyVolume, &params);
	 
	 	if (!markedOne && pop2->ItemAt(0))
	 		pop2->ItemAt(0)->SetMarked(true);
	}

	SetVolumePopupTitle(pop2);
	 
	ASSERT(pop1->FindMarked());
	ASSERT(pop2->FindMarked());
}

bool 
TEngine::HasEnoughSpace(const BVolume *volume) const
{
	off_t requestedSpace(fWindow->InstallFileSize() + fWindow->InstallAttrSize()
						 + SizeOfSelectedOptions());
	
	system_info info;
	BDirectory root;
	BPath path;
	BFile swap_file;
	off_t swap_file_size;
	
	// Get ahold of the RAM info and the swap file info
	if (get_system_info(&info) == B_NO_ERROR
		&& volume->GetRootDirectory(&root) == B_NO_ERROR
		&& path.SetTo(&root, "var/swap") == B_NO_ERROR
		&& swap_file.SetTo(path.Path(), B_READ_ONLY) == B_NO_ERROR
		&& swap_file.GetSize(&swap_file_size) == B_NO_ERROR)
	{
		// If we're at 16 MB, truncate the swap file to 64 MB
		if (info.max_pages * 4 / 1024 <= 16
			&& swap_file_size > 64 * 1024 * 1024)
		{
			swap_file.SetSize(64 * 1024 * 1024);
		}
		// If we're above 16 MB, truncate the swap file to 80 MB
		else if (info.max_pages * 4 / 1024 > 16
			&& swap_file_size > 80 * 1024 * 1024)
		{
			swap_file.SetSize(80 * 1024 * 1024);
		}
	}
	
	return volume->FreeBytes() >= requestedSpace;
}

void TEngine::ClearItems(BPopUpMenu* pop)
{
	while (pop->ItemAt(0))
		delete(pop->RemoveItem(0L));
}


void TEngine::StopInstall()
{
	if (State() == INSTALL_FROM)
		SetState(STOP);
}

bool
TEngine::CheckDiskSpaceAndPromptIfTooLow(const BVolume *volume)
{
	if (!HasEnoughSpace(volume)) {
		// if not enough space, just suggest that we might not fit
		char buffer[255];
		sprintf(buffer, kProbablyNotEnoughSpace,
			(SizeOfSelectedOptions() > 0) ? kDontInstallOptionalItems : "");

		if ((CenterAlert(new BAlert("", buffer,
			"Try installing anyway", "Cancel",
			NULL, B_WIDTH_FROM_LABEL, B_WARNING_ALERT)))->Go() == 1)
			return false;
	}

	return true;
}

off_t 
TEngine::SizeOfSelectedOptions() const
{
	OptionView *options = dynamic_cast<OptionView *>(fWindow->FindView("optionView"));
	ASSERT(options);
	if (options)
		return options->CalculateSelectedSize();
	else
		return -1;
}


bool
TEngine::ConfigurationOKWithSinglePartition()
{
	// return true if we are a BeBox

	system_info sysInfo;
	get_system_info(&sysInfo);

	return sysInfo.platform_type == B_BEBOX_PLATFORM;
}

bool
TEngine::NoFormattedDisks(BPopUpMenu *targetVolumeMenu)
	// returns true if there are no disks in <targetVolumeMenu>
{
	BMenuItem *item = targetVolumeMenu->FindMarked();
	int32 partitionID;

	if (!item
		|| !item->Message()
		|| item->Message()->FindInt32("partition_id", &partitionID) != B_NO_ERROR)
		return true;

	return false;
}

struct OneNonBFSParams {
	int32 count;
	DeviceList *deviceList;
};

static BMenuItem *
OneNonBFSPartition(BMenuItem *item, void *castToParams)
{
	OneNonBFSParams *params = (OneNonBFSParams *)castToParams;
	int32 partitionID;
	
	if (item->Message()
		&& item->Message()->FindInt32("partition_id", &partitionID) != B_NO_ERROR) {

		Partition *partition = params->deviceList->PartitionWithID(partitionID);
		if (!partition)
			return 0;
			
		if (strcmp(partition->FileSystemShortName(), "bfs") != 0) {
			params->count++;
			return 0;
		}
		params->count = 2;
			// we hit a bfs disk, the search is over
		return item;
	}
	return 0;
}


bool
TEngine::CheckSingleNonBFSPartion(BPopUpMenu *targetVolumeMenu)
	// returns true if <targetVolumeMenu> contains just a single, non-bfs
	// partition
{
	OneNonBFSParams params;
	params.count = 0;
	params.deviceList = &deviceList;

	EachVolume(targetVolumeMenu, OneNonBFSPartition, &params);
	return params.count == 1;
}

bool
TEngine::CheckDestinationPrimaryPartition(BPopUpMenu *INTEL_ONLY(targetVolumeMenu))
{
#if !__INTEL__
	return false;
#else
	// check if selected partition is the primary bus zeroth partition -
	// an unlikely target on windows machines
	BMenuItem *item = targetVolumeMenu->FindMarked();
	int32 partitionID;

	if (!item
		|| !item->Message()
		|| item->Message()->FindInt32("partition_id", &partitionID) != B_NO_ERROR)
		return false;
	
	return ::CheckDestinationPrimaryPartition(deviceList.PartitionWithID(partitionID));		
#endif
}

void
TEngine::LaunchDriveSetup()
{
	BPath path;
	status_t result = find_directory(B_BEOS_PREFERENCES_DIRECTORY, &path);
	if (result == B_OK) {
		path.Append("DriveSetup");
		PRINT(("launch drive setup %s\n", path.Path()));
		system(path.Path());
		Rescan();
	} else 
		CenterAlert(new BAlert("", "There was a problem launching Drive Setup",
			"OK"))->Go();
}

void
TEngine::HandleButton()
{
	bool done = false;

	if (showingDone) {
		be_app->PostMessage(B_QUIT_REQUESTED);
		return;
	}
		
	TIView *view = (TIView*)fWindow->FindView("IView");
	DisableAllItems(true);

	if (NoFormattedDisks(view->VolumeMenu2())) {
		if (ConfigurationOKWithSinglePartition()) {
			// if no disks and on a BeBox, ask to run DriveSetup
			if (CenterAlert(new BAlert("", kErrNoDisksCanDriveSetup,
				"Cancel", "OK", NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1)
			{
				LaunchDriveSetup();
			}
			else
			{
				EnableAllItems(done);
			}
			
			return;
		} else {
			// non-BeBox, don't see no disks, should
			CenterAlert(new BAlert("", kErrNoDisks, "OK"))->Go();
			EnableAllItems(done);
			return;
		}
	} else if (CheckDestinationPrimaryPartition(view->VolumeMenu2())
		&& CenterAlert(new BAlert("", kFirstPartitionWarning,
			"Go ahead", "Cancel", 0,
			B_WIDTH_FROM_LABEL, B_STOP_ALERT))->Go() == 1) {
		EnableAllItems(done);
		return;
	} else if (CheckSingleNonBFSPartion(view->VolumeMenu2())
		&& !ConfigurationOKWithSinglePartition()) {
		CenterAlert(new BAlert("", kErrSingleMandatorySystemDisk, "OK"))->Go();
		EnableAllItems(done);
		return;
	}
	
	int32 partitionID;
	BMenuItem *item = view->VolumeMenu2()->FindMarked();
	ASSERT (item && item->Message()
		&& item->Message()->FindInt32("partition_id", &partitionID) == B_NO_ERROR);
	
	item->Message()->FindInt32("partition_id", &partitionID);
	
	Partition *partition = deviceList.PartitionWithID(partitionID);
	ASSERT(partition);

	if (!MountSelectedDestinationIfNeeded(partition)) {
		EnableAllItems(done);
		return;
	}
	
	ASSERT(partition->Mounted() == kMounted);
	
	if (view->VolumeMenu1()->FindMarked() == NULL
		|| view->VolumeMenu1()->FindMarked()->Message() == NULL)
	{
		CenterAlert(new BAlert("", "No source volume found!  "
			"This means that Installer cannot detect the device "
			"from which the system booted.  Please contact Be Customer "
			"Technical Support for assistance.  Our U.S. Support "
			"Center can be reached by email at custsupport@be.com "
			"or on the Web at http://www.be.com/support/.",
			"Sorry", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		EnableAllItems(done);
		return;
	}
	
	BVolume src_vol(view->VolumeMenu1()->FindMarked()->
		Message()->FindInt32("volume_id"));
	BVolume dest_vol(partition->VolumeDeviceID());

	if (src_vol == dest_vol)
		fWindow->SetMessage(kErrInstallOnSelf);

	else if (strcmp(partition->MountedAt(), "/boot") == 0
		&& CenterAlert(new BAlert("", kInstallingOverBoot,
			"OK", "Cancel"))->Go() == 1) 
		;

	else if (dest_vol.IsReadOnly())
		fWindow->SetMessage(kErrReadOnly);

	else if (CheckDiskSpaceAndPromptIfTooLow(&dest_vol)) {

		bool installingOverBoot = strcmp(partition->MountedAt(), "/boot") == 0;
		BDirectory	src_dir;
		BDirectory	dest_dir;
		
		src_vol.GetRootDirectory(&src_dir);
		dest_vol.GetRootDirectory(&dest_dir);

		SetState(INSTALL_FROM);
		fWindow->SetButton("Stop");
		
		// look for "home" entry on the destination volume.  If it exists, this must
		// not be a fresh install.
		BEntry homeEntry;
		status_t fresh = dest_dir.FindEntry("home", &homeEntry);
		if (fresh == B_OK) {
			homeEntry.Unset();
			SetFreshInstall(false);
		} else {
			SetFreshInstall(true);
		}


		status_t err = RunInitScript(&src_vol, &dest_vol);
		fWindow->SetBarberPoleVisible(false);
		fWindow->SetMessage("");
		
		if (err == B_NO_ERROR)
		{
			fWindow->SetSizeBarVisible(true);
			
			fWindow->SetSizeBarMaxValue();
			err = CopyFiles(&src_dir, &dest_dir); // copy main files
			
			if (err == B_NO_ERROR)
			{
				// install selected packages
				OptionalPackage **selected = view->Options()->GetSelectedOptions();
				OptionalPackage *package;
				BDirectory pkg_dir;
				int32 index(0);
				
				if (selected != NULL) {
					while ((package = selected[index++]) != NULL) {
						if (pkg_dir.SetTo(package->path) == B_NO_ERROR)
						{
							// We never want clean install when copying packages
							bool saved_clean_install(CleanInstall());
							SetCleanInstall(false);
							err = CopyFiles(&pkg_dir, &dest_dir);
							SetCleanInstall(saved_clean_install);
						}
					}
					free(selected);
				}
			}
			
			fWindow->SetSizeBarVisible(false);
		}
		
		// flush out the disk disks so that we can afford not shutting down 
		// cleanly
		sync();

		bool installLilo = false;
		if (err == B_NO_ERROR)
			installLilo = PromptForLilo(&dest_vol);
		if (err == B_NO_ERROR) {
			// get the name of the device for the destination volume 
			fs_info fileSystemDeviceInfo;
			int32 deviceID = partition->VolumeDeviceID();
			const char *deviceName = 0;
			int32 cookie = 0;
			while (_kstatfs_(-1, &cookie, -1, NULL, &fileSystemDeviceInfo) == B_NO_ERROR)
				if (fileSystemDeviceInfo.dev == deviceID) {
					deviceName = fileSystemDeviceInfo.device_name;
					break;
				}

			ASSERT(deviceName);

			err = RunFinishScript(&src_vol, &dest_vol, deviceName, installLilo);
		}
		if (err == B_NO_ERROR)
			err = PromptForBoot(&dest_vol);

		switch (err) {
			case COPY_CANCELED:
				fWindow->SetMessage("Installation stopped.");
				break;
			case B_NO_ERROR:
				if (err == B_NO_ERROR) {
					fWindow->SetMessage(kDoneInstalling);

					done = true;
					if (installingOverBoot) {
						(CenterAlert(new BAlert("", kMustRebootNow, "OK")))->Go();
						BMessenger(ROSTER_SIG).SendMessage(new BMessage(CMD_REBOOT_SYSTEM));
					}
					break;
				}		
				
			default:
				char text[300];
				sprintf(text, "Error [%s] during installation.", strerror(err));
				fWindow->SetMessage(text);
				break;
		}
	}

	EnableAllItems(done);
	SetState(IDLE);
}

//void 
//TEngine::RunOptionsDialog()
//{
//	fWindow->Lock();
//	
//	if (!m_options_running)
//	{
//		m_options_running = true;
//		TIView *view = (TIView*)fWindow->FindView("IView");
//		ASSERT(view);
//		BWindow *options = new BWindow(CenterRectIn(kOptionViewRect, 
//			view->Window()->Frame()), "", B_MODAL_WINDOW, B_NOT_RESIZABLE);
//		options->AddChild(new OptionView(options->Bounds(), this, view->MenuField1()));
//		options->Show();
//	}
//	
//	fWindow->Unlock();
//}

// This is a list of folders from the AA release that would not
// be replaced by the new installation; in the case of a clean 
// install we offer to delete them for the user

const char *removeIfClean[] = {
	"system",
	"bin",
	"documentation",
	"" };

static int
is_meta(char ch)
{
	if (ch == '['  || ch == ']'  || ch == '*'  || ch == '"' || ch == '\'' ||
		ch == '?'  || ch == '^'  || ch == '\\' || ch == ' ' || ch == '\t' ||
		ch == '\n' || ch == '\r' || ch == '('  || ch == ')' || ch == '{'  ||
		ch == '}'  || ch == '!'  || ch == '$'  || ch == '%' || ch == '~'  ||
		ch == '`'  || ch == '@'  || ch == '#'  || ch == '&' || ch == '>'  ||
		ch == '<'  || ch == ';'  || ch == '|')
		return 1;

    return 0;
}


static char *
escape_meta_chars(char *str)
{
    int len, num_meta = 0;
    char *tmp, *ptr;

    for(len=0, tmp=str; *tmp; tmp++) {
        if (is_meta(*tmp))
            num_meta++;
        else
            len++;
    }

    ptr = (char *)malloc(len + num_meta*2 + 1);
    if (ptr == NULL)
        return ptr;

    for(tmp=ptr; *str; str++) {
        if (is_meta(*str)) {
            *tmp++ = '\\';
            *tmp++ = *str;
        } else {
            *tmp++ = *str;
        }
    }

    *tmp = '\0';     /* null terminate */
    
    return ptr;
}

status_t
TEngine::RunInstallerScriptCommon(BVolume *, BVolume *destination,
	const char *script, const char *destDevice, bool dontInstallLilo)
{
	char buffer[512];
	char dstVolume[256];
	destination->GetName(dstVolume);
	struct stat st;

	// installer init script lives in /boot/beos/system/boot
	BPath scriptPath;
	if (find_directory(B_BEOS_BOOT_DIRECTORY, &scriptPath) != B_OK)
		return B_NO_ERROR;

	scriptPath.Append(script);

	// if we don't have one, just give up silently
	if (stat(scriptPath.Path(), &st) < 0)
		return B_NO_ERROR;

#if	!__INTEL__
	dontInstallLilo = false;
#endif

	char *escapedVolumeName = escape_meta_chars(dstVolume);
	sprintf(buffer, "%s %s %s%s%s%s%s", scriptPath.Path(), escapedVolumeName,
		destDevice ? destDevice : "-",
		CleanInstall() ? " -clean" : " -notclean",
		mergeInstall && !CleanInstall() ? " -merge" : " -notmerge",
		dontInstallLilo ? " -nobootmenu" : " -bootmenu",
		FreshInstall() ? " -fresh" : " -notfresh");
	free(escapedVolumeName);

	PRINT(("executing %s \n", buffer));
	status_t result = Execute(buffer);
	
	return result;
}

status_t
TEngine::RunInitScript(BVolume *source, BVolume *destination)
{
	fWindow->SetMessage("Starting Installation.");
	status_t result = RunInstallerScriptCommon(source, destination,
		"InstallerInitScript");

	if (result != B_NO_ERROR && CenterAlert(new BAlert("", 
			"There was an error while old items were being moved out of the way. "
			"Would you like to continue with the installation?", 
			"Stop", "Continue"))->Go() == 1)
			return B_NO_ERROR;

	return result;
}

status_t
TEngine::RunFinishScript(BVolume *source, BVolume *destination,
	const char *destDevice, bool installLilo)
{
	fWindow->SetMessage("Finishing Installation.");
	return RunInstallerScriptCommon(source, destination,
		"InstallerFinishScript", destDevice, !installLilo);
}

#define BULLET	"\xE2\x80\xA2"
bool
TEngine::PromptForLilo(BVolume *volume)
{
	char name[B_FILE_NAME_LENGTH];
	volume->GetName(name);

#if 0 && __INTEL__
	// currently we are not asking to overwrite MBR, we may
	// in future releases

//	char buf[300];
//	sprintf(buf, "Would you like to install lilo?");
	
	return (CenterAlert(new BAlert("", "Would you like to install a boot menu in the "
		"Master Boot Record on the master disk? \n"
		BULLET"  If you do not do this, you will "
		"have to use the boot floppy every time you want to "
		"to boot the BeOS.\n"
		BULLET"  If you do, you may not be able to boot any "
		"other operating systems off the master disk.\n",

		"Overwrite", "Don't overwrite", 0,
		B_WIDTH_FROM_LABEL, B_STOP_ALERT)))->Go() == 0;

#else
	return false;
#endif
	
}
struct CountRWBFSPartitionsParams {
	int32 count;
};

static Partition *
MatchOneWritableBFS(Partition *partition, void *castToParams)
{
	CountRWBFSPartitionsParams *params = (CountRWBFSPartitionsParams *)castToParams;
	
	if (!partition->Hidden()
		&& UseableAsDestinationPartition(partition)
		&& strcmp(partition->FileSystemShortName(), "bfs") == 0)
		params->count++;

	return NULL;
}

status_t
TEngine::PromptForBoot(BVolume *volume)
{

	CountRWBFSPartitionsParams params;
	params.count = 0;
	
	deviceList.EachPartition(MatchOneWritableBFS, &params);
	ASSERT(params.count > 0);

	char name[B_FILE_NAME_LENGTH];
	volume->GetName(name);

	// On Intel we use bootman to manage which volume to boot 
#if !__INTEL__
	char buf[300];
	sprintf(buf, "Do you want to make \"%s\" the startup disk?", name);
	
	if (params.count == 1
		// if there is multiple bfs partition available, ask if to set
		// new partition as boot
		|| CenterAlert(new BAlert("", buf, "No", "Yes", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go() == 1) {

		dev_t device = volume->Device();
		fs_info info;
		partition_info p_info;
		int32 session = 0;
		int32 partition = 0;
		int32 fd;
		int32 cookie = 0;
		char *dev_name = NULL;
		
		while (_kstatfs_(-1, &cookie, -1, NULL, &info) == B_NO_ERROR) {
			if (info.dev == device) {
				if (!strstr(info.device_name, "/raw")) {
					if ((fd = open(info.device_name, 0)) >= 0) {
						if (ioctl(fd, B_GET_PARTITION_INFO, &p_info) == B_NO_ERROR) {
							dev_name = (char *)&p_info.device;
							session = p_info.session;
							partition = p_info.partition;
						}
						else
							dev_name = (char *)&info.device_name;
						close(fd);
					}
				}
				else
					dev_name = (char *)&info.device_name;
				break;
			}
		}
			
	
		if (dev_name) {
			int dev;
			int bus;
			int id;
			int lun;
			char devicestring[6]; // strlen("atapi")+1
			char idstring[7]; // strlen("master")+1

			if(sscanf(dev_name, "/dev/disk/scsi/%d/%d/%d/", &bus, &id, &lun) == 3) {
				dev = bootdev_scsi;
			}
			else if(sscanf(dev_name, "/dev/disk/ide/%5[a-z]/%d/%6[a-z]/%d/", devicestring, &bus, idstring, &lun) == 4) {
				if(strcmp("ata", devicestring) == 0) {
					dev = bootdev_ata;
				}
				else if(strcmp("ata", devicestring) == 0) {
					dev = bootdev_atapi;
				}
				else
					return B_ERROR;

				if(strcmp("master", idstring) == 0) {
					id = 0;
				}
				else if(strcmp("slave", idstring) == 0) {
					id = 1;
				}
				else
					return B_ERROR;
			}
			else
				return B_ERROR;

			PRINT(("setting %s, dev %d, session %d, partition %d, bus %d, "
				"id %d, lun %d as boot device\n", 
				dev_name, dev, session, partition + 1, bus,
				id, lun));

			write_config_item(CFG_boot_dev, (char *)&dev);
			write_config_item(CFG_boot_bus, (char *)&bus);
			write_config_item(CFG_boot_id, (char *)&id);
			write_config_item(CFG_boot_lun, (char *)&lun);
			write_config_item(CFG_boot_sid, (char *)&session);
			write_config_item(CFG_boot_pid, (char *)&partition);

		}
		else
			return B_ERROR;
	}
#endif			

	return B_OK;
}

// Looks for optional packages to install on the source disk and adds the packages
// to the options list.
status_t TEngine::ScanSourceVolumeForOptions()
{
	TIView *view = (TIView*)fWindow->FindView("IView");

	if (!view->VolumeMenu1()->FindMarked()
		|| !view->VolumeMenu1()->FindMarked()->Message())
		// no source volume selected, bail out
		return B_ERROR;

	BVolume src_vol(view->VolumeMenu1()->FindMarked()->Message()->FindInt32("volume_id"));
	if (src_vol.InitCheck() == B_OK) {
		// load install sizes
		off_t size(0), attr(0), num(0);
		ReadInstallSizes(&src_vol, &size, &attr, &num);
		fWindow->SetInstallSizes(size, attr, num);
		
		// load package info
		BDirectory dir;
		BDirectory pkg_dir;

		src_vol.GetRootDirectory(&dir);
		BEntry entry;
		char name[B_FILE_NAME_LENGTH];
		struct stat entry_stat;
		// look for the _packages_ directory 
		while (dir.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
			entry.GetName(name);
			if (!strcmp(name, "_packages_")) {
				entry.GetStat(&entry_stat);
				if (S_ISDIR(entry_stat.st_mode)) {
					pkg_dir.SetTo(&entry);
				}
			}
		}
	
		OptionView *optview = view->Options();
		optview->Clear();
	
		if (pkg_dir.InitCheck() != B_OK) { // no _packages_ directory
			optview->ArrangeOptions();
			return B_ERROR;	
		}
	
		OptionalPackage *opkg;
		// scan through packages directory, creating an OptionalPackage for each subdirectory
		while (pkg_dir.GetNextEntry(&entry) == B_OK) {
			entry.GetStat(&entry_stat);
			if (S_ISDIR(entry_stat.st_mode) || S_ISLNK(entry_stat.st_mode)) {
				opkg = OptionalPackage::CreateFromEntry(entry);
				if (opkg != NULL) {
					optview->AddOption(new OptionCheckBox(opkg, new BMessage(OPTION_CHECKED)));
				}
			}
		}
	
		optview->ArrangeOptions();
		view->UpdateSpaceLabel();
	
		return B_OK;
	} else {
		return B_ERROR;
	}
}


class InstallerCopyLoopControl : public _CopyLoopControl {
public:
	InstallerCopyLoopControl(TEngine *, bool cleanInstall = false);
	virtual ~InstallerCopyLoopControl() {}

	virtual bool FileError(const char *message, const char *name, bool allowContinue);
		// inform that a file error occurred while copying <name>
		// returns true if user decided to continue

	virtual void UpdateStatus(const char *name, entry_ref ref, int32 count, 
		bool optional);

	virtual bool CheckUserCanceled();
		// returns true if canceled
	
	virtual OverwriteMode OverwriteOnConflict(const BEntry *srcEntry,
		const char *destName, const BDirectory *destDir, 
		bool directory, bool canMerge = true);
		// override to always overwrite, never overwrite, let user decide, etc.

	virtual bool SkipEntry(const BEntry *, bool file);
		// override to prevent copying of a given file or directory

	virtual void ChecksumChunk(const char *block, size_t size);
	virtual bool ChecksumFile(const entry_ref*);

	virtual bool SkipAttribute(const char *attributeName);
	virtual bool PreserveAttribute(const char *attributeName);

private:
	TEngine *engine;
	bool alwaysReplace;
	bool cleanInstall;
	MD4Checksum checksum;
	bool ignoreAllFileErrors;
};

InstallerCopyLoopControl::InstallerCopyLoopControl(TEngine *engine, bool cleanInstall)
	:	engine(engine),
		alwaysReplace(false),
		cleanInstall(cleanInstall),
		ignoreAllFileErrors(false)
{
}

bool 
InstallerCopyLoopControl::FileError(const char *message, const char *name, 
	bool allowContinue)
{
	char buf[500];
	sprintf(buf, message, name);
	engine->fWindow->SetMessage(buf);

	if (!allowContinue) {
		engine->CenterAlert(new BAlert("", buf, "Cancel"))->Go();
	} else {
		BAlert *alert = engine->CenterAlert(new BAlert("", buf, "Cancel", "OK"));
		return (alert->Go() != 0);
	}
	return false;
}

void 
InstallerCopyLoopControl::UpdateStatus(const char *name, entry_ref a_entry, int32,
	bool)
{
	if (!name)
		return;

	// update message with next filename
	BString buff;
	buff << "Installing " << name;
	engine->fWindow->SetMessage(buff.String());
	
	if (engine->fWindow->InstallFileSize() > 0
		&& engine->fWindow->InstallNum() > 0)
	{
		BEntry entry(&a_entry);
		BNode node;
		off_t size(0);
		
		if (node.SetTo(&entry) == B_NO_ERROR
			&& node.GetSize(&size) == B_NO_ERROR)
		{
			engine->fWindow->Update(size + 2048);
		}
	}
}

bool 
InstallerCopyLoopControl::CheckUserCanceled()
{
	char buf[300];
	// check for user stop
	if (engine->State() == STOP) {
		sprintf(buf, "Are you sure you want to stop the installation?"
			" If you do, you may not be able to boot from or use all"
			" the software on the disk onto which you were installing.");
		BAlert *alert = engine->CenterAlert(new BAlert("", buf, "Continue", "Stop", NULL,
						   B_WIDTH_AS_USUAL, B_WARNING_ALERT));
		int result = alert->Go();
		if (result == 1) {		// user wants to stop
			return true;
		} else
			engine->SetState(INSTALL_FROM);
	}
	return false;
}

const bool cleanInstall = false;

const char kSettingsDir[] = "/boot/home/config/settings";
const int32 kSettingsLength = 26;
const char kMimeSettingsDir[] = "/boot/home/config/settings/beos_mime";
const int32 kMimeSettingsLength = 36;
const char kVarDir[] = "/boot/var";
const int32 kVarDirLength = 9;

_CopyLoopControl::OverwriteMode 
InstallerCopyLoopControl::OverwriteOnConflict(const BEntry *srcEntry,
	const char *name, const BDirectory *dstDir, bool srcIsFolder, bool dstIsFolder)
{
	if (srcIsFolder && dstIsFolder && !cleanInstall)
		return kMerge;

	BPath path;
	srcEntry->GetPath(&path);

	if (!cleanInstall) {
		ASSERT(strlen(kMimeSettingsDir) == kMimeSettingsLength);
		ASSERT(strlen(kSettingsDir) == kSettingsLength);
		ASSERT(strlen(kVarDir) == kVarDirLength);

		if (strncmp(path.Path(), kMimeSettingsDir, kMimeSettingsLength) == 0) {
			// Update mime database, merging attributes where needed.
			return kMerge;
		}

		if (strncmp(path.Path(), kSettingsDir, kSettingsLength) == 0
			|| strncmp(path.Path(), kVarDir, kVarDirLength) == 0) {
			// do not overwrite anything in /boot/home/config/settings or in /boot/var
			PRINT(("skipping %s in user settings, already installed\n", path.Path()));
			return kSkip;
		}
		
		if (strcmp(path.Path(), "/boot/var/log/syslog") == 0) {
			// avoid prompting about a newer syslog, just don't overwrite it
			PRINT(("skipping %s, already installed\n", path.Path()));
			return kSkip;
		}
	}

	if (strcmp(path.Path(), "/boot/home/config/settings/beos_mime/__mime_table") == 0)
		// always replace the cached app signatures
		return kReplace;

	// user already said replace all
	if (alwaysReplace)
		return kReplace;


	char buf[500];
	if (!srcIsFolder && !dstIsFolder) {
		// check out the dates
		time_t srcTime = 0;
		time_t dstTime = 0;


		srcEntry->GetModificationTime(&srcTime);
		BEntry(dstDir, name).GetModificationTime(&dstTime);
		
		if (dstTime <= srcTime)    /* older destination files always get replaced */
			return kReplace;

		engine->mergeInstall = true;
			// we are overwriting something old, must be a merge install
		sprintf(buf, "A newer copy of \"%s\" already exists on the destination volume."
					" Do you want to replace it?", name);

		// handle overwriting an new file with an old one
		// the most likely thing to do is to keep the new file

		BAlert *alert = engine->CenterAlert(new BAlert("", buf, 
			"Replace All", "Replace", "Keep New", 
			B_WIDTH_FROM_LABEL, B_STOP_ALERT));

		switch (alert->Go()) {
			case 0:
				alwaysReplace = true;
				return kReplace;
			case 1:
				return kReplace;
			case 2:
				return kSkip;
		}
	}
	
	// handle clean install, overwriting a folder with a file or a file
	// with a folder
	
	sprintf(buf, "\"%s\" already exists on the destination volume."
				" Do you want to replace it?%s", name,
				dstIsFolder ? " If you choose to replace a folder, all "
					"of its contents will be deleted." : "");
	
	BAlert *alert = engine->CenterAlert(new BAlert("", buf, 
		 "Keep New", "Replace", "Replace All", 
			B_WIDTH_FROM_LABEL, B_STOP_ALERT));

	switch (alert->Go()) {
		case 0:
			return kSkip;
		case 1:
			return kReplace;
		case 2:
			alwaysReplace = true;
			return kReplace;
	}
	return kSkip;
}

bool 
InstallerCopyLoopControl::SkipEntry(const BEntry *entry, bool file)
{
	char entryName[B_FILE_NAME_LENGTH];

	if (entry->IsDirectory()) {
		BDirectory dir(entry);
		
		// always skip directories with a _do_not_install_ entry
		if (dir.Contains("_do_not_install_"))
			return true;
		
//		// avoid directories with an _optional_install_ entry
//		if (dir.Contains("_optional_install_"))
//			return true;
		entry->GetName(entryName);
		// don't install _packages_ directory
		if (strcmp(entryName, "_packages_") == 0)
			return true;
		if (strcmp(entryName, "_do_not_install_") == 0)
			return true;
	}

	if (file) {
		// avoid the kernel's swap
		entry->GetName(entryName);
	
		if (strcmp(entryName, "swap") != 0)
			return false;

		// XXX: this test is insufficient to avoid copying a swap file.  We should never copy
		// any file named 'swap' in a directory 'var' that is in the source volume's root dir.
		// -nschrenk
		BDirectory varDirectory("/boot/var");
		BDirectory parent;
		entry->GetParent(&parent);
	
		if (parent == varDirectory)
			return true;
	}
	
	return false;
}

void 
InstallerCopyLoopControl::ChecksumChunk(const char *block, size_t size)
{
	if (!ignoreAllFileErrors)
		checksum.Process((char *)block, size);
}

bool 
InstallerCopyLoopControl::ChecksumFile(const entry_ref *ref)
{
	if (ignoreAllFileErrors)
		return true;

	bool fileOk = false;

	BNode node(ref);
	if (node.InitCheck() == B_OK) {
		char checkAttr[16];
		ssize_t attrSize = node.ReadAttr(kChecksumAttrName, kChecksumAttrType,
			0, checkAttr, 16);
		if (attrSize == 16) {
			if (checksum.Equals(checkAttr)) {
				fileOk = true;
			} else {
				fileOk = false;
			}
		} else {
			if ( (attrSize == B_ENTRY_NOT_FOUND) || (attrSize == EINVAL) ) {
				// should we warn that the file does not have a checksum?
				fileOk = true;
			} else {
				fileOk = false;
			}
		}
	} else {
		fileOk = false;
	}
	if (!fileOk) {
		BEntry ent(ref);
		BPath path;
		ent.GetPath(&path);
		BString errmsg;
		errmsg << "The file " << path.Path() << " appears to be corrupted.  The system "
			"may not work correctly after it is installed.  Do you want to continue "
			"anyway?";

		BAlert *alert = engine->CenterAlert(new BAlert("", errmsg.String(),
			"Ignore All", "Ignore", "Abort", B_WIDTH_AS_USUAL, B_WARNING_ALERT));
		int result = alert->Go();
		switch (result) {
		case 0:		// ignore all
			fileOk = true;
			ignoreAllFileErrors = true;
			engine->SetState(INSTALL_FROM);
			break;
		
		case 1:		// Ignore
			fileOk = true;
			engine->SetState(INSTALL_FROM);
			break;
			
		case 2:		// abort
			engine->fWindow->SetMessage("Stopping Install");
			engine->StopInstall();
			break;
			
		}
	}

	checksum.Reset();	
	return fileOk;
}

bool 
InstallerCopyLoopControl::SkipAttribute(const char *attributeName)
{
	for (const char **skipAttribute = kInstallerSkipAttributes; *skipAttribute;
		skipAttribute++) {
		if (strcmp(*skipAttribute, attributeName) == 0)
			return true;
	}

	return false;
}

bool 
InstallerCopyLoopControl::PreserveAttribute(const char *attributeName)
{
	for (const char **preserveAttribute = kPreserveAttributes; *preserveAttribute;
		preserveAttribute++) {
		if (strcmp(*preserveAttribute, attributeName) == 0)
			return true;
	}

	return false;
}

status_t 
TEngine::CopyFiles(BDirectory *sourceDirectory, BDirectory *destDirectory)
{
	InstallerCopyLoopControl loopControl(this, CleanInstall());

	sourceDirectory->Rewind();
	BEntry entry;

	struct stat statbuf;
	sourceDirectory->GetStat(&statbuf);
	dev_t sourceDeviceID = statbuf.st_dev;

	while (sourceDirectory->GetNextEntry(&entry) == B_NO_ERROR) {
		if (loopControl.CheckUserCanceled()) {
			PRINT(("cancelling \n"));
			return COPY_CANCELED;
		}


//+#define TESTING
#if DEBUG && defined TESTING

		char name[256];
		entry.GetName(name);
		if (strcmp("testing", name) != 0)
			continue;
#endif

		entry.GetStat(&statbuf);

		status_t err;
		if (S_ISDIR(statbuf.st_mode)) {
			// entry is a mount point, do not copy it
			if (statbuf.st_dev != sourceDeviceID) {
				PRINT(("Avoiding mount point %d, %d	\n", statbuf.st_dev, sourceDeviceID));
				continue;
			}

			if ((err = FSCopyFolder(&entry, destDirectory, &loopControl)) != B_NO_ERROR) {
#if DEBUG
				char name[256];
				entry.GetName(name);
				PRINT(("error %s copying directory %s\n", strerror(err), name));
#endif
				return err;
			}
		} else {
			if ((err = FSCopyFile(&entry, &statbuf, destDirectory, &loopControl)) != B_NO_ERROR) {
#if DEBUG
				BPath		pathName;
				status_t pathErr = entry.GetPath(&pathName);
				PRINT(("error %s copying %s\n", 
					strerror(pathErr), pathName.Path()));
#endif
				return err;
			}
		}
	}

	return B_NO_ERROR;
}


bool 
TEngine::CleanInstall() const
{
	return cleanInstall;
}

void 
TEngine::SetCleanInstall(bool /*on*/)
{
	// in the current Installer, cleanInstall is never turned on
	//cleanInstall = on;
	cleanInstall = false;
}

bool 
TEngine::FreshInstall() const
{
	return freshInstall;
}

void 
TEngine::SetFreshInstall(bool fresh)
{
	freshInstall = fresh;
}



