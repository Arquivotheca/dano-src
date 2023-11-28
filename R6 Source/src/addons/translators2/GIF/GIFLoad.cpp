//******************************************************************************
//
//	File:			GIFLoad.cpp
//
//	Copyright 1999, Be Incorporated, All Rights Reserved.
//  Copyright 1998-1999, Daniel Switkin
//
//******************************************************************************

#include "GIFLoad.h"
#include <ByteOrder.h>
#include <TranslatorFormats.h>
#include <stdlib.h>
#include <stdio.h>

static bool debug = false;

GIFLoad::GIFLoad(BPositionIO *input, GIFDecoder *gifdecoder) {
	this->input = input;
	this->gifdecoder = gifdecoder;
	Init();
	
	if (!ReadGIFHeader()) {
		fatalerror = true;
		return;
	}
	
	// Set the fatalerror flag either because something went wrong,
	// or this was the only frame. In either case, release memory ASAP.
	if (!DecodeNextFrame()) {
		fatalerror = true;
	}
}

bool GIFLoad::DecodeNextFrame() {
	if (fatalerror) return false;

	unsigned char c;
	while (true) {
		if (input->Read(&c, 1) < 1) {
			fatalerror = true;
			return false;
		}

		if (c == 0x2c) {
			if (!ReadGIFImageHeader() || !ReadGIFImageData()) {
				if (debug) printf("GIFLoad::GIFLoad() - A fatal error occurred\n");
				fatalerror = true;
				return false;
			}
			
			if (first_time) {
				if (!LookAheadForAnimated()) {
					fatalerror = true;
					return false;
				}
				first_time = false;
			}
			frame_number++;
			return true;
		} else if (c == 0x21) {
			unsigned char d;
			if (input->Read(&d, 1) < 1) {
				fatalerror = true;
				return false;
			}
			if (d == 0xff) {
				if (!ReadGIFLoopBlock()) {
					fatalerror = true;
					return false;
				}
			} else if (d == 0xf9) {
				if (!ReadGIFControlBlock()) {
					fatalerror = true;
					return false;
				}
			} else if (d == 0xfe) {
				if (!ReadGIFCommentBlock()) {
					fatalerror = true;
					return false;
				}
			} else {
				if (!ReadGIFUnknownBlock(d)) {
					fatalerror = true;
					return false;
				}
			}
		} else if (c == 0x3b) {
			if (debug) printf("GIFLoad::GIFLoad() - Reached terminating char\n");
			if (loop_count <= 0) break;
			else if (!SeekToBeginning()) {
				fatalerror = true;
				return false;
			}
		} else if (c != 0x00) {
			if (!ReadGIFUnknownBlock(c)) {
				fatalerror = true;
				return false;
			}
		}
	}
	if (debug) printf("GIFLoad::GIFLoad() - Image finished\n");
	return false;
}

bool GIFLoad::IsTransparent() const {
	if (global_palette != NULL) return global_palette->usetransparent;
	else return false;
}

bool GIFLoad::NeedsBackgroundRepainted() const {
	// Without the RESTORE_BACKGROUND flag, we're allowed to draw
	// directly over the previous contents (after restoring the
	// background color, if requested). This is the only situation
	// where the browser must repaint the background before every frame.
	if (global_palette != NULL) return (global_palette->usetransparent && previous_disposal_method == RESTORE_BACKGROUND);
	else return false;
}

void GIFLoad::Init() {
	const char *debug_text = getenv("GIF_TRANSLATOR_DEBUG");
	if ((debug_text != NULL) && (atoi(debug_text) != 0)) debug = true;

	fatalerror = false;
	palette = NULL;
	global_palette = NULL;
	local_palette = NULL;
	frame_number = 0;
	disposal_method = NO_DISPOSAL;
	previous_disposal_method = NO_DISPOSAL;
	loop_count = -1;
	animated = false;
	duration = 20;
	first_time = true;
	input->Seek(0, SEEK_SET);
	head_memblock = NULL;
}

// Used for looping animations. It skips the global header and
// palette, going straight to the first frame.
bool GIFLoad::SeekToBeginning() {
	loop_count--;
	frame_number = 0;
	if (input->Seek(0, SEEK_SET) != 0) return false;
	unsigned char header[13];
	if (input->Read(header, 13) < 13) return false;
	if (header[10] & 0x80) {
		int size = (header[10] & 0x07) + 1;
		size = (1 << size) * 3;
		if (input->Seek(size, SEEK_CUR) != 13 + size) return false;
	}
	return true;
}

