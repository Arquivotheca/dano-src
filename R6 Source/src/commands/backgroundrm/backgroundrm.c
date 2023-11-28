#include <ctype.h>
#include <image.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fs_attr.h>
#include <errno.h>
#include <TypeConstants.h>
#include <OS.h>

void
reduce_load()
{
	system_info si;
	bigtime_t last;
	get_system_info(&si);
	last = si.cpu_infos[0].active_time;
	while(1) {
		snooze(100000);
		get_system_info(&si);
		if(si.cpu_infos[0].active_time - last < 20000)
			break;
		last = si.cpu_infos[0].active_time;
	}
}

static int
single_rm_rf(char * str, bool quiet)
{
	struct stat st;
	int res = 0;
	reduce_load();
	if (lstat(str, &st) && !quiet) {
		fprintf(stderr, "%s: %s\n", str, strerror(errno));
		return -1;
	}
	if (S_ISDIR(st.st_mode)) {
		DIR * d = opendir(str);
		struct dirent * dent;
		char * e = str+strlen(str);
		*e++ = '/';
		*e = 0;
		if (d) while ((dent = readdir(d)) != NULL) {
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, "..")) {
				continue;
			}
			strcpy(e, dent->d_name);
			res = single_rm_rf(str,quiet) ? -1 : res;
		}
		closedir(d);
		e[-1] = 0;
		res = rmdir(str) < 0 ? -1 : res;
	}
	else {
		res = unlink(str);
	}
	if ((res < 0) && (!quiet)) {
		fprintf(stderr, "%s: could not remove\n", str);
	}
	return res;
}

static int
do_rm_rf(int argc, char **argv, bool quiet)
{
	int res = 0;
	char cpath[512], path[512];

	getcwd(cpath, 512);
	strcat(cpath, "/");

	while (argc > 0) {
		reduce_load();
		if (argv[0][0] == '/') {
			strcpy(path, argv[0]);
		}
		else {
			strcpy(path, cpath);
			strcat(path, argv[0]);
		}
		if (single_rm_rf(path,quiet) < 0) {
			res = 1;
		}
		argc--;
		argv++;
	}
	return res;
}

status_t 
main(int argc, char **argv)
{
	status_t res = 0;
	struct stat s;
	bool quiet = false;
	
	argc--;
	argv++;

	if (strcmp(argv[0], "-q") == 0) {
		quiet = true;
		argc--;
		argv++;
	}
	
	if (!strcmp(argv[0], "-rf")) {
		return do_rm_rf(argc-1, argv+1, quiet);
	}
	
	while (argc){
		reduce_load();
		if(!lstat(argv[0],&s) && (s.st_mode & S_IFDIR)){
			if (!quiet) {
				fprintf(stderr,"rm: %s: is a directory\n",argv[0]);
			}
			res = 1;
		} else {
			if(unlink(argv[0])){
				if (!quiet) {
					fprintf(stderr,"rm: ");
					perror(argv[0]);
				}
				res = 1;
			}
		}
		argc--;
		argv++;
	}
	return res;
}
