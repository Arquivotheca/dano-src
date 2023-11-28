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

#define MAX_OUTPUT_RASTERS 32

void ResSynStart ( int WidthInPixels,
				   int ScaleFactorMultiplier,
				   int ScaleFactorDivisor,
//				   int /*CallerAlloc*/,
				   RESSYNSTRUCT *ResSynStruct)
{
	ResSynStruct->Width		    = WidthInPixels+2;		/* add 2 for edges */
	ResSynStruct->ScaleFactorMultiplier = ScaleFactorMultiplier;
	ResSynStruct->ScaleFactorDivisor    = ScaleFactorDivisor;
//	ResSynStruct->CallerAlloc           = 1;                  // allways external
	ResSynStruct->RastersinBuffer       = 0;
	ResSynStruct->Remainder		    = 0;
	ResSynStruct->Repeat		    =   (ScaleFactorMultiplier * 256 / ScaleFactorDivisor)
										  -  ((ScaleFactorMultiplier/ScaleFactorDivisor) * 256);

	ResSynStruct->BufferSize = ( ResSynStruct->Width * NUMBER_PLANES /* RGB format */  
											  * NUMBER_RASTERS); /* Number raster to buffer */
	
} 



Scaler::Scaler(SystemServices* pSys,unsigned int inputwidth,unsigned int numerator,unsigned int denominator)
		: pSS(pSys), iInputWidth(inputwidth)
{

	constructor_error=NO_ERROR;

	ScaleFactor= (float)numerator / (float)denominator;
    if (ScaleFactor > (float)MAX_OUTPUT_RASTERS)      // 
    {
        constructor_error = INDEX_OUT_OF_RANGE;
        return;
    }

	iOutputWidth = (int)(
							 ((float)iInputWidth / (float)denominator) * 
								(float)numerator);
	iOutputWidth++;			// safety measure to protect against roundoff error

	if (numerator == denominator)
	  	scaling=FALSE;
	else scaling=TRUE;

	if (scaling)
	 {
		pRSstruct=(RESSYNSTRUCT*)pSS->AllocMem(sizeof(RESSYNSTRUCT));
		if (pRSstruct==NULL)
			{  
				constructor_error=ALLOCMEM_ERROR; 
				return; 
			}

		ResSynStart(inputwidth,numerator,denominator,pRSstruct);
		pRSstruct->Buffer = (BYTE*)pSS->AllocMem(pRSstruct->BufferSize); 
		if (pRSstruct->Buffer == NULL)
			{  
				pSS->FreeMem((BYTE*)pRSstruct);
				constructor_error=ALLOCMEM_ERROR; 
				return; 
			}
	  }
	else pRSstruct=NULL;
	
	// ScaleBound=max number of output rows per input row; 
	// i.e., if scale=4.28, then sometimes 5 rows will come out

	int ScaleBound = int(ScaleFactor);
	if  (ScaleFactor > (float) ScaleBound)
		ScaleBound++;

	// allocate a buffer for one output row, in the job_struct
	int RSBuffSize= (int)(((float)(3*iOutputWidth)) * ScaleBound );
	pOutputBuffer=(BYTE*)pSS->AllocMem(RSBuffSize);
	if (pOutputBuffer == NULL)
	   {  
			if (scaling)
			  {
				pSS->FreeMem(pRSstruct->Buffer);
				pSS->FreeMem((BYTE*)pRSstruct);
			  }
			constructor_error=ALLOCMEM_ERROR; 
			return; 
		}

	if (ScaleFactor < 2.0)
		ReplicateOnly = TRUE;
	else ReplicateOnly = FALSE;
		
}

Scaler::~Scaler()
{

	if (pRSstruct)
	  {
		pSS->FreeMemory(pRSstruct->Buffer);
		pSS->FreeMem((BYTE*)pRSstruct);	
	  }
	pSS->FreeMemory(pOutputBuffer);
}

