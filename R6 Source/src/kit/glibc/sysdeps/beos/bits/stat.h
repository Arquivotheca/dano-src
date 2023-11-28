/* Copyright (C) 1992, 1996, 1997, 1998 Free Software Foundation, Inc.
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

/* This structure needs to be defined in accordance with the
   implementation of __stat, __fstat, and __lstat.  */

#include <bits/types.h>

/* Structure describing file characteristics.  */
struct stat
  {
    /* These are the members that POSIX.1 requires.  */
    __dev_t st_dev;		/* Device containing the file.  */
    __ino_t st_ino;		/* File serial number.  */
    __mode_t st_mode;		/* File mode.  */
    __nlink_t st_nlink;		/* Link count.  */
    __uid_t st_uid;		/* User ID of the file's owner.  */
    __gid_t st_gid;		/* Group ID of the file's group.  */
    __off_t st_size;		/* Size of file, in bytes.  */
    __dev_t st_rdev;		/* Device type (not used).  */
    unsigned long int st_blksize;/* Preferred block size for I/O.  */
    __time_t st_atime;		/* Time of last access.  */
    __time_t st_mtime;		/* Time of last modification.  */
    __time_t st_ctime;		/* Time of last status change.  */
    __time_t st_crtime;		/* Time of creation.  */

    /* This should be defined if there is a `st_blksize' member.  */
#define	_STATBUF_ST_BLKSIZE	1
  };

/* Encoding of the file mode.  These are the standard Unix values,
   but POSIX.1 does not specify what values should be used.  */

#define	__S_IFMT	0170000	/* These bits determine file type.  */

/* File types.  */
#define	__S_IFDIR	0040000	/* Directory.  */
#define	__S_IFCHR	0020000	/* Character device.  */
#define	__S_IFBLK	0060000	/* Block device.  */
#define	__S_IFREG	0100000	/* Regular file.  */
#define	__S_IFIFO	0010000	/* FIFO.  */
#define __S_IFLNK	0120000	/* Symbolic link.  */

/* Protection bits.  */

#define	__S_ISUID	04000	/* Set user ID on execution.  */
#define	__S_ISGID	02000	/* Set group ID on execution.  */
#define __S_ISVTX	01000	/* Save swapped text even after use.  */
#define	__S_IREAD	0400	/* Read by owner.  */
#define	__S_IWRITE	0200	/* Write by owner.  */
#define	__S_IEXEC	0100	/* Execute by owner.  */

#ifdef __USE_MISC
/* Be specific attributes.  */
# define S_ATTR_DIR		01000000000
# define S_ATTR			02000000000
# define S_INDEX_DIR		04000000000
# define S_STR_INDEX		00100000000
# define S_INT_INDEX		00200000000
# define S_UINT_INDEX		00400000000
# define S_LONG_LONG_INDEX	00010000000
# define S_ULONG_LONG_INDEX	00020000000
# define S_FLOAT_INDEX		00040000000
# define S_DOUBLE_INDEX		00001000000
# define S_ALLOW_DUPS		00002000000
#endif

#define     S_ISINDEX(m)  (((m) & S_INDEX_DIR) == S_INDEX_DIR)

#ifdef __USE_LARGEFILE64
struct stat64
  {
    /* These are the members that POSIX.1 requires.  */
    __dev_t st_dev;		/* Device containing the file.  */
    __ino_t st_ino;		/* File serial number.  */
    __mode_t st_mode;		/* File mode.  */
    __nlink_t st_nlink;		/* Link count.  */
    __uid_t st_uid;		/* User ID of the file's owner.  */
    __gid_t st_gid;		/* Group ID of the file's group.  */
    __off_t st_size;		/* Size of file, in bytes.  */
    __dev_t st_rdev;		/* Device type (not used).  */
    unsigned long int st_blksize;/* Preferred block size for I/O.  */
    __time_t st_atime;		/* Time of last access.  */
    __time_t st_mtime;		/* Time of last modification.  */
    __time_t st_ctime;		/* Time of last status change.  */
    __time_t st_crtime;		/* Time of creation.  */
  };
#endif

#define	S_IUMSK 07777		/* user settable bits */
