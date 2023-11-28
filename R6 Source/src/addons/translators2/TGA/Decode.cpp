//****************************************************************************************
//
//	File:		Decode.cpp
//
//	Written by:	Jon Watte and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "Decode.h"
#include <ByteOrder.h>
#include <TranslationDefs.h>
#include <TranslatorFormats.h>
#include <DataIO.h>
#include <Message.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define comment_extension			"/comment"
extern bool debug;

// This function needs to be pretty stingy, because Targa doesn't have a magic
// number in its header - these 18 bytes could be total garbage
bool ValidateHeader(TargaHeader &header) {
	// Size must be reasonable
	if (B_LENDIAN_TO_HOST_INT16(header.width) > 4096) return false;
	if (B_LENDIAN_TO_HOST_INT16(header.height) > 4096) return false;

	// Palette must be present or missing
	if (header.maptype != 0 && header.maptype != 1) return false;
	
	// If there is a palette, and this is not a greyscale image, make sure its origin
	// and size are valid (can't have one color images, etc.)
	if (header.maptype == 1 && header.pixsize < 16 && header.version != 3 && header.version != 11) {
		int origin = header.cmap_origin + (header.cmap_origin_hi << 8);
		int size = header.cmap_size + (header.cmap_size_hi << 8);
		if (origin > 254 || (origin + size) > 256 || size < 2 || size > 256) return false;
	}
	
	// Check for a valid pixel depth
	switch (header.pixsize) {
		case 8:
			if (header.descriptor != 0 && header.descriptor != 32 && header.descriptor != 8 &&
				header.descriptor != 40) return false;
			// Greyscale images should not have a palette
			if (header.version == 3 || header.version == 11) {
				if (header.cmap_bits != 0) return false;
			} else { // Color images need a 24 or 32 bit per entry palette
				if (header.cmap_bits != 24 && header.cmap_bits != 32) return false;
			}
			break;
		case 16:
			// 16 bit can have one bit of alpha or none
			if ((header.descriptor & 0x0f) != 0 && (header.descriptor & 0x0f) != 1) return false;
			break;
		case 24:
			// 24 bit data can't have any alpha information
			if ((header.descriptor & 0x0f) != 0) return false;
			break;
		case 32:
			// 32 bit data must have 8 bits of alpha
			if ((header.descriptor & 0x0f) != 8) return false;
			break;
		default:
			return false;
			break;
	}
	
	return true;
}

bool GetColorMap(BPositionIO &input, TargaHeader &header, uint32 *palette) {
	if (header.maptype == 1) {
		// BGRA, black to white
		for (int x = 0; x < 256; x++) {
			palette[x] = B_BENDIAN_TO_HOST_INT32((x * 0x01010100) | 0xff);
		}
		
		int read = header.cmap_size + (header.cmap_size_hi << 8);
		int count = read;
		if (read + header.cmap_origin > 256) return false;;
		read = read * header.cmap_bits / 8;
		
		status_t err = input.Read(&palette[header.cmap_origin], read);
		if (err < B_OK) return false;
		if (err != count * header.cmap_bits / 8) return false;

		// Expand palette to include alpha info if missing
		if (header.cmap_bits != 32) {
			uchar *ptr = (uchar *)&palette[header.cmap_origin];
			for (int x = count - 1; x >= 0; x--) {
				uint32 blue = ptr[x * 3];
				uint32 green = ptr[x * 3 + 1];
				uint32 red = ptr[x * 3 + 2];
				ptr[x * 4] = blue;
				ptr[x * 4 + 1] = green;
				ptr[x * 4 + 2] = red;
				ptr[x * 4 + 3] = 255;
			}
		}
		return true;
	}
	return false;
}

