/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1997)
 *                        All Rights Reserved
 *
 *   filename: mpegbitstream.cpp
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1997-12-05
 *   contents/description: ISO/MPEG bitstream
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/02/18 09:46:07 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mpegbitstream.cpp,v 1.11 1999/02/18 09:46:07 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mpegbitstream.h"

/*-------------------------- defines --------------------------------------*/

/*
 * mask syncword, idex, id, layer, sampling-frequency
 */
static const unsigned long gdwHeaderSyncMask = 0xfffe0c00L;


/*-------------------------------------------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C M p e g B i t S t r e a m
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CMpegBitStream::CMpegBitStream(int cbSize) : CBitStream(cbSize)
{
  Reset();
}

//-------------------------------------------------------------------------*

CMpegBitStream::CMpegBitStream(unsigned char *pBuf, int cbSize, bool fDataValid) :
    CBitStream(pBuf, cbSize, fDataValid)
{
  // must do the same as Reset(), exept of call to CBitStream::Reset()!!
  m_SyncState      = SSC_W_MPGA_SYNCSEARCHED;
  m_FirstHdr       = 0;
  m_nFramesToCheck = 0;
  m_SyncPosition   = 0;
}

//-------------------------------------------------------------------------*
//   destructor
//-------------------------------------------------------------------------*

CMpegBitStream::~CMpegBitStream()
{
}

//-------------------------------------------------------------------------*
//   Reset
//-------------------------------------------------------------------------*

void CMpegBitStream::Reset()
{
  CBitStream::Reset();

  m_SyncState      = SSC_W_MPGA_SYNCSEARCHED;
  m_FirstHdr       = 0;
  m_nFramesToCheck = 0;
  m_SyncPosition   = 0;
}

//-------------------------------------------------------------------------*
//   DoSync
//-------------------------------------------------------------------------*

SSC CMpegBitStream::DoSync()
{
  // don't do anything in case of EOF
  if ( m_SyncState == SSC_W_MPGA_SYNCEOF )
    return m_SyncState;

  // automatic refill if connected
  if ( IsConnected() && 
       ( (m_Hdr.GetFrameLen() && GetValidBits() < m_Hdr.GetFrameLen()) ||
         (m_SyncState == SSC_W_MPGA_SYNCNEEDDATA) ||
         (m_SyncState == SSC_W_MPGA_SYNCSEARCHED) ||
         (GetValidBits() == 0)
       )
     )
    {
    Refill();
    }

  // do the sync
  if ( GetValidBits() < 32 )
    {
    // if there are less than 32 bits -> no sync possible 
    if ( (m_SyncState == SSC_OK) || (m_SyncState == SSC_W_MPGA_SYNCNEEDDATA) )
      m_SyncState = SSC_W_MPGA_SYNCNEEDDATA;
    else
      m_SyncState = SSC_W_MPGA_SYNCSEARCHED;
    }
  else
    {
    // at least 32bits in buffer, try to sync
    // *** begin MMX bitread block
    BeginRead();
    
    if ( (m_SyncState == SSC_OK) || (m_SyncState == SSC_W_MPGA_SYNCNEEDDATA) )
      {
      // sync continue 
      m_SyncState = DoSyncContinue();
      }
    else
      {
      // initial sync 
      m_SyncState = DoSyncInitial();
      }

    // *** end MMX bitread block
    EndRead();
    }

  // check for EOF
  if ( IsEof() && 
       (m_SyncState == SSC_W_MPGA_SYNCSEARCHED || m_SyncState == SSC_W_MPGA_SYNCNEEDDATA) )
    m_SyncState = SSC_W_MPGA_SYNCEOF;

  return m_SyncState;
}

//-------------------------------------------------------------------------*
//   DoSyncInitial
//-------------------------------------------------------------------------*

SSC CMpegBitStream::DoSyncInitial()
{
  unsigned long ulHdr1;
  unsigned long ulHdr2;

  // keep track of actual position
  ResetBitCnt();

  // (try to) sync while there are more than 32 bits 
  while ( GetValidBits() >= 32 )
    {
    // read header (32bits)
    ulHdr1 = Get32Bits();

    // check, if header is valid
    if ( m_Hdr.FromInt(ulHdr1) )
      {
      if ( GetValidBits() < m_Hdr.GetFrameLen() )
        {
        //
        // not enough data in buffer
        // --> SYNC SEARCHED
        //
        Rewind(GetBitCnt());
        return SSC_W_MPGA_SYNCSEARCHED;
        }
      else
        {
        // seek to beginning of (possible) next frame
        // (as we already read 32 bits for the first header
        //  we have to subtract 32 bits here)
        Ff(m_Hdr.GetFrameLen()-32);

        // read header (32bits)
        ulHdr2 = Get32Bits();

        // check if "relevant" bits/fields are consistent
        if ( !((ulHdr1 ^ ulHdr2) & gdwHeaderSyncMask) )
          {
          //
          // SYNC OK
          //
           
          // mask out relevant fields from header and save
          m_FirstHdr = ulHdr1 & gdwHeaderSyncMask;

          // set number of frames, to be checked for next mpeg header
          m_nFramesToCheck = FRAMES_TO_CHECK;

          // clean up
          Rewind(GetBitCnt());
          return SSC_OK;
          }
        }
      }

    // skip one bit and try again
    m_SyncPosition++;
    Rewind(GetBitCnt()-1);
    ResetBitCnt();
    }

  // buffer emtpy
  return SSC_W_MPGA_SYNCSEARCHED;
}

//-------------------------------------------------------------------------*
//   DoSyncContinue
//-------------------------------------------------------------------------*

SSC CMpegBitStream::DoSyncContinue()
{
  unsigned long ulHdr1;

  // reset m_SyncPosition
  m_SyncPosition = 0;

  // keep track of actual position 
  ResetBitCnt();

  // read header (32bits)
  ulHdr1 = Get32Bits();

  //
  // check if
  //  - "relevant" bits/fields did change OR
  //  - header is not valid
  //
  if ( ((ulHdr1 & gdwHeaderSyncMask) != m_FirstHdr) || !m_Hdr.FromInt(ulHdr1) )
    {
    //
    // "relevant" bits in header changed OR this header is not valid
    // --> SYNC LOST
    //
    Rewind(GetBitCnt());
    return SSC_W_MPGA_SYNCLOST;
    }

  //
  // check if at least the whole frame is in the buffer
  // (have to add 32 bits to valid bits, as we already read the header)
  //
  if ( GetValidBits()+32 < m_Hdr.GetFrameLen() )
    {
    //
    // not enough bits in buffer to hold entire frame
    // --> SYNC NEEDDATA
    //
    Rewind(GetBitCnt());
    return SSC_W_MPGA_SYNCNEEDDATA;
    }

  //
  // looks like SYNC OK, but we want to check the next header for consistency as well
  // (but only for the first xx frames after last successful SyncInitial())
  //
  if ( m_nFramesToCheck > 0 && GetValidBits() >= m_Hdr.GetFrameLen() )
    {
    unsigned long ulHdr2;

    // seek to beginning of (possible) next frame
    // (as we already read 32 bits for the first header
    // we have to subtract 32 bits here)
    Ff(m_Hdr.GetFrameLen()-32);
    
    // read header (32bits)
    ulHdr2 = Get32Bits();
    
    // check consistency of header
    if ( (ulHdr2 & gdwHeaderSyncMask) != m_FirstHdr )
      {
      //
      // MPEG header of next frame does not match
      // --> SYNC LOST
      //
      Rewind(GetBitCnt());
      return SSC_W_MPGA_SYNCLOST;
      }
    }

  //
  // all tests passed
  // --> SYNC OK
  //

  // decrement number of frames to be checked for next mpeg header
  // (not below zero)
  if ( m_nFramesToCheck > 0 )
    m_nFramesToCheck--;

  Rewind(GetBitCnt());
  return SSC_OK;
}

/*-------------------------------------------------------------------------*/
