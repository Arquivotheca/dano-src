//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------


#ifndef RASTER_NSLICES_H
#define RASTER_NSLICES_H

#include "raster.h"

class BEpson;
class DitheringObjectBase;

/*
** Mode microweave manuel
*/

class MDitherNSlices : public MDither
{
public:
				MDitherNSlices(BEpson& epson, int32 bitmap_width, int32 next_pixel_offset, uint32 x_loop, uint32 y_loop);
	virtual		~MDitherNSlices(void);

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
	int			CalcDeltaNozzle(int NbBuses, int NbBlock);
	status_t	eject_buffer(int out_buffer, int hpos, int offset, int nb_lines);
	void		Compute(uint32 line, int *blocks, int *nolines);
	uint8		*CalcLineAddress(int buffer, int line, int plane);
	void		CalcXDisplacement();

	uint32				blank_lines;
	uint32				fCountLines;

	uint8				*pScanLines;

	int		fDeltaNozzle;
	int 	fNbRasters;
	int		fNbBlocks;
	int		fNbSlices;
	int		fFirstActiveLine;
	uint8	*pTempOutBuf;

	struct tDitherBlock
	{
		bool first_pass;
		int first;
		int	delta;
		int first_next;
		int	delta_next;
		int xdelta;
	};
	tDitherBlock	*fBlocks;
	
	struct tAdvancement
	{
		uint8 NbSchemes;
		uint8 Size;
		const int8 *Advancement;
	};
	tAdvancement fDeltaLines[9];
	const int8 *fAdvancements;
};

#endif
