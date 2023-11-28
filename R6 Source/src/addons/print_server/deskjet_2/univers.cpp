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

#ifdef _UNIVERS

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif


// size data referenced by GetSizes()
// must be in ascending order
// also note that Ordinal() returns index based on these values

// Italics is only supported for sizes 6, 10 & 12 so we'll use that
// as a limiting factor for supported sizes
#define UNIVERS_SIZE_1 6
#define UNIVERS_SIZE_2 10
#define UNIVERS_SIZE_3 12

BYTE UniversSizes[]={UNIVERS_SIZE_1, UNIVERS_SIZE_2, UNIVERS_SIZE_3};

// FONT-WIDTH METRICS TABLES defined in printer units
//
// [Univers  6pt Upright Medium ECMA 94 Latin 1]
const BYTE Univers6UpLo[96] = { // chars: 32..127
           9,  9, 13, 16, 16, 25, 19,  9,  9,  9, 16, 25,  9,  9,  9,  9, 
          16, 16, 16, 16, 16, 16, 16, 16, 16, 16,  9,  9, 25, 25, 25, 13, 
          25, 19, 16, 18, 18, 15, 14, 19, 18,  7, 14, 17, 14, 23, 18, 19, 
          15, 20, 16, 16, 16, 18, 18, 25, 18, 17, 15,  9,  9,  9, 16, 13, 
           9, 14, 15, 14, 15, 14,  9, 15, 15,  6,  6, 14,  6, 22, 15, 15, 
          15, 15,  9, 13,  9, 15, 14, 22, 14, 14, 12, 11, 13, 11, 16, 25  };
const BYTE Univers6UpHi[96] = { // chars: 160..255
		   9,  9, 16, 16, 16, 16, 13, 16, 16, 13, 10, 14, 25,  9, 13, 13, 
          16, 25, 10, 10, 16, 14, 16,  2, 16, 10, 10, 14, 25, 25, 25, 13, 
          19, 19, 19, 19, 19, 19, 25, 18, 15, 15, 15, 15,  7,  7,  7,  7, 
          18, 18, 19, 19, 19, 19, 19, 25, 19, 18, 18, 18, 18, 17, 15, 15, 
          14, 14, 14, 14, 14, 14, 21, 14, 14, 14, 14, 14,  6,  6,  6,  6, 
          15, 15, 15, 15, 15, 15, 15, 25, 15, 15, 15, 15, 15, 14, 15, 14  };

// [Univers Italic    6pt Italic Medium ECMA 94 Latin 1]
const BYTE Univers6ItLo[96] = { // chars: 32..127
           9,  9, 13, 16, 16, 25, 19,  9,  9,  9, 16, 25,  9,  9,  9,  9, 
          16, 16, 16, 16, 16, 16, 16, 16, 16, 16,  9,  9, 25, 25, 25, 13, 
          25, 19, 16, 18, 18, 15, 14, 19, 18,  7, 14, 17, 14, 23, 18, 19, 
          15, 20, 16, 16, 16, 18, 18, 25, 18, 17, 15,  9,  9,  9, 16, 13, 
           9, 14, 15, 14, 15, 14,  9, 15, 15,  6,  6, 14,  6, 22, 15, 15, 
          15, 15,  9, 13,  9, 15, 14, 22, 14, 14, 12, 11, 13, 11, 16, 25  };
const BYTE Univers6ItHi[96] = { // chars: 160..255
		   9,  9, 16, 16, 16, 16, 13, 16, 16, 13, 10, 14, 25,  9, 13, 13, 
          16, 25, 10, 10, 16, 14, 16,  2, 16, 10, 10, 14, 25, 25, 25, 13, 
          19, 19, 19, 19, 19, 19, 25, 18, 15, 15, 15, 15,  7,  7,  7,  7, 
          18, 18, 19, 19, 19, 19, 19, 25, 19, 18, 18, 18, 18, 17, 15, 15, 
          14, 14, 14, 14, 14, 14, 21, 14, 14, 14, 14, 14,  6,  6,  6,  6, 
          15, 15, 15, 15, 15, 15, 15, 25, 15, 15, 15, 15, 15, 14, 15, 14  };