status_t WriteBitmap(BPositionIO &input, BPositionIO & output, BMessage *ioExtension) {
	// Read targa header
	TargaHeader tgaheader;
	ssize_t read = sizeof(tgaheader);
	status_t err = input.Read(&tgaheader, read);
	if (err != read) {
		if (err >= 0) err = B_ERROR;
		return err;
	}
	
	if (!ValidateHeader(tgaheader)) return B_NO_TRANSLATOR;

	int width = B_LENDIAN_TO_HOST_INT16(tgaheader.width);
	int height = B_LENDIAN_TO_HOST_INT16(tgaheader.height);
	int row_bytes = width * 4;
	if (debug) printf("Size is %d x %d, row_bytes %d\n", width, height, row_bytes);

	// Set up header
	TranslatorBitmap tbheader;
	tbheader.magic = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	tbheader.bounds.left = 0;
	tbheader.bounds.top = 0;
	tbheader.bounds.right = B_HOST_TO_BENDIAN_FLOAT(width - 1);
	tbheader.bounds.bottom = B_HOST_TO_BENDIAN_FLOAT(height - 1);
	tbheader.rowBytes = B_HOST_TO_BENDIAN_INT32(row_bytes);
	tbheader.dataSize = B_HOST_TO_BENDIAN_INT32(row_bytes * height);

	// Always output in 32 bit format
	tbheader.colors = (color_space)B_HOST_TO_BENDIAN_INT32(B_RGBA32);
	err = output.Write(&tbheader, sizeof(tbheader));
	if (err != sizeof(tbheader)) {
		if (err >= 0) err = B_DEVICE_FULL;
		return err;
	}
	err = output.SetSize(B_BENDIAN_TO_HOST_INT32(tbheader.dataSize) + sizeof(tbheader));
	if (err < B_OK) return err;

	// Deal with comment, if any
	if (tgaheader.cmtlen) {
		char comment[256];
		read = tgaheader.cmtlen;
		err = input.Read(comment, read);
		if (err < B_OK) return err;
		if (err != tgaheader.cmtlen) return B_ERROR;
		comment[tgaheader.cmtlen] = 0;
		if (ioExtension != NULL) ioExtension->AddString(comment_extension, comment);
	}

	// Set up buffer
	uint32 *scanline = (uint32 *)malloc(row_bytes);
	if (scanline == NULL) return B_NO_MEMORY;
	off_t pos = output.Position();
	
	// Common to all
	int start, end, add;
	if (tgaheader.descriptor & 0x20) {
		start = 0;
		end = B_LENDIAN_TO_HOST_INT16(tgaheader.height);
		add = 1;
	} else {
		start = B_LENDIAN_TO_HOST_INT16(tgaheader.height) - 1;
		end = -1;
		add = -1;
	}

	// True color uncompressed images
	if ((tgaheader.version == 2) && ((tgaheader.pixsize == 32) || (tgaheader.pixsize == 24) || (tgaheader.pixsize == 16))) {
		err = WriteBitmapTrueColorUncompressed(input, output, scanline, pos, start, end, add,
			tgaheader.pixsize, row_bytes, width);
	}
	// True color RLE compressed images
	else if ((tgaheader.version == 10) && ((tgaheader.pixsize == 32) || (tgaheader.pixsize == 24) || (tgaheader.pixsize == 16))) {
		err = WriteBitmapTrueColorCompressed(input, output, scanline, pos, start, end, add,
			tgaheader.pixsize, row_bytes, width);
	}
	// 8 bit color uncompressed images
	else if (tgaheader.pixsize == 8 && tgaheader.version == 1) {
		uint32 palette[256];
		if (!GetColorMap(input, tgaheader, palette)) return B_ERROR;
		err = WriteBitmap8BitUncompressed(input, output, scanline, pos, start, end, add,
			tgaheader.pixsize, row_bytes, width, palette);
	}
	// 8 bit greyscale uncompressed images
	else if (tgaheader.pixsize == 8 && tgaheader.version == 3) {
		err = WriteBitmap8BitUncompressed(input, output, scanline, pos, start, end, add,
			tgaheader.pixsize, row_bytes, width, NULL);
	}
	// 8 bit paletted RLE images
	else if ((tgaheader.version == 9) && (tgaheader.pixsize == 8)) {
		uint32 palette[256];
		if (!GetColorMap(input, tgaheader, palette)) return B_ERROR;
		err = WriteBitmap8BitCompressed(input, output, scanline, pos, start, end, add,
			tgaheader.pixsize, row_bytes, width, palette);
	} else {
		err = B_NO_TRANSLATOR;
	}

	free(scanline);
	return err;
}

