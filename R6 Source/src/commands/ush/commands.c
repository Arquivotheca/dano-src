
#include "ush.h"
#include "recovery.h"

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

#include <priv_syscalls.h>

extern status_t do_putserialnumber(int argc, char **argv);
extern status_t do_put_bios_vendor(int argc, char **argv);
extern status_t do_put_bios_version(int argc, char **argv);
extern status_t do_put_bios_date(int argc, char **argv);
extern status_t do_put_mac_address(int argc, char **argv);

extern status_t do_binderput(int argc, char **argv);
extern status_t do_binderget(int argc, char **argv);

static thread_id 
load_image_path(int32 argc, const char **argv, const char **envp, int foreground,
				char *errbuf, int errbufsiz)
{
	const char *save = NULL;
	thread_id thid;
	struct stat s;
	errbuf[0] = 0;
	
	if(strchr(argv[0],'/') == NULL) {
		char *path = getenv("PATH");
		if(path){
			char *buf;
			char *item;
			if((path = strdup(path)) == NULL) return 1;			
			if((buf = (char*) malloc(strlen(argv[0])+strlen(path)+2)) == NULL){
				free(path);
				return 1;
			}
			for(item = strtok(path,":"); item; item = strtok(NULL,":")){
				sprintf(buf,"%s/%s",item,argv[0]);
				if(!stat(buf,&s)){
					save = argv[0];
					argv[0] = buf;
					free(path);
					goto done;
				}
			}
			free(path);
			free(buf);
		}
	} 
done:
#if 0
	fprintf(stderr,"< ");
	for(i = 0; argv[i];i++){
		fprintf(stderr,"\"%s\" ",argv[i]);
	}
	fprintf(stderr,">\n");
#endif
	thid = _kload_image_etc_(argc,(char**)argv,(char**)envp,errbuf,errbufsiz);
	if((thid > 0) && foreground){
		setpgid( thid, thid);	
		ioctl(0, 'pgid', thid);
		ioctl(1, 'pgid', thid);
		ioctl(2, 'pgid', thid);	
	}
	if(save) {
		free((char*) argv[0]);
		argv[0] = save;
	}
	return thid;
}
	
status_t
do_echo(int argc, char **argv)
{
	int i;
	for(i=0;i<argc;i++){
		printf("%s%s",argv[i],(i==argc-1?"":" "));
	}
	printf("\n");
	return 0;
}

status_t
do_setenv(int argc, char **argv)
{
	char *x;
	if((x = (char*) malloc(strlen(argv[0])+strlen(argv[1])+2))) {
		sprintf(x,"%s=%s",argv[0],argv[1]);
		putenv(x);
		free(x);
	}
	return 0;
}

status_t
do_increment(int argc, char **argv)
{
	char *var= getenv(argv[0]);
	int32 var_value= 0;
	int32 inc_value= 1;

	char result[256];


	if(var) {
		sscanf(var, "%ld", &var_value);
	}
	if(argc>1) {
		sscanf(argv[1], "%ld", &inc_value);
	}

	sprintf(result, "%s=%ld", argv[0], var_value+inc_value);

	putenv(result);

	return 0;
}


status_t
do_filecat(int argc, char **argv)
{
	FILE *file;
	
	if(argc < 2)
		goto err;
	
	file = fopen(argv[0], "a");
		
	if( file == 0 )
		goto err;
			
	fprintf(file, "%s", argv[1]);
	fclose(file);
		
	return 0;

err:	
	return -1;
}


status_t 
do_spawn(int argc, char **argv)
{
	thread_id t;
	char err[1024];
	
	t = load_image_path(argc, (const char**) argv, (const char **) environ, 0, err, 1024);
	if(t > 0){
		resume_thread(t);
		return 0;
	} else {
		fprintf(stderr,"cannot execute \"%s\": %s %s\n",argv[0],strerror(t),err);
		return 1;
	}
}

status_t
do_run(int argc, char **argv)
{
	status_t status = 0;
	thread_id t;
	char err[1024];
	
	t = load_image_path(argc, (const char**) argv, (const char **) environ, 1, err, 1024);
	if(t > 0){
		resume_thread(t);
		wait_for_thread(t,&status);
		return status;
	} else {
		fprintf(stderr,"cannot execute \"%s\": %s %s\n",argv[0],strerror(t),err);
		return 127;
	}
}

