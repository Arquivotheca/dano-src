/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: conceal.cpp
 *   project : ISO/MPEG-Decoder
 *   author  : Stefan Gewinner
 *   date    : 1998-05-26
 *   contents/description: error concealment class
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/16 08:40:27 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/conceal.cpp,v 1.6 1999/02/16 08:40:27 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "conceal.h"
#include "l3table.h"
#include <string.h>
#include <math.h>

/*-------------------------------------------------------------------------*/

//
// number of granules we use to predict the energy in the next granule
//
#define N_USE_HISTORY 3

#ifdef USE_ENERGY_PREDICTION
  #define NPREDCOFF 5
  #define MUPRED    0.1
#endif

//
// global variables for error concealment
// (experimental only, delete ASAP) 
//
static const int   nRepeat         = 0;    /* non-operational */
static const float noiseLevel      = 4.0f; /* level of white noise injection */
static const int   formSpectrum    = 1;    /* spectrum forming / white noise injection */
static const int   sbmin_use_noise = 0;    /* lowest codec band in which noise is injected */
static const int   sbmax_use_noise = 22;   /* highest codec band+1 in which noise is injected */
static const int   randBoundary    = 0;    /* lower codec bands use another random method */

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CErrorConcealment::CErrorConcealment ()
{
  Init();
}

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

void CErrorConcealment::Init()
{
  int i,j,k;

  for ( i=0; i<2; i++ )
    {
    SpecDataBuffer[i].writeOffset = 0;

    for (j=0; j<MAX_SPECTRUM_DATA; j++) 
      {
      // reset side info
      memset(&SpecDataBuffer[i].gran[j].gr, 0, sizeof(MP3SI_GRCH));

      // reset line amplitudes
      for (k=0; k<SBLIMIT*SSLIMIT; k++) 
        {
	      SpecDataBuffer[i].gran[j].Rs[k] = 0.0f;
        }

      // reset sf-band energies
      for (k=0; k<23; k++)
        SpecDataBuffer[i].gran[j].nrg[k] = 0.0f;

      // reset valid flag
      SpecDataBuffer[i].gran[j].nrgValid = 0;
      }

    // reset side info
    memset(&SpecDataBuffer[i].estGran.gr, 0, sizeof(MP3SI_GRCH));

    // reset line amplitudes
    for (k=0; k<SBLIMIT*SSLIMIT; k++) 
      {
      SpecDataBuffer[i].estGran.Rs[k] = 0.0f;
      }

    // reset sf-band energies
    for (k=0; k<23; k++)
      SpecDataBuffer[i].estGran.nrg[k] = 0.0f;

    // reset valid flag
    SpecDataBuffer[i].estGran.nrgValid = 0;
    }

  f_seed = 42;
  b_seed = 0xfeed4U;
  w_seed = 0x23423;

  iff = 0;

#ifdef DEBUG_CONCEALMENT
  currentFrame = 0;
#endif
}

//-------------------------------------------------------------------------*
//   Apply
//-------------------------------------------------------------------------*

void CErrorConcealment::Apply
    (
    bool             fApply,
    const MPEG_INFO &Info,
    MP3SI           &Si,
    float           *lpSpec, 
    int              gr, 
    int              ch
    )
{
  if ( fApply )
    {
    Restore(Info, Si, lpSpec, gr, ch);
    }
  else
    {
    /*
     * calling specDataInit() here does not work correctly with noise
     * substitution because it destroys information that will be needed
     * if the next frame misses. We rather use somewhat old spectral data
     * than none at all... Maybe need to re-consider this.
     */

#if 0 /* temporarily (?) disabled */
    /* reset SpecDataBuffer if first correct frame after crc error */
    if ( SpecDataBuffer.nRestoreCount )
	    {       
	    SpecDataBuffer.nRestoreCount = 0;
	    specdataInit();
	    }
#endif

    Store(Info, Si, lpSpec, gr, ch);
    }

#ifdef DEBUG_CONCEALMENT
  if ( (gr ==(Info.IsMpeg1 ? 1:0)) && (ch==(Info.stereo==2?1:0)) )
    currentFrame++;
#endif
}

