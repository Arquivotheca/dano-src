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

#if __GNUC__	/* FIXME: Should this be !__MWC__ or something? */
#include <sys/ioctl.h>
#include <dirent.h>
#endif

#include <OS.h>
#include <FindDirectory.h>
#include <fs_query.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <fs_info.h>
#include <fsproto.h>
#include <TypeConstants.h>
#include <StorageDefs.h>

#include "priv_syscalls.h"


#define MAX_ENTRY 32
#define MAX_GECOS 128

#define OLD_CHEEZE 0

static const char *B_PW_NAME = "sys:user:name";
static const char *B_PW_PASSWD = "sys:user:passwd";
static const char *B_PW_UID = "sys:user:uid";
static const char *B_PW_GID = "sys:user:gid";
static const char *B_PW_DIR = "sys:user:dir";
static const char *B_PW_GECOS = "sys:user:longname";
static const char *B_PW_SHELL = "sys:user:shell";

static const char *B_GR_NAME = "sys:group:name";
static const char *B_GR_GID = "sys:group:gid";


/* globals and initializer function for this module */
static int need_init = 1;
static dev_t boot_device = 0;
static int multiuser = 0;

static void do_init(void);

static char *members[] = { NULL };

static struct group gid1000 = { "users", "", 1000, &members[0] };
static struct group gid0 = { "system", "", 0, &members[0] };

static struct group gbuffer;
static char __gname[MAX_ENTRY+1];
static char __gpass[MAX_ENTRY+1];


static void 
file_to_group(int fd, struct group *gr)
{
	ssize_t len;
	
	len = fs_read_attr(fd,B_GR_NAME,B_STRING_TYPE,0,__gname,MAX_ENTRY);
	if(len < 0){
		gr->gr_name = "unknown";
	} else {
		gr->gr_name = __gname;
		__gname[len] = 0;
	}
	
	gr->gr_passwd = "";
	
	gr->gr_mem = &members[0];
	
	if(fs_read_attr(fd,B_GR_GID,B_INT32_TYPE,0,&gr->gr_gid,4) != 4){
		gr->gr_gid = 1000;
	}
}

struct group *
getgrgid(gid_t gid)
{
	if(need_init) do_init();
	
	if(multiuser){
		char path[B_PATH_NAME_LENGTH];
		int fd;
		DIR *d;
		dirent_t *dir;
		sprintf(path,"sys:group:gid=%d",gid);
		
		if(d = fs_open_query(boot_device,path,0)){
			if(dir = fs_read_query(d)){
				if(get_path_for_dirent(dir,path,B_PATH_NAME_LENGTH) == B_OK){
					if((fd = open(path,O_RDONLY)) >= 0){
						file_to_group(fd,&gbuffer);
						close(fd);
						fs_close_query(d);
						return &gbuffer;
					}
				}
			}
			fs_close_query(d);
		}
	}
	
	if(gid == 0) {
		memcpy(&gbuffer, &gid0, sizeof(struct group));
		return &gbuffer;
	}
	if(gid == 1000) {
		memcpy(&gbuffer, &gid1000, sizeof(struct group));
		return &gbuffer;
	}
	return NULL;
}

struct group *
getgrnam(const char *name)
{	
	if(need_init) do_init();
	
	if(multiuser){
		char path[B_PATH_NAME_LENGTH];
		int fd;
		DIR *d;
		dirent_t *dir;
		sprintf(path,"sys:group:name=%s",name); /* XXX:bjs length & specials check */
		
		if(d = fs_open_query(boot_device,path,0)){
			if(dir = fs_read_query(d)){
				if(get_path_for_dirent(dir,path,B_PATH_NAME_LENGTH) == B_OK){
					if((fd = open(path,O_RDONLY)) >= 0){
						file_to_group(fd,&gbuffer);
						close(fd);
						fs_close_query(d);
						return &gbuffer;
					}
				}
			}
			fs_close_query(d);
		}
	}
	
	if(!strcmp(name,gid0.gr_name)){
		memcpy(&gbuffer, &gid0, sizeof(struct group));
		return &gbuffer;
	}
	if(!strcmp(name,gid1000.gr_name)){
		memcpy(&gbuffer, &gid1000, sizeof(struct group));
		return &gbuffer;
	}
	return NULL;
}

