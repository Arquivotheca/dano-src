#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <drivers/KernelExport.h>

#include "cfs.h"
#include "cfs_compress.h"
#include <cfs_ioctls.h>

#include <boot.h>

#include "bt_fs.h"

#define PRINT_FLOW(x)
#define PRINT_ERROR(x) dprintf x

char *
strdup(const char *src)
{
	char *dest = malloc(strlen(src)+1);
	if(dest == NULL)
		return NULL;
	strcpy(dest, src);
	return dest;
}

typedef struct cfs_vblock_list {
	struct cfs_vblock_list  *next;
	uint32                   flags;			/* 0 uncompressed, 1 compressed */
	off_t                    data_pos;
	size_t                   size;
	size_t                   compressed_size;
	off_t                    location;      /* disk location of the entry */
} cfs_vblock_list;

typedef struct cfs_file {
	cfs_vblock_list  *datablocks;
} cfs_file;

typedef struct cfs_node cfs_node;

typedef struct cfs_directory {
	off_t             first_entry;
} cfs_directory;

typedef struct cfs_link {
	char             *link;
} cfs_link;

struct cfs_node {
	off_t             location;
	off_t             parent;
	off_t             attr_location;
	char             *name;
	int32             create_time;
	int32             mod_time;
	int32             flags;
	union {
		cfs_file      file;
		cfs_directory dir;
		cfs_link   link;
	} u;
};

typedef struct log_block log_block;
struct log_block {
	log_block  *next;
	off_t       location;
	void       *data;
};

typedef struct cfs_info {
	nspace_id nsid;
	int       dev_fd;
	size_t    _dev_block_size;
	size_t    cache_block_size;
	off_t     size;
	off_t     super_block_2_pos;
	off_t     reserved_space_pos;
	off_t     reserved_space_size;
	int32     create_time;
	int		  version;
	cfs_node  *root_dir;
	char      name[64];
	log_block *log;
} cfs_info;

typedef struct cfs_dir_cookie {
	off_t last_offset;
	int   index;
	char  last_name[CFS_MAX_NAME_LEN];
} cfs_dir_cookie;

static status_t 
cfs_read_disk_uncached(cfs_info *fsinfo, off_t pos, void *buffer,
                       size_t buffer_size)
{
	ssize_t read_size;
	read_size = read_pos(fsinfo->dev_fd, pos, buffer, buffer_size);
	if(read_size < buffer_size) {
		dprintf("cfs_read_disk_uncached: read %Ld, size %ld failed\n",
		        pos, buffer_size);
		return B_IO_ERROR;
	}
	return B_NO_ERROR;
}

static status_t 
cfs_read_disk_etc(cfs_info *fsinfo, off_t pos, void *buffer,
                  size_t *buffer_size, size_t min_read_size)
{
	size_t size_left;
	off_t curpos;
	uint8 *dest;

	if(pos < fsinfo->super_block_2_pos || pos+min_read_size > fsinfo->size) {
		dprintf("cfs_read_disk: position %Ld outsize fs bounds\n", pos);
		return B_IO_ERROR;
	}
	
	curpos = pos & ~(fsinfo->cache_block_size-1);
	size_left = *buffer_size;
	dest = buffer;
	
	if(pos+size_left > fsinfo->size) {
		size_left = fsinfo->size-pos;
	}
	
	*buffer_size = 0;
	
	//dprintf("cfs_read_disk_etc %Ld %p %ld min %ld\n",
	//        pos, buffer, *buffer_size, min_read_size);

	while(size_left > 0) {
		log_block *lb;
		uint8 *cache_block;
		size_t block_used = fsinfo->cache_block_size;
		if(curpos < pos)
			block_used -= (pos-curpos);
		if(size_left < block_used)
			block_used = size_left;

		//dprintf("cfs_read_disk_etc curpos %Ld %p %ld left %ld\n",
		//        curpos, dest, *buffer_size, size_left);
	
		cache_block = NULL;
		for(lb = fsinfo->log; lb != NULL; lb = lb->next) {
			if(lb->location == curpos) {
				dprintf("cfs_read_disk_etc got block at %Ld from log\n",
				        curpos);
				cache_block = lb->data;
				break;
			}
		}
		if(cache_block == NULL) {
			ssize_t readlen;
			
			if(curpos < pos)
				readlen = read_pos(fsinfo->dev_fd, pos, dest, block_used);
			else
				readlen = read_pos(fsinfo->dev_fd, curpos, dest, block_used);

			if(readlen < block_used) {
				if(*buffer_size < min_read_size) {
					dprintf("cfs_read_disk: could not get block at %Ld\n",
					        curpos);
					return B_IO_ERROR;
				}
				else
					return B_NO_ERROR;
			}
		}
		else {
			if(curpos < pos) {
				memcpy(dest, cache_block+(pos-curpos), block_used);
			}
			else {
				memcpy(dest, cache_block, block_used);
			}
		}
		curpos += fsinfo->cache_block_size;
		size_left -= block_used;
		dest += block_used;
		*buffer_size += block_used;
	}
	return B_NO_ERROR;
}

