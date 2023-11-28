#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <drivers/KernelExport.h>

#include "bios.h"
#include "bt_devfs.h"

struct disk {
	uint32 biosid;
	char name[sizeof("disk/0x00/raw")];
	uint32 block_size;
	uchar MBR[512];
	struct disk *next;
};

static struct disk *disks = NULL;
static char **names;

static const char pattern1[] =
		"This is a pattern that you are unlikely to find on a disk";
static const char pattern2[] =
		"This is another pattern that you are unlikely to find on a disk";
static const char pattern3[] =
		"This is yet another pattern that you are unlikely to find on a disk";

static void
fill_MBR_with_pattern(uchar MBR[2048])
{
	memcpy(MBR, pattern1, sizeof(pattern1));
	memcpy(MBR + 512, pattern2, sizeof(pattern2));
	memcpy(MBR + 1536, pattern3, sizeof(pattern3));
}

static uint32
determine_block_size(uchar MBR[2048])
{
	if (memcmp(MBR + 1536, pattern3, sizeof(pattern3)))
		return 2048;
	if (memcmp(MBR + 512, pattern2, sizeof(pattern2)))
		return 1024;
	if (memcmp(MBR, pattern1, sizeof(pattern1)))
		return 512;
	return 0;
}

static status_t
disk_init_driver()
{
	int drives, id, i;
	struct disk *d;
	uchar MBR[2048];

	static uint32 mounted_count = 0;
	extern uint32 boot_drive;

	disks = NULL;
	names = NULL;

	drives = bios_get_number_hard_drives();
	if (drives < 0) drives = 8;

	i = 0;

	/* create one disk entry for each drive in the system */
	for (id=0x80+drives-1;id>=0x80;id--) {
		fill_MBR_with_pattern(MBR);
		if (read_disk(id, 0, MBR, 1) < 0) continue;
		if (determine_block_size(MBR) == 0) continue;

		d = malloc(sizeof(*d));
		d->biosid = id;
		sprintf(d->name, "disk/0x%2.2x/raw", id);
		d->block_size = determine_block_size(MBR);
		memcpy(d->MBR, MBR, 512);
		d->next = disks;
		disks = d;

		i++;
	}

	if ((mounted_count++ != 0) || (boot_drive == 0)) {
		fill_MBR_with_pattern(MBR);
		if (	(read_disk(0, 0, MBR, 1) == B_OK) &&
				(determine_block_size(MBR) != 0)) {
			/* copy that floppy */
			d = malloc(sizeof(*d));
			d->biosid = 0;
			strcpy(d->name, "disk/0x00/raw");
			d->block_size = determine_block_size(MBR);
			memcpy(d->MBR, MBR, 512);
			d->next = disks;
			disks = d;

			i++;
		}
	}

	names = calloc(i + 1, sizeof(char *));
	for (i=0,d=disks;d;d=d->next,i++) {
		names[i] = malloc(strlen(d->name) + 1);
		strcpy(names[i], d->name);
	}

	return B_OK;
}

static status_t
disk_uninit_driver()
{
	struct disk *d, *e;
	int i;

	for (d=disks;d;d=e) {
		e = d->next;
		free(d);
		d = e;
	}

	for (i=0;names[i];i++)
		free(names[i]);
	free(names);
	
	return 0;
}

static const char **disk_publish_devices(void)
{
	return (const char **)names;
}

static status_t
disk_open(const char *name, void **cookie)
{
	struct disk *d;

	for (d=disks;d;d=d->next)
		if (!strcmp(name, d->name)) {
			*cookie = (void *)d;
			return B_OK;
		}

	return ENOENT;
}

static status_t
disk_close(void *cookie)
{
	return B_OK;
}

static ssize_t
disk_read(void *cookie, off_t pos, void *buffer, size_t len)
{
	struct disk *d = (struct disk *)cookie;
	size_t amount, alen;
	status_t result;
	uchar copybuffer[2048];

	if ((pos == 0LL) && (len <= 512)) {
		memcpy(buffer, d->MBR, len);
		return len;
	}

	amount = 0;

	if (pos & (d->block_size - 1)) {
		result = read_disk(d->biosid, pos / d->block_size, copybuffer, 1);
		if (result != B_OK)
			return (result < B_OK) ? result : EIO;
		if ((pos & (d->block_size - 1)) + len > d->block_size)
			amount = d->block_size - (pos & (d->block_size - 1));
		else
			amount = len;
		memcpy(buffer, copybuffer + (pos & (d->block_size - 1)), amount);

		pos += amount;
		buffer = (void *)((uchar *)buffer + amount);
	}

	alen = (len - amount) & ~(d->block_size - 1);
	if (alen) {
		result = read_disk(d->biosid, pos / d->block_size, buffer, alen / d->block_size);
		if (result != B_OK)
			return (result < B_OK) ? result : EIO;

		amount += alen;
		pos += alen;
		buffer = (void *)((uchar *)buffer + alen);
	}

	if (amount != len) {
		result = read_disk(d->biosid, pos / d->block_size, copybuffer, 1);
		if (result != B_OK)
			return (result < B_OK) ? result : EIO;
		memcpy(buffer, copybuffer, len - amount);
	}

	return len;
}

static status_t
disk_ioctl(void *cookie, uint32 op, void *buffer, uint32 size)
{
	struct disk *d = (struct disk *)cookie;

	if (op == B_GET_GEOMETRY) {
		return get_drive_geometry(d->biosid, buffer);
	} else if (op == B_GET_BIOS_DRIVE_ID) {
		*(uchar *)buffer = d->biosid;
		return 0;
	}

//	dprintf("disk: Unknown ioctl(): %lx\n", op);

	return ENOSYS;
}

struct bt_driver_hooks disk_driver_hooks = {
	"disk",

	disk_init_driver,
	disk_uninit_driver,

	disk_publish_devices,

	disk_open,
	disk_close,
	disk_read,
	disk_ioctl
};