static DIR *grent_query = NULL;
static int grent_active = 0;

struct group *
getgrent(void)
{
	dirent_t *dir;
	int fd;
	
	if(need_init) do_init();

	if(!grent_active){
		grent_active = 1;
		if(multiuser) {
			grent_query = fs_open_query(boot_device,"sys:group:gid>-1",0);
		} else {
			memcpy(&gbuffer, &gid0, sizeof(struct group));
			return &gbuffer;
		}
	}
	
	if(!grent_query) return NULL;
	
	if(dir = fs_read_query(grent_query)){
		char path[B_PATH_NAME_LENGTH];
		if(get_path_for_dirent(dir,path,B_PATH_NAME_LENGTH) == B_OK){
			if((fd = open(path,O_RDONLY)) >= 0){
				file_to_group(fd,&gbuffer);
				close(fd);
				return &gbuffer;
			}
		}
	} else {
		fs_close_query(grent_query);
		grent_query = NULL;
	}
	return NULL;
}

void
setgrent(void)
{
	if(grent_query) fs_close_query(grent_query);
	grent_query = NULL;
	grent_active = 0;
}

void
endgrent(void)
{
	if(grent_query) fs_close_query(grent_query);
	grent_query = NULL;
	grent_active = 0;
}


int
getgroups(int x, gid_t grps[])
{
	return 0;
}

static struct passwd uid0 = { 
	"system", "", 0, 0, "/boot/home", "/bin/sh", "system" 
};
static struct passwd uid1000 = { 
	"baron", "", 1000, 1000, "/boot/home", "/bin/sh", "baron" 
};

static struct passwd pbuffer;
static char __name[MAX_ENTRY+1];
static char __pass[MAX_ENTRY+1];
static char __dir[B_PATH_NAME_LENGTH+1];
static char __shell[B_PATH_NAME_LENGTH+1];
static char __gecos[MAX_GECOS+1];

static void 
file_to_passwd(int fd, struct passwd *pw)
{
	ssize_t len;
	
	len = fs_read_attr(fd,B_PW_NAME,B_STRING_TYPE,0,__name,MAX_ENTRY);
	if(len < 0){
		pw->pw_name = "unknown";
	} else {
		pw->pw_name = __name;
		__name[len] = 0;
	}
	
	len = fs_read_attr(fd,B_PW_PASSWD,B_STRING_TYPE,0,__pass,MAX_ENTRY);
	if(len < 0){
		pw->pw_passwd = "";
	} else {
		pw->pw_passwd = __pass;
		__pass[len] = 0;
	}
	
	len = fs_read_attr(fd,B_PW_GECOS,B_STRING_TYPE,0,__gecos,MAX_GECOS);
	if(len < 0){
		pw->pw_gecos = "Unknown User";
	} else {
		pw->pw_gecos = __gecos;
		__gecos[len] = 0;
	}
		
	len = fs_read_attr(fd,B_PW_DIR,B_STRING_TYPE,0,__dir,B_PATH_NAME_LENGTH);
	if(len < 0){
		pw->pw_dir = "/boot";
	} else {
		pw->pw_dir = __dir;
		__dir[len] = 0;
	}
		
	len = fs_read_attr(fd,B_PW_SHELL,B_STRING_TYPE,0,__shell,B_PATH_NAME_LENGTH);
	if(len < 0){
		pw->pw_shell = "/bin/sh";
	} else {
		pw->pw_shell = __shell;
		__shell[len] = 0;
	}
		
	if(fs_read_attr(fd,B_PW_GID,B_INT32_TYPE,0,&pw->pw_gid,4) != 4){
		pw->pw_gid = 1000;
	}
	if(fs_read_attr(fd,B_PW_UID,B_INT32_TYPE,0,&pw->pw_uid,4) != 4){
		pw->pw_uid = 1000;
	}
}

