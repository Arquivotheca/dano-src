#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

#include <fmap.h>

#include "bootmenu.h"
#include "bt_devfs.h"
#include "bt_misc.h"

struct pairs {
	char fs[8], file[32];
} pairs[] = {
	{ "dos", "BEOS/IMAGE.BE" },
	{ "ext2", "beos/image.be" },
	{ "ntfs", "beos/image.be" },
	{ "", "" }
};

struct node {
	char name[32];
	char master[32];
	struct pairs *p;
	partition_info pi;
	uchar biosid;

	char mountpoint[16];
	int fd;

	struct node *next;
};

static int num_fmaps;
static struct node *fmaps;
static char **names;

static uint32 crc32(uchar *buff, int len)
{
	uint32 crc = 0xffffffff;
	static uint32 crc32tab[0x100];
	static bool initialized = FALSE;
	
	if (!initialized) {
		uint32 i, j, c;
		for (i=0;i<0x100;i++) {
			c = i;
			for (j=0;j<8;j++)
				c = (c & 1) ? ((c >> 1) ^ 0xedb88320) : (c >> 1);
			crc32tab[i] = c;
		}
		initialized = TRUE;
	}

	while (len--)
		crc = (crc >> 8) ^ crc32tab[(crc ^ *(buff++)) & 0xff];

	return crc ^ 0xffffffff;
}


static uint32
calculate_fmap_checksum(struct fmap_info *f)
{
	return crc32((uchar *)f + sizeof(uint32), f->size - sizeof(uint32));
}

static status_t
add_fmap(void *cookie, const char *path)
{
	int i;

	for (i=0;pairs[i].fs[0];i++) {
		int fd, gd;
		char fname[32], mpoint[32];
		struct node *f, *n;
		partition_info pi;
		uchar biosid;

		sprintf(mpoint, "/fmap%d", num_fmaps);
		if (mount(pairs[i].fs, mpoint, path, 0, NULL, 0) < B_OK)
			continue;

		sprintf(fname, "%s/%s", mpoint, pairs[i].file);
		fd = open(fname, 0);
		if (fd < 0) {
			unmount(mpoint);
			continue;
		}

		gd = open(path, 0);
		if (gd < 0) {
			close(fd);
			unmount(mpoint);
			return gd;
		}
		if (	(ioctl(gd, B_GET_PARTITION_INFO, &pi, sizeof(pi)) != B_OK) ||
				(ioctl(gd, B_GET_BIOS_DRIVE_ID, &biosid, sizeof(biosid)) != B_OK)) {
			close(gd);
			close(fd);
			unmount(mpoint);
			return B_ERROR;
		}
		close(gd);

		n = malloc(sizeof(*n));
		if (!n) {
			close(fd);
			unmount(mpoint);
			return ENOMEM;
		}
		
		sprintf(n->name, "fmap/%d/raw", num_fmaps++);
		strcpy(n->master, path);
		n->pi = pi;
		n->biosid = biosid;
		n->p = pairs + i;
		strcpy(n->mountpoint, mpoint);
		n->fd = fd;
		n->next = NULL;
		if (fmaps == NULL) {
			fmaps = n;
		} else {
			for (f=fmaps;f->next;f = f->next)
				;
			f->next = n;
		}

		return B_OK;
	}

	return B_OK;
}

static status_t
fmap_init_driver()
{
	status_t error;
	int i;
	struct node *n;

	num_fmaps = 0;
	fmaps = NULL;
	names = NULL;

	if ((error = recurse("/dev/disk", add_fmap, NULL)) < B_OK)
		return error;

	names = calloc(num_fmaps + 1, sizeof(char *));
	for (i=0,n=fmaps;n;n=n->next,i++) {
		names[i] = malloc(strlen(n->name) + 1);
		strcpy(names[i], n->name);
	}

	return B_OK;
}

static status_t
fmap_uninit_driver()
{
	struct node *node, *anode;
	int i;

	for (node=fmaps;node;node=anode) {
		anode = node->next;
		close(node->fd);
	 	unmount(node->mountpoint);
		free(node);
		node = anode;
	}

	for (i=0;names[i];i++)
		free(names[i]);
	free(names);
	
	return 0;
}

static const char **fmap_publish_devices(void)
{
	return (const char **)names;
}

static status_t
fmap_open(const char *name, void **cookie)
{
	struct node *n;

	for (n=fmaps;n;n=n->next)
		if (!strcmp(name, n->name)) {
			*cookie = (void *)n;
			return B_OK;
		}

	return ENOENT;
}

static status_t
fmap_close(void *cookie)
{
	return B_OK;
}

static ssize_t
fmap_read(void *cookie, off_t pos, void *buffer, size_t len)
{
	struct node *n = (struct node *)cookie;

	return read_pos(n->fd, pos, buffer, len);
}

static status_t
fmap_ioctl(void *cookie, uint32 op, void *buffer, uint32 size)
{
	status_t error;
	struct node *n = (struct node *)cookie;

	if (op == 'imfs') {
		struct bootable_volume_info *v = (struct bootable_volume_info *)buffer;
		strcpy(v->master_boot_device, n->master);
		strcpy(v->boot_image_file, n->p->file);
		strcpy(v->boot_fs, n->p->fs);
		return B_OK;
	} else if (op == 'fmap') {
		error = ioctl(n->fd, op, buffer, size);

		if (error == B_OK) {
			struct fmap_info *f = *(struct fmap_info **)buffer;

			if (	(f->type != FMAP_TYPE_BLOCK) &&
					(f->type != FMAP_TYPE_BYTE))
				panic("Unknown fmap type: %x\n", f->type);
			if (f->size < sizeof(*f) - sizeof(struct fmap_byte_run))
				panic("fmap size too small: %x\n", f->size);

			f->offset += n->pi.offset;
			f->bios_id = n->biosid;

			f->checksum = calculate_fmap_checksum(f);
		}
		
		return error;
	} else if (op == B_GET_GEOMETRY) {
		device_geometry *g = (device_geometry *)buffer;
		struct stat st;

		if ((error = fstat(n->fd, &st)) < B_OK)
			return error;

		g->cylinder_count = st.st_size / 0x200;
		g->sectors_per_track = 1;
		g->head_count = 1;
		g->bytes_per_sector = 0x200;

		return B_OK;
	}

//	dprintf("fmap: Unknown ioctl(): %lx\n", op);

	return ENOSYS;
}

struct bt_driver_hooks fmap_driver_hooks = {
	"fmap",

	fmap_init_driver,
	fmap_uninit_driver,

	fmap_publish_devices,

	fmap_open,
	fmap_close,
	fmap_read,
	fmap_ioctl
};
