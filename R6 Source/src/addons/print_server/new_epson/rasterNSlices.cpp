//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <BeBuild.h>
#include <SupportDefs.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "rasterNSlices.h"

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


#if 0
#	define D(_x)	_x
#	define bug		printf
#else
#	define D(_x)	
#	define bug
#endif

MDitherNSlices::MDitherNSlices(BEpson& epson, int32 bitmap_width, int32 next_pixel_offset, uint32 x_loop, uint32 y_loop)
		: MDither(epson, bitmap_width, next_pixel_offset, x_loop, y_loop),
		pScanLines(NULL),
		pTempOutBuf(NULL),
		fBlocks(NULL),
		fNbSlices(fEpson.PrinterDef().hslices)	
{
	static const int8 adv0[] = {-1,-1,-1,-1, -1,-1,-1,-1};
	static const int8 adv6[] = {2,-4,3,-2,4,-3,				-1,-1,-1,-1,-1,-1};
	static const int8 adv8[] = {3,-5,11,-4,-3, 5,-11,4,		3,-5,3,-4,-3,5,-3,4,	-1,-1,-1,-1,-1,-1,-1,-1};
	fDeltaLines[0].NbSchemes = 0;	fDeltaLines[0].Size = 0;	fDeltaLines[0].Advancement = NULL;
	fDeltaLines[1].NbSchemes = 1;	fDeltaLines[1].Size = 1;	fDeltaLines[1].Advancement = adv0;
	fDeltaLines[2].NbSchemes = 1;	fDeltaLines[2].Size = 2;	fDeltaLines[2].Advancement = adv0;
	fDeltaLines[3].NbSchemes = 1;	fDeltaLines[3].Size = 3;	fDeltaLines[3].Advancement = adv0;
	fDeltaLines[4].NbSchemes = 1;	fDeltaLines[4].Size = 4;	fDeltaLines[4].Advancement = adv0;
	fDeltaLines[5].NbSchemes = 0;	fDeltaLines[5].Size = 0;	fDeltaLines[5].Advancement = NULL;
	fDeltaLines[6].NbSchemes = 2;	fDeltaLines[6].Size = 6;	fDeltaLines[6].Advancement = adv6;
	fDeltaLines[7].NbSchemes = 0;	fDeltaLines[7].Size = 0;	fDeltaLines[7].Advancement = NULL;
	fDeltaLines[8].NbSchemes = 3;	fDeltaLines[8].Size = 8;	fDeltaLines[8].Advancement = adv8;

	// Adapt the byte per plane to the resolution (must be multiple of 4)
	fBytePerPlane = ((fBytePerPlane / fNbSlices) + 3) & 0xFFFFFFFC;

	// Software microweave parameters
	fNbRasters		= fEpson.PrinterDef().printer_info.gNbColorNozzle;				// # nozzles
	fNbBlocks		= fYDpi / fEpson.PrinterDef().printer_info.gColorNozzleDPI;		// # vertical slices

	// Computes the print head advancements
	const int base = fNbRasters/fNbSlices;
	const tAdvancement& adv = fDeltaLines[fNbBlocks];
	for (int i=0 ; i<adv.NbSchemes ; i++)
	{
		fAdvancements = adv.Advancement + (i * adv.Size * sizeof(adv.Advancement[0]));
		int j;
		for (j=0 ; j<fNbBlocks ; j++)
			if ((base + fAdvancements[j]) <= 0)
				break;
		if (j == fNbBlocks)
			break;
	}
	// Compute howmany nozzle are not used
	int notUsed = 0;
	for (int i=0 ; i<fNbBlocks ; i++)
		notUsed += fAdvancements[i];	// (!) Advancements are <0
	fDeltaNozzle = fNbRasters + (notUsed / fNbBlocks);

	D(bug("fAdvancements: "));
	D(for (int i=0 ; i<fNbBlocks ; i++))
		D(bug("%d ", fAdvancements[i]));
	D(bug("\n"));
	D(bug("notUsed: %d\n", -notUsed/fNbBlocks));

	pTempOutBuf	= new uint8[fBytePerPlane * fNbRasters];
	memset(pTempOutBuf, 0, fBytePerPlane * fNbRasters);
	fBlocks	= new tDitherBlock[fNbBlocks * fNbSlices];

	// Buffer tampon des donnees a envoyer a l'imprimante (apres tramage)
	const size_t sizeScanline = fNbPlanes * fBytePerPlane * fNbRasters * fNbBlocks * fNbSlices; // We need 'fNbPlanes' planes * fNbRasters * (fNbBlocks*fNbSlices)
	pScanLines	= new uint8[sizeScanline];	
	memset(pScanLines, 0, sizeScanline);

	// XDisplacement never changes, we can precalculate it
	CalcXDisplacement();
//	printf ("Xd =\n");
//	for (int i=0 ; i<fNbBlocks ; i++, printf("\n"))
//		for (int j=0 ; j<fNbSlices ; j++)
//			printf("%3d ", fBlocks[i*fNbSlices+j].xdelta);
//	printf("\n");
}


