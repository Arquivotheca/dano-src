
#if !defined(__MP2DECODE_H__)
#define __MP2DECODE_H__

#include "mpeg.h"
#include "mpegbitstream.h"
#include "mpga_internal.h"
#include "polyphase.h"

// MPEG Layer-2 decoder class
// [em 14feb00] based on Mp3Decode
class Mp2Decode : public IMpgaDecode
{
public:
	Mp2Decode(
		CMpegBitStream& _bs,
		int quality,
		int resolution,
		int downmix);
	~Mp2Decode();
	
  void Init(bool fFullReset = true);

  // PcmFormat: 0: integer, 1: 32 bit float (IEEE)
  SSC Decode(void *pPcm, int cbPcm, int *pcbUsed, int PcmFormat = 0);

	bool SupportsLayer(int layer) const;

private:

  const int         _mQuality;        // 0: full, 1: half, 2: quarter
  const int         _mResolution;     // 0: 16 bit, 1: 8 bit
  const int         _mDownmix;        // 0: no downmix, 1: downmix

	MPEG_INFO         _mInfo;
	CMpegBitStream&   _mBitStream;
	CPolyphase        _mPolyphase;
	
	unsigned int      _mJSBound;
	
	// scale index
	unsigned int      _mScaleIndex[2][3][SBLIMIT];
	
	// scale-factor selection
	unsigned int      _mScaleSel[2][SBLIMIT];

	// bit alloc table index
	unsigned int      _mAllocTable;

	// bit allocation
	unsigned int      _mBitAlloc[2][SBLIMIT];
	
	// raw sample data
	unsigned int      _mSample[2][3][SBLIMIT];
	
	// dequantized/denormalized spectrum data
	// (only 3 chunks -- not SSLIMIT -- are populated)
	float             _mSpectrum[2][SSLIMIT][SBLIMIT];

#ifndef L2_FH_POLYPHASE
	// sub-band synth state
	float             _mSBTBuf[2][1024]; // 2*HAN_SIZE
	int               _mSBTBufOffset[2];
	double            _mFilter[64][SBLIMIT];
#endif

	// one frame's worth of outbound PCM
	short             _mPCM[2][SSLIMIT][SBLIMIT];
	
	void _initFrame(
		const CMpegHeader& header);

	void _setInfo(
		const CMpegHeader& header);
	
};

#endif
