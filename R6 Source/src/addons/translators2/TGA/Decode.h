//****************************************************************************************
//
//	File:		Decode.h
//
//	Written by:	Jon Watte and Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef DECODE_H
#define DECODE_H

#include "TGATranslator.h"
#include <SupportDefs.h>
class BPositionIO;
class BMessage;

bool ValidateHeader(TargaHeader &header);
bool GetColorMap(BPositionIO &input, TargaHeader &header, uint32 *palette);

status_t WriteBitmap(BPositionIO &input, BPositionIO &output, BMessage *ioExtension);
status_t WriteBitmapTrueColorUncompressed(BPositionIO &input, BPositionIO &output, uint32 *scanline,
	int pos, int start, int end, int add, int bit_depth, int row_bytes, int width);
status_t WriteBitmapTrueColorCompressed(BPositionIO &input, BPositionIO &output, uint32 *scanline,
	int pos, int start, int end, int add, int bit_depth, int row_bytes, int width);
status_t WriteBitmap8BitUncompressed(BPositionIO &input, BPositionIO &output, uint32 *scanline,
	int pos, int start, int end, int add, int bit_depth, int row_bytes, int width, uint32 *palette);
status_t WriteBitmap8BitCompressed(BPositionIO &input, BPositionIO &output, uint32 *scanline,
	int pos, int start, int end, int add, int bit_depth, int row_bytes, int width, uint32 *palette);

#endif
