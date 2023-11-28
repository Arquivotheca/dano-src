#include <stdio.h>
#include <OS.h>

#include "dvcodec.h"
extern "C" {
#include "getimage.h"
#include "putimage.h"
}

DVCodec::DVCodec()
{
	bool has_mmx;
	const char *path;

	has_mmx = false;
#if __INTEL__
    cpuid_info ci;
    get_cpuid(&ci, 1, 0);
    if ((ci.eax_1.features & (1<<23))) 		// check for mmx instructions
		has_mmx = true;
#endif

	/* XXX: remove hard-coded path */
	path = has_mmx ? "/boot/beos/system/lib/csedv.so" :
			"/boot/beos/system/lib/csedv-c.so";
	fImid = load_add_on(path);
	if (fImid < 0) {
		printf("Error loading image %s\n", path);
		fInitStatus = fImid;
		return;
	}


	#define LOOKUP(x,y) \
			fInitStatus = get_image_symbol(fImid, y, B_SYMBOL_TYPE_TEXT, (void **)&x); \
			if (fInitStatus < 0) { \
				printf("Error finding symbol %s\n", y); \
				return; \
			}

	LOOKUP(rSetupCodec, "SetupCodec");
	LOOKUP(rSoftEngineDecodeDV, "SoftEngineDecodeDV");
	LOOKUP(rSoftEngineEncodeDV, "SoftEngineEncodeDV");

#if __INTEL__
	#define SET(x,y) \
		x = (has_mmx) ? mmx##y : y
#else
	#define SET(x,y) \
		x = y
#endif

	SET(rPutImage525_RGBQ, PutImage525_RGBQ);
	SET(rPutImage525_RGB16, PutImage525_RGB16);
	SET(rPutImage525_RGB15, PutImage525_RGB15);
	SET(rPutImage525_YUY2, PutImage525_YUY2);
	SET(rPutImage625_RGBQ, PutImage625_RGBQ);
	SET(rPutImage625_RGB16, PutImage625_RGB16);
	SET(rPutImage625_RGB15, PutImage625_RGB15);
	SET(rPutImage625_YUY2, PutImage625_YUY2);

	SET(rGetImage525_RGBQ, GetImage525_RGBQ);
	SET(rGetImage525_RGB16, GetImage525_RGB16);
	SET(rGetImage525_RGB15, GetImage525_RGB15);
	rGetImage525_YUY2 = GetImage525_YUY2;
	SET(rGetImage625_RGBQ, GetImage625_RGBQ);
	SET(rGetImage625_RGB16, GetImage625_RGB16);
	SET(rGetImage625_RGB15, GetImage625_RGB15);
	rGetImage625_YUY2 = GetImage625_YUY2;

	(rSetupCodec)();

	fInitStatus = B_OK;
}

DVCodec::~DVCodec()
{
	if (fImid >= 0)
		unload_add_on(fImid);
}

status_t DVCodec::DecodeDVNTSC(void *DVin, void *out, color_space cs)
{
	if (InitCheck() < B_OK)
		return InitCheck();

	switch(cs)
	{
		case B_RGB32:
			rSoftEngineDecodeDV( DV_525_60_SYSTEM, (PBYTE)DVin, (PBYTE)out, 720 * 4, 4, (PVOID)rPutImage525_RGBQ );
			break;
		case B_RGB16:
			rSoftEngineDecodeDV( DV_525_60_SYSTEM, (PBYTE)DVin, (PBYTE)out, 720 * 2, 2, (PVOID)rPutImage525_RGB16 );
			break;
		case B_RGB15:
			rSoftEngineDecodeDV( DV_525_60_SYSTEM, (PBYTE)DVin, (PBYTE)out, 720 * 2, 2, (PVOID)rPutImage525_RGB15 );
			break;
		case B_YCbCr422:
			rSoftEngineDecodeDV( DV_525_60_SYSTEM, (PBYTE)DVin, (PBYTE)out, 720 * 2, 2, (PVOID)rPutImage525_YUY2 );
			break;
		default:
			printf("Color space %d not yet supported.\n",cs);
			return -1;
	}
	return 0;
}

