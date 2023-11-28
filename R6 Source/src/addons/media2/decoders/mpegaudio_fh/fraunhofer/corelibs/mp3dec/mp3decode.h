/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3decode.h
 *   project : ISO/MPEG-Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: MPEG Layer-3 decoder
 *
 * MODIFICATIONS by em@be.com:
 *   implement IMpgaDecode
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/06/24 10:31:14 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3decode.h,v 1.15 1999/06/24 10:31:14 sir Exp $
 */

#ifndef __MP3DECODE_H__
#define __MP3DECODE_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"
#include "mpegbitstream.h"
#include "huffdec.h"
#include "mdct.h"
#include "polyphase.h"
#include "mpga_internal.h"

#ifdef ERROR_CONCEALMENT
  #include "conceal.h"
#endif

/*-------------------------------------------------------------------------*/

//
// MPEG Layer-3 decoding class.
//
//  This is the main MPEG Layer-3 decoder object.
//

class CMp3Decode : public IMpgaDecode
{
public:

  CMp3Decode(CMpegBitStream &_Bs, int _Quality, int _Resolution, int _Downmix);

  ~CMp3Decode();

  void Init(bool fFullReset = true);

  // PcmFormat: 0: integer, 1: 32 bit float (IEEE)
  SSC Decode(void *pPcm, int cbPcm, int *pcbUsed, int PcmFormat = 0);

	bool SupportsLayer(int layer) const;

protected:
 
  SSC  DecodeOnNoMainData(void *pPcm, int PcmFormat);
  SSC  DecodeNormal      (void *pPcm, int PcmFormat, bool fCrcOk);

  void PolyphaseReorder();
  void ZeroISpectrum();
  void ZeroSpectrum();
  void ZeroPolySpectrum();
  void SetInfo();

  CMp3Huffman       m_Mp3Huffman;  // huffman decoder
  CMdct             m_Mdct;        // mdct
  CPolyphase        m_Polyphase;   // polyphase

#ifdef ERROR_CONCEALMENT
  CErrorConcealment m_Conceal;     // error concealment
#endif

  MPEG_INFO         m_Info;        // info structure
  CMpegBitStream   &m_Bs;          // bitstream
  CBitStream        m_Db;          // dynamic buffer
  MP3SI             m_Si;          // side info
  MP3SCF            m_ScaleFac[2]; // scalefactors

  long              m_ISpectrum[2][SSLIMIT*SBLIMIT]; // spectrum (integer)
  SPECTRUM          m_Spectrum;                      // spectrum (float)
  POLYSPECTRUM      m_PolySpectrum;                  // spectrum (post-mdct)

  int               m_Quality;        // 0: full, 1: half, 2: quarter
  int               m_Resolution;     // 0: 16 bit, 1: 8 bit
  int               m_Downmix;        // 0: no downmix, 1: downmix

  enum { dynBufSize = 2048 } ;
  
  unsigned char     m_dynBufMemory [dynBufSize] ;

private:
};

/*-------------------------------------------------------------------------*/
#endif
