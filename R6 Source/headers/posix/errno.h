#ifndef _ERRNO_H_
#define _ERRNO_H_

#include <be_errors.h>

#if !defined(SINGLE_THREADED_ERRNO)

__extern_c_start
extern int *_errnop(void);
__extern_c_end

#define	errno (*(_errnop()))
#define	__set_errno(val) (*_errnop ()) = (val)

#else
extern int errno;
#endif

#endif /* _ERRNO_H_ */
