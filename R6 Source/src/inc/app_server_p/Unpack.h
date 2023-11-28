//--------------------------------------------------------------------
//	
//	Unpack.h
//
//	Written by: Pierre Raynaud-Richard
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef _UNPACK_H_
#define _UNPACK_H_

#include <SupportDefs.h>

// Switch between RGB16 and RGB15
#if 1
enum {
	RED_MASK = 0xf800,
	GREEN_MASK = 0x07e0,
	BLUE_MASK = 0x001f
};
#else
enum {
	RED_MASK = 0xf800,
	GREEN_MASK = 0x07c0,
	BLUE_MASK = 0x001f
};
#endif

class LBX_Unpack {
public:
	LBX_Unpack(uint16 *color_table);
	~LBX_Unpack();
	void 		ExtractTo32(uint8 *src_input, uint32 *dst, uint32 dst_left, uint32 dst_right);
	void		ExtractTo32(uint8 *src_input, uint8 *src1, uint8 *src2,
							uint32 *dst, uint32 dst_left, uint32 dst_right);
	void		DrawTo16(uint8 *src_input, uint8 *src1, uint8 *src2,
						 uint16 *dst, uint32 dst_left, uint32 dst_right);
	
private:
	void 		ExtractTo32(int32 count);
	void		RGB16To32(int32 count);
	void		RGBA16To32(int32 count);
	void 		DrawTo16(int32 count);
	void		RGB16To16(int32 count);
	void		RGBA16To16(int32 count);
	void		GetNextSource();
	void		Write32(uint32 val32) { dst32[0] = val32; dst_offset--; dst32 += dst_offset>>31; };
	void		Write16(uint16 val16) { if ((int32)dst_offset<=0) *dst16++ = val16; else dst_offset--; };
	void		Shade16(uint32 alpha);
	void		Draw16(uint16 val16, uint32 alpha);
	uint8		ReadByte() { if (src == src_end) GetNextSource(); return *src++; };
	uint8		ReadAlpha() { alpha_parity++; if (alpha_parity&1) { alpha_memo = ReadByte(); return alpha_memo&15; } return alpha_memo>>4; };  
	void		SyncAlpha() { alpha_parity = 0; };
	
	uint16		*table_rgb;
	uint16		last_color;
	uint8		*src, *src_end, *next_src;
	uint8		*similar_src1, *similar_src2;
	int32		alpha_parity;
	uint8		alpha_memo;
	uint32		dst_offset;
	uint32		*dst32;
	uint16		*dst16;
};

#endif
