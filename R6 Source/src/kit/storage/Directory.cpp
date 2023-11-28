#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

#include <Debug.h>
#include <Directory.h>
#include <File.h>
#include <SymLink.h>
#include <Entry.h>
#include <Path.h>

#include "Storage.h"

#include "priv_syscalls.h"
#include "token.h"
#include "message_util.h"


BEntryList::BEntryList()
{
	// NOTE: Not added until Maui.
}

#if !_PR3_COMPATIBLE_

BEntryList::~BEntryList()
{
}

void 
BEntryList::_ReservedEntryList1()
{
}

void 
BEntryList::_ReservedEntryList2()
{
}

void 
BEntryList::_ReservedEntryList3()
{
}

void 
BEntryList::_ReservedEntryList4()
{
}

void 
BEntryList::_ReservedEntryList5()
{
}

void 
BEntryList::_ReservedEntryList6()
{
}

void 
BEntryList::_ReservedEntryList7()
{
}

void 
BEntryList::_ReservedEntryList8()
{
}

#else

BEntryList::~BEntryList()
{
	// NOTE: Not added until Maui.
}

#endif

static void clean_path(char *path)
{
	char *ptr = path;
	char *fptr;
	while ((ptr = strstr(ptr, "/./")) != 0) {
		fptr = ptr + 3;
		memmove(ptr+1, fptr, strlen(fptr)+1);
	}
	
	ptr = path;
	while ((ptr = strstr(ptr, "//")) != 0) {
		fptr = ptr + 2;
		while (*fptr == '/')
			fptr++;
		ptr++;
		memmove(ptr, fptr, strlen(fptr)+1);
	}
}

status_t create_directory(const char *path, mode_t mode)
{
  char *sptr, *stake;
  char *newpath;
  status_t err;
  int fd=-1, dfd=-1;
  bool abs=false, first=true;
  size_t len;
  
  if (!path || *path == '\0')
	return B_BAD_VALUE;

  len  = strlen(path);
  newpath = (char *)malloc(len+3);

  if (!newpath)
	return B_NO_MEMORY;

  memset(newpath, len+3, 0);

  /* Add slashes to make path checking easier. */
  abs = (path[0] == '/');

  if (!abs)
	sprintf(newpath, "/%s/", path);
  else
	sprintf(newpath, "%s/", path);

  if (strstr(newpath, "/../") != NULL) {
	err = B_BAD_VALUE;
	goto error1;
  }

  clean_path(newpath);
  fd = -1;
  dfd = -1;
  stake = newpath;

  /* Roll forward until we find a null entry. */
  while (1) {
	sptr = stake;

	if (*sptr == '/')
	  sptr++;

	if (*sptr == '\0')
	  goto exit;

	stake = strchr(sptr, '/');
	if (stake) {
	  *stake = '\0';
	}
	if (fd != -1) {
	  _kclosedir_(dfd);
	  dfd = fd;
	}

	/* Need to special case first time for abs case. */
	if (first && abs) {
	  --sptr;
	  first = false;
	}

	fd = _kopendir_(dfd, sptr, true);

	if (fd == ENOENT) {
	  if ((err = _kmkdir_(dfd, sptr, mode)) < B_NO_ERROR)
		goto error1;
	  fd = _kopendir_(dfd, sptr, true);
	}

	if (fd < 0) {
	  err = fd;
	  goto error1;
	}
	stake++;
  }

exit:
  err = B_NO_ERROR;

error1:
  if (fd >= 0)
	_kclosedir_(fd);

  if (dfd >= 0)
	_kclosedir_(dfd);

  free(newpath);
  return (err);
}  

BDirectory::BDirectory()
{
	fDirFd = -1;
}

BDirectory::BDirectory(const entry_ref *ref)
{
	fDirFd = -1;
	SetTo(ref);
}

BDirectory::BDirectory(const BEntry *entry)
{
	fDirFd = -1;
	SetTo(entry);
}

BDirectory::BDirectory(const char *path)
{
	fDirFd = -1;
	SetTo(path);
}

BDirectory::BDirectory(const BDirectory *dir, const char *path)
{
	fDirFd = -1;
	SetTo(dir, path);
}

BDirectory::BDirectory(const node_ref *ref)
{
	fDirFd = -1;
	SetTo(ref);
}


BDirectory::BDirectory(const BDirectory &dir)
	:	BNode(),
		BEntryList()
{
	fDirFd = -1;
	*this = dir;
}

BDirectory::~BDirectory()
{
	if (fDirFd >= 0)
		_kclosedir_(fDirFd);
}

