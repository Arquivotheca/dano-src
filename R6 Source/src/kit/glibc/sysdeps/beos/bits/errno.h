/* Error constants.  BeOS specific version.
   Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
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

#ifdef _ERRNO_H

# include <Errors.h>

# define ENOERR         0
# define EOK 			ENOERR  /* some code assumes EOK exists */

# ifndef __ASSEMBLER__
/* We now need a declaration of the `errno' variable.  */
extern int errno;

/* Function to get address of global `errno' variable.  */
extern int *_errnop __P ((void)) __attribute__ ((__const__));

#  if defined _LIBC
/* We wouldn't need a special macro anymore but it is history.  */
#   define __set_errno(val) (*_errnop ()) = (val)
#  endif /* _LIBC */

#  if !defined _LIBC || defined _LIBC_REENTRANT
/* When using threads, errno is a per-thread value.  */
#   define errno (*_errnop ())
#  endif
# endif /* !__ASSEMBLER__ */
#endif /* _ERRNO_H */

#if !defined _ERRNO_H && defined __need_Emath
/* This is ugly but the kernel header is not clean enough.  We must
   define only the values EDOM and ERANGE in case __need_Emath is
   defined.  The value is the same for all Linux ports.  */
# define EDOM	(-2147454960)	/* Math argument out of domain of function.  */
# define EILSEQ	(-2147454938)	/* Illegal byte sequence.  */
# define ERANGE	(-2147454959)	/* Math result not representable.  */
#endif /* !_ERRNO_H && __need_Emath */
