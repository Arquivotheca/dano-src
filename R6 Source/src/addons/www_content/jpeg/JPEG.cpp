// ===========================================================================
//	JPEG.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "JPEG.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>

static JPEGError AllocateBlockBuffer(JPEGDecoder*);
static void MakeFastHuffmanTable(HuffmanTable*);
static JPEGError SetupHuffmanTable(HuffmanTable*);

JPEGError JPEGBounds(const uchar *image, long size, BRect* bounds)
{
	long result;
	long offset = 0;
	JPEGDecoder	*decoder;

	bounds->Set(0,0,0,0);
	decoder = NewJPEGDecoder(bounds, 0, NULL, NULL);
	if (decoder == 0)
		return kLowMemory;

	while ((result = JPEGWrite(decoder, image + offset, size - offset)) > 0) {
		if (decoder->CompInFrame > 0) {
			bounds->Set(0,0,decoder->Width,decoder->Height);
			DisposeJPEGDecoder(decoder);
			return kNoError;			// Leave after Start of Frame
		}

		offset += result;
	}

	DisposeJPEGDecoder(decoder);
	if (result < 0 || decoder->phase > kWaitingForFrame)
		return kGenericError;

	return kCannotParse;				// Not enough data to reach kSOS
}

JPEGError JPEGDecodeImage(JPEGDecoder *decoder, uchar *image, long size)
{
	long offset = 0;
	long result;
	while ((result = JPEGWrite(decoder, image + offset,size - offset)) > 0) {
		offset += result;
		if (size <= (offset + 2))
			break;						// Stopped decoding in a paricular phase
	}

	if (result < 0) {
		if (result == kJPEGCompletedPass)
			result = 0;

		return (JPEGError) result;
	}

	if (decoder->phase != kWaitingForNewImage) 
		return kCannotParse;

	return kNoError;
}

// resets the state for doing multiple passes to render progressive JPEGs in slices
static JPEGError AllocateBlockBuffer(JPEGDecoder *decoder)
{
	for (long nf = 0; nf < decoder->CompInFrame; nf++) {
		CompSpec *cspec = &decoder->Comp[nf];
		cspec->blockCount = decoder->HeightMCU * decoder->WidthMCU * cspec->blocksMCU;
		long bsize = cspec->blockCount * kMCUSize * sizeof(short);
		cspec->blockBufferData = (short*) malloc(bsize);
	}

	return kNoError;
}

