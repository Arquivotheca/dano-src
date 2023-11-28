#ifndef _BOOT_BIOS_H
#define _BOOT_BIOS_H

#include <drivers/Drivers.h>

void set_video_mode(uchar modenum);
int bios_key_hit(void);
int bios_get_key(void);
int bios_shift_state(void);
status_t bios_get_number_hard_drives(void);
status_t get_drive_geometry(uint32 id, device_geometry *geometry);
status_t read_disk(uint32 id, uint64 sector, void *buffer, uint32 num);

void bios_move_cursor(int x, int y);

extern uchar *scratch;

uint32 construct_memory_map(uint32 *map, uint32 entries);

struct _apm_bios_info;

status_t initialize_apm(struct _apm_bios_info *info);

struct vbe_info {
	uint32	VbeSignature;
	uint16	VbeVersion;
	uint32	OemStringPtr;
	char	Capabilities[4];
	uint32	VideoModePtr;
	uint16	TotalMemory;
	uint16	OemSoftwareRev;
	uint32	OemVendorNamePtr;
	uint32	OemProductNamePtr;
	uint32	OemProductRevPtr;
	uchar	Reserved[222];
	uchar	OemData[256];
} _PACKED;

int vesa_get_info(struct vbe_info *info);

struct vbe_mode_info {
	uint16	ModeAttributes;
	uchar	WinAAttributes;
	uchar	WinBAttributes;
	uint16	WinGranularity;
	uint16	WinSize;
	uint16	WinASegment;
	uint16	WinBSegment;
	uint32	WinFuncPtr;
	uint16	BytesPerScanLine;
	uint16	XResolution;
	uint16	YResolution;
	uchar	XCharSize;
	uchar	YCharSize;
	uchar	NumberOfPlanes;
	uchar	BitsPerPixel;
	uchar	NumberOfBanks;
	uchar	MemoryModel;
	uchar	BankSize;
	uchar	NumberOfImagePages;
	uchar	Reserved1;
	uchar	RedMaskSize;
	uchar	RedFieldPosition;
	uchar	GreenMaskSize;
	uchar	GreenFieldPosition;
	uchar	BlueMaskSize;
	uchar	BlueFieldPosition;
	uchar	RsvdMaskSize;
	uchar	RsvdFieldPosition;
	uchar	DirectColorModeInfo;
	uint32	PhysBasePtr;
	uint32	OffScreenMemOffset;
	uint16	OffScreenMemSize;
	uchar	Reserved2[206];
} _PACKED;

int vesa_get_mode_info(uint16 modenum, struct vbe_mode_info *info);
int vesa_set_mode(uint16 modenum);
int vesa_set_palette(void *palette);

status_t enable_a20(void);
status_t disable_a20(void);

/* functions to interact with DOS */

/* get_bpb() returns length of BPB or an error code */
status_t get_bpb(uchar drive_letter, uchar bpb[53]);
status_t get_canonical_name(const char *path, char canonicalname[261]);

#endif /* _BOOT_BIOS_H */
