/* Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
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

/* This file is included multiple times.  */

/* Standard signals  */
  init_sig (SIGHUP, "HUP", N_("Hangup"))
  init_sig (SIGINT, "INT", N_("Interrupt"))
  init_sig (SIGQUIT, "QUIT", N_("Quit"))
  init_sig (SIGILL, "ILL", N_("Illegal Instruction"))
  init_sig (SIGTRAP, "TRAP", N_("Trace/breakpoint trap"))
  init_sig (SIGABRT, "ABRT", N_("Aborted"))
  init_sig (SIGFPE, "FPE", N_("Floating point exception"))
  init_sig (SIGKILL, "KILL", N_("Killed"))
  init_sig (SIGBUS, "BUS", N_("Bus error"))
  init_sig (SIGSEGV, "SEGV", N_("Segmentation fault"))
  init_sig (SIGPIPE, "PIPE", N_("Broken pipe"))
  init_sig (SIGALRM, "ALRM", N_("Alarm clock"))
  init_sig (SIGTERM, "TERM", N_("Terminated"))
  init_sig (SIGURG, "URG", N_("Urgent I/O condition"))
  init_sig (SIGSTOP, "STOP", N_("Stopped (signal)"))
  init_sig (SIGTSTP, "TSTP", N_("Stopped"))
  init_sig (SIGCONT, "CONT", N_("Continued"))
  init_sig (SIGCHLD, "CHLD", N_("Child exited"))
  init_sig (SIGTTIN, "TTIN", N_("Stopped (tty input)"))
  init_sig (SIGTTOU, "TTOU", N_("Stopped (tty output)"))
  init_sig (SIGIO, "IO", N_("I/O possible"))
  init_sig (SIGXCPU, "XCPU", N_("CPU time limit exceeded"))
  init_sig (SIGXFSZ, "XFSZ", N_("File size limit exceeded"))
  init_sig (SIGVTALRM, "VTALRM", N_("Virtual timer expired"))
  init_sig (SIGPROF, "PROF", N_("Profiling timer expired"))
  init_sig (SIGWINCH, "WINCH", N_("Window changed"))
  init_sig (SIGUSR1, "USR1", N_("User defined signal 1"))
  init_sig (SIGUSR2, "USR2", N_("User defined signal 2"))

/* Variations  */
#ifdef SIGEMT
  init_sig (SIGEMT, "EMT", N_("EMT trap"))
#endif
#ifdef SIGSYS
  init_sig (SIGSYS, "SYS", N_("Bad system call"))
#endif
#ifdef SIGSTKFLT
  init_sig (SIGSTKFLT, "STKFLT", N_("Stack fault"))
#endif
#ifdef SIGINFO
  init_sig (SIGINFO, "INFO", N_("Information request"))
#elif defined(SIGPWR)
  init_sig (SIGPWR, "PWR", N_("Power failure"))
#endif

#define	OLD_SIGLIST_SIZE_STR	"32"
