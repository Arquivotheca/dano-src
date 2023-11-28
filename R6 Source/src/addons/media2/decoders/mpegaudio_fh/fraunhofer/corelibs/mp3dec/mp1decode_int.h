
#if !defined(__MP1DECODE_INT_H__)
#define __MP1DECODE_INT_H__

#include "mpeg.h"
#include "mpegbitstream.h"
#include "mpga_internal.h"
#include "polyphase_int.h"

// MPEG Layer-1 decoder class
// [em 17feb00] based on Mp3Decode
// fixed-point version
class Mp1DecodeInt : public IMpgaDecode
{
public:
	Mp1DecodeInt(
		CMpegBitStream& _bs,
		int quality,
		int resolution,
		int downmix);
	~Mp1DecodeInt();
	
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
	CPolyphaseInt     _mPolyphase;
	
	unsigned int      _mJSBound;
	
	// dequantized/denormalized spectrum data
	// (only 3 chunks -- not SSLIMIT -- are populated)
	INT_SPECTRUM      _mSpectrum;

	// raw sample data
	unsigned int      _mSample[2][3][SBLIMIT];
	
	// bit allocation
	unsigned int      _mBitAlloc[2][SBLIMIT];
	
	// scale index
	unsigned int      _mScaleIndex[2][3][SBLIMIT];
	
	// one frame's worth of outbound PCM
	short             _mPCM[2][SSLIMIT][SBLIMIT];
	
	void _initFrame(
		const CMpegHeader& header);

	void _setInfo(
		const CMpegHeader& header);
	
};

#endif
