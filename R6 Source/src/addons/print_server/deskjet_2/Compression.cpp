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

#ifdef PROTO
#include "../include/Header.h"
#else
#include "Header.h"
#endif
 
Compressor::Compressor(SystemServices* pSys, unsigned int RasterSize, BOOL useseed)
	: pSS(pSys), SeedRow(NULL), UseSeedRow(useseed)
{
	constructor_error=NO_ERROR;
    iRastersReady=0;

    if (!UseSeedRow)
        return;

	SeedRow=pSS->AllocMem(RasterSize);
    CNEWCHECK(SeedRow);
}

Compressor::~Compressor()
{
	pSS->FreeMem(compressBuf);
    if (SeedRow)
        pSS->FreeMem(SeedRow);
}

BYTE* Compressor::NextOutputRaster()
// since we return 1-for-1, just return result first call
{ 
    if (iRastersReady==0)
        return (BYTE*)NULL;
    iRastersReady=0;
    return compressBuf; 
}

Mode9::Mode9(SystemServices* pSys,unsigned int RasterSize)
	: Compressor(pSys,RasterSize, TRUE)
{ 
    if (constructor_error != NO_ERROR)  // if error in base constructor
        return;

// In the worst case, compression expands data by 50%
	compressBuf = (BYTE*)pSS->AllocMem(RasterSize + RasterSize/2); 
	if (compressBuf == NULL)
		constructor_error=ALLOCMEM_ERROR;
    memset(SeedRow,0,RasterSize);
}

Mode9::~Mode9()
{ }


// Mode2 items are kept with DJ890 code

////////////////////////////////////////////////////////////////////////////
typedef union {
	unsigned char comchar;	/* command byte as char */
	struct 
	{
#if defined(_LITTLE_ENDIAN) || defined(LITTLE_ENDIAN_HW)
		unsigned replace_count:3;	/* replace count 1-7, 8=8+next byte */
		unsigned roff:4;	/* relative offset 0-14, 15=15+next byte */
		unsigned type:1;      /* type of replacement: 0=mode0, 1=mode1 */
#else
		unsigned type:1;      /* type of replacement: 0=mode0, 1=mode1 */		
		unsigned roff:4;	/* relative offset 0-14, 15=15+next byte */	
		unsigned replace_count:3;	/* replace count 1-7, 8=8+next byte */
#endif
	} bitf0;
	struct  
	{
#if defined(_LITTLE_ENDIAN) || defined(LITTLE_ENDIAN_HW)
		unsigned replace_count:5;	/* replace count 2-32, 33=33+next byte */
		unsigned roff:2;	/* relative offset 0-2, 3=3+next byte */
		unsigned type:1;      /* type of replacement: 0=mode0, 1=mode1 */
#else	
		unsigned type:1;      /* type of replacement: 0=mode0, 1=mode1 */
		unsigned roff:2;	/* relative offset 0-2, 3=3+next byte */		
		unsigned replace_count:5;	/* replace count 2-32, 33=33+next byte */
#endif
	} bitf1;
} Mode9_comtype;


#define MIN(a,b)	(((a)>=(b))?(b):(a))



#define kPCLMode9			9

#define MAX_OFFSET0 		14  	/* Largest unscaled value an offset can have before extra byte is needed. */
#define OFFSET_START0 		0
#define MAX_COUNT0 			6   	/* Largest unscaled value a count can have before extra byte is needed */
#define COUNT_START0 		1  		/* What a count of zero has a value of. */

#define MAX_OFFSET1 		2
#define OFFSET_START1 		0
#define MAX_COUNT1 			30
#define COUNT_START1 		2

//*********************************************************
// This is based on code that came from Kevin Hudson.

