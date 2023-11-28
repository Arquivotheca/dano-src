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

#ifndef _filterHPA_
#define _filterHPA_

/*	flags that track the block formations use bits which are specified in the 
	following enumeration. The first number is the horizontal block size. The 
	second number is the vertical block size. Then north (upper) or south (lower)
	and west (left) or east (right) are specified when appropriate. Finally if a
	location sequence number is needed then it is listed.
*/

enum eBlockType
{
	eDone	= 0x0,
	e11n	= 0x00000001,
	e11s	= 0x00000002,
	e12		= 0x00000004,
	e14n	= 0x00000008,
	e14s	= 0x00000010,
	e21nw	= 0x00000020,
	e21ne	= 0x00000040,
	e21sw	= 0x00000080,
	e21se	= 0x00000100,
	e22w	= 0x00000200,
	e22e	= 0x00000400,
	e24nw	= 0x00000800,
	e24ne	= 0x00001000,
	e24sw	= 0x00002000,
	e24se	= 0x00004000,
	e41ni	= 0x00008000,
	e41n	= 0x00010000,
	e41si	= 0x00020000,
	e41s	= 0x00040000,
	e42i	= 0x00080000,
	e42		= 0x00100000,
	e44ni	= 0x00200000,
	e44n	= 0x00400000,
	e44si	= 0x00800000,
	e44s	= 0x01000000,
	e84ni	= 0x02000000,
	e84n	= 0x04000000,
	e84si	= 0x08000000,
	e84s	= 0x10000000,
	
	eNorths = e11n | e21nw | e21ne | e41ni | e41n,
	eSouths = e11s | e21sw | e21se | e41si | e41s,
	eTheRest = ~(eNorths|eSouths),
	
	eTopLeftOfBlocks = e12 | e14n | e21nw | e21sw | e22w | e24nw | e41ni | e41si | e42i | e44ni | e84ni
};

#define isOdd(x) (x & 0x01)
#define isWhite(x) (((x) & kWhite) == kWhite)

#define kMemWritesOptimize	0	// disables mem write optimizations.

//#ifndef kGatherStats
//#error "must define kGatherStats. Try including platform.h"
//#endif

#if kGatherStats == 1

// Used now to track blocks being formed. These enums are only used in gathering 
// statistics for the developer to look at later. These are not critical to the actual
// functioning of the algorithms.

enum StatisticBlocks
{
	es11n	= 0,
	es11s,
	es12,
	es14n,
	es14s,
	es21nw,
	es21ne,
	es21sw,
	es21se,
	es22w,
	es22e,
	es24nw,
	es24ne,
	es24sw,
	es24se,
	es41ni,
	es41n,
	es41si,
	es41s,
	es42i,
	es42,
	es44ni,
	es44n,
	es44si,
	es44s,
	es84ni,
	es84n,
	es84si,
	es84s,
	
	esDoneStat,
	esWhiteFound,
	
	eLastAveragingFlagPosition
};

#endif

enum pixelTypes
{
	eBGRPixelData = 0
};

class TErnieFilter : public Processor
{
public:
	TErnieFilter(int rowWidthInPixels, pixelTypes pixelType, unsigned int maxErrorForTwoPixels, int bytesPerPixel = 3);
	virtual ~TErnieFilter();

	void submitRowToFilter(unsigned char *rowPtr);
	void writeBufferedRows();

     // Processor interface  /////////////////////////////////////
    BOOL Process(BYTE* InputRaster=NULL, unsigned int size=0);
    void Flush();
    unsigned int GetOutputWidth();
	unsigned int GetInputWidth();
    BYTE* NextOutputRaster();
    /////////////////////////////////////////////////////////////
	
private:
	unsigned int		*fRowBuf[4];
	unsigned char		*fRowPtr[4];
	unsigned char		*fCompressionOutBuf;
	unsigned int		*fPixelFilteredFlags[2];
	
	int					fNumberOfBufferedRows;
	int					fPixelOffset[8];
	int					fPixelOffsetIndex;
	int					fRowWidthInPixels;
	int					fRowWidthInBytes;
	int					fInternalBufferPixelSize;
	int					fOriginalPixelSize;
	unsigned int		fMaxErrorForTwoPixels;
	
	void Filter1RawRow(unsigned char *currPtr, int rowWidthInPixels, unsigned int *flagsPtr);
	void Filter2RawRows(unsigned char *currPtr, unsigned char *upPtr, int rowWidthInPixels, unsigned int *flagsPtr);
	void Filter2PairsOfFilteredRows(unsigned char *row1Ptr, unsigned char *row2Ptr, unsigned char *row3Ptr, unsigned char *row4Ptr);
	void Filter3FilteredRows(unsigned char *row1Ptr, unsigned char *row2Ptr, unsigned char *row3Ptr);
	inline unsigned int DeltaE(int dr0, int dr1, int dg0, int dg1, int db0, int db1);
	inline bool NewDeltaE(int dr0, int dr1, int dg0, int dg1, int db0, int db1, int tolerance);
	inline unsigned int GradDeltaE(int dr, int dg, int db);

#if kMemWritesOptimize == 1
	void WriteBlockPixels();
#endif

	enum
	{
		eBufferedPixelWidthInBytes = 4	
	};
};

#endif
