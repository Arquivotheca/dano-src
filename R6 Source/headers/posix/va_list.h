#ifndef _VA_LIST_H_
#define _VA_LIST_H_

#if __GNUC__

#include <stdarg.h>

#elif __POWERPC__

typedef char *va_list;

#endif

#endif /* _VA_LIST_H_ */
