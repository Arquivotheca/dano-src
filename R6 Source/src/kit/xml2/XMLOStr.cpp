
#include <xml2/XMLOStr.h>

namespace B {
namespace XML {

const BValue IXMLOStr::descriptor(BValue::TypeInfo(typeid(IXMLOStr)));

status_t 
IXMLOStr::Comment(const char *, int32)
{
	return B_OK;
}

status_t 
CXMLOStr::Told(value &)
{
	#warning Implement CXMLOStr::Told()
	return B_UNSUPPORTED;
}

}; // namespace XML
}; // namespace B
