#include <stdio.h>
#include <string.h>
#include <fs_info.h>

#include <Entry.h>
#include <Path.h>
#include <Volume.h>
#include <Directory.h>
#include <Mime.h>
#include <Bitmap.h>

#include "Storage.h"
#include "priv_syscalls.h"



BVolume::BVolume()
{
	fDev = -1;
	fCStatus = B_NO_INIT;
}

BVolume::BVolume(dev_t dev)
{
	fDev = -1;
	SetTo(dev);
}

BVolume::BVolume(const BVolume &vol)
{
	fDev = -1;
	SetTo(vol.Device());
}

BVolume::~BVolume()
{
}

status_t BVolume::InitCheck() const
{
  return fCStatus;
}

status_t	BVolume::SetTo(dev_t dev)
{
	int					err;
	struct fs_info		info;

	fDev = -1;
	err = _kstatfs_(dev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return (fCStatus=err);
	fDev = dev;
	return (fCStatus=B_NO_ERROR);
}

void BVolume::Unset() 
{
  fDev = -1;
  fCStatus = B_NO_INIT;
}

dev_t	BVolume::Device() const
{
	return fDev;
}


status_t	BVolume::GetRootDirectory(BDirectory *dir) const
{
	int				err;
	struct fs_info	info;
	int				fd = -1;
	uint			i;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		goto error1;

	for(i=0; i < sizeof(_omodes_) / sizeof(int); i++) {
		fd = _kopen_vn_(info.dev, info.root, NULL, _omodes_[i], true);
		if (fd >= 0)
			break;
	}

	if (i == sizeof(_omodes_) / sizeof(int)) {
		err = fd;
		goto error1;
	}

	err = dir->set_fd(fd);
	if (err != B_OK)
		goto error2;

	return 0;

error2:
	_kclose_(fd);
error1:
	return err;
}

off_t	BVolume::Capacity() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return (off_t) err;
	return info.total_blocks * info.block_size;
}

off_t	BVolume::FreeBytes() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return (off_t) err;
	return info.free_blocks * info.block_size;
}

status_t	BVolume::GetIcon(BBitmap *icon, icon_size which) const
{
	int32 size = (int32) which;
	if (!icon)
		return B_BAD_VALUE;

	fs_info	info;
	status_t err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return err;

	int32 fda = _kopen_vn_(info.dev, info.root, NULL, O_RDONLY, true);
	if (fda >= 0) {
			if (size == 256)
				err = _kread_attr_(fda,"BEOS:M:STD_ICON",0x4d49434e,0,icon->Bits(),size);
			else if (size == 1024)
				err = _kread_attr_(fda,"BEOS:L:STD_ICON",0x4d49434e,0,icon->Bits(),size);
			else
				err = B_ERROR;
		_kclose_(fda);
	} else
		err = fda;

	if (err != B_OK)
		err = get_device_icon(info.device_name, icon->Bits(), size);

	return err;
}

status_t	BVolume::GetName(char *buf) const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return err;
	strcpy(buf, info.volume_name);
	return 0;
}

status_t	BVolume::SetName(const char *name)
{
	status_t	err;
	int			cnt;
	char		bufa[B_FILE_NAME_LENGTH], bufb[PATH_MAX];
	fs_info		info;
	struct stat	st;

	err = GetName(bufa);
	if (err != B_OK)
		return err;

	strcpy(info.volume_name, name);
	err = _kwfsstat_(fDev, &info, WFSSTAT_NAME);
	if (err != B_OK)
		return err;

	/* ###
	'keep' the volume name and the mount point in sync. note this is not
	robust code: changing the volume name though the C API will not update
	the mount point, thus creating a 'minor' inconsistency. note also that
	the following code is not thread safe.
	*/

	cnt = 1;
	sprintf(bufb, "/%s", bufa);
	do {
		err = _krstat_(-1, bufb, &st, true);
		if (!err && (st.st_dev == fDev))
			break;
		sprintf(bufb, "/%s%d", bufa, cnt++);
	} while (cnt < 128);
	if (cnt == 128)
		return 0;

	err = _krstat_(-1, "/boot", &st, true);
	if (err != B_OK)
		return 0;

	sprintf(bufa, "/%s", name);
	cnt = 1;
	do {
		err = _krename_(-1, bufb, -1, bufa);
		sprintf(bufa, "/%s%d", name, cnt++);
	} while (err && (cnt < 128));

	return 0;
}

bool	BVolume::IsRemovable() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return false;
	return (info.flags & B_FS_IS_REMOVABLE);
}

bool	BVolume::IsReadOnly() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return false;
	return (info.flags & B_FS_IS_READONLY);
}

bool	BVolume::IsPersistent() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return false;
	return (info.flags & B_FS_IS_PERSISTENT);
}

bool	BVolume::IsShared() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return false;
	return (info.flags & B_FS_IS_SHARED);
}

bool	BVolume::KnowsMime() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return false;
	return (info.flags & B_FS_HAS_MIME);
}

bool	BVolume::KnowsAttr() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return false;
	return (info.flags & B_FS_HAS_ATTR);
}

bool	BVolume::KnowsQuery() const
{
	int				err;
	struct fs_info	info;

	err = _kstatfs_(fDev, NULL, -1, NULL, &info);
	if (err != B_OK)
		return false;
	return (info.flags & B_FS_HAS_QUERY);
}

bool	BVolume::operator==(const BVolume &vol) const
{
	return (fDev == vol.fDev);
}

bool	BVolume::operator!=(const BVolume &vol) const
{
	return (fDev != vol.fDev);
}

BVolume &	BVolume::operator=(const BVolume &vol)
{
  fCStatus = SetTo(vol.Device());
  return *this;
}

void
BVolume::_TurnUpTheVolume1()
{
}

void
BVolume::_TurnUpTheVolume2()
{
}

#if !_PR3_COMPATIBLE_

void 
BVolume::_TurnUpTheVolume3()
{
}

void 
BVolume::_TurnUpTheVolume4()
{
}

void 
BVolume::_TurnUpTheVolume5()
{
}

void 
BVolume::_TurnUpTheVolume6()
{
}

void 
BVolume::_TurnUpTheVolume7()
{
}

void 
BVolume::_TurnUpTheVolume8()
{
}

#endif
