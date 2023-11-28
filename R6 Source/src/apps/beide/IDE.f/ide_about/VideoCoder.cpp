//========================================================================
//	VideoCoder.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Class VideoCoder
//	Implement a simple RLE animation scheme for encoding 
//	frames into a compressed stream

#include "VideoCoder.h"

#if 0
void VideoHeader::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		version = B_BENDIAN_TO_HOST_INT32(version);
		xSize = B_BENDIAN_TO_HOST_INT32(xSize);
		ySize = B_BENDIAN_TO_HOST_INT32(ySize);
	}
}
#endif
void VideoHeader::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		version = B_HOST_TO_BENDIAN_INT32(version);
		xSize = B_HOST_TO_BENDIAN_INT32(xSize);
		ySize = B_HOST_TO_BENDIAN_INT32(ySize);
	}
}

void ItemHeader::SwapBigToHost()
{
	if (B_HOST_IS_LENDIAN)
	{
		tag = B_BENDIAN_TO_HOST_INT32(tag);
		size = B_BENDIAN_TO_HOST_INT32(size);
	}
}
void ItemHeader::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		tag = B_HOST_TO_BENDIAN_INT32(tag);
		size = B_HOST_TO_BENDIAN_INT32(size);
	}
}
