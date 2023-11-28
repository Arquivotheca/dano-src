/* ++++++++++
	intel_map.h
	Copyright (C) 1997 Be Inc.  All Rights Reserved.

	Structures that define the first block and the partition
	table of a intel file system.
 ++++++++++ */

#ifndef _INTEL_MAP_H
#define _INTEL_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int32		id;
	char		name[256];
	bool		hidden;
} partition_type;

// types glommed from: http://www.netins.net/showcase/spectre/appi.htm
//					   http://linux.ucs.indiana.edu/hypermail/linux/kernel/9510/0029.html
//					   fdisk.c

#define BEOS_PARTITION		0xeb
#define EXTENDED_PARTITION1 0x05
#define EXTENDED_PARTITION2 0x0f
#define EXTENDED_PARTITION3 0x85

extern partition_type types[];

#define BOOT_CODE_LEN		0x1be
#define MAX_PARTITION		4
#define BOOT_SIGNATURE		0xAA55
#define ACTIVE_PARTITION	0x80

typedef struct {
        uchar		bootid;
        uchar		beghd;
        uchar		begsec;
        uchar		begcyl;
        uchar		systid;
        uchar		endhd;
        uchar		endsec;
        uchar		endcyl;
		uchar		relsect[4];
		uchar		numsect[4];
} fdisk;

typedef struct {
        uchar		boot[BOOT_CODE_LEN];
        fdisk		part[MAX_PARTITION];
        uchar		signature[2];
} bootsector; 

struct _partition_info_ {
	uchar	type;		/* type code */
	int64	sblock;		/* starting block */
	int64	nblocks;	/* ending block */
};

bool id_intel_partition(uchar *sb, int32 size);

status_t get_nth_intel_partition(int32 dev, uchar *sb, uint64 block_num,
    int32 index, struct _partition_info_ *opinfo,
    struct _partition_info_ *optinfo, uchar *osb, fdisk **ofdisk);

#ifdef __cplusplus
}
#endif

#endif /* _INTEL_MAP_H */