status_t	BDirectory::SetTo(const entry_ref *ref)
{
	status_t		err;

	if (fDirFd >= 0)
		_kclosedir_(fDirFd);
	fDirFd = -1;
	err = set_to(ref, true);
	if (err)
		goto error1;
	fDirFd = _kopendir_(fFd, NULL, true);
	if (fDirFd < 0) {
		err = fDirFd;
		goto error2;
	}
	return 0;

error2:
	Unset();
error1:
	fCStatus = err;
	return err;
}

status_t	BDirectory::SetTo(const BEntry *entry)
{
	status_t		err;

	if (fDirFd >= 0)
		_kclosedir_(fDirFd);
	fDirFd = -1;
	err = set_to(entry, true);
	if (err)
		goto error1;
	fDirFd = _kopendir_(fFd, NULL, true);
	if (fDirFd < 0) {
		err = fDirFd;
		goto error2;
	}
	return 0;

error2:
	Unset();
error1:
	fCStatus = err;
	return err;
}

status_t	BDirectory::SetTo(const char *path)
{
	status_t		err;

	if (fDirFd >= 0)
		_kclosedir_(fDirFd);
	fDirFd = -1;
	err = set_to(path, true);
	if (err)
		goto error1;
	fDirFd = _kopendir_(fFd, NULL, true);
	if (fDirFd < 0) {
		err = fDirFd;
		goto error2;
	}
	return 0;

error2:
	Unset();
error1:
	fCStatus = err;
	return err;
}

status_t BDirectory::SetTo(const BDirectory *dir, const char *path)
{
	status_t err;
	if (dir == this) {
		// Handle the case where the user passes the directory
		// into itself as the first parameter.  set_to relies on
		// fDirFD being set, so don't close it until after it is
		// called.
		int oldDirFD = fDirFd;
		err = set_to(dir, path, true);
		if (oldDirFD >= 0)
			_kclosedir_(oldDirFD);
	} else {
		if (fDirFd >= 0)
			_kclosedir_(fDirFd);
		fDirFd = -1;
		err = set_to(dir, path, true);
	}

	if (err)
		goto error1;

	fDirFd = _kopendir_(fFd, NULL, true);
	if (fDirFd < 0) {
		err = fDirFd;
		goto error2;
	}
	
	return 0;

error2:
	Unset();
error1:
	fCStatus = err;
	return err;
}


status_t	BDirectory::SetTo(const node_ref *node)
{
	status_t	err;
	int			fd = -1;
	uint32		i;

	for(i=0; i < sizeof(_omodes_) / sizeof(int); i++) {
		fd = _kopen_vn_(node->device, node->node, NULL, _omodes_[i], true);
		if (fd >= 0)
			break;
	}

	if (i == sizeof(_omodes_) / sizeof(int)) {
		err = fd;
		goto error1;
	}

	err = set_fd(fd);
	if (err)
		goto error2;
	return err;

error2:
	_kclose_(fd);
error1:
	Unset();
	fCStatus = err;
	return err;
}

status_t		BDirectory::GetEntry(BEntry *entry) const
{
	int				err;
	int				fd;
	dev_t			dev, pdev;
	ino_t			ino;
	struct stat		st;
	struct dirent	*d;
	struct dirent	ents[1024 / sizeof(struct dirent)];
	long			i, count;
	
	entry->clear();

	if (fFd < 0) {
		err = B_NO_INIT;
		goto error1;
	}

	fd = _kopendir_(fDirFd, _dotdot_, true);
	if (fd < 0) {
		err = fd;
		goto error1;
	}

	err = _krstat_(fDirFd, NULL, &st, true);
	if (err)
		goto error2;
	ino = st.st_ino;
	dev = st.st_dev;
	
	err = _krstat_(fd, NULL, &st, true);
	if (err)
		goto error2;

	/*
	we are at the root. create a special entry.
	*/

	if ((st.st_ino == ino) && (st.st_dev == dev)) {
		err = entry->set(fd, _dot_, false);
		_kclosedir_(fd);
		return err;
	}

	pdev = st.st_dev;
	do {
		count = _kreaddir_(fd, ents, sizeof ents, sizeof ents/sizeof *ents);
		if (count < 0) {
			err = count;
			goto error2;
		}
		d = ents;
		for(i=0; i<count; i++, d = (dirent*)((char*)d+d->d_reclen)) {
			if (d->d_ino == ino) {
				if (pdev == dev)
					goto exit;
				else {
					err = _krstat_(fd, d->d_name, &st, false);
					if (err)
						goto error2;
					if (st.st_dev == dev)
						goto exit;
				}
			}
		}
	} while (count > 0);

exit:
	if (count == 0) {
		err = ENOENT;
		goto error2;
	}
	
	err = entry->set(fd, d->d_name, false);
	_kclosedir_(fd);
	return err;

error2:
	_kclosedir_(fd);
error1:
	return err;
}

