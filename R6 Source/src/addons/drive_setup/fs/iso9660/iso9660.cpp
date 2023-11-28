//--------------------------------------------------------------------
//	
//	iso9660.cpp
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
#include "iso_fs.h"


//====================================================================

bool ds_fs_id(partition_data *part, int32 dev, uint64 offset, int32 block_size)
{
	bool							result = FALSE;
	struct iso_primary_descriptor	block;
	int								i;

	if (block_size != ISOFS_BLOCK_SIZE)
		return result;

	lseek(dev, (offset + ISO_VOL_START) * block_size, 0);
	if (read(dev, &block, sizeof(block)) >= 0) {
		if (!(strncmp(block.id, ISO_STANDARD_ID, sizeof(block.id)))) {
			strcpy(part->file_system_short_name, "iso9660");
			strcpy(part->file_system_long_name, "iso9660");
			memset(part->volume_name, ' ', sizeof(part->volume_name));
			memcpy(part->volume_name, block.volume_id, sizeof(block.volume_id));
			result = TRUE;
		}
		else if (!(strncmp(block.id, HS_STANDARD_ID, sizeof(block.id)))) {
			strcpy(part->file_system_short_name, "highsierra");
			strcpy(part->file_system_long_name, "highsierra");
			memset(part->volume_name, ' ', sizeof(part->volume_name));
			memcpy(part->volume_name, block.volume_id, sizeof(block.volume_id));
			result = TRUE;
		}

		for (i=strlen(part->volume_name)-1;i>=0;i--)
			if (part->volume_name[i] != ' ')
				break;
		if (i < 0)
			strcpy(part->volume_name, "UNKNOWN");
		else
			part->volume_name[i+1] = 0;
	}
	return result;
}

//--------------------------------------------------------------------

void ds_fs_flags(drive_setup_fs_flags *flags)
{
	flags->can_initialize = FALSE;
	flags->has_options = FALSE;
}
