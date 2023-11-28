#include <drivers/KernelExport.h>

#include <malloc.h>

#include <boot.h>
#include <bootscreen.h>

#include "bios.h"
#include "bt_misc.h"
#include "cpu_asm.h"
#include "inflate.h"
#include "platform.h"
#include "vesa.h"

#define CHROMAKEY 0xff

long cv_factor = 0x11111111;

static int
keyfilter(int code)
{
	code &= 0xffff;
	switch (code >> 8) {
		case 0x47 : return KEY_HOME;
		case 0x48 : return KEY_UP;
		case 0x49 : return KEY_PGUP;
		case 0x4b : return KEY_LEFT;
		case 0x4d : return KEY_RIGHT;
		case 0x4f : return KEY_END;
		case 0x50 : return KEY_DOWN;
		case 0x51 : return KEY_PGDN;
		case 0x53 : return KEY_DEL;

		case 0x3b : return KEY_F1;
		case 0x42 : return KEY_F8;

		default : return code & 0xff;
	}
}

int platform_key_hit(void)
{
	return keyfilter(bios_key_hit());
}

int platform_get_key(void)
{
	return keyfilter(bios_get_key());
}

int platform_shift_state(void)
{
	return bios_shift_state();
}

void platform_move_cursor(int x, int y)
{
	bios_move_cursor(x, y);
}

// XXX
extern uchar *loadaddr;

#define RESOLUTION	0
#define PALETTE		1
#define ON			2
#define WIPE		3
#define BACKGROUND	4

struct packentry {
    int size, offset;
};

struct packimagehdr {
    int w, h, x, y;
	uchar data[0];
};

struct packresolution {
    int x, y, bits;
};

static struct packentry *pack;

struct regs {
    uint32 eax, ecx, edx, ebx, ebp, esi, edi;
};

extern int int86(uchar intnum, struct regs *in, struct regs *out);

screen curscreen;

static int base_resolution_x, base_resolution_y;
static unsigned char trans_palette[4 * 0x100];

void platform_notify_video_mode(
		int width, int height, int depth, int rowbyte, void *base)
{
	int i, j;
	uint16 c16;
	int palette_entries;
	uchar *logo_palette;
	
	curscreen.width = width;
	curscreen.height = height;
	curscreen.depth = depth;
	curscreen.refresh = 60.f;
	curscreen.rowbyte = rowbyte;
	curscreen.base = base;

	memset(trans_palette, 0, sizeof(trans_palette));

	if (!pack || !pack[PALETTE].offset) {
		palette_entries = 256;
		logo_palette = colormap;
	} else {
		palette_entries = pack[PALETTE].size / 3;
		logo_palette = (uchar *)pack + pack[PALETTE].offset;
	}

	if (curscreen.depth == 8) {
		float dd, d, mind;
		int mini;
		for (i=0;i<palette_entries;i++) {
			mind = 1.0;
			mini = 0;
			for (j=0;j<0x100;j++) {
				dd = (colormap[3*j] - logo_palette[3*i])/255.;
				d = dd * dd;
				dd = (colormap[3*j+1] - logo_palette[3*i+1])/255.;
				d += dd * dd;
				dd = (colormap[3*j+2] - logo_palette[3*i+2])/255.;
				d += dd * dd;
				if (d < mind) {
					mind = d;
					mini = j;
				}
			}
			trans_palette[i] = mini;
		}
	} else if (curscreen.depth == 15) {
		for (i=0;i<palette_entries;i++) {
			c16 =	((logo_palette[3*i] >> 3) << 10) +
					((logo_palette[3*i+1] >> 3) << 5) +
					(logo_palette[3*i+2] >> 3);
			memcpy(trans_palette + 2*i, &c16, 2);
		}
	} else if (curscreen.depth == 16) {
		for (i=0;i<palette_entries;i++) {
			c16 =	((logo_palette[3*i] >> 3) << 11) +
					((logo_palette[3*i+1] >> 2) << 5) +
					(logo_palette[3*i+2] >> 3);
			memcpy(trans_palette + 2*i, &c16, 2);
		}
	} else if (curscreen.depth == 32) {
		for (i=0;i<palette_entries;i++)
			memcpy(trans_palette + 4*i, logo_palette + 3*i, 3);
	}
}

