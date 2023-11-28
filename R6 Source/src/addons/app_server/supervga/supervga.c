/*----------------------------------------------------------

	Supervga.c, the generic VGA driver with a fake name to
	            confuse Microsoft's spies...
	
	by Pierre Raynaud-Richard.

	Copyright (c) 1998 by Be Incorporated.
	All Rights Reserved.
	
----------------------------------------------------------*/

#ifdef __INTEL__

#include <OS.h>
#include <Debug.h>
#include "supervga.h"
#include <GraphicsCard.h>
#include <fs_attr.h> /* quiet warning */
#include <fs_index.h> /* quiet warning */
#include <dirent.h> /* quiet warning */
#include <priv_syscalls.h>
#include <syscall_gen.h>
#include <boot.h>
#include <fcntl.h>
#include <sys/stat.h>

/*----------------------------------------------------------
 private globals
----------------------------------------------------------*/

#define		COLOR_MODE		0
#define		GRAY_MODE		1
#define		COUNT			8
#define		BAND			20

static	int			buffer_row = 0;
static	int			buffer_width = 0;
static	int			buffer_height = 0;
static	int			buffer_size = 0;
static	int			display_height = 0;
static	int			display_width = 0;
static	int			display_h = 0;
static	int			display_v = 0;
static	int			vga_width;
static	int			vga_height;
static	int			cursor_h = 0;
static	int			cursor_v = 0;
static	int			cursor_hot_h = 0;
static	int			cursor_hot_v = 0;
static	int			cursor_dh = 16;
static	int			cursor_dv = 16;
static	int			buffer_offset_h = 0;
static	int			buffer_offset_v = 0;
static	int			vga_row;
static	int			copy_count = 0;
static	bool		cursor_visible = false;
static	bool		last_on_corner = false;
static	bool		out_corner = false;
static	uchar		vga_mode = 255;
static	uchar		*vga_base;
static	uchar		*buffer_base;
static	area_id		vga_area = (area_id)-1;
static	area_id		buffer_area = (area_id)-1;
static	thread_id	viewer;
static	bigtime_t	last_corner;
static	bigtime_t	total_copy = 0LL;
static	int			notify_bootscript_fd = -1;
#define	NOTIFY_BOOTSCRIPT_FILE_SVGA	"/tmp/stub_vga_driver"
#define	NOTIFY_BOOTSCRIPT_FILE_VESA	"/tmp/stub_vesa_driver"

static	uchar		default_buffer[8];
static	uchar		cursor_shape[16*16];
static	uchar		my_color_map[3*256];
static	uint32		my_grayscale[256];
static	uint32		my_lines[BAND*4*COUNT];

#define inp(a)		read_isa_io (0, (char *)(a), 1)
#define outp(a, b)	write_isa_io (0, (char *)(a), 1, (b))

/*----------------------------------------------------------
 global tables (for all depth)
----------------------------------------------------------*/

static ushort	vga_table[] =
{
	0x3c2,	0x63,
	0x3da, 	0x00,
//	0x3d8,	0x0a,
//	0x3bf,	0x01,
	0x102,	0x01,
	
	0x3c4,	0x00,	0x3c5,	0x03,
	0x3c4,	0x02,	0x3c5,	0x0f,
	0x3c4,	0x03,	0x3c5,	0x00,
	0x3c4,	0x04,	0x3c5,	0x0e,

	0x3d4,	0x11,	0x3d5,	0x0e,
	
	0x3d4,	0x00,	0x3d5,	0x5f,
	0x3d4,	0x01,	0x3d5,	0x4f,
	0x3d4,	0x02,	0x3d5,	0x50,
	0x3d4,	0x03,	0x3d5,	0x82,
	0x3d4,	0x04,	0x3d5,	0x54,
	0x3d4,	0x05,	0x3d5,	0x80,
	0x3d4,	0x06,	0x3d5,	0xbf,
	0x3d4,	0x07,	0x3d5,	0x1f,
	0x3d4,	0x08,	0x3d5,	0x00,
	0x3d4,	0x09,	0x3d5,	0x41,
	0x3d4,	0x0a,	0x3d5,	0x00,
	0x3d4,	0x0b,	0x3d5,	0x00,
	0x3d4,	0x0c,	0x3d5,	0x00,
	0x3d4,	0x0d,	0x3d5,	0x00,
	0x3d4,	0x0e,	0x3d5,	0x00,
	0x3d4,	0x0f,	0x3d5,	0x00,
	0x3d4,	0x10,	0x3d5,	0x9c,
	0x3d4,	0x12,	0x3d5,	0x8f,
	0x3d4,	0x13,	0x3d5,	0x28,
	0x3d4,	0x14,	0x3d5,	0x40,
	0x3d4,	0x15,	0x3d5,	0x96,
	0x3d4,	0x16,	0x3d5,	0xb9,
	0x3d4,	0x17,	0x3d5,	0xa3,
	0x3d4,	0x18,	0x3d5,	0xff,
	
	0x3d4,	0x11,	0x3d5,	0x8e,

	0x3ce,	0x00,	0x3cf,	0x00,
	0x3ce,	0x01,	0x3cf,	0x00,
	0x3ce,	0x02,	0x3cf,	0x00,
	0x3ce,	0x03,	0x3cf,	0x00,
	0x3ce,	0x04,	0x3cf,	0x00,
	0x3ce,	0x05,	0x3cf,	0x40,
	0x3ce,	0x06,	0x3cf,	0x05,
	0x3ce,	0x07,	0x3cf,	0x0f,
	0x3ce,	0x08,	0x3cf,	0xff,

	0x00,	0x00			/* End-of-table */
};

