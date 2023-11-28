//--------------------------------------------------------------------
//	
//	drive_setup.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef DRIVE_SETUP_H
#define DRIVE_SETUP_H

#include <BeBuild.h>
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <partition.h>

#define DS_SESSION_ADDONS	"drive_setup/session/"
#define DS_PART_ADDONS		"drive_setup/partition/"
#define DS_FS_ADDONS		"drive_setup/fs/"


/* Session add-on entry points */
/*-----------------------------*/

_EXPORT status_t	ds_get_nth_session	(int32 /* dev */, int32 /* index */,
								 int32 /*block_size */, session_data*);

#define DS_GET_NTH_SESSION	"ds_get_nth_session"


/* Partition add-on entry points */
/*-------------------------------*/

typedef struct {
	bool	can_partition;
	bool	can_repartition;
} drive_setup_partition_flags;

_EXPORT bool		ds_partition_id		(uchar* /* sb */, int32 /* block_size */);
_EXPORT char*		ds_partition_name	(void);
_EXPORT status_t	ds_get_nth_map		(int32 /* dev */, uchar* /* sb */,
								 uint64 /* block_num */, int32 /* block_size */,
								 int32 /* index */, partition_data*);
_EXPORT void		ds_partition_flags	(drive_setup_partition_flags*);
_EXPORT void		ds_partition		(BMessage*);
_EXPORT status_t	ds_update_map		(int32 /* dev */, int32 /* index */,
								 partition_data*);

#define DS_PARTITION_ID		"ds_partition_id"
#define DS_PARTITION_NAME	"ds_partition_name"
#define DS_PARTITION_MAP	"ds_get_nth_map"
#define DS_PARTITION_FLAGS	"ds_partition_flags"
#define DS_PARTITION		"ds_partition"
#define DS_UPDATE_MAP		"ds_update_map"


/* File system add-on entry points */
/*---------------------------------*/

typedef struct {
	bool	can_initialize;
	bool	has_options;
} drive_setup_fs_flags;

_EXPORT bool	ds_fs_id			(partition_data*, int32 /* dev */,
							 uint64 /* offset */, int32 /* block_size */);
_EXPORT void	ds_fs_flags			(drive_setup_fs_flags*);
_EXPORT void	ds_fs_initialize	(BMessage*);

#define DS_FS_ID			"ds_fs_id"
#define DS_FS_FLAGS			"ds_fs_flags"
#define DS_FS_INITIALIZE	"ds_fs_initialize"

#ifdef __cplusplus
}
#endif

#endif
