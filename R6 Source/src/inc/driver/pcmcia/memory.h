/*
 * memory.h 1.4 1999/07/20 16:08:00
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

#ifndef _LINUX_MEMORY_H
#define _LINUX_MEMORY_H

typedef struct erase_info_t {
    u_long	Offset;
    u_long	Size;
} erase_info_t;

#define MEMGETINFO		_IOR('M', 1, region_info_t)
#define MEMERASE		_IOW('M', 2, erase_info_t)

#endif /* _LINUX_MEMORY_H */
