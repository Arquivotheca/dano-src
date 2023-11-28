/*
 * BeOS user compatibility layer
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
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
 *  are Copyright (C) 1998 David A. Hinds.  All Rights Reserved.
 */

#ifndef _BE_U_COMPAT_H
#define _BE_U_COMPAT_H

#include <OS.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>

#if 0
#define select(a,b,c,d,e) ((*(b) -= *(b) & (*(b)-1)),0)
#endif

/* ioctl macros */
#define IOC_IN			1
#define IOC_OUT			2
#define IOCSIZE_MASK		0xfffc
#define IOCSIZE_SHIFT		2
#define _IOC(dir,tag,num,sz)	(((tag)<<24)|((num)<<16)|((sz)<<2)|(dir))
#define _IO(tag,num)		_IOC(0,tag,num,0)
#define _IOW(tag,num,type)	_IOC(1,tag,num,sizeof(type))
#define _IOR(tag,num,type)	_IOC(2,tag,num,sizeof(type))
#define _IOWR(tag,num,type)	_IOC(3,tag,num,sizeof(type))

/* Miscellaneous */
#define usleep(m) snooze(m)
#ifndef ENODATA
#define ENODATA ENOSPC
#endif

#endif /* _BE_U_COMPAT_H */
