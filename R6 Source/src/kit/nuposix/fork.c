#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <module.h>

#include <OS.h>

#define _MALLOC_INTERNAL
#include "malloc.h"
#include "malloc_priv.h"
#include "priv_syscalls.h"
#include "priv_runtime.h"

#if !__ELF__
extern void __fork_critical_regions();
#endif

/*
 * POSIX Style Fork
 */

extern long __main_thread_id;

#define	AT_FORK_LIMIT	64

void	(*__at_fork_funcs[AT_FORK_LIMIT])(void);
int		__at_fork_count = 0;

#if __ELF__
pid_t
__fork()
#else
pid_t
fork()
#endif
{
	int	pid, i;
	sigset_t	holdall_sigs, old_sigs ;

	/*
	 * Hold all signals until the new child context is
	 * setup and can handle signals properly.
	 */

	sigfillset(&holdall_sigs);
	sigprocmask(SIG_SETMASK, &holdall_sigs, &old_sigs);

	__lock_gen_malloc();  /* have to lock this so that it is in a known state */

	pid = _kfork_();

	if (pid == 0) {
		/* 
		 * The child task has to reinitialize a couple of things before
 		 * it is allowed to continue
		 */

		_init_tls();
		__main_thread_id = find_thread((void *) 0);
#if !__ELF__
		__fork_critical_regions();
#endif
		__fork_sbrk_();
		__fork_gen_malloc();
		__fork_addon_();

		/* Now run through a list of registered atfork() functions */
		for (i = 0; i < __at_fork_count; i++) 
			__at_fork_funcs[i]();


	} else {                         /* we're the parent */
		if (pid < 0) {
			errno = pid;
			pid = -1;
		}

		__unlock_gen_malloc();
	}

	/* Restore signal set.. */
	sigprocmask(SIG_SETMASK, &old_sigs, NULL);

	return pid;
}
#if __ELF__
#pragma weak fork = __fork
#endif

int
atfork(void (*func)(void))
{
	if (__at_fork_count >= AT_FORK_LIMIT) 
		return ENOMEM;

	__at_fork_funcs[__at_fork_count++] = func;

	return 0;
}
