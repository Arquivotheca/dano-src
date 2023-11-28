/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1997)
 *                        All Rights Reserved
 *
 *   filename: mpegheader.cpp
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1997-12-05
 *   contents/description: ISO/MPEG Header
 *
 *
\***************************************************************************/

/*
 * $Date: 1998/06/04 15:54:33 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mpegheader.cpp,v 1.6 1998/06/04 15:54:33 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mpegheader.h"
#include "bitstream.h"

/*-------------------------- defines --------------------------------------*/

#define MPEG_SYNCWORD 0x7ff

#define MPEG1          0
#define MPEG2          1
#define MPEG25         2

#define MODE_MONO      3

/*-------------------------------------------------------------------------*/

static const int gaSampleRate[3][4] = 
{
  {44100, 48000, 32000, 0}, // MPEG-1
  {22050, 24000, 16000, 0}, // MPEG-2
  {11025, 12000,  8000, 0}, // MPEG-2.5
};

static const int gaBitrate[2][3][15] =
{
  {
  // MPEG-1
  {  0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448}, // Layer 1
  {  0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384}, // Layer 2
  {  0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320}, // Layer 3
  },

  {
  // MPEG-2, MPEG-2.5
  {  0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256}, // Layer 1
  {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, // Layer 2
  {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, // Layer 3
  },
};

static const int gaSamplesPerFrame[3][3] = 
{
  // Layer 1, Layer 2, Layer 3
  {  384,     1152,    1152 }, // MPEG1
  {  384,     1152,     576 }, // MPEG2
  {  384,     1152,     576 }, // MPEG2.5
};

static const int gaBitsPerSlot[3] = { 32, 8, 8 }; // Layer 1, 2, 3

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C M p e g H e a d e r
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CMpegHeader::CMpegHeader()
{
  // reset data
  ResetMembers();
  m_HeaderValid = 0;
}

//-------------------------------------------------------------------------*
//   destructor
//-------------------------------------------------------------------------*

CMpegHeader::~CMpegHeader()
{
}

//-------------------------------------------------------------------------*
//   ReadFrom
//-------------------------------------------------------------------------*

int CMpegHeader::ReadFrom(CBitStream &Bs)
{
  return FromInt(Bs.Get32Bits());
}

//-------------------------------------------------------------------------*
//   FromInt
//-------------------------------------------------------------------------*

int CMpegHeader::FromInt(unsigned long dwHdrBits)
{
  // read header fields
  m_Syncword      =     (dwHdrBits >> 21) & 0x000007ff;
  m_Idex          =     (dwHdrBits >> 20) & 0x00000001;
  m_Id            =     (dwHdrBits >> 19) & 0x00000001;
  m_Layer         = 4 -((dwHdrBits >> 17) & 0x00000003);
  m_CrcCheck      =   !((dwHdrBits >> 16) & 0x00000001);
  m_BitrateNdx    =     (dwHdrBits >> 12) & 0x0000000f;
  m_SampleRateNdx =     (dwHdrBits >> 10) & 0x00000003;
  m_Padding       =     (dwHdrBits >>  9) & 0x00000001;
  m_Private       =     (dwHdrBits >>  8) & 0x00000001;
  m_Mode          =     (dwHdrBits >>  6) & 0x00000003;
  m_ModeExt       =     (dwHdrBits >>  4) & 0x00000003;
  m_Copyright     =     (dwHdrBits >>  3) & 0x00000001;
  m_Original      =     (dwHdrBits >>  2) & 0x00000001;
  m_Emphasis      =     (dwHdrBits      ) & 0x00000003;

  // check if header is valid
  if ( 
       (m_Syncword      != MPEG_SYNCWORD) ||

#ifndef SYNC_ALL_LAYERS
       (m_Layer         !=  3           ) ||
#else
       (m_Layer         ==  4           ) ||
#endif

       (m_BitrateNdx    ==  15          ) ||
       (m_BitrateNdx    ==  0           ) ||
       (m_SampleRateNdx == 3            ) ||
       (m_Idex == 0 && m_Layer != 3     ) ||
       (m_Idex == 0 && m_Id == 1 && m_Layer == 3)
     )
    {
    m_HeaderValid = 0;
    ResetMembers();
    }
  else
    {
    m_HeaderValid = 1;
    SetMembers();
    }

  return m_HeaderValid;
}

//-------------------------------------------------------------------------*
//   Get<xxx> Functions
//-------------------------------------------------------------------------*

int   CMpegHeader::GetSamplesPerFrame() const
  { return gaSamplesPerFrame[m_MpegVersion][m_Layer-1]; }

//-------------------------------------------------------------------------*
//   CalcFrameLen
//-------------------------------------------------------------------------*

int CMpegHeader::CalcFrameLen()
{
  int nBitsPerSlot;
  int nSamplesPerFrame;
  int nAvgSlotsPerFrame;

  nBitsPerSlot     = gaBitsPerSlot[m_Layer-1];
  nSamplesPerFrame = gaSamplesPerFrame[m_MpegVersion][m_Layer-1];

  nAvgSlotsPerFrame = (nSamplesPerFrame * (m_Bitrate / nBitsPerSlot)) / 
                      m_SampleRate;

  return (nAvgSlotsPerFrame + m_Padding) * nBitsPerSlot;
}

//-------------------------------------------------------------------------*
//   ResetMembers
//-------------------------------------------------------------------------*

void CMpegHeader::ResetMembers()
{
  m_Syncword      = 0;
  m_Idex          = 0;
  m_Id            = 0;
  m_Layer         = 0;
  m_CrcCheck      = 0;
  m_BitrateNdx    = 0;
  m_SampleRateNdx = 0;
  m_Padding       = 0;
  m_Private       = 0;
  m_Mode          = 0;
  m_ModeExt       = 0;
  m_Copyright     = 0;
  m_Original      = 0;
  m_Emphasis      = 0;

  m_HeaderValid   = 0;
  m_MpegVersion   = 0;
  m_Channels      = 0;
  m_SampleRate    = 0;
  m_Bitrate       = 0;
  m_FrameLen      = 0;
  m_Duration      = 0.0f;
}

//-------------------------------------------------------------------------*
//   SetMembers
//-------------------------------------------------------------------------*

void CMpegHeader::SetMembers()
{
  if ( m_HeaderValid )
    {
    m_MpegVersion = m_Id==1 ? MPEG1 : (m_Idex==1?MPEG2 : MPEG25);
    m_Channels    = m_Mode == MODE_MONO ? 1:2;
    m_SampleRate  = gaSampleRate[m_MpegVersion][m_SampleRateNdx];
    m_Bitrate     = gaBitrate[m_MpegVersion==MPEG1?0:1][m_Layer-1][m_BitrateNdx] * 1000;
    m_FrameLen    = CalcFrameLen();
    m_Duration    = (GetSamplesPerFrame() * 1000.0f) / m_SampleRate;
    }
}

/*-------------------------------------------------------------------------*/
