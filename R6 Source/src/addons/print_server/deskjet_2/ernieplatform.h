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

// Endianess of the architecture we're on
#ifdef _LITTLE_ENDIAN
#define bigEndian		0
#define littleEndian	1
#else
#define bigEndian		1
#define littleEndian	0
#endif

// Slow down the system and gather stats or not.
#define kGatherStats	0
#define kDecompressStats 0

// While it may seem like the need to be both
// big endian and little endian versions of
// these macros, due to the face that the
// shift command in general knows about the
// endian order, you get the same result
// from these commands no matter what order
// the bytes are in.  Strange isn't it.
#define GetRed(x) (((x >> 16) & 0x0FF))
#define GetGreen(x) (((x >> 8) & 0x0FF))
#define GetBlue(x) ((x & 0x0FF))

#define MIN(a,b)	(((a)>=(b))?(b):(a))
#define MAX(a,b)    (((a)<=(b))?(b):(a))

#define kWhite 0x00FFFFFE

inline unsigned int get4Pixel(unsigned char *pixAddress);
inline unsigned int get4Pixel(unsigned char *pixAddress)
{
//	slower simple code	
//	unsigned int toReturn = *((unsigned int*)pixAddress); // load toReturn with XRGB
//	toReturn &= kWhite; // Strip off unwanted X. EGW stripped lsb blue.
//	
//	return toReturn;

	return (((unsigned int*)pixAddress)[0]) & kWhite;
}


inline unsigned int get4Pixel(unsigned char *pixAddress, int pixelOffset);
inline unsigned int get4Pixel(unsigned char *pixAddress, int pixelOffset)
{
//	slower simple code
//	unsigned int *uLongPtr = (unsigned int *)pixAddress;
//	uLongPtr += pixelOffset;
//
//	return *uLongPtr & kWhite;

	return ((unsigned int*)pixAddress)[pixelOffset] & kWhite;
}


void put3Pixel(unsigned char *pixAddress, int pixelOffset, unsigned int pixel);

inline void put4Pixel(unsigned char *pixAddress, int pixelOffset, unsigned int pixel);
inline void put4Pixel(unsigned char *pixAddress, int pixelOffset, unsigned int pixel)
{
	(((unsigned int*)pixAddress)[pixelOffset] = pixel & kWhite);
}
