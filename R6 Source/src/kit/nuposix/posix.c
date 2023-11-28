#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <utime.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>
#include <termios.h>
#include <stdarg.h>
#include <setjmp.h>
#include <limits.h>
#include <iovec.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <sys/select.h>
#include <sys/utsname.h>

#if __ELF__
# include <sys/ioctl.h>
# include <dirent.h>
#endif

#include <OS.h>
#include <FindDirectory.h>
#include <fs_query.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <fs_info.h>
#include <fsproto.h>

#include "priv_syscalls.h"

#if _R5_COMPATIBLE_
	#if __POWERPC__
	/* -----
		These were defined in Preview Release 2.
		Define them the same way for binary compatibility,
		even though they really should be:
		char *sys_errlist[] = {0};
		int	sys_nerr = 0;
	
		It ends up making no diference to the referencing
		code - the bits are zero either way...
	----- */
	char	*sys_errlist = NULL;
	char	*sys_nerr = NULL;
	#endif
#endif  // _R5_COMPATIBLE_


int
mount(const char *filesystem, const char *where, const char *device,
	  ulong flags, void *parms, int len)
{
	int		err;

	err = _kmount_(filesystem, -1, where, device, flags, parms, len);
	if (!err)
		return 0;
	errno = err;
	return err;
}


int
unmount(const char *path)
{
	int		err;

	err = _kunmount_(-1, path);
	if (!err)
		return 0;
	errno = err;
	return err;
}

DIR *
opendir(const char *name)
{
	int		err;
	DIR		*d;

	d = (DIR *) malloc(sizeof(DIR) + B_FILE_NAME_LENGTH);
	if (!d) {
		err = ENOMEM;
		goto error1;
	}
	d->fd = _kopendir_(-1, name, FALSE);
	if (d->fd < 0) {
		err = d->fd;
		goto error2;
	}

	return d;

error2:
	free(d);
error1:
	errno = err;
	return NULL;
}

struct dirent *
#if __ELF__
__readdir(DIR *d)
#else
readdir(DIR *d)
#endif
{
	long		n;

	n = _kreaddir_(d->fd, &d->ent, sizeof(d->ent)+B_FILE_NAME_LENGTH, 1);
	if (n == 1)
		return &d->ent;
	if (n == 0)
		return NULL;
	errno = n;
	return NULL;	
}
#if __ELF__
#pragma weak readdir = __readdir
#endif

void
rewinddir(DIR *d)
{
	_krewinddir_(d->fd);
}

