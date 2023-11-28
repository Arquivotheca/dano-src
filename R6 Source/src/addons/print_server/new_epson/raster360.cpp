//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "raster360.h"

#include "Driver.h"
#include "MPrinter.h"
#include "DitheringObject.h"

#include "Tcolor4.h"
#include "Tcolor4_2.h"
#include "Tcolor6.h"
#include "Tcolor6_2.h"
#include "Tcolor4_mmx.h"
#include "Tcolor4_2_mmx.h"
#include "Tcolor6_mmx.h"
#include "Tcolor6_2_mmx.h"

#include "Tpixel4.h"
#include "Tpixel4_2.h"
#include "Tpixel6.h"
#include "Tpixel6_2.h"
#include "Tpixel4_mmx.h"
#include "Tpixel4_2_mmx.h"
#include "Tpixel6_mmx.h"
#include "Tpixel6_2_mmx.h"

#include "MCS_Conversion4.h"
#include "MCS_Conversion6.h"


// -----------------------------------------------------
// class name: MDither
// purpose: Bitmap dithering
// -----------------------------------------------------

MDither360::MDither360(BEpson& epson, int32 bitmap_width, int32 next_pixel_offset, uint32 x_loop, uint32 y_loop)

		:	MDither(epson, bitmap_width, next_pixel_offset, x_loop, y_loop),
			pScanLines(NULL)
{
	// Buffer tampon des donnees a envoyer a l'imprimante (apres tramage)
	const long sizeScanline = fNbPlanes * fBytePerPlane ; // We need 'fNbPlanes' planes
	pScanLines	= new uint8[sizeScanline];	
	memset(pScanLines, 0, sizeScanline);
}

MDither360::~MDither360(void)
{
	delete [] pScanLines;
	delete fDitherObject;
}


// Doit etre appele pour chaque nouvelle page si
// l'objet n'a pas ete reconstruit
void MDither360::DoImage(void)
{
	// init blank lines counters to zero (blank & total)
	blank_lines = 0;
	fDitherObject->Init();
}


status_t MDither360::StreamBitmap(void)
{
	status_t result = B_OK;
	int plane;
	uint32 pos_x;
	uint32 delta_x;
	uint32 line;	
	uint32 *pTempStart;							// Address of the begining of the real one-plane data
	uint32 *pCurStart;							// Address of the begining of the real one-plane data
	uint32 *pCurEnd;							// Address of the end of the real one-plane data
	const int32 WordPerScanLine = fBytePerPlane/4;	// # of 32-bits word per scanline
	const uint32 nb_lines_to_process = fNbLinesToProcess;	

	for (line=0 ; (line<nb_lines_to_process) ; line++)
	{
		for (uint32 yloop=0 ; (yloop<fYLoop) && (result == B_OK) && fEpson.CanContinue() ; yloop++)
		{
			// Callback to the driver to let it a chance to do stuffs
			fEpson.EveryLine();

			// Erase, then dither the line
			const long sizeBuffer = fNbPlanes * fBytePerPlane;
			memset(pScanLines, 0, sizeBuffer);
			Compute(line);
	
			// Ejecter la ligne
			for (plane=0 ; ((plane<fNbPlanes) && (result == B_OK)) ; plane++)
			{ 		
				// Line pointer
				pCurStart = ((uint32 *)pScanLines) + (plane * WordPerScanLine);		// Address of the begining of the real one-plane data
				pCurEnd = pCurStart + WordPerScanLine ;								// Address of the end of the real one-plane data
				pTempStart = pCurStart;
				
				// find the first non-zero '32 bits word' in the buffer
				for ( ; pCurStart < pCurEnd ; pCurStart++)
				{
					if (*pCurStart != 0)
						break;
				}
	
				if (pCurStart < pCurEnd)
				{
					// find the last non-zero byte in the buffer				
					for (pCurEnd-- ; pCurEnd > (pCurStart+1) ; pCurEnd--)
					{
						if (*pCurEnd != 0)
							break;
					}
					pCurEnd++;
					
					pos_x = (8 * ((uint32)pCurStart - (uint32)pTempStart))/fNbBitPixel;
					delta_x = (8 * ((uint32)pCurEnd - (uint32)pCurStart))/fNbBitPixel;
					
					if (blank_lines	!= 0)
					{
						result = fPrinter->SetVertPosition(blank_lines);	// move the printer head to this line, reset blank_lines
						blank_lines = 0;									// reset blank_lines
					}
	
					if (result == B_OK)
						result = fPrinter->SelectColor(TabColor[plane]);					// select color
	
					if (result == B_OK)
						result = fPrinter->SetHorPosition(pos_x);							// move the printer head to the right row
	
					if (result == B_OK)
						result = fPrinter->PrintRasterData((char *)pCurStart, delta_x, 1);	// send the scanline, nb of pixels
				}
			}
	
			// Update the raster_line pointer
			blank_lines++;
		}
	}
	return result;
}


