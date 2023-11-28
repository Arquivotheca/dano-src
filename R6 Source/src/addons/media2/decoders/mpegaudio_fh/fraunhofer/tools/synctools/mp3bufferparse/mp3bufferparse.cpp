/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3bufferparse.cpp
 *   project : MPEG Syncer
 *   author  : Martin Sieler
 *   date    : 1998-08-26
 *   contents/description: parse a buffer for mp3 format
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/09/14 18:09:20 $
 * $Header: /home/cvs/mms/tools/synctools/mp3bufferparse/mp3bufferparse.cpp,v 1.5 1999/09/14 18:09:20 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mpegbitstream.h"
#include "sequencedetector.h"
#include "meanvalue.h"

#include "mp3bufferparse.h"

/*-------------------------- defines --------------------------------------*/

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
//   CheckForVbriHeader
//-------------------------------------------------------------------------*

static bool CheckForVbriHeader
    (
    unsigned char              *pBuffer,
    unsigned int                cbBuffer,
    const MPEGLAYER3WAVEFORMAT,
    unsigned int               &nFrames,
    unsigned int               &nBytes
    )
{
  static const unsigned int VBRI_VERSION = 1;
  static const unsigned int VbriHdrSize  = 4 + 2 + 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2;
  // hdr, version, delay, quality, bytes, frames, table size, table scale, entry bytes, entry frames

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
  unsigned int VbriVersion = GetBigEndian2(pBuffer);
  pBuffer += 2;

  if ( VBRI_VERSION != VbriVersion )
    return false; // wrong version

  //
  // yes, this is a vbri header
  //

  //
  // delay
  //
  pBuffer += 2;

  //
  // quality
  //
  pBuffer += 2;

  //
  // bytes
  //
  nBytes = GetBigEndian4(pBuffer);
  pBuffer += 4;
  
  //
  // frames
  //
  nFrames = GetBigEndian4(pBuffer);
  pBuffer += 4;

  return true;
}

//-------------------------------------------------------------------------*
//   CheckForXingHeader
//-------------------------------------------------------------------------*

static bool CheckForXingHeader
    (
    unsigned char              *pBuffer,
    unsigned int                cbBuffer,
    const MPEGLAYER3WAVEFORMAT &wf,
    unsigned int               &nFrames,
    unsigned int               &nBytes
    )
{
  //
  // flags/structure for Xing VBR header
  //
  static const unsigned int XingFramesFlag = 0x00000001L;
  static const unsigned int XingBytesFlag  = 0x00000002L;
  static const unsigned int XingHdrSize    = 4 + 4 + 4 + 4 + 100 + 4; // hdr, flags, frames, bytes, toc, vbr-scale

  nFrames = 0;
  nBytes  = 0;

  //
  // determine offset of xing header
  //
  unsigned int XingHdrOffset;
  if ( wf.wfx.nSamplesPerSec >= 32000 )
    {
    // mpeg-1
    XingHdrOffset = (wf.wfx.nChannels==2) ? 32+4 : 17+4;
    }
  else if ( wf.wfx.nSamplesPerSec >= 16000 )
    {
    // mpeg-2
    XingHdrOffset = (wf.wfx.nChannels==2) ? 17+4 : 9+4;
    }
  else
    {
    // mpeg-2.5 (not supported)
    return false;
    }

  //
  // check if buffer is large enough
  //
  if ( cbBuffer < XingHdrOffset + XingHdrSize )
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

  //
  // get flags
  //
  unsigned int XingFlags = GetBigEndian4(pBuffer);
  pBuffer += 4;

  //
  // get frames
  //
  if ( XingFlags & XingFramesFlag )
    {
    nFrames = GetBigEndian4(pBuffer);
    pBuffer += 4;
    }

  //
  // get bytes
  //
  if ( XingFlags & XingBytesFlag )
    {
    nBytes = GetBigEndian4(pBuffer);
    pBuffer += 4;
    }

  return true;
}

//-------------------------------------------------------------------------*
//   mp3ScanBufferFormat
//-------------------------------------------------------------------------*

