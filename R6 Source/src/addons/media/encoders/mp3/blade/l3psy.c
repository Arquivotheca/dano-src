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
#include "l3psy.h"
#include "l3side.h"


#define maximum(x,y) ( (x>y) ? x : y )
#define minimum(x,y) ( (x<y) ? x : y )



void read_absthr(float * absthr, int table)
{
	float	*	pSource;
	int		j;

 switch(table)
 {
    case 0 : 
			pSource = absthr_0;
      break;
    case 1 : 
			pSource = absthr_1;
      break;
    case 2 : 
			pSource = absthr_2;
      break;
    default : printf("absthr table: Not valid table number\n");
 }

 for(j=0; j<HBLKSIZE; j++)
    absthr[j] =  pSource[j];
}



void L3para_read( int sfreq, int numlines[CBANDS], int partition_l[HBLKSIZE],
		  double minval[CBANDS], double qthr_l[CBANDS], double norm_l[CBANDS],
		  double s3_l[CBANDS][CBANDS], int partition_s[HBLKSIZE_s], double qthr_s[CBANDS_s],
		  double norm_s[CBANDS_s], double SNR_s[CBANDS_s],
		  int cbw_l[SBMAX_l], int bu_l[SBMAX_l], int bo_l[SBMAX_l],
		  double w1_l[SBMAX_l], double w2_l[SBMAX_l],
		  int cbw_s[SBMAX_s], int bu_s[SBMAX_s], int bo_s[SBMAX_s],
		  double w1_s[SBMAX_s], double w2_s[SBMAX_s] );

