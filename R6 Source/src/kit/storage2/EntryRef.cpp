#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <OS.h> 
#include <support2/Debug.h>
#include <support2/ITextStream.h>
#include <storage2/Entry.h>
#include <storage2/Path.h>

#include "priv_syscalls.h"
#include "Storage.h"

namespace B {
namespace Storage2 {

extern const char	_dot_[] = ".";
extern const char	_dotdot_[] = "..";

long	_parse_path_(int fd, const char *path, entry_ref *ref, int *nfd,
						char *name);

status_t
get_ref_for_path(const char *path, entry_ref *ref)
{
	return _parse_path_(-1, path, ref, NULL, NULL);
}
	
entry_ref::entry_ref()
{
	device = -1;
	directory = -1;
	name = NULL;
}

entry_ref::entry_ref(dev_t dev, ino_t dir, const char *in_name)
{
	if (in_name) 
		name = strdup(in_name);
	else
		name = NULL;

	device = dev;
	directory = dir;
}

entry_ref::entry_ref(const entry_ref &ref)
{
	if (ref.name)
		name = strdup(ref.name);
	else
		name = NULL;
	if (!name) {
		device = -1;
		directory = -1;
	} else {
		device = ref.device;
		directory = ref.directory;
	}
}

/* A bad set_name (because !new_name or a bad realloc)
 * DOES NOT clear the dev or dir fields; but the name will
 * be cleared.
 */
status_t entry_ref::set_name(const char *new_name)
{
	char *p;
	size_t len;
	status_t err;

	if (!new_name) {
		err = B_BAD_VALUE;
		if (name)
		  free(name);
		goto error;
	}

	len = strlen(new_name) + 1;
	if (len > B_FILE_NAME_LENGTH) {
		err = ENAMETOOLONG;
		if (name)
		  free(name);
		goto error;
	}

	p = (char *) realloc(name, len);

	if (!p) {
	  if(name)
		free(name);
	  err = B_NO_MEMORY;
	  goto error;
	}

	name = p;
	memcpy(name, new_name, len);
	return B_NO_ERROR;
  
error:
	name = NULL;
	return err;
}

entry_ref::~entry_ref()
{
	if (name)
		free(name);
}

bool		entry_ref::operator==(const entry_ref &ref) const
{
	if (!name ^ !ref.name)
		return false;
	return ((device == ref.device) &&
			(directory == ref.directory) &&
			(!name || (name && !strcmp(name, ref.name))));
}

bool		entry_ref::operator!=(const entry_ref &ref) const
{
	if (!name ^ !ref.name)
		return true;
	return ((device != ref.device) ||
			(directory != ref.directory) ||
			(name && strcmp(name, ref.name)));
}

entry_ref &	entry_ref::operator=(const entry_ref &ref)
{
	size_t		len;
	char		*p;

	if (&ref == this)
	  return *this;

	if (!ref.name)
		goto empty;
	len = strlen(ref.name) + 1;
	p = (char *) realloc(name, len);

	if (!p)
		goto empty;
	name = p;		
	memcpy(name, ref.name, len);

	device = ref.device;
	directory = ref.directory;
	return *this;

empty:
	free(name);
	device = -1;
	directory = -1;
	name = NULL;
	return *this;
}


bool operator<(const entry_ref & a, const entry_ref & b)
{
        if (a.device != b.device) return a.device < b.device;
        if (a.directory != b.directory) return a.directory < b.directory;
        /* treat NULL and empty string the same -- can't hurt */
        if (!a.name) return (b.name != NULL) && b.name[0];
        if (!b.name) return false;
        return strcmp(a.name, b.name) < 0;
}

ITextOutput::arg operator<<(ITextOutput::arg io, const entry_ref& ref)
{
	io << "entry_ref(dev=" << ref.device
	   << ", dir=" << ref.directory
	   << ", name=" << ref.name << ")";
	return io;
}

long	_parse_path_(int fd, const char *path, entry_ref *ref, int *nfd,
						char *name)
{
	long			err;
	struct stat		st;
	const char		*q;
	int				nl, len;
	char			buf[1024];
	dev_t			dev, pdev;
	ino_t			ino, pino;
	long			i, count;
	int				dfd;
	struct dirent	ents[1024 / sizeof(struct dirent)];
	struct dirent	*d;

	q = strrchr(path, '/');
	if (!q) {
		q = path;
		strcpy(buf, _dot_);
	} else {
		q++;
		if (q - path + 1 > (ssize_t)sizeof(buf)) {
			err = -1;
			goto error1;
		}
		memcpy(buf, path, q - path);
		buf[q-path] = '\0';
	}

	len = strlen(path);
	if (len > B_PATH_NAME_LENGTH) {
		err = E2BIG;
		goto error1;
	}
	
	
	nl = len - (q - path) + 1;
	if (len == 0) {
		err = ENOENT;
		goto error1;
	}

	if (nl > B_FILE_NAME_LENGTH) {
		err = E2BIG;
		goto error1;
	}



	// first the easy case (the path is of the form "xxx/name" or "name")

	if ((*q != '\0') && strcmp(q, _dot_) && strcmp(q, _dotdot_)) {
		if (ref) {
			err = _krstat_(fd, buf, &st, false);
			if (err)
				goto error1;

			ref->device = st.st_dev;
			ref->directory = st.st_ino;
			err = ref->set_name(q);
			if (err)
				goto error1;

		} else {
			*nfd = _kopen_(fd, *buf ? buf : NULL, O_RDONLY, 0, true);
			if (*nfd < 0) {
				err = *nfd;
				goto error1;
			}
			memcpy(name, q, nl);
		}
		return 0;
	}

	// then the hard case (all the other path forms)

	err = _krstat_(fd, path, &st, false);
	if (err)
		goto error1;
	dev = st.st_dev;
	ino = st.st_ino;
	
	if (len+1+strlen(_dotdot_)+1 > sizeof buf) {
		err = -1;
		goto error1;
	}
	memcpy(buf, path, len);
	buf[len] = '/';
	memcpy(&buf[len+1], _dotdot_, strlen(_dotdot_)+1);
	
	err = _krstat_(fd, buf, &st, false);
	if (err)
		goto error1;
	
	// are we at the root of the name space?

	pdev = st.st_dev;
	pino = st.st_ino;
	if ((pdev == dev) && (pino == ino)) {
		if (ref) {
			ref->device = dev;
			ref->directory = ino;
			ref->set_name(_dot_);
		} else {
			*nfd = _kopen_(-1, "/", O_RDONLY, 0, true);
			if (*nfd < 0) {
				err = *nfd;
				goto error1;
			}
			strcpy(name, _dot_);
		}
		return 0;
	}

	dfd = _kopendir_(fd, buf, true);
	if (dfd < 0) {
		err = dfd;
		goto error1;
	}
	do {
		count = _kreaddir_(dfd, ents, sizeof ents, sizeof ents/sizeof *d);
		if (count < 0) {
			err = count;
			goto error2;
		}
		d = ents;
		for(i=0; i<count; i++, d = (dirent*)((char*)d+d->d_reclen)) {
			if (d->d_ino == ino)
				if (pdev == dev)
					goto exit;
				else {
					err = _krstat_(dfd, d->d_name, &st, false);
					if (err)
						goto error2;
					if (st.st_dev == dev)
						goto exit;
				}
		}
	} while (count > 0);

exit:
	if (count == 0) {
		err = ENOENT;
		goto error2;
	}
	nl = strlen(d->d_name) + 1;
	if (ref) {
		ref->device = pdev;
		ref->directory = pino;
		err = ref->set_name(d->d_name);
		if (err)
			goto error2;
	} else {
		*nfd = _kopen_(dfd, NULL, O_RDONLY, 0, true);
		if (*nfd < 0) {
			err = *nfd;
			goto error2;
		}
		memcpy(name, d->d_name, nl);
	}
	_kclosedir_(dfd);
	return 0;

error2:
	_kclosedir_(dfd);
error1:
	return err;
}

} }	// namespace B::Storage2
