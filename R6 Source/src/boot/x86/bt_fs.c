/* vyt: add magic numbers */

#include <drivers/KernelExport.h>

#include <dirent.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>

#include "bt_fs.h"

extern struct fs_ops devfs_ops, bt_bfs_ops, bt_cfs_ops, ffs_ops,
		bt_dos_ops, bt_ext2_ops, bt_ntfs_ops;

static struct fsdefs_t {
	const char *name;
	struct fs_ops *ops;
} fsdefs[] = {
	{ "dev", &devfs_ops },
	{ "bfs", &bt_bfs_ops },
	{ "cfs", &bt_cfs_ops },
	{ "ffs", &ffs_ops },
	{ NULL, NULL }
};

struct ns_info {
	int32 ref;
	char mountpoint[128];
	int mountpoint_strlen;
	struct fs_ops *ops;
	void *ns_cookie;	/* fs cookie */
	nspace_id nsid;
	vnode_id root;

	struct ns_info *next;
};

static struct ns_info *mounted = NULL;

struct node_info {
	struct ns_info *ns;
	vnode_id vnid;
	void *node_cookie;
};

struct file_info {
	struct node_info *node;
	off_t pos;
	void *file_cookie;
};

static status_t __get_vnode(
		struct ns_info *ns, vnode_id vnid, struct node_info **node)
{
	struct node_info *n;
	status_t error;
	void *cookie;

	n = malloc(sizeof(*n));
	if (!n)
		return ENOMEM;

	error = ns->ops->read_vnode(ns->ns_cookie, vnid, &cookie);
	if (error) {
		free(n);
		return error;
	}

	n->ns = ns;
	n->vnid = vnid;
	n->node_cookie = cookie;

	*node = n;

	return B_OK;
}

static void __put_vnode(struct node_info *node)
{
	node->ns->ops->write_vnode(node->ns->ns_cookie, node->node_cookie);
	free(node);
}

struct vnode_cache_entry {
	int32 ref;
	struct ns_info *ns;
	vnode_id vnid;
	struct node_info *node;

	struct vnode_cache_entry *prev, *next;
};

#define MAX_VNODE_CACHE_ENTRIES 64
int32 vnode_cache_entries = 0;
struct vnode_cache_entry *vnode_cache = NULL;

#if 0
static void dump_vnode_cache(void)
{
	struct vnode_cache_entry *c;

	dprintf("vnode cache:\n");
	for (c=vnode_cache;c;c=c->next)
		dprintf("%x %x %x: %p %Lx %p (%ld)\n", c, c->prev, c->next, c->ns, c->vnid, c->node, c->ref);
	dprintf("END\n");
}
#endif

static status_t _get_vnode(
		struct ns_info *ns, vnode_id vnid, struct node_info **node)
{
	status_t error;
	struct vnode_cache_entry *c;

//	dprintf("_get_vnode %p %Lx\n", ns, vnid);
	
	for (c=vnode_cache;c;c=c->next) {
		if ((c->ns == ns) && (c->vnid == vnid)) {
//			dprintf("_get_vnode %p %Lx found in cache (%ld)\n", c->ns, c->vnid, c->ref);

			*node = c->node;
			/* increment refcount and move to front of list */
			ns->ref++;
			c->ref++;
			if (c->prev)
				c->prev->next = c->next;
			else
				vnode_cache = c->next;
			if (c->next)
				c->next->prev = c->prev;
			c->prev = NULL;
			c->next = vnode_cache;
			if (c->next)
				c->next->prev = c;
			vnode_cache = c;

			return B_OK;
		}
	}

//	dprintf("adding %p %Lx to vnode cache\n", ns, vnid);

	error = __get_vnode(ns, vnid, node);
	if (error)
		return error;

	if (vnode_cache_entries >= MAX_VNODE_CACHE_ENTRIES) {
		struct vnode_cache_entry *e = NULL;

		for (c=vnode_cache;c;c=c->next)
			if (c->ref == 0)
				e = c;
		
		if (e == NULL)
			panic("vnode cache full\n");

//		dprintf("vnode cache purge %p %Lx\n", e->ns, e->vnid);

		if (e->prev)
			e->prev->next = e->next;
		else
			vnode_cache = e->next;
		if (e->next)
			e->next->prev = e->prev;
		__put_vnode(e->node);
		free(e);
		
		vnode_cache_entries--;
	}
	
	vnode_cache_entries++;