#if 0
status_t 
cfs_read_disk_etc(cfs_info *fsinfo, off_t pos, void *buffer,
                  size_t *buffer_size, size_t min_read_size)
{
	ssize_t readlen;
	log_block *lb;

	//dprintf("cfs_read_disk_etc: pos %Ld size %ld\n", pos, *buffer_size);

	if(pos < super_block_2_pos || pos+min_read_size > fsinfo->size) {
		dprintf("cfs_read_disk: position %Ld outsize fs bounds\n", pos);
		return B_IO_ERROR;
	}
	for(lb = fsinfo->log; lb != NULL; lb < 
	readlen = read_pos(fsinfo->dev_fd, pos, buffer, *buffer_size);
	if(readlen < min_read_size) {
		dprintf("cfs_read_disk: short read, at %Ld\n", pos);
		return B_IO_ERROR;
	}
	*buffer_size = readlen;
	return B_NO_ERROR;
}
#endif

status_t 
cfs_read_disk(cfs_info *fsinfo, off_t pos, void *buffer, size_t buffer_size)
{
	return cfs_read_disk_etc(fsinfo, pos, buffer, &buffer_size, buffer_size);
}

status_t
cfs_read_entry(cfs_info *fsinfo, off_t vnid, cfs_entry_info *entry,
               char *name)
{
	status_t err;
	ssize_t readlen;
	int i;
	off_t location = cfs_vnid_to_offset(vnid);

	err = cfs_read_disk(fsinfo, location, entry, get_entry_size(fsinfo->version));
	if(err != B_NO_ERROR)
		return err;

	if(fsinfo->version <= 1) entry->logical_size = 0;

	readlen = CFS_MAX_NAME_LEN;
	err = cfs_read_disk_etc(fsinfo, entry->name_offset, name, &readlen, 1);
	if(err != B_NO_ERROR)
		return err;

	for(i = 0; i < readlen; i++) {
		if(name[i] == '\0')
			break;
	}
	if(i == readlen) {
		dprintf("cfs_read_entry: bad entry name\n");
		return B_IO_ERROR;
	}
	return B_NO_ERROR;
}

static bool
log_info_valid(cfs_log_info *li, int index, int index_count)
{
	if(li->log_id != CFS_LOG_ID)
		return false;
	if(li->version != li->last_version + 1)
		return false;
	if(li->version % index_count != index)
		return false;
	if(li->log_id + li->version + (uint32)li->head + (uint32)li->tail +
	   li->last_version != li->checksum)
		return false;
	return true;
}

static status_t
read_log_info(cfs_info *fsinfo, off_t log_pos, off_t log_size,
              off_t *log_head, off_t *log_tail)
{
	status_t err;
	uint32 log_version = 0;
	int i;
	off_t log_info_pos[2];
	int info_count = 2;
	off_t log_base = log_pos + fsinfo->_dev_block_size;
	
	cfs_log_info log_info;

	log_info_pos[0] = log_pos;
	log_info_pos[1] = log_pos + log_size - fsinfo->_dev_block_size;

	PRINT_FLOW(("cfs: read_log_info\n"));
	*log_head = 0;
	*log_tail = 0;

	for(i = 0; i < info_count; i++) {
		err = cfs_read_disk_uncached(fsinfo, log_info_pos[i], &log_info,
		                             sizeof(log_info));
		if(err == B_NO_ERROR && log_info_valid(&log_info, i, info_count)) {
			if(*log_head < log_base ||
			   log_version / info_count * info_count + i == log_info.version) {
				*log_head = log_info.head;
				*log_tail = log_info.tail;
				log_version = log_info.version;
			}
		}
	}
	if(*log_head < log_base) {
		PRINT_ERROR(("cfs_read_log_info: "
		             "could not find valid log info block\n"));
		return B_ERROR;
	}
	
	return B_NO_ERROR;
}
	
static status_t
playback_block(cfs_info *fsinfo, off_t log_pos, off_t block_pos)
{
	log_block *lb;

	for(lb = fsinfo->log; lb != NULL; lb = lb->next) {
		if(lb->location == block_pos) {
			break;
		}
	}
	if(lb == NULL) {
		lb = malloc(sizeof(*lb));
		if(lb == NULL)
			return B_NO_MEMORY;
		lb->location = block_pos;
		lb->data = malloc(fsinfo->cache_block_size);
		if(lb->data == NULL) {
			free(lb);
			return B_NO_MEMORY;
		}
		lb->next = fsinfo->log;
		fsinfo->log = lb;
	}

	return cfs_read_disk_uncached(fsinfo, log_pos, lb->data,
	                              fsinfo->cache_block_size);
}

static status_t
playback_log(cfs_info *fsinfo, off_t log_base, off_t log_size,
             off_t log_head, off_t log_tail)
{
	status_t err;
	int i;
	bool wrapped = false;
	cfs_log_entry_header *log_header = malloc(fsinfo->_dev_block_size);
	if(log_header == NULL)
		return B_NO_MEMORY;
	
	while(log_head != log_tail) {
		dprintf("cfs playback_log: entry at %Ld\n", log_head);
		err = cfs_read_disk_uncached(fsinfo, log_head, log_header,
		                             fsinfo->_dev_block_size);
		if(err != B_NO_ERROR)
			goto err;
		if(log_header->log_entry_id != CFS_LOG_ENTRY_ID) {
			PRINT_ERROR(("cfs_playback_log: bad log entry header at %Ld\n",
			             log_head));
			err = B_ERROR;
			goto err;
		}
		if(log_header->num_blocks < 1 ||
		   log_header->num_blocks >
		   max_blocks_per_log_entry(fsinfo->_dev_block_size)) {
			PRINT_ERROR(("cfs_playback_log: bad log entry header at %Ld, "
			             "num blocks %ld\n", log_head, log_header->num_blocks));
			err = B_ERROR;
			goto err;
		}
		log_head += fsinfo->_dev_block_size;
		for(i=0; i < log_header->num_blocks; i++) {
			if(log_head + fsinfo->cache_block_size > log_base + log_size) {
				if(wrapped) {
					PRINT_ERROR(("cfs_playback_log: log does not end\n"));
					err = B_ERROR;
					goto err;
				}
				wrapped = true;
				log_head = log_base;
			}
			err = playback_block(fsinfo, log_head, log_header->blocks[i]);
			log_head += fsinfo->cache_block_size;
		}
		if(log_head >= log_base + log_size) {
			if(wrapped) {
				PRINT_ERROR(("cfs_playback_log: log does not end\n"));
				err = B_ERROR;
				goto err;
			}
			wrapped = true;
			log_head = log_base;
		}
	}
	err = B_NO_ERROR;
err:
	free(log_header);
	return err;
}

static status_t
cfs_init_log(cfs_info *fsinfo, off_t log_pos, off_t log_size)
{
	status_t err;
	off_t log_head;
	off_t log_tail;

	err = read_log_info(fsinfo, log_pos, log_size, &log_head, &log_tail);
	if(err)
		return err;
	
	if(log_head == log_tail)	/* nothing to do */
		return B_NO_ERROR;

	dprintf("cfs: playback log...\n");

	return playback_log(fsinfo, log_pos + fsinfo->_dev_block_size,
	                    log_size - 2 * fsinfo->_dev_block_size,
	                    log_head, log_tail);
}

static status_t
read_sb1(int dev_fd, cfs_super_block_1 *sb1)
{
	if(read_pos(dev_fd, cfs_superblock_1_offset, sb1, sizeof(*sb1)) <
	   sizeof(*sb1))
		return B_FILE_ERROR;
	if(sb1->cfs_id_1 != CFS_ID_1)
		return B_FILE_ERROR;
	if(sb1->cfs_version > CFS_CURRENT_VERSION) {
		PRINT_ERROR(("cfs: version # (%ld) is too high. Max supported is %d\n",
			sb1->cfs_version, CFS_CURRENT_VERSION));
		return B_FILE_ERROR;
	}
	if(sb1->cfs_version < CFS_MIN_SUPPORTED_VERSION) {
		PRINT_ERROR(("cfs: version # (%ld) is too low. Min supported is %d\n",
			sb1->cfs_version, CFS_MIN_SUPPORTED_VERSION));
		return B_FILE_ERROR;
	}		
	if(sb1->dev_block_size <= 0) {
		PRINT_ERROR(("cfs: invalid dev block size %ld\n", sb1->dev_block_size));
		return B_FILE_ERROR;
	}
	if(sb1->fs_block_size <= 0) {
		PRINT_ERROR(("cfs: invalid fs block size %ld\n", sb1->fs_block_size));
		return B_FILE_ERROR;
	}
	if(sb1->fs_block_size % sb1->dev_block_size != 0) {
		PRINT_ERROR(("cfs: fs block size (%ld) is not a multiple of dev "
		             "block size (%ld)\n", sb1->fs_block_size,
		             sb1->dev_block_size));
		return B_FILE_ERROR;
	}
	if(sb1->size % sb1->fs_block_size != 0) {
		PRINT_ERROR(("cfs: fs size (%Ld) is not divisible by %ld\n",
		             sb1->size, sb1->fs_block_size));
		return B_FILE_ERROR;
	}
	if(sb1->super_block_2 % sb1->fs_block_size != 0) {
		PRINT_ERROR(("cfs: superblock2 location (%Ld) is not divisible by %ld\n",
		             sb1->super_block_2, sb1->fs_block_size));
		return B_FILE_ERROR;
	}
	return B_NO_ERROR;
}

status_t
bt_cfs_mount(nspace_id nsid, const char *devname, uint32 flags, void *parms,
          int len, void **data, vnode_id *vnid)
{
	status_t err;
	cfs_info *info;
	cfs_super_block_1 sb1;
	cfs_super_block_2 sb2;
	//dprintf("bt_cfs_mount\n");
	err = B_NO_MEMORY;
	info = malloc(sizeof(cfs_info));
	if(info == NULL)
		goto err0;
	info->nsid = nsid;
	err = info->dev_fd = open(devname, O_RDWR);
	if(err < B_NO_ERROR)
		goto err1;

	err = read_sb1(info->dev_fd, &sb1);
	if(err != B_NO_ERROR)
		goto err2;
	
	info->version = sb1.cfs_version;
	info->_dev_block_size = sb1.dev_block_size;
	info->cache_block_size = sb1.fs_block_size;
	info->create_time = sb1.create_time;
	info->size = sb1.size;
	//dprintf("cfs_mount: fs size %Ld\n", sb.size);
	info->super_block_2_pos = sb1.super_block_2;
	info->reserved_space_pos = sb1.reserved_space_1_pos;
	info->reserved_space_size = sb1.reserved_space_1_size;
	info->log = NULL;
	if(sb1.log_size != 0) {
		err = cfs_init_log(info, sb1.log_pos, sb1.log_size);
		if(err != B_NO_ERROR)
			goto err2;
	}

	err = cfs_read_disk(info, info->super_block_2_pos, &sb2, sizeof(sb2));
	if(err != B_NO_ERROR)
		goto err2;

	strncpy(info->name, sb2.volume_name, sizeof(info->name));

	//err = cfs_read_vnode(info, cfs_root_id, 1, (void**)&info->root_dir);
	
	*vnid = sb1.root;
	*data = info;
	//dprintf("cfs_mount: root id %Ld data %p\n", cfs_root_id, info);
	return B_NO_ERROR;

err2:	close(info->dev_fd);
err1:	free(info);
err0:	return err;
}

status_t
bt_cfs_unmount(void *ns)
{
	cfs_info *info = ns;
	close(info->dev_fd);
	free(ns);
	return B_NO_ERROR;
}

status_t 
bt_cfs_walk(void *ns, void *base, const char *file, char *newpath, vnode_id *vnid)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *dir = (cfs_node *)base;
	off_t entry_offset;
	if(dir == NULL) {
		dprintf("cfs_walk: no base directory\n");
		return B_ERROR;
	}

	if (!S_ISDIR(dir->flags)) {
		dprintf("cfs_walk: base directory is not a directory\n");
		err = ENOTDIR;
		goto err;
	}
	//dprintf("bt_cfs_walk: base %s, entry %s\n", dir->name, file);
	
	if(strcmp(file, ".") == 0) {
		*vnid = dir->location;
		goto found_entry;
	}
	if(strcmp(file, "..") == 0) {
		*vnid = dir->parent;
		goto found_entry;
	}

	//dprintf("cfs_walk: file %s\n", file);
	entry_offset = dir->u.dir.first_entry;
	while(entry_offset != 0) {
		cfs_entry_info entry;
		char entry_name[CFS_MAX_NAME_LEN];
		
		err = cfs_read_entry(fsinfo, entry_offset, &entry, entry_name);
		if(err != B_NO_ERROR) {
			dprintf("cfs_walk: read failed\n");
			goto err;
		}
		if(entry.parent != dir->location) {
			dprintf("cfs_walk: bad entry at %Ld, parent %Ld should be %Ld\n",
			        entry_offset, entry.parent, dir->location);
			err = B_ENTRY_NOT_FOUND;
			goto err;
		}
		if(strcmp(entry_name, file) == 0) {
			*vnid = (vnode_id)entry_offset;
			goto found_entry;
		}
		if(cfs_vnid_to_offset(entry.next_entry) < 0 ||
		   cfs_vnid_to_offset(entry.next_entry) > fsinfo->size) {
			dprintf("cfs_walk: vnid 0x016%Lx, name %s, "
			        "next directory entry stored out of filesystem bounds\n",
			        entry_offset, entry_name);
			err = B_ENTRY_NOT_FOUND;
			goto err;
		}
		entry_offset = entry.next_entry;
	}

	//dprintf("cfs_walk: entry %s not found\n", file);
	err = B_ENTRY_NOT_FOUND;
	goto err;

found_entry:
	{
		cfs_node *node;
		//dprintf("cfs_walk: found entry %s\n", file);
		err = get_vnode(fsinfo->nsid, *vnid, (void **)&node);
		if(err != B_NO_ERROR)
			goto err;

		if(S_ISLNK(node->flags)) {
			//dprintf("cfs_walk: found link %s, newpath %p\n", file, newpath);
		}
		if(S_ISLNK(node->flags) && newpath) {
			strcpy(newpath, node->u.link.link);
			//put_vnode(fsinfo->nsid, *vnid);
		}
		put_vnode(fsinfo->nsid, *vnid); // bool loader only
	}
	err = B_NO_ERROR;
err:
	return err;
}