status_t WriteBitmapTrueColorUncompressed(BPositionIO &input, BPositionIO &output, uint32 *scanline,
	int pos, int start, int end, int add, int bit_depth, int row_bytes, int width) {

	if (debug) printf("True color, uncompressed image, %d bits per pixel\n", bit_depth);
	status_t err = B_OK;
	int read = width * bit_depth / 8;

	while (start != end) {
		err = output.Seek(pos + row_bytes * start, SEEK_SET);
		if (err < B_OK) return err;
		
		if (bit_depth == 32) { // Already in correct order
			err = input.Read(scanline, read);
			if (err != read) {
				if (err >= 0) err = B_ERROR;
				return err;
			}
		} else if (bit_depth == 24) {
			uchar *src = ((uchar *)scanline) + width;
			err = input.Read(src, read);
			if (err != read) {
				if (err >= 0) err = B_ERROR;
				return err;
			}
			
			uchar *dst = (uchar *)scanline;
			while (dst != src) {
				dst[0] = src[0]; // B
				dst[1] = src[1]; // G
				dst[2] = src[2]; // R
				dst[3] = 255;    // A
				dst += 4;
				src += 3;
			}
		} else { // 16 bit
			uchar *src = ((uchar *)scanline) + (width * 2);
			err = input.Read(src, read);
			if (err != read) {
				if (err >= 0) err = B_ERROR;
				return err;
			}
			
			uchar *dst = (uchar *)scanline;
			while (dst != src) {
				dst[0] = (src[0] & 0x1f) << 3;								// B
				dst[1] = ((src[1] & 0x03) << 6) | ((src[0] & 0xe0) >> 2);	// G
				dst[2] = (src[1] & 0x7c) << 1;								// R
				dst[3] = 255;												// A
				dst += 4;
				src += 2;
			}
		}
		
		// Output the 32 bit data
		if (debug) printf("Wrote %d bytes\n", row_bytes);
		err = output.Write(scanline, row_bytes);
		if (err != row_bytes) {
			if (err >= 0) err = B_DEVICE_FULL;
			return err;
		}
		start += add;
	}
	return B_OK;
}

