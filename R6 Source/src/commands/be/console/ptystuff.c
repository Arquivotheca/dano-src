#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <OS.h>
#include <image.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>

#define ctrl(c) ((c) - 0100)  
              
const static struct termios tdefault = {
	ICRNL,                          /* c_iflag */
	OPOST|ONLCR,                    /* c_oflag */
	B19200|CS8|CREAD|HUPCL,         /* c_cflag */
	ISIG|ICANON|ECHO|ECHOE|ECHONL,  /* c_lflag */
	0,                              /* c_line */
	0,                              /* c_ixxxxx */
	0,                              /* c_oxxxxx */
	{
	ctrl( 'C'),                     /* c_cc[VINTR] */
	ctrl( '\\'),                    /* c_cc[VQUIT] */
	ctrl( 'H'),                     /* c_cc[VERASE] */
	ctrl( 'U'),                     /* c_cc[VKILL] */
	ctrl( 'D'),                     /* c_cc[VEOF] */
	0,                              /* c_cc[VEOL] */
	0,                              /* c_cc[VEOL2] */
	0,                              /* c_cc[VSWTCH] */
	ctrl( 'S'),                     /* c_cc[VSTART] */
	ctrl( 'Q'),                     /* c_cc[VSTOP] */
	ctrl( 'Z')                      /* c_cc[VSUSP] */
	}
};


#ifdef __ZRECOVER
/*
 * since all "teams" run in the same address space
 * "char **get_environ()" is used instead the typical
 * global var "char **environ"
 */
extern char ** get_environ(void);
#endif

          
/* find a tty/pty pair, launch argv[0], return ptyfd on success, -1 on failure */
int32 ExecControlled(char *argv[], int argc, thread_id *thr)
{
	uint32 count;
	char *suf1Pt = "pqrstuvwxyzPQRST";
	char ptyDev[20];
	char ttyDev[20];
	int32 ttyfd = -1;
	int32 ptyfd = -1;
	int32 pgid;
	thread_id shellThread;
	int stdinfd, stdoutfd, stderrfd;
		
	/* open a pseudoterminal */
	while(*suf1Pt && ttyfd < 0) {
		for(count = 0; count < 16; count++) {
			/* find an unused pty/tty name */
			sprintf(ptyDev, "/dev/pt/%c%x", *suf1Pt, (unsigned)count);
			sprintf(ttyDev, "/dev/tt/%c%x", *suf1Pt, (unsigned)count);
			ptyfd = open(ptyDev, O_RDWR);
			if (ptyfd >= 0) {
				/* pty master opened, try opening matching slave */
				ttyfd = open(ttyDev, O_RDWR);
				if (ttyfd >= 0) {
					/* we are done */
					break;
				}				
				/* failed to open slave, try with another master */
				close(ptyfd);
				ptyfd = -1;
			} else if (errno == ENOENT) {	
				break;
			}				
		}
		suf1Pt++;		 
	}
	
	if (ptyfd < 0) {
		fprintf(stderr,"failed to oen a pty\n");
		return -1;
	}
	
	/* initialize the tty */
	ioctl(ttyfd, TCSETA, &tdefault);
	
	/* save copies of our existing STDIO */
	stdinfd = dup(STDIN_FILENO);
	stdoutfd = dup(STDOUT_FILENO);
	stderrfd = dup(STDERR_FILENO);
	
	/* close the originals */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	
	/* hook up the tty */
	dup(ttyfd);
	dup(ttyfd);
	dup(ttyfd);
	
	/* ensure the spawned process doesn't keep our fd's */
	fcntl(stdinfd, F_SETFD, FD_CLOEXEC);
	fcntl(stdoutfd, F_SETFD, FD_CLOEXEC);
	fcntl(stderrfd, F_SETFD, FD_CLOEXEC);
	fcntl(ptyfd, F_SETFD, FD_CLOEXEC);
	fcntl(ttyfd, F_SETFD, FD_CLOEXEC);
        
		/* prepare an environment */
	{
		char buffer[40];
		sprintf(buffer, "TTY=%s", ttyDev);
		putenv("TERM=beterm");
		putenv(buffer);
	}


	/* load the new process */
#ifdef __ZRECOVER
	shellThread = load_image(argc, (char const**)argv, (char const**)get_environ());
#else
	shellThread = load_image(argc, (char const**)argv, (char const**)environ);
#endif

	/* restore STDIO */
	dup2(stdinfd, STDIN_FILENO);
	dup2(stdoutfd, STDOUT_FILENO);
	dup2(stderrfd, STDERR_FILENO);
	close(stdinfd);
	close(stdoutfd);
	close(stderrfd);
	close(ttyfd);
	
	if (shellThread < 0) {             
		close(ptyfd);
		fprintf(stderr,"can't find the shell\n");
		return -1;
	}

	*thr = shellThread;
	
	/* start the child process goinf */
	setpgid(shellThread, shellThread);
	resume_thread(shellThread);
	pgid = shellThread;
	ioctl(ptyfd, 'pgid', pgid);
	
	return ptyfd;
}