status_t 
bt_cfs_read_vnode(void *ns, vnode_id vnid, void **node)
{
	status_t err;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *new_node;
	cfs_entry_info entry;
	char entry_name[CFS_MAX_NAME_LEN];

	if(cfs_vnid_to_offset(vnid) < 0 ||
	   cfs_vnid_to_offset(vnid) > ((cfs_info*)ns)->size) {
		dprintf("CFS - read_vnode: bad vnid, 0x%016Lx\n", vnid);
		return B_BAD_VALUE;
	}

	new_node = malloc(sizeof(cfs_node));
	if(new_node == NULL) {
		err = B_NO_MEMORY;
		goto err0;
	}

	err = cfs_read_entry(fsinfo, (off_t)vnid, &entry, entry_name);
	if(err != B_NO_ERROR)
		goto err1;

	new_node->location = (off_t)vnid;
	new_node->parent = entry.parent;
	new_node->name = strdup(entry_name);
	new_node->create_time =  entry.create_time;
	new_node->mod_time =  entry.modification_time;
	new_node->attr_location = entry.attr;
	new_node->flags = entry.flags;
		
	switch(entry.flags & S_IFMT) {
		case S_IFDIR: {
			new_node->flags |= S_IFDIR;
			new_node->u.dir.first_entry = entry.data;
			if(cfs_vnid_to_offset(entry.data) < 0 ||
			   cfs_vnid_to_offset(entry.data) > fsinfo->size) {
				dprintf("cfs_read_vnode: vnid %Ld, name %s, "
				        "directory entries stored out of filesystem bounds\n",
				        vnid, new_node->name);
				err = B_IO_ERROR;
				goto err1;
			}
		} break;

		case S_IFREG: {
			off_t filesize = 0; // size of disk data and blocklist
			off_t next_data = entry.data;
			cfs_vblock_list *last_block = NULL;

			new_node->flags |= S_IFREG;
			new_node->u.file.datablocks = NULL;
			
			err = B_NO_ERROR;
			while(next_data != 0) {
				cfs_vblock_list *new_block = NULL;
				cfs_data_block block;
				
				if(next_data <= 0 || next_data > fsinfo->size) {
					dprintf("cfs_read_vnode: vnid %Ld, name %s, "
					        "blocklist links out of filesystem bounds\n",
					        vnid, new_node->name);
					err = B_IO_ERROR;
					break;
				}
				if(filesize > fsinfo->size) {
					dprintf("cfs_read_vnode: vnid %Ld, name %s, "
					        "blocklist does not end\n",
					        vnid, new_node->name);
					err = B_IO_ERROR;
					break;
				}
				
				err = cfs_read_disk(fsinfo, next_data, &block, sizeof(block));
				if(err != B_NO_ERROR) {
					dprintf("cfs_read_vnode: vnid %Ld, name %s could not read blocklist\n", vnid, new_node->name);
					break;
				}
				if(block.data <= 0 ||
				  block.data + block.disk_size > fsinfo->size) {
					dprintf("cfs_read_vnode: vnid %Ld, name %s, "
					        "file data out of filesystem bounds\n",
					        vnid, new_node->name);
					err = B_IO_ERROR;
					break;
				}
				filesize += block.disk_size + sizeof(block);

				new_block = calloc(1, sizeof(cfs_vblock_list));
				if(new_block == NULL) {
					err = B_NO_MEMORY;
					break;
				}
				new_block->data_pos = block.data;
				if(block.raw_size != 0) {
					new_block->flags = 1;
					new_block->size = block.raw_size;
					new_block->compressed_size = block.disk_size;
				}
				else {
					new_block->flags = 0;
					new_block->size = block.disk_size;
				}
				new_block->location = next_data;
				next_data = block.next;
				if(last_block)
					last_block->next = new_block;
				else
					new_node->u.file.datablocks = new_block;
				last_block = new_block;
			}
			if(next_data != 0) {
				cfs_vblock_list *cur, *next;
				cur = new_node->u.file.datablocks;
				while(cur) {
					next = cur->next;
					free(cur);
					cur = next;
				}
				if(err >= B_NO_ERROR)
					err = B_IO_ERROR;
				goto err1;
			}
		} break;

		case S_IFLNK: {
			char link[PATH_MAX];

			new_node->flags |= S_IFLNK;
			if(entry.data <= 0 || entry.data > fsinfo->size) {
				dprintf("cfs_read_vnode: vnid %Ld, name %s, "
				        "symlink stored out of filesystem bounds\n",
				        vnid, new_node->name);
				err = B_IO_ERROR;
				goto err1;
			}
			err = cfs_read_disk(fsinfo, entry.data, &link, sizeof(link));
			if(err != B_NO_ERROR) {
				dprintf("cfs_read_vnode: vnid %Ld, name %s could not read link\n", vnid, new_node->name);
				goto err1;
			}
			link[PATH_MAX-1] = '\0';
			new_node->u.link.link = strdup(link);
			if(new_node->u.link.link == NULL) {
				err = B_NO_MEMORY;
				goto err1;
			}
		} break;

		default:
			dprintf("cfs_read_vnode: vnid %Ld, name %s bad type\n", vnid, new_node->name);
			err = B_BAD_VALUE;
			goto err1;
	}

	*node = new_node;
	//dprintf("cfs_read_vnode: vnid %Ld, name %s\n", vnid, new_node->name);
	err = B_NO_ERROR;
	goto done;
	
err1:
	free(new_node);
err0:
done:
	return err;
}

