#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <fs_attr.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <Looper.h>
#include <Node.h>
#include <Path.h>
#include <String.h>


#include "message_util.h"
#include "priv_syscalls.h"
#include "token.h"

extern const int	_omodes_[2] = { O_RDWR, O_RDONLY };

BNode::BNode()
{
	fFd = -1;
	fAttrFd = -1;
	fCStatus = B_NO_INIT;
	fAttrIndex = -1;
}

BNode::BNode(const entry_ref *ref)
{
	fFd = -1;
	fAttrFd = -1;
	SetTo(ref);
}

BNode::BNode(const BEntry *entry)
{
	fFd = -1;
	fAttrFd = -1;
	SetTo(entry);
}

BNode::BNode(const char *path)
{
	fFd = -1;
	fAttrFd = -1;
	SetTo(path);
}

BNode::BNode(const BDirectory *dir, const char *path)
{
	fFd = -1;
	fAttrFd = -1;
	SetTo(dir, path);
}

BNode::BNode(const BNode &node)
	:	BStatable()
{
	fFd = -1;
	fAttrFd = -1;
	fCStatus = B_NO_INIT;
	*this = node;
}

BNode::~BNode()
{
	set_fd(-1);
}

status_t BNode::InitCheck() const
{
  return fCStatus;
}

status_t	BNode::GetStat(struct stat *st) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;

	return _krstat_(fFd, NULL, st, FALSE);
}

status_t	BNode::SetTo(const entry_ref *ref)
{
	return set_to(ref);
}

status_t	BNode::set_to(const entry_ref *ref, bool traverse)
{
	status_t	err;
	int			fd = -1;
	uint32		i;
	int			omode;

	if (!ref) {
		err = B_BAD_VALUE;
		goto error1;
	}

	fAttrIndex = -1;
	
	omode = 0;
	if (!traverse)
		omode |= O_NOTRAVERSE;
	for(i=0; i < sizeof(_omodes_) / sizeof(int); i++) {
		fd = _kopen_vn_(ref->device, ref->directory, ref->name, _omodes_[i] | omode, TRUE);
		if (fd >= 0)
			break;
	}
	if (i == sizeof(_omodes_) / sizeof(int)) {
		err = fd;
		goto error1;
	}

	set_fd(fd);
	return 0;

error1:
	clear();
	fCStatus = err;
	return err;
}

status_t	BNode::SetTo(const BEntry *entry)
{
	return set_to(entry);
}

status_t	BNode::set_to(const BEntry *entry, bool traverse)
{
	status_t	err;
	int			fd = -1;
	uint32		i;
	int			omode;

	if (!entry) {
		err = B_BAD_VALUE;
		goto error1;
	}

	fAttrIndex = -1;
	
	omode = 0;
	if (!traverse)
		omode |= O_NOTRAVERSE;
	for(i=0; i < sizeof(_omodes_) / sizeof(int); i++) {
		fd = _kopen_(entry->fDfd, entry->fName, _omodes_[i] | omode, 0, TRUE);
		if (fd >= 0)
			break;
	}
	if (i == sizeof(_omodes_) / sizeof(int)) {
		err = fd;
		goto error1;
	}

	set_fd(fd);
	return 0;

error1:
	clear();
	fCStatus = err;
	return err;
}

status_t	BNode::SetTo(const char *path)
{
	return set_to(path);
}

status_t	BNode::set_to(const char *path, bool traverse)
{
	status_t	err;
	int			fd = -1;
	uint32		i;
	int			omode;

	if (!path) {
		err = B_BAD_VALUE;
		goto error1;
	}

	fAttrIndex = -1;
	
	omode = 0;
	if (!traverse)
		omode |= O_NOTRAVERSE;
	for(i=0; i < sizeof(_omodes_) / sizeof(int); i++) {
		fd = _kopen_(-1, path, _omodes_[i] | omode, 0, TRUE);
		if (fd >= 0)
			break;
	}
	if (i == sizeof(_omodes_) / sizeof(int)) {
		err = fd;
		goto error1;
	}

	set_fd(fd);
	return 0;

error1:
	clear();
	fCStatus = err;
	return err;
}

status_t BNode::SetTo(const BDirectory *dir, const char *path)
{
	return set_to(dir, path);
}

status_t BNode::set_to(const BDirectory *dir, const char *path, bool)
{
	BEntry	entry;
	BPath	p;

	if (!dir || !path) {
		clear();
		fCStatus = B_BAD_VALUE;
		return fCStatus;
	}

	dir->GetEntry(&entry);
	entry.GetPath(&p);
	p.Append(path);
	return SetTo(p.Path());
}

void BNode::Unset()
{
	clear_virtual();
}

status_t	BNode::Lock()
{
	return _klock_node_(fFd);
}

status_t	BNode::Unlock()
{
	return _kunlock_node_(fFd);
}


status_t	BNode::Sync()
{
	return fsync(fFd);
}

ssize_t	BNode::WriteAttr(const char *attribute, type_code type,
			off_t off, const void *buf, size_t l)
{
	fAttrIndex = -1;
	return _kwrite_attr_(fFd, attribute, type, off, buf, l);
}


ssize_t	BNode::ReadAttr(const char *attr, type_code type, off_t off,
			void *buf, size_t l) const
{
	return _kread_attr_(fFd, attr, type, off, buf, l);
}

