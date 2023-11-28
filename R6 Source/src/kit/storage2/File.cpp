
#include <fs_attr.h>
#include <fcntl.h>
#include <stdio.h>

#include <support2/Debug.h>
#include <support2/String.h>
#include <support2/TypeConstants.h>
#include <storage2/File.h>
#include <storage2/Directory.h>
#include <storage2/Entry.h>
#include <storage2/Path.h>

#include "Storage.h"
#include "priv_syscalls.h"

namespace B {
namespace Storage2 {

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
		IStorage()
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
				 DEFAULT_MODE_BITS, true);
	if (fd < 0) {
		if((open_mode & O_RWMASK) == O_RDWR){
			open_mode = (open_mode & ~O_RWMASK) | O_RDONLY;
			fd = _kopen_(entry->fDfd, entry->fName, open_mode,
						 DEFAULT_MODE_BITS, true);
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

off_t 
BFile::Size() const
{
	const off_t cur = _klseek_(fFd, 0, SEEK_END);
	return cur >= 0 ? _klseek_(fFd, cur, SEEK_SET) : cur;
}

status_t 
BFile::SetSize(off_t size)
{
	struct stat st;
	st.st_size = size;
	return set_stat(st, WSTAT_SIZE);
}

ssize_t 
BFile::ReadAtV(off_t position, const struct iovec *vector, ssize_t count)
{
	int err;
	ssize_t res = _kreadv_pos_(fFd, position, vector, count, &err);

	if (res < 0)
		return err;
	else
		return res;
}

ssize_t 
BFile::WriteAtV(off_t position, const struct iovec *vector, ssize_t count)
{
	int err = 0;
	size_t res = _kwritev_pos_(fFd, position, vector, count, &err);

	if (err)
		return err;
	else
		return res;
}

status_t
BFile::Sync()
{
	return BNode::Sync();
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

	fd = _kopen_(file.fFd, NULL, file.fMode, 0, true);
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

} }	// namespace B::Storage2
