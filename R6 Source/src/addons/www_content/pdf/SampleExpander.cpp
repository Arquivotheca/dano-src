#include "SampleExpander.h"
#define DEBUG_LEVEL 10
#if DEBUG > DEBUG_LEVEL
#include <stdio.h>
#endif

SampleExpander::SampleExpander(Pusher *sink, uint32 componentsPerLine, uint32 bitsPerComponent, bool scale)
	: Pusher(sink), fRowBytes(componentsPerLine), fBitsPerComponent(bitsPerComponent),
		fBitBuffer(0), fBitsInBuffer(0), fComponent(0), fRowBuffer(new uint8[fRowBytes]),
		fScale(scale ? ((255 - ((1 << fBitsPerComponent) - 1)) / ((1 << fBitsPerComponent) - 1)) : 0)
{
#if DEBUG > DEBUG_LEVEL
	printf("SampleExpander  fBitsPerComponent: %lu, scale: %s, fScale: %.02x\n", fBitsPerComponent, scale ? "true" : "false", fScale);
#endif
}

SampleExpander::~SampleExpander()
{
	delete [] fRowBuffer;
}

ssize_t 
SampleExpander::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	uint8 aByte;
	ssize_t origLength = length;
	ssize_t result = Pusher::OK;

	while (length)
	{
		// fill bit buffer
		if (fBitsInBuffer < fBitsPerComponent)
		{
			fBitBuffer <<= 8;
			fBitBuffer |= *buffer++;
			length--;
			fBitsInBuffer += 8;
#if DEBUG > DEBUG_LEVEL
			printf("\n%.02x ", fBitBuffer);
#endif
		}
		while (fBitsInBuffer >= fBitsPerComponent)
		{
			fBitsInBuffer -= fBitsPerComponent;
			aByte = fBitBuffer >> fBitsInBuffer;
#if DEBUG > DEBUG_LEVEL
			printf("%x", aByte);
#endif
			//fBitBuffer >>= fBitsPerComponent;
			fBitBuffer &= (1 << fBitsInBuffer) - 1;
			if (fScale) aByte += fScale * aByte;
			fRowBuffer[fComponent++] = aByte;
			// check for end of row
			if (fComponent == fRowBytes)
			{
#if DEBUG > DEBUG_LEVEL
				printf("\n");
#endif
				result = Pusher::Write(fRowBuffer, fRowBytes, length != 0 ? false : finish);
				if (result != (ssize_t)fRowBytes) goto error1;
				// reset bit buffer
				fBitsInBuffer = 0;
				fBitBuffer = 0;
				fComponent = 0;
			}
		}
	}
	if (finish) Pusher::Write(0, 0, true);
error1:
	return result < 0 ? result : origLength - length;
}

