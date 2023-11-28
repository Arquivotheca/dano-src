#include "BitStream.h"

CBitStream::CBitStream()
	: fRemainder(0),
	  fRemainderBitsLeft(0),
	  fBuffer(new uint8[kBufferSize]),
	  fByteOffset(kBufferSize),
	  fBufferLength(0)
{
}

CBitStream::~CBitStream()
{
	delete[] fBuffer;
}

void 
CBitStream::SkipBytes(size_t size)
{
	SkipBits(8*size);
}

void 
CBitStream::SeekByteBoundary()
{
	SkipBits(fRemainderBitsLeft % 8);
}

void 
CBitStream::MakeEmpty()
{
	fRemainderBitsLeft=0;
	fByteOffset=kBufferSize;
}

size_t 
CBitStream::BufferedBitsLeft() const
{
	return fRemainderBitsLeft+8*(fBufferLength-fByteOffset);
}

void 
CBitStream::GetBytes (void *data, size_t size)
{
	ASSERT((fRemainderBitsLeft % 8)==0);

	while (fRemainderBitsLeft>0)
	{
		*(uint8 *)data=GetBits(8);
		data=(char *)data+1;
		--size;
	}
		
	size_t cut=min_c(fBufferLength-fByteOffset,size);
	
	if (cut)
	{
		memcpy(data,fBuffer+fByteOffset,cut);
		data=(char *)data+cut;
		size-=cut;	
		fByteOffset+=cut;
	}
	
	ssize_t nbytes=Read(data,size);

	if (nbytes<(ssize_t)size)
		throw eof_exception();
}

