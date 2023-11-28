/* Copyright (C) 1996 Free Software Foundation, Inc.
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

#ifndef	_SYS_IO_H

#define	_SYS_IO_H	1
#include <features.h>

__BEGIN_DECLS

/* Get constants from kernel header files. */
#include <asm/io.h>

/* If TURN_ON is TRUE, request for permission to do direct i/o on the
   port numbers in the range [FROM,FROM+NUM-1].  Otherwise, turn I/O
   permission off for that range.  This call requires root privileges.

   Portability note: not all Linux platforms support this call.  Most
   platforms based on the PC I/O architecture probably will, however.
   E.g., Linux/Alpha for Alpha PCs supports this.  */
extern int ioperm __P ((unsigned long int __from, unsigned long int __num,
			int __turn_on));

/* Set the I/O privilege level to LEVEL.  If LEVEL>3, permission to
   access any I/O port is granted.  This call requires root
   privileges. */
extern int iopl __P ((int __level));

/* Return the physical address of the DENSE I/O memory or NULL if none
   is available (e.g. on a jensen).  */
extern unsigned long _bus_base __P ((void)) __attribute__ ((const));
extern unsigned long bus_base __P ((void)) __attribute__ ((const));

/* Return the physical address of the SPARSE I/O memory.  */
extern unsigned long _bus_base_sparse __P ((void)) __attribute__ ((const));
extern unsigned long bus_base_sparse __P ((void)) __attribute__ ((const));

/* Return the HAE shift used by the SPARSE I/O memory.  */
extern int _hae_shift __P ((void)) __attribute__ ((const));
extern int hae_shift __P ((void)) __attribute__ ((const));

/* Access PCI space protected from machine checks.  */
extern int pciconfig_read __P ((unsigned long int __bus,
				unsigned long int __dfn,
				unsigned long int __off,
				unsigned long int __len,
				unsigned char *__buf));

extern int pciconfig_write __P ((unsigned long int __bus,
				 unsigned long int __dfn,
				 unsigned long int __off,
				 unsigned long int __len,
				 unsigned char *__buf));

__END_DECLS

#endif /* _SYS_IO_H */
