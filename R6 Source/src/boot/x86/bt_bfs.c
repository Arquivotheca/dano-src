#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <drivers/KernelExport.h>

#include <bfs.h>

#include <boot.h>

#include "bt_fs.h"

struct minibfs_vcookie {
	uint32 fd;
	struct disk_super_block sb;
	uint32 spb; /* sectors/block */
	uint32 logblocks;
	void *log;
};

struct minibfs_node {
	uint64 bnum; /* aka vnode_id */
	bfs_inode *inode;
};

#define NEXT_SD(sd)                                          \
	(small_data *)((char *)sd + sizeof(*sd) +           \
		sd->name_size + sd->data_size)

#define SD_DATA(sd)                                          \
	(void *)((char *)sd + sizeof(*sd) + (sd->name_size-sizeof(sd->name)))

typedef struct log_entry {
	int       num_entries;          /* # of entries in the blocks array */
	int       max_entries;          /* max # of entries we have space for */
	block_run blocks[1];            /* an array of block_runs */
} log_entry;

static uint64
inode_addr_to_bnum(const struct minibfs_vcookie *cookie, inode_addr addr)
{
	return ((uint64)addr.allocation_group << cookie->sb.ag_shift) + addr.start;
}

static status_t
verify_log(struct minibfs_vcookie *cookie, disk_super_block *sb)
{
	uchar *p, *endp;
	uint32 max_entries = 0;

	p = cookie->log;
	endp = p + cookie->logblocks * 0x200;
	while (p < endp) {
		int i;
		uchar *q;
		log_entry *le = (log_entry *)p;
		
		if ((!max_entries && !(max_entries = le->max_entries)) ||
			(max_entries && (max_entries != le->max_entries))) {
			dprintf("Bad max_entries value (%x) in log\n", le->max_entries);
			return -1;
		}

		q = p + 0x200 * cookie->spb;
		for (i=0;i<le->num_entries;i++) {
			uint64 bnum = inode_addr_to_bnum(cookie, le->blocks[i]);

			if ((bnum >= sb->num_blocks) || (le->blocks[i].len != 1)) {
				dprintf("Bad block run value in log (%Lx %x)\n",
						bnum, le->blocks[i].len);
				return -1;
			}
			q += 0x200 * cookie->spb;
		}
		p = q;
	}

	return 0;
}

static status_t
_read_blocks_(const struct minibfs_vcookie *cookie,
		uint64 block, void *buffer, uint32 num)
{
	status_t res;
	uint64 sector;
	uchar *p, *endp;

	sector = block * cookie->spb;
	res = read_pos(cookie->fd, sector * 512, buffer, num * cookie->spb * 512);
	if (res != num * cookie->spb * 512) {
		dprintf("error reading block %Lx (sector %Lx) (count %lx)\n",
				block, sector, num);
	
		return (res <= 0) ? res : -1;
	}
	
	if (!cookie->logblocks) return 0;

	/* check if any of the blocks are in the log */
	p = cookie->log;
	endp = p + cookie->logblocks * 0x200;
	while (p < endp) {
		int i;
		uchar *q;
		log_entry *le = (log_entry *)p;

		q = p + 0x200 * cookie->spb;

		for (i=0;i<le->num_entries;i++) {
			uint64 bnum = inode_addr_to_bnum(cookie, le->blocks[i]);

			if ((bnum >= block) && (bnum < block + num)) {
				dprintf("Replaying block %Lx\n", bnum);
				memcpy((uchar *)buffer + (bnum - block) * 0x200 * cookie->spb,
						q, 0x200 * cookie->spb);
			}
			
			q += 0x200 * cookie->spb;
		}
		
		p = q;
	}

	return 0;
}

static status_t
read_inode(const struct minibfs_vcookie *cookie, uint64 bnum, bfs_inode *bi)
{
	status_t status;
	status = _read_blocks_(cookie, bnum, bi, 1);
	if ((status == B_OK) && (bi->magic1 != INODE_MAGIC1)) {
		dprintf("inode has invalid magic number\n");
		return B_ERROR;
	}
	if (status) dprintf("error reading inode %Lx\n", bnum);
	return status;
}

