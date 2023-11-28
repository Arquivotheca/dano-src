#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include <Debug.h>
#include <Messenger.h>
#include <Handler.h>
#include <Looper.h>
#include <Entry.h>
#include <Directory.h>
#include <Path.h>
#include <NodeMonitor.h>

#include "Storage.h"
#include "priv_syscalls.h"
#include "token.h"
#include "message_util.h"

#define     PREFERRED_TOKEN (0xFFFFFFFE)

/* I'm guessing */
status_t 	watch_node(const node_ref *node, uint32 flags,
					   const BHandler *handler, const BLooper *looper)
{
	return watch_node(node, flags, BMessenger(handler, looper));
}

status_t	watch_node(const node_ref *node, uint32 flags, 
					   BMessenger target)
{
	status_t err1=0, err2=0;

	if (flags == 0 && !node)
	  return B_BAD_VALUE;

	if (!target.IsValid())
		return B_BAD_VALUE;
	
	const port_id port = _get_messenger_port_(target);
	const int32 token = _get_messenger_preferred_(target)
						? PREFERRED_TOKEN : _get_messenger_token_(target);
	if (port < B_OK)
		return port;

	/* First check for stop watching; we know there's a node */
	if (flags == B_STOP_WATCHING) {
	  return (_kstop_watching_vnode_(node->device, node->node, 
								 port, token));
	}

	/* Check for volume watching--don't need a node. */
	if (flags & B_WATCH_MOUNT) {
		err1 = _kstart_watching_vnode_(-1, -1, 0, port, token);
	  flags = flags & ~(B_WATCH_MOUNT);
	}

	/* Now everything else. */
	if (flags) {
	  if (!node) 
		return B_ERROR;
	  err2 = _kstart_watching_vnode_(node->device, node->node, 
									 flags, port, token);
	}

	return (err2)?err2:err1;
}

status_t 	stop_watching(const BHandler *handler, const BLooper *looper)
{
	return stop_watching(BMessenger(handler, looper));
}

status_t	stop_watching(BMessenger target)
{
	if (!target.IsValid())
		return B_BAD_VALUE;
	
	const port_id port = _get_messenger_port_(target);
	const int32 token = _get_messenger_preferred_(target)
						? PREFERRED_TOKEN : _get_messenger_token_(target);
	if (port < B_OK)
		return port;

	return _kstop_notifying_(port, token);
}

BEntry::BEntry()
{
	fName = NULL;
	fDfd = -1;
	fCStatus = B_NO_INIT;
}


BEntry::BEntry(const BDirectory *dir, const char *path, bool traverse)
{
	fDfd = -1;
	fName = NULL;
	SetTo(dir, path, traverse);
}

BEntry::BEntry(const entry_ref *ref, bool traverse)
{
	fDfd = -1;
	fName = NULL;
	SetTo(ref, traverse);
}

BEntry::BEntry(const char *path, bool traverse)
{
	fDfd = -1;
	fName = NULL;
	SetTo(path, traverse);
}

BEntry::BEntry(const BEntry &entry)
	:	BStatable()
{
	fDfd = -1;
	fName = NULL;
	*this = entry;
}

BEntry::~BEntry()
{
	if (fDfd >= 0)
		clear();
}

status_t BEntry::InitCheck() const
{
	return fCStatus;
}

bool	BEntry::Exists() const
{
	if (fCStatus != B_OK)
		return false;
		
 	struct stat	st;
 	return _krstat_(fDfd, fName, &st, false) == 0;
}

status_t	BEntry::GetStat(struct stat *st) const
{
	if (fCStatus != B_OK)
		return B_NO_INIT;

	return _krstat_(fDfd, fName, st, false);
}

status_t	BEntry::SetTo(const BDirectory *dir, const char *path, bool traverse)
{
  if (!path)
	return set(dir->fFd, ".", traverse);
  else
	return set(dir->fFd, path, traverse);
}

status_t	BEntry::SetTo(const entry_ref *ref, bool traverse)
{
	int		fd;
	int 	err;

	if (!ref || !ref->name) {
	  err = B_BAD_VALUE;
	  goto error1;
	}

	fd = _kopen_vn_(ref->device, ref->directory, NULL, O_RDONLY, TRUE);
	if (fd < 0) {
		err = fd;
		goto error1;
	}

	err = set(fd, ref->name, traverse);
	if (err != B_NO_ERROR)
		goto error2;

	_kclose_(fd);
	return 0;

error2:
	_kclose_(fd);
error1:
	clear();
	fCStatus = err;
	return fCStatus;
}

