//--------------------------------------------------------------------
//	
//	LBXFormat0.h
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _LBXFORMAT0_H_
#define _LBXFORMAT0_H_

#include "DrawEngine.h"
#include "Unpack.h"

class LBX_Container_0 : public LBX_Container {
friend class LBX_Stepper_0;
public:
	LBX_Container_0(uint8 *buffer);
	~LBX_Container_0();

	virtual void		GetBitmapName(uint32 index, char *name);
	virtual uint16		BitmapWidth(uint32 index);
	virtual uint16		BitmapHeight(uint32 index);
	virtual int32		GetIndexFromName(char *name); // return -1 if unknown
#if ENABLE_DRAWING
	virtual LBX_Stepper	*Stepper(uint32 bitmap_index);

private:
	virtual uint16		*CmapBuffer(uint32 index);
#endif
	
private:
	LBX_bitmap_header0	*bitmap_list;
	char				*name_buffer;
	uint16				*name_offset;
#if ENABLE_DRAWING
	int32				scanline_multiplier;
	uint32				scanline_mask;
	uint8				*scanline_list;	
	uint8				*scanbitmap_list;
#endif
};

#if ENABLE_DRAWING
class LBX_Stepper_0 : public LBX_Stepper {
public:
	LBX_Stepper_0(uint32 bitmap_index, LBX_Container_0 *context);
	virtual ~LBX_Stepper_0();
	
	virtual void	SeekToLine(uint32 index, uint8 **ptr);
	virtual void	NextLine(uint8 **ptr);

private:
	void			InitLineState();

	LBX_Container_0		*ctxt;
	uint8				*scanbitmap;
	uint8				*scanline, *scanline0, *scanline1, *scanline2;
	uint8				*pixel_offset;
	int16				similar_delta[2];
	uint8				*src1, *src2;
	int32				step;
	uint32				scan_index;
	uint32				scanline_mask;
};
#endif

#endif