static status_t
read_block_run(const struct minibfs_vcookie *cookie,
		block_run addr, void **buffer, uint32 *maxblocks)
{
	uint64 cur = inode_addr_to_bnum(cookie, addr);
	uint32 num;
	status_t error;

	num = addr.len;
	if (maxblocks && (*maxblocks < addr.len))
		num = *maxblocks;
	error = _read_blocks_(cookie, cur, *buffer, num);
	if (error) return error;
	if (maxblocks) *maxblocks -= num;
	(uchar *)(*buffer) += 512 * num * cookie->spb;

	return B_OK;
}

static status_t
read_file1(const struct minibfs_vcookie *cookie,
		inode_addr *addrs, uint32 num_addrs, void **buffer,
		uint32 *maxblocks, int level)
{
	int i;
	
	for (i=0;i<num_addrs;i++) {
		status_t result;
		if (addrs[i].len == 0) return B_OK;
		if (level == 0) {
			result = read_block_run(cookie, addrs[i], buffer, maxblocks);
		} else {
			inode_addr *buff, *p;
if (level == 2) {
	dprintf("DOUBLE INDIRECT BLOCK ENCOUNTERED\n");
}
			p = buff = malloc(addrs[i].len * 0x200 * cookie->spb);
			result = read_block_run(cookie, addrs[i], (void **)&p, NULL);
			if (result) return result;
			result = read_file1(cookie, buff,
					addrs[i].len * 0x200 * cookie->spb / sizeof(inode_addr),
					buffer, maxblocks, level - 1);
			free(buff);
		}
		if (result) return result;
	}
	
	return B_OK;
}

/* read a file to the buffer */
static status_t
minibfs_read_file(struct minibfs_vcookie *cookie,
		bfs_inode *inode, void *buffer, uint32 maxblocks)
{
	status_t result;
	void *orig = buffer;
	result = read_file1(cookie, inode->data.direct + 0, NUM_DIRECT_BLOCKS, &buffer, &maxblocks, 0);
	if (result) return result;
	result = read_file1(cookie, &(inode->data.indirect), 1, &buffer, &maxblocks, 1);
	if (result) return result;
	result = read_file1(cookie, &(inode->data.double_indirect), 1, &buffer, &maxblocks, 2);
	if (result == 0)
		return (uint32)buffer - (uint32)orig;
	else
		return result;
}

static status_t
bt_bfs_mount(nspace_id nsid, const char *device, 
		uint32 flags, void *parms, int len, void **ns, vnode_id *root)
{
	struct minibfs_vcookie *cookie;
	uchar buffer[16384], *bitmap;
	disk_super_block *sb = (disk_super_block *)(buffer + 512);
	int fd, num_bitmap_blocks, bsize, n, i;

	uint32 logblocks;
	void *log;

	fd = open(device, 0);
	if (fd < 0) {
		dprintf("bt_bfs_mount: can't open %s\n", device);
		return fd;
	}

	if (read_pos(fd, 0, buffer, 16384) < 16384)
		goto err1;

	/* check taken from drive setup add-on */
	if (	(sb->magic1 != SUPER_BLOCK_MAGIC1) ||
			(sb->magic2 != SUPER_BLOCK_MAGIC2) ||
			(sb->magic3 != SUPER_BLOCK_MAGIC3) ||
			(sb->fs_byte_order != BFS_BIG_ENDIAN))
		goto err1;

	bsize = sb->block_size;
	bitmap = buffer + bsize;

	num_bitmap_blocks  = sb->num_blocks / 8;
	num_bitmap_blocks  = ((num_bitmap_blocks + bsize - 1) & ~(bsize - 1));
	num_bitmap_blocks /= bsize;

	n = 1 + num_bitmap_blocks;
	if (n > (16384 - bsize - 512))
		n = 16384 - bsize - 512;

	for(i=0; i < n; i++)
		if ((bitmap[i/8] & (1 << (i%8))) == 0)
			goto err1;

	cookie = (struct minibfs_vcookie *)malloc(sizeof(*cookie));
	if (cookie == NULL)
		goto err1;

	cookie->fd = fd;

	/* initially disable log check */
	cookie->logblocks = 0;
	cookie->log = NULL;

	memcpy(&(cookie->sb), sb, sizeof(cookie->sb));
	cookie->spb = cookie->sb.block_size / 512;

	if (sb->log_start != sb->log_end) {
		dprintf("log_blocks: %Lx %x log_start: %Lx log_end %Lx\n",
				inode_addr_to_bnum(cookie, sb->log_blocks), sb->log_blocks.len,
				sb->log_start, sb->log_end);

		if (sb->log_start < sb->log_end) {
			logblocks = sb->log_end - sb->log_start;
			log = malloc(logblocks * 0x200 * cookie->spb);
			if (_read_blocks_(cookie,
					inode_addr_to_bnum(cookie, sb->log_blocks) + sb->log_start,
					log, logblocks) < 0) {
				dprintf("Error reading log\n");
				goto err2;
			}
		} else {
			logblocks = sb->log_blocks.len - sb->log_start + sb->log_end;
			log = malloc(logblocks * 0x200 * cookie->spb);
			if (_read_blocks_(cookie,
					inode_addr_to_bnum(cookie, sb->log_blocks) + sb->log_start,
					log, sb->log_blocks.len - sb->log_start) < 0) {
				dprintf("Error reading log\n");
				goto err2;
			}
			if (_read_blocks_(cookie,
					inode_addr_to_bnum(cookie, sb->log_blocks),
					log + (sb->log_blocks.len - sb->log_start) * 0x200 * cookie->spb, sb->log_end) < 0) {
				dprintf("Error reading log\n");
				goto err2;
			}
		}

		/* start using the log */
		cookie->logblocks = logblocks;
		cookie->log = log;

		if (verify_log(cookie, sb) != B_OK)
			goto err2;
	}

	*ns = (void *)cookie;
	*root = inode_addr_to_bnum(cookie, cookie->sb.root_dir);

	return 0;

err2:
	free(log);
	free(cookie);
err1:
	close(fd);
	return -1;
}

