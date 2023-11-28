

#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>
#include	<Looper.h>
#include	<Message.h>
#include	<termios.h>
#include	<string.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	"rico.h"
#include	"RNode.h"
#include	"Shell.h"


const static struct termios TDEFAULT = {
	ICRNL,				/* c_iflag */
	OPOST|ONLCR,			/* c_oflag */
	B19200|CS8|CREAD|HUPCL,		/* c_cflag */
	ISIG|ICANON|ECHO|ECHOE|ECHONL,	/* c_lflag */
	0,				/* c_line */
	0,				/* c_ixxxxx */
	0,				/* c_oxxxxx */
	ctrl( 'C'),			/* c_cc[VINTR] */
	ctrl( '\\'),			/* c_cc[VQUIT] */
	ctrl( 'H'),			/* c_cc[VERASE] */
	ctrl( 'U'),			/* c_cc[VKILL] */
	ctrl( 'D'),			/* c_cc[VEOF] */
	0,				/* c_cc[VEOL] */
	0,				/* c_cc[VEOL2] */
	0,				/* c_cc[VSWTCH] */
	ctrl( 'S'),			/* c_cc[VSTART] */
	ctrl( 'Q'),			/* c_cc[VSTOP] */
	0				/* c_cc[VSUSP] */
};


uint	narg( char **);


BShell::Obj::Obj( char **com, RNode n1, RNode n2, RNode n3):
Data( n1), Req( n2), Ctl( n3)
{
	char	tty[20],
		pty[20],
		te[40];
	int	ttyfd;

	uint i = 0;
	uint j = 0;
	loop {
		sprintf( pty, "/dev/pt/%c%x", 'p'+i/16, i%16);
		sprintf( tty, "/dev/tt/%c%x", 'p'+i/16, i%16);
		ptyfd = open( pty, O_RDWR);
		unless (ptyfd < 0) {
			ttyfd = open( tty, O_RDWR);
			unless (ttyfd < 0)
				break;
			close( ptyfd);
		}
		if ((++i == ('z'+1-'p')*16)
		or (errno == ENOENT)) {
			if (++j == 10) {
				report( "can't open a pty");
				return;
			}
			snooze( 1000000);
			i = 0;
		}
	}
	sprintf( te, "TTY=%s", tty);
	ioctl( ttyfd, TCSETA, &TDEFAULT);

	int myfd0 = dup( 0);
	int myfd1 = dup( 1);
	int myfd2 = dup( 2);
	close( 0);
	close( 1);
	close( 2);
	dup( ttyfd);
	dup( ttyfd);
	dup( ttyfd);
	fcntl( myfd0, F_SETFD, FD_CLOEXEC);
	fcntl( myfd1, F_SETFD, FD_CLOEXEC);
	fcntl( myfd2, F_SETFD, FD_CLOEXEC);
	fcntl( ptyfd, F_SETFD, FD_CLOEXEC);
	fcntl( ttyfd, F_SETFD, FD_CLOEXEC);
	putenv( "TERM=beterm");
	putenv( te);
	void (*s)( int) = signal( SIGHUP, SIG_DFL);
	thread_id t = load_image( narg( com), (const char **)com, (const char **)environ);
	signal( SIGHUP, s);
	dup2( myfd0, 0);
	dup2( myfd1, 1);
	dup2( myfd2, 2);
	close( myfd0);
	close( myfd1);
	close( myfd2);
	close( ttyfd);
	if (t < 0) {
		report( "can't find the shell");
		return;
	}
	setpgid( t, t);
	resume_thread( t);
	pgid = t;
	ioctl( ptyfd, 'pgid', pgid);
	trlocal = FALSE;
	twlocal = TRUE;
	trmesg = FALSE;
	twmesg = 0;
	signaling = FALSE;

	dinit( );
	rinit( );
	cinit( );
	trinit( );
	twinit( );
}


void
BShell::Obj::sigproc( )
{
	bool	b	= TRUE;

	//printf( "BShell::Obj::sigproc: signaling=%d trlocal=%d twlocal=%d trmesg=%d twmesg=%d\n", signaling, trlocal, twlocal, trmesg, twmesg);
	if (signaling) {
		if ((trlocal)
		and (twlocal)) {
			int i = ioctl( ptyfd, TCFLSH, 1);
			i = ioctl( ptyfd, TCSETDTR, &b);
			fcntl( ptyfd, F_SETFL, fcntl( ptyfd, F_GETFL)&~O_NONBLOCK);
			signaling = FALSE;
			sigproc( );
		}
	}
	else {
		if (twmesg) {
			twsend( twmesg);
			twmesg = 0;
		}
		if (trmesg) {
			trsend( new BMessage);
			trmesg = FALSE;
		}
	}
}


BShell::Data::Data( RNode n):
RNode( n)
{

}


void
BShell::Data::dinit( )
{

	Establish( this, "BShell::Data");
}


void
BShell::Data::dsend( BMessage *m)
{

	*this >> m;
}