status_t DVCodec::DecodeDVPAL(void *DVin, void *out, color_space cs)
{
	if (InitCheck() < B_OK)
		return InitCheck();

	switch(cs)
	{
		case B_RGB32:
			rSoftEngineDecodeDV( DV_625_50_SYSTEM, (PBYTE)DVin, (PBYTE)out, 720 * 4, 4, (PVOID)rPutImage625_RGBQ );
			break;
		case B_RGB16:
			rSoftEngineDecodeDV( DV_625_50_SYSTEM, (PBYTE)DVin, (PBYTE)out, 720 * 2, 2, (PVOID)rPutImage625_RGB16 );
			break;
		case B_RGB15:
			rSoftEngineDecodeDV( DV_625_50_SYSTEM, (PBYTE)DVin, (PBYTE)out, 720 * 2, 2, (PVOID)rPutImage625_RGB15 );
			break;
		case B_YCbCr422:
			rSoftEngineDecodeDV( DV_625_50_SYSTEM, (PBYTE)DVin, (PBYTE)out, 720 * 2, 2, (PVOID)rPutImage625_YUY2 );
			break;
		default:
			printf("Color space %d not yet supported.\n",cs);
			return -1;
	}
	return 0;
}

status_t DVCodec::EncodeDVNTSC(void *in, void *DVout, color_space cs)
{
	if (InitCheck() < B_OK)
		return InitCheck();

	switch(cs)
	{
		case B_RGB32:
			rSoftEngineEncodeDV( DV_525_60_SYSTEM, (PBYTE)DVout, (PBYTE)in, 720 * 4, 4, (PVOID)rGetImage525_RGBQ );
			break;
		case B_RGB16:
			rSoftEngineEncodeDV( DV_525_60_SYSTEM, (PBYTE)DVout, (PBYTE)in, 720 * 2, 2, (PVOID)rGetImage525_RGB16 );
			break;
		case B_RGB15:
			rSoftEngineEncodeDV( DV_525_60_SYSTEM, (PBYTE)DVout, (PBYTE)in, 720 * 2, 2, (PVOID)rGetImage525_RGB15 );
			break;
		case B_YCbCr422:
			rSoftEngineEncodeDV( DV_525_60_SYSTEM, (PBYTE)DVout, (PBYTE)in, 720 * 2, 2, (PVOID)rGetImage525_YUY2 );
			break;
		default:
			printf("EncodeDVNTSC: Color space %d not yet supported.\n",cs);
			return -1;
	}
	return 0;
}

status_t DVCodec::EncodeDVPAL(void *in, void *DVout, color_space cs)
{
	if (InitCheck() < B_OK)
		return InitCheck();

	switch(cs)
	{
		case B_RGB32:
			rSoftEngineEncodeDV( DV_625_50_SYSTEM, (PBYTE)DVout, (PBYTE)in, 720 * 4, 4, (PVOID)rGetImage625_RGBQ );
			break;
		case B_RGB16:
			rSoftEngineEncodeDV( DV_625_50_SYSTEM, (PBYTE)DVout, (PBYTE)in, 720 * 2, 2, (PVOID)rGetImage625_RGB16 );
			break;
		case B_RGB15:
			rSoftEngineEncodeDV( DV_625_50_SYSTEM, (PBYTE)DVout, (PBYTE)in, 720 * 2, 2, (PVOID)rGetImage625_RGB15 );
			break;
		case B_YCbCr422:
			rSoftEngineEncodeDV( DV_625_50_SYSTEM, (PBYTE)DVout, (PBYTE)in, 720 * 2, 2, (PVOID)rGetImage625_YUY2 );
			break;
		default:
			printf("EncodeDVPAL: Color space %d not yet supported.\n",cs);
			return -1;
	}
	return 0;
}
