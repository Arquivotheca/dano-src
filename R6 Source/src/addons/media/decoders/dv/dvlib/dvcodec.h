#ifndef _DVCODEC_H_
#define _DVCODEC_H_

#define DVASM
extern "C" {
	#include "windows.h"
	#include "csedv.h"
}

#include <interface/GraphicsDefs.h>
#include <kernel/image.h>

struct DVCodec {
	DVCodec();
	~DVCodec();

	status_t InitCheck() { return fInitStatus; }

	status_t DecodeDVNTSC(void *DVin, void *out, color_space cs);
	status_t EncodeDVNTSC(void *in, void *DVout, color_space cs);
	status_t DecodeDVPAL(void *DVin, void *out, color_space cs);
	status_t EncodeDVPAL(void *in, void *DVout, color_space cs);

private:
	status_t fInitStatus;
	image_id fImid;

	void (*rSetupCodec)();

	int (*rSoftEngineDecodeDV)(int, PBYTE, PBYTE, int, int, PVOID);
	int (*rSoftEngineEncodeDV)(int, PBYTE, PBYTE, int, int, PVOID);

	void (*rPutImage525_RGBQ)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	void (*rPutImage525_RGB16)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	void (*rPutImage525_RGB15)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	void (*rPutImage525_YUY2)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	
	void (*rPutImage625_RGBQ)( PBYTE pImage, int Stride, PDCT_DATA pY );
	void (*rPutImage625_RGB16)( PBYTE pImage, int Stride, PDCT_DATA pY );
	void (*rPutImage625_RGB15)( PBYTE pImage, int Stride, PDCT_DATA pY );
	void (*rPutImage625_YUY2)( PBYTE pImage, int Stride, PDCT_DATA pY );
	
	void (*rGetImage525_RGBQ)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	void (*rGetImage525_RGB16)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	void (*rGetImage525_RGB15)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	void (*rGetImage525_YUY2)( PBYTE pImage, int Stride, int SizeX, int SizeY, PDCT_DATA pY, PDCT_DATA pC );
	
	void (*rGetImage625_RGBQ)( PBYTE pImage, int Stride, PDCT_DATA pY );
	void (*rGetImage625_RGB16)( PBYTE pImage, int Stride, PDCT_DATA pY );
	void (*rGetImage625_RGB15)( PBYTE pImage, int Stride, PDCT_DATA pY );
	void (*rGetImage625_YUY2)( PBYTE pImage, int Stride, PDCT_DATA pY );
};

#endif
