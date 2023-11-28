
#include <xml2/BDataSource.h>
#include <support2/IByteStream.h>

namespace B {
namespace XML {

// =====================================================================
BXMLIByteInputSource::BXMLIByteInputSource(IByteInput::arg data)
	:_data(data)
{
	// Nothing
}


// =====================================================================
BXMLIByteInputSource::~BXMLIByteInputSource()
{
	// Nothing
}


// =====================================================================
status_t
BXMLIByteInputSource::GetNextBuffer(size_t * size, uchar ** data, int * done)
{
	if (_data == NULL)
		return B_NO_INIT;
	ssize_t bufSize = *size;
	bufSize = _data->Read(*data, bufSize);
	if (bufSize < 0)
		return bufSize;						// Error
	*done = ((size_t) bufSize) < *size;
	*size = bufSize;
	return B_OK;
}

}; // namespace XML
}; // namespace B