bool		BDirectory::IsRootDirectory() const
{
	int				err;
	fs_info			info;
	struct stat		st;

	err = GetStat(&st);
	if (err)
		return false;
	err = _kstatfs_(st.st_dev, NULL, -1, NULL, &info);
	if (err)
		return false;
	return (info.root == st.st_ino);
}

status_t		BDirectory::FindEntry(const char *path, BEntry *entry,
								  bool traverse) const
{
	status_t		err;

	err = entry->set(fDirFd, path, traverse);

	if (err)
		return err;

	if (entry->Exists())
		return 0;

	entry->clear();
	return ENOENT;
}

bool		BDirectory::Contains(const char *path, int32 nf) const
{
	struct stat	st;

	if (_krstat_(fDirFd, path, &st, false) != 0)
		return false;

	if (nf == B_ANY_NODE)
		return true;
	if ((nf & B_FILE_NODE) && S_ISREG(st.st_mode))
		return true;
	if ((nf & B_SYMLINK_NODE) && S_ISLNK(st.st_mode))
		return true;
	if ((nf & B_DIRECTORY_NODE) && S_ISDIR(st.st_mode))
		return true;

	return false;
}

bool		BDirectory::Contains(const BEntry *entry, int32 nf) const
{
	struct stat		sta, stb;
	dev_t			pdev;
	ino_t			pino;
	int				fd, nfd;

	if (GetStat(&sta))
		goto error1;
	fd = _kopendir_(entry->fDfd, NULL, true);

	if (fd < 0)
		goto error1;
	
	pdev = (dev_t) -1;
	pino = (dev_t) -1;

	while (true) {
		if (_krstat_(fd, NULL, &stb, false))
			goto error2;

		if ((sta.st_dev == stb.st_dev) && (sta.st_ino == stb.st_ino)) {
			_kclosedir_(fd);
			goto modecheck;
		}

		if ((stb.st_dev == pdev) && (stb.st_ino == pino))
			break;
		pdev = stb.st_dev;
		pino = stb.st_ino;
		nfd = _kopendir_(fd, _dotdot_, true);
		if (nfd < 0)
			goto error2;
		_kclosedir_(fd);
		fd = nfd;
	}

	_kclosedir_(fd);
	return false;

error2:
	_kclosedir_(fd);
error1:
	return false;

modecheck:

	if (nf == B_ANY_NODE)
		return true;

	if ((nf & B_FILE_NODE) && S_ISREG(stb.st_mode))
		return true;
	if ((nf & B_SYMLINK_NODE) && S_ISLNK(stb.st_mode))
		return true;
	if ((nf & B_DIRECTORY_NODE) && S_ISDIR(stb.st_mode))
		return true;
	
	return false;
}

status_t	BDirectory::GetStatFor(const char *path, struct stat *st) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;

	return _krstat_(fDirFd, path, st, false);
}

long		BDirectory::GetNextEntry(BEntry *entry, bool traverse)
{
	long			count;
	char			tmp[sizeof(struct dirent) + 256];
	struct dirent	*d = (struct dirent *) &tmp;
	status_t		res;

	do {
		do {
			count = _kreaddir_(fDirFd, d, sizeof(tmp), 1);
			if (count < 0) {
				entry->clear();
				return count;
			}

			if (count == 0) {
				entry->clear();
				return ENOENT;
			}

		} while (!strcmp(d->d_name, _dot_) || !strcmp(d->d_name, _dotdot_));
		
		res = entry->set(fDirFd, d->d_name, traverse);
	} while (res == ENOENT);
	
	return res;
}

long		BDirectory::GetNextRef(entry_ref *ref)
{
  	status_t		err;
	long			count;
	struct stat		st;
	size_t			l;
	char			*p;
	char			tmp[sizeof(struct dirent) + 256];
	struct dirent	*d = (struct dirent *) &tmp;

	if (fFd < 0) {
		err = B_NO_INIT;
		goto clear;
	}
	
	err = _krstat_(fDirFd, NULL, &st, true);
	if (err) {
	  goto clear;
	}

	do {
		count = _kreaddir_(fDirFd, d, sizeof(tmp), 1);
		if (count < 0) {
		  err = count;
		  goto clear;
		}
		if (count == 0) {
		  err = ENOENT;
		  goto clear;
		}
	} while (!strcmp(d->d_name, _dot_) || !strcmp(d->d_name, _dotdot_));

	l = strlen(d->d_name) + 1;
	p = (char *) realloc(ref->name, l);
	if (!p) {
	  err = ENOMEM;
	  goto clear;
	}

	memcpy(p, d->d_name, l);
	ref->name = p;
	ref->device = st.st_dev;
	ref->directory = st.st_ino;
	return 0;

clear:
	ref->device = (dev_t) -1;
	ref->directory = (ino_t) -1;
	ref->set_name(NULL);
	return err;
}