// [Univers  10pt Upright Medium ECMA 94 Latin 1]
const BYTE Univers10UpLo[96] = { // chars: 32..127
          14, 14, 21, 26, 26, 41, 32, 14, 14, 14, 26, 41, 14, 14, 14, 14, 
          26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 14, 14, 41, 41, 41, 22, 
          41, 31, 26, 29, 30, 24, 23, 31, 30, 12, 23, 28, 22, 38, 30, 32, 
          25, 32, 27, 27, 26, 30, 30, 41, 30, 28, 25, 14, 14, 14, 26, 21, 
          14, 22, 24, 22, 24, 22, 15, 24, 24, 10, 10, 22, 10, 36, 24, 24, 
          24, 24, 15, 21, 15, 24, 23, 36, 23, 23, 20, 18, 21, 18, 26, 41  };
const BYTE Univers10UpHi[96] = { // chars: 160..255
          14, 14, 26, 26, 26, 26, 21, 26, 26, 21, 17, 23, 41, 14, 21, 21, 
          26, 41, 16, 16, 26, 23, 26,  4, 26, 16, 17, 23, 41, 41, 41, 22, 
          31, 31, 31, 31, 31, 31, 41, 29, 24, 24, 24, 24, 12, 12, 12, 12, 
          30, 30, 32, 32, 32, 32, 32, 41, 32, 30, 30, 30, 30, 28, 25, 25, 
          22, 22, 22, 22, 22, 22, 35, 22, 22, 22, 22, 22, 10, 10, 10, 10, 
          24, 24, 24, 24, 24, 24, 24, 41, 24, 24, 24, 24, 24, 23, 24, 23  };

// [Univers Italic    10pt Italic Medium ECMA 94 Latin 1]
const BYTE Univers10ItLo[96] = { // chars: 32..127
          14, 14, 21, 26, 26, 41, 32, 14, 14, 14, 26, 41, 14, 14, 14, 14, 
          26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 14, 14, 41, 41, 41, 22, 
          41, 31, 26, 29, 30, 24, 23, 31, 30, 12, 23, 28, 22, 38, 30, 32, 
          25, 32, 27, 27, 26, 30, 30, 41, 30, 28, 25, 14, 14, 14, 26, 21, 
          14, 22, 24, 22, 24, 22, 15, 24, 24, 10, 10, 22, 10, 36, 24, 24, 
          24, 24, 15, 21, 15, 24, 23, 36, 23, 23, 20, 18, 21, 18, 26, 41  };
const BYTE Univers10ItHi[96] = { // chars: 160..255
          14, 14, 26, 26, 26, 26, 21, 26, 26, 21, 17, 23, 41, 14, 21, 21, 
          26, 41, 16, 16, 26, 23, 26,  4, 26, 16, 17, 23, 41, 41, 41, 22, 
          31, 31, 31, 31, 31, 31, 41, 29, 24, 24, 24, 24, 12, 12, 12, 12, 
          30, 30, 32, 32, 32, 32, 32, 41, 32, 30, 30, 30, 30, 28, 25, 25, 
          22, 22, 22, 22, 22, 22, 35, 22, 22, 22, 22, 22, 10, 10, 10, 10, 
          24, 24, 24, 24, 24, 24, 24, 41, 24, 24, 24, 24, 24, 23, 24, 23  };

// [Univers  12pt Upright Medium ECMA 94 Latin 1]
const BYTE Univers12UpLo[96] = { // chars: 32..127
          17, 17, 25, 31, 31, 50, 38, 17, 17, 17, 31, 50, 17, 17, 17, 17, 
          31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 17, 17, 50, 50, 50, 26, 
          50, 37, 31, 35, 36, 29, 28, 37, 36, 14, 28, 33, 27, 45, 36, 38, 
          30, 39, 32, 32, 31, 36, 36, 50, 36, 34, 30, 17, 17, 17, 31, 25, 
          17, 27, 29, 27, 29, 27, 18, 29, 29, 12, 12, 27, 12, 43, 29, 29, 
          29, 29, 18, 25, 18, 29, 28, 43, 28, 28, 24, 22, 25, 22, 31, 50  };
