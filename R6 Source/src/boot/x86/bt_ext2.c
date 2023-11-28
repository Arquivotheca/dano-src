#include <drivers/KernelExport.h>
#include <drivers/Drivers.h>

#if USER
	#define dprintf printf
#endif

#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <boot.h>
#include <fmap.h>
#include "bt_ext2.h"

#if !USER
	#include "bt_fs.h"
#else
	typedef long long vnode_id;
#endif

#define ROOTINODE 2

struct ext2_fmap_block_run {
	uint32 block;
	uint32 num_blocks;
};

struct ext2_fmap_info
{
	uint32 block_size;
	uint32 num_block_runs;
	uint32 fmap_alloc_length; // this stores the allocated length of the fmap structure measured in runs
	struct ext2_fmap_block_run block_runs[1];
}; 


// filesystem info struct
typedef struct {
	int fd;
	nspace_id nsid;
	uint32 sector_size;
	uint32 num_sectors;
	ext2_super_block sb;
	ext2_group_desc *gd;
	
	uint32 s_block_size;
	uint32 s_inodes_per_block;
	uint32 s_groups_count;	
	
	uint32 num_direct_ptrs;
	uint32 num_1st_indirect_ptrs;
	uint32 num_2nd_indirect_ptrs;
	uint32 num_3rd_indirect_ptrs;
} ext2_fs_struct;

typedef struct {
	uint32 vnid;
	ext2_inode inode;
	struct ext2_fmap_info *fmap;
//	int fmap_alloc_length;
} ext2_vnode;

typedef struct {
	uint32 next_entry;
		
	// Stuff to deal with a dir buffer
	uint8 *dir_buf_cache;
	uint32 cached_dir_buf_block;
} ext2_dcookie;
	
// Function declarations
static status_t bt_ext2_mount(nspace_id nsid, const char *device, uint32 flags, void *parms, int len, void **ns, vnode_id *root);
static status_t bt_ext2_unmount(void *ns);
static status_t bt_ext2_walk(void *ns, void *dir, const char *file, char *newpath, vnode_id *vnid);
static status_t bt_ext2_readvnode(void *ns, vnode_id vnid, void **node);
static status_t bt_ext2_writevnode(void *ns, void *node);
static status_t bt_ext2_open(void *ns, void *node, void **cookie);
static status_t bt_ext2_close(void *ns, void *node, void *cookie);
static ssize_t bt_ext2_read(void *ns, void *node, void *cookie, off_t pos, void *buffer, size_t len);
static status_t bt_ext2_ioctl(void *ns, void *node, void *cookie, uint32 op, void *buffer, uint32 size);
static status_t bt_ext2_fstat(void *ns, void *node, void *cookie, struct stat *st);
static ssize_t bt_ext2_freadlink(void *ns, void *node, void *cookie, char *buf, size_t len);
static status_t bt_ext2_opendir(void *ns, void *node, void **cookie);
static status_t bt_ext2_closedir(void *ns, void *node, void *cookie);
static status_t bt_ext2_readdir(void *ns, void *node, void *cookie, struct dirent *de);
static status_t bt_ext2_rewinddir(void *ns, void *node, void *cookie);

static status_t bt_ext2_load_fmap_indirect(ext2_fs_struct *ext2, ext2_vnode *v, uint32 *ptr_block, uint32 *ptrs_to_process, int level, uint32 *curr_ptr);
static status_t bt_ext2_load_fmap(ext2_fs_struct *ext2, ext2_vnode *v);
static status_t bt_ext2_find(ext2_fs_struct *ext2, ext2_vnode *v, const char *path, vnode_id *vnode_num);
static status_t bt_ext2_add_block_to_fmap(ext2_fs_struct *ext2, ext2_vnode *v, uint32 block);
static status_t bt_ext2_get_block_from_fmap(struct ext2_fmap_info *fmap, uint32 *block, uint32 block_in_fmap);

// Feature checking macros
#define USING_FILETYPES_FEATURE(ext2) (((ext2)->sb.s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE) != 0)


