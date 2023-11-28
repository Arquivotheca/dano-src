/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (1997)
 *                        All Rights Reserved
 *
 *   filename: mpegbitstream.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1997-12-05
 *   contents/description: MPEG bitstream - HEADER
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/03/11 08:51:57 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mpegbitstream.h,v 1.10 1999/03/11 08:51:57 sir Exp $
 */

#ifndef __MPEGBITSTREAM_H__
#define __MPEGBITSTREAM_H__

/* ------------------------ includes --------------------------------------*/

#include "bitstream.h"
#include "mpegheader.h"
#include "mp3sscdef.h"

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

//
// MPEG bitstream class.
//
//  This object is derived from CBitStream. In addition to CBitStream
//  this object is able to sync to the next ISO/MPEG header position.
//

class CMpegBitStream : public CBitStream
{
public:
  CMpegBitStream(int cbSize);
  CMpegBitStream(unsigned char *pBuf, int cbSize, bool fDataValid = false);
  virtual ~CMpegBitStream();

  virtual void       Reset();

  SSC                DoSync();
  int                GetSyncPosition() const { return m_SyncPosition; }
  const CMpegHeader *GetHdr() const { return &m_Hdr; }

protected:

private:

  SSC DoSyncInitial();
  SSC DoSyncContinue();

  enum { FRAMES_TO_CHECK = 10 };

  CMpegHeader   m_Hdr;            // mpeg header
  unsigned long m_FirstHdr;       // "relevant" bits of first good header
  unsigned long m_nFramesToCheck; // # frames to be checked for next mpeg header
  int           m_SyncPosition;   // offset of first sync in bits
  SSC           m_SyncState;      // last sync state
};

/*-------------------------------------------------------------------------*/
#endif
