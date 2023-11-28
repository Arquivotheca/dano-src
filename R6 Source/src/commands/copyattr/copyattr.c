#include <stdio.h>
#include <string.h>
#include <fs_attr.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <TypeConstants.h>
#include <sys/stat.h>
#include <Mime.h>
#include <byteorder.h>
#include <stdlib.h>
#include <alloca.h>
#include <utime.h>
#include <dirent.h>
#include <signal.h>


#define BLKSIZE (512*1024L)


int verbose = 0;
char * removefile = NULL;

void
sig_handler(
	int signal)
{
	char * ptr = removefile;
	if (ptr != NULL) {
		unlink(ptr);
	}
	exit(1);
}


static void
usage(
	const char *	arg)
{
	if (arg)
		fprintf(stderr, "bad argument: %s\n", arg);
	fprintf(stderr, "usage: copyattr [ options ] <source> [ ... ] <destination>\n");
	fprintf(stderr, "-n, --name <name>      copy attribute <name>\n");
	fprintf(stderr, "-t, --type <type>      copy attributes of <type>\n");
	fprintf(stderr, "-d, --data             copy data of file, too\n");
	fprintf(stderr, "-r, --recursive        copy directories recursively\n");
	fprintf(stderr, "-u, --touch            do not preserve modification time\n");
	fprintf(stderr, "-m, --move             remove source when copied\n");
	fprintf(stderr, "-v, --verbose          print more messages\n");
	fprintf(stderr, " -, --                 end of options\n");
	fprintf(stderr, "<type> is one of: int, llong, string, mimestr, float, double, boolean.\n");
	exit(1);
}

static void
error(
	const char *	name)
{
	perror(name);
	exit(2);
}

static status_t
do_error(
	const char * arg)
{
	status_t err = errno;
	if (verbose) {
		perror(arg);
	}
	return err;
}


static void
fatal(
	const char *	fmt,
	...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(2);
}


static const char *
get_attrname(
	const char * name)
{
	if (!name)
		usage("missing attribute name");
	return name;
}


static uint32
get_type(
	const char * name)
{
	uint32 ret = 0;

	if (!strcmp(name, "string"))
		return B_STRING_TYPE;
	if (!strncmp(name, "mimestr", 7))
		return B_MIME_STRING_TYPE;
	if (!strcmp(name, "int64") || !strcmp(name, "llong"))
		return B_INT64_TYPE;
	if (!strcmp(name, "int32") || !strcmp(name, "int"))
		return B_INT32_TYPE;
	if (!strcmp(name, "int16"))
		return B_INT16_TYPE;
	if (!strcmp(name, "int8"))
		return B_INT8_TYPE;
	if (!strcmp(name, "boolean"))
		return B_BOOL_TYPE;
	if (!strcmp(name, "float"))
		return B_FLOAT_TYPE;
	if (!strcmp(name, "double"))
		return B_DOUBLE_TYPE;
	if (strlen(name) != 4)
		usage("type code is not recognized");
	memcpy(&ret, name, 4);
	ret = B_BENDIAN_TO_HOST_INT32(ret);
	return ret;
}


static status_t copy_attr_recursive(const char * source, const char * destination,
	const char * aname, uint32 atype, int touch);

static status_t
copy_attr(
	const char *	source,
	const char *	destination,
	const char *	aname,
	uint32			atype,
	int			recursive,
	int			touch)
{
	int srcf = open(source, O_RDONLY | O_NOTRAVERSE);
	DIR * srcd = NULL;
	int dstf = -1;
	struct dirent * dent;
	struct attr_info info;
	void * data;
	struct utimbuf ut;
	struct stat srcstat;
	status_t err = B_OK;

	if (srcf < 0) error(source);
	dstf = open(destination, O_RDWR | O_NOTRAVERSE);
	if (dstf < 0) error(source);

	srcd = fs_fopen_attr_dir(srcf);
	if (!srcd) error(source);

	while ((dent = fs_read_attr_dir(srcd)) != NULL)
	{
		if (aname && strcmp(dent->d_name, aname)) continue;
		if (fs_stat_attr(srcf, dent->d_name, &info)) error(dent->d_name);
		if (atype && (atype != info.type)) continue;
		/* so copy the attribute */
		data = malloc(info.size);
		if (data == NULL) error(dent->d_name);
		if (info.size != fs_read_attr(srcf, dent->d_name, info.type, 0, data, info.size))
			error(dent->d_name);
		fs_remove_attr(dstf, dent->d_name);
		if (info.size != fs_write_attr(dstf, dent->d_name, info.type, 0, data, info.size))
			error(dent->d_name);
		free(data);
	}
	fs_close_attr_dir(srcd);
	close(srcf);
	close(dstf);
	if (!lstat(source, &srcstat)) {
		if (recursive && S_ISDIR(srcstat.st_mode)) {
			err = copy_attr_recursive(source, destination, aname, atype, touch);
		}
		/* UNIX attributes */
		if (!S_ISLNK(srcstat.st_mode) && !err && chmod(destination, srcstat.st_mode & 0777)) {
			err = do_error(destination);
		}
		if ( !touch ) {
			ut.actime = srcstat.st_atime;
			ut.modtime = srcstat.st_mtime;
			if (!S_ISLNK(srcstat.st_mode) && !err && utime(destination, &ut)) {
				err = do_error(destination);
			}
		}
		/* File ownership */
		if (!S_ISLNK(srcstat.st_mode) && !err && chown(destination, srcstat.st_gid, srcstat.st_uid)) {
			if(errno != EACCES) {
				err = do_error(destination);
			}
		}
	}

	return err;
}

