#include <drivers/KernelExport.h>

#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <fmap.h>

#include "bt_fs.h"

struct minidos_vcookie {
	int fd;

	int32 bytes_per_sector;
	int32 sectors_per_cluster;
	int32 bytes_per_cluster;
	int32 reserved_sectors;
	int32 fat_bits;
	int32 fat_count;
	int32 sectors_per_FAT;
	int32 total_sectors;
	int32 active_FAT;
	int32 data_start;
	int32 total_clusters;
	int32 root_cluster;
	int32 root_entries;
	int32 root_start;

	#define FAT_CACHE_SIZE 0x10000
	struct {
		int32 sector;
		uchar FAT[FAT_CACHE_SIZE];
	} fat_cache;
};

#define read32(bsfer,off) \
	(((uchar *)bsfer)[(off)] + (((uchar *)bsfer)[(off)+1] << 8) + \
	 (((uchar *)bsfer)[(off)+2] << 16) + (((uchar *)bsfer)[(off)+3] << 24))

#define read16(bsfer,off) \
	(((uchar *)bsfer)[(off)] + (((uchar *)bsfer)[(off)+1] << 8))

#define ROOT_CLUSTER  0
#define END_FAT_ENTRY 0x0ffffff8
#define BAD_FAT_ENTRY 0x0ffffff1

static status_t
bt_dos_mount(nspace_id nsid, const char *device, 
		uint32 flags, void *parms, int len, void **ns, vnode_id *root)
{
	int fd;
	struct minidos_vcookie *cookie;
	status_t err;
	uchar bs[512];

	fd = open(device, 0);
	if (fd < 0) {
		dprintf("bt_dos_mount: can't open %s\n", device);
		return fd;
	}

	cookie = (struct minidos_vcookie *)malloc(sizeof(*cookie));
	cookie->fd = fd;

	if ((err = read_pos(fd, 0, bs, 512)) != 512)
		goto err1;

	if (	((bs[0x1fe] != 0x55) || (bs[0x1ff] != 0xaa)) &&
			(bs[0x15] == 0xf8)) {
//		dprintf("missing boot signature\n");
		goto err1;
	}

	if (!memcmp(bs+3, "NTFS    ", 8) || !memcmp(bs+3, "HPFS    ", 8)) {
//		dprintf("not a FAT volume\n");
		goto err1;
	}

	cookie->bytes_per_sector = read16(bs,0xb);
	if (	(cookie->bytes_per_sector != 0x200) && (cookie->bytes_per_sector != 0x400) &&
			(cookie->bytes_per_sector != 0x800)) {
//		dprintf("unsupported sector size (%lx)\n", cookie->bytes_per_sector);
		goto err1;
	}

	cookie->sectors_per_cluster = bs[0xd];
	switch (cookie->sectors_per_cluster) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 0x10:
		case 0x20:
		case 0x40:
		case 0x80:
			break;
		default:
//			dprintf("unsupported sectors/cluster (%lx)\n", cookie->sectors_per_cluster);
			goto err1;
	}

	cookie->reserved_sectors = read16(bs, 0xe);
	cookie->fat_count = bs[0x10];

	if ((cookie->fat_count == 0) || (cookie->fat_count > 8)) {
//		dprintf("unreasonable FAT count (%lx)\n", cookie->fat_count);
		goto err1;
	}

	if (bs[0x15] != 0xf8) {
//		dprintf("unsupported media descriptor byte (%x)\n", bs[0x15]);
		goto err1;
	}

	cookie->sectors_per_FAT = read16(bs, 0x16);
	if (cookie->sectors_per_FAT == 0) {
		cookie->fat_bits = 32;
		cookie->sectors_per_FAT = read32(bs,0x24);
		cookie->total_sectors = read32(bs,0x20);
		cookie->active_FAT = (bs[0x28] & 0x80) ? 0 : (bs[0x28] & 0xf);
		cookie->data_start = cookie->reserved_sectors + cookie->fat_count * cookie->sectors_per_FAT;
		cookie->total_clusters = (cookie->total_sectors - cookie->data_start) / cookie->sectors_per_cluster;
		cookie->root_cluster = read32(bs,0x2c);
		if (cookie->root_cluster >= cookie->total_clusters) {
			dprintf("root cluster too large (%lx > %lx)\n", cookie->root_cluster, cookie->total_clusters);
			goto err1;
		}
		cookie->root_entries = 0;
	} else {
		if (cookie->fat_count != 2) {
			dprintf("illegal FAT count (%lx)\n", cookie->fat_count);
			goto err1;
		}

		cookie->root_cluster = 0;
		cookie->root_entries = read16(bs,0x11);
		if (cookie->root_entries % (cookie->bytes_per_sector / 0x20)) {
			dprintf("illegal number of root entries (%lx)\n", cookie->root_entries);
			goto err1;
		}
		cookie->total_sectors = read16(bs,0x13);
		if (cookie->total_sectors == 0)
			cookie->total_sectors = read32(bs,0x20);
		cookie->root_start = cookie->reserved_sectors + cookie->fat_count * cookie->sectors_per_FAT;
		cookie->data_start = cookie->root_start + cookie->root_entries * 0x20 / cookie->bytes_per_sector;
		cookie->total_clusters = (cookie->total_sectors - cookie->data_start) / cookie->sectors_per_cluster;

		if (cookie->total_clusters < 0xff2) {
			dprintf("smell FAT12, not supported\n");
			goto err1;
		}
		cookie->fat_bits = 16;
	}

	cookie->bytes_per_cluster = cookie->sectors_per_cluster * cookie->bytes_per_sector;
	cookie->fat_cache.sector = -1;

	// XXX: check partition large enough