int mp3ScanBufferFormat
    (
    unsigned char        *pBuffer, 
    int                   cbSize, 
    MPEGLAYER3WAVEFORMAT &wf,
    bool                  fForceBlocksize, /* = false */
    int                  *pBitrate         /* = NULL  */
    )
{
  CMpegBitStream    mbs(pBuffer, cbSize, true);
  SSC               ssc          = SSC_OK;
  bool              bValid       = true;
  bool              fFirstTime   = true;
  int               SyncPosition = -1;

  CMeanValue        MeanChannels;
  CMeanValue        MeanSampleRate;
  CMeanValue        MeanBitrate;
  CMeanValue        MeanPadding;
  CMeanValue        MeanCrcCheck;

  CSequenceDetector BlockAlign(4);
  CSequenceDetector Bitrate(4);

  if ( pBitrate )
    *pBitrate = 0;

  mbs.SetEof();

  while ( bValid && SSC_W_MPGA_SYNCEOF != ssc )
    {
    // sync to next frame
    ssc = mbs.DoSync();

    // check for success
    if ( SSC_SUCCESS(ssc) )
      {
      // success, do the work

      if ( fFirstTime )
        {
        fFirstTime   = false;
        SyncPosition = mbs.GetSyncPosition();
        }

      //
      // gather stats about this mp3 track
      //
      MeanChannels   += mbs.GetHdr()->GetChannels();
      MeanSampleRate += mbs.GetHdr()->GetSampleRate();
      MeanBitrate    += mbs.GetHdr()->GetBitrate();
      MeanPadding    += mbs.GetHdr()->GetPadding();
      MeanCrcCheck   += mbs.GetHdr()->GetCrcCheck();
      BlockAlign     += mbs.GetHdr()->GetFrameLen()/8;
      Bitrate        += mbs.GetHdr()->GetBitrate();

      // seek to end of frame
      mbs.Seek(mbs.GetHdr()->GetFrameLen() - mbs.GetBitCnt());
      }
    else
      {
      // no sync
      if ( SSC_ERROR(ssc) )
        bValid = false;
      }
    }

  if ( fFirstTime == false && bValid )
    {
    int fdwFlags = 0;
    int nBa;
    int nFpb;

    // get blockalign and frames per block
    if ( MeanBitrate.IsFixed() && !MeanPadding.IsFixed() )
      {
      // prevent detection if bitrate is fixed and padding is ISO
      nBa  = 1;
      nFpb = 1;
      }
    else
      {
      nBa  = BlockAlign.GetSum()    ? BlockAlign.GetSum()    : 1;
      nFpb = BlockAlign.GetLength() ? BlockAlign.GetLength() : 1;
      }

    // set padding flag
    if ( MeanPadding.IsFixed() )
      {
      // padding was always on or always off
      fdwFlags |= (int(MeanPadding) == 0) ? MPEGLAYER3_FLAG_PADDING_OFF : MPEGLAYER3_FLAG_PADDING_ON;
      }
    else
      {
      // padding flag switched between on and off
      fdwFlags |= MPEGLAYER3_FLAG_PADDING_ISO;
      }

    // set crc flag
    if ( MeanCrcCheck.IsFixed() )
      {
      fdwFlags |= (int(MeanCrcCheck) == 0) ? MPEGLAYER3_FLAG_CRC_OFF : MPEGLAYER3_FLAG_CRC_ON;
      }

    // calculate bitrate of the detected sequence or, if none is found,
    // from mean bitrate
    // [+++++ em added cast 25jan00]
    int myBitrate = (Bitrate.GetLength() > 0) ? int(Bitrate.GetSum() / Bitrate.GetLength()) : int(MeanBitrate);

    // check for variable bitrate
    if ( Bitrate.GetLength() == 0 )
      fdwFlags |= MPEGLAYER3_FLAG_VBR;

    //
    // set members of wf
    //
    wf.wfx.wFormatTag      = WAVE_FORMAT_MPEGLAYER3;
    wf.wfx.nChannels       = int(MeanChannels);
    wf.wfx.nSamplesPerSec  = int(MeanSampleRate);
    wf.wfx.nAvgBytesPerSec = myBitrate / 8;
    wf.wfx.nBlockAlign     = 1;
    wf.wfx.wBitsPerSample  = 0;
    wf.wfx.cbSize          = MPEGLAYER3_WFX_EXTRA_BYTES;
    
    wf.wID                 = MPEGLAYER3_ID_MPEG;
    wf.fdwFlags            = fdwFlags;
    wf.nBlockSize          = nBa;
    wf.nFramesPerBlock     = nFpb;
    wf.nCodecDelay         = 0; // unknown delay

    if ( pBitrate )
      *pBitrate = myBitrate;

    // if we need to fill something in here, let's make a good guess
    if ( wf.nBlockSize == 1 && fForceBlocksize )
      {
      int sampleRate     = wf.wfx.nSamplesPerSec;
      int multiplier     = (sampleRate > 24000) ? 144 : 72;
      int bytesPerPacket = (int)((double)myBitrate / (double)sampleRate * (double)multiplier);

      wf.nBlockSize = wf.nFramesPerBlock * bytesPerPacket;
      }

    //
    // check for various VBR headers
    //
    if ( 0 == (SyncPosition % 8) ) // bitstream must be byte-aligned
      {
      bool         bVbrHeader;
      unsigned int nFrames = 0;
      unsigned int nBytes  = 0;

      //
      // check if a VBRI header is present
      //
      bVbrHeader = CheckForVbriHeader(&pBuffer[SyncPosition/8], cbSize - SyncPosition/8, wf, nFrames, nBytes);

      //
      // ... if not, check if a Xing header is present
      //
      if ( !bVbrHeader )
        bVbrHeader = CheckForXingHeader(&pBuffer[SyncPosition/8], cbSize - SyncPosition/8, wf, nFrames, nBytes);

      //
      // if all needed information is available, patch bitrate/nAvgBytesPerSec
      //
      if ( bVbrHeader && nFrames > 0 && nBytes > 0 )
        {
        int   SamplesPerFrame = (wf.wfx.nSamplesPerSec>24000) ? 1152 : 576;
        float fFrameDuration  = float(SamplesPerFrame) / float(wf.wfx.nSamplesPerSec) * 1000.0f;
        float fDuration       = nFrames * fFrameDuration;
        int   AvgBitRate      = int(8.0f * float(nBytes) / (fDuration/1000.0f));

        wf.fdwFlags            |= MPEGLAYER3_FLAG_VBR;
        wf.wfx.nAvgBytesPerSec  = AvgBitRate / 8;

        if ( pBitrate )
          *pBitrate = AvgBitRate;
        }
      }

    } // if ( fFirstTime == false && bValid )

  return bValid ? SyncPosition : -1;
}

/*-------------------------------------------------------------------------*/
