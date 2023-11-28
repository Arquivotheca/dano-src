#include <drivers/KernelExport.h>

#if USER
	#define dprintf printf
#endif

#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <boot.h>
#include <fmap.h>
#include "bt_ntfs.h"

#if !USER
	#include "bt_fs.h"
#else
	typedef long long vnode_id;
#endif

#define MAX_FILE_RECORDS 1024

struct ntfs_fmap_block_run {
	uint64 block;
	uint64 num_blocks;
};

struct ntfs_fmap_info
{
	uint32 num_block_runs;
	uint32 fmap_alloc_length; // this stores the allocated length of the fmap structure measured in runs
	struct ntfs_fmap_block_run block_runs[1];
}; 

typedef struct {
	uint64 vnid;
	uint16 sequence_num;
	struct ntfs_fmap_info *fmap;
	uint64 fmap_data_len;
	uint64 fmap_initialized_data_len;
	bool is_dir;
	
	// for local data
	bool data_local; // stores whether or not the data is stored in the file record itself.
	void *data; // if the data is local, stick it here
	uint32 local_data_len;
	
	// for index (dir) data
	int index_buffer_size;
	int clusters_per_index_record;
	void *root_dirents; // this stores dirents that come from the INDEX_ROOT attribute
	int64 root_data_len;
	bool local_dirents;
	bool nonlocal_dirents;
	
	// standard info, can probably remove some of these later
//	time_t file_creation_time;
	time_t last_modification_time;
//	time_t last_FILE_rec_mod_time;
//	time_t last_access_time;
//	uint32 DOS_permissions;
	
	// stores a list of file records that this vnode occupies
	// its filled in when a ATT_LIST_ATTRIBUTE is loaded, if there is one
	FILE_REC file_records[MAX_FILE_RECORDS];
	uint32 num_file_records;
} ntfs_vnode;

// filesystem info struct
typedef struct {
	int fd;
	nspace_id nsid;

	uint8 vol_maj_version;
	uint8 vol_min_version;

	ntfs_bios_block nbb;
	uint32 cluster_size;
	uint32 clusters_per_compressed_block;
	uint32 mft_recordsize;
	uint64 num_clusters;

	uint8  *file_rec_buffer;

	// This stores a pointer to the vnode for the MFT
	ntfs_vnode *MFT;
	ntfs_vnode *VOL;
} ntfs_fs_struct;

typedef struct {
	// stuff to store the position
	uint64 next_index_buffer; 	// stores the next index buffer to read
	uint32 next_pos; 			// stores the next position inside the index buffer
	bool in_root;    			// stores if we're looking in the index root
	bool at_end;				// stores if we're done
	
	// Stuff to deal with a dir buffer
	uint8 *index_buf_cache;
	uint64 cached_index_buf_num;
	
} ntfs_dcookie;
	
// Function declarations
static status_t bt_ntfs_mount(nspace_id nsid, const char *device, uint32 flags, void *parms, int len, void **ns, vnode_id *root);
static status_t bt_ntfs_unmount(void *ns);
static status_t bt_ntfs_walk(void *ns, void *dir, const char *file, char *newpath, vnode_id *vnid);
static status_t bt_ntfs_readvnode(void *ns, vnode_id vnid, void **node);
static status_t bt_ntfs_writevnode(void *ns, void *node);
static status_t bt_ntfs_open(void *ns, void *node, void **cookie);
static status_t bt_ntfs_close(void *ns, void *node, void *cookie);
static ssize_t  bt_ntfs_read(void *ns, void *node, void *cookie, off_t pos, void *buffer, size_t len);
static status_t bt_ntfs_ioctl(void *ns, void *node, void *cookie, uint32 op, void *buffer, uint32 size);
static status_t bt_ntfs_fstat(void *ns, void *node, void *cookie, struct stat *st);
static ssize_t  bt_ntfs_freadlink(void *ns, void *node, void *cookie, char *buf, size_t len);
static status_t bt_ntfs_opendir(void *ns, void *node, void **cookie);
static status_t bt_ntfs_closedir(void *ns, void *node, void *cookie);
static status_t _bt_ntfs_readdir(void *ns, void *node, void *cookie, struct dirent *de);
static status_t bt_ntfs_readdir(void *ns, void *node, void *cookie, struct dirent *de);
static status_t bt_ntfs_rewinddir(void *ns, void *node, void *cookie);

static status_t bt_ntfs_get_block_from_fmap(struct ntfs_fmap_info *fmap, uint64 *block, uint64 block_in_fmap);
static status_t bt_ntfs_add_run_to_fmap(ntfs_fs_struct *ntfs, ntfs_vnode *v, uint64 start_block, uint64 length);
static status_t bt_ntfs_load_fmap_from_runlist(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *runlist, int32 length_to_read, uint64 blocks_to_load);
static status_t bt_ntfs_read_bios_block(ntfs_fs_struct *ntfs);
static status_t bt_ntfs_fixup_buffer(void *buf, uint32 length);
static status_t bt_ntfs_read_FILE_record(ntfs_fs_struct *ntfs, FILE_REC record_num, char *buf);
static status_t bt_ntfs_load_attributes(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *FILE_buf);
static int      bt_ntfs_load_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf);
static status_t bt_ntfs_load_std_info_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf);
static status_t bt_ntfs_load_vol_info_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf);
static status_t bt_ntfs_load_att_list_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf);
static status_t bt_ntfs_load_data_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf);
static time_t   bt_ntfs_time_to_posix_time(uint64 posix_time);
status_t bt_ntfs_unicode_to_utf8(const char *src, int32 *srcLen, char *dst, int32 *dstLen);
static void    *bt_ntfs_load_nonresident_attribute_data(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buffrom);
static ssize_t  bt_ntfs_read_from_fmap(ntfs_fs_struct *ntfs, struct ntfs_fmap_info *fmap, void *buf, off_t pos, size_t read_len);
static status_t bt_ntfs_load_index_root_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf);

#define strip_high_16(x) ((x) &  0x0000FFFFFFFFFFFFLL)
#define get_high_16(x) ((uint16)((x) >> 48))

// Pierre's Uber Macro
#define u_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|((*uni_str++)&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|((*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}	

status_t bt_ntfs_unicode_to_utf8(const char *src, int32 *srcLen, char *dst, int32 *dstLen)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	for (srcCount = 0; srcCount < srcLimit; srcCount += 2) {
		uint16  *UNICODE = (uint16 *)&src[srcCount];
		uchar	utf8[4];
		uchar	*UTF8 = utf8;
		int32 utf8Len;
		int32 j;

		u_to_utf8(UTF8, UNICODE);

		utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : B_ERROR);
}

/*
// copy len unicode characters from from to to :)
static void bt_ntfs_uni2ascii(char *to,char *from,int len)
{
	int i;

	for(i=0;i<len;i++)
		to[i]=from[2*i];
	to[i]='\0';
}
*/
static time_t bt_ntfs_time_to_posix_time(uint64 posix_time)
{
	// Move to 1970 time
	posix_time -= 0x019db1ded53e8000;
	
	// Divide by 10000000
	posix_time /= 10000000;
	
	return posix_time;
}

static void bt_ntfs_string_tolower(char *str)
{
	int i;

	for(i=0; i<strlen(str); i++) {
		if((str[i] >= 'A') && (str[i] <= 'Z')) 
			str[i] = str[i] - ('A' - 'a');
	}
}

static status_t bt_ntfs_mount(nspace_id nsid, const char *device, uint32 flags, void *parms, int len, void **ns, vnode_id *root)
{
	ntfs_fs_struct *ntfs;
	int fd;
	status_t err;

//	dprintf("bt_ntfs_mount called on device '%s'.\n", device);
	
	// Open the device
	fd = open(device, 0);
	if (fd < 0) {
//		dprintf("ntfs can't open '%s'\n", device);
		return fd;
	}

	// Allocate the filesystem structure
	ntfs = malloc(sizeof(ntfs_fs_struct));
	if(!ntfs) return ENOMEM;
	
	ntfs->fd = fd;
	ntfs->MFT = NULL;
	ntfs->VOL = NULL;
	ntfs->nsid = nsid;
	ntfs->file_rec_buffer = NULL;
	
	// Read in the partition info
	if((err = bt_ntfs_read_bios_block(ntfs)) < B_NO_ERROR) {
		goto error;
	}	

	// Load in the MFT
	err = bt_ntfs_readvnode((void *)ntfs, FILE_MFT, (void **)&ntfs->MFT); 
	if(err < B_NO_ERROR) {
		goto error;
	}

	// Load in the volume data
	err = bt_ntfs_readvnode((void *)ntfs, FILE_VOLUME, (void **)&ntfs->VOL); 
	if(err < B_NO_ERROR) {
		goto error;
	}

	// Check the filesystem's version number
	if((ntfs->vol_maj_version > 3) || ((ntfs->vol_maj_version == 3) && (ntfs->vol_min_version > 0))) {
		// accept only up to version 3.0 (Windows 2000)
		dprintf("NTFS volume version is unsupported. Version %d.%d\n", ntfs->vol_maj_version, ntfs->vol_min_version);
		err = B_ERROR;
		goto error;
	}


//	dprintf("ntfs_mount done.\n");

	*ns = ntfs;	
	*root = FILE_ROOT;

	return B_NO_ERROR;
	
error:
//	dprintf("ntfs mount failed for some reason.\n");
	if(ntfs->MFT) bt_ntfs_writevnode(ntfs, ntfs->MFT);
	if(ntfs->VOL) bt_ntfs_writevnode(ntfs, ntfs->VOL);
	close(ntfs->fd);
	free(ntfs);
	return err;
}