/*
	dprintf("bytes_per_sector %lx\nsectors_per_cluster %lx\n"
		"reserved_sectors %lx\nfat_bits %lx\nfat_count %lx\n"
		"sectors_per_FAT %lx\ntotal_sectors %lx\nactive_FAT %lx\n"
		"data_start %lx\ntotal_clusters %lx\nroot_cluster %lx\n"
		"root_entries %lx\nroot_start %lx\n",
		cookie->bytes_per_sector, cookie->sectors_per_cluster,
		cookie->reserved_sectors, cookie->fat_bits, cookie->fat_count,
		cookie->sectors_per_FAT, cookie->total_sectors, cookie->active_FAT,
		cookie->data_start, cookie->total_clusters, cookie->root_cluster,
		cookie->root_entries, cookie->root_start);
*/
	*ns = (void *)cookie;
	*root = 0LL;

	return B_OK;

err1:
	free(cookie);
	close(fd);
	if (err >= 0)
		err = B_ERROR;
	return err;
}

static status_t
bt_dos_unmount(void *ns)
{
	struct minidos_vcookie *cookie = (struct minidos_vcookie *)ns;

	close(cookie->fd);
	free(cookie);

	return B_OK;
}

static int32
get_fat_entry_for(struct minidos_vcookie *v, int32 cluster)
{
	int32 sector, offset;
	int32 result;

	if (cluster < 2)
		return B_ERROR;

	if (cluster >= v->total_clusters + 2)
		return B_ERROR;

	sector = cluster * (v->fat_bits / 8) / FAT_CACHE_SIZE;
	offset = (cluster * (v->fat_bits / 8)) % FAT_CACHE_SIZE;

	if (v->fat_cache.sector != sector) {
		if (read_pos(v->fd, v->reserved_sectors * v->bytes_per_sector +
				sector * FAT_CACHE_SIZE, v->fat_cache.FAT, FAT_CACHE_SIZE) < FAT_CACHE_SIZE) {
			dprintf("Error reading FAT\n");
			return -1;
		}
		v->fat_cache.sector = sector;
	}

	if (v->fat_bits == 16) {
		result = read16(v->fat_cache.FAT,offset);
		if (result > 0xfff0)
			result |= 0x0fff0000;
	} else {
		result = read32(v->fat_cache.FAT,offset) & 0x0fffffff;
	}

	if (result > 0x0ffffff7)
		return END_FAT_ENTRY;

	if (result > 0x0ffffff0)
		return BAD_FAT_ENTRY;

	return result;
}