static uchar attr_table[21] = {
	0x00,	0x01,	0x02,	0x03,	0x04,	0x05,	0x06,	0x07,
	0x08,	0x09,	0x0a,	0x0b,	0x0c,	0x0d,	0x0e,	0x3f,
	0x41,	0x00,	0x0f,	0x00,	0x00
};

static ushort	vga_table2[] =
{
	0x3c2,	0xe3,
	0x3da, 	0x00,
//	0x3d8,	0x0a,
//	0x3bf,	0x01,
	0x102,	0x01,
	
	0x3c4,	0x00,	0x3c5,	0x03,
	0x3c4,	0x02,	0x3c5,	0x0f,
	0x3c4,	0x03,	0x3c5,	0x00,
	0x3c4,	0x04,	0x3c5,	0x06,

	0x3d4,	0x11,	0x3d5,	0x0c,
	
	0x3d4,	0x00,	0x3d5,	0x5f,
	0x3d4,	0x01,	0x3d5,	0x4f,
	0x3d4,	0x02,	0x3d5,	0x50,
	0x3d4,	0x03,	0x3d5,	0x82,
	0x3d4,	0x04,	0x3d5,	0x54, // 24
	0x3d4,	0x05,	0x3d5,	0x80,
	0x3d4,	0x06,	0x3d5,	0x0b,
	0x3d4,	0x07,	0x3d5,	0x3e,
	0x3d4,	0x08,	0x3d5,	0x00,
	0x3d4,	0x09,	0x3d5,	0x40,
	0x3d4,	0x0a,	0x3d5,	0x00,
	0x3d4,	0x0b,	0x3d5,	0x00,
	0x3d4,	0x0c,	0x3d5,	0x00,
	0x3d4,	0x0d,	0x3d5,	0x00,
	0x3d4,	0x0e,	0x3d5,	0x00,
	0x3d4,	0x0f,	0x3d5,	0x00, // 31
	0x3d4,	0x10,	0x3d5,	0xea,
	0x3d4,	0x12,	0x3d5,	0xdf,
	0x3d4,	0x13,	0x3d5,	0x28,
	0x3d4,	0x14,	0x3d5,	0x00,
	0x3d4,	0x15,	0x3d5,	0xe7,
	0x3d4,	0x16,	0x3d5,	0x04,
	0x3d4,	0x17,	0x3d5,	0xe3,
	0x3d4,	0x18,	0x3d5,	0xff,
	
	0x3d4,	0x11,	0x3d5,	0x8c,

	0x3ce,	0x00,	0x3cf,	0x00,
	0x3ce,	0x01,	0x3cf,	0x00,
	0x3ce,	0x02,	0x3cf,	0x00,
	0x3ce,	0x03,	0x3cf,	0x00,
	0x3ce,	0x04,	0x3cf,	0x00,
	0x3ce,	0x05,	0x3cf,	0x00,
	0x3ce,	0x06,	0x3cf,	0x05,
	0x3ce,	0x07,	0x3cf,	0x0f,
	0x3ce,	0x08,	0x3cf,	0xff,

	0x00,	0x00			/* End-of-table */
};

static uchar attr_table2[21] = {
	0x00,	0x01,	0x02,	0x03,	0x04,	0x05,	0x06,	0x07,
	0x38,	0x39,	0x3a,	0x3b,	0x3c,	0x3d,	0x3e,	0x3f,
	0x01,	0x00,	0x0f,	0x00,	0x00
};

/*----------------------------------------------------------
 card io access protection
----------------------------------------------------------*/

static sem_id	io_sem = -1;
static long		io_lock;

void init_locks()
{
	io_lock = 0L;
	io_sem = create_sem(0,"vga io sem");
}

void dispose_locks()
{
	if (io_sem >= 0)
		delete_sem(io_sem);
}

