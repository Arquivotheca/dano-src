
#ifndef _XML_B_WRITER_H
#define _XML_B_WRITER_H

#include <xml/BContent.h>

#include <SmartArray.h>

namespace B {
namespace XML {

// Convenience Functions
// =====================================================================
status_t WriteXML(const BXMLObject * object, BDataIO * stream, bool noWhitespace);


// Used for writing.  Should be private XXX
// =====================================================================
enum {
	stfLeaf		= 0x00000001,
	stfCanAddWS	= 0x00000002
};


// BCodifier 
// =====================================================================
class BCodifier {
public:
						BCodifier();
	virtual				~BCodifier();
	
	virtual status_t	StartTag(const BString &name, const BStringMap &attributes, uint32 formattingHints=0);
	virtual status_t	EndTag(const BString &name);
	virtual status_t	TextData(const char	* data, int32 size);
	virtual status_t	Comment(const char *data, int32 size);
};

// BOutputStream
// =====================================================================
class BOutputStream : public BCodifier {
public:
						BOutputStream(BDataIO &stream, bool writeHeader=false);
	virtual				~BOutputStream();
	
	virtual status_t	StartTag(const BString &name, const BStringMap &attributes, uint32 formattingHints=0);
	virtual status_t	EndTag(const BString &name);
	virtual status_t	TextData(const char	* data, int32 size);
	virtual status_t	Comment(const char *data, int32 size);

private:
	void		Indent();

	BDataIO	&	m_stream;
	int32		m_depth;
	int32		m_lastPrettyDepth;
	bool		m_isLeaf:1;
	bool		m_openStartTag:1;
	uint8		m_reserved:6;
};



// Consider this an inverse parser
class BWriter
{
public:
						BWriter(BDataIO * data, uint32 formattingStyle);
	virtual				~BWriter();
	
	// Stuff that goes only in DTDs
	virtual status_t	BeginDoctype(const BString & elementName, const BString & systemID, const BString & publicID);
	virtual status_t	EndDoctype();
	virtual status_t	OnElementDecl(const BElementDecl * decl);
	virtual status_t	OnEntityDecl(const BEntityDecl * decl);
	
	// Stuff that goes only in the document part
	virtual status_t	StartTag(const BString &name, const BStringMap &attributes, uint32 formattingHints=0);
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
	
	BDataIO				* m_stream;
	SmartArray<BString>	m_elementStack;
	uint32				m_formattingStyle;
	bool				m_openStartTag;
	bool				m_doneDOCTYPE;
	bool				m_startedInternalDTD;
	int32				m_depth;
	int32				m_lastPrettyDepth;
};

}; // namespace XML
}; // namespace B

#endif // _XML_B_WRITER_H

