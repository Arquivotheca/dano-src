/* ------------------------ includes --------------------------------------*/

#include "mp3decode_int.h"

#include "mp3read.h"
#include "mp3quant.h"
#include "mp3tools.h"

#include <cstring>
#include <stdio.h>
#define DEBUG 1
#include <Debug.h>

/* ------------------------------------------------------------------------*/

#ifdef ERROR_CONCEALMENT
  #pragma message(__FILE__": Error-Concealment enabled")
#else
  #pragma message(__FILE__": Error-Concealment disabled")
#endif


/* ------------------------------------------------------------------------*/

// +++++ clipcheck central

//int32	fixed32_mul(int32 a, int32 b)
//{
//	double v = fixed32_to_double(a) * fixed32_to_double(b);
//
//	int64 temp = int64(a) * int64(b);
//	int32 ret = int32(temp >> fixed32_frac_bits);
//	
//	if(fabs(fixed32_to_double(ret) - v) > 1.0)
//	{
//		fprintf(stderr, "CLIP: (%.2f * %.2f) = %.2lf %f\n",
//			fixed32_to_float(a),
//			fixed32_to_float(b),
//			v, fixed32_to_float(ret));
//		DEBUGGER("fixed32_mul CLIP");
//	}
//	return ret;
//}

//-------------------------------------------------------------------------*
//
//                   C M p 3 D e c o d e
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CMp3DecodeInt::CMp3DecodeInt
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

CMp3DecodeInt::~CMp3DecodeInt()
{
}

//-------------------------------------------------------------------------*
//   Init
//-------------------------------------------------------------------------*

void CMp3DecodeInt::Init(bool fFullReset)
{
  // flush huffman buffer
  m_Db.Reset();

  if ( fFullReset )
    {
    // reset mdct
    m_Mdct.Init();

    // reset polyphase
    m_Polyphase.Init();

    // reset spectrum buffer
    ZeroSpectrum();
    }
}

//-------------------------------------------------------------------------*
//   Decode
//-------------------------------------------------------------------------*

SSC CMp3DecodeInt::Decode
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

  // don't bother with float output
  if(PcmFormat != 0)
  {
  	return SSC_E_WRONGPARAMETER;
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

  //
  // read side info (will check for crc error)
  //
  fCrcOk = mp3SideInfoRead(m_Bs, m_Si, m_Info);

  //
  // read main data (will check if enough available)
  //
  fMainData = mp3MainDataRead(m_Bs, m_Db, m_Si, m_Info);

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

SSC CMp3DecodeInt::DecodeNormal(void *pPcm, int PcmFormat, bool fCrcOk)
{
	// stop compiler from complaining about unused arg
	fCrcOk = fCrcOk;
	
	//
	// decode all channels, granules
	//
	
	int nChannels = m_Downmix ? 1 : m_Bs.GetHdr()->GetChannels();
	int gr, ch;
	
	for ( gr=0; gr<(m_Info.IsMpeg1 ? 2:1); gr++ )
	{
		for ( ch=0; ch<m_Info.stereo; ch++ )
		{
			// read scalefactors
			mp3ScaleFactorRead(m_Db,
				m_Si.ch[ch].gr[gr],
				m_ScaleFac[ch],
				m_Info,
				m_Si.ch[ch].scfsi,
				gr,
				ch);
			
			// read huffman data
			m_Mp3Huffman.Read(
				m_Db,
				m_Spectrum+ch,
				m_Si.ch[ch].gr[gr],
				m_Info,
				2); // stereo-interleave     

			// dequantize spectrum
			mp3DequantizeSpectrum(
				m_Spectrum+ch,
				m_Si.ch[ch].gr[gr], 
				m_ScaleFac[ch], 
				m_Info);
		}

		// stereo processing
		mp3StereoProcessing(
			m_Spectrum,
			m_Si.ch[0].gr[gr],
			m_Si.ch[1].gr[gr],
			m_ScaleFac[1],
			m_Info,
			m_Downmix);
		
		for ( ch=0; ch<nChannels; ch++ )
		{
			// sample reordering
			mp3Reorder(
				m_Spectrum+ch,
				m_Si.ch[ch].gr[gr],
				m_Info);

			// antialiasing
			mp3Antialias(
				m_Spectrum+ch,
				m_Si.ch[ch].gr[gr],
				m_Info,
				m_Quality);

			// mdct
			m_Mdct.Apply(
				ch,
				m_Si.ch[ch].gr[gr],
				m_Spectrum);
		}
		
		// reordering (neccessary for polyphase)
		PolyphaseReorder();

		// polyphase
		ASSERT(PcmFormat == 0);
		pPcm = m_Polyphase.Apply(
			m_PolySpectrum,
			(short*)pPcm);
	} // for ( gr=0...)
	
	return SSC_OK;
}

bool 
CMp3DecodeInt::SupportsLayer(int layer) const
{
	return (layer == 3);
}


//-------------------------------------------------------------------------*
//   DecodeOnNoMainData
//-------------------------------------------------------------------------*

SSC CMp3DecodeInt::DecodeOnNoMainData(void *pPcm, int PcmFormat)
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
        
      // mdct
      m_Mdct.Apply(ch, m_Si.ch[ch].gr[gr], m_Spectrum);
      }
    
    // reordering (neccessary for polyphase)
    PolyphaseReorder();
    
    // polyphase
    ASSERT(PcmFormat == 0);
    pPcm = m_Polyphase.Apply(m_PolySpectrum, (short*)pPcm);
    }

  return SSC_I_MPGA_NOMAINDATA;
}

//-------------------------------------------------------------------------*
//   PolyphaseReorder
//-------------------------------------------------------------------------*

void CMp3DecodeInt::PolyphaseReorder()
{
//	int nChannels = m_Downmix ? 1 : m_Bs.GetHdr()->GetChannels();

	int32* src = m_Spectrum;
	int32* dest = m_PolySpectrum;
	for ( int sb=0; sb<SBLIMIT; sb++ )
	{
		int32* scur = src;
		int32* dcur = dest;
		for ( int ss=0; ss<SSLIMIT; ss++ )
		{
			*dcur = *scur++;
			*(dcur+1) = *scur++; // unneccessary for mono...
			dcur += (SBLIMIT*2);
		}
		src += (SSLIMIT*2);
		dest += 2;
	}
}

//-------------------------------------------------------------------------*
//   ZeroSpectrum
//-------------------------------------------------------------------------*

void CMp3DecodeInt::ZeroSpectrum()
{
  int ch, ss, sb;

  // reset spectrum to zero
  memset(m_Spectrum, 0, sizeof(INT_SPECTRUM));
  memset(m_PolySpectrum, 0, sizeof(INT_SPECTRUM));
}

//-------------------------------------------------------------------------*
//   SetInfo
//-------------------------------------------------------------------------*

void CMp3DecodeInt::SetInfo()
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