status_t	BEntry::SetTo(const char *path, bool traverse)
{
	return set(-1, path, traverse);
}

void  BEntry::Unset()
{
	clear();
}

status_t	BEntry::GetRef(entry_ref *ref) const
{
	int				err;
	struct stat		st;
	char 			*tmp;

	if (fCStatus != B_NO_ERROR) {
	  err = B_NO_INIT;
	  goto error1;
	}

	err = _krstat_(fDfd, NULL, &st, FALSE);

	if (err) {
	  goto error1;
	}

	tmp = strdup(fName);

	if (ref->name) {
	  free(ref->name);
	  ref->name = NULL;
	}

	if (!tmp) {
	  err = B_NO_MEMORY;
	  goto error1;
	}

	ref->name = tmp;
	ref->device = st.st_dev;
	ref->directory = st.st_ino;

	return B_OK;

error1:
	ref->device = (dev_t) -1;
	ref->directory = (ino_t) -1;
	ref->set_name(NULL);
	return err;
}

status_t	BEntry::GetPath(BPath *path) const
{
	if (fCStatus != B_NO_ERROR)
		return B_NO_INIT;

	/* special treatment of root: the returned path is "/", not "/." */
	if (!strcmp(fName, _dot_))
		return path->SetTo("/");

	// We will fill the path starting at the end of the buffer
	path->Unset();
	char thePath[B_PATH_NAME_LENGTH];
	char *p = thePath + B_PATH_NAME_LENGTH;
	*(--p) = 0;		// Ends with \0
	
	size_t l;
	struct stat	st;
	struct stat	st_next;	// This is the stat structure of the next ".."
	status_t result;

	// Add the entry's name at the end of the path
	p -= (l=strlen(fName)+1);
	if (p < thePath)
		return B_NO_MEMORY;
	*p = '/';
	memcpy(p+1, fName, l-1);
	
	// Then go down in the tree from the leaf to the root
	// record the parent's name at each step in order to
	// build the complete path of the entry

	// We stat the entry's parent to get its dev_t and ino_t
	int fd = fDfd;
	if ((result = _krstat_(fd, _dot_, &st, false)) != B_OK)
		return result;
	dev_t dev = st.st_dev;
	ino_t ino = st.st_ino;

	// We open the entry's grand parent (the parent of the parent)
	int pfd;
	if ((pfd = _kopendir_(fd, _dotdot_, true)) < 0)
		return pfd;

	// and we stat() it
	if ((result = _krstat_(pfd, _dot_, &st, false)) != B_OK)
	{
		_kclosedir_(pfd);
		return result;
	}

	// now, we will go through the directory and look for our entry's parent name
	while (true)
	{
		if ((st.st_ino == ino) && (st.st_dev == dev))
		{ // We are at the root, so just exit
			_kclosedir_(pfd);
			break;
		}

		// pfd is the parent dir
		dev_t pdev = st.st_dev;
		long count;
		struct dirent ents[4096 / sizeof(struct dirent)];	// We use a 4K block to store the directory content
		struct dirent *dent = NULL;
		char found_dent[MAXNAMLEN];
		bool have_dotdot = false;
		bool have_ent = false;
		do
		{
			// We found the next ".." (so we won't need to stat it) and the child. Ok, next level.
			if (have_dotdot && have_ent)
				goto found;

			// Here we read the directory. As many entry as possible that can fit in our 4K buffer
			// NOTE: The last parameter is unrelevant. It must be >= sizeof(ents)/sizeof(*ents)
			count = _kreaddir_(pfd, ents, sizeof(ents), sizeof(ents));
			if (count <= 0)
				break;			

			dent = ents;
			for (int i=0; i<count; i++, dent = (dirent *)((char *)dent + dent->d_reclen))
			{
				// We found the next ".." (so we won't need to stat it) and the child. Ok, next level.
				if (have_dotdot && have_ent)
					goto found;
			
				if ((!have_dotdot) && (!strcmp(dent->d_name, _dotdot_)))
				{
					// here we found the parent (..). Store it's stat infos
					st_next.st_ino = dent->d_ino;
					st_next.st_dev = dent->d_dev;
					have_dotdot = true;
				}
			
				if ((!have_ent) && (dent->d_ino == ino))	
				{
					if (pdev == dev)
					{ // same dev_t. all right. We have our child.
						strcpy(found_dent, dent->d_name);
						have_ent = true;
					}
					else
					{
						// mathias: I really don't know what this code is for (!)
						// I guess it is a workaround for some buggy filesystems (?)
						// In any case, it can't hurt.
						struct stat	st;
						if ((result = _krstat_(pfd, dent->d_name, &st, false)) != B_OK)
						{
							_kclosedir_(pfd);
							return result;
						}
						if (st.st_dev == dev)
						{
							strcpy(found_dent, dent->d_name);
							have_ent = true;
						}
					}
				}
			}
		} while (count > 0);

		// We didn't find the child or the parent (gasp!). Return an error.
		_kclosedir_(pfd);
		return B_ENTRY_NOT_FOUND;
		
	found:
		// Add the name of the child to the path (backward)
		p -= (l=strlen(found_dent)+1);
		if (p < thePath)
		{
			_kclosedir_(pfd);
			return B_NO_MEMORY;
		}
		*p = '/';
		memcpy(p+1, found_dent, l-1);

		// And go to the next level...
		dev = st.st_dev;
		ino = st.st_ino;
		fd = pfd;
		if ((pfd = _kopendir_(fd, _dotdot_, true)) < 0)
		{
			_kclosedir_(fd);
			return pfd;
		}
		_kclosedir_(fd);

		// Here, we don't need to stat() the parent, since we alread have to info.
		st = st_next;
	}

	// Return the result. eventually.
	return path->SetTo(p);
}