MDitherNSlices::~MDitherNSlices(void)
{
	delete [] pScanLines;
	delete [] pTempOutBuf;
	delete [] fBlocks;
	delete fDitherObject;
}

// Must be called for each page
void MDitherNSlices::DoImage(void)
{
	int k,p,q;
	const int base = fNbRasters/fNbSlices;

	D(bug("fDeltaNozzle = %d\n", fDeltaNozzle));
	D(bug("fNbBlocks = %d\n", fNbBlocks));
	D(bug("fNbSlices = %d\n", fNbSlices));
	D(bug("base = %d\n", base));

	for (int i=k=p=q=0 ; i<fNbSlices ; i++)
	{
		D(bug("%d\n",i));
		for (int j=0 ; j<fNbBlocks ; j++, k++)
		{
			const int n = base + fAdvancements[j];
			const int m = (n + fNbBlocks) % fNbBlocks;
			D(bug("n =%3d, m =%3d\n",n,m));
			fBlocks[k].first_pass 	= true;
			fBlocks[k].first		= q;
			fBlocks[k].delta		= m;
			fBlocks[k].first_next	= p;
			fBlocks[k].delta_next	= n;
			q += fBlocks[k].delta;
			p += fBlocks[k].delta_next;
		}
	}

	// Adjust the last slice's displacement
	q = q - fBlocks[k-1].delta + fBlocks[k-1].delta_next;
	fBlocks[k-1].delta = fBlocks[k-1].delta_next;

	D(bug("q = %d\n", q));

	fFirstActiveLine = fBlocks[k-1].first;
	for (int i=0 ; i<fNbBlocks*fNbSlices ; i++)
		fBlocks[i].first_next += q;

	// init blank lines counters to zero (blank & total)
	blank_lines = 0;
	fCountLines = 0;
	fDitherObject->Init();
}

void MDitherNSlices::CalcXDisplacement()
{ // TODO: We must change that, I don't think it's the best way to place X pixels
	for (int i=0,k=0 ; i<fNbSlices ; i++)
	{
		for (int j=0 ; j<fNbBlocks ; j++, k++)
		{
			fBlocks[k].xdelta = (j + i) % fNbSlices;
		}
	}
}



status_t MDitherNSlices::StreamBitmapEnd(void)
{
	status_t result = B_OK;
	// Ejecter tous les buffers en attente, dans l'ordre

	for (int i=0 ; ((i<fNbBlocks*fNbSlices) && (result == B_OK)) ; i++)
	{
		int ejectblock = 0;
		for (int j=0 ; j<fNbBlocks*fNbSlices ; j++)
			if (fBlocks[j].first < fBlocks[ejectblock].first)
				ejectblock = j;

		result = eject_buffer(ejectblock, fBlocks[ejectblock].xdelta, fBlocks[ejectblock].delta, fNbRasters);
		fBlocks[ejectblock].first = 0x7fffffff;
	}

	return result;
}


