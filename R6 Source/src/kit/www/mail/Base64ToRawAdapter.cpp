/*
	Base64ToRawAdapter.cpp
*/
#include <OS.h>
#include <String.h>
#include <ctype.h>
#include <stdio.h>
#include "Base64ToRawAdapter.h"

static char kBase64Value[] = {
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0,62,  0, 0, 0,63,
    52,53,54,55, 56,57,58,59, 60,61, 0, 0,  0, 0, 0, 0,
     0, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25, 0,  0, 0, 0, 0,
     0,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
     0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0
};

Base64ToRawAdapter::Base64ToRawAdapter(BDataIO *source, bool owning)
	:	fSource(source),
		fOwning(owning),
		fLeftCount(0),
		fDecodedSize(0),
		fReadOffset(0),
		fMoreData(true)
{

}

Base64ToRawAdapter::~Base64ToRawAdapter()
{
	if (fOwning)
		delete fSource;
}

inline ssize_t Base64ToRawAdapter::FillBuffer()
{
	if (!fMoreData)
		return -1;

	fReadOffset = 0;

	// Recover unused codes
	if (fLeftCount)
		memcpy(fBuffer, fLeftOver, fLeftCount);
	
	int rawSize = fSource->Read(&fBuffer[fLeftCount], kRawBufferSize - fLeftCount);
	if (rawSize <= 0) {
		fMoreData = false;
		return -1;
	}

	rawSize += fLeftCount;

	// Convert the buffer in-place.
	uchar code[4];
	int codeCount = 0;
	int in = 0;
	fDecodedSize = 0;
	while (in < rawSize) {
		// Try to grab 4 codes, skipping whitespace.
		if (codeCount < 4) {
			char c = fBuffer[in++];
			if (isspace(c))
				continue;
				
			code[codeCount++] = c;
			if (codeCount < 4)
				continue;
		}

		codeCount = 0;

		// Translate into 3 bytes
		if (code[0] == '=' || code[1] == '=') {
			fMoreData = false;
			break;
		}
			
		code[0] = kBase64Value[code[0]];
		code[1] = kBase64Value[code[1]];
		fBuffer[fDecodedSize++] = (code[0] << 2) | (code[1] >> 4);
		if (code[2] != '=') {
			code[2] = kBase64Value[code[2]];
			fBuffer[fDecodedSize++] = ((code[1] << 4) | (code[2] >> 2)) & 0xff;
			if (code[3] != '=') {
				code[3] = kBase64Value[code[3]];
				fBuffer[fDecodedSize++] = ((code[2] << 6) | code[3]) & 0xff;
			} else
				fMoreData = false;
		} else
			fMoreData = false;
	}

	// Save unused codes
	fLeftCount = codeCount;
	if (codeCount)
		memcpy(fLeftOver, code, codeCount);
	
	return fDecodedSize;
}

ssize_t Base64ToRawAdapter::Read(void *buffer, size_t size)
{
	if (fReadOffset == fDecodedSize)
		if (FillBuffer() < 0)
			return -1;
			
	ssize_t sizeToRead = MIN((ssize_t)size, (ssize_t)(fDecodedSize - fReadOffset));
	memcpy(buffer, &fBuffer[fReadOffset], sizeToRead);
	fReadOffset += sizeToRead;
	return sizeToRead;
}

ssize_t Base64ToRawAdapter::Write(const void *, size_t )
{
	return B_ERROR;
}

int32 Base64ToRawAdapter::Decode(const void *data, int32 dataSize, BString *out)
{
	*out = B_EMPTY_STRING;
	BMallocIO buffer;
	buffer.Write(data, dataSize);
	buffer.Seek(0, SEEK_SET);
	Base64ToRawAdapter adapter(&buffer, false);
	ssize_t read = 0;
	ssize_t totalRead = 0;
	ssize_t size = (dataSize * 4) / 3;
	if (size > 0) {
		char *encoded = out->LockBuffer(size);
		while ((read = adapter.Read(encoded + totalRead, size - totalRead)) > 0)
			totalRead += read;
		out->UnlockBuffer(totalRead);
	}
	return totalRead;
}

int32 Base64ToRawAdapter::Decode(const void *data, int32 dataSize, BDataIO *out)
{
	BMallocIO buffer;
	buffer.Write(data, dataSize);
	buffer.Seek(0, SEEK_SET);
	Base64ToRawAdapter adapter(&buffer, false);
	ssize_t read = 0;
	ssize_t totalRead = 0;
	char temp[1024];
	while ((read = adapter.Read(temp, 1024)) > 0)
		totalRead += out->Write(temp, read);
	return totalRead;
}
