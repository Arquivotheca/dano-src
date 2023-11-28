#ifndef _BE_SETUP_H
#define _BE_SETUP_H

#ifdef  __cplusplus
#define __extern_c_start extern "C" {
#define __extern_c_end   }
#else
#define __extern_c_start
#define __extern_c_end
#endif

#if __POWERPC__
# define _STD
# define __std(ref)  ref
# define __global()                                        

# define __be_os 2
# define __dest_os __be_os
# ifndef __MSL__
#  define __MSL__ 0x4011
# endif /* __MSL__ */
#endif

#if __GNUC__
# define __CONCAT(x,y)  x ## y     
# define __GLIBC__		2
# define __GLIBC_MINOR__	1
#endif

#include <BeBuild.h>

#endif /* _BE_SETUP_H */