const BYTE Univers12UpHi[96] = { // chars: 160..255
          17, 17, 31, 31, 31, 31, 25, 31, 31, 25, 20, 28, 50, 17, 25, 25, 
          31, 50, 19, 19, 31, 28, 31,  4, 31, 19, 20, 28, 50, 50, 50, 26, 
          37, 37, 37, 37, 37, 37, 49, 35, 29, 29, 29, 29, 14, 14, 14, 14, 
          36, 36, 38, 38, 38, 38, 38, 50, 38, 36, 36, 36, 36, 34, 30, 30, 
          27, 27, 27, 27, 27, 27, 42, 27, 27, 27, 27, 27, 12, 12, 12, 12, 
          29, 29, 29, 29, 29, 29, 29, 50, 29, 29, 29, 29, 29, 28, 29, 28  };

// [Univers Italic    12pt Italic Medium ECMA 94 Latin 1]
const BYTE Univers12ItLo[96] = { // chars: 32..127
          17, 17, 25, 31, 31, 50, 38, 17, 17, 17, 31, 50, 17, 17, 17, 17, 
          31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 17, 17, 50, 50, 50, 26, 
          50, 37, 31, 35, 36, 29, 28, 37, 36, 14, 28, 33, 27, 45, 36, 38, 
          30, 39, 32, 32, 31, 36, 36, 50, 36, 34, 30, 17, 17, 17, 31, 25, 
          17, 27, 29, 27, 29, 27, 18, 29, 29, 12, 12, 27, 12, 43, 29, 29, 
          29, 29, 18, 25, 18, 29, 28, 43, 28, 28, 24, 22, 25, 22, 31, 50  };
const BYTE Univers12ItHi[96] = { // chars: 160..255
          17, 17, 31, 31, 31, 31, 25, 31, 31, 25, 20, 28, 50, 17, 25, 25, 
          31, 50, 19, 19, 31, 28, 31,  4, 31, 19, 20, 28, 50, 50, 50, 26, 
          37, 37, 37, 37, 37, 37, 49, 35, 29, 29, 29, 29, 14, 14, 14, 14, 
          36, 36, 38, 38, 38, 38, 38, 50, 38, 36, 36, 36, 36, 34, 30, 30, 
          27, 27, 27, 27, 27, 27, 42, 27, 27, 27, 27, 27, 12, 12, 12, 12, 
          29, 29, 29, 29, 29, 29, 29, 50, 29, 29, 29, 29, 29, 28, 29, 28  };



Univers::Univers(BYTE size, BOOL bold, BOOL italic, 
				 BOOL underline,TEXTCOLOR color, unsigned int SizesAvailable)
	: ReferenceFont(SizesAvailable,size,bold,italic,underline,color)
{ 

	// mitigate the requested text size with available text size
	iPointsize=AssignSize(size); 

	if (bItalic)
	 {
		pWidthLo[0]=Univers6ItLo; pWidthHi[0]=Univers6ItHi;
		pWidthLo[1]=Univers10ItLo; pWidthHi[1]=Univers10ItHi;
		pWidthLo[2]=Univers12ItLo; pWidthHi[2]=Univers12ItHi;
	  }
	else
	  {
		pWidthLo[0]=Univers6UpLo; pWidthHi[0]=Univers6UpHi;
		pWidthLo[1]=Univers10UpLo; pWidthHi[1]=Univers10UpHi;
		pWidthLo[2]=Univers12UpLo; pWidthHi[2]=Univers12UpHi;
	  }

}

Univers::Univers(const Univers& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline)
	: ReferenceFont(f,bSize,color,bold,italic,underline)
{ 
	// mitigate the requested text size with available text size
	iPointsize=AssignSize(bSize); 

	if (bItalic)
	 {
		pWidthLo[0]=Univers6ItLo; pWidthHi[0]=Univers6ItHi;
		pWidthLo[1]=Univers10ItLo; pWidthHi[1]=Univers10ItHi;
		pWidthLo[2]=Univers12ItLo; pWidthHi[2]=Univers12ItHi;
	  }
	else
	  {
		pWidthLo[0]=Univers6UpLo; pWidthHi[0]=Univers6UpHi;
		pWidthLo[1]=Univers10UpLo; pWidthHi[1]=Univers10UpHi;
		pWidthLo[2]=Univers12UpLo; pWidthHi[2]=Univers12UpHi;
	  }

}

int Univers::Ordinal(unsigned int pointsize) const
{
	switch(pointsize)
	{
		case(UNIVERS_SIZE_1): return 0;
		case(UNIVERS_SIZE_2): return 1;
		case(UNIVERS_SIZE_3): return 2;
		default: return -1;
	}
}

#endif // _UNIVERS