//	SOF defines Y and X line counts and frame components:
//	Defines h and v subsampling and Q table index for each component
//
//	The decoder also only supports several interleaving factors
//	so we check for those too
JPEGError InterpretSOF(JPEGDecoder *decoder, const uchar *data, short size)
{
	short	nf,i,MaxHi,MaxVi;
	CompSpec	*cspec;
	long dstWidth,dstHeight;
	const uchar	*dataEnd = data + size -2;
	long	totalMCUs = 0;
	bool	justSizeCheck;
	
	dstWidth = (long)(decoder->mDrawRect.right - decoder->mDrawRect.left);
	dstHeight = (long)(decoder->mDrawRect.bottom - decoder->mDrawRect.top);
	justSizeCheck = dstWidth == 0;
	
	if (*data++ != 8) {
		printf("Only 8-bit precision JPEG is supported");
		return kBadSOFMarkerErr;				// Only understand 8 bit precision
	}
		
	decoder->Height = *data++;					// Number of vertical lines
	decoder->Height = (decoder->Height << 8) | *data++;
	if (decoder->Height == 0) {
		// height is defined by DNL marker at end of first scan, we can't handle that now
		printf("JPEG Images with DNL markers not supported");
		return kBadSOFMarkerErr;				// Only understand 8 bit precision
	}

	decoder->Width = *data++;						// Number of horizontal lines
	decoder->Width = (decoder->Width << 8) | *data++;
	decoder->CompInFrame = *data++;				// Number of components in this frame
	if (decoder->CompInFrame < 1 || decoder->CompInFrame > kMaxFrameComponents)
		return kBadSOFMarkerErr;
	
	//	Read the component identifiers, sampling factors and Q table numbers
	MaxHi = -1;
	MaxVi = -1;
	for (nf = 0; nf < decoder->CompInFrame; nf++) {
		cspec = &decoder->Comp[nf];
		cspec->Ci = *data++;		// Component identifier
		i = *data++;
		cspec->Hi = (i >> 4);		// Horizontal blocks in MCU
		cspec->Vi = (i & 0xF);		// Vertical blocks in MCU
		i = *data++;			// Q table to use
		if (i >= kMaxQuantizationTables)
			return kBadSOFMarkerErr;
		
		cspec->QTable = decoder->QTables[i];

		// per scan info
		cspec->inScan = false;
		cspec->blocksMCU = cspec->Hi*cspec->Vi;
		totalMCUs += cspec->blocksMCU;
		
		if (totalMCUs > kMaxMCUs) {
			printf("JPEG Unsupported scan interleave > %d blocks",kMaxMCUs);
			return kBadMarkerErr;
		}

		if (MaxHi < cspec->Hi)
			MaxHi = cspec->Hi;
			
		if (MaxVi < cspec->Vi)
			MaxVi = cspec->Vi;
		
		if (cspec->Hi > 2 || cspec->Vi > 2 ||  (cspec->Hi == 1 && cspec->Vi == 2)) {
			printf("JPEG Unsupported scan interleave cspec %d (%d x %d)",	cspec->Ci,cspec->Hi,cspec->Vi);
			return kBadMarkerErr;
		}
	}

	if ((MaxHi <= 0) || (MaxVi <= 0))
		return kBadMarkerErr;

	if (data != dataEnd)
		printf("Bad SOF marker");

	data = (const unsigned char *)-1;	// can't use this anymore since if a block buffer is allocated in the  cach it will be invalid
		
	decoder->WidthMCU = ((decoder->Width + (MaxHi << 3)-1)/(MaxHi << 3));
	decoder->HeightMCU = ((decoder->Height + (MaxVi << 3)-1)/(MaxVi << 3));
	decoder->MCUHPixels = decoder->thumbnail ? MaxHi : MaxHi << 3;
	decoder->MCUVPixels = decoder->thumbnail ? MaxVi : MaxVi << 3;
	
	// dont allocate if we are just checking size
	if (justSizeCheck) {
		decoder->phase = kSkippingToEnd;
		return kNoError;
	}
		
	if (SetupDrawing(decoder) != kNoError)
		return kLowMemory;
	
	// if bounds is empty, we aren't really drawing yet, just getting the dimensions 
	// otherwise setup the params for clipping by slice and MCU 
	if (dstWidth != 0 && dstHeight != 0) {
		BRect srcClip = decoder->mClipRect;
		BRect srcRect = decoder->mDrawRect;

		srcRect.OffsetBy(-srcRect.left,-srcRect.top);
		srcClip.OffsetBy(-decoder->mDrawRect.left,-decoder->mDrawRect.top);
		srcClip = srcRect;
		if (srcClip.Width() == 0 && srcClip.Height() == 0) {
			decoder->firstSlice = decoder->lastSlice = 0;
			decoder->leftMCU = decoder->rightMCU = 0;
			decoder->phase = kSkippingScan;
			return kNoError;
		} else {
			if (decoder->isProgressive) {
				decoder->firstSlice =  0;
				decoder->lastSlice = decoder->Height / decoder->MCUVPixels;
				decoder->leftMCU = 0;
				decoder->rightMCU  = (decoder->Width + decoder->MCUHPixels) / decoder->MCUHPixels;
			} else {
				// translate clip from dest space to source space
				if (dstWidth != decoder->Width || dstHeight != decoder->Height) {
					srcClip.right =  (srcClip.right * decoder->Width) / dstWidth;
					srcClip.bottom =  (srcClip.bottom * decoder->Height) / dstHeight;
					srcClip.left =  (srcClip.left * decoder->Width) / dstWidth;
					srcClip.top =  (srcClip.top * decoder->Height) / dstHeight;
				}

				decoder->firstSlice = (long)(srcClip.top / decoder->MCUVPixels);
				decoder->lastSlice = (long)(srcClip.bottom / decoder->MCUVPixels);
				decoder->leftMCU = (long)(srcClip.left / decoder->MCUHPixels);
				decoder->rightMCU  = (long)(srcClip.right+decoder->MCUHPixels) / decoder->MCUHPixels;
			}
		}
	
		// progressive JPEG requires the DCT coefficients from
		// all previous scans, so we have to buffer them until all passes
		// are drawn. If there isn't enough memory we have to draw
		// all scans of the image muliple times depending on how many
		// slices worth of DCT data we can buffer
	
		// based on number of blocks we can allocate
		// if we can buffer all the DCT blocks for an image we can 
		// draw it much better and make it look better so it would be 
		// nice to be able to get this memory if we can
		if (decoder->isProgressive && !decoder->thumbnail) {
			decoder->firstSlice = 0;		// cant clip off top for progressive
			long heightMCU = decoder->HeightMCU;
			if ((decoder->lastSlice+1) < heightMCU)
				heightMCU = decoder->lastSlice+1;

			// on first pass only, allocate the block buffer
			if (decoder->anotherPass == 0) {	
				if (AllocateBlockBuffer(decoder) != kNoError)
					return kLowMemory;
			}
			
			for (nf = 0; nf < decoder->CompInFrame; nf++) {
				cspec = &decoder->Comp[nf];
				cspec->blockBuffer = (short *)cspec->blockBufferData;
				if (cspec->blockBuffer == NULL) {
					printf("Zero blockBuffer for progressive JPEG");
					return kLowMemory;
				}
				memset(cspec->blockBuffer,0,cspec->blockCount * kMCUSize * sizeof(*cspec->blockBuffer));
			}
		}
	}
	
	// final slice clipping info
	decoder->firstMCU = (decoder->firstSlice * decoder->WidthMCU) + decoder->leftMCU;
	decoder->lastMCU = (decoder->lastSlice * decoder->WidthMCU) + decoder->rightMCU;

	decoder->phase = kWaitingForScan;					// Got the start of frame
	DumpJPEGDecode(decoder);
	return kNoError;
}

