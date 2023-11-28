#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <support/SupportDefs.h>
#include <drivers/Drivers.h>

#include <partition.h>
#include <intel_map.h>

extern int dprintf(const char *, ...);

static uint32 swap32(uchar data[4])
{
	return data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0];
}

bool id_intel_partition(uchar *sb, int32 size)
{
	int i;
	bool any = FALSE;

	if (size < 512) return FALSE;

	if ((sb[0x1fe] != 0x55) || (sb[0x1ff] != 0xaa))
		return FALSE;

	/* crude verification of partition map */
	for (i=0x1be;i<0x1fe;i+=0x10) {
		/* check for a non-zero partition id */
		if (sb[i+4] != 0) any = TRUE;

		/* if it's not marked active, there's nothing more we can do */
		if (sb[i] == 0) continue;

		/* if it's active, it must point to a hard disk */
		if ((sb[i] & 0x80) == 0) return FALSE;

		/* sanity check the bios drive number */
		if (sb[i] > 0x87) return FALSE;
	}

	return any;
}

static status_t _fill_partition_info_(int32 dev, uint64 block_num,
	fdisk *pfdisk, struct _partition_info_ *opinfo,
	uint64 extended_partition_lba_base, uint32 sector_size)
{
	opinfo->type = 0;
	opinfo->sblock = opinfo->nblocks = 0;

	if (pfdisk->systid == 0)
		return B_OK;

	opinfo->type = pfdisk->systid;
	opinfo->sblock = swap32(pfdisk->relsect) + block_num;
	if (((opinfo->type == EXTENDED_PARTITION1) ||
	    (opinfo->type == EXTENDED_PARTITION2) ||
	    (opinfo->type == EXTENDED_PARTITION3)) && 
	    (extended_partition_lba_base != 0)) {
	    // dos fdisk is buggy and calculates extended partitions
	    // based on the offset of the first extended partition
		opinfo->sblock = swap32(pfdisk->relsect) + 
				extended_partition_lba_base;
#if 0
		// guard against circular chains
		if (opinfo->sblock <= block_num)
			opinfo->sblock = swap32(pfdisk->relsect) + block_num;
#endif
	}
	opinfo->nblocks = swap32(pfdisk->numsect);

	if ((opinfo->type == EXTENDED_PARTITION1) ||
	    (opinfo->type == EXTENDED_PARTITION2) ||
	    (opinfo->type == EXTENDED_PARTITION3)) {
		uchar extsb[512];

		if ((read_pos(dev, opinfo->sblock * sector_size, extsb, 512) == 512) &&
			(extsb[0x1fe] == 0x55) && (extsb[0x1ff] == 0xaa)) {
			return B_OK;
		}

		dprintf("invalid extended partition detected at block %Lx\n", block_num);
		
		return B_ERROR;
	}
	return B_OK;
}