//-------------------------------------------------------------------------*
//   Store
//-------------------------------------------------------------------------*

void CErrorConcealment::Store
    (
    const MPEG_INFO & /* Info */,
    const MP3SI      &Si,
    const float     *lpSpec, 
    int              gr, 
    int              ch
    )
{
  if ( Si.ch[ch].gr[gr].block_type != 2) 
    {
    int    wp     = SpecDataBuffer[ch].writeOffset;
    float *stSpec = SpecDataBuffer[ch].gran[wp].Rs;

#ifdef DEBUG_CONCEALMENT
    SpecDataBuffer[ch].gran[wp].frameNumber = currentFrame;
#endif

    /* 
     * save spectrum
     */
    memcpy(stSpec, lpSpec, SBLIMIT*SSLIMIT*sizeof(float));
    stSpec[0] = 0.0f; /* delete DC components always */

    /*
     * save si
     */
    memcpy(&(SpecDataBuffer[ch].gran[wp].gr), &Si.ch[ch].gr[gr], sizeof(MP3SI_GRCH));

    /* energies have not been calculated */
    SpecDataBuffer[ch].gran[wp].nrgValid = 0;
    
    /* need to re-evaluate predicted band energies */
    SpecDataBuffer[ch].estGran.nrgValid = 0;
    SpecDataBuffer[ch].writeOffset      = (wp+1) % MAX_SPECTRUM_DATA;
    }
}

//-------------------------------------------------------------------------*
//   Restore
//-------------------------------------------------------------------------*

void CErrorConcealment::Restore
    (
    const MPEG_INFO &Info,
    MP3SI           &Si,
    float           *lpSpec, 
    int              gr, 
    int              ch
    )
{
  int    sfreq = Info.sample_rate_ndx;
  int    rp    = (SpecDataBuffer[ch].writeOffset+MAX_SPECTRUM_DATA-1)%MAX_SPECTRUM_DATA;
  int    i,cb;

  double norm_nrg;
  double nrg;

#ifdef DEBUG_CONCEALMENT
  if ( ch == 0 )
    printf("%d %d %d\n",currentFrame,SpecDataBuffer[ch].gran[rp].frameNumber,
		       SpecDataBuffer[ch].gran[rp].gr.block_type);
#endif

  /*
   * calculate an estimate of how our spectral energy should be distributed
   * (if we have not already done so)
   */

  if ( !SpecDataBuffer[ch].estGran.nrgValid )
    {
    predictEnergies(Info, SpecDataBuffer + ch);
    }

  /* restore channel-dependent side-info from last long block */
  /* don't touch main_data_begin */

  memcpy(&Si.ch[ch].gr[gr], &SpecDataBuffer[ch].gran[rp].gr,sizeof(MP3SI_GRCH));

  /*
   * form spectrum after previous blocks? (as opposed to simply inserting
   * white noise)
   */

  if ( formSpectrum ) 
    {
    /*
     * clear spectrum below codec_band[sbmin_use_noise]
     */
    for ( i=0; i<sfBandIndex[Info.fhgVersion][sfreq].l[sbmin_use_noise]; i++ )
      lpSpec[i] = 0.0f;

    for ( cb=sbmin_use_noise; cb<sbmax_use_noise; cb++ )
      {
      int	startNdx = sfBandIndex[Info.fhgVersion][sfreq].l[cb];
	    int endNdx   = sfBandIndex[Info.fhgVersion][sfreq].l[cb+1];

      /*
       * This is the energy that the injected noise in this band should have
       */
      norm_nrg = SpecDataBuffer[ch].estGran.nrg[cb];

      if ( norm_nrg > 0.0 ) 
        {
	      nrg = 0.0;
	      for ( i=startNdx; i<endNdx; i++ ) 
          {
          /*
           * Below randBoundary, use sign flipping to randomize the spectrum.
           * Above, multiply the spectrum lines by random numbers.
           */

          if ( cb > randBoundary )
            {
            lpSpec[i]  = ranHigh2 (SpecDataBuffer[ch].estGran.Rs[i]);
            }
          else
            {
            lpSpec[i]  = ranLow (SpecDataBuffer[ch].estGran.Rs[i]);
            }

          nrg += lpSpec[i]*lpSpec[i];
          }

	      if ( nrg )
          {
	        float scale = (float)sqrt(norm_nrg / nrg);

	        for ( i=startNdx; i<endNdx; i++ )
	          lpSpec[i] *= scale;
	        }
        }
      else
        {
        /* if energy in this band is to be zero, clear spectrum */
	      for ( i=startNdx; i<endNdx; i++ )
	        lpSpec[i] = 0.0f;
        }
      }

    /*
     * clear all lines that have not been touched until now. This depends
     * on having a valid value i from previous loops.
     */
    for (; i<SBLIMIT*SSLIMIT; i++)
      lpSpec[i] = 0.0f;
    }
  else 
    {
    /*
     * white noise injection
     */
    norm_nrg = 1E5*noiseLevel;
    nrg      = 0.0;

    for ( i=0; i<sfBandIndex[Info.fhgVersion][sfreq].l[sbmax_use_noise]; i++ )
      {
	    lpSpec[i]  = (float)(2.0*ran3(&w_seed)-1.0);
	    nrg       += lpSpec[i]*lpSpec[i];
      }

    if ( nrg )
      {
      nrg = sqrt(norm_nrg/nrg);
      
      for ( i=0; i<sfBandIndex[Info.fhgVersion][sfreq].l[sbmax_use_noise]; i++ )
	      {
	      lpSpec[i] *= (float)nrg;
	      }
      }
    }

  Si.ch[ch].gr[gr].zeroStartNdx   = sfBandIndex[Info.fhgVersion][sfreq].l[sbmax_use_noise];
  Si.ch[ch].gr[gr].zeroSbStartNdx = sbmax_use_noise;
}

