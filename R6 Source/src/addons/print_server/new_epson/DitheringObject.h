//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef DITHERING_OBJECT_H
#define DITHERING_OBJECT_H

#include "MColorProcess.h"
#include "DitheringObjectBase.h"
 

template <class Tcolor, class Tpixel, class Tconversion>
class DitheringObject : public DitheringObjectBase
{
public:
	DitheringObject(	MColorProcess *colorProcess,
						uint32 bytePerPlane,
						int32 nextPixelOffset,
						uint32 width,
						uint32 x_loop,
						int xdpi, int ydpi);

	virtual ~DitheringObject();

	void Init(void);

	virtual bool Dither(uint32 *pRGB, uint32 *pCmyk1, uint32 *pCmyk2=NULL);
	virtual bool Dither(uint32 *pRGB, uint32 **pCmyk) = 0;

protected:
	bool do_dither(uint32 *pRGB, Tpixel& pixel);

private:
	Tconversion		*fColorSpaceConversion;
	Tcolor			*fBackLines;
	MColorProcess 	*fColorProcess;

	int32	fNextPixelOffset;
	uint32	fWidth;				// Width of the source
	uint32	fWidthPixels;		// Width of the dest (fWidth*fXLoop)
	uint32	fNbXPixels;			// NB of source pixels really used
	uint32	fXLoop;
	bool	fDirection;
	int		fXDpi;
	int 	fYDpi;
	int		fErrorCoefShift[3];
};


template <class Tcolor, class Tpixel, class Tconversion>
class DitheringObjectNSlices : public DitheringObject<Tcolor, Tpixel, Tconversion>
{
public:
	DitheringObjectNSlices(	MColorProcess *colorProcess,
							uint32 bytePerPlane,
							int32 nextPixelOffset,
							uint32 width, uint32 x_loop,
							int xdpi, int ydpi,
							int nslices)
			: DitheringObject<Tcolor, Tpixel, Tconversion>(colorProcess, bytePerPlane, nextPixelOffset, width, x_loop, xdpi, ydpi),
			pixel(nslices, bytePerPlane)
	{
	}

	virtual ~DitheringObjectNSlices()
	{
	}

	virtual bool Dither(uint32 *pRGB, uint32 **pCmyk)
	{
		pixel.set(pCmyk);
		return DitheringObject<Tcolor, Tpixel, Tconversion>::do_dither(pRGB, pixel);
	}

private:
	Tpixel pixel;
};



// do the dithering for one line
template <class Tcolor, class Tpixel, class Tconversion>
DitheringObject<Tcolor, Tpixel, Tconversion>::DitheringObject(
													MColorProcess *colorProcess,
													uint32 bytePerPlane,
													int32 nextPixelOffset,
													uint32 width, uint32 x_loop,
													int xdpi, int ydpi)
		:	DitheringObjectBase(),
			fColorSpaceConversion(0),
			fBackLines(0),
			fNextPixelOffset(nextPixelOffset),
			fWidth(width),
			fWidthPixels(width*x_loop),
			fNbXPixels(width-(2*M_DITHER_JUMP_PIXEL)),
			fXLoop(x_loop),
			fXDpi(xdpi),
			fYDpi(ydpi)

{
	fColorSpaceConversion = new Tconversion(colorProcess->InitSpaceConversion());
	fBackLines = new Tcolor[fWidthPixels];		// # of 'delay' lines for the dithering

	if (fXDpi < fYDpi)
	{	// Aspect ratio = 2.0 (eg: 360x720), use a corrected filter
		//         X  8
		//       [24]
		fErrorCoefShift[0] = 2;		// 1/4 (8/32)
	}
	else if (fXDpi > fYDpi)
	{	// Aspect ratio = 0.5 (eg: 720x360), use a corrected filter
		//        X [24]
		//        8
		fErrorCoefShift[0] = 2;		// 1/4 (8/32)
	}
	else
	{	// Aspect ratio = 1.0, use a regular filter
		//        X  [16]
		//  	 16
		fErrorCoefShift[0] = 1;		// 1/2 (16/32)
	}
}

template <class Tcolor, class Tpixel, class Tconversion>
DitheringObject<Tcolor, Tpixel, Tconversion>::~DitheringObject(void)
{
	delete [] fBackLines;
	delete fColorSpaceConversion;
}

template <class Tcolor, class Tpixel, class Tconversion>
bool DitheringObject<Tcolor, Tpixel, Tconversion>::Dither(uint32 *pRGB, uint32 *pCmyk1, uint32 *pCmyk2)
{
	uint32 *pcmyk[2];
	pcmyk[0] = pCmyk1;
	pcmyk[1] = pCmyk2;
	return Dither(pRGB, pcmyk);
}