void platform_enter_console_mode(void)
{
	if (curscreen.depth) {
		set_video_mode(3);
		platform_notify_video_mode(80, 25, 0, 80, (void *)0xb8000);
	}
}

void platform_enter_graphics_mode(void)
{
	bool using_vesa = FALSE;

	if (!pack)
		return;

	if (pack[RESOLUTION].offset) {
		struct packresolution *r;
		r = (struct packresolution *)((uchar *)pack + pack[RESOLUTION].offset);
		if (set_vesa_mode(r->x, r->y, r->bits) == B_OK)
			using_vesa = TRUE;
		else
			dprintf("Unable to set video mode %dx%dx%d. Falling back to 640x480x4\n", r->x, r->y, r->bits);
	}

	if (!using_vesa) {
		set_video_mode(0x12);
		platform_notify_video_mode(640, 480, 4, 640 / 8, (void *)0xa0000);
	}
}

static uchar *find_gzip_header(uchar *buffer, int buffersize, const char *name)
{
	int i, len;

	len = strlen(name) + 1;

	for (i=0;i<buffersize-len-10;i++)
		if (	!memcmp(buffer + i, "\x1f\x8b\x08\x08", 4) &&
				!memcmp(buffer + i + 10, name, len))
			return buffer + i;

	return NULL;
}

status_t platform_initialize(void)
{
	uchar *s, *t;
	uint32 size;
	
	pack = NULL;

	s = find_gzip_header(loadaddr + 48 * 1024, 48 * 1024, "images");
	if (s == NULL) {
		dprintf("Could not find image pack\n");
		return ENOENT;
	}
	
	t = malloc(512 * 1024);
	if (!t) {
		dprintf("Out of core\n");
		return ENOMEM;
	}
	
	size = gunzip(s, t);
	if (!size) {
		dprintf("Error expanding image pack\n");
		free(t);
		return B_ERROR;
	}
	
	pack = malloc(size);
	if (!pack) {
		dprintf("Out of core\n");
		free(t);
		return ENOMEM;
	}
	
	memcpy(pack, t, size);

	free(t);

	return B_OK;
}

static void set_palette(unsigned char *palette, float multiplier)
{
	int i;

	if (curscreen.depth != 4) return;

	if (multiplier > 1.) multiplier = 1.;
	write_io_8(0x3c6, 0xff);
	write_io_8(0x3c8, 0);
	for (i=0;i<16;i++) {
		write_io_8(0x3c9, (int)(palette[3*i] * multiplier) >> 2);
		write_io_8(0x3c9, (int)(palette[3*i+1] * multiplier) >> 2);
		write_io_8(0x3c9, (int)(palette[3*i+2] * multiplier) >> 2);
	}
}

static void blit4(const unsigned char *bitmap,
		int x0, int y0, int dx, int dy, int w, int h)
{
	int p, x, y, temp;
	const uchar *ptr;
	uchar *v;

	for (p=0;p<4;p++) {
		/* set write plane */
		write_io_8(0x3c4, 2); write_io_8(0x3c5, 1 << p);
		/* set read plane */
		write_io_8(0x3ce, 4); write_io_8(0x3cf, p);

		for (y=0;y<dy;y++) {
			ptr = bitmap + y * w;
			v = (uchar *)0xa0000 + ((y + y0) * 640 + x0) / 8;
			temp = *v;
			for (x=0;x<dx;x++,ptr++) {
				if (*ptr != CHROMAKEY) {
					if (*ptr & (1 << p))
						temp |= (0x80 >> ((x + x0) & 7));
					else
						temp &= ~(0x80 >> ((x + x0) & 7));
				}
				if (((x + x0) & 7) == 7) {
					*(v++) = temp;
					temp = *v;
				}
			}
			if ((x + x0) & 7)
				*v = temp;
		}
	}
}