static status_t bt_ntfs_unmount(void *ns)
{
	ntfs_fs_struct *ntfs = (ntfs_fs_struct *)ns;
	
	if(ntfs) {
		if(ntfs->MFT) bt_ntfs_writevnode(ntfs, ntfs->MFT);
		if(ntfs->VOL) bt_ntfs_writevnode(ntfs, ntfs->VOL);
		if(ntfs->file_rec_buffer) free(ntfs->file_rec_buffer);
		close(ntfs->fd);
		free(ntfs);
	}
	
	return B_NO_ERROR;	
}

static status_t bt_ntfs_open(void *ns, void *node, void **cookie)
{
	*cookie = NULL;

	return B_NO_ERROR;
}

static status_t bt_ntfs_close(void *ns, void *node, void *cookie)
{
	return B_NO_ERROR; 
}

static status_t bt_ntfs_fstat(void *ns, void *node, void *cookie, struct stat *st)
{
	ntfs_vnode *v = (ntfs_vnode *)node;

	st->st_ino = v->vnid;
	// Set the permissions.
	if(v->is_dir) {
		st->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH | S_IFDIR;
		if((v->local_dirents) && (!v->nonlocal_dirents))
			st->st_size = v->root_data_len;
		else
			st->st_size = v->fmap_data_len;
	} else {
		st->st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IFREG;
		if(v->data_local)
			st->st_size = v->local_data_len;
		else
			st->st_size = v->fmap_data_len;
	}
	
	st->st_mtime = v->last_modification_time;

	return B_NO_ERROR; 
}

static ssize_t bt_ntfs_freadlink(void *ns, void *node, void *cookie, char *buf, size_t len)
{
	// No links here, bud.
	
	return EINVAL;
}

static ssize_t bt_ntfs_read(void *ns, void *node, void *cookie, off_t pos, void *buffer, size_t len)
{
	ntfs_fs_struct *ntfs = (ntfs_fs_struct *)ns;
	ntfs_vnode *v = (ntfs_vnode *)node;
	ssize_t left_to_read;

	// Check to see if we're not a directory
	if(v->is_dir) {
		return EISDIR;
	}

	// Negative pos? hmmm
	if(pos < 0) return EINVAL;

	// Deal with local data
	if(v->data_local) {
		// see if we're off the end of the buffer
		if(pos >= v->local_data_len) return 0;
		// trim the read if necessary
		left_to_read = min(len, v->local_data_len - pos);
		// do the memcpy
		memcpy(buffer, v->data, left_to_read);
		return left_to_read;
	} else {	
		// Check to see if we're not off the end of the file
		if(pos >= v->fmap_data_len) {
			return 0;
		}

		// trim the read so it doesn't read off the end of the file
		left_to_read = min(len, v->fmap_data_len - pos);
	
		// read it
		return bt_ntfs_read_from_fmap(ntfs, v->fmap, buffer, pos, left_to_read);
	}
}

