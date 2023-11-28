#include "GreyToRGB.h"

GreyToRGB::GreyToRGB(Pusher *sink, uint32 samplesPerLine)
	: Pusher(sink), fRowBytes(3 * samplesPerLine), fComponent(0), fRowBuffer(new uint8[fRowBytes])
{
}


GreyToRGB::~GreyToRGB()
{
	delete [] fRowBuffer;
}

ssize_t 
GreyToRGB::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	ssize_t result = Pusher::OK;
	ssize_t origLength = length;
	while (length)
	{
		fRowBuffer[fComponent++] = *buffer;
		fRowBuffer[fComponent++] = *buffer;
		fRowBuffer[fComponent++] = *buffer++;
		length--;
		if (fComponent == fRowBytes)
		{
			result = Pusher::Write(fRowBuffer, fRowBytes, length != 0 ? false : finish);
			if (result != (ssize_t)fRowBytes) break;
			fComponent = 0;
		}
	}
	if (finish) Pusher::Write(0, 0, true);
	return result < 0 ? result : origLength - length;
}

