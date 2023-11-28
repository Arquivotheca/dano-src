

#include	<OS.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<termios.h>
#include	<errno.h>
#include	<string.h>
#include	<stdarg.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<ctype.h>
#include	"rico.h"

#define min(a,b) ((a)>(b)?(b):(a))
#define max(a,b) ((a)>(b)?(a):(b))

static termios	tline,
		tactive,
		tstdin;
static thread_id
		l2o;
static bool	ending;
static	int linefd,
		done( bool);
static bool	init( char *[]),
		process( ),
		openline( char *),
		setline( ),
		shutdown( ),
		readstr( int, char [], uint),
		isopen( ),
		usage( ),
		help( ),
		complain( char *, ...);
static char	*strcatquote( char *, char *);
static void	line2stdout( ),
		stdin2line( ),
		dial( char *[]),
		closeline( ),
		trestore( ),
		parse( char *, char *[], uint),
		iusage( ),
		ihelp( );
static uint	baudconv( uint);
extern "C" char	**environ;

static struct {
	uint	baud;
	char	parity;
	bool	seven,
		two,
		flow;
} line = {
	19200, FALSE, FALSE, 'n', FALSE
};

int
main( int, char *argv[])
{

	unless (init( argv))
		return (done( FALSE));
	if (process( ))
		return (done( shutdown( )));
	trestore( );
	return (done( FALSE));
}


static bool
init( char **av)
{
	char	*p;

	++av;
	loop {
		unless (p = *av++)
			return (usage( ));
		unless (*p == '-')
			break;
		++p;
		while (*p)
			switch (uint c = *p++) {
			case 'b':
				unless (*av)
					return (complain( "-b requires baud rate"));
				line.baud = atoi( *av++);
				break;
			case 'e':
			case 'o':
				line.parity = c;
				break;
			case '7':
				line.seven = TRUE;
				break;
			case '2':
				line.two = TRUE;
				break;
			case 'f':
				line.flow = TRUE;
				break;
			case 'h':
				return (help( ));
			default:
				return (usage( ));
			}
	}
	if (*av)
		return (usage( ));
	unless ((streq( p, "none"))
	or (openline( p)))
		return (FALSE);

	tcgetattr( 0, &tstdin);
	tactive = tstdin;
	tactive.c_iflag &= ~ ICRNL;
	tactive.c_oflag &= ~ ONLCR;
	tactive.c_lflag &= ~ (ISIG|ECHO|ICANON);
	tcsetattr( 0, TCSANOW, &tactive);

	return (TRUE);
}


static bool
process( )
{
	long	z;

	thread_id i2l = spawn_thread( (thread_entry)stdin2line, "stdin2line", B_NORMAL_PRIORITY, 0);
	resume_thread( i2l);
	wait_for_thread( i2l, &z);
	closeline( );
	return (TRUE);
}


static void
stdin2line( )
{
	char	c,
		s[200],
		*av[7];

	while (read( 0, &c, 1) == 1)
		switch (c) {
		case '\1':
			unless (read( 0, &c, 1) == 1)
				return;
			switch (c) {
			case ':':
				unless (readstr( 0, s, sizeof s))
					return;
				switch (s[0]) {
				case 'c':
					closeline( );
					openline( s+1);
					break;
				case 'd':
					closeline( );
					parse( s+1, av, nel( av));
					if (openline( av[0]))
						dial( av+1);
					break;
				case 'z':
					closeline( );
					break;
				case 'b':
					line.baud = atoi( s+1);
					setline( );
					break;
				case 'N':
				case 'E':
				case 'O':
					line.parity = tolower( s[0]);
					setline( );
					break;
				case '8':
					line.seven = FALSE;
					setline( );
					break;
				case '7':
					line.seven = TRUE;
					setline( );
					break;
				case '1':
					line.two = FALSE;
					setline( );
					break;
				case '2':
					line.two = TRUE;
					setline( );
					break;
				case 'n':
					line.flow = FALSE;
					setline( );
					break;
				case 'h':
					line.flow = TRUE;
					setline( );
				}
				break;
			case 'q':
				return;
			case '$':
				sprintf( s, "PARROT_FD=%d", linefd);
				putenv( s);
				tcsetattr( 0, TCSANOW, &tstdin);
				fprintf( stderr, "Starting sub-shell (serial line fd = %d)...\n", linefd);
				system( "/bin/sh");
				tcsetattr( 0, TCSANOW, &tactive);
				break;
			case 'B':
				if (isopen( ))
					ioctl( linefd, TCSBRK);
				break;
			case 'h':
				ihelp( );
				break;
			default:
				iusage( );
			}
			break;
		default:
			if (isopen( ))
				write( linefd, &c, 1);
			else
				complain( "\7Not connected to any device!");
		}
}


