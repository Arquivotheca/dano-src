#ifndef __MP3DECODE_INT_H__
#define __MP3DECODE_INT_H__

/* ------------------------ includes --------------------------------------*/

#include "mpeg.h"
#include "mpegbitstream.h"
#include "huffdec.h"
#include "mdct_int.h"
#include "polyphase_int.h"
#include "mpga_internal.h"

#ifdef ERROR_CONCEALMENT
  #include "conceal.h"
#endif

/*-------------------------------------------------------------------------*/

//
// Fixed-point MPEG Layer-3 decoding class.
//

class CMp3DecodeInt : public IMpgaDecode
{
public:
	CMp3DecodeInt(CMpegBitStream &_Bs, int _Quality, int _Resolution, int _Downmix);	
	~CMp3DecodeInt();
	
	void Init(bool fFullReset = true);
	
	// PcmFormat: 0: integer, 1: 32 bit float (IEEE)
	SSC Decode(void *pPcm, int cbPcm, int *pcbUsed, int PcmFormat = 0);
	
	bool SupportsLayer(int layer) const;

protected:
 
  SSC  DecodeOnNoMainData(void *pPcm, int PcmFormat);
  SSC  DecodeNormal      (void *pPcm, int PcmFormat, bool fCrcOk);

  void PolyphaseReorder();
  void ZeroSpectrum();
  void SetInfo();

  CMp3Huffman       m_Mp3Huffman;  // huffman decoder
  CMdctInt          m_Mdct;        // mdct
  CPolyphaseInt     m_Polyphase;   // polyphase

  MPEG_INFO         m_Info;        // info structure
  CMpegBitStream   &m_Bs;          // bitstream
  CBitStream        m_Db;          // dynamic buffer
  MP3SI             m_Si;          // side info
  MP3SCF            m_ScaleFac[2]; // scalefactors

  INT_SPECTRUM      m_Spectrum;    // spectrum buffer
  INT_SPECTRUM      m_PolySpectrum;

  int               m_Quality;        // 0: full, 1: half, 2: quarter
  int               m_Resolution;     // 0: 16 bit, 1: 8 bit
  int               m_Downmix;        // 0: no downmix, 1: downmix

  enum { dynBufSize = 2048 } ;
  
  unsigned char     m_dynBufMemory [dynBufSize] ;

private:
};

/*-------------------------------------------------------------------------*/
#endif