/*-------------------------------------------------------------------------*/

#ifdef USE_ENERGY_PREDICTION

//-------------------------------------------------------------------------*
//   predict
//-------------------------------------------------------------------------*

float CErrorConcealment::predict(const float *hist, const float *coff, int n)
{
  double y = 0.0;
  int    i;

  for ( i=0; i<n; i++)
    y += coff[i]*hist[i];

  return (float)y;
}

//-------------------------------------------------------------------------*
//   adaptPredictor
//-------------------------------------------------------------------------*

void CErrorConcealment::adaptPredictor
    (
    const float *hist,
    float        pwr,
    float       *coff,
    float        d,
    int          n
    )
{
  double mu;
  int    i;

  if ( d == 0.0f || pwr == 0.0f )
    return;

  mu = 2.0*MUPRED * d/((1+NPREDCOFF)*pwr);

  for ( i=0; i<n; i++ )
    {
    coff[i] += mu*hist[i];
    }
}

#endif

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//   estimateBandEnergies
//
//   calculate total spectral power per codec band
//-------------------------------------------------------------------------*

void CErrorConcealment::estimateBandEnergies(const MPEG_INFO &Info, GRAN_DATA *g)
{
  int cb;
  int sfreq = Info.sample_rate_ndx;

  for ( cb=0; cb<22; cb++ )
    {
    int i;
    int startNdx = sfBandIndex[Info.fhgVersion][sfreq].l[cb];
    int endNdx   = sfBandIndex[Info.fhgVersion][sfreq].l[cb+1];

    double nrg = 0.0;

    for ( i=startNdx; i<endNdx; i++ ) 
      {
      nrg += g->Rs[i]*g->Rs[i];
      }

    g->nrg[cb] = (float)nrg;
    }

  g->nrgValid = 1;
}

