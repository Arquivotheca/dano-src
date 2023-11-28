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
#include <OS.h>

#include "main.h"

#define BLKSIZE (512*1024L)


char * removefile = NULL;

void
sig_handler(
	int signal)
{
	char * ptr = removefile;
	(void)signal;
	if (ptr != NULL) {
		unlink(ptr);
	}
	exit(1);
}

static status_t
do_error(
	const char * arg)
{
	status_t err = errno;
	perror(arg);
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

static status_t
copy_attr(
	const char *	source,
	const char *	destination,
	struct beinstall_options *opt)
{
	int srcf = -1;
	DIR * srcd = NULL;
	int dstf = -1;
	struct dirent * dent;
	struct attr_info info;
	void * data;
	status_t err = B_OK;

	if(!opt->ignore_attributes) {
		srcf = open(source, O_RDONLY | (opt->dereference_links ? 0 : O_NOTRAVERSE));
		if (srcf < 0) error(source);
		dstf = open(destination, O_RDWR | O_NOTRAVERSE);
		if (dstf < 0) error(source);

		srcd = fs_fopen_attr_dir(srcf);
		if (!srcd) error(source);
	
		while ((dent = fs_read_attr_dir(srcd)) != NULL)
		{
			if (fs_stat_attr(srcf, dent->d_name, &info)) error(dent->d_name);
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
	}
	if(!err) {
		err = copy_posix_attr(source, destination, opt, -1);
	}
	return err;
}

/* The mode-override is used when creating preceding directories for an installed file */
status_t
copy_posix_attr(
	const char *	source,
	const char *	destination,
	struct beinstall_options *opt,
	unsigned int mode)
{
	int dstf = -1;
	struct utimbuf ut;
	struct stat srcstat;
	struct stat dststat;
	status_t err = B_OK;

	if( source != NULL ) {
		if(opt->dereference_links) {
			err = stat(source, &srcstat);
		}else{
			err = lstat(source, &srcstat);
		}
	}
	if(mode == NO_MODE_OVERRIDE){
		mode = opt->pmode;
	}
	if( !err ) {
		/* We may need to dereference the source, but lstat() should always be
		 * accurate for the destination */
		err = lstat(destination, &dststat);
		if( source == NULL ) {
			/* If source is null, then we simply may or may not be over-riding
			 * the current posix attributes */
			memcpy(&srcstat, &dststat, sizeof(struct stat));
		}
	}
	if (!err) {
		/* UNIX attributes */
		
		/* File ownership */
		if(source != NULL || (opt->use_pgroup && opt->pgroup != srcstat.st_gid) ||
		  (opt->use_powner && opt->powner != srcstat.st_uid)) {
			if(S_ISLNK(dststat.st_mode)) {
				/* for symlinks we need to use fchown() */
				dstf = open(destination, O_RDWR | O_NOTRAVERSE);
				if (dstf < 0) error(source);
				if (!err && fchown(dstf,
				  (opt->use_pgroup ? opt->pgroup : srcstat.st_gid),
				  (opt->use_powner ? opt->powner : srcstat.st_uid))) {
					if(errno != EACCES) {
						err = do_error(destination);
					}
				}
				close(dstf);
			}
			else {
				/* For directories, it's probably best to avoid fchown() */
				/* Plain files don't matter and either method could be used. */
				if (!err && chown(destination,
				  (opt->use_pgroup ? opt->pgroup : srcstat.st_gid),
				  (opt->use_powner ? opt->powner : srcstat.st_uid))) {
					if(errno != EACCES) {
						err = do_error(destination);
					}
				}
			}
		}
		
		/* File permission bits */
		if(source != NULL || (opt->use_pmode && (srcstat.st_mode & 07777) != mode)) {
			if (!S_ISLNK(dststat.st_mode) && !err && chmod(destination,
			  (opt->use_pmode == 0 ? srcstat.st_mode & 07777 : mode))) {
				err = do_error(destination);
			}
		}
		/* Time-stamp */
		if ( opt->preserve_timestamps ) {
			if (source != NULL ) {
				ut.actime = srcstat.st_atime;
				ut.modtime = srcstat.st_mtime;
				if (!S_ISLNK(dststat.st_mode) && !err && utime(destination, &ut)) {
					err = do_error(destination);
				}
			}
		} else {
			/* Note: We can't update the time-stamp on symlinks. :/ */
			if(!S_ISLNK(dststat.st_mode) && !err && utime(destination, NULL)){
				err = do_error(destination);
			}
		}
	}

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
	sprintf(ptr, "_tempdest_%lx_%lx", find_thread(NULL), atomic_add(&cnt, 1));
	if (!stat(ret, &st)) {
		/* existing name is not OK */
		goto again;
	}
	return ret;
}


/* four megs should be enough */
#define COPY_SIZE (4096L*1024L)

static status_t
copy_link(
	const char *source,
	const char *destination,
	struct beinstall_options *opt)
{
	status_t dsterr = B_OK;
	status_t err = B_OK;
	struct stat srcstat;
	struct stat dststat;
	char * link;
	char * d2;
	int linklen;

	/* The only time we get here when dereference_links is enabled is when we're
	 * dealling with a broken symlink. */
	err = lstat(source, &srcstat);
	if (err) {
		err = do_error(source);
		return err;
	}
	dsterr = lstat(destination, &dststat);
	if(!dsterr && opt->force) {
		unlink(destination);
		dsterr = lstat(destination, &dststat);
	}

	if (opt->verbose) {
		puts(destination);
	}
	if (!dsterr && !S_ISREG(dststat.st_mode) && !S_ISLNK(dststat.st_mode)) {
		err = EEXIST;
		if (opt->verbose) {
			fprintf(stderr, "%s: cannot replace with link\n", destination);
		}
		return err;
	}
	link = (char *)malloc(2048);
	if ((linklen = readlink(source, link, 2048)) < 0) {
		err = do_error(source);
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
		err = copy_attr(source, destination, opt);
	}
	return(err);
}

static status_t
copy_data(
	const char *	source,
	const char *	destination,
	struct beinstall_options *opt)
{
	struct stat srcstat;
	int srcf;
	char * d2 = tempdest(destination);
	int dstf;
	size_t size;
	char * data = NULL;
	ssize_t wr;
	ssize_t rd;
	ssize_t cur;
	char buf[1024];
	status_t err = B_OK;
	int dsterr;

	if(opt->dereference_links) {
		err = stat(source, &srcstat);
	}
	else {
		err = lstat(source, &srcstat);
	}
	if (err) {
		err = do_error(source);
		return err;
	}

	if (S_ISLNK(srcstat.st_mode)) {
		return(copy_link(source, destination, opt));
	}

	if (opt->verbose) {
		puts(destination);
	}
	if(opt->directories_if_needed){
		create_directory(destination, opt, 0755, 1);
	}
	dsterr = stat(destination, (struct stat *)buf);
	if(!dsterr && opt->force) {
		unlink(destination);
		dsterr = stat(destination, (struct stat *)buf);
	}

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
		err = copy_attr(source, destination, opt);
	}
	return err;
}


static status_t
copy_data_recursive(
	const char *	source,
	const char *	destination,
	struct beinstall_options *opt)
{
	struct stat srcstat;
	struct stat dststat;
	status_t dsterr = B_OK;
	status_t err = B_OK;

	if(opt->directories_if_needed){
		create_directory(destination, opt, 0755, 1);
	}

	if(opt->dereference_links) {
		err = stat(source, &srcstat);
	}
	else {
		err = lstat(source, &srcstat);
	}
	if (err) {
		err = do_error(source);
		return err;
	}
	dsterr = lstat(destination, &dststat);

	/* switch on what we're copying */
	if (S_ISREG(srcstat.st_mode)) {
		err = copy_data(source, destination, opt);
	}
	else if (S_ISDIR(srcstat.st_mode)) {
		struct stat lnkstat;
		status_t direrr = B_OK;
		if (opt->verbose) {
			puts(destination);
		}
		if(opt->force && !dsterr){
			unlink(destination);
			dsterr = lstat(destination, &dststat);
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
				if (opt->verbose) {
					fprintf(stderr, "%s: cannot replace file with directory\n", destination);
				}
			}
		}
		/* Note: It doesn't make sense to use the --mode for directories
		 *   created with --recursive */
		if (!err && dsterr && mkdir(destination, srcstat.st_mode & 0777)) {
			direrr = do_error(destination);
		}
		if (!err && !direrr) {

			char * spath = (char *)malloc(strlen(source)+257);
			char * dpath = (char *)malloc(strlen(destination)+257);
			char * sptr = NULL;
			char * dptr = NULL;

			if (!spath || !dpath) {
				err = B_NO_MEMORY;
			}
			if(!err) {
				err = copy_attr(source, destination, opt);
			}
			if(spath!=NULL){
				strcpy(spath, source);
				sptr = strrchr(spath, '/');
				if (!sptr) {
					sptr = spath + strlen(spath);
				}
				else if (sptr[1] != 0) {
					sptr += strlen(sptr);
				}
				*(sptr++) = '/';
			}
			if(dpath!=NULL) {
				strcpy(dpath, destination);
				dptr = strrchr(dpath, '/');
				if (!dptr) {
					dptr = dpath + strlen(dpath);
				}
				else if (dptr[1] != 0) {
					dptr += strlen(dptr);
				}
				*(dptr++) = '/';
			}
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
						e = copy_data_recursive(spath, dpath, opt);
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
			if(spath!=NULL) free(spath);
			if(dpath!=NULL) free(dpath);
		}
	}
	else if (S_ISLNK(srcstat.st_mode)) {
		err = copy_link(source, destination, opt);
	}
	else {
		err = ENOENT;
		if (opt->verbose) {
			fprintf(stderr, "%s: unknown file kind\n", source);
		}
	}
	return err;
}