static status_t bt_ext2_mount(nspace_id nsid, const char *device, uint32 flags, void *parms, int len, void **ns, vnode_id *root)
{
	ext2_fs_struct *ext2;
	ssize_t retval;
	int fd;
	status_t err;
	int gd_start;
	int gd_len;
	partition_info p;


//	dprintf("bt_ext2_mount called on device '%s'.\n", device);
	
	// Open the device
	fd = open(device, 0);
	if (fd < 0) {
//		dprintf("bt_ext2_mount: can't open '%s'\n", device);
		return fd;
	}

	// Allocate the filesystem structure
	ext2 = malloc(sizeof(ext2_fs_struct));
	if(!ext2) {
//		dprintf("ext2 mount: couldn't allocate space for ext2 filesystem structure.\n");
		close(fd);
		return ENOMEM;
	}
	
	// Get the partition info
	err = ioctl(fd, B_GET_PARTITION_INFO, &p, sizeof(partition_info));
	if(err < B_NO_ERROR) {
		free(ext2);
		close(fd);
		return EINVAL;
	}
	
//	dprintf("partition info:\n");
//	dprintf("offset = %Ld size = %Ld logical block size = %ld.\n", p.offset, p.size, p.logical_block_size);

	ext2->fd = fd;
	ext2->sector_size = 512; // XXX - can we check for this?
	ext2->nsid = nsid;
	
	// read in the superblock
	retval = read_pos(fd, 1024, &(ext2->sb), 1024);
	if(retval != 1024) {	
//		dprintf("ext2_mount had error reading in superblock.\n");
		err = EIO;
		goto error;
	}
	
	// Check to make sure the magic number is right
	if(ext2->sb.s_magic != 0xEF53) {
		// Must not be a linux partition, abort
//		dprintf("ext2_mount superblock's magic number doesn't match, read 0x%x.\n", ext2->sb.s_magic);
		err = EINVAL;
		goto error;
	}

	// If this volume isn't a dynamic rev volume, anything from s_first_ino
	// to the end of the superblock are going to be unused, so lets make
	// sure they are zeroed out, then we can still check the feature flags,
	// which will be zero, or no feature.
	if(ext2->sb.s_rev_level < EXT2_DYNAMIC_REV) {
		memset(&ext2->sb.s_first_ino, 0, sizeof(ext2_super_block) - 0x54);
	}

	// If the volume revision is greater than what we know, bail
	if(ext2->sb.s_rev_level > EXT2_DYNAMIC_REV) {
		dprintf("ext2fs volume revision greater than we know on device %s.\n", device);
		err = EINVAL;
		goto error;
	}
	
	// Check for RO incompatible flags
	// if, after masking out the ones we can handle, there are bits left in the flag,
	// don't mount it.
	if((ext2->sb.s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPP) != 0) {
		dprintf("incompatible ext2fs features enabled on device %s.\n", device);
		err = EINVAL;
		goto error;
	}

	// Check the dirty flag
	if((ext2->sb.s_state & EXT2_ERROR_FS) || (!(ext2->sb.s_state & EXT2_VALID_FS))) {
		// It's not clean
		dprintf("found dirty ext2fs volume on device %s. aborting mount...\n", device);
		err = EINVAL;
		goto error;
	}
	
	// It's read in, now lets calculate some filesystem constants and do some boundary checks
	ext2->s_block_size = (1024 << ext2->sb.s_log_block_size);
	if((ext2->s_block_size != 0x400) && (ext2->s_block_size != 0x800) && (ext2->s_block_size != 0x1000)) {
//		dprintf("ext2_mount block_size is invalid with size 0x%x.\n", (unsigned int)ext2->s_block_size);
		err = EINVAL;
		goto error;
	}
	
	if((ext2->sb.s_blocks_count * ext2->s_block_size) > (p.size * p.logical_block_size)) {
		err = EINVAL;
		goto error;
	}
	
	ext2->s_inodes_per_block = ext2->s_block_size / sizeof(ext2_inode);
	
	if((ext2->sb.s_inodes_per_group == 0) || (ext2->sb.s_inodes_per_group > ext2->sb.s_blocks_per_group)) {
		err = EINVAL;
		goto error;
	}

	if(ext2->sb.s_blocks_per_group == 0) {
		err = EINVAL;
		goto error;
	}
	
	ext2->s_groups_count = ext2->sb.s_blocks_count / ext2->sb.s_blocks_per_group;
	if((ext2->sb.s_blocks_count % ext2->sb.s_blocks_per_group) > 0) {
		// There are a few blocks in the last group, so one more group
		ext2->s_groups_count++;
	}
	ext2->num_sectors = ext2->sb.s_blocks_count * (2 << ext2->sb.s_log_block_size);

	// Calculate how many block pointers are stored in the direct and indirect block pointers
	ext2->num_direct_ptrs = EXT2_NDIR_BLOCKS;
	ext2->num_1st_indirect_ptrs = ext2->s_block_size / sizeof(uint32);
	ext2->num_2nd_indirect_ptrs = ext2->num_1st_indirect_ptrs * ext2->num_1st_indirect_ptrs;
	ext2->num_3rd_indirect_ptrs = ext2->num_2nd_indirect_ptrs * ext2->num_1st_indirect_ptrs;

	// Need to read in the group descriptors
//	dprintf("bt_ext2_mount reading group descriptors, count = %d.\n", ext2->s_groups_count);
	gd_len = sizeof(ext2_group_desc) * ext2->s_groups_count;
	ext2->gd = (ext2_group_desc *)malloc(gd_len);
	if(!ext2->gd) {
//		dprintf("ext2_mount: error allocating memory for group descriptors.\n");
		err = ENOMEM;
		goto error;
	}
	// calculate the position of the first group descriptor table
	if(ext2->s_block_size == 4096) {
		gd_start = 4096;
	} else {
		gd_start = 2048;
	}

	// read it in
	err = read_pos(ext2->fd, gd_start, ext2->gd, gd_len);
	if(err != gd_len) {
		dprintf("ext2_mount: error loading group descriptors from device %s\n", device);
		err = EBADF;
		goto error1;
	}

//	dprintf("ext2_mount done.\n");

	*ns = ext2;
	*root = ROOTINODE;

	return B_NO_ERROR;

error1:
	free(ext2->gd);
error:
//	dprintf("ext2_mount exiting with error.\n");
	close(ext2->fd);
	free(ext2);
	return err;
}

static status_t bt_ext2_unmount(void *ns)
{
	ext2_fs_struct *ext2 = (ext2_fs_struct *)ns;
	
//	dprintf("bt_ext2_unmount entry.\n");
	
	if(ext2) {
		if(ext2->gd) free(ext2->gd);
		close(ext2->fd);
		free(ext2);
	}
	
	return B_OK;	
}

