#include <SupportDefs.h>
#include <ByteOrder.h>
#include "convert.h"

#include <string.h>

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

static void rgb24_to_rgb32(uint32 width, uint32 height,
                           const void *src, uint32 src_bytes_per_row,
                           void *dest, uint32 dest_bytes_per_row);

static void rgb24_to_rgb32_flip(uint32 width, uint32 height,
                                const void *src, uint32 src_bytes_per_row,
                                void *dest, uint32 dest_bytes_per_row);

static void rgb24big_to_rgb32big(uint32 width, uint32 height,
                                 const void *src, uint32 src_bytes_per_row,
                                 void *dest, uint32 dest_bytes_per_row);


static uint32
src_bytes_per_row(int width, int bytes_per_pixel)
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
get_converter(int srcdepth, bool bigendian, bool flip, media_video_display_info *display)
{
	if(srcdepth == 24 && bigendian) {
		if(flip)
			return NULL;
		if(display->format != B_RGB32_BIG) {
			display->format = B_RGB32_BIG;
			display->bytes_per_row = default_bytes_per_row(display->line_width, 4);
		}
		return rgb24big_to_rgb32big;
	}
	else if(srcdepth == 24) {
		if(display->format != B_RGB32) {
			display->format = B_RGB32;
			display->bytes_per_row = default_bytes_per_row(display->line_width, 4);
		}
		return flip ? rgb24_to_rgb32_flip : rgb24_to_rgb32;
	}
	else if(srcdepth == 32) {
		uint32 src_bytes_per_row_32 = src_bytes_per_row(display->line_width, 4);
		if(display->format != (bigendian ? B_RGB32_BIG : B_RGB32)) {
			display->format = bigendian ? B_RGB32_BIG : B_RGB32;
			display->bytes_per_row =
				default_bytes_per_row(display->line_width, 4);
		}
		if(flip)
			return raw_line_flip;
		else if(display->bytes_per_row == src_bytes_per_row_32)
			return raw_copy;
		else
			return raw_line_copy;
	}
	else if(srcdepth == 16) {
		uint32 src_bytes_per_row_16 = src_bytes_per_row(display->line_width, 2);
		if(display->format != (bigendian ? B_RGB15_BIG : B_RGB15)) {
			display->format = bigendian ? B_RGB15_BIG : B_RGB15;
			display->bytes_per_row =
				default_bytes_per_row(display->line_width, 2);
		}
		if(flip)
			return raw_line_flip;
		else if(display->bytes_per_row == src_bytes_per_row_16)
			return raw_copy;
		else
			return raw_line_copy;
	}
	return NULL;
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

static void
rgb24_to_rgb32(uint32 width, uint32 height,
               const void *src, uint32 src_bytes_per_row,
               void *dest, uint32 dest_bytes_per_row)
{
	TRACE(("RGB: rgb24_to_rgb32(%ld, %ld, %p, %ld, %p, %ld\n",
	       width, height, src, src_bytes_per_row, dest, dest_bytes_per_row));
	uint8 *srcline = (uint8*)src;
	uint8 *destline = (uint8*)dest;
	uint32 line = height;
	while(line > 0) {
		line--;

		uint8 *srcPtr = srcline;
		uint8 *dst = destline;
		for(uint32 x = 0; x < width; x++) {
			*dst++ = *srcPtr++;
			*dst++ = *srcPtr++;
			*dst++ = *srcPtr++;
			*dst++ = 0xff;
		}
		srcline += src_bytes_per_row;
		destline += dest_bytes_per_row;
	}
}

static void
rgb24big_to_rgb32big(uint32 width, uint32 height,
                     const void *src, uint32 src_bytes_per_row,
                     void *dest, uint32 dest_bytes_per_row)
{
	TRACE(("RGB: rgb24_to_rgb32(%ld, %ld, %p, %ld, %p, %ld\n",
	       width, height, src, src_bytes_per_row, dest, dest_bytes_per_row));
	uint8 *srcline = (uint8*)src;
	uint8 *destline = (uint8*)dest;
	uint32 line = height;
	while(line > 0) {
		line--;

		uint8 *srcPtr = srcline;
		uint8 *dst = destline;
		for(uint32 x = 0; x < width; x++) {
			*dst++ = 0xff;
			*dst++ = *srcPtr++;
			*dst++ = *srcPtr++;
			*dst++ = *srcPtr++;
		}
		srcline += src_bytes_per_row;
		destline += dest_bytes_per_row;
	}
}

static void
rgb24_to_rgb32_flip(uint32 width, uint32 height,
                    const void *src, uint32 src_bytes_per_row,
                    void *dest, uint32 dest_bytes_per_row)
{
	TRACE(("RGB: rgb24_to_rgb32_flip(%ld, %ld, %p, %ld, %p, %ld\n",
	       width, height, src, src_bytes_per_row, dest, dest_bytes_per_row));
	uint8 *srcline = (uint8*)src+src_bytes_per_row*height;
	uint8 *destline = (uint8*)dest;
	uint32 line = height;
	while(line > 0) {
		line--;
		srcline -= src_bytes_per_row;

		uint8 *srcPtr = srcline;
		uint8 *dst = destline;
		for(uint32 x = 0; x < width; x++) {
			*dst++ = *srcPtr++;
			*dst++ = *srcPtr++;
			*dst++ = *srcPtr++;
			*dst++ = 0xff;
		}
		destline += dest_bytes_per_row;
	}

#if 0  //may give better perfomance on ppc

	//	Setup pointers
	uchar  *srcPtr = (uchar *)src, *line;
	uint32 *dst32 = (uint32 *)dest;
	uint16 *dst16 = (uint16 *)dest;
	uint32 pix32;
	uint16 pix16;

	for (int32 rows = height-1; rows >= 0; rows--) {
		line = &srcPtr[width * 3 * rows];
		for (int32 columns = 0; columns < width; columns++) {
#if B_HOST_IS_LENDIAN
			pix32 = (line[2] << 16) | (line[1] << 8) | line[0];
#else
			pix32 = (line[0] << 24) | (line[1] << 16) | (line[2] << 8);
#endif
			*dst32++ = pix32;
			line += 3;
		}
	}
#endif
}