// The GIF format doesn't tell you whether a given image is animated or not.
// It is therefore necessary to look ahead for a second image header to make
// a judgement. This is done so that state for the second image does not trample
// state for the first image before Opera is done using it.
bool GIFLoad::LookAheadForAnimated() {
	off_t position = input->Position();

	unsigned char type;
	while (true) {
		if (input->Read(&type, 1) < 1) return false;
		if (type == 0x2c) {
			// Found an image header for the second frame - we're animated
			if (debug) printf("GIFLoad::LookAheadForAnimated() - Animated\n");
			animated = true;
			break;
		} else if (type == 0x3b) {
			// Reached the end of the file without finding another frame,
			// so this GIF is not animated
			if (debug) printf("GIFLoad::LookAheadForAnimated() - Not animated\n");
			break;
		} else if (type == 0x00) {
			continue;
		} else {
			// Some other block - just skip it
			unsigned char length;
			if (type == 0x21) {
				// Skip the subblock id if present
				if (input->Read(&length, 1) < 1) return false;
			}
			do {
				if (input->Read(&length, 1) < 1) return false;
				input->Seek(length, SEEK_CUR);
			} while (length != 0x00);
		}
	}

	// Restore where we were in the file
	input->Seek(position, SEEK_SET);
	return true;
}

bool GIFLoad::ReadGIFHeader() {
	// Standard header
	unsigned char header[13];
	if (input->Read(header, 13) < 13) return false;
	if (header[0] != 'G' || header[1] != 'I' || header[2] != 'F') return false;
	global_width = header[6] + (header[7] << 8);
	global_height = header[8] + (header[9] << 8);
	
	// Creat BBitmap from global information
	BRect rect(0, 0, global_width - 1, global_height - 1);
	if (gifdecoder->colors == B_RGBA15) {
		// Don't DWORD align the BBitmap, and don't initialize to white
		gifdecoder->bitmap = new BBitmap(rect, 0, B_RGBA15, global_width * 2, B_MAIN_SCREEN_ID);
		if (gifdecoder->bitmap == NULL) return false;
		uint16 *bits = (uint16 *)gifdecoder->bitmap->Bits();
		if (bits == NULL) return false;
		int area = global_width * global_height;
		while (area > 0) {
			bits[0] = B_TRANSPARENT_MAGIC_RGBA15;
			bits++;
			area--;
		}
	} else {
		// Don't initialize to white
		gifdecoder->bitmap = new BBitmap(rect, 0, B_RGBA32, global_width * 4, B_MAIN_SCREEN_ID);
		if (gifdecoder->bitmap == NULL) return false;
		uint32 *bits = (uint32 *)gifdecoder->bitmap->Bits();
		if (bits == NULL) return false;

		uint32 *max = bits + global_width * global_height;
		while (bits < max) {
			*bits = B_TRANSPARENT_MAGIC_RGBA32;
			bits++;
		}
	}
		
	if (debug) printf("GIFLoad::ReadGIFHeader() - Global dimensions are %dx%d\n", global_width, global_height);
	
	global_palette = new Palette();
	palette = global_palette;
	// Global palette
	if (header[10] & 0x80) {
		global_palette->size_in_bits = (header[10] & 0x07) + 1;
		int s = 1 << global_palette->size_in_bits;
		global_palette->size = s;

		unsigned char *gp = new unsigned char[s * 3];
		if (input->Read(gp, s * 3) < s * 3) {
			delete [] gp;
			return false;
		}
		for (int x = 0; x < s; x++) {
			global_palette->pal[x].red = gp[x * 3];
			global_palette->pal[x].green = gp[x * 3 + 1];
			global_palette->pal[x].blue = gp[x * 3 + 2];
			global_palette->pal[x].alpha = 0xff;
		}
		delete [] gp;
		global_palette->backgroundindex = header[11];
		if (debug) printf("GIFLoad::ReadGIFHeader() - Found %d bit global palette, background is %d\n",
			global_palette->size_in_bits, global_palette->backgroundindex);
	} else { // Install BeOS system palette in case global palette isn't present
		if (debug) printf("GIFLoad::ReadGIFHeader() - No global palette found\n");
		color_map *map = (color_map *)system_colors();
		memcpy(global_palette->pal, map->color_list, sizeof(rgb_color) * 256);
		global_palette->size = 256;
		global_palette->size_in_bits = 8;
	}
	return true;
}