static status_t bt_ext2_open(void *ns, void *node, void **cookie)
{
	*cookie = NULL;

	return B_NO_ERROR;
}

static status_t bt_ext2_close(void *ns, void *node, void *cookie)
{
	return B_NO_ERROR; 
}

static status_t bt_ext2_fstat(void *ns, void *node, void *cookie, struct stat *st)
{
	ext2_vnode *v = (ext2_vnode *)node;

	st->st_ino = v->vnid;
	st->st_mode = v->inode.i_mode;
	st->st_size = v->inode.i_size;
	st->st_mtime = v->inode.i_mtime;

	return B_NO_ERROR; 
}

static ssize_t bt_ext2_freadlink(void *ns, void *node, void *cookie, char *buf, size_t len)
{
	ext2_vnode *v = (ext2_vnode *)node;
	ext2_fs_struct *ext2 = (ext2_fs_struct *)ns;

	// Check to see if the buffer is big enough
	if(v->inode.i_size > len) {
		// The buffer is too small
		return 0;
	}

	if(v->inode.i_size > ext2->s_block_size) return 0;

	// Check for fast symlink and process accordingly
	if(v->inode.i_size <= sizeof(int)*EXT2_N_BLOCKS) {
		memcpy(buf, v->inode.i_block, v->inode.i_size);
	} else {
		// This must be a slow symbolic link
		// Read in the first block, this contains the full path

		// Check for bad block. 
		if((v->inode.i_block[0] == 0)) {
			return 0;
		}
		if(read_pos(ext2->fd, (off_t)v->inode.i_block[0] * ext2->s_block_size, buf, v->inode.i_size) != v->inode.i_size) {
			return 0;
		}
	} 
		
	return v->inode.i_size;
}

static ssize_t bt_ext2_read(void *ns, void *node, void *cookie, off_t pos, void *buffer, size_t len)
{
	ext2_vnode *v = (ext2_vnode *)node;
	ext2_fs_struct *ext2 = (ext2_fs_struct *)ns;
	ssize_t size_read;
	ssize_t bytes_read=0;
	uint32 fs_block;
	uint32 block;
	uint32 offset;
	status_t err;
	uchar *buf_pos = (uchar *)buffer;
	size_t left_to_read;
	size_t length_to_read;

//	dprintf("ext2_read called on vnode %ld, position %Ld, len %ld.\n", v->vnid, pos, len);
	
	// Check to make sure we have a real file
	if(!S_ISREG(v->inode.i_mode)) return 0;

	// Check to see if we're not off the end of the file
	if((pos >= v->inode.i_size) || (pos < 0)) return 0;

	// Check to see if we have a fmap
	if(!v->fmap) return 0;
	
	// trim the read so it doesn't read off the end of the file
	left_to_read = min(len, v->inode.i_size - pos);
	
	// Calculate the starting block into the file to read.
	block = pos / ext2->s_block_size;
	// Calculate the offset into the block to read
	offset = pos % ext2->s_block_size;

	while(left_to_read > 0) {
		length_to_read = min(left_to_read, ext2->s_block_size - offset);
		err = bt_ext2_get_block_from_fmap(v->fmap, &fs_block, block++);
		if(err < B_NO_ERROR) {
			return 0;
		}
		if(fs_block > ext2->sb.s_blocks_count) {
			return 0;
		}
		if(fs_block != 0) {
//			dprintf("ext2_read reading at block %ld offset %ld length %ld.\n", fs_block, offset, length_to_read);
			size_read = read_pos(ext2->fd, (off_t)fs_block * ext2->s_block_size + offset, buf_pos, length_to_read);
			if(size_read != length_to_read) {
				return 0;
			}
		} else {
			// we aint doin sparse files here
			return 0;
		}  
		offset = 0;
		left_to_read -= length_to_read;
		bytes_read += length_to_read;
		buf_pos += length_to_read;
	}

	return bytes_read;
}