	c = malloc(sizeof(*c));
	/* XXX: error checking */
	ns->ref++;
	c->ref = 1;
	c->ns = ns;
	c->vnid = vnid;
	c->node = *node;
	c->prev = NULL;
	c->next = vnode_cache;
	if (c->next)
		c->next->prev = c;
	vnode_cache = c;

	return B_OK;
}

int get_vnode(nspace_id nsid, vnode_id vnid, void **node)
{
	struct ns_info *c;
	struct node_info *ni;
	status_t error;

	for (c=mounted;c;c=c->next)
		if (c->nsid == nsid)
			break;

	if (!c)
		return EINVAL;

	error = _get_vnode(c, vnid, &ni);
	if (error == B_OK)
		*node = ni->node_cookie;

	return error;
}

static void _put_vnode(
		struct node_info *node)
{
	struct vnode_cache_entry *c;

//	dprintf("_put_vnode %p %Lx %p\n", node->ns, node->vnid, node);
	
	for (c=vnode_cache;c;c=c->next) {
		if (c->node == node) {
			c->ns->ref--;
			c->ref--;
			return;
		}
	}
	panic("_put_vnode: can't find vnode\n");
}

int put_vnode(nspace_id nsid, vnode_id vnid)
{
	struct vnode_cache_entry *c;

	for (c=vnode_cache;c;c=c->next) {
		if ((c->ns->nsid == nsid) && (c->vnid == vnid)) {
			c->ns->ref--;
			c->ref--;
			return 0;
		}
	}
	panic("put_vnode: can't find vnode\n");
	return ENOENT;
}

static void flush_vnode_cache_entries(struct ns_info *ns)
{
	struct vnode_cache_entry *c, *n;

	for (c=vnode_cache;c;c=n) {
		n = c->next;
		if (c->ns == ns) {
			if (c->ref)
				panic("ref count on unmount = %d\n", c->ref);
			if (c->prev)
				c->prev->next = c->next;
			else
				vnode_cache = c->next;
			if (c->next)
				c->next->prev = c->prev;
			__put_vnode(c->node);
			free(c);
		
			vnode_cache_entries--;
		}
	}
}

struct walk_cache_entry {
	struct ns_info *ns;
	vnode_id dir, vnid;
	#define WALK_NAME_LEN 64
	char file[WALK_NAME_LEN];

	struct walk_cache_entry *prev, *next;
};

#define MAX_WALK_CACHE_ENTRIES 64
int32 walk_cache_entries = 0;
struct walk_cache_entry *walk_cache = NULL;

#if 0
static void dump_walk_cache(void)
{
	struct walk_cache_entry *c;

	dprintf("walk_cache (%ld entries):\n", walk_cache_entries);
	for (c=walk_cache;c;c=c->next)
		dprintf("%p %p %p: %p %Lx %s %Lx\n", c->prev, c, c->next, c->ns, c->dir, c->file, c->vnid);
	dprintf("END\n");
}
#endif

static void add_walk_cache(struct ns_info *ns, vnode_id dir, char *file, vnode_id vnid)
{
	struct walk_cache_entry *e;

//	dprintf("add walk cache entry: %p %Lx %s %Lx\n", ns, dir, file, vnid);

	if (strlen(file) >= WALK_NAME_LEN )
		return;

	if (walk_cache_entries++ >= MAX_WALK_CACHE_ENTRIES) {
		for (e=walk_cache;e->next->next;e=e->next)
			;
		dprintf("walk cache purge: %p %Lx %s %Lx\n", e->next->ns, e->next->dir, e->next->file, e->next->vnid);
		free(e->next);
		e->next = NULL;
		walk_cache_entries--;
	}

	e = malloc(sizeof(*e));
	e->ns = ns;
	e->dir = dir;
	e->vnid = vnid;
	strcpy(e->file, file);
	e->prev = NULL;
	e->next = walk_cache;
	if (e->next) e->next->prev = e;
	walk_cache = e;
}

static status_t lookup_walk_cache(
		struct ns_info *ns, vnode_id dir, char *file, vnode_id *vnid)
{
	struct walk_cache_entry *e;

//	dprintf("lookup walk cache entry: %p %Lx %s\n", ns, dir, file);

