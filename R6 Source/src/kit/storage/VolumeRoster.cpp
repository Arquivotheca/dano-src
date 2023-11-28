#include <stdio.h>
#include <fs_info.h>

#include <VolumeRoster.h>
#include <Volume.h>
#include <Messenger.h>
#include <NodeMonitor.h>

#include "Storage.h"
#include "priv_syscalls.h"


BVolumeRoster::BVolumeRoster()
{
	fPos = 0;
	fTarget = NULL;
}

BVolumeRoster::~BVolumeRoster()
{
  if (fTarget)
	StopWatching();
}

void		BVolumeRoster::Rewind()
{
	fPos = 0;
}

status_t		BVolumeRoster::GetNextVolume(BVolume *vol)
{
	struct fs_info	info;
	status_t		err;

	err = _kstatfs_(-1, &fPos, -1, NULL, &info);
	if (err == B_NO_ERROR)
		err = vol->SetTo(info.dev);
	else
	  vol->Unset();

	return err;
}


status_t	BVolumeRoster::GetBootVolume(BVolume *vol)
{
	struct fs_info	info;
	status_t			err;
	
	err = _kstatfs_(-1, NULL, -1, "/boot", &info);

	if (err == B_NO_ERROR)
	  err = vol->SetTo(info.dev);
	else
	  vol->Unset();

	return err;
}

status_t		BVolumeRoster::StartWatching(BMessenger msngr)
{
  if (fTarget)
	StopWatching();
  fTarget = new BMessenger(msngr);
  return (watch_node(NULL, B_WATCH_MOUNT, *fTarget));
}

void BVolumeRoster::StopWatching(void)
{
  if (fTarget) {
	stop_watching(*fTarget);
	delete fTarget;
	fTarget = NULL;
  }
}

BMessenger BVolumeRoster::Messenger() const
{
  BMessenger target;
  if (fTarget)
	target = *fTarget;
  return (target);
}

void	BVolumeRoster::_SeveredVRoster1() {}
void	BVolumeRoster::_SeveredVRoster2() {}

