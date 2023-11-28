/* termios type and macro definitions.  4.4 BSD/generic GNU version.
   Copyright (C) 1993, 1994, 1996, 1997 Free Software Foundation, Inc.
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

#ifndef _TERMIOS_H
# error "Never include <bits/termios.h> directly; use <termios.h> instead."
#endif

/* These macros are also defined in some <bits/ioctls.h> files (with
   numerically identical values), but this serves to shut up cpp's
   complaining. */
#ifdef __USE_BSD

# ifdef MDMBUF
#  undef MDMBUF
# endif
# ifdef FLUSHO
#  undef FLUSHO
# endif
# ifdef PENDIN
#  undef PENDIN
# endif

#endif /* __USE_BSD */

#ifdef ECHO
# undef ECHO
#endif
#ifdef TOSTOP
# undef TOSTOP
#endif
#ifdef NOFLSH
# undef NOFLSH
#endif


/* These definitions match those used by the 4.4 BSD kernel.
   If the operating system has termios system calls or ioctls that
   correctly implement the POSIX.1 behavior, there should be a
   system-dependent version of this file that defines `struct termios',
   `tcflag_t', `cc_t', `speed_t' and the `TC*' constants appropriately.  */

/* Type of terminal control flag masks.  */
typedef unsigned long int tcflag_t;

/* Type of control characters.  */
typedef unsigned char cc_t;

/* Type of baud rate specifiers.  */
typedef unsigned char speed_t;

/* Terminal control structure.  */
struct termios
{
  /* Input modes.  */
  tcflag_t c_iflag;
#define	IGNBRK	(1 << 0)	/* Ignore break condition.  */
#define	BRKINT	(1 << 1)	/* Signal interrupt on break.  */
#define	IGNPAR	(1 << 2)	/* Ignore characters with parity errors.  */
#define	PARMRK	(1 << 3)	/* Mark parity and framing errors.  */
#define	INPCK	(1 << 4)	/* Enable input parity check.  */
#define	ISTRIP	(1 << 5)	/* Strip 8th bit off characters.  */
#define	INLCR	(1 << 6)	/* Map NL to CR on input.  */
#define	IGNCR	(1 << 7)	/* Ignore CR.  */
#define	ICRNL	(1 << 8)	/* Map CR to NL on input.  */
#define IUCLC	(1 << 9)	/* Map all upper case to lower case.  */
#define	IXON	(1 << 10)	/* Enable start/stop output control.  */
#ifdef	__USE_BSD
#define	IXANY	(1 << 11)	/* Any character will restart after stop.  */
#endif
#define	IXOFF	(1 << 12)	/* Enable start/stop input control.  */

  /* Output modes.  */
  tcflag_t c_oflag;
#define	OPOST	(1 << 0)	/* Perform output processing.  */
#define OLCUC	(1 << 1)	/* Map lower case to upper case.  */
#ifdef	__USE_BSD
# define ONLCR	(1 << 2)	/* Map NL to CR-NL on output.  */
#endif
#define OCRNL	(1 << 3)	/* Map CR to NL on output.  */
#define ONOCR	(1 << 4)	/* No CR output when in column 0.  */
#define ONLRET	(1 << 5)	/* Newline performs CR functions.  */
#define OFILL	(1 << 6)	/* Uses fill characters for delay.  */
#define OFDEL	(1 << 7)	/* Fills are DEL, otherwise NUL.  */

#define	NLDLY	0x0100		/* Newline delays: */
#define	NL0	0
#define	NL1	0x0100
#define	CRDLY	0x0600		/* Carriage return delays: */
#define	CR0	0x0000
#define	CR1	0x0200
#define	CR2	0x0400
#define	CR3	0x0600
#define	TABDLY	0x1800		/* Tab delays: */
#define	TAB0	0x0000
#define	TAB1	0x0800
#define	TAB2	0x1000
#define	TAB3	0x1800
#define	BSDLY	0x2000		/* Backspace delays: */
#define	BS0	0x0000
#define	BS1	0x2000
#define	VTDLY	0x4000		/* Vertical tab delays: */
#define	VT0	0x0000
#define	VT1	0x4000
#define	FFDLY	0x8000		/* Form feed delays: */
#define	FF0	0x0000
#define	FF1	0x8000


  /* Control modes.  */
  tcflag_t c_cflag;
#ifdef	__USE_BSD
# define CIGNORE	(1 << 0)	/* Ignore these control flags.  */
#endif
#define	CSIZE	(CS5|CS6|CS7|CS8)	/* Number of bits per byte (mask).  */
#define	CS5	0		/* 5 bits per byte.  */
#define	CS6	0		/* 6 bits per byte.  */
#define	CS7	0		/* 7 bits per byte.  */
#define	CS8	0x20		/* 8 bits per byte.  */
#define	CSTOPB	(1 << 6)	/* Two stop bits instead of one.  */
#define	CREAD	(1 << 7)	/* Enable receiver.  */
#define	PARENB	(1 << 8)	/* Parity enable.  */
#define	PARODD	(1 << 9)	/* Odd parity instead of even.  */
#define	HUPCL	(1 << 10)	/* Hang up on last close.  */
#define	CLOCAL	(1 << 11)	/* Ignore modem status lines.  */
#ifdef __USE_MISC
# define XLOBLK	(1 << 12)	/* Block layer output?  */
# define CTSFLOW (1 << 13)	/* Enable CTS flow.  */
# define RTSFLOW (1 << 14)	/* Enable RTS flow.  */
# define CRTSCTS (RTSFLOW|CTSFLOW)
#endif

