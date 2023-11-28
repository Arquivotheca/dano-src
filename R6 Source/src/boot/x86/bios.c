#include <drivers/KernelExport.h>

#include <apm.h>

#include "bios.h"
#include "cpu_asm.h"
#include "platform.h"

struct regs {
	uint32 eax, ecx, edx, ebx, ebp, esi, edi;
};

extern int int86(uchar intnum, struct regs *in, struct regs *out);

#define CF 0x0001
#define ZF 0x0040

uchar *scratch;

void set_video_mode(uchar modenum)
{
	struct regs r;
	r.eax = modenum;
	int86(0x10, &r, &r);
}

int bios_key_hit(void)
{
	struct regs r;
	r.eax = 0x100;
	/* ZF clear if key available */
	return (int86(0x16, &r, &r) & ZF) ? 0 : (r.eax & 0xffff);
}

int bios_get_key(void)
{
	struct regs r;
	r.eax = 0;
	int86(0x16, &r, &r);
	return (r.eax & 0xffff);
}

int bios_shift_state(void)
{
	struct regs r;
	r.eax = 0x200;
	int86(0x16, &r, &r);
	return (r.eax & 0xff);
}

void bios_move_cursor(int x, int y)
{
	struct regs r;
	r.eax = 0x200;
	r.ebx = 0;
	r.edx = y * 0x100 + x;
	int86(0x10, &r, &r);
}

status_t
bios_get_number_hard_drives(void)
{
	struct regs r;
	r.eax = 0x800;
	r.edx = 0x80;
	if (int86(0x13, &r, &r) & CF) {
		dprintf("Error fetching drive parameters\n");
		return B_ERROR;
	}

	return (r.edx & 0xff);
}

status_t
get_drive_geometry(uint32 id, device_geometry *geometry)
{
	struct regs r;

	r.eax = 0x800;
	r.edx = id & 0xff;
	if (int86(0x13, &r, &r) & CF) {
		dprintf("Error getting drive geometry for 0x%lx\n", id);
		return B_ERROR;
	}
	geometry->cylinder_count =
			((r.ecx & 0xff00) >> 8) + ((r.ecx & 0xc0) << 2) + 1;
	geometry->sectors_per_track = r.ecx & 0x3f;
	geometry->head_count = ((r.edx & 0xff00) >> 8) + 1;
	geometry->bytes_per_sector = 0x200;

	/* workaround for bad CD-floppy emulation geometry reported by ACER
	 * TravelMate 512T laptop (cx = 0x5012, dh = 0x02) */
	if (	(geometry->cylinder_count == 81) &&
			(geometry->sectors_per_track == 18) &&
			(geometry->head_count == 3)) {
		geometry->cylinder_count--;
		geometry->head_count--;
	}

	return B_OK;
}

#if 0
status_t
get_extended_drive_geometry(uint32 device_cookie, device_geometry *geometry)
{
	struct regs r;

	r.eax = 0x4800;
	r.edx = device_cookie & 0xff;
	r.esi = 0x1000;
	if ((int86(0x13, &r, &r) & CF) == 0) {
		geometry->cylinder_count = *(uint64 *)(scratch + 0x10);
		geometry->sectors_per_track = 1;
		geometry->head_count = 1;
		geometry->bytes_per_sector = *(uint16 *)(scratch + 0x18);

		/* vyt: more sanity checking */
		if (geometry->cylinder_count && geometry->bytes_per_sector)
			return B_OK;
	}

	return get_normal_drive_geometry(device_cookie, geometry);
}
#endif

struct disk_address_packet {
	uchar	size;
	uchar	reserved;
	uint16	sectors;
	uint16	buffer_ofs;
	uint16	buffer_seg;
	uint64	sectornum;
} _PACKED;

