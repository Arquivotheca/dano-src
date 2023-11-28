#include <support2/StringBuffer.h>

using namespace B::Support2;

BStringBuffer &BStringBuffer::AppendInternal(char c)
{
	if (fOffset >= fMaxLength)
		return *this;	// Just drop extra characters
		
	// Note: compare against kStringBufferAllocSize - 1 because it will increment the
	// offset after this.  Need to save at least one free character at the
	// end of the buffer for the terminating null that gets inserted when
	// String() is called.
	if (fOffset < kStringBufferAllocSize - 1) {
		// Use the fixed size buffer.
		fFixedBuffer[fOffset++] = c;
	} else if (fOffset == kStringBufferAllocSize - 1) {
		// This is larger than the fixed size buffer, switch to the
		// dynamic buffer.  Note that I realloc because the dynamic buffer may
		// still be allocated if someone Clear()ed this.  Try to be as lazy as
		// possible about freeing it.
		fDynamicBuffer = (char*) realloc(fDynamicBuffer, kStringBufferAllocSize * 2);	
		memcpy(fDynamicBuffer, fFixedBuffer, kStringBufferAllocSize);
		fDynamicBuffer[fOffset++] = c;
	} else {
		// Use the dynamic buffer
		if (fOffset % kStringBufferAllocSize == kStringBufferAllocSize - 1)
			fDynamicBuffer = (char*) realloc(fDynamicBuffer, fOffset + kStringBufferAllocSize + 1);
		
		fDynamicBuffer[fOffset++] = c;
	}
	
	return *this;
}

BStringBuffer& BStringBuffer::AppendInternal(const char *s)
{
	const char *c = s;
	while (*c)
		AppendInternal(*c++);

	return *this;
}

BStringBuffer& BStringBuffer::Append(char c)
{
	return AppendInternal(c);
}

BStringBuffer& BStringBuffer::Append(const char *s)
{
	return AppendInternal(s);
}

BStringBuffer& BStringBuffer::Append(unsigned dec)
{
	char buf[11];
	buf[10] = '\0';
	char *c = &buf[10];
	if (dec == 0)
		*--c = '0';
	
	while (dec > 0) {
		*--c = (dec % 10) + '0';
		dec /= 10;
	}

	return Append(c);
}

BStringBuffer& BStringBuffer::Append(int64 dec)
{
	char buf[21];
	bool negative = dec < 0;
	if (negative)
		dec = -dec;
		
	buf[20] = '\0';
	char *c = &buf[20];
	if (dec == 0) *--c = '0';
	while (dec > 0) {
		*--c = (dec % 10) + '0';
		dec /= 10;
	}

	if (negative)
		*--c = '-';

	return AppendInternal(c);
}

const char* BStringBuffer::String()
{
	if (fOffset == 0)
		return "";
	else if (fOffset < kStringBufferAllocSize) {
		fFixedBuffer[fOffset] = '\0';
		return fFixedBuffer;
	}

	fDynamicBuffer[fOffset] = '\0';
	return fDynamicBuffer;
}