static status_t
copy_attr_recursive(
	const char * source,	/* we know source is a directory */
	const char * destination,
	const char * aname,
	uint32 atype,
	int touch)
{
	struct stat dststat;
	struct dirent * ent;
	DIR * d;
	char * path = (char *)malloc(strlen(source)+257);
	char * dpath = (char *)malloc(strlen(destination)+257);
	status_t err = B_OK;

	if (!path || !dpath) {
		free(path);
		free(dpath);
		err = B_NO_MEMORY;
		if (verbose) {
			fprintf(stderr, "%s: out of memory\n", source);
		}
		return err;
	}
	if (stat(destination, &dststat) || !S_ISDIR(dststat.st_mode)) {
		fatal("%s is a directory; %s is not!", source, destination);
	}
	d = opendir(source);
	if (d != NULL) {
		while ((ent = readdir(d)) != NULL) {
			if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}
			strcpy(path, source);
			strcat(path, "/");
			strcat(path, ent->d_name);
			strcpy(dpath, destination);
			strcat(dpath, "/");
			strcat(dpath, ent->d_name);
			if ((err = copy_attr(path, dpath, aname, atype, 1, touch)) < B_OK) {
				break;
			}
		}
		closedir(d);
	}
	else {
		err = do_error(source);
	}
	free(path);
	free(dpath);
	return err;
}


static char *
tempdest(
	const char * path)
{
	struct stat st;
	static int32 cnt = 1;
	char *ret = malloc(strlen(path)+32);
	char * ptr;
	if (!ret) {
		error(path);
	}
	strcpy(ret, path);
	if (ret[strlen(ret)-1] == '/') {
		ret[strlen(ret)-1] = 0;
	}
	ptr = strrchr(ret, '/');
	if (ptr) {
		ptr += 1;
	}
	else {
		ptr = ret;
	}
again:
	sprintf(ptr, "_tempdest_%x_%x", find_thread(NULL), atomic_add(&cnt, 1));
	if (!stat(ret, &st)) {
		/* existing name is not OK */
		goto again;
	}
	return ret;
}


/* four megs should be enough */
#define COPY_SIZE (65536)


static status_t
copy_data(
	const char *	source,
	const char *	destination,
	const char *	aname,
	uint32			atype,
	int				move,
	int				touch)
{
	int srcf;
	char * d2 = tempdest(destination);
	int dstf;
	size_t size;
	char * data;
	ssize_t wr;
	ssize_t rd;
	ssize_t cur;
	char buf[1024];
	status_t err = B_OK;
	int dsterr;

	if (verbose) {
		puts(destination);
	}
	dsterr = stat(destination, (struct stat *)buf);

	srcf = open(source, O_RDONLY);
	if (srcf < 0) {
		err = do_error(source);
		return err;
	}
	removefile = d2;
	dstf = open(d2, O_RDWR | O_TRUNC | O_CREAT, 0777);
	if (dstf < 0) {
		err = do_error(destination);
		unlink(d2);
		removefile = NULL;
		free(d2);
		return err;
	}

	for (size = COPY_SIZE; size>1024; size--) {
		data = (char*)malloc(size);
		if (data != NULL) {
			break;
		}
	}
	if (!data) {
		data = buf;
		size = 1024;
	}
again:
	while ((rd = read(srcf, data, size)) > 0) {
		cur = 0;
		do {
			if ((wr = write(dstf, data+cur, rd-cur)) < 0) {
				if (errno == B_INTERRUPTED) {
					continue;
				}
				err = do_error(destination);
				break;
			}
			cur += wr;
		} while (cur < rd);
	}
	if ((rd < 0) && (errno == B_INTERRUPTED)) {
		goto again;
	}
	free(data);
	close(srcf);
	close(dstf);
	if (err) {
		unlink(d2);
	}
	else if (!dsterr && unlink(destination)) {
		err = do_error(destination);
		unlink(d2);
	}
	else if (rename(d2, destination)) {
		err = do_error(destination);
	}
	removefile = NULL;
	free(d2);
	if (!err) {
		err = copy_attr(source, destination, aname, atype, 0, touch);
	}
	if (!err && move && unlink(source)) {
		err = do_error(source);
	}
	return err;
}