status_t read_disk(uint32 id, uint64 sector64, void *buffer, uint32 num)
{
	struct regs r;
	uint32 c, h, s, t, sector32, numread;
	int retries;
	struct disk_address_packet *dap;

	static uint32 last_id = -1;
	static device_geometry g;

	if (last_id == id) goto cached;

	if (get_drive_geometry(id, &g) < B_OK) {
		dprintf("Could not get geometry for drive %lx\n", id);
		goto err;
	}

	if (!g.head_count || !g.sectors_per_track) {
		dprintf("Bad geometry for drive %lx\n", id);
		goto err;
	}

	last_id = id;

cached:
	sector32 = (uint32)(sector64 & 0xffffffff);
	
	if (sector32 + num >= g.head_count * g.sectors_per_track * g.cylinder_count)
		goto extended_read;
	
	while (num) {
		uint32 num1;

		c = sector32 / (g.head_count * g.sectors_per_track);
		if (c > 1023) {
			dprintf("Cannot read past cylinder 1023 without extended BIOS support\n");
			goto err;
		}
		t = sector32 % (g.head_count * g.sectors_per_track);
		h = t / g.sectors_per_track;
		s = t % g.sectors_per_track + 1;

		if (num + s - 1 > g.sectors_per_track)
			num1 = g.sectors_per_track - s + 1;
		else
			num1 = num;

		/* vyt: should also check spanning 64k boundary for floppies */

		for (retries=3;retries>=0;retries--) {
			if (retries < 3) {
				dprintf("retrying read %lx, %lx\n", id, sector32);
				r.eax = 0;
				r.edx = id;
				int86(0x13, &r, &r);
				spin(100000);
			}
	
			r.eax = 0x200 + num1;
			r.ebx = 0x1000;
			r.ecx = (c & 0xff) * 0x100 + ((c >> 2) & 0xc0) + s;
			r.edx = h * 0x100 + id;

			platform_set_disk_state(1);
			if ((int86(0x13, &r, &r) & CF) == 0) {
				memcpy(buffer, scratch + 0x1000, num1 * 0x200);
				break;
			}
		}

		if (retries == -1) {
			dprintf("Retries exhausted. Read failing.\n");
			goto err;
		}
		
		num -= num1;
		sector32 += num1;
		(char *)buffer += num1 * 512;
	}

	goto success;

extended_read:
	dap = (struct disk_address_packet *)(scratch + 0x1000);
	numread = 0;
	while (numread < num) {
		uint32 n;
		dap->size = 0x10;
		dap->reserved = 0;
		/* make sure doesn't write past end of scratch buffer */
		dap->sectors = n = ((num - numread) > 0x40) ? 0x40 : (num - numread);
		dap->buffer_ofs = 0;
		dap->buffer_seg = ((uint32)scratch >> 4) + 0x108;
		dap->sectornum = sector64 + numread;
		r.eax = 0x4200;
		r.edx = id & 0xff;
		r.esi = 0x1000;
		platform_set_disk_state(1);
		if ((int86(0x13, &r, &r) & CF) != 0) {
			dprintf("Error reading sector %Lx (%lx)\n", sector64 + numread, n);
			goto err;
		}
		memcpy(buffer + numread * 0x200, scratch + 0x1080, n * 0x200);
		numread += n;
	}

success:
	platform_set_disk_state(0);
	return B_OK;

err:
	platform_set_disk_state(0);
	return B_ERROR;
}

struct smap {
	uint64 base;
	uint64 len;
	uint32 type;
};

enum smap_types { SMAP_FREE = 1, SMAP_RESERVED, SMAP_ACPI_RECLAIM, SMAP_ACPI_NVS };

/* each entry is 2 uint32's */
uint32 construct_memory_map(uint32 *map, uint32 entries)
{
	struct regs r;
	uint32 continuation, index, m;
	volatile struct smap *s;

	memset(map, 0, entries * 2 * sizeof(uint32));

	continuation = 0;
	index = 0;
	m = 0;
	s = (struct smap *)(scratch + 0x1000);
	do {
		r.eax = 0xE820;
		r.ebx = continuation;
		r.ecx = 20;
		r.edx = 0x534D4150;
		r.edi = 0x1000;
		if (int86(0x15, &r, &r) & CF) break;
		continuation = r.ebx;
		if ((r.ecx == 20) && (s->type == SMAP_FREE)) {
			map[index++] = (uint32)(s->base & 0xffffffff);
			map[index++] = (uint32)(s->len & 0xffffffff);
			if (map[index-2] + map[index-1] > m)
				m = map[index-2] + map[index-1];
		}
	} while ((r.ebx != 0) && (r.ecx == 20) && (index < 2 * entries));

	/* On some machines, Win98 intercepts the above call and returns no memory
	 * free above 1 MB, so we need this cheesy work around. */
	if (m < 0x400000)
		index = 0;
	else
		dprintf("Found memory map (Method 1)\n");

	/* failing that, try alternative methods for finding memory */
	if (index == 0) {
		int86(0x12, &r, &r);	/* ax = K starting at 0 */
		map[index++] = 0;
		map[index++] = (r.eax & 0xffff) * 1024;

		r.eax = 0xe801;
		if (!(int86(0x15, &r, &r) & CF) &&
			((r.ecx & 0xffff) || (r.edx & 0xffff))) {
			/*   AX = extended memory 1M-16M in K
			 *   BX = extended memory above 16M in 64K
			 *   CX = configured memory 1M-16M in K
			 *   DX = configured memory above 16M in 64K */
			if (r.ecx & 0xffff) {
				map[index++] = 0x100000;
				map[index++] = (r.ecx & 0xffff) * 1024;
			}
			if (r.edx & 0xffff) {
				map[index++] = 0x1000000;
				map[index++] = (r.edx & 0xffff) * 64 * 1024;
			}
			dprintf("Found memory map (Method 2)\n");
		} else {
			r.eax = 0x8800;
			int86(0x15, &r, &r); /* ax = K starting at 1 M */
			if (r.eax & 0xffff) {
				map[index++] = 0x100000;
				map[index++] = (r.eax & 0xffff) * 1024;
				dprintf("Found memory map (Method 3)\n");
			} else {
				/* vyt: test this code */
				uchar high, low;
				write_io_8(0x70, 0x18); spin(100);
				high = read_io_8(0x71); spin(100);
				write_io_8(0x70, 0x17); spin(100);
				low = read_io_8(0x71); spin(100);
				map[index++] = 0x100000;
				map[index++] = (high * 0x100 + low) * 1024;
				dprintf("Found memory map (Method 4)\n");
			}
		}
	}

	/* return size of memory */
	m = 0;
	for (index=0;index<2 * entries;index+=2) {
		if (map[index] + map[index+1] > m)
			m = map[index] + map[index+1];
	}

	return m;
}

