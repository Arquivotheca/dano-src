#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

#include "bt_fs.h"
#include "bt_devfs.h"

extern struct bt_driver_hooks disk_driver_hooks, fmap_driver_hooks;

struct bt_driver_hooks *drivers[] = {
	&disk_driver_hooks,
	&fmap_driver_hooks,
	NULL
};

struct node {
	char name[16], path[16];
	bool scanned;
	struct bt_driver_hooks *h;

	int partition;
	off_t offset;
	off_t length;

	struct node *master, *next_sibling, *first_child;
};

static status_t
add_device(struct node *dir, const char *name, const char *path,
		struct bt_driver_hooks *h, int partition, off_t offset, off_t length)
{
	struct node *node, *n;
	char *p, dn[64];

	if ((p = strchr(name, '/')) == NULL) {
		node = calloc(sizeof(*node), 1);
		if (!node)
			return ENOMEM;

		strcpy(node->name, name);
		strcpy(node->path, path);
		node->scanned = TRUE;
		if (h) {
			node->master = node;
			node->h = h;
		} else {
			node->master = dir->first_child;
			node->h = dir->first_child->h;
		}
		node->partition = partition;
		node->offset = offset;
		node->length = length;
		node->next_sibling = node->first_child = NULL;

		if (dir->first_child) {
			for (n=dir->first_child;n->next_sibling;n=n->next_sibling)
				;
			n->next_sibling = node;
		} else {
			dir->first_child = node;
		}

		return B_OK;
	}

	memcpy(dn, name, p - name);
	dn[p-name] = 0;

	for (n=dir->first_child;n;n=n->next_sibling) {
		if (!strcmp(n->name, dn))
			return add_device(n, p + 1, path, h, partition, offset, length);
	}

	node = calloc(sizeof(*node), 1);
	if (!node)
		return ENOMEM;

	strcpy(node->name, dn);
	node->scanned = TRUE;

	if (!(dir->first_child)) {
		dir->first_child = node;
	} else {
		for (n=dir->first_child;n->next_sibling;n=n->next_sibling)
			;
		n->next_sibling = node;
	}

	return add_device(node, p + 1, path, h, partition, offset, length);
}

static status_t add_devices(struct node *root, struct bt_driver_hooks *h)
{
	int i;
	status_t error;
	const char **names;

	if ((error = (*h->init_driver)()) < B_OK)
		return error;

	names = (*h->publish_devices)();

	for (i=0;names && names[i];i++)
		if ((error = add_device(root, names[i], names[i], h, 0, 0, 0)) < B_OK)
			return error;

	return B_OK;
}

static void remove_devices(struct node *node)
{
	struct node *c, *n;

	for (c=node;c;c=n) {
		n = c->next_sibling;
		remove_devices(c->first_child);
		free(c);
	}
}

static status_t devfs_mount(nspace_id nsid, const char *device,
		uint32 flags, void *parms, int len, void **ns, vnode_id *root)
{
	struct node *nodes, *node, *n;
	int i;

	/* create root directory */
	nodes = calloc(sizeof(*nodes), 1);
	if (!nodes)
		return ENOMEM;
	nodes->scanned = TRUE;

	n = NULL;
	for (i=0;drivers[i];i++) {
		node = calloc(sizeof(*node), 1);
		strcpy(node->name, drivers[i]->name);
		node->scanned = FALSE;
		node->h = drivers[i];
		if (!n) {
			nodes->first_child = node;
		} else {
			n->next_sibling = node;
		}
		n = node;
	}

	*ns = nodes;
	*root = (uint32)nodes;

	return 0;
}

static status_t devfs_unmount(void *ns)
{
	int i;
	remove_devices((struct node *)ns);
	for (i=0;drivers[i];i++)
		;
	for (i=i-1;i>=0;i--)
		(*drivers[i]->uninit_driver)();
	return 0;
}

status_t devfs_walk(void *ns, void *_dir, const char *file,
		char *newpath, vnode_id *vnid)
{
	struct node *dir = (struct node *)_dir, *c;

	for (c=dir->first_child;c;c=c->next_sibling)
		if (!strcmp(file, c->name)) {
			*vnid = (uint32)c;
			return B_OK;
		}

	return ENOENT;
}

