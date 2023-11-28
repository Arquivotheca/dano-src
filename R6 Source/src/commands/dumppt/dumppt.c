#if 0
todo:
- more consistency checks
	- holes
- test consistency checks
- option to turn off consistency checks
#endif

#include <Drivers.h>
#include <support/SupportDefs.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct partition_entry {
	uchar	active;
	uchar	start_chs[3];
	uchar	id;
	uchar	end_chs[3];
	uint32	start_lba;
	uint32	len_lba;
};

#define CHS2LBA(c,h,s,geo) \
	(((c) * (geo)->head_count + (h)) * (geo)->sectors_per_track + (s) - 1)
#define ENT2LBA(z,g) \
	CHS2LBA((z[2] + 4*(z[1]&0xc0)),(z[0]),(z[1] & ~0xc0),(g))

#define IS_EXTENDED(id) \
	((id == 0x05) || (id == 0x0f) || (id == 0x85))

static void print_expected(uint32 lba, device_geometry *bios_geometry,
		uchar found[3])
{
	uint32 c, h, s, temp;
	if (!bios_geometry) return;
	c = lba / (bios_geometry->head_count * bios_geometry->sectors_per_track);
	temp = lba % (bios_geometry->head_count * bios_geometry->sectors_per_track);
	h = temp / bios_geometry->sectors_per_track;
	s = temp % bios_geometry->sectors_per_track + 1;
	if (CHS2LBA(c,h,s,bios_geometry) != lba)
		printf("bad lba calculated\n");
	printf("*** Expected CHS: %x/%x/%x (packed: ", c, h, s);
	if (c > 0x3ff)
		printf("ff/ff/ff) ");
	else
		printf("%2.2x/%2.2x/%2.2x) ",
				h, (s & ~0xc0) + ((c >> 2) & 0xc0), c & 0xff);
	printf("(found %2.2x/%2.2x/%2.2x) ***\n", found[0], found[1], found[2]);
}