static status_t bt_ext2_ioctl(void *ns, void *node, void *cookie, uint32 op, void *buffer, uint32 size)
{
	ext2_fs_struct *ext2 = (ext2_fs_struct *)ns;
	ext2_vnode *v = (ext2_vnode *)node;

//	dprintf("ext2_ioctl entry.\n");

	if((op == 'fmap') || (op == 'dmap')) {
		// Convert our fmap info to the real one
		int32 i;
		uint32 new_fmap_len;
		struct fmap_info *fm = NULL;
		int fmap_type_to_use;
		int new_blocks_per_ext2_block = ext2->s_block_size / 512;
		
//		dprintf("ext2_ioctl: returning fmap.\n");
		
		if(!v->fmap) {
			fm = malloc(sizeof(struct fmap_info));
			if(!fm) return ENOMEM;
			fm->size = sizeof(struct fmap_info) - sizeof(struct fmap_byte_run);
			fm->checksum = 0;
			fm->type = FMAP_TYPE_BYTE;
			fm->bios_id = 0xff;
			fm->offset = 0;
			fm->u.byte.num_bytes = 0;
			fm->u.byte.num_byte_runs = 0;
			*(struct fmap_info **)buffer = fm;
			return B_OK;
		}
		
		// Lets figure out which fmap version to use
		fmap_type_to_use = FMAP_TYPE_BLOCK; // we want to use the small one first
		for(i=0; i<v->fmap->num_block_runs; i++) {
			// See if any of the blocks numbers, converted to 512 byte lengths, are > MAX(int32)
			// or, if this, plus the block run length, reference a block > MAX(int32)
			uint64 start_block = (uint64)v->fmap->block_runs[i].block * new_blocks_per_ext2_block;
			uint64 end_block_run = start_block + (uint64)v->fmap->block_runs[i].num_blocks * new_blocks_per_ext2_block - new_blocks_per_ext2_block; 
			if((start_block > INT_MAX) || (end_block_run > INT_MAX)) {
				fmap_type_to_use = FMAP_TYPE_BYTE;
				break;
			}
		}
		
		switch(fmap_type_to_use) {
			case FMAP_TYPE_BLOCK:
				new_fmap_len = sizeof(struct fmap_info) + (v->fmap->num_block_runs - 1) * sizeof(struct fmap_block_run);

				fm = malloc(new_fmap_len);
				if(!fm) return ENOMEM;

//				dprintf("fmap dump:\n");	
				// Convert our fmap to the other guys fmap		
				for(i=0; i<v->fmap->num_block_runs; i++) {
					fm->u.block.block_runs[i].block = v->fmap->block_runs[i].block * new_blocks_per_ext2_block;
					fm->u.block.block_runs[i].num_blocks = v->fmap->block_runs[i].num_blocks * new_blocks_per_ext2_block;
//					dprintf("start_block = %ld, run_length = %ld.\n", fm->u.block.block_runs[i].block, fm->u.block.block_runs[i].num_blocks);
				}
				
				fm->offset = 0;
				fm->u.block.block_size = 512;
				fm->u.block.num_block_runs = v->fmap->num_block_runs;
				fm->type = FMAP_TYPE_BLOCK;
				fm->size = new_fmap_len;
				
				// See if we need to truncate the last block run to account for a partial ext2 block
				{
					uint32 last_block_size = v->inode.i_size % ext2->s_block_size;
					
					if(last_block_size) {
						fm->u.block.block_runs[fm->u.block.num_block_runs-1].num_blocks -=
							new_blocks_per_ext2_block - last_block_size / 512;
//						dprintf("ext2_ioctl the last block is %ld bytes long.\n", last_block_size);
//						dprintf("ext2_ioctl subtracting %d blocks from the last run.\n", new_blocks_per_ext2_block - last_block_size / 512);
						if(fm->u.block.block_runs[fm->u.block.num_block_runs-1].num_blocks <= 0) {
							fm->u.block.num_block_runs--;
							fm->size -= sizeof(struct fmap_block_run);
						}
					}
				}

				// figure the number of blocks 
				{
					fm->u.block.num_blocks = 0;
					for (i=0;i<fm->u.block.num_block_runs;i++)
						fm->u.block.num_blocks += fm->u.block.block_runs[i].num_blocks;						
				}

				break;
			case FMAP_TYPE_BYTE:
				new_fmap_len = sizeof(struct fmap_info) + (v->fmap->num_block_runs - 1) * sizeof(struct fmap_byte_run);

				fm = malloc(new_fmap_len);
				if(!fm) return ENOMEM;
				
//				dprintf("fmap dump:\n");	
				// Convert our fmap to the other guys fmap		
				for(i=0; i<v->fmap->num_block_runs; i++) {
					fm->u.byte.byte_runs[i].byte = (off_t)v->fmap->block_runs[i].block * ext2->s_block_size;
					fm->u.byte.byte_runs[i].num_bytes = (off_t)v->fmap->block_runs[i].num_blocks * ext2->s_block_size;
//					dprintf("start byte = %Ld, run_length = %Ld.\n", fm->u.byte.byte_runs[i].byte, fm->u.byte.byte_runs[i].num_bytes);
				}
				fm->offset = 0;
				fm->u.byte.num_byte_runs = v->fmap->num_block_runs;
				fm->type = FMAP_TYPE_BYTE;
				fm->size = new_fmap_len;

				// See if we need to truncate the last byte run to account for a partial ext2 block
				{
					uint32 last_block_size = v->inode.i_size % ext2->s_block_size;
					
					if(last_block_size) {
						fm->u.byte.byte_runs[fm->u.byte.num_byte_runs-1].num_bytes -=
							ext2->s_block_size - (last_block_size / 512) * 512;
						if(fm->u.byte.byte_runs[fm->u.byte.num_byte_runs-1].num_bytes == 0) {
							fm->u.byte.num_byte_runs--;
							fm->size -= sizeof(struct fmap_byte_run);
						}
					}
				}

				// figure the number of bytes 
				{
					fm->u.byte.num_bytes = 0;
					for (i=0;i<fm->u.byte.num_byte_runs;i++)
						fm->u.byte.num_bytes += fm->u.byte.byte_runs[i].num_bytes;						
				}

				break;
		}									
		*(struct fmap_info **)buffer = fm;
		
		return B_OK;
	}

	return ENOSYS;
}

static status_t bt_ext2_opendir(void *ns, void *node, void **cookie)
{
	ext2_vnode *v = (ext2_vnode *)node;
	ext2_dcookie *dcookie;

	// Make sure this is a directory
	if(!S_ISDIR(v->inode.i_mode)) {
//		dprintf("ext2_opendir: vnode %ld is not a directory. mode = 0x%x.\n", v->vnid, v->inode.i_mode);
		return EINVAL;
	}

	dcookie = (ext2_dcookie *)malloc(sizeof(ext2_dcookie));
	if(!dcookie) {
		return ENOMEM;
	}

	dcookie->next_entry = 0;
	
	dcookie->dir_buf_cache = NULL;
	dcookie->cached_dir_buf_block = 0;

	*cookie = dcookie;

	return B_NO_ERROR;
}

