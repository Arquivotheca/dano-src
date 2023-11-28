#include "CMYKtoRGB.h"

CMYKtoRGB::CMYKtoRGB(Pusher *sink, uint32 samplesPerLine)
	: Pusher(sink), fRowBytes(samplesPerLine * 3), fInComponent(0),
	fRowBuffer(new uint8[fRowBytes]), fOutComponent(0)
{
}


CMYKtoRGB::~CMYKtoRGB()
{
	delete [] fRowBuffer;
}

ssize_t 
CMYKtoRGB::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	ssize_t result = Pusher::OK;
	ssize_t origLength = length;

	while (length)
	{
		fSample[fInComponent++] = *buffer++;
		length--;
		if (fInComponent == 4)
		{
			uint16 comp;
			comp = fSample[CYAN] + fSample[BLACK];
			if (comp > 255) comp = 255;
			fRowBuffer[fOutComponent++] = 255 - comp;
			comp = fSample[MAGENTA] + fSample[BLACK];
			if (comp > 255) comp = 255;
			fRowBuffer[fOutComponent++] = 255 - comp;
			comp = fSample[YELLOW] + fSample[BLACK];
			if (comp > 255) comp = 255;
			fRowBuffer[fOutComponent++] = 255 - comp;
			if (fOutComponent == fRowBytes)
			{
				// write the line
				result = Pusher::Write(fRowBuffer, fRowBytes, length != 0 ? false : finish);
				if (result != (ssize_t)fRowBytes) break;
				fOutComponent = 0;
			}
			fInComponent = 0;
		}
	}
	return result < 0 ? result : origLength - length;
}