status_t 
bt_cfs_write_vnode(void *ns, void *_node)
{
	cfs_node *node = (cfs_node *)_node;
	//dprintf("cfs_write_vnode: vnid %Ld, name %s\n", ((cfs_node *)node)->location, ((cfs_node *)node)->name);
	if(S_ISLNK(node->flags)) {
		free(node->u.link.link);
	}
	else if(S_ISREG(node->flags)) {
		cfs_vblock_list *cur, *next;
		cur = node->u.file.datablocks;
		while(cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}
	}
	free(node->name);
	free(node);
	return B_NO_ERROR;
}

status_t 
bt_cfs_open(void *ns, void *node, void **cookie)
{
	*cookie = NULL;
	return B_NO_ERROR;
}

status_t 
bt_cfs_close(void *ns, void *node, void *cookie)
{
	return B_NO_ERROR;
}

status_t 
cfs_read_data(cfs_info *fsinfo, cfs_node *node, off_t pos,
              void *buf, size_t *len)
{
	status_t err;
	off_t cur_filepos = 0;
	cfs_vblock_list *cur_block;
	size_t request_len = *len;
	uint8 *cur_buffer = buf;

	*len = 0;
	
	if(!S_ISREG(node->flags)) {
		return B_BAD_VALUE;
	}

	cur_block = node->u.file.datablocks;
	
	//dprintf("cfs_read: file %s, pos %Ld, len %ld\n", node->name, pos, request_len);
	
	while(request_len > 0) {
		off_t readpos;
		size_t readlen = request_len;
//dprintf("cfs_read: ... pos %Ld, len %ld, fpos %Ld\n", pos, request_len, cur_filepos);
		while(cur_block && cur_filepos + cur_block->size <= pos) {
//dprintf("cfs_read: skip block size %ld\n", cur_block->size);
			cur_filepos += cur_block->size;
			cur_block = cur_block->next;
		}
		if(cur_block == NULL) {
			err = B_NO_ERROR;
			goto err;
		}
//dprintf("cfs_read: current block size %ld at %Ld\n", cur_block->size, cur_block->data_pos);
		if(cur_block->flags) {
			//uint8 *cbuffer = malloc(cur_block->compressed_size);
			uint8 *cbuffer = malloc(cur_block->size);
//dprintf("cfs_read: compressed block at %Ld\n", cur_block->data_pos);
			if(cbuffer == NULL) {
				err = B_NO_MEMORY;
				goto err;
			}
			err = cfs_read_disk(fsinfo, cur_block->data_pos, cbuffer,
			                    cur_block->compressed_size);
			if(err != B_NO_ERROR) {
				dprintf("cfs_read: could not read compressed data\n");
				goto err;
			}
			
			if(request_len < cur_block->size || cur_filepos < pos) {
				uint8 *rbuffer = malloc(cur_block->size);
				if(rbuffer == NULL) {
					err = B_NO_MEMORY;
				}
				else {
//dprintf("cfs_read: decompress %p size %ld to %p\n", cbuffer, cur_block->compressed_size, rbuffer);
					err = cfs_decompress(cbuffer, cur_block->compressed_size,
					                     rbuffer, cur_block->size);
					if(err == B_NO_ERROR) {
						readlen = request_len;
						if(readlen > cur_block->size - (pos-cur_filepos))
							readlen = cur_block->size - (pos-cur_filepos);

						memcpy(cur_buffer, rbuffer+pos-cur_filepos, readlen);
					}
					free(rbuffer);
				}
			}
			else {
//dprintf("cfs_read: decompress %p size %ld to %p\n", cbuffer, cur_block->compressed_size, cur_buffer);
				err = cfs_decompress(cbuffer, cur_block->compressed_size,
				                     cur_buffer, cur_block->size);
				if(err == B_NO_ERROR) {
					readlen = cur_block->size;
				}
			}
			
			free(cbuffer);

			if(err < B_NO_ERROR)
				goto err;
		}
		else {
			readpos = cur_block->data_pos + pos-cur_filepos;
			if(readlen > cur_block->size - (pos-cur_filepos))
				readlen = cur_block->size - (pos-cur_filepos);
			err = cfs_read_disk_etc(fsinfo, readpos, cur_buffer, &readlen, 1);
			if(err) {
				dprintf("cfs_read: could not read raw data\n");
				goto err;
			}
		}
		*len += readlen;
		cur_buffer += readlen;
		request_len -= readlen;
		pos += readlen;
	}
	return B_NO_ERROR;
err:
	return err;
}