void
BShell::Data::MessageReceived( BMessage *m)
{

	//printf( "BShell::Data::MessageReceived: what='%.4s' (0x%X)\n", &m->what, m->what);
	switch (m->what) {
	case EOF:
		ioctl( ptyfd, TCFLSH, 1);
		fcntl( ptyfd, F_SETFL, fcntl( ptyfd, F_GETFL)|O_NONBLOCK);
		break;
	case 'sig2':
		signaling = TRUE;
		fcntl( ptyfd, F_SETFL, fcntl( ptyfd, F_GETFL)|O_NONBLOCK);
		sigproc( );
		break;
	default:
		long n = m->FlattenedSize();
		char *s = (char*) malloc(n);
		m->Flatten( s, n);
		m = new BMessage;
		m->Unflatten( s);
		free( s);
		twmesg = m;
		sigproc( );
	}
}


BShell::Req::Req( RNode n):
RNode( n)
{

}


void
BShell::Req::rinit( )
{

	Establish( this, "BShell::Req");
}


void
BShell::Req::MessageReceived( BMessage *m)
{

	//printf( "BShell::Req::MessageReceived: what='%.4s' (0x%X)\n", &m->what, m->what);
	unless (m->what) {
		trmesg = TRUE;
		sigproc( );
	}
}


void
BShell::Req::rsend( BMessage *m)
{

	*this >> m;
}


BShell::Ctl::Ctl( RNode n):
RNode( n)
{

}


void
BShell::Ctl::cinit( )
{

	Establish( this, "BShell::Ctl");
}


void
BShell::Ctl::MessageReceived( BMessage *m)
{
	const void	*p;
	int32	n;

	//printf( "BShell::Ctl::MessageReceived: what='%.4s' (0x%X)\n", &m->what, m->what);
	switch (m->what) {
	case 'sigW':
		*this >> 'Gr&c';
		break;
	case 'Gr&c':
		m->FindData( "", B_UINT8_TYPE, &p, &n);
		ioctl( ptyfd, 'wsiz', p);
	}
}


void
BShell::TR::trinit( )
{
	RNode	n;

	Establish( this, "BShell::TR");
	SetDestination( n);
	n.SetDestination( *this);
	new ttyReader( ptyfd, n);
	*this >> 0;
}


void
BShell::TR::MessageReceived( BMessage *m)
{

	//printf( "BShell::TR::MessageReceived: what='%.4s' (0x%X)\n", &m->what, m->what);
	long n = m->FlattenedSize();
	char *s = (char*) malloc(n);
	m->Flatten( s, n);
	m = new BMessage;
	m->Unflatten( s);
	free( s);
	dsend( m);
	trlocal = TRUE;
	sigproc( );
}


void
BShell::TR::trsend( BMessage *m)
{

	*this >> m;
	trlocal = FALSE;
}


void
BShell::TW::twinit( )
{
	RNode	n;

	Establish( this, "BShell::TW");
	SetDestination( n);
	n.SetDestination( *this);
	new ttyWriter( ptyfd, n);
}


void
BShell::TW::MessageReceived( BMessage *m)
{

	//printf( "BShell::TW::MessageReceived: what='%.4s' (0x%X)\n", &m->what, m->what);
	unless (m->what) {
		rsend( new BMessage);
		twlocal = TRUE;
		sigproc( );
	}
}


void
BShell::TW::twsend( BMessage *m)
{

	*this >> m;
	twlocal = FALSE;
}


BShell::ttyReader::ttyReader( int fd, RNode n):
ptyfd( fd), RNode( n)
{

	Run( );
	//printf( "BShell::ttyReader: %d\n", Thread( ));
	Establish( 0, this, "BShell::ttyReader");
}


void
BShell::ttyReader::MessageReceived( BMessage *m)
{
	char	buf[1024];

	//printf( "BShell::ttyReader::MessageReceived: what='%.4s' (0x%X)\n", &m->what, m->what);
	switch (m->what) {
	case EOF:
		Quit( );
		break;
	default:
		int i = read( ptyfd, buf, sizeof buf);
		if (i < 0) {
			unless (errno == EAGAIN) {
				Quit( );
				break;
			}
			i = 0;
		}
		m = new BMessage;
		m->AddData( "", B_UINT8_TYPE, buf, i);
		*this >> m;
	}
}


BShell::ttyWriter::ttyWriter( int fd, RNode n):
ptyfd( fd), RNode( n)
{

	Run( );
	//printf( "BShell::ttyWriter: %d\n", Thread( ));
	Establish( 0, this, "BShell::ttyWriter");
}


void
BShell::ttyWriter::MessageReceived( BMessage *m)
{
	const void	*p;
	int32	n;

	//printf( "BShell::ttyWriter::MessageReceived: what='%.4s' (0x%X)\n", &m->what, m->what);
	switch (m->what) {
	case EOF:
		Quit( );
		break;
	default:
		m->FindData( "", B_UINT8_TYPE, &p, &n);
		if ((p)
		and (n)
		and (write( ptyfd, p, n) < 0))
			Quit( );
		else
			*this >> 0;
	}
}


void
BShell::Obj::report( char *error)
{
	char	s[99];

	sprintf( s, "BShell: %s\r\n", error);
	BMessage *m = new BMessage;
	m->AddData( "", B_UINT8_TYPE, s, strlen( s));
	dsend( m);
}
