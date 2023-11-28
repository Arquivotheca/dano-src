/* GCC defines its own stdarg.h */

#if __GNUC__
# ifndef _STDARG_H /* This is what GCC defines */
#  include_next <stdarg.h>
# endif /* _STDARG_H */
#elif	__POWERPC__   /* really metrowerks style */

# ifndef _STDARG_H_
#  define _STDARG_H_

#  include <be_setup.h>

#  include <va_list.h>

#  define va_start(ap, parm)	ap = __va_start(parm)
#  define va_end(ap)

#  define __va_start(parm) (__std(va_list)) ((char *)&parm + (sizeof (parm) + (1U) & ~(1U)))
#  define va_arg(ap, type) (*(((type *) (ap += sizeof(type))) - 1))

# endif /* _STDARG_H_ */

#endif /* __POWERPC__ */ 

