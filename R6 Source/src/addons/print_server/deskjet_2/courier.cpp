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

#ifdef _COURIER

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif


// size data referenced by GetSizes()
// must be in ascending order
// also note that Ordinal() returns index based on these values
#define COURIER_SIZE_1 6
#define COURIER_SIZE_2 12
#define COURIER_SIZE_3 24

BYTE CourierSizes[]={COURIER_SIZE_1, COURIER_SIZE_2, COURIER_SIZE_3};

Courier::Courier(BYTE size, BOOL bold, BOOL italic, 
				 BOOL underline,TEXTCOLOR color, unsigned int SizesAvailable)
	: ReferenceFont(SizesAvailable,size,bold,italic,underline,color) 
{ 
	iPointsize=AssignSize(size); 
	iPitch=GetPitch(iPointsize);

}

Courier::Courier(const Courier& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline)
	: ReferenceFont(f,bSize,color,bold,italic,underline)
{ 
	iPointsize=AssignSize(bSize); 
	iPitch=GetPitch(iPointsize);
}

Courier::~Courier()
{ }


int Courier::Ordinal(unsigned int pointsize) const
{
	switch(pointsize)
	{
		case(COURIER_SIZE_1): return 0;
		case(COURIER_SIZE_2): return 1;
		case(COURIER_SIZE_3): return 2;
		default: return -1;
	}
}

BYTE Courier::GetPitch(const BYTE pointsize) const
{ 
	switch(pointsize)
		{	case 6:		return 20;
			case 12:	return 10;
			case 24:	return 5;
			default: return 0;
		}
}


#ifdef _DJ400

BYTE Courier400Sizes[]={COURIER_SIZE_1, COURIER_SIZE_2};

Courier400::Courier400(BYTE size, 
				 BOOL bold, BOOL italic, BOOL underline)
	: Courier(size,bold,italic,underline,BLACK_TEXT,2)
{
	iPointsize=AssignSize(size);  // overides iPointSize from Courier constructor
}

Courier400::Courier400(const Courier400& f,const BYTE bSize,
		 const TEXTCOLOR color, const BOOL bold,
		 const BOOL italic, const BOOL underline)
	: Courier(f,bSize,color,bold,italic,underline)
{
	iPointsize=AssignSize(bSize);  // overides iPointSize from Courier (copy)-constructor
}

#endif  // _DJ400

#endif // _COURIER