static status_t
copy_data_recursive(
	const char *	source,
	const char *	destination,
	const char *	aname,
	uint32			atype,
	int				move,
	int				touch)
{
	struct stat srcstat;
	struct stat dststat;
	status_t dsterr = B_OK;
	status_t err = B_OK;
	int islink = 0;

	if (lstat(source, &srcstat)) {
		err = do_error(source);
		return err;
	}
	dsterr = lstat(destination, &dststat);

	/* switch on what we're copying */
	if (S_ISREG(srcstat.st_mode)) {
		err = copy_data(source, destination, aname, atype, move, touch);
	}
	else if (S_ISLNK(srcstat.st_mode)) {
		char * link;
		char * d2;
		int linklen;
		if (verbose) {
			puts(destination);
		}
		if (!dsterr && !S_ISREG(dststat.st_mode) && !S_ISLNK(dststat.st_mode)) {
			err = EEXIST;
			if (verbose) {
				fprintf(stderr, "%s: cannot replace with link\n", destination);
			}
			return err;
		}
		link = (char *)malloc(2048);
		if ((linklen = readlink(source, link, 2048)) < 0) {
			err = do_error(source);
			free(link);
			return err;
		}
		link[linklen] = 0;
		d2 = tempdest(destination);
		removefile = d2;
		if (symlink(link, d2) < 0) {
			err = do_error(destination);
			unlink(d2);
		}
		else if (!dsterr && unlink(destination)) {
			err = do_error(destination);
			unlink(d2);
		}
		else if (rename(d2, destination)) {
			err = do_error(destination);
		}
		removefile = NULL;
		free(d2);
		if (!err) {
			err = copy_attr(source, destination, aname, atype, 0, touch);
		}
		if (!err && move && unlink(source)) {
			err = do_error(source);
		}
		free(link);
	}
	else if (S_ISDIR(srcstat.st_mode)) {
		struct stat lnkstat;
		status_t direrr = B_OK;
		if (verbose) {
			puts(destination);
		}
		if (!dsterr && S_ISLNK(dststat.st_mode)) {
			if (stat(destination, &lnkstat)) {
				if (unlink(destination)) {
					err = do_error(destination);
				}
				dsterr = -1;
			}
			else if (!S_ISDIR(lnkstat.st_mode)) {
				err = EEXIST;
				if (verbose) {
					fprintf(stderr, "%s: cannot replace file with directory\n", destination);
				}
			}
		}
		if (!err && dsterr && mkdir(destination, 0777)) {
			direrr = do_error(destination);
		}
		if (!err && !direrr) {
			char * spath = (char *)malloc(strlen(source)+257);
			char * dpath = (char *)malloc(strlen(destination)+257);
			char * sptr;
			char * dptr;
			if (!spath || !dpath) {
				free(spath);
				free(dpath);
				err = B_NO_MEMORY;
			}
			strcpy(spath, source);
			sptr = strrchr(spath, '/');
			if (!sptr) {
				sptr = spath + strlen(spath);
			}
			else if (sptr[1] != 0) {
				sptr += strlen(sptr);
			}
			*(sptr++) = '/';
			strcpy(dpath, destination);
			dptr = strrchr(dpath, '/');
			if (!dptr) {
				dptr = dpath + strlen(dpath);
			}
			else if (dptr[1] != 0) {
				dptr += strlen(dptr);
			}
			*(dptr++) = '/';
			/* finally, walk the directory */
			if (!err) {
				DIR * d;
				struct dirent * ent;
				d = opendir(source);
				if (d != NULL) {
					while ((ent = readdir(d)) != NULL) {
						status_t e = B_OK;
						if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
							continue;
						}
						strcpy(sptr, ent->d_name);
						strcpy(dptr, ent->d_name);
						e = copy_data_recursive(spath, dpath, aname, atype, move, touch);
						if (e < 0) {
							err = e;
						}
					}
					closedir(d);
				}
				else {
					err = do_error(source);
				}
			}
			free(spath);
			free(dpath);
		}
		if (!direrr) {
			err = copy_attr(source, destination, aname, atype, 0, touch);
		}
		else {
			err = direrr;
		}
		if (!err && move && rmdir(source)) {
			err = do_error(source);
		}
	}
	else {
		err = ENOENT;
		if (verbose) {
			fprintf(stderr, "%s: unknown file kind\n");
		}
	}
	return err;
}


