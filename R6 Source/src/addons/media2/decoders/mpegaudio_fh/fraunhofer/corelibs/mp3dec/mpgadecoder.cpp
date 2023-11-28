/***************************************************************************\ 
 *
 *               (C) copyright Fraunhofer - IIS (1998)
 *                        All Rights Reserved
 *
 *   filename: mpgadecoder.cpp
 *   project : MPEG Decoder
 *   author  : Martin Sieler
 *   date    : 1998-05-26
 *   contents/description: MPEG Decoder class
 *
 *
\***************************************************************************/

/*
 * $Date: 1999/06/24 10:31:17 $
 * $Header: /home/cvs/mms/corelibs/mp3dec/mpgadecoder.cpp,v 1.11 1999/06/24 10:31:17 sir Exp $
 */

/* ------------------------ includes --------------------------------------*/

#include "mpgadecoder.h"
#include "mp1decode.h"
#include "mp1decode_int.h"
#include "mp2decode.h"
#include "mp2decode_int.h"
#include "mp3decode.h"
#include "mp3decode_int.h"

#include <cstdio>

/*-------------------------- defines --------------------------------------*/

//-------------------------------------------------------------------------*
//
//                   C M p g a D e c o d e r
//
//-------------------------------------------------------------------------*

//-------------------------------------------------------------------------*
//   constructor
//-------------------------------------------------------------------------*

CMpgaDecoder::CMpgaDecoder(
	int Quality, int Resolution, int Downmix, bool UseFixedPoint) :
  m_Mbs(8192),
//  m_Mp3Decode(m_Mbs, Quality, Resolution, Downmix),
	m_Decode(0),
	m_Layer(0),
  m_Quality(Quality),
  m_Resolution(Resolution),
  m_Downmix(Downmix),
  m_UseFixedPoint(UseFixedPoint)
{
  // reset myself
  Reset();
}

//-------------------------------------------------------------------------*
//   constructor with external bitbuffer memory
//-------------------------------------------------------------------------*

CMpgaDecoder::CMpgaDecoder(
	unsigned char *pBuf, int cbSize, int Quality, int Resolution, int Downmix, bool UseFixedPoint) :
  m_Mbs(pBuf, cbSize),
//  m_Mp3Decode(m_Mbs, Quality, Resolution, Downmix),
	m_Decode(0),
	m_Layer(0),
  m_Quality(Quality),
  m_Resolution(Resolution),
  m_Downmix(Downmix),
  m_UseFixedPoint(UseFixedPoint)
{
  // reset myself
  Reset();
}

//-------------------------------------------------------------------------*
//   destructor
//-------------------------------------------------------------------------*

CMpgaDecoder::~CMpgaDecoder()
{
	if(m_Decode)
		delete m_Decode;
}

//-------------------------------------------------------------------------*
//   SetLayer
//-------------------------------------------------------------------------*

SSC 
CMpgaDecoder::SetLayer(int Layer)
{
	if(m_Decode) {
		if(m_Decode->SupportsLayer(Layer))
			return SSC_OK;
		delete m_Decode;
		m_Decode = 0;
	}
	
	if(m_UseFixedPoint)
	{
#ifdef ENABLE_FIXED_POINT
		switch(Layer)
		{
		case 3:
			m_Decode = new CMp3DecodeInt(
				m_Mbs, m_Quality, m_Resolution, m_Downmix);
			break;
		case 2:
			m_Decode = new Mp2DecodeInt(
				m_Mbs, m_Quality, m_Resolution, m_Downmix);
			break;
		case 1:
			m_Decode = new Mp1DecodeInt(
				m_Mbs, m_Quality, m_Resolution, m_Downmix);
			break;
		}
#endif
	}
	else
	{
#ifdef ENABLE_FLOATING_POINT
		switch(Layer)
		{
		case 3:
			m_Decode = new CMp3Decode(
				m_Mbs, m_Quality, m_Resolution, m_Downmix);
			break;
			
		case 2:
			m_Decode = new Mp2Decode(
				m_Mbs, m_Quality, m_Resolution, m_Downmix);
			break;
			
		case 1:
			m_Decode = new Mp1Decode(
				m_Mbs, m_Quality, m_Resolution, m_Downmix);
			break;		
		}
#endif
	}
	

	if(!m_Decode)
	{
#if DEBUG
		fprintf(stderr, "### CMpgaDecoder::SetLayer(): no %s decoder for layer %d\n",
			m_UseFixedPoint ? "fixed-point" : "floating-point",
			Layer);
#endif
		return SSC_E_MPGA_WRONGLAYER;
	}
	return SSC_OK;
}


//-------------------------------------------------------------------------*
//   Reset
//-------------------------------------------------------------------------*

void CMpgaDecoder::Reset()
{
  // no, we are not eof
  m_IsEof = false;

  // reset MPEG bitstream object
  m_Mbs.Reset();

	if(m_Decode)
	  // reset decoder object
	  m_Decode->Init(true);
}

//-------------------------------------------------------------------------*
//   DecodeFrame
//-------------------------------------------------------------------------*

SSC CMpgaDecoder::DecodeFrame(unsigned char *pPcm, int cbPcm, int *pcbUsed)
{
  // integer
  return DecodeFrameIntern(pPcm, cbPcm, pcbUsed, 0);
}


