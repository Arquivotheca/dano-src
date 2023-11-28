#ifndef _B_XML_DATA_SOURCE_H
#define _B_XML_DATA_SOURCE_H

#include <SupportDefs.h>


// Forward References not in namespace B::XML
// =====================================================================
class BDataIO;

namespace B {
namespace XML {

// BXMLDataSource
// =====================================================================
// Allows you to get some data to parse with minimal copying
class BXMLDataSource
{
public:
	// GetNextBuffer is the same idea as DataIO, except it doesn't require
	// a data copy.  It will be provided with a buffer in *data, so if you
	// have to copy data, then copy into that, but if you have your own
	// buffer, then just replace *data with your pointer.
	// You should return how much data there is in size, and
	// if this is the last iteration, then return a non-zero value in done.
	virtual status_t	GetNextBuffer(size_t * size, uchar ** data, int * done) = 0;
	virtual 			~BXMLDataSource();
};



// BXMLDataIOSource
// =====================================================================
// Subclass of BBXMLDataSource that uses a BDataIO to get the data
class BXMLDataIOSource : public BXMLDataSource
{
public:
						BXMLDataIOSource(BDataIO * data);
	virtual				~BXMLDataIOSource();
	virtual status_t	GetNextBuffer(size_t * size, uchar ** data, int * done);

private:
						// Illegal
						BXMLDataIOSource();
						BXMLDataIOSource(const BXMLDataIOSource & );
	BDataIO		* _data;
};



// BXMLBufferSource
// =====================================================================
// Subclass of BBXMLDataSource that uses a buffer you give it to get the data
class BXMLBufferSource : public BXMLDataSource
{
public:
						// If len < 0, it is null terminated, so do strlen
						BXMLBufferSource(const char * buffer, int32 len = -1);
	virtual				~BXMLBufferSource();
	virtual status_t	GetNextBuffer(size_t * size, uchar ** data, int * done);

private:
						// Illegal
						BXMLBufferSource();
						BXMLBufferSource(const BXMLBufferSource & copy);
	const char	* _data;
	int32		_length;
};


}; // namespace XML
}; // namespace B


#endif // _B_XML_DATA_SOURCE_H
