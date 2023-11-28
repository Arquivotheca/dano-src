#include <SupportDefs.h>
#include <ByteOrder.h>
#include <string.h>
#include "convert.h"

#if 0
#include <stdio.h>
#define TRACE(x) printf x
#else
#define TRACE(x)
#endif

static void raw_copy(uint32 width, uint32 height,
                     const void *src, uint32 src_bytes_per_row,
                     void *dest, uint32 dest_bytes_per_row);
static void raw_line_copy(uint32 width, uint32 height,
                          const void *src, uint32 src_bytes_per_row,
                          void *dest, uint32 dest_bytes_per_row);
static void raw_line_flip(uint32 width, uint32 height,
                          const void *src, uint32 src_bytes_per_row,
                          void *dest, uint32 dest_bytes_per_row);

static void yv12_to_rgb32(uint32 width, uint32 height,
                          const void *src, uint32 src_bytes_per_row,
                          void *dest, uint32 dest_bytes_per_row);

static void yuy2_to_rgb32(uint32 width, uint32 height,
                          const void *src, uint32 src_bytes_per_row,
                          void *dest, uint32 dest_bytes_per_row);

static uint32
get_src_bytes_per_row(int width, int bytes_per_pixel)
{
	return (width + (width&1)) * bytes_per_pixel;
}

static uint32
default_bytes_per_row(int width, int bytes_per_pixel)
{
	int bytes_per_row = width * bytes_per_pixel;
	bytes_per_row += -bytes_per_row & 3;
	return bytes_per_row;
}

convertf *
get_converter(uint32 fourcc, int *src_bytes_per_row,
              media_video_display_info *display)
{
	switch(fourcc) {
		case 'YV12':
			*src_bytes_per_row = display->line_width + display->line_width / 2;
			if(display->format != B_RGB32) {
				display->format = B_RGB32;
				display->bytes_per_row = default_bytes_per_row(display->line_width, 4);
			}
			return yv12_to_rgb32;

		case 'YUY2':
		case 'YUV2':
			*src_bytes_per_row = get_src_bytes_per_row(display->line_width, 2);
			if(display->format == B_YCbCr422) {
				if (!display->bytes_per_row)
					display->bytes_per_row = *src_bytes_per_row;
				if(*src_bytes_per_row == (int)display->bytes_per_row)
					return raw_copy;
				else
					return raw_line_copy;
			}
			if(display->format != B_RGB32) {
				display->format = B_RGB32;
				display->bytes_per_row = default_bytes_per_row(display->line_width, 4);
			}
			return yuy2_to_rgb32;

		default:
			return NULL;
	}
}


static void
raw_copy(uint32 width, uint32 height,
         const void *src, uint32 src_bytes_per_row,
         void *dest, uint32 dest_bytes_per_row)
{
	TRACE(("RGB: raw_copy(%ld, %ld, %p, %ld, %p, %ld\n",
	       width, height, src, src_bytes_per_row, dest, dest_bytes_per_row));
	memcpy(dest, src, dest_bytes_per_row*height);
}

static void
raw_line_copy(uint32 width, uint32 height,
              const void *src, uint32 src_bytes_per_row,
              void *dest, uint32 dest_bytes_per_row)
{
	TRACE(("RGB: raw_line_copy(%ld, %ld, %p, %ld, %p, %ld\n",
	       width, height, src, src_bytes_per_row, dest, dest_bytes_per_row));
	uint8 *srcline = (uint8*)src;
	uint8 *destline = (uint8*)dest;
	uint32 line = height;
	while(line > 0) {
		line--;
		memcpy(destline, srcline, src_bytes_per_row);
		srcline += src_bytes_per_row;
		destline += dest_bytes_per_row;
	}
}

static void
raw_line_flip(uint32 width, uint32 height,
              const void *src, uint32 src_bytes_per_row,
              void *dest, uint32 dest_bytes_per_row)
{
	TRACE(("RGB: raw_line_flip(%ld, %ld, %p, %ld, %p, %ld\n",
	       width, height, src, src_bytes_per_row, dest, dest_bytes_per_row));
	uint8 *srcline = (uint8*)src+src_bytes_per_row*height;
	uint8 *destline = (uint8*)dest;
	uint32 line = height;
	while(line > 0) {
		line--;
		srcline -= src_bytes_per_row;
		memcpy(destline, srcline, src_bytes_per_row);
		destline += dest_bytes_per_row;
	}
}


#define SHIFT_BITS 8

extern int32 LUT_1_164[0x100];
extern int32 LUT_1_596[0x100];
extern int32 LUT_0_813[0x100];
extern int32 LUT_0_392[0x100];
extern int32 LUT_2_017[0x100];

uchar fixed32toclipped8(int32 fixed)
{
	if (fixed <= 0)
		return 0;
	else if (fixed >= (255 << SHIFT_BITS))
		return 255;
	else
		return (fixed + (1 << (SHIFT_BITS - 1))) >> SHIFT_BITS;
}