/* vnode ids:
	0 = root
	otherwise = disk offset of directory entry
*/

struct minidos_node
{
	vnode_id vnid;
	uchar dentry[0x20];
	bool initialized;
	uint32 fsize;
	struct fmap_info *fmap;
};

static status_t
_append_to_fmap(struct minidos_vcookie *v, struct minidos_node *node,
		uint64 offset, uint32 block, uint32 num_blocks)
{
	struct fmap_block_run *run;
	
	#define QUANTUM 32
	
	if (!node->fmap) {
		node->fmap = malloc(sizeof(*(node->fmap)) + QUANTUM * sizeof(struct fmap_block_run));
		if (!(node->fmap))
			return ENOMEM;
		node->fmap->size = sizeof(*(node->fmap));
		node->fmap->checksum = 0;
		node->fmap->type = FMAP_TYPE_BLOCK;
		node->fmap->bios_id = 0xff;
		node->fmap->offset = offset;
		node->fmap->u.block.block_size = 0x200;
		node->fmap->u.block.num_blocks = num_blocks;
		node->fmap->u.block.num_block_runs = 1;
		node->fmap->u.block.block_runs[0].block = block;
		node->fmap->u.block.block_runs[0].num_blocks = num_blocks;

		return B_OK;
	}

	node->fmap->u.block.num_blocks += num_blocks;

	run = node->fmap->u.block.block_runs + node->fmap->u.block.num_block_runs - 1;

	if (block == run->block + run->num_blocks) {
		run->num_blocks += num_blocks;
		return B_OK;
	}

	if ((node->fmap->u.block.num_block_runs % QUANTUM) == 0) {
		struct fmap_info *f;
		uint32 size;
		
		size = sizeof(*(node->fmap)) + (node->fmap->u.block.num_block_runs + QUANTUM) * sizeof(struct fmap_block_run);

		f = malloc(size);
		if (!f)
			return ENOMEM;
		memcpy(f, node->fmap, size - QUANTUM * sizeof(struct fmap_block_run));
		free(node->fmap);
		node->fmap = f;
	}
	
	node->fmap->size += sizeof(struct fmap_block_run);
	node->fmap->u.block.num_block_runs++;

	run = node->fmap->u.block.block_runs + node->fmap->u.block.num_block_runs - 1;

	run->block = block;
	run->num_blocks = num_blocks;

	return B_OK;
}

static status_t
append_to_fmap(struct minidos_vcookie *v, struct minidos_node *node,
		uint32 cluster)
{
	return _append_to_fmap(v, node, v->data_start * v->bytes_per_sector,
			(cluster - 2) * (v->bytes_per_cluster / 0x200),
			v->bytes_per_cluster / 0x200);
}

