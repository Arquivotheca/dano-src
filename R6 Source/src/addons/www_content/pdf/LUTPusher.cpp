#include "LUTPusher.h"

LUTPusher::LUTPusher(Pusher *sink, bool oneToMany, uint32 samplesPerLine, uint32 componentsPerSample, uint8 *lut)
	: Pusher(sink), fRowBytes(samplesPerLine * componentsPerSample), fComponentsPerSample(componentsPerSample),
	fLUT(lut), fComponent(0), fRowBuffer(new uint8[fRowBytes]), fOneToMany(oneToMany ? fComponentsPerSample : 1)
{
}


LUTPusher::~LUTPusher()
{
	delete [] fRowBuffer;
	delete [] fLUT;
}

ssize_t 
LUTPusher::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	ssize_t result = Pusher::OK;
	ssize_t origLength = length;
	
	while (length)
	{
		uint8 *lut = fLUT + (*buffer * fComponentsPerSample) + (fComponent % fComponentsPerSample);
		uint32 i = fOneToMany;
		while (i--) fRowBuffer[fComponent++] = *lut++;
		buffer++;
		length--;
		if (fComponent == fRowBytes)
		{
			result = Pusher::Write(fRowBuffer, fRowBytes, length != 0 ? false : finish);
			if (result != (ssize_t)fRowBytes) break;
			fComponent = 0;
		}
	}
	return result < 0 ? result : origLength - length;
}

