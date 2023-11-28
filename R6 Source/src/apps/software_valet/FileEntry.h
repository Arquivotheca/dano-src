#ifndef _FILEENTRY_H_
#define _FILEENTRY_H_

#include <SupportDefs.h>

// FileEntry.h

typedef struct {
	long	size;
	ulong	groups;
	int32	platform;
} FileEntry;


#define MAX_FILES 8192

// memory usage is 96K for up to a maximum of 8192 files

#endif