status_t 
bt_cfs_read(void *ns, void *_node, void *cookie, off_t pos, void *buf, size_t size)
{
	status_t err;
	size_t len = size;
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	//dprintf("bt_cfs_read: file %s pos %Ld size %ld\n", node->name, pos, size);
	if (S_ISDIR(node->flags)) {
		err = EISDIR;
		goto err2;
	}

	if(!S_ISREG(node->flags)) {
		err = B_BAD_VALUE;
		goto err2;
	}

	err = cfs_read_data(fsinfo, node, pos, buf, &len);
	if(len < size)
		err = B_IO_ERROR;
err2:
	if(err != B_NO_ERROR)
		return err;
	//dprintf("bt_cfs_read: file %s pos %Ld size %ld got %ld\n", node->name, pos, size, len);
	return len;
}

static status_t
bt_cfs_ioctl(void *ns, void *node, void *cookie,
		uint32 op, void *buf, uint32 len)
{
	cfs_info *fsinfo = (cfs_info *)ns;

	switch(op) {
		case 0x01020304:
			/* private ioctl to get volume name */
			strcpy(buf, fsinfo->name);
			return B_NO_ERROR;

		case B_CFS_READ_RESERVED_BLOCK: {
			if(fsinfo->reserved_space_size < 512)
				return B_NOT_ALLOWED;
			return cfs_read_disk_uncached(fsinfo, fsinfo->reserved_space_pos,
			                             buf, 512);
		} 

#if 0
		case B_CFS_WRITE_RESERVED_BLOCK: {
			ssize_t writelen;
			if(fsinfo->reserved_space_size < 512)
				return B_NOT_ALLOWED;
			writelen = write_pos(fsinfo->dev_fd, fsinfo->reserved_space_pos,
			                     buf, 512);
			if(writelen != 512)
				return B_IO_ERROR;
			else
				return B_NO_ERROR;
		}
#endif
		default:
			return B_NOT_ALLOWED;
	}

	return ENOSYS;
}