status_t MDitherNSlices::StreamBitmap(void)
{
	status_t result = B_OK;
	uint32 line = 0;
	while ((result == B_OK) && (line < fNbLinesToProcess))
	{
		for (uint32 yloop=0 ; (yloop<fYLoop) && (result == B_OK) && fEpson.CanContinue() ; yloop++)
		{
			// Callback to the driver to let it a chance to do stuffs
			fEpson.EveryLine();
			
			// Trouver le block/buse
			int block_to_eject = -1;
			int nbfound = 0;
			int blocks[16];
			int nozzle[16];
			const int maxlines = fDeltaNozzle;
	
			for (int i=0 ; (i<(fNbBlocks*fNbSlices)) && (nbfound < fNbSlices) ; i++)
			{
				if ((fCountLines >= fBlocks[i].first) &&
					(fCountLines <= fBlocks[i].first + ((fNbRasters-1)*fNbBlocks)) &&
					(((fCountLines - fBlocks[i].first) % fNbBlocks) == 0)) 
				{
					int index = fBlocks[i].xdelta;
					nozzle[index] = (fCountLines - fBlocks[i].first) / fNbBlocks;
					blocks[index] = i;
	
					if (nozzle[index] == maxlines-1)
						block_to_eject = blocks[index];
					nbfound++;
				}
			}
	
			if ((nbfound == fNbSlices) && (fCountLines >= fFirstActiveLine))
			{ // Ne pas imprimer les 1ere lignes (car non completes)
				Compute(line, blocks, nozzle);
			}		
	
			if (block_to_eject >= 0)
			{
				result = eject_buffer(block_to_eject, fBlocks[block_to_eject].xdelta, fBlocks[block_to_eject].delta, maxlines);
				if (fBlocks[block_to_eject].first_pass == true)
				{
					fBlocks[block_to_eject].first_pass = false;
					fBlocks[block_to_eject].first = fBlocks[block_to_eject].first_next;
					fBlocks[block_to_eject].delta = fBlocks[block_to_eject].delta_next;
				}
				else
				{
					// Here the advancement is given in real lines (not in print head's lines)
					fBlocks[block_to_eject].first += fNbBlocks*(fNbRasters - ((fNbRasters-maxlines)*fNbSlices));
				}
			}
	
			// Update counters
			fCountLines++;
		}
		line++;
	}

	return result;
}




status_t MDitherNSlices::eject_buffer(	int out_buffer,		// # of the buffer to print
										int hpos,			// # horiz. position
										int offset,			// # of lines to advance the head
										int nb_lines		// # of lines is the buffer
									)
{
	status_t result = B_OK;

	// Ejecter le buffer no 'out_buffer'
	for (int plane=0 ; ((plane<fNbPlanes) && (result == B_OK)) ; plane++)
	{ 		
		for (int j=0; j<nb_lines ; j++)
		{
			uint8 *p = CalcLineAddress(out_buffer, j, plane);
			memcpy(pTempOutBuf + j*fBytePerPlane, p, fBytePerPlane);
		}

		// Find the margins for this slice
		int start_offset = fBytePerPlane/4;
		int end_offset = 0;
		for (int j=0 ; j<nb_lines ; j++)
		{
			uint32 *pLineStart = (uint32 *)pTempOutBuf + j*(fBytePerPlane/4);
			uint32 *pLineEnd = pLineStart + (fBytePerPlane/4);				
			uint32 *p;
			for (p=pLineStart ; (p < pLineEnd) && (*p == 0) ; p++) { /* Nothing to do */ }
			start_offset = min_c(start_offset, (p-pLineStart));
			for (p=pLineEnd ; (p > pLineStart) && (*(p-1) == 0) ; p--) { /* Nothing to do */ }
			end_offset = max_c(end_offset, (p-pLineStart));
			
			if ((end_offset-start_offset) == (fBytePerPlane/4))
			{ // We can't reduce the margins for this slice, useless to keep trying
				break;
			}				
		}

		const int minimumByteLength = 4*max_c(0, end_offset-start_offset);
		start_offset *= 4;
		
		// The slice is non-empty
		if (minimumByteLength > 0)
		{
			// Remove the margins if needed
			if (minimumByteLength < fBytePerPlane)
			{ // We can do it in-place
				for (int j=0; j<nb_lines ; j++)
				{
					uint32 *p = (uint32 *)pTempOutBuf + j*(fBytePerPlane/4) + start_offset/4;
					memmove(pTempOutBuf + j*minimumByteLength, p, minimumByteLength);
				}
			}

			if (blank_lines != 0)
			{
				if (result == B_OK)
					result = fPrinter->SetVertPosition(blank_lines);							
				blank_lines = 0;
			}
		
			const uint32 delta_x = (8 * minimumByteLength)/fNbBitPixel;			// nb of pixels

			if (result == B_OK)
				result = fPrinter->SelectColor(TabColor[plane]);	// select color

			if (result == B_OK)
				result = fPrinter->SetHorPosition(hpos + (start_offset*fNbSlices*(8/fNbBitPixel)));

			if (result == B_OK)
				result = fPrinter->PrintRasterData(	(char *)pTempOutBuf,
													delta_x,
													nb_lines);		// send the scanline, nb of pixels
		}
	}

	if (blank_lines == 0)
	{
		// Effacer le buffer qu'on vient d'ejecter, s'il etait non vide
		const long sizeBuffer = fNbRasters * fNbPlanes * fBytePerPlane;
		uint8 *p = CalcLineAddress(out_buffer, 0, 0);
		memset(p, 0, sizeBuffer);
	}

	blank_lines += offset;
	return result;
}


