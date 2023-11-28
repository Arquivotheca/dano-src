//****************************************************************************************
//
//	File:		Encode.cpp
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "Encode.h"
#include "TGATranslator.h"
#include "Prefs.h"
#include <Screen.h>
#include <ByteOrder.h>
#include <TranslationDefs.h>
#include <TranslatorFormats.h>
#include <stdlib.h>

status_t WriteTarga(BPositionIO &input, BPositionIO &output) {
	TranslatorBitmap tbheader;
	status_t err = input.Read(&tbheader, sizeof(TranslatorBitmap));
	if (err < B_OK) return err;
	else if (err < (int)sizeof(TranslatorBitmap)) return B_ERROR;
	
	// Grab dimension, color space, and size information from the stream
	BRect bounds;
	bounds.left = B_BENDIAN_TO_HOST_FLOAT(tbheader.bounds.left);
	bounds.top = B_BENDIAN_TO_HOST_FLOAT(tbheader.bounds.top);
	bounds.right = B_BENDIAN_TO_HOST_FLOAT(tbheader.bounds.right);
	bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(tbheader.bounds.bottom);
	
	int width = bounds.IntegerWidth() + 1;
	int height = bounds.IntegerHeight() + 1;
	color_space colors = (color_space)B_BENDIAN_TO_HOST_INT32(tbheader.colors);
	int row_bytes = B_BENDIAN_TO_HOST_INT32(tbheader.rowBytes);
	
	Prefs prefs("TGATranslatorSettings");
	err = WriteTargaHeader(output, bounds, prefs.bits_per_pixel, prefs.compressed,
		prefs.greyscale);
	if (err != B_OK) return err;
	
	unsigned char *scanline = (unsigned char *)malloc(width * 4);
	if (scanline == NULL) return B_NO_MEMORY;
	
	for (int y = 0; y < height; y++) {
		err = input.Read(scanline, row_bytes);
		if (err < row_bytes) {
			free(scanline);
			return (err < B_OK) ? err : B_ERROR;
		}
		
		err = ExpandScanline(scanline, colors, width);
		if (err == B_OK) err = CollapseScanline(scanline, prefs.bits_per_pixel, width, prefs.greyscale);
		if (err != B_OK) {
			free(scanline);
			return err;
		}
		
		if (prefs.bits_per_pixel == 32 || prefs.bits_per_pixel == 24 ||
			prefs.bits_per_pixel == 16 || prefs.bits_per_pixel == 8) {
			
			int size = (prefs.bits_per_pixel >> 3) * width;
			err = output.Write(scanline, size);
			if (err != size) {
				if (err > 0) err = B_DEVICE_FULL;
				free(scanline);
				return err;
			}
		}
	}
	
	free(scanline);
	return B_OK;
}

status_t WriteTargaHeader(BPositionIO &output, BRect bounds, int bits_per_pixel,
	bool compressed, bool greyscale) {
	
	// Set up top-down, left to right data with 8, 16, 24, or 32 bpp
	TargaHeader header;
	header.cmtlen = 0;
	header.pixsize = bits_per_pixel;
	header.descriptor = 0x20;
	
	header.left = 0;
	header.top = 0;
	header.width = B_HOST_TO_LENDIAN_INT16(bounds.IntegerWidth() + 1);
	header.height = B_HOST_TO_LENDIAN_INT16(bounds.IntegerHeight() + 1);
	
	// No palette by default
	header.maptype = 0;
	header.cmap_origin = 0;
	header.cmap_origin_hi = 0;
	header.cmap_size = 0;
	header.cmap_size_hi = 0;
	header.cmap_bits = 0;

	if (bits_per_pixel == 8) {
		if (greyscale) {
			header.version = 3;
			if (compressed) header.version += 8;
		} else {
			header.version = 1;
			if (compressed) header.version += 8;
			
			// Use a 256 color palette
			header.maptype = 1;
			header.cmap_origin = 0;
			header.cmap_origin_hi = 0;
			header.cmap_size = 0;
			header.cmap_size_hi = 1;
			header.cmap_bits = 32;
		}
	} else {
		header.version = 2;
		if (compressed) header.version += 8;
		
		// 32 bit data has eight bits of alpha
		if (bits_per_pixel == 32) header.descriptor |= 0x08;
	}
	
	status_t err = output.Write(&header, sizeof(header));
	if (err != sizeof(header)) {
		if (err >= 0) err = B_DEVICE_FULL;
		return err;
	}
	
	// Write palette
	if (bits_per_pixel == 8 && !greyscale) {
		const color_map *map = system_colors();
		unsigned char data[1024];
		
		for (int x = 0; x < 256; x++) {
			data[x * 4] = map->color_list[x].blue;
			data[x * 4 + 1] = map->color_list[x].green;
			data[x * 4 + 2] = map->color_list[x].red;
			data[x * 4 + 3] = map->color_list[x].alpha;
		}
		data[B_TRANSPARENT_8_BIT * 4 + 3] = 0;
		
		err = output.Write(data, 1024);
		if (err != 1024) {
			if (err > 0) err = B_DEVICE_FULL;
			return err;
		}
	}
	
	return B_OK;
}

