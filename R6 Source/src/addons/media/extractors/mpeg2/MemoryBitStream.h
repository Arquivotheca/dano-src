#ifndef C_MEMORY_BITSTREAM_H

#define C_MEMORY_BITSTREAM_H

#include "BitStream.h"

class CMemoryBitStream : public CBitStream
{
	CMemoryBitStream (const CMemoryBitStream &);
	CMemoryBitStream &operator= (const CMemoryBitStream &);

	const char *fData;
	size_t fSize;
	size_t fOffset;
		
	protected:
		virtual ssize_t Read (void *bytes, size_t size);
		virtual ssize_t Skip (size_t size);
	
	public:
		CMemoryBitStream (const char *data, size_t size);
		
		size_t Position() const;
		bool HasData() const;
};

#endif