int
copyattr_start(struct beinstall_options *opt)
{
	int ix = 0;
	char * srcfile = NULL;
	char * dstfile = NULL;
	struct stat dststat;
	int dirdest = 0;

	signal(SIGINT, sig_handler);

	dstfile = opt->dest;
	if (!stat(dstfile, &dststat) && S_ISDIR(dststat.st_mode)) {
		dirdest = 1;
	}
	if (opt->install_mode==BEINSTALL_MODE_FILES) {
		if (!dirdest) {
			fatal("%s: %s is not a directory (required when copying multiple files)\n", 
				argv0, dstfile);
		}
	}
	/* do each copy as its own thing */
	ix = 0;
	while (opt->sources[ix]!=NULL) {
		status_t err = B_OK;
		srcfile = strdup(opt->sources[ix]);
		if (!srcfile) {
			err = B_NO_MEMORY;
		}
		else {
			if (srcfile[strlen(srcfile)-1] == '/') {
				srcfile[strlen(srcfile)-1] = 0;
			}
		}
		dstfile = opt->dest;
		if (!err && dirdest) {
			const char * name = strrchr(srcfile, '/');
			if (name != NULL) {
				name += 1;
			}
			else {
				name = srcfile;
			}
			dstfile = (char *)malloc(strlen(opt->dest)+257);
			if (dstfile) {
				strcpy(dstfile, opt->dest);
				if (dstfile[strlen(dstfile)-1] != '/') {
					strcat(dstfile, "/");
				}
				strcat(dstfile, name);
			}
			else {
				err = B_NO_MEMORY;
			}
		}
		if (!err) {
				if (opt->recursive) {
					err = copy_data_recursive(srcfile, dstfile, opt);
				}
				else {
					err = copy_data(srcfile, dstfile, opt);
				}
		}
		if (dstfile != opt->dest) {
			free(dstfile);
		}
		if (err) {
			if (!opt->verbose) {
				fprintf(stderr, "%s: [%lx] %s\n", dstfile, err, strerror(err));
			}
		}
		free(srcfile);
		ix++;
	}
	return 0;
}

