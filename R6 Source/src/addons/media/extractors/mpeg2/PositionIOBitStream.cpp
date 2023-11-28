#include "PositionIOBitStream.h"

#include <DataIO.h>
#include <BufferIO.h>
#include <Debug.h>

CPositionIOBitStream::CPositionIOBitStream(BPositionIO *byte_stream, bool own)
	: fByteStream(new BBufferIO(byte_stream,BBufferIO::DEFAULT_BUF_SIZE,own))
{
}

CPositionIOBitStream::~CPositionIOBitStream()
{
	delete fByteStream;
}

ssize_t 
CPositionIOBitStream::Read(void *bytes, size_t size)
{
	return fByteStream->Read(bytes,size);
}

ssize_t 
CPositionIOBitStream::Skip(size_t size)
{
	fByteStream->Seek(size,SEEK_CUR);
	
	return (ssize_t)size;
}