static void
line2stdout( )
{
	char	buf[512];

	loop {
		int i = ioctl( linefd, TCWAITEVENT, 0);
		if ((i < 0)
		or (ending))
			break;
		if (i) {
			i = read( linefd, buf, min( i, sizeof buf));
			if (i <= 0)
				break;
			write( 1, buf, i);
		}
	}
}


static void
dial( char *av[])
{
	char	s[1000];
	long	z;

	printf( "\r\nScript begins...\r\n");
	ending = TRUE;
	wait_for_thread( l2o, &z);
	uint i = 0;
	strcpy( s, av[i++]);
	while (av[i]) {
		strcat( s, " '");
		strcatquote( s, av[i++]);
		strcat( s, "'");
	}
	int myfd0 = dup( 0);
	int myfd1 = dup( 1);
	close( 0);
	close( 1);
	dup( linefd);
	dup( linefd);
	fcntl( myfd0, F_SETFD, FD_CLOEXEC);
	fcntl( myfd1, F_SETFD, FD_CLOEXEC);
        system( s);
	dup2( myfd0, 0);
	dup2( myfd1, 1);
	close( myfd0);
	close( myfd1);
	ending = FALSE;
	l2o = spawn_thread( (thread_entry)line2stdout, "line2stdout", B_NORMAL_PRIORITY, 0);
	resume_thread( l2o);
	printf( "\r\nScript complete.  Returning to interactive mode.\r\n");
}


static bool
openline( char *p)
{

	unless (linefd = atoi( p)) {
		char n[100];
		n[0] = 0;
		unless (*p == '/')
			strcat( n, "/dev/ports/");
		strcat( n, p);
		linefd = open( n, O_RDWR|O_NONBLOCK|O_EXCL);
		if (linefd < 0)
			return (complain( "can't open %s (%s)", n, strerror( errno)));
		fcntl( linefd, F_SETFL, fcntl( linefd, F_GETFL)&~O_NONBLOCK);
	}
	unless (setline( )) {
		close( linefd);
		linefd = 0;
		return (FALSE);
	}
	ending = FALSE;
	l2o = spawn_thread( (thread_entry)line2stdout, "line2stdout", B_NORMAL_PRIORITY, 0);
	resume_thread( l2o);
	return (TRUE);
}


static bool
setline( )
{
	static termios	tnull;
	termios		t;

	if (isopen( )) {
		if ((tcgetattr( linefd, &tline) < 0)
		or (memcmp( &tline, &tnull, sizeof tline) == 0))
			return (complain( "device is not a serial line"));
		t = tline;
		if (baudconv( line.baud) == B0)
			return (complain( "illegal baud rate %d", line.baud));
		t.c_cflag &= ~ (CBAUD|CSIZE|PARENB|CRTSCTS);
		t.c_cflag |= baudconv( line.baud) | CLOCAL;
		t.c_cflag |= line.seven? CS7: CS8;
		switch (line.parity) {
		case 'e':
			t.c_cflag |= PARENB;
			break;
		case 'o':
			t.c_cflag |= PARENB | PARODD;
		}
		if (line.flow)
			t.c_cflag |= CRTSCTS;
		t.c_cc[VTIME] = 10;
		t.c_cc[VMIN] = 1;
		tcsetattr( linefd, TCSANOW, &t);
	}
	return (TRUE);
}


static void
closeline( )
{
	const bool	false_var	= FALSE;
	long		z;

	if (isopen( )) {
		ioctl( linefd, TCSETDTR, &false_var);
		ending = TRUE;
		wait_for_thread( l2o, &z);
		close( linefd);
		linefd = 0;
	}
}


static bool
shutdown( )
{

	trestore( );
	return (TRUE);
}


static void
trestore( )
{

	if (isopen( ))
		tcsetattr( linefd, TCSANOW, &tline);
	tcsetattr( 0, TCSANOW, &tstdin);
}


static int
done( bool ok)
{

	return (ok? 0: 1);
}


static bool
isopen( )
{

	return (linefd > 0);
}