int
main(
	int				argc,
	char *			argv[])
{
	const char * aname = NULL;
	uint32 atype = 0;
	int ix = 0;
	char * srcfile = NULL;
	char * dstfile = NULL;
	int data = 0;
	int ignore = 0;
	int move = 0;
	int recursive = 0;
	int touch = 0;
	struct stat dststat;
	int dirdest = 0;

	signal(SIGINT, sig_handler);

	for (ix=1; ix<argc; ix++)
	{
		if (!ignore &&
			!strcmp(argv[ix], "-") ||
			!strcmp(argv[ix], "--"))
		{
			ignore = 1;
		}
		else if (!ignore &&
			!strcmp(argv[ix], "-d") ||
			!strcmp(argv[ix], "--data"))
		{
			if (data) usage("--data can only be specified once");
			data = 1;
		}
		else if (!ignore &&
			!strcmp(argv[ix], "-u") ||
			!strcmp(argv[ix], "--touch"))
		{
			if (data) usage("--touch can only be specified once");
			touch = 1;
		}
		else if (!ignore &&
			!strcmp(argv[ix], "-n") ||
			!strcmp(argv[ix], "--name"))
		{
			if (aname) usage("--name can only be specified once");
			aname = get_attrname(argv[++ix]);
			if (!aname)
				usage(argv[ix]);
		}
		else if (!ignore &&
			!strcmp(argv[ix], "-t") ||
			!strcmp(argv[ix], "--type"))
		{
			if (atype) usage("--type can only be specified once");
			atype = get_type(argv[++ix]);
			if (!atype)
				usage(argv[ix]);
		}
		else if (!ignore &&
			!strcmp(argv[ix], "-m") ||
			!strcmp(argv[ix], "--move"))
		{
			if (move) usage("--move can only be specified once");
			move = 1;
		}
		else if (!ignore &&
			!strcmp(argv[ix], "-r") ||
			!strcmp(argv[ix], "--recursive"))
		{
			if (recursive) usage("--recursive can only be specified once");
			recursive = 1;
		}
		else if (!ignore && 
			!strcmp(argv[ix], "-v") || 
			!strcmp(argv[ix], "--verbose"))
		{
			if (verbose) usage("--verbose can only be specified once");
			verbose = 1;
		}
		else if (!ignore && (argv[ix][0] == '-'))
		{
			usage(argv[ix]);
		}
		else
		{
			break; /* done with options */
		}
	}
	if (ix > argc-2) {
		usage(NULL);	/* must have at least two arguments */
	}
	dstfile = argv[argc-1];
	if (!stat(dstfile, &dststat) && S_ISDIR(dststat.st_mode)) {
		dirdest = 1;
	}
	if (ix < argc-2) {
		if (!dirdest) {
			fatal("copyattr: %s is not a directory (required when copying multiple files)\n", 
				dstfile);
		}
	}
	/* do each copy as its own thing */
	while (ix <= argc-2) {
		status_t err = B_OK;
		srcfile = strdup(argv[ix]);
		if (!srcfile) {
			err = B_NO_MEMORY;
		}
		else {
			if (srcfile[strlen(srcfile)-1] == '/') {
				srcfile[strlen(srcfile)-1] = 0;
			}
		}
		dstfile = argv[argc-1];
		if (!err && dirdest && (recursive || data)) {
			const char * name = strrchr(srcfile, '/');
			if (name != NULL) {
				name += 1;
			}
			else {
				name = srcfile;
			}
			dstfile = (char *)malloc(strlen(argv[argc-1])+257);
			if (dstfile) {
				strcpy(dstfile, argv[argc-1]);
				if (dstfile[strlen(dstfile)-1] != '/') {
					strcat(dstfile, "/");
				}
				strcat(dstfile, name);
			}
			else {
				err = B_NO_MEMORY;
			}
		}
		if (!err && data) {
				if (recursive) {
					err = copy_data_recursive(srcfile, dstfile, aname, atype, move, touch);
				}
				else {
					err = copy_data(srcfile, dstfile, aname, atype, move, touch);
				}
		}
		else if (!err) {
			err = copy_attr(srcfile, dstfile, aname, atype, recursive, touch);
		}
		if (dstfile != argv[argc-1]) {
			free(dstfile);
		}
		if (err) {
			if (!verbose) {
				fprintf(stderr, "%s: [%x] %s\n", srcfile, err, strerror(err));
			}
		}
		free(srcfile);
		ix++;
	}
	return 0;
}

