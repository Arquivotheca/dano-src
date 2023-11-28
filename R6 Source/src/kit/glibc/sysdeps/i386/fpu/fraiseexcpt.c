/* Raise given exceptions.
   Copyright (C) 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

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

#include <fenv.h>
#include <math.h>

void
feraiseexcept (int excepts)
{
  /* Raise exceptions represented by EXPECTS.  But we must raise only
     one signal at a time.  It is important that if the overflow/underflow
     exception and the inexact exception are given at the same time,
     the overflow/underflow exception follows the inexact exception.  */

  /* First: invalid exception.  */
  if ((FE_INVALID & excepts) != 0)
    {
      /* One example of a invalid operation is 0.0 / 0.0.  */
      double d;
      __asm__ __volatile__ ("fldz; fdiv %%st, %%st(0); fwait" : "=t" (d));
      (void) &d;
    }

  /* Next: division by zero.  */
  if ((FE_DIVBYZERO & excepts) != 0)
    {
      double d;
      __asm__ __volatile__ ("fldz; fld1; fdivp %%st, %%st(1); fwait"
			    : "=t" (d));
      (void) &d;
    }

  /* Next: overflow.  */
  if ((FE_OVERFLOW & excepts) != 0)
    {
      /* There is no way to raise only the overflow flag.  Do it the
	 hard way.  */
      fenv_t temp;

      /* Bah, we have to clear selected exceptions.  Since there is no
	 `fldsw' instruction we have to do it the hard way.  */
      __asm__ __volatile__ ("fnstenv %0" : "=m" (*&temp));

      /* Set the relevant bits.  */
      temp.status_word |= FE_OVERFLOW;

      /* Put the new data in effect.  */
      __asm__ __volatile__ ("fldenv %0" : : "m" (*&temp));

      /* And raise the exception.  */
      __asm__ __volatile__ ("fwait");
    }

  /* Next: underflow.  */
  if ((FE_UNDERFLOW & excepts) != 0)
    {
      /* There is no way to raise only the underflow flag.  Do it the
	 hard way.  */
      fenv_t temp;

      /* Bah, we have to clear selected exceptions.  Since there is no
	 `fldsw' instruction we have to do it the hard way.  */
      __asm__ __volatile__ ("fnstenv %0" : "=m" (*&temp));

      /* Set the relevant bits.  */
      temp.status_word |= FE_UNDERFLOW;

      /* Put the new data in effect.  */
      __asm__ __volatile__ ("fldenv %0" : : "m" (*&temp));

      /* And raise the exception.  */
      __asm__ __volatile__ ("fwait");
    }

  /* Last: inexact.  */
  if ((FE_INEXACT & excepts) != 0)
    {
      /* There is no way to raise only the inexact flag.  Do it the
	 hard way.  */
      fenv_t temp;

      /* Bah, we have to clear selected exceptions.  Since there is no
	 `fldsw' instruction we have to do it the hard way.  */
      __asm__ __volatile__ ("fnstenv %0" : "=m" (*&temp));

      /* Set the relevant bits.  */
      temp.status_word |= FE_INEXACT;

      /* Put the new data in effect.  */
      __asm__ __volatile__ ("fldenv %0" : : "m" (*&temp));

      /* And raise the exception.  */
      __asm__ __volatile__ ("fwait");
    }
}
