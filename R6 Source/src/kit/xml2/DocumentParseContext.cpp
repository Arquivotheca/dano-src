
#include <xml2/BParser.h>
#include <xml2/BContent.h>

#include <parsing.h>

#include <stdio.h>

namespace B {
namespace XML {

// =====================================================================
BXMLDocumentParseContext::BXMLDocumentParseContext(BDocument * document,
											BXMLObjectFactory * factory,
											BEntityStore * entityStore)
	:BXMLDocumentCreationContext(false, factory, entityStore),
	 _document(document)
{
	// Nothing
}


// =====================================================================
BXMLDocumentParseContext::~BXMLDocumentParseContext()
{
	// Nothing -- All members are owned by someone else
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnBeginDocumentNode()
{
	if (!_document)
		return B_NO_INIT;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnEndDocumentNode()
{
	return B_NO_ERROR;
}


// =====================================================================
// Note that PIs, comments, and elements are the only thing that can
// be added to the document root
status_t
BXMLDocumentParseContext::OnStartTagNode(BElement * element)
{
	if (!element->Parent())		// This is the document element
		return _document->AddChildLast(element);
	else
		return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnEndTagNode(BElement * element)
{
	(void) element;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnTextNode(BText * text)
{
	(void) text;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnCDataNode(BCData * cData)
{
	(void) cData;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnCommentNode(BComment * comment)
{
	if (!comment->Parent())
		return _document->AddChildLast(comment);
	else
		return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnBeginDocumentTypeNode(BDocumentType * dt)
{
	status_t err =  _document->AddChildLast(dt);
	return err;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnExternalSubsetNode(BDocumentTypeDefinition * dtd)
{
	BDocumentType * dt = _document->DocumentType();
	return dt->SetExternalSubset(dtd);
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnBeginInternalSubsetNode(BDocumentTypeDefinition * dtd)
{
	BDocumentType * dt = _document->DocumentType();
	return dt->SetInternalSubset(dtd);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnEndInternalSubsetNode()
{
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnEndDocumentTypeNode()
{
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnProcessingInstructionNode(BProcessingInstruction * pi)
{
	if (!pi->Parent())
		_document->AddChildLast(pi);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnElementDeclNode(BElementDecl * decl)
{
	BDocumentType * dt = _document->DocumentType();
	BDocumentTypeDefinition * dtd = dt->InternalSubset();
	return dtd->AddElementDecl(decl);;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnAttributeDeclNode(BAttributeDecl * decl)
{
	BDocumentType * dt = _document->DocumentType();
	BDocumentTypeDefinition * dtd = dt->InternalSubset();
	return dtd->AddAttributeDecl(decl);;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnEntityDeclNode(BEntityDecl * decl)
{
	BDocumentType * dt = _document->DocumentType();
	BDocumentTypeDefinition * dtd = dt->InternalSubset();
	return dtd->AddEntityDecl(decl);
}


// XXX Not implemented
// =====================================================================
// status_t
// BXMLDocumentParseContext::OnNotationDeclNode(BNotationDecl * decl)
// {
// 	return B_NO_ERROR;
// }


// =====================================================================
BDocumentType *
BXMLDocumentParseContext::GetDocumentType()
{
	return _document->DocumentType();
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnGeneralParsedEntityRef(	BString	& name				)
{
	BDocumentType * dt = _document->DocumentType();
	if (!dt)
	{
		BString v;
		// Use the defaults
		if (name == "quot")
			v = "\"";
		else if (name == "apos")
			v = "\'";
		else if (name == "gt")
			v = ">";
		else if (name == "lt")
			v = "<";
		else if (name == "amp")
			v = "&";
		else
			return B_XML_ENTITY_NOT_FOUND;
		BText * text = new BText();
		text->SetValue(v, true);
		if (_currentElement)
			_currentElement->AddChildLast(text);
		else
			_document->AddChildLast(text);	// I don't think this is well-formed, but whatever.
		return B_OK;
	}
	
	BEntityDecl * decl = dt->FindEntityDecl(name.String(), false);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	
	// As required by XML 1.0 ss 4.1 [WFC: Parsed Entity]
	if (decl->GetType() == B_XML_UNPARSED_ENTITY)
		return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	
	// Add in each of the BContents in the set
	BVector<BXMLObject *> & set = decl->GetReplacementContent();
	int32 count = set.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BXMLObject * obj = set.ItemAt(i);
		BContent * c = dynamic_cast<BContent *>(obj);
		if (c)	// This should never not be true
		{
			if (_currentElement)
				_currentElement->AddChildLast(c);
			else
				_document->AddChildLast(c);	// I don't think this is well-formed, but whatever.
		}
	}
	
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnGeneralParsedEntityRef(	BString	& name,
											BString & replacement		)
{
	BDocumentType * dt = _document->DocumentType();
	if (!dt)
	{
		if (name == "quot")
			replacement = "\"";
		else if (name == "apos")
			replacement = "\'";
		else if (name == "gt")
			replacement = ">";
		else if (name == "lt")
			replacement = "<";
		else if (name == "amp")
			replacement = "&";
		else
			return B_XML_ENTITY_NOT_FOUND;
		return B_OK;
	}
	
	BEntityDecl * decl = dt->FindEntityDecl(name.String(), false);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	
	// As required by XML 1.0 ss 4.1 [WFC: Parsed Entity]
	//if (decl->GetType() == B_XML_UNPARSED_ENTITY)
	//	return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	// Not here because this is for in attributes
	
	replacement = decl->GetReplacementText();
	
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnParameterEntityRef(		BString	& name				)
{
	status_t err;
	BDocumentType * dt = _document->DocumentType();
	if (!dt)
		return B_XML_NO_DTD;
	BDocumentTypeDefinition * dtd = dt->InternalSubset();
	
	BEntityDecl * decl = dt->FindEntityDecl(name.String(), true);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	
	// As required by XML 1.0 ss 4.1 [WFC: Parsed Entity]
	if (decl->GetType() == B_XML_UNPARSED_ENTITY)
		return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	
	BVector<BXMLObject *> & set = decl->GetReplacementContent();
	int32 count = set.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BXMLObject		* obj = set.ItemAt(i);
		BElementDecl	* el = dynamic_cast<BElementDecl *>(obj);
		BAttributeDecl	* at = dynamic_cast<BAttributeDecl *>(obj);
		BEntityDecl		* en = dynamic_cast<BEntityDecl *>(obj);
		
		if (el)
		{
			// Element Declaration
			err = dtd->AddElementDecl(el);
			if (err != B_OK)
				return err;
		}
		else if (at)
		{
			// Attribute Declaration
			err = dtd->AddAttributeDecl(at);
			if (err != B_OK)
				return err;
		}
		else if (en)
		{
			// Entity Declaration
			err = dtd->AddEntityDecl(en);
			if (err != B_OK)
				return B_OK;
		}
	}	
	
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLDocumentParseContext::OnParameterEntityRef(		BString	& name,
											BString & replacement		)
{
	BDocumentType * dt = _document->DocumentType();
	if (!dt)
		return B_XML_NO_DTD;
	
	BEntityDecl * decl = dt->FindEntityDecl(name.String(), true);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	
	// As required by XML 1.0 ss 4.1 [WFC: Parsed Entity]
	// Unparsed Entities don't have replacement text (for declarations in DTDs)
	if (decl->GetType() == B_XML_UNPARSED_ENTITY)
		return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	
	replacement = decl->GetReplacementText();
	
	return B_NO_ERROR;
}


}; // namespace XML
}; // namespace B