//-------------------------------------------------------------------------*
//   predict
//
//   description : make a reasonable (and conservative) estimation of the 
//                 energy that should go into substituted frames. 
//                 The estimation is based on the polyphase line absolute
//                 values of the past N_USE_HISTORY lines.
//                 Absolute values of single polyphase lines are averaged
//                 over these frames and the resulting total signal energy
//                 per codec band is then calculated.
//
//                 To make sure we do not double-calculate values, a flag
//                 is checked/set which indicates that values have been
//                 calculated already.
//-------------------------------------------------------------------------*

void CErrorConcealment::predictEnergies(const MPEG_INFO &Info, SPECTRUM_DATA *s)
{
  int i;

  for ( i=0; i<SBLIMIT*SSLIMIT; i++ )
    s->estGran.Rs[i]=0.0f;

  for ( i=s->writeOffset + MAX_SPECTRUM_DATA - N_USE_HISTORY;
        i!=s->writeOffset + MAX_SPECTRUM_DATA; i++) 
    {
    int        j;
    GRAN_DATA *thisGran = s->gran + (i % MAX_SPECTRUM_DATA);

    for ( j=0; j<SBLIMIT*SSLIMIT; j++ )
      s->estGran.Rs[j] += (float)fabs(thisGran->Rs[j]);
    }

  /*
   * divide by number of averaged granules
   */
  for ( i=0; i<SBLIMIT*SSLIMIT; i++ )
    s->estGran.Rs[i] *= (1.0f/N_USE_HISTORY);

  estimateBandEnergies(Info, &(s->estGran));
}

//-------------------------------------------------------------------------*
//   ranHigh1
//-------------------------------------------------------------------------*

float CErrorConcealment::ranHigh1(float a)
{
  return (float)((-0.5+ran3(&f_seed))*6.0f*a);
}

//-------------------------------------------------------------------------*
//   ranHigh2
//-------------------------------------------------------------------------*

float CErrorConcealment::ranHigh2(float /* a */)
{
  return (float)((-0.5+ran3(&f_seed)));
}

//-------------------------------------------------------------------------*
//   ranLow
//-------------------------------------------------------------------------*

float CErrorConcealment::ranLow (float a)
{
  return irbit2(&b_seed)?a:-a;
}

//-------------------------------------------------------------------------*
//   ran3
//-------------------------------------------------------------------------*

float CErrorConcealment::ran3 (long *idum)
{
  #define MBIG  1000000000
  #define MSEED  161803398
  #define MZ             0
  #define FAC (1.0f/(float)MBIG)

  long        mj,mk;
  int         i,ii,k;

  if ( *idum < 0 || iff == 0 )
    {
    iff     = 1;
    mj      = MSEED-(*idum < 0? -*idum : *idum);
    mj     %= MBIG;
    ma[55]  = mj;
    mk      = 1;

    for ( i=1; i<=54; i++ ) 
      {
      ii     = (21*i) % 55;
      ma[ii] = mk;
      mk     = mj-mk;
      mj     = ma[ii];
      if ( mk<MZ )
        mk += MBIG;
      }

    for ( k=1; k<=4; k++ )
      {
      for ( i=1; i<=55; i++ )
        {
	      ma[i] -= ma[1+(i+30)%55];
	      if (ma[i] < MZ)
          ma[i] += MBIG;
        }
      }

    inext  = 0;
    inextp = 31;
    *idum  = 1;
    }

  /* Here is where we start, except on initialization */
  if ( ++inext == 56  ) 
    inext=1;

  if ( ++inextp == 56 )  
    inextp=1;

  mj = ma[inext] - ma[inextp];

  if ( mj < MZ )
    mj += MBIG;

  ma[inext] = mj;

  return mj*FAC;
}

//-------------------------------------------------------------------------*
//   irbit2
//-------------------------------------------------------------------------*

int CErrorConcealment::irbit2 (unsigned long *iseed)
{
  #define IRBIT_MASK (8UL|1UL)

  /*
   * generate random bits
   */

  if ( *iseed & (1UL<<31) ) 
    {
    *iseed = ((*iseed ^ IRBIT_MASK) << 1) | 1UL;
    return 1;
    }
  else
    {
    *iseed <<= 1;
    return 0;
    }
}

/*-------------------------------------------------------------------------*/
