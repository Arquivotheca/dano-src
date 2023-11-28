/* mkdos: creates a fresh dos file system on a drive/partition */

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Drivers.h>
#include <SupportDefs.h>

bool test_mode = false, noprompt_mode = false;

/* careful with these macros with if statements! */
#define write32(buffer,off,val) \
	((uint8 *)(buffer))[(off)] = (val) & 0xff; \
	((uint8 *)(buffer))[(off)+1] = ((val) >> 8) & 0xff; \
	((uint8 *)(buffer))[(off)+2] = ((val) >> 16) & 0xff; \
	((uint8 *)(buffer))[(off)+3] = ((val) >> 24) & 0xff

#define write16(buffer,off,val) \
	((uint8 *)(buffer))[(off)] = (val) & 0xff; \
	((uint8 *)(buffer))[(off)+1] = ((val) >> 8) & 0xff

#define read16(buffer,off) \
	(((uint8 *)buffer)[(off)] + (((uint8 *)buffer)[(off)+1] << 8))

#define read32(buffer,off) \
	(read16((buffer),(off)) + 0x10000 * read16((buffer),(off)+2))

struct media_types {
	uchar descriptor;
	int c, h, s;
} descriptors[] = {
	{ 0xf0, 80, 2, 36 }, /* 2.88 */
	{ 0xf0, 80, 2, 18 }, /* 1.44 */
	{ 0xf9, 80, 2,  9 }, /* 720k */
	{ 0xf9, 80, 2, 15 }, /* 1.2 */
	{ 0xfd, 40, 2,  9 }, /* 360k */
	{ 0xff, 40, 2,  8 }, /* 320k */
	{ 0xfc, 40, 1,  9 }, /* 180k */
	{ 0xfe, 40, 1,  8 }, /* 160k */
	{ 0xf8, 0, 0, 0 }    /* good ol' winchester drive */
};

/* vyt: replace with a boot loader that can actually load system files */
uchar bootloader[] = {
	0x31, 0xc0, 0x8e, 0xd0, 0xbc, 0x00, 0x7c, 0x8e, 0xd8,
	0xb4, 0x0e, 0x31, 0xdb, 0xbe, 0x00, 0x7d, 0xfc,
	0xac, 0x08, 0xc0, 0x74, 0x04, 0xcd, 0x10, 0xeb, 0xf7,
	0xb4, 0x00, 0xcd, 0x16, 0xcd, 0x19
};

uchar bootstring[] =	"Sorry, this disk is not bootable.\r\n"
			"Please press any key to reboot.\r\n";

static void usage(const char *format, ...)
{
	va_list args;
	va_start(args,format);
	vfprintf(stdout, format, args);
	printf(	"\nusage: mkdos [-n] [-t] [-f 12|16|32] device [volume_label]\n"
		"	-n, --noprompt  do not prompt before writing\n"
		"       -t, --test      enable test mode (will not write to disk)\n"
		"       -f, --fat       use FAT entries of the specified size\n"
	      );
	exit(B_ERROR);
}

static uint32 calc_spc(uint32 sectors, int shift)
{
	int result = 1;
	sectors >>= shift;
	while (sectors) {
		result <<= 1;
		sectors >>= 1;
	}
	return result;
}

#ifdef DEBUG
static void display_volume_info(uchar *buffer)
{
	uint32 sectors = read16(buffer,19), fat_sectors = read16(buffer,22);
	if (!sectors) sectors = read32(buffer,32);
	if (!fat_sectors) fat_sectors = read32(buffer,36);

	if ((read16(buffer,0x1fe) != 0xaa55) ||
		(!strncmp((char *)buffer+3,"NTFS    ",8)) ||
		(!strncmp((char *)buffer+3,"OS2 ",4)) ||
		(buffer[13] == 0)) {
		printf("Not a FAT volume\n");
		return;
	}
	
	printf(	"existing:\n"
		"%d sectors (%d M) %d reserved\n"
		"%d sectors/cluster, descriptor %x\n"
		"%d sectors/FAT, %d root entries, %d clusters\n"
		"creator '%8.8s'\n",
		sectors, sectors/2048, read16(buffer,14),
		buffer[13], buffer[21],
		fat_sectors, read16(buffer,17), (sectors - buffer[16]*fat_sectors - read16(buffer,14) - read16(buffer,17)/0x10)/buffer[13],
		buffer+3
	);
}
#endif

