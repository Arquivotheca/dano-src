#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "drive_setup.h"
#include "ext2_fs.h"
#include <byteorder.h>


//====================================================================

bool ds_fs_id(partition_data *part, int32 dev, uint64 offset, int32 block_size)
{
	bool	result = FALSE;
	ext2_super_block sb;

//	printf("ds_fd_id called block size %d.\n", block_size);

	// Clear the magic number in the superblock
	sb.s_magic = 0;

	// Seek and read in the superblock of the filesystem
	if(lseek(dev, (offset * block_size) + (part->offset * part->logical_block_size) + 1024, 0) < B_NO_ERROR) {
		return result;
	}
	if(read(dev, &sb, sizeof(sb)) >= B_NO_ERROR) {
//		printf("Read in the superblock.\n");
//		printf("Magic number 0x%x.\n", B_LENDIAN_TO_HOST_INT16(sb.s_magic));
		// Check the magic number in the superblock
		sb.s_magic = B_LENDIAN_TO_HOST_INT16(sb.s_magic);
		if(sb.s_magic == 0xEF53) {
			// Ok, it's good. Now lets check to see if the volume name is stored.
			if(sb.s_volume_name[0] != '\0') {
				memcpy(part->volume_name, sb.s_volume_name, EXT2_MAX_VOLUME_NAME_LEN);
				part->volume_name[EXT2_MAX_VOLUME_NAME_LEN] = '\0';
			} else {
				strcpy(part->volume_name, "ext2 untitled");	
			}			
			strcpy(part->file_system_short_name, "ext2");
			strcpy(part->file_system_long_name, "Linux ext2fs");
			result = TRUE;
		}
	}
	return result;

}

//--------------------------------------------------------------------

void ds_fs_flags(drive_setup_fs_flags *flags)
{
	flags->can_initialize = FALSE;
	flags->has_options = FALSE;
}