status_t 
bt_cfs_fstat(void *ns, void *_node, void *cookie, struct stat *stat)
{
	cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;

	memset(stat, 0, sizeof(*stat));
	
	if(S_ISREG(node->flags)) {
		cfs_vblock_list *cur;
		cur = node->u.file.datablocks;
		while(cur) {
			stat->st_size += cur->size;
			cur = cur->next;
		}
		//dprintf("cfs_rstat file %s size = %Ld\n", node->name, stat->st_size);
	}

	stat->st_dev = fsinfo->nsid;
	stat->st_nlink = 1;

	stat->st_ino = node->location;
	stat->st_blksize = cfs_compressed_block_size;
	stat->st_mode = node->flags;

	stat->st_atime = node->mod_time;
	stat->st_mtime = node->mod_time;
	stat->st_ctime = node->mod_time;
	stat->st_crtime = node->create_time;

	return B_NO_ERROR;
}

ssize_t 
bt_cfs_freadlink(void *ns, void *_node, void *cookie, char *buf, size_t bufsize)
{
	//cfs_info *fsinfo = (cfs_info *)ns;
	cfs_node *node = (cfs_node *)_node;
	size_t len;

	if (!S_ISLNK(node->flags)) {
		return B_BAD_VALUE;
	}
	len = strlen(node->u.link.link) + 1;
	if(len > bufsize)
		len = bufsize;
	memcpy(buf, node->u.link.link, len);

	//dprintf("cfs_readlink: len %ld, %s -> %s\n", len, node->name, node->u.link.link);
	//return len;
	return B_NO_ERROR;
}