template <class Tcolor, class Tpixel, class Tconversion>
void DitheringObject<Tcolor, Tpixel, Tconversion>::Init(void)
{
	for (uint32 i=0 ; i<fWidthPixels ; i++)
		fBackLines[i] = 0;
	fDirection = false;
}

template <class Tcolor, class Tpixel, class Tconversion>
bool DitheringObject<Tcolor, Tpixel, Tconversion>::do_dither(uint32 *pRGB, Tpixel& pixel)
{
	int32 x;
	int32 dirX;
	int32 next_pixel_offset;

	// Inverser le sens de tramage pour la prochaine passe
	fDirection = !fDirection;

	// Initialisation des lignes a retard
	if (fDirection == false)
	{			
		pRGB += (M_DITHER_JUMP_PIXEL * fNextPixelOffset);
		next_pixel_offset = fNextPixelOffset;
		for (int i=0 ; i<=M_DITHER_JUMP_PIXEL*fXLoop ; i++)
			fBackLines[i] = 0;
		x = M_DITHER_JUMP_PIXEL*fXLoop;
		dirX = 1;
	}
	else
	{
		pRGB += ((fWidth - M_DITHER_JUMP_PIXEL - 1) * fNextPixelOffset);
		next_pixel_offset = -fNextPixelOffset;
		for (int i=0 ; i<=M_DITHER_JUMP_PIXEL*fXLoop ; i++)
			fBackLines[fWidthPixels-i-1] = 0;
		x = (fWidth - M_DITHER_JUMP_PIXEL)*fXLoop - 1;
		dirX = -1;
	}	

	// Check if the line is empty
	int32 cpt;
	for (cpt=0 ; cpt<fNbXPixels ; cpt++, pRGB += next_pixel_offset)
	{
		#if B_HOST_IS_LENDIAN
		if (((*pRGB) & 0x00FFFFFF) != 0x00FFFFFF)
			break;
		#else
		if (((*pRGB) & 0xFFFFFF00) != 0xFFFFFF00)
			break;
		#endif
	}
	if (cpt == fNbXPixels)	// The line was empty
		return false;

	// Init MMX engine if needed
	Tcolor::InitMMX();
 
	// The line is not empty, let's dither it
	Tcolor error(0)			ALLIGN(8);
	Tcolor error0(0)		ALLIGN(8);
	Tcolor prev_error(0)	ALLIGN(8);

	// Tramage
	for (x+=(cpt*dirX*fXLoop) ; cpt<fNbXPixels ; cpt++)
	{
		int16 cmyk[6];
		const bool white = (fColorSpaceConversion->SpaceConversion(*pRGB, cmyk) == false);
		pRGB += next_pixel_offset;
		for (uint32 xloop=0 ; xloop<fXLoop ; xloop++)
		{
			if (!white)
			{
				int32 random_number;
				rand3(&random_number, 1, 0); // 1 coef @ 100% (>>0)
				error.dither(prev_error, pixel.data(), cmyk);
				error0.error(error, random_number, fErrorCoefShift[0]);
			}
			else
			{
				error.dither(prev_error, pixel.data());
				error0.error(error, fErrorCoefShift[0]);
			}
			error -= error0;
	
			// Put pixel
			pixel.put(x);
			Tcolor *pBack = fBackLines + x;
			x += dirX;
	
		 	if (fXDpi < fYDpi)
		 	{
	 			prev_error.prev(error0, pBack[dirX]);
				pBack[0] = error;
		 	}
			else
			{
				prev_error.prev(error, pBack[dirX]);
				pBack[0] = error0;
			}
		}
	}
	
	// Close MMX engine
	Tcolor::ExitMMX();

	return true;	// la ligne n'était pas vide
}

// Floyd : 16
//     X  7
//  3  5  1

// Burkes : 32
//        X  8  4
//  2  4  8  4  2

// Stuki : 42
//        X  8  4
//  2  4  8  4  2
//  1  2  4  2  1

// Sierra : 32
//        X  5  3
//  2  4  5  4  2
//     2  3  2 

// Gravis, Judice and Ninke : 48
//        X  7  5
//  3  5  7  5  3
//  1  3  5  3  1 

// Stevenson and Arce : 200 (only for hexagonal grid)
//           X  .  32 .
//  12 .  26 .  30 .  16
//  .  12 .  26 .  12 .
//  5  .  12 .  12 .  5


#endif




