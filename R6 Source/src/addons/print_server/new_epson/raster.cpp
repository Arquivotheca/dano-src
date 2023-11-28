//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <BeBuild.h>
#include <OS.h>
#include <SupportDefs.h>

#include <stdlib.h>
#include <stdio.h>

#include "Driver.h"
#include "raster.h"

//#define DEBUG 1
#if DEBUG
#	define bug		printf
#	define D(_x)	_x
#else
#	define bug		printf
#	define D(_x)
#endif

const t_printer_color MDither::TabCMYKLCLM[6]	= {M_CYAN_COLOR, M_MAGENTA_COLOR, M_YELLOW_COLOR, M_BLACK_COLOR, M_LIGHT_CYAN_COLOR, M_LIGHT_MAGENTA_COLOR};
const t_printer_color MDither::TabCMYK[4]		= {M_CYAN_COLOR, M_MAGENTA_COLOR, M_YELLOW_COLOR, M_BLACK_COLOR};
const t_printer_color MDither::TabK[1]			= {M_BLACK_COLOR};

// -----------------------------------------------------
// class name: MDither
// purpose: Bitmap dithering
// -----------------------------------------------------

MDither::MDither(BEpson& epson, int32 bitmap_width,
								int32 next_pixel_offset,
								uint32 x_loop,
								uint32 y_loop)
	:	fEpson(epson),
		fDitherObject(NULL),
		fNbPlanes(fEpson.PrinterDef().color_mode),
		fNextPixelOffset(next_pixel_offset),
		fInitWidth(bitmap_width),
		fXLoop(x_loop),
		fYLoop(y_loop)
{
	// Clip the bitmap if it's larger than the max size allowed by the printer
	D(bug("fInitWidth = %ld / %ld\n", fInitWidth, fEpson.PrinterDef().printer_width);)
	if (fInitWidth > fEpson.PrinterDef().printer_width)
		fInitWidth = fEpson.PrinterDef().printer_width;

	// Nb of bit per pixel
	fNbBitPixel = ((fEpson.PrinterDef().microdot >> 4) & 0x01)+1;

	// width of a physical line in pixels
	fBytePerPlane = ((((fInitWidth*x_loop) + 7)/8 + 3) & 0xFFFFFFFC) * fNbBitPixel;

	// Color Matching
	fColorProcess = fEpson.PrinterDev()->instantiate_MColorProcess(&fEpson.PrinterDef());

	// Get the resolution
	fXDpi = fEpson.Settings().DeviceXdpi();
	fYDpi = fEpson.Settings().DeviceYdpi();
}



MDither::~MDither(void)
{
}

status_t MDither::InitDithering()
{
	const bool mmx = IsMMX();
	
	// Init the color table
	switch (fNbPlanes)
	{
		case 1:
			TabColor = TabK;
			break;

		case 4:
			TabColor = TabCMYK;
			if (fNbBitPixel == 2)
			{
				if (mmx)	create_dither_4_2_mmx(fNextPixelOffset, fInitWidth);
				else		create_dither_4_2(fNextPixelOffset, fInitWidth);
			}
			else
			{
				if (mmx)	create_dither_4_1_mmx(fNextPixelOffset, fInitWidth);
				else		create_dither_4_1(fNextPixelOffset, fInitWidth);
			}
			break;

		case 6:
			TabColor = TabCMYKLCLM;
			if (fNbBitPixel == 2)
			{
				if (mmx)	create_dither_6_2_mmx(fNextPixelOffset, fInitWidth);
				else		create_dither_6_2(fNextPixelOffset, fInitWidth);
			}
			else
			{
				if (mmx)	create_dither_6_1_mmx(fNextPixelOffset, fInitWidth);
				else		create_dither_6_1(fNextPixelOffset, fInitWidth);
			}
			break;
	}

	D(bug("EPSON: InitDithering... (color=%d, mmx=%d, bit/pixel=%d) : %p\n", fNbPlanes, mmx, fNbBitPixel, fDitherObject);)
	if (fDitherObject == NULL)
		return B_ERROR;

	return B_OK;
}

status_t MDither::Init(	MPrinter *printer,
						uint32 *ptr,
						int32 next_line_offset,
						uint32 nb_lines_to_process)
{
	fPrinter = printer;
	fSourceBitmapPtr = ptr;
	fNextLineOffset = next_line_offset;
	fNbLinesToProcess = nb_lines_to_process;
	return B_OK;
}

bool MDither::IsMMX(void)
{
#ifdef __INTEL__
	cpuid_info info;
	if (get_cpuid(&info, 1, 0) == B_OK)	// Request for feature flag (1)
	{
		if (info.eax_1.features & 0x800000)
		{
			return true;
		}
	}
#endif

	return false;
}

