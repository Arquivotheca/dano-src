/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mp3decode.c
 *   project : ISO/MPEG-Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: MPEG Layer-3 decoder
 *
 *
 \***************************************************************************/

/*
 * $Date: 1999/06/24 10:31:14 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mp3decode.cpp,v 1.14 1999/06/24 10:31:14 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mp3decode.h"

#include "mp3read.h"
#include "mp3quant.h"
#include "mp3tools.h"

#include <cstdio>

/* ------------------------------------------------------------------------*/

#ifdef ERROR_CONCEALMENT
  #pragma message(__FILE__": Error-Concealment enabled")
#else
  #pragma message(__FILE__": Error-Concealment disabled")
#endif

//-------------------------------------------------------------------------*
//
//                   C M p 3 D e c o d e
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CMp3Decode::CMp3Decode
    (
    CMpegBitStream &_Bs, 
    int             _Quality, 
    int             _Resolution, 
    int             _Downmix
    ) :

  m_Mdct(m_Info, _Quality),
  m_Polyphase(m_Info, _Quality, _Resolution, _Downmix),

  m_Bs(_Bs),
  m_Db(m_dynBufMemory, dynBufSize),

  m_Quality(_Quality),
  m_Resolution(_Resolution),
  m_Downmix(_Downmix)
{
  // full reset
  Init(true);
}

//-------------------------------------------------------------------------*
//   destructor
//-------------------------------------------------------------------------*

CMp3Decode::~CMp3Decode()
{
}

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

void CMp3Decode::Init(bool fFullReset)
{
  // flush huffman buffer
  m_Db.Reset();

  if ( fFullReset )
    {
    // reset mdct
    m_Mdct.Init();

    // reset polyphase
    m_Polyphase.Init();

#ifdef ERROR_CONCEALMENT
    // reset error concealment
    m_Conceal.Init();
#endif

    // reset all spectrum members
    ZeroISpectrum();
    ZeroSpectrum();
    ZeroPolySpectrum();
    }
}

//-------------------------------------------------------------------------*
//   Decode
//-------------------------------------------------------------------------*

SSC CMp3Decode::Decode
    (
    void *pPcm, 
    int   cbPcm, 
    int  *pcbUsed,
    int   PcmFormat
    )
{
  int  nChannels = m_Downmix ? 1 : m_Bs.GetHdr()->GetChannels();
  SSC  dwResult  = SSC_OK;
  int  nOutBytes;
  bool fCrcOk;
  bool fMainData;

  //
  // return if wrong layer
  //
  if ( m_Bs.GetHdr()->GetLayer() != 3 )
    {
    // error wrong layer
    return SSC_E_MPGA_WRONGLAYER;
    }

  //
  // calculate number of ouput bytes
  //
  if ( PcmFormat == 0 /* integer */ )
    {
    nOutBytes = (m_Bs.GetHdr()->GetSamplesPerFrame() << nChannels) >> (m_Quality+m_Resolution);
    }
  else /* 32 bit float (IEEE) */
    {
    nOutBytes = (m_Bs.GetHdr()->GetSamplesPerFrame() * sizeof(float) * nChannels) >> m_Quality;
    }

  //
  // check if PCM buffer is large enough
  //
  if ( cbPcm < nOutBytes )
    {
    // error buffer too small
    return SSC_E_MPGA_BUFFERTOOSMALL;
    }

  //
  // skip mpeg header
  //
  m_Bs.Ff(m_Bs.GetHdr()->GetHeaderLen());

  //
  // set info structure
  //
  SetInfo();


  // *** begin MMX bitread block
  m_Bs.BeginRead();

  //
  // read side info (will check for crc error)
  //
  fCrcOk = mp3SideInfoRead(m_Bs, m_Si, m_Info);

  //
  // read main data (will check if enough available)
  //
  fMainData = mp3MainDataRead(m_Bs, m_Db, m_Si, m_Info);

  // *** end MMX bitread block
  m_Bs.EndRead();

  //
  // decode this frame
  //
  if ( fMainData )
    {
    dwResult = DecodeNormal(pPcm, PcmFormat, fCrcOk);
    }
  else
    {
    dwResult = DecodeOnNoMainData(pPcm, PcmFormat);
    }

  //
  // seek to end of frame
  //
  m_Bs.Seek(m_Bs.GetHdr()->GetFrameLen() - m_Bs.GetBitCnt());

  //
  // set number of bytes used in PCM buffer
  //
  if ( pcbUsed && SSC_SUCCESS(dwResult) )
    *pcbUsed = nOutBytes;

  //
  // do a "soft" decoder reset in case of CRC error
  //
  if ( !fCrcOk )
    {
    Init(false);

    // patch dwResult (only if decoding was successfull)
    if ( SSC_SUCCESS(dwResult) )
      dwResult = SSC_I_MPGA_CRCERROR;
    }

  return dwResult;
}