bool GIFLoad::ReadGIFLoopBlock() {
	unsigned char length;
	if (input->Read(&length, 1) < 1) return false;
	unsigned char type[256];
	if (input->Read(type, length) < length) return false;
	type[length] = 0x00;
	
	if (strcmp((char *)type, "NETSCAPE2.0") != 0) {
		do {
			if (input->Read(&length, 1) < 1) return false;
			input->Seek(length, SEEK_CUR);
		} while (length != 0x00);
		return true;
	}
	
	if (input->Read(&length, 1) < 1) return false;
	unsigned char data[256];
	if (input->Read(data, length + 1) < length + 1) return false;

	// Only set the loop count if we haven't seen it before
	if (loop_count == -1) {
		loop_count = data[1] + (data[2] << 8);
		// Zero means loop infinitely
		if (loop_count == 0) loop_count = 1 << 24;
		if (debug) printf("GIFLoad::ReadGIFLoopBlock() - Setting loop_count to %d\n", loop_count);
	} else if (debug) printf("GIFLoad::ReadGIFLoopBlock() - Already found loop_count\n");
	return true;
}

bool GIFLoad::ReadGIFControlBlock() {
	unsigned char data[6];
	if (input->Read(data, 6) < 6) return false;
	previous_disposal_method = disposal_method;
	disposal_method = (data[1] & 0x1c) >> 2;
	if (disposal_method >= RESTORE_PREVIOUS) disposal_method = RESTORE_BACKGROUND;
	if (debug) printf("GIFLoad::ReadGIFControlBlock() - Disposal method is %d\n", disposal_method);
	
	// Frame duration is in 100ths of second - default to 1/5th if not present
	// or set unreasonably fast
	duration = data[2] + (data[3] << 8);
	if (duration == 0) duration = 20;
	
	if (data[1] & 0x01) {
		global_palette->usetransparent = true;
		global_palette->transparentindex = data[4];
		if (debug) printf("GIFLoad::ReadGIFControlBlock() - Transparency active, using palette index %d\n", data[4]);
	}
	return true;
}

bool GIFLoad::ReadGIFCommentBlock() {
	if (debug) printf("GIFLoad::ReadGIFCommentBlock() - Found the following comment:\n");
	unsigned char length;
	char comment_data[256];
	do {
		if (input->Read(&length, 1) < 1) return false;
		if (input->Read(comment_data, length) < length) return false;
		comment_data[length] = 0x00;
		if (debug) printf("%s", comment_data);
	} while (length != 0x00);
	if (debug) printf("\n");
	return true;
}

bool GIFLoad::ReadGIFUnknownBlock(unsigned char c) {
	if (debug) printf("GIFLoad::ReadGIFUnknownBlock() - Found: %d\n", c);
	unsigned char length;
	do {
		if (input->Read(&length, 1) < 1) return false;
		input->Seek(length, SEEK_CUR);
	} while (length != 0x00);
	return true;
}

bool GIFLoad::ReadGIFImageHeader() {
	if (frame_number > 0) animated = true;
	unsigned char data[9];
	if (input->Read(data, 9) < 9) return false;

	// Has local palette
	if (data[8] & 0x80) {
		local_palette = new Palette(*global_palette);
		palette = local_palette;

		local_palette->size_in_bits = (data[8] & 0x07) + 1;
		int s = 1 << local_palette->size_in_bits;
		local_palette->size = s;
		
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Found %d bit local palette\n",
			local_palette->size_in_bits);
		
		unsigned char *gp = new unsigned char[s * 3];
		if (input->Read(gp, s * 3) < s * 3) {
			delete [] gp;
			return false;
		}
		for (int x = 0; x < s; x++) {
			local_palette->pal[x].red = gp[x * 3];
			local_palette->pal[x].green = gp[x * 3 + 1];
			local_palette->pal[x].blue = gp[x * 3 + 2];
			local_palette->pal[x].alpha = 0xff;
		}
		delete [] gp;
	}
	
	// Dispose of the previous frame after we've displayed it for the right
	// amount of time and after a new local palette may have been loaded,
	// so we clear to the correct background color
	if (!DisposeFrame()) return false;
	
	// Don't clobber these until DisposeFrame() is done with them
	xoffset = data[0] + (data[1] << 8);
	yoffset = data[2] + (data[3] << 8);
	width = data[4] + (data[5] << 8);
	height = data[6] + (data[7] << 8);
	
	// This is illegal according to the GIF spec and impossible to handle,
	// since we decode to a single bitmap of size determined by global information.
	if ((width + xoffset > global_width) || (height + yoffset > global_height)) {
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Local dimensions exceed bounds of global dimensions\n");
		return false;
	}
	if (debug) printf("GIFLoad::ReadGIFImageHeader() - Found image %dx%d with xoffset %d and yoffset %d\n",
		width, height, xoffset, yoffset);
	
	if (data[8] & 0x40) {
		interlaced = true;
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Image is interlaced\n");
	} else {
		interlaced = false;
		if (debug) printf("GIFLoad::ReadGIFImageHeader() - Image is not interlaced\n");
	}
	if (debug) printf("GIFLoad::ReadGIFImageHeader() - Done\n");
	return true;
}

