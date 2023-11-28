/* Copyright (C) 1992, 1993, 1996, 1997, 1998 Free Software Foundation, Inc.
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

#include <termios.h>
#include <errno.h>
#include <stddef.h>

struct speed_struct
{
  speed_t value;
  speed_t internal;
};

static const struct speed_struct speeds[] =
  {
#ifdef B0
    { 0, B0 },
#endif
#ifdef B50
    { 50, B50 },
#endif
#ifdef B75
    { 75, B75 },
#endif
#ifdef B110
    { 110, B110 },
#endif
#ifdef B134
    { 134, B134 },
#endif
#ifdef B150
    { 150, B150 },
#endif
#ifdef B200
    { 200, B200 },
#endif
#ifdef B300
    { 300, B300 },
#endif
#ifdef B600
    { 600, B600 },
#endif
#ifdef B1200
    { 1200, B1200 },
#endif
#ifdef B1200
    { 1200, B1200 },
#endif
#ifdef B1800
    { 1800, B1800 },
#endif
#ifdef B2400
    { 2400, B2400 },
#endif
#ifdef B4800
    { 4800, B4800 },
#endif
#ifdef B9600
    { 9600, B9600 },
#endif
#ifdef B19200
    { 19200, B19200 },
#endif
#ifdef B38400
    { 38400, B38400 },
#endif
#ifdef B57600
    { 57600, B57600 },
#endif
#ifdef B76800
    { 76800, B76800 },
#endif
#ifdef B115200
    { 115200, B115200 },
#endif
#ifdef B153600
    { 153600, B153600 },
#endif
#ifdef B230400
    { 230400, B230400 },
#endif
#ifdef B307200
    { 307200, B307200 },
#endif
#ifdef B460800
    { 460800, B460800 },
#endif
  };


/* Set both the input and output baud rates stored in *TERMIOS_P to SPEED.  */
int
cfsetspeed (struct termios *termios_p, speed_t speed)
{
  size_t cnt;

  for (cnt = 0; cnt < sizeof (speeds) / sizeof (speeds[0]); ++cnt)
    if (speed == speeds[cnt].internal)
      {
	cfsetispeed (termios_p, speed);
	cfsetospeed (termios_p, speed);
	return 0;
      }
    else if (speed == speeds[cnt].value)
      {
	cfsetispeed (termios_p, speeds[cnt].internal);
	cfsetospeed (termios_p, speeds[cnt].internal);
	return 0;
      }

  __set_errno (EINVAL);

  return -1;
}