status_t
do_eq(int argc, char **argv)
{
	char *var;
	
	var = getenv(argv[0]);
	if(!var) var = "";
	
	if(strcmp(var,argv[1])){
		return 1;
	} else {
		return 0;
	}	
}


status_t
do_empty(int argc, char **argv)
{
	char *var;
	
	var = getenv(argv[0]);
	if(!var || strlen(var) == 0)
		return 0;
	
	return 1;
}


status_t
do_neq(int argc, char **argv)
{
	char *var;
	
	var = getenv(argv[0]);
	if(!var) var = "";
	
	if(strcmp(var,argv[1])){
		return 0;
	} else {
		return 1;
	}	
}

status_t
do_isneg(int argc, char **argv)
{
	char *var;

	var = getenv(argv[0]);
	if (!var) var = "";

	if(strtol(var,NULL,10)<0){
		return 0;
	} else {
		return 1;
	}
}

status_t
do_not(int argc, char **argv)
{
	if(execute(argc, argv)) {
		return 0;
	} else {
		return 1;
	}
}

status_t
do_exit(int argc, char **argv)
{
	if(argc){
		exit(atoi(argv[0]));
	} else {
		exit(0);
	}
}

status_t
do_source(int argc, char **argv)
{
	int fd;
	FILE *f = NULL;
	
	fd = open(argv[0], O_RDONLY | O_CLOEXEC);
	if(fd >= 0)
		f = fdopen(fd,"r");
	if(f){
		status_t res = runfile(f,0);
		fclose(f);
		return res;
	} else {
		fprintf(stderr,"cannot open script \"%s\"\n",argv[0]);
		if(fd >= 0) close(fd);
		return 1;
	}
}

status_t
do_true(int argc, char **argv)
{
	return 0;
}

status_t
do_false(int argc, char **argv)
{
	return 1;
}

status_t
do_isdir(int argc, char **argv)
{
	struct stat s;
	if(!stat(argv[0],&s)){
		return s.st_mode & S_IFDIR ? 0 : 1;
	} else {
		return 1;
	}
}

status_t
do_isfile(int argc, char **argv)
{
	struct stat s;
	if(!stat(argv[0],&s)){
		return s.st_mode & S_IFREG ? 0 : 1;
	} else {
		return 1;
	}
}

status_t
do_exists(int argc, char **argv)
{
	struct stat s;
	if(!stat(argv[0],&s)){
		return 0;
	} else {
		return 1;
	}
}

status_t
do_waitfor(int argc, char **argv)
{
	for(;;){
		if(find_thread(argv[0]) > 0) return 0;
		snooze(100000);
	}
}

status_t
do_pwd(int argc, char **argv)
{
	char buf[MAXPATHLEN];
	getcwd(buf,MAXPATHLEN);
	printf("%s\n",buf);
	return 0;
}

status_t 
do_cd(int argc, char **argv)
{
	char *home;
	if(argc == 0) {
		home = getenv("HOME");
		if(home == NULL) home = "/";
	} else {
		home = argv[0];
	}
	
	if(chdir(home)){
		perror("chdir failed");
		return 1;
	} else {
		return 0;
	}
}

status_t
do_sleep(int argc, char **argv)
{
	sleep(atoi(argv[0]));
	return 0;
}

status_t 
do_chmod(int argc, char **argv)
{
	if(chmod(argv[1],strtoul(argv[0],NULL,8))){
		perror("chmod");
		return 1;
	} else {
		return 0;
	}
}

status_t 
do_env(int argc, char **argv)
{
	int i;
	for(i=0;environ[i];i++){
		printf("%s\n",environ[i]);
	}
	return 0;
}

status_t 
do_kill(int argc, char **argv)
{
	status_t err;
	status_t res = 0;
	thread_id thid;
	int num = atoi(argv[0]);
	
	argc--;
	argv++;
	
	while(argc){
		if((argv[0][0]=='-') || isdigit(argv[0][0])){
			thid = atoi(argv[0]);
		} else {
			if((thid = find_thread(argv[0])) < 1) goto oops;
		}
		err = send_signal(thid, num);
		if(err != B_OK) {
			if(err == B_BAD_THREAD_ID){
oops:
				fprintf(stderr,"kill: no such thread \"%s\"\n",argv[0]);
			} else if (err == B_BAD_VALUE){
				fprintf(stderr,"kill: bad signal %d\n",num);
			} else {
				fprintf(stderr,"kill: %s\n",strerror(err));
			}
			res = 1;
		}
		argc--;
		argv++;
	}
	
	return res;
}

