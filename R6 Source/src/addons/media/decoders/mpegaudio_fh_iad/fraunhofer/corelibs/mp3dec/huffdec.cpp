/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: huffdec.cpp
 *   project : ISO/MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: main huffman decoding
 *
 *
\***************************************************************************/

/*
 * $Date: 1998/05/28 10:10:01 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/huffdec.cpp,v 1.2 1998/05/28 10:10:01 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "huffdec.h"
#include "l3table.h"

/*-------------------------------------------------------------------------*/

#ifndef min
  #define min(a,b) (((a) < (b)) ? (a):(b))
#endif

/***************************************************************************\ 
 *
 *   functionname: mp3HuffmanRead
 *   description : read layer3 hufman data
 *
 *   returns : <none>
 *
 *   input   : 
 *   output  : 
 *   global  : 
 *
\***************************************************************************/

void CMp3Huffman::Read
    (
    CBitStream      &Bs,
    int32           *pISpectrum,
    MP3SI_GRCH      &SiGrCh,
    const MPEG_INFO &Info,
    int              interleave
    )
{
  int cb;
  int region1Start;
  int region2Start;
  int regionEnd[3];

  //
  // find region boundary
  //
  if ( (SiGrCh.window_switching_flag) && (SiGrCh.block_type == 2) )
    {
    // short blocks
    if ( SiGrCh.mixed_block_flag == 0 )
      {
      region1Start = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[(SiGrCh.region0_count+1)/3] * 3;
      }
    else
      {
      if ( Info.IsMpeg1 )
        {
        region1Start = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[SiGrCh.region0_count+1];
        }
      else
        {
        // width of sfb 3 (short)
        int nSfb3Width = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[4] - 
                         sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[3];

        /* 
         * MPEG-2/2.5: short AND mixed -> sfb 0..5 long, sfb 3..12 short 
         *             region0_count is always 7 (i.e. 8 sfb in region 0)
         *             --> sfb 0..5 long, sfb 3 short (subwindow 0..1) in region 0
         *                 sfb 3 (subwindow 2) .. sfb 12 in region 1
         */
        region1Start = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[6] + 
                       nSfb3Width*2;
        }
      }

    region2Start = 576; // no region2 for short blocks
    }
  else
    {                           
    // long blocks
    region1Start = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[SiGrCh.region0_count+1];
    region2Start = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[SiGrCh.region0_count+
                   SiGrCh.region1_count + 2];
    }

  // regions for bigvalue area
  regionEnd[0] = min(SiGrCh.big_values*2, region1Start);
  regionEnd[1] = min(SiGrCh.big_values*2, region2Start);
  regionEnd[2] = SiGrCh.big_values*2;

  //
  // read huffman code
  //
  SiGrCh.zeroStartNdx = ReadHuffmanCode(Bs,
                                        pISpectrum,
                                        SiGrCh.table_select,
                                        regionEnd,
                                        SiGrCh.count1table_select+32,
                                        SiGrCh.part2_3_length,
                                        interleave);

  //
  // find zero boundaries aligned to scalefactor bands
  //

  // determine, if type of zeroSfbStartNdx is long or short
  if ( SiGrCh.window_switching_flag && (SiGrCh.block_type == 2) )
    {
    if ( SiGrCh.mixed_block_flag )
      {
      // long-short mixed block
      int LastLongSfb      = ( Info.IsMpeg1 ? 8:6 ) -1;
      int LastLongSfbStart = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[LastLongSfb];

      if ( SiGrCh.zeroStartNdx <= LastLongSfbStart )
        SiGrCh.zeroSfbStartNdxIsLong = 1;
      else
        SiGrCh.zeroSfbStartNdxIsLong = 0;
      }
    else
      {
      // short blocks
      SiGrCh.zeroSfbStartNdxIsLong = 0;
      }
    }
  else
    {
    // long blocks
    SiGrCh.zeroSfbStartNdxIsLong = 1;
    }

  // now adjust zero index to sfb-band boundary
  if ( SiGrCh.zeroSfbStartNdxIsLong )
    {
    // long blocks
    for ( cb=0; cb<22; cb++ )
      {
      if ( SiGrCh.zeroStartNdx <= sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].l[cb] )
        break;
      }
    SiGrCh.zeroSfbStartNdxL = cb;
    }
  else
    {
    // short blocks
    int   subwin, i;
    int   SfbStartNdx, SfbEndNdx, StartNdx, EndNdx, SfbWidth, NonZeroSampleFound;

    // find max. sfb where all samples are zero
    for ( cb=0; cb<13; cb++ )
      {
      if ( SiGrCh.zeroStartNdx <= sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb]*3 )
        break;
      }

    SiGrCh.zeroSfbStartNdxSMax = cb;
    SiGrCh.zeroSfbStartNdxS[0] = cb;
    SiGrCh.zeroSfbStartNdxS[1] = cb;
    SiGrCh.zeroSfbStartNdxS[2] = cb;

    // now find zero sfb for each sub-window
    for ( subwin=0; subwin<3; subwin++ )
      {
      NonZeroSampleFound = 0;

      for ( cb=SiGrCh.zeroSfbStartNdxSMax-1; cb>=0; cb-- )
        {
        SfbStartNdx = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb];
        SfbEndNdx   = sfBandIndex[Info.fhgVersion][Info.sample_rate_ndx].s[cb+1];
        SfbWidth    = SfbEndNdx - SfbStartNdx;

        StartNdx = SfbStartNdx*3 + subwin*SfbWidth;
        EndNdx   = StartNdx + SfbWidth;

        for ( i=StartNdx; i<EndNdx; i++ )
          {
          if ( pISpectrum[i*interleave] )
            {
            NonZeroSampleFound = 1;
            break;
            }
          }

        if ( NonZeroSampleFound )
          {
          SiGrCh.zeroSfbStartNdxS[subwin] = cb+1;
          break;
          }

        } // for ( cb=zeroSfbStartNdxShortMax; cb>0; cb-- )
      } // for ( subwin=0; subwin<3; subwin++ )
    }
}

/*-------------------------------------------------------------------------*/