SSC CMpgaDecoder::DecodeFrame(float *pPcm, int cbPcm, int *pcbUsed)
{
  // 32 bit float (IEEE)
  return DecodeFrameIntern(pPcm, cbPcm, pcbUsed, 1);
}

//-------------------------------------------------------------------------*
//   DecodeFrameIntern
//-------------------------------------------------------------------------*

SSC CMpgaDecoder::DecodeFrameIntern
    (
    void *pPcm,
    int   cbPcm,
    int  *pcbUsed,
    int   PcmFormat
    )
{
  SSC dwReturn;

  //
  // reset pcbUsed
  //
  if ( pcbUsed )
    *pcbUsed = 0;

  //
  // sync to next frame
  //
  dwReturn = m_Mbs.DoSync();

  //
  // check for success
  //
  if ( SSC_SUCCESS(dwReturn) )
    {
    //
    // success, do the work
    //

		int Layer = m_Mbs.GetHdr()->GetLayer();
		if(!m_Decode || !m_Decode->SupportsLayer(Layer)) {
			// find a decoder
			dwReturn = SetLayer(Layer);
			if(dwReturn != SSC_OK)
				return dwReturn;
		}


    // decode one frame
    // (decoder has to eat up all data of this frame!!)
    if ( pPcm)
      dwReturn = m_Decode->Decode(pPcm, cbPcm, pcbUsed, PcmFormat);

    // set streaminfo object
    SetStreamInfo(dwReturn);
    }
  else if ( dwReturn == SSC_W_MPGA_SYNCEOF )
    {
    //
    // end of file (track) reached
    //

    m_IsEof = true;
    }
  else if ( dwReturn == SSC_W_MPGA_SYNCLOST )
    {
    //
    // sync lost: reset decoder
    //

		if(m_Decode)
	    m_Decode->Init(true);
    }
  else
    {
    //
    // handle all other sync states
    //
    }

  return dwReturn;
}

//-------------------------------------------------------------------------*
//   GetStreamInfo
//-------------------------------------------------------------------------*

const CMp3StreamInfo *CMpgaDecoder::GetStreamInfo() const
{
  //
  // return pointer to MP3 streaminfo object
  //
  return &m_Info;
}

//-------------------------------------------------------------------------*
//   Connect
//-------------------------------------------------------------------------*

void CMpgaDecoder::Connect(CGioBase *gf)
{
  //
  // connect "gf" with bistream object
  // data will be read automatically by MPEG bitstream object
  // do not call "Fill", if connected!!
  //
  m_Mbs.Connect(gf);
}

//-------------------------------------------------------------------------*
//   Fill
//-------------------------------------------------------------------------*

int CMpgaDecoder::Fill(const unsigned char *pBuffer, int cbBuffer)
{
  //
  // provide data to bitstream object
  //
  return m_Mbs.Fill(pBuffer, cbBuffer);
}

//-------------------------------------------------------------------------*
//   GetInputFree
//-------------------------------------------------------------------------*

int CMpgaDecoder::GetInputFree() const
{
  //
  // get number of bytes bitstream object will accept
  //
  return m_Mbs.GetFree();
}

//-------------------------------------------------------------------------*
//   GetInputLeft
//-------------------------------------------------------------------------*

int CMpgaDecoder::GetInputLeft() const
{
  //
  // get number of bytes left in bitstream object
  //
  return (m_Mbs.GetValidBits() / 8);
}

//-------------------------------------------------------------------------*
//   SetInputEof
//-------------------------------------------------------------------------*

void CMpgaDecoder::SetInputEof()
{
  //
  // indicate end-of-input-data to MPEG bitstream object
  // note: the bitstream object may still have some data,
  //       end-of-output-data will be indicated by this object
  //
  m_Mbs.SetEof();
}

//-------------------------------------------------------------------------*
//   IsEof
//-------------------------------------------------------------------------*

bool CMpgaDecoder::IsEof() const
{
  //
  // no more PCM data will be produced after EOF
  //
  return m_IsEof;
}

//-------------------------------------------------------------------------*
//   SetStreamInfo
//-------------------------------------------------------------------------*

void CMpgaDecoder::SetStreamInfo(SSC dwReturn)
{
  //
  // set streaminfo object
  //

  const CMpegHeader *hdr = m_Mbs.GetHdr();

  m_Info.SetLayer            (hdr->GetLayer());
  m_Info.SetMpegVersion      (hdr->GetMpegVersion());
  m_Info.SetBitrate          (hdr->GetBitrate());
  m_Info.SetBitrateIndex     (hdr->GetBitrateNdx());
  m_Info.SetChannels         (hdr->GetChannels());
  m_Info.SetSFreq            (hdr->GetSampleRate());
  m_Info.SetEffectiveChannels(m_Downmix ? 1 : hdr->GetChannels());
  m_Info.SetEffectiveSFreq   (hdr->GetSampleRate() >> m_Quality);
  m_Info.SetBitsPerSample    (m_Resolution==0 ? 16:8);
  m_Info.SetBitsPerFrame     (hdr->GetFrameLen());
  m_Info.SetDuration         (hdr->GetDuration());

  m_Info.SetCrcError         (0);
  m_Info.SetNoMainData       (0);

  switch ( dwReturn )
    {
    case SSC_I_MPGA_CRCERROR:
      m_Info.SetCrcError(1);
      break;
    case SSC_I_MPGA_NOMAINDATA:
      m_Info.SetNoMainData(1);
      break;
    default:
      break;
    }
}

/*-------------------------------------------------------------------------*/