static status_t bt_ntfs_ioctl(void *ns, void *node, void *cookie, uint32 op, void *buffer, uint32 size)
{
	ntfs_fs_struct *ntfs = (ntfs_fs_struct *)ns;
	ntfs_vnode *v = (ntfs_vnode *)node;

	if((op == 'fmap') || (op == 'dmap')) {
		// Convert our fmap info to the real one
		int i;
		uint32 new_fmap_len;
		struct fmap_info *fm = NULL;
		int fmap_type_to_use;
		int new_blocks_per_ntfs_block = ntfs->cluster_size / 512;
		
		// Check to see if this file has uninitialized spots
		if(v->fmap_initialized_data_len < v->fmap_data_len) {
			panic("A disk image file on an NTFS partition has an uninitialized area. "
				  "Please remove the file or write some data to it before continuing.");
			return B_ERROR;
		}			
		
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
			return B_NO_ERROR;
		}

		// Lets figure out which fmap version to use
		fmap_type_to_use = FMAP_TYPE_BLOCK; // we want to use the small one first
		for(i=0; i<v->fmap->num_block_runs; i++) {
			// See if any of the blocks numbers, converted to 512 byte lengths, are > MAX(int32)
			// or, if this, plus the block run length, reference a block > MAX(int32)
			uint64 start_block = v->fmap->block_runs[i].block * new_blocks_per_ntfs_block;
			uint64 end_block_run = start_block + v->fmap->block_runs[i].num_blocks * new_blocks_per_ntfs_block - new_blocks_per_ntfs_block; 
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
					fm->u.block.block_runs[i].block = v->fmap->block_runs[i].block * new_blocks_per_ntfs_block;
					fm->u.block.block_runs[i].num_blocks = v->fmap->block_runs[i].num_blocks * new_blocks_per_ntfs_block;
//					dprintf("start_block = %ld, run_length = %ld.\n", fm->u.block.block_runs[i].block, fm->u.block.block_runs[i].num_blocks);
				}
				
				fm->offset = 0;
				fm->u.block.block_size = 512;
				fm->u.block.num_block_runs = v->fmap->num_block_runs;
				fm->type = FMAP_TYPE_BLOCK;
				fm->size = new_fmap_len;

				// See if we need to truncate the last block run to account for a partial ntfs block
				{
					uint32 last_block_size;
					
					if(v->data_local)
						last_block_size = v->local_data_len % ntfs->cluster_size;
					else
						last_block_size = v->fmap_data_len % ntfs->cluster_size;
					
					if(last_block_size) {
						fm->u.block.block_runs[fm->u.block.num_block_runs-1].num_blocks -=
							new_blocks_per_ntfs_block - last_block_size / 512;
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
					fm->u.byte.byte_runs[i].byte = v->fmap->block_runs[i].block * ntfs->cluster_size;
					fm->u.byte.byte_runs[i].num_bytes = v->fmap->block_runs[i].num_blocks * ntfs->cluster_size;
//					dprintf("start byte = %Ld, run_length = %Ld.\n", fm->u.byte.byte_runs[i].byte, fm->u.byte.byte_runs[i].num_bytes);
				}
				fm->offset = 0;
				fm->u.byte.num_byte_runs = v->fmap->num_block_runs;
				fm->type = FMAP_TYPE_BYTE;
				fm->size = new_fmap_len;

				// See if we need to truncate the last byte run to account for a partial ntfs block
				{
					uint32 last_block_size;
					
					if(v->data_local)
						last_block_size = v->local_data_len % ntfs->cluster_size;
					else
						last_block_size = v->fmap_data_len % ntfs->cluster_size;
										
					if(last_block_size) {
						fm->u.byte.byte_runs[fm->u.byte.num_byte_runs-1].num_bytes -=
							ntfs->cluster_size - (last_block_size / 512) * 512;
						if(fm->u.byte.byte_runs[fm->u.byte.num_byte_runs-1].num_bytes <= 0) {
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
		
		return B_NO_ERROR;
	}

	return EINVAL;
}

static status_t bt_ntfs_opendir(void *ns, void *node, void **cookie)
{
	ntfs_vnode *v = (ntfs_vnode *)node;
	ntfs_dcookie *dcookie;

	// check to see if it's a directory
	if(!v->is_dir) return ENOTDIR;

	dcookie = (ntfs_dcookie *)malloc(sizeof(ntfs_dcookie));
	if(!dcookie) return ENOMEM;

	dcookie->next_index_buffer = 0;
	dcookie->next_pos = 0;
	dcookie->in_root = true;
	dcookie->at_end = false;
	dcookie->index_buf_cache = NULL;
	dcookie->cached_index_buf_num = 0;

	*cookie = dcookie;

	return B_NO_ERROR;
}

static status_t bt_ntfs_closedir(void *ns, void *node, void *cookie)
{
	ntfs_dcookie *d = (ntfs_dcookie *)cookie;

	if(d) {
		if(d->index_buf_cache) free(d->index_buf_cache);
		free(d);
	}

	return B_NO_ERROR;
}

// This version of readdir strips the sequence number off of the vnid # returned by the other readdir
static status_t bt_ntfs_readdir(void *ns, void *node, void *cookie, struct dirent *de)
{
	status_t err;
	
	err = _bt_ntfs_readdir((ntfs_fs_struct *)ns, (ntfs_vnode *)node, (ntfs_dcookie *)cookie, de);
	if(err < B_NO_ERROR) return err;
	
	// Strip the sequence number off the top of the returned inode #
	de->d_ino = strip_high_16(de->d_ino);
	
	return err;
}

// This version of readdir leaves the sequence number attached to the top 16 bits of the vnid
static status_t _bt_ntfs_readdir(void *ns, void *node, void *cookie, struct dirent *de)
{
	ntfs_dcookie *d = (ntfs_dcookie *)cookie;
	ntfs_vnode *v = (ntfs_vnode *)node;
	ntfs_fs_struct *ntfs = (ntfs_fs_struct *)ns;
	
	status_t err;
	ntfs_index_buffer_header *ntfs_bh;
	ntfs_index_entry *ntfs_ie;
	ntfs_attr_FILE_NAME *ntfs_fn;
	
//	dprintf("bt_ntfs_readdir entry on vnode %ld.\n", v->vnid);
	

tryagain:	
	if(d->at_end) {
		// We've already hit the end of this directory
//		dprintf("end of directory hit.\n");
		return B_ERROR;
	}
	
	if(d->in_root) {
		// Handle if we're still in the index root, stored in the vnode itself
		if(!v->local_dirents) {
			d->in_root = false;
			d->next_index_buffer = 0;
			d->next_pos = 0;
			goto tryagain;
		}
		
//		dprintf("\tlooking in the local data.\n");

		// make sure we wont look past the end of the buffer
		if(d->next_pos >= v->root_data_len) {
			// This meant that the last directory entry read pushed the pointer too far.
			// lets try to continue
			d->in_root = false;
			d->next_index_buffer = 0;
			d->next_pos = 0;
			goto tryagain;
		}			
		
		// position the structures for looking at the entry
		ntfs_ie = (ntfs_index_entry *)(v->root_dirents + d->next_pos);
		
		// see if we're looking at the last entry in the root
		if(ntfs_ie->flags & INDEX_ENTRY_LAST_ENTRY) {
//			dprintf("\tfound the last local entry. moving on..\n");
			// this entry is null, so kick the pointers up and try again
			d->in_root = false;
			d->next_index_buffer = 0;
			d->next_pos = 0;
			goto tryagain;
		}
		
		// If we've made it this far, we're looking at the indexed data stored locally in the vnode
		ntfs_fn = (ntfs_attr_FILE_NAME *)(v->root_dirents + d->next_pos + sizeof(ntfs_index_entry));
		d->next_pos += ntfs_ie->entry_size;
	} else {
		if(!v->nonlocal_dirents) {
			// we're done
			d->at_end = true;
			goto tryagain;
		} 	
		
//		dprintf("\tlooking at nonlocal data.\n");
		
		// see if there is a fmap for this vnode, ie. there was an index allocation attribute loaded
		if(!v->fmap) return B_ERROR;
		
//		dprintf("\tchecking end of buffer. data_len = %Ld, position = %Ld.\n", v->fmap_data_len, (d->next_index_buffer * v->index_buffer_size + d->next_pos));
		
		// make sure we aren't looking past the end of index. This would signify end-of-directory.
		if((d->next_index_buffer * v->index_buffer_size + d->next_pos) >= v->fmap_data_len) {
			d->at_end = true;
			goto tryagain;
		}
	
		// See if we don't have a cached copy of this index buffer
		if((!d->index_buf_cache) || (d->cached_index_buf_num != d->next_index_buffer)) {
			// Allocate temporary buffer space in the dircookie, if it isn't already
			if(!d->index_buf_cache) {
				d->index_buf_cache = malloc(v->index_buffer_size);
				if(!d->index_buf_cache) return ENOMEM;
			}
					
			// read the index buffer in
			err = bt_ntfs_read_from_fmap(ntfs, v->fmap, d->index_buf_cache, d->next_index_buffer * v->index_buffer_size, v->index_buffer_size);		
			if(err < B_NO_ERROR) return err;
			
			d->cached_index_buf_num = d->next_index_buffer;

			// check the magic number of the buffer
			ntfs_bh = (ntfs_index_buffer_header *)d->index_buf_cache;
			if(ntfs_bh->magic != INDEX_BUFFER_MAGIC) { // INDX
				dprintf("bt_ntfs_readdir found index buffer that failed the magic test.\n");
				return B_ERROR;
			}
			
			// fix it up
			err = bt_ntfs_fixup_buffer(d->index_buf_cache, v->index_buffer_size);
			if(err < B_NO_ERROR) {
				dprintf("bt_ntfs_readdir failed to fixup the index buffer.\n");
				return B_ERROR;
			}
		} else {
			ntfs_bh = (ntfs_index_buffer_header *)d->index_buf_cache;
		}				
		
		// if the current offset pointer in the dircookie is 0, lets push it forward
		// enough to get past the index buffer header
		if(d->next_pos == 0) 
			d->next_pos += ntfs_bh->header_size + 0x18;

		if(d->next_pos >= v->index_buffer_size) return B_ERROR;

//		dprintf("d->next_pos = %ld.\n", d->next_pos);
			
		// find the index entry
		ntfs_ie = (ntfs_index_entry *)(d->index_buf_cache + d->next_pos);

		// see if we're looking at the last entry in the buffer
		if(ntfs_ie->flags & INDEX_ENTRY_LAST_ENTRY) {
			// this entry is null, so kick the pointers up and try again
//			dprintf("hit last entry in index buffer, moving up one.\n");
			d->next_index_buffer++;
			d->next_pos = 0;
			goto tryagain;
		}
		
		// lay the standard attribute structure over the entry that d->next_pos points at
		ntfs_fn = (ntfs_attr_FILE_NAME *)(d->index_buf_cache + d->next_pos + sizeof(ntfs_index_entry));
		d->next_pos += ntfs_ie->entry_size;
	}
	
	// get rid of the 8.3 entries
	if(ntfs_fn->file_name_type == FILE_NAME_ATTR_DOS_FLAG) goto tryagain;

	// at this point, ntfs_fn points to the data in the directory entry
	de->d_ino = ntfs_ie->record;
	if(ntfs_fn->name_length >= v->index_buffer_size) return B_ERROR;
	{
	
		#define MAX_FILENAME_LENGTH 256
	
		int32 unicode_name_len = ntfs_fn->name_length*2;
		int32 max_utf8_name_len = MAX_FILENAME_LENGTH-1;
		
		err = bt_ntfs_unicode_to_utf8((char *)((int)ntfs_fn + 0x42), &unicode_name_len, de->d_name, &max_utf8_name_len);
		if(err < B_NO_ERROR) return B_ERROR;
		de->d_name[max_utf8_name_len] = 0;
	}		
	
//	dprintf("bt_ntfs_readdir found entry '%s'\n", de->d_name);

	return B_OK;
}

static status_t bt_ntfs_rewinddir(void *ns, void *node, void *cookie)
{
	ntfs_dcookie *d = (ntfs_dcookie *)cookie;

	d->next_index_buffer = 0;
	d->next_pos = 0;
	d->in_root = true;
	d->at_end = false;

	return B_NO_ERROR;
}

static status_t bt_ntfs_get_block_from_fmap(struct ntfs_fmap_info *fmap, uint64 *block, uint64 block_in_fmap)
{
	int i;
	uint64 blocks_left = block_in_fmap;
	
/*	dprintf("fmap dump:\n");
	for(i=0; i<fmap->num_block_runs; i++) {
		dprintf("\tRun %d: start=%Ld, num_blocks=%Ld.\n", i, fmap->block_runs[i].block, fmap->block_runs[i].num_blocks);
	}
*/	
	for(i=0; i<fmap->num_block_runs; i++) {
//		dprintf("\tlooking at run %ld for block %Ld: start=%Ld, num_blocks=%ld.\n", i, block_in_fmap, fmap->block_runs[i].block, fmap->block_runs[i].num_blocks);
		if(blocks_left < fmap->block_runs[i].num_blocks) {
			// It's in this block run
			*block = fmap->block_runs[i].block + blocks_left;
			return B_NO_ERROR;
		}	
		blocks_left -= fmap->block_runs[i].num_blocks;
	}

//	dprintf("\treturning block %Ld.\n", *block);

	return B_ERROR;
}

static status_t bt_ntfs_add_run_to_fmap(ntfs_fs_struct *ntfs, ntfs_vnode *v, uint64 start_block, uint64 length)
{
	// check to see if the fmap exists
	if(!v->fmap) {
//		dprintf("bt_ntfs_add_run_to_fmap creating new fmap structure.\n");
		v->fmap = malloc(sizeof(struct ntfs_fmap_info) + sizeof(struct ntfs_fmap_block_run) * 15);
		if(!v->fmap) return ENOMEM;
		// Initialize the new fmap
		memset(v->fmap, 0, sizeof(struct ntfs_fmap_info) + sizeof(struct ntfs_fmap_block_run) * 15);
		v->fmap->fmap_alloc_length = 16;
	}

	// we need a new run
	if((v->fmap->num_block_runs + 1) > v->fmap->fmap_alloc_length) {
		// we also need a new fmap structure
		struct ntfs_fmap_info *temp_fmap = malloc(sizeof(struct ntfs_fmap_info) + sizeof(struct ntfs_fmap_block_run) * (v->fmap->fmap_alloc_length * 2 - 1));
		if(!temp_fmap) return ENOMEM;
		// copy the old structure over
		memcpy(temp_fmap, v->fmap, sizeof(struct ntfs_fmap_info) + sizeof(struct ntfs_fmap_block_run) * (v->fmap->fmap_alloc_length - 1));
		// free the old structure & update things
		free(v->fmap);
		v->fmap = temp_fmap;
		v->fmap->fmap_alloc_length *= 2;
	}

	// update things
	v->fmap->block_runs[v->fmap->num_block_runs].block = start_block;
	v->fmap->block_runs[v->fmap->num_block_runs].num_blocks = length;	
	v->fmap->num_block_runs++;

//	dprintf("nt_ntfs_add_run_to_fmap added a run starting at block %Ld, length %Ld.\n", start_block, length);

	return B_NO_ERROR;
}

static status_t bt_ntfs_readvnode(void *ns, vnode_id vnid, void **node)
{
	ntfs_fs_struct *ntfs = (ntfs_fs_struct *)ns;
	ntfs_FILE_record *fr;
	status_t err;
	ntfs_vnode *v;

	// Allocate file record buffer space, if necessary
	if(!ntfs->file_rec_buffer) {
		ntfs->file_rec_buffer = malloc(ntfs->mft_recordsize);
		if(!ntfs->file_rec_buffer) return ENOMEM;
	}

	// Create a new vnode
	v = malloc(sizeof(ntfs_vnode));
	if(!v) return ENOMEM;

	// If this is the MFT, set the pointer in the fs struct
	if(vnid == FILE_MFT) 
		ntfs->MFT = v;
		
	// Initialize the vnode struct
	memset(v, 0, sizeof(ntfs_vnode));
	v->vnid = vnid;
	v->data_local = false;
	v->local_dirents = false;
	v->nonlocal_dirents = false;
	v->num_file_records = 1;
	v->file_records[0] = vnid;

//	dprintf("bt_ntfs_readvnode entry on vnode %Ld.\n", vnid);

	// start parsing through the attributes
	{
		uint32 i = 0;
		// This will walk through all of the MFT entries that this vnode occupies
		// If there are additional MFT records used, the list will get filled by loading
		// in a ATT_LIST_ATTRIBUTE in the first MFT entry.
		while(i<v->num_file_records) {
			err = bt_ntfs_read_FILE_record(ntfs, v->file_records[i], ntfs->file_rec_buffer);
			if(err < B_NO_ERROR) {
				return err;
			}

			// We should have a MFT record now, lets verify
			fr = (ntfs_FILE_record *)ntfs->file_rec_buffer;
			if(fr->magic != FILE_REC_MAGIC) {
				dprintf("ntfs_readvnode FILE record failed magic test\n");
				return EINVAL;
			}
		
			// Ok, lets do the fixup
			err = bt_ntfs_fixup_buffer(ntfs->file_rec_buffer, ntfs->mft_recordsize);
			if(err < B_NO_ERROR) {
				dprintf("ntfs_readvnode FILE record failed fixup\n");
				return err;
			}
			
			// Check the in-use flag
			if(!(fr->flags & FILE_REC_IN_USE_FLAG)) {
				dprintf("ntfs_readvnode FILE record is not marked in-use.\n");
				return EINVAL;
			}
	
			// Check the sequence number
			if(i==0) {
				// If this is the first record to read in for this vnode, stores it's sequence #
				v->sequence_num = fr->seq_number;
				// Add the sequence number to the FILE record number stored in the file_record list for this vnode
				v->file_records[0] += (uint64)v->sequence_num << 48;
				// Check to see if base_file_rec is 0. If it isn't, this record is an extension and therefore not what we want
				if(fr->base_file_rec != 0) {
					dprintf("ntfs_readvnode found nonzero base_file_rec pointer on base file record.\n");
					return EINVAL;
				}
			} else {
				// If it's not the first record, verify that the base file record pointer is pointing to the base
				// and the sequence number matches
				if((strip_high_16(fr->base_file_rec) != v->vnid) || (get_high_16(fr->base_file_rec) != v->sequence_num)) {
					dprintf("ntfs_readvnode extended FILE record failed base file record sequence number check\n");
					return EINVAL;
				}
				// Now, check that the sequence number in the current file record matches the sequence number stored in the top
				// 16 bits of the record we were instructed to load. This number was loaded in a previous attribute list attribute
				// load. This verifies that the attribute list attribute is in sync with this file record
				if(get_high_16(v->file_records[i]) != fr->seq_number) {
					dprintf("ntfs_readvnode extended FILE record's sequence number does not match previous sequence #.\n");
					return EINVAL;
				}
			}				
		
			err = bt_ntfs_load_attributes(ntfs, v, ntfs->file_rec_buffer);
			if(err < B_NO_ERROR) {
				return err;
			}
		
			i++;
		}
	}

	// Check to see if we've loaded anything..
	if((!v->fmap) && (!v->data_local) && (!v->local_dirents)) {
		bt_ntfs_writevnode(ntfs, v);
		return EINVAL;
	}

//	if(v->fmap)
//		dprintf("fmap @ 0x%x, fmap length = %d.\n", v->fmap, v->fmap->num_block_runs);

//	dprintf("fmap_data_len = %Ld.\n", v->fmap_data_len);

	*node = v;

	return B_NO_ERROR;	
}

static status_t bt_ntfs_writevnode(void *ns, void *node)
{
	ntfs_vnode *v = (ntfs_vnode *)node;
	
	if(v) {
		if(v->fmap) free(v->fmap);
		if(v->root_dirents) free(v->root_dirents);
		if(v->data) free(v->data);
		free(v);	
	}

	return B_NO_ERROR;
}

static status_t bt_ntfs_load_attributes(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *FILE_buf)
{
	ntfs_FILE_record *file_rec = (ntfs_FILE_record *)FILE_buf;
	void *curr_ptr = FILE_buf;
	int attributes_to_read;
	
//	dprintf("bt_ntfs_load_attributes entry.\n");
	
	// Do some sanity checks
	if(!FILE_buf) return B_ERROR;

	// Dump some info
//	dprintf("bt_ntfs_load_attributes() info dump:\n");
//	dprintf("\tsequence_number = %d\n\thard link count = %d\n", file_rec->seq_number, file_rec->hard_link_count);
//	dprintf("\tflags = %d\n\tmax attribute number+1 = %d\n", file_rec->flags, file_rec->max_attr_ident); 

	attributes_to_read = file_rec->max_attr_ident - 1;

	// Fill in some vnode data
	if(file_rec->flags & 0x2) v->is_dir = true;
	else v->is_dir = false;

	// Push the current pointer to the first attribute
	curr_ptr += sizeof(ntfs_FILE_record) + 2*(file_rec->fixup_list_size-1);
	{
		int retval = 1;
		while(retval != 0xffffffff && retval > 0) {
			retval = bt_ntfs_load_attribute(ntfs, v, curr_ptr);
			if(retval == 0) {
//				dprintf("bt_ntfs_load_attributes had error loading an attribute.\n");
				return B_ERROR;
			}
			curr_ptr += retval;
			attributes_to_read--;
		}
	}
		
	return B_NO_ERROR;
}

// Loads the current attribute into the vnode, returns the number of bytes to push the curr_ptr by.
static int bt_ntfs_load_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf)
{
	ntfs_attr_header *attr_header;
	uint32 move_ahead_length = 0;
	status_t err = B_NO_ERROR;

	// Sanity check
	if(!buf) return 0;
	
	// Set the attribute header
	attr_header = buf;
	
//	dprintf("load_attribute() found attribute type 0x%x, ", (unsigned int)attr_header->type);	
//	dprintf("length = %d\n", attr_header->length);
	
	// Select what to do with the attribute
	switch(attr_header->type) {
		case ATT_STD_INFO:
//			dprintf("\tloading STD_INFO attribute...\n");
			err = bt_ntfs_load_std_info_attribute(ntfs, v, buf);		
			break;
		case ATT_ATT_LIST:
//			dprintf("\tloading ATT_LIST attribute...\n");
			err = bt_ntfs_load_att_list_attribute(ntfs, v, buf);
			break;
/*		case ATT_FILE_NAME:
			dprintf("\tloading FILE_NAME attribute...\n");
			attr_struct_to_add = ntfs_malloc(sizeof(file_name_attr));
			// do the memory verify inside the load attribute functions
			if(attr_struct_to_add) {
				move_ahead_length = ntfs_load_file_name_attribute(attr_struct_to_add, buf, ntfs);		
			}
			break;*/
/*		case ATT_VOLUME_NAME:
			dprintf("\tloading VOLUME_NAME attribute...\n");
			attr_struct_to_add = ntfs_malloc(sizeof(volume_name_attr));
			// do the memory verify inside the load attribute functions
			if(attr_struct_to_add) {
				move_ahead_length = ntfs_load_volume_name_attribute(attr_struct_to_add, buf, ntfs);		
			}
			break;*/
		case ATT_VOLUME_INFORMATION:
//			dprintf("\tloading VOLUME_INFORMATION attribute...\n");
			if(v->vnid != FILE_VOLUME) {
				// This is not the volume file, so just quit
				break;
			}
			err = bt_ntfs_load_vol_info_attribute(ntfs, v, buf);
			break;
		case ATT_DATA:
//			dprintf("\tloading DATA attribute...\n");
			if(v->is_dir == true) {
				// This is a index vnode, so ignore any data attributes.
				break;
			}
			err = bt_ntfs_load_data_attribute(ntfs, v, buf);
			break;

		case ATT_INDEX_ALLOCATION:			
//			dprintf("\tloading INDEX_ALLOCATION attribute...\n");	
			err = bt_ntfs_load_data_attribute(ntfs, v, buf);
			break;
		case ATT_INDEX_ROOT:
//			dprintf("\tloading INDEX_ROOT attribute...\n");
			move_ahead_length = bt_ntfs_load_index_root_attribute(ntfs, v, buf);
			break;
		case 0xffffffff:
			return attr_header->type;
			break;
		default:
//			dprintf("\tunable to handle attribute type 0x%x\n", (unsigned int)attr_header->type);
	}

	if(err < B_NO_ERROR) {
//		dprintf("bt_ntfs_load_attribute had error reading attribute 0x%x\n", attr_header->type);
		return 0;
	}

//	dprintf("bt_ntfs_load_attribute exiting with %d.\n", attr_header->length);

	return attr_header->length;
}

// this function loads a index root attribute from a MFT record
static status_t bt_ntfs_load_index_root_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf)
{
	ntfs_attr_header *header = buf;
	ntfs_attr_INDEX_ROOT *ir = NULL;

//	dprintf("ntfs_load_index_root_attribute() entry.\n");
	
	// Sanity check
	if(header->type != ATT_INDEX_ROOT) {
		return B_ERROR;
	}

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = buf+0x10;			
//		dprintf("\tresident.\n");

		v->root_data_len = ir->size_of_entries - 0x10;

		ir = buf+res_header->specific_value_offset;
	} else {
		// non-resident
		ntfs_attr_nonresident *nres_header = buf+0x10;
//		dprintf("\tnonresident.\n");

		v->root_data_len = nres_header->real_size;

		// Load the attribute data		
		ir = bt_ntfs_load_nonresident_attribute_data(ntfs, v, buf);
		if(!ir) goto error1;
	}

	// Copy the data
	v->index_buffer_size = ir->index_record_size;
//	dprintf("\tindex record size = %ld\n", v->index_buffer_size);
	v->clusters_per_index_record = ir->clusters_per_index_record;
//	dprintf("\tclusters per index record = %ld\n", v->clusters_per_index_record);
	if(ir->flags & INDEX_ROOT_ATTR_LARGE_INDEX_FLAG) v->nonlocal_dirents = true;
	v->local_dirents = true;
	v->root_dirents = malloc(ir->size_of_entries - 0x10);
	if(!v->root_dirents) {
//		dprintf("bt_ntfs_load_index_root_attribute had error allocating space for local directory storage.\n");
		goto error1;
	}
//	dprintf("\tcopying %d bytes of local data.\n", ir->size_of_entries - 0x10);
	memcpy(v->root_dirents, (void *)ir+0x20, ir->size_of_entries - 0x10);  

	if(header->non_resident)
		if(ir) free(ir);

	return header->length;

error1:
	v->root_data_len = 0;	
	if(header->non_resident)
		if(ir) free(ir);
	return B_ERROR;		
	
}