status_t BEntry::GetParent(BEntry *entry) const
{
	BDirectory dir;
	status_t err;

	err = GetParent(&dir);
	if (err == B_NO_ERROR) 
		err = dir.FindEntry(".", entry);
	else
	  entry->Unset();
	return err;
}

status_t	BEntry::GetParent(BDirectory *dir) const
{
	status_t	err;
	int			fd = -1;
	uint32		i;

	dir->Unset();

  	if (fCStatus != B_NO_ERROR) {
		err =  B_NO_INIT;
		goto error1;
	}

	/* Get out if we're at the top. */
	if (strcmp(fName, _dot_) == 0) {
	  err = ENOENT;
	  goto error1;
	}

	for(i=0; i < sizeof(_omodes_) / sizeof(int); i++) {
		fd = _kopen_(fDfd, NULL, _omodes_[i], 0, TRUE);
		if (fd >= 0)
			break;
	}

	if (i == sizeof(_omodes_) / sizeof(int)) {
		err = fd;
		goto error1;
	}

	err = dir->set_fd(fd);
	if (err)
		goto error2;
	return B_NO_ERROR;

error2:
	_kclose_(fd);
error1:
	return err;
}

status_t	BEntry::GetName(char *buffer) const
{
  if (buffer)
	*buffer = '\0';

  if (fCStatus != B_NO_ERROR)
	return B_NO_INIT;

  strcpy(buffer, fName);
  return 0;
}

status_t	BEntry::Rename(const char *path, bool clobber)
{
	struct stat	st;

	if (fCStatus != B_NO_ERROR)
	  return B_NO_INIT;

	if (!clobber)
	  if (_krstat_(fDfd, path, &st, FALSE) == 0)
		return EEXIST;
	return move(fDfd, path);
}

status_t	BEntry::MoveTo(BDirectory *dir, const char *path, bool clobber)
{
	struct stat st;
	const char *ptr;

	if (fCStatus != B_NO_ERROR)
	  return B_NO_INIT;

	if (!path)
	  ptr = fName;
	else
	  ptr = path;

	if (!clobber)
	  if (_krstat_(dir->fFd, ptr, &st, FALSE) == 0) {
		return EEXIST;
	  }
	return move(dir->fFd, path);
}

status_t	BEntry::Remove()
{
  	if (fCStatus != B_NO_ERROR)
		return B_NO_INIT;
	if (IsDirectory())
		return _krmdir_(fDfd, fName);
	else
		return _kunlink_(fDfd, fName);
}

bool	BEntry::operator==(const BEntry &entry) const
{
	int				err;
	struct stat		sta, stb;

	if (!fName && !entry.fName)
		return TRUE;
	if (!fName || !entry.fName)
		return FALSE;
	err = _krstat_(fDfd, _dot_, &sta, FALSE);
	if (err)
		return FALSE;
	err = _krstat_(entry.fDfd, _dot_, &stb, FALSE);
	if (err)
		return FALSE;
	return ((sta.st_dev == stb.st_dev) &&
			(sta.st_ino == stb.st_ino) &&
			!strcmp(fName, entry.fName));
}

bool	BEntry::operator!=(const BEntry &entry) const
{
	return !(*this == entry);
}

