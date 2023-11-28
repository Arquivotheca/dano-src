#include <File.h>

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <fs_attr.h>
#include <Path.h>
#include <String.h>

#include <fcntl.h>
#include <stdio.h>

#include "Storage.h"
#include "priv_syscalls.h"


BFile::BFile()
{
	fMode = 0;
}

BFile::BFile(const entry_ref *ref, uint32 open_mode)
{
	fMode = 0;
	SetTo(ref, open_mode);
}

BFile::BFile(const BEntry *entry, uint32 open_mode)
{
	fMode = 0;
	SetTo(entry, open_mode);
}

BFile::BFile(const char *path, uint32 open_mode)
{
	fMode = 0;
	SetTo(path, open_mode);
}

BFile::BFile(const BDirectory *dir, const char *path, uint32 open_mode)
{
	fMode = 0;
	SetTo(dir, path, open_mode);
}

BFile::BFile(const BFile &file)
	:	BNode(),		// don't call BNode(file) - would cause double fd open
		BPositionIO()
{
	fMode = 0;
	*this = file;
}

BFile::~BFile()
{
	set_fd(-1);
}

status_t	BFile::SetTo(const entry_ref *ref, uint32 open_mode)
{
	BEntry	entry;

	entry.SetTo(ref);

	return SetTo(&entry, open_mode);
}

status_t	BFile::SetTo(const BEntry *entry, uint32 open_mode)
{
	status_t	err;
	int			fd;

	if (!entry) {
		err = B_BAD_VALUE;
		goto error1;
	}

	if (entry->fDfd < 0 && entry->fName == NULL) {
		err = B_BAD_VALUE;
		goto error1;
	}

	fd = _kopen_(entry->fDfd, entry->fName, open_mode, 
				 DEFAULT_MODE_BITS, TRUE);
	if (fd < 0) {
		if((open_mode & O_RWMASK) == O_RDWR){
			open_mode = (open_mode & ~O_RWMASK) | O_RDONLY;
			fd = _kopen_(entry->fDfd, entry->fName, open_mode,
						 DEFAULT_MODE_BITS, TRUE);
			if(fd < 0) {
				err = fd;
				goto error1;
			}
		} else {
			err = fd;
			goto error1;
		}
	}
	
	set_fd(fd);
	fMode = open_mode & ~(O_CREAT | O_TRUNC);
	return 0;

error1:
	Unset();
	fCStatus = err;
	return err;
}
		
status_t	BFile::SetTo(const char *path, uint32 open_mode)
{
	BEntry	entry;

	entry.SetTo(path);

	return SetTo(&entry, open_mode);
}

status_t		BFile::SetTo(const BDirectory *dir, const char *path,
							  uint32 open_mode)
{
 	BEntry entry;
 	BPath p;

 	if (!dir || !path)
		return B_BAD_VALUE;

 	/* We'll let the SetTo do the error check. */
 	dir->GetEntry(&entry);
 	entry.GetPath(&p);
 	p.Append(path);

	return (SetTo(p.Path(), open_mode));
}
  
bool BFile::IsReadable() const
{
	if (fCStatus != B_NO_ERROR)
		return false;
	return !((fMode & O_RWMASK) == O_WRONLY); 
}

bool BFile::IsWritable() const
{
	if (fCStatus != B_NO_ERROR)
		return false;
	return !((fMode & O_RWMASK) == O_RDONLY);  
}

off_t	BFile::Position() const
{
	return _klseek_(fFd, 0, SEEK_CUR);
}

status_t	BFile::SetSize(off_t size)
{
	struct stat		st;

	st.st_size = size;
	return set_stat(st, WSTAT_SIZE);
}

off_t	BFile::Seek(off_t pos, uint32 seek_mode)
{
	return _klseek_(fFd, pos, seek_mode);
}

ssize_t	BFile::Read(void *buf, size_t numBytes)
{
	int res, err;

	res = _kread_(fFd, buf, numBytes, &err);

	if (res < 0)
		return err;
	else
		return res;
}

