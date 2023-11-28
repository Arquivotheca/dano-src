/* Copyright (C) 1998 Free Software Foundation, Inc.
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

#include <stdlib.h>
#include <unistd.h>
#include <elf/ldsodefs.h>

extern void __libc_init_first (int argc, char **argv, char **envp);

extern int _dl_starting_up;
weak_extern (_dl_starting_up)
extern int __libc_multiple_libcs;

struct startup_info
{
  void *sda_base;
  int (*main) (int, char **, char **, void *);
  int (*init) (int, char **, char **, void *);
  void (*fini) (void);
};

int
__libc_start_main (int argc, char **argv, char **envp,
		   void *auxvec, void (*rtld_fini) (void),
		   struct startup_info *stinfo,
		   char **stack_on_entry)
{
#ifndef PIC
  /* The next variable is only here to work around a bug in gcc <= 2.7.2.2.
     If the address would be taken inside the expression the optimizer
     would try to be too smart and throws it away.  Grrr.  */
  int *dummy_addr = &_dl_starting_up;

  __libc_multiple_libcs = dummy_addr && !_dl_starting_up;
#endif

  /* the PPC SVR4 ABI says that the top thing on the stack will
     be a NULL pointer, so if not we assume that we're being called
     as a statically-linked program by Linux...	 */
  if (*stack_on_entry != NULL)
    {
      /* ...in which case, we have argc as the top thing on the
	 stack, followed by argv (NULL-terminated), envp (likewise),
	 and the auxilary vector.  */
      argc = *(int *) stack_on_entry;
      argv = stack_on_entry + 1;
      envp = argv + argc + 1;
      auxvec = envp;
      while (*(char **) auxvec != NULL)
	++auxvec;
      ++auxvec;
      rtld_fini = NULL;
    }

  /* Register the destructor of the dynamic linker if there is any.  */
  if (rtld_fini != NULL)
    atexit (rtld_fini);

  /* Set the global _environ variable correctly.  */
  __environ = envp;

  /* Call the initializer of the libc.  */
#ifdef PIC
  if (_dl_debug_impcalls)
    _dl_debug_message (1, "\ninitialize libc\n\n", NULL);
#endif
  __libc_init_first (argc, argv, envp);

  /* Call the initializer of the program.  */
#ifdef PIC
  if (_dl_debug_impcalls)
    _dl_debug_message (1, "\ninitialize program: ", argv[0], "\n\n", NULL);
#endif
  stinfo->init (argc, argv, __environ, auxvec);

  /* Register the destructor of the program.  */
  atexit (stinfo->fini);

#ifdef PIC
  if (_dl_debug_impcalls)
    _dl_debug_message (1, "\ntransferring control: ", argv[0], "\n\n", NULL);
#endif

  exit (stinfo->main (argc, argv, __environ, auxvec));
}
