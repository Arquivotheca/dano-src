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

#ifndef _SYS_IOCTL_H
# error "Never use <bits/ioctls.h> directly; include <sys/ioctl.h> instead."
#endif

/* Identifiers to control serial driver.  */
#define TCGETA		 0x8000
#define TCSETA		 0x8001
#define TCSETAF		 0x8002
#define TCSETAW		 0x8003
#define TCWAITEVENT	 0x8004
#define TCSBRK		 0x8005
#define TCFLSH		 0x8006
#define TCXONC		 0x8007
#define TCQUERYCONNECTED 0x8008
#define TCGETBITS	 0x8009
#define TCSETDTR	 0x800a
#define TCSETRTS	 0x800b
#define TIOCGWINSZ	 0x800c	/* Pass in a struct winsize.  */
#define TIOCSWINSZ	 0x800d	/* Pass in a struct winsize.  */

/* Bits for TCGETBITS.  */
#define TCGB_CTS	 0x01
#define TCGB_DSR	 0x02
#define TCGB_RI		 0x04
#define TCGB_DCD	 0x08

/* Event codes.  Returned from TCWAITEVENT.  */
#define EV_RING		0x0001
#define EV_BREAK	0x0002
#define EV_CARRIER	0x0004
#define EV_CARRIERLOST	0x0008
