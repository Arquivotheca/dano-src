#ifndef	_ALLOCA_H
#define	_ALLOCA_H

#include <be_setup.h>
#include <sys/types.h>

__extern_c_start

/* Remove any previous definitions.  */
#undef	__alloca
#undef	alloca

#if __GNUC__ 
/* Allocate a block that will be freed when the calling function exits.  */
extern void * __alloca (size_t __size);
extern void * alloca (size_t __size);

# define __alloca(size)	__builtin_alloca (size)
# define alloca(size)	__alloca (size)

#elif  __MWERKS__	

#define alloca __alloca

#endif	/* __MWERKS__ */

__extern_c_end

#endif /* alloca.h */
