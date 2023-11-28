// mp1decode_int.cpp

#include "mp1decode_int.h"
#include "mp2.h"
#include "l2table.h"

#include <cstdio>

Mp1DecodeInt::Mp1DecodeInt(CMpegBitStream &bs, int quality, int resolution, int downmix) :
	_mQuality(quality),
	_mResolution(resolution),
	_mDownmix(downmix),
	_mBitStream(bs),
	_mPolyphase(_mInfo, quality, resolution, downmix)
{
	Init(true);
}


Mp1DecodeInt::~Mp1DecodeInt()
{
}

void 
Mp1DecodeInt::Init(bool fFullReset)
{
//	fprintf(stderr, "Mp1DecodeInt::Init(%s)\n", fFullReset ? "true" : "false");
	if(fFullReset) {

		_mPolyphase.Init();
	}
}

SSC 
Mp1DecodeInt::Decode(void *pcmBuffer, int bufferSize, int *outUsed, int pcmFormat)
{
  const CMpegHeader& header = *_mBitStream.GetHdr();

	int  nChannels = _mDownmix ? 1 : header.GetChannels();
  SSC  dwResult  = SSC_OK;
  int  nOutBytes;
  //bool fCrcOk;
  bool fMainData;

  //
  // return if wrong layer
  //
  if (header.GetLayer() != 1 ) {
    // error wrong layer
    fprintf(stderr, "### Mp1DecodeInt: wrong layer\n");
    return SSC_E_MPGA_WRONGLAYER;
	}

	//
	// no float support
	//
	if(pcmFormat) {
    fprintf(stderr, "### Mp1DecodeInt: no float support\n");
		return SSC_E_MPGA_GENERIC;
	}

	nOutBytes = (header.GetSamplesPerFrame() << nChannels) >> (_mQuality+_mResolution);

  //
  // check if PCM buffer is large enough
  //
  if (bufferSize < nOutBytes )
	{
    // error buffer too small
    return SSC_E_MPGA_BUFFERTOOSMALL;
	}

  //
  // skip mpeg header & CRC
  //
	_mBitStream.ResetBitCnt();
	_mBitStream.Ff(header.GetHeaderLen());
  
	_initFrame(header);
	
	// DECODE FRAME

	short* shortPtr = pcmFormat ? 0 : (short*)pcmBuffer;
	
	// *** begin MMX bitread block
	_mBitStream.BeginRead();
	
	mp1ReadBitAlloc(
		_mBitStream,
		_mBitAlloc, // WRITE
		header,
		_mJSBound);	

	mp1ReadScale(
		_mBitStream,
		_mScaleIndex, // WRITE
		_mBitAlloc,
		header,
		_mJSBound);		

	// *** begin MMX bitread block
	_mBitStream.EndRead();

	for(int blockIndex = 0; blockIndex < MP2_SCALE_BLOCK; blockIndex++) {

		// *** begin MMX bitread block
		_mBitStream.BeginRead();

		mp1BufferSample(
			_mBitStream,
			_mSample, // WRITE
			_mBitAlloc,
			header,
			_mJSBound);	

		// *** end MMX bitread block
		_mBitStream.EndRead();

		mp1Dequantize(
			_mSpectrum, //WRITE
			_mSample,
			_mBitAlloc,
			_mScaleIndex,
			header,
			_mJSBound);	

		_mPolyphase.Apply(_mSpectrum, shortPtr, 1);
		shortPtr += (nChannels * SBLIMIT);
	}	

  //
  // seek to end of frame
  //
  int frameLen = header.GetFrameLen();
  int bitCount = _mBitStream.GetBitCnt();
  _mBitStream.Seek(header.GetFrameLen() - _mBitStream.GetBitCnt());

  //
  // set number of bytes used in PCM buffer
  //
  if ( outUsed && SSC_SUCCESS(dwResult) )
    *outUsed = nOutBytes;

  return dwResult;
}

bool 
Mp1DecodeInt::SupportsLayer(int layer) const
{
	return (layer == 1);
}

void 
Mp1DecodeInt::_initFrame(const CMpegHeader &header)
{
	_setInfo(header);
	
	// figure jsbound
	if(header.GetMode() == MPG_MD_JOINT_STEREO)
		_mJSBound = mpgaCalcJSBound(header);
	else
		_mJSBound = SBLIMIT;
}

void 
Mp1DecodeInt::_setInfo(const CMpegHeader &header)
{
  static const int fhgVTab[] = {1, 0, 2};

  _mInfo.stereo             = header.GetChannels();
  _mInfo.sample_rate_ndx    = header.GetSampleRateNdx();
  _mInfo.frame_bits         = header.GetFrameLen();
  _mInfo.mode               = header.GetMode();
  _mInfo.mode_ext           = header.GetModeExt();
  _mInfo.header_size        = header.GetHeaderLen();
  _mInfo.IsMpeg1            = header.GetMpegVersion()==0 ? true:false;
  _mInfo.fhgVersion         = fhgVTab[header.GetMpegVersion()];
  _mInfo.protection         = header.GetCrcCheck();
}



// END -- mp1decode.cpp --
