#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <StorageKit.h>
#include <Application.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <errno.h>

#include <string.h>
#include <sys/stat.h>

#include <Debug.h>

#define FOR_REAL 1

/* This version has /boot/var/common/config  instead of /boot/config */
#define ADJUST_VERSION 0x401

/* This version is the most correct */
#define UPDATE_VERSION 0x403

void set_volume(const char *volume);
void set_directory(const char *dir);
static void 
FixUp(BDirectory *dir, int top, int system)
{
	BEntry entry;
	BNode node;
	struct stat info;
	int local_system;
	dir->Rewind();
	
	{
		BPath path(dir,".");
		set_directory(path.Path());
	}
	
	while(dir->GetNextEntry(&entry,false) == B_OK){
		if(entry.GetStat(&info) != B_OK) continue;

		local_system = system;
		
		if(top){
			/* if this is the root dir, check for a beos dir and ignore it */
			char name[B_FILE_NAME_LENGTH];
			if(entry.GetName(name) == B_OK){
				if(!strcmp(name,"beos")) local_system = 1;
				if(!strcmp(name,"var")) local_system = 1;
				if(!strcmp(name,"config")) local_system = 1;
			}
		}

		/* links have no perms or ownership of their own */	
		if(S_ISLNK(info.st_mode)) continue;

#if FOR_REAL		
		/* everything else should be corrected */	
		if(local_system){
			entry.SetOwner(0);
			entry.SetGroup(0);
		} else {
			entry.SetOwner(1000);
			entry.SetGroup(1000);
		}
		
		/* SV often creates executables with X but not R bits set */
		if(S_ISREG(info.st_mode)){
			if(info.st_mode & 0111) {
				entry.SetPermissions(info.st_mode | 0511);
			}
		}
#endif		

		/* make sure directories are accessable */
		if(S_ISDIR(info.st_mode)){
#if FOR_REAL
			entry.SetPermissions(info.st_mode | 0755); 
#endif
			BDirectory dir2(&entry);
			if(dir2.InitCheck() == B_OK) FixUp(&dir2,0,local_system);
		}
	}
}


static const char *B_PW_NAME = "sys:user:name";
//static const char *B_PW_PASSWD = "sys:user:passwd";
static const char *B_PW_UID = "sys:user:uid";
static const char *B_PW_GID = "sys:user:gid";
static const char *B_PW_DIR = "sys:user:dir";
static const char *B_PW_GECOS = "sys:user:longname";
static const char *B_PW_SHELL = "sys:user:shell";

static const char *B_GR_NAME = "sys:group:name";
static const char *B_GR_GID = "sys:group:gid";

static void 
mkuser(const char *root, const char *name, const char *longname,
	   int uid, int gid, const char *dir, const char *shell)
{
	int fd;
	char path[512];
	sprintf(path,"%s/config/users/%d",root,uid);
	if((fd = open(path,O_RDWR|O_CREAT|O_EXCL,0644)) < 0) return;
	
	fs_write_attr(fd, B_PW_NAME, B_STRING_TYPE, 0, name, strlen(name)+1);
	fs_write_attr(fd, B_PW_GECOS, B_STRING_TYPE, 0, longname, strlen(longname)+1);
	fs_write_attr(fd, B_PW_DIR, B_STRING_TYPE, 0, dir, strlen(dir)+1);
	fs_write_attr(fd, B_PW_SHELL, B_STRING_TYPE, 0, shell, strlen(shell)+1);
	fs_write_attr(fd, B_PW_UID, B_INT32_TYPE, 0, &uid, 4);
	fs_write_attr(fd, B_PW_GID, B_INT32_TYPE, 0, &gid, 4);
	close(fd);	
}

static void 
mkgroup(const char *root, const char *name, int gid)
{
	int fd;
	char path[512];
	sprintf(path,"%s/config/groups/%d",root,gid);
	if((fd = open(path,O_RDWR|O_CREAT,0644)) < 0) return;
	
	fs_write_attr(fd, B_GR_NAME, B_STRING_TYPE, 0, name, strlen(name)+1);
	fs_write_attr(fd, B_GR_GID, B_INT32_TYPE, 0, &gid, 4);
	close(fd);	
}

struct {
	char *path;
	int perms;
    int uid;
	int gid;
} paths[] = {
	{ "%s", 01777, 0, 0 },
	{ "%s/beos", 0755, 0, 0 },
	{ "%s/var", 0755, 0, 0 },
	{ "%s/var/tmp", 01777, 0, 0 },
	{ "%s/config", 0755, 0, 0 },
	{ "%s/config/settings", 01777, 0, 0 },
	{ "%s/config/add-ons", 01777, 0, 0 },
	{ "%s/config/users", 0755, 0, 0 },
	{ "%s/config/groups", 0755, 0, 0 },
	{ NULL, 0, 0, 0 },
};

static int 
ConvertVolume(int isboot, const char *root, BDirectory *dir, dev_t device, int fixperms)
{
	/* only work your magic on the boot volume for now */
	if(isboot || !strcmp(root,"/boot")) {
#if FOR_REAL
		int i;
		char path[512];	
	
		/* basic perms fixup */
		for(i=0;paths[i].path;i++){		
			sprintf(path, paths[i].path, root);
			create_directory(path, paths[i].perms);
			chmod(path, paths[i].perms);
			chown(path, paths[i].uid, paths[i].gid);
		}

		fs_create_index(device, B_PW_UID, B_INT32_TYPE, 0);
		fs_create_index(device, B_PW_NAME, B_STRING_TYPE, 0);
		fs_create_index(device, B_GR_GID, B_INT32_TYPE, 0);
		fs_create_index(device, B_GR_NAME, B_STRING_TYPE, 0);
		
		mkuser(root,"system","BeOS System",0,0,"/boot/home","/bin/sh");
		mkuser(root,"user","BeOS User",1000,1000,"/boot/home","/bin/sh");
		mkgroup(root,"system",0);
		mkgroup(root,"users",1000);
#endif
	}
	
	if(fixperms) {
		FixUp(dir, 1, 0);	
	}
	
	return 1;
}

static void 
UpdateVolume(int force, int isboot, BVolume *vol)
{
	BDirectory dir;
	BEntry entry;
	BPath path;
	int32 version;
	status_t err;

	if(vol->GetRootDirectory(&dir)) return;
	if(dir.GetEntry(&entry)) return;
	if(entry.GetPath(&path)) return;
	
	set_volume(path.Path());	
	set_directory(path.Path());
	
	bool isbfs = vol->KnowsMime() && vol->KnowsAttr() && vol->KnowsQuery();
	
	if(!isbfs || vol->IsReadOnly()) return;

	if(!force){	
		if(dir.ReadAttr("sys:volume_version",B_INT32_TYPE, 0, &version, 4) == 4){
			if(version >= UPDATE_VERSION) {
				return;
			}
		}
	}

	if(ConvertVolume(isboot,path.Path(),&dir,vol->Device(), (version < ADJUST_VERSION) || force)){
		version = UPDATE_VERSION;
#if FOR_REAL
		err = dir.WriteAttr("sys:volume_version",B_INT32_TYPE,0,&version,4);
#endif
	}
}

void
FixVolumes(int force, int isboot, const char *data)
{
	char *foo =  (char *) malloc(strlen(data) + 10);
	sprintf(foo,"%s/.",data);
	struct stat s;
	if(!stat(foo,&s)){
		BVolume vol(s.st_dev);
		if(vol.InitCheck() == B_OK) UpdateVolume(force,isboot,&vol);
	}
}

