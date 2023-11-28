#define _MALLOC_INTERNAL

#include "malloc.h"
#include "malloc_priv.h"
#include "priv_runtime.h"

#if !__INTEL__ && !__arm__	/* FIXME: Is this right? */
extern void __clean_up_C_library();
#endif

#if __INTEL__ || __arm__	/* FIXME: Is this right? */
extern void _initialize_madviser(void);
#endif

void
initialize_before(void)
{
	/*
	 * This function is used by both dynamically and statically linked
	 * applications.
	 */

	/* initialize the Be world */
	_init_tls();
	_init_gen_malloc();
	_init_sbrk_();
	_init_kernel_export_data();
#if _SUPPORTS_LEAK_CHECKING
	_init_leak_checker();
#endif
#if _SUPPORTS_PROFILING
	_init_profiling();
#endif
#if _SUPPORTS_FEATURE_MEMORY_ADVISER
	_initialize_madviser();
#endif
	_init_system_time_();
	_init_fast_syscall();
}

void
terminate_after(void)
{
	/*
	 * This function is used by both dynmaically and statically linked
	 * applications.
	 */

#if _SUPPORTS_PROFILING
	_cleanup_profiling();
#endif
#if _SUPPORTS_LEAK_CHECKING
	_cleanup_leak_checker();
#endif
	/* tear down the Be world */
	_cleanup_gen_malloc();				/* let heap cleanup occur (sems) */
}
