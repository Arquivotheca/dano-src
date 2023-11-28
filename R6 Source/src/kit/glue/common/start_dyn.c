
/* --------------------------------------------------------- */
/*
 * This is the C Runtime startup for Be, dynamically liked version.
 *
 * The theory is:

	start:
			main();
			exit(retval);
 */

#include <stdlib.h>
#include <thread_magic.h>
#include <priv_runtime.h>

extern int	main(int argc, char **argv, char **envp);
extern char **environ;

#if __ELF__
extern void _init_c_library_(int argc, ...);
#endif

/* --------------------------------------------------------- */

void __start(int argc, char **argv, char**envp);

static char *default_environ[] = { "NO_ENVIRON=true", 0 };

/*
 * This is the main entry point for Metrowerks apps.  The stack is set up
 * by the kernel loader as indicated.  
 */

void
#if __ELF__
_start(int argc, char **argv, char **envp)
#elif __POWERPC__ && !defined (__ELF__)
__start(int argc, char **argv, char **envp)
#endif
{
	int				retval;

	argv_save = argv;
	environ = envp;
	if (environ == 0)
		environ = default_environ;
	
	__main_thread_id = find_thread(0);

#if __PPC__

	/*
	 * This is the old-style per-thread data area. This copies the magic number in
	 * (see srcx/c/ANSI_Be/thread_data.c)
	 * we have to leave this in here so that newly compiled app still run with
	 * the old libroot on Macs.
	 */

	...
	memcpy(&thread_data[THREAD_DATA_SIZE] - sizeof(magic_template),
		   &magic_template, sizeof(magic_template));

#endif

#if __ELF__
	_init_c_library_(argc, argv, envp);
#endif

	_call_init_routines_();

	retval = main(argc, argv, envp);	/* Call main() */

	_thread_do_exit_notification();
	exit(retval);			/* if returned, call library exit */
}
