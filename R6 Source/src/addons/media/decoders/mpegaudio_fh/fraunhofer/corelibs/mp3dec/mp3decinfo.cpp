/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: mp3decinfo.cpp
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1999-09-13
 *   contents/description: MP3 Decoder Info
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/09/15 09:04:31 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3decinfo.cpp,v 1.5 1999/09/15 09:04:31 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mp3decinfo.h"

/*-------------------------- defines --------------------------------------*/

#ifndef NULL
  #define NULL 0
#endif

//
// flags/structure for Xing VBR header
//
static const unsigned int XingFramesFlag   = 0x00000001L;
static const unsigned int XingBytesFlag    = 0x00000002L;
static const unsigned int XingTocFlag      = 0x00000004L;
static const unsigned int XingVbrScaleFlag = 0x00000008L;

static const unsigned int VBRI_VERSION = 1;

/*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//   GetBigEndian2
//-------------------------------------------------------------------------*

static inline unsigned short GetBigEndian2(unsigned char *pBuffer)
{
  return ((unsigned int)(pBuffer[0]) <<  8) |
         ((unsigned int)(pBuffer[1])      );
}

//-------------------------------------------------------------------------*
//   GetBigEndian4
//-------------------------------------------------------------------------*

static inline unsigned int GetBigEndian4(unsigned char *pBuffer)
{
  return ((unsigned int)(pBuffer[0]) << 24) |
         ((unsigned int)(pBuffer[1]) << 16) |
         ((unsigned int)(pBuffer[2]) <<  8) |
         ((unsigned int)(pBuffer[3])      );
}

//-------------------------------------------------------------------------*
//   GetBigEndianX
//-------------------------------------------------------------------------*

static inline unsigned int GetBigEndianX(unsigned char *pBuffer, int nBytes)
{
  unsigned int X = 0;

  for ( int i=0; i<nBytes; i++ )
    {
    X <<= 8;
    X  |= (pBuffer[i] & 0xff);
    }

  return X;
}

//-------------------------------------------------------------------------*
//
//                   C M p 3 D e c I n f o 
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CMp3DecInfo::CMp3DecInfo()
{
  m_bVbriHeader   = false;
  m_bXingHeader   = false;
  m_pVbriToc      = NULL;
  m_cbMp3Data     = 0;
  m_nSyncPosition = -1;

  m_Mp3FileInfo.Reset();
}

//-------------------------------------------------------------------------*
//   destructor
//-------------------------------------------------------------------------*

CMp3DecInfo::~CMp3DecInfo()
{
  if ( m_pVbriToc )
    delete[] m_pVbriToc;
  m_pVbriToc = NULL;
}

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

bool CMp3DecInfo::Init
    (
    unsigned char *pBuffer, 
    unsigned int   cbBuffer, 
    unsigned int   cbMp3Data
    )
{
  int nAvgBitrate;

  //
  // keep number of mp3 bytes
  //
  m_cbMp3Data = cbMp3Data;

  //
  // reset fileinfo structure
  //
  m_Mp3FileInfo.Reset();

  //
  // scan buffer
  //
  m_nSyncPosition = mp3ScanBufferFormat(pBuffer, cbBuffer, m_Mp3Wf, false, &nAvgBitrate);

  //
  // fill out fileinfo structure (as good as possible w/ information available)
  //
  if ( -1 != m_nSyncPosition )
    {
    m_Mp3FileInfo.m_AvgBitrate = nAvgBitrate;
    m_Mp3FileInfo.m_Channels   = m_Mp3Wf.wfx.nChannels;
    m_Mp3FileInfo.m_Samplerate = m_Mp3Wf.wfx.nSamplesPerSec;

    if ( m_cbMp3Data > 0 && m_Mp3FileInfo.m_AvgBitrate > 0 )
      m_Mp3FileInfo.m_Duration = (unsigned int)(m_cbMp3Data / (m_Mp3FileInfo.m_AvgBitrate/8.0f) * 1000.0f);

    m_Mp3FileInfo.m_bVbr                 = (m_Mp3Wf.fdwFlags & MPEGLAYER3_FLAG_VBR) ? true : false;
    m_Mp3FileInfo.m_bVbrHeader           = false;
    m_Mp3FileInfo.m_bSeekPointsAvailable = m_Mp3FileInfo.m_bVbr ? false : true;
    }
  else
    {
    //
    // not an mp3 file
    //
    return false;
    }

  //
  // update pBuffer and cbBuffer
  //
  pBuffer  += m_nSyncPosition/8;
  cbBuffer -= m_nSyncPosition/8;

  int   SamplesPerFrame = (m_Mp3Wf.wfx.nSamplesPerSec>24000) ? 1152 : 576;
  float fFrameDuration  = float(SamplesPerFrame) / float(m_Mp3Wf.wfx.nSamplesPerSec) * 1000.0f;
  float fDuration;

  //
  // check if a VBRI header is given
  //
  CheckForVbriHeader(pBuffer, cbBuffer);

  if ( m_bVbriHeader )
    {
    m_cbMp3Data = m_VbriBytes; // might be more accurate
    fDuration   = m_VbriFrames * fFrameDuration;

    if ( fDuration > 0 )
      m_Mp3FileInfo.m_AvgBitrate = int(8.0f * float(m_cbMp3Data) / (fDuration/1000.0f));

    m_Mp3FileInfo.m_Duration             = (unsigned int)(fDuration);
    m_Mp3FileInfo.m_bVbr                 = true;
    m_Mp3FileInfo.m_bVbrHeader           = true;
    m_Mp3FileInfo.m_bSeekPointsAvailable = true;

    m_Mp3Wf.wfx.nAvgBytesPerSec          = m_Mp3FileInfo.m_AvgBitrate / 8;
    }
  else
    {
    //
    // check if a XING header is given
    //
    CheckForXingHeader(pBuffer, cbBuffer);
    
    if ( m_bXingHeader )
      {
      if ( m_XingFlags & XingBytesFlag )
        m_cbMp3Data = m_XingBytes; // might be more accurate
      
      if ( m_XingFlags & XingFramesFlag )
        {
        fDuration = m_XingFrames * fFrameDuration;

        if ( fDuration > 0 )
          m_Mp3FileInfo.m_AvgBitrate = int(8.0f * float(m_cbMp3Data) / (fDuration/1000.0f));

        m_Mp3FileInfo.m_Duration     = (unsigned int)(fDuration);

        m_Mp3Wf.wfx.nAvgBytesPerSec  = m_Mp3FileInfo.m_AvgBitrate / 8;
        }

      m_Mp3FileInfo.m_bVbr                 = true;
      m_Mp3FileInfo.m_bVbrHeader           = true;
      m_Mp3FileInfo.m_bSeekPointsAvailable = (m_XingFlags & XingTocFlag) ? true : false;
      }
    }

  return true;
}

//-------------------------------------------------------------------------*
//   SeekPointByPercent
//-------------------------------------------------------------------------*

unsigned int CMp3DecInfo::SeekPointByPercent(float fPercent) const
{
  if ( fPercent <   0.0f ) fPercent =   0.0f;
  if ( fPercent > 100.0f ) fPercent = 100.0f;

  //
  // XING header
  //
  if ( m_bXingHeader && (m_cbMp3Data > 0) && (m_XingFlags & XingTocFlag) )
    {
    // interpolate in TOC to get file seek point in bytes
    unsigned int a;
    unsigned int SeekPoint;
    float        fa, fb, fx;
    
    a = (unsigned int)fPercent;
    
    if ( a > 99 ) a = 99;
    
    fa = m_XingToc[a];
    
    if ( a < 99 )
      fb = m_XingToc[a+1];
    else
      fb = 256.0f;
    
    fx = fa + (fb-fa)*(fPercent-a);
    
    SeekPoint = (unsigned int)((1.0f/256.0f) * fx * m_cbMp3Data);
    
    if ( SeekPoint <=           0 ) SeekPoint = 0;
    if ( SeekPoint >= m_cbMp3Data ) SeekPoint = m_cbMp3Data;

    return SeekPoint;
    }

  //
  // all others are calculated by time
  //
  return SeekPointByTime((unsigned int)((fPercent/100.0f) * m_Mp3FileInfo.m_Duration));
}

//-------------------------------------------------------------------------*
//   SeekPointByTime
//-------------------------------------------------------------------------*

unsigned int CMp3DecInfo::SeekPointByTime(unsigned int dwTime) const
{
  if ( dwTime <=                        0 ) dwTime =   0;
  if ( dwTime >= m_Mp3FileInfo.m_Duration ) dwTime = m_Mp3FileInfo.m_Duration;

  //
  // XING header
  //
  if ( m_bXingHeader && (m_cbMp3Data > 0) && (m_XingFlags & XingTocFlag) )
    {
    return SeekPointByPercent((dwTime/m_Mp3FileInfo.m_Duration) * 100.0f);
    }

  unsigned int SeekPoint = 0;

  int   SamplesPerFrame = (m_Mp3Wf.wfx.nSamplesPerSec>24000) ? 1152 : 576;
  float fFrameDuration  = float(SamplesPerFrame) / float(m_Mp3Wf.wfx.nSamplesPerSec) * 1000.0f;
  float fTargetFrame    = dwTime / fFrameDuration;

  //
  // VBRI header
  //
  if ( m_bVbriHeader && m_VbriEntryFrames > 0 )
    {
    float        fNdx = fTargetFrame / m_VbriEntryFrames;
    unsigned int nNdx = (unsigned int)(fNdx);

    if ( nNdx <=               0 ) nNdx = 0;
    if ( nNdx >= m_VbriTableSize ) nNdx = m_VbriTableSize;

    unsigned int EntryA = m_pVbriToc[nNdx];
    unsigned int FrameA = nNdx * m_VbriEntryFrames;

    unsigned int EntryB = ((nNdx+1) > m_VbriTableSize) ? m_cbMp3Data  : m_pVbriToc[nNdx+1];
    unsigned int FrameB = ((nNdx+1) > m_VbriTableSize) ? m_VbriFrames : (nNdx+1) * m_VbriEntryFrames;

    SeekPoint = (unsigned int)(EntryA + float(EntryB-EntryA) / float(FrameB-FrameA) * (fTargetFrame - FrameA));
    }
  else
    {
    //
    // calculate or make best guess
    //
    if ( (m_Mp3Wf.nBlockSize > 1) && (m_Mp3Wf.nFramesPerBlock >= 1) )
      {
      SeekPoint = (unsigned int)(fTargetFrame / m_Mp3Wf.nFramesPerBlock) * m_Mp3Wf.nBlockSize;
      }
    else
      {
      SeekPoint = (unsigned int)((dwTime / 1000.0f) * (m_Mp3FileInfo.GetAvgBitrate() / 8.0f));
      }
    }

  //
  // limit SeekPoint to number of mp3 bytes
  //
  if ( SeekPoint <=           0 ) SeekPoint = 0;
  if ( SeekPoint >= m_cbMp3Data ) SeekPoint = m_cbMp3Data;

  return SeekPoint;
}

//-------------------------------------------------------------------------*
//   CheckForVbriHeader
//-------------------------------------------------------------------------*

bool CMp3DecInfo::CheckForVbriHeader
    (
    unsigned char *pBuffer,
    unsigned int   cbBuffer
    )
{
  static const unsigned int VbriHdrSize  = 4 + 2 + 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2;
  // hdr, version, delay, quality, bytes, frames, table size, table scale, entry bytes, entry frames

  unsigned int i;

  m_VbriVersion     = 0;
  m_VbriDelay       = 0;
  m_VbriQuality     = 0;
  m_VbriBytes       = 0;
  m_VbriFrames      = 0;
  m_VbriTableSize   = 0;
  m_VbriTableScale  = 0;
  m_VbriEntryBytes  = 0;
  m_VbriEntryFrames = 0;
  
  if ( m_pVbriToc )
    delete[] m_pVbriToc;
  m_pVbriToc = NULL;

  //
  // determine offset of vbri header
  //
  unsigned int VbriHdrOffset = 4 + 32;

  //
  // check if buffer is large enough
  //
  if ( cbBuffer < VbriHdrOffset + VbriHdrSize )
    {
    return false;
    }

  //
  // go to start of vbri header
  //
  pBuffer += VbriHdrOffset;

  //
  // check for vbri header id
  //
  if ( pBuffer[0] != 'V' ) return false;
  if ( pBuffer[1] != 'B' ) return false;
  if ( pBuffer[2] != 'R' ) return false;
  if ( pBuffer[3] != 'I' ) return false;
  pBuffer += 4;

  //
  // version
  //
  m_VbriVersion = GetBigEndian2(pBuffer);
  pBuffer += 2;

  if ( VBRI_VERSION != m_VbriVersion )
    return false; // wrong version

  //
  // yes, this is a vbri header
  //
  m_bVbriHeader = true;

  //
  // delay
  //
  m_VbriDelay = GetBigEndian2(pBuffer);
  pBuffer += 2;

  //
  // quality
  //
  m_VbriQuality = GetBigEndian2(pBuffer);
  pBuffer += 2;

  //
  // bytes
  //
  m_VbriBytes = GetBigEndian4(pBuffer);
  pBuffer += 4;
  
  //
  // frames
  //
  m_VbriFrames = GetBigEndian4(pBuffer);
  pBuffer += 4;
  
  //
  // table size
  //
  m_VbriTableSize = GetBigEndian2(pBuffer);
  pBuffer += 2;

  //
  // table scale
  //
  m_VbriTableScale = GetBigEndian2(pBuffer);
  pBuffer += 2;

  //
  // entry bytes
  //
  m_VbriEntryBytes = GetBigEndian2(pBuffer);
  pBuffer += 2;

  //
  // entry frames
  //
  m_VbriEntryFrames = GetBigEndian2(pBuffer);
  pBuffer += 2;

  //
  // check if buffer is large enough
  //
  if ( cbBuffer < VbriHdrOffset + VbriHdrSize + m_VbriTableSize * m_VbriEntryBytes )
    {
    m_bVbriHeader = false;
    return false;
    }

  //
  // table data
  //
  m_pVbriToc = new unsigned int[m_VbriTableSize+1];

  if ( m_pVbriToc )
    {
    m_pVbriToc[0] = 0;

    for ( i=1; i<m_VbriTableSize+1; i++ )
      {
      m_pVbriToc[i] = m_pVbriToc[i-1] + GetBigEndianX(pBuffer, m_VbriEntryBytes) * m_VbriTableScale;
      pBuffer += m_VbriEntryBytes;
      }
    }
  else
    {
    // failed to allocate memory for TOC
    m_bVbriHeader = false;
    return false;
    }

  return true;
}

//-------------------------------------------------------------------------*
//   CheckForXingHeader
//-------------------------------------------------------------------------*

bool CMp3DecInfo::CheckForXingHeader
    (
    unsigned char *pBuffer,
    unsigned int   cbBuffer
    )
{
  static const int sr_table[4] = { 44100, 48000, 32000, 99999 };
  static const int XingHdrSize = 4 + 4 + 4 + 4 + 100 + 4; 
                                 // hdr, flags, frames, bytes, toc, vbr-scale
  
  unsigned int i;

  //
  // reset everything
  //
  m_bXingHeader  = false;
  m_XingBytes    = 0;
  m_XingFlags    = 0;
  m_XingFrames   = 0;
  m_XingVbrScale = 0;

  for ( i=0; i<100; i++ )
    m_XingToc[i] = 0;

  //
  // read some MPEG header data
  //
  int h_idex     = (pBuffer[1] >> 4) & 1;
  int h_id       = (pBuffer[1] >> 3) & 1;
  int h_sr_index = (pBuffer[2] >> 2) & 3;
  int h_mode     = (pBuffer[3] >> 6) & 3;

  int samplerate = sr_table[h_sr_index];
  
  if ( h_id == 0 && h_idex == 1 )
    samplerate /= 2; // MPEG 2

  if ( h_id == 0 && h_idex == 0 )
    samplerate /= 4; // MPEG 2.5

  //
  // determine offset of xing header
  //
  unsigned int XingHdrOffset;

  if ( samplerate >= 32000 )
    {
    // mpeg-1
    XingHdrOffset = (h_mode != 3) ? 32+4 : 17+4;
    }
  else if ( samplerate >= 16000 )
    {
    // mpeg-2
    XingHdrOffset = (h_mode != 3) ? 17+4 : 9+4;
    }
  else
    {
    // mpeg-2.5 (not supported)
    return false;
    }

  //
  // check if buffer is large enough
  //
  if ( cbBuffer < (XingHdrOffset + XingHdrSize) )
    {
    return false;
    }

  //
  // go to start of xing header
  //
  pBuffer += XingHdrOffset;

  //
  // check for xing header id
  //
  if ( pBuffer[0] != 'X' ) return false;
  if ( pBuffer[1] != 'i' ) return false;
  if ( pBuffer[2] != 'n' ) return false;
  if ( pBuffer[3] != 'g' ) return false;
  pBuffer += 4;

  //
  // yes, this is a xing header
  //
  m_bXingHeader = true;

  //
  // get flags
  //
  m_XingFlags = GetBigEndian4(pBuffer);
  pBuffer += 4;

  //
  // get frames
  //
  if ( m_XingFlags & XingFramesFlag )
    {
    m_XingFrames = GetBigEndian4(pBuffer);
    pBuffer += 4;
    }

  //
  // get bytes
  //
  if ( m_XingFlags & XingBytesFlag )
    {
    m_XingBytes = GetBigEndian4(pBuffer);
    pBuffer += 4;
    }

  //
  // get toc
  //
  if ( m_XingFlags & XingTocFlag )
    {
    for ( i=0; i<100; i++ )
      m_XingToc[i]  = pBuffer[i];

    pBuffer  += 100;
    }

  //
  // get vbr scale
  //
  if ( m_XingFlags & XingVbrScaleFlag )
    {
    m_XingVbrScale = GetBigEndian4(pBuffer);
    pBuffer += 4;
    }

  return true;
}

/*-------------------------------------------------------------------------*/
