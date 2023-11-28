/*
	MallocIOAdapter.cpp
*/
#include "MallocIOAdapter.h"
#include <stdio.h> // For SEEK_SET

MallocIOAdapter::MallocIOAdapter()
	:	BMallocIO(),
		fNeedToReset(false)
{
	// This class allows me to use the same malloc io source
	// for several adapters. Normally after one of the adapters
	// had finished reading from the source, the subsequent
	// adapter would have to be aware to reset. This is somethimes
	// tricky to keep track of. This class takes care of that. Kenny
}

ssize_t MallocIOAdapter::Read(void *buffer, size_t size)
{
	if (fNeedToReset) {
		Seek(0, SEEK_SET);
		fNeedToReset = false;
	}
	ssize_t read = BMallocIO::Read(buffer, size);
	if (read == 0) {
		fNeedToReset = true;
	}
	return read;
}

ssize_t MallocIOAdapter::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (Position() == BufferLength())
		Seek(0, SEEK_SET);
	return BMallocIO::ReadAt(pos, buffer, size);
}

