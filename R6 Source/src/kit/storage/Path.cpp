#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>

#include <Debug.h>
#include <OS.h> 
#include <Entry.h>
#include <Directory.h>
#include <Path.h>

#include "Storage.h"
#include "priv_syscalls.h"


/*
 * check_path. makes sure that:
 * - there is no '.' and '..' components
 * - only one slash at a time (eg. "/foo//fred" forbidden)
 * - no '/' at the end (unless the path is "/")
 */

static int
check_path(const char *path)
{
	const char	*p;
	bool		slash;

	p = path;
	slash = false;
	while (*p) {
		if ((p == path) || (p[0] == '/')) {
			if (p[1]) {
				if (p[1] == '.')
					if (p[2]) {
						if ((p[2] == '.') && (!p[3] || (p[3] == '/')))
							return -1;
						if (p[2] == '/')
							return -1;
					} else
						return -1;
			} else {
				if (p != path)
					return -1;
			}
		}
		if (*p == '/') {
			if (slash)
				return -1;
			slash = true;
		} else
			slash = false;
		p++;
	}
	return 0;
}

BPath::BPath()
{
  fCStatus = B_NO_INIT;
  fName = NULL;
}
	
BPath::BPath(const char *dir, const char *leaf, bool normalize)
{
  fCStatus = B_NO_INIT;
  fName = NULL;
  SetTo(dir, leaf, normalize);
}

BPath::BPath(const BDirectory *dir, const char *leaf, bool normalize)
{
  fCStatus = B_NO_INIT;
  fName = NULL;
  SetTo(dir, leaf, normalize);
}

BPath::BPath(const BPath &path)
	:	BFlattenable()
{
  fCStatus = B_NO_INIT;
  fName = NULL;
  *this = path;
}

BPath::BPath(const BEntry *entry)
{
  fCStatus = B_NO_INIT;
  fName = NULL;
  SetTo(entry);
}


BPath::BPath(const entry_ref *ref)
{
  fCStatus = B_NO_INIT;
  fName = NULL;
  SetTo(ref);
}


BPath::~BPath()
{
	free(fName);
}

status_t BPath::SetTo(const BEntry *entry)
{
	if (entry) {
		fCStatus = entry->GetPath(this);
	} else {
		fCStatus = B_BAD_VALUE;
		Unset();
	}
	
	return fCStatus;
}

status_t 
BPath::SetTo(const entry_ref *ref)
{
	if (!ref) {
		fCStatus = B_BAD_VALUE;
		Unset();
	} else {
		BEntry tmp(ref);
		fCStatus = tmp.GetPath(this);
	}

	return fCStatus;
}

status_t BPath::SetTo(const char *dir, const char *leaf, bool normalize)
{
	status_t	err;
	char		*p;
	size_t		l, m, n;
	BEntry		entry;

	if (!dir || (leaf && (leaf[0] == '/'))) {
		err = B_BAD_VALUE;
		goto error1;
	}

	l = strlen(dir);
	m = 0;
	n = 0;
	if (leaf) {
		if (dir[l-1] != '/')
			n = 1;
		m = strlen(leaf);
	}

	/*
	don't do a realloc() here. this allows to pass fName as dir or
	leaf to SetTo().
	*/

	p = (char *) malloc(l+m+n+1);
	if (!p) {
		err = B_NO_MEMORY;
		goto error1;
	}

	memcpy(&p[0], dir, l);
	if (leaf) {
		memcpy(&p[l], "/", n);
		memcpy(&p[l+n], leaf, m);
	}
	p[l+m+n] = '\0';	

	/*
	if the paths look ok, we're fine. otherwise, we have to
	'normalize' them.
	*/

	if (normalize || (p[0] != '/') || check_path(p)) {
		err = entry.SetTo(p);
		if (err != B_OK)
			goto error2;
		err = entry.GetPath(this);
		if (err != B_OK)
			goto error2;
		free(p);
		return (fCStatus=B_NO_ERROR);
	}

	free(fName);
	fName = p;

	return (fCStatus=B_NO_ERROR);

error2:
	free(p);
error1:
//+ PRINT(("PATH.SETTO(%s, %s) FAILED\n", dir ? dir : NULL, leaf ? leaf : "NULL"));
	Unset();
	return (fCStatus=err);
}

status_t  BPath::SetTo(const BDirectory *dir, const char *leaf, bool normalize)
{
	status_t	err;
	BEntry		e;

	if ((err=dir->InitCheck()) != B_NO_ERROR) {
	  err = B_BAD_VALUE;
	  goto error1;
	}

	err = dir->GetEntry(&e);
	if (err) 
	  goto error1;

	err = e.GetPath(this);
	if (err != B_OK)
	  goto error1;

	return (fCStatus=Append(leaf, normalize));

error1:
	Unset();
	return (fCStatus = err);
}

void BPath::Unset()
{
	free(fName);
	fName = NULL;
	fCStatus = B_NO_INIT;
}

status_t BPath::InitCheck() const
{
  return fCStatus;
}

