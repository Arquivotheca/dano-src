//--------------------------------------------------------------------
//	
//	DrawEngine.cpp
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include "DrawEngine.h"
#include "LBXFormat0.h"
#include "LBXFormat1.h"
#include <stdlib.h>

#if ENABLE_DRAWING
/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_DrawEngine::LBX_DrawEngine(LBX_Container *ctxt, uint32 bitmap_index) :
	LBX_Unpack(ctxt->CmapBuffer(bitmap_index)) {	
	stepper = ctxt->Stepper(bitmap_index);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_DrawEngine::~LBX_DrawEngine() {
	delete stepper;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_DrawEngine::DrawFirstLine(uint32 *dst, uint32 left2, uint32 top, uint32 right2) {
	left = left2;
	right = right2;
	stepper->SeekToLine(top, ptr);
	ExtractTo32(ptr[0], ptr[1], ptr[2], dst, left, right);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_DrawEngine::DrawNextLine(uint32 *dst) {
	stepper->NextLine(ptr);
	ExtractTo32(ptr[0], ptr[1], ptr[2], dst, left, right);
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
void LBX_DrawEngine::DrawRect(uint16 *dst, uint32 row,
							   uint32 left2, uint32 top, uint32 right2, uint32 bottom) {
	left = left2;
	right = right2;
	stepper->SeekToLine(top, ptr);
	DrawTo16(ptr[0], ptr[1], ptr[2], dst, left, right);
	row >>= 1;
	dst += row; 
	for (top++; top<bottom; top++) {
		stepper->NextLine(ptr);
		DrawTo16(ptr[0], ptr[1], ptr[2], dst, left, right);
		dst += row; 
	}
}
#endif

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Container *LBX_BuildContainer(uint8 *buffer) {
	switch (((LBX_header*)buffer)->status & 0xf0) {
#if ENABLE_REVISION_0
	case 0x00 :
		return new LBX_Container_0(buffer);
#endif
#if ENABLE_REVISION_1
	case 0x10 :
		return new LBX_Container_1(buffer);
#endif
	default:
		return NULL;
	}		
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Container::LBX_Container(uint8 *buffer) {
	status = ((LBX_header*)buffer)->status;
	bitmap_count = ((LBX_header*)buffer)->bitmap_count;
}

/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Container::~LBX_Container() {
}

#if ENABLE_DRAWING
/*--------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------*/
LBX_Stepper::~LBX_Stepper() {
}
#endif
