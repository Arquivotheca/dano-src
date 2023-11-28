//--------------------------------------------------------------------
//	
//	hfs.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drive_setup.h>

//====================================================================

bool ds_fs_id(partition_data *part, int32 dev, uint64 offset, int32 block_size)
{
	bool	result = FALSE;
	uchar	*block;
	int32	logical_block_size;

	logical_block_size = part->logical_block_size;
	if ((logical_block_size == 0) || (logical_block_size > 512))
		logical_block_size = 512;
	block = (uchar *)malloc(logical_block_size);
	lseek(dev, (offset * block_size) +
				part->offset * part->logical_block_size +
				2 * logical_block_size, 0);
	if (read(dev, block, logical_block_size) == logical_block_size) {
		if ((block[0] == 'B') && (block[1] == 'D')) {
			strcpy(part->file_system_short_name, "hfs");
			strcpy(part->file_system_long_name, "Mac HFS");
			memcpy(part->volume_name, &block[0x25], block[0x24]);
			part->volume_name[block[0x24]] = 0;
			result = TRUE;
		} else if ((block[0] == 'R') && (block[1] == 'W')) {
			strcpy(part->file_system_short_name, "mfs");
			strcpy(part->file_system_long_name, "Mac MFS");
			memcpy(part->volume_name, &block[0x25], block[0x24]);
			part->volume_name[block[0x24]] = 0;
			result = TRUE;
		}
	}
	free(block);

	return result;
}

//--------------------------------------------------------------------

void ds_fs_flags(drive_setup_fs_flags *flags)
{
	flags->can_initialize = TRUE;
	flags->has_options = FALSE;
}