static void blit8(const unsigned char *bitmap,
		int x0, int y0, int dx, int dy, int w, int h)
{
	int x, y;
	const uchar *ptr;
	uchar *v;

	for (y=0;y<dy;y++) {
		ptr = bitmap + y * w;
		v = (uchar *)curscreen.base + (y + y0) * curscreen.rowbyte + x0;
		for (x=0;x<dx;x++,ptr++,v++) {
			if (*ptr != CHROMAKEY)
				*v = trans_palette[*ptr];
		}
	}
}

static void blit16(const unsigned char *bitmap,
		int x0, int y0, int dx, int dy, int w, int h)
{
	int x, y;
	const uchar *ptr;
	uint16 *v;

	for (y=0;y<dy;y++) {
		ptr = bitmap + y * w;
		v = (uint16 *)((uchar *)curscreen.base + (y + y0) * curscreen.rowbyte + 2*x0);
		for (x=0;x<dx;x++,ptr++,v++) {
			if (*ptr != CHROMAKEY)
				*v = *(((uint16 *)trans_palette) + *ptr);
		}
	}
}

static void blit32(const unsigned char *bitmap,
		int x0, int y0, int dx, int dy, int w, int h)
{
	int x, y;
	const uchar *ptr;
	uint32 *v;

	for (y=0;y<dy;y++) {
		ptr = bitmap + y * w;
		v = (uint32 *)((uchar *)curscreen.base + (y + y0) * curscreen.rowbyte + 4*x0);
		for (x=0;x<dx;x++,ptr++,v++) {
			if (*ptr != CHROMAKEY)
				*v = *(((uint32 *)trans_palette) + *ptr);
		}
	}
}

static void blit(const unsigned char *bitmap,
		int x0, int y0, int dx, int dy, int w, int h)
{
	if (curscreen.depth == 4)
		blit4(bitmap, x0, y0, dx, dy, w, h);
	else if (curscreen.depth == 8)
		blit8(bitmap, x0, y0, dx, dy, w, h);
	else if ((curscreen.depth == 15) || (curscreen.depth == 16))
		blit16(bitmap, x0, y0, dx, dy, w, h);
	else if (curscreen.depth == 32)
		blit32(bitmap, x0, y0, dx, dy, w, h);
}

/* passes boot icons to kernel */
void platform_pass_boot_icons(void)
{
	struct boot_icons *icons;
	struct packimagehdr *h;

	/* pass boot icons only if they exist and only if the splash screen
	 * has been displayed yet */
	if (!pack || !pack[ON].offset || !base_resolution_x)
		return;

	h = (struct packimagehdr *)((uchar *)pack + pack[ON].offset);
		
	icons = malloc(sizeof(*icons) + h->w * h->h);
	if (!icons) return;

	icons->x = h->x * curscreen.width / base_resolution_x;
	icons->y = h->y * curscreen.height / base_resolution_y;
	icons->w = h->w;
	icons->h = h->h;
	icons->icon_w = h->w / 7;

	/* the kernel uses the palette field to convert the data to a format
	 * appropriate for the screen */
	memcpy(icons->palette, trans_palette, 4 * 0x100);
	memcpy(icons->data, h->data, h->w * h->h);

	add_boot_item(BOOT_ICONS, sizeof(*icons) + h->w * h->h, icons);

	free(icons);
}

