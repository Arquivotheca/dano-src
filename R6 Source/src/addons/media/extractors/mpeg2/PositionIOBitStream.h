#ifndef C_POSITION_IO_BITSTREAM_H

#define C_POSITION_IO_BITSTREAM_H

#include "BitStream.h"

class BPositionIO;

class CPositionIOBitStream : public CBitStream
{
	CPositionIOBitStream (const CPositionIOBitStream &);
	CPositionIOBitStream &operator= (const CPositionIOBitStream &);
	
	BPositionIO *fByteStream;

	protected:
		virtual ssize_t Read (void *bytes, size_t size);
		virtual ssize_t Skip (size_t size);
			
	public:
		CPositionIOBitStream (BPositionIO *byte_stream, bool own = false);
		virtual ~CPositionIOBitStream();
};

#endif
