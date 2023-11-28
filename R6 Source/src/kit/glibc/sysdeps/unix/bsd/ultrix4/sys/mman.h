/* Definitions for BSD-style memory management.  Ultrix 4 version.
   Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
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

#ifndef	_SYS_MMAN_H

#define	_SYS_MMAN_H	1
#include <features.h>

#include <bits/types.h>
#define __need_size_t
#include <stddef.h>


/* Protections are chosen from these bits, OR'd together.  The
   implementation does not necessarily support PROT_EXEC or PROT_WRITE
   without PROT_READ.  The only guarantees are that no writing will be
   allowed without PROT_WRITE and no access will be allowed for PROT_NONE. */

#define	PROT_NONE	0x00	/* No access.  */
#define	PROT_READ	0x01	/* Pages can be read.  */
#define	PROT_WRITE	0x02	/* Pages can be written.  */
#define	PROT_EXEC	0x04	/* Pages can be executed.  */


/* Sharing types (must choose one and only one of these).  */
#define	MAP_SHARED	0x01	/* Share changes.  */
#define	MAP_PRIVATE	0x02	/* Changes private; copy pages on write.  */
#ifdef __USE_BSD
# define MAP_TYPE	0x0f	/* Mask for sharing type.  */
#endif

/* Other flags.  */
#define	MAP_FIXED	0x10	/* Map address must be exactly as requested. */

/* Advice to `madvise'.  */
#ifdef __USE_BSD
# define MADV_NORMAL	0	/* No further special treatment.  */
# define MADV_RANDOM	1	/* Expect random page references.  */
# define MADV_SEQUENTIAL	2	/* Expect sequential page references.  */
# define MADV_WILLNEED	3	/* Will need these pages.  */
# define MADV_DONTNEED	4	/* Don't need these pages.  */
#endif

/* Return value of `mmap' in case of an error.  */
#define MAP_FAILED	((__ptr_t) -1)


__BEGIN_DECLS
/* Map addresses starting near ADDR and extending for LEN bytes.  from
   OFFSET into the file FD describes according to PROT and FLAGS.  If ADDR
   is nonzero, it is the desired mapping address.  If the MAP_FIXED bit is
   set in FLAGS, the mapping will be at ADDR exactly (which must be
   page-aligned); otherwise the system chooses a convenient nearby address.
   The return value is the actual mapping address chosen or MAP_FAILED
   for errors (in which case `errno' is set).  A successful `mmap' call
   deallocates any previous mapping for the affected region.  */

extern __ptr_t __mmap __P ((__ptr_t __addr, size_t __len, int __prot,
			  int __flags, int __fd, off_t __offset));
extern __ptr_t mmap __P ((__ptr_t __addr, size_t __len, int __prot,
			int __flags, int __fd, off_t __offset));

/* Deallocate any mapping for the region starting at ADDR and extending LEN
   bytes.  Returns 0 if successful, -1 for errors (and sets errno).  */
extern int __munmap __P ((__ptr_t __addr, size_t __len));
extern int munmap __P ((__ptr_t __addr, size_t __len));

/* Change the memory protection of the region starting at ADDR and
   extending LEN bytes to PROT.  Returns 0 if successful, -1 for errors
   (and sets errno).  */
extern int __mprotect __P ((__ptr_t __addr, size_t __len, int __prot));
extern int mprotect __P ((__ptr_t __addr, size_t __len, int __prot));

/* Ultrix 4 does not implement `msync' or `madvise'.  */

/* Synchronize the region starting at ADDR and extending LEN bytes with the
   file it maps.  Filesystem operations on a file being mapped are
   unpredictable before this is done.  */
extern int msync __P ((__ptr_t __addr, size_t __len));

#ifdef __USE_BSD
/* Advise the system about particular usage patterns the program follows
   for the region starting at ADDR and extending LEN bytes.  */
extern int madvise __P ((__ptr_t __addr, size_t __len, int __advice));
#endif

__END_DECLS


#endif	/* sys/mman.h */
