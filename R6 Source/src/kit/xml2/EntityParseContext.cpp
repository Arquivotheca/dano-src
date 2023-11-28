


#include <xml2/BEntityParseContext.h>

#include <parsing.h>


namespace B {
namespace XML {

// =====================================================================
BXMLEntityParseContext::BXMLEntityParseContext(BEntityDecl * decl,
										bool parameterEntity,
										BDocumentType * dt,
										BXMLObjectFactory * factory,
										BEntityStore * entityStore,
										BDocumentTypeDefinition * dtd)
	:BXMLDocumentCreationContext(parameterEntity, factory, entityStore),
	 _parameterEntity(parameterEntity),
	 _decl(decl),
	 _dt(dt),
	 _deleteDTDObjectsWhenDone(false),
	 _dtdObjects(dtd)
{
	if (!_dtdObjects)
	{
		_deleteDTDObjectsWhenDone = true;
		_dtdObjects = new BDocumentTypeDefinition("<private>");
	}
}


// =====================================================================
BXMLEntityParseContext::~BXMLEntityParseContext()
{
	if (_deleteDTDObjectsWhenDone)
		delete _dtdObjects;
}



// =====================================================================
status_t
BXMLEntityParseContext::OnBeginDocumentNode()
{
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnEndDocumentNode()
{
	return B_NO_ERROR;
}


// =====================================================================
// Note that PIs, comments, and elements are the only thing that can
// be added to the document root
status_t
BXMLEntityParseContext::OnStartTagNode(BElement * element)
{
	if (_decl && !_parameterEntity && !element->Parent())
		_decl->AddItem(element);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnEndTagNode(BElement * element)
{
	(void) element;
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnTextNode(BText * text)
{
	if (_decl && !_parameterEntity && !text->Parent())
		_decl->AddItem(text);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnCDataNode(BCData * cData)
{
	if (_decl && !_parameterEntity && !cData->Parent())
		_decl->AddItem(cData);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnCommentNode(BComment * comment)
{
	if (_decl && !comment->Parent())
		_decl->AddItem(comment);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnBeginDocumentTypeNode(BDocumentType * dt)
{
	(void) dt;
	// Entities are either parameter or general, but can't do stuff like this
	return B_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnExternalSubsetNode(BDocumentTypeDefinition * dtd)
{
	(void) dtd;
	// Entities are either parameter or general, but can't do stuff like this
	return B_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnBeginInternalSubsetNode(BDocumentTypeDefinition * dtd)
{
	(void) dtd;
	// Entities are either parameter or general, but can't do stuff like this
	return B_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnEndInternalSubsetNode()
{
	// Entities are either parameter or general, but can't do stuff like this
	return B_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnEndDocumentTypeNode()
{
	// Entities are either parameter or general, but can't do stuff like this
	return B_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnProcessingInstructionNode(BProcessingInstruction * pi)
{
	if (_decl && !pi->Parent())
		_decl->AddItem(pi);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnElementDeclNode(BElementDecl * decl)
{
	if (_decl && _parameterEntity)
		_decl->AddItem(decl);
	_dtdObjects->AddElementDecl(decl);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnAttributeDeclNode(BAttributeDecl * decl)
{
	if (_decl && _parameterEntity)
		_decl->AddItem(decl);
	_dtdObjects->AddAttributeDecl(decl);
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnEntityDeclNode(BEntityDecl * decl)
{
	if (_decl && _parameterEntity)
		_decl->AddItem(decl);
	_dtdObjects->AddEntityDecl(decl);
	return B_NO_ERROR;
}


// XXX Not implemented
// =====================================================================
// status_t
// BXMLEntityParseContext::OnNotationDeclNode(BNotationDecl * decl)
// {
// 	return B_NO_ERROR;
// }



// =====================================================================
BDocumentType *
BXMLEntityParseContext::GetDocumentType()
{
	return _dt;
}




// =====================================================================
// For the Entity Reference Functions:
// =====================================================================
// The behavior here might not be right w.r.t. whether this is a parameter
// or a general entity.
// =====================================================================

// =====================================================================
status_t
BXMLEntityParseContext::OnGeneralParsedEntityRef(	BString	& name				)
{
	// Try to find it declared in _dtdObjects, then try to find it in
	// _dt.  Note that _dt is not required.
	BEntityDecl * decl = _dtdObjects->FindEntityDecl(name.String(), false);
	if (!decl && _dt)
		decl = _dt->FindEntityDecl(name.String(), false);
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
			else if (_decl)
				_decl->AddItem(c);	// I don't think this is well-formed, but whatever.
		}
	}
	
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnGeneralParsedEntityRef(	BString	& name,
											BString & replacement		)
{
	BEntityDecl * decl = _dtdObjects->FindEntityDecl(name.String(), false);
	if (!decl && _dt)
		decl = _dt->FindEntityDecl(name.String(), false);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	
	// As required by XML 1.0 ss 4.1 [WFC: Parsed Entity]
	//if (decl->GetType() == B_XML_UNPARSED_ENTITY)
	//	return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	// Not here because this is for in attributes
	
	if (_decl)
		replacement = decl->GetReplacementText();
	
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnParameterEntityRef(		BString	& name				)
{
	status_t err;
	BEntityDecl * decl = _dtdObjects->FindEntityDecl(name.String(), true);
	if (!decl && _dt)
		decl = _dt->FindEntityDecl(name.String(), false);
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
			err = OnElementDeclNode(el);
			if (err != B_OK)
				return err;
		}
		else if (at)
		{
			// Attribute Declaration
			err = OnAttributeDeclNode(at);
			if (err != B_OK)
				return err;
		}
		else if (en)
		{
			// Entity Declaration
			err = OnEntityDeclNode(en);
			if (err != B_OK)
				return B_OK;
		}
	}	
	
	return B_NO_ERROR;
}


// =====================================================================
status_t
BXMLEntityParseContext::OnParameterEntityRef(		BString	& name,
											BString & replacement		)
{
	BEntityDecl * decl = _dtdObjects->FindEntityDecl(name.String(), true);
	if (!decl && _dt)
		decl = _dt->FindEntityDecl(name.String(), false);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	
	// As required by XML 1.0 ss 4.1 [WFC: Parsed Entity]
	// Unparsed Entities don't have replacement text (for declarations in DTDs)
	if (decl->GetType() == B_XML_UNPARSED_ENTITY)
		return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	
	if (_decl)
		replacement = decl->GetReplacementText();
	
	return B_NO_ERROR;
}

// =====================================================================
status_t
BXMLEntityParseContext::ExpandEntities(BString & str, const char delim)
{
	status_t err;
	
	BString entity;
	BString entityVal;
	BString newValue("");
	
	int32 offset = 0, oldOffset = 0;
	int32 end = -1;
	
	while (true)
	{
		oldOffset = end + 1;
		offset = str.FindFirst(delim, oldOffset);
		if (offset < 0)
			break;
		end = str.FindFirst(';', offset);
		if (end < 0)
			break;
		
		newValue.Append(str.String() + oldOffset, offset-oldOffset);
		str.CopyInto(entity, offset+1, end-offset-1);
		
		if (delim == '%')
			err = OnParameterEntityRef(entity, entityVal);
		else
			err = OnGeneralParsedEntityRef(entity, entityVal);
		if (err != B_OK)
			return err;
		
		newValue.Append(entityVal);
	}
	
	newValue.Append(str.String() + oldOffset, str.Length()-oldOffset);
	str.Adopt(newValue);
	return B_OK;
}


}; // namespace XML
}; // namespace B

