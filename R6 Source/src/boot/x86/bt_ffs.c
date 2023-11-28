/* floppy file system - a gzip'ped tar starting at sector 0x100 */

#include <dirent.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>

#include <drivers/KernelExport.h>

#include "bt_fs.h"
#include "inflate.h"

struct file {
	char *filename;
	uint32 size;
	unsigned char *data;	/* pointer into vffs buff */
	
	struct file *next;
};

struct vffs {
	int fd;
	unsigned char *buff;
	struct file *files;
};

static status_t
add_file(struct vffs *v, char *filename, uint32 size, unsigned char *data)
{
	struct file *f;
	
	for (f=v->files;f;f=f->next)
		if (!strcmp(filename, f->filename))
			return B_OK;

	f = malloc(sizeof(*f));
	if (!f)
		return ENOMEM;
	f->filename = malloc(strlen(filename) + 1);
	if (!(f->filename)) {
		free(f);
		return ENOMEM;
	}
	strcpy(f->filename, filename);
	f->size = size;
	f->data = data;
	f->next = v->files;
	v->files = f;
	return B_OK;
}

static status_t
add_path(struct vffs *v, char *filename, uint32 size, unsigned char *data)
{
	char *p, *q;
	p = filename;
	while ((q = strchr(p, '/')) != NULL) {
		*q = 0;
		if (add_file(v, filename, 0, NULL) < B_OK)
			return B_ERROR;
		*q = '/';
		p = q + 1;
	}
	return add_file(v, filename, size, data);
}

static status_t init_data(struct vffs *v)
{
	unsigned char *zbuf, *buff, *p;
	uint32 size;
	
	zbuf = malloc(768 * 1024);
	
	if (read_pos(v->fd, 0x100 * 512, zbuf, 768 * 1024) < 768 * 1024) {
		free(zbuf);
		return -1;
	}
	if (!is_gzip(zbuf)) {
		free(zbuf);
		return -1;
	}

	/* inflate */
	buff = malloc(3 * 1024 * 1024);
	size = gunzip(zbuf, buff);
	free(zbuf);
	zbuf = malloc(size);
	memcpy(zbuf, buff, size);
	free(buff);
	buff = zbuf;

	p = buff;
	/* read the tar directory */
	while (1) {
		unsigned char *pp;
		uint32 fsize;

		/* check tar signature */
		if (*(uint32 *)(p + 0x101) != 'atsu')
			break;

		fsize = 0;
		for (pp=p+0x7c;pp<p+0x87;pp++)
			if (*pp != 0x20)
				fsize = fsize * 8 + *pp - '0';

		if (p[strlen(p)-1] != '/') {
			dprintf("%s / %ld\n", p, fsize);

			if (add_path(v, p, fsize, p + 0x200) < B_OK)
				return B_ERROR;	/* XXX: real cleanup */
		}

		p += 0x200 * (1 + (fsize + 0x1ff) / 0x200);
	}

	v->buff = buff;
	
	return 0;
}

static status_t ffs_mount(nspace_id nsid, const char *device, 
		uint32 flags, void *parms, int len, void **ns, vnode_id *root)
{
	int fd;
	unsigned char buff[512];
	struct vffs *v;
	
	fd = open(device, 0);
	if (fd < 0) return -1;

	if ((read_pos(fd, 0x100 * 512, buff, 512) < 512) || !is_gzip(buff)) {
		close(fd);
		return -1;
	}

	v = malloc(sizeof(*v));
	if (!v) return ENOMEM;

	v->fd = fd;
	v->buff = NULL;
	v->files = NULL;

	if (add_file(v, "", 0, NULL) < B_OK)
		return B_ERROR;

	*ns = (void *)v;
	*root = (uint32)v->files;

	return 0;
}

static status_t ffs_unmount(void *ns)
{
	struct vffs *v = (struct vffs *)ns;
	struct file *f, *g;
	
	f = v->files;
	while (f) {
		g = f->next;
		free(f->filename);
		free(f);
		f = g;
	}
	if (v->buff) free(v->buff);
	close(v->fd);
	free(v);
	return 0;
}

