
#include <unistd.h> 
#include <scsi.h> 
#include <stdio.h>
#include <string.h>
#include "cfs.h"


typedef struct { 
  char    partition_name[B_FILE_NAME_LENGTH]; 
  char    partition_type[B_FILE_NAME_LENGTH]; 
  char    file_system_short_name[B_FILE_NAME_LENGTH]; 
  char    file_system_long_name[B_FILE_NAME_LENGTH]; 
  char    volume_name[B_FILE_NAME_LENGTH]; 
  char    mounted_at[B_FILE_NAME_LENGTH]; 
  uint32  logical_block_size; 
  uint64  offset;    /* in logical blocks from start of session */ 
  uint64  blocks;    /* in logical blocks */ 
  bool    hidden;    /* non-file system partition */ 
  bool    reserved1; 
  uint32  reserved2; 
} partition_data;



extern "C" _EXPORT  bool ds_fs_id(partition_data*, int32, uint64, int32);;

bool ds_fs_id(partition_data *partition, int32 dev, uint64 session_offset, int32 block_size)
{
	bool result;
	status_t err;
	cfs_super_block_1 sb1;
	result = false;

	err = read_pos(dev, partition->offset*partition->logical_block_size+cfs_superblock_1_offset, &sb1, sizeof(sb1));
	if(err >= B_NO_ERROR)
	{
		if(sb1.cfs_id_1 == CFS_ID_1)
		{
			strcpy(partition->file_system_short_name,"cfs");
			strcpy(partition->file_system_long_name,"Compressed File System");

			cfs_super_block_2 sb2;
			if(read_pos(dev, partition->offset*partition->logical_block_size+sb1.super_block_2, &sb2, sizeof(sb2))>=B_NO_ERROR)
				strncpy(partition->volume_name, (char*)sb2.volume_name, sizeof(partition->volume_name));
			else
				strncpy(partition->volume_name, "cfs_volume", sizeof(partition->volume_name));
			
			result=true;
		}
	}

	return result;
}


