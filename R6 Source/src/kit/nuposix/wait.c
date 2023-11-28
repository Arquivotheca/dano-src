#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <OS.h>

#include "priv_syscalls.h"
#include "wait_priv.h"

pid_t
#if __ELF__
__waitpid(pid_t pid, int *stat, int options)
#else
waitpid(pid_t pid, int *stat, int options)
#endif
{
	status_t    res;
	pid_t		thid;
	int32		wait_opts = 0, result, reason;

	if (options & WNOHANG)
			wait_opts |= B_WAIT_NO_BLOCK;

	if (options & WUNTRACED)
			wait_opts |= B_WAIT_STOPPED;


	res = _kwait_for_team_(pid, wait_opts, &thid, &reason, &result);
	if (res < 0) {
		errno = res;
		if ((options & WNOHANG) && res == B_WOULD_BLOCK)
			return 0;
		else
			return -1;
	}

	if (stat == NULL)    /* user doesn't want more detailed info */
		return thid;

	switch (reason) {
	case	B_THREAD_EXITED:
			*stat = result & 0xff;	/* *sigh* POSIX only returns 8 bits */
			break;

	case	B_THREAD_KILLED:
			*stat = (SIGKILL << 8);
			break;

	case	B_THREAD_SIGNALED:
			*stat = ((result & 0xff)  << 8);
			break;

	case	B_THREAD_FAULTED:
			*stat = (result & 0xff) << 8;
			break;

	case	B_THREAD_STOPPED_:
			*stat = (result & 0xff) << 16;
			break;

	default:
			/* Hmm... what happened here??. */
			*stat = result & 0xff;
			break;
	}

	return thid;
}
#if __ELF__
#pragma weak waitpid = __waitpid
#endif

pid_t
wait(int *status)
{
	return (pid_t) waitpid(-1,(int *) status, 0);
}
