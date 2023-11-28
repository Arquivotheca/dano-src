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

#ifdef _CGTIMES

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif


// size data referenced by GetSizes()
// must be in ascending order
// also note that Ordinal() returns index based on these values

#define CGTIMES_SIZE_1 6
#define CGTIMES_SIZE_2 8
#define CGTIMES_SIZE_3 10
#define CGTIMES_SIZE_4 12
#define CGTIMES_SIZE_5 14

BYTE CGTimesSizes[]={CGTIMES_SIZE_1, CGTIMES_SIZE_2, CGTIMES_SIZE_3, 
					 CGTIMES_SIZE_4, CGTIMES_SIZE_5};

// FONT-WIDTH METRICS TABLES defined in printer units
//
// [CG Times    6pt  Upright  Medium   ECMA-94 Latin 1]
const BYTE Cgtimes6UpLo[96] = { // chars: 32..127
           8,  9, 12, 13, 12, 22, 20,  9,  9,  9, 12, 22,  9,  9,  9,  9, 
          13, 13, 13, 13, 13, 13, 12, 13, 13, 13,  9,  9, 25, 22, 25, 11, 
          22, 18, 16, 17, 19, 16, 15, 18, 18,  8, 10, 18, 16, 22, 18, 18, 
          15, 18, 17, 14, 15, 18, 18, 24, 18, 18, 16,  8,  9,  9, 13, 13, 
           9, 11, 13, 11, 12, 11,  9, 12, 13,  7,  7, 13,  7, 20, 13, 12, 
          13, 13,  9,  9,  7, 13, 12, 18, 13, 12, 11, 11, 13, 11, 13, 25  }; 
const BYTE Cgtimes6UpHi[96] = { // chars: 160..255
           8,  9, 13, 13, 12, 12, 13, 13, 13, 13, 13, 10, 22,  9, 13, 13, 
          13, 22,  9,  9, 12, 14, 13,  1, 13,  9, 13, 10, 22, 22, 22, 11, 
          18, 18, 18, 18, 18, 18, 22, 17, 16, 16, 16, 16,  8,  8,  8,  8, 
          19, 18, 18, 18, 18, 18, 18, 22, 18, 18, 18, 18, 18, 18, 14, 13, 
          11, 11, 11, 11, 11, 11, 17, 11, 11, 11, 11, 11,  7,  7,  7,  7, 
          12, 13, 12, 12, 12, 12, 12, 22, 13, 13, 13, 13, 13, 12, 13, 12  };

// [CG Times    6pt  Italic   Medium   ECMA-94 Latin 1]
const BYTE Cgtimes6ItLo[96] = { // chars: 32..127
           8,  9, 12, 13, 13, 22, 20,  9,  9,  9, 12, 22,  9,  9,  9,  9, 
          13, 13, 13, 12, 13, 13, 13, 13, 13, 13,  9,  9, 25, 22, 25, 13, 
          22, 15, 15, 17, 18, 16, 15, 18, 19,  9, 11, 17, 14, 21, 17, 18, 
          15, 18, 15, 13, 14, 18, 16, 21, 16, 15, 14,  9,  9,  9, 13, 13, 
           9, 13, 13, 11, 13, 11,  7, 13, 13,  7,  8, 12,  7, 18, 12, 13, 
          13, 13, 10, 10,  7, 13, 11, 17, 11, 11, 10, 11, 13, 11, 13, 25  };
const BYTE Cgtimes6ItHi[96] = { // chars: 160..255
		   8,  9, 13, 13, 12, 13, 13, 13, 13, 13, 13, 12, 22,  9, 13, 13, 
          13, 22,  9,  9, 13, 14, 13,  1, 13,  9, 13, 12, 22, 22, 22, 13, 
          15, 15, 15, 15, 15, 15, 22, 17, 16, 16, 16, 16,  9,  9,  9,  9, 
          18, 17, 18, 18, 18, 18, 18, 22, 18, 18, 18, 18, 18, 15, 15, 13, 
          13, 13, 13, 13, 13, 13, 17, 11, 11, 11, 11, 11,  7,  7,  7,  7, 
          13, 12, 13, 13, 13, 13, 13, 22, 13, 13, 13, 13, 13, 11, 13, 11  };