static int display_partition(int fd, uint32 bnum_base, uint32 bnum, uint32 blen,
	device_geometry *bios_geometry)
{
	status_t error;
	uchar sector[512];
	struct partition_entry *entry;
	int i, j;

	lseek(fd, 512LL*bnum, 0);

	if ((error = read(fd, sector, 512)) < 512) {
		printf("error reading sector %x (%s)\n", bnum, strerror(error));
		return error;
	}

	if ((sector[0x1fe] != 0x55) || (sector[0x1ff] != 0xaa)) {
		printf("*** Warning: sector %x lacks partition table signature ***\n", bnum);
	}

	if (bnum == 0)
		printf(	"Partition         |  Starting     Ending | Starting   Ending   Number\n"
				"  Sector  # A  ID |   C  H  S    C  H  S |   Sector   Sector  Sectors\n"
				"==================+======================+===========================\n");

	for (i=0,entry=(struct partition_entry *)(sector + 0x1be);i<4;i++,entry++) {
		uint32 start_lba, end_lba;

		if (entry->id == 0)
			continue;

		printf(" %8x %d %02x %02x ", bnum, i, entry->active, entry->id);

		start_lba = entry->start_lba + ((IS_EXTENDED(entry->id)) ? bnum_base : bnum);
		end_lba = start_lba + entry->len_lba - 1;

#define PRINTF_CHS(z) \
	printf("%3x %2x %2x ",(z[2] + 4*(z[1]&0xc0)),(z[0]),(z[1] & ~0xc0))

		printf("| ");
		PRINTF_CHS(entry->start_chs);
		printf(" ");
		PRINTF_CHS(entry->end_chs);
		printf("| ");

		printf("%8x %8x %8x\n", start_lba, end_lba, entry->len_lba);

		/* consistency checks follow */

		if (entry->active & 0x7f)
			printf("*** Warning: Active field (A) is usually set to either 00 or 80 ***\n");

		/* vyt: fix this check */
		if ((end_lba >= bnum + blen) && !IS_EXTENDED(entry->id))
			printf("*** Error: Partition extends past end (%x) of %s. ***\n", bnum+blen-1, bnum ? "logical partition" : "drive");

		if ((entry->start_chs[1] & ~0xc0) != 1)
			printf("*** Warning: Partition doesn't start on a head boundary. ***\n");
		else if (entry->start_chs[0] > 1)
			printf("*** Warning: Starting head should be 0 or 1. ***\n");

		if (bios_geometry) {
			uint32	start_chs = ENT2LBA(entry->start_chs, bios_geometry),
				end_chs = ENT2LBA(entry->end_chs, bios_geometry);

			if ((entry->end_chs[0] != bios_geometry->head_count - 1) &&
					((entry->end_chs[1] & ~0xc0) != bios_geometry->sectors_per_track))
				printf("*** Warning: Partition doesn't end on cylinder boundary. ***\n");

			if (start_chs != start_lba) {
				printf("*** Warning: CHS (%x) and LBA (%x) starting sectors don't match. ***\n", start_chs, start_lba);
				print_expected(start_lba, bios_geometry, entry->start_chs);
			}

			if (end_chs != end_lba) {
				printf("*** Warning: CHS (%x) and LBA (%x) ending sectors don't match. ***\n", end_chs, end_lba);
				print_expected(end_lba, bios_geometry, entry->end_chs);
			}
		}

		/* check for overlap */
		for (j=0;j<i;j++) {
			struct partition_entry *e = (struct partition_entry *)(sector + 0x1be + j*0x10);
			if (((start_lba >= e->start_lba) && (start_lba < e->start_lba + e->len_lba)) ||
					((end_lba >= e->start_lba) && (end_lba < e->start_lba + e->len_lba)))
				printf("*** Error: Partitions %x and %x overlap ***\n", i, j);
		}
	}

	/* scan extended partitions */
	for (i=0,entry=(struct partition_entry *)(sector + 0x1be);i<4;i++,entry++) {
		if (!IS_EXTENDED(entry->id))
			continue;
		display_partition(fd, (bnum == 0) ? entry->start_lba : bnum_base, bnum_base + entry->start_lba, entry->len_lba, bios_geometry);
	}
}

void display_drive(char *path)
{
	status_t error;
	uint32 sectors;
	int fd;
	device_geometry geometry;
	bool has_bios_geometry = false;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("Error opening %s (%s)\n", path, strerror(fd));
		return;
	}
	printf("Partition information for %s:\n", path);
	if ((error = ioctl(fd, B_GET_GEOMETRY, &geometry)) < 0) {
		printf("error getting geometry of %s (%s)\n", path, strerror(error));
		close(fd);
		return;
	}
	printf("LBA geometry: %x sectors, %x bytes/sector\n",
		geometry.cylinder_count *
		geometry.sectors_per_track *
		geometry.head_count,
		geometry.bytes_per_sector);
	sectors = geometry.cylinder_count * geometry.sectors_per_track * geometry.head_count;
	if (ioctl(fd, B_GET_BIOS_GEOMETRY, &geometry) == B_OK) {
		has_bios_geometry = true;
		printf("BIOS geometry: %x cylinders, %x sectors/track, %x heads, %x bytes/sector\n",
			geometry.cylinder_count,
			geometry.sectors_per_track,
			geometry.head_count,
			geometry.bytes_per_sector);
		if ((geometry.sectors_per_track == 0) || 
				(geometry.head_count == 0)) {
			printf("*** Warning: BIOS geometry appears to be invalid.  Ignored. ***\n");
			has_bios_geometry = false;
		}
	} else
		printf("*** Warning: BIOS geometry unavailable. Some checks will be skipped. ***\n");

	display_partition(fd, 0, 0, sectors, has_bios_geometry ? &geometry : NULL);
	printf("\n");

	close(fd);

	return;
}

int main(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		printf("usage: %s [ dev1 [ dev2 ... ] ]\n", *argv);
		return 1;
	}

	for (i=1;i<argc;i++)
		display_drive(argv[i]);

	return 0;
}
