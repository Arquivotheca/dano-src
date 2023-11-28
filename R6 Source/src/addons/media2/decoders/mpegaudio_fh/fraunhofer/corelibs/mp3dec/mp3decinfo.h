/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (1999)
 *                        All Rights Reserved
 *
 *   filename: mp3decinfo.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1999-09-13
 *   contents/description: MP3 Decoder Info - HEADER
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/09/15 09:04:32 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3decinfo.h,v 1.3 1999/09/15 09:04:32 sir Exp $
 */

#ifndef __MP3DECINFO_H__
#define __MP3DECINFO_H__

/* ------------------------ includes --------------------------------------*/

#ifdef USE_MP3DECIFC
  #include "mp3decifc.h"
  #define IMPLEMENTS_INTERFACE(ifc) : public ifc
  #pragma message(__FILE__": compiling CMp3DecInfo with abstract base class")
#else
  #define IMPLEMENTS_INTERFACE(ifc)
  #pragma message(__FILE__": compiling CMp3DecInfo without abstract base class")
#endif

#include "mp3fileinfo.h"
#include "mp3bufferparse.h"

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

//
// Mp3 Decoder Info Object.
//
// It is however recommended to use IMp3DecInfo (see mp3decifc.h) instead.
// Define USE_MP3DECIFC when planning to use IMp3DecInfo.
//

class CMp3DecInfo IMPLEMENTS_INTERFACE(IMp3DecInfo)
{
public:

  CMp3DecInfo();
  ~CMp3DecInfo();

  bool Init(unsigned char *pBuffer, unsigned int cbBuffer, unsigned int cbMp3Data);
  
  const CMp3FileInfo *GetInfo() const { return &m_Mp3FileInfo; }

  unsigned int SeekPointByPercent(float fPercent) const;
  unsigned int SeekPointByTime(unsigned int dwTime) const;

protected:

private:

  bool CheckForXingHeader(unsigned char *pBuffer, unsigned int cbBuffer);
  bool CheckForVbriHeader(unsigned char *pBuffer, unsigned int cbBuffer);

  bool m_bVbriHeader;
  bool m_bXingHeader;

  CMp3FileInfo         m_Mp3FileInfo;
  MPEGLAYER3WAVEFORMAT m_Mp3Wf;

  unsigned int    m_cbMp3Data;
  int             m_nSyncPosition;

  unsigned int    m_VbriVersion;
  unsigned int    m_VbriDelay;
  unsigned int    m_VbriQuality;
  unsigned int    m_VbriBytes;
  unsigned int    m_VbriFrames;
  unsigned int    m_VbriTableSize;
  unsigned int    m_VbriTableScale;
  unsigned int    m_VbriEntryBytes;
  unsigned int    m_VbriEntryFrames;
  unsigned int   *m_pVbriToc;

  unsigned int    m_XingFlags;
  unsigned int    m_XingFrames;
  unsigned int    m_XingBytes;
  unsigned char   m_XingToc[100];
  unsigned int    m_XingVbrScale;
};

/*-------------------------------------------------------------------------*/
#endif