// [CG Times   8pt Upright Medium ECMA-94 Latin 1]
const BYTE Cgtimes8UpLo[96] = { // chars: 32..127
          10, 11, 15, 17, 17, 30, 26, 11, 11, 11, 17, 30, 11, 11, 11, 11, 
          17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 11, 11, 33, 30, 33, 15, 
          30, 24, 21, 23, 25, 21, 20, 24, 25, 11, 13, 23, 21, 30, 25, 24, 
          19, 24, 22, 18, 21, 25, 24, 31, 24, 24, 22, 11, 11, 11, 17, 17, 
          11, 15, 17, 15, 17, 15, 11, 17, 17,  9,  9, 17,  9, 26, 17, 17, 
          17, 17, 12, 13,  9, 17, 17, 24, 17, 17, 15, 15, 17, 15, 17, 33  };
const BYTE Cgtimes8UpHi[96] = { // chars: 160..255
          10, 11, 17, 17, 17, 17, 17, 17, 17, 17, 17, 14, 30, 11, 17, 17, 
          17, 30, 12, 12, 17, 18, 17,  4, 17, 12, 17, 14, 30, 30, 30, 15, 
          24, 24, 24, 24, 24, 24, 30, 23, 21, 21, 21, 21, 11, 11, 11, 11, 
          25, 25, 24, 24, 24, 24, 24, 30, 24, 25, 25, 25, 25, 24, 19, 17, 
          15, 15, 15, 15, 15, 15, 22, 15, 15, 15, 15, 15,  9,  9,  9,  9, 
          17, 17, 17, 17, 17, 17, 17, 30, 17, 17, 17, 17, 17, 17, 17, 17  };

// [CG Times Italic    8pt Italic Medium ECMA 94 Latin 1]
const BYTE Cgtimes8ItLo[96] = { // chars: 32..127
	      10, 11, 16, 17, 17, 30, 26, 11, 11, 11, 17, 30, 11, 11, 11, 11, 
          17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 11, 11, 33, 30, 33, 17, 
          30, 20, 20, 22, 24, 20, 19, 24, 24, 11, 15, 22, 18, 28, 22, 24, 
          20, 24, 20, 17, 18, 24, 20, 28, 20, 18, 18, 11, 11, 11, 17, 17, 
          11, 17, 17, 15, 17, 15,  9, 17, 17,  9,  9, 15,  9, 24, 17, 17, 
          17, 17, 13, 13,  9, 17, 15, 22, 15, 15, 13, 15, 17, 15, 17, 33  };
const BYTE Cgtimes8ItHi[96] = { // chars: 160..255
          10, 11, 17, 17, 17, 17, 17, 17, 17, 17, 17, 15, 30, 11, 17, 17, 
          17, 30, 12, 12, 17, 18, 17,  4, 17, 12, 17, 15, 30, 30, 30, 17, 
          20, 20, 20, 20, 20, 20, 30, 22, 20, 20, 20, 20, 11, 11, 11, 11, 
          24, 22, 24, 24, 24, 24, 24, 30, 24, 24, 24, 24, 24, 18, 20, 17, 
          17, 17, 17, 17, 17, 17, 22, 15, 15, 15, 15, 15,  9,  9,  9,  9, 
          17, 17, 17, 17, 17, 17, 17, 30, 17, 17, 17, 17, 17, 15, 17, 15  };

// [CG Times  10pt Upright Medium ECMA 94 Latin 1]
const BYTE Cgtimes10UpLo[96] = { // chars: 32..127
	      12, 14, 19, 21, 21, 37, 32, 14, 14, 14, 21, 37, 14, 14, 14, 14, 
          21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 14, 14, 41, 37, 41, 18, 
          37, 30, 26, 28, 31, 26, 25, 30, 31, 14, 16, 29, 26, 38, 31, 30, 
          24, 30, 28, 22, 26, 31, 30, 39, 30, 30, 27, 14, 14, 14, 21, 21, 
          14, 18, 21, 18, 21, 18, 14, 21, 21, 12, 12, 21, 12, 32, 21, 21, 
          21, 21, 15, 16, 12, 21, 21, 30, 21, 21, 18, 18, 21, 18, 21, 41  };
const BYTE Cgtimes10UpHi[96] = { // chars: 160..255
          12, 14, 21, 21, 21, 21, 21, 21, 21, 21, 21, 17, 37, 14, 21, 21, 
          21, 37, 15, 15, 21, 23, 21,  4, 21, 15, 21, 17, 37, 37, 37, 18, 
          30, 30, 30, 30, 30, 30, 37, 28, 26, 26, 26, 26, 14, 14, 14, 14, 
          31, 31, 30, 30, 30, 30, 30, 37, 30, 31, 31, 31, 31, 30, 24, 21, 
          18, 18, 18, 18, 18, 18, 28, 18, 18, 18, 18, 18, 12, 12, 12, 12, 
          21, 21, 21, 21, 21, 21, 21, 37, 21, 21, 21, 21, 21, 21, 21, 21  };