static status_t
initialize_node(struct minidos_vcookie *v, struct minidos_node *node)
{
	status_t err = EINVAL;
	bool is_dir;
	int32 cluster, acluster, num_clusters, expected_remaining_clusters;

	if (node->initialized)
		return B_OK;

	node->initialized = TRUE;
	node->fsize = 0;

	if ((node->vnid == 0) && (v->fat_bits != 32)) {
		node->fsize = v->root_entries * 0x20;
		return _append_to_fmap(v, node, v->root_start * v->bytes_per_sector,
				0, v->root_entries * 0x20 / 0x200);
	}

	if (node->vnid == 0) {
		is_dir = TRUE;
		expected_remaining_clusters = 0; /* quiet warning */
		cluster = v->root_cluster;
	} else {
		is_dir = (node->dentry[0x0b] & 0x10) ? TRUE : FALSE;
		if (!is_dir)
			node->fsize = read32(node->dentry,0x1c);
		expected_remaining_clusters =
				(read32(node->dentry,0x1c) + v->bytes_per_cluster - 1) /
					v->bytes_per_cluster;
		cluster = read16(node->dentry,0x1a);
		if (v->fat_bits == 32)
			cluster += 0x10000 * read16(node->dentry,0x14);
	}

	if (!is_dir && (expected_remaining_clusters == 0)) {
		if (cluster) {
			dprintf("expected_clusters = 0, cluster = %lx\n", cluster);
			goto err1;
		}
		return B_OK;
	}

	if (!is_dir)
		dprintf("reading block bitmap (this can take a long time)");

	num_clusters = 0;
	while (1)
	{
		if (is_dir)
			node->fsize += v->bytes_per_cluster;

		acluster = get_fat_entry_for(v, cluster);
		if (	(acluster != END_FAT_ENTRY) &&
				!((acluster >= 2) && (acluster < v->total_clusters + 2))) {
			dprintf("\nbad fat entry\n");
			goto err1;
		}

		if (((num_clusters++ & 255) == 0) && !is_dir)
			dprintf(".");

		err = append_to_fmap(v, node, cluster);
		if (err < 0) {
			dprintf("\nerror appending to fmap\n");
			goto err1;
		}

		cluster = acluster;

		if (is_dir) {
			if (acluster == END_FAT_ENTRY)
				break;
			continue;
		}

		expected_remaining_clusters--;
		if ((acluster == END_FAT_ENTRY) || (expected_remaining_clusters == 0)) {
			if ((acluster != END_FAT_ENTRY) || expected_remaining_clusters) {
				dprintf("\nFAT chain length and file size mismatch\n");
				goto err1;
			}
			break;
		}
	}

	if (!is_dir)
		dprintf("\ndone!\n");

	return B_OK;
	
err1:
	if (node->fmap)
		free(node->fmap);
	node->fmap = NULL;
	return err;
}

static status_t
bt_dos_open(void *ns, void *node, void **cookie)
{
	return B_OK;
}

static status_t
bt_dos_close(void *ns, void *node, void *cookie)
{
	return B_OK;
}

static status_t
bt_dos_fstat(void *ns, void *_node, void *cookie, struct stat *st)
{
	struct minidos_node *node = (struct minidos_node *)_node;

	st->st_ino = node->vnid;
	if (node->dentry[0x0b] & 0x10) {
		st->st_mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH;
	} else {
		st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
	}
	st->st_size = node->fsize;
	st->st_mtime = 0; //read32(node->dentry,0x16); /* XXX - convert to beos time */

	return B_OK;
}

static ssize_t
bt_dos_freadlink(void *ns, void *node, void *cookie, char *buf, size_t len)
{
	return B_ERROR;
}

static ssize_t
bt_dos_read(void *ns, void *_node, void *cookie, off_t pos, void *buf, size_t size)
{
	struct minidos_vcookie *v = (struct minidos_vcookie *)ns;
	struct minidos_node *node = (struct minidos_node *)_node;
	struct fmap_block_run *run;
	status_t error;
	size_t amt = 0;
	off_t cpos;

	error = initialize_node(v, node);
	if (error < 0)
		return error;

	if (pos > node->fsize)
		return 0;

	if (pos + size > node->fsize)
		size = node->fsize - pos;

	if (!size)
		return 0;

	run = node->fmap->u.block.block_runs + 0;
	cpos = 0;
	while (cpos + run->num_blocks * 0x200LL < pos) {
		cpos += run->num_blocks * 0x200LL;
		run++;
	}

	while (amt < size) {
		size_t asize = size - amt;
		if (asize > run->num_blocks * 0x200LL - (pos - cpos))
			asize = run->num_blocks * 0x200LL - (pos - cpos);
		error = read_pos(v->fd, node->fmap->offset +
				run->block * 0x200LL + (pos - cpos), buf, asize);
		if (error < asize)
			return amt;
		cpos = pos;
		amt += asize;
		buf = (void *)((uchar *)buf + asize);
		run++;
	}
	
	return amt;
}

