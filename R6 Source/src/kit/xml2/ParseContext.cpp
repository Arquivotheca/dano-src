
#include <xml2/BParser.h>
#include <stdio.h>
#include <string.h>


namespace B {
namespace XML {

// =====================================================================
BXMLParseContext::BXMLParseContext()
	:line(0),
	 column(0)
{
	
}


// =====================================================================
BXMLParseContext::~BXMLParseContext()
{
	
}


// =====================================================================
status_t
BXMLParseContext::OnDocumentBegin(uint32 flags)
{
	(void) flags;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnDocumentEnd()
{
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnStartTag(			BString		& name,
										BValue		& attributes		)
{
	(void) name;
	(void) attributes;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnEndTag(				BString		& name				)
{
	(void) name;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnTextData(			const char	* data,
										int32		size				)
{
	(void) data;
	(void) size;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnCData(				const char	* data,
										int32		size				)
{
	(void) data;
	(void) size;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnComment(			const char	* data,
										int32		size				)
{
	(void) data;
	(void) size;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnDocumentTypeBegin(	BString		& name				)
{
	(void) name;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnExternalSubset(		BString		& publicID,
										BString		& systemID,
										uint32 		parseFlags			)
{
	(void) publicID;
	(void) systemID;
	(void) parseFlags;
	return B_NO_ERROR;
}


	
// =====================================================================
status_t
BXMLParseContext::OnInternalSubsetBegin()
{
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnInternalSubsetEnd()
{
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnDocumentTypeEnd()
{
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnProcessingInstruction(	BString		& target,
											BString		& data			)
{
	(void) target;
	(void) data;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnElementDecl(			BString		& name,
											BString		& data			)
{
	(void) name;
	(void) data;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnAttributeDecl(		BString		& element,
										BString		& name,
										BString		& type,
										BString		& behavior,
										BString		& defaultVal		)
{
	(void) element;
	(void) name;
	(void) type;
	(void) behavior;
	(void) defaultVal;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnInternalParsedEntityDecl(	BString	& name,
												BString & internalData,
												bool	parameter,
												uint32	parseFlags		)
{
	(void) name;
	(void) internalData;
	(void) parameter;
	(void) parseFlags;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnExternalParsedEntityDecl(	BString	& name,
												BString & publicID,
												BString & systemID,
												bool	 parameter,
												uint32	parseFlags		)
{
	(void) name;
	(void) publicID;
	(void) systemID;
	(void) parameter;
	(void) parseFlags;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnUnparsedEntityDecl(		BString	& name,
											BString & publicID,
											BString & systemID,
											BString & notation			)
{
	(void) name;
	(void) publicID;
	(void) systemID;
	(void) notation;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnNotationDecl(			BString	& name,
											BString	& value				)
{
	(void) name;
	(void) value;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnGeneralParsedEntityRef(	BString	& name				)
{
	// If this doesn't get overridden, then we pretend that we just
	// got some text instead of trying to worry about it.
	BString replacement("&");
	replacement += name;
	replacement += ';';
	return OnTextData(replacement.String(), replacement.Length());
}


// =====================================================================
status_t
BXMLParseContext::OnGeneralParsedEntityRef(	BString	& name,
											BString & replacement		)
{
	replacement = "&";
	replacement += name;
	replacement += ';';
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnParameterEntityRef(		BString	& name				)
{
	(void) name;
	
	// Don't mimic the behavior of other entity refs because there can't
	// be Text nodes in any place where a Parameter Entity Reference can happen
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLParseContext::OnParameterEntityRef(		BString	& name,
											BString & replacement		)
{
	(void) replacement;
	
	replacement = "%";
	replacement += name;
	replacement += ';';
	return B_NO_ERROR;
}

// debugLineNo is the line number that the OnError call came from
// =====================================================================
status_t
BXMLParseContext::OnError(status_t err, bool fatal, int32 debugLineNo,
							uint32 code, void * data)
{
	(void) fatal;
	(void) debugLineNo;
	(void) code;
	(void) data;
	
#if 0
	const char * error_message = "unknown";
	switch (err)
	{
#include "error_strings.cpp"
		default:
			error_message = strerror(err);
	}
	fprintf(stderr, "%sXML Error 0x%08lx (%s) on line %ld, column %ld, "
					"debugLineNo: %ld\n",
					fatal ? "FATAL " : "", err, error_message, line, column,
					debugLineNo);
#endif
	
	return err;
}



}; // namespace XML
}; // namespace B