status_t WriteBitmapTrueColorCompressed(BPositionIO &input, BPositionIO &output, uint32 *scanline,
	int pos, int start, int end, int add, int bit_depth, int row_bytes, int width) {

	if (debug) printf("True color, compressed image, %d bits per pixel\n", bit_depth);
	status_t err = B_OK;
	
	// The worst RLE case, depending on the encoder, is five bytes per pixel
	int packet_buffer_size = width * (1 + bit_depth / 8);
	uchar *packet_buffer = (uchar *)malloc(packet_buffer_size);
	if (packet_buffer == NULL) return B_NO_MEMORY;
	
	// These must persist across scanlines
	uchar *packet_pos = (uchar *)packet_buffer;
	int packet_count = 0, normal = 0, compressed = 0;
	bool get_more_data = true;
	int bpp = bit_depth >> 3;
	
	while (start != end) {
		int w = width;
		err = output.Seek(pos + row_bytes * start, SEEK_SET);
		if (err < B_OK) { free(packet_buffer); return err; }
		
		// Grab batches of data in advance so we don't Read() for every packet
		if (get_more_data) {
			if (packet_count != 0) memmove(packet_buffer, packet_pos, packet_count); // slide everything to the front
			err = input.Read(packet_buffer + packet_count, packet_buffer_size - packet_count);
			if (err < B_OK) { free(packet_buffer); return err; }
			if (err < packet_buffer_size - packet_count) get_more_data = false; // done fetching
			packet_count = packet_count + err;
			packet_pos = (uchar *)packet_buffer;
		}
		
		uchar *dst = (uchar *)scanline;
		while (w > 0) {
			// Add leftover compressed pixels if present
			if (compressed > 0 && w > 0) {
				if (packet_count < bpp) { free(packet_buffer); return B_ERROR; }
				int count = (compressed > w) ? w : compressed;
				if (bit_depth == 32) {
					for (int x = 0; x < count; x++) {
						dst[0] = packet_pos[0]; // B
						dst[1] = packet_pos[1]; // G
						dst[2] = packet_pos[2]; // R
						dst[3] = packet_pos[3]; // A
						dst += 4;
					}
					packet_pos += 4;
					packet_count -= 4;
				} else if (bit_depth == 24) {
					for (int x = 0; x < count; x++) {
						dst[0] = packet_pos[0]; // B
						dst[1] = packet_pos[1]; // G
						dst[2] = packet_pos[2]; // R
						dst[3] = 255;           // A
						dst += 4;
					}
					packet_pos += 3;
					packet_count -= 3;
				} else { // 16 bit
					for (int x = 0; x < count; x++) {
						dst[0] = (packet_pos[0] & 0x1f) << 3;									// B
						dst[1] = ((packet_pos[1] & 0x03) << 6) | ((packet_pos[0] & 0xe0) >> 2);	// G
						dst[2] = (packet_pos[1] & 0x7c) << 1;									// R
						dst[3] = 255;															// A
						dst += 4;
					}
					packet_pos += 2;
					packet_count -= 2;
				}
				w -= count;
				compressed -= count;
			}
			// Add leftover normal pixels if present
			if (normal > 0 && w > 0) {
				int count = (normal > w) ? w : normal;
				if (packet_count < bpp * count) { free(packet_buffer); return B_ERROR; }
				if (bit_depth == 32) {
					memcpy(dst, packet_pos, count * 4);
					dst += (count * 4);
					packet_pos += (count * 4);
					packet_count -= (count * 4);
				} else if (bit_depth == 24) {
					for (int x = 0; x < count; x++) {
						dst[0] = packet_pos[0]; // B
						dst[1] = packet_pos[1]; // G
						dst[2] = packet_pos[2]; // R
						dst[3] = 255;           // A
						dst += 4;
						packet_pos += 3;
					}
					packet_count -= (count * 3);
				} else {
					for (int x = 0; x < count; x++) {
						dst[0] = (packet_pos[0] & 0x1f) << 3;									// B
						dst[1] = ((packet_pos[1] & 0x03) << 6) | ((packet_pos[0] & 0xe0) >> 2);	// G
						dst[2] = (packet_pos[1] & 0x7c) << 1;									// R
						dst[3] = 255;															// A
						dst += 4;
						packet_pos += 2;
					}
					packet_count -= (count * 2);
				}
				normal -= count;
				w -= count;
			}
			// Read new packet header
			if (compressed == 0 && normal == 0 && w > 0) {
				if (packet_count < 1) { free(packet_buffer); return B_ERROR; }
				if (*packet_pos & 0x80) compressed = (*packet_pos & 0x7f) + 1;
				else normal = (*packet_pos & 0x7f) + 1;
				packet_pos++;
				packet_count--;
            }
		}
		
		// Output the 32 bit data
		if (debug) printf("Wrote %d bytes\n", row_bytes);
		err = output.Write(scanline, row_bytes);
		if (err != row_bytes) {
			if (err >= 0) err = B_DEVICE_FULL;
			free(packet_buffer);
			return err;
		}
		start += add;
	}
	free(packet_buffer);
	return B_OK;
}