status_t initialize_apm(struct _apm_bios_info *info)
{
	struct regs r;

	/* APM init check */	
	r.eax = 0x5300;
	r.ebx = 0;
	if (	(int86(0x15, &r, &r) & CF) ||
			((r.ebx & 0xffff) != 0x504d) ||	/* 'PM' signature */
			!(r.ecx & 2))					/* 32-bit pmode interface */
		return B_ERROR;

	info->version = r.eax & 0xffff;
	info->flags = r.ecx & 0xffff;

	r.eax = 0x5304;			/* disconnect */
	r.ebx = 0;
	int86(0x15, &r, &r);

	r.eax = 0x5303;			/* connect to 32-bit protected mode interface */
	r.ebx = 0;
	if (int86(0x15, &r, &r) & CF)
		return B_ERROR;

	info->cs32 = r.eax & 0xffff;
	info->offset = r.ebx;
	info->cs16 = r.ecx & 0xffff;
	info->ds = r.edx & 0xffff;
	info->cs_len = r.esi & 0xffff;
	info->ds_len = r.edi & 0xffff;

	return B_OK;
}

void scc_out_hw_dependent (char c)
{
	static bool enabled = TRUE;
	bigtime_t start = system_time();
	struct regs r;

	if (!enabled) return;

	r.eax = 0x100 + c;
	r.edx = 0;
	int86(0x14, &r, &r);

	if (system_time() > start + 1000)
		enabled = FALSE;
}

static uint32 relocate_pointer(uint32 orig, uint32 delta)
{
	uint16 seg, off;
	uint32 linear;

	seg = (orig >> 16) & 0xffff;
	off = orig & 0xffff;

	linear = seg * 0x10 + off;
	if ((linear >= (uint32)(scratch + 0x1000)) &&
			(linear < (uint32)(scratch + 0x1000 + sizeof(struct vbe_info))))
		linear += delta;

	return linear;
}

int vesa_get_info(struct vbe_info *info)
{
	struct regs r;
	uint32 delta;

	memcpy(scratch + 0x1000, info, sizeof(*info));
	r.eax = 0x4f00;
	r.edi = 0x1000;
	int86(0x10, &r, &r);
	if (r.eax & 0xff00) return B_ERROR;
	memcpy(info, scratch + 0x1000, sizeof(*info));
	delta = (uint32)info - (uint32)scratch - 0x1000;
	info->OemStringPtr = relocate_pointer(info->OemStringPtr, delta);
	info->VideoModePtr = relocate_pointer(info->VideoModePtr, delta);
	info->OemVendorNamePtr = relocate_pointer(info->OemVendorNamePtr, delta);
	info->OemProductNamePtr = relocate_pointer(info->OemProductNamePtr, delta);
	info->OemProductRevPtr = relocate_pointer(info->OemProductRevPtr, delta);
	return B_OK;
}

int vesa_get_mode_info(uint16 modenum, struct vbe_mode_info *info)
{
	struct regs r;

	memcpy(scratch + 0x1000, info, sizeof(*info));
	r.eax = 0x4f01;
	r.ecx = modenum;
	r.edi = 0x1000;
	int86(0x10, &r, &r);
	if (r.eax & 0xff00) return B_ERROR;
	memcpy(info, scratch + 0x1000, sizeof(*info));
	return B_OK;
}

int vesa_set_mode(uint16 modenum)
{
	struct regs r;
	r.eax = 0x4f02;
	r.ebx = modenum | 0x4000;
	int86(0x10, &r, &r);
	return (r.eax & 0xff00) ? B_ERROR : B_OK;
}