int32		BDirectory::GetNextDirents(struct dirent *buf, size_t length,
				int32 count)
{
	return _kreaddir_(fDirFd, buf, length, count);
}

long		BDirectory::Rewind()
{
	return _krewinddir_(fDirFd);	
}

int32		BDirectory::CountEntries()
{
	int			err;
	char		buf[2048];
	dirent		*d, *ents;
	long		i, n, cnt;

	err = _krewinddir_(fDirFd);	
	if (err)
		goto error1;
	cnt = 0;
	do {
		ents = (dirent *) buf;
		n = _kreaddir_(fDirFd, ents, sizeof buf, sizeof buf / sizeof(dirent));
		if (n < 0) {
			err = n;
			goto error1;
		}
		for(i=0, d=ents; i<n; i++, d = (dirent *)((char*)d+d->d_reclen))
			if (strcmp(d->d_name, _dotdot_) && strcmp(d->d_name, _dot_))
				cnt++;
	} while (n > 0);
	err = _krewinddir_(fDirFd);	
	if (err)
		goto error1;

	return cnt;

error1:
	return err;
}


long		BDirectory::CreateDirectory(const char *path, BDirectory *dir)
{
	int		err;
	BEntry	entry;

	err = _kmkdir_(fDirFd, path, DEFAULT_DIR_MODE_BITS);
	if (err)
		goto error1;
	
	err = FindEntry(path, &entry);
	if (err)
		goto error2;

	if (dir) {
		err = dir->SetTo(&entry);
		if (err)
			goto error2;
	}

	return B_OK;
	
error2:
	_krmdir_(fDirFd, path);
error1:
	if (dir)
		dir->Unset();
	return err;
}

long		BDirectory::CreateFile(const char *path, BFile *file,
								bool failIfExists)
{
	int		err;
	int		fd;
	BEntry	entry;
	uint32 omode = O_CREAT | O_TRUNC | O_RDWR;
	if (failIfExists)
		omode |= O_EXCL;

	fd = _kopen_(fDirFd, path, omode,
			DEFAULT_MODE_BITS, true);
	if (fd < 0) {
		err = fd;
		goto error1;
	}
	_kclose_(fd);

	err = FindEntry(path, &entry);
	if (err)
		goto error2;
	if (file) {
		err = file->SetTo(&entry, O_RDWR);
		if (err)
			goto error2;
	}

	return 0;
	
error2:
	_kunlink_(fDirFd, path);
error1:
	if (file)
		file->Unset();
	return err;
}

long		BDirectory::CreateSymLink(const char *path, const char *content,
					BSymLink *link)
{
	int		err;
	BEntry	entry;

	err = _ksymlink_(content, fDirFd, path);
	if (err)
		goto error1;

	err = FindEntry(path, &entry);
	if (err)
		goto error2;
	if (link) {
		err = link->SetTo(&entry);
		if (err)
			goto error2;
	}

	return 0;
	
error2:
	_kunlink_(fDirFd, path);
error1:
	if (link)
		link->Unset();
	return err;
}

BDirectory &	BDirectory::operator=(const BDirectory &dir)
{
	status_t	err;
	int			fd = -1;
	uint32		i;

	if (&dir == this)
		return *this;

	Unset();

	if (dir.fDirFd < 0) {
		err = B_BAD_VALUE;
		goto error1;
	}

	for(i=0; i < sizeof(_omodes_) / sizeof(int); i++) {
		fd = _kopen_(dir.fDirFd, NULL, _omodes_[i], 0, true);
		if (fd >= 0)
			break;
	}

	if (i == sizeof(_omodes_) / sizeof(int)) {
		err = fd;
		goto error1;
	}

	err = set_fd(fd);
	if (err)
		goto error2;

	return *this;

error2:
	_kclose_(fd);
error1:
	fCStatus = err;
	return *this;
}

void	BDirectory::close_fd()
{
	_kclosedir_(fDirFd);
	fDirFd = -1;
	BNode::close_fd();
}

status_t	BDirectory::set_fd(int fd)
{
	status_t	err;

	if (fDirFd >= 0)
		_kclosedir_(fDirFd);

	fDirFd = _kopendir_(fd, NULL, true);
	if (fDirFd < 0) {
		err = fDirFd;
		goto error1;
	}

	BNode::set_fd(fd);
	return 0;

error1:
	Unset();
	fCStatus = err;
	return err;
}

void		BDirectory::_ErectorDirectory1() {}
void		BDirectory::_ErectorDirectory2() {}
void		BDirectory::_ErectorDirectory3() {}

void		BDirectory::_ErectorDirectory4() {}
void		BDirectory::_ErectorDirectory5() {}
void		BDirectory::_ErectorDirectory6() {}

