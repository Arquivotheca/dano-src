//--------------------------------------------------------------------
//	
//	LBXFormat1.h
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _LBXFORMAT1_H_
#define _LBXFORMAT1_H_

#include "DrawEngine.h"
#include "Unpack.h"

class LBX_Container_1 : public LBX_Container {
public:
	LBX_Container_1(uint8 *buffer);
	~LBX_Container_1();

	virtual void		GetBitmapName(uint32 index, char *name);
	virtual uint16		BitmapWidth(uint32 index);
	virtual uint16		BitmapHeight(uint32 index);
	virtual int32		GetIndexFromName(char *name); // return -1 if unknown
#if ENABLE_DRAWING
	virtual LBX_Stepper	*Stepper(uint32 bitmap_index);

	virtual uint16		*CmapBuffer(uint32 index);
	uint32				BitmapIndex(uint32 scanline_index, uint32 hint);
#endif

	LBX_bitmap_header1	*bitmap_list;
	char				*name_buffer;
	uint16				*name_offset;
	uint16				*word_offset;
	bool				compressed_names;
#if ENABLE_DRAWING
	uint8				*scanbuffer;	
	uint8				*pixel_buffer;
#endif
};

#if ENABLE_DRAWING
enum {
	INSTR_SCANLIST8 = 0,
	INSTR_SCANLIST16 = 1,
	INSTR_SCANCOPY = 2,
	INSTR_BITMAP = 3
};

struct InstrInfos {
	uint8		*param_list;
	uint16		param;	
	uint16		count;
	uint16		current;
	uint8		command;
	uint8		step;
	
	uint8 		*ParseInstr(uint8 *instr);
};

class InstrCtxt {
public:
	InstrCtxt(LBX_Container_1 *container, uint32 bitmap_index, bool master);
	~InstrCtxt();		
	
	void		Reset(uint32 bitmap_index);
	void		SeekTo(uint32 index, InstrCtxt *copy_ctxt);
	void		ForwardStep(InstrCtxt *copy_ctxt);
	void		BackStep(InstrCtxt *copy_ctxt);

	uint8			*pixel;
	uint8			*cur_instr;
	uint8			secondary_bitmap_index, init_bitmap;
	int32			scan_index, max_index;
	uint32			similarity_count;
	int16			similarity[2];
	InstrCtxt		*simi_ctxt[2];
	InstrInfos		scan0;
	LBX_Container_1	*container;
};
	
class LBX_Stepper_1 : public LBX_Stepper {
public:
	LBX_Stepper_1(uint32 bitmap_index, LBX_Container_1 *context);
	virtual ~LBX_Stepper_1();
	
	virtual void	SeekToLine(uint32 index, uint8 **ptr);
	virtual void	NextLine(uint8 **ptr);

private:
	void			SetPtr(uint8 **ptr);

	InstrCtxt		*main_ctxt, *copy_ctxt;
	uint32			offset;
};
#endif

#endif