int
closedir(DIR *d)
{
	int		err;

	err = _kclosedir_(d->fd);
	free(d);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

static int umask_val = 022;

int
#if __ELF__
__open(const char *path, int omode, ...)
#else
open(const char *path, int omode, ...)
#endif
{
	int			res;
	va_list		ap;
	mode_t		perms;
	bool		coe;

	perms = 0;
	if (omode & O_CREAT) {
		va_start(ap, omode);
		perms = va_arg(ap, mode_t);
		perms = perms & ~umask_val;
		va_end(ap);
	}
	coe = (omode & O_CLOEXEC) ? true : false;
	res = _kopen_(-1, path, omode, perms, coe);
	if (res >= 0)
		return res;
	errno = res;
	return -1;
}
#if __ELF__
#pragma weak open = __open
#endif

int
#if __ELF__
__close(int fd)
#else
close(int fd)
#endif
{
	int		err;

	err = _kclose_(fd);
	if (!err)
		return 0;
	errno = err;
	return -1;
}
#if __ELF__
#pragma weak close = __close
#endif

int
creat(const char *path, mode_t perms)
{
	return open(path, O_CREAT | O_TRUNC | O_WRONLY, perms);
}

ssize_t
#if __ELF__
__libc_read(int fd, void *buf, size_t count)
#else
read(int fd, void *buf, size_t count)
#endif
{
	int	res, err;

	res = _kread_(fd, buf, count, &err);
	if (res > 0 || (err == 0 && res == 0))
		return res;
	errno = err;
	return -1;
}
#if __ELF__
#pragma weak __read = __libc_read
#pragma weak read = __libc_read
#endif

ssize_t
read_pos(int fd, off_t pos, void *buf, size_t count)
{
	int	res, err;

	res = _kread_pos_(fd, pos, buf, count, &err);
	if (res > 0 || (err == 0 && res == 0))
		return res;
	errno = err;
	return -1;
}

ssize_t
write_pos(int fd, off_t pos, const void *buf, size_t count)
{
	int	res, err;

	res = _kwrite_pos_(fd, pos, buf, count, &err);
	if (res > 0 || (err == 0 && res == 0))
		return res;
	errno = err;
	return -1;
}

ssize_t
readv_pos(int fd, off_t pos, const iovec *vec, size_t count)
{
	int	res, err;

	res = _kreadv_pos_(fd, pos, vec, count, &err);
	if (res > 0 || (err == 0 && res == 0))
		return res;
	errno = err;
	return -1;
}

ssize_t
writev_pos(int fd, off_t pos, const iovec *vec, size_t count)
{
	int	res, err;

	res = _kwritev_pos_(fd, pos, vec, count, &err);
	if (res > 0 || (err == 0 && res == 0))
		return res;
	errno = err;
	return -1;
}

ssize_t
#if __ELF__
__readv(int fd, const iovec *vec, size_t count)
#else
readv(int fd, const iovec *vec, size_t count)
#endif
{
	int	res, err;

	res = _kreadv_pos_(fd, -1, vec, count, &err);
	if (res > 0 || (err == 0 && res == 0))
		return res;
	errno = err;
	return -1;
}
#if __ELF__
#pragma weak readv = __readv
#endif

ssize_t
#if __ELF__
__writev(int fd, const iovec *vec, size_t count)
#else
writev(int fd, const iovec *vec, size_t count)
#endif
{
	int	res, err;

	res = _kwritev_pos_(fd, -1, vec, count, &err);
	if (res > 0 || (err == 0 && res == 0))
		return res;
	errno = err;
	return -1;
}
#if __ELF__
#pragma weak writev = __writev
#endif

off_t
#if __ELF__
__lseek(int fd, off_t pos, int mode)
#else
lseek(int fd, off_t pos, int mode)
#endif
{
	off_t		res;

	res = _klseek_(fd, pos, mode);
	if (res >= 0)
		return res;
	errno = res;
	return -1;
}
#if __ELF__
#pragma weak lseek = __lseek
#endif

ssize_t
#if __ELF__
__libc_write(int fd, const void *buf, size_t count)
#else
write(int fd, const void *buf, size_t count)
#endif
{
	int	res, err;

	res = _kwrite_(fd, buf, count, &err);
	if (res > 0 || (err == 0 && res == 0))
		return res;
	errno = err;
	return -1;
}
#if __ELF__
#pragma weak __write = __libc_write
#pragma weak write = __libc_write
#endif


int
ioctl(int fd, unsigned long int cmd, ...)
{
	int			res;
	va_list		ap;
	void		*arg;

	va_start(ap, cmd);
	arg = va_arg(ap, void *);
	va_end(ap);
	res = _kioctl_(fd, cmd, arg, 0);
	if (res >= 0)
		return res;
	errno = res;
	return -1;
}

int
#if __ELF__
__fcntl(int fd, int op, ...)
#else
fcntl(int fd, int op, ...)
#endif
{
	int			res;
	va_list		ap;
	long		arg;

	va_start(ap, op);
	arg = va_arg(ap, long);
	va_end(ap);
	res = _kfcntl_(fd, op, arg);
	if (res >= 0)
		return res;
	errno = res;
	return -1;
}
#if __ELF__
#pragma weak fcntl = __fcntl
#endif

int
dup(int fd)
{
	return fcntl(fd, F_DUPFD, 0);
}

int
dup2(int fd, int nfd)
{
	int		res;

	res = _kdup2_(fd, nfd, FALSE);
	if (res >= 0)
		return res;
	errno = res;
	return -1;
}

int
chdir(const char *path)
{
	int		err;

	err = _kchdir_(-1, path);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
fchdir(int fd)
{
	int		err;

	err = _kchdir_(fd, NULL);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
access(const char *path, int mode)
{
	int		err;

	err = _kaccess_(-1, path, mode);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
link(const char *oldpath, const char *newpath)
{
	int		err;

	err = _klink_(-1, oldpath, -1, newpath);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
mkdir(const char *path, mode_t perms)
{
	int		err;

	err = _kmkdir_(-1, path, (perms & ~umask_val));
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
#if __ELF__
__rmdir(const char *path)
#else
rmdir(const char *path)
#endif
{
	int		err;

	err = _krmdir_(-1, path);
	if (!err)
		return 0;
	errno = err;
	return -1;
}
#if __ELF__
#pragma weak rmdir = __rmdir
#endif

int
symlink(const char *oldpath, const char *newpath)
{
	int		err;

	err = _ksymlink_(oldpath, -1, newpath);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

ssize_t
#if __ELF__
__readlink(const char *path, char *buf, size_t bufsize)
#else
readlink(const char *path, char *buf, size_t bufsize)
#endif
{
	int		res;

	res = _kreadlink_(-1, path, buf, bufsize);
	if (res >= 0)
		return res;
	errno = res;
	return -1;
}
#if __ELF__
#pragma weak readlink = __readlink
#endif

int
rename(const char *oldpath, const char *newpath)
{
	int		err;

	err = _krename_(-1, oldpath, -1, newpath);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
#if __ELF__
__unlink(const char *path)
#else
unlink(const char *path)
#endif
{
	int		err;

	err = _kunlink_(-1, path);
	if (!err)
		return 0;
	errno = err;
	return -1;
}
#if __ELF__
#pragma weak unlink = __unlink
#endif

int
#if __ELF__
__xstat(int version, const char *path, struct stat *st)
#else
stat(const char *path, struct stat *st)
#endif
{
	int		err;

	err = _krstat_(-1, path, st, TRUE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
#if __ELF__
__lxstat(int version, const char *path, struct stat *st)
#else
lstat(const char *path, struct stat *st)
#endif
{
	int		err;

	err = _krstat_(-1, path, st, FALSE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
#if __ELF__
__fxstat(int version, int fd, struct stat *st)
#else
fstat(int fd, struct stat *st)
#endif
{
	int		err;

	err = _krstat_(fd, NULL, st, TRUE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

#if __ELF__
#pragma weak stat = __xstat
#pragma weak lstat = __lxstat
#pragma weak fstat = __fxstat
#endif


int
chmod(const char *path, mode_t mode)
{
	int				err;
	struct stat		st;

	st.st_mode = mode;
	err = _kwstat_(-1, path, &st, WSTAT_MODE, TRUE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
chown(const char *path, uid_t uid, gid_t gid)
{
	int				err;
	struct stat		st;

	st.st_uid = uid;
	st.st_gid = gid;
	err = _kwstat_(-1, path, &st, WSTAT_UID | WSTAT_GID, TRUE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
fchmod(int fd, mode_t mode)
{
	int				err;
	struct stat		st;

	st.st_mode = mode;
	err = _kwstat_(fd, NULL, &st, WSTAT_MODE, TRUE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
truncate(const char *path, off_t newsize)
{
	int				err;
	struct stat		st;

	st.st_size = newsize;
	err = _kwstat_(-1, path, &st, WSTAT_SIZE, TRUE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
ftruncate(int fd, off_t newsize)
{
	int				err;
	struct stat		st;

	st.st_size = newsize;
	err = _kwstat_(fd, NULL, &st, WSTAT_SIZE, TRUE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

int
utime(const char *path, const struct utimbuf *buf)
{
	int				err;
	time_t			now;
	struct stat		st;

	if (buf) {
		st.st_mtime = buf->modtime;
		st.st_atime = buf->actime;
	} else {
		now = time(NULL);
		st.st_mtime = now;
		st.st_atime = now;
	}
	err = _kwstat_(-1, path, &st, WSTAT_MTIME | WSTAT_ATIME, TRUE);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

char *
#if __ELF__
__getcwd(char *buf, size_t bufsize)
#else
getcwd(char *buf, size_t bufsize)
#endif
{
	char			p[1024];
	char			*q;
	int				ps, qs, l;
	struct dirent	*e;
	DIR				*d;
	struct stat		st;
	ino_t			ino;
	dev_t			dev, pdev;

	if (buf == NULL) {
		bufsize = MAXPATHLEN;
		buf = (char *)malloc(MAXPATHLEN);
 		if (buf == NULL)
			return NULL;
	}

	buf[0] = '\0';
	
	ps = 1;
	strcpy(p, ".");
	if (bufsize < 2)
		goto error1;
	qs = 0;
	q = buf;

	while(TRUE) {
		if (ps + 3 + 1 + B_FILE_NAME_LENGTH > sizeof(p))
			goto error1;
		if (stat(p, &st))
			goto error1;
		dev = st.st_dev;
		ino = st.st_ino;
		strcpy(&p[ps], "/..");
		ps += 3;
		d = opendir(p);
		if (!d)
			goto error1;
		if (stat(p, &st))
			goto error2;
		if ((st.st_ino == ino) && (st.st_dev == dev)) {
			if (qs == 0)        /* nothing in the buffer yet, must be in "/" */
				strcpy(buf, "/");
			
			closedir(d);
			break;
		}
		p[ps] = '/';
		pdev = st.st_dev;
		while (TRUE) {
			e = readdir(d);
			if (!e)
				goto error2;
			if (e->d_ino == ino)
				if (pdev == dev)
					break;
				else {
					strcpy(&p[ps+1], e->d_name);
					if (stat(p, &st))
						goto error2;
					if (st.st_dev == dev)
						break;
				}
		}
		p[ps] = '\0';
		l = strlen(e->d_name) + 1;
		if (qs + l + 1 > bufsize)
			goto error2;
		memmove(q+l, q, qs);
		memcpy(q+1, e->d_name, l-1);
		closedir(d);
		q[0] = '/';
		qs += l;
		q[qs] = '\0';
	}
	return buf;

error2:
	closedir(d);
error1:
	return NULL;
}
#if __ELF__
#pragma weak getcwd = __getcwd
#endif

int
pipe(int fds[2])
{
	char		pipename[48];
	int			serrno;
	bigtime_t	r;

	r = system_time();
	while (r == system_time())
		;

	sprintf(pipename, "/pipe/%x-%Ld", find_thread(NULL), system_time());

	if ((fds[0] = open(pipename, O_CREAT | O_RDONLY, 0777)) < 0)
		return(-1);

	if ((fds[1] = open(pipename, O_WRONLY)) < 0) {
		serrno = errno;
		close(fds[0]);
		errno = serrno;
		return(-1);
	}
	return(0);
}

#if !__ELF__ 
FILE *
fdopen(int fd, const char *mode)
{
    static char BT[]    = "/dev/null";
    FILE        *f;
    int     z;

    f = fopen( BT, mode);
    z = f->handle;
    f->handle = fd;
    close( z);
    return (f);
}
#endif

mode_t
umask(mode_t cmask)
{
	int omask = umask_val;
	
	umask_val = cmask & 0777;

	return omask;
}

int
#if __ELF__
__xmknod(int version, const char *name, mode_t mode, dev_t *dev)
#else
mknod(const char *name, mode_t mode, dev_t dev)
#endif
{
	errno = EINVAL;
	return -1;
}
#if __ELF__
#pragma weak mknod = __xmknod
#endif

int
mkfifo(const char *path, mode_t mode)
{
	errno = EINVAL;
	return -1;
}

#if !__ELF__
int
fileno(FILE *f)
{
    return (f->handle);
}

int sigemptyset(sigset_t *set)
{
	*set = 0;
	return 0;
}

int sigfillset(sigset_t *set)
{
	*set = ~0;
	return 0;
}

int sigaddset(sigset_t *set, int signo)
{
	*set |= (1 << (signo-1));
	return 0;
}


int sigdelset(sigset_t *set, int signo)
{
	*set &= ~(1 << (signo-1));
	return 0;
}

int sigismember(const sigset_t *set, int signo)
{
	return (*set & (1 << (signo-1)));
}
#endif /* !__ELF__ */

int		kill(pid_t pid, int sig)
{
	if (send_signal(pid, (uint)sig) != B_NO_ERROR) {
		errno = EINVAL;
		return -1;
	}
	return 0;
}

#if 0
static char *members[] = { NULL };

static struct group gr = { "users", "", 0, &members[0] };

struct group *
getgrgid(gid_t gid)
{
	if (gid != getgid())
		return NULL;

	gr.gr_gid = getgid();
	if ((gr.gr_name = getenv("GROUP")) == NULL)
		gr.gr_name = "users";

	return &gr;
}



struct group *
getgrnam(const char *name)
{
	if ((gr.gr_name = getenv("GROUP")) == NULL)
		gr.gr_name = "users";

	if (strcmp(name, gr.gr_name) != 0)
		return NULL;
	gr.gr_gid = getgid();

	return &gr;
}

static int grent_called = 0;

struct group *
getgrent(void)
{
	if (grent_called == 0) {
		grent_called = 1;

		return &gr;
	}

	return 0;
}

void
setgrent(void)
{
	grent_called = 0;
}

void
endgrent(void)
{
}


int
getgroups(int x, gid_t grps[])
{
	return 0;
}

#define USER_NAME  "baron"

struct passwd *getpwnam(const char *name)
{
	static int init=0;
	static struct passwd pw;

	if (init == 0) {
		char *ptr;
		char wd[PATH_MAX];
		
		if ((pw.pw_name = getenv("USER")) == NULL)
			pw.pw_name = USER_NAME;
		
		pw.pw_passwd = "gilamelio";
		ptr = getenv("UID");
		if (ptr)
			pw.pw_uid = atoi(ptr);
		else
			pw.pw_uid = 42;

		ptr = getenv("GID");
		if (ptr)
			pw.pw_gid = atoi(ptr);
		else
			pw.pw_gid = 42;
		
		if ((pw.pw_dir = getenv("HOME")) == NULL)
			if (find_directory (B_USER_DIRECTORY, -1, false, wd, PATH_MAX) == B_OK)
				pw.pw_dir = strdup (wd);
			else
				pw.pw_dir = "/boot";
		
		if ((pw.pw_shell = getenv("SHELL")) == NULL)
			pw.pw_shell = "/bin/sh";
	}

	return &pw;
}

struct passwd *getpwuid(uid_t uid)
{
	return getpwnam(NULL);
}


static int pwent_called=0;

void
setpwent(void)
{
	pwent_called = 0;
}


struct passwd *getpwent(void)
{

  if (pwent_called == 0) {
 	 pwent_called = 1;
	 return getpwnam(NULL);
  }

  return NULL;
}

void
endpwent(void)
{
}
#endif

#if __ELF__
int __isatty(int fd)
#else
int isatty(int fd)
#endif
{
  int save;
  int is_tty;
  struct termios term;

  save = errno;
  is_tty = tcgetattr(fd, &term) == 0;
  errno = save;

  return is_tty;
}
#if __ELF__
#pragma weak isatty = __isatty
#endif

char *
ctermid(char *s)
{
	static char buf[L_ctermid];

	//currently, the name of the "controlling terminal" is stored
	//in the TTY environment variable
	char *name = getenv("TTY");

	if (name == NULL)
		name = "";

	if (s == NULL)
		s = buf;
	return strncpy(s, name, L_ctermid);
}

char *
cuserid(char *s)
{
	char *tmp;

	if ((tmp = getenv("USER")) == NULL)
		tmp = "userid";
	
	return strncpy(s, tmp, L_cuserid);
}

int (tcgetattr)(int f, struct termios *t);
	// need a local prototype here because tcgetattr is otherwise
	// declared as a #define to an ioctl call, need this to not
	// get a warning

int
(tcgetattr)(int f, struct termios *t)
{
	return ioctl(f, TCGETA, (char *)t);
}

int tcsetattr(int fd, int opt, const struct termios *tp)
{
	if (opt == TCSANOW) {
		return ioctl(fd, TCSETA, tp);
	} else if (opt == TCSADRAIN) {
		return ioctl(fd, TCSETAW, tp);
	} else if (opt == TCSAFLUSH) {
		return ioctl(fd, TCSETAF, tp);
	} else {
		return -1;
	}
}

int
tcsendbreak( int fd, int duration)
{
	return (ioctl( fd, TCSBRK, 0));
}

int
tcdrain( int fd)
{
	return (ioctl( fd, TCSBRK, 1));
}

/*
 * (Not yet implemented)
 */
int
tcflow(int arg1, int arg2)
{
	return 0;
}

int
tcflush( int fd, int qsel)
{
	switch (qsel) {
	case TCIFLUSH:
		return (ioctl( fd, TCFLSH, 0));
	case TCOFLUSH:
		return (ioctl( fd, TCFLSH, 1));
	case TCIOFLUSH:
		return (ioctl( fd, TCFLSH, 2));
	}
	errno = EINVAL;
	return (-1);
}

/*
 * This is evidently only for Metaware and plumhall.
 */

clock_t	
#if __ELF__
__times(struct tms *buffer)
#else
times(struct tms *buffer)
#endif
{
	if (buffer) {
		team_usage_info ti_self, ti_children;

		get_team_usage_info(0, RUSAGE_SELF, &ti_self);
		get_team_usage_info(0, RUSAGE_CHILDREN, &ti_children);

		buffer->tms_utime = ti_self.user_time;
		buffer->tms_stime = ti_self.kernel_time;
		buffer->tms_cutime = ti_children.user_time;
		buffer->tms_cstime = ti_children.kernel_time;
	}

	return ((clock_t)(system_time() / 1000));
}
#if __ELF__
#pragma weak times = __times
#endif

int
getrusage(int who, struct rusage *r)
{
	status_t err;
	team_usage_info ti;

	err = get_team_usage_info(0, who, &ti);
	r->ru_utime.tv_sec = ti.user_time / 1000000;
	r->ru_utime.tv_usec = ti.user_time % 1000000;

	r->ru_stime.tv_sec = ti.kernel_time / 1000000;
	r->ru_stime.tv_usec = ti.kernel_time % 1000000;

	if (err < 0) {
		errno = err;
		return -1;
	}
	return 0;
}

int
ftime(struct timeb *tp)
{
	struct timeval	tv;
	struct timezone	tz;

	gettimeofday(&tv, &tz);
	
	tp->time 		= tv.tv_sec;
	tp->millitm 	= tv.tv_usec / 1000;
	tp->timezone 	= tz.tz_minuteswest;
	tp->dstflag 	= tz.tz_dsttime;
	return 0;
}

int 
#if __ELF__
__gettimeofday(struct timeval *tv, struct timezone *tz)
#else
gettimeofday(struct timeval *tv, struct timezone *tz)
#endif
{
	bigtime_t 	now;
	rtc_info	info;

	if (tv) {
		now = real_time_clock_usecs();
		tv->tv_sec  = now / 1000000;
		tv->tv_usec = now % 1000000;
	}

	if (tz) {           
		_kget_rtc_info_(&info);
		tz->tz_minuteswest 	= info.tz_minuteswest;
		tz->tz_dsttime 		= info.tz_dsttime;
	}

	return (0);
}
#if __ELF__
#pragma weak gettimeofday = __gettimeofday
#endif

/*
 * stime sets the system's idea of the time and date.  Time, pointed to
 * by t, is measured in seconds from  00:00:00  GMT  January  1,  1970.
 * stime() may only be executed by the super user.
 */
int
stime(const time_t *t)
{
	if (geteuid() != 0) {
		errno = EPERM;
		return -1;
	}

	set_real_time_clock(*t);

	return 0;
}

long
#if __ELF__
__sysconf(int name)
#else
sysconf(int name)
#endif
{
	int ret = -1;
	
	switch (name) {
	case _SC_ARG_MAX:          ret = ARG_MAX;
		                       break;
	case _SC_CHILD_MAX:        ret = CHILD_MAX;
		                       break;
	case _SC_CLK_TCK:          ret = CLK_TCK;
		                       break;
	case _SC_NGROUPS_MAX:      ret = NGROUPS_MAX;
		                       break;
	case _SC_STREAM_MAX:       ret = STREAM_MAX;
		                       break;
	case _SC_TZNAME_MAX:       ret = TZNAME_MAX;
		                       break;
	case _SC_OPEN_MAX:         ret = OPEN_MAX;
		                       break;
	case _SC_JOB_CONTROL:      ret = 1;
		                       break;
	case _SC_SAVED_IDS:        ret = 1;
		                       break;
	case _SC_VERSION:          ret = 199009L;
		                       break;
	}

	return ret;
}
#if __ELF__
#pragma weak sysconf = __sysconf
#endif

long pathconf(const char *str, int name)
{
	int ret = -1;
	
	switch (name) {
	case _PC_LINK_MAX:         ret = LINK_MAX;
		                       break;
	case _PC_MAX_CANON:        ret = MAX_CANON;
		                       break;
	case _PC_MAX_INPUT:        ret = MAX_INPUT;
		                       break;
	case _PC_NAME_MAX:         ret = NAME_MAX;
		                       break;
	case _PC_PIPE_BUF:         ret = 4096; /* XXXdbg - is that good? */
		                       break;
	case _PC_CHOWN_RESTRICTED: ret = 1;
		                       break;
	case _PC_NO_TRUNC:         ret = 0;
		                       break;
	case _PC_VDISABLE:         errno = EIO;
		                       ret = -1;
		                       break;
	}

	return ret;
}


long
fpathconf(int x, int name)
{
	int ret = -1;
	
	switch (name) {
	case _PC_LINK_MAX:         ret = LINK_MAX;
		                       break;
	case _PC_MAX_CANON:        ret = MAX_CANON;
		                       break;
	case _PC_MAX_INPUT:        ret = MAX_INPUT;
		                       break;
	case _PC_NAME_MAX:         ret = NAME_MAX;
		                       break;
	case _PC_PATH_MAX:         ret = PATH_MAX;
		                       break;
	case _PC_PIPE_BUF:         ret = 4096; /* XXXdbg - is that good? */
		                       break;
	case _PC_CHOWN_RESTRICTED: ret = 1;
		                       break;
	case _PC_NO_TRUNC:         ret = 0;
		                       break;
	case _PC_VDISABLE:         ret = -1;
		                       break;
	}

	return ret;
}

extern char **	environ;

int system(const char *cmd)
{
	long		result = -1;
	char		*argv[4];
	char		*shell = "/bin/sh";

	if (!cmd || *cmd == '\0')
		return 0;

	argv[0] = shell;
	argv[1] = "-c";
	argv[2] = (char *)cmd;
	argv[3] = NULL;
	
	if ((result = load_image (3, argv, environ)) >= 0)
		while (wait_for_thread (result, &result) == B_INTERRUPTED)
			;

	return result;
}

unsigned int
#if __ELF__
__sleep(unsigned int secs)
#else
sleep(unsigned int secs)
#endif
{
	int ret;
	bigtime_t start = system_time() / 1000000;

	ret = snooze(secs*(bigtime_t)1000000);
	if (ret < 0)
		return secs - ((system_time() / 1000000) - start);

	return 0;
}
#if __ELF__
#pragma weak sleep = __sleep
#endif


#if !__ELF__
/* ----------
	tempnam simulates the Unix tempnam call.
----- */
char *
tempnam (char *base, char *pfx)
{
	static int sequence = 0;
	char *p;

	if (!(p = (char *) malloc (14 + (pfx ? strlen (pfx) : 0))))
		return NULL;
	strcpy (p, "/tmp/");
	if (pfx)
		strcat (p, pfx);
	sprintf (p + strlen(p), "%.4d%.4d", find_thread(NULL), sequence++);
	return p;
}


char *
mktemp(char *name)
{
    char    *p;
    int32    time_bits;
    struct stat st;

    p = name;
    while (*p)
        p++;        /* point to end */

    p -= 6;         /* back up six X's */

    while (1) {
        time_bits = system_time() & 0x7ffff;
        sprintf(p, "%06.6d", time_bits);
        if (stat(name, &st) != 0)
            break;
    }
    return(name);
}
#endif /* !__ELF__ */

#if 0
char
*getlogin(void)
{
	char *tmp;

	if ((tmp = getenv("USER")) == NULL)
		tmp = USER_NAME;

	return tmp;
}
#endif

void
fs_info_to_struct_statvfs(fs_info *info, struct statvfs *buf)
{

	memset(buf, 0, sizeof(struct statvfs));
	
	buf->f_bsize	= info->io_size;
	buf->f_frsize	= info->block_size;
	buf->f_blocks	= info->total_blocks;
	buf->f_bfree	= info->free_blocks;
	buf->f_bavail	= info->free_blocks;
	/* f_bavail : number of free blocks available to
		non-priveleged process */
	
	buf->f_files	= LONG_MAX;/* XXX */
	buf->f_ffree	= LONG_MAX;/* XXX */
	buf->f_favail	= LONG_MAX;/* XXX */

	
	buf->f_fsid		= info->dev;
	buf->f_flag		= (info->flags & B_FS_IS_READONLY) ? ST_RDONLY : 0;
	buf->f_namemax 	= B_FILE_NAME_LENGTH;

	return;
}



int
statvfs(const char *path, struct statvfs *buf)
{
	fs_info info;
	int	err;

	err = _kstatfs_(-1, NULL, -1, path, &info);
	if (err != 0) {
		errno = err;
		return -1;
	}

	fs_info_to_struct_statvfs(&info, buf);
	return 0;
}


int
fstatvfs(int fildes, struct statvfs *buf)
{
	fs_info info;
	int err;
		
	err = _kstatfs_(-1, NULL, fildes, NULL, &info);
	if (err != 0) {
		errno = err;
		return -1;
	}
	
 	fs_info_to_struct_statvfs(&info, buf);
	return 0;
}


dev_t
dev_for_path(const char *path)
{
	status_t		err;
	struct stat		st;

	err = _krstat_(-1, path, &st, TRUE);
	if (!err)
		return st.st_dev;
	return err;
}

dev_t
next_dev(int32 *pos)
{
	status_t	err;
	fs_info		fs;

	err = _kstatfs_(-1, pos, -1, NULL, &fs);
	if (!err)
		return fs.dev;
	return err;
}

int
fs_stat_dev(dev_t dev, fs_info *info)
{
	status_t	err;

	err = _kstatfs_(dev, NULL, -1, NULL, info);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

ssize_t
fs_read_attr(int fd, const char *attribute, uint32 type, off_t pos,
			 void *buf, size_t count)
{
	int res;
	
	res = _kread_attr_(fd, attribute, type, pos, buf, count);
	if (res >= 0)
		return res;

	errno = res;
	return -1;
}

ssize_t
fs_write_attr(int fd, const char *attribute, uint32 type, off_t pos,
			  const void *buf, size_t count)
{
	int res;
	
	res = _kwrite_attr_(fd, attribute, type, pos, buf, count);
	if (res >= 0)
		return res;

	errno = res;
	return -1;
}


int
fs_remove_attr(int fd, const char *attr)
{
	int res;
	
	res = _kremove_attr_(fd, attr);
	if (res >= 0)
		return res;

	errno = res;
	return -1;
}

/* fs_rename_attr currently unused, when fully implemented, should get prototyped
 * and exported in fs_attr.h
 */
static int
fs_rename_attr(int fd, const char *old, const char *new)
{
	int res;
	
	res = _krename_attr_(fd, old, new);
	if (res >= 0)
		return res;

	errno = res;
	return -1;
}

DIR *
fs_open_attr_dir(const char *path)
{
	int		err;
	DIR		*d;

	d = (DIR *) malloc(sizeof(DIR) + B_FILE_NAME_LENGTH);
	if (!d) {
		err = ENOMEM;
		goto error1;
	}
	d->fd = _kopen_attr_dir_(-1, path, TRUE);
	if (d->fd < 0) {
		err = d->fd;
		goto error2;
	}

	return d;

error2:
	free(d);
error1:
	errno = err;
	return NULL;
}

DIR *
fs_fopen_attr_dir(int fd)
{
	int		err;
	DIR		*d;

	d = (DIR *) malloc(sizeof(DIR) + B_FILE_NAME_LENGTH);
	if (!d) {
		err = ENOMEM;
		goto error1;
	}
	d->fd = _kopen_attr_dir_(fd, NULL, TRUE);
	if (d->fd < 0) {
		err = d->fd;
		goto error2;
	}

	return d;

error2:
	free(d);
error1:
	errno = err;
	return NULL;
}

int
fs_close_attr_dir(DIR *d)
{
	int		err;

	err = _kclose_attr_dir_(d->fd);
	free(d);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

struct dirent *
fs_read_attr_dir(DIR *d)
{
	long		n;

	n = _kread_attr_dir_(d->fd, &d->ent, sizeof(d->ent)+B_FILE_NAME_LENGTH, 1);
	if (n == 1)
		return &d->ent;
	if (n == 0)
		return NULL;
	errno = n;
	return NULL;	

}

void
fs_rewind_attr_dir(DIR *dirp)
{
	_krewind_attr_dir_(dirp->fd);
}

int
fs_stat_attr(int fd, const char *name, struct attr_info *ai)
{
	int res;
	
	res = _kstat_attr_(fd, name, ai);
	if (!res)
		return 0;

	errno = res;
	return -1;
}

DIR *
fs_open_index_dir(dev_t device)
{
	int		err;
	DIR		*d;

	d = (DIR *) malloc(sizeof(DIR) + B_FILE_NAME_LENGTH);
	if (!d) {
		err = ENOMEM;
		goto error1;
	}
	d->fd = _kopen_index_dir_(device, TRUE);
	if (d->fd < 0) {
		err = d->fd;
		goto error2;
	}

	return d;

error2:
	free(d);
error1:
	errno = err;
	return NULL;
}

int
fs_close_index_dir(DIR *d)
{
	int		err;

	err = _kclose_index_dir_(d->fd);
	free(d);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

struct dirent *
fs_read_index_dir(DIR *d)
{
	long		n;

	n = _kread_index_dir_(d->fd, &d->ent, sizeof(d->ent)+B_FILE_NAME_LENGTH, 1);
	if (n == 1)
		return &d->ent;
	if (n == 0)
		return NULL;
	errno = n;
	return NULL;	
}

void
fs_rewind_index_dir(DIR *dirp)
{
	_krewind_index_dir_(dirp->fd);
}

int
fs_create_index(dev_t device, const char *name, int type, uint flags)
{
	int res;
	
	res = _kcreate_index_(device, name, type, flags);
	if (!res)
		return 0;

	errno = res;
	return -1;
}

int
fs_remove_index(dev_t device, const char *name)
{
	int res;
	
	res = _kremove_index_(device, name);
	if (!res)
		return 0;

	errno = res;
	return -1;
}

/* fs_rename_index currently unused, when fully implemented, should get prototyped
 * and exported in fs_attr.h
 */
static int
fs_rename_index(dev_t device, const char *old, const char *new)
{
	int res;
	
	res = _krename_index_(device, old, new);
	if (!res)
		return 0;

	errno = res;
	return -1;
}

int
fs_stat_index(dev_t device, const char *name, struct index_info *buf)
{
	int res;
	
	res = _kstat_index_(device, name, buf);
	if (!res)
		return 0;

	errno = res;
	return -1;
}

DIR *
fs_open_query(dev_t device, const char *query, uint32 flags)
{
	int		err;
	DIR		*d;

	d = (DIR *) malloc(sizeof(DIR) + B_FILE_NAME_LENGTH);
	if (!d) {
		err = ENOMEM;
		goto error1;
	}
	d->fd = _kopen_query_(device, query, flags, 0, 0, TRUE);
	if (d->fd < 0) {
		err = d->fd;
		goto error2;
	}

	return d;

error2:
	free(d);
error1:
	errno = err;
	return NULL;
}


DIR *
fs_open_live_query(dev_t device, const char *query, uint32 flags,
				   port_id port, int32 token)
{
	int		err;
	DIR		*d;

	d = (DIR *) malloc(sizeof(DIR) + B_FILE_NAME_LENGTH);
	if (!d) {
		err = ENOMEM;
		goto error1;
	}
	d->fd = _kopen_query_(device, query, flags | B_LIVE_QUERY,
						  port, token, TRUE);
	if (d->fd < 0) {
		err = d->fd;
		goto error2;
	}

	return d;

error2:
	free(d);
error1:
	errno = err;
	return NULL;
}


int
fs_close_query(DIR *d)
{
	int		err;

	err = _kclose_query_(d->fd);
	free(d);
	if (!err)
		return 0;
	errno = err;
	return -1;
}

struct dirent *
fs_read_query(DIR *d)
{
	long		n;

	n = _kread_query_(d->fd, &d->ent, sizeof(d->ent)+B_FILE_NAME_LENGTH, 1);
	if (n == 1)
		return &d->ent;
	if (n == 0)
		return NULL;
	errno = n;
	return NULL;	
}


int
getdtablesize(void)
{
	return OPEN_MAX;
}


int
pause(void)
{
    sigset_t junk;

    sigemptyset(&junk);

    sigsuspend(&junk);

    return -1;
}


speed_t
(cfgetispeed)(const struct termios *tp)
{
	return (tp->c_cflag & CBAUD);
}

speed_t
(cfgetospeed)(const struct termios *tp)
{
	return (tp->c_cflag & CBAUD);
}

int
(cfsetispeed)(struct termios *tp, speed_t spd)
{
	return (0);
}

int
(cfsetospeed)(struct termios *tp, speed_t sp)
{
	if (sp & ~CBAUD) {
		errno = EINVAL;
		return (-1);
	}
	tp->c_cflag = tp->c_cflag&~CBAUD | sp;
	return (0);
}


/* XXXdbg this needs lots of work */
int
(sigsetjmp)(jmp_buf jmp, int x)
{
#if __ELF__
	return setjmp(jmp);
#elif __POWERPC__
	return setjmp((long **)jmp);
#endif
}

/* XXXdbg this needs lots of work */
void
siglongjmp(jmp_buf jmp, int val)
{
#if __ELF__
	longjmp(jmp, val);
#elif __POWERPC__
	longjmp((long **)jmp, val);
#endif
}


int
fchown(int fd, uid_t owner, gid_t group)
{
   int err;
   struct stat st;

   st.st_uid = owner;
   st.st_gid = group;

   err = _kwstat_(fd, NULL, &st, WSTAT_UID | WSTAT_GID, 0);
   if (err == 0)
      return err;

   errno = err;
   return -1;
}

pid_t
#if __ELF__
__getpid(void)
#else
getpid(void)
#endif
{
	return find_thread(NULL);
}
#if __ELF__
#pragma weak getpid = __getpid
#endif

#if __ELF__ /* XXX pragma weak? */ 
int 
_fseek(FILE *a, fpos_t b, int c)
{
	return fseeko(a, b, c);
}

fpos_t
_ftell(FILE *a)
{
	return ftello(a);
}
#endif /* __ELF__ */

int 
setpgrp(void)
{
	return setpgid(0,0);
}


unsigned int
alarm(unsigned int sec)
{
	unsigned int	res;
	bigtime_t		left, timeout;
	int				mode;

	timeout = sec * (bigtime_t)1000000;
	mode = B_ONE_SHOT_RELATIVE_ALARM;
	if (sec == 0) {
		timeout = B_INFINITE_TIMEOUT;
		mode = B_ONE_SHOT_ABSOLUTE_ALARM;
	}
	left = set_alarm(timeout, mode);
	if (left == B_INFINITE_TIMEOUT)
		return 0;
	res = (unsigned int)((left - system_time() + 500000) / 1000000);
	if (res == 0)
		res = 1;
	return res;
}


int select(int fd, fd_set *rfds, fd_set *wfds, fd_set *efds, struct timeval *timeout)
{
	bigtime_t waittime;
	int rc;
	
	if( timeout )
		waittime = ((bigtime_t)timeout->tv_sec << 20) + timeout->tv_usec;
	else
		waittime = B_INFINITE_TIMEOUT;
	
	/* Lame use of select to sleep with us precision hack */
	/* This should be fixed in the kernel instead! */
	if( rfds == NULL && wfds == NULL && efds == NULL )
	{
		snooze( waittime );
		return 0;
	}
	
	if( (rc = (int) waiton(fd, rfds->fds_bits, wfds->fds_bits, efds->fds_bits, waittime)) < 0 )
	{
		/* Until this gets fixed in the kernel, we are going to remap EAGAIN to EINTR.
		/* kernel select remaps B_INTERRUPTED -> EAGAIN for some reason and this
		/* is not compatible with most things which expect "normal" error codes from
		/* select. Eventually this should be fixed in the kernel because EAGAIN == B_WOULD_BLOCK
		/* which may be a valid return code if the timeout is set to zero. */
		//if( rc == EAGAIN )
		//	rc = EINTR;
		
		errno = rc;
		rc = -1;
	}
	return rc;	
}


int gethostname(char *hname, size_t namelen)
{
	int fd;
	int rc = -1;
	int rd;
	char *stop;
	
	if( namelen == 0 )
		return -1;
	
	fd = open("/etc/hostname", O_RDONLY);
	if(fd < 0)
		fd = open("/etc/HOSTNAME", O_RDONLY);
		
	if(fd >= 0)
	{
		if((rd = read(fd, hname, namelen - 1)) >= 0)
		{
			hname[rd] = 0;
			if( (stop = strpbrk(hname, "\r\n")) != NULL )
				*stop = 0;
 			rc = 0;
		}
		else
			hname[0] = 0;
		close(fd);
	}
	else
		hname[0] = 0;
	
	return rc;
}


int sethostname(const char *hname, size_t namelen)
{
	int fd;
	int rc = -1;
	
	fd = open("/etc/hostname", O_CREAT | O_WRONLY);
	if(fd > 0)
	{
		write(fd, hname, namelen);
		write(fd, "\n", 1);
		rc = 0;
		close(fd);
	}
	return rc;
}


int
uname(struct utsname *name)
{
	system_info sysinfo;

	if(name == 0)
	{
		errno = EINVAL;
		return -1;
	}

	get_system_info(&sysinfo);
	
	strcpy(name->sysname, "BeOS");

	name->nodename[0] = '\0';
	gethostname(name->nodename, sizeof(name->nodename));
	if (name->nodename[0] == '\0')
		strcpy(name->nodename, "trantor");
	name->nodename[sizeof(name->nodename)-1] = '\0';  /* null-terminate! */
	
	if (sysinfo.platform_type == B_BEBOX_PLATFORM)
		strcpy(name->machine, "BeBox");
	else if (sysinfo.platform_type == B_MAC_PLATFORM)
		strcpy(name->machine, "BeMac");
	else if (sysinfo.platform_type == B_AT_CLONE_PLATFORM)
		strcpy(name->machine, "BePC");
	else
		strcpy(name->machine, "BeUnknown");

	/* B_BEOS_VERSION is 1 byte major, 1 nibble minor, 1 nibble patchlevel */
	sprintf(name->release, "%x.%x", (B_BEOS_VERSION >> 8) & 0xff,
									(B_BEOS_VERSION >> 4) & 0xf);
	if ((B_BEOS_VERSION & 0xf) != 0)
		sprintf(name->release, "%s.%x", name->release, (B_BEOS_VERSION & 0xf));
		
	sprintf(name->version, "%Ld", sysinfo.kernel_version);
	
	return 0;
}
