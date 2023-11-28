#include "DCTDecode.h"
#include <Debug.h>
#ifndef NDEBUG
#include <stdio.h>
#endif
#include <string.h>

DCTDecode::DCTDecode(Pusher *sink)
	: Pusher(sink), JPEGDecompressor(), fBytesPerRow(0), fImageComplete(false)
{
}

DCTDecode::~DCTDecode()
{
}

ssize_t 
DCTDecode::Write(const uint8 *buffer, ssize_t length, bool )
{
	if (fImageComplete) return Pusher::SINK_FULL;
	ssize_t result = WriteData(buffer, length);
	if (fImageComplete)
	{
		// clean up
		Pusher::Write(buffer, 0, true);
	}
	return result;
}

status_t 
DCTDecode::ProcessHeader(uint width, uint height, uint bytesPerPixel)
{
#if DEBUG > 0
	printf("width x height: %u x %u, %u bytesPerPixel\n", width, height, bytesPerPixel);
	fScanLine = 0;
#endif
	fBytesPerRow = width * bytesPerPixel;
	return B_OK;
}

status_t 
DCTDecode::ProcessScanLine(const uint8 *buffer, uint rows)
{
	ssize_t bytes = rows * fBytesPerRow;
#if DEBUG > 0
	printf("ProcessScanLine: first %lu, rows: %u\n", fScanLine, rows);
	fScanLine += rows;
#endif
	return Pusher::Write(buffer, bytes, false) == bytes ? B_OK : B_ERROR;
}

void 
DCTDecode::ImageComplete(void)
{
#if DEBUG > 0
	printf("ImageCompleted!\n");
#endif
	//Pusher::Write(0, 0, true);
	fImageComplete = true;
}

