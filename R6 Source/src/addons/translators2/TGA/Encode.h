//****************************************************************************************
//
//	File:		Encode.h
//
//	Written by:	Jon Watte and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef ENCODE_H
#define ENCODE_H

#include <SupportDefs.h>
#include <GraphicsDefs.h>
class BPositionIO;

status_t WriteTarga(BPositionIO &input, BPositionIO &output);
status_t WriteTargaHeader(BPositionIO &output, BRect bounds, int bits_per_pixel,
	bool compressed, bool greyscale);
status_t ExpandScanline(uchar *scanline, color_space colors, int width);
status_t CollapseScanline(uchar *scanline, int bits, int width, bool greyscale);

#endif