static status_t
bt_bfs_unmount(void *ns)
{
	struct minibfs_vcookie *cookie = (struct minibfs_vcookie *)ns;

	if (cookie->log) free(cookie->log);
	close(cookie->fd);
	free(cookie);

	return 0;
}

/* vyt: make it waste less memory */
static status_t
bt_bfs_walk(void *_ns, void *_dir, const char *name, char *newpath, vnode_id *bnum)
{
	struct minibfs_vcookie *cookie = (struct minibfs_vcookie *)_ns;
	struct minibfs_node *dir = (struct minibfs_node *)_dir;
	bfs_inode *directory = dir->inode;

	uchar *buffer;
	uint32 size;
	uint32 namelen;
	status_t error;
	int i;

	size = directory->data.size;
	if (!directory->data.size || (directory->data.size & 0x3ff)) {
		dprintf("directory inode has bad size field (%Lx)\n",
				directory->data.size);
		return B_ERROR;
	}
	size = (size + 512 * cookie->spb - 1) & ~(512 * cookie->spb - 1);
	buffer = (uchar *)malloc(size);

	error = minibfs_read_file(cookie, directory, buffer,
			size / (512 * cookie->spb));
	if (error < 0) goto err;

	namelen = strlen(name);

	for (i=0;i<directory->data.size;i += 1024) {
		char *p = buffer + i;
		uint16 entries, len, *offsets, j, cur;
		uint64 *bnums;

		if (*(uint64 *)(p + 0x10) != 0xffffffffffffffffLL) continue;

		entries = *(ushort *)(p + 0x18);
		len = *(ushort *)(p + 0x1a);
		cur = 0;
		offsets = (uint16 *)(p + ((len + 0x1c + 7) & ~7));
		for (j=0;j<entries;j++) {
			uint32 entrylen = *offsets - cur;
			if ((entrylen == namelen) &&
					(!strncmp(name, p + 0x1c + cur, entrylen)))
				break;
			cur = *(offsets++);
		}
		if (j == entries) continue;
		
		bnums = (uint64 *)((uint32)p + ((len + 0x1c + 7) & ~7) + 2*entries);
		*bnum = bnums[j];
		error = B_OK;
		goto err;
	}
	
	dprintf("unable to find %s\n", name);
	error = ENOENT;

err:
	free(buffer);

	return error;
}

static status_t
bt_bfs_read_vnode(void *_ns, vnode_id vnid, void **_node)
{
	struct minibfs_vcookie *ns = (struct minibfs_vcookie *)_ns;
	struct minibfs_node *node;
	void *inode;
	status_t error;

	inode = malloc(ns->spb * 0x200);
	if (!inode)
		return ENOMEM;

	error = read_inode(ns, vnid, inode);
	if (error < 0) {
		free(inode);
		return error;
	}
	
	node = malloc(sizeof(*node));
	if (!node) {
		free(inode);
		return ENOMEM;
	}

	node->bnum = vnid;
	node->inode = inode;

	*_node = (void *)node;

	return B_OK;
}