status_t BNode::RemoveAttr(const char *attr)
{
	fAttrIndex = -1;
	return _kremove_attr_(fFd, attr);
}

status_t BNode::RenameAttr(const char *oldname, const char *newname)
{
	fAttrIndex = -1;
	return _krename_attr_(fFd, oldname, newname);
}

status_t BNode::GetAttrInfo(const char *attr, struct attr_info *buf) const
{
	return _kstat_attr_(fFd, attr, buf);
}

status_t BNode::WriteAttrString(const char *attr, const BString *string)
{
	fAttrIndex = -1;
	ssize_t result = _kwrite_attr_(fFd, attr, B_STRING_TYPE, 0, string->String(),
		string->Length() + 1);
	return result == string->Length() + 1 ? B_OK : (status_t)result;
}

status_t BNode::ReadAttrString(const char *attr, BString *result) const
{
	attr_info info;

	status_t error = _kstat_attr_(fFd, attr, &info);
	if (error != B_OK)
		return error;

	ssize_t size = _kread_attr_(fFd, attr, B_STRING_TYPE, 0, result->LockBuffer(info.size),
		info.size);
	result->UnlockBuffer(info.size - 1);
	
	return size == info.size ? B_OK : (status_t)size;
}

status_t	BNode::GetNextAttrName(char *buf)
{
	char			tmp[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
	struct dirent	*d;
	long			res;

	if (fAttrFd < 0) {
		fAttrFd = _kopen_attr_dir_(fFd, NULL, TRUE);
		if (fAttrFd < 0)
			return fAttrFd;
		fAttrIndex = 0;
	}
	d = (struct dirent *) tmp;
	res = _kread_attr_dir_(fAttrFd, d, sizeof(tmp), 1);
	switch (res) {
	case 1:
		strcpy(buf, d->d_name);
		fAttrIndex++;
		return B_NO_ERROR;
	case 0:
		return ENOENT;
	default:
		return res;
	}
}

status_t	BNode::RewindAttrs()
{
	if (fAttrFd < 0) {
		fAttrFd = _kopen_attr_dir_(fFd, NULL, TRUE);
		if (fAttrFd < 0)
			return fAttrFd;
	}
	status_t result = _krewind_attr_dir_(fAttrFd);
	if (result >= B_OK)
		fAttrIndex = 0;
	return result;
}

BNode &	BNode::operator=(const BNode &node)
{
	uint32		i;
	int			fd = -1;

	if (&node == this)
	  return *this;

	clear();

	if (node.fFd < 0) {
		fCStatus = B_BAD_VALUE;
		return *this;
	}

	for(i=0; i < sizeof(_omodes_) / sizeof(int); i++) {
		fd = _kopen_(node.fFd, NULL, _omodes_[i], 0, FALSE);
		if (fd >= 0)
			break;
	}
			
	if (i == sizeof(_omodes_) / sizeof(int)) {
		fCStatus = fd;
		return *this;
	}

	fCStatus = B_NO_ERROR;
	set_fd(fd);
	return *this;
}

bool	BNode::operator==(const BNode &node) const
{
	int				err;
	struct stat		sta, stb;

	if ((fFd < 0) && (node.fFd < 0))
		return TRUE;
	if ((fFd < 0) || (node.fFd < 0))
		return FALSE;
	err = _krstat_(fFd, NULL, &sta, FALSE);
	if (err)
		return FALSE;
	err = _krstat_(node.fFd, NULL, &stb, FALSE);
	if (err)
		return FALSE;
	return ((sta.st_dev == stb.st_dev) && (sta.st_ino == stb.st_ino));	
}

bool	BNode::operator!=(const BNode &node) const
{
	return !(*this == node);
}

int BNode::Dup()
{
	if (fFd < 0) {
		errno = EBADF;
		return -1;	/* probably unnecessary, but safe */
	}
	return dup(fFd);
}

status_t 	BNode::set_stat(struct stat &st, uint32 what)
{
	return _kwstat_(fFd, NULL, &st, what, FALSE);
}

status_t BNode::set_fd(int fd)
{
	if (fFd >= 0)
		_kclose_(fFd);
	if (fAttrFd >= 0)
		_kclose_attr_dir_(fAttrFd);
	fAttrFd = -1;
	fFd = fd;
	fCStatus = B_NO_ERROR;
	fAttrIndex = -1;
   	return fCStatus;
}

void BNode::close_fd()
{
	if (fFd >= 0) {
		_kclose_(fFd);
		if (fAttrFd >= 0)
			_kclose_attr_dir_(fAttrFd);
	}
}

status_t BNode::clear()
{
	BNode::close_fd();
	fAttrFd = -1;
	fFd = -1;
	fCStatus = B_NO_INIT;
	fAttrIndex = -1;
   	return fCStatus;
}

status_t BNode::clear_virtual()
{
	close_fd();
	fAttrFd = -1;
	fFd = -1;
	fCStatus = B_NO_INIT;
	fAttrIndex = -1;
   	return fCStatus;
}



	void		BNode::_RudeNode1() {}
	void		BNode::_RudeNode2() {}
	void		BNode::_RudeNode3() {}

	void		BNode::_RudeNode4() {}
	void		BNode::_RudeNode5() {}
	void		BNode::_RudeNode6() {}