// This does the cleanup after a frame, by replacing the background color if requested.
// It has to get called after the frame has been displayed for the right amount of time,
// and after the next frame's header has been read in case a new local palette comes
// along. This works, really.
bool GIFLoad::DisposeFrame() {
	// Don't cleanup on first frame
	if (frame_number == 0) return true;
	// Only set background color if this frame's dimensions are smaller than the global.
	// It would be wasted effort otherwise - I've never seen a case where this was wrong.
	if ((width >= global_width && height >= global_height) || previous_disposal_method != RESTORE_BACKGROUND) return true;
	
	if (debug) printf("GIFLoad::DisposeFrame() - Local dimensions do not match global dimensions\n");
	rgb_color color = palette->ColorForIndex(palette->backgroundindex);
	uchar *bits = (uchar *)gifdecoder->bitmap->Bits();
	if (bits == NULL) return false;

	if (gifdecoder->colors == B_RGBA32) {
		int bpr = width << 2;
		uchar *line = (uchar *)malloc(bpr);
		if (line == NULL) return false;
		for (int x = 0; x < width; x++) {
			line[x << 2] = color.blue;
			line[(x << 2) + 1] = color.green;
			line[(x << 2) + 2] = color.red;
			line[(x << 2) + 3] = color.alpha;
		}

		for (int y = yoffset; y < height + yoffset; y++) {
			memcpy(bits + ((y * global_width + xoffset) << 2), line, bpr);
		}
		free(line);
	} else {
		int bpr = width << 1;
		uint16 *line = (uint16 *)malloc(bpr);
		if (line == NULL) return false;

		uint16 data;
		if (color.alpha == B_TRANSPARENT_32_BIT.alpha && color.red == B_TRANSPARENT_32_BIT.red &&
			color.green == B_TRANSPARENT_32_BIT.green && color.blue == B_TRANSPARENT_32_BIT.blue)
				data = B_TRANSPARENT_MAGIC_RGBA15;
		else {
			data = ((color.green & 0x38) << 10) + ((color.blue & 0xf8) << 5) +
				0x80 + ((color.red & 0xf8) >> 1) + ((color.green & 0xc0) >> 6);
			B_BENDIAN_TO_HOST_INT16(data);
		}

		for (int x = 0; x < width; x++) {
			line[x] = data;
		}
		for (int y = yoffset; y < height + yoffset; y++) {
			memcpy(bits + ((y * global_width + xoffset) << 1), line, bpr);
		}
		free(line);
	}
	return true;
}

