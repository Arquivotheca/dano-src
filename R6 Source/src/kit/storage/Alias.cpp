/* ++++++++++
	FILE:	Alias.cpp
	REVS:	$Revision: 1.1 $
	NAME:	cyril
	DATE:	Wed Apr 23 12:55:21 PDT 1997
	Copyright (c) 1997 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <dirent.h>

#include <Directory.h>
#include <Entry.h>
#include <DataIO.h>
#include <Alias.h>
#include <Path.h>

#include "Storage.h"
#include "priv_syscalls.h"

#define		REF_CUR_VERSION		0x00000001

static status_t
stream_write(BDataIO *s, const void *buf, size_t len)
{
	ssize_t		cnt;
	size_t		sz;
	const char	*p;

	p = (const char *) buf;
	sz = len;
	while (sz > 0) {
		cnt = s->Write(p, sz);
		if (cnt < 0)
			return cnt;
		sz -= cnt;		
		p += cnt;
	} 
	return 0;
}

static status_t
stream_read(BDataIO *s, void *buf, size_t len)
{
	ssize_t		cnt;
	size_t		sz;
	char		*p;

	p = (char *) buf;
	sz = len;
	while (sz > 0) {
		cnt = s->Read(p, sz);
		if (cnt < 0)
			return cnt;
		sz -= cnt;		
		p += cnt;
	} 
	return 0;
}

status_t
write_alias(const char *path, void *buf, size_t *len)
{
	BMemoryIO	s(buf, *len);

	return write_alias(path, &s, len);
}

status_t
write_alias(const char *path, BDataIO *stream, size_t *len)
{
	status_t	err;
	size_t		l, refsize;
	uint32		version;

	l = strlen(path);
	refsize = sizeof(uint32) + sizeof(uint32) + l + 1;
	if (len)
		*len = refsize;

	version = REF_CUR_VERSION;
	err = stream_write(stream, &version, sizeof(uint32));
	if (err)
		return err;
	err = stream_write(stream, &refsize, sizeof(uint32));
	if (err)
		return err;
	err = stream_write(stream, path, l+1);
	if (err)
		return err;
	return 0;
}


status_t
read_alias(const void *buf, BPath *result, size_t *len, bool block)
{
	BMemoryIO	s(buf, *len);

	return read_alias(&s, result, len, block);
}

status_t
read_alias(BDataIO *s, BPath *result, size_t *len, bool block)
{
	status_t	err;
	uint32		version;
	uint		refsize;
	size_t		l;
	char		buf[PATH_MAX];

	err = stream_read(s, &version, sizeof(uint32));
	if (err)
		return err;
	if (version != REF_CUR_VERSION)
		return B_ERROR;
	err = stream_read(s, &refsize, sizeof(uint32));
	if (err)
		return err;
	if (len)
		*len = refsize;

	l = refsize - sizeof(uint32) - sizeof(uint32);
	err = stream_read(s, buf, l);
	if (err)
		return err;
	return resolve_link(buf, result, block);
}

status_t
resolve_link(const char *path, BPath *result, bool)
{
	status_t		err;
	struct stat		st;
	char			lnk[PATH_MAX];
	int				iter;
	ssize_t			sz;
	BDirectory		dir;
	BEntry			entry;
	  
	err = _krstat_(-1, path, &st, FALSE);
	switch(err) {
	case 0:
		break;
	case ENOENT:
		return result->SetTo(path);	
	default:
		return err;
	}
		
	err = result->SetTo(path);
	if (err)
		return err;

	iter = 0;
	while (S_ISLNK(st.st_mode)) {
		if (iter++ == SYMLINKS_MAX)
			return ELOOP;
		sz = _kreadlink_(-1, result->Path(), lnk, sizeof lnk);		
		if (sz < 0)
			return sz;
		if (sz >= (ssize_t)sizeof(lnk))
	  		return ENAMETOOLONG;
		lnk[sz] = '\0';

		if (lnk[0] != '/') {
			err = result->GetParent(result);
			if (err)
				return err;
			err = dir.SetTo(result->Path());
			if (err)
				return err;
			err = dir.FindEntry(lnk, &entry);
			if (err)
				return err;
			err = entry.GetPath(result);
		} else
			err = result->SetTo(lnk);
		if (err)
			return err;

		err = _krstat_(-1, result->Path(), &st, FALSE);
		if (err)
			return err;
	}
	return 0;
}