void lock_io()
{
	int	old;

	old = atomic_add (&io_lock, 1);
	if (old >= 1) {
		acquire_sem(io_sem);	
	}	
}

void unlock_io()
{
	int	old;

	old = atomic_add (&io_lock, -1);
	if (old > 1) {
		release_sem(io_sem);
	}
}	

/*----------------------------------------------------------
 Global In/Out routines
----------------------------------------------------------*/

static void	set_attr_table(uchar *attr_val)
{
	uchar	p1,v;
	
	inp(0x3da);
	v = inp(0x3c0);
	
	for (p1=0; p1<0x15; p1++) {
		inp(0x3da);
		outp(0x3c0, p1);
		outp(0x3c0, attr_val[p1]);
	}

	inp(0x3da);
	outp(0x3c0, v | 0x20);
}

static void	set_table(ushort *ptr)
{
	ushort	p1;
	ushort	p2;

	while(1) {
		p1 = *ptr++;
		p2 = *ptr++;
		if (p1 == 0 && p2 == 0) {
			return;
		}
		outp(p1, p2);
	}
}

static void	setup_dac()
{
/*	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	inp(0x3c6);
	outp(0x3c6, 0x00);
	inp(0x3c8); */
	outp(0x3c6, 0xff);
}

/*----------------------------------------------------------
 vid_selectmode : set current depth and resolution
----------------------------------------------------------*/

static void set_color_entry(int index, int red, int green, int blue) {
	outp(0x3c6, 0xff);
	inp(0x3c7);
	outp(0x3c8, index);
	inp(0x3c7);
	outp(0x3c9, red >> 2);
	inp(0x3c7);
	outp(0x3c9, green >> 2);
	inp(0x3c7);
	outp(0x3c9, blue >> 2);
	inp(0x3c7);
}

static void set_vga_mode(int mode)
{
	int      i, j;

	if (mode == vga_mode)
		return;

	outp(0x3c4, 0x01);
	outp(0x3c5, 0x21);		/* blank the screen */

	switch (mode) {
	case COLOR_MODE :
		set_attr_table(attr_table);
		set_table(vga_table);
		setup_dac();
		vga_height = 200;
		vga_width = 320;
		vga_row = 320;
		vga_mode = COLOR_MODE;
		
		for (i=0; i<256; i++)
			set_color_entry(i, my_color_map[i*3], my_color_map[i*3+1], my_color_map[i*3+2]);

		for (i = 0; i < vga_height*vga_width; i++)
			vga_base[i] = 0x00;
		break;
	case GRAY_MODE :
		set_attr_table(attr_table2);
		set_table(vga_table2);
		setup_dac();
		vga_height = 480;
		vga_width = 640;
		vga_row = 80;
		vga_mode = GRAY_MODE;
		
		for (i=0; i<256; i++) {
			j = (i&15)*17;
			set_color_entry(i, j, j, j);
		}
		break;
	}
	
	outp(0x3c4, 0x01);
	outp(0x3c5, 0x01);		/* unblank the screen */
}

static void set_buffer_size(int size) {
	int		alloc;

	if (size > buffer_size) {
		if (buffer_area >= 0)
			delete_area(buffer_area);
		buffer_size = size;
		alloc = (buffer_size+B_PAGE_SIZE-1)&~(B_PAGE_SIZE-1);
		buffer_area = create_area(	"fake buffer",
									(void**)&buffer_base,
									B_ANY_KERNEL_ADDRESS,
									alloc,
									0,
									B_READ_AREA | B_WRITE_AREA);
		if (buffer_area < 0) {
			buffer_base = (uchar*)default_buffer;
			buffer_width = 1;
			buffer_height = 1;
			buffer_row = 1;
			return;
		}
	}
}

static void set_buffer_mode(int width, int height) {
	int		i, size, alloc;

	if ((width == buffer_width) &&
		(height == buffer_height))
		return;
	
	size = width*height;
	set_buffer_size(size);
	
	buffer_width = width;
	buffer_height = height;
	buffer_row = width;
	
	display_h = 0;
	display_v = 0;
	display_width = width;
	display_height = height;
	
	for (i=0; i<size; i++)
		buffer_base[i] = 0x00;
}

int32 set_cursor_shape(uchar *data, /* XorMask */
					   uchar *mask, /* AndMask */
					   int32 dim_h, /* Dimensions of the cursor (in pixel) */
					   int32 dim_v,
					   int32 hot_h, /* Position of the hot_spot in the cursor */
					   int32 hot_v)
{
	int		i, b_xor, b_and;

	lock_io();
	
	cursor_dh = dim_h;
	cursor_dv = dim_v;
	cursor_hot_h = hot_h;
	cursor_hot_v = hot_v;

	for (i=0; i<256; i++) {
		b_xor = (data[i>>3]>>(7 - (i&7)))&1;		
		b_and = (mask[i>>3]>>(7 - (i&7)))&1;
		cursor_shape[i] = b_xor*2 + b_and;
	}
	
	unlock_io();
	return B_NO_ERROR;
}
	

