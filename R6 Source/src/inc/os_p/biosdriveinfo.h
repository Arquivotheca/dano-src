#ifndef BIOSDRIVEINFO_H
#define BIOSDRIVEINFO_H

struct drive_hash {
	unsigned long long  offset;     /* starting offset */
	unsigned int        size;       /* number of bytes */
	unsigned int        hash;       /* checksum */
};

struct bios_drive_info {
	char beos_id[32];               /* device name */
	unsigned char bios_id;
	unsigned int c, h, s;
	unsigned int num_hashes;
	struct drive_hash hashes[5];
};

/* vyt: should access through normal exported kernel function channels */
extern struct bios_drive_info *bios_drive_info;
extern unsigned int boot_calculate_hash(void *buffer, unsigned int bytes);

#endif /* BIOSDRIVEINFO_H */
