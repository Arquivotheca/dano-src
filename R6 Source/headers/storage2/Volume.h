/*****************************************************************************
//
//	File:		Volume.h
//
//	Description:	BVolume class
//
//	Copyright 1992-98, Be Incorporated
//
*****************************************************************************/

#ifndef _STORAGE2_VOLUME_H
#define _STORAGE2_VOLUME_H

#include <sys/types.h>
#include <support2/SupportDefs.h>
#include <storage2/Mime.h>

namespace B {
namespace Storage2 {

class BVolume {
public:
							BVolume();
							BVolume(dev_t dev);
							BVolume(const BVolume &vol);

virtual						~BVolume();

			status_t		InitCheck() const;

			status_t		SetTo(dev_t dev);
			void			Unset(void);

			dev_t			Device() const;

			status_t		GetRootDirectory(BDirectory *dir) const;

			off_t			Capacity() const;
			off_t			FreeBytes() const;

			status_t		GetName(char *name) const;
			status_t		SetName(const char *name);

//			status_t		GetIcon(BBitmap *icon, icon_size which) const;
		
			bool			IsRemovable() const;
			bool			IsReadOnly() const;
			bool			IsPersistent() const;
			bool			IsShared() const;
			bool			KnowsMime() const;
			bool			KnowsAttr() const;
			bool			KnowsQuery() const;
		
			bool			operator==(const BVolume &vol) const;
			bool			operator!=(const BVolume &vol) const;
			BVolume &		operator=(const BVolume &vol);

private:

friend class BVolumeRoster;

virtual	void		_PumpUpTheVolume_();
virtual	void		_PumpUpTheVolume__();
virtual	void		_PumpUpTheVolume___();
virtual	void		_Dance_();
virtual	void		_Dance__();
virtual	void		_TurnUpTheVolume6();
virtual	void		_TurnUpTheVolume7();
virtual	void		_TurnUpTheVolume8();

		dev_t			fDev;
		status_t		fCStatus;

#if !_PR3_COMPATIBLE_
		int32			_reserved[8];
#endif

};

} } // namespace B::Storage2

#endif	// _STORAGE2_VOLUME_H
