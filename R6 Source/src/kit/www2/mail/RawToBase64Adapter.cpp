/*
	RawToBase64Adapter.cpp
*/

#include <Debug.h>
#include <String.h>
#include <stdio.h>
#include "RawToBase64Adapter.h"

const int kBreakWidth = 72;

RawToBase64Adapter::RawToBase64Adapter(BDataIO *source, bool owning)
	:	fBufferSize(0),
		fBufferOffset(0),
		fColumnCount(0),
		fOwning(owning),
		fBreakLines(true),
		fSource(source),
		fMoreData(true),
		fFinished(false)
{

}

RawToBase64Adapter::~RawToBase64Adapter()
{
	if (fOwning)
		delete fSource;
}

void RawToBase64Adapter::FillBuffer()
{
	const char *kBase64Char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	fBufferOffset = 0;
	fBufferSize = 0;

	if (!fMoreData)
		return;

	// Grab raw data.  Note that kRawBufferSize will be a multiple of 3 because
	// kBase64BufferSize is a multiple of 4.
	const int kRawBufferSize = kBase64BufferSize * 3 / 4;
	char rawBuffer[kRawBufferSize];
	int len = 0;
	while (len < kRawBufferSize) {
		int got = fSource->Read(rawBuffer + len, kRawBufferSize - len);
		if (got <= 0) {
			fMoreData = false;
			break;
		}

		len += got;	
	}
	
	// Decode and save
	uint8 *in = reinterpret_cast<uint8 *>(rawBuffer);
	while (len >= 3) {
		fBuffer[fBufferSize++] = kBase64Char[in[0] >> 2];
		fBuffer[fBufferSize++] = kBase64Char[((in[0] & 3) << 4) | (in[1] >> 4)];
		fBuffer[fBufferSize++] = kBase64Char[((in[1] & 0xf) << 2) | ((in[2] >> 6) & 3)];
		fBuffer[fBufferSize++] = kBase64Char[in[2] & 63];
		len -= 3;
		in += 3;
	}
	
	switch (len) {
		case 1:
			fBuffer[fBufferSize++] = kBase64Char[in[0] >> 2];
			fBuffer[fBufferSize++] = kBase64Char[(in[0] & 3) << 4];
			fBuffer[fBufferSize++] = '=';
			fBuffer[fBufferSize++] = '=';
			break;
	
		case 2:
			fBuffer[fBufferSize++] = kBase64Char[in[0] >> 2];
			fBuffer[fBufferSize++] = kBase64Char[((in[0] & 3) << 4) | (in[1] >> 4)];
			fBuffer[fBufferSize++] = kBase64Char[(in[1] & 0xf) << 2];
			fBuffer[fBufferSize++] = '=';
			break;
	}
}

ssize_t RawToBase64Adapter::Read(void *buffer, size_t size)
{
	int copied = 0;
	char * const out = reinterpret_cast<char*>(buffer);
	while (copied < (int)size) {
		if (fBufferOffset == fBufferSize) {
			FillBuffer();
			if (fBufferSize == 0)
				break;
		}
		
		if (fBreakLines && (fColumnCount == kBreakWidth)) {
			out[copied++] = '\r';
			fColumnCount++;
			if (copied >= (int)size)
				break;
		}

		if (fBreakLines && (fColumnCount == (kBreakWidth + 1))) {
			out[copied++] = '\n';
			fColumnCount = 0;
			if (copied >= (int)size)
				break;
		}
		
		ssize_t sizeToRead = MIN((ssize_t)(size - copied), (ssize_t)(fBufferSize - fBufferOffset));
		
		if (fBreakLines)
			sizeToRead = MIN(sizeToRead, kBreakWidth - fColumnCount);
		
		memcpy(out + copied, fBuffer + fBufferOffset, sizeToRead);
		fBufferOffset += sizeToRead;
		fColumnCount += sizeToRead;
		copied += sizeToRead;
	}

	if ((copied == 0) && (!fFinished)) {
		fFinished = true;
		if (size < 2) 
			return 0;
		memcpy(buffer, "\r\n", 2);
		return 2;
	}

	return copied;
}

void RawToBase64Adapter::SetBreakLines(bool state)
{
	fBreakLines = state;
}

ssize_t RawToBase64Adapter::Write(const void *, size_t )
{
	return B_ERROR;
}

int32 RawToBase64Adapter::Encode(const void *data, int32 dataSize, BString *out)
{
	*out = B_EMPTY_STRING;
	BMallocIO buffer;
	buffer.Write(data, dataSize);
	buffer.Seek(0, SEEK_SET);
	RawToBase64Adapter adapter(&buffer, false);
	ssize_t read = 0;
	ssize_t totalRead = 0;
	ssize_t size = dataSize * 4 / 3;
	size += size % 4;
	if (size > 0) {
		char *encoded = out->LockBuffer(size);
		while ((read = adapter.Read(encoded + totalRead, size - totalRead)) > 0)
			totalRead += read;
		out->UnlockBuffer(totalRead);
	}
	return totalRead;
}

// #endif