struct passwd *
getpwnam(const char *name)
{
	if(need_init) do_init();
	
	if(multiuser){
		char path[B_PATH_NAME_LENGTH];
		int fd;
		DIR *d;
		dirent_t *dir;
		sprintf(path,"sys:user:name=%s",name); /* XXX:bjs length & specials check */
	
		if(d = fs_open_query(boot_device,path,0)){
			if(dir = fs_read_query(d)){
				if(get_path_for_dirent(dir,path,B_PATH_NAME_LENGTH) == B_OK){
					if((fd = open(path,O_RDONLY)) >= 0){
						file_to_passwd(fd,&pbuffer);
						close(fd);
						fs_close_query(d);
						return &pbuffer;
					}
				}
			}
			fs_close_query(d);
		}
	}
	
	
	if(!strcmp(name,uid1000.pw_name)){
		memcpy(&pbuffer, &uid1000, sizeof(struct passwd));
		return &pbuffer;
	}
	return NULL;	
}

struct passwd *
getpwuid(uid_t uid)
{
	if(need_init) do_init();
	
	if(multiuser){
		char path[B_PATH_NAME_LENGTH];
		int fd;
		DIR *d;
		dirent_t *dir;
		sprintf(path,"sys:user:uid=%d",uid);
	
		if(d = fs_open_query(boot_device,path,0)){
			if(dir = fs_read_query(d)){
				if(get_path_for_dirent(dir,path,B_PATH_NAME_LENGTH) == B_OK){
					if((fd = open(path,O_RDONLY)) >= 0){
						file_to_passwd(fd,&pbuffer);
						close(fd);
						fs_close_query(d);
						return &pbuffer;
					}
				}
			}
			fs_close_query(d);
		}
	}
	
	if(uid == 0) {
		memcpy(&pbuffer, &uid0, sizeof(struct passwd));
		return &pbuffer;
	}
	if(uid == 1000) {
		memcpy(&pbuffer, &uid1000, sizeof(struct passwd));
		return &pbuffer;
	}
	return NULL;
}

static DIR *pwent_query = NULL;
static int pwent_active = 0;

struct passwd *
getpwent(void)
{
	dirent_t *dir;
	int fd;
	
	if(need_init) do_init();

	if(!pwent_active){
		pwent_active = 1;
		if(multiuser){
			pwent_query = fs_open_query(boot_device,"sys:user:uid>-1",0);
		} else {
			memcpy(&pbuffer, &uid0, sizeof(struct passwd));			
			return &pbuffer;
		}
	}
	
	if(!pwent_query) return NULL;
	
	if(dir = fs_read_query(pwent_query)){
		char path[B_PATH_NAME_LENGTH];
		if(get_path_for_dirent(dir,path,B_PATH_NAME_LENGTH) == B_OK){
			if((fd = open(path,O_RDONLY)) >= 0){
				file_to_passwd(fd,&pbuffer);
				close(fd);
				return &pbuffer;
			}
		}
	} else {
		fs_close_query(pwent_query);
		pwent_query = NULL;
	}
	return NULL;
}

void
setpwent(void)
{
	if(pwent_query) fs_close_query(pwent_query);
	pwent_query = NULL;
	pwent_active = 0;
}

void
endpwent(void)
{
	if(pwent_query) fs_close_query(pwent_query);
	pwent_query = NULL;
	pwent_active = 0;
}

char *
getlogin(void)
{
	struct passwd *pw;
	
	if(pw = getpwuid(getuid())) return pw->pw_name;

	return NULL;
}

static void do_init_exit(void)
{
	free(gid0.gr_name);
	free(uid0.pw_name);
}

static void do_init(void)
{
	char *x;
	
	if(x = getenv("MULTIUSER")){
		if(!strcmp(x,"true")) multiuser = 1;
	}
	
	if(multiuser) {
		struct stat s;
		stat("/boot",&s);
		boot_device = s.st_dev;
	} else {
		if(x = getenv("GROUP")){
			gid0.gr_name = strdup(x);
		} else {
			gid0.gr_name = strdup("users");
		}
		if(x = getenv("USER")){
			uid0.pw_name = strdup(x);
		} else {
			uid0.pw_name = strdup("baron");
		}
		gid1000.gr_name = "g1000";
		uid1000.pw_name = "u1000";
		atexit(do_init_exit);
	}
	need_init = 0;
}

