#ifndef _SYS_UTSNAME_H_
#define _SYS_UTSNAME_H_

#include <be_setup.h>

struct utsname {
	char sysname[32];
	char nodename[32];
	char release[32];
	char version[32];
	char machine[32];
};

__extern_c_start

int uname(struct utsname *name);

__extern_c_end

#endif /* _SYS_UTSNAME_H_ */
