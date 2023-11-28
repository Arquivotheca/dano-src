#if __GNUC__
# ifndef _STDDEF_H /* This is what GCC defines */
#  include_next <stddef.h>
# endif /* _STDDEF_H */
#elif __POWERPC__


# ifndef _STDDEF_H_
#  define _STDDEF_H_

typedef long	ptrdiff_t;

#  include <size_t.h>
#  include <wchar_t.h>
#  include <null.h>

#  define offsetof(type, member)	((size_t)&(((type *)0)->member))
# endif /* _STDDEF_H_ */

#endif /* __POWERPC__ */