int vesa_set_palette(void *palette)
{
	struct regs r;
	r.eax = 0x4f09;
	r.ebx = 0x0000;
	r.ecx = 0x0100;
	r.edx = 0x0000;
	r.edi = 0x1000;
	memcpy(scratch + 0x1000, palette, 0x400);
	int86(0x10, &r, &r);
	return (r.eax & 0xff00) ? B_ERROR : B_OK;
}

static bool a20_enabled(void)
{
	bool enabled;
	volatile unsigned char *b1, *b2;

	b1 = (volatile unsigned char *)4;
	b2 = (volatile unsigned char *)0x100004;
	*b2 = ~(*b1);
	enabled = (*b1 != *b2);
	*b2 = ~(*b2);

	return enabled;
}

static status_t wait_8042(uchar mask, uchar value)
{
	int i;

	for (i=0;i<100;i++) {
		if ((read_io_8(0x64) & mask) == value)
			return B_OK;
		spin(100);
	}

	return B_ERROR;
}

static status_t read_60(void)
{
	if (wait_8042(0x01, 0x01)) return B_ERROR;
	spin(7);
	return read_io_8(0x60);
}

static status_t write_60(uchar val)
{
	if (wait_8042(0x02, 0x00)) return B_ERROR;
	write_io_8(0x60, val);
	return wait_8042(0x02, 0x00);
}

static status_t write_64(uchar val)
{
	if (wait_8042(0x02, 0x00)) return B_ERROR;
	write_io_8(0x64, val);
	return wait_8042(0x02, 0x00);
}

status_t enable_a20(void)
{
	uint32 state;
	status_t val;
	struct regs r;

	if (a20_enabled()) {
		dprintf("A20 already enabled\n");
		return B_OK;
	}

	state = disable_interrupts();

	if ((write_64(0xd1) == B_OK) &&
		(write_60(0xdf) == B_OK) &&
		a20_enabled()) {
		restore_interrupts(state);
		dprintf("A20 enabled (Method 1)\n");
		return B_OK;
	}

	if ((write_64(0xd0) == B_OK) &&
		((val = read_60()) >= 0) &&
		(write_64(0xd1) == B_OK) &&
		(write_60(val | 2) == B_OK) &&
		a20_enabled()) {
		restore_interrupts(state);
		dprintf("A20 enabled (Method 2)\n");
		return B_OK;
	}

	restore_interrupts(state);

	r.eax = 0x2401;
	int86(0x15, &r, &r);
	if (a20_enabled()) {
		dprintf("A20 enabled (Method 3)\n");
		return B_OK;
	}

	return B_ERROR;
}

status_t disable_a20(void)
{
	uint32 state;
	status_t val;
	struct regs r;

	state = disable_interrupts();

	if ((write_64(0xd1) == B_OK) &&
		(write_60(0xdd) == B_OK) &&
		(a20_enabled() == FALSE)) {
		restore_interrupts(state);
		return B_OK;
	}

	if ((write_64(0xd0) == B_OK) &&
		((val = read_60()) >= 0) &&
		(write_64(0xd1) == B_OK) &&
		(write_60(val & 0xfd) == B_OK) &&
		(a20_enabled() == FALSE)) {
		restore_interrupts(state);
		return B_OK;
	}

	restore_interrupts(state);

	r.eax = 0x2400;
	int86(0x15, &r, &r);
	if (a20_enabled() == FALSE)
		return B_OK;

	return B_ERROR;
}

status_t get_bpb(uchar drive_letter, uchar bpb[53])
{
	struct regs r;

	r.eax = 0x440d;
	r.ebx = drive_letter - 'A' + 1;
	r.ecx = 0x4860;
	r.edx = 0x1000;
	if ((int86(0x21, &r, &r) & CF) == 0) {
		memcpy(bpb, scratch + 0x1007, 53);
		return 53;
	}

	r.eax = 0x440d;
	r.ebx = drive_letter - 'A' + 1;
	r.ecx = 0x0860;
	r.edx = 0x1000;
	if (int86(0x21, &r, &r) & CF)
		return B_ERROR;
	memcpy(bpb, scratch + 0x1007, 31);
	return 31;
}

status_t get_canonical_name(const char *path, char *canonicalname)
{
	struct regs r;

	strcpy(scratch + 0x1000, path);

	r.eax = 0x6000;
	r.esi = 0x1000;
	r.edi = 0x2000;
	if (int86(0x21, &r, &r) & CF) {
		r.eax = 0x7160;
		r.ecx = 0x0001;
		r.esi = 0x1000;
		r.edi = 0x2000;
		if (int86(0x21, &r, &r) & CF)
			return B_ERROR;
	}

	strcpy(canonicalname, scratch + 0x2000);

	if ((canonicalname[0] == '\\') || (canonicalname[1] != ':'))
		return B_ERROR;

	return B_OK;
}