static status_t bt_ntfs_load_data_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf)
{
	ntfs_attr_header *header = buf;
	status_t err;

//	dprintf("bt_ntfs_load_data_attribute entry.\n");
	
	// Sanity check
	if((header->type != ATT_DATA) && (header->type != ATT_INDEX_ALLOCATION)) {
		// This is screwed up
//		dprintf("\tbuffer not pointing at data or index allocation attribute!\n");
		return EINVAL;
	}
	
	// Copy the name
	if(header->name_len > 0) {
		// If it's a DATA attribute, we are not concerned with any data streams other than the primary, unnamed one
		if(header->type == ATT_DATA) {
			if(header->name_len > 0) {
				// This is a named attribute, so bail
//				dprintf("\tfound named attribute.\n");
				return B_NO_ERROR;
			}
		} else {
			// We are a INDEX_ALLOCATION
			uint32 name_offset;
			char temp_name[256];
			// If it's an index allocation attribute, we are only concerned with attributes named $I30.
			// Get the name now
			if(!header->non_resident) 
				name_offset = 0x18;
			else
				name_offset = 0x40;
			// Copy the name
			{				
				int32 unicode_name_len = header->name_len*2;
				int32 max_utf8_name_len = 256-1;
				err = bt_ntfs_unicode_to_utf8(buf+name_offset, &unicode_name_len, temp_name, &max_utf8_name_len);
				if(err < B_NO_ERROR) return B_NO_ERROR;
				temp_name[max_utf8_name_len] = 0;
			}
			// see if the name is $I30
			if(strncmp(temp_name, "$I30", header->name_len) != 0) {
				// It's not... Never seen an index other than type 0x30, but it can happen...
//				dprintf("found index allocation attribute for something other than attribute 0x30.\n");
				return B_NO_ERROR;
			}
		}
	}	

	// Check the resident flag
	if(!header->non_resident) {
		ntfs_attr_resident *res_header = buf+0x10;
//		dprintf("\tresident.\n");
		// Handle the resident case first
		if(v->data_local) {
//			dprintf("load_data_attribute found resident attribute, but data was already loaded into the vnode.\b");
			return B_ERROR;
		}
		v->data_local = true;
		v->local_data_len = res_header->specific_value_length;
//		dprintf("\tsetting v->local_data_len to %Ld.\n", v->local_data_len);
		if(v->local_data_len) {
			// Copy the data
			v->data = malloc(v->local_data_len);
			if(!v->data) {
				v->data_local = false;
				v->local_data_len = 0;
				return B_ERROR;
			};
			memcpy(v->data, buf+res_header->specific_value_offset, v->local_data_len);
		}
	} else {
		ntfs_attr_nonresident *nres_header = buf+0x10;
//		dprintf("\tnonresident.\n");
		
		// Is it compressed ?
		if(header->compressed) {
			dprintf("bt_ntfs_load_data_attribute the file is compressed. I cannot handle this.\n");
			return B_ERROR;
		}

		// Get the runlist
		err = bt_ntfs_load_fmap_from_runlist(ntfs, v, buf + nres_header->runlist_offset, header->length - nres_header->runlist_offset, 
			(strip_high_16(nres_header->last_VCN)+1) - strip_high_16(nres_header->starting_VCN));
		if(err < B_NO_ERROR) return B_ERROR;				
		
		// If this is not the first time we're adding to the runlist, the real_size field will be zero.
		// Dont update the v->fmap_data_len field
		if((v->fmap_data_len == 0) && (nres_header->real_size != 0)) {
			v->fmap_data_len = nres_header->real_size;
			v->fmap_initialized_data_len = nres_header->initialized_data_size;
//			dprintf("\tsetting v->fmap_data_len to %Ld.\n", v->fmap_data_len);
		}
	}
	
	return B_NO_ERROR;
}