const char *BPath::Path(void) const
{
	return fName;
}

const char *BPath::Leaf(void) const
{
	char	*p;
	
	p = NULL;
	if (fName) {
		p = strrchr(fName, '/');
		if (p)
			p++;
		else
			p = fName;
	}
	return p;
}

status_t BPath::GetParent(BPath *path) const
{
	status_t	err;
	char		buf[PATH_MAX];
	char		*p;
	size_t		l;

	if (!path) {
		err = B_BAD_VALUE;
		goto error1;
	}

	p = strrchr(fName, '/');
	if (p == fName) {
		if (p[1] == '\0') {
			err = ENOENT;
			goto error1;
		}
		return path->SetTo("/");	
	}
	l = p - fName;
	if (l >= PATH_MAX) {
		err = ENAMETOOLONG;
		goto error1;
	}
	memcpy(buf, fName, l);
	buf[l] = '\0';

	return path->SetTo(buf);

error1:
	path->Unset();
	return err;
}

status_t BPath::Append(const char *leaf, bool normalize)
{
	return SetTo(fName, leaf, normalize);
}


bool	BPath::operator==(const BPath &item) const
{
	if (!fName || !item.fName)
		return false;
	return (!strcmp(fName, item.fName));
}

bool	BPath::operator==(const char *path) const
{
	if (!fName || !path)
		return false;
	return (!strcmp(fName, path));
}

bool	BPath::operator!=(const BPath &item) const
{
	if (!fName || !item.fName)
		return true;
	return (strcmp(fName, item.fName) ? true : false);
}

bool	BPath::operator!=(const char *path) const
{
	if (!fName || !path)
		return true;
	return (strcmp(fName, path) ? true : false);
}

BPath &	BPath::operator=(const BPath &item)
{
	if (this == &item) {
		return *this;
	}

	Unset();

	if (!item.fName) {
		fCStatus = B_NO_INIT;
		return *this;
	}

	fName = strdup(item.fName);
	fCStatus = fName ? B_OK : B_NO_INIT;
	return *this;
}

BPath &	BPath::operator=(const char *path)
{
	Unset();

	if (!path) {
		fCStatus = B_NO_INIT;
		return *this;
	}

	fName = strdup(path);
	fCStatus = fName ? B_OK : B_NO_INIT;
	return *this;
}

bool	BPath::IsFixedSize() const
{
	return false;
}

type_code	BPath::TypeCode() const
{
	return B_REF_TYPE;
}

bool		BPath::AllowsTypeCode(type_code code) const
{
	if (code == B_REF_TYPE)
		return true;
	return false;
}

ssize_t		BPath::FlattenedSize() const
{
	const char	*p;
	int32		l;

	p = Leaf();
	if (!p)
		l = 0;
	else {
		if (*p)
			l = strlen(p) + 1;
		else
			l = 1+1;
	}
	return (sizeof(dev_t) + sizeof(ino_t) + l);
}

status_t	BPath::Flatten(void *buffer, ssize_t) const
{
	status_t	err;
	char		*q;
	const char	*p;
	char		path[PATH_MAX];
	struct stat	st;

	q = (char *) buffer;
	p = Leaf();
	if (!p) {
		st.st_dev = (dev_t) -1;
		st.st_ino = (ino_t) -1;
	} else {
		if (p-fName >= PATH_MAX)
			return ENAMETOOLONG;
		memcpy(path, fName, p-fName);
		path[p-fName] = '\0';
		err = _krstat_(-1, path, &st, false);
		if (err != B_OK)
			return err;
		if (*p)
			strcpy(q + sizeof(dev_t) + sizeof(ino_t), p);
		else
			strcpy(q + sizeof(dev_t) + sizeof(ino_t), ".");
	}
	memcpy(q, &st.st_dev, sizeof(dev_t));
	memcpy(q + sizeof(dev_t), &st.st_ino, sizeof(ino_t));
	return B_OK;
}

status_t	BPath::Unflatten(type_code c, const void *buf, ssize_t size)
{
	status_t	err;
	entry_ref	ref;
	BEntry		entry;
	const char	*p;

	Unset();

	if (c != B_REF_TYPE)
		return (fCStatus = B_BAD_VALUE);

	p = (const char *) buf;

	memcpy(&ref.device, p, sizeof(dev_t));
	memcpy(&ref.directory, p + sizeof(dev_t), sizeof(ino_t));
	if (ref.device == (dev_t)-1 || size <= (ssize_t)(sizeof(dev_t) + sizeof(ino_t)))
		return B_OK;

	err = ref.set_name(p + sizeof(dev_t) + sizeof(ino_t));
	if (err != B_OK)
		return (fCStatus=err);
	err = entry.SetTo(&ref);
	if (err != B_OK)
		return (fCStatus=err);
	return (fCStatus = entry.GetPath(this));
}

void		BPath::_WarPath1() {}
void		BPath::_WarPath2() {}
void		BPath::_WarPath3() {}