	for (e=walk_cache;e;e=e->next) {
		if (	(e->ns == ns) &&
				(e->dir == dir) &&
				!strcmp(e->file, file)) {
			*vnid = e->vnid;

//			dprintf("found entry: %Lx\n", *vnid);

			if (e->prev)
				e->prev->next = e->next;
			else
				walk_cache = e->next;
			if (e->next)
				e->next->prev = e->prev;
			e->prev = NULL;
			e->next = walk_cache;
			if (e->next)
				e->next->prev = e;
			walk_cache = e;

			return B_OK;
		}
	}
	return ENOENT;
}

static void flush_walk_cache_entries(struct ns_info *ns)
{
	struct walk_cache_entry *c, *n;

	for (c=walk_cache;c;c=n) {
		n = c->next;
		if (c->ns == ns) {
			if (c->prev)
				c->prev->next = c->next;
			else
				walk_cache = c->next;
			if (c->next)
				c->next->prev = c->prev;
			free(c);
		
			walk_cache_entries--;
		}
	}
}

static status_t walk(const char *path, struct ns_info **ns, vnode_id *vnid)
{
	const char *p;
	char file[128], nfile[128], *f;
	struct node_info *ni;
	struct ns_info *n;
	status_t error;

	if (*path != '/')
		return EINVAL;

	n = mounted;
	while (n) {
		if (	!strncmp(path, n->mountpoint, n->mountpoint_strlen) &&
				((path[n->mountpoint_strlen] == '/') ||
				 (path[n->mountpoint_strlen] == 0))) {
			*ns = n;
			*vnid = n->root;
			break;
		}
		n = n->next;
	}

	if (!n)
		return ENOENT;

	p = path + n->mountpoint_strlen;

	while (*p) {
		if (*p == '/') {
			p++;
			continue;
		}

		f = file;
		while (*p && (*p != '/'))
			*(f++) = *(p++);
		*f = 0;
		nfile[0] = 0;

		if (lookup_walk_cache(*ns, *vnid, file, vnid) != B_OK) {
			vnode_id dir = *vnid;
			error = _get_vnode(*ns, *vnid, &ni);
			if (error)
				return error;
			error = (*ns)->ops->walk((*ns)->ns_cookie, ni->node_cookie, file, nfile, vnid);
			_put_vnode(ni);
			if (error)
				return error;
			add_walk_cache(*ns, dir, file, *vnid);
		}
	}

	return B_OK;
}

int mount(const char *filesystem, const char *where, const char *device,
		unsigned long flags, void *parms, int len)
{
	int i;
	struct fs_ops *ops = NULL;
	struct ns_info *ns;
	status_t error;
	void *cookie;
	vnode_id root;

	static nspace_id last_nsid = 0;

	for (i=0;fsdefs[i].name;i++) {
		if (!strcmp(filesystem, fsdefs[i].name)) {
			ops = fsdefs[i].ops;
			break;
		}
	}

	if (!ops) {
		dprintf("mount: unknown fs type %s\n", filesystem);
		return EINVAL;
	}

	ns = malloc(sizeof(*ns));

	error = ops->mount(last_nsid, device, flags, parms, len, &cookie, &root);
	if (error) {
		free(ns);
		return error;
	}

	strcpy(ns->mountpoint, where);
	ns->ref = 0;
	ns->mountpoint_strlen = strlen(where);
	ns->ops = ops;
	ns->ns_cookie = cookie;
	ns->nsid = last_nsid++;
	ns->root = root;

	ns->next = mounted;
	mounted = ns;

	return B_OK;
}

int unmount(const char *path)
{
	struct ns_info *p, *c;

	p = c = mounted;
	while (c) {
		if (!strcmp(path, c->mountpoint)) {
			if (p == c)
				mounted = c->next;
			else
				p->next = c->next;
			c->ops->unmount(c->ns_cookie);
			flush_vnode_cache_entries(c);
			flush_walk_cache_entries(c);
			if (c->ref)
				panic("unmount called on %s with refcount %ld\n", path, c->ref);
			free(c);

			return B_OK;
		}
		p = c;
		c = c->next;
	}

	panic("unmount: can't find %s\n", path);

	return B_ERROR;
}

int open(const char *path, int mode, ...)
{
	status_t error;
	struct ns_info *ns;
	vnode_id vnid;
	struct node_info *node;
	struct file_info *f;
	void *fp;

	error = walk(path, &ns, &vnid);
	if (error)
		return error;

	error = _get_vnode(ns, vnid, &node);
	if (error)
		return error;

	error = ns->ops->open(ns->ns_cookie, node->node_cookie, &fp);
	if (error) {
		_put_vnode(node);
		return error;
	}

	f = malloc(sizeof(*f));
	if (!f) {
		ns->ops->close(ns->ns_cookie, node->node_cookie, fp);
		_put_vnode(node);
		return ENOMEM;
	}
	f->node = node;
	f->file_cookie = fp;
	f->pos = 0;

	return (uint32)f;
}