int32 move_cursor(int32 new_h, /* New hot_spot coordinates */
				  int32 new_v)
{
	bigtime_t		time;

	lock_io();
/*	
	if ((new_h == 0) && (new_v == 0)) {
		if (!last_on_corner) {
			time = system_time();
			if ((time - last_corner < 1500000) && out_corner) {
				if (vga_mode == COLOR_MODE)
					set_vga_mode(GRAY_MODE);
				else
					set_vga_mode(COLOR_MODE);
				last_corner = 0LL;
			}
			else
				last_corner = time;
		}
		out_corner = false;
		last_on_corner = true;
	}
	else
		last_on_corner = false;

	if ((new_h > 16) || (new_v > 16))
		out_corner = true;
*/	
	cursor_h = new_h;
	cursor_v = new_v;

	unlock_io();
	return B_NO_ERROR;
}


int32 show_cursor(bool state)
{
	lock_io();
	
	cursor_visible = state;

	unlock_io();
	return B_NO_ERROR;
}

/*
void fmemcpy(void* dest, void* src, uint dw_count)
{
	asm
	{
		mov ecx, dw_count
		mov esi, src
		mov edi, dest
		rep movsd  
	}
} 
*/

int32 do_viewer(void *date) {
	int				i, j, k, l, hmin, hmax, vmin, vmax;
	uint8			*lines8;
	uint8			*from8;
	uint32			gray, banks;
	uint32			*from, *to, *to2;
	uint32			*lines;
	bigtime_t		start, time, delay, end, time2;
	
	while (true) {
		start = system_time();
		
		lock_io();		
		
		/* buffer scrolling */
		hmin = cursor_h - cursor_hot_h + display_h;
		vmin = cursor_v - cursor_hot_v + display_v;
		hmax = hmin + cursor_dh;
		vmax = vmin + cursor_dv;
				
		if (hmin < buffer_offset_h)
			buffer_offset_h = hmin;
		if (buffer_offset_h < display_h)
			buffer_offset_h = display_h;
		
		if (vmin < buffer_offset_v)
			buffer_offset_v = vmin;
		if (buffer_offset_v < display_v)
			buffer_offset_v = display_v;
		
		if (hmax > buffer_offset_h+vga_width)
			buffer_offset_h = hmax-vga_width;
		if (buffer_offset_h > display_h+display_width-vga_width)
			buffer_offset_h = display_h+display_width-vga_width;
		
		if (vmax > buffer_offset_v+vga_height)
			buffer_offset_v = vmax-vga_height;
		if (buffer_offset_v > display_v+display_height-vga_height)
			buffer_offset_v = display_v+display_height-vga_height;
		
		hmin -= buffer_offset_h;
		vmin -= buffer_offset_v;
		hmax -= buffer_offset_h;
		vmax -= buffer_offset_v;
		
		if (!cursor_visible)
			vmin = vmax = -10;
			
		if (hmax > vga_width)
			hmax = vga_width;

		/* buffer copy */
		if (buffer_width >= 640) {
			if (vga_mode == COLOR_MODE) {
				from = (uint32*)(buffer_base + buffer_row*buffer_offset_v + buffer_offset_h);
				to = (uint32*)vga_base;
				for (i=0; i<vga_height; i++) {
					if ((i < vmin) || (i >= vmax))
						for (j=0; j<vga_width>>2; j++)
							to[j] = from[j];
					else {
						for (j=0; j<hmin-3; j+=4)
							to[j>>2] = from[j>>2];
						for (; j<hmax; j+=4)
							for (k=j; k<j+4; k++) {
								if ((k<hmin) || (k>=hmax))
									((uint8*)to)[k] = ((uint8*)from)[k];
								else {
									switch(cursor_shape[((i-vmin)<<4) + (k-hmin)]) {
									case 0 :
										((uint8*)to)[k] = 0x3f;
										break;
									case 1 :
										((uint8*)to)[k] = ((uint8*)from)[k];
										break;
									case 2 :
										((uint8*)to)[k] = 0x00;
										break;
									}
								}
							}
						for (; j<vga_width; j+=4)
							to[j>>2] = from[j>>2];
					}
					from += buffer_row>>2;
					to += vga_row>>2;
				}
				delay = 30000;
			}
			else if (vga_mode == GRAY_MODE)	{
				from8 = (uint8*)(buffer_base + buffer_row*buffer_offset_v + buffer_offset_h);
				to = (uint32*)vga_base;
				
				outp(0x3c4, 0x02);
				for (i=0; i<vga_height; i+=COUNT) {
					for (j=i; j<i+COUNT; j++) {
						lines8 = ((uint8*)my_lines)+(BAND*16)*(j-i);
						if ((j < vmin) || (j >= vmax))
							convert_line(vga_width>>3, my_grayscale, from8, lines8);
						else {
							if (hmin >= 8) {
								convert_line(hmin>>3, my_grayscale, from8, lines8);
								lines8 += (hmin>>3);
							}
							
							if (hmin < 0)
								k = 0;
							else
								k = (hmin>>3)<<3;
							for (; k<hmax; k+=8) {
								banks = 0;
								for (l=k; l<k+8; l++) {
									if ((l<hmin) || (l>=hmax))
										gray = my_grayscale[from8[l]];
									else {
										switch(cursor_shape[((j-vmin)<<4) + (l-hmin)]) {
										case 0 :
											gray = 0x01010101;
											break;
										case 1 :
											gray = my_grayscale[from8[l]];
											break;
										case 2 :
											gray = 0x00000000;
											break;
										}
									}
									banks = (banks<<1) | gray;
								}
								lines8[BAND*0] = banks;
								lines8[BAND*4] = banks>>8;
								lines8[BAND*8] = banks>>16;
								lines8[BAND*12] = banks>>24;
								lines8++;
							}	
							
							if (vga_width > k)
								convert_line((vga_width-k)>>3, my_grayscale, from8 + k, lines8);
						}
						from8 += buffer_row;
					}
					if (vga_width != 640) {
						for (l=0; l<4; l++) {
							outp(0x3c5, 1<<l);
							lines = my_lines+l*BAND;
							to2 = to;
							for (j=i; j<i+COUNT; j++) {
								for (k=0; k<(vga_width>>5); k++)
									to2[k] = lines[k];
								lines += 4*BAND;
								to2 += vga_row>>2;
							}
						}
					}
					else {
						for (l=0; l<4; l++) {
							outp(0x3c5, 1<<l);
							lines = my_lines+l*BAND;
							to2 = to;
							for (j=i; j<i+COUNT; j++) {
								to2[0] = lines[0];
								to2[1] = lines[1];
								to2[2] = lines[2];
								to2[3] = lines[3];
								to2[4] = lines[4];
								to2[5] = lines[5];
								to2[6] = lines[6];
								to2[7] = lines[7];
								to2[8] = lines[8];
								to2[9] = lines[9];
								to2[10] = lines[10];
								to2[11] = lines[11];
								to2[12] = lines[12];
								to2[13] = lines[13];
								to2[14] = lines[14];
								to2[15] = lines[15];
								to2[16] = lines[16];
								to2[17] = lines[17];
								to2[18] = lines[18];
								to2[19] = lines[19];
//								fmemcpy(to2, lines, 20);
								lines += 4*BAND;
								to2 += vga_row>>2;
							}
						}
					}
					to = to2;
				}

				delay = 50000;
			}
			else delay = 100000;
		}
		else delay = 100000;	
		unlock_io();

		end = system_time();
		time2 = end-start;
/*	
		if (delay < 100000) {
			total_copy += time2;
			copy_count += 1;
			if ((copy_count & 63) == 0) {
				_sPrintf("Average copy time : %dus\n", total_copy/copy_count);
				copy_count = 0;
				total_copy = 0LL;
			}
		}
*/
		time = delay - time2;
		if (time < time2)
			time = time2;
		snooze(time);
	}
}

