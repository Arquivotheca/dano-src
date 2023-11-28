//	OBlock.h

#ifndef _OBLOCK_H
#define _OBLOCK_H

#include <OS.h>
#include <string.h>

struct OType {
	OType(ulong t = 0, OType *ptr = NULL) {
		next = ptr; type = t;
	}
	OType		*next;
	ulong		type;
};

struct OBlock {
	OBlock() {
		memset(this, 0, sizeof(*this));
	}
	~OBlock() {
		delete[] outFileName;
	}
	
	port_id		fromIDE;
	port_id		toIDE;
	char		*outFileName;
	OType		*types;
	long		numErrors;
	long		lastError;
};

extern void usage(const char *message, OBlock & o);
extern void error(long err, OBlock & o, const char *message, ...);

#endif
