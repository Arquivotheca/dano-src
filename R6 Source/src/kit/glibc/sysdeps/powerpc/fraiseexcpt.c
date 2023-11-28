/* Raise given exceptions.
   Copyright (C) 1997 Free Software Foundation, Inc.
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

#include <fenv_libc.h>

#undef feraiseexcept
void
feraiseexcept (int excepts)
{
  fenv_union_t u;

  /* Raise exceptions represented by EXCEPTS.  It is the responsibility of
     the OS to ensure that if multiple exceptions occur they are fed back
     to this process in the proper way; this can happen in hardware,
     anyway (in particular, inexact with overflow or underflow). */

  /* Get the current state.  */
  u.fenv = fegetenv_register ();

  /* Add the exceptions */
  u.l[1] = (u.l[1]
	    | excepts & FPSCR_STICKY_BITS
	    /* Turn FE_INVALID into FE_INVALID_SOFTWARE.  */
	    | (excepts >> (31 - FPSCR_VX) - (31 - FPSCR_VXSOFT)
	       & FE_INVALID_SOFTWARE));

  /* Store the new status word (along with the rest of the environment),
     triggering any appropriate exceptions.  */
  fesetenv_register (u.fenv);

  if ((excepts & FE_INVALID)
      /* For some reason, some PowerPC chips (the 601, in particular)
	 don't have FE_INVALID_SOFTWARE implemented.  Detect this
	 case and raise FE_INVALID_SNAN instead.  */
      && !fetestexcept (FE_INVALID))
    set_fpscr_bit (FPSCR_VXSNAN);
}
