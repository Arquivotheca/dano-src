#ifndef _ASSERT_H_
#define _ASSERT_H_

#include <be_setup.h>

#undef assert

#ifdef NDEBUG

#define assert(condition) ((void) 0)

#else


#if __GNUC__

__extern_c_start

extern void __assert_fail(const char *__assertion,
						  const char *__file,
						  unsigned int __line,
						  const char *__function);

/* Likewise, but prints the error text for ERRNUM.  */
extern void __assert_perror_fail(int __errnum,
								 const char *__file,
								 unsigned int __line,
								 const char *__function);

__extern_c_end

#if _SUPPORTS_FEATURE_SMALL_ASSERT
# define assert(expr)						      \
  ((void) ((expr) ? 0 :						      \
	   (__assert_fail (#expr,				      \
			   "***not available", __LINE__, __PRETTY_FUNCTION__), 0)))
#else
# define assert(expr)						      \
  ((void) ((expr) ? 0 :						      \
	   (__assert_fail (#expr,				      \
			   __FILE__, __LINE__, __PRETTY_FUNCTION__), 0)))
#endif

#elif __POWERPC__

__extern_c_start

void __assertion_failed(char * condition, char * testfilename, int lineno);

__extern_c_end


#if _SUPPORTS_FEATURE_SMALL_ASSERT
#define assert(condition) ((condition) ? ((void) 0) : __std(__assertion_failed)(#condition, "***not available", __LINE__))
#else
#define assert(condition) ((condition) ? ((void) 0) : __std(__assertion_failed)(#condition, __FILE__, __LINE__))
#endif

#endif

#endif /* def NDEBUG */

#endif /* _ASSERT_H_ */