BEntry &BEntry::operator=(const BEntry &entry)
{
	char		*name;
	size_t		l;
	int			fd;

	if (&entry == this)
	  return *this;

	clear();

	if (!entry.fName) {
 	  fCStatus = B_BAD_VALUE;
	  return *this;
	}

	l = strlen(entry.fName) + 1;
	name = (char *) malloc(l);

	if (!name) {
	  fCStatus = B_NO_MEMORY;
	  return *this;
	}
	memcpy(name, entry.fName, l);

	fd = _kopen_(entry.fDfd, NULL, O_NOTRAVERSE | O_RDONLY, 0, FALSE);

	if (fd < 0) {
		free(name);
		fCStatus = fd;
		return *this;
	}

	fName = name;
	fDfd = fd;
	fCStatus = B_NO_ERROR;

	return *this;
}


status_t	BEntry::set_stat(struct stat &st, uint32 what)
{
	return _kwstat_(fDfd, fName, &st, what, FALSE);
}

status_t	BEntry::move(int dfd, const char *path)
{
	int				err;
	int				ndfd;
	int				l;
	char			name[B_FILE_NAME_LENGTH];
	char			*p;

	if (!fName || fDfd < 0) {
	  err = B_NO_INIT;
	  goto error1;
	}

	if (dfd < 0) {
	  err = B_BAD_VALUE;
	  goto error1;
	}

	if (!path)
		path = fName;

	err = _parse_path_(dfd, path, NULL, &ndfd, name);
	if (err)
		goto error1;

	l = strlen(name) + 1;
	p = (char *) malloc(l);
	if (!p) {
		err = ENOMEM;
		goto error2;
	}
	memcpy(p, name, l);

	err = _krename_(fDfd, fName, ndfd, p);
	if (err)
		goto error3;

	_kclose_(fDfd);
	fDfd = ndfd;
	free(fName);
	fName = p;

	return 0;

error3:
	free(p);
error2:
	_kclose_(ndfd);
error1:
	return err;
}

status_t		BEntry::set(int fd, const char *path, bool traverse)
{
  int				err;
  char			*p;
  int				dfd;
  char			name[B_FILE_NAME_LENGTH];

  /* Closes fDfd, frees fName, and sets fCStatus to B_NO_INIT */
  clear();

  if (!path) 
	return (fCStatus=B_BAD_VALUE);

  err = _parse_path_(fd, path, NULL, &dfd, name);

  if (err) 
	return (fCStatus=err);

  /* After this point, must go to error2 */

  if (traverse) {
	struct stat		st;
	char			buf[MAXPATHLEN];
	int				ndfd;
	int				iter;
	ssize_t			sz;

	err = _krstat_(dfd, name, &st, FALSE);
	switch(err) {
	case 0:
	  break;
	case ENOENT:
	  goto exit;
	default:
	  goto error2;
	}
		
	iter = 0;
	while (S_ISLNK(st.st_mode)) {
	  if (iter++ == SYMLINKS_MAX) {
		err = ELOOP;
		goto error2;
	  }
	  sz = _kreadlink_(dfd, name, buf, sizeof buf);		
	  if (sz < 0) {
		err = sz;
		goto error2;
	  }
	  if (sz > (ssize_t)sizeof(buf)) {
		err = ENAMETOOLONG;
		goto error2;
	  }
	  buf[sz] = '\0';
	  err = _parse_path_(dfd, buf, NULL, &ndfd, name);
	  if (err)
		goto error2;
	  _kclose_(dfd);
	  dfd = ndfd;

	  err = _krstat_(dfd, name, &st, FALSE);
	  if (err)
		goto error2;
	}
  }

exit:
  p = strdup(name);
  if (!p) {
	err = B_NO_MEMORY;
	goto error2;
  }

  fDfd = dfd;
  fName = p;
  return (fCStatus=B_NO_ERROR);

error2:
  _kclose_(dfd);

  return (fCStatus=err);
}

status_t BEntry::clear()
{
  if (fName) {
	free(fName);
	fName = NULL;
  }	
  if (fDfd >= 0) {
	_kclose_(fDfd);
	fDfd = -1;
  }
  return (fCStatus = B_NO_INIT);
}


void		BEntry::_PennyEntry1() {}
void		BEntry::_PennyEntry2() {}
void		BEntry::_PennyEntry3() {}
void		BEntry::_PennyEntry4() {}
void		BEntry::_PennyEntry5() {}
void		BEntry::_PennyEntry6() {}