#define high_4bits(a) (((a) & 0xF0) >> 4)
#define low_4bits(a)  ((a) & 0x0F)

static status_t bt_ntfs_load_fmap_from_runlist(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *runlist, int32 length_to_read, uint64 blocks_to_load)
{
	void *curr_ptr = runlist;
	uint64 block, length;
	int64 offset;
	int64 sign_temp;
	uint8 sizes;
	int i;
	status_t err;
	bool loaded_a_block = false;
	
//	dprintf("ntfs_load_fmap_from_runlist entry. length_to_read = %d\n", (int)length_to_read);
	
	dprintf("ntfs loading fmap.");
	
	while((length_to_read > 0) && (blocks_to_load)) {
		block = length = offset = 0;				
		sizes = *(uint8 *)curr_ptr;
		if(sizes == 0) {
			// push the pointer ahead one (maybe just quit here)
//			dprintf("\tboth offset length and length length are zero. hmmm.\n");	
			curr_ptr++;
			length_to_read--;
		} else {
			curr_ptr++;
			length_to_read--;
			// Handle the length
			for(i=0; i<low_4bits(sizes); i++) {
				length = length + (*(uint8 *)curr_ptr << 8*i);
				curr_ptr++;				
				length_to_read--;
			}
			// handle the new starting block
			for(i=0; i<high_4bits(sizes); i++) {
				offset = offset | (*(uint8 *)curr_ptr << 8*i);				
				curr_ptr++;
				length_to_read--;
			}
			// Handle negative offsets
			if(i) {
				if(*(uint8 *)(curr_ptr-1) >= 0x80) {
					// Needs to be sign extended
					sign_temp = 0xFFFFFFFFFFFFFFFF << 8*high_4bits(sizes);
					offset = offset | sign_temp;
				}
			}
			
			if(length > blocks_to_load) blocks_to_load = 0;
			else blocks_to_load -= length;
			
			if(length) {
//				dprintf("\tgoing to do something with offset %Ld (0x%8x)", offset, (unsigned int)offset);
//				dprintf(", length %Ld.\n", length);
												
				// Calculate starting block				
				if(loaded_a_block)
					block = v->fmap->block_runs[v->fmap->num_block_runs-1].block + offset;
				else
					block = offset;
								
				dprintf(".");								
				err = bt_ntfs_add_run_to_fmap(ntfs, v, block, length);								
				if(err < B_NO_ERROR) {
//					dprintf("error adding run to fmap.\n");
					return err;
				}
				
				loaded_a_block = true;	
			}
		}
	}
	
//	dprintf("ntfs_load_extent_from_runlist exit.\n");

	dprintf("done\n"); 
		
	return B_NO_ERROR;

}

