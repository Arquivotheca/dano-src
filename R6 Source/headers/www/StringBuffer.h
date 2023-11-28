#ifndef _STRING_BUFFER_H
#define _STRING_BUFFER_H

#include <stdlib.h>
#include "URL.h"

const int kStringBufferAllocSize = 1024;

class StringBuffer {
public:
	inline StringBuffer(int maxLength=0x7fffffff);
	inline ~StringBuffer();
	inline StringBuffer& Clear();
	const char *String();
	inline int Length() const;
	StringBuffer& Append(char c);
	StringBuffer& Append(const char *s);
	StringBuffer& Append(unsigned d);
	StringBuffer& Append(int64 d);
	inline StringBuffer& Append(const Wagner::URL&);
	inline StringBuffer& operator<<(char c);
	inline StringBuffer& operator<<(const char *s);
	inline StringBuffer& operator<<(unsigned d);
	inline StringBuffer& operator<<(unsigned long d);
	inline StringBuffer& operator<<(int d);
	inline StringBuffer& operator<<(long d);
	inline StringBuffer& operator<<(int64 d);
	inline StringBuffer& operator<<(const Wagner::URL&);
	inline StringBuffer& SetTo(const char *s);
	inline StringBuffer& operator=(const char *s);
private:
	inline StringBuffer &AppendInternal(char c);
	inline StringBuffer &AppendInternal(const char *c);

	int fOffset;
	int fMaxLength;
	char fFixedBuffer[kStringBufferAllocSize];
	char *fDynamicBuffer;
};

inline StringBuffer::StringBuffer(int maxLength)
	:	fOffset(0),
		fMaxLength(maxLength - 1),
		fDynamicBuffer(0)
{
}

inline StringBuffer::~StringBuffer()
{
	free(fDynamicBuffer);
}

inline StringBuffer& StringBuffer::Append(const Wagner::URL &url)
{
	url.AppendTo(*this);
	return *this;
}

inline StringBuffer& StringBuffer::Clear()
{
	fOffset = 0;
	return *this;
}

inline StringBuffer& StringBuffer::operator<<(char c)
{
	return Append(c);
}

inline StringBuffer& StringBuffer::operator<<(const char *s)
{
	return Append(s);
}

inline StringBuffer& StringBuffer::operator<<(unsigned d)
{
	return Append(d);
}

inline StringBuffer& StringBuffer::operator<<(unsigned long d)
{
	return Append(static_cast<unsigned>(d));
}

inline StringBuffer& StringBuffer::operator<<(int d)
{
	return Append(static_cast<unsigned>(d));
}

inline StringBuffer& StringBuffer::operator<<(long d)
{
	return Append(static_cast<unsigned>(d));
}

inline StringBuffer& StringBuffer::operator<<(int64 d)
{
	return Append(d);
}

inline StringBuffer& StringBuffer::operator<<(const Wagner::URL &url)
{
	return Append(url);
}

inline int StringBuffer::Length() const
{
	return fOffset;
}

inline StringBuffer& StringBuffer::SetTo(const char *s)
{
	Clear();
	return Append(s);
}

inline StringBuffer& StringBuffer::operator=(const char *s)
{
	return SetTo(s);
}

#endif
