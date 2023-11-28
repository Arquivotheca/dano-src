/***************************************************************************\
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mpgadecoder.h
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: MPEG Decoder class - HEADER
 *
 * MODIFICATIONS by em@be.com:
 *   support multiple layers via IMpgaDecode
 *
\***************************************************************************/

/*
 * $Date: 1999/06/24 10:31:17 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mpgadecoder.h,v 1.14 1999/06/24 10:31:17 sir Exp $
 */

#ifndef __MPGADECODER_H__
#define __MPGADECODER_H__

/* ------------------------ includes --------------------------------------*/

#ifdef USE_MP3DECIFC
  #include "mp3decifc.h"
  #define IMPLEMENTS_INTERFACE(ifc) : public ifc
  #pragma message(__FILE__": compiling CMpgaDecoder with abstract base class")
#else
  #include "mp3sscdef.h"
  #include "mp3streaminfo.h"
  #define IMPLEMENTS_INTERFACE(ifc)
  #pragma message(__FILE__": compiling CMpgaDecoder without abstract base class")
#endif

#include "mpegbitstream.h"
//#include "mp3decode.h"
#include "mpga_internal.h"

/*-------------------------- defines --------------------------------------*/

/*-------------------------------------------------------------------------*/

//
// Mp3 Decoder Top Level Object.
//
// This is the main ISO/MPEG decoder object that interfaces with the 
// application code.
//
// It is however recommended to use IMpgaDecoder (see mp3decifc.h) instead.
// Define USE_MP3DECIFC when planning to use IMpgaDecoder.
//

class CMpgaDecoder IMPLEMENTS_INTERFACE(IMpgaDecoder)
{
public:

  CMpgaDecoder(
  	int Quality=0, int Resolution=0, int Downmix=0, bool UseFixedPoint=false);
  CMpgaDecoder(
  	unsigned char *pBuf, int cbSize,
	 	int Quality=0, int Resolution=0, int Downmix=0, bool UseFixedPoint=false);

  ~CMpgaDecoder();

	SSC  SetLayer(int Layer);

  void Reset();
  SSC  DecodeFrame(unsigned char *pPcm, int cbPcm, int *pcbUsed);
  SSC  DecodeFrame(float         *pPcm, int cbPcm, int *pcbUsed);

  const CMp3StreamInfo *GetStreamInfo() const;
  
  void Connect(CGioBase *gf);
  int  Fill(const unsigned char *pBuffer, int cbBuffer);
  int  GetInputFree() const;
  int  GetInputLeft() const;
  void SetInputEof();
  bool IsEof() const;

protected:

  void SetStreamInfo(SSC dwReturn);
  SSC  DecodeFrameIntern(void *pPcm, int cbPcm, int *pcbUsed, int PcmFormat = 0);
  CMp3StreamInfo m_Info;
  CMpegBitStream m_Mbs;

	IMpgaDecode*   m_Decode;
	int            m_Layer;
	
  int            m_Quality;
  int            m_Resolution;
  int            m_Downmix;
  bool           m_UseFixedPoint;
  bool           m_IsEof;

private:

};

/*-------------------------------------------------------------------------*/
#endif