static status_t
bt_dos_ioctl(void *ns, void *_node, void *cookie, uint32 op, void *buf, uint32 len)
{
	struct minidos_vcookie *v = (struct minidos_vcookie *)ns;
	struct minidos_node *node = (struct minidos_node *)_node;

	if (op == 'fmap') {
		status_t error;
		struct fmap_info *vd;

		error = initialize_node(v, node);
		if (error < 0)
			return error;

		if (node->fmap == NULL) {
			vd = malloc(sizeof(*vd));
			vd->size = sizeof(*vd) - sizeof(struct fmap_byte_run);
			vd->checksum = 0;
			vd->type = FMAP_TYPE_BYTE;
			vd->bios_id = 0xff;
			vd->offset = 0;
			vd->u.byte.num_bytes = 0;
			vd->u.byte.num_byte_runs = 0;
			*(struct fmap_info **)buf = vd;
			return B_OK;
		}

#if 0
		/* byte form of fmap */
{
		int32 i;
		vd = malloc(sizeof(*vd) + node->fmap->u.block.num_block_runs * sizeof(struct fmap_byte_run));
		if (!vd)
			return ENOMEM;
		vd->size = sizeof(*vd) + (node->fmap->u.block.num_block_runs - 1) * sizeof(struct fmap_byte_run);
		vd->checksum = 0;
		vd->type = FMAP_TYPE_BYTE;
		vd->bios_id = 0xff;
		vd->offset = 0;
		vd->u.byte.num_bytes = node->fsize;
		vd->u.byte.num_byte_runs = node->fmap->u.block.num_block_runs;
		for (i=0;i<vd->u.byte.num_byte_runs;i++) {
			vd->u.byte.byte_runs[i].byte =
					node->fmap->offset +
					node->fmap->u.block.block_runs[i].block * 0x200LL;
			if ((i == vd->u.byte.num_byte_runs - 1) &&
					(node->fsize % v->bytes_per_cluster))
				vd->u.byte.byte_runs[i].num_bytes =
						node->fmap->u.block.block_runs[i].num_blocks * 0x200LL -
						v->bytes_per_cluster +
						(node->fsize % v->bytes_per_cluster);
			else
				vd->u.byte.byte_runs[i].num_bytes =
						node->fmap->u.block.block_runs[i].num_blocks * 0x200LL;
		}
}
#else
		/* block form of fmap */

		vd = malloc(node->fmap->size);
		if (!vd)
			return ENOMEM;
		memcpy(vd, node->fmap, node->fmap->size);

		/* round down to the nearest 512 byte sector */
		vd->u.block.num_blocks = node->fsize / 0x200;
		if (node->fsize % v->bytes_per_cluster) {
			uint32 delta;

			delta = (v->bytes_per_cluster - ((node->fsize % v->bytes_per_cluster)) + 0x1ff) / 0x200;
			if (vd->u.block.block_runs[vd->u.block.num_block_runs - 1].num_blocks < delta)
				panic("bad fmap rounding!\n");
			vd->u.block.block_runs[vd->u.block.num_block_runs - 1].num_blocks -= delta;
			if (vd->u.block.block_runs[vd->u.block.num_block_runs - 1].num_blocks == 0) {
				vd->u.block.num_block_runs--;
				vd->size -= sizeof(struct fmap_block_run);
			}
		}
#endif
		*(struct fmap_info **)buf = vd;
		return B_OK;
	}

	return ENOSYS;
}

struct minidos_dcookie {
	uint32 index;

	struct {
		int32 offset;
		uchar data[512];
	} cache;
};

static void
filename_from_dirent(char *fname, const uchar entry[0x20])
{
	int32 i;

	for (i=0;i<11;i++) {
		if (entry[i] == ' ') {
			if (i < 8) {
				i = 7;
				continue;
			}
			break;
		}
		if (i == 8)
			*(fname++) = '.';
		*(fname++) = entry[i];
	}
	*fname = 0;
}

static status_t
bt_dos_opendir(void *ns, void *_node, void **cookie)
{
	struct minidos_dcookie *d;
	struct minidos_node *node = (struct minidos_node *)_node;

	/* not a directory */
	if (node->vnid && (node->dentry[0x0b] & 0x10) == 0)
		return ENOTDIR;

	d = malloc(sizeof(*d));
	if (!d)
		return ENOMEM;
	d->index = 0;
	d->cache.offset = -1;

	*cookie = d;

	return B_OK;
}

static status_t
bt_dos_closedir(void *ns, void *node, void *cookie)
{
	free(cookie);

	return B_OK;
}

