/*--------------------------------------------------------------------*\
  File:      intel.cpp
  Creator:   Robert Polic, Matt Bogosian <mattb@be.com>
  Copyright: (c)1997, 1998, Be, Inc. All rights reserved.
  Description: Source file for the intel DriveSetup add-on. Original
      version by Robert Polic (I just work here).
\*--------------------------------------------------------------------*/

#define DEBUG 1
#include <Debug.h>
#include <Drivers.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "intel.h"

bool ds_partition_id(uchar *sb, int32 size)
{
	return id_intel_partition(sb, size);
}

//--------------------------------------------------------------------

char *ds_partition_name(void)
{
	return "intel";
}

//--------------------------------------------------------------------

static void _set_partition_type_(partition_data *partition, uchar type)
{
	int			i;
	for (i = 0; ; i++) {
		if (types[i].id < 0) {
			sprintf(partition->partition_type, "INTEL 0x%.2x", type);
			partition->hidden = false;
			break;
		} else if (type == types[i].id) {
			sprintf(partition->partition_type, types[i].name);
			partition->hidden = types[i].hidden;
			break;
		}
	}
}

status_t ds_get_nth_map(int32 dev, uchar *sb, uint64 block_num, int32 block_size,
						int32 index, partition_data *partition)
{
	struct _partition_info_ pinfo;

	if (get_nth_intel_partition(dev, sb, block_num, index,
			&pinfo, NULL, NULL, NULL) != B_OK)
		return B_ERROR;

	partition->offset = pinfo.sblock;
	partition->blocks = pinfo.nblocks;
	partition->partition_code = pinfo.type;
	partition->logical_block_size = block_size;

	_set_partition_type_(partition, pinfo.type);
	
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

void ds_partition_flags(drive_setup_partition_flags *flags)
{
	flags->can_partition = true;
	flags->can_repartition = true;
}

//--------------------------------------------------------------------

status_t ds_update_map(int32 dev, int32 index, partition_data *partition)
{
	struct _partition_info_ ptinfo;
	fdisk *pfdisk;
	uint64 nblock_num, num_blocks;
	uchar block[512], oblock[512];
	device_geometry geometry;

	if (ioctl(dev, B_GET_GEOMETRY, &geometry) < 0)
		return B_ERROR;

	lseek(dev, 0, 0);
	if (read(dev, block, 512) < 512)
		return B_ERROR;

	if (get_nth_intel_partition(dev, block, 0, index, 
			NULL, &ptinfo, oblock, &pfdisk) != B_OK)
		return B_ERROR;
	
	if (pfdisk->systid != partition->partition_code) {
		pfdisk->systid = partition->partition_code;
		lseek(dev, geometry.bytes_per_sector*ptinfo.sblock, 0);
		if (write(dev, oblock, 512) < 512)
			return B_ERROR;
		_set_partition_type_(partition, partition->partition_code);
	}

	return 0;
}

//--------------------------------------------------------------------

void ds_partition(BMessage *msg)
{
}

static uint32 swap32(uchar data[4])
{
	return data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0];
}

static uint32 swap16(uchar data[2])
{
	return data[1] << 8 | data[0];
}

/* support code for libpartition.a */
extern "C" {
	int dprintf(const char *format, ...);
}

int dprintf(const char *format, ...)
{
	int r;
	va_list args;
	va_start(args, format);
	r = vprintf(format, args);
	va_end(args);
	return r;
}