inline uint32 ycbcr_to_rgb(uchar y, uchar cb, uchar cr)
{
	int32 Y;
	uchar red, green, blue;

/*
	red = clip8(1.164 * (y - 16) + 1.596 * (cr - 128));
	green = clip8(1.164 * (y - 16) - 0.813 * (cr - 128) - 0.392 * (cb - 128));
	blue = clip8(1.164 * (y - 16) + 2.017 * (cb - 128));
*/

	Y = LUT_1_164[y];

	red =	fixed32toclipped8(Y + LUT_1_596[cr]);
	green =	fixed32toclipped8(Y - LUT_0_813[cr] - LUT_0_392[cb]);
	blue =	fixed32toclipped8(Y + LUT_2_017[cb]);

#if B_HOST_IS_LENDIAN
	return (blue | (green << 8) | (red << 16));
#else
	return ((red << 8) | (green << 16) | (blue << 24));
#endif
}

#if 0
inline uint32 clip8(double d)
{
	if (d > 255.0)
		return 255;
	else if (d < 0)
		return 0;
	else
		return (uint32) d;
}


inline uint32 ycbcr_to_rgb(double y, double cb, double cr)
{
	uint8 red = clip8(1.164 * (y - 16) + 1.596 * (cr - 128));
	uint8 green = clip8(1.164 * (y - 16) - 0.813 * (cr - 128) - 0.392 * (cb - 128));
	uint8 blue = clip8(1.164 * (y - 16) + 2.017 * (cb - 128));

	//	TRACE(("YCbCr %.0f %.0f %.0f >> RGB %d %d %d\n", y, cb, cr, red, green, blue));

#if B_HOST_IS_LENDIAN
	return (blue | (green << 8) | (red << 16));
#else
	return ((red << 8) | (green << 16) | (blue << 24));
#endif
}

inline uint32 yuv_to_rgb(double y, double u, double v)
{
	uint8 red = clip8(1.164 * (y - 16) + 1.596 * (u));
	uint8 green = clip8(1.164 * (y - 16) - 0.813 * (u) - 0.392 * (v));
	uint8 blue = clip8(1.164 * (y - 16) + 2.017 * (v));

//	TRACE(("YCbCr %.0f %.0f %.0f > RGB %d %d %d\n", y, cb, cr, red, green, blue));

#if B_HOST_IS_LENDIAN
	return (blue | (green << 8) | (red << 16));
#else
	return ((red << 8) | (green << 16) | (blue << 24));
#endif
}
#endif

static void
yv12_to_rgb32(uint32 width, uint32 height,
              const void *src, uint32 src_bytes_per_row,
              void *dest, uint32 dest_bytes_per_row)
{
	TRACE(("RGB: yv12_to_rgb32(%ld, %ld, %p, %ld, %p, %ld\n",
	       width, height, src, src_bytes_per_row, dest, dest_bytes_per_row));
	uint8 *destline = (uint8*)dest;
	uint32 line = height;
	uint32 UVsrc_bytes_per_row = src_bytes_per_row / 3;
	uint32 Ysrc_bytes_per_row = src_bytes_per_row / 3 * 2;
	uint8 *Yline = (uint8*)src;
	uint8 *Vline = Yline + Ysrc_bytes_per_row * height;
	uint8 *Uline = Vline + UVsrc_bytes_per_row * height / 2;

//	TRACE(("yv12_to_rgb32 start Y:%p V:%p U:%p\n", Yline, Vline, Uline));

	while(line > 0) {
		line--;

		uint8 *YPtr = Yline;
		uint8 *VPtr = Vline;
		uint8 *UPtr = Uline;
		uint32 *dst = (uint32*)destline;
		for(uint32 x = 0; x < width/2; x++) {
			uint8 Y = *YPtr++;
			uint8 V = *VPtr++;
			uint8 U = *UPtr++;
			*dst++ =  ycbcr_to_rgb(Y, U, V);
			Y = *YPtr++;
			*dst++ =  ycbcr_to_rgb(Y, U, V);
		}
		Yline += Ysrc_bytes_per_row;
		if(!(line & 1)) {
			Vline += UVsrc_bytes_per_row;
			Uline += UVsrc_bytes_per_row;
		}
		destline += dest_bytes_per_row;
	}
//	TRACE(("yv12_to_rgb32 end   Y:%p V:%p U:%p\n", Yline, Vline, Uline));
}

static void
yuy2_to_rgb32(uint32 width, uint32 height,
              const void *src, uint32 src_bytes_per_row,
              void *dest, uint32 dest_bytes_per_row)
{
	TRACE(("RGB: yvy2_to_rgb32(%ld, %ld, %p, %ld, %p, %ld\n",
	       width, height, src, src_bytes_per_row, dest, dest_bytes_per_row));

	uint8 *srcline = (uint8*)src;
	uint8 *destline = (uint8*)dest;
	uint32 line = height;
	while(line > 0) {
		line--;
		
		uint32 *dstPtr = (uint32*)destline;
		uint8 *srcPtr = srcline;
		for(uint32 x = 0; x < width; x+=2) {
			uint8 Y0 = *srcPtr++;
			uint8 Cb0 = *srcPtr++;
			uint8 Y1 = *srcPtr++;
			uint8 Cr0 = *srcPtr++;
			*dstPtr++ = ycbcr_to_rgb(Y0, Cb0, Cr0);
			*dstPtr++ = ycbcr_to_rgb(Y1, Cb0, Cr0);
		}
		srcline += src_bytes_per_row;
		destline += dest_bytes_per_row;
	}
}