/*----------------------------------------------------------
 Graphic add-on main entry point
----------------------------------------------------------*/

static long svga_control_graphics_card(ulong message,void *buf)
{
	int			i, col, err, index, index2, fw, fh, row, x, y, dw, dh;
	
	err = B_NO_ERROR;

	switch (message) {
	case B_GET_GRAPHICS_CARD_INFO :
		lock_io();
		((graphics_card_info*)buf)->height = buffer_height;
		((graphics_card_info*)buf)->width = buffer_width;
		((graphics_card_info*)buf)->bits_per_pixel = 8;
		((graphics_card_info*)buf)->frame_buffer = (void*)buffer_base;
		((graphics_card_info*)buf)->bytes_per_row = buffer_row;
		((graphics_card_info*)buf)->flags = B_FRAME_BUFFER_CONTROL; /* |B_PARALLEL_BUFFER_ACCESS; -- No BDirectWindow (DMA issues) */
		*((long*)&((graphics_card_info*)buf)->rgba_order) = (long)('rgba');
		((graphics_card_info*)buf)->version = 2;
		((graphics_card_info*)buf)->id = 0;
		unlock_io();
		break;
		
	case B_GET_GRAPHICS_CARD_HOOKS :
		((graphics_card_hook*)buf)[0] = (graphics_card_hook)set_cursor_shape;
		((graphics_card_hook*)buf)[1] = (graphics_card_hook)move_cursor;
		((graphics_card_hook*)buf)[2] = (graphics_card_hook)show_cursor;
		for (i=3;i<B_HOOK_COUNT;i++)
			((graphics_card_hook*)buf)[i] = 0L;
		break;
		
	case B_OPEN_GRAPHICS_CARD :
		vga_area = _kmap_physical_memory_(	"A0000 VGA bank",
											(void*)0xa0000,
											0x10000,
											B_ANY_KERNEL_ADDRESS,
											B_READ_AREA | B_WRITE_AREA,
											(void**)&vga_base);
		if (vga_area < 0) {
			err = B_ERROR;
			break;
		}
		set_vga_mode(GRAY_MODE);
		viewer = spawn_thread(do_viewer, "Viewer deamon", B_URGENT_DISPLAY_PRIORITY, NULL);
		if (viewer < 0) {
			delete_area(vga_area);
			err = B_ERROR;
			break;
		}
		init_locks();
		notify_bootscript_fd = creat (NOTIFY_BOOTSCRIPT_FILE_SVGA, 
			S_IRUSR + S_IWUSR + S_IRGRP + S_IWGRP + S_IROTH + S_IWOTH);
		resume_thread(viewer);
		break;
		
	case B_CLOSE_GRAPHICS_CARD :
		lock_io();
		outp(0x3c4, 0x01);
		outp(0x3c5, 0x21);
		dispose_locks();
		if (viewer >= 0)
			kill_thread(viewer);
		if (vga_area >= 0)
			delete_area(vga_area);
		if (buffer_area >= 0)
			delete_area(buffer_area);
		if (notify_bootscript_fd >= 0) {
			close (notify_bootscript_fd);
			unlink (NOTIFY_BOOTSCRIPT_FILE_SVGA);
		}
		break;
		
	case B_SET_INDEXED_COLOR:
		lock_io();
		if (vga_mode == COLOR_MODE)
			set_color_entry(((indexed_color*)buf)->index,
							((indexed_color*)buf)->color.red,
							((indexed_color*)buf)->color.green,
							((indexed_color*)buf)->color.blue);

		i = ((indexed_color*)buf)->index*3;
		my_color_map[i+0] = ((indexed_color*)buf)->color.red;
		my_color_map[i+1] = ((indexed_color*)buf)->color.green;
		my_color_map[i+2] = ((indexed_color*)buf)->color.blue;
		index = my_color_map[i+0] + my_color_map[i+1] + my_color_map[i+2];
		index2 = my_color_map[i+0];
		if (index2 < my_color_map[i+1])
			index2 = my_color_map[i+1];
		if (index2 < my_color_map[i+2])
			index2 = my_color_map[i+2];
		index = (index+index2)>>6;
		my_grayscale[i/3] = ((index&8)<<21)|((index&4)<<14)|((index&2)<<7)|(index&1);
		unlock_io();
		break;
		
	case B_GET_SCREEN_SPACES:
		((graphics_card_config*)buf)->space =
			B_8_BIT_640x480 | B_8_BIT_800x600 | B_8_BIT_1024x768 |
			B_8_BIT_1152x900 | B_8_BIT_1280x1024 | B_8_BIT_1600x1200;
		break;
		
	case B_CONFIG_GRAPHICS_CARD:
		lock_io();
		switch (((graphics_card_config*)buf)->space) {
		case B_8_BIT_640x480 :
			set_buffer_mode(640, 480);
			break;
		case B_8_BIT_800x600 :
			set_buffer_mode(800, 600);
			break;
		case B_8_BIT_1024x768 :
			set_buffer_mode(1024, 768);
			break;
		case B_8_BIT_1152x900 :
			set_buffer_mode(1152, 900);
			break;
		case B_8_BIT_1280x1024 :
			set_buffer_mode(1280, 1024);
			break;
		case B_8_BIT_1600x1200 :
			set_buffer_mode(1600, 1200);
			break;
		default:
			err = B_ERROR;
			break;
		}
		unlock_io();
		break;

	case B_GET_REFRESH_RATES :
		((refresh_rate_info*)buf)->min = 60.0;
		((refresh_rate_info*)buf)->max = 60.0;
		((refresh_rate_info*)buf)->current = 60.0;
		break;
		
	case B_GET_INFO_FOR_CLONE_SIZE :
		*((long*)buf) = 4;
		break;

	case B_GET_INFO_FOR_CLONE :
	case B_SET_CLONED_GRAPHICS_CARD :
	case B_CLOSE_CLONED_GRAPHICS_CARD :
		break;

	case B_PROPOSE_FRAME_BUFFER :
		lock_io();
		((frame_buffer_info*)buf)->bytes_per_row = -1;
		((frame_buffer_info*)buf)->height = -1;
		if (((frame_buffer_info*)buf)->bits_per_pixel != 8) {
			err = B_ERROR;
			unlock_io();
			break;
		}
		row = (((frame_buffer_info*)buf)->width+3)&(~3);
		((frame_buffer_info*)buf)->bytes_per_row = row;
		((frame_buffer_info*)buf)->height = (1<<22)/row;
		unlock_io();
		break;

	case B_SET_FRAME_BUFFER :
		lock_io();
		fw = ((frame_buffer_info*)buf)->width;
		fh = ((frame_buffer_info*)buf)->height;
		dw = ((frame_buffer_info*)buf)->display_width;
		dh = ((frame_buffer_info*)buf)->display_height;
		row = ((frame_buffer_info*)buf)->bytes_per_row;
		x = ((frame_buffer_info*)buf)->display_x;
		y = ((frame_buffer_info*)buf)->display_y;

		x &= ~3;

		if ((((frame_buffer_info*)buf)->bits_per_pixel != 8) ||
			(fw > row) || (dw > fw) || (dh > fh) ||
			((row & 3) != 0) || (row < 320) || (row > 4096) ||
			(row*fh) > (1<<22) ||
			(x < 0) || (y < 0) || (y+dh > fh) || (x+dw > fw)) {
			err = B_ERROR;
			unlock_io();
			break;
		}	
		
		set_buffer_mode(fw, fh);
		display_width = dw;
		display_height = dh;
		buffer_offset_h += x-display_h;
		buffer_offset_v += y-display_v;
		display_h = x;
		display_v = y;
		unlock_io();
		break;

	case B_SET_DISPLAY_AREA :
		lock_io();
		dw = ((frame_buffer_info*)buf)->display_width;
		dh = ((frame_buffer_info*)buf)->display_height;
		x = ((frame_buffer_info*)buf)->display_x;
		y = ((frame_buffer_info*)buf)->display_y;
		
		x &= ~3;

		if ((dw > buffer_width) || (dh > buffer_height) ||
			(x < 0) || (y < 0) ||
			(y+dh > buffer_height) || (x+dw > buffer_width)) {
			err = B_ERROR;
			unlock_io();
			break;
		}
		
		display_width = dw;
		display_height = dh;
		buffer_offset_h += x-display_h;
		buffer_offset_v += y-display_v;
		display_h = x;
		display_v = y;
		unlock_io();
		break;

// Set the origin of the display rectangle in the frame rectangle
	case B_MOVE_DISPLAY_AREA :
		lock_io();
		x = ((frame_buffer_info*)buf)->display_x;
		y = ((frame_buffer_info*)buf)->display_y;
			
		x &= ~3;

		if ((x < 0) || (x+display_width > buffer_width) ||
			(y < 0) || (y+display_height > buffer_height)) {
			err = B_ERROR;
			unlock_io();
			break;
		}
		
		buffer_offset_h += x-display_h;
		buffer_offset_v += y-display_v;
		display_h = x;
		display_v = y;
		unlock_io();
		break;

	default:
		err = B_ERROR;
	}
	
	return err;
}

