//--------------------------------------------------------------------
//	
//	LBXFormat0.cpp
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include "LBXFormat0.h"
#include <stdlib.h>
#include <string.h>

#if ENABLE_REVISION_0
/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Container_0::LBX_Container_0(uint8 *buffer) : LBX_Container(buffer) {
	int32		i;
	uint16		hash_key;
	char		*cur_name;

#if ENABLE_DRAWING
	// calculate scanline multipler and mask
	if (status & 0x02) {
		scanline_multiplier = 3;
		scanline_mask = 0xffffff;
	}
	else {
		scanline_multiplier = 2;
		scanline_mask = 0xffff;
	}
#endif
	// if it's revision 0, gather useful information
	bitmap_list = (LBX_bitmap_header0*)(buffer+sizeof(LBX_header0));
	name_buffer = (char*)(bitmap_list+bitmap_count);
	name_offset = (uint16*)malloc(sizeof(uint16)*2*bitmap_count);
	cur_name = name_buffer;
	for (i=0; i<bitmap_count; i++) {
		name_offset[i*2] = cur_name-name_buffer;
		hash_key = 0;
		while (*cur_name != 0)
			hash_key = ((hash_key>>13) | (hash_key<<3)) ^ *cur_name++;
		name_offset[i*2+1] = hash_key;
		cur_name++;
	}
#if ENABLE_DRAWING
	cmap_list = (uint16*)cur_name;
	scanbitmap_list = (uint8*)(cmap_list+((LBX_header0*)buffer)->cmap_count);
	scanline_list = (uint8*)(scanbitmap_list+((LBX_header0*)buffer)->scanline_count);
	pixel_buffer = (uint8*)(scanline_list+scanline_multiplier*((LBX_header0*)buffer)->scanline_count);
#endif
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Container_0::~LBX_Container_0() {
	free(name_offset);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
int32 LBX_Container_0::GetIndexFromName(char *name) {
	int32		i;
	char		*str;
	uint16		hash_key;
	
	// calculate hash key for the name we are looking for
	str = name;
	hash_key = 0;
	while (*str != 0)
		hash_key = ((hash_key>>13) | (hash_key<<3)) ^ *str++;
	// find an entry whose name match the key and match the string
	for (i=0; i<bitmap_count; i++)
		if (name_offset[2*i+1] == hash_key)
			if (strcmp(name_buffer+name_offset[2*i], name) == 0)
				return i;
	// found none
	return -1;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Container_0::GetBitmapName(uint32 index, char *name) {
	strcpy(name, name_buffer+name_offset[index*2]);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
uint16 LBX_Container_0::BitmapWidth(uint32 index) {
	return bitmap_list[index].width;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
uint16 LBX_Container_0::BitmapHeight(uint32 index) {
	return bitmap_list[index].height;
}

#if ENABLE_DRAWING
/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
uint16 *LBX_Container_0::CmapBuffer(uint32 index) {
	return cmap_list + bitmap_list[index].cmap_index;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Stepper	*LBX_Container_0::Stepper(uint32 bitmap_index) {
	return (LBX_Stepper*)(new LBX_Stepper_0(bitmap_index, this));
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Stepper_0::LBX_Stepper_0(uint32 bitmap_index, LBX_Container_0 *context) {
	ctxt = context;
	step = ctxt->scanline_multiplier;
	pixel_offset = ctxt->pixel_buffer;
	similar_delta[0] = ctxt->bitmap_list[bitmap_index].similar_delta[0];
	similar_delta[1] = ctxt->bitmap_list[bitmap_index].similar_delta[1];
	scanline_mask = ctxt->scanline_mask;
	scan_index = ctxt->bitmap_list[bitmap_index].scan_index;	
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Stepper_0::~LBX_Stepper_0() {
}
	
/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Stepper_0::SeekToLine(uint32 index, uint8 **ptr) {
	scanbitmap = ctxt->scanbitmap_list + (scan_index + index);
	scanline = ctxt->scanline_list + step*(scan_index + index);
	InitLineState();
	ptr[0] = pixel_offset+(((uint32*)scanline0)[0]&scanline_mask);
	ptr[1] = pixel_offset+(((uint32*)scanline1)[0]&scanline_mask);
	ptr[2] = pixel_offset+(((uint32*)scanline2)[0]&scanline_mask);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Stepper_0::NextLine(uint8 **ptr) {
	scanbitmap++;
	scanline += step;

	if ((scanbitmap[0] == scanbitmap[-1]) &&
		((scanbitmap[0] == 0xff) || (((uint32*)scanline)[0] == ((uint32*)(scanline-step))[0]+1))) {
		scanline0 += step;
		scanline1 += step;
		scanline2 += step;
	}
	else
		InitLineState();
	ptr[0] = pixel_offset+(((uint32*)scanline0)[0]&scanline_mask);
	ptr[1] = pixel_offset+(((uint32*)scanline1)[0]&scanline_mask);
	ptr[2] = pixel_offset+(((uint32*)scanline2)[0]&scanline_mask);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_Stepper_0::InitLineState() {
	uint32		bitmap_index, index;

	bitmap_index = scanbitmap[0];
	if (bitmap_index == 0xff) {
		scanline0 = scanline;
		scanline1 = scanline - step*similar_delta[0];
		scanline2 = scanline - step*similar_delta[1];
	}
	else {
		index = ctxt->bitmap_list[bitmap_index].scan_index + (((uint32*)scanline)[0]&scanline_mask);
		scanline0 = ctxt->scanline_list + step*index;
		scanline1 = scanline0 - step*ctxt->bitmap_list[bitmap_index].similar_delta[0];
		scanline2 = scanline0 - step*ctxt->bitmap_list[bitmap_index].similar_delta[1];
	}
}
#endif

#endif