static status_t bt_ext2_closedir(void *ns, void *node, void *cookie)
{
	ext2_dcookie *d = (ext2_dcookie *)cookie;

	if(d) {
		if(d->dir_buf_cache) free(d->dir_buf_cache);
		free(d);
	}

	return B_NO_ERROR;
}

static status_t bt_ext2_readdir(void *ns, void *node, void *cookie, struct dirent *de)
{
	ext2_fs_struct *ext2 = (ext2_fs_struct *)ns;
	ext2_vnode *v = (ext2_vnode *)node;
	ext2_dcookie *d = (ext2_dcookie *)cookie;

	status_t err;
	ssize_t read_size;
	uint32 file_block;
	uint32 offset;
	uint32 fs_block;
	
//	dprintf("bt_ext2_readdir entry on vnode %ld.\n", v->vnid);
	
	// see if we're at the end of the directory
	if(d->next_entry >= v->inode.i_size) return B_ERROR;

	// Calculate where we are
	file_block = d->next_entry / ext2->s_block_size;
	offset = d->next_entry % ext2->s_block_size;	

	// See if we don't have a cached copy of this dir block
	if((!d->dir_buf_cache) || (d->cached_dir_buf_block != file_block)) {
		// Allocate temporary buffer space in the dircookie, if it isn't already
		if(!d->dir_buf_cache) {
			d->dir_buf_cache = malloc(ext2->s_block_size);
			if(!d->dir_buf_cache) return ENOMEM;
		}
	
		err = bt_ext2_get_block_from_fmap(v->fmap, &fs_block, file_block);
		if(err < B_NO_ERROR) goto error;
		
	//	dprintf("reading block %ld.\n", fs_block);
		
		// Read in the block
		read_size = read_pos(ext2->fd, (off_t)fs_block * ext2->s_block_size, d->dir_buf_cache, ext2->s_block_size);
		if(read_size != ext2->s_block_size) goto error;
	
		// Remember what we just read
		d->cached_dir_buf_block = file_block;
	}
	
	// If using the filetypes feature, the directory structure is slightly different	
	if(USING_FILETYPES_FEATURE(ext2)) {
		ext2_dir_entry_2 *e2de2;

		// Overlay the ext2 directory structure
		e2de2 = (ext2_dir_entry_2 *)(d->dir_buf_cache + offset);

		// Do some directory sanity checks
		if((e2de2->inode == 0) || (e2de2->inode >= ext2->sb.s_inodes_count)) goto error;
		if((e2de2->rec_len == 0) || (e2de2->name_len == 0)) goto error;
		if((e2de2->rec_len + offset) > ext2->s_block_size) goto error;
		if(((uint16)e2de2->name_len + 8) > e2de2->rec_len) goto error;

		// Copy the data
		de->d_ino = e2de2->inode;
		memcpy(de->d_name, e2de2->name, e2de2->name_len);
		de->d_name[e2de2->name_len] = 0;
	
//		dprintf("found entry 2 '%s'\n", de->d_name);
	
		// Move the pointer up
		d->next_entry += e2de2->rec_len;
	} else {
		ext2_dir_entry *e2de;

		// Overlay the ext2 directory structure
		e2de = (ext2_dir_entry *)(d->dir_buf_cache + offset);

		// Do some directory sanity checks
		if((e2de->inode == 0) || (e2de->inode >= ext2->sb.s_inodes_count)) goto error;
		if((e2de->rec_len == 0) || (e2de->name_len == 0)) goto error;
		if((e2de->rec_len + offset) > ext2->s_block_size) goto error;
		if((e2de->name_len + 8) > e2de->rec_len) goto error;

		// Copy the data
		de->d_ino = e2de->inode;
		memcpy(de->d_name, e2de->name, e2de->name_len);
		de->d_name[e2de->name_len] = 0;
	
//		dprintf("found entry '%s'\n", de->d_name);
	
		// Move the pointer up
		d->next_entry += e2de->rec_len;
	}

	return B_OK;

error:
//	dprintf("ext2: exiting readdir with error.\n");
	return B_ERROR;		
	
}

static status_t bt_ext2_rewinddir(void *ns, void *node, void *cookie)
{
	ext2_dcookie *d = (ext2_dcookie *)cookie;

	d->next_entry = 0;

	return B_NO_ERROR;
}


static status_t
bt_ext2_get_block_from_fmap(struct ext2_fmap_info *fmap, uint32 *block, uint32 block_in_fmap)
{
	int i;
	uint32 blocks_left = block_in_fmap;
	
	for(i=0; i<fmap->num_block_runs; i++) {
//		dprintf("looking at run %d: start = %Ld, length = %ld.\n", i, fmap->u.block.block_runs[i].block, fmap->u.block.block_runs[i].num_blocks);
		if(blocks_left < fmap->block_runs[i].num_blocks) {
			// It's in this block run
			*block = fmap->block_runs[i].block + blocks_left;
			return B_NO_ERROR;
		}	
		blocks_left -= fmap->block_runs[i].num_blocks;
	}

	return B_ERROR;
}

