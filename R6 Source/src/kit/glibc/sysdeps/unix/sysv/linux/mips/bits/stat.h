/* Copyright (C) 1992, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _SYS_STAT_H
# error "Never include <bits/stat.h> directly; use <sys/stat.h> instead."
#endif

/* Versions of the `struct stat' data structure.  */
#define _STAT_VER_LINUX_OLD	1
#define _STAT_VER_SVR4		2
#define _STAT_VER_LINUX		3
#define _STAT_VER		_STAT_VER_LINUX	/* The one defined below.  */

/* Versions of the `xmknod' interface.  */
#define _MKNOD_VER_LINUX	1
#define _MKNOD_VER_SVR4		2
#define _MKNOD_VER		_MKNOD_VER_LINUX /* The bits defined below.  */

/* Structure describing file characteristics.  */
struct stat
  {
    unsigned long int st_dev;
    long int st_pad1[3];
#ifndef __USE_FILE_OFFSET64
    __ino_t st_ino;		/* File serial number.		*/
#else
    __ino64_t st_ino;		/* File serial number.		*/
#endif
    __mode_t st_mode;		/* File mode.  */
    __nlink_t st_nlink;		/* Link count.  */
    __uid_t st_uid;		/* User ID of the file's owner.	*/
    __gid_t st_gid;		/* Group ID of the file's group.*/
    unsigned long int st_rdev;	/* Device number, if device.  */
    long int st_pad2[2];
#ifndef __USE_FILE_OFFSET64
    __off_t st_size;		/* Size of file, in bytes.  */
#else
    __off64_t st_size;		/* Size of file, in bytes.  */
#endif
    /* SVR4 added this extra long to allow for expansion of off_t.  */
    long int st_pad3;
    /*
     * Actually this should be timestruc_t st_atime, st_mtime and
     * st_ctime but we don't have it under Linux.
     */
    __time_t st_atime;		/* Time of last access.  */
    long int __reserved0;
    __time_t st_mtime;		/* Time of last modification.  */
    long int __reserved1;
    __time_t st_ctime;		/* Time of last status change.  */
    long int __reserved2;
    long int st_blksize;	/* Optimal block size for I/O.  */
#ifndef __USE_FILE_OFFSET64
    __blkcnt_t st_blocks;	/* Number of 512-byte blocks allocated.  */
#else
    __blkcnt64_t st_blocks;	/* Number of 512-byte blocks allocated.  */
#endif
    char st_fstype[16];		/* Filesystem type name */
    long int st_pad4[8];
    /* Linux specific fields */
    unsigned int st_flags;
    unsigned int st_gen;
  };

#ifdef __USE_LARGEFILE64
struct stat64
  {
    unsigned long int st_dev;
    long int st_pad1[3];
    __ino64_t st_ino;		/* File serial number.		*/
    __mode_t st_mode;		/* File mode.  */
    __nlink_t st_nlink;		/* Link count.  */
    __uid_t st_uid;		/* User ID of the file's owner.	*/
    __gid_t st_gid;		/* Group ID of the file's group.*/
    unsigned long int st_rdev;	/* Device number, if device.  */
    long int st_pad2[2];
    __off64_t st_size;		/* Size of file, in bytes.  */
    /* SVR4 added this extra long to allow for expansion of off_t.  */
    long int st_pad3;
    /*
     * Actually this should be timestruc_t st_atime, st_mtime and
     * st_ctime but we don't have it under Linux.
     */
    __time_t st_atime;		/* Time of last access.  */
    long int __reserved0;
    __time_t st_mtime;		/* Time of last modification.  */
    long int __reserved1;
    __time_t st_ctime;		/* Time of last status change.  */
    long int __reserved2;
    long int st_blksize;	/* Optimal block size for I/O.  */
    __blkcnt64_t st_blocks;	/* Number of 512-byte blocks allocated.  */
    char st_fstype[16];		/* Filesystem type name */
    long int st_pad4[8];
    /* Linux specific fields */
    unsigned int st_flags;
    unsigned int st_gen;
  };
#endif

/* Tell code we have these members.  */
#define	_STATBUF_ST_BLKSIZE
#define	_STATBUF_ST_RDEV

/* Encoding of the file mode.  */

#define	__S_IFMT	0170000	/* These bits determine file type.  */

/* File types.  */
#define	__S_IFDIR	0040000	/* Directory.  */
#define	__S_IFCHR	0020000	/* Character device.  */
#define	__S_IFBLK	0060000	/* Block device.  */
#define	__S_IFREG	0100000	/* Regular file.  */
#define	__S_IFIFO	0010000	/* FIFO.  */

/* These don't actually exist on System V, but having them doesn't hurt.  */
#define	__S_IFLNK	0120000	/* Symbolic link.  */
#define	__S_IFSOCK	0140000	/* Socket.  */

/* Protection bits.  */

#define	__S_ISUID	04000	/* Set user ID on execution.  */
#define	__S_ISGID	02000	/* Set group ID on execution.  */
#define	__S_ISVTX	01000	/* Save swapped text after use (sticky).  */
#define	__S_IREAD	0400	/* Read by owner.  */
#define	__S_IWRITE	0200	/* Write by owner.  */
#define	__S_IEXEC	0100	/* Execute by owner.  */
