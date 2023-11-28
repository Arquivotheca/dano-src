/* ++++++++++
	bfs_map.h
	Copyright (C) 1997 Be Inc.  All Rights Reserved.

	Structures that define the first block and the partition
	table of a Be BFS file system.
 ++++++++++ */

#ifndef _BFS_MAP_H
#define _BFS_MAP_H

#include <time.h>
#include <sys/types.h>



typedef struct block_run
{
	uint   allocation_group;
	ushort start;
	ushort len;       /* in blocks */
} block_run;

typedef block_run inode_addr;

#define NUM_DIRECT_BLOCKS    12

typedef struct data_stream
{
	block_run direct[NUM_DIRECT_BLOCKS];
	off_t     max_direct_range;
	block_run indirect;
	off_t     max_indirect_range;
	block_run double_indirect;
	off_t     max_double_indirect_range;  /* not really necessary */
	off_t     size;
} data_stream;

typedef struct disk_super_block   /* super block as it is on disk */
{
	char         name[B_OS_NAME_LENGTH];
	int32        magic1;
	int32        fs_byte_order;

	uint32       block_size;             /* in bytes */
	uint32       block_shift;            /* block_size == (1 << block_shift) */

    off_t    	 num_blocks;
	off_t    	 used_blocks;

	int32        inode_size;             /* # of bytes per inode */

	int32        magic2;
	int32        blocks_per_ag;          /* in blocks */
    int32        ag_shift;               /* # of bits to shift to get ag num */
	int32        num_ags;                /* # of allocation groups */
	int32        flags;                  /* if it's clean, etc */

	block_run    log_blocks;             /* a block_run of the log blocks */
	off_t    	 log_start;              /* block # of the beginning */
	off_t    	 log_end;                /* block # of the end of the log */

	int32        magic3;
	inode_addr   root_dir;
	inode_addr   indices;

	int32        pad[8];                 /* extra stuff for the future */
} disk_super_block;

#define BFS_CLEAN   0x434c454e           /* 'CLEN', for flags field */ 
#define BFS_DIRTY   0x44495254           /* 'DIRT', for flags field */ 

#define SUPER_BLOCK_MAGIC1   0x42465331    /* BFS1 */
#define SUPER_BLOCK_MAGIC2   0xdd121031
#define SUPER_BLOCK_MAGIC3   0x15b6830e

#define BFS_BIG_ENDIAN       0x42494745    /* BIGE */
#define BFS_LITTLE_ENDIAN    0x45474942    /* EGIB */

/* flags for the bfs_info flags field */
#define FS_READ_ONLY         0x00000001

#endif /* _BFS_MAP_H */