BOOL Scaler::Process(BYTE* raster_in, unsigned int size)
// returns TRUE if rasters ready (see iRastersReady)
// raster_in=NULL when flushing pipeline at end
// no errors returned
{
    iRastersDelivered=0;

	if (!scaling)
	  {
		// just copy to output buffer
		if (raster_in == NULL)
			return FALSE;
		memcpy(pOutputBuffer, raster_in, iOutputWidth*3);
		iRastersReady = 1;
        return TRUE;
	  }
		
	if (!ReplicateOnly)
    {
		iRastersReady = ResSyn(raster_in);
        return (iRastersReady>0);
    }

	// new code to work for scale between 1 and 2
	if (raster_in == NULL)
			return FALSE;
	
	// first copy the data into our buffer, as would have been done by "rez_synth"
	memcpy(pOutputBuffer,raster_in, iInputWidth*3);

	
	iRastersReady = create_out(TRUE);	// simple replication
    return TRUE;

}


unsigned int Scaler::GetOutputWidth()
{ 
	return (iOutputWidth-1)*3;	// we padded it in case of roundoff error
}


BYTE* Scaler::NextOutputRaster()
{
    unsigned int offset= iRastersDelivered;
    if (iRastersReady==0)
        return (BYTE*)NULL;

    iRastersReady--;
    iRastersDelivered++;

    return &(pOutputBuffer[offset * (iOutputWidth-1)*3]);// we padded iOutputWidth in case of roundoff error
}

unsigned int Scaler::ResSyn(const unsigned char *raster_in)
{
	int i;

   if(pRSstruct->RastersinBuffer == 0)
   {
	if(raster_in == NULL)  // were finished
		return 0;

	// else this is the first time through so Initilize your variables etc
	
     InitInternals(); 

	/* Set up internal raster pointers */
	 pRSstruct->Bufferpt[0]=pRSstruct->Buffer;
	 pRSstruct->Bufferpt[1]=pRSstruct->Buffer+(pRSstruct->Width * NUMBER_PLANES);
	 pRSstruct->Bufferpt[2]=pRSstruct->Buffer+(2*pRSstruct->Width * NUMBER_PLANES);

/* pad the first raster */  
	for ( i = 0; i < 2; i++ )
	{
		memcpy(pRSstruct->Bufferpt[i],raster_in, NUMBER_PLANES);	  // replicate first pixel
		memcpy((pRSstruct->Bufferpt[i]+NUMBER_PLANES),raster_in, (pRSstruct->Width-2) * NUMBER_PLANES);
		memcpy((pRSstruct->Bufferpt[i]+((pRSstruct->Width-1)* NUMBER_PLANES)) ,
										  (raster_in+((pRSstruct->Width-3) * NUMBER_PLANES)), NUMBER_PLANES);																				
	}
	
	pRSstruct->RastersinBuffer       = 2;
	return 0;
   
   } /* end read filter */
   else if(pRSstruct->RastersinBuffer == 2)
   {
	   /* copy the new input raster to the 3rd buffered raster and call RS */
    if(raster_in != NULL)
	{
		memcpy((pRSstruct->Bufferpt[2]+NUMBER_PLANES),raster_in, (pRSstruct->Width-2) * NUMBER_PLANES);	  // replicate first pixel
		memcpy(pRSstruct->Bufferpt[2],raster_in, NUMBER_PLANES);	  // replicate first pixel
		memcpy((pRSstruct->Bufferpt[2]+((pRSstruct->Width-1)   // replicate  last pixel
										  * NUMBER_PLANES)) ,(raster_in+((pRSstruct->Width-3) 
																		 * NUMBER_PLANES)), NUMBER_PLANES);				
		pRSstruct->RastersinBuffer = 3;
	}
	else // raster_in == NULL copy last raster to third
	{
		memcpy((pRSstruct->Bufferpt[2]),	(pRSstruct->Bufferpt[1]),pRSstruct->Width * NUMBER_PLANES);
		pRSstruct->RastersinBuffer = 0;	
	}

	rez_synth(pRSstruct, pOutputBuffer);
	return(create_out(FALSE));
   	
   }
   else if(pRSstruct->RastersinBuffer == 3)
   {
	 /* update the buffer pointers and place the new raster in the 3rd slot */
	 unsigned char* temp_ptr = pRSstruct->Bufferpt[0]; 
	 pRSstruct->Bufferpt[0]= pRSstruct->Bufferpt[1];
	 pRSstruct->Bufferpt[1]= pRSstruct->Bufferpt[2];
	 pRSstruct->Bufferpt[2]= temp_ptr;

	if(raster_in != NULL)
	{ 	
		memcpy((pRSstruct->Bufferpt[2]+NUMBER_PLANES),raster_in,(pRSstruct->Width-2) * NUMBER_PLANES);  
		memcpy(pRSstruct->Bufferpt[2],raster_in,NUMBER_PLANES);
		memcpy((pRSstruct->Bufferpt[2]+((pRSstruct->Width-1)   // replicate  last pixel
										  * NUMBER_PLANES)) ,(raster_in+((pRSstruct->Width-3) 
																		 * NUMBER_PLANES)), NUMBER_PLANES);			
		pRSstruct->RastersinBuffer = 3;

	}
    else // raster_in == NULL copy last raster to third
	{
		memcpy((pRSstruct->Bufferpt[2]),	(pRSstruct->Bufferpt[1]),pRSstruct->Width * NUMBER_PLANES);
		pRSstruct->RastersinBuffer = 0;	
	}

	rez_synth( pRSstruct, pOutputBuffer);
	return(create_out(FALSE));

   }

  return 0;
}
	   
