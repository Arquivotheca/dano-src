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
#include "../../include/Header.h"
#include "../scaler_open.h"
#else
#include "Header.h"
#include "scaler_open.h"
#endif


Scaler_Open::Scaler_Open(SystemServices* pSys,int inputwidth,int numerator,int denominator)
	: Scaler(pSys,inputwidth,numerator,denominator)
{ }

Scaler_Open::~Scaler_Open()
{ }

void Scaler_Open::rez_synth(RESSYNSTRUCT *ResSynStruct, unsigned char *raster_out)
{
  int i, j;
  int rez_r, rez_g, rez_b, h_offset, v_offset;
  int rcolor, gcolor, bcolor;
  int index;
  unsigned char *output_raster[2], **input_raster;
  
  // set up pointers into the output buffer
  int out_width = (( (ResSynStruct->Width-2) * ResSynStruct->ScaleFactorMultiplier) 
						 / ResSynStruct->ScaleFactorDivisor); 
	
  output_raster[0] = raster_out;
  output_raster[1] = (raster_out + (out_width*NUMBER_PLANES));

  input_raster = ResSynStruct->Bufferpt;
  
  /* leave a border of 1 for the filter size of 3 */
  
    i = 1;                                   // only RS the middle row
    for(j = 1; j < ResSynStruct->Width-1 ; j++) 
	{
      
	  index = j * NUMBER_PLANES;
      rcolor = *(input_raster[i]+index);   
      gcolor = *(input_raster[i]+index+1); 
      bcolor = *(input_raster[i]+index+2); 
      
      rez_r = 1;
      rez_g = 1;
      rez_b = 1;
      h_offset = 2*j-2;	  // Subtract 2 from the index into the high res image to compensate for border
      v_offset = 0;
      
	  rez_r = rez_g = rez_b = 0;
 
      if(rez_r == 0)
		Pixel_ReplicateF(rcolor, h_offset, 0, output_raster, 0);
      if(rez_g == 0)
		Pixel_ReplicateF(gcolor, h_offset, 0, output_raster, 1); 
      if(rez_b == 0)
		Pixel_ReplicateF(bcolor, h_offset, 0, output_raster, 2); 
	}
}	