int close(int fd)
{
	struct file_info *f = (struct file_info *)fd;
	status_t err;

	err = f->node->ns->ops->close(f->node->ns->ns_cookie, f->node->node_cookie,
			f->file_cookie);
	_put_vnode(f->node);
	free(f);

	return err;
}

off_t lseek(int fd, off_t pos, int whence)
{
	struct file_info *f = (struct file_info *)fd;
	
	if (whence != SEEK_SET) panic("lseek: unsupported whence\n");
	
	return f->pos = pos;
}

ssize_t read(int fd, void *buf, size_t size)
{
	struct file_info *f = (struct file_info *)fd;
	ssize_t result;

	result = f->node->ns->ops->read(f->node->ns->ns_cookie, f->node->node_cookie,
			f->file_cookie, f->pos, buf, size);

	if (result > 0)
		f->pos += result;

	return result;
}

ssize_t read_pos(int fd, off_t pos, void *buf, size_t size)
{
	struct file_info *f = (struct file_info *)fd;
	f->pos = pos;
	return read(fd, buf, size);
}

int ioctl(int fd, unsigned long op, ...)
{
	va_list args;
	void *buf;
	int len;
	struct file_info *f = (struct file_info *)fd;

	va_start(args, op);
	buf = va_arg(args, void *);
	len = va_arg(args, int);
	va_end(args);

	return f->node->ns->ops->ioctl(f->node->ns->ns_cookie, f->node->node_cookie,
			f->file_cookie, op, buf, len);
}

int fstat(int fd, struct stat *st)
{
	struct file_info *f = (struct file_info *)fd;
	return f->node->ns->ops->fstat(f->node->ns->ns_cookie, f->node->node_cookie,
			f->file_cookie, st);
}

int stat(const char *path, struct stat *st)
{
	int fd, result;

	fd = open(path, 0);
	if (fd < 0) return fd;
	result = fstat(fd, st);
	close(fd);

	return result;
}

ssize_t readlink(const char *path, char *buf, size_t len)
{
	int fd;
	struct file_info *f;
	ssize_t result;

	fd = open(path, 0);
	if (fd < 0) return fd;
	f = (struct file_info *)fd;
	result = f->node->ns->ops->freadlink(f->node->ns->ns_cookie, f->node->node_cookie,
			f->file_cookie, buf, len);
	close(fd);

	return result;
}

DIR *opendir(const char *path)
{
	struct ns_info *ns;
	vnode_id vnid;
	struct node_info *node;
	DIR *d;
	struct file_info *f;
	void *dirp;
	status_t error;

	error = walk(path, &ns, &vnid);
	if (error)
		return NULL;

	error = _get_vnode(ns, vnid, &node);
	if (error)
		return NULL;

	error = ns->ops->opendir(ns->ns_cookie, node->node_cookie, &dirp);
	if (error) {
		_put_vnode(node);
		return NULL;
	}

	f = malloc(sizeof(*f));
	f->node = node;
	f->file_cookie = dirp;
	f->pos = 0;

	d = malloc(sizeof(*d) + 256); /* enough room for filename too */
	d->fd = (int)f;

	return d;
}

int closedir(DIR *dirp)
{
	int err;
	struct file_info *f = (struct file_info *)dirp->fd;
	err = f->node->ns->ops->closedir(f->node->ns->ns_cookie, f->node->node_cookie,
			f->file_cookie);
	_put_vnode(f->node);
	free(f);
	free(dirp);
	return err;
}

struct dirent *readdir(DIR *dirp)
{
	struct file_info *f = (struct file_info *)dirp->fd;
	return (f->node->ns->ops->readdir(f->node->ns->ns_cookie, f->node->node_cookie,
			f->file_cookie, &(dirp->ent)) == B_OK) ? &(dirp->ent) : NULL;
}

void rewinddir(DIR *dirp)
{
	struct file_info *f = (struct file_info *)dirp->fd;
	f->node->ns->ops->rewinddir(f->node->ns->ns_cookie, f->node->node_cookie,
			f->file_cookie);
}