static void *bt_ntfs_load_nonresident_attribute_data(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buffrom)
{
	ntfs_attr_header *header = buffrom;
	ntfs_attr_nonresident *nres_header;
	uint64 read_len;
	void *return_buffer;
	status_t err;

//	dprintf("bt_ntfs_load_nonresident_attribute_data entry.\n");
	
	if(!header->non_resident) {
		// This must be a resident attribute
		goto error;
	}
	
	nres_header = buffrom+0x10;
	
	// load the runlist
	err = bt_ntfs_load_fmap_from_runlist(ntfs, v, buffrom+nres_header->runlist_offset,
		header->length - nres_header->runlist_offset, (strip_high_16(nres_header->last_VCN)+1) - strip_high_16(nres_header->starting_VCN));
	if(err < B_NO_ERROR) {
//		dprintf("bt_ntfs_load_nonresident_attribute_data had error loading in fmap.\n");
		goto error1;
	}
		
	// load the data the runlist points to
	return_buffer = malloc(nres_header->real_size);
	if(!return_buffer) {
//		dprintf("bt_ntfs_load_nonresident_attribute_data couldn't allocate space for temporary buffer.\n");
		goto error1;
	}
	
	read_len = bt_ntfs_read_from_fmap(ntfs, v->fmap, return_buffer, 0, nres_header->real_size);
	if(read_len != nres_header->real_size) {
//		dprintf("bt_ntfs_load_nonresident_attribute_data had error reading in data.\n");
		goto error1;
	}

	// free the fmap memory
	if(v->fmap) free(v->fmap);
	v->fmap = NULL;

//	dprintf("ntfs_load_nonresident_attribute_data exit.\n");

	return return_buffer;

error1:
	if(v->fmap) free(v->fmap);
	v->fmap = NULL;

error:
	return NULL;
	

}

static ssize_t bt_ntfs_read_from_fmap(ntfs_fs_struct *ntfs, struct ntfs_fmap_info *fmap, void *buf, off_t pos, size_t read_len)
{
	uchar *buf_pos = (uchar *)buf;
	ssize_t size_read;
	uint64 fs_block;
	uint64 block;
	uint32 offset;
	ssize_t length_to_read;
	status_t err;
	ssize_t bytes_read=0;
	size_t left_to_read = read_len;

//	dprintf("bt_ntfs_read_from_fmap entry. pos = %Ld, read_len = %Ld.\n", pos, read_len);
	
	if(pos < 0) return 0;
	
	// Calculate the starting block into the file to read.
	block = pos / ntfs->cluster_size;
	// Calculate the offset into the block to read
	offset = pos % ntfs->cluster_size;

	while(left_to_read > 0) {
		length_to_read = min(left_to_read, ntfs->cluster_size - offset);
		err = bt_ntfs_get_block_from_fmap(fmap, &fs_block, block++);
		if(err < B_NO_ERROR) {
			return 0;
		}
//		dprintf("ntfs_read reading at block %Ld offset %ld length %Ld.\n", fs_block, offset, length_to_read);
		size_read = read_pos(ntfs->fd, fs_block * ntfs->cluster_size + offset, buf_pos, length_to_read);
		if(size_read != length_to_read) {
			return 0;
		}
		offset = 0;
		left_to_read -= length_to_read;
		bytes_read += length_to_read;
		buf_pos += length_to_read;
	}

	return bytes_read;	
}

static status_t bt_ntfs_load_att_list_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf)
{
	ntfs_attr_header *header = buf;
	void *curr_ptr;
	uint32 length_to_read;
	ntfs_attr_ATT_LIST_record *record;
	void *non_res_buf_ptr = NULL;

		
//	dprintf("ntfs_load_att_list_attribute entry.\n");
	
	// Sanity check
	if(header->type != ATT_ATT_LIST) {
		// This is screwed up
		return EINVAL;;
	}

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = buf+0x10;			
//		dprintf("\tnresident.\n");

		length_to_read = res_header->specific_value_length;
		curr_ptr = buf + res_header->specific_value_offset;
	} else {
		// non-resident		
		ntfs_attr_nonresident *nres_header = buf + 0x10;
//		dprintf("\tnonresident.\n");
		
		// Load the attribute data		
		non_res_buf_ptr = bt_ntfs_load_nonresident_attribute_data(ntfs, v, buf);
		if(!non_res_buf_ptr) return EINVAL;
		length_to_read = nres_header->real_size;
		curr_ptr = non_res_buf_ptr;
	}

	while(length_to_read > 0) {
		record = curr_ptr;
//		dprintf("record dump:\n");
//		dprintf("\ttype = %d, rec_length = %d, starting VCN = %Ld, attribute_record = %Ld.\n", record->type, record->rec_length, record->starting_VCN, strip_high_16(record->attribute_record));
		// Modify the pointers
		curr_ptr += record->rec_length;
		length_to_read -= record->rec_length;
//		dprintf("length_to_read = %d.\n", length_to_read);
		// Add one to the FILE record list in the vnode
		{
			int i;
			
			FILE_REC new_record = record->attribute_record;
			bool is_new_record = true;
			// Only add it if it doesn't already exist
			for(i=0; i<v->num_file_records; i++) 
				if(new_record == v->file_records[i]) {
					is_new_record = false;
					break;
				}
				
			if(is_new_record) {
				if(v->num_file_records < MAX_FILE_RECORDS) {
					v->file_records[v->num_file_records] = new_record;
					v->num_file_records++;
//					dprintf("\tfound pointer to FILE record %Ld , sequence num %d, for attribute 0x%x.\n", strip_high_16(new_record), get_high_16(new_record), record->type);
				} else {
					dprintf("vnode %Ld has more than %d MFT records. aborting vnode load..\n", v->vnid, MAX_FILE_RECORDS);										
					break;
				}
			}
		}
	}

	if(header->non_resident) {
		if(non_res_buf_ptr) free(non_res_buf_ptr);
		if(v->fmap) free(v->fmap);
		v->fmap = NULL;
	}

//	dprintf("bt_ntfs_load_att_list_attribute exit\n");

	return header->length;
}