// CalcDeltaNozzle:
// Trouve l'increment-Y le mieux approprié
int MDitherNSlices::CalcDeltaNozzle(int NbBuses, int NbBlock)
{
	assert((NbBuses % NbBlock)==0);
	for (int n=2; n<=NbBlock/2; n++) {
		for (int m=2; m<=n; m++) {
			if (((NbBlock % m) == 0) && ((n % m) == 0)) {
				goto next_n;
			}
		}
		return (NbBuses-n);
next_n:;
	}
	return (NbBuses-1);
}


void MDitherNSlices::Compute(uint32 line, int *blocks, int *nolines)
{
	// Indiquer le buffer pour les pixels pairs et impairs
	uint32 *pCmyk[16];
	for (int i=0 ; i<fNbSlices ; i++)
		pCmyk[i] = (uint32 *)CalcLineAddress(blocks[i], nolines[i], 0);

	uint32 * const pRGB = fSourceBitmapPtr + fNextLineOffset*line;
	fDitherObject->Dither(pRGB, pCmyk);
}


uint8 *MDitherNSlices::CalcLineAddress(int buffer, int line, int plane)
{
	const size_t offset = ((buffer*fNbRasters) + line) * (fNbPlanes * fBytePerPlane) + (plane * fBytePerPlane);
	return pScanLines + offset;
}





void MDitherNSlices::create_dither_4_2_mmx(int32 next_pixel_offset, uint32 width)
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
							fNbSlices);
#endif
#endif
#endif
}

void MDitherNSlices::create_dither_4_2(int32 next_pixel_offset, uint32 width)
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
							fNbSlices);
#endif
#endif
}

void MDitherNSlices::create_dither_4_1_mmx(int32 next_pixel_offset, uint32 width)
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
							fNbSlices);
#endif
#endif
#endif
}

void MDitherNSlices::create_dither_4_1(int32 next_pixel_offset, uint32 width)
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
							fNbSlices);
#endif
#endif
}




void MDitherNSlices::create_dither_6_2_mmx(int32 next_pixel_offset, uint32 width)
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
							fNbSlices);
#endif
#endif
#endif
}

void MDitherNSlices::create_dither_6_2(int32 next_pixel_offset, uint32 width)
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
							fNbSlices);
#endif
#endif
}

void MDitherNSlices::create_dither_6_1_mmx(int32 next_pixel_offset, uint32 width)
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
							fNbSlices);
#endif
#endif
#endif
}

void MDitherNSlices::create_dither_6_1(int32 next_pixel_offset, uint32 width)
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
							fNbSlices);
#endif
#endif
}