// This is only called for the first frame of an animation
bool GIFLoad::FillBackground() {
	// Only set background color if this frame's dimensions are smaller than the global.
	// It would be wasted effort otherwise - I've never seen a case where this was wrong.
	if ((width >= global_width && height >= global_height) || previous_disposal_method != RESTORE_BACKGROUND) return true;
	
	if (debug) printf("GIFLoad::FillBackground() - Local dimensions do not match global dimensions\n");
	rgb_color color = palette->ColorForIndex(palette->backgroundindex);
	uchar *bits = (uchar *)gifdecoder->bitmap->Bits();
	if (bits == NULL) return false;
	
	int global_bpr = global_width;
	if (gifdecoder->colors == B_RGBA32) {
		global_bpr <<= 2;
		uchar *line = (uchar *)malloc(global_bpr);
		if (line == NULL) return false;
		for (int x = 0; x < global_width; x++) {
			line[x << 2] = color.blue;
			line[(x << 2) + 1] = color.green;
			line[(x << 2) + 2] = color.red;
			line[(x << 2) + 3] = color.alpha;
		}
		
		for (int y = 0; y < global_height; y++) {
			memcpy(bits + (y * global_bpr), line, global_bpr);
		}
		free(line);
	} else {
		global_bpr <<= 1;
		uint16 *line = (uint16 *)malloc(global_bpr);
		if (line == NULL) return false;
		
		uint16 data;
		if (color.alpha == B_TRANSPARENT_32_BIT.alpha && color.red == B_TRANSPARENT_32_BIT.red &&
			color.green == B_TRANSPARENT_32_BIT.green && color.blue == B_TRANSPARENT_32_BIT.blue)
				data = B_TRANSPARENT_MAGIC_RGBA15;
		else {
			data = ((color.green & 0x38) << 10) + ((color.blue & 0xf8) << 5) +
				0x80 + ((color.red & 0xf8) >> 1) + ((color.green & 0xc0) >> 6);
			B_BENDIAN_TO_HOST_INT16(data);
		}
		
		for (int x = 0; x < global_width; x++) {
			line[x] = data;
		}
		for (int y = 0; y < global_height; y++) {
			memcpy(bits + (y * global_bpr), line, global_bpr);
		}
		free(line);
	}
	return true;
}

// Grab the next LZW code and top off the byte_buffer if necessary
short GIFLoad::NextCode() {
	while (bit_count < BITS) {
		if (byte_count == 0) {
			if (input->Read(&byte_count, 1) < 1) {
				if (debug) printf("GIFLoad::NextCode() - Couldn't read byte_count length\n");
				return -1;
			}
			if (byte_count == 0) return end_code;
			if (input->Read(byte_buffer + (255 - byte_count), byte_count) < byte_count) {
				if (debug) printf("GIFLoad::NextCode() - Couldn't read byte_count data, %d bytes\n", byte_count);
				return -1;
			}
		}
		bit_buffer |= (unsigned int)byte_buffer[255 - byte_count] << bit_count;
		byte_count--;
		bit_count += 8;
	}

	short s = bit_buffer & ((1 << BITS) - 1);
	bit_buffer >>= BITS;
	bit_count -= BITS;
	return s;
}

// Done whenever a clear code is encountered
void GIFLoad::ResetTable() {
	BITS = code_size + 1;
	next_code = clear_code + 2;
	max_code = (1 << BITS) - 1;
	
	MemblockDeleteAll();
	for (int x = 0; x < 4096; x++) {
		table[x] = NULL;
		if (x < (1 << code_size)) {
			table[x] = MemblockAllocate(1);
			table[x][0] = x;
			entry_size[x] = 1;
		}
	}
}

bool GIFLoad::InitFrame(int size) {
	code_size = size;
	if (code_size == 1) code_size++;
	BITS = code_size + 1;
	clear_code = 1 << code_size;
	end_code = clear_code + 1;
	next_code = clear_code + 2;
	max_code = (1 << BITS) - 1;
	pass = 0;
	if (interlaced) row = gl_pass_starts_at[0];
	else row = 0;
	
	bit_count = 0;
	bit_buffer = 0;
	byte_count = 0;
	old_code_length = 0;
	new_code = 0;
	if (gifdecoder->colors == B_RGBA32) bitmap_position = xoffset << 2;
	else bitmap_position = xoffset << 1;
	
	if (frame_number == 0) {
		if (!FillBackground()) return false;
	}
	
	ResetTable();
	return true;
}