status_t WriteBitmap8BitUncompressed(BPositionIO &input, BPositionIO &output, uint32 *scanline,
	int pos, int start, int end, int add, int bit_depth, int row_bytes, int width, uint32 *palette) {

	if (debug) printf("Paletted, uncompressed image\n");
	status_t err = B_OK;

	while (start != end) {
		err = output.Seek(pos + row_bytes * start, SEEK_SET);
		if (err < B_OK) return err;
		// Based on advice from JBQ, I'm reading into the last 1/4 of the buffer
		// and expanding my four byte values from lower to higher memory for
		// performance reasons.
		err = input.Read(((uchar *)scanline) + (width * 3), width);
		if (err != width) {
			if (err >= 0) err = B_ERROR;
			return err;
		}

		uchar *src = ((uchar *)scanline) + (width * 3);
		if (palette != NULL) { // Color
			uint32 *dst = scanline;
			while (src != (uchar *)dst) {
				dst[0] = palette[src[0]];
				src += 1;
				dst += 1;
			}
		} else {
			uchar *dst = (uchar *)scanline;
			// We are writing to B_RGB32 on both platforms, always little endian
			// By using uchar we avoid writing different code for each
			while (src != dst) { // Greyscale
				dst[0] = src[0]; // B
				dst[1] = src[0]; // G
				dst[2] = src[0]; // R
				dst[3] = 255;    // A
				src += 1;
				dst += 4;
			}
		}

		// Output the 32 bit data
		if (debug) printf("Wrote %d bytes\n", row_bytes);
		err = output.Write(scanline, row_bytes);
		if (err != row_bytes) {
			if (err >= 0) err = B_DEVICE_FULL;
			return err;
		}
		start += add;
	}
	return B_OK;
}

status_t WriteBitmap8BitCompressed(BPositionIO &input, BPositionIO &output, uint32 *scanline,
	int pos, int start, int end, int add, int bit_depth, int row_bytes, int width, uint32 *palette) {

	if (debug) printf("Paletted, compressed image\n");
	status_t err = B_OK;

	// The worst RLE case, depending on the encoder, is two bytes per pixel
	int packet_buffer_size = width * 2;
	uchar *packet_buffer = (uchar *)malloc(packet_buffer_size);
	if (packet_buffer == NULL) return B_NO_MEMORY;
	
	// These must persist across scanlines
	uchar *packet_pos = (uchar *)packet_buffer;
	int packet_count = 0, normal = 0, compressed = 0;
	bool get_more_data = true;
	
	while (start != end) {
		int w = width;
		err = output.Seek(pos + row_bytes * start, SEEK_SET);
		if (err < B_OK) { free(packet_buffer); return err; }
		
		// Grab batches of data in advance so we don't Read() for every packet
		if (get_more_data) {
			if (packet_count != 0) memmove(packet_buffer, packet_pos, packet_count); // slide everything to the front
			err = input.Read(packet_buffer + packet_count, packet_buffer_size - packet_count);
			if (err < B_OK) { free(packet_buffer); return err; }
			if (err < packet_buffer_size - packet_count) get_more_data = false; // done fetching
			packet_count = packet_count + err;
			packet_pos = (uchar *)packet_buffer;
		}
		
		uint32 *dst = (uint32 *)scanline;
		while (w > 0) {
			// Add leftover compressed pixels if present
			if (compressed > 0 && w > 0) {
				if (packet_count < 1) { free(packet_buffer); return B_ERROR; }
				int count = (compressed > w) ? w : compressed;
				for (int x = 0; x < count; x++) {
					dst[0] = palette[*packet_pos];
					dst++;
				}
				packet_pos++;
				packet_count--;
				w -= count;
				compressed -= count;
			}
			// Add leftover normal pixels if present
			if (normal > 0 && w > 0) {
				int count = (normal > w) ? w : normal;
				if (packet_count < count) { free(packet_buffer); return B_ERROR; }
				for (int x = 0; x < count; x++) {
					dst[0] = palette[*packet_pos];
					packet_pos++;
					dst++;
				}
				packet_count -= count;
				normal -= count;
				w -= count;
			}
			// Read new packet header
			if (compressed == 0 && normal == 0 && w > 0) {
				if (packet_count < 1) { free(packet_buffer); return B_ERROR; }
				if (*packet_pos & 0x80) compressed = (*packet_pos & 0x7f) + 1;
				else normal = (*packet_pos & 0x7f) + 1;
				packet_pos++;
				packet_count--;
            }
		}
		
		// Output the 32 bit data
		if (debug) printf("Wrote %d bytes\n", row_bytes);
		err = output.Write(scanline, row_bytes);
		if (err != row_bytes) {
			if (err >= 0) err = B_DEVICE_FULL;
			free(packet_buffer);
			return err;
		}
		start += add;
	}
	free(packet_buffer);
	return B_OK;
}