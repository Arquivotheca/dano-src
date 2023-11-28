#include "ResampleArray.h"

// It's the cheeze!
void resample_farray( float *dst, float *src, int32 dstElm, int32 srcElm )
{
	float	srcInc = float(srcElm)/float(dstElm);
	float	srcIndex = 0;
	
	for( int32 i=0; i<dstElm; i++, srcIndex += srcInc )
	{
		dst[i] = src[int32(srcIndex)];
	}
}
