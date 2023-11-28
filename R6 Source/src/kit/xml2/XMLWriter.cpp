
#include <support2/IByteStream.h>
#include <xml2/BWriter.h>
#include <xml2/BParser.h>

namespace B {
namespace XML {

/****************************************************************************/

const char *indents		= "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
const char *markup		= "&<>'\"";
int32 entityLengths[]	= { 5, 4, 4, 6, 6 };
const char *entities[]	= { "&amp;", "&lt;", "&gt;", "&apos;", "&quot;" };

status_t write_xml_data(IByteOutput::arg stream, const char *data, int32 size)
{
	const char *start = data;
	const char *end = start;
	const char *p = markup;
	char c;

	while (size>0) {
		while (size--) {
			c = *end;
			p = markup;
			while (*p) { if (c == *p) goto gotSegment; p++; };
			end++;
		}
		
		gotSegment:

		if (end-start) stream->Write(start,end-start);
		if (*p) { stream->Write(entities[p-markup],entityLengths[p-markup]); end++; }
		start=end;
	}
	
	return B_OK;
}

BOutputStream::BOutputStream(IByteOutput::arg stream, bool writeHeader)
	: m_stream(stream)
{
	m_depth = writeHeader?-1:0;
	m_lastPrettyDepth = 0;
	m_isLeaf = false;
	m_openStartTag = false;
}


BOutputStream::~BOutputStream()
{
	m_stream->Write("\n",1);
}

void 
BOutputStream::Indent()
{
	if (m_lastPrettyDepth == m_depth) {
		const char *s = indents;
		int32 onetime=1,size,depth = m_depth;
		do {
			size = depth;
			if (size > 16) size = 16;
			depth -= size;
			m_stream->Write(indents,size+onetime);
			s += onetime;
			onetime = 0;
		} while (depth);
	}
}

const char *xmlHeader = "<?xml version=\"1.0\"?>";

status_t 
BOutputStream::StartTag(BString &name, BValue &attr)
{
	BValue key, value;
	
	if (m_openStartTag) {
		m_stream->Write(">",1);
		m_openStartTag = false;
	}

	if (m_depth == -1) {
		m_stream->Write(xmlHeader,strlen(xmlHeader));
		m_depth = 0;
	};
	
	Indent();
	
	m_stream->Write("<",1);
	m_stream->Write(name.String(),name.Length());
	
	void * cookie;
	while (B_OK == attr.GetNextItem(&cookie,&key,&value)) {
		BString k = key.AsString();
		BString v = value.AsString();
		m_stream->Write(" ",1);
		m_stream->Write(k.String(),k.Length());
		m_stream->Write("=\"",2);
		write_xml_data(m_stream,v.String(),v.Length());
		m_stream->Write("\"",1);
	}
	
	m_openStartTag = true;
//	m_stream.Write(">",1);
//	m_isLeaf = (formattingHints & stfLeaf);
	m_isLeaf = false;
//	if ((m_lastPrettyDepth == m_depth) && (formattingHints & stfCanAddWS))
//		m_lastPrettyDepth = m_depth+1;
	m_depth++;

	return B_OK;
}

status_t 
BOutputStream::EndTag(BString &name)
{
	m_depth--;
	if (m_lastPrettyDepth > m_depth) m_lastPrettyDepth = m_depth;

	if (m_openStartTag) {
		m_stream->Write("/>",2);
		m_openStartTag = false;
		m_isLeaf = false;
		return B_OK;
	}

	if (!m_isLeaf) Indent();

	m_stream->Write("</",2);
	m_stream->Write(name.String(),name.Length());
	m_stream->Write(">",1);

	m_isLeaf = false;

	return B_OK;
}

status_t 
BOutputStream::TextData(const char *data, int32 size)
{
	if (m_openStartTag) {
		m_stream->Write(">",1);
		m_openStartTag = false;
	}

	return write_xml_data(m_stream,data,size);
}

status_t 
BOutputStream::Comment(const char *data, int32 size)
{
	(void) data;
	(void) size;
	return B_OK;
}


}; // namespace XML
}; // namespace B