static status_t
_readdir_(void *ns, void *node, void *cookie, uchar entry[0x20])
{
	struct minidos_dcookie *d = (struct minidos_dcookie *)cookie;
	int32 offset;
	uchar *p;

retry:
	offset = (d->index * 0x20) & ~0x1ff;

	if (offset != d->cache.offset) {
		if (bt_dos_read(ns, node, NULL, offset, d->cache.data, 0x200) < 0x200)
			return ENOENT;
		d->cache.offset = offset;
	}

	offset = (d->index * 0x20) & 0x1ff;
	p = d->cache.data + offset;

	d->index++;

	/* skip erased entries, long file names, and volume labels */
	if ((p[0] == 0xe5) || (p[0x0b] & 8))
		goto retry;

	if (p[0] == 0) {
		d->index = 0x10000000;
		return ENOENT;
	}

	memcpy(entry, p, 0x20);

	return B_OK;
}

static status_t
bt_dos_readdir(void *ns, void *node, void *cookie, struct dirent *de)
{
	uchar entry[0x20];
	status_t err;

	err = _readdir_(ns, node, cookie, entry);
	if (err < 0)
		return err;

	de->d_ino = 0;
	filename_from_dirent(de->d_name, entry);

	return B_OK;
}

static status_t
bt_dos_rewinddir(void *ns, void *node, void *cookie)
{
	struct minidos_dcookie *d = (struct minidos_dcookie *)cookie;

	d->index = 0;

	return B_OK;
}

static status_t
bt_dos_walk(void *ns, void *dir, const char *file,
		char *newpath, vnode_id *vnid)
{
	status_t error;
	void *cookie;
	uchar entry[0x20];
	char fname[14];

	error = bt_dos_opendir(ns, dir, &cookie);
	if (error < 0)
		return error;

	error = ENOENT;

	while (_readdir_(ns, dir, cookie, entry) == B_OK) {
		filename_from_dirent(fname, entry);
		if (!strcmp(file, fname)) {
			struct minidos_node *node = (struct minidos_node *)dir;
			struct minidos_dcookie *d = (struct minidos_dcookie *)cookie;
			struct fmap_block_run *run = node->fmap->u.block.block_runs + 0;
			int32 offset, pos = 0;

			offset = (d->index - 1) * 0x20;
			while (pos + run->num_blocks * 0x200LL < offset) {
				pos += run->num_blocks * 0x200LL;
				run++;
			}
			*vnid = node->fmap->offset + run->block * 0x200LL +
					(offset - pos);
			error = B_OK;
			break;
		}
	}

	bt_dos_closedir(ns, dir, cookie);
	
	return error;
}

static status_t
bt_dos_read_vnode(void *ns, vnode_id vnid, void **_node)
{
	struct minidos_vcookie *v = (struct minidos_vcookie *)ns;
	struct minidos_node *node;
	uchar buffer[0x20];
	status_t error;

	if (vnid) {
		error = read_pos(v->fd, vnid, buffer, 0x20);
		if (error < 0x20)
			return (error >= 0) ? B_ERROR : error;
	}

	node = malloc(sizeof(*node));
	if (!node)
		return ENOENT;
	node->vnid = vnid;
	memcpy(node->dentry, buffer, 0x20);

	node->initialized = FALSE;
	node->fsize = 0;
	node->fmap = NULL;

	*_node = node;

	return B_OK;
}

static status_t
bt_dos_write_vnode(void *ns, void *_node)
{
	struct minidos_node *node = (struct minidos_node *)_node;
	if (node->fmap)
		free(node->fmap);
	free(node);
	return B_OK;
}

struct fs_ops bt_dos_ops = {
	bt_dos_mount,
	bt_dos_unmount,

	bt_dos_walk,
	bt_dos_read_vnode,
	bt_dos_write_vnode,

	bt_dos_open,
	bt_dos_close,
	bt_dos_read,
	bt_dos_ioctl,
	bt_dos_fstat,
	bt_dos_freadlink,

	bt_dos_opendir,
	bt_dos_closedir,
	bt_dos_readdir,
	bt_dos_rewinddir
};