  /* Local modes.  */
  tcflag_t c_lflag;
#define	_ISIG	(1 << 0)	/* Enable signals.  */
#define ISIG	_ISIG
#define	_ICANON	(1 << 1)	/* Do erase and kill processing.  */
#define	ICANON	_ICANON
#ifdef __USE_MISC
# define XCASE	(1 << 2)	/* Canonical u/l case conversion.  */
#endif
#define	_ECHO	(1 << 3)	/* Enable echo.  */
#define	ECHO	_ECHO
#define	_ECHOE	(1 << 4)	/* Visual erase for ERASE.  */
#define	ECHOE	_ECHOE
#define	_ECHOK	(1 << 5)	/* Echo NL after KILL.  */
#define	ECHOK	_ECHOK
#define	_ECHONL	(1 << 6)	/* Echo NL even if ECHO is off.  */
#define	ECHONL	_ECHONL
#define	_NOFLSH	(1 << 7)	/* Disable flush after interrupt.  */
#define	NOFLSH	_NOFLSH
#define	_TOSTOP	(1 << 8)	/* Send SIGTTOU for background output.  */
#define	TOSTOP	_TOSTOP
#define	_IEXTEN	(1 << 9)	/* Enable DISCARD and LNEXT.  */
#define	IEXTEN	_IEXTEN

  /* Line dicipline.  */
  char c_line;

  /* Unused.  */
  speed_t c_ixxxxx;
  speed_t c_oxxxxx;

  /* Control characters.  */
#define	VINTR	0		/* Interrupt character [ISIG].  */
#define	VQUIT	1		/* Quit character [ISIG].  */
#define	VERASE	2		/* Erase character [ICANON].  */
#define	VKILL	3		/* Kill-line character [ICANON].  */
#define	VEOF	4		/* End-of-file character [ICANON].  */
#define	VEOL	5		/* End-of-line character [ICANON].  */
#define	VMIN	4		/* Minimum number of bytes read at once [!ICANON].  */
#define	VTIME	5		/* Time-out value (tenths of a second) [!ICANON].  */
#ifdef	__USE_BSD
# define VEOL2	6		/* Second EOL character [ICANON].  */
#endif
#define VSWTCH	7
#define	VSTART	8		/* Start (X-ON) character [IXON, IXOFF].  */
#define	VSTOP	9		/* Stop (X-OFF) character [IXON, IXOFF].  */
#define	VSUSP	10		/* Suspend character [ISIG].  */
#define	NCCS	11		/* Value duplicated in <hurd/tioctl.defs>.  */
#ifdef __USE_MISC
/* Be-specific name.  */
# define NCC	NCCS
#endif
  cc_t c_cc[NCC];

  /* Input and output baud rates.  */
  speed_t __ispeed, __ospeed;
#ifdef __USE_MISC
# define CBAUD	0x1f
#endif
#define	B0	0x00		/* Hang up.  */
#define	B50	0x01		/* 50 baud.  */
#define	B75	0x02		/* 75 baud.  */
#define	B110	0x03		/* 110 baud.  */
#define	B134	0x04		/* 134.5 baud.  */
#define	B150	0x05		/* 150 baud.  */
#define	B200	0x06		/* 200 baud.  */
#define	B300	0x07		/* 300 baud.  */
#define	B600	0x08		/* 600 baud.  */
#define	B1200	0x09		/* 1200 baud.  */
#define	B1800	0x0a		/* 1800 baud.  */
#define	B2400	0x0b		/* 2400 baud.  */
#define	B4800	0x0c		/* 4800 baud.  */
#define	B9600	0x0d		/* 9600 baud.  */
#define	B19200	0x0e		/* 19200 baud.  */
#define	B38400	0x0f		/* 38400 baud.  */
#ifdef	__USE_BSD
# define EXTA	B19200
# define EXTB	B38400
#endif
#define B57600	0x10		/* 57600 baud.  */
#define B115200	0x11		/* 115200 baud.  */
#define B230400	0x12		/* 230400 baud.  */
#define B31250	0x13		/* 31250 baud, for MIDI.  */
};

/* Values for the OPTIONAL_ACTIONS argument to `tcsetattr'.  */
#define	TCSANOW		1	/* Change immediately.  */
#define	TCSADRAIN	2	/* Change when pending output is written.  */
#define	TCSAFLUSH	4	/* Flush pending input before changing.  */

/* Values for the QUEUE_SELECTOR argument to `tcflush'.  */
#define	TCIFLUSH	1	/* Discard data received but not yet read.  */
#define	TCOFLUSH	2	/* Discard data written but not yet sent.  */
#define	TCIOFLUSH	3	/* Discard all pending data.  */

/* Values for the ACTION argument to `tcflow'.  */
#define	TCOOFF	0x01		/* Suspend output.  */
#define	TCOON	0x02		/* Restart suspended output.  */
#define	TCIOFF	0x04		/* Send a STOP character.  */
#define	TCION	0x08		/* Send a START character.  */

#define termio termios   /* for old non-posix code */

#include <sys/ioctl.h>			/* Everyone uses this; include it anyway */