// Expand to B_RGBA32
status_t ExpandScanline(uchar *scanline, color_space colors, int width) {
	if (colors == B_RGB32 || colors == B_RGBA32) {
		return B_OK;
	} else if (colors == B_RGB16) {
		uchar *in = scanline + (width - 1) * 2;
		uchar *out = scanline + (width - 1) * 4;
		for (int x = 0; x < width; x++) {
			out[0] = (in[0] & 0x1f) << 3;							// B
			out[1] = ((in[1] & 0x07) << 5) | ((in[0] & 0xe0) >> 3);	// G
			out[2] = (in[1] & 0xf8);								// R
			out[3] = 255;											// A
			in -= 2;
			out -= 4;
		}
	} else if (colors == B_RGB15 || colors == B_RGBA15) {
		uchar *in = scanline + (width - 1) * 2;
		uchar *out = scanline + (width - 1) * 4;
		for (int x = 0; x < width; x++) {
			out[0] = (in[0] & 0x1f) << 3;							// B
			out[1] = ((in[1] & 0x03) << 6) | ((in[0] & 0xe0) >> 2); // G
			out[2] = (in[1] & 0x7c) << 1;							// R
			out[3] = 255;											// A
			in -= 2;
			out -= 4;
		}
	} else if (colors == B_CMAP8) {
		BScreen screen;
		const color_map *map = screen.ColorMap();
		const rgb_color *list = map->color_list;
		uchar *in = scanline + width - 1;
		uchar *out = scanline + (width - 1) * 4;
		for (int x = 0; x < width; x++) {
			rgb_color color = list[*in];
			out[0] = color.blue;
			out[1] = color.green;
			out[2] = color.red;
			out[3] = color.alpha;
			in--;
			out -= 4;
		}
	} else return B_ERROR;
	return B_OK;
}

// Collapse from B_RGBA32 to chosen depth
status_t CollapseScanline(uchar *scanline, int bits, int width, bool greyscale) {
	uchar *in = scanline;
	uchar *out = scanline;
	
	if (bits == 32) return B_OK;
	else if (bits == 24) {
		for (int x = 0; x < width; x++) {
			out[0] = in[0];
			out[1] = in[1];
			out[2] = in[2];
			in += 4;
			out += 3;
		}
	} else if (bits == 16) {
		for (int x = 0; x < width; x++) {
			out[0] = ((in[1] & 0x38) << 2) + ((in[0] & 0xf8) >> 3);
			out[1] = ((in[2] & 0xf8) >> 1) + ((in[1] & 0xc0) >> 6);
			in += 4;
			out += 2;
		}
	} else if (bits == 8) {
		if (greyscale) {
			for (int x = 0; x < width; x++) {
				out[0] = (in[0] + (in[1] << 1) + in[2]) >> 2;
				in += 4;
				out++;
			}
		} else {
			BScreen screen;
			for (int x = 0; x < width; x++) {
				out[0] = screen.IndexForColor(in[2], in[1], in[0], in[3]);
				in += 4;
				out++;
			}
		}
	} else return B_ERROR;
	return B_OK;
}