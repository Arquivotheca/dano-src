//--------------------------------------------------------------------
//	
//	ofs.cpp
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
#include "ofs_map.h"
#include <ide_calls.h>


//====================================================================

bool ds_fs_id(partition_data *part, int32 dev, uint64 offset, int32 block_size)
{
	bool	result = FALSE;
	Track0	*sb;
	uint16	*buf16, *end16, tmp16;

	sb = (Track0 *)malloc(part->logical_block_size);
	lseek(dev, (offset * block_size) + (part->offset * part->logical_block_size), 0);
	if ((read(dev, sb, part->logical_block_size) >= 0) &&
					(sb->VersionNumber == CURRENTVERSION)) {
		strcpy(part->file_system_short_name, "ofs");
		strcpy(part->file_system_long_name, "Old Be File System");
		strcpy(part->volume_name, sb->VolName);
		result = TRUE;
	}
	else {
		lseek(dev, (offset * block_size) + (part->offset * part->logical_block_size), 0);
		if (read(dev, sb, part->logical_block_size) >= 0) {
			/* dr8 ide driver wrote things swapped - unswap them */
			buf16 = (uint16 *)sb;
			end16 = buf16 + part->logical_block_size/2;
			while (buf16 < end16) {
				tmp16 = *buf16;
				*buf16++ = (tmp16 >> 8) | (tmp16 << 8);
			}
			if (sb->VersionNumber == CURRENTVERSION) {
				strcpy(part->file_system_short_name, "ofs");
				strcpy(part->file_system_long_name, "Old Be File System");
				strcpy(part->volume_name, sb->VolName);
				result = TRUE;
			}
		}
	}
	free(sb);
	return result;
}

//--------------------------------------------------------------------

void get_flags(drive_setup_fs_flags *flags)
{
	flags->can_initialize = TRUE;
	flags->has_options = FALSE;
}
