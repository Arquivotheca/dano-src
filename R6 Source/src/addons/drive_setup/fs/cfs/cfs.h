#ifndef CFS_H
#define CFS_H

#include <SupportDefs.h>
#include <ByteOrder.h>

#define CFS_ID_1 B_HOST_TO_BENDIAN_INT32('cfs1')
#define CFS_ID_2 B_HOST_TO_BENDIAN_INT32('cfs2')
#define CFS_CURRENT_VERSION 1

#define cfs_superblock_1_offset ((off_t)512)

//#define cfs_compressed_block_size 8192
#define cfs_compressed_block_size 16384
//#define cfs_compressed_block_size 32768
//#define cfs_compressed_block_size 65536

#define cfs_align_block(x) (((x) + 7) & ~7)
#define cfs_string_size(x) (cfs_align_block(strlen(x)+1))

typedef struct cfs_super_block_1 {
	int32 cfs_id_1;
	int32 cfs_version;
	int32 flags;
	int32 create_time;
	int32 dev_block_size;
	int32 fs_block_size;
	off_t size;
	off_t super_block_2;
	off_t root;
	off_t log_pos;
	off_t log_size;
	off_t reserved_space_1_pos;
	off_t reserved_space_1_size;
	off_t reserved_space_2_pos;
	off_t reserved_space_2_size;
} cfs_super_block_1;

#define CFS_SB1_FLAGS_FS_READ_ONLY 0x00000001

typedef struct cfs_super_block_2 {
	int32 cfs_id_2;
	int32 cfs_version;
	int32 flags;				/* 0 */
	int32 modification_time;
	off_t free_list;
	off_t unlinked_entries;
	uint8 volume_name[32];
} cfs_super_block_2;

#define CFS_LOG_ID B_HOST_TO_BENDIAN_INT32('logh')
typedef struct cfs_log_info {
	int32	log_id;
	uint32	version;
	off_t	head;
	off_t	tail;
	uint32	last_version;
	int32	checksum;
} cfs_log_info;

#define CFS_LOG_ENTRY_ID B_HOST_TO_BENDIAN_INT32('loge')
#define max_blocks_per_log_entry 63

typedef struct cfs_log_entry_header {
	uint32 log_entry_id;
	uint32 num_blocks;
	off_t  blocks[max_blocks_per_log_entry];
} cfs_log_entry_header;

typedef struct cfs_free_block {
	off_t next;					/* if next & 1 is 1 then only 8 bytes is free */
	off_t size;					/* and the size field is not valid */
} cfs_free_block;

typedef struct cfs_data_block {
	off_t  next;
	off_t  data;
	size_t disk_size;
	size_t raw_size;			/* if zero, uncompressed */
} cfs_data_block;

typedef struct cfs_entry_info {
	off_t  parent;
	off_t  next_entry;
	off_t  data;	                /* block list / directory entries */
	off_t  attr;
	off_t  name_offset;
	uint32 create_time;
	uint32 modification_time;
	uint32 flags;
	uint32 cfs_flags;
} cfs_entry_info;

#define CFS_FLAG_COMPRESS_FILE 0x0001

#define CFS_FLAG_DEFAULT_FLAGS CFS_FLAG_COMPRESS_FILE

//size_t get_entry_size(cfs_entry_info *entry);

#define CFS_MAX_NAME_LEN 256

#if 0
#define CFS_NODE_ENTRY_MIN_SIZE 40
#define CFS_NODE_ENTRY_NAME_LEN 256

#define CFS_NODE_FLAGS_TYPE       0x03
#define CFS_NODE_FLAGS_TYPE_ATTR  0x06
#define CFS_NODE_FLAGS_TYPE_DIR   0x01
#define CFS_NODE_FLAGS_TYPE_FILE  0x02
#define CFS_NODE_FLAGS_TYPE_LINK  0x03
#define CFS_NODE_FLAGS_EXECUTE    0x10
#define CFS_NODE_FLAGS_WRITE      0x20
#endif

#endif