// returns information about the given partition table entry in opinfo,
// a copy of the partition table in osb, and a pointer to the entry in ofdisk
static status_t _get_nth_intel_partition_helper_(int32 dev, uchar *sb, 
	struct _partition_info_ *ipinfo, int32 index,
	struct _partition_info_ *opinfo, 
	struct _partition_info_ *optinfo, uchar *osb, fdisk **ofdisk,
	uint64 extended_partition_lba_base,
	uint32 sector_size)
{
	int32		i, j, pnum;
	bool		found = FALSE;
	struct		_partition_info_ partitions[4];

	if (index < 0) return B_ERROR;

	// check for signature
	if ((sb[0x1fe] != 0x55) || (sb[0x1ff] != 0xaa)) {
		if (ipinfo->sblock) dprintf("bad signature at block %Lx (%2.2x%2.2x)\n", ipinfo->sblock, sb[0x1fe], sb[0x1ff]);
		return B_ERROR;
	}

	pnum = -1;
	for (i=0;i<4;i++) {
		// load up partition information
		if (_fill_partition_info_(dev, ipinfo->sblock,  
				((bootsector *)sb)->part + i, partitions+i,
				extended_partition_lba_base, sector_size) < 0) {
			// bad extended partition; don't let it zap the whole
			// partition table if we are at the MBR
			if (ipinfo->sblock != 0)
				return B_ERROR;

			// make it a dummy extended partition
			partitions[i].sblock = partitions[i].nblocks = 0;
		}

		// check boundary
		if ((partitions[i].nblocks != 0) && (partitions[i].type != 0)) {
			if ((partitions[i].type != EXTENDED_PARTITION1) &&
				(partitions[i].type != EXTENDED_PARTITION2) &&
				(partitions[i].type != EXTENDED_PARTITION3)) {
				// check disk/partition boundary (not valid for entries
				// describing extended partition tables)

#ifndef BOOT
				// this special case is for CD-ROM drives,
				// which report incorrect geometries
				if (ipinfo->nblocks) {
					if ((partitions[i].sblock < ipinfo->sblock) ||
							(partitions[i].sblock + partitions[i].nblocks - 1 > ipinfo->sblock + ipinfo->nblocks - 1)) {
						dprintf("partition %d in block %Lx extends beyond %s\n", i, ipinfo->sblock, (ipinfo->sblock == 0) ? "disk" : "partition");
						dprintf("sblock = %Lx, nblocks = %Lx, p->sblock = %Lx, p->nblocks = %Lx\n", partitions[i].sblock, partitions[i].nblocks, ipinfo->sblock, ipinfo->nblocks);
						return B_ERROR;
					}
				}
#endif
			}
			
			// check for overlap
			for (j=0;j<i;j++) {
				if (partitions[j].nblocks == 0)
					continue;
	
				if (partitions[i].sblock + partitions[i].nblocks - 1 < partitions[j].sblock)
					continue;
				
				if (partitions[i].sblock > partitions[j].sblock + partitions[j].nblocks - 1)
					continue;
	
				dprintf("partitions %d and %d overlap in block %Lx\n", i, j, ipinfo->sblock);				
				dprintf("i: sblock = %Lx, nblocks = %Lx, j: sblock = %Lx, nblocks = %Lx\n", partitions[i].sblock, partitions[i].nblocks, partitions[j].sblock, partitions[j].nblocks);
	
				return B_ERROR;
			}
		}

		if (((partitions[i].type == EXTENDED_PARTITION1) || 
			 (partitions[i].type == EXTENDED_PARTITION2) ||
			 (partitions[i].type == EXTENDED_PARTITION3)) && (ipinfo->sblock != 0))
			continue;

		// if there is a valid partition, up the partition count
		if ((ipinfo->sblock == 0) || (partitions[i].type != 0))
			if (++pnum == index) {
				// fill in info if this is the partition we are seeking

				if (opinfo) {
					*opinfo = partitions[i];
					if ((opinfo->type == EXTENDED_PARTITION1) ||
					    (opinfo->type == EXTENDED_PARTITION2) ||
					    (opinfo->type == EXTENDED_PARTITION3))
						opinfo->nblocks = 1;
				}

				if (optinfo) {
					*optinfo = *ipinfo;
					memcpy(osb, sb, 512);
					*ofdisk = ((bootsector *)osb)->part + pnum;
				}
				found = TRUE;
			}
	}

	
	// recursively scan extended partitions
	for (i=0;i<4;i++) {
		uchar extsb[512];
		
		if (((partitions[i].type != EXTENDED_PARTITION1) &&
			(partitions[i].type != EXTENDED_PARTITION2) &&
			(partitions[i].type != EXTENDED_PARTITION3)) ||
			(partitions[i].nblocks == 0))
			continue;

		if (read_pos(dev, partitions[i].sblock * sector_size, extsb, 512) != 512) {
			dprintf("read from block %Lx failed\n", partitions[i].sblock);
			// don't allow mounting of any extended partitions, but
			// leave the primary ones alone so people don't get
			// stuck without any partitions at all
			return ((ipinfo->sblock != 0) || (index > 3)) ? B_ERROR : B_OK;
		}

		j = _get_nth_intel_partition_helper_(dev, extsb, partitions + i,
				(found ? 0xffff : index - pnum - 1),
				opinfo, optinfo, osb, ofdisk,
				(ipinfo->sblock == 0) ? partitions[i].sblock : extended_partition_lba_base,
				sector_size);

		if (j < 0)
			return ((ipinfo->sblock != 0) || (index > 3)) ? B_ERROR : B_OK;

		if (j == 0)
			found = TRUE;

		if (!found)
			pnum += j - 1;
	}
	
	if (found)
		return 0;

	// if we didn't find enough partitions, return the number found
	// (plus one to avoid confusion with B_NO_ERROR)
	return pnum + 2;
}

status_t get_nth_intel_partition(int32 dev, uchar *sb, uint64 block_num,
	int32 index, struct _partition_info_ *opinfo,
	struct _partition_info_ *optinfo, uchar *osb, fdisk **ofdisk)
{
	struct _partition_info_ ipinfo;
	device_geometry lbageo;

	if (ioctl(dev, B_GET_GEOMETRY, &lbageo) < 0)
		return B_ERROR;
	
	ipinfo.sblock = 0;
	ipinfo.nblocks = lbageo.cylinder_count * lbageo.sectors_per_track *
			lbageo.head_count;

	return _get_nth_intel_partition_helper_(dev, sb, &ipinfo, index, 
		opinfo, optinfo, osb, ofdisk, 0, lbageo.bytes_per_sector);
}
