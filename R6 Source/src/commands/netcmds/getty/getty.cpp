/* Copyright (C) 1997 Be Inc.  All Rights Reserved */

/*
 * Simple getty-like program
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <SerialPort.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <termios.h>

const char *LOGIN[] = { "/bin/login", "-n", NULL };

int respawn;

typedef struct {
	int pty;
	BSerialPort port;
} reader_args;

void
usage(const char *name)
{
	char *slash;

	slash = strrchr(name, '/');
	if (slash != NULL) {
		name = slash + 1;
	}
	fprintf(stderr, "usage: %s [-f] port speed [cmd arg1 ...]\n", name);
	exit(1);
}

int
openpty(int *slavep)
{
	DIR *dd;
	struct dirent *d;
	char ttyname[12];
	int pty;
	int slave;

	dd = opendir("/dev/tt");
	while (d = readdir(dd)) {
		if (strcmp(d->d_name, ".") == 0) continue;
		if (strcmp(d->d_name, "..") == 0) continue;
		sprintf(ttyname, "/dev/pt/%s", d->d_name);
		pty = open(ttyname, O_RDWR, 0);
		if (pty < 0) continue;
		sprintf(ttyname, "/dev/tt/%s", d->d_name);
		slave = open(ttyname, O_RDWR, 0);
		if (slave < 0) {
			close(pty);
			continue;
		}
		closedir(dd);
		*slavep = slave;
		return (pty);
	}
	closedir(dd);
	return (-1);		
}

int32
ptyreader(void *varg)
{
	reader_args *args = (reader_args *)varg;
	char buf[1024];
	int len;
	int wlen;

	while ((len = read(args->pty, buf, sizeof(buf))) > 0) {
		if ((wlen = args->port.Write(buf, len)) < 0) {
			break;
		}
	}
	close(args->pty);
	args->port.Close();
	return (0);
}

void
sigchld(int sig)
{
	respawn++;
}

pid_t
spawn_child(const char **argv, int pty, int slave)
{
	pid_t pid;
	struct termios c;

	respawn = 0;
	signal(SIGCHLD, sigchld);
	switch (pid = fork()) {
	case 0:
		signal(SIGCHLD, SIG_IGN);
		close(pty);
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		close(slave);
		tcgetattr(0, &c);
		c.c_lflag |= ICANON|ISIG|ECHO|ECHOE|ECHOK;
		c.c_oflag = OPOST|ONLCR|TAB0;
		c.c_iflag |= ICRNL;
		tcsetattr(0, TCSANOW, &c);
		setpgid(0, 0);
		ioctl(0, 'pgid', getpid());
		execv(argv[0], (char **)argv);
		_exit(1);
	default:
		break;
	}
	return (pid);
}

void
run_getty(const char **login, const char *sport, int speed, int flow_control)
{
	BSerialPort port;
	char buf[1024];
	int len;
	int baudrate;
	int pty;
	int slave;
	reader_args *args;
	int wlen;
	int pid;

	pty = openpty(&slave);
	if (pty < 0) {
		fprintf(stderr, "out of ptys\n");
		return;
	}
	if (port.Open(sport) < 0) {
		perror(sport);
		return;
	}


#define DOIT(baud)  if (speed == baud) { port.SetDataRate(B_##baud##_BPS); break; }
	do {
		DOIT(50);
		DOIT(75);
		DOIT(110);
		DOIT(134);
		DOIT(150);
		DOIT(200);
		DOIT(300);
		DOIT(600);
		DOIT(1200);
		DOIT(1800);
		DOIT(2400);
		DOIT(4800);
		DOIT(9600);
		DOIT(19200);
		DOIT(38400);
		DOIT(57600);
		DOIT(115200);
		DOIT(230400);

		/* default */
		fprintf(stderr, "invalid baud rate\n");
		return;
	} while (0);
#undef DOIT

	port.SetFlowControl(flow_control ? B_HARDWARE_CONTROL : 0);
	port.SetBlocking(false);
	port.SetTimeout(B_INFINITE_TIMEOUT);

	/*
	 * Spawn the login process
	 */
	pid = spawn_child(login, pty, slave);
	if (pid < 0) {
		perror("can't spawn child");
		exit(1);
	}

	/*
	 * Spawn the pty reader thread
	 */
	args = new reader_args;
	args->pty = pty;
	args->port = port;
	resume_thread(spawn_thread(ptyreader, "pty-reader", B_NORMAL_PRIORITY, 
							   (void *)args));

	/*
	 * This is the pty writer
	 */
	for (;;) {
		while ((len = port.WaitForInput()) > 0) {
			len = port.Read(buf, sizeof(buf));
			if (len < 0) {
				break;
			}
			if ((wlen = write(pty, buf, len)) < 0) {
				break;
			}
		}
		if (respawn) {
			int status;
			pid_t gotpid;

			gotpid = waitpid(-1, &status, WNOHANG);
			if (gotpid < 0) {
				perror("getty -- wait");
			} else if (gotpid == pid) {
				do {
					pid = spawn_child(login, pty, slave);
					if (pid < 0) {
						perror("can't spawn child");
						sleep(1);
					}
				} while (pid < 0);
			}
		} else {
			break;
		}
	}
	close(pty);
	port.Close();
}

int
main(int argc, char **argv)
{
	int flow_control = 0;
	int baud_rate;
	char *serialport;
	const char *myname = argv[0] ? argv[0] : "getty";

	for (argc--, argv++; argc > 0 && **argv == '-'; argc--, argv++) {
		if (strcmp(argv[0], "-f") == 0) {
			flow_control++;
			continue;
		}
		usage(myname);
	}
	if (argc < 2) {
		usage(myname);
	}
	serialport = argv[0];
	baud_rate = atoi(argv[1]);
	argv += 2;
	argc -= 2;
	run_getty(argc ? (const char **) argv : LOGIN, serialport, baud_rate, flow_control);
	exit(1);
}