static status_t write_fats(int fd, off_t pos, const void *buf, size_t count, uint32 sectors_per_fat)
{
	status_t error;
	
	if ((error = write_pos(fd, pos, buf, count)) < count) {
		printf("mkdos error writing FAT (%s)\n", strerror(error));
		return error;
	}
	
	if ((error = write_pos(fd, pos + sectors_per_fat*0x200ULL, buf, count)) < count) {
		printf("mkdos error writing FAT (%s)\n", strerror(error));
		return error;
	}

	return error;
}

static status_t make_dos(char *device, char *name, int fat_bits)
{
	bool has_bios_geometry = false;
	int fd, error, i, j;
	uint32 sectors, spc /* sectors/clusters */, 
		reserved_sectors, root_entries, clusters, fat_sectors;
	uchar descriptor, *buffer = NULL;
	char *system_id = "BADSYSID";
	uchar volume_label[11];
	device_geometry geometry, bios_geometry;

	assert((fat_bits == 0) || (fat_bits == 12) || (fat_bits == 16) || (fat_bits == 32));

	if (name) {
		/* sanitize name */
		static char acceptable[] = " !#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~";
		memset(volume_label, ' ', 11);
		for (i=j=0;name[i] && (j < 11);i++) {
			if (islower(name[i])) name[i] += 'A' - 'a';
			if (strchr(acceptable,name[i]))
				volume_label[j++] = name[i];
		}
		if (j == 0) {
			printf("mkdos warning: no valid characters in volume name\n");
			name = NULL;
		}
	}

	/* vyt: should change device to canonical form, else can skirt around
	 * the following check 
	 */
	if (!strncmp(device, "/dev/disk/", 10) && 
			!strstr(device, "floppy") && 
			strstr(device, "raw"))
		usage("mkdos error: can't overwrite entire disk with the dos fs!\n");

	if ((fd = open(device, O_RDWR)) < 0) {
		printf("mkdos error: cannot open %s for read/write\n", device);
		return fd;
	}

	/* vyt: support disk images later */
	if ((error = ioctl(fd, B_GET_GEOMETRY, &geometry)) < 0) {
		printf("mkdos error: cannot get geometry for %s\n", device);
		goto bi;
	}
	
	printf("geometry: cylinders = %d, sectors = %d, heads = %d\n",
			geometry.cylinder_count,
			geometry.sectors_per_track,
			geometry.head_count);

	if (ioctl(fd, B_GET_BIOS_GEOMETRY, &bios_geometry) == B_OK) {
		has_bios_geometry = true;
		printf("BIOS geometry: cylinders = %d, sectors = %d, heads = %d\n",
				bios_geometry.cylinder_count,
				bios_geometry.sectors_per_track,
				bios_geometry.head_count);
	}

	{struct media_types *d;
	for (d=descriptors;d->c;d++)
		if ((d->c == geometry.cylinder_count) &&
				(d->h == geometry.head_count) &&
				(d->s == geometry.sectors_per_track))
			break;
	descriptor = d->descriptor;}
	
	sectors = geometry.cylinder_count * geometry.sectors_per_track * geometry.head_count;

	if (fat_bits == 0) {
		if (sectors < 32768) {		/* < 16 mb */
			fat_bits = 12;
		} else if (sectors < 4194304) {	/* < 2048 mb */
			fat_bits = 16;
		} else {
			fat_bits = 32;
		}
	}

	if (fat_bits == 12) {
		system_id = "FAT12   ";
		spc = (descriptor == 0xf8) ? 8 : 1;
		root_entries = 0xe0;
		reserved_sectors = 1;
	} else if (fat_bits == 16) {
		system_id = "FAT16   ";
		/* < 128M = 4 spc, < 256M = 8 spc, < 512M = 16 spc, etc */
		spc = calc_spc(sectors, 16);
		root_entries = 0x200;
		reserved_sectors = 1;
	} else {
		system_id = "FAT32   ";
		/* supposedly fat32 sectors/cluster must be at least 4, 
	   	   but experience shows otherwise */
		/* vyt: this is just a guess */
		spc = calc_spc(sectors, 20);
		root_entries = 0;
		reserved_sectors = 32;
	}

	if (spc > 64) {
		printf(	"mkdos error: Calculated sectors/cluster too large (%d).\n"
			"Try using a larger sized FAT\n",
			spc);
		goto bi;
	}

	clusters = (sectors - reserved_sectors - root_entries/0x10) / spc;
	do {
		i = clusters;
		fat_sectors = ((clusters * fat_bits + 7) / 8 + 511) / 512;
		clusters = (sectors - reserved_sectors - root_entries/0x10 - 2*fat_sectors) / spc;
	} while (i != clusters);

	if (((fat_bits == 12) && (clusters > 0xfef)) ||
	    ((fat_bits == 16) && (clusters > 0xffef))) {
		printf(	"mkdos error: Too many clusters for a %d-bit FAT (%d).\n"
			"Try using a larger sized FAT\n",
			fat_bits, clusters);
		goto bi;
	}

	printf("initializing %s\n", device);
	printf(	"%d sectors (%d M) %d reserved\n"
			"%d-bit FAT, %d sectors/cluster, descriptor %x\n"
			"%d sectors/FAT, %d root entries, %d clusters\n", 
			sectors, sectors/2048, reserved_sectors,
			fat_bits, spc, descriptor,
			fat_sectors, root_entries, clusters);
	if (name)
		printf("volume label '%11.11s'\n", volume_label);
	else
		printf("no volume label\n");

	if ((buffer = malloc(0x10000)) == NULL) {
		printf("mkdos error: out of memory\n");
		goto bi;
	}
	memset(buffer, 0, 0x10000);

	if (noprompt_mode == false) {
		char confirm[8];
		printf(	"\nInitializing will erase all existing data on the drive.\n"
			"Do you wish to proceed? ");
		if (fgets(confirm,7,stdin) == NULL)
			goto bi;
		if (strcasecmp(confirm,"y\n") && strcasecmp(confirm,"yes\n")) {
			printf("drive NOT initialized\n");
			goto bi;
		}
	}

	if (test_mode) {
#ifdef DEBUG
		if ((error = read_pos(fd, 0ULL, buffer, 0x200)) < 0x200) {
			printf("mkdos error reading boot sector (%s)\n", strerror(error));
			goto bi;
		}
		
		display_volume_info(buffer);
#endif
		goto bi;
	}

	/* create boot block */
	printf("Writing boot block\n");
	memset(buffer, 0, 512);
	write16(buffer,0x1fe,0xaa55);
	buffer[0] = 0xeb;
	buffer[1] = 0x7e;
	buffer[2] = 0x90;
	memcpy(buffer+3, "BeOS    ", 8);
	write16(buffer,11,0x200); /* bytes per sector */
	buffer[13] = spc; /* sectors/cluster */
	write16(buffer,14,reserved_sectors); /* reserved sectors */
	buffer[16] = 2;       /* # fats */
	write16(buffer,17,root_entries); /* root entries */
	if ((sectors < 0x10000) && (fat_bits != 32)) {
		write16(buffer,19,sectors); /* small sectors */
	}
	buffer[21] = descriptor; /* media descriptor */
	if (fat_bits != 32) {
		write16(buffer,22,fat_sectors); /* sectors/fat */
	}
	if (has_bios_geometry) {
		write16(buffer,24,bios_geometry.sectors_per_track); /* sectors/track */
		write16(buffer,26,bios_geometry.head_count); /* heads */
	}
	/* vyt: hidden sectors are from start of disk! how to calculate? */
	write16(buffer,28,0); /* hidden sectors */
	write32(buffer,32,sectors); /* large sectors */
	i = 36;
	if (fat_bits == 32) {
		write32(buffer,36,fat_sectors); /* large sectors/fat */
		write16(buffer,40,0x0080); /* extended flags */
		write16(buffer,42, 0); /* file system version */
		write32(buffer,44, 2); /* root cluster */
		write16(buffer,48, 1); /* fs info sector */
		write16(buffer,50, 0xffff); /* backup boot sector */
		i = 64;
	}
	if (descriptor == 0xf8) buffer[i] = 0x80; /* physical drive number */
	buffer[i+2] = 0x29; /* signature */
	/* vyt: fill in serial number */
	memcpy(buffer+i+7, "NO NAME    ", 11); /* volume label */
	memcpy(buffer+i+18, system_id, 8); /* FATXX */

	memcpy(buffer+0x80, bootloader, sizeof(bootloader));
	memcpy(buffer+0x100, bootstring, sizeof(bootstring));
	
	if ((error = write_pos(fd, 0ULL, buffer, 0x200)) < 0x200) {
		printf("mkdos error writing boot sector (%s)\n", strerror(error));
		goto bi;
	}

	/* write boot info */
	if (fat_bits == 32) {
		printf("Writing boot info\n");
		memset(buffer, 0, 0x200);

		/* patch in magic numbers */
		write32(buffer,0,0x41615252);
		write32(buffer,0x1e4,0x61417272);
		write16(buffer,0x1fe,0xaa55);

		write32(buffer,0x1e8, clusters - 1); /* free clusters (1 for root) */
		write32(buffer,0x1ec, 2); /* last allocated (root dir) */

		if ((error = write_pos(fd, 0x200ULL, buffer, 0x200)) < 0x200) {
			printf("mkdos error writing fsinfo (%s)\n", strerror(error));
			goto bi;
		}
	}
	
	/* now write fats and root directory */
	if (fat_bits == 32)
		root_entries = spc * 0x10;

	lseek(fd,0x200ULL * reserved_sectors, 0);

	printf("Writing FAT\n");
	memset(buffer, 0, 0x200);
	// first two entries of each fat table are invalid;
	// first byte of each table should match the descriptor
	buffer[0] = descriptor;
	buffer[1] = buffer[2] = 0xff;
	if (fat_bits != 12) {
		buffer[3] = 0xff;
		if (fat_bits == 32)
			memset(buffer+4,0xff,8); /* terminate cluster 2 too */
	}
	if ((error = write_fats(fd, 0x200 * reserved_sectors, buffer, 0x200, fat_sectors)) < 0x200)
		goto bi;

	memset(buffer, 0, 12);

	for (i=1;i < fat_sectors; i+=j) {
		j = fat_sectors - i;
		if (j > 0x10000/0x200)
			j = 0x10000/0x200;
		if ((error = write_fats(fd, 0x200 * (reserved_sectors + i), buffer, j * 0x200, fat_sectors)) < j * 0x200)
			goto bi;
	}

	printf("Writing root directory\n");
	i=0;
	if (name) {
		memcpy(buffer, volume_label, 11);
		buffer[0x0b] = 8;
		if ((error = write_pos(fd, 0x200ULL * (reserved_sectors + 2*fat_sectors), buffer, 0x200)) < 0x200) {
			printf("mkdos error writing root directory (%s)\n", strerror(error));
			goto bi;
		}
		memset(buffer, 0, 0x20);
		i=1;
	}
	for (;i < root_entries/0x10; i+=j) {
		j = root_entries/0x10 - i;
		if (j > 0x10000/0x200)
			j = 0x10000/0x200;
		if ((error = write_pos(fd, 0x200ULL * (reserved_sectors + 2*fat_sectors + i), buffer, j * 0x200)) < j * 0x200) {
			printf("mkdos error writing root directory (%s)\n", strerror(error));
			goto bi;
		}
	}
	
	fsync(fd);

	error = B_OK;

bi:
	if (buffer) free(buffer);
	
	close(fd);

	return error;
}