int	fInit_L3psycho_anal;
int	init_L3;
 
									
void L3psycho_anal( short int *buffer, short int savebuf[1344], int chn, int lay, float snr32[32],
		    double sfreq, double ratio_d[21], double ratio_ds[12][3],
		    double *pe, gr_info *cod_info )
{
  static double ratio[2][21];
  static double ratio_s[2][12][3];
  int blocktype;
  unsigned int   b, i, j, k;
  double         r_prime, phi_prime; /* not FLOAT */
  float          freq_mult, bval_lo;
  double         temp1,temp2,temp3;

    /*         nint(); Layer III */
  double   thr[CBANDS];

/* The static variables "r", "phi_sav", "new_", "old" and "oldest" have    */
/* to be remembered for the unpredictability measure.  For "r" and        */
/* "phi_sav", the first index from the left is the channel select and     */
/* the second index is the "age" of the data.                             */


  static FLOAT window_s[BLKSIZE_s] ;
	static int     new_ = 0, old = 1, oldest = 0;
	static int     flush, sync_flush, syncsize, sfreq_idx;
	static double 	cw[HBLKSIZE], eb[CBANDS];
	static double 	ctb[CBANDS];
	static double	SNR_l[CBANDS], SNR_s[CBANDS_s];
	static double	minval[CBANDS],qthr_l[CBANDS],norm_l[CBANDS];
	static double	qthr_s[CBANDS_s],norm_s[CBANDS_s];
	static double	nb_1[2][CBANDS], nb_2[2][CBANDS];
	static double	s3_l[CBANDS][CBANDS]; /* s3_s[CBANDS_s][CBANDS_s]; */

/* Scale Factor Bands */
	static int	cbw_l[SBMAX_l],bu_l[SBMAX_l],bo_l[SBMAX_l] ;
	static int	cbw_s[SBMAX_s],bu_s[SBMAX_s],bo_s[SBMAX_s] ;
	static double	w1_l[SBMAX_l], w2_l[SBMAX_l];
	static double	w1_s[SBMAX_s], w2_s[SBMAX_s];
	static double	en[SBMAX_l],   thm[SBMAX_l] ;
	static int	blocktype_old[2] ;
	int	sb,sblock;
	static int	partition_l[HBLKSIZE],partition_s[HBLKSIZE_s];


/* The following static variables are constants.                           */


	static float  crit_band[27] = {0,  100,  200, 300, 400, 510, 630,  770,
                               920, 1080, 1270,1480,1720,2000,2320, 2700,
                              3150, 3700, 4400,5300,6400,7700,9500,12000,
                             15500,25000,30000};


/* The following pointer variables point to large areas of memory         */
/* dynamically allocated by the mem_alloc() function.  Dynamic memory     */
/* allocation is used in order to avoid stack frame or data area          */
/* overflow errors that otherwise would have occurred at compile time     */
/* on the Macintosh computer.                                             */

	static FLOAT	energy_s[3][256];
	static FLOAT phi_s[3][256] ; /* 256 samples not 129 */

	static	int	*numlines ;
	static int     *partition;
	static FLOAT   *cbval, *rnorm;
	static FLOAT   *window;
	static FLOAT   *absthr;
	static double  *tmn;
	static FCB     *s;
	static FHBLK   *lthr;
	static F2HBLK  *r, *phi_sav;


	float nb[sizeof(FCB)/4];
  float cb[sizeof(FCB)/4];
  float ecb[sizeof(FCB)/4];
  float bc[sizeof(FCB)/4];

  float wsamp_r[sizeof(FBLK)/4];
  float wsamp_i[sizeof(FBLK)/4];
  float phi[sizeof(FBLK)/4];
  float energy[sizeof(FBLK)/4];
  float c[sizeof(FHBLK)/4];
  float fthr[sizeof(FHBLK)/4];
  F32   snrtmp[2];   

return;

  memset(nb, 0, sizeof(FCB));
  memset(cb, 0, sizeof(FCB));
  memset(ecb, 0, sizeof(FCB));
  memset(bc, 0, sizeof(FCB));

  memset(wsamp_r, 0, sizeof(FBLK));
  memset(wsamp_i, 0, sizeof(FBLK));
  memset(phi, 0, sizeof(FBLK));
  memset(energy, 0, sizeof(FBLK));
  memset(c, 0, sizeof(FHBLK));
  memset(fthr, 0, sizeof(FHBLK));
  memset(snrtmp, 0, sizeof(F32)*2);


	if(fInit_L3psycho_anal==0)
	{

		new_ = 0;
		old = 1;
		oldest = 0;

		for( i = 0 ; i < 21 ; i++ )
		{

			ratio[0][i] = 0.0;
			ratio[1][i] = 0.0;
		}

		for( i = 0 ; i < 2 ; i++ )
			for( j = 0 ; j < 12 ; j++ )
			{
				ratio_s[i][j][0] = 0.0;
				ratio_s[i][j][0] = 0.0;
				ratio_s[i][j][0] = 0.0;
			}


/* These dynamic memory allocations simulate "static" variables placed    */
/* in the data space.  Each mem_alloc() call here occurs only once at     */
/* initialization time.  The mem_free() function must not be called.      */
     numlines = (int *) mem_alloc(sizeof(ICB), "numlines");
     partition = (int *) mem_alloc(sizeof(IHBLK), "partition");
     cbval = (FLOAT *) mem_alloc(sizeof(FCB), "cbval");
     rnorm = (FLOAT *) mem_alloc(sizeof(FCB), "rnorm");
     window = (FLOAT *) mem_alloc(sizeof(FBLK), "window");
     absthr = (FLOAT *) mem_alloc(sizeof(FHBLK), "absthr"); 
     tmn = (double *) mem_alloc(sizeof(DCB), "tmn");
     s = (FCB *) mem_alloc(sizeof(FCBCB), "s");
     lthr = (FHBLK *) mem_alloc(sizeof(F2HBLK), "lthr");
     r = (F2HBLK *) mem_alloc(sizeof(F22HBLK), "r");
     phi_sav = (F2HBLK *) mem_alloc(sizeof(F22HBLK), "phi_sav");

/*#if 0 */
     i = sfreq + 0.5;
     switch(i){
        case 32000: sfreq_idx = 0; break;
        case 44100: sfreq_idx = 1; break;
        case 48000: sfreq_idx = 2; break;
/*        default:    printf("error, invalid sampling frequency: %d Hz\n",i);
        exit(-1); */
     }
     read_absthr(absthr, sfreq_idx);

		sync_flush=768;
		flush=576;
		syncsize=1344;


/* calculate HANN window coefficients */
     for(i=0;i<BLKSIZE;i++)  window[i]  =0.5*(1-cos(2.0*PI*(i-0.5)/BLKSIZE));
     for(i=0;i<BLKSIZE_s;i++)window_s[i]=0.5*(1-cos(2.0*PI*(i-0.5)/BLKSIZE_s));
/* reset states used in unpredictability measure */
     for(i=0;i<HBLKSIZE;i++){
        r[0][0][i]=r[1][0][i]=r[0][1][i]=r[1][1][i]=0;
        phi_sav[0][0][i]=phi_sav[1][0][i]=0;
        phi_sav[0][1][i]=phi_sav[1][1][i]=0;
        lthr[0][i] = 60802371420160.0;
        lthr[1][i] = 60802371420160.0;
     }
/*****************************************************************************
 * Initialization: Compute the following constants for use later             *
 *    partition[HBLKSIZE] = the partition number associated with each        *
 *                          frequency line                                   *
 *    cbval[CBANDS]       = the center (average) bark value of each          *
 *                          partition                                        *
 *    numlines[CBANDS]    = the number of frequency lines in each partition  *
 *    tmn[CBANDS]         = tone masking noise                               *
 *****************************************************************************/
/* compute fft frequency multiplicand */
     freq_mult = sfreq/BLKSIZE;
 
/* calculate fft frequency, then bval of each line (use fthr[] as tmp storage)*/
     for(i=0;i<HBLKSIZE;i++){
        temp1 = i*freq_mult;
        j = 1;
        while(temp1>crit_band[j])j++;
        fthr[i]=j-1+(temp1-crit_band[j-1])/(crit_band[j]-crit_band[j-1]);
     }
     partition[0] = 0;
/* temp2 is the counter of the number of frequency lines in each partition */
     temp2 = 1;
     cbval[0]=fthr[0];
     bval_lo=fthr[0];
     for(i=1;i<HBLKSIZE;i++){
        if((fthr[i]-bval_lo)>0.33){
           partition[i]=partition[i-1]+1;
           cbval[partition[i-1]] = cbval[partition[i-1]]/temp2;
           cbval[partition[i]] = fthr[i];
           bval_lo = fthr[i];
           numlines[partition[i-1]] = temp2;
           temp2 = 1;
        }
        else {
           partition[i]=partition[i-1];
           cbval[partition[i]] += fthr[i];

           temp2++;
        }
     }
     numlines[partition[i-1]] = temp2;
     cbval[partition[i-1]] = cbval[partition[i-1]]/temp2;
 
/************************************************************************
 * Now compute the spreading function, s[j][i], the value of the spread-*
 * ing function, centered at band j, for band i, store for later use    *
 ************************************************************************/
     for(j=0;j<CBANDS;j++){
        for(i=0;i<CBANDS;i++){
           temp1 = (cbval[i] - cbval[j])*1.05;
           if(temp1>=0.5 && temp1<=2.5){
              temp2 = temp1 - 0.5;
              temp2 = 8.0 * (temp2*temp2 - 2.0 * temp2);
           }
           else temp2 = 0;
           temp1 += 0.474;
           temp3 = 15.811389+7.5*temp1-17.5*sqrt((double) (1.0+temp1*temp1));
           if(temp3 <= -100) s[i][j] = 0;
           else {
              temp3 = (temp2 + temp3)*LN_TO_LOG10;
              s[i][j] = exp(temp3);
           }
				}
     }

  /* Calculate Tone Masking Noise values */
     for(j=0;j<CBANDS;j++){
        temp1 = 15.5 + cbval[j];
        tmn[j] = (temp1>24.5) ? temp1 : 24.5;
  /* Calculate normalization factors for the net spreading functions */
        rnorm[j] = 0;
        for(i=0;i<CBANDS;i++){
           rnorm[j] += s[j][i];
        }
     }
		fInit_L3psycho_anal++;
 }
 
/************************* End of Initialization *****************************/

	if ( init_L3 == 0 )
	{
	    L3para_read( (int)sfreq,numlines,partition_l,minval,qthr_l,norm_l,s3_l,
			 partition_s,qthr_s,norm_s,SNR_s,
			 cbw_l,bu_l,bo_l,w1_l,w2_l, cbw_s,bu_s,bo_s,w1_s,w2_s );
	    init_L3 ++ ;
	}
	
	for ( j = 0; j < 21; j++ )
	    ratio_d[j] = ratio[chn][j];
	for ( j = 0; j < 12; j++ )
	    for ( i = 0; i < 3; i++ )
		ratio_ds[j][i] = ratio_s[chn][j][i];
	
	if ( chn == 0 )
	{
	    if ( new_ == 0 )
	    {
		new_ = 1;
		old = 0;
		oldest = 1;
	    }
	    else
	    {
		new_ = 0;
		old = 1;
		oldest = 0;
	    }
	}

/**********************************************************************
*  Delay signal by sync_flush=768 samples                             *
**********************************************************************/
	
	for ( j = 0; j < sync_flush; j++ ) /* for long window samples */
	    savebuf[j] = savebuf[j+flush];
	
	for ( j = sync_flush; j < syncsize; j++ )
	    savebuf[j] = *buffer++;
	
	for ( j = 0; j < BLKSIZE; j++ )
	{ /**window data with HANN window**/
	    wsamp_r[j] = window[j] * savebuf[j];  
	    wsamp_i[j] = 0.0;
	}


/**********************************************************************
*    compute unpredicatability of first six spectral lines            * 
**********************************************************************/

	fft( wsamp_r, wsamp_i, energy, phi, 1024 );		/**long FFT**/
	for ( j = 0; j < 6; j++ )
	{	 /* calculate unpredictability measure cw */
	    r_prime = 2.0 * r[chn][old][j] - r[chn][oldest][j];
	    phi_prime = 2.0 * phi_sav[chn][old][j]-phi_sav[chn][oldest][j];
	    r[chn][new_][j] = sqrt((double) energy[j]);
	    phi_sav[chn][new_][j] = phi[j];
	    temp1 = r[chn][new_][j] * cos((double) phi[j])
		- r_prime * cos(phi_prime);
	    temp2 = r[chn][new_][j] * sin((double) phi[j])
		- r_prime * sin(phi_prime);
	    temp3=r[chn][new_][j] + fabs(r_prime);
	    
	    if ( temp3 != 0.0 )
		cw[j] = sqrt( temp1*temp1+temp2*temp2 ) / temp3;
	    else
		cw[j] = 0;
	}


/**********************************************************************
*     compute unpredicatibility of next 200 spectral lines            *
**********************************************************************/ 
	for ( sblock = 0; sblock < 3; sblock++ )
	{ /**window data with HANN window**/
	    for ( j = 0, k = 128 * (2 + sblock); j < 256; j++, k++ )
	    {
		wsamp_r[j] = window_s[j]* savebuf[k]; 
		wsamp_i[j] = 0.0;
	    }							/* short FFT*/
	    
	    fft( wsamp_r, wsamp_i, &energy_s[sblock][0], &phi_s[sblock][0], 256 );
        }
 
        sblock = 1;

	for ( j = 6; j < 206; j += 4 )
	{/* calculate unpredictability measure cw */
	    double r2, phi2, temp1, temp2, temp3;
	    
	    k = (j+2) / 4; 
	    r_prime = 2.0 * sqrt((double) energy_s[0][k])
		- sqrt((double) energy_s[2][k]);
	    phi_prime = 2.0 * phi_s[0][k] - phi_s[2][k];
	    r2 = sqrt((double) energy_s[1][k]);
	    phi2 = phi_s[1][k];
	    temp1 = r2 * cos( phi2 ) - r_prime * cos( phi_prime );
	    temp2 = r2 * sin( phi2 ) - r_prime * sin( phi_prime );
	    temp3 = r2 + fabs( r_prime );
	    if ( temp3 != 0.0 )
		cw[j] = sqrt( temp1 * temp1 + temp2 * temp2 ) / temp3;
	    else
		cw[j] = 0.0;
	    cw[j+1] = cw[j+2] = cw[j+3] = cw[j];
	}


/**********************************************************************
*    Set unpredicatiblility of remaining spectral lines to 0.4        *
**********************************************************************/
	for ( j = 206; j < HBLKSIZE; j++ )
	    cw[j] = 0.4;
	


/**********************************************************************
*    Calculate the energy and the unpredictability in the threshold   *
*    calculation partitions                                           *
**********************************************************************/

	for ( b = 0; b < CBANDS; b++ )
	{
	    eb[b] = 0.0;
	    cb[b] = 0.0;
	}
	for ( j = 0; j < HBLKSIZE; j++ )
	{
	    int tp = partition_l[j];
	    if ( tp >= 0 )
	    {
		eb[tp] += energy[j];
		cb[tp] += cw[j] * energy[j];
	    }
	}


/**********************************************************************
*      convolve the partitioned energy and unpredictability           *
*      with the spreading function, s3_l[b][k]                        *
******************************************************************** */
	
	for ( b = 0; b < CBANDS; b++ )
	{
	    ecb[b] = 0.0;
	    ctb[b] = 0.0;
	}
	for ( b = 0;b < CBANDS; b++ )
	{
	    for ( k = 0; k < CBANDS; k++ )
	    {
		ecb[b] += s3_l[b][k] * eb[k];	/* sprdngf for Layer III */
		ctb[b] += s3_l[b][k] * cb[k];
	    }
	}

	/* calculate the tonality of each threshold calculation partition */
	/* calculate the SNR in each threshhold calculation partition */

	for ( b = 0; b < CBANDS; b++ )
	{
	    double cbb,tbb;
	    if (ecb[b] != 0.0 )
                {
		cbb = ctb[b]/ecb[b];
                if (cbb <0.01) cbb = 0.01;
		cbb = log( cbb);
                }
	    else
		cbb = 0.0 ;
	    tbb = -0.299 - 0.43*cbb;  /* conv1=-0.299, conv2=-0.43 */
	    tbb = minimum( 1.0, maximum( 0.0, tbb) ) ;  /* 0<tbb<1 */
	    SNR_l[b] = maximum( minval[b], 29.0*tbb+6.0*(1.0-tbb) );
	}	/* TMN=29.0,NMT=6.0 for all calculation partitions */
	
	for ( b = 0; b < CBANDS; b++ ) /* calculate the threshold for each partition */
	    nb[b] = ecb[b] * norm_l[b] * exp( -SNR_l[b] * LN_TO_LOG10 );

	for ( b = 0; b < CBANDS; b++ )
	{ /* pre-echo control */
	    double temp_1; /* BUG of IS */
	    temp_1 = minimum( nb[b], minimum(2.0*nb_1[chn][b],16.0*nb_2[chn][b]) );
	    thr[b] = maximum( qthr_l[b], temp_1 );/* rpelev=2.0, rpelev2=16.0 */
	    nb_2[chn][b] = nb_1[chn][b];
	    nb_1[chn][b] = nb[b];
	}


	*pe = 0.0;		/*  calculate percetual entropy */
	for ( b = 0; b < CBANDS; b++ )
	{
	    double tp ;
	    tp = minimum( 0.0, log((thr[b]+1.0) / (eb[b]+1.0) ) ) ; /*not log*/
	    *pe -= numlines[b] * tp ;
	}	/* thr[b] -> thr[b]+1.0 : for non sound portition */
	
#define switch_pe  1800
        blocktype = NORM_TYPE;
	

	if ( *pe < switch_pe )
	{				/* no attack : use long blocks */
	    switch( blocktype_old[chn] ) 
	    {
	      case NORM_TYPE:
	      case STOP_TYPE:
		blocktype = NORM_TYPE;
		break;
    
	      case SHORT_TYPE:
		blocktype = STOP_TYPE;
		break;
    
/*	      case START_TYPE:
		fprintf( stderr, "Error in block selecting\n" );
		abort();
		break; problem */
	    }

	    /* threshold calculation (part 2) */
	    for ( sb = 0; sb < SBMAX_l; sb++ )
	    {
		en[sb] = w1_l[sb] * eb[bu_l[sb]] + w2_l[sb] * eb[bo_l[sb]];
		thm[sb] = w1_l[sb] *thr[bu_l[sb]] + w2_l[sb] * thr[bo_l[sb]];
		for ( b = bu_l[sb]+1; b < bo_l[sb]; b++ )
		{
		    en[sb]  += eb[b];
		    thm[sb] += thr[b];
		}
		if ( en[sb] != 0.0 )
		    ratio[chn][sb] = thm[sb]/en[sb];
		else
		    ratio[chn][sb] = 0.0;
	    }
	}
	else 
	{
	    /* attack : use short blocks */
	    blocktype = SHORT_TYPE;
	    
	    if ( blocktype_old[chn] == NORM_TYPE ) 
		blocktype_old[chn] = START_TYPE;
	    if ( blocktype_old[chn] == STOP_TYPE )
		blocktype_old[chn] = SHORT_TYPE ;
	    
	    /* threshold calculation for short blocks */
	    
	    for ( sblock = 0; sblock < 3; sblock++ )
	    {
		for ( b = 0; b < CBANDS_s; b++ )
		{
		    eb[b] = 0.0;
		    ecb[b] = 0.0;
		}
		for ( j = 0; j < HBLKSIZE_s; j++ )
		    eb[partition_s[j]] += energy_s[sblock][j];
		for ( b = 0; b < CBANDS_s; b++ )
		    for ( k = 0; k < CBANDS_s; k++ )
			ecb[b] += s3_l[b][k] * eb[k];
		for ( b = 0; b < CBANDS_s; b++ )
		{
		    nb[b] = ecb[b] * norm_l[b] * exp( (double) SNR_s[b] * LN_TO_LOG10 );
		    thr[b] = maximum (qthr_s[b],nb[b]);
		}
		for ( sb = 0; sb < SBMAX_s; sb++ )
		{
		    en[sb] = w1_s[sb] * eb[bu_s[sb]] + w2_s[sb] * eb[bo_s[sb]];
		    thm[sb] = w1_s[sb] *thr[bu_s[sb]] + w2_s[sb] * thr[bo_s[sb]];
		    for ( b = bu_s[sb]+1; b < bo_s[sb]; b++ )
		    {
			en[sb] += eb[b];
			thm[sb] += thr[b];
		    }
		    if ( en[sb] != 0.0 )
			ratio_s[chn][sb][sblock] = thm[sb]/en[sb];
		    else
			ratio_s[chn][sb][sblock] = 0.0;
		}
	    }
	} 
	
	cod_info->block_type = blocktype_old[chn];
	blocktype_old[chn] = blocktype;

	if ( cod_info->block_type == NORM_TYPE )
	    cod_info->window_switching_flag = 0;
	else
	    cod_info->window_switching_flag = 1;
	cod_info->mixed_block_flag = 0;

}


