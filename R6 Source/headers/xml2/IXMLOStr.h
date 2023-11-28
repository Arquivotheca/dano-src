
#ifndef	_XML2_IXMLOSTR_H
#define	_XML2_IXMLOSTR_H

#include <support2/IInterface.h>
#include <support2/String.h>

namespace B {
namespace XML {

using namespace Support2;

/**************************************************************************************/

class IXMLOStr : public IInterface
{
	public:

		//the following could be B_DECLARE_META_INTERFACE(), but we've no RXMLOStr
		B_STANDARD_ATOM_TYPEDEFS(IXMLOStr)
		static const BValue							descriptor;

		virtual status_t	StartTag(BString &name, BValue &attributes) = 0;
		virtual status_t	EndTag(BString &name) = 0;
		virtual status_t	Content(const char	*data, int32 size) = 0;
		virtual status_t	Comment(const char *data, int32 size);
};

/**************************************************************************************/

} } // namespace B::XML

#endif /* _XML2_XMLOSTR_H */
