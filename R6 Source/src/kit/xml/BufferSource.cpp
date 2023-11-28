#include <string.h>

#include <xml/BDataSource.h>


namespace B {
namespace XML {


// =====================================================================
BXMLBufferSource::BXMLBufferSource(const char * buffer, int32 len)
	:_data(buffer),
	 _length(len)
{
	if (_length < 0 && buffer)
		_length = strlen(buffer);
	else
		_length = len;
	// Nothing
}


// =====================================================================
BXMLBufferSource::~BXMLBufferSource()
{
	// Nothing
}


// =====================================================================
status_t
BXMLBufferSource::GetNextBuffer(size_t * size, uchar ** data, int * done)
{
	if (!_data || _length < 0)
		return B_NO_INIT;
	*size = _length;
	*data = (uchar *) _data;
	*done = 1;
	return B_OK;
}


}; // namespace XML
}; // namespace B
