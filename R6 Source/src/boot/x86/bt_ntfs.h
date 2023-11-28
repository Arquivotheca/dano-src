#ifndef NTFS_FS
#define NTFS_FS

#include <SupportDefs.h>


typedef	unsigned long long __u64;
typedef unsigned int __u32;
typedef unsigned short __u16;
typedef int __s32;
typedef short __s16;
typedef unsigned char __u8;
typedef char __s8;

#define LCN	uint64
#define VCN uint64
#define FILE_REC uint64

/* The first 11 inodes correspond to special files */
#define FILE_MFT      0
#define FILE_MFTMIRR  1
#define FILE_LOGFILE  2
#define FILE_VOLUME   3
#define FILE_ATTRDEF  4
#define FILE_ROOT     5
#define FILE_BITMAP   6
#define FILE_BOOT     7
#define FILE_BADCLUS  8
#define FILE_QUOTA    9
#define FILE_UPCASE  10

// Attribute Definitions
#define ATT_STD_INFO			0x10
#define ATT_ATT_LIST			0x20
#define ATT_FILE_NAME			0x30
#define ATT_VOLUME_VERSION		0x40
#define ATT_SECURITY_DESCRIPTOR 0x50
#define ATT_VOLUME_NAME			0x60
#define ATT_VOLUME_INFORMATION	0x70
#define ATT_DATA				0x80
#define ATT_INDEX_ROOT			0x90
#define ATT_INDEX_ALLOCATION	0xA0
#define ATT_BITMAP				0xB0
#define ATT_SYMBOLIC_LINK		0xC0
#define ATT_EA_INFORMATION		0xD0
#define ATT_EA					0xE0

// This is the structure of the bootsector of a NTFS partition.
typedef struct {
    __u32   magic;						// 0x03 'NTFS'
	__u16	bytes_per_sector;			// 0x0b
	__u8	sectors_per_cluster;		// 0x0d
	__u16	reserved_sectors;			// 0x0e
	__u8	media_type;					// 0x15
	__u16	sectors_per_track;			// 0x18
	__u16	num_heads;					// 0x1a
	__u32	hidden_sectors;				// 0x1c
	__u64	total_sectors;				// 0x28
	LCN		MFT_cluster;				// 0x30
	LCN		MFT_mirror_cluster;			// 0x38
	__s8	clusters_per_file_record;	// 0x40
	__u32	clusters_per_index_block;	// 0x44
	__u64	volume_serial;				// 0x48
	__u32	checksum;					// ?	
} ntfs_bios_block;

#define FILE_REC_MAGIC 0x454c4946 // 'FILE'

// NTFS FILE record
typedef struct {
	uint32 magic; // Should be 'FILE'
	uint16 fixup_offset;
	uint16 fixup_list_size;
	uint64 filler;
// 0x10
	uint16 seq_number;
	uint16 hard_link_count;
	uint16 offset_to_attributes;
	uint16 flags;
	uint32 real_FILE_record_size;
	uint32 alloc_FILE_record_size;
// 0x20
	FILE_REC base_file_rec;
	uint16 max_attr_ident;
// 0x2a
	uint16 fixup_pattern;
} ntfs_FILE_record;

#define FILE_REC_IN_USE_FLAG 0x1
#define FILE_REC_IS_DIR_FLAG 0x2

// NTFS attribute structures
typedef struct {
	uint32 type;
	uint32 length;
	uint8  non_resident;
	uint8  name_len;		// will be 0 if no name
	uint16 value_offset;
	uint16  compressed;
	uint16 identificator;
} ntfs_attr_header;

typedef struct {
	uint32 specific_value_length;
	uint16 specific_value_offset;
	uint16 indexed_flag;
} ntfs_attr_resident;

#define KNOWN_COMPRESSION_ENGINE 0x4

typedef struct {
	VCN    starting_VCN;
	VCN	   last_VCN;
	uint16 runlist_offset;
	uint16 compression_engine; // 2 bytes or more?
	uint32 filler;
	uint64 allocated_size;
	uint64 real_size;
	uint64 initialized_data_size;		
} ntfs_attr_nonresident;

