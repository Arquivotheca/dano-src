#ifndef _BT_FS_H
#define _BT_FS_H

#ifndef _FSPROTO_H
	typedef dev_t nspace_id;
	typedef ino_t vnode_id;
#endif

struct fs_ops {
	status_t (*mount)(nspace_id nsid, const char *device, 
					uint32 flags, void *parms, int len,
					void **ns, vnode_id *root);
	status_t (*unmount)(void *ns);

	status_t (*walk)(void *ns, void *dir, const char *file,
					char *newpath, vnode_id *vnid);
	status_t (*read_vnode)(void *ns, vnode_id vnid, void **node);
	status_t (*write_vnode)(void *ns, void *node);

	status_t (*open)(void *ns, void *node, void **cookie);
	status_t (*close)(void *ns, void *node, void *cookie);
	ssize_t  (*read)(void *ns, void *node, void *cookie,
					off_t pos, void *buffer, uint32 len);
	status_t (*ioctl)(void *ns, void *node, void *cookie,
					uint32 op, void *buffer, uint32 size);
	status_t (*fstat)(void *ns, void *node, void *cookie, struct stat *st);
	ssize_t  (*freadlink)(void *ns, void *node, void *cookie, char *buf, size_t len);

	status_t (*opendir)(void *ns, void *node, void **cookie);
	status_t (*closedir)(void *ns, void *node, void *cookie);
	status_t (*readdir)(void *ns, void *node, void *cookie, struct dirent *d);
	status_t (*rewinddir)(void *ns, void *node, void *cookie);
};

int get_vnode(nspace_id nsid, vnode_id vnid, void **node);
int put_vnode(nspace_id nsid, vnode_id vnid);

#endif
