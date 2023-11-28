
#ifndef _XML2_XMLPARSER_H
#define _XML2_XMLPARSER_H

#include <xml2/IXMLOStr.h>
#include <xml2/BParser.h>

namespace B {
namespace XML {

using namespace Support2;

/**************************************************************************************/

class BXMLParser : public BXMLParseContext
{
	public:

								BXMLParser(IXMLOStr::arg stream);
		virtual					~BXMLParser();
	
		virtual status_t		OnDocumentBegin(uint32 flags);
		virtual status_t		OnDocumentEnd();
	
		virtual status_t		OnStartTag(BString &name, BValue &attributes);
		virtual status_t		OnEndTag(BString &name);
		virtual status_t		OnTextData(const char *data, int32 size);
		virtual status_t		OnCData(const char *data, int32 size);
	
		virtual status_t		OnGeneralParsedEntityRef(BString &name);
		virtual status_t		OnGeneralParsedEntityRef(BString &name, BString &replacement);
	
	private:
	
		IXMLOStr::ptr 			m_stream;
};

/**************************************************************************************/

} } // namespace B::XML

#endif // _B_XML2_PARSER_H