status_t ffs_walk(void *ns, void *_dir, const char *file,
		char *newpath, vnode_id *vnid)
{
	struct vffs *v = (struct vffs *)ns;
	struct file *f, *dir = (struct file *)_dir;
	int length;

	if (!v->buff && (init_data(v) != B_OK))
		return -1;

	length = strlen(dir->filename);
	for (f=v->files;f;f=f->next) {
		if (	(	length &&
					!strncmp(f->filename, dir->filename, length) &&
					(f->filename[length] == '/') &&
					!strcmp(f->filename + length + 1, file)) ||
				(	!length &&
					!strcmp(f->filename, file))) {
			*vnid = (uint32)f;
			return B_OK;
		}
	}
	
	return ENOENT;
}

status_t ffs_read_vnode(void *ns, vnode_id vnid, void **node)
{
	*node = (void *)((uint32)vnid);
	return B_OK;
}

status_t ffs_write_vnode(void *ns, void *node)
{
	return B_OK;
}

static status_t ffs_open(void *ns, void *node, void **cookie)
{
	return B_OK;
}

static status_t ffs_close(void *ns, void *node, void *cookie)
{
	return B_OK;
}

static ssize_t ffs_read(void *ns, void *node, void *cookie,
		off_t pos, void *buffer, uint32 len)
{
	struct file *f = (struct file *)node;

	if (f->data == NULL)
		return EISDIR;

	if (pos > f->size) return 0;

	if (pos + len > f->size)
		len = f->size - pos;

	memcpy(buffer, f->data + pos, len);

	return len;
}

static status_t ffs_ioctl(void *ns, void *node, void *cookie,
					uint32 op, void *buffer, uint32 size)
{
	dprintf("ffs_ioctl %lx\n", op);

	return ENOSYS;
}

static status_t ffs_fstat(void *ns, void *node, void *cookie, struct stat *st)
{
	struct file *f = (struct file *)node;

	st->st_ino = (int)f; /* fake out load_elf.c */
	st->st_mode = f->data ? 0 : S_IFDIR;
	st->st_size = f->size;

	return 0;
}

static ssize_t ffs_freadlink(void *ns, void *node, void *cookie, char *buf, size_t len)
{
	return ENOSYS;
}

struct dffs {
	char name[B_FILE_NAME_LENGTH];
	int namelen;
	struct file *cur;
};

static status_t ffs_opendir(void *ns, void *node, void **cookie)
{
	struct vffs *v = (struct vffs *)ns;
	struct file *f = (struct file *)node;
	struct dffs *d;

	if (!v->buff && (init_data(v) != B_OK))
		return -1;

	d = malloc(sizeof(*d));
	if (!d) return ENOMEM;

	d->namelen = strlen(f->filename) + 1;
	strcpy(d->name, f->filename);
	d->name[d->namelen-1] = '/';
	d->name[d->namelen] = 0;
	d->cur = v->files;

	*cookie = (void *)d;
	
	return B_OK;
}

static status_t ffs_closedir(void *ns, void *node, void *cookie)
{
	free(cookie);
	return B_OK;
}

static status_t ffs_readdir(void *ns, void *node, void *cookie, struct dirent *de)
{
	struct dffs *d = (struct dffs *)cookie;

	while (d->cur) {
		if (!strncmp(d->name, d->cur->filename, d->namelen) &&
				d->cur->filename[d->namelen] &&
				(strchr(d->cur->filename + d->namelen, '/') == NULL)) {
			strcpy(de->d_name, d->cur->filename + d->namelen);
			d->cur = d->cur->next;
			return B_OK;
		}
		d->cur = d->cur->next;
	}

	return ENOENT;
}

static status_t ffs_rewinddir(void *ns, void *node, void *cookie)
{
	struct vffs *v = (struct vffs *)ns;
	struct dffs *d = (struct dffs *)cookie;
	d->cur = v->files;
	return 0;
}

struct fs_ops ffs_ops = {
	ffs_mount,
	ffs_unmount,

	ffs_walk,
	ffs_read_vnode,
	ffs_write_vnode,

	ffs_open,
	ffs_close,
	ffs_read,
	ffs_ioctl,
	ffs_fstat,
	ffs_freadlink,

	ffs_opendir,
	ffs_closedir,
	ffs_readdir,
	ffs_rewinddir
};