/*____ L3para_read() __________________________________________________________*/

void L3para_read(int sfreq, int *numlines, int *partition_l, double *minval, double *qthr_l, double *norm_l, double (*s3_l)[63], int *partition_s, double *qthr_s, double *norm_s, double *SNR, int *cbw_l, int *bu_l, int *bo_l, double *w1_l, double *w2_l, int *cbw_s, int *bu_s, int *bo_s, double *w1_s, double *w2_s)
{
   static double bval_l[CBANDS], bval_s[CBANDS];
   int   cbmax, cbmax_tp;
   static double s3_s[CBANDS][CBANDS];

   int  sbmax ;
   int  i,j,k,k2, part_max ;


	 psyDataElem * rpa1;
	 psyDataElem2 * rpa2;
	 psyDataElem3 * rpa3;


	 /* Read long block data */


	switch( sfreq )
	{
		case	32000:
			rpa1 = psy_longBlock__32000_58;
			cbmax_tp = 59;
			break;
		case	44100:
			rpa1 = psy_longBlock_44100_62;
			cbmax_tp = 63;
			break;
		case	48000:
			rpa1 = psy_longBlock_48000_61;
			cbmax_tp = 62;
			break;
	}
	 
	cbmax = cbmax_tp;
	for(i=0,k2=0;i<cbmax_tp;i++)
	{
		numlines[i] = rpa1->lines;
		minval[i] = rpa1->minVal;
		qthr_l[i] = rpa1->qthr;
		norm_l[i] = rpa1->norm;
		bval_l[i] = rpa1->bVal;
		rpa1++;

		for(k=0;k<numlines[i];k++)
			partition_l[k2++] = i ;
	}

		

/************************************************************************
 * Now compute the spreading function, s[j][i], the value of the spread-*
 * ing function, centered at band j, for band i, store for later use    *
 ************************************************************************/
	  part_max = cbmax ;
          for(i=0;i<part_max;i++)
	  {
	  double tempx,x,tempy,temp;
            for(j=0;j<part_max;j++)
	    {
             tempx = (bval_l[i] - bval_l[j])*1.05;
             if (j>=i) tempx = (bval_l[i] - bval_l[j])*3.0;
               else    tempx = (bval_l[i] - bval_l[j])*1.5;
/*             if (j>=i) tempx = (bval_l[j] - bval_l[i])*3.0;
               else    tempx = (bval_l[j] - bval_l[i])*1.5; */
             if(tempx>=0.5 && tempx<=2.5)
	     {
               temp = tempx - 0.5;
               x = 8.0 * (temp*temp - 2.0 * temp);
             }
             else x = 0.0;
             tempx += 0.474;
             tempy = 15.811389 + 7.5*tempx - 17.5*sqrt(1.0+tempx*tempx);
             if (tempy <= -60.0) s3_l[i][j] = 0.0;
             else                s3_l[i][j] = exp( (x + tempy)*LN_TO_LOG10 );

            }
          }


/* Read short block data */

	switch( sfreq )
	{
		case	32000:
			rpa2 = psy_shortBlock_32000_41;
			cbmax_tp = 42;
			break;
		case	44100:
			rpa2 = psy_shortBlock_44100_38;
			cbmax_tp = 39;
			break;
		case	48000:
			rpa2 = psy_shortBlock_48000_37;
			cbmax_tp = 38;
			break;
	}



	cbmax = cbmax_tp;
	for(i=0,k2=0;i<cbmax_tp;i++)
	{
		numlines[i] = rpa2->lines;
		qthr_s[i] = rpa2->qthr;
		norm_s[i] = rpa2->norm;
		SNR[i] = rpa2->snr;
		bval_s[i] = rpa2->bVal;
		rpa2++;
		
		for(k=0;k<numlines[i];k++)
			partition_s[k2++] = i ;
	}


/************************************************************************
 * Now compute the spreading function, s[j][i], the value of the spread-*
 * ing function, centered at band j, for band i, store for later use    *
 ************************************************************************/
	  part_max = cbmax ;
          for(i=0;i<part_max;i++)
	  {
	  double tempx,x,tempy,temp;
            for(j=0;j<part_max;j++)
	    {
             tempx = (bval_s[i] - bval_s[j])*1.05;
             if (j>=i) tempx = (bval_s[i] - bval_s[j])*3.0;
               else    tempx = (bval_s[i] - bval_s[j])*1.5;
             if(tempx>=0.5 && tempx<=2.5)
	     {
               temp = tempx - 0.5;
               x = 8.0 * (temp*temp - 2.0 * temp);
             }
             else x = 0.0;
             tempx += 0.474;
             tempy = 15.811389 + 7.5*tempx - 17.5*sqrt(1.0+tempx*tempx);
             if (tempy <= -60.0) s3_s[i][j] = 0.0;
             else                s3_s[i][j] = exp( (x + tempy)*LN_TO_LOG10 );
            }
          }
/* Read long block data for converting threshold calculation 
   partitions to scale factor bands */

	switch( sfreq )
	{
		case	32000:
			rpa3 = psy_data3_32000_20;
			sbmax = 21;
			break;
		case	44100:
			rpa3 = psy_data3_44100_20;
			sbmax = 21;
			break;
		case	48000:
			rpa3 = psy_data3_48000_20;
			sbmax = 21;
			break;
	}

  for(i=0;i<sbmax;i++)
  {
		cbw_l[i] = rpa3->cbw;
		bu_l[i] = rpa3->bu;
		bo_l[i] = rpa3->bo;
		w1_l[i] = rpa3->w1;
		w2_l[i] = rpa3->w2;
		rpa3++;		
	}

/* Read short block data for converting threshold calculation 
   partitions to scale factor bands */

	switch( sfreq )
	{
		case	32000:
			rpa3 = psy_data4_32000_11;
			sbmax = 12;
			break;
		case	44100:
			rpa3 = psy_data4_44100_11;
			sbmax = 12;
			break;
		case	48000:
			rpa3 = psy_data4_48000_11;
			sbmax = 12;
			break;
	}

  for(i=0;i<sbmax;i++)
  {
		cbw_s[i] = rpa3->cbw;
		bu_s[i] = rpa3->bu;
		bo_s[i] = rpa3->bo;
		w1_s[i] = rpa3->w1;
		w2_s[i] = rpa3->w2;
		rpa3++;		
	}	

}