static uint32 matchmode(uint32 w, uint32 h, uint32 bits)
{
	static struct mode {
		uint32 w, h, bits, flag;
	} modes[] = {
		{ 640, 480, 8, B_8_BIT_640x480 },
		{ 800, 600, 8, B_8_BIT_800x600 },
		{ 1024, 768, 8, B_8_BIT_1024x768 },
		{ 1152, 900, 8, B_8_BIT_1152x900 },
		{ 1280, 1024, 8, B_8_BIT_1280x1024 },
		{ 1600, 1200, 8, B_8_BIT_1600x1200 },
		{ 640, 480, 15, B_15_BIT_640x480 },
		{ 800, 600, 15, B_15_BIT_800x600 },
		{ 1024, 768, 15, B_15_BIT_1024x768 },
		{ 1152, 900, 15, B_15_BIT_1152x900 },
		{ 1280, 1024, 15, B_15_BIT_1280x1024 },
		{ 1600, 1200, 15, B_15_BIT_1600x1200 },
		{ 640, 480, 16, B_16_BIT_640x480 },
		{ 800, 600, 16, B_16_BIT_800x600 },
		{ 1024, 768, 16, B_16_BIT_1024x768 },
		{ 1152, 900, 16, B_16_BIT_1152x900 },
		{ 1280, 1024, 16, B_16_BIT_1280x1024 },
		{ 1600, 1200, 16, B_16_BIT_1600x1200 },
		{ 640, 480, 32, B_32_BIT_640x480 },
		{ 800, 600, 32, B_32_BIT_800x600 },
		{ 1024, 768, 32, B_32_BIT_1024x768 },
		{ 1152, 900, 32, B_32_BIT_1152x900 },
		{ 1280, 1024, 32, B_32_BIT_1280x1024 },
		{ 1600, 1200, 32, B_32_BIT_1600x1200 },
		{ 0, 0, 0, 0 }
	};
	int i;

	for (i=0;modes[i].w;i++)
		if ((w == modes[i].w) && (h == modes[i].h) && 
				(bits == modes[i].bits))
			return modes[i].flag;

	if (bits == 8) return B_8_BIT_640x480;
	if (bits == 15) return B_15_BIT_640x480;
	if (bits == 16) return B_16_BIT_640x480;
	if (bits == 32) return B_32_BIT_640x480;

	return 0;
}