void
cfs_rewinddir_etc(cfs_dir_cookie *c, bool attr)
{
	c->last_offset = 0;
	c->index = attr ? 2 : 0;
	c->last_name[0] = '\0';
}

status_t
cfs_opendir_etc(cfs_node *node, void **cookie, bool attr)
{
	cfs_dir_cookie *c;
	if(!attr) {
		if(!S_ISDIR(node->flags)) {
			dprintf("cfs_opendir: %s, not a directory\n", node->name);
			return ENOTDIR;
		}
	}
	c = malloc(sizeof(cfs_dir_cookie));
	if(c == NULL)
		return B_NO_MEMORY;
	cfs_rewinddir_etc(c, attr);
	*cookie = c;
	return B_NO_ERROR;
}

status_t 
cfs_read_dir_etc(cfs_info *fsinfo, cfs_node *node, cfs_dir_cookie *c, long *num,
                 struct dirent *buf, size_t bufsize, bool attr)
{
	status_t err;

	if(!attr) {
		if(!S_ISDIR(node->flags)) {
			dprintf("cfs_read_dir: %s, not a directory\n", node->name);
			return ENOTDIR;
		}
	}
	//dprintf("cfs_read_dir_etc dir %s curr index %d\n", node->name, c->index);
	if(c->index == 0) {
		buf->d_ino = node->location;
		strcpy(buf->d_name, ".");
		buf->d_reclen = 1;
		*num = 1;
	}
	else if(c->index == 1) {
		buf->d_ino = node->parent;
		strcpy(buf->d_name, "..");
		buf->d_reclen = 2;
		*num = 1;
	}
	else {
		off_t curoffset;
		cfs_entry_info entry;
		char entry_name[CFS_MAX_NAME_LEN];
		if(c->last_offset == 0) {
			if(attr) {
				curoffset = node->attr_location;
			}
			else {
				curoffset = node->u.dir.first_entry;
			}
		}
		else {
			curoffset = c->last_offset;
			err = cfs_read_entry(fsinfo, curoffset, &entry, entry_name);
			if(err ||
			   entry.parent != node->location ||
			   strcmp(entry_name, c->last_name) != 0) {
				dprintf("cfs_read_dir: entry %Ld, parent %Ld name %s shuld be %s, scan from start\n",
				        curoffset, entry.parent, entry_name, c->last_name);
				if(attr) {
					curoffset = node->attr_location;
				}
				else {
					curoffset = node->u.dir.first_entry;
				}
			} else
				curoffset = entry.next_entry;
		}
	
		*num = 0;
		while(curoffset != 0) {
			err = cfs_read_entry(fsinfo, curoffset, &entry, entry_name);
			if(err)
				goto err;
			
			if(entry.parent != node->location) {
				dprintf("cfs_read_dir: entry %Ld, parent %Ld shuld be %Ld\n",
				        curoffset, entry.parent, node->location);
				err = B_IO_ERROR;
				goto err;
			}
			if(strcmp(entry_name, c->last_name) > 0) {
				strcpy(c->last_name, entry_name);
				c->last_offset = curoffset;
	
				buf->d_ino = curoffset;
				strcpy(buf->d_name, entry_name);
				buf->d_reclen = strlen(entry_name);
				*num = 1;
				break;
			}
			curoffset = entry.next_entry;
		}
	}
	c->index++;
	if(*num == 0)
		err = B_ENTRY_NOT_FOUND;
	else
		err = B_NO_ERROR;
err:
	return err;
}

