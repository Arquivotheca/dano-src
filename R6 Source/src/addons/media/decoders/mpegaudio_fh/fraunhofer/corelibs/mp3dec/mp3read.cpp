/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3read.cpp
 *   project : ISO/MPEG-Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: 
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/09 10:59:55 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3read.cpp,v 1.7 1999/03/09 10:59:55 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mp3read.h"
#include "crc16.h"
#include "bitstream.h"

/* ------------------------------------------------------------------------*/

static const struct 
  {
  int l[5];
  int s[3];
  } sfbtable = 
  {
    {0, 6, 11, 16, 21},
    {0, 6, 12}
  };

/*-------------------------------------------------------------------------*/

static const int slen[2][16] = 
  {
    {0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
    {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}
  };

/*-------------------------------------------------------------------------*/

static const int mpeg2_nr_of_sfbands[6][3][4] =
{
   { { 6, 5, 5, 5}, { 9, 9, 9, 9 }, { 6, 9, 9, 9} },
   { { 6, 5, 7, 3}, { 9, 9,12, 6 }, { 6, 9,12, 6} },
   { {11,10, 0, 0}, {18,18, 0, 0 }, {15,18, 0, 0} },

   { { 7, 7, 7, 0}, {12,12,12, 0 }, { 6,15,12, 0} },
   { { 6, 6, 6, 3}, {12, 9, 9, 6 }, { 6,12, 9, 6} },
   { { 8, 8, 5, 0}, {15,12, 9, 0 }, { 6,18, 9, 0} }
};

/*-------------------------------------------------------------------------*/

static const int Mpeg2MaxSf[6] = {  1,  1,  3,  7, 15, 31 };

/*-------------------------------------------------------------------------*/

/***************************************************************************\ 
 *
 *   functionname: CrcOk
 *   description : check for crc-errors (info.CrcError is set on error)
 *                 *sideinfo is skipped in case of crc-error*
 *
 *   returns : true  if no crc-error
 *             false if crc-error
 *
 *   input   : 
 *   output  : 
 *   global  : 
 *
\***************************************************************************/

static bool CrcOk(CBitStream &Bs, const MPEG_INFO &Info)
{
  int            nSlots;
  unsigned short CrcValue, tmp;

  if ( !Info.protection )
    {
    return true;
    }

  // set number of crc-protected bytes
  if ( Info.stereo == MONO )
    nSlots = (Info.IsMpeg1 ? 17:9);
  else
    nSlots = (Info.IsMpeg1 ? 32:17);

  //
  // rewind to second 16-header-bits
  // actual poition is AFTER header & crc-word
  //
  Bs.Rewind(32);

  tmp      = CalcCrc(Bs, 16, 0xffff);
  CrcValue = Bs.GetBits(16);
  tmp      = CalcCrc(Bs, 8*nSlots, tmp);

  // compare stored against calc'd value
  if ( tmp != CrcValue )
    {
    return false;
    }
  else
    {
    Bs.Rewind(nSlots*8);
    return true;
    }
}

/***************************************************************************\ 
 *
 *   functionname: mp3SideInfoRead
 *   description : read MPEG Layer-3 side-information
 *
 *   returns : true  on success
 *             false if a crc error occurs
 *
 *   input   : 
 *   output  : 
 *   global  : 
 *
\***************************************************************************/

bool mp3SideInfoRead
    (
    CBitStream      &Bs,
    MP3SI           &Si,
    const MPEG_INFO &Info
    )
{
  int ch, gr, i;

  // reset calculated values
  for ( gr=0; gr<(Info.IsMpeg1 ? 2:1); gr++ )
    {
    for ( ch=0; ch<Info.stereo; ch++ )
      {
      Si.ch[ch].gr[gr].intensity_scale       = 0;
      Si.ch[ch].gr[gr].zeroStartNdx          = 0;
      Si.ch[ch].gr[gr].zeroSfbStartNdxIsLong = 0;
      Si.ch[ch].gr[gr].zeroSfbStartNdxL      = 0;
      Si.ch[ch].gr[gr].zeroSfbStartNdxSMax   = 0;
      Si.ch[ch].gr[gr].zeroSfbStartNdxS[0]   = 0;
      Si.ch[ch].gr[gr].zeroSfbStartNdxS[1]   = 0;
      Si.ch[ch].gr[gr].zeroSfbStartNdxS[2]   = 0;
      Si.ch[ch].gr[gr].zeroSbStartNdx        = 0;
      }
    }

#if !defined(IGNORE_CRC)
  // check for crc-errors
  if ( CrcOk(Bs, Info) )
#endif
    {
    //
    // read common part for all granules, channels
    //
    if ( Info.IsMpeg1 )
      {
      // MPEG 1
      Si.main_data_begin = Bs.GetBits(9);
      
      if ( Info.stereo == MONO )
        Si.private_bits = Bs.GetBits(5);
      else
        Si.private_bits = Bs.GetBits(3);
      
      for ( ch=0; ch<Info.stereo; ch++ )
        for ( i=0; i<4; i++ )
          Si.ch[ch].scfsi[i] = Bs.GetBits(1);
      }
    else
      {
      // MPEG 2, MPEG 2.5
      Si.main_data_begin = Bs.GetBits(8);
      
      if ( Info.stereo == MONO )
        Si.private_bits = Bs.GetBits(1);
      else
        Si.private_bits = Bs.GetBits(2);
      }
    
    //
    // read granule/channel part
    //
    for ( gr=0; gr<(Info.IsMpeg1 ? 2:1); gr++ )
      {
      for ( ch=0; ch<Info.stereo; ch++ )
        {
        Si.ch[ch].gr[gr].part2_3_length        = Bs.GetBits(12);
        Si.ch[ch].gr[gr].big_values            = Bs.GetBits(9);        
        Si.ch[ch].gr[gr].global_gain           = Bs.GetBits(8);
        Si.ch[ch].gr[gr].scalefac_compress     = Bs.GetBits(Info.IsMpeg1 ? 4:9);
        Si.ch[ch].gr[gr].window_switching_flag = Bs.GetBits(1);
        
        // check BigValues
        if ( Si.ch[ch].gr[gr].big_values > 288 )
          {
          Si.ch[ch].gr[gr].big_values        = 0;
          Si.ch[ch].gr[gr].part2_3_length    = 0;
          Si.ch[ch].gr[gr].scalefac_compress = 0;          
          }

        if (Si.ch[ch].gr[gr].window_switching_flag)
          {
          // start, stop, short or mixed block
          Si.ch[ch].gr[gr].block_type       = Bs.GetBits(2);
          Si.ch[ch].gr[gr].mixed_block_flag = Bs.GetBits(1);

          for ( i=0; i<2; i++ )
            Si.ch[ch].gr[gr].table_select[i] = Bs.GetBits(5);

          // not used, set to zero
          Si.ch[ch].gr[gr].table_select[2] = 0;

          for (i=0; i<3; i++)
            Si.ch[ch].gr[gr].subblock_gain[i] = Bs.GetBits(3);
          
          // set region_count parameters since they are implicit in this case
          if ( Si.ch[ch].gr[gr].block_type == 0 )
            {
            // block type can't be zero in this case
            Si.ch[ch].gr[gr].big_values        = 0;
            Si.ch[ch].gr[gr].part2_3_length    = 0;
            Si.ch[ch].gr[gr].scalefac_compress = 0;
            }
          else if ( Si.ch[ch].gr[gr].block_type == 2 &&
                    Si.ch[ch].gr[gr].mixed_block_flag == 0 )
            {
            // short, not mixed
            Si.ch[ch].gr[gr].region0_count = 8;
            }
          else 
            {
            // start, stop or short and mixed
            Si.ch[ch].gr[gr].region0_count = 7;
            }
          
          Si.ch[ch].gr[gr].region1_count = 20 - Si.ch[ch].gr[gr].region0_count;
          }
        else
          {
          // normal block
          for ( i=0; i<3; i++ )
            Si.ch[ch].gr[gr].table_select[i] = Bs.GetBits(5);

          Si.ch[ch].gr[gr].region0_count = Bs.GetBits(4);
          Si.ch[ch].gr[gr].region1_count = Bs.GetBits(3);
          Si.ch[ch].gr[gr].block_type    = 0;
          }

        if ( Info.IsMpeg1 )
          Si.ch[ch].gr[gr].preflag = Bs.GetBits(1);

        Si.ch[ch].gr[gr].scalefac_scale     = Bs.GetBits(1);
        Si.ch[ch].gr[gr].count1table_select = Bs.GetBits(1);
        }
      }
    return true;
    }
#if !defined(IGNORE_CRC)
  else
    {
    //
    // crc - error
    // sideinfo will be skipped by crc-check function
    //
    Si.main_data_begin = 0;

    for ( ch=0; ch<Info.stereo; ch++ )
      for ( i=0; i<4; i++ )
        Si.ch[ch].scfsi[i] = 0;

    for ( gr=0; gr<(Info.IsMpeg1 ? 2:1); gr++ )
      {
      for ( ch=0; ch<Info.stereo; ch++ )
        {
        Si.ch[ch].gr[gr].part2_3_length        = 0;
        Si.ch[ch].gr[gr].big_values            = 0;
        Si.ch[ch].gr[gr].scalefac_compress     = 0;
        Si.ch[ch].gr[gr].window_switching_flag = 0;
        Si.ch[ch].gr[gr].block_type            = 0;
        Si.ch[ch].gr[gr].mixed_block_flag      = 0;
        Si.ch[ch].gr[gr].table_select[0]       = 0;
        Si.ch[ch].gr[gr].table_select[1]       = 0;
        Si.ch[ch].gr[gr].table_select[2]       = 0;
        Si.ch[ch].gr[gr].subblock_gain[0]      = 0;
        Si.ch[ch].gr[gr].subblock_gain[1]      = 0;
        Si.ch[ch].gr[gr].subblock_gain[2]      = 0;
        Si.ch[ch].gr[gr].region0_count         = 0;
        Si.ch[ch].gr[gr].region1_count         = 0;
        Si.ch[ch].gr[gr].preflag               = 0;
        Si.ch[ch].gr[gr].scalefac_scale        = 0;
        Si.ch[ch].gr[gr].count1table_select    = 0;
        }
      }
    return false;
    }
#endif
}

/***************************************************************************\ 
 *
 *   functionname: mp3MainDataRead
 *   description : read MPEG Layer-3 main-data
 *
 *   returns : true  if there are enough data in hufman-buffer 
 *                   (back-pointer!)
 *             false otherwise
 *
 *   input   : 
 *   output  : 
 *   global  : 
 *
\***************************************************************************/

bool mp3MainDataRead
    (
    CBitStream      &Bs, // bitstream
    CBitStream      &Db, // dynamic buffer
    const MP3SI     &Si,
    const MPEG_INFO &Info
    )
{
  int nSlots = (Info.frame_bits - Info.header_size)/8;
  int nBytesValid;

  // get number of bytes in main data
  if ( Info.stereo == MONO )
    nSlots-= (Info.IsMpeg1 ? 17:9);
  else
    nSlots-= (Info.IsMpeg1 ? 32:17);

  // get number of bytes still valid in dynamic buffer
  nBytesValid = Db.GetValidBits() >> 3;

  // check if all data fits into dynamic buffer
  if ( Db.GetFree() < nSlots )
    {
    // skip some bytes to make room
    Db.Ff((nSlots - Db.GetFree()) * 8);
    }

  // copy main data to dynamic buffer
  if ( Db.Fill(Bs, nSlots) != nSlots )
    {
    // shouldn't happen, Bs should have enough data and
    // Db should have enough room
    return false;
    }

  // check if enough data available for decoding
  if ( nBytesValid >= Si.main_data_begin )
    {
    // seek to main_data_begin
    Db.Ff(Db.GetValidBits());
    Db.Rewind((nSlots + Si.main_data_begin)*8);
    return true;
    }
  else
    {
    // not enough data in buffer
    return false;
    }
}

/***************************************************************************\ 
 *
 *   functionname: mp3ScaleFactorRead
 *   description : read MPEG Layer-3 scalefactors
 *
 *   returns : <none>
 *
 *   input   : 
 *   output  : 
 *   global  : 
 *
\***************************************************************************/

void mp3ScaleFactorRead
    (
    CBitStream      &Bs,
    MP3SI_GRCH      &SiGrCh,
    MP3SCF          &ScaleFac,
    const MPEG_INFO &Info,
    const int       *pScfsi,
    int              gr,
    int              ch
    )
{
  int tab_index1 = 0;
  int tab_index2 = 0;

  int slen2[4];
  int mode;
  int sc_comp;
  int sfb, i, j, window;  

  // reset bit counter to zero
  Bs.ResetBitCnt();
  
  if ( Info.IsMpeg1 )
    {
    //
    // MPEG 1
    //
    if ( SiGrCh.window_switching_flag && (SiGrCh.block_type == 2) )
      {
      if ( SiGrCh.mixed_block_flag )
        {
        // short/mixed-block

        // long blocks, slen0
        for ( sfb = 0; sfb < 8; sfb++ )
          ScaleFac.l[sfb] = Bs.GetBits(slen[0][SiGrCh.scalefac_compress]);
        
        // short blocks, slen0
        for ( sfb = 3; sfb < 6; sfb++ )
          for (window=0; window<3; window++)
            ScaleFac.s[window][sfb] = Bs.GetBits(slen[0][SiGrCh.scalefac_compress]);            
        
        // short blocks, slen1
        for ( sfb = 6; sfb < 12; sfb++ )
          for ( window=0; window<3; window++ )
            ScaleFac.s[window][sfb] = Bs.GetBits(slen[1][SiGrCh.scalefac_compress]);

        // scalefactor for last sfb not transmitted
        for ( sfb=12,window=0; window<3; window++ )
          ScaleFac.s[window][sfb] = 0;

        // set illegal intensity-position
        for ( sfb = 0; sfb < 23; sfb++ )
          ScaleFac.l_iip[sfb] = 7;

        for ( sfb = 0; sfb < 13; sfb++ )
          ScaleFac.s_iip[sfb] = 7;
        }
      else
        {
        // short blocks
        for ( i=0; i<2; i++ )
          for ( sfb = sfbtable.s[i]; sfb < sfbtable.s[i+1]; sfb++ )
            for ( window=0; window<3; window++ )
              ScaleFac.s[window][sfb] = Bs.GetBits(slen[i][SiGrCh.scalefac_compress]);

        // scalefactor for last sfb not transmitted
        for ( sfb=12,window=0; window<3; window++ )
          ScaleFac.s[window][sfb] = 0;

        // set illegal intensity-position
        for ( sfb = 0; sfb < 13; sfb++ )
          ScaleFac.s_iip[sfb] = 7;
        }
      }
    else
      {
      // long blocks
      for ( i=0; i<4; i++ )
        {
        //
        // if gr==1 and scfsi[i]!=0: do not overwrite scalefactor from last granule!! 
        //
        if ( (gr == 0) || (pScfsi[i] == 0) )
          {
          for ( sfb = sfbtable.l[i]; sfb < sfbtable.l[i+1]; sfb++ )
            ScaleFac.l[sfb] = Bs.GetBits(slen[(i<2)?0:1][SiGrCh.scalefac_compress]);
          }
        }

      // scalefactor for last sfb not transmitted
      ScaleFac.l[21] = 0;
      ScaleFac.l[22] = 0; /* SCFB 22 */

      // set illegal intensity-position
      for ( sfb = 0; sfb < 23; sfb++ )
        ScaleFac.l_iip[sfb] = 7;
      }
    }
  else
    {
    //
    // MPEG 2, MPEG 2.5
    //
    mode            = Info.mode_ext;
    sc_comp         = SiGrCh.scalefac_compress;
    SiGrCh.preflag  = 0;

    for ( i=0; i<4; i++ )
      slen2[i] = 0;

    if ( !((mode & 0x1) && (ch==1)) )
      {
      SiGrCh.intensity_scale = 0;
      if ( sc_comp < 400 )
        {
        slen2[0]         = (sc_comp >> 4  ) /  5;
        slen2[1]         = (sc_comp >> 4  ) %  5;
        slen2[2]         = (sc_comp &  0xf) >> 2;    /* %16 */
        slen2[3]         = (sc_comp &  0x3);         /* %4  */

        tab_index1       = 0;
        }
      else if ( sc_comp < 500 )
        {
        sc_comp -= 400;

        slen2[0]         = (sc_comp >> 2  ) /  5;
        slen2[1]         = (sc_comp >> 2  ) %  5;
        slen2[2]         = (sc_comp &  0x3);         /* %4  */

        tab_index1       = 1;
        }
      else if ( sc_comp < 512 )
        {
        sc_comp -= 500;

        slen2[0]         = (sc_comp / 3);
        slen2[1]         = (sc_comp % 3);

        tab_index1       = 2;
        SiGrCh.preflag   = 1;
        }
      }
    else
      {
      SiGrCh.intensity_scale   = sc_comp & 0x1; /* intensity_scale = sc_comp % 2 */
      sc_comp                >>= 1;

      if ( sc_comp < 180 )
        {
        slen2[0]         = (sc_comp / 36);
        slen2[1]         = (sc_comp % 36) / 6;
        slen2[2]         = (sc_comp % 36) % 6;

        tab_index1       = 3;
        }
      else if ( sc_comp < 244 )
        {
        sc_comp -= 180;

        slen2[0]         = (sc_comp & 0x3f) >> 4;   /* %64 */
        slen2[1]         = (sc_comp & 0xf ) >> 2;   /* %16 */
        slen2[2]         = (sc_comp & 0x3);         /*  %4 */

        tab_index1       = 4;
        }
      else if ( sc_comp <= 255 )
        {
        sc_comp -= 244;

        slen2[0]         = (sc_comp / 3);
        slen2[1]         = (sc_comp % 3);

        tab_index1       = 5;
        }
      }

    if ( SiGrCh.block_type != 2 )
      {
      // non-short blocks
      tab_index2 = 0;
      }
    else
      {
      if ( SiGrCh.mixed_block_flag == 0 )
        {
        // short AND non-mixed
        tab_index2 = 1;
        }
      else
        {
        // short AND mixed
        tab_index2 = 2;
        }
      }

    if ( SiGrCh.block_type == 2 )
      {
      if ( SiGrCh.mixed_block_flag )
        {
        // mixed/short blocks

        // long-block part
        for ( sfb=0; sfb<6; sfb++ )
          {
          ScaleFac.l[sfb]     = Bs.GetBits(slen2[0]);
          ScaleFac.l_iip[sfb] = Mpeg2MaxSf[slen2[0]];
          }

        sfb = 3;

        // process first short-block scalefacs with slen2[0] if necc.
        for ( j=0; j<(mpeg2_nr_of_sfbands[tab_index1][tab_index2][0]-6)/3; j++ )
          {
          for ( window=0;window<3;window++ )
            ScaleFac.s[window][sfb] = Bs.GetBits(slen2[0]);

          ScaleFac.s_iip[sfb] = Mpeg2MaxSf[slen2[0]];
          sfb++;
          }

        // short-block part
        for ( i=1; i<4; i++ )
          {
          for ( j=0; j<mpeg2_nr_of_sfbands[tab_index1][tab_index2][i]/3; j++ )
            {
            for ( window=0;window<3;window++ )
              ScaleFac.s[window][sfb] = Bs.GetBits(slen2[i]);

            ScaleFac.s_iip[sfb] = Mpeg2MaxSf[slen2[i]];
            sfb++;
            }
          }

        // scalefactor for last sfb not transmitted
        for ( sfb=12,window=0; window<3; window++ )
          ScaleFac.s[window][sfb] = 0;

        ScaleFac.s_iip[12] = 1;
        }
      else
        {
        // short blocks
        sfb = 0;

        for ( i=0; i<4; i++ )
          {
          for( j=0; j<mpeg2_nr_of_sfbands[tab_index1][tab_index2][i]/3; j++ )
            {
            for ( window=0; window<3; window++ )              
              ScaleFac.s[window][sfb] = Bs.GetBits(slen2[i]);
           
            ScaleFac.s_iip[sfb] = Mpeg2MaxSf[slen2[i]];
            sfb++;
            }
          }

        // scalefactor for last sfb not transmitted
        for ( sfb=12,window=0; window<3; window++ )
          ScaleFac.s[window][sfb] = 0;

        ScaleFac.s_iip[12] = 1;
        }
      }
    else
      {
      // long blocks
      sfb = 0;      
      for ( i=0;i<4;i++ )
        {
        for ( j=0; j<mpeg2_nr_of_sfbands[tab_index1][tab_index2][i]; j++ )
          {
          ScaleFac.l[sfb]     = Bs.GetBits(slen2[i]);
          ScaleFac.l_iip[sfb] = Mpeg2MaxSf[slen2[i]];
          sfb++;
          }
        }

      // scalefactor for last sfb not transmitted
      ScaleFac.l[21]     = 0;
      ScaleFac.l_iip[21] = 1;

      ScaleFac.l[22]     = 0; /* SCFB 22 */
      ScaleFac.l_iip[22] = 1; /* SCFB 22 */
      }
    }
}

/*-------------------------------------------------------------------------*/
