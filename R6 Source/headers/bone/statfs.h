/*
	statfs.h
	
	statistics filesystem public definitions
	
	Copyright 1999, Be Incorporated, All Rights Reserved.
*/

#ifndef H_STATFS
#define H_STATFS

#ifdef __cplusplus
extern "C" {
#endif

#include "stat_module.h"

typedef struct stats_ioctl
{
	stat_class_t 	*sc;
	void			*data;
} stats_ioctl_t;

#define STATFS_INIT_IOCTL 0x23
#define STATFS_DELETE_IOCTL 0x24
#define STATFS_NOTIFY_IOCTL 0x25
#ifdef __cplusplus
}
#endif

#endif
