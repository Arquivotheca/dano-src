#if !defined(__MPGA_INTERNAL_H__)
#define __MPGA_INTERNAL_H__

// MPEG audio decoder internal interface
// [em] 15feb00

#include "mpegbitstream.h"

class IMpgaDecode {
public:
	virtual ~IMpgaDecode() {}
	
	virtual void Init(bool fFullReset = true) =0;

	// PcmFormat: 0: integer, 1: 32 bit float (IEEE)
	virtual SSC Decode(void *pPcm, int cbPcm, int *pcbUsed, int PcmFormat = 0) =0;
	
	virtual bool SupportsLayer(int layer) const =0;
};

#endif