//	SOS defines the number of image components in the scan
//	Defines which component will be the jth component in the scan,
//	along with the DC and AC huffman table number to use with each component
JPEGError InterpretSOS(JPEGDecoder *decoder, const uchar *data, short size)
{
	short ns,i,Cs;
	CompSpec* cspec;
	const uchar	*dataEnd = data + size-2;
	
	decoder->CompInScan =	*data++;			// Number of components in this scan
	
	if (decoder->CompInScan > kMaxScanComponents)
		return kBadSOSMarkerErr;
	 
	for (i = 0; i < decoder->CompInFrame; i++) 
		decoder->Comp[i].inScan = false;
		
	for (ns = 0; ns < decoder->CompInScan; ns++) {
		Cs = *data++;		// Component selector
		cspec = NULL;
		for (i=0; i < decoder->CompInFrame; i++) {
			if (decoder->Comp[i].Ci == Cs) {
				cspec = &decoder->Comp[i];
				break;
			}
		}

		if (cspec == NULL) 
			return kBadSOSMarkerErr;

		decoder->scanComps[ns] = cspec;
		i = *data++;
		short htd = (i >> 4);		// DC Huffman table number
		short hta = (i & 0xF);		// AC Huffman table number
		if (htd >= kMaxHuffmanTables) 
			return kBadSOSMarkerErr;

		if (hta >= kMaxHuffmanTables) 
			return kBadSOSMarkerErr;
		
		cspec->DCH = (HuffmanTable*)decoder->HTables[htd];
		cspec->ACH = (HuffmanTable*)decoder->HTables[hta + 4];
		cspec->inScan = true;
	}
	
	// Progressive JPEG images have two types of progression -
	// spectrum selection
	//  	this encodes parts of the frequency spectrum in seperate scans
	// successive approximation 
	//		this encodes a base level for each DCT coefficient in a scan
	//		and then successive scans adjust each non-zero coefficient or 
	//		add new bits
	// both can be used at the same time
	if (decoder->isProgressive) {
		uchar succLow,succHigh;
		uchar specStart,specEnd;
		
		specStart = *data++;
		specEnd = *data++;
		i = *data++;
		succHigh = i>>4;
		succLow = i & 0xf;
		if (specStart == 0) {
			if (specEnd != 0)	// Annex G.1.1.1.1		DC components are separate
				return kBadSOSMarkerErr;
			
			for (ns = 0; ns < decoder->CompInScan; ns++) {
				cspec = decoder->scanComps[ns];
				cspec->dcPrevBitPos = succHigh;
				cspec->dcBitPos = succLow;
				cspec->specStart = specStart;
				cspec->specEnd = specEnd;
			}
		} else {
			if (decoder->CompInScan != 1)
				return kBadSOSMarkerErr;
											// Annex G.1.1.1.1		Only dc can be interleaved
			cspec = decoder->scanComps[0];
			cspec->acPrevBitPos = succHigh;
			cspec->acBitPos = succLow;
			cspec->specStart = specStart;
			cspec->specEnd = specEnd;
		}
	} else
		data += 3;		

	if (data != dataEnd)
		return kBadSOSMarkerErr;		
		
	DumpJPEGDecode(decoder);
	
	decoder->phase = kProcessingScan;					// Now in the main scan
	decoder->RstMarker = kSOS;				// Trigger an interval for DecodeScan
	return kNoError;					
}