ssize_t	BFile::ReadAt(off_t pos, void *buf, size_t numBytes)
{
	int err;
	ssize_t res = _kread_pos_(fFd, pos, buf, numBytes, &err);

	if (res < 0)
		return err;
	else
		return res;
}

ssize_t	BFile::Write(const void *buf, size_t numBytes)
{
	int err = 0;
	size_t res = _kwrite_(fFd, buf, numBytes, &err);

	if (res == numBytes)
		return res;
	else
		return err;
}

ssize_t	BFile::WriteAt(off_t pos, const void *buf, size_t numBytes)
{
	int err = 0;
	size_t res = _kwrite_pos_(fFd, pos, buf, numBytes, &err);

	if (res == numBytes)
		return res;
	else
		return err;
}

ssize_t BFile::MetaWrite(	const char *in_name, type_code in_type,
							int32 in_index, off_t in_offset,
							const void *in_buf, size_t in_size)
{
	if (in_index != 0) return B_BAD_INDEX;
	return WriteAttr(in_name, in_type, in_offset, in_buf, in_size);
}

ssize_t BFile::MetaRead(	const char *in_name, type_code in_type,
							int32 in_index, off_t in_offset,
							void *out_buf, size_t in_size) const
{
	if (in_index != 0) return B_BAD_INDEX;
	return ReadAttr(in_name, in_type, in_offset, out_buf, in_size);
}

status_t BFile::MetaRemove(	const char *in_name, int32 in_index)
{
	if (in_index > 0) return B_BAD_INDEX;
	if (in_name) return RemoveAttr(in_name);
	else {
		RewindAttrs();
		char name[B_ATTR_NAME_LENGTH];
		while (GetNextAttrName(name) >= B_OK) {
			RemoveAttr(name);
			RewindAttrs();
		}
	}
	return B_OK;
}

status_t BFile::MetaGetInfo(const char *in_name, int32 in_index,
							meta_info *out_info, BString *out_name,
							void **inout_cookie) const
{
	status_t result = B_OK;
	
	attr_info info;
	info.type = B_ANY_TYPE;
	info.size = 0;
	
	if (in_name) {
		if (in_index > 0)
			return B_BAD_INDEX;
		
		attr_info info;
		result = GetAttrInfo(in_name, &info);
		if (out_name)
			*out_name = in_name;
	
	} else {
		if (!inout_cookie)
			return B_BAD_VALUE;
		
		char name[B_ATTR_NAME_LENGTH];
		
		if (*(int32*)inout_cookie != fAttrIndex) {
			// Need to re-scan attributes to find next item.
			const_cast<BFile*>(this)->RewindAttrs();
			for (int32 i=0; i<*(int32*)inout_cookie && result >= B_OK; i++) {
				result = const_cast<BFile*>(this)->GetNextAttrName(name);
			}
		}
		
		if (result >= B_OK) result = const_cast<BFile*>(this)->GetNextAttrName(name);
		if (result >= B_OK) {
			if (out_name)
				*out_name = name;
			++(*(int32*)inout_cookie);
			result = GetAttrInfo(name, &info);
		}
	}
	
	if (out_info) {
		out_info->type = info.type;
		out_info->count = 1;
		out_info->size = info.size;
	}
	
	return result;
}

BFile &	BFile::operator=(const BFile &file)
{
	status_t	err;
	int			fd;

	if (&file == this)
		return *this;

	if (file.fFd < 0) {
		err = B_BAD_VALUE;
		goto error1;
	}

	fd = _kopen_(file.fFd, NULL, file.fMode, 0, TRUE);
	if (fd < 0) {
		err = fd;
		goto error1;
	}

	set_fd(fd);
	fMode = file.fMode;
	return *this;

error1:
	Unset();
	fCStatus = err;
	return *this;

}

void		BFile::close_fd()
{
	fMode = 0;
	BNode::close_fd();
}


	void		BFile::_PhiloFile1() {}
	void		BFile::_PhiloFile2() {}
	void		BFile::_PhiloFile3() {}
	void		BFile::_PhiloFile4() {}
	void		BFile::_PhiloFile5() {}
	void		BFile::_PhiloFile6() {}