bool GIFLoad::ReadGIFImageData() {
	unsigned char new_entry[4096];
	
	// Code size sometimes differs from what it should be, when the compression
	// is not as aggressive as the palette->size_in_bits allows.
	unsigned char cs;
	if (input->Read(&cs, 1) < 1) goto bad_end;
	if (cs == palette->size_in_bits) {
		if (debug) printf("GIFLoad::ReadGIFImageData() - Code_size is correct at %d\n", palette->size_in_bits);
		if (!InitFrame(palette->size_in_bits)) goto bad_end;
	} else if (cs > palette->size_in_bits && cs <= 8) {
		if (debug) printf("GIFLoad::ReadGIFImageData() - Code_size should be %d, but was in fact %d, allowing it\n", palette->size_in_bits, cs);
		if (!InitFrame(cs)) goto bad_end;
	} else {
		if (debug) printf("GIFLoad::ReadGIFImageData() - Code_size should be %d, but was in fact %d\n", palette->size_in_bits, cs);
		goto bad_end;
	}
	
	if (debug) printf("GIFLoad::ReadGIFImageData() - Starting LZW\n");
	
	while ((new_code = NextCode()) != -1 && new_code != end_code) {
		if (new_code == clear_code) {
			ResetTable();
			new_code = NextCode();
			old_code[0] = new_code;
			old_code_length = 1;
			if (!OutputColor(old_code, 1)) goto bad_end;
			if (new_code == -1 || new_code == end_code) {
				if (debug) printf("GIFLoad::ReadGIFImageData() - Premature end_code or error\n");
				goto bad_end;
			}
			continue;
		}
		
		// Explicitly check for lack of clear code at start of file
		if (old_code_length == 0) {
			old_code[0] = new_code;
			old_code_length = 1;
			if (!OutputColor(old_code, 1)) goto bad_end;
			continue;
		}
		
		if (table[new_code] != NULL) { // Does exist in table
			if (!OutputColor(table[new_code], entry_size[new_code])) goto bad_end;
			memcpy(new_entry, old_code, old_code_length);
			memcpy(new_entry + old_code_length, table[new_code], 1);
		} else { // Does not exist in table
			memcpy(new_entry, old_code, old_code_length);
			memcpy(new_entry + old_code_length, old_code, 1);
			if (!OutputColor(new_entry, old_code_length + 1)) goto bad_end;
		}
		table[next_code] = MemblockAllocate(old_code_length + 1);
		
		memcpy(table[next_code], new_entry, old_code_length + 1);
		entry_size[next_code] = old_code_length + 1;
		memcpy(old_code, table[new_code], entry_size[new_code]);
		old_code_length = entry_size[new_code];
		next_code++;
		
		if (next_code > max_code && BITS != 12) {
			BITS++;
			max_code = (1 << BITS) - 1;
		}
	}
	
	if (debug) printf("GIFLoad::ReadGIFImageData() - Done\n");
	MemblockDeleteAll();
	if (local_palette != NULL) {
		palette = global_palette;
		delete local_palette;
		local_palette = NULL;
	}

	// Comment this out so that we display whatever was there, even if
	// the file was corrupted or ends abruptly
	//if (new_code == -1) return false;
	return true;
	
bad_end:
	if (debug) printf("GIFLoad::ReadGIFImageData() - Arrived at a bad end\n");
	MemblockDeleteAll();
	if (local_palette != NULL) {
		palette = global_palette;
		delete local_palette;
		local_palette = NULL;
	}
	return false;
}

GIFLoad::~GIFLoad() {
	delete global_palette;
	if (local_palette != NULL) delete local_palette;
}

// Do 4k mallocs, keep them in a linked list, do a first fit across them
// when a new request comes along
uchar *GIFLoad::MemblockAllocate(int size) {
	if (head_memblock == NULL) {
		head_memblock = new memblock;
		head_memblock->data = (uchar *)malloc(4096);
		if (head_memblock->data == NULL) {
			delete head_memblock;
			head_memblock = NULL;
			return NULL;
		}

		uchar *value = head_memblock->data;
		head_memblock->offset = size;
		head_memblock->next = NULL;
		return value;
	} else {
		memblock *block = head_memblock;
		memblock *last = NULL;
		while (block != NULL) {
			if (4096 - block->offset > size) {
				uchar *value = block->data + block->offset;
				block->offset += size;
				return value;
			}
			last = block;
			block = block->next;
		}

		block = new memblock;
		block->data = (uchar *)malloc(4096);
		if (block->data == NULL) {
			delete block;
			return NULL;
		}

		uchar *value = block->data;
		block->offset = size;
		block->next = NULL;
		last->next = block;
		return value;
	}
}

// Delete the linked list
void GIFLoad::MemblockDeleteAll() {
	memblock *block = NULL;
	while (head_memblock != NULL) {
		block = head_memblock->next;
		free(head_memblock->data);
		delete head_memblock;
		head_memblock = block;
	}
}