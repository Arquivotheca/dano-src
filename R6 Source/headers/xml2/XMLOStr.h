
#ifndef _XML2_XMLOSTR_H
#define _XML2_XMLOSTR_H

#include <xml2/IXMLOStr.h>
#include <support2/Binder.h>

namespace B {
namespace XML {

using namespace Support2;

/**************************************************************************************/

class CXMLOStr : public LInterface<IXMLOStr>
{
	public:

		virtual	status_t	Told(value &in);
};

/**************************************************************************************/

} } // namespace B::XML

#endif // _B_XML2_PARSER_H
