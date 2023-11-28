/*
 * mem_op.h 1.9 1999/07/20 16:07:59
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dhinds@hyper.stanford.edu>.  Portions created by David A. Hinds
 * are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
 */

#ifndef _LINUX_MEM_OP_H
#define _LINUX_MEM_OP_H

/*
   If UNSAFE_MEMCPY is defined, we use the (optimized) system routines
   to copy between a card and kernel memory.  These routines do 32-bit
   operations which may not work with all PCMCIA controllers.  The
   safe versions defined here will do only 8-bit and 16-bit accesses.
*/

#ifdef UNSAFE_MEMCPY

#define copy_from_pc memcpy_fromio
#define copy_to_pc memcpy_toio

static inline void copy_pc_to_user(void *to, const void *from, size_t n)
{
    size_t odd = (n & 3);
    n -= odd;
    while (n) {
	put_user(readl_ns(from), (int *)to);
	(char *)from += 4; (char *)to += 4; n -= 4;
    }
    while (odd--)
	put_user(readb((char *)from++), (char *)to++);
}

static inline void copy_user_to_pc(void *to, const void *from, size_t n)
{
    int l;
    char c;
    size_t odd = (n & 3);
    n -= odd;
    while (n) {
	get_user(l, (int *)from);
	writel_ns(l, to);
	(char *)to += 4; (char *)from += 4; n -= 4;
    }
    while (odd--) {
	get_user(c, (char *)from++);
	writeb(c, (char *)to++);
    }
}

#else /* UNSAFE_MEMCPY */

static inline void copy_from_pc(void *to, const void *from, size_t n)
{
    size_t odd = (n & 1);
    n -= odd;
    while (n) {
	*(u_short *)to = readw_ns(from);
	(char *)to += 2; (char *)from += 2; n -= 2;
    }
    if (odd)
	*(u_char *)to = readb(from);
}

static inline void copy_to_pc(void *to, const void *from, size_t n)
{
    size_t odd = (n & 1);
    n -= odd;
    while (n) {
	writew_ns(*(u_short *)from, to);
	(char *)to += 2; (char *)from += 2; n -= 2;
    }
    if (odd)
	writeb(*(u_char *)from, to);
}

static inline void copy_pc_to_user(void *to, const void *from, size_t n)
{
    size_t odd = (n & 1);
    n -= odd;
    while (n) {
	put_user(readw_ns(from), (short *)to);
	(char *)to += 2; (char *)from += 2; n -= 2;
    }
    if (odd)
	put_user(readb(from), (char *)to);
}

static inline void copy_user_to_pc(void *to, const void *from, size_t n)
{
    short s;
    char c;
    size_t odd = (n & 1);
    n -= odd;
    while (n) {
	get_user(s, (short *)from);
	writew_ns(s, to);
	(char *)to += 2; (char *)from += 2; n -= 2;
    }
    if (odd) {
	get_user(c, (char *)from);
	writeb(c, to);
    }
}

#endif /* UNSAFE_MEMCPY */

#endif /* _LINUX_MEM_OP_H */
