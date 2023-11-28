#include "MemoryBitStream.h"

#include <Debug.h>

ssize_t 
CMemoryBitStream::Read(void *bytes, size_t size)
{
	size_t cut=min_c(fSize-fOffset,size);
	
	memcpy (bytes,fData+fOffset,cut);
	
	fOffset+=cut;
	
	return cut;		
}

ssize_t 
CMemoryBitStream::Skip(size_t size)
{
	size_t cut=min_c(size,fSize-fOffset);
	
	fOffset+=cut;
	
	return cut;
}


CMemoryBitStream::CMemoryBitStream(const char *data, size_t size)
	: fData(data),
	  fSize(size),
	  fOffset(0)
{
}

size_t 
CMemoryBitStream::Position() const
{
	size_t buffered_bits_left=BufferedBitsLeft();
	ASSERT((buffered_bits_left % 8)==0);
	
	return fOffset-buffered_bits_left/8;
}

bool 
CMemoryBitStream::HasData() const
{
	return BufferedBitsLeft()>0 || fOffset<fSize;
}

