/*****************************************************************************
//
//	File:		VolumeRoster.h
//
//	Description:	BVolumeRoster class.  Iterate over mounted volumes,
//					and watch volumes as they're mounted and unmounted.
//
//	Copyright 1992-98, Be Incorporated
//
*****************************************************************************/

#ifndef _STORAGE2_VOLUMEROSTER_H
#define _STORAGE2_VOLUMEROSTER_H

#include <support2/SupportDefs.h>
#include <storage2/Volume.h>

namespace B {
namespace Storage2 {

class BVolumeRoster {

public:
						BVolumeRoster();
virtual					~BVolumeRoster();

		status_t		GetNextVolume(BVolume *vol);
		void			Rewind();

		status_t		GetBootVolume(BVolume *vol);
//		status_t		StartWatching(BMessenger msngr=be_app_messenger);

		void			StopWatching(void);

//		BMessenger		Messenger(void) const;

private:

virtual	void		_SeveredVRoster1();
virtual	void		_SeveredVRoster2();

		int32			fPos;
		BMessenger		*fTarget;

#if !_PR3_COMPATIBLE_
		uint32			_reserved[3];
#endif

};

} }	// namespace B::Storage2

#endif	// _STORAGE2_VOLUMEROSTER_H