//-------------------------------------------------------------------------*
//   DecodeNormal
//-------------------------------------------------------------------------*

SSC CMp3Decode::DecodeNormal(void *pPcm, int PcmFormat, bool fCrcOk)
{
  //
  // decode all channels, granules
  //

  int nChannels = m_Downmix ? 1 : m_Bs.GetHdr()->GetChannels();
  int gr, ch;

  for ( gr=0; gr<(m_Info.IsMpeg1 ? 2:1); gr++ )
    {
    for ( ch=0; ch<m_Info.stereo; ch++ )
      {
		// *** begin MMX bitread block
		// +++++ the more bit unpacking done in one block,
		//       the better!
		m_Db.BeginRead();

      // read scalefactors
      mp3ScaleFactorRead(m_Db,
        m_Si.ch[ch].gr[gr],
        m_ScaleFac[ch],
        m_Info,
        m_Si.ch[ch].scfsi,
        gr,
        ch);
      
      // read huffman data
      m_Mp3Huffman.Read(m_Db, m_ISpectrum[ch], m_Si.ch[ch].gr[gr], m_Info, 1);
      
		// *** end MMX bitread block
		m_Db.EndRead();

      // dequantize spectrum
      mp3DequantizeSpectrum(m_ISpectrum[ch], &m_Spectrum[ch][0][0],
        m_Si.ch[ch].gr[gr], 
        m_ScaleFac[ch], 
        m_Info);
	  }

   // stereo processing

    mp3StereoProcessing(&m_Spectrum[0][0][0], &m_Spectrum[1][0][0], 
      m_Si.ch[0].gr[gr], m_Si.ch[1].gr[gr],
      m_ScaleFac[1],
      m_Info,
      m_Downmix);

   for ( ch=0; ch<nChannels; ch++ )
      {

      // sample reordering
      mp3Reorder(&m_Spectrum[ch][0][0], m_Si.ch[ch].gr[gr], m_Info);

      // antialiasing
      mp3Antialias(&m_Spectrum[ch][0][0], m_Si.ch[ch].gr[gr], m_Info, m_Quality);
	        
#ifdef ERROR_CONCEALMENT
      //
      // error concealment (apply in case of crc error)
      //
      m_Conceal.Apply(!fCrcOk, m_Info, m_Si, &m_Spectrum[ch][0][0], gr, ch);
#else
      // stop compiler from complaining about unused arg
      fCrcOk = fCrcOk;
#endif

      // mdct
      m_Mdct.Apply(ch, m_Si.ch[ch].gr[gr], m_Spectrum);
      
     }

    // reordering (neccessary for polyphase)

    PolyphaseReorder();

	short* pPrev = (short*)pPcm;
	
     // polyphase

    if ( PcmFormat == 0 /* integer */ )
      pPcm = m_Polyphase.Apply(m_PolySpectrum, (short*)pPcm);
    else /* 32 bit float (IEEE) */
      pPcm = m_Polyphase.Apply(m_PolySpectrum, (float*)pPcm);

    } // for ( gr=0...)

  return SSC_OK;
}

bool 
CMp3Decode::SupportsLayer(int layer) const
{
	return (layer == 3);
}


//-------------------------------------------------------------------------*
//   DecodeOnNoMainData
//-------------------------------------------------------------------------*

