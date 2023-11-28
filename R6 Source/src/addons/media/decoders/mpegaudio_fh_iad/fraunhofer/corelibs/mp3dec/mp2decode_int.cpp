// mp2decode_int.cpp

#include "mp2decode_int.h"
#include "mp2.h"
#include "l2table.h"

#include <cstdio>
#include <Debug.h>

Mp2DecodeInt::Mp2DecodeInt(CMpegBitStream &bs, int quality, int resolution, int downmix) :
	_mQuality(quality),
	_mResolution(resolution),
	_mDownmix(downmix),
	_mBitStream(bs),
	_mPolyphase(_mInfo, quality, resolution, downmix)
{
	Init(true);
}


Mp2DecodeInt::~Mp2DecodeInt()
{
}

void 
Mp2DecodeInt::Init(bool fFullReset)
{
//	fprintf(stderr, "Mp2DecodeInt::Init(%s)\n", fFullReset ? "true" : "false");
	if(fFullReset) {
		_mPolyphase.Init();
	}
}

SSC 
Mp2DecodeInt::Decode(void *pcmBuffer, int bufferSize, int *outUsed, int pcmFormat)
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
  if (header.GetLayer() != 2 ) {
    // error wrong layer
    fprintf(stderr, "### Mp2DecodeInt: wrong layer\n");
    return SSC_E_MPGA_WRONGLAYER;
	}

	//
	// no float support
	//
	if(pcmFormat) {
    fprintf(stderr, "### Mp2DecodeInt: no float support\n");
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
	
	mp2ReadBitAlloc(
		_mBitStream,
		_mBitAlloc, // WRITE
		header,
		_mJSBound,
		sBitAllocTables[_mAllocTable]);	

	mp2ReadScale(
		_mBitStream,
		_mScaleIndex, // WRITE
		_mScaleSel, // WRITE
		_mBitAlloc,
		header,
		_mJSBound,
		sBitAllocTables[_mAllocTable]);		

	// *** end MMX bitread block
	_mBitStream.EndRead();

	for(int blockIndex = 0; blockIndex < MP2_SCALE_BLOCK; blockIndex++) {

		// *** begin MMX bitread block
		_mBitStream.BeginRead();

		mp2BufferSample(
			_mBitStream,
			_mSample, // WRITE
			_mBitAlloc,
			header,
			_mJSBound,
			sBitAllocTables[_mAllocTable]);	

		// *** end MMX bitread block
		_mBitStream.EndRead();

		mp2Dequantize(
			_mSpectrum, //WRITE
			_mSample,
			_mBitAlloc,
			_mScaleIndex,
			blockIndex >> 2,
			header,
			_mJSBound,
			sBitAllocTables[_mAllocTable]);
		
		// apply polyphase
		ASSERT(!pcmFormat);
		_mPolyphase.Apply(_mSpectrum, shortPtr, 3);
		shortPtr += (nChannels * 3 * SBLIMIT);
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
Mp2DecodeInt::SupportsLayer(int layer) const
{
	return (layer == 2);
}

void 
Mp2DecodeInt::_initFrame(const CMpegHeader &header)
{
	_setInfo(header);
	
	// pick bit-alloc table
	_mAllocTable = mpgaPickBitAllocTable(header);
	
	// figure jsbound
	if(header.GetMode() == MPG_MD_JOINT_STEREO)
		_mJSBound = mpgaCalcJSBound(header);
	else
		_mJSBound = sBitAllocTables[_mAllocTable].sblimit;
}

void 
Mp2DecodeInt::_setInfo(const CMpegHeader &header)
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



// END -- mp2decode.cpp --
