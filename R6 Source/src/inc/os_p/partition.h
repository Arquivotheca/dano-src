//--------------------------------------------------------------------
//	
//	partition.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef PARTITION_H
#define PARTITION_H

#ifndef B_FILE_NAME_LENGTH
	#include <StorageDefs.h>
#endif

typedef struct {
	uint64	offset;				/* in device blocks */
	uint64	blocks;
	bool	data;				/* audio or data session */
} session_data;

typedef struct {
	char	partition_name[B_FILE_NAME_LENGTH];
	char	partition_type[B_FILE_NAME_LENGTH];
	char	file_system_short_name[B_FILE_NAME_LENGTH];
	char	file_system_long_name[B_FILE_NAME_LENGTH];
	char	volume_name[B_FILE_NAME_LENGTH];
	char	mounted_at[B_FILE_NAME_LENGTH];
	uint32	logical_block_size;
	uint64	offset;				/* in logical blocks from start of session */
	uint64	blocks;				/* in logical blocks */
	bool	hidden;				/* non-file system partition */
	uchar	partition_code;
	bool	reserved1;
	uint32	reserved2;
} partition_data;

#endif