// Structures for the individual attributes
typedef struct {
	uint64 file_creation_time;		// Note: These times are stored
	uint64 last_modification_time;	// in NT's format.
	uint64 last_FILE_rec_mod_time;
	uint64 last_access_time;
	uint32 DOS_permissions;
	// Theres some space here of length 0x0C. 
} ntfs_attr_STD_INFO;

// few defines for STD_INFO
#define DOS_COMPRESSED_FLAG 0x0800
#define DOS_SYMLINK_FLAG    0x0400
#define DOS_ARCHIVE_FLAG    0x0020
#define DOS_SYSTEM_FLAG     0x0004
#define DOS_HIDDEN_FLAG     0x0002
#define DOS_RO_FLAG         0x0001

typedef struct {
	uint32 type;
	uint16 rec_length;
	uint8  name_size;
	uint8  filler;
	VCN    starting_VCN;
	FILE_REC attribute_record;
	uint16 identificator;
	// The name goes here, and is length name_size*2
} ntfs_attr_ATT_LIST_record;

typedef struct {
	FILE_REC container_directory;
	uint64 file_creation_time;		// Note: These times are stored
	uint64 last_modification_time;	// in NT's format.
	uint64 last_FILE_rec_mod_time;
	uint64 last_access_time;
	uint64 allocated_size;
	uint64 real_size;
	uint64 flags;	
	uint8  name_length;
	uint8  file_name_type;
	// The name goes here, length = name_length * 2
} ntfs_attr_FILE_NAME;

// Few defines for FILE_NAME attribute
#define FILE_NAME_ATTR_DIR_FLAG 0x10000000
#define FILE_NAME_ATTR_COMPRESSED_FLAG DOS_COMPRESSED_FLAG
#define FILE_NAME_ATTR_ARCHIVE_FLAG DOS_ARCHIVE_FLAG
#define FILE_NAME_ATTR_SYSTEM_FLAG DOS_SYSTEM_FLAG
#define FILE_NAME_ATTR_HIDDEN_FLAG DOS_HIDDEN_FLAG
#define FILE_NAME_ATTR_RO_FLAG DOS_RO_FLAG

#define FILE_NAME_ATTR_POSIX_FLAG 0
#define FILE_NAME_ATTR_UNICODE_FLAG 1
#define FILE_NAME_ATTR_DOS_FLAG 2
#define FILE_NAME_ATTR_BOTH_FLAG 3

typedef struct {
	// This one is empty. The specific part of this attribute is just a unicode string
} ntfs_attr_VOLUME_NAME;

#define VOL_INFO_CHKDSK_FLAG 0x01

typedef struct {
	uint64 unknown;
	uint8  maj_version;
	uint8  min_version;
	uint16 flags;
} ntfs_attr_VOLUME_INFO;

typedef struct {
	// Same as above. The specific part of this attribute is just a string of data.
} ntfs_attr_DATA;

typedef struct {
	// Again, empty.
} ntfs_attr_INDEX_ALLOCATION;

typedef struct {
	// This is really flaky. Not sure what a lot of this means
	uint32 index_type;
	uint32 always1;
	uint32 index_record_size;
	uint32 clusters_per_index_record;
	uint32 always0x10;
	uint32 size_of_entries; // +0x10;
	uint32 same_as_above;
	uint32 flags;
} ntfs_attr_INDEX_ROOT;

#define INDEX_ROOT_ATTR_LARGE_INDEX_FLAG 0x01

#define INDEX_BUFFER_MAGIC 0x58444e49 // 'INDX'

typedef struct {
	uint32 magic; // should be 'INDX'
	uint16 fixup_offset;
	uint16 num_fixups;
	uint64 space;
	VCN    buffer_VCN;
	uint16 header_size;				// -0x18
	uint16 space1;
	uint32 inuse_buffer_length;		// -0x18
	uint32 total_buffer_length;		// -0x18
	uint32 leaf_node;
	// theres some fixup stuff after this
} ntfs_index_buffer_header;

#define INDEX_ENTRY_SUB_NODE 0x01
#define INDEX_ENTRY_LAST_ENTRY 0x02

typedef struct {
	FILE_REC record;
	uint16 entry_size;
	uint16 stream_length;
	uint8  flags;
	uint8  filler;
	uint16 filler1;
	// this part contains a copy of the indexed attribute.
	// for directories, this is the FILE_NAME attribute, or 0x30.
} ntfs_index_entry;

#endif
