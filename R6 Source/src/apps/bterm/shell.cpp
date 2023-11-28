#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <image.h>

#include <Locker.h>

#include <OS.h>

#define	ctrl( c)	((c) - 0100)

static BLocker spawn_lock("spawn_lock");

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


int spawn_proc(char **args)
{
	char tty[20], pty[20], te[40];
	int	ttyfd, ptyfd;

	int n;

	if(spawn_lock.Lock() == false) return B_ERROR;
			
	for(n=0;args[n];n++);
	
	uint i = 0;
	uint j = 0;
	for(;;) {
		sprintf( pty, "/dev/pt/%c%x", 'p'+i/16, i%16);
		sprintf( tty, "/dev/tt/%c%x", 'p'+i/16, i%16);
		ptyfd = open( pty, O_RDWR);
		if( ptyfd >= 0 ) {
			ttyfd = open( tty, O_RDWR);
			if( ttyfd >= 0) break;
			close( ptyfd);
		}
		if ((++i == ('z'+1-'p')*16) || (errno == ENOENT)) {
			if (++j == 10) {
				fprintf(stderr,"can't open a pty");
				spawn_lock.Unlock();
				return -1;
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
//	void (*s)( int) = signal( SIGHUP, SIG_DFL);
	thread_id t = load_image(n, (const char**) args, (const char **)environ);
//	signal( SIGHUP, s);
	dup2( myfd0, 0);
	dup2( myfd1, 1);
	dup2( myfd2, 2);
	close( myfd0);
	close( myfd1);
	close( myfd2);
	close( ttyfd);
	if (t < 0) {
		fprintf(stderr, "can't find the shell");
		close( ptyfd);
		spawn_lock.Unlock();
		return -1;
	}
	setpgid( t, t);
	resume_thread( t);
	ioctl( ptyfd, 'pgid', t);
	
	spawn_lock.Unlock();
	return ptyfd;
}