static status_t bt_ext2_add_block_to_fmap(ext2_fs_struct *ext2, ext2_vnode *v, uint32 block)
{
	
//	dprintf("bt_ext2_add_block_to_fmap called. Adding block %d.\n", block);

	// check to see if the fmap exists
	if(!v->fmap) {
		// We need to allocate the space for the fmap
		v->fmap = malloc(sizeof(struct ext2_fmap_info) + sizeof(struct ext2_fmap_block_run) * (16 - 1));
		if(!v->fmap) {
			dprintf("bt_ext2_add_block_to_fmap couldn't allocate space for new fmap.\n");
			return ENOMEM;
		}
		memset(v->fmap, 0, sizeof(struct ext2_fmap_info) + sizeof(struct ext2_fmap_block_run) * (16 - 1));
		v->fmap->block_size = ext2->s_block_size;
		v->fmap->num_block_runs = 1;
		v->fmap->fmap_alloc_length = 16;
	}	
	
	// Add the block to the fmap
	if(v->fmap->block_runs[v->fmap->num_block_runs-1].num_blocks == 0) {
		v->fmap->block_runs[v->fmap->num_block_runs-1].block = block;
	} else {
		// See if we need to go to the next run
		if((v->fmap->block_runs[v->fmap->num_block_runs-1].block + v->fmap->block_runs[v->fmap->num_block_runs-1].num_blocks) != block) {
			// we need a new run
			if((v->fmap->num_block_runs + 1) > v->fmap->fmap_alloc_length) {
				// we also need a new fmap structure
				struct ext2_fmap_info *temp_fmap = malloc(sizeof(struct ext2_fmap_info) + sizeof(struct ext2_fmap_block_run) * (v->fmap->fmap_alloc_length * 2 - 1));
				if(!temp_fmap) {
//					dprintf("bt_ext2_add_block_to_fmap had problem allocating memory for expanded fmap.\n");
					return ENOMEM;
				}
				// copy the old structure over
				memcpy(temp_fmap, v->fmap, sizeof(struct ext2_fmap_info) + sizeof(struct ext2_fmap_block_run) * (v->fmap->fmap_alloc_length - 1));
				// free the old structure & update things
				free(v->fmap);
				v->fmap = temp_fmap;
				v->fmap->fmap_alloc_length *= 2;
			}
			// erase the new run
			memset(&v->fmap->block_runs[v->fmap->num_block_runs], 0, sizeof(struct ext2_fmap_block_run));
			// update things
			v->fmap->num_block_runs++;
			// add the first block to this run
			v->fmap->block_runs[v->fmap->num_block_runs-1].block = block;
		}
	}
						
	v->fmap->block_runs[v->fmap->num_block_runs-1].num_blocks++; 

	return B_NO_ERROR;
}

static status_t bt_ext2_load_fmap_indirect(ext2_fs_struct *ext2, ext2_vnode *v, uint32 *ptr_block, uint32 *ptrs_to_process, int level, uint32 *curr_ptr)
{
	int i;
	uint32 ptr;
	status_t err = B_NO_ERROR;
	ssize_t read_val;
	uint32 *ptr_block1 = NULL;
	
	if(level == 1) {
		// we're looking at the last leaf of a indirect block list
		dprintf(".");
		for(i=0; i<ext2->num_1st_indirect_ptrs; i++) {
			ptr = ptr_block[i];
			if((ptr == 0) || (ptr >= ext2->sb.s_blocks_count)) {
				err = B_ERROR;
				goto out;
			}
			// Add the block to the run
			bt_ext2_add_block_to_fmap(ext2, v, ptr);
			(*ptrs_to_process)--;
			(*curr_ptr)++;
			if(*ptrs_to_process <= 0) break;
		}
	} else {
		// We have more levels to go
		uint32 ptr;
		uint32 block_num;
		
		// Allocate the space to store the pointer block
		ptr_block1 = malloc(ext2->num_1st_indirect_ptrs * sizeof(uint32));
		if(!ptr_block1) {
			err = ENOMEM;
			goto out;
		}
	
		for(block_num = 0; block_num < ext2->num_1st_indirect_ptrs; block_num++) {
			ptr = ptr_block[block_num];
			if((ptr == 0) || (ptr >= ext2->sb.s_blocks_count)) {
				err = B_ERROR;
				goto out;
			}
//			dprintf("looking at pointer to block %ld\n", ptr);
			read_val = read_pos(ext2->fd, (off_t)ptr * ext2->s_block_size, ptr_block1, ext2->s_block_size);		
			if(read_val != ext2->s_block_size) {
				err = B_ERROR;
				goto out;
			}
			err = bt_ext2_load_fmap_indirect(ext2, v, ptr_block1, ptrs_to_process, level - 1, curr_ptr);
			if(err < B_NO_ERROR) {
				err = B_ERROR;
				goto out;
			}
			if(*ptrs_to_process <= 0) break;
		}
	}

out:
	if(ptr_block1) free(ptr_block1);
	return err;
}

