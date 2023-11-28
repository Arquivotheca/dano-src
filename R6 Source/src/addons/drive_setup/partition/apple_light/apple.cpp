//--------------------------------------------------------------------
//	
//	apple.cpp
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
#include <byteorder.h>

#ifndef APPLE_H
#include "apple.h"
#endif


//====================================================================

bool ds_partition_id(uchar *sb, int32 size)
{
	if (B_BENDIAN_TO_HOST_INT16(((Block0 *)sb)->sbSig) == sbSIGWord)
		return TRUE;
	return FALSE;
}

//--------------------------------------------------------------------

char *ds_partition_name(void)
{
	return "apple";
}

//--------------------------------------------------------------------

status_t ds_get_nth_map(int32 dev, uchar *sb, uint64 block_num, int32 block_size,
						int32 index, partition_data *partition)
{
	int32		size;
	status_t	result = B_ERROR;
	Partition	*block;

    if (B_BENDIAN_TO_HOST_INT16(((Block0 *)sb)->sbSig) == sbSIGWord) {

	block = (Partition *)malloc(block_size);
	lseek(dev, block_num * block_size, 0);
	if ((read(dev, block, block_size) >= 0) && (B_BENDIAN_TO_HOST_INT16(((Block0 *)block)->sbSig) == sbSIGWord)) {
		size = B_BENDIAN_TO_HOST_INT16(((Block0 *)block)->sbBlkSize);
		if (size > block_size) {
			free(block);
			block = (Partition *)malloc(size);
		}
	}
	else
		goto exit;
	lseek(dev, (block_num * block_size) + (1 * size), 0);
	if ((read(dev, block, size) >= 0) && (B_BENDIAN_TO_HOST_INT16(block->pmSig) == pMapSIG) &&
		(index >= 0) && (index <= (B_BENDIAN_TO_HOST_INT32(block->pmMapBlkCnt) - 1))) {
		lseek(dev, (block_num * block_size) + ((1 + index) * size), 0);
		if ((read(dev, block, size) >= 0) && (B_BENDIAN_TO_HOST_INT16(block->pmSig) == pMapSIG)) {
			partition->blocks = B_BENDIAN_TO_HOST_INT32(block->pmPartBlkCnt);
			partition->offset = B_BENDIAN_TO_HOST_INT32(block->pmPyPartStart);
			partition->logical_block_size = size;
			memcpy(partition->partition_name, &block->pmPartName, sizeof(block->pmPartName));
			memcpy(partition->partition_type, &block->pmParType, sizeof(block->pmParType));
			if ((!strcmp((char *)block->pmParType, "Apple_MFS")) ||
				(!strcmp((char *)block->pmParType, "Apple_HFS")) ||
				(!strcmp((char *)block->pmParType, "Apple_Unix_SVR2")) ||
				(!strcmp((char *)block->pmParType, "Apple_PRODOS")) ||
				(!strcmp((char *)block->pmParType, "Apple_Free")) ||
				(!strcmp((char *)block->pmParType, "Be_BFS")) ||
				(!strcmp((char *)block->pmParType, "BeOS")))
				partition->hidden = FALSE;
			else
				partition->hidden = TRUE;
			partition->partition_code = 0;
			result = B_NO_ERROR;
		}
	}
exit:
	free(block);
    }
	return result;
}

//--------------------------------------------------------------------

void ds_partition_flags(drive_setup_partition_flags *flags)
{
	flags->can_partition = TRUE;
	flags->can_repartition = FALSE;
}

//--------------------------------------------------------------------

status_t ds_update_map(int32 dev, int32 index, partition_data *partition)
{
	Partition	*block;
	status_t	result = B_NO_ERROR;

	block = (Partition *)malloc(partition->logical_block_size);
	lseek(dev, (index + 1) * partition->logical_block_size, 0);
	if ((read(dev, block, partition->logical_block_size) >= 0) &&
		(strcmp((char *)block->pmParType, "Be_BFS"))) {
		strncpy((char *)block->pmParType, "Be_BFS", sizeof(block->pmParType));
		lseek(dev, (index + 1) * partition->logical_block_size, 0);
		result = write(dev, block, partition->logical_block_size);
		if (result >= 0) {
			result = B_NO_ERROR;
			strcpy(partition->partition_type, "Be_BFS");
		}
	}
	free(block);
	return result;
}

//--------------------------------------------------------------------

void ds_partition(BMessage *msg)
{
}
