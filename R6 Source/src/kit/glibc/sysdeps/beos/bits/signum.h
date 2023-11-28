/* Copyright (C) 1991, 1993, 1996 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifdef	_SIGNAL_H

#define	SIG_DFL	((__sighandler_t) 0)	/* Default action.  */
#define	SIG_IGN	((__sighandler_t) 1)	/* Ignore signal.  */
#define	SIG_ERR	((__sighandler_t) -1)	/* Error return.  */


#define	SIGHUP	   1	/* Hangup.  */
#define	SIGINT	   2	/* Interactive attention signal.  */
#define	SIGQUIT	   3	/* Quit.  */
#define	SIGILL	   4	/* Illegal instruction.  */
#define	SIGCHLD	   5	/* Child terminated or stopped.  */
#define	SIGABRT	   6	/* Abnormal termination.  */
#define	SIGPIPE	   7	/* Broken pipe.  */
#define	SIGFPE	   8	/* Erroneous arithmetic operation.  */
#define	SIGKILL	   9	/* Kill (cannot be blocked, caught, or ignored).  */
#define	SIGSTOP	   10	/* Stop (cannot be blocked, caught, or ignored).  */
#define	SIGSEGV	   11	/* Invalid access to storage.  */
#define	SIGCONT	   12	/* Continue.  */
#define	SIGTSTP	   13	/* Keyboard stop.  */
#define	SIGALRM	   14	/* Alarm clock.  */
#define	SIGTERM	   15	/* Termination request.  */
#define SIGTTIN    16	/* Background read from tty (POSIX).  */
#define SIGTTOU    17	/* Background write from tty (POSIX).  */
#define SIGUSR1    18	/* User-defined signal 1 (POSIX).  */
#define SIGUSR2    19	/* User-defined signal 2 (POSIX).  */
#define SIGWINCH   20	/* Window size change (4.3 BSD, Sun).  */
#define SIGKILLTHR 21	/* Kill just the thread, not the team (Be).  */
#define SIGTRAP    22

#define SIGBUS		SIGSEGV	/* for old code */

#endif	/* <signal.h> included.  */

/* Signal numbers 23-32 are currently free but may be used in future
   releases.  Use them at your own peril (if you do use them, at least
   be smart and use them backwards from signal 32).  */
#define __signal_max  22
#define _NSIG (__signal_max + 1)
