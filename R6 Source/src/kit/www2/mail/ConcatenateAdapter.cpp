#include "ConcatenateAdapter.h"


ConcatenateAdapter::ConcatenateAdapter(bool owning)
	:	fOwning(owning),
		fIndex(0)
{
}

ConcatenateAdapter::~ConcatenateAdapter()
{
	for (int i = 0; i < fSources.CountItems(); i++)
		delete reinterpret_cast<BDataIO*>(fSources.ItemAt(i));
}

ssize_t ConcatenateAdapter::Read(void *buf, size_t size)
{
	ssize_t totalRead = 0;
	
	while (totalRead < size) {
		BDataIO *stream = reinterpret_cast<BDataIO*>(fSources.ItemAt(fIndex));
		if (stream == 0)
			break;
			
		ssize_t got = stream->Read(reinterpret_cast<char*>(buf) + totalRead,
			size - totalRead);
		if (got <= 0)
			fIndex++;
		else
			totalRead += got;
	}
	
	return totalRead;
}

void ConcatenateAdapter::AddStream(BDataIO *stream)
{
	fSources.AddItem(stream);
}

ssize_t ConcatenateAdapter::Write(const void *, size_t)
{
	return B_ERROR;
}