status_t 
bt_cfs_opendir(void *ns, void *node, void **cookie)
{
	return cfs_opendir_etc((cfs_node *)node, cookie, false);
}

status_t 
bt_cfs_closedir(void *ns, void *node, void *cookie)
{
	free(cookie);
	return B_NO_ERROR;
}

status_t 
bt_cfs_rewinddir(void *ns, void *node, void *cookie)
{
	cfs_rewinddir_etc((cfs_dir_cookie *)cookie, false);
	return B_NO_ERROR;
}

status_t 
bt_cfs_readdir(void *ns, void *node, void *cookie, struct dirent *buf)
{
	long num;
	size_t bufsize = sizeof(struct dirent);
	return cfs_read_dir_etc((cfs_info *)ns, (cfs_node *)node,
	                        (cfs_dir_cookie *)cookie, &num, buf, bufsize, false);
}

struct fs_ops bt_cfs_ops = {
	bt_cfs_mount,
	bt_cfs_unmount,

	bt_cfs_walk,
	bt_cfs_read_vnode,
	bt_cfs_write_vnode,
	
	bt_cfs_open,
	bt_cfs_close,
	bt_cfs_read,
	bt_cfs_ioctl,
	bt_cfs_fstat,
	bt_cfs_freadlink,
	
	bt_cfs_opendir,
	bt_cfs_closedir,
	bt_cfs_readdir,
	bt_cfs_rewinddir
};