static int
single_rm_rf(char * str, bool quiet)
{
	struct stat st;
	int res = 0;
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
do_rm(int argc, char **argv)
{
	status_t res = 0;
	struct stat s;
	bool quiet = false;
	
	if (strcmp(argv[0], "-q") == 0) {
		quiet = true;
		argc--;
		argv++;
	}
	
	if (!strcmp(argv[0], "-rf")) {
		return do_rm_rf(argc-1, argv+1, quiet);
	}
	
	while (argc){
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

status_t 
do_rmdir(int argc, char **argv)
{
	status_t res = 0;
	
	while(argc){
		if(rmdir(argv[0])){
			fprintf(stderr,"rmdir: ");
			perror(argv[0]);
			res = 1;
		}
		argc--;
		argv++;
	}
	
	return res;
}

static int
mkdir_path(const char * dir, int mode)
{
	char * str = (char *)alloca(strlen(dir)+1);
	char * c = str+1;
	char * d;
	struct stat st;

	strcpy(str, dir);
	while ((d = strchr(c, '/')) != NULL) {
		*d = 0;
		mkdir(str, mode);
		*d = '/';
		c = d+1;
	}
	mkdir(dir, mode);
	if (stat(dir, &st) < 0) {
		return -1;
	}
	if (!S_ISDIR(st.st_mode)) {
		errno = EEXIST;
		return -1;
	}
	return 0;
}

status_t 
do_mkdir(int argc, char **argv)
{
	status_t res = 0;
	int path = 0;

	if (!strcmp(argv[0], "-p")) {
		path = 1;
		argc--;
		argv++;
	}
	if (!argv[0]) {
		fprintf(stderr, "mkdir: requires an argument\n");
		return 1;
	}	
	while(argc){
		if (mkdir(argv[0],0755) && (!path || mkdir_path(argv[0], 0755))){
			fprintf(stderr,"mkdir: ");
			perror(argv[0]);
			res = 1;
		}
		argc--;
		argv++;
	}
	
	return res;
}

status_t 
do_sync(int argc, char **argv)
{
	sync();
	return 0;
}

status_t 
do_rescan(int argc, char **argv)
{
	int fd;
	if((fd = open("/dev",O_WRONLY))){
		printf("rescanning %s\n",argv[0]);
		write(fd,argv[0],strlen(argv[0]));
		close(fd);
		return 0;
	}
	return 1;
}

static char *bits[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };
	
void listname(const char * buf)
{
	struct stat s;
	int x;
	char link[256];
	const char * n = strrchr(buf, '/');
	if (n) n++; else n = buf;

	if(!lstat(buf,&s)){
		char datestr[20];
		struct tm tmt = *localtime(&s.st_mtime);
		strftime(datestr, 20, "%Y-%m-%d %H:%M:%S", &tmt);
		link[0] = 0;
		if(S_ISDIR(s.st_mode)) {
			x = 'd';
		} else if(S_ISLNK(s.st_mode)) {
			x = 'l';
			strcpy(link, " -> ");
			link[4+readlink(buf, &link[4], 251)] = 0;
		} else {
			x = '-';
		}
		printf("%c%s%s%s %s %8Ld %s%s\n",x,
			   bits[(s.st_mode & S_IRWXU) >> 6],
			   bits[(s.st_mode & S_IRWXG) >> 3],
			   bits[s.st_mode & S_IRWXO], 
			   datestr,
			   s.st_size, n, link);
	}
}

void listdir(const char *name, int wide, int all)
{
	char buf[1024];
	int pos = 0;
	struct stat s;
	struct dirent *de;
	DIR *d;
	
	d = opendir(name);
	if(d) {
		while((de = readdir(d))) {
//			if (!strcmp(de->d_name,".") || !strcmp(de->d_name,"..")) continue;
			if (!all && (de->d_name[0] == '.')) continue;
			if (wide) {
				sprintf(buf,"%s/%s",name,de->d_name);
				listname(buf);
			} else {
				if(pos){
					pos += strlen(de->d_name) + 1;
					if(pos > 78) {
						printf("\n");
						pos = strlen(de->d_name);
					}
				} else {
					pos = strlen(de->d_name);
				}
				printf("%s ",de->d_name);
			}
		}
		closedir(d);
	}
	else {
		if (wide) {
			listname(name);
		}
		else if (-1 < lstat(name, &s)) {
			printf("%s ", name);
			pos = strlen(name);
		}
	}
	if (!wide && pos) printf("\n");
}

status_t
do_attr(int argc, char **argv)
{
	status_t err = B_OK;
	while (argc) {
		int fd = open(argv[0], O_RDONLY);
		if (fd < 0) {
			err = errno;
			perror(argv[0]);
		}
		else {
			DIR * ad;
			struct dirent * ae;
			struct attr_info ai;
			ad = fs_fopen_attr_dir(fd);
			if (!ad) {
				err = errno;
				perror(argv[0]);
			}
			else {
				fprintf(stdout, "# %s\n", argv[0]);
				while ((ae = fs_read_attr_dir(ad)) != NULL) {
					if (fs_stat_attr(fd, ae->d_name, &ai) < 0) {
						err = errno;
						perror(ae->d_name);
					}
					else {
						char typestr[20];
						char * m;
						typestr[0] = ai.type>>24;
						typestr[1] = ai.type>>16;
						typestr[2] = ai.type>>8;
						typestr[3] = ai.type;
						typestr[4] = 0;
						if ((typestr[0] < 32) || (typestr[1] < 32) || (typestr[2] < 32) ||
								(typestr[3] < 32)) {
							sprintf(typestr, "0x%lx", ai.type);
						}
						fprintf(stdout, "%-30.30s %s %6Ld ", ae->d_name, typestr, ai.size);
						if (ai.size > 32768)
							ai.size = 32768;
						m = malloc(ai.size+1);
						if (!m) {
							fprintf(stdout, "<out of memory>\n");
						}
						else {
							if (ai.size > fs_read_attr(fd, ae->d_name, ai.type, 0, m, ai.size)) {
								err = errno;
								perror(ae->d_name);
							}
							else if ((ai.type == 'CSTR') || (ai.type == 'TEXT') || (ai.type == 'MIMS')) {
								m[ai.size] = 0;
								fprintf(stdout, "%s\n", m);
							}
							else {
								int ix;
								for (ix=0; ix<8 && ix<ai.size; ix++) {
									fprintf(stdout, "%02X ", (unsigned char)m[ix]);
								}
								if (ai.size > 8)
									fprintf(stdout, "...\n");
								else
									fprintf(stdout, "\n");
							}
							free(m);
						}
					}
				}
				fs_close_attr_dir(ad);
			}
			close(fd);
		}
		argv++;
		argc--;
	}
	return err;
}

status_t
do_rdattr(int argc, char **argv)
{
	status_t 	err = EXIT_FAILURE;
	int			fd, len;
	char		*fname = argv[0];
	char		*attr_name = argv[1];
	char		*env_var = argv[2];
	char		*attr_val;
	attr_info 	ai;
	
	if( (fd = open(fname, O_RDONLY)) < 0 )
		return EXIT_FAILURE;
	
	if( (fs_stat_attr(fd, attr_name, &ai) == B_OK)&&(ai.type == B_STRING_TYPE) )
	{
		len = strlen( env_var );
		if( (attr_val = (char *)malloc( len + ai.size + 2 )) != NULL )
		{
			sprintf( attr_val, "%s=", env_var );
			if( fs_read_attr(fd, attr_name, ai.type, 0, attr_val + len + 1, ai.size) >= 0 )
			{
				putenv( attr_val );
				err = EXIT_SUCCESS;
			}
			free( attr_val );
		}
	}
	
	close(fd);
	return err;
}

status_t
do_ls(int argc, char **argv)
{
	int wide = 0;
	int done = 0;
	int all = 0;

	while (argc){
		if (argv[0][0]=='-'){
			char *x = argv[0] + 1;
			while (*x){
				if (*x == 'l') wide = 1;
				else if (*x == 'a') all = 1;
				x++;
			}
		} else {
			listdir(argv[0],wide,all);
			done = 1;
		}
		argc--;
		argv++;
	}	
	
	if(!done) listdir(".",wide,all);
	
	return 0;
}

status_t 
do_mv(int argc, char **argv)
{
	status_t res = 0;
	struct stat s;
	char *dest = argv[1];
	
	if(argc > 2){
		fprintf(stderr,"mv: too many args\n");
		return 1;
	}
	
	if(!stat(argv[1],&s) && (s.st_mode & S_IFDIR)){
		char *tmp;
		if(!(dest = (char*) malloc(strlen(argv[0])+strlen(argv[1])+2))){
			fprintf(stderr,"mv: no memory\n");
			return 1;
		}
		tmp = strrchr(argv[0],'/');
		
		sprintf(dest,"%s/%s",argv[1],tmp ? tmp+1 : argv[0]);
		if(!stat(dest,&s) && (s.st_mode & S_IFDIR)){
			fprintf(stderr,"mv: %s is a directory\n",dest);
			res = 1;
			goto done;
		}
	}
	
	if(rename(argv[0],dest)){
		perror("rename");
		res = 1;
	}
	
done:
	if(dest != argv[1]) free(dest);
	return res;
}

status_t 
do_ln(int argc, char **argv)
{
	if ((argc != 3) || strcmp(argv[0], "-s")) {
		fprintf(stderr, "usage: ln -s source dest\n");
		return -1;
	}
	if (symlink(argv[1], argv[2])) {
		fprintf(stderr, "ln -s: ");
		perror(argv[2]);
		return -1;
	}
	return 0;
}

status_t 
do_unmount(int argc, char **argv)
{
	int		err;

	if (argc != 1) {
		fprintf(stderr, "usage: unmount path\n");
		return 1;
	}
	err = unmount(argv[0]);
	if (err) {
		fprintf(stderr, "unmount: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

status_t 
do_read(int argc, char **argv)
{
	char *x;
	char buf[1024];
	int l;
	
	fgets(buf, 1024, stdin);
	
	l = strlen(buf);
	
	if(l && (buf[l-1] == '\n')) buf[l-1] = 0;
	
	if((x = (char*) malloc(strlen(argv[0])+strlen(buf)+2))) {
		sprintf(x,"%s=%s",argv[0],buf);
		putenv(x);
		free(x);
		return 0;
	}
	
	return 1;
}

status_t do_reboot(int argc, char **argv)
{
	if(argc && argv[0] && (strcmp(argv[0], "-z")== 0)) {
		/*
		 * Force reboot into zrecover
		 *
		 * WARNING: this command option is dangerous... it's only
		 *          supposed to be enabled in BeIA, never on the
		 *          desktop
		 */
		char *argv_run[]= { "bootcomplete", "--do_recovery", NULL };
		do_run(sizeof(argv_run)/sizeof(argv_run[0]) - 1, argv_run);
	}

	_kshutdown_(1);
	return B_OK;
}

status_t
do_mountcfs(int argc, char **argv)
{
	char *dir;
	char *dev;
	int rc;
	struct stat st;
	
	if (argc < 2) {
		fprintf(stderr,"usage: mountcfs <dir> <dev>\n");
		return 1;
	}
	
	dir = argv[0];
	dev = argv[1];
	
	if (stat(dir,&st) < 0) {
		if (mkdir(dir, S_IRWXU|S_IRWXG|S_IRWXO) < 0) {
			fprintf(stderr, "%s : Doesn't exist, can't create\n", dir);
			return 1;
		}
	}
	
	rc = mount("cfs",dir,dev,0,NULL,0);
	if (rc < 0) {
		fprintf(stderr, "mount: %s\n", strerror(errno));
		return 1;
	}
	return 0;
}

status_t do_help(int argc, char **argv);

// Please update the README when adding commands
command commands[] = 
{
	{ do_read,     1, "read",    "<var> - read one line into an env var" },
	{ do_cd,       0, "cd",      "<path> - change current directory" },
	{ do_pwd,      0, "pwd",     "- print current directory" },
	{ do_echo,     0, "echo",    "... - echo arguments to stdout" },
	{ do_setenv,   2, "setenv",  "<var> <value> - $var=value" },
	{ do_increment,1, "increment","<var> <value> - increment var by value" },
	{ do_run,      1, "run",     "<path> - execute a process" },
	{ do_spawn,    1, "spawn",   "<path> - run a background process" },
	{ do_eq,       2, "eq",      "<var> <value> - true if $var==value" },
	{ do_neq,      2, "neq",     "<var> <value> - true if $var!=value" },
	{ do_empty,    1, "empty",   "<var> - true if strlen($var) == 0"},
	{ do_isneg,    1, "isneg",   "<var> - true if $var < 0" },
	{ do_not,      1, "not",     "<expr> - invert the return code" },
	{ do_exit,     0, "exit",    "[<value>] - exit this script" },
	{ do_source,   1, "source",  "<path> - run a script" },
	{ do_true,     0, "true",    "" },
	{ do_false,    0, "false",   ""	},
	{ do_exists,   1, "exists",  "<path> - true if path exists" },
	{ do_isdir,    1, "isdir",   "<path> - true if path is a directory" },
	{ do_isfile,   1, "isfile",  "<path> - true if path is a plain file" },
	{ do_waitfor,  1, "waitfor", "<name> - wait until a thread appears"	},
	{ do_sleep,    1, "sleep",   "<seconds> - pause for a while"	},
	{ do_chmod,    2, "chmod",   "<mode> <path> - change the mode bits"	},
	{ do_env,      0, "env",     "- print the environment" },
	{ do_kill,     2, "kill",    "<signal> <pid>|<name> - send a signal" },
	{ do_rm,       1, "rm",      "[-rf] <path> ... - remove object(s)" },
	{ do_rmdir,    1, "rmdir",   "<path> - remove a directory" },
	{ do_mkdir,    1, "mkdir",   "<path> - make a new directory" },
	{ do_sync,     0, "sync",    "" },
	{ do_rescan,   1, "rescan",  "<driver> - republish a driver" },
	{ do_mv,       2, "mv",      "<source> <dest> - move something" },
	{ do_ln,       2, "ln",      "-s <target> <link> - create a symbolic link" },
	{ do_ls,       0, "ls",      "[-l] [<path>] - list files" },
	{ do_help,     0, "help",    "- print this list" },
	{ do_attr,     0, "attr",    "<path> ... - print attributes of object(s)" },
	{ do_rdattr,   3, "rdattr",  "<path> <attr> <var> - load attribute into env var" },
	{ do_reboot,   0, "reboot",  "- quick reboot.  does NOT save data."},
	{ do_filecat,	2,	"filecat", "<path> <string> - concatenates string to end of file specified by path" },
	{ do_mountcfs,   2, "mountcfs",  "<dir> <device> - mount a cfs disk, creating directory if necessary"},
	{ do_unmount,   1, "unmount",  "<mount point> - unmount a mounted volume"},
//	{ do_addfirstpartition,   0, "addfirstpartition",  " - makes the drives 0_0 dev appear, if its not there."},
//	{ do_getdialupsettings,   1, "getdialupsettings",  "<file> - try to obtain user,password,and phone number from a DUN settings file."},
	{ do_putserialnumber, 	  1, "putserialnumber", "<var> - $var=SMBIOS system serial number" },
	{ do_put_bios_vendor, 	  1, "putbiosvendor", "<var> - $var=BIOS vendor" },
	{ do_put_bios_version, 	  1, "putbiosversion", "<var> - $var=BIOS version" },
	{ do_put_bios_date, 	  1, "putbiosdate", "<var> - $var=BIOS release date" },
	{ do_put_mac_address, 	  2, "putmacaddress", "<device> <var> - $var=device mac address" },
	{ do_binderput,			2, "binderput", "<path> <value> - store a property in the binder" },
	{ do_binderget,			2, "binderget", "<path> <var> - store a property from the binder into $var" },	
	{ NULL,        0, NULL,      NULL }
};

status_t 
do_help(int argc, char **argv)
{
	int i;
	for(i=0;commands[i].func;i++){
		printf("  %s %s\n",commands[i].name,commands[i].help);
	}
	return 0;
}




