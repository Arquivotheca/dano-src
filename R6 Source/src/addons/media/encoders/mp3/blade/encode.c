/*
			(c) Copyright 1998, 1999 - Tord Jansson
			=======================================

		This file is part of the BladeEnc MP3 Encoder, based on
		ISO's reference code for MPEG Layer 3 compression, and might
		contain smaller or larger sections that are directly taken
		from ISO's reference code.

		All changes to the ISO reference code herein are either
		copyrighted by Tord Jansson (tord.jansson@swipnet.se)
		or sublicensed to Tord Jansson by a third party.

	BladeEnc is free software; you can redistribute this file
	and/or modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

*/

#include "common.h"
#include "encoder.h"



/*____ rebuffer_audio() __________________________________________________*/

void rebuffer_audio( short buffer[2][1152], short * insamp, unsigned long samples_read, int stereo )
{
  int j;

  if(stereo == 2)
	{ 

		for(j=0;j<1152;j++) 
		{
	    buffer[0][j] = insamp[2*j];
	    buffer[1][j] = insamp[2*j+1];
		}
  }
  else
	{		
		for(j=0;j<1152;j++)
		{
			buffer[0][j] = insamp[j];
			buffer[1][j] = 0;
		}
  }

  return;
}


/************************************************************************
*
* create_ana_filter()
*
* PURPOSE:  Calculates the analysis filter bank coefficients
*
* SEMANTICS:
* Calculates the analysis filterbank coefficients and rounds to the
* 9th decimal place accuracy of the filterbank tables in the ISO
* document.  The coefficients are stored in #filter#

************************************************************************/

void create_ana_filter(double filter[SBLIMIT][64])
{
   register int i,k;

   for (i=0; i<32; i++)
      for (k=0; k<64; k++) {
          if ((filter[i][k] = 1e9*cos((double)((2*i+1)*(16-k)*PI64))) >= 0)
             modf(filter[i][k]+0.5, &filter[i][k]);
          else
             modf(filter[i][k]-0.5, &filter[i][k]);
          filter[i][k] *= 1e-9;
   }
}



/************************************************************************
*
* filter_subband()
*
* PURPOSE:  Calculates the analysis filter bank coefficients
*
* SEMANTICS:
*      The windowed samples #z# is filtered by the digital filter matrix #m#
* to produce the subband samples #s#. This done by first selectively
* picking out values from the windowed samples, and then multiplying
* them by the filter matrix, producing 32 subband samples.
*
************************************************************************/

int	fInit_windowFilterSubband;

/*____ windowFilterSubband() ____________________________________________*/

void  windowFilterSubband( short ** buffer, int k, double s[SBLIMIT] )
{
 typedef double MM[SBLIMIT][64];

 double y[64];
 int    i,j;

  double  t;    /* TJ */
  double *  rpMM;    /* TJ */

  static int off[2];


  typedef double XX[2][512];

  static MM m;
  static XX x;


  if (!fInit_windowFilterSubband) 
  {
		off[0] = 0;
		off[1] = 0;
    for (i=0;i<2;i++)
      for (j=0;j<512;j++)
        x[i][j] = 0;

    create_ana_filter(m);
    fInit_windowFilterSubband = 1;
  }

  /* replace 32 oldest samples with 32 new samples */
  for ( i=0 ; i<32 ; i++ )
		x[k][31-i+off[k]] = (double) *(*buffer)++/SCALE;


  
  for( i = 0 ; i<64 ; i++ )
  {
    t =  x[k][(i+64*0+off[k])&(512-1)] * enwindow[i+64*0];
    t += x[k][(i+64*1+off[k])&(512-1)] * enwindow[i+64*1];
    t += x[k][(i+64*2+off[k])&(512-1)] * enwindow[i+64*2];
    t += x[k][(i+64*3+off[k])&(512-1)] * enwindow[i+64*3];
    t += x[k][(i+64*4+off[k])&(512-1)] * enwindow[i+64*4];
    t += x[k][(i+64*5+off[k])&(512-1)] * enwindow[i+64*5];
    t += x[k][(i+64*6+off[k])&(512-1)] * enwindow[i+64*6];
    t += x[k][(i+64*7+off[k])&(512-1)] * enwindow[i+64*7];      
    y[i] = t;
  }

  off[k] += 480;              /*offset is modulo (HAN_SIZE-1)*/
  off[k] &= 512-1;



  rpMM = (double *) (m);

  for ( i=0 ; i<SBLIMIT ; i++ )
  {
		t = 0;
		for( j = 0 ; j < 64 ; j++ )
			t += (*rpMM++) * y[j];
		s[i] = t;
	}	 
	 
}