BOOL Mode9::Process(BYTE* input, unsigned int size)
// compresses input data,
// result in Mode9::compressbuf,
// updates compressedsize 
{
    if (input==NULL)    // flushing pipeline
    {
        compressedsize=0;
        iRastersReady=0;
        return FALSE;
    }

    unsigned int originalsize=size;
    unsigned int layer;

    if ((myphase) && (myphase->prev))       // if in pipeline, as opposed to autonomous call
        layer = myphase->prev->Exec->iRastersDelivered;
    else layer = 1;

    layer--;    // using as offset    
	char *sptr = (char*)(&SeedRow[size*layer]);

	BYTE *nptr=input;
	BYTE *tempPtr;
	char last_byte;
	unsigned int 	offset,byte_count,rem_count;
	Mode9_comtype 		command;
	char* dest= (char*)compressBuf;
	register char  		*dptr=dest;

	while ( size > 0 )
	{
		offset = 0;
		
		/* find a difference between the seed row and this row. */
		while ((*sptr++ == *nptr++) && (offset < size) )
		{
			offset++;
		}
		sptr--;
		nptr--;

		if ( offset >= size )	/* too far to find diff, bail */
		  goto bail;

		size -= offset;

		if ((*nptr != nptr[1]) || (size < 2))   /************  if doing a mode 0 **********/
		{
		    command.bitf0.type = 0;
		    last_byte = *nptr++;   /* keep track of the last_byte */
		    sptr++;        /* seed pointer must keep up with nptr */
		    byte_count = 1;

		    /* Now find all of the bytes in a row that don't match
		       either a run of mode3 or mode 1.  A slight
		       optimization here would be to not jump out of this
		       run of mode 0 for a single mode 3 or two mode 1
		       bytes if we have to jump right back into mode 0,
		       especially if there are already 7 mode 0 bytes here
		       (we've already spent the extra byte for the
		       byte-count) */
		    while ((*sptr++ != *nptr) && (last_byte != *nptr) && (byte_count < size))
		    {
			   byte_count++;
			   last_byte = *nptr++;
			}
			sptr--;

		    /* Adjust the count if the last_byte == current_byte.
		       Save these bytes for the upcomming run of mode 1. */
		    if ((byte_count < size) && (last_byte == *nptr))
		    {
				nptr--;  /* Now sptr points to first byte in the new run of mode 1. */
				sptr--;
				byte_count--;
		    }

		    size -= byte_count;
		    /* Now output full command.  If offset is over 14 then
		       need optional offset bytes.  If byte count is over 7
		       then need optional byte count. */

			if (offset > (MAX_OFFSET0+OFFSET_START0))
				command.bitf0.roff = MAX_OFFSET0+1;
			else
				command.bitf0.roff = offset-OFFSET_START0;
			
			if (byte_count > (MAX_COUNT0+COUNT_START0))
				command.bitf0.replace_count = MAX_COUNT0+1;
			else
				command.bitf0.replace_count = byte_count-COUNT_START0;
			
			*dptr++ = command.comchar;

			if (offset > (MAX_OFFSET0+OFFSET_START0))
		    {
				offset -= (MAX_OFFSET0+OFFSET_START0+1);
			 	if (offset == 0)
				{
					*dptr++ = 0;
				}
				else
				{
					 while( offset )
					 {
					   *dptr++ = MIN ( offset, 255 );
					
					   if ( offset == 255 )
					   *dptr++ = 0;
					
						offset -= MIN ( offset, 255 );
					}
				}
			}

			if (byte_count > (MAX_COUNT0+COUNT_START0))
			{
			 	rem_count = byte_count - (MAX_COUNT0+COUNT_START0+1);
			 	if (rem_count == 0)
					*dptr++ = 0;
				else
				{
			 		while( rem_count )
			 		{
				   		*dptr++ = MIN ( rem_count, 255 );
			
				   		if ( rem_count == 255 )
						*dptr++ = 0;
			
				  		 rem_count -= MIN ( rem_count, 255 );
					}
				}
			}
			
			/* Now output the run of bytes.  First set up a pointer to the first source byte. */

			tempPtr = nptr - byte_count;
			for(;byte_count;byte_count--)
			{
				*dptr++ = *tempPtr++;
			}

		} else  	/************ If doing a mode 1 *************/
		{
		    /* mode 1, next two bytes are equal */
		    command.bitf1.type = 1;
		    nptr++;
		    last_byte = *nptr++;
		    byte_count = 2;

		    while ((last_byte == *nptr++) && (byte_count < size))
			{
			   byte_count++;
		    }
			nptr--;
		    sptr += byte_count;
		    size -= byte_count;

			if (offset > (MAX_OFFSET1+OFFSET_START1))
				command.bitf1.roff = MAX_OFFSET1+1;
			else
				command.bitf1.roff = offset-OFFSET_START1;
			
			if (byte_count > (MAX_COUNT1+COUNT_START1))
				command.bitf1.replace_count = MAX_COUNT1+1;
			else
				command.bitf1.replace_count =  byte_count-COUNT_START1;
			
			*dptr++ = command.comchar;
			
			if (offset > (MAX_OFFSET1+OFFSET_START1))
			{
			 	offset -= (MAX_OFFSET1+OFFSET_START1+1);
				if (offset == 0)
				{
				 *dptr++ = 0;
				}
				else
				{
					 while( offset )
					 {
						*dptr++ = MIN ( offset, 255 );
				
						if ( offset == 255 )
							*dptr++ = 0;
				
						offset -= MIN ( offset, 255 );
					}
				}
			}  /* if (offset > MAX...  */

			if (byte_count > (MAX_COUNT1+COUNT_START1))
          	{
				rem_count = byte_count - (MAX_COUNT1+COUNT_START1+1);
				if (rem_count == 0)
					*dptr++ = 0;
				else
				{
					while( rem_count )
					{
						*dptr++ = MIN ( rem_count, 255 );
						
						if ( rem_count == 255 )
							*dptr++ = 0;
						
						rem_count -= MIN ( rem_count, 255 );
					}
				 }
		     }  /* if (byte_count > ... */

		     *dptr++ = last_byte;  /* Now output the repeated byte. */
		}
	}  /* while (size > 0) */

bail:
	size = ( dptr - dest );
    compressedsize = size;
    memcpy(&(SeedRow[layer*originalsize]), input, originalsize);
    iRastersReady=1;

return TRUE;
}

////////////////////////////////////////////////////////////////////