// [CG Times Italic   10pt Italic Medium ECMA 94 Latin 1]
const BYTE Cgtimes10ItLo[96] = { // chars: 32..127
	      12, 14, 20, 21, 21, 37, 32, 14, 14, 14, 21, 37, 14, 14, 14, 14, 
          21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 14, 14, 41, 37, 41, 21, 
          37, 25, 25, 28, 30, 25, 24, 30, 30, 14, 18, 28, 23, 35, 28, 30, 
          25, 30, 25, 21, 23, 30, 25, 35, 25, 23, 23, 14, 14, 14, 21, 21, 
          14, 21, 21, 18, 21, 18, 12, 21, 21, 12, 12, 18, 12, 30, 21, 21, 
          21, 21, 16, 16, 12, 21, 18, 28, 18, 18, 16, 18, 21, 18, 21, 41  };
const BYTE Cgtimes10ItHi[96] = { // chars: 160..255
          12, 14, 21, 21, 21, 21, 21, 21, 21, 21, 21, 19, 37, 14, 21, 21, 
          21, 37, 15, 15, 21, 23, 21,  4, 21, 15, 21, 19, 37, 37, 37, 21, 
          25, 25, 25, 25, 25, 25, 37, 28, 25, 25, 25, 25, 14, 14, 14, 14, 
          30, 28, 30, 30, 30, 30, 30, 37, 30, 30, 30, 30, 30, 23, 25, 21, 
          21, 21, 21, 21, 21, 21, 28, 18, 18, 18, 18, 18, 12, 12, 12, 12, 
          21, 21, 21, 21, 21, 21, 21, 37, 21, 21, 21, 21, 21, 18, 21, 18  };

// [CG Times   12pt  Upright  Medium   ECMA-94 Latin 1]
const BYTE Cgtimes12UpLo[96] = { // chars: 32..127
	      15, 17, 23, 25, 25, 44, 39, 17, 17, 17, 25, 44, 17, 17, 17, 17, 
          25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 17, 17, 50, 44, 50, 22, 
          44, 36, 31, 34, 37, 31, 30, 36, 37, 17, 19, 35, 31, 45, 37, 36, 
          29, 36, 33, 27, 31, 37, 36, 47, 36, 36, 32, 17, 17, 17, 25, 25, 
          17, 22, 25, 22, 25, 22, 17, 25, 25, 14, 14, 25, 14, 39, 25, 25, 
          25, 25, 18, 19, 14, 25, 25, 36, 25, 25, 22, 22, 25, 22, 25, 50  };
const BYTE Cgtimes12UpHi[96] = { // chars: 160..255
	// note that the extent of char 183 (0) is a correct reflection of the char width
          15, 17, 25, 25, 25, 25, 25, 25, 25, 25, 25, 20, 44, 17, 25, 25, 
		  25, 44, 18, 18, 25, 28, 25,  0, 25, 18, 25, 20, 44, 44, 44, 22,
          36, 36, 36, 36, 36, 36, 44, 34, 31, 31, 31, 31, 17, 17, 17, 17, 
          37, 37, 36, 36, 36, 36, 36, 44, 36, 37, 37, 37, 37, 36, 29, 25, 
          22, 22, 22, 22, 22, 22, 33, 22, 22, 22, 22, 22, 14, 14, 14, 14, 
          25, 25, 25, 25, 25, 25, 25, 44, 25, 25, 25, 25, 25, 25, 25, 25  };

// [CG Times   12pt  Italic   Medium   ECMA-94 Latin 1]
const BYTE Cgtimes12ItLo[96] = { // chars: 32..127
	      15, 17, 24, 25, 25, 44, 39, 17, 17, 17, 25, 44, 17, 17, 17, 17, 
          25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 17, 17, 50, 44, 50, 25, 
          44, 30, 30, 33, 36, 30, 29, 36, 36, 17, 22, 33, 28, 41, 33, 36, 
          30, 36, 30, 25, 28, 36, 30, 41, 30, 28, 28, 17, 17, 17, 25, 25, 
          17, 25, 25, 22, 25, 22, 14, 25, 25, 14, 14, 22, 14, 36, 25, 25, 
          25, 25, 19, 19, 14, 25, 22, 33, 22, 22, 19, 22, 25, 22, 25, 50  };
