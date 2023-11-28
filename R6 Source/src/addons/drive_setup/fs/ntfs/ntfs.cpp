//	
//	ntfs-dsplugin.cpp
//
//	Written by: Travis Geiselbrecht 12/18/98
//
//  To see debug messages, uncomment the printfs and run DriveSetup from the command line.
//	

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <drive_setup.h>

#include <ByteOrder.h>
#include <UTF8.h>

#include "ntfs.h"

// Function declarations
static int ntfs_read_and_check_bios_block(ntfs_fs_info *nbb, int32 dev);
static int unicode_to_utf8(char *to, const char *from, int32 unicode_length);

//====================================================================

bool ds_fs_id(partition_data *part, int32 dev, uint64 offset, int32 block_size)
{
	ntfs_fs_info nbb;
	uint32 cluster_size;
	int32 mft_recordsize;
	uint64 start_offset = (offset * block_size) + (part->offset * part->logical_block_size);
	uchar *file_rec_buf;
	uchar *file_rec_buf_end;

	//printf("ds_fd_id called block size %d.\n", block_size);

	// Erase the volume name
	part->volume_name[0] = '\0';
	
	// Read in the NTFS bios block and check the magic number
	lseek(dev, start_offset, 0);
	if(ntfs_read_and_check_bios_block(&nbb, dev) != B_NO_ERROR) {
		goto error1;
	}

	// calculate the cluster size
	cluster_size = nbb.bytes_per_sector*nbb.sectors_per_cluster;
	//printf("\tcluster size = %d.\n", cluster_size);

	// calculate the mft record size
	if(nbb.clusters_per_file_record>0)
		mft_recordsize= cluster_size*nbb.clusters_per_file_record;
	else
		mft_recordsize= 1 << (- nbb.clusters_per_file_record);

	// check to make sure the mft record size is sane
	// 16k seems reasonable. 
	if(mft_recordsize >= 16384) {
		printf("NTFS: MFT record size > 16k, doesn't look good.");
		goto error1;
	}

	//printf("\tnew MFT record size = %d.\n", mft_recordsize);

	// Ok, lets go get MFT record 3. This record is for file $Volume, which contains the volume name
	{
		if(!(file_rec_buf = (uchar *)malloc(mft_recordsize))) goto error1;
		file_rec_buf_end = file_rec_buf + mft_recordsize;
		
	
		// Seek to the supposed position of the MFT record #3.
		lseek(dev, start_offset + nbb.MFT_cluster*cluster_size + mft_recordsize*3, 0);
		if(read(dev, file_rec_buf, mft_recordsize)!= mft_recordsize) {
			// This should catch invalid sizes.
			printf("NTFS: MFT record #3 not read.\n");
			goto error2;
		}
	
		// Overlay the ntfs FILE record struct to the just read buffer and check
		ntfs_FILE_record *file_rec = (ntfs_FILE_record *)file_rec_buf;
		// Check the magic.
		if(B_LENDIAN_TO_HOST_INT32(file_rec->magic) != 0x454c4946) { // 'FILE'
			printf("NTFS: record #3 is invalid.\n");
			goto error2;
		}
	
		// Find the VOLUME_NAME attribute
		if(B_LENDIAN_TO_HOST_INT16(file_rec->offset_to_attributes) > mft_recordsize) {
			// The pointer to the first attribute in the FILE rec is invalid.
			printf("NTFS: pointer to first attribute is invalid.\n");
			goto error2;
		}
		uchar *attr_pos = file_rec_buf + B_LENDIAN_TO_HOST_INT16(file_rec->offset_to_attributes);
		ntfs_attr_header *attr_header = (ntfs_attr_header *)attr_pos;
		// Loop while there are still attributes to be read
		while(((attr_pos - file_rec_buf) < mft_recordsize) && (B_LENDIAN_TO_HOST_INT32(attr_header->type)!=0xffffffff)) {
			//printf("found attribute 0x%x.\n", attr_header->type);
			if(B_LENDIAN_TO_HOST_INT32(attr_header->type) == ATT_VOLUME_NAME) {
				// We have the volume name attribute
				//printf("found volume name attribute.\n");
				
				// Make sure the attribute is resident. Hope it isn't
				if(attr_header->non_resident) {
					printf("NTFS: non resident volume name attribute, can't handle at this point\n");
					goto no_name;
				}
				
				// find and copy the name
				ntfs_attr_resident *res_attr = (ntfs_attr_resident *)(attr_pos + 0x10);
				// Check to make sure we're still in the buffer
				if((uchar *)res_attr + sizeof(ntfs_attr_resident)> file_rec_buf_end) {
					printf("NTFS: resident attribute pointer past end of buffer.\n");
					goto error2;
				}
				//printf("unicode name length = %d.\n", res_attr->specific_value_length);
				// Check the name length
				if(B_LENDIAN_TO_HOST_INT32(res_attr->specific_value_length)/2+1 > B_FILE_NAME_LENGTH) {
					printf("NTFS: volume name too long to return.\n");
					goto no_name;
				}
				// Check to make sure the copy length would be acceptable
				if(attr_pos + B_LENDIAN_TO_HOST_INT16(res_attr->specific_value_offset) + B_LENDIAN_TO_HOST_INT32(res_attr->specific_value_length) > file_rec_buf_end) {
					// This would copy from past the buffer's end.
					printf("NTFS: volume name would be copied past buffer end.\n"); 
					goto error2;
				}
				// copy the unicode name to the volume structure
				unicode_to_utf8(part->volume_name, (char *)(attr_pos + B_LENDIAN_TO_HOST_INT16(res_attr->specific_value_offset)), B_LENDIAN_TO_HOST_INT32(res_attr->specific_value_length));
				//printf("volume name is '%s'\n", part->volume_name);
				break;
			}
			attr_pos += B_LENDIAN_TO_HOST_INT16(attr_header->length);
			// check to make sure the next pointer would still be inside the buffer
			if(attr_pos + sizeof(ntfs_attr_header) > file_rec_buf_end) break;
			attr_header = (ntfs_attr_header *)attr_pos;
		}
			
		free(file_rec_buf);
	}   
	
no_name:	
	if(strlen(part->volume_name) == 0) 
		strcpy(part->volume_name, "Untitled NTFS Volume");
	strcpy(part->file_system_short_name, "ntfs");
	strcpy(part->file_system_long_name, "ntfs");
	return true;

error2:
	free(file_rec_buf);
error1:
	return false;

}