status_t MDither360::StreamBitmapEnd(void)
{
	return B_OK;
}

void MDither360::Compute(uint32 line)
{
	uint32 * const pRGB = fSourceBitmapPtr + fNextLineOffset*line;
	fDitherObject->Dither(pRGB, (uint32 *)pScanLines);
}




void MDither360::create_dither_4_2_mmx(int32 next_pixel_offset, uint32 width)
{
#ifdef MFOUR_COLORS
#ifdef __INTEL__
#ifdef M_MMX
	fDitherObject = new DitheringObjectNSlices<	t_fcolor_cmyk_2_mmx,
												t_pixel_cmyk_nslices_2_mmx,
												MCS_Conversion4 >
						(	fColorProcess,
							fBytePerPlane,
							next_pixel_offset,
							width, fXLoop,
							fXDpi, fYDpi,
							1);
#endif
#endif
#endif
}

void MDither360::create_dither_4_2(int32 next_pixel_offset, uint32 width)
{
#ifdef MFOUR_COLORS
#ifdef M_NO_MMX
	fDitherObject = new DitheringObjectNSlices<	t_fcolor_cmyk_2,
												t_pixel_cmyk_nslices_2,
												MCS_Conversion4 >
						(	fColorProcess,
							fBytePerPlane,
							next_pixel_offset,
							width, fXLoop,
							fXDpi, fYDpi,
							1);
#endif
#endif
}

void MDither360::create_dither_4_1_mmx(int32 next_pixel_offset, uint32 width)
{
#ifdef MFOUR_COLORS
#ifdef __INTEL__
#ifdef M_MMX
	fDitherObject = new DitheringObjectNSlices<	t_fcolor_cmyk_mmx,
												t_pixel_cmyk_nslices_mmx,
												MCS_Conversion4 >
						(	fColorProcess,
							fBytePerPlane,
							next_pixel_offset,
							width, fXLoop,
							fXDpi, fYDpi,
							1);
#endif
#endif
#endif
}

void MDither360::create_dither_4_1(int32 next_pixel_offset, uint32 width)
{
#ifdef MFOUR_COLORS
#ifdef M_NO_MMX
	fDitherObject = new DitheringObjectNSlices<	t_fcolor_cmyk,
												t_pixel_cmyk_nslices,
												MCS_Conversion4 >
						(	fColorProcess,
							fBytePerPlane,
							next_pixel_offset,
							width, fXLoop,
							fXDpi, fYDpi,
							1);
#endif
#endif
}




void MDither360::create_dither_6_2_mmx(int32 next_pixel_offset, uint32 width)
{
#ifdef MSIX_COLORS
#ifdef __INTEL__
#ifdef M_MMX
	fDitherObject = new DitheringObjectNSlices<	t_fcolor_cmyklclm_2_mmx,
												t_pixel_cmyklclm_nslices_2_mmx,
												MCS_Conversion6 >
						(	fColorProcess,
							fBytePerPlane,
							next_pixel_offset,
							width, fXLoop,
							fXDpi, fYDpi,
							1);
#endif
#endif
#endif
}

void MDither360::create_dither_6_2(int32 next_pixel_offset, uint32 width)
{
#ifdef MSIX_COLORS
#ifdef M_NO_MMX
	fDitherObject = new DitheringObjectNSlices<	t_fcolor_cmyklclm_2,
												t_pixel_cmyklclm_nslices_2,
												MCS_Conversion6 >
						(	fColorProcess,
							fBytePerPlane,
							next_pixel_offset,
							width, fXLoop,
							fXDpi, fYDpi,
							1);
#endif
#endif
}

void MDither360::create_dither_6_1_mmx(int32 next_pixel_offset, uint32 width)
{
#ifdef MSIX_COLORS
#ifdef __INTEL__
#ifdef M_MMX
	fDitherObject = new DitheringObjectNSlices<	t_fcolor_cmyklclm_mmx,
												t_pixel_cmyklclm_nslices_mmx,
												MCS_Conversion6 >
						(	fColorProcess,
							fBytePerPlane,
							next_pixel_offset,
							width, fXLoop,
							fXDpi, fYDpi,
							1);
#endif
#endif
#endif
}

void MDither360::create_dither_6_1(int32 next_pixel_offset, uint32 width)
{
#ifdef MSIX_COLORS
#ifdef M_NO_MMX
	fDitherObject = new DitheringObjectNSlices<	t_fcolor_cmyklclm,
												t_pixel_cmyklclm_nslices,
												MCS_Conversion6 >
						(	fColorProcess,
							fBytePerPlane,
							next_pixel_offset,
							width, fXLoop,
							fXDpi, fYDpi,
							1);
#endif
#endif
}