const BYTE Cgtimes12ItHi[96] = { // chars: 160..255
          15, 17, 25, 25, 25, 25, 25, 25, 25, 25, 25, 23, 44, 17, 25, 25, 
          25, 44, 18, 18, 25, 28, 25,  0, 25, 18, 25, 23, 44, 44, 44, 25, 
          30, 30, 30, 30, 30, 30, 44, 33, 30, 30, 30, 30, 17, 17, 17, 17, 
          36, 33, 36, 36, 36, 36, 36, 44, 36, 36, 36, 36, 36, 28, 30, 25, 
          25, 25, 25, 25, 25, 25, 33, 22, 22, 22, 22, 22, 14, 14, 14, 14, 
          25, 25, 25, 25, 25, 25, 25, 44, 25, 25, 25, 25, 25, 22, 25, 22  };

// [CG Times  14pt Upright Medium ECMA 94 Latin 1]
const BYTE Cgtimes14UpLo[96] = { // chars: 32..127
          17, 19, 27, 29, 29, 52, 45, 19, 19, 19, 29, 52, 19, 19, 19, 19, 
          29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 19, 19, 58, 52, 58, 26, 
          52, 42, 37, 40, 43, 37, 34, 42, 43, 19, 23, 41, 37, 53, 43, 42, 
          33, 42, 39, 31, 37, 43, 42, 55, 42, 42, 38, 19, 19, 19, 29, 29, 
          19, 26, 29, 26, 29, 26, 19, 29, 29, 16, 16, 29, 16, 45, 29, 29, 
          29, 29, 22, 23, 16, 29, 29, 42, 29, 29, 26, 26, 29, 26, 29, 58  };
const BYTE Cgtimes14UpHi[96] = { // chars: 160..255
          17, 19, 29, 29, 29, 29, 29, 29, 29, 29, 29, 24, 52, 19, 29, 29, 
          29, 52, 20, 20, 29, 32, 29,  4, 29, 20, 29, 24, 52, 52, 52, 26, 
          42, 42, 42, 42, 42, 42, 52, 40, 37, 37, 37, 37, 19, 19, 19, 19, 
          43, 43, 42, 42, 42, 42, 42, 52, 42, 43, 43, 43, 43, 42, 33, 29, 
          26, 26, 26, 26, 26, 26, 39, 26, 26, 26, 26, 26, 16, 16, 16, 16, 
          29, 29, 29, 29, 29, 29, 29, 52, 29, 29, 29, 29, 29, 29, 29, 29  };

// [CG Times Italic   14pt Italic Medium ECMA 94 Latin 1]
const BYTE Cgtimes14ItLo[96] = { // chars: 32..127
          17, 19, 28, 29, 29, 52, 45, 19, 19, 19, 29, 52, 19, 19, 19, 19, 
          29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 19, 19, 58, 52, 58, 29, 
          52, 35, 35, 39, 42, 35, 33, 42, 42, 19, 26, 39, 32, 48, 39, 42, 
          35, 42, 35, 29, 32, 42, 35, 48, 35, 32, 32, 19, 19, 19, 29, 29, 
          19, 29, 29, 26, 29, 26, 16, 29, 29, 16, 16, 26, 16, 42, 29, 29, 
          29, 29, 23, 23, 16, 29, 26, 39, 26, 26, 23, 26, 29, 26, 29, 58  };
const BYTE Cgtimes14ItHi[96] = { // chars: 160..255
          17, 19, 29, 29, 29, 29, 29, 29, 29, 29, 29, 27, 52, 19, 29, 29, 
          29, 52, 20, 20, 29, 32, 29,  4, 29, 20, 29, 27, 52, 52, 52, 29, 
          35, 35, 35, 35, 35, 35, 52, 39, 35, 35, 35, 35, 19, 19, 19, 19, 
          42, 39, 42, 42, 42, 42, 42, 52, 42, 42, 42, 42, 42, 32, 35, 29, 
          29, 29, 29, 29, 29, 29, 39, 26, 26, 26, 26, 26, 16, 16, 16, 16, 
          29, 29, 29, 29, 29, 29, 29, 52, 29, 29, 29, 29, 29, 26, 29, 26  };



