/*
	StreamToFileAdapter.cpp
*/
#include <stdio.h>
#include "StreamToFileAdapter.h"

// CFS Friendly block size
const int32 kBlockSize = 1024 * 16;

StreamToFileAdapter::StreamToFileAdapter(BDataIO *source, BFile &file, bool owning)
	:	fSource(source),
		fFile(file),
		fOwning(owning)
{
	fBuffer.SetBlockSize(kBlockSize);
}

StreamToFileAdapter::~StreamToFileAdapter()
{
 	Flush();
	if (fOwning)
		delete fSource;
}

ssize_t StreamToFileAdapter::Read(void *buffer, size_t size)
{
	ssize_t read = fSource->Read(buffer, size);
	if (read > -1)
		fBuffer.Write(buffer, read);

	if (fBuffer.BufferLength() > kBlockSize)
		Flush();
		
	return read;
}

ssize_t StreamToFileAdapter::Write(const void *, size_t )
{
	return B_ERROR;
}

inline void StreamToFileAdapter::Flush()
{
	if (fBuffer.BufferLength() > 0) {
		fFile.Write(fBuffer.Buffer(), fBuffer.BufferLength());
		fBuffer.Seek(0, SEEK_SET);
		fBuffer.SetSize(0);
	}
}
