#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H

#include <be_setup.h>
#include <sys/types.h>

#define ST_RDONLY 0x00000001
#define ST_NOSUID 0x00000002

struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	fsblkcnt_t    f_blocks;
	fsblkcnt_t    f_bfree;
	fsblkcnt_t    f_bavail;
	fsfilcnt_t    f_files;
	fsfilcnt_t    f_ffree;
	fsfilcnt_t    f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
};


__extern_c_start

int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fildes, struct statvfs *buf);

__extern_c_end


#endif