void platform_splash_screen(bool check_keys)
{
	int i;
	struct regs r;
	struct packimagehdr *h;
	
	if (!pack || !pack[PALETTE].offset)
		return;

	base_resolution_x = 640;
	base_resolution_y = 480;

	if (pack && pack[RESOLUTION].offset) {
		struct packresolution *r;
		r = (struct packresolution *)((uchar *)pack + pack[RESOLUTION].offset);
		base_resolution_x = r->x;
		base_resolution_y = r->y;
	}

	/* clear the screen */
	if (curscreen.depth == 4) {
		write_io_8(0x3c4, 2); write_io_8(0x3c5, 0xf);
		memset((void *)0xa0000, 0, 640*480/8);
	} else {
		memset(curscreen.base, 0, curscreen.height * curscreen.rowbyte);
	}

	/* set the palette */
	set_palette((uchar *)pack + pack[PALETTE].offset, 0.0);

	for (i=0;i<0x10;i++) {
		r.eax = 0x1000;
		r.ebx = 0x101 * i;
		int86(0x10, &r, &r);
	}

#if 0
	/* set overscan color */
	r.eax = 0x1001;
	r.ebx = 0x0100;
	int86(0x10, &r, &r);
#endif

	/* display beos logo */
	for (i=0;pack[BACKGROUND+i].offset;i++) {
		h = (struct packimagehdr *)((uchar *)pack + pack[BACKGROUND+i].offset);
		blit(h->data,
				h->x * curscreen.width / base_resolution_x,
				h->y * curscreen.height / base_resolution_y,
				h->w, h->h, h->w, h->h);
	}

	/* fade in */
	if (check_keys)
		for (i=0;(i<2000) && !boot_menu_enabled && !fast_boot_enabled;i++) {
			set_palette((uchar *)pack + pack[PALETTE].offset,
					(i / 1000.) * (i / 1000.));
			spin(1000);
			check_boot_keys();
		}

	set_palette((uchar *)pack + pack[PALETTE].offset, 1.0);
	
	/* bring in version number */
	if (pack[WIPE].offset) {
		h = (struct packimagehdr *)((uchar *)pack + pack[WIPE].offset);
		if (check_keys) {
			for (i=0;i<h->w;i++) {
				blit(h->data + i,
						h->x * curscreen.width / base_resolution_x + i,
						h->y * curscreen.height / base_resolution_y,
						1, h->h, h->w, h->h);
				if (!boot_menu_enabled && !fast_boot_enabled) {
					spin(1000000 / h->w);
					check_boot_keys();
				}
			}
		} else {
			blit(h->data,
					h->x * curscreen.width / base_resolution_x,
					h->y * curscreen.height / base_resolution_y,
					h->w, h->h, h->w, h->h);
		}
	} else {
		if (check_keys) {
			for (i=0;i<1000;i++) {
				if (!boot_menu_enabled && !fast_boot_enabled) {
					spin(1000000 / 1000);
					check_boot_keys();
				}
			}
		}
	}
}

void platform_set_disk_state(int state)
{
	static unsigned char colors[] = { 10, 11, 12 };
	static unsigned int c = 0;

	unsigned char data[8*8];

	if (!pack)
		return;

	if (state == 0) {
		memset(data, 0, 8*8);
		c = 0;
	} else {
		int y;
		memset(data, colors[c++ % 3], 8*8);
		memset(data, colors[12], 8);
		for (y=1;y<8;y++)
			data[y*8] = colors[12];
	}

	blit(data, 8, 8, 8, 8, 8, 8);
}

double time_base_to_usec;

#define TIMER_CLKNUM_HZ 1193167

void	calculate_cpu_clock() 
{
	uchar	low, high;
	ulong	expired;
	double	t1, t2;
	double	timer_usecs, time_base_ticks;

	/* program the timer to count down mode */
    write_io_8(0x43, 0x34);              

	write_io_8(0x40, 0xff);		/* low and then high */
	write_io_8(0x40, 0xff);
	t1 = get_time_base();

	execute_n_instructions(16*20000);

	t2 = get_time_base();
	write_io_8(0x43, 0x00); /* latch counter value */
	low = read_io_8(0x40);
	high = read_io_8(0x40);

	expired = (ulong)0xffff - ((((ulong)high) << 8) + low);

	/* set to the default (max) period 54.9 ms */
	write_io_8(0x43, 0x36);              
	write_io_8(0x40, 0);		/* low and then high */
	write_io_8(0x40, 0);

	
	timer_usecs = (expired * 1.0) / (TIMER_CLKNUM_HZ/1000000.0);
	time_base_ticks = t2 -t1;
	time_base_to_usec = timer_usecs /(t2-t1); 
}

void
init_timing (void)
{
	extern void system_time_setup(uint32);
	system_time_setup ((int)(1000000.0/time_base_to_usec+0.5));
}
