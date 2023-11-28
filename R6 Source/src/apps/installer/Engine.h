//******************************************************************************
//
//	File:			Engine.h
//
//	Description:	Installer application engine.
//
//	Written by:		Steve Horowitz
//
//	Copyright 1996, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#ifndef ENGINE_H
#define ENGINE_H


#ifndef _BE_H_
#include <Application.h>
#include <File.h>
#include <Locker.h>
#include <Volume.h>
#include <Directory.h>
#include <Looper.h>
#endif
class BPopUpMenu;


#ifndef _STANDALONE_INSTALLER_BUILD_
#include <private/storage/DeviceMap.h>
#define _CopyLoopControl CopyLoopControl
#else
#include "InternalGlue.h"
#endif

#include "IWindow.h"

// ToDo:
// Split these up into separate groups -
// they are a mix of engine state values, message constants, etc.
enum {
	INIT_ENGINE = 1,
	IDLE,
	NEXT_DISK,
	LOADING,
	STOP,
	FINISH_INSTALL,
	DONE,
	QUITTING,
	INSTALL_FROM,
	FORMAT_VOLUME,
	INITIALIZE_VOLUME,
	COMMAND_SELECTED,
	SOURCE_VOLUME_SELECTED,
	DESTINATION_VOLUME_SELECTED,
	MOUNT_SOURCE_VOLUME,
	UNMOUNTED_SOURCE_PARTITION_SELECTED,
	MOUNT_DESTINATION_VOLUME,
	UNMOUNTED_DESTINATION_PARTITION_SELECTED,
	INITIALIZE_AND_MOUNT_VOLUME,
	DONE_INITIALIZING,
	DONE_INSTALLING,
	LAUNCH_DRIVE_SETUP,
	DISPLAY_MESSAGE,
	SHOW_BARBER_POLE,
	HIDE_BARBER_POLE
};

class TrackerCopyLoopControl;
class DeviceList;
class BAlert;
class BStringView;

class TEngine : public BLooper {
public:
	TEngine();
	virtual ~TEngine();
	
	virtual void MessageReceived(BMessage*);
	virtual bool QuitRequested();

	void StartMeUp();
	void StopInstall();
	int32 State();
	void SetState(int32);
	void Rescan();
		// call after DriveSetup

	bool CleanInstall() const;
	void SetCleanInstall(bool);

	bool FreshInstall() const;
	void SetFreshInstall(bool);
	TIWindow *Window()
		{ return fWindow; }

private:
	void RescanCommon(bool initial);

	void SelectSourceDest(BPopUpMenu* BPopUpMenu, BPopUpMenu* destPopup,
		BStringView *sourceText,
 		dev_t initialSourceDevice = -1, 
 		dev_t initialDestDevice = -1);
	void UpdateSourceVolumeText(BPopUpMenu *, BStringView *);

	void EnableAllItems(bool showDone);
	void EnableDisableCommon(bool beginButton, bool rest);
	void DisableAllItems(bool keepStopEnabled);

	status_t PromptForBoot(BVolume*);
	bool PromptForLilo(BVolume *);

	status_t RunInitScript(BVolume *, BVolume *);
	status_t RunFinishScript(BVolume *, BVolume *, const char *destDevice,
		bool installLilo);	
	status_t RunInstallerScriptCommon(BVolume *, BVolume *, const char *script,
		const char *destDevice = 0, bool dontInstallLilo = false);

	void AdjustVolumeMenus();
	void ClearItems(BPopUpMenu* pop);
	
	BAlert *CenterAlert(BAlert *);

	bool HasEnoughSpace(const BVolume *) const;	
	bool CheckDiskSpaceAndPromptIfTooLow(const BVolume *volume);

	bool ConfigurationOKWithSinglePartition();
		// return true if we are a BeBox
	bool NoFormattedDisks(BPopUpMenu *);
		// returns true if there are no disks in <targetVolumeMenu>
	bool CheckSingleNonBFSPartion(BPopUpMenu *);
		// returns true if <targetVolumeMenu> contains just a single, non-bfs
		// partition
	bool CheckDestinationPrimaryPartition(BPopUpMenu *);
		// returns true if <targetVolumeMenu> has the first partition on a primary
		// master IDE disk selected and is an unlikely installation target


	void BuildDestinationMenu(BPopUpMenu *menu);
	void BuildSourceMenu(BPopUpMenu *menu);
	
	void UnmountedSourcePartitionSelected(BMessage *);
	
	bool MountSelectedDestinationIfNeeded(Partition *);

	void HandleButton();
	
	status_t CopyFiles(BDirectory *src, BDirectory *dest);
	
//	bool CheckUserStop();
 		// checks for stops, brings up alert, returns true if stopped
	void ReadInstallSizes(BVolume *vol, off_t *file_size, off_t *attr_size, off_t *num);
	status_t ScanSourceVolumeForOptions(); // looks for _packages_ dir and loads the options
	
	off_t SizeOfSelectedOptions() const;

	void LaunchDriveSetup();

	BLocker fStateLocker;
	TIWindow *fWindow;
	int32 fState;
	int32 fDisk;
	DeviceList deviceList;

	bool cleanInstall;
	bool showingDone;
	bool mergeInstall;
	bool freshInstall;
	
	dev_t defaultSourceVolume;
	dev_t defaultDestinationVolume;

friend class InstallerCopyLoopControl;


	typedef BLooper inherited;
};

#endif
