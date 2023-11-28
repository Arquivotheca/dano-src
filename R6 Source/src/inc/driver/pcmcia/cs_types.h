/*
 * cs_types.h 1.14 1999/07/20 16:07:55
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

#ifndef _LINUX_CS_TYPES_H
#define _LINUX_CS_TYPES_H

#ifdef __linux__
#include <linux/types.h>
#endif

typedef u_short	socket_t;
typedef u_short	ioaddr_t;
typedef u_int	event_t;
typedef u_char	cisdata_t;
typedef u_short	page_t;

struct client_t;
typedef struct client_t *client_handle_t;

struct window_t;
typedef struct window_t *window_handle_t;

struct region_t;
typedef struct region_t *memory_handle_t;

struct eraseq_t;
typedef struct eraseq_t *eraseq_handle_t;

#ifndef DEV_NAME_LEN
#define DEV_NAME_LEN 32
#endif

typedef char dev_info_t[DEV_NAME_LEN];

#endif /* _LINUX_CS_TYPES_H */