static long vesa_control_graphics_card(ulong message,void *buf)
{
	static screen s;
	static void *framebuffer;
	static area_id aid;

	status_t err = B_NO_ERROR;

	switch (message) {
	case B_GET_GRAPHICS_CARD_INFO :
		((graphics_card_info*)buf)->height = s.height;
		((graphics_card_info*)buf)->width = s.width;
		((graphics_card_info*)buf)->bits_per_pixel = s.depth;
		((graphics_card_info*)buf)->frame_buffer = framebuffer;
		((graphics_card_info*)buf)->bytes_per_row = s.rowbyte;
		((graphics_card_info*)buf)->flags = B_FRAME_BUFFER_CONTROL;
		*((long*)&((graphics_card_info*)buf)->rgba_order) = (long)('bgra');
		((graphics_card_info*)buf)->version = 2;
		((graphics_card_info*)buf)->id = 0;
		break;
		
	case B_GET_GRAPHICS_CARD_HOOKS :
		memset(buf, 0, B_HOOK_COUNT * sizeof(graphics_card_hook));
		break;
		
	case B_OPEN_GRAPHICS_CARD : {
		uint32 base;
		if (_kget_default_screen_info_(&s) != B_OK) return B_ERROR;
		aid = _kmap_physical_memory_(
				"VESA Linear",
				(void *)s.base,
				((s.height * s.rowbyte)+0xfff)&~0xfff,
				B_ANY_KERNEL_ADDRESS,
				B_READ_AREA | B_WRITE_AREA,
				(void**)&framebuffer);
		if (aid < 0) {
			err = B_ERROR;
			break;
		}
		notify_bootscript_fd = creat (NOTIFY_BOOTSCRIPT_FILE_VESA,
			S_IRUSR + S_IWUSR + S_IRGRP + S_IWGRP + S_IROTH + S_IWOTH);

		/* try to recover unused VESA frame buffer memory */
		_kgeneric_syscall_(GSC_STEAL_VESA_MEMORY, NULL, 0);	
		break;
	}
		
	case B_CLOSE_GRAPHICS_CARD :
		if (aid >= 0) delete_area(aid);
		if (notify_bootscript_fd >= 0) {
			close (notify_bootscript_fd);
			unlink (NOTIFY_BOOTSCRIPT_FILE_VESA);
		}
		break;
		
	case B_GET_SCREEN_SPACES: {
		((graphics_card_config*)buf)->space =
				matchmode(s.width, s.height, s.depth);
		break;
	}

	case B_CONFIG_GRAPHICS_CARD:
		break;

	case B_GET_REFRESH_RATES :
		((refresh_rate_info*)buf)->min = ((refresh_rate_info*)buf)->max = 
				((refresh_rate_info*)buf)->current = s.refresh;
		break;
		
	case B_GET_INFO_FOR_CLONE_SIZE :
		*((long*)buf) = 4;
		break;

	case B_GET_INFO_FOR_CLONE :
	case B_SET_CLONED_GRAPHICS_CARD :
	case B_CLOSE_CLONED_GRAPHICS_CARD :
		break;

	default:
		err = B_ERROR;
	}
	
	return err;
}

long control_graphics_card(ulong message,void *buf)
{
	static int32 which = 0;

	if (!which) {
		screen s;
		uint32 len = sizeof(s);
		which = 'svga';
		if ((_kget_default_screen_info_(&s) == B_OK) &&
			(s.depth > 4))
			which = 'vesa';
	}

	if (which == 'svga')
		return svga_control_graphics_card(message, buf);

	return vesa_control_graphics_card(message, buf);
}

#endif
