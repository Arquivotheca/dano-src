/* Compute positive difference.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.
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

#include <sysdep.h>

	.text
ENTRY(__fdiml)
	fldt	4(%esp)		// x
	fldt	16(%esp)	// x : y

	fucomi	%st(1), %st
	jp	1f

	fsubp	%st(1), %st
	fldz
	fcomi	%st(1), %st
	fcmovb	%st(1), %st
	jmp	2f

1:	fucomi	%st(0), %st
	fcmovnu	%st(1), %st
2:	fstp	%st(1)
	ret
END(__fdiml)
weak_alias (__fdiml, fdiml)
