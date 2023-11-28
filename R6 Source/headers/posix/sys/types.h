#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <time.h>
#include <null.h>

typedef long long 			blkcnt_t;
typedef int 				blksize_t;
typedef long long 			fsblkcnt_t;
typedef long long			fsfilcnt_t;
typedef long long           ino_t;
typedef int                 cnt_t;
typedef unsigned int        mode_t;
typedef int                 nlink_t;
typedef long                dev_t;
typedef long long           off_t;
typedef long		        pid_t;

typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef unsigned int umode_t;
typedef int          daddr_t;

/* bsd */
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

/* sysv */
typedef unsigned char unchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef char *caddr_t;

#include <size_t.h>

/* include select()'s FD_SET declarations */
#include <sys/select.h>

#endif
