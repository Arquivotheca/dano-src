
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "main.h"

int
create_directory(
	const char * path,
	struct beinstall_options *opt,
	unsigned int mode,
	int make_parent)
{
	char nupath[1024];
	char nudir[256];
	const char * slash;
	char *pbuf = NULL;
	int err = 0;
	char *cp;

	getcwd(nupath, 1024);
	if(make_parent) {
		pbuf = strdup(path);
		if(pbuf!=NULL) {
			cp = strrchr(pbuf,'/');
			if(cp==NULL || cp == pbuf){
				path = NULL;
			} else {
				*cp = '\0';
				path = pbuf;
			}
		}
	}
	if (path!=NULL && chdir(path)) {
		if(opt->verbose) {
			printf("Making directory: %s... ",path);
		}
		if (*path == '/')
		{
			chdir("/");
			while(*path == '/') path++;
		}
		slash = path;
		while (*path)
		{
			slash = strchr(path, '/');
			if (!slash) {
				if(make_parent && pbuf==NULL) {
					path = NULL;
					continue;
				} else {
					slash = path+strlen(path);
				}
			}
			strncpy(nudir, path, slash-path);
			nudir[slash-path] = 0;
			path = slash;
			while(*path == '/') path++;
			if(!mkdir(nudir, 0777)) {
				/* Only if we actually created the directory do we tweak it's attrs */
				err = copy_posix_attr(NULL, nudir, opt, mode);
			}
			if (chdir(nudir) < 0)
			{
				err = errno;
				if(opt->verbose) {
					printf("Error creating %s: %s, 0x%x\n",nudir, strerror(err),err);
				}
				break;
			}
		}
	}
	if(!err && path!=NULL) {
		/* Always tweak the attrs of our target */
		err = copy_posix_attr(NULL, path, opt, mode);
	}
	if(pbuf!=NULL) free(pbuf);
	chdir(nupath);
	return err;
}