status_t devfs_read_vnode(void *ns, vnode_id vnid, void **node)
{
	struct node *n = (struct node *)((uint32)vnid);

	if (!(n->scanned)) {
		add_devices((struct node *)ns, n->h);
		n->scanned = TRUE;
		n->h = NULL;
	}

	*node = (void *)n;
	
	return B_OK;
}

status_t devfs_write_vnode(void *ns, void *node)
{
	return B_OK;
}

static status_t devfs_open(void *ns, void *node, void **cookie)
{
	struct node *n = (struct node *)node;
	if (!(n->h)) return B_OK;
	return n->h->open(n->master->path, cookie);
}

static status_t devfs_close(void *ns, void *node, void *cookie)
{
	struct node *n = (struct node *)node;
	if (!(n->h)) return B_OK;
	return n->h->close(cookie);
}

static ssize_t devfs_read(void *ns, void *node, void *cookie,
		off_t pos, void *buffer, uint32 len)
{
	struct node *n = (struct node *)node;

	if (n->h == NULL)
		return EISDIR;

	if (n->offset) {
		if (pos >= n->length) {
			dprintf("Warning: read beginning past end of partition requested\n");
			return 0;
		}

		/* clamp reads to end of partitions */
		if ((pos + len > n->length)) {
			dprintf("Warning: read extending past end of partition requested\n");
			len = n->length - pos;
		}
	}

	return n->h->read(cookie, pos + n->offset, buffer, len);
}

static status_t devfs_ioctl(void *ns, void *node, void *cookie,
					uint32 op, void *buffer, uint32 size)
{
	struct node *nodes = (struct node *)ns, *n = (struct node *)node;
	partition_info *p = (partition_info *)buffer;
	char device[20], *s;

	if (!(n->h)) return EISDIR;

	if (op == B_SET_PARTITION) {
		s = strrchr(p->device, '/');
		if (!s)
			return EINVAL;
		memcpy(device, p->device, s - p->device + 1);
		sprintf(device + (s - p->device) + 1, "%ld", p->partition);
		return add_device(nodes, device, device, NULL, p->partition, p->offset, p->size);
	} else if (op == B_GET_PARTITION_INFO) {
		p->offset = n->offset;
		p->size = n->length;
		p->logical_block_size = 0x200;
		p->session = 0;
		p->partition = n->partition;
		strcpy(p->device, n->master->name);
		return B_OK;
	}

	return n->h->ioctl(cookie, op, buffer, size);
}

static status_t devfs_fstat(void *ns, void *node, void *cookie, struct stat *st)
{
	struct node *n = (struct node *)node;

	st->st_mode = n->h ? 0 : S_IFDIR;
	st->st_size = 0LL;

	return 0;
}

static ssize_t devfs_freadlink(void *ns, void *node, void *cookie, char *buf, size_t len)
{
	return ENOSYS;
}

struct dcookie {
	struct node *n;
};

static status_t devfs_opendir(void *ns, void *node, void **cookie)
{
	struct node *n = (struct node *)node;
	struct dcookie *d;

	if (n->h)
		return ENOTDIR;

	d = malloc(sizeof(*d));
	if (!d)
		return ENOMEM;
	d->n = n->first_child;
	*cookie = (void *)d;
	
	return B_OK;
}

static status_t devfs_closedir(void *ns, void *node, void *cookie)
{
	free(cookie);
	return B_OK;
}

static status_t devfs_readdir(void *ns, void *node, void *cookie, struct dirent *de)
{
	struct dcookie *d = (struct dcookie *)cookie;

	if (!d->n)
		return ENOENT;

	strcpy(de->d_name, d->n->name);
	d->n = d->n->next_sibling;

	return B_OK;
}

static status_t devfs_rewinddir(void *ns, void *node, void *cookie)
{
	struct dcookie *d = (struct dcookie *)cookie;
	d->n = ((struct node *)node)->first_child;
	return 0;
}

struct fs_ops devfs_ops = {
	devfs_mount,
	devfs_unmount,

	devfs_walk,
	devfs_read_vnode,
	devfs_write_vnode,

	devfs_open,
	devfs_close,
	devfs_read,
	devfs_ioctl,
	devfs_fstat,
	devfs_freadlink,

	devfs_opendir,
	devfs_closedir,
	devfs_readdir,
	devfs_rewinddir
};