static status_t
bt_bfs_write_vnode(void *ns, void *_node)
{
	struct minibfs_node *node = (struct minibfs_node *)_node;
	free(node->inode);
	free(node);
	return B_OK;
}

struct fcookie {
	void	*data;
};

static status_t
bt_bfs_open(void *ns, void *node, void **_cookie)
{
	struct fcookie *cookie;

	cookie = malloc(sizeof(*cookie));
	if (!cookie)
		return ENOMEM;

	cookie->data = NULL;
	*_cookie = (void *)cookie;

	return 0;
}

static status_t
bt_bfs_close(void *ns, void *node, void *_cookie)
{
	struct fcookie *cookie = (struct fcookie *)_cookie;
	if (cookie->data)
		free(cookie->data);
	free(cookie);
	return 0;
}

static status_t
bt_bfs_fstat(void *ns, void *_node, void *cookie, struct stat *st)
{
	struct minibfs_node *node = (struct minibfs_node *)_node;

	st->st_ino = node->bnum;
	st->st_mode = node->inode->mode;
	st->st_size = node->inode->data.size;
	st->st_mtime = (time_t)(node->inode->last_modified_time >> TIME_SCALE);

	return B_OK;
}

static ssize_t
bt_bfs_freadlink(void *ns, void *_node, void *cookie, char *buf, size_t len)
{
	struct minibfs_node *node = (struct minibfs_node *)_node;

	if (!S_ISLNK(node->inode->mode)) return -1;
	
	strncpy(buf, (char *)&node->inode->data, len);

	/* vyt: should return # bytes copied */
	return 0;
}

static ssize_t
bt_bfs_read(void *_ns, void *_node, void *cookie, off_t pos, void *buf, size_t size)
{
	struct minibfs_vcookie *ns = (struct minibfs_vcookie *)_ns;
	struct minibfs_node *node = (struct minibfs_node *)_node;
	struct fcookie *f = (struct fcookie *)cookie;

	/* lazy read */
	if (!f->data) {
		uint32 len, bpb;
		status_t result;
		bpb = 512 * ns->spb;
		len = node->inode->data.size+bpb-1; /* err on the side of caution */
		f->data = malloc(len);
		if (!f->data) return ENOMEM;
		result = minibfs_read_file(ns, node->inode, f->data, len / bpb);
		if (result < 0) return result;
	}
	
	if (pos + size > node->inode->data.size)
		size = node->inode->data.size - pos;

	if (size < 0) return B_ERROR;

	memcpy(buf, (char *)f->data	+ pos, size);
	
	pos += size;

	return size;
}

static status_t
bt_bfs_ioctl(void *_ns, void *node, void *cookie,
		uint32 op, void *buf, uint32 len)
{
	struct minibfs_vcookie *ns = (struct minibfs_vcookie *)_ns;

	/* private ioctl to get volume name */
	if (op == 0x01020304) {
		strcpy(buf, ns->sb.name);
		return 0;
	}
	return ENOSYS;
}

/*
struct dirent
  {
    __dev_t d_dev;
    __dev_t d_pdev;
    __ino_t d_fileno;
    __ino_t d_pino;
    unsigned short int d_reclen;
    char d_name[1];             // Variable length.
  };
*/

struct minibfs_dcookie {
	uint32 bpb;
	uint32 len;
	uchar *data;
	uchar *p;							/* points to current 1k block */
	uint32 index;
};

static status_t
bt_bfs_opendir(void *_ns, void *_node, void **_cookie)
{
	struct minibfs_vcookie *ns = (struct minibfs_vcookie *)_ns;
	struct minibfs_node *node = (struct minibfs_node *)_node;
	struct minibfs_dcookie *cookie;
	status_t error;

	cookie = (struct minibfs_dcookie *)malloc(sizeof(*cookie));

	cookie->bpb = 512 * ns->spb;
	cookie->len = node->inode->data.size;
	if (cookie->len % 1024)
		panic("bad directory length (%lx)\n", cookie->len);
	cookie->data = malloc(cookie->len + cookie->bpb);
	error = minibfs_read_file(ns, node->inode, cookie->data, (cookie->len + cookie->bpb - 1) / cookie->bpb);
	if (error < 0) {
		free(cookie->data);
		free(cookie);
		return error;
	}
	cookie->p = cookie->data;
	cookie->index = 0;

	*_cookie = (void *)cookie;

	return 0;
}

