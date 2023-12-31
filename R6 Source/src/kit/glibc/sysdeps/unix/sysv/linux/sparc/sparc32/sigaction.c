/* POSIX.1 sigaction call for Linux/SPARC.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Miguel de Icaza (miguel@nuclecu.unam.mx), 1997.

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

#include <syscall.h>
#include <sys/signal.h>
#include <errno.h>
#include <kernel_sigaction.h>

/* The variable is shared between all wrappers around signal handling
   functions which have RT equivalents.  */
int __libc_missing_rt_sigs;

int
__sigaction (int sig, __const struct sigaction *act, struct sigaction *oact)
{
  int ret;
  struct kernel_sigaction k_sigact, k_osigact;

  /* Magic to tell the kernel we are using "new-style" signals, in that
     the signal table is not kept in userspace.  Not the same as the
     really-new-style rt signals.  */
  sig = -sig;

  if (act)
    {
      k_sigact.k_sa_handler = act->sa_handler;
      memcpy (&k_sigact.sa_mask, &act->sa_mask, sizeof (sigset_t));
      k_sigact.sa_flags = act->sa_flags;
    }

  {
    register int r_syscallnr __asm__("%g1") = __NR_sigaction;
    register int r_sig __asm__("%o0") = sig;
    register struct kernel_sigaction *r_act __asm__("%o1");
    register struct kernel_sigaction *r_oact __asm__("%o2");

    r_act = act ? &k_sigact : NULL;
    r_oact = oact ? &k_osigact : NULL;

    __asm__ __volatile__("t 0x10\n\t"
			 "bcc 1f\n\t"
			 " nop\n\t"
			 " sub %%g0,%%o0,%%o0\n"
			 "1:"
			 : "=r"(r_sig)
			 : "r"(r_syscallnr), "r"(r_act), "r"(r_oact),
			   "0"(r_sig));

    ret = r_sig;
  }

  if (ret >= 0)
    {
      if (oact)
	{
	  oact->sa_handler = k_osigact.k_sa_handler;
	  memcpy (&oact->sa_mask, &k_osigact.sa_mask, sizeof (sigset_t));
	  oact->sa_flags = k_osigact.sa_flags;
	  oact->sa_restorer = NULL;
	}
      return 0;
    }

  __set_errno (-ret);
  return -1;
}

weak_alias (__sigaction, sigaction);