static uint
baudconv( uint b)
{

	switch (b) {
	case 50:
		return (B50);
	case 75:
		return (B75);
	case 110:
		return (B110);
	case 134:
		return (B134);
	case 150:
		return (B150);
	case 200:
		return (B200);
	case 300:
		return (B300);
	case 600:
		return (B600);
	case 1200:
		return (B1200);
	case 1800:
		return (B1800);
	case 2400:
		return (B2400);
	case 4800:
		return (B4800);
	case 9600:
		return (B9600);
	case 19200:
		return (B19200);
	case 38400:
		return (B38400);
	case 57600:
		return (B57600);
	case 115200:
		return (B115200);
	case 230400:
		return (B230400);
	case 31250:
		return (B31250);
	}
	return (B0);
}


static bool
readstr( int fd, char buf[], uint n)
{
	char	c;

	uint i = 0;
	while (read( fd, &c, 1) == 1) {
		if (c == '\n') {
			buf[i] = 0;
			return (TRUE);
		}
		if (i < n-1)
			buf[i++] = c;
	}
	return (FALSE);
}


static void
parse( char *s, char *a[], uint n)
{
	uint	i;

	for (i=0; i<n-1; ++i) {
		a[i] = s;
		if (char *p = strchr( s, '\1')) {
			s = p;
			*s++ = 0;
		}
	}
	a[i] = 0;
}


/*
 * Like strcat( ), but escape the  '  shell quote character.
 */
static char	*
strcatquote( char *dst, char *src)
{

	char *p = strchr( dst, 0);
	loop
		switch (int c = *src++) {
		case 0:
			*p = 0;
			return (dst);
		case '\'':
			*p++ = '\'';
			*p++ = '\\';
			*p++ = '\'';
			*p++ = '\'';
			break;
		default:
			*p++ = c;
		}
}


static void
iusage( )
{

	fprintf( stderr, "\r\nparrot: no such command (try   ^A h   for help)\r\n");
}


static void
ihelp( )
{

	fprintf( stderr, "Interactive commands:\r\n");
	fprintf( stderr, "	q	Quit from parrot\r\n");
	fprintf( stderr, "	B	Send break\r\n");
	fprintf( stderr, "	$	Invoke sub-shell\r\n");
}


static bool
usage( )
{

	fprintf( stderr, "usage: parrot [ -help ] [ options ] device\n");
	return (FALSE);
}


static bool
help( )
{

	fprintf( stderr, "Usage:\n");
	fprintf( stderr, "	parrot [ -help ] [ options ] device\n");
	fprintf( stderr, "Options:\n");
	fprintf( stderr, "	-b B	set baud rate to B (default is 19200)\n");
	fprintf( stderr, "	-e	use even parity (default is no parity)\n");
	fprintf( stderr, "	-o	use odd parity (default is no parity)\n");
	fprintf( stderr, "	-7	use 7 data bits (default is 8)\n");
	fprintf( stderr, "	-2	use 2 stop bits (default is 1)\n");
	fprintf( stderr, "	-f	use RTS/CTS flow control (default is none)\n");
	fprintf( stderr, "Baud rates:\n");
	fprintf( stderr, "	50 75 110 134 150 200 300 600 1200 1800 2400 4800 9600 19200 38400\n");
	fprintf( stderr, "	57600 115200 230400 31250\n");
	fprintf( stderr, "Device:\n");
	fprintf( stderr, "	Any serial device can be used, e.g. \"serial2\".\n");
	fprintf( stderr, "Description:\n");
	fprintf( stderr, "	`parrot' connects to the specified serial device, allow the user\n");
	fprintf( stderr, "	to interactively operate a remote device, such as a modem.  The\n");
	fprintf( stderr, "	user may invoke a special action by typing two keys, the first being\n");
	fprintf( stderr, "	Ctrl-A.  The possible actions are\n");
	fprintf( stderr, "		^A q	Quit from `parrot'\n");
	fprintf( stderr, "		^A B	Send a break\n");
	fprintf( stderr, "		^A $	Invoke sub-shell\n");
	fprintf( stderr, "		^A h	Print help message\n");
	fprintf( stderr, "	When a sub-shell is invoked, it retains the file descriptor to\n");
	fprintf( stderr, "	the open serial device.  This permits activities, like file\n");
	fprintf( stderr, "	downloading, by other programs.  The file descriptor is recorded\n");
	fprintf( stderr, "	in the shell variable PARROT_FD.  `parrot' will not read from the\n");
	fprintf( stderr, "	line until the sub-shell exits.\n");
	return (FALSE);
}


static bool
complain( char *mesg, ...)
{
	va_list	ap;

	fprintf( stderr, "parrot: ");
	va_start( ap, mesg);
	vfprintf( stderr, mesg, ap);
	fprintf( stderr, "\r\n");
	return (FALSE);
}
