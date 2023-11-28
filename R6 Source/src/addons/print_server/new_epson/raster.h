//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef RASTER_H
#define RASTER_H

#include "DriverTypes.h"
#include "MPrinterTypes.h"
#include "MDefinePrinter.h"
#include "MColorProcess.h"

class MPrinter;
class BEpson;
class DitheringObjectBase;

class MDither
{
public:
				MDither(BEpson& epson, int32 bitmap_width, int32 next_pixel_offset, uint32 x_loop, uint32 y_loop);
	virtual		~MDither(void);

	virtual		void		DoImage(void)			= 0;
	virtual		status_t	StreamBitmap(void)		= 0;
	virtual		status_t	StreamBitmapEnd(void)	= 0;
				status_t	Init(	MPrinter *printer,
									uint32 *ptr,
									int32 next_line_offset,
									uint32 nb_scan_lines);

	status_t 	InitDithering();

protected:
		virtual void create_dither_4_2_mmx(int32 next_pixel_offset, uint32 width) 	= 0;
		virtual void create_dither_4_2(int32 next_pixel_offset, uint32 width) 		= 0;
		virtual void create_dither_4_1_mmx(int32 next_pixel_offset, uint32 width) 	= 0;
		virtual void create_dither_4_1(int32 next_pixel_offset, uint32 width) 		= 0;
		virtual void create_dither_6_2_mmx(int32 next_pixel_offset, uint32 width) 	= 0;
		virtual void create_dither_6_2(int32 next_pixel_offset, uint32 width) 		= 0;
		virtual void create_dither_6_1_mmx(int32 next_pixel_offset, uint32 width) 	= 0;
		virtual void create_dither_6_1(int32 next_pixel_offset, uint32 width) 		= 0;

				bool		IsMMX(void);

protected:
	// initialized by ctor
	BEpson&			fEpson;
	MColorProcess 	*fColorProcess;
	DitheringObjectBase	*fDitherObject;
	const t_printer_color *TabColor;

	// initialized by Init()
	MPrinter		*fPrinter;
	uint32			*fSourceBitmapPtr;
	int32			fNextLineOffset;
	uint32			fNbLinesToProcess;
	int				fNbBitPixel;
	uint32			fBytePerPlane;
	int				fNbPlanes;
	int32			fNextPixelOffset;
	uint32			fInitWidth;
	int				fXDpi;
	int				fYDpi;
	uint32			fXLoop;
	uint32			fYLoop;

	static const t_printer_color TabCMYKLCLM[6];
	static const t_printer_color TabCMYK[4];
	static const t_printer_color TabK[1];
};


#endif

