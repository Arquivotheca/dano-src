//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------


#ifndef RASTER360_H
#define RASTER360_H

#include "raster.h"

/*
** Mode microweave manuel
*/

class BEpson;

class MDither360 : public MDither
{
public:
				MDither360(BEpson& epson, int32 bitmap_width, int32 next_pixel_offset, uint32 x_loop, uint32 y_loop);
	virtual		~MDither360(void);

	virtual		void		DoImage(void);
	virtual		status_t	StreamBitmap(void);
	virtual		status_t	StreamBitmapEnd(void);

protected:
	virtual void create_dither_4_2_mmx(int32 next_pixel_offset, uint32 width);
	virtual void create_dither_4_2(int32 next_pixel_offset, uint32 width);
	virtual void create_dither_4_1_mmx(int32 next_pixel_offset, uint32 width);
	virtual void create_dither_4_1(int32 next_pixel_offset, uint32 width);
	virtual void create_dither_6_2_mmx(int32 next_pixel_offset, uint32 width);
	virtual void create_dither_6_2(int32 next_pixel_offset, uint32 width);
	virtual void create_dither_6_1_mmx(int32 next_pixel_offset, uint32 width);
	virtual void create_dither_6_1(int32 next_pixel_offset, uint32 width);

private:
				void		Compute(uint32 line);

	uint32 blank_lines;
	uint8 *pScanLines;
};

#endif