CGTimes::CGTimes(BYTE size, BOOL bold, BOOL italic, 
				 BOOL underline,TEXTCOLOR color, unsigned int SizesAvailable)
	: ReferenceFont(SizesAvailable,size,bold,italic,underline,color)
{ 

	// mitigate the requested text size with available text size
	iPointsize=AssignSize(size); 

	if (bItalic)
	 {
		pWidthLo[0]=Cgtimes6ItLo; pWidthHi[0]=Cgtimes6ItHi;
		pWidthLo[1]=Cgtimes8ItLo;  pWidthHi[1]=Cgtimes8ItHi;
		pWidthLo[2]=Cgtimes10ItLo; pWidthHi[2]=Cgtimes10ItHi;
		pWidthLo[3]=Cgtimes12ItLo; pWidthHi[3]=Cgtimes12ItHi;
		pWidthLo[4]=Cgtimes14ItLo; pWidthHi[4]=Cgtimes14ItHi;
	  }
	else
	  {
		pWidthLo[0]=Cgtimes6UpLo; pWidthHi[0]=Cgtimes6UpHi;
		pWidthLo[1]=Cgtimes8UpLo; pWidthHi[1]=Cgtimes8UpHi;
		pWidthLo[2]=Cgtimes10UpLo; pWidthHi[2]=Cgtimes10UpHi;
		pWidthLo[3]=Cgtimes12UpLo; pWidthHi[3]=Cgtimes12UpHi;
		pWidthLo[4]=Cgtimes14UpLo; pWidthHi[4]=Cgtimes14UpHi;
	  }

}

CGTimes::CGTimes(const CGTimes& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline)
	: ReferenceFont(f,bSize,color,bold,italic,underline)
{ 
	// mitigate the requested text size with available text size
	iPointsize=AssignSize(bSize); 

	if (bItalic)
	 {
		pWidthLo[0]=Cgtimes6ItLo; pWidthHi[0]=Cgtimes6ItHi;
		pWidthLo[1]=Cgtimes8ItLo;  pWidthHi[1]=Cgtimes8ItHi;
		pWidthLo[2]=Cgtimes10ItLo; pWidthHi[2]=Cgtimes10ItHi;
		pWidthLo[3]=Cgtimes12ItLo; pWidthHi[3]=Cgtimes12ItHi;
		pWidthLo[4]=Cgtimes14ItLo; pWidthHi[4]=Cgtimes14ItHi;
	  }
	else
	  {
		pWidthLo[0]=Cgtimes6UpLo; pWidthHi[0]=Cgtimes6UpHi;
		pWidthLo[1]=Cgtimes8UpLo; pWidthHi[1]=Cgtimes8UpHi;
		pWidthLo[2]=Cgtimes10UpLo; pWidthHi[2]=Cgtimes10UpHi;
		pWidthLo[3]=Cgtimes12UpLo; pWidthHi[3]=Cgtimes12UpHi;
		pWidthLo[4]=Cgtimes14UpLo; pWidthHi[4]=Cgtimes14UpHi;
	  }
}

int CGTimes::Ordinal(unsigned int pointsize) const
{
	switch(pointsize)
	{
		case(CGTIMES_SIZE_1): return 0;
		case(CGTIMES_SIZE_2): return 1;
		case(CGTIMES_SIZE_3): return 2;
		case(CGTIMES_SIZE_4): return 3;
		case(CGTIMES_SIZE_5): return 4;
		default: return -1;
	}
}


#ifdef _DJ400

BYTE CGTimes400Sizes[]={CGTIMES_SIZE_1,CGTIMES_SIZE_4};

CGTimes400::CGTimes400(BYTE size, 
				 BOOL bold, BOOL italic, BOOL underline)
	: CGTimes(size,bold,italic,underline,BLACK_TEXT,2)
{  
	iPointsize=AssignSize(size);  // overides iPointSize from CGTimes constructor
}

CGTimes400::CGTimes400(const CGTimes400& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline)
	: CGTimes(f,bSize,color,bold,italic,underline)
{ 
	iPointsize=AssignSize(bSize);  // overides iPointSize from CGTimes (copy)-constructor
}

int CGTimes400::Ordinal(unsigned int pointsize) const
{
	switch(pointsize)
	{
		case(CGTIMES_SIZE_1): return 0;
		case(CGTIMES_SIZE_4): return 3;
		default: return -1;
	}
}
			
#endif  //_DJ400

#endif // _CGTIMES