static const ushort	QScale[64] = {
	 8192,11362,11362,10703,15760,10703, 9632,14845,
	14845, 9632, 8192,13361,13984,13361, 8192, 6436,
	11362,12585,12585,11362, 6436, 4433, 8927,10703,
	11326,10703, 8927, 4433, 2260, 6149, 8409, 9632,
	 9632, 8409, 6149, 2260, 3134, 5792, 7568, 8192,
	 7568, 5792, 3134, 2953, 5213, 6436, 6436, 5213,
	 2953, 2657, 4433, 5057, 4433, 2657, 2260, 3483,
	 3483, 2260, 1775, 2399, 1775, 1223, 1223, 623
};

//	DQT defines the quantization tables
//	A common encoder error is to omit the Q table number
JPEGError InterpretDQT(JPEGDecoder *decoder, const uchar *data, short size)
{
	short q, i;
	ushort *qtable;
	ushort scaledQ;
	const uchar *end = data + size - 2;
	while (data < end) {
		q = *data++;
		i =  (q & 0xf);
		if (i > 3) {
			printf("Bad DQT Marker");
			return kBadMarkerErr;
		}
		qtable = decoder->QTables[i];			// Quantization table number

		i =  (q & 0xf0);
		if (i == 0) {
			for (i = 0; i < kMCUSize; i++) {
				scaledQ = *data++;
				scaledQ = (((long)(scaledQ * QScale[i])) + (1<<(I_SHIFT_F-1))) >> I_SHIFT_F;		// 8 by 8
				qtable[i] = scaledQ;
			}
		} else if (i == 0x10) {
			// these are illegal for 8-bit JPEG, but try to deal with them anyway
			// since some encoders make them
			for (i = 0; i < kMCUSize; i++)  {
				scaledQ = data[0];
				scaledQ <<= 8;
				scaledQ |= data[1];
				data += 2;
				if (scaledQ > 0xff)
					scaledQ = 0xff;
				
				scaledQ = (((long)(scaledQ * QScale[i])) + (1<<(I_SHIFT_F-1))) >> I_SHIFT_F;		// 8 by 8
				qtable[i] = scaledQ;
			}
		} else {
			printf("Bad DQT Marker");
			return kBadMarkerErr;
		} 
	}
	return kNoError;
}

//	MakeFastHuffmanTable
//
//	Creates a 256 entry table that speeds up the huffman decode
//	Each entry in the table may describe an entire symbol, it may
//	describe an full code, of it may fail ot describe either.
//
//	note: could extend this for progressive JPEG successive approximation codes
static void MakeFastHuffmanTable(HuffmanTable *h)
{
	int	l,p,v;
	ushort 	code8,code,s,r;
	
	for (code8 = 0; code8 < 256; code8++) {
		l = 1;
		code = code8 >> 7;								// GetBit from code8
		while ((code > h->maxcode[l]) && (l < 8)) {
			l++;
			code = code8 >> (8-l);
		}

		if ((code > h->maxcode[l]) || (code == 0xFF)) {	// 8 bits did not make a code
			h->fast[code8].length = 0;					// Unknown code
			h->fast[code8].run = 0;						// Unknown run
			h->fast[code8].value = 0;					// Unknown value
		} else {
			p = h->valptr[l] + code - h->mincode[l];	// code was <= 8 bits long
			code = h->huffval[p];
			s = (code & 0xF);							// scale of symbol
			r = (code >> 4);							// run length
			if ((l + s) <= 8) { 						// See if symbol fits in 8 bits
				v = (code8 >> (8 - (l + s))) & (0xFF >> (8 - s));
				h->fast[code8].length = l + s;			// Total length of symbol
				h->fast[code8].run = r;					// Zero run
				h->fast[code8].value = Extend(v,s);		// Decoded symbol value
			} else {
				h->fast[code8].length = -l;				// -ve Total length of symbol
				h->fast[code8].run = r;					// Zero run
				h->fast[code8].value = s;				// value set to scale
			}
		}
	}
}

