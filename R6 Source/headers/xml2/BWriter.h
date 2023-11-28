
#ifndef _XML2_B_WRITER_H
#define _XML2_B_WRITER_H

#include <support2/Vector.h>
#include <support2/IByteStream.h>
#include <xml2/BContent.h>
#include <xml2/XMLOStr.h>

namespace B {
namespace XML {

using namespace Support2;

// Convenience Functions
// =====================================================================
status_t WriteXML(const BXMLObject * object, IByteOutput::arg stream, bool noWhitespace);


// Used for writing.  Should be private XXX
// =====================================================================
enum {
	stfLeaf		= 0x00000001,
	stfCanAddWS	= 0x00000002
};

// BOutputStream
// =====================================================================
class BOutputStream : public CXMLOStr {
public:
						BOutputStream(IByteOutput::arg stream, bool writeHeader=false);
	virtual				~BOutputStream();
	
	virtual status_t	StartTag(BString &name, BValue &attributes);
	virtual status_t	EndTag(BString &name);
	virtual status_t	TextData(const char	* data, int32 size);
	virtual status_t	Comment(const char *data, int32 size);

private:
	void		Indent();

	IByteOutput::ptr	m_stream;
	int32				m_depth;
	int32				m_lastPrettyDepth;
	bool				m_isLeaf:1;
	bool				m_openStartTag:1;
	uint8				m_reserved:6;
};



// Consider this an inverse parser
class BWriter
{
public:
						BWriter(IByteOutput::arg data, uint32 formattingStyle);
	virtual				~BWriter();
	
	// Stuff that goes only in DTDs
	virtual status_t	BeginDoctype(const BString & elementName, const BString & systemID, const BString & publicID);
	virtual status_t	EndDoctype();
	virtual status_t	OnElementDecl(const BElementDecl * decl);
	virtual status_t	OnEntityDecl(const BEntityDecl * decl);
	
	// Stuff that goes only in the document part
	virtual status_t	StartTag(const BString &name, const BValue &attributes, uint32 formattingHints=0);
	virtual status_t	EndTag(); // Will always do propper start-tag matching
	virtual status_t	TextData(const char	* data, int32 size);
	virtual status_t	CData(const char	* data, int32 size);
	
	
	// Stuff that can go anywhere
	virtual status_t	Comment(const char *data, int32 size);
	virtual status_t	ProcessingInstruction(const BString & target, const BString & data);
	
	// Formatting Styles
	enum
	{
		BALANCE_WHITESPACE		= 0x00000001,
		SKIP_DOCTYPE_OPENER		= 0x00000002
	};
	
	// Formatting Hints
	enum
	{
		NO_EXTRA_WHITESPACE		= 0x80000002
	};
	
private:
	status_t	open_doctype();
	status_t	indent();
	
	IByteOutput::ptr	m_stream;
	BVector<BString>	m_elementStack;
	uint32				m_formattingStyle;
	bool				m_openStartTag;
	bool				m_doneDOCTYPE;
	bool				m_startedInternalDTD;
	int32				m_depth;
	int32				m_lastPrettyDepth;
};

}; // namespace XML
}; // namespace B

#endif // _XML2_B_WRITER_H