static status_t
bt_bfs_closedir(void *ns, void *node, void *_cookie)
{
	struct minibfs_dcookie *cookie = (struct minibfs_dcookie *)_cookie;

	free(cookie->data);
	free(cookie);

	return B_OK;
}

static status_t
bt_bfs_readdir(void *ns, void *node, void *_cookie, struct dirent *d)
{
	struct minibfs_dcookie *cookie = (struct minibfs_dcookie *)_cookie;

	if (cookie->p - cookie->data >= cookie->len)
		return ENOENT;

	while ((*(uint64 *)(cookie->p + 0x10) != 0xffffffffffffffffLL) ||
				(cookie->index >= *(uint16 *)(cookie->p + 0x18))) {
		cookie->p += 0x400;
		cookie->index = 0;

		if (cookie->p - cookie->data >= cookie->len)
			return ENOENT;
	}

	{
		uchar *p = cookie->p;
		uint16 entries = *(uint16 *)(p + 0x18);
		uint16 len = *(uint16 *)(p + 0x1a);
		uint16 *offsets = (uint16 *)(p + ((len + 0x1c + 7) & ~7));
		uint64 *bnums = (uint64 *)((uchar *)offsets + 2*entries);
		uint32 cur = (cookie->index) ? offsets[cookie->index-1] : 0;
		uint32 entrylen = offsets[cookie->index] - cur;

		memcpy(d->d_name + 0, p + 0x1c + cur, entrylen);
		d->d_name[entrylen] = 0;
		d->d_ino = bnums[cookie->index];

		cookie->index++;
	}
	
	return 0;
}

static status_t
bt_bfs_rewinddir(void *ns, void *node, void *_cookie)
{
	struct minibfs_dcookie *cookie = (struct minibfs_dcookie *)_cookie;
	cookie->p = cookie->data;
	cookie->index = 0;
	return 0;
}

struct fs_ops bt_bfs_ops = {
	bt_bfs_mount,
	bt_bfs_unmount,

	bt_bfs_walk,
	bt_bfs_read_vnode,
	bt_bfs_write_vnode,
	
	bt_bfs_open,
	bt_bfs_close,
	bt_bfs_read,
	bt_bfs_ioctl,
	bt_bfs_fstat,
	bt_bfs_freadlink,
	
	bt_bfs_opendir,
	bt_bfs_closedir,
	bt_bfs_readdir,
	bt_bfs_rewinddir
};

#if 0
/* This is only used to read the "be:volume_id" attribute, which is guaranteed
 * to be in the small data area. If you want to be able to read arbitrary
 * attributes, you're going to have to add the ability to parse the attribute
 * directory */
status_t minibfs_read_attribute(struct minibfs_vcookie *cookie, bfs_inode *inode,
		const char *name, uint32 *type, uint32 *len, void *buffer)
{
	small_data *sd;
	int nlen = strlen(name);
	
	sd = (small_data *)&(inode->small_data);
	while ((sd->type) && (sd < (small_data *)(inode + cookie->sb.block_size))) {
		if ((sd->name_size == nlen) && !strncmp(name, sd->name, nlen)) {
			if (sd->data_size > *len)
				return E2BIG;
			*type = sd->type;
			*len = sd->data_size;
			memcpy(buffer, SD_DATA(sd), sd->data_size);
			return B_OK;
		}
		sd = NEXT_SD(sd);
	}
	
	return B_ERROR;
}

status_t
minibfs_get_volume_id(void *_cookie, uint64 *vid)
{
	bfs_inode *root_inode;
	struct minibfs_vcookie *cookie = (struct minibfs_vcookie *)_cookie;
	status_t result;
	uint32 type, len;
	
	root_inode = (bfs_inode *)malloc(512 * cookie->spb);
	result = read_root_inode(cookie, root_inode);
	if (result < 0) goto error;

	len = 8;
	result = minibfs_read_attribute(cookie, root_inode,
			"be:volume_id", &type, &len, vid);

	if ((len != 8) || (type != 0x554c4c47)) result = B_ERROR;

error:
	free(root_inode);

	return result;
}

#endif
