
#include <xml2/XMLParser.h>

namespace B {
namespace XML {

/*********************************************************************/

BXMLParser::BXMLParser(IXMLOStr::arg stream) : m_stream(stream)
{
}

BXMLParser::~BXMLParser()
{
}

status_t 
BXMLParser::OnDocumentBegin(uint32)
{
	return B_OK;
}

status_t 
BXMLParser::OnDocumentEnd()
{
	return B_OK;
}

status_t 
BXMLParser::OnStartTag(BString &name, BValue &attributes)
{
	return m_stream->StartTag(name,attributes);
}

status_t 
BXMLParser::OnEndTag(BString &name)
{
	return m_stream->EndTag(name);
}

status_t 
BXMLParser::OnTextData(const char *data, int32 size)
{
	return m_stream->Content(data,size);
}

status_t 
BXMLParser::OnCData(const char *data, int32 size)
{
	return m_stream->Content(data,size);
}

status_t 
BXMLParser::OnGeneralParsedEntityRef(BString &name)
{
	char c;
	if (name == "quot") c = '"';
	else if (name == "amp")  c = '&';
	else if (name == "lt")   c = '<';
	else if (name == "gt")   c = '>';
	else if (name == "apos") c = '\'';
	else return B_XML_ENTITY_NOT_FOUND;

	return OnTextData(&c,1);
}

status_t 
BXMLParser::OnGeneralParsedEntityRef(BString &name, BString &replacement)
{
	char c;
	if (name == "quot") c = '"';
	else if (name == "amp")  c = '&';
	else if (name == "lt")   c = '<';
	else if (name == "gt")   c = '>';
	else if (name == "apos") c = '\'';
	else return B_XML_ENTITY_NOT_FOUND;
	
	replacement.SetTo(c,1);
	
	return B_OK;
}

}; // namespace XML
}; // namespace B
