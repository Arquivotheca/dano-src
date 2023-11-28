//*****************************************************************************
//
//	File:		 Scale.cpp
//
//	Description: bitmap scaling source
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************
#include "Scale.h"
#include <Bitmap.h>
#include <string.h>

void ddascale32(BBitmap *src, BBitmap *dest, float xratio, float yratio, volatile const bool *terminate)
{
	uint32 xSize = src->Bounds().IntegerWidth() + 1;
	uint32 ySize = src->Bounds().IntegerHeight() + 1;
	int32 scaledx = dest->Bounds().IntegerWidth() + 1;
	int32 scaledy = dest->Bounds().IntegerHeight() + 1;

	// separate xmap and ymap because alignment of area to
	// be scaled might be different for x and y
	uint32 *xmap = new uint32 [xSize];
	uint32 *ymap = new uint32 [ySize];

	int32 dda = 0;
	int32 x = 0;
	for(uint32 i = 0; i < xSize; i++)
	{
		xmap[x] = 0;
		dda -= 32768;
		while(dda < 0)
		{
			xmap[x]++;
			dda += (int32)(32768.0 / xratio);
		}
		x++;
	}
	dda = 0;
	x = 0;
	for(uint32 i = 0; i < ySize; i++)
	{
		ymap[x] = 0;
		dda -= 32768;
		while(dda < 0)
		{
			ymap[x]++;
			dda += (int32)(32768. / yratio);
		}
		x++;
	}
	uint32 srcRowBytes = src->BytesPerRow();
	uint32 outRowBytes = dest->BytesPerRow();
	uint8 *source = (uint8 *)src->Bits();
	uint8 *outData = (uint8 *)dest->Bits();

	int32 j, k;
	k = 0;
	switch (src->ColorSpace() & ~0x3000) {
		case B_RGB32:	// 32bpp
			for(uint32 curry = 0; curry < ySize; curry++)
			{
				for(uint32 y = ymap[curry]; y && k < scaledy; y--)
				{
					j = 0;
					for(uint32 currx = 0; currx < xSize; currx++)
						for(uint32 x = xmap[currx]; x && j < scaledx; x--)
							((uint32 *)outData)[j++] = ((uint32 *)source)[currx];
					outData += outRowBytes;
					k++;
				}
				source += srcRowBytes;
				if(*terminate == true)
					break;
			}
			break;
		case B_RGB16:
		case B_RGB15:	// 16bpp
			for(uint32 curry = 0; curry < ySize; curry++)
			{
				for(uint32 y = ymap[curry]; y && k < scaledy; y--)
				{
					j = 0;
					for(uint32 currx = 0; currx < xSize; currx++)
						for(uint32 x = xmap[currx]; x && j < scaledx; x--)
							((uint16 *)outData)[j++] = ((uint16 *)source)[currx];
					outData += outRowBytes;
					k++;
				}
				source += srcRowBytes;
				if(*terminate == true)
					break;
			}
			break;
		case B_CMAP8:
		case B_GRAY8:	// 8bpp
			for(uint32 curry = 0; curry < ySize; curry++)
			{
				for(uint32 y = ymap[curry]; y && k < scaledy; y--)
				{
					j = 0;
					for(uint32 currx = 0; currx < xSize; currx++)
						for(uint32 x = xmap[currx]; x && j < scaledx; x--)
							((uint8 *)outData)[j++] = ((uint8 *)source)[currx];
					outData += outRowBytes;
					k++;
				}
				source += srcRowBytes;
				if(*terminate == true)
					break;
			}
			break;
		case B_GRAY1:	// 8ppb
			for(uint32 curry = 0; curry < ySize; curry++)
			{
				for(uint32 y = ymap[curry]; y && k < scaledy; y--)
				{
					j = 0;
					memset(outData, 0, outRowBytes);
					for(uint32 currx = 0; currx < xSize; currx++)
						for(uint32 x = xmap[currx]; x && j < scaledx; x--)
						{
							if(((uint8 *)source)[currx / 8] & (1 << (8 - (currx % 8))))
								((uint8 *)outData)[j / 8] |= 1 << (8 - (j % 8));
							j++;
						}
					outData += outRowBytes;
					k++;
				}
				source += srcRowBytes;
				if(*terminate == true)
					break;
			}
			break;
		default:
			// punt
			break;
	}
	delete [] ymap;
	delete [] xmap;
}
