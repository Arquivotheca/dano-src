//******************************************************************************
//
//	File:			GIFLoad.h
//
//	Copyright 1999, Be Incorporated, All Rights Reserved.
//  Copyright 1998-1999, Daniel Switkin
//
//******************************************************************************

#ifndef GIFLOAD_H
#define GIFLOAD_H

#include <DataIO.h>
#include <Bitmap.h>
#include "Palette.h"
#include "GIFDecoder.h"
#include <stdio.h>

typedef struct memblock {
	uchar *data;
	int offset;
	memblock *next;
} memblock;

const int gl_pass_starts_at[] = {0, 4, 2, 1};
const int gl_increment_pass_by[] = {8, 8, 4, 2};

enum {
	NO_DISPOSAL = 0,
	LEAVE_PREVIOUS = 1,
	RESTORE_BACKGROUND = 2,
	RESTORE_PREVIOUS = 3
};

class GIFLoad {
	friend class GIFDecoder;

	public:
		GIFLoad(BPositionIO *input, GIFDecoder *gifdecoder);
		bool DecodeNextFrame();
		bool IsTransparent() const;
		bool NeedsBackgroundRepainted() const;
		~GIFLoad();

		bool fatalerror;
		
	private:
		void Init();
		
		bool ReadGIFHeader();
		bool ReadGIFLoopBlock();
		bool ReadGIFControlBlock();
		bool ReadGIFCommentBlock();
		bool ReadGIFUnknownBlock(unsigned char c);
		bool ReadGIFImageHeader();
		bool ReadGIFImageData();
		
		bool SeekToBeginning();
		bool LookAheadForAnimated();
		bool FillBackground();
		bool DisposeFrame();
		bool InitFrame(int size);
		short NextCode();
		void ResetTable();
		
		uchar *MemblockAllocate(int size);
		void MemblockDeleteAll();

		inline bool OutputColor(unsigned char *string, int size) {
			uchar *bits = (uchar *)gifdecoder->bitmap->Bits();
			if (bits == NULL) return false;
			
			if (gifdecoder->colors == B_RGBA32) {
				int bpr = width << 2;
				int global_bpr = global_width << 2;
			
				for (int x = 0; x < size; x++) {
					rgb_color color = palette->ColorForIndex(string[x]);
					if (color.alpha != B_TRANSPARENT_32_BIT.alpha ||
						color.red != B_TRANSPARENT_32_BIT.red ||
						color.green != B_TRANSPARENT_32_BIT.green ||
						color.blue != B_TRANSPARENT_32_BIT.blue) {
						
						int offset = (row + yoffset) * global_bpr + bitmap_position;
						bits[offset] = color.blue;
						bits[offset + 1] = color.green;
						bits[offset + 2] = color.red;
						bits[offset + 3] = color.alpha;
					} else if (frame_number == 0 || disposal_method == RESTORE_BACKGROUND) {
						uint32 *alpha_pointer = (uint32 *)(bits + (row + yoffset) * global_bpr + bitmap_position);
						alpha_pointer[0] = B_TRANSPARENT_MAGIC_RGBA32;
					}
					bitmap_position += 4;
					
					if (bitmap_position - (xoffset << 2) >= bpr) {
						if (interlaced) {
							row += gl_increment_pass_by[pass];
							while (row >= height) {
								pass++;
								row = gl_pass_starts_at[pass];
							}
						} else row++;
						bitmap_position = xoffset << 2;
					}
				}
			} else { // B_RGBA15
				int bpr = width << 1;
				int global_bpr = global_width << 1;
			
				for (int x = 0; x < size; x++) {
					rgb_color color = palette->ColorForIndex(string[x]);
					if (color.alpha != B_TRANSPARENT_32_BIT.alpha ||
						color.red != B_TRANSPARENT_32_BIT.red ||
						color.green != B_TRANSPARENT_32_BIT.green ||
						color.blue != B_TRANSPARENT_32_BIT.blue) {
						
						int offset = (row + yoffset) * global_bpr + bitmap_position;
						bits[offset] = ((color.green & 0x38) << 2) + ((color.blue & 0xf8) >> 3);
						bits[offset + 1] = 0x80 + ((color.red & 0xf8) >> 1) + ((color.green & 0xc0) >> 6);
					} else if (frame_number == 0 || disposal_method == RESTORE_BACKGROUND) {
						uint16 *alpha_pointer = (uint16 *)(bits + (row + yoffset) * global_bpr + bitmap_position);
						alpha_pointer[0] = B_TRANSPARENT_MAGIC_RGBA15;
					}
					bitmap_position += 2;
					
					if (bitmap_position - (xoffset << 1) >= bpr) {
						if (interlaced) {
							row += gl_increment_pass_by[pass];
							while (row >= height) {
								pass++;
								row = gl_pass_starts_at[pass];
							}
						} else row++;
						bitmap_position = xoffset << 1;
					}
				}
			}
			return true;
		}
		
		BPositionIO *input;
		GIFDecoder *gifdecoder;
		int bitmap_position;
		Palette *palette, *global_palette, *local_palette;
		
		int global_width, global_height;
		int pass, row, width, height, xoffset, yoffset;
		bool interlaced, animated;
		short duration;
		int frame_number, loop_count;
		int disposal_method, previous_disposal_method;
		
		unsigned char old_code[4096];
		int old_code_length;
		short new_code, clear_code, end_code, next_code;
		int BITS, max_code, code_size;
		
		unsigned char *table[4096];
		short entry_size[4096];
		memblock *head_memblock;
		
		int bit_count;
		unsigned int bit_buffer;
		unsigned char byte_count;
		unsigned char byte_buffer[255];
		bool first_time;
};

#endif
