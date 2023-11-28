//--------------------------------------------------------------------
//	
//	LBXFormat1.cpp
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include "LBXFormat1.h"
#include <stdlib.h>
#include <string.h>

#if ENABLE_REVISION_1
// Predefined suffix strings
static char suffix_buffer[] = "_disabled.png\0_down.png\0_over.png\0_up.png\0.png\0.gif";
static uint8 suffix_offset[6] = { 0, 14, 24, 34, 42, 47 };

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Container_1::LBX_Container_1(uint8 *buffer) : LBX_Container(buffer) {
	int32		i, word_count;
	uint16		hash_key;
	char		*cur_name, *name;
	char		tmp_buffer[256];

	// gather useful information
	bitmap_list = (LBX_bitmap_header1*)(buffer+sizeof(LBX_header1));
	name_buffer = (char*)(bitmap_list+bitmap_count);
	name_offset = (uint16*)malloc(sizeof(uint16)*2*bitmap_count);
	cur_name = name_buffer;
	compressed_names = ((((LBX_header1*)buffer)->status&2) != 0);
	// identify and hash compressed names
	if (compressed_names) {
		word_count = ((LBX_header1*)buffer)->scanline_length>>24;
		word_offset = (uint16*)malloc(sizeof(uint16)*word_count);
		for (i=0; i<bitmap_count; i++) {
			name_offset[i*2] = cur_name-name_buffer;
			while (((uint8*)cur_name)[0] >= 32) cur_name++;
			cur_name++;
		}
		for (i=0; i<word_count; i++) {
			word_offset[i] = cur_name-name_buffer;
			while (*cur_name != 0) cur_name++;
			cur_name++;
		}
		for (i=0; i<bitmap_count; i++) {
			name = tmp_buffer;
			GetBitmapName(i, name);
			hash_key = 0;
			while (*name != 0)
				hash_key = ((hash_key>>13) | (hash_key<<3)) ^ *name++;
			name_offset[i*2+1] = hash_key;
		}
	}
	// identify and hash uncompressed names
	else {
		word_offset = NULL;
		for (i=0; i<bitmap_count; i++) {
			name_offset[i*2] = cur_name-name_buffer;
			hash_key = 0;
			while (*cur_name != 0)
				hash_key = ((hash_key>>13) | (hash_key<<3)) ^ *cur_name++;
			name_offset[i*2+1] = hash_key;
			cur_name++;
		}
	}
#if ENABLE_DRAWING
	// initialise pointers to cmap, scanline instructions and pixels
	cmap_list = (uint16*)cur_name;
	scanbuffer = (uint8*)(cmap_list+((LBX_header1*)buffer)->cmap_count);
	pixel_buffer = scanbuffer+(((LBX_header1*)buffer)->scanline_length & 0xffffff);
#endif
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Container_1::~LBX_Container_1() {
	free(name_offset);
	if (word_offset)
		free(word_offset);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
int32 LBX_Container_1::GetIndexFromName(char *name) {
	int32		i;
	char		*str;
	char		name_buffer[256];
	uint16		hash_key;
	
	// calculate hash key for the name we are looking for
	str = name;
	hash_key = 0;
	while (*str != 0)
		hash_key = ((hash_key>>13) | (hash_key<<3)) ^ *str++;
	// find an entry whose name match the key and match the string
	for (i=0; i<bitmap_count; i++)
		if (name_offset[2*i+1] == hash_key) {
			if (compressed_names) {	
				str = name_buffer;
				GetBitmapName(i, str);
			}
			else
				str = name_buffer+name_offset[2*i];
 			if (strcmp(str, name) == 0)
				return i;
		}
	// found none
	return -1;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Container_1::GetBitmapName(uint32 index, char *name) {
	char		*word;
	int32		i, len;
	uint8		*str;

	if (compressed_names) {	
		str = (uint8*)name_buffer+name_offset[index*2];
		while (str[0] >= 32) {
			word = name_buffer+word_offset[str[0]-32];
			len = strlen(word);
			for (i=0; i<len; i++)
				*name++ = word[i];
			str++;
			if (str[0] >= 32)
				*name++ = '_';
		}
		strcpy(name, suffix_buffer+suffix_offset[str[0]-1]);
	}
	else
		strcpy(name, name_buffer+name_offset[index*2]);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
uint16 LBX_Container_1::BitmapWidth(uint32 index) {
	return bitmap_list[index].width;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
uint16 LBX_Container_1::BitmapHeight(uint32 index) {
	return bitmap_list[index].height;
}

#if ENABLE_DRAWING
/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
uint16 *LBX_Container_1::CmapBuffer(uint32 index) {
	return cmap_list + bitmap_list[index].cmap_index;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Stepper	*LBX_Container_1::Stepper(uint32 bitmap_index) {
	return (LBX_Stepper*)(new LBX_Stepper_1(bitmap_index, this));
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
uint32 LBX_Container_1::BitmapIndex(uint32 scanline_index, uint32 i) {
	if (bitmap_list[i].scan_index > scanline_index) {
		do i--;
		while (bitmap_list[i].scan_index > scanline_index); 
		return i;
	}
	else {
		do i++;
		while ((i<bitmap_count) && (bitmap_list[i].scan_index <= scanline_index));
		return i-1;
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
static uint8 LBX_Stepper_1_ParseTable[32] = {
	1,	1,	1,	1,		1,	1,	1,	1,
	1,	1,	1,	1,		3,	3,	3,	3,
	0,	0,	0,	2,		0,	0,	0,	0,
	5,	5,	5,	6,		4,	4,	4,	4
};

uint8 *InstrInfos::ParseInstr(uint8 *instr) {
	uint8		type;
	
	type = LBX_Stepper_1_ParseTable[instr[0]>>3];
	current = 0;
	if (type < 4) {
		if (type == 0) {
			// short encoding : 101[n5][lg8]xN
			count = (instr[0]&31)+1;
			step = (instr[0]>>5)&1;
			param_list = instr+1;
			command = INSTR_SCANLIST8;
			return instr+(2+(count-1)*step);
		}
		else if (type == 1) {
			// short encoding 0[step2][n5][ln8]
			count = (instr[0]&31)+1;
			step = (instr[0]>>5)&3;
			param = instr[1];
			command = INSTR_SCANCOPY;
			return instr+2;
		}
		else if (type == 2) {
			// short encoding : 10011[n11][lg8]xN
			count = ((uint16)((instr[0]&7)<<8)+instr[1])+1;
			step = 1;
			param_list = instr+2;
			command = INSTR_SCANLIST8;
			return instr+(2+count);
		}
		else {
			// short encoding 011[bm5]
			count = 0;
			param = instr[0]&31;
			command = INSTR_BITMAP;
			return instr+1;
		}
	}
	else if (type == 4) {
		// long encoding : 111010[n10][lg16]xN
		count = (((uint16)(instr[0]&3)<<8)|instr[1])+1;
		step = (instr[0]>>2)&7;
		param_list = instr+2;
		command = INSTR_SCANLIST16;
		return instr+(4+(count-1)*step);
	}
	else if (type == 5) {
		// long encoding 110[step2][ln11][n8]
		count = ((uint16)instr[2])+1;
		step = (instr[0]>>3)&3;
		param = ((uint16)(instr[0]&7)<<8)|instr[1];
		command = INSTR_SCANCOPY;
		return instr+3;
	}
	else {
		// long encoding 11011xxx[bm8]
		count = 0;
		param= instr[1];
		command = INSTR_BITMAP;
		return instr+2;
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
InstrCtxt::InstrCtxt(LBX_Container_1 *container2, uint32 bitmap_index, bool master) {
	if (master)
		for (int i=0; i<2; i++)
			simi_ctxt[i] = new InstrCtxt(container2, bitmap_index, false);
	else
		simi_ctxt[0] = NULL;
	init_bitmap = bitmap_index;
	container = container2;
	scan_index = 100000000;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
InstrCtxt::~InstrCtxt() {
	if (simi_ctxt[0])
		for (int i=0; i<2; i++)
			delete simi_ctxt[i];
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void InstrCtxt::Reset(uint32 bitmap_index) {
	LBX_bitmap_header1	*bitmap;

// pointer on the source bitmap
	bitmap = container->bitmap_list+bitmap_index;
// keep a copy of the bitmap where the reset is done
	init_bitmap = bitmap_index;
// pointer to the first non duplicated scanline of the bitmap
	pixel = container->pixel_buffer + bitmap->pixel_offset;
// global index of the first scanline of the bitmap
	scan_index = bitmap->scan_index;
// set the max index for the bitmap
	if (bitmap_index < container->BitmapCount()-1)
		max_index = bitmap[1].scan_index;
	else
		max_index = 10000000;
// parse the first instruction
	cur_instr = scan0.ParseInstr(container->scanbuffer + bitmap->scan_offset);
// Handle master context only init
	if (simi_ctxt[0]) {
	// secondary bitmap index needed only for master context
		secondary_bitmap_index = bitmap_index;
	// Init similarity count, offset copy and invalidate context
		if (bitmap->similar_delta[0] != 0) {
			similarity[0] = bitmap->similar_delta[0];
			simi_ctxt[0]->scan_index = 100000000;
			if (bitmap->similar_delta[1] != 0) {
				similarity[1] = bitmap->similar_delta[1];
				simi_ctxt[1]->scan_index = 100000000;
				similarity_count = 2;
			}
			else
				similarity_count = 1;
		}
		else similarity_count = 0;
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void InstrCtxt::SeekTo(uint32 index, InstrCtxt *copy_ctxt) {
	uint32		i;
	int32		new_scan;

// Nothing to do
	if ((int32)index == scan_index)
		goto check_similarity;
// Need to jump back first
	if (((int32)index < scan_index) || ((int32)index >= max_index))
		Reset(container->BitmapIndex(index, init_bitmap));
// move forward, one instruction at a time, until we reach the right instruction
	index -= scan_index;
	while (index >= (uint32)(scan0.count-scan0.current)) {
		// adjust scan_index
		index -= (scan0.count-scan0.current);
		scan_index += (scan0.count-scan0.current);
		// adjust pixel when needed
		if (scan0.command == INSTR_SCANLIST8)
			for (i=scan0.current; i<scan0.count; i++) {
				pixel += 1+(uint32)*scan0.param_list;
				scan0.param_list += scan0.step;
			}
		else if (scan0.command == INSTR_SCANLIST16) {
			for (i=scan0.current; i<scan0.count; i++) {
				pixel += 1+(uint32)*((uint16*)scan0.param_list);
				scan0.param_list += scan0.step;
			}
		}
		// adjust the secondary bitmap pointer as needed
		else if (scan0.command == INSTR_BITMAP)
			secondary_bitmap_index = scan0.param;
		// read next instruction
		cur_instr = scan0.ParseInstr(cur_instr);
	}
// once at the right instruction, do a partial move
	// adjust scan_index
	scan_index += index;
	// adjust pixel when needed
	if (scan0.command == INSTR_SCANLIST8) {
		for (i=0; i<index; i++) {
			pixel += 1+(uint32)*scan0.param_list;
			scan0.param_list += scan0.step;
		}
	}
	else if (scan0.command == INSTR_SCANLIST16) {
		for (i=0; i<index; i++) {
			pixel += 1+(uint32)*((uint16*)scan0.param_list);
			scan0.param_list += scan0.step;
		}
	}	
	else if ((scan0.command == INSTR_SCANCOPY) && copy_ctxt) {
		copy_ctxt->init_bitmap = secondary_bitmap_index;
		copy_ctxt->SeekTo(container->bitmap_list[secondary_bitmap_index].scan_index +
						  scan0.param + ((int32)scan0.step-1)*index, NULL);
	}
	// adjust current
	scan0.current += index;
// if it's a master context, also seek both simi_ctxt
check_similarity:
	if (simi_ctxt[0]) {
		for (i=0; i<similarity_count; i++) {
			new_scan = scan_index - similarity[i];
			if (new_scan >= 0)
				simi_ctxt[i]->SeekTo(new_scan, NULL);
			else
				simi_ctxt[i]->scan_index = new_scan;
		}
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void InstrCtxt::ForwardStep(InstrCtxt *copy_ctxt) {
// update pixel pointer if needed
	if (scan0.command == INSTR_SCANLIST8)
		pixel += 1+(uint32)*scan0.param_list;
	else if (scan0.command == INSTR_SCANLIST16)
		pixel += 1+(uint32)*((uint16*)scan0.param_list);
	scan0.param_list += scan0.step;
// update count of remaining step if needed
	scan0.current++;
// update global scan_index
	scan_index++;
	if (scan_index == max_index)
		Reset(init_bitmap+1);
// execute more instruction if needed
	while (scan0.current == scan0.count) {
		cur_instr = scan0.ParseInstr(cur_instr);
		if (copy_ctxt) {
			if (scan0.command == INSTR_BITMAP)
				secondary_bitmap_index = scan0.param;
			else if (scan0.command == INSTR_SCANCOPY) {
				copy_ctxt->init_bitmap = secondary_bitmap_index;
				copy_ctxt->SeekTo(container->bitmap_list[secondary_bitmap_index].scan_index +
						 		 scan0.param, NULL);
			}
		}
	}
// Forward Step simi if needed
	if (simi_ctxt[0]) {
		for (uint32 i=0; i<similarity_count; i++) {
			if (simi_ctxt[i]->scan_index >= 0)
				simi_ctxt[i]->ForwardStep(NULL);
			else if (simi_ctxt[i]->scan_index == -1)
				simi_ctxt[i]->Reset(0);
			else
				simi_ctxt[i]->scan_index++;
		}
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void InstrCtxt::BackStep(InstrCtxt *copy_ctxt) {
// use a seek if we arrived at the beginning of the current instruction
	if (scan0.current == 0) {
		SeekTo(scan_index-1, copy_ctxt);
		return;
	}
// update global scan_index
	scan_index--;
// update pixel pointer if needed
	scan0.param_list -= scan0.step;
	if (scan0.command == INSTR_SCANLIST8)
		pixel -= 1+(uint32)*scan0.param_list;
	else if (scan0.command == INSTR_SCANLIST16)
		pixel -= 1+(uint32)*((uint16*)scan0.param_list);
// update count of remaining step if needed
	scan0.current--;
// update simi if needed
	if (simi_ctxt[0]) {
		for (uint32 i=0; i<similarity_count; i++) {
			if (simi_ctxt[i]->scan_index > 0)
				simi_ctxt[i]->BackStep(NULL);
			else
				simi_ctxt[i]->scan_index--;
		}
	}
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Stepper_1::LBX_Stepper_1(uint32 bitmap_index, LBX_Container_1 *container) {
	main_ctxt = new InstrCtxt(container, bitmap_index, true);
	copy_ctxt = new InstrCtxt(container, bitmap_index, true);
	offset = container->bitmap_list[bitmap_index].scan_index;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Stepper_1::~LBX_Stepper_1() {
	delete main_ctxt;
	delete copy_ctxt;
}
	
/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Stepper_1::SeekToLine(uint32 index, uint8 **ptr) {
// seek forward to the index scanline
	main_ctxt->SeekTo(index+offset, copy_ctxt);
	SetPtr(ptr);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Stepper_1::NextLine(uint8 **ptr) {
// if we are going through a scancopy...
	if (main_ctxt->scan0.command == INSTR_SCANCOPY) {
	// Negative step
		if (main_ctxt->scan0.step == 0)
			copy_ctxt->BackStep(NULL);
	// Positive step
		else if (main_ctxt->scan0.step == 2)
			copy_ctxt->ForwardStep(NULL);
	}
// one step forward on the main instruction
	main_ctxt->ForwardStep(copy_ctxt);
	SetPtr(ptr);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Stepper_1::SetPtr(uint8 **ptr) {
	if (main_ctxt->scan0.command == INSTR_SCANCOPY) {
		ptr[0] = copy_ctxt->pixel;
		ptr[1] = copy_ctxt->simi_ctxt[0]->pixel;
		ptr[2] = copy_ctxt->simi_ctxt[1]->pixel;
	}
	else {
		ptr[0] = main_ctxt->pixel;
		ptr[1] = main_ctxt->simi_ctxt[0]->pixel;
		ptr[2] = main_ctxt->simi_ctxt[1]->pixel;
	}
}
#endif

#endif
