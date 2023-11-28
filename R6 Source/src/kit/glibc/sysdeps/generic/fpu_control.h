/* FPU control word definitions.  Stub version.
   Copyright (C) 1996, 1997 Free Software Foundation, Inc.
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

#ifndef _FPU_CONTROL_H
#define _FPU_CONTROL_H

#define _FPU_RESERVED ???	/* These bits are reserved are not changed.  */

/* The fdlibm code requires no interrupts for exceptions.  Don't
   change the rounding mode, it would break long double I/O!  */
#define _FPU_DEFAULT  0x00000000 /* Default value.  */

/* Type of the control word.  */
typedef unsigned int fpu_control_t __attribute__ ((__mode__ (__?I__)));

/* Macros for accessing the hardware control word.  */
#define _FPU_GETCW(cw) __asm__ ("fetch fpu control word into %0" : "=g" (cw))
#define _FPU_SETCW(cw) __asm__ ("set fpu control word to %0" : : "g" (cw))

/* Default control word set at startup.  */
extern fpu_control_t __fpu_control;

__BEGIN_DECLS

/* Called at startup.  It can be used to manipulate fpu control register.  */
extern void __setfpucw __P ((fpu_control_t));

__END_DECLS

#endif /* _FPU_CONTROL_H */