static status_t bt_ext2_load_fmap(ext2_fs_struct *ext2, ext2_vnode *v)
{		
	status_t err;
	uint32 ptrs_to_process;
	uint32 curr_ptr = 0;
	uint32 ptr;
	uint32 level;	
	uint32 *ptr_block = NULL;
			
//	dprintf("bt_ext2_load_fmap entry.\n");		

	dprintf("ext2 loading fmap.");	

	ptrs_to_process = v->inode.i_size / ext2->s_block_size;
	if(v->inode.i_size % ext2->s_block_size) ptrs_to_process++;

//	dprintf("bt_ext2_load_fmap sez ptrs_to_process = %ld.\n", ptrs_to_process);	

	while(ptrs_to_process > 0) {
		if(curr_ptr < ext2->num_direct_ptrs) {
//			dprintf("inside direct block zone.\n");
			level = 0;
			ptr = v->inode.i_block[curr_ptr];
			if((ptr == 0) || (ptr >= ext2->sb.s_blocks_count)) {
				err = EINVAL;
				goto error;
			}
//			dprintf("looking at pointer to block %ld\n", ptr);
			// Add the block to the run
			dprintf(".");
			bt_ext2_add_block_to_fmap(ext2, v, ptr);
			ptrs_to_process--;
			curr_ptr++;
		} else if(curr_ptr < ext2->num_1st_indirect_ptrs) {
			level = 1;
		} else if(curr_ptr < ext2->num_2nd_indirect_ptrs) {
			level = 2;
		} else if(curr_ptr < ext2->num_3rd_indirect_ptrs) {
			level = 3;
		} else {
			err = EINVAL;			
			goto error;
		}		
		if(level > 0) {
			ssize_t read_len;
			uint32 block_num = v->inode.i_block[EXT2_IND_BLOCK + level - 1];

			// Allocate the space to store the pointer block
			ptr_block = malloc(ext2->num_1st_indirect_ptrs * sizeof(uint32));
			if(!ptr_block) {
				err = ENOMEM;
				goto error;
			}

//			dprintf("inside %ld level.\n", level);

			if((block_num == 0) || (block_num >= ext2->sb.s_blocks_count)) {
				err = EINVAL;
				goto error;
			}

			read_len = read_pos(ext2->fd, (off_t)block_num * ext2->s_block_size, ptr_block, ext2->s_block_size);
			if(read_len != ext2->s_block_size) {
				err = EIO;
				goto error;
			}		
			err = bt_ext2_load_fmap_indirect(ext2, v, ptr_block, &ptrs_to_process, level, &curr_ptr);
			if(err != B_NO_ERROR) {
				goto error;
			}
		}
	}	

//	dprintf("bt_ext2_load_fmap exit.\n");		

	dprintf("done\n");

	return B_NO_ERROR;

error:	
//	dprintf("ext2: exiting load_fmap with error.\n");
	if(ptr_block) free(ptr_block);
	return err;	
}

static status_t bt_ext2_walk(void *ns, void *dir, const char *file, char *newpath, vnode_id *vnid)
{
	ext2_fs_struct *ext2 = (ext2_fs_struct *)ns;
	ext2_vnode *v = (ext2_vnode *)dir;

	// Lets find the file
	return bt_ext2_find(ext2, v, file, vnid);
}

static status_t bt_ext2_readvnode(void *ns, vnode_id vnid, void **node)
{
	ext2_fs_struct *ext2 = (ext2_fs_struct *)ns;
	ext2_vnode *v;
	unsigned int block, offset;
	ssize_t retval;
	status_t err;

//	dprintf("bt_ext2_readvnode called on node %Ld.\n", vnid);

	// Allocate the space for the new vnode
	v = malloc(sizeof(ext2_vnode));
	if(!v) return ENOMEM;

	// Initialize the new vnode
	memset(v, 0, sizeof(ext2_vnode));
	v->vnid = vnid;

	// Calculate the block and offset
	vnid--;
	block = ext2->gd[vnid / ext2->sb.s_inodes_per_group].bg_inode_table;
	block += (vnid % ext2->sb.s_inodes_per_group) / ext2->s_inodes_per_block;
	offset = (vnid % ext2->s_inodes_per_block) * sizeof(ext2_inode);

	if((block == 0) || (block >= ext2->sb.s_blocks_count)) {
		err = EINVAL;
		goto error;
	}

//	dprintf("bt_ext2_readvnode reading vnode %Ld, in group %ld, found at block %d, offset %d.\n",
//		vnid+1, vnid / ext2->sb.s_inodes_per_group, block, offset);

	// read the vnode
	retval = read_pos(ext2->fd, (off_t)block * ext2->s_block_size + offset, &v->inode, sizeof(ext2_inode));
	if(retval != sizeof(ext2_inode)) {
		err = EIO; 
		goto error;
	}
	
	// do some verification of the inode, can't do too much
	if(v->inode.i_links_count == 0) {
		err = EINVAL;
		goto error;
	}
		
	// Now load the fmap
	err = bt_ext2_load_fmap(ext2, v);
	if(err < B_NO_ERROR) {
		goto error;
	}
	
//	dprintf("bt_ext2_readvnode file size %d.\n", v->inode.i_size);
//	dprintf("bt_ext2_readvnode file mode is 0x%x.\n", v->inode.i_mode);

	*node = v;

	return B_NO_ERROR;

error:
//	dprintf("ext2: exiting readvnode with an error.\n");
	if(v) {
		if(v->fmap) free(v->fmap);
		free(v);
	}
	return err;
}

static status_t bt_ext2_writevnode(void *ns, void *node)
{
	ext2_vnode *v = (ext2_vnode *)node;

	if(v) {
		if(v->fmap) free(v->fmap);
		free(v);
	}

	return B_NO_ERROR;
}