int main(int argc, char **argv)
{
	int i;
	int fat_bits = 0;
	char *device = NULL, *name = NULL;

	struct option longopts[] = {
		{ "fat", required_argument, 0, 'f' },
		{ "noprompt", no_argument, 0, 'n' },
		{ "test", no_argument, 0, 't' },
		{ NULL, 0, 0, 0 }
	};

	while ((i = getopt_long(argc, argv, "tnf:", longopts, NULL)) != EOF) {
		switch (i) {
			case 't' :
				printf("test mode enabled (no writes will occur)\n");
				test_mode = true;
				break;
			case 'f' :
				if (fat_bits != 0)
					usage("mkdos error: already specified %d-bit fat\n", fat_bits);
				if ((*optarg < '0') || (*optarg > '9'))
					usage("mkdos error: fat option requires an argument\n");

				fat_bits = atoi(optarg);
				if ((fat_bits != 12) && (fat_bits != 16) && 
						(fat_bits != 32))
					usage("mkdos error: fat must be 12, 16, or 32 bits, not %d\n", fat_bits);
				printf("using a %d-bit fat\n", fat_bits);
				break;
			case 'n' :
				printf("will not prompt for confirmation\n");
				noprompt_mode = true;
				break;
			default: usage("");
		}
	}

	if (optind != argc) {
		device = argv[optind++];
		if (optind != argc)
			name = argv[optind++];
		if (optind != argc)
			usage("%s: unrecognized option '%s'\n", *argv, argv[optind]);
	}

	if (device == NULL) {
		usage(	"mkdos error: you must specify a device or partition\n"
			"             such as /dev/disk/ide/ata/1/master/0/0_0\n" );
	}

	return make_dos(device, name, fat_bits);
}