// this function loads the volume info attribute from a MFT record
static status_t bt_ntfs_load_vol_info_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf)
{
	ntfs_attr_header *header = buf;
	ntfs_attr_VOLUME_INFO *vol_info;

	// Sanity check
	if(header->type != ATT_VOLUME_INFORMATION) {
		// This is screwed up
		return EINVAL;
	}

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = buf+0x10;			

		vol_info = buf+res_header->specific_value_offset;
		
	} else {
/*		// non-resident
	//	ntfs_attr_nonresident *nres_header = buf+0x10;
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));

		// Load the attribute data		
		std_info = ntfs_load_nonresident_attribute_data(ntfs, buf);
		if(!std_info) goto error1;*/
		
//		dprintf("bt_ntfs_load_std_info_attribute found non-resident standard attribute? this can not be..\n");
		return EINVAL;
	}

	// Copy the data
	ntfs->vol_maj_version = vol_info->maj_version;
	ntfs->vol_min_version = vol_info->min_version;

//	dprintf("loaded volume version. major = %d, minor = %d.\n", ntfs->vol_maj_version, ntfs->vol_min_version);

	return B_NO_ERROR;	
}

// this function loads a standard info attribute from a MFT record
static status_t bt_ntfs_load_std_info_attribute(ntfs_fs_struct *ntfs, ntfs_vnode *v, void *buf)
{
	ntfs_attr_header *header = buf;
	ntfs_attr_STD_INFO *std_info;

//	dprintf("bt_ntfs_load_std_info_attribute entry.\n");
	
	// Sanity check
	if(header->type != ATT_STD_INFO) {
		// This is screwed up
		return EINVAL;
	}

	if(!header->non_resident) {
		// resident
		ntfs_attr_resident *res_header = buf+0x10;			

		std_info = buf+res_header->specific_value_offset;
		
	} else {
/*		// non-resident
	//	ntfs_attr_nonresident *nres_header = buf+0x10;
		DEBUGPRINT(ATT, 7, ("\tnonresident.\n"));

		// Load the attribute data		
		std_info = ntfs_load_nonresident_attribute_data(ntfs, buf);
		if(!std_info) goto error1;*/
		
//		dprintf("bt_ntfs_load_std_info_attribute found non-resident standard attribute? this can not be..\n");
		return EINVAL;
	}

	// Copy the data
//	v->file_creation_time = bt_ntfs_time_to_posix_time(std_info->file_creation_time);
	v->last_modification_time = bt_ntfs_time_to_posix_time(std_info->last_modification_time);
//	v->last_FILE_rec_mod_time = bt_ntfs_time_to_posix_time(std_info->last_FILE_rec_mod_time);
//	v->last_access_time = bt_ntfs_time_to_posix_time(std_info->last_access_time);
//	v->DOS_permissions = std_info->DOS_permissions;

	return B_NO_ERROR;	
}

static status_t bt_ntfs_read_FILE_record(ntfs_fs_struct *ntfs, FILE_REC record_num, char *buf)
{
	uint64 block;
	uint32 blocks_to_read;
	uint32 block_offset;
	ssize_t retval;
	status_t err;

//	dprintf("bt_ntfs_read_FILE_record called on record number %Ld.\n", record_num);

	// strip the sequence number out of the record num, if any
	record_num = strip_high_16(record_num);

	// If we're loading from the first 16 MFT records, we know where it is without consulting
	// the MFT. This is nice, especially for reading in the MFT itself. 
	if(record_num <= 0xF) {
		// We know where these are, independant of the knowledge of the $MFT file.
		// Also, if multiple blocks need to be read in, they are guaranteed to be contiguous.
		block = ntfs->nbb.MFT_cluster;
		block += (record_num*ntfs->mft_recordsize) / ntfs->cluster_size;
		block_offset = (record_num*ntfs->mft_recordsize) % ntfs->cluster_size;

//		dprintf("\tgoing to read FILE record at block %Ld, offset %d.\n", block, block_offset);

		retval = read_pos(ntfs->fd, block * ntfs->cluster_size + block_offset, buf, ntfs->mft_recordsize);
		if(retval != ntfs->mft_recordsize) {
//			dprintf("bt_ntfs_read_FILE_record had error reading file record %Ld.\n", record_num);
			return EIO;
		}
	} else {
		VCN mft_vcn;
		uint64 read_len;
		char *temp_buf = buf;
		
//		dprintf("\tlooking at MFT's fmap.\n");
		
		// Calculate the MFT's VCN to start reading in
		mft_vcn = (record_num * ntfs->mft_recordsize) / ntfs->cluster_size;

		// Calculate how many blocks we'll need to read and the size of the read per pass
		if(ntfs->mft_recordsize > ntfs->cluster_size) {
			blocks_to_read = ntfs->mft_recordsize / ntfs->cluster_size;
			read_len = ntfs->cluster_size;
		} else {
			blocks_to_read = 1;
			read_len = ntfs->mft_recordsize;
		}

//		dprintf("blocks_to_read = %d, read_len = %d.\n", blocks_to_read, read_len);

		while(blocks_to_read > 0) {
			// get the block
			err = bt_ntfs_get_block_from_fmap(ntfs->MFT->fmap, &block, mft_vcn++);
			if(err < B_NO_ERROR) {
//				dprintf("bt_ntfs_read_FILE_record had error getting block from MFT's fmap.\n");
				return err;
			}
			// This only calculates to >0 if the cluster size is > mft recordsize
			block_offset = (record_num*ntfs->mft_recordsize) % ntfs->cluster_size;
	
//			dprintf("\tgoing to read FILE record at block %Ld, offset %d.\n", block, block_offset);
	
			// Read the block
			retval = read_pos(ntfs->fd, block * ntfs->cluster_size + block_offset, temp_buf, read_len);
			if(retval != read_len) {
//				dprintf("bt_ntfs_read_FILE_record had error reading file record %Ld.\n", record_num);
				return EIO;
			}
			blocks_to_read--;
			temp_buf += read_len;
		}
	}
	
	return B_NO_ERROR;
}

static status_t bt_ntfs_walk(void *ns, void *dir, const char *file, char *newpath, vnode_id *vnid)
{
	ntfs_fs_struct *ntfs = (ntfs_fs_struct *)ns;
	ntfs_vnode *v = (ntfs_vnode *)dir;
	status_t err;
	ntfs_dcookie *d;
	uint8 dirbuf[1024];
	struct dirent *de = (struct dirent *)dirbuf;
	bool done = false;
	ntfs_vnode *v1;

//	dprintf("bt_ntfs_find called to find %s\n", path);
	
	err = bt_ntfs_opendir(ntfs, v, (void **)&d);
	if(err < B_NO_ERROR) return err;
	
	while(!done) {
		err = _bt_ntfs_readdir(ntfs, v, d, de);
		if(err < B_NO_ERROR) {
			goto out;
		}
		
		bt_ntfs_string_tolower(de->d_name);
//		dprintf("de->d_name = %s\n", de->d_name);
		
		if(!strcmp(file, de->d_name)) {
//			dprintf("found it at vnode %d.\n", (int)de->d_ino);
			*vnid = strip_high_16(de->d_ino);
			done = true;
		}
	}
	
	// We should have found it
	// Read in the vnode so we can do a consistancy check
	err = get_vnode(ntfs->nsid, *vnid, (void **)&v1);
	if(err < B_NO_ERROR) {
//		dprintf("ntfs_walk error calling get vnode on vnid %Ld.\n", *vnid);
		err = B_ERROR;
		goto out;
	}
	
	// Compare the sequence number stored in the returned vnode # with the sequence number in the
	// main vnode structure.
	if(v1->sequence_num != get_high_16(de->d_ino)) {
		dprintf("ntfs_walk dir entry's sequence number does not match real vnode's sequence number.\n");
		err = B_ERROR;
	} else {
		err = B_NO_ERROR;
	}

	put_vnode(ntfs->nsid, *vnid);
	
out:	
	bt_ntfs_closedir(ntfs, v, d);
	
	return err;
}