//--------------------------------------------------------------------

void ds_fs_flags(drive_setup_fs_flags *flags)
{
	flags->can_initialize = FALSE;
	flags->has_options = FALSE;
}

// reads in 512 bytes of the partition starting at the current seek spot. 
// Checks the magic number and returns the bios block structure.
static int ntfs_read_and_check_bios_block(ntfs_fs_info *nbb, int32 dev)
{
	char temp[512];
	int32 *magic;
	
	if(read(dev, temp, 512)!=512) {
		return B_ERROR;
	}

//	printf("checking MAGIC....");
	// Check the magic number. 'NTFS' at offset 0x3
	if (memcmp(temp + 3, "NTFS", 4)) {
//		printf("bad.\n");
		return B_ERROR;
	}			

	nbb->bytes_per_sector = B_LENDIAN_TO_HOST_INT16(*(uint16 *)(temp + 0x0b));
	nbb->sectors_per_cluster = *(uchar *)(temp + 0x0d);
	nbb->reserved_sectors = B_LENDIAN_TO_HOST_INT16(*(uint16 *)(temp + 0x0e));
	nbb->media_type = *(uchar *)(temp + 0x15);
	nbb->sectors_per_track = B_LENDIAN_TO_HOST_INT16(*(uint16 *)(temp + 0x18));
	nbb->num_heads = B_LENDIAN_TO_HOST_INT16(*(uint16 *)(temp + 0x1a));
	nbb->hidden_sectors = B_LENDIAN_TO_HOST_INT32(*(uint32 *)(temp + 0x1c));
	nbb->total_sectors = B_LENDIAN_TO_HOST_INT64(*(uint64 *)(temp + 0x28));
	nbb->MFT_cluster = B_LENDIAN_TO_HOST_INT64(*(uint64 *)(temp + 0x30));
	nbb->MFT_mirror_cluster = B_LENDIAN_TO_HOST_INT64(*(uint64 *)(temp + 0x38));
	nbb->clusters_per_file_record = *(uchar *)(temp + 0x40);
	nbb->clusters_per_index_block = B_LENDIAN_TO_HOST_INT32(*(uint32 *)(temp + 0x44));
	nbb->volume_serial = B_LENDIAN_TO_HOST_INT64(*(uint64 *)(temp + 0x48));
		
/*	printf("NTFS: MFT Cluster = %Ld.\n", nbb->MFT_cluster);
	printf("\tsectors per cluster = %d.\n", nbb->sectors_per_cluster);
	printf("\ttotal sectors = %Ld.\n", nbb->total_sectors);
	printf("\tclusters per file record = %d.\n", nbb->clusters_per_file_record);
*/	
		
	return B_NO_ERROR;
}

static int unicode_to_utf8(char *to, const char *from, int32 unicode_length)
{
	status_t result;
	int32 i, state = 0;
	int32 utf8len = B_FILE_NAME_LENGTH;
	char *swapped;

	/* We differ with NT over unicode byte ordering. */

	swapped = (char *)malloc(unicode_length);
	if (!swapped) return ENOMEM;

	for (i=0;i<unicode_length;i+=2) {
		swapped[i] = from[i+1];
		swapped[i+1] = from[i];
	}

	result = convert_to_utf8(B_UNICODE_CONVERSION, swapped, &unicode_length,
			to, &utf8len, &state);

	free(swapped);

	to[utf8len] = 0;

	return (result >= 0) ? B_OK : result;
}