int Scaler::create_out(BOOL simple)
{


	int factor;
	if (simple)		// just replicating up to 1<n<2
		factor=1;
	else factor=2;	// already doubled by rez_synth

	long lx;
	int number_out;
	unsigned char *output_raster[MAX_OUTPUT_RASTERS];       // max of 8x output rasters 
	

	// Determine the number of output rasters you need
	int out_width = (( (pRSstruct->Width-2) * pRSstruct->ScaleFactorMultiplier) 
						 / pRSstruct->ScaleFactorDivisor); 

	number_out = pRSstruct->ScaleFactorMultiplier / pRSstruct->ScaleFactorDivisor; 
	if(number_out == 2 && !(pRSstruct->ScaleFactorMultiplier % pRSstruct->ScaleFactorDivisor) ) //Were allready finished
	{
		return(2);
	}

	if (pRSstruct->ScaleFactorMultiplier % pRSstruct->ScaleFactorDivisor)
	{
		pRSstruct->Remainder = pRSstruct->Remainder + pRSstruct->Repeat;  

		if(pRSstruct->Remainder >= 256)	//send extra raster
		{
			number_out++;
			pRSstruct->Remainder = pRSstruct->Remainder - 256; 
		}
	}

	// set up pointers into the output buffer
	int i; 
	
	for (i = 0; i < number_out; i++)
		output_raster[i] = (pOutputBuffer + (out_width*i*NUMBER_PLANES));

	// convert the input data by starting at the bottom right corner and move left + up
	int x_index, y_index;
	for(i = (number_out-1); i>=0 ; i--)
	{
		y_index = factor*i*pRSstruct->ScaleFactorDivisor/pRSstruct->ScaleFactorMultiplier; 
		for ( lx = out_width-1; lx >= 0; --lx )
		{
			x_index =  factor * lx *pRSstruct->ScaleFactorDivisor/pRSstruct->ScaleFactorMultiplier;
			*(output_raster[i]+lx*NUMBER_PLANES)  = *(output_raster[y_index]+ x_index*NUMBER_PLANES); 
			*(output_raster[i]+lx*NUMBER_PLANES+1)= *(output_raster[y_index]+ x_index*NUMBER_PLANES+1);
			*(output_raster[i]+lx*NUMBER_PLANES+2)= *(output_raster[y_index]+ x_index*NUMBER_PLANES+2); 		
		}
	}
		return( number_out);
}


void Scaler::Pixel_ReplicateF(int color, int h_offset, int v_offset, unsigned char **out_raster, int plane)
{
	*(out_raster[v_offset]+(h_offset*NUMBER_PLANES)+plane)        = color;
	*(out_raster[v_offset+1]+(h_offset*NUMBER_PLANES)+plane)      = color;
	*(out_raster[v_offset]+((h_offset+1)*NUMBER_PLANES)+plane)    = color;
	*(out_raster[v_offset+1]+((h_offset+1)*NUMBER_PLANES)+plane)  = color;
}
