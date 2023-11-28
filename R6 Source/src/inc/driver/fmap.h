#ifndef _FMAP_H
#define _FMAP_H

#define FMAP_TYPE_BLOCK 'kclb'
#define FMAP_TYPE_BYTE 'etyb'

struct fmap_block_run {
	int32 block;
	int32 num_blocks;
};

struct fmap_byte_run {
	off_t byte;
	off_t num_bytes;
};

struct fmap_info
{
	uint32 checksum;

	uint32 size;
	uint32 type;
	uint32 bios_id;
	off_t offset;

	union {
		struct {
			int32 block_size;
			int32 num_blocks;
			int32 num_block_runs;
			struct fmap_block_run block_runs[1];
		} block;
		struct {
			off_t num_bytes;
			int32 num_byte_runs;
			struct fmap_byte_run byte_runs[1];
		} byte;
	} u;
};

#endif
