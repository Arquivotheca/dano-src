#include <drivers/KernelExport.h>

#include <malloc.h>

#include <biosdriveinfo.h>

#include "bios.h"

status_t create_bios_drive_info_table(
		uint32 *num_entries, struct bios_drive_info **table)
{
	int i, num_drives;
	struct bios_drive_info *drives;

	num_drives = bios_get_number_hard_drives();
	if (num_drives < 1) {
		dprintf("Where are the hard drives?\n");
		return B_ERROR;
	}

	dprintf("%d hard drives detected\n", num_drives);

	*table = (struct bios_drive_info *)calloc(num_drives + 1, sizeof(**table));
	if (!(*table)) return ENOMEM;

	drives = *table;

	for (i=0;i<num_drives;i++) {
		device_geometry g;
		uchar mbr[0x200], *buffer;
		int n, part;
		bool partition_map_exists;

		drives[i].bios_id = 0x80 + i;
		if (get_drive_geometry(drives[i].bios_id, &g) == B_OK) {
			drives[i].c = g.cylinder_count;
			drives[i].h = g.head_count;
			drives[i].s = g.sectors_per_track;
		}

		/* always read at least 4 sectors */
		n = g.sectors_per_track;
		if (n < 4) n = 4;

		buffer = malloc(0x200 * n);

		if (read_disk(drives[i].bios_id, 0, buffer, n)) {
			free(buffer);
			continue;
		}
		
	
		/*
			Check for the combination of a broken BIOS & old ATA (not ATAPI!) ZIP drive
			that don't return an error on read of an empty drive.
		*/
		if( (drives[i].c == 511) &&	/* Geometry of a ATA ZIP drive */
			(drives[i].h == 12)  &&
			(drives[i].s == 32)  )
		{
			int j;
			for(j=0; j<n*0x200; j++)
				if(buffer[j] != 0)
					break;
			
			if(j == n*0x200)
			{
				dprintf("Can't read ZIP disk\n");
				free(buffer);
				continue;
			}
		}

		memcpy(mbr, buffer, 0x200);

		drives[i].hashes[0].offset = 0;
		drives[i].hashes[0].size = n * 0x200;
		drives[i].hashes[0].hash = boot_calculate_hash(buffer, n * 0x200);
		drives[i].num_hashes++;

		partition_map_exists = FALSE;

		/* crude verification of partition map */
		if ((mbr[0x1fe] == 0x55) && (mbr[0x1ff] == 0xaa)) {
			for (part = 0; part < 4; part++) {
				/* check for a non-zero partition id */
				if (mbr[0x1be + 0x10*part + 4] == 0)
					continue;

				/* if it's not marked active, there's nothing more we can do */
				if (mbr[0x1be + 0x10*part] == 0)
					continue;

				/* sanity check the bios drive number */
				if (	(mbr[0x1be + 0x10*part] < 0x80) ||
						(mbr[0x1be + 0x10*part] > 0x87))
					break;
			}
			if (part == 4)
				partition_map_exists = TRUE;
		}

		if (partition_map_exists) {
			for (part = 0; part < 4; part++) {
				uint32 sector;

				if (mbr[0x1be +0x10*part+4] == 0) continue; /* check if empty */

				sector = *(uint32 *)(mbr + 0x1be + 0x10*part + 8);

				if (read_disk(drives[i].bios_id, sector, buffer, n))
					continue;

#define idx (drives[i].num_hashes)
				drives[i].hashes[idx].offset = sector * 0x200LL;
				drives[i].hashes[idx].size = n * 0x200;
				drives[i].hashes[idx].hash =
						boot_calculate_hash(buffer, n * 0x200);
				drives[i].num_hashes++;
#undef idx
			}
		}

		free(buffer);
	}

	*num_entries = num_drives + 1;

	return B_OK;
}
