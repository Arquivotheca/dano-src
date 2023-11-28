#ifndef _STRING_BUFFER_H
#define _STRING_BUFFER_H

#include <support2/URL.h>

#include <stdlib.h>

namespace B {
namespace Support2 {

const int kStringBufferAllocSize = 1024;

class BStringBuffer
{
	public:
		inline BStringBuffer(int maxLength = 0x7fffffff);
		inline ~BStringBuffer();
		inline BStringBuffer& Clear();
		const char *String();
		inline int Length() const;
		BStringBuffer& Append(char c);
		BStringBuffer& Append(const char *s);
		BStringBuffer& Append(unsigned d);
		BStringBuffer& Append(int64 d);
		inline BStringBuffer& Append(const BUrl &url);
		inline BStringBuffer& operator<<(char c);
		inline BStringBuffer& operator<<(const char *s);
		inline BStringBuffer& operator<<(unsigned d);
		inline BStringBuffer& operator<<(unsigned long d);
		inline BStringBuffer& operator<<(int d);
		inline BStringBuffer& operator<<(long d);
		inline BStringBuffer& operator<<(int64 d);
		inline BStringBuffer& operator<<(const BUrl &url);
		inline BStringBuffer& SetTo(const char *s);
		inline BStringBuffer& operator=(const char *s);
	private:
		inline BStringBuffer &AppendInternal(char c);
		inline BStringBuffer &AppendInternal(const char *c);

		int fOffset;
		int fMaxLength;
		char fFixedBuffer[kStringBufferAllocSize];
		char *fDynamicBuffer;
};

inline BStringBuffer::BStringBuffer(int maxLength)
		: fOffset(0),
		fMaxLength(maxLength - 1),
		fDynamicBuffer(0)
{
}

inline BStringBuffer::~BStringBuffer()
{
	free(fDynamicBuffer);
}

inline BStringBuffer& BStringBuffer::Append(const BUrl &url)
{
	url.AppendTo(*this);
	return *this;
}

inline BStringBuffer& BStringBuffer::Clear()
{
	fOffset = 0;
	return *this;
}

inline BStringBuffer& BStringBuffer::operator<<(char c)
{
	return Append(c);
}

inline BStringBuffer& BStringBuffer::operator<<(const char *s)
{
	return Append(s);
}

inline BStringBuffer& BStringBuffer::operator<<(unsigned d)
{
	return Append(d);
}

inline BStringBuffer& BStringBuffer::operator<<(unsigned long d)
{
	return Append(static_cast < unsigned > (d));
}

inline BStringBuffer& BStringBuffer::operator<<(int d)
{
	return Append(static_cast < unsigned > (d));
}

inline BStringBuffer& BStringBuffer::operator<<(long d)
{
	return Append(static_cast < unsigned > (d));
}

inline BStringBuffer& BStringBuffer::operator<<(int64 d)
{
	return Append(d);
}

inline BStringBuffer& BStringBuffer::operator<<(const BUrl &url)
{
	return Append(url);
}

inline int BStringBuffer::Length() const
{
	return fOffset;
}

inline BStringBuffer& BStringBuffer::SetTo(const char *s)
{
	Clear();
	return Append(s);
}

inline BStringBuffer& BStringBuffer::operator=(const char *s)
{
	return SetTo(s);
}

} } // namespace B::Support2

#endif /* _STRING_BUFFER_H */