static status_t bt_ext2_find(ext2_fs_struct *ext2, ext2_vnode *v, const char *path, vnode_id *vnode_num)
{
	status_t err;
	ext2_dcookie *d;
	uint8 dirbuf[1024];
	struct dirent *de = (struct dirent *)dirbuf;
	bool done = false;
	
//	dprintf("ext2_find called to find '%s' in dir %ld.\n", path, v->vnid);	

	err = bt_ext2_opendir(ext2, v, (void **)&d);
	if(err < B_NO_ERROR) return err;
	
	while(!done) {
		err = bt_ext2_readdir(ext2, v, d, de);
		if(err < 0) {
//			dprintf("there was an error in readdir.\n");
			goto out;
		}
		
		if(!strcmp(path, de->d_name)) {
//			dprintf("found it at vnode %d.\n", (int)de->d_ino);
			*vnode_num = de->d_ino;
			done = true;
		}
	}
	
	
out:	
//	if(!done) dprintf("did not find file.\n");

	bt_ext2_closedir(ext2, v, d);
	
	return err;
}



#if !USER

struct fs_ops bt_ext2_ops = {
	bt_ext2_mount,
	bt_ext2_unmount,
	
	bt_ext2_walk,
	bt_ext2_readvnode,
	bt_ext2_writevnode,

	bt_ext2_open,
	bt_ext2_close,
	bt_ext2_read,
	bt_ext2_ioctl,
	bt_ext2_fstat,
	bt_ext2_freadlink,

	bt_ext2_opendir,
	bt_ext2_closedir,
	bt_ext2_readdir,
	bt_ext2_rewinddir
};

#endif

#if USER

#include <string.h>

int main(int argc, char *argv[])
{
	void *vol;
	char path[1024];
	status_t err;
	vnode_id rootv;

	dprintf("mounting...\n");

	if(argc < 2) 
		err = bt_ext2_mount("/dev/disk/floppy/raw", &vol, &rootv);
	else
		err = bt_ext2_mount(argv[1], &vol, &rootv);
	if (err < 0) {
		dprintf("Error mounting volume (%s)\n", strerror(err));
		return 1;
	}

	if(argc>=3) {
		strcpy(path, argv[2]);
	} else {
		strcpy(path, "bigfile");
	}

	dprintf("mounted. rootv = %Ld.\n", rootv);

	
	{
		char *temp_path;
		vnode_id vnid;
		ext2_vnode *filevnode;
		void *fcookie;
		uint8 buf[1024*32];
		struct stat *st = buf;
		struct fmap_info *fmap_buf;
		char *p;
		ssize_t bytes_read;	
			
		// copy the path passed in
		temp_path = malloc(strlen(path)+1);
		if(!temp_path) {
			return ENOMEM;
		}
		strcpy(temp_path, path);
	
		err = bt_ext2_readvnode(vol, rootv, &filevnode);
		if(err < B_NO_ERROR) {
			dprintf("error loading root vnode.\n");
			return 1;
		}
	
		// walk through the directories, looking for the file
		while (temp_path) {
			p = strchr(temp_path, '/');
	
			if (p) *p = 0;
			
			if(strlen(temp_path)>0) {	
				// find the file in the current inode.
				err = bt_ext2_walk(vol, filevnode, temp_path, NULL, &vnid);
				if (err < B_NO_ERROR) {
					printf("ext2_walk ext2_find didn't find file.\n");
					return 1;
				}
				
				bt_ext2_writevnode(vol, filevnode);
				
				err = bt_ext2_readvnode(vol, vnid, &filevnode);
				if(err < B_NO_ERROR) {
					printf("couldn't load new vnode");
					return 1;
				}		

			}
			
			if (p) *p = '/';
			temp_path = p ? p+1 : NULL;
		}


		err = bt_ext2_open(vol, filevnode, &fcookie);
		if(err < B_NO_ERROR) {
			printf("couldn't load file.\n");
			return 1;
		}
		
		bt_ext2_fstat(vol, filevnode, fcookie, st);
		dprintf("st->st_ino = 0x%x, st->st_size = 0x%x.\n", st->st_ino, st->st_size);

		if(!S_ISLNK(st->st_mode)) {			
			uint32 offset = 0;
			
			bt_ext2_ioctl(vol, filevnode, fcookie, 'fmap', &fmap_buf, 0);
			dprintf("fmap_buf = 0x%x.\n", fmap_buf);

			{
				FILE *fp;

				fp = fopen("test", "w+");
							
				bytes_read = 1;
				while(bytes_read > 0) {
					bytes_read = bt_ext2_read(vol, filevnode, fcookie, offset, buf, 1024*32);		
					fwrite(buf, bytes_read, 1, fp);
					dprintf("read %d bytes at offset %d.\n", bytes_read, offset);
					offset += bytes_read;
				}
				fclose(fp);
			}
				
			{
				FILE *fp;
				
				fp = fopen("fmap", "w+");
				fwrite(fmap_buf, sizeof(struct fmap_info) + sizeof(struct fmap_block_run) * (fmap_buf->num_block_runs - 1), 1, fp);
				fclose(fp);
			}		
		} else {
			
			bytes_read = bt_ext2_freadlink(vol, filevnode, fcookie, buf, 1024*32);
			{
				FILE *fp;
				
				fp = fopen("link", "w+");
				fwrite(buf, bytes_read, 1, fp);
				fclose(fp);
			}
			
		}
			
		bt_ext2_close(vol, filevnode, fcookie);
		
		bt_ext2_writevnode(vol, filevnode);
	
	}

	bt_ext2_unmount(vol);

	return 0;
}

#endif
