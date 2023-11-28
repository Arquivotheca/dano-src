
#ifndef _B_XML2_ENTITY_PARSE_CONTEXT_H
#define _B_XML2_ENTITY_PARSE_CONTEXT_H

#include <xml2/BParser.h>

namespace B {
namespace XML {

using namespace Support2;

// BXMLEntityParseContext
// =====================================================================
class BXMLEntityParseContext : public BXMLDocumentCreationContext
{
public:
				BXMLEntityParseContext(BEntityDecl * decl,
										bool parameterEntity,
										BDocumentType * dt,
										BXMLObjectFactory * factory,
										BEntityStore * entityStore,
										BDocumentTypeDefinition * dtd = NULL);
	virtual		~BXMLEntityParseContext();
	
	
	status_t	ExpandEntities(BString & str, const char delim);
	
private:
	bool				_parameterEntity;
	BEntityDecl			* _decl;
	BDocumentType		* _dt;
	bool					_deleteDTDObjectsWhenDone;
	BDocumentTypeDefinition * _dtdObjects;
	
public:
	// Implementations of BXMLDocumentCreationContext functions
	virtual status_t	OnBeginDocumentNode();
	virtual status_t	OnEndDocumentNode();
	virtual status_t	OnStartTagNode(BElement * element);
	virtual status_t	OnEndTagNode(BElement * element);
	virtual status_t	OnTextNode(BText * text);
	virtual status_t	OnCDataNode(BCData * cData);
	virtual status_t	OnCommentNode(BComment * comment);
	virtual status_t	OnBeginDocumentTypeNode(BDocumentType * dt);
	virtual status_t	OnExternalSubsetNode(BDocumentTypeDefinition * dtd);
	virtual status_t	OnBeginInternalSubsetNode(BDocumentTypeDefinition * dtd);
	virtual status_t	OnEndInternalSubsetNode();
	virtual status_t	OnEndDocumentTypeNode();
	virtual status_t	OnProcessingInstructionNode(BProcessingInstruction * pi);
	virtual	status_t	OnElementDeclNode(BElementDecl * decl);
	virtual status_t	OnAttributeDeclNode(BAttributeDecl * decl);
	virtual status_t	OnEntityDeclNode(BEntityDecl * decl);
	//virtual status_t	OnNotationDeclNode(BNotationDecl * decl);	XXX Not implemented
	
	virtual BDocumentType *	GetDocumentType();
	
	// Implementations of BXMLParseContext functions
	virtual status_t	OnGeneralParsedEntityRef(BString & name);
	virtual status_t	OnGeneralParsedEntityRef(BString & name, BString & replacement);
	virtual status_t	OnParameterEntityRef(BString & name);
	virtual status_t	OnParameterEntityRef(BString & name, BString & replacement);
};



}; // namespace XML
}; // namespace B


#endif // _B_XML2_ENTITY_PARSE_CONTEXT_H
