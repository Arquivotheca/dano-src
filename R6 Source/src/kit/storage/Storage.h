#ifndef _STORAGE_H
#define	_STORAGE_H

#include <sys/param.h>

#define	DEFAULT_MODE_BITS		(0644)
#define	DEFAULT_DIR_MODE_BITS	(0755)

struct entry_ref;

extern const char	_dot_[];
extern const char	_dotdot_[];

extern const int	_omodes_[2];


extern long	_parse_path_(int fd, const char *path, entry_ref *ref, int *nfd,
							char *name);

// ### duplicated from fsproto.h

#define		WSTAT_MODE		0x0001
#define		WSTAT_UID		0x0002
#define		WSTAT_GID		0x0004
#define		WSTAT_SIZE		0x0008
#define		WSTAT_ATIME		0x0010
#define		WSTAT_MTIME		0x0020
#define		WSTAT_CRTIME	0x0040

#define		WFSSTAT_NAME	0x0001

#endif
