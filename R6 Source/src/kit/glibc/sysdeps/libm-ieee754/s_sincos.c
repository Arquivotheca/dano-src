/* Compute sine and cosine of argument.
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

#include <math.h>

#include "math_private.h"


void
__sincos (double x, double *sinx, double *cosx)
{
  int32_t ix;

  /* High word of x. */
  GET_HIGH_WORD (ix, x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if (ix <= 0x3fe921fb)
    {
      *sinx = __kernel_sin (x, 0.0, 0);
      *cosx = __kernel_cos (x, 0.0);
    }
  else if (ix>=0x7ff00000)
    {
      /* sin(Inf or NaN) is NaN */
      *sinx = *cosx = x - x;
    }
  else
    {
      /* Argument reduction needed.  */
      double y[2];
      int n;

      n = __ieee754_rem_pio2 (x, y);
      switch (n & 3)
	{
	case 0:
	  *sinx = __kernel_sin (y[0], y[1], 1);
	  *cosx = __kernel_cos (y[0], y[1]);
	  break;
	case 1:
	  *sinx = __kernel_cos (y[0], y[1]);
	  *cosx = -__kernel_sin (y[0], y[1], 1);
	  break;
	case 2:
	  *sinx = -__kernel_sin (y[0], y[1], 1);
	  *cosx = -__kernel_cos (y[0], y[1]);
	  break;
	default:
	  *sinx = -__kernel_cos (y[0], y[1]);
	  *cosx = __kernel_sin (y[0], y[1], 1);
	  break;
	}
    }
}
weak_alias (__sincos, sincos)
#ifdef NO_LONG_DOUBLE
strong_alias (__sincos, __sincosl)
weak_alias (__sincos, sincosl)
#endif
