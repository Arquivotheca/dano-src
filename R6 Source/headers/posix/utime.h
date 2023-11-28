#ifndef _UTIME_H_
#define _UTIME_H_

#include <be_setup.h>

struct utimbuf
{
	time_t actime;
	time_t modtime;
};


__extern_c_start

int utime(const char *path, const struct utimbuf *times);

__extern_c_end

#endif /* _UTIME_H_ */