SSC CMp3Decode::DecodeOnNoMainData(void *pPcm, int PcmFormat)
{
  //
  // not enough data in dynamic buffer
  //

  int nChannels = m_Downmix ? 1 : m_Bs.GetHdr()->GetChannels();
  int gr, ch;

  for ( gr=0; gr<(m_Info.IsMpeg1 ? 2:1); gr++ )
    {
    // zero spectrum
    ZeroSpectrum();

    for ( ch=0; ch<nChannels; ch++ )
      {
      // set some fields in sideinfo
      m_Si.ch[ch].gr[gr].zeroSbStartNdx        = 0;
      m_Si.ch[ch].gr[gr].window_switching_flag = 0;
      m_Si.ch[ch].gr[gr].mixed_block_flag      = 0;
      m_Si.ch[ch].gr[gr].block_type            = 0;        
        
#ifdef ERROR_CONCEALMENT
      //
      // error concealment, predict spectrum
      //
      m_Conceal.Apply(1, m_Info, m_Si, &m_Spectrum[ch][0][0], gr, ch);
#endif
        
      // mdct
      m_Mdct.Apply(ch, m_Si.ch[ch].gr[gr], m_Spectrum);
      }
    
    // reordering (neccessary for polyphase)
    PolyphaseReorder();
    
    // polyphase
    if ( PcmFormat == 0 /* integer */ )
      pPcm = m_Polyphase.Apply(m_PolySpectrum, (short*)pPcm);
    else /* 32 bit float (IEEE) */
      pPcm = m_Polyphase.Apply(m_PolySpectrum, (float*)pPcm);
    }

  return SSC_I_MPGA_NOMAINDATA;
}

//-------------------------------------------------------------------------*
//   PolyphaseReorder
//-------------------------------------------------------------------------*

void CMp3Decode::PolyphaseReorder()
{
  int nChannels = m_Downmix ? 1 : m_Bs.GetHdr()->GetChannels();
  int ch, sb, ss;

  for ( ch=0; ch<nChannels; ch++ )
    {
    for ( ss=0; ss<SSLIMIT; ss++ )
      for ( sb=0; sb<SBLIMIT; sb++ )
        m_PolySpectrum[ch][ss][sb] = m_Spectrum[ch][sb][ss];
    }
}

//-------------------------------------------------------------------------*
//   ZeroISpectrum
//-------------------------------------------------------------------------*

void CMp3Decode::ZeroISpectrum()
{
  int ch, i;

  // reset spectrum to zero
  for ( ch=0; ch<2; ch++ )
    for ( i=0; i<SSLIMIT*SBLIMIT; i++ )
      m_ISpectrum[ch][i] = 0;
}

//-------------------------------------------------------------------------*
//   ZeroSpectrum
//-------------------------------------------------------------------------*

void CMp3Decode::ZeroSpectrum()
{
  int ch, ss, sb;

  // reset spectrum to zero
  for ( ch=0; ch<2; ch++ )
    for ( sb=0; sb<SBLIMIT; sb++ )
      for ( ss=0; ss<SSLIMIT; ss++ )
        m_Spectrum[ch][sb][ss] = 0.0f;
}

//-------------------------------------------------------------------------*
//   ZeroPolySpectrum
//-------------------------------------------------------------------------*

void CMp3Decode::ZeroPolySpectrum()
{
  int ch, ss, sb;

  // reset spectrum to zero
  for ( ch=0; ch<2; ch++ )
    for ( ss=0; ss<SSLIMIT; ss++ )
      for ( sb=0; sb<SBLIMIT; sb++ )
        m_PolySpectrum[ch][ss][sb] = 0.0f;
}

//-------------------------------------------------------------------------*
//   SetInfo
//-------------------------------------------------------------------------*

void CMp3Decode::SetInfo()
{
  static const int fhgVTab[] = {1, 0, 2};

  const CMpegHeader *hdr = m_Bs.GetHdr();

  m_Info.stereo             = hdr->GetChannels();
  m_Info.sample_rate_ndx    = hdr->GetSampleRateNdx();
  m_Info.frame_bits         = hdr->GetFrameLen();
  m_Info.mode               = hdr->GetMode();
  m_Info.mode_ext           = hdr->GetModeExt();
  m_Info.header_size        = hdr->GetHeaderLen();
  m_Info.IsMpeg1            = hdr->GetMpegVersion()==0 ? true:false;
  m_Info.fhgVersion         = fhgVTab[hdr->GetMpegVersion()];
  m_Info.protection         = hdr->GetCrcCheck();
}

/*-------------------------------------------------------------------------*/