static status_t bt_ntfs_read_bios_block(ntfs_fs_struct *ntfs)
{
	int retval;
	char temp[512];
	int32 magic;
	
//	dprintf("ntfs_read_bios_block entry.\n");
	
	retval = read_pos(ntfs->fd, 0, temp, 512);
	if(retval != 512) {
//		dprintf("ntfs_read_bios_block error reading block.\n");
		return EIO;
	}
	
	// Check the magic number. 'NTFS' at offset 0x3
	magic = *(int32 *)(temp + 0x3);
	if(magic != 0x5346544e) {
//		dprintf("ntfs_read_bios_block sez magic number does not match, aborting mount...\n");
		return EINVAL;
	}		

	// Copy the bios block into the data structure. 
	memcpy(&ntfs->nbb.bytes_per_sector, temp+0x0b, 2);
//	dprintf("\tbytes per sector (according to the boot block) = %d.\n", ntfs->nbb.bytes_per_sector);
	memcpy(&ntfs->nbb.sectors_per_cluster, temp+0x0d, 1);
//	dprintf("\tsectors per cluster = %d.\n", ntfs->nbb.sectors_per_cluster);
	// calculate the cluster size
	ntfs->cluster_size = ntfs->nbb.bytes_per_sector*ntfs->nbb.sectors_per_cluster;
	if((ntfs->cluster_size == 0) || (ntfs->cluster_size % 512)) return EINVAL;
//	dprintf("\tcluster size = %ld.\n", ntfs->cluster_size);
	// calculate the compressed clusters size
	ntfs->clusters_per_compressed_block = 4096 / ntfs->cluster_size;
//	dprintf("\tclusters per compressed block = %ld.\n", ntfs->clusters_per_compressed_block);
	memcpy(&ntfs->nbb.reserved_sectors, temp+0x0e, 2);
	memcpy(&ntfs->nbb.media_type, temp+0x15, 1);
	memcpy(&ntfs->nbb.sectors_per_track, temp+0x18, 2);
	memcpy(&ntfs->nbb.num_heads, temp+0x1a, 2);
	memcpy(&ntfs->nbb.hidden_sectors, temp+0x1c, 4);	
	memcpy(&ntfs->nbb.total_sectors, temp+0x28, 8);
//	dprintf("\ttotal sectors = %Ld.\n", ntfs->nbb.total_sectors);
	memcpy(&ntfs->nbb.MFT_cluster, temp+0x30, 8);
//	dprintf("\tMFT cluster = %Ld.\n", ntfs->nbb.MFT_cluster);
	memcpy(&ntfs->nbb.MFT_mirror_cluster, temp+0x38, 8);
//	dprintf("\tMFT Mirror cluster = %Ld.\n", ntfs->nbb.MFT_mirror_cluster);
	memcpy(&ntfs->nbb.clusters_per_file_record, temp+0x40, 1);
//	dprintf("\tclusters per file record = %d.\n", ntfs->nbb.clusters_per_file_record);
	memcpy(&ntfs->nbb.clusters_per_index_block, temp+0x44, 4);
//	dprintf("\tclusters per index block = %d.\n", ntfs->nbb.clusters_per_index_block);
	memcpy(&ntfs->nbb.volume_serial, temp+0x48, 8);

	ntfs->num_clusters = ntfs->nbb.total_sectors / ntfs->nbb.sectors_per_cluster;

	// calculate the mft record size
	if(ntfs->nbb.clusters_per_file_record>0)
		ntfs->mft_recordsize= ntfs->cluster_size*ntfs->nbb.clusters_per_file_record;
	else
		ntfs->mft_recordsize= 1 << (- ntfs->nbb.clusters_per_file_record);
//	dprintf("\tmft recordsize = %ld.\n", ntfs->mft_recordsize);

	// check to make sure the mft record size is sane
	// 16k seems reasonable, probably something like 4k is realistic
	// also, check to make sure it's a multiple of 512
	if((ntfs->mft_recordsize >= 1024*16) || (ntfs->mft_recordsize % 512)) {
//		dprintf("NTFS: MFT record size > 16k, doesn't look good.");
		return EINVAL;
	}		

	return B_NO_ERROR;
}

// This function performs the fixup technique
static status_t bt_ntfs_fixup_buffer(void *buf, uint32 length)
{
	uint16 fixup_offset = *((uint16 *)(buf + 0x4));
	uint16 fixup_list_size = *((uint16 *)(buf + 0x6)) - 1;
	uint16 *fixup_list = (uint16 *)(buf + fixup_offset);
	uint32 sectors = length/512;
	int i;
	uint16 *fixup_spot;
	
//	dprintf("bt_ntfs_fixup_buffer called with buffer length %d.\n", length);
//	dprintf("\tfixup offset = %d\n\tfixup list size = %d\n\tfixup_pattern = 0x%x.\n", fixup_offset, fixup_list_size, fixup_list[0]);
	
	if(fixup_list_size < sectors) {
//		dprintf("bt_ntfs_fixup_buffer buffer was too small to do entire fixup list.\n");
		return B_ERROR;
	}
		
	for(i=1; i<=sectors; i++) {
		fixup_spot = ((uint16 *)(buf + i*512 - 0x2));
		if(*fixup_spot == fixup_list[0]) {
//			dprintf("ntfs_fixup buffer found valid pattern at buffer offset %d. writing %d.\n", i*512 - 0x2, fixup_list[i]);
			*fixup_spot = fixup_list[i];	
		} else {
//			dprintf("ntfs_fixup_buffer found invalid pattern, 0x%x\n", *fixup_spot);
			return EINVAL;
		}	
	}

	return B_NO_ERROR;
}

#if !USER

struct fs_ops bt_ntfs_ops = {
	bt_ntfs_mount,
	bt_ntfs_unmount,
	
	bt_ntfs_walk,
	bt_ntfs_readvnode,
	bt_ntfs_writevnode,

	bt_ntfs_open,
	bt_ntfs_close,
	bt_ntfs_read,
	bt_ntfs_ioctl,
	bt_ntfs_fstat,
	bt_ntfs_freadlink,

	bt_ntfs_opendir,
	bt_ntfs_closedir,
	bt_ntfs_readdir,
	bt_ntfs_rewinddir
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
		err = bt_ntfs_mount("/dev/disk/floppy/raw", &vol, &rootv);
	else
		err = bt_ntfs_mount(argv[1], &vol, &rootv);
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
		ntfs_vnode *filevnode;
		void *fcookie;
		uint8 buf[1024*32];
		struct stat *st = buf;
		struct ntfs_fmap_info *fmap_buf;
		char *p;
		ssize_t bytes_read;	
			
		// copy the path passed in
		temp_path = malloc(strlen(path)+1);
		if(!temp_path) {
			return ENOMEM;
		}
		strcpy(temp_path, path);
	
		err = bt_ntfs_readvnode(vol, rootv, &filevnode);
		if(err < B_NO_ERROR) {
			dprintf("error loading root vnode.\n");
			return 1;
		}
		
		dprintf("walking directories.\n");
	
		// walk through the directories, looking for the file
		while (temp_path) {
			p = strchr(temp_path, '/');
	
			if (p) *p = 0;
			
			if(strlen(temp_path)>0) {	
				// find the file in the current inode.
				dprintf("going to call walk. filevnode = 0x%x\n", filevnode);
				err = bt_ntfs_walk(vol, filevnode, temp_path, NULL, &vnid);
				if (err < B_NO_ERROR) {
					printf("ntfs_walk ntfs_find didn't find file.\n");
					return 1;
				}
				
				bt_ntfs_writevnode(vol, filevnode);
				
				err = bt_ntfs_readvnode(vol, vnid, &filevnode);
				if(err < B_NO_ERROR) {
					printf("couldn't load new vnode");
					return 1;
				}		

			}
			
			if (p) *p = '/';
			temp_path = p ? p+1 : NULL;
		}


		err = bt_ntfs_open(vol, filevnode, &fcookie);
		if(err < B_NO_ERROR) {
			printf("couldn't load file.\n");
			return 1;
		}
		
		bt_ntfs_fstat(vol, filevnode, fcookie, st);
		dprintf("st->st_ino = 0x%x, st->st_size = 0x%x.\n", st->st_ino, st->st_size);

		if(!S_ISLNK(st->st_mode)) {			
			uint32 offset = 0;
			
			bt_ntfs_ioctl(vol, filevnode, fcookie, 'fmap', &fmap_buf, 0);
			dprintf("fmap_buf = 0x%x.\n", fmap_buf);

			{
				FILE *fp;

				fp = fopen("test", "w+");
							
				bytes_read = 1;
				while(bytes_read > 0) {
					bytes_read = bt_ntfs_read(vol, filevnode, fcookie, offset, buf, 1024*32);		
					fwrite(buf, bytes_read, 1, fp);
					dprintf("read %d bytes at offset %d.\n", bytes_read, offset);
					offset += bytes_read;
				}
				fclose(fp);
			}
				
			{
				FILE *fp;
				
				fp = fopen("fmap", "w+");
				fwrite(fmap_buf, sizeof(struct ntfs_fmap_info) + sizeof(struct ntfs_fmap_block_run) * (fmap_buf->num_block_runs - 1), 1, fp);
				fclose(fp);
			}		
		} else {
			
			bytes_read = bt_ntfs_freadlink(vol, filevnode, fcookie, buf, 1024*32);
			{
				FILE *fp;
				
				fp = fopen("link", "w+");
				fwrite(buf, bytes_read, 1, fp);
				fclose(fp);
			}
			
		}
			
		bt_ntfs_close(vol, filevnode, fcookie);
		
		bt_ntfs_writevnode(vol, filevnode);
	
	}

	bt_ntfs_unmount(vol);

	return 0;
}

#endif
