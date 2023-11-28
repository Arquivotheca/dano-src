//
//  Copyright (c) 2000, Hewlett-Packard Co.
//  All rights reserved.
//  
//  This software is licensed solely for use with HP products.  Redistribution
//  and use with HP products in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//  
//  -	Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//  -	Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//  -	Neither the name of Hewlett-Packard nor the names of its contributors
//      may be used to endorse or promote products derived from this software
//      without specific prior written permission.
//  -	Redistributors making defect corrections to source code grant to
//      Hewlett-Packard the right to use and redistribute such defect
//      corrections.
//  
//  This software contains technology licensed from third parties; use with
//  non-HP products is at your own risk and may require a royalty.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
//  CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL HEWLETT-PACKARD OR ITS CONTRIBUTORS
//  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
//  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
//  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//  DAMAGE.
//

#ifndef _compression_
#define _compression_

#define kUseOldBert 0


#define kBertDecompressPixelSize 3

// Follows are all the masks for the command byte.
#define kTypeMask			0x80
#define kTypeShiftAmount	7

#define kCacheLiteralBitsMask 0x60
#define kCacheLiteralBitsShiftAmount 5

#define kCacheBitsMask 0x60
#define kCacheBitsShiftAmount 5

#define kRoffMask			0x18
#define kRoffShiftAmount	3

#define kReplace_countMask	0x07

// Now have the compiler check to make sure none of the masks overlap/underlap bits accidently.
#if ((kTypeMask | kCacheLiteralBitsMask | kRoffMask | kReplace_countMask) != 255)
#error "Your mask bits are messed up!"
#endif

#if ((kTypeMask | kCacheBitsMask | kRoffMask | kReplace_countMask) != 255)
#error "Your mask bits are messed up!"
#endif


enum
{
	eLiteral = 0,
	eRLE = 0x80
};

enum
{
	eeNewPixel = 0x0,
	eeWPixel = 0x20,
	eeNEPixel = 0x40,
	eeCachedColor = 0x60,
};

enum
{
	eNewColor		= 0x0,
	eWestColor		= 0x1,
	eNorthEastColor	= 0x2,
	eCachedColor	= 0x3
};


// Literal
#define M10_MAX_OFFSET0 		2		/* Largest unscaled value an offset can have before extra byte is needed. */
#define M10_MAX_COUNT0 			6   	/* Largest unscaled and unbiased value a count can have before extra byte is needed */
#define M10_COUNT_START0 		1  		/* What a count of zero has a value of. */

// RLE
#define M10_MAX_OFFSET1 		2
#define M10_MAX_COUNT1 			6
#define M10_COUNT_START1 		2

//int BertCompress(unsigned char *currentRow,  int byteCount, unsigned char *dest,  unsigned char *seed, int pixelSizeInBytes);
int BertCompress(unsigned char *currentRow, unsigned char *seed, unsigned char *dest, int byteCount, int pixelSizeInBytes);
unsigned int decodeDataToPixel(const unsigned char *compressedDataPtr, int *CBI, const unsigned char *seedRowPtr, const int curPixel, int pixelSizeInBytes);
void BertUncompress(const unsigned char *compressedDataPtr,
					unsigned char *seedRowPtr,
					int	numberOfCompressedBytes,
					int pixelSizeInBytes,
					int rowWidthInBytes,
					int *firstPixelThatChanged,
					int *lastPixelThatChanged);
/*unsigned int BertUncompress(const unsigned char *inPtr, 
								int ulInputBytes, 
								int ulOutputBytes, 
								unsigned char *ulSeedPtr,
								unsigned char *ulOutPtr);
*/
#endif // _compression_