static JPEGError SetupHuffmanTable(HuffmanTable *h)
{
  	uint16		p,size,code,i;
	uint16		huffcode[257];
	uchar		huffsize[257];
  
	//	Figure C.1 - Generation of Huffman code sizes
	p = 0;
	for (size = 1; size <= 16; size++) {
		for (i = 1; i <= h->bits[size]; i++) {
			if (p > 256)
				return kBadMarkerErr;
			huffsize[p++] = size;
		}
	}

	if (p > 256)
		return kBadMarkerErr;

	huffsize[p] = 0;
	
	//	Figure C.2 - Generation of Huffman codes
	code = 0;
	size = huffsize[0];
	p = 0;
	while (huffsize[p]) {
		while (huffsize[p] == size) {
			if (p > 256)
				return kBadMarkerErr;

			huffcode[p++] = code;
			code++;
		}

		code <<= 1;
		size++;
	}

	//	Figure F.15: Decoder table generation
	p = 0;
	for (size = 1; size <= 16; size++) {
		if (h->bits[size]) {
			h->valptr[size] = p;				// huffval[] index of first value of this size
			h->mincode[size] = huffcode[p];		// minimum code this size
			p += h->bits[size];
			h->maxcode[size] = huffcode[p-1];	// maximum code this size
		} else 
			h->maxcode[size] = -1;
	}
	
	h->maxcode[17] = 0xFFFFF;
	MakeFastHuffmanTable(h);
	return kNoError; 
}


//	DHT defines the huffman tables
JPEGError InterpretDHT(JPEGDecoder *decoder, const uchar *data, short size)
{
	short			n,i,c,Tc,Th;
	HuffmanTable	*h;

	size -= 2;				// Total size of huffman table data
	while (size > 0) {
		i = *data++;
		Tc = (i >> 4);					// Table class
		Th = (i & 0xF);					// Table identifier
		
		if (Tc > 1) {
			printf("Invalid huffman table");
			return kBadMarkerErr;
		}

		if (decoder->isProgressive) {
			if (Th > 3) {
				printf("Invalid huffman table id");
				return kBadMarkerErr;
			}
		} else { 
			if (Th > 1) {
				printf("Invalid huffman table id");
				return kBadMarkerErr;
			}
		}
		
		h = (HuffmanTable*) decoder->HTables[(Tc << 2) + Th];	// Huffman tables (DC 0, DC 1, AC 0, AC 1)
		c = 0;
		for (n = 1; n < 17; n++) {
			i = *data++;				// Number of huffman codes of length i
			h->bits[n] = i;				// fill in bits
			c += i;						// Add to total number of codes				
		}

		if (c > 255)
			return kBadMarkerErr;

		for (n = 0; n < c; n++) 
			h->huffval[n] = *data++;	// Write it to the huffman table

		size -= (17 + c);				// Size of this table
		if (SetupHuffmanTable(h) != kNoError)		// Create tables for decode
			return kBadMarkerErr;
	}

	return kNoError;
}

//	DRI defines the restart interval
JPEGError InterpretDRI(JPEGDecoder *decoder, const uchar *data, short)
{
	decoder->RstInterval = *data++;			// 16 bit restart interval
	decoder->RstInterval = (decoder->RstInterval << 8) | *data++;
	return kNoError;
}

//	APP is an application specific marker
JPEGError InterpretAPP(JPEGDecoder*, const uchar*, short)
{
	// could check for JFIF and use the thumbnail if it's there - but
	// its probably better to strip that during transcoding since
	// we can't depend on it anyway and it takes up space
	return kNoError;
}

//	COM is a comment
JPEGError InterpretCOM(JPEGDecoder*, const uchar*, short)
{
	return kNoError;
}
