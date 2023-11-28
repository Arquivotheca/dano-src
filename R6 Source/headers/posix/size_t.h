#ifndef _SIZE_T_H_
#define _SIZE_T_H_

#if __GNUC__

# include <stddef.h>

#else

typedef unsigned long size_t;
typedef          long ssize_t;

#endif

#endif /* _SIZE_T_H_ */
