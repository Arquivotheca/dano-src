
#include <xml2/BParser.h>
#include <xml2/BStringUtils.h>
#include <parsing.h>

namespace B {
namespace XML {

// XXX Maybe this should be exported for everyone to use?
BXMLObjectFactory be_default_xml_factory;

static bool
split_namespace_part(BString & name, BString & space)
{
	int32 pos = name.FindFirst(':');
	if (pos < 0)
		return false;
	space.Adopt(name);
	name.SetTo(space.String()+pos+1);
	space.Truncate(pos);
	return true;
}

// =====================================================================
BXMLDocumentCreationContext::BXMLDocumentCreationContext(bool inDTD,
											BXMLObjectFactory * factory,
											BEntityStore * entityStore)
	:BXMLParseContext(),
	 _currentElement(NULL),
	 _flags(0),
	 _factory(factory),
	 _entityStore(entityStore),
	 _inDTD(inDTD),
	 _deleteEntityStoreWhenDone(false)
{
	if (!factory)
		_factory = &be_default_xml_factory;
	
}


// =====================================================================
BXMLDocumentCreationContext::~BXMLDocumentCreationContext()
{
	if (_deleteEntityStoreWhenDone)
		delete _entityStore;
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnDocumentBegin(uint32 flags)
{
	_flags = flags;
	// We need one of these puppies.
	if (!_entityStore)
	{
		_deleteEntityStoreWhenDone = true;
		_entityStore = new BEntityStore(flags);
	}
	return OnBeginDocumentNode();
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnDocumentEnd()
{
	return OnEndDocumentNode();
}


// =====================================================================
class _AttrToBeRec_
{
public:
	_AttrToBeRec_() {}
	_AttrToBeRec_(BString & s, BString & n, BString & v)
	{
		space.Adopt(s);
		name.Adopt(n);
		value.Adopt(v);
	}

	BString space, name, value;
};

status_t
BXMLDocumentCreationContext::OnStartTag(	BString		& name,
											BValue		& attributes		)
{
	BString nameDummi = name;
	
	status_t err;
	BString space;
	BNamespace * elementNS = NULL;
	BNamespace * newNS;
	BNamedSet newNamespaces;
	void * cookie;
	BVector<_AttrToBeRec_> attrsToBe;
	BValue key, value;
	
	if (_flags & B_XML_HANDLE_NAMESPACES)
	{
		// Split the element name namespace
		split_namespace_part(name, space);
		if (name.Length() < 1)
			return B_XML_NO_EMPTY_NAMES;
		
		// Poke through the attributes, creating namespaces and pulling apart attributes
		cookie = NULL;
		while (B_OK == attributes.GetNextItem(&cookie, &key, &value))
		{
			BString n, v, s;
			n = key.AsString();
			v = value.AsString();
			
			// An xmlns="" means use no namespace
			if (n == "xmlns")
			{
				newNS = _factory->NamespaceFactory(s, v, 0 != (_flags & B_XML_GET_NAMESPACE_DTDS),
														_entityStore, _flags);
				err = newNamespaces.Add(newNS);
				if (err != B_OK)
				{
					newNamespaces.MakeEmpty(true);
					delete newNS;
					return err;
				}
				continue;
			}
			else if (split_namespace_part(n, s))
			{
				if (n.Length() < 1)
				{
					newNamespaces.MakeEmpty(true);
					return B_XML_NO_EMPTY_NAMES;
				}
				
				if (s == "xmlns")
				{
					newNS = _factory->NamespaceFactory(n, v, 0 != (_flags & B_XML_GET_NAMESPACE_DTDS),
														_entityStore, _flags);
					err = newNamespaces.Add(newNS);
					if (err != B_OK)
					{
						newNamespaces.MakeEmpty(true);
						delete newNS;
						return err;
					}
					continue;
				}
			}
			attrsToBe.AddItem(_AttrToBeRec_(s, n, v));
		}
		
		// Look for a namespace for this element
		elementNS = (BNamespace *) newNamespaces.Find(space.String());
		if (!elementNS && _currentElement)
			elementNS = _currentElement->FindNamespaceByPrefix(space.String());
	}
	
	// Create the element
	BElement * newElement = _factory->ElementFactory(name, elementNS);
	
	if (_flags & B_XML_HANDLE_NAMESPACES)
	{
		int32 count;
		
		// Add the namespaces
		count = newNamespaces.CountItems();
		for (int32 i=count-1; i>=0; i--)
		{
			newNS = (BNamespace *) newNamespaces.ItemAt(i);
			newNamespaces.Remove(i);
			err = newElement->AddNamespace(newNS);
			if (err != B_OK)
			{
				newNamespaces.MakeEmpty(true);
				return err;
			}
		}
		
		newNamespaces.MakeEmpty(false);
		
		// Add the attributes
		count = attrsToBe.CountItems();
		for (int32 i=0; i<count; i++)
		{
			const _AttrToBeRec_ & rec = attrsToBe.ItemAt(i);
			BString n = rec.name;
			BAttribute * attr = _factory->AttributeFactory(n, newElement->FindNamespaceByPrefix(rec.space.String()));
			attr->SetValue(rec.value, true);
			err = newElement->AddAttribute(attr);
			if (err != B_OK)
				return err;
		}
	}
	else
	{
		// Add the attributes
		cookie = NULL;
		while (B_OK == attributes.GetNextItem(&cookie, &key, &value))
		{
			BString n, v;
			n = key.AsString();
			v = value.AsString();
			
			BAttribute * attr = _factory->AttributeFactory(n, NULL);
			attr->SetValue(v, true);
			err = newElement->AddAttribute(attr);
			if (err != B_OK)
				return err;
		}
	}
	
	if (_currentElement)	// This is not a top level element
		_currentElement->AddChildLast(newElement);
	
	if (elementNS)
		newElement->SetNamespace(elementNS);
	
	err = OnStartTagNode(newElement);
	if (err != B_OK)
		return err;
	
	// Set _currentElement for next time
	_currentElement = newElement;
	
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnEndTag(		BString		& name				)
{
	BString space;
	bool elementHasNamespace;
	
	if (!_currentElement)
		return B_XML_BAD_PARENT;
	
	// Find Namespace for Element Name
	if (_flags & B_XML_HANDLE_NAMESPACES)
	{
		elementHasNamespace = split_namespace_part(name, space);
		if (elementHasNamespace)
		{
			if (name.Length() < 1)
				return B_XML_NO_EMPTY_NAMES;
			const BNamespace * ns = _currentElement->Namespace();
			if (!ns || space != ns->Name())
				return B_XML_BAD_ELEMENT_NESTING;
		}
	}
	
	if (name != _currentElement->Name())
			return B_XML_BAD_ELEMENT_NESTING;
	
	BElement * oldElement = _currentElement;
	
	// Pop back up the stack, _currentElement can become NULL
	_currentElement = _currentElement->Parent();
	
	return OnEndTagNode(oldElement);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnTextData(	const char	* data,
											int32		size				)
{
	BText * text;
	if (_flags & B_XML_COALESCE_WHITESPACE)
	{
		BString val(data, size);
		MushString(val);
		if (val == "")
			return B_OK;
		text = _factory->TextFactory();
		text->SetValue(val, true);
	}
	else
	{
		text = _factory->TextFactory();
		text->SetValue(data, size);
	}
	
	if (_currentElement)
		_currentElement->AddChildLast(text);
	
	return OnTextNode(text);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnCData(		const char	* data,
										int32		size				)
{
	BCData * cData = _factory->CDataFactory();
	cData->SetValue(data, size);
	
	if (_currentElement)
		_currentElement->AddChildLast(cData);
	
	return OnCDataNode(cData);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnComment(	const char	* data,
										int32		size				)
{
	BComment * comment = _factory->CommentFactory();
	comment->SetValue(data, size);
	
	if (_currentElement)
		_currentElement->AddChildLast(comment);
	
	return OnCommentNode(comment);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnDocumentTypeBegin(BString & name)
{
	(void)name;
	
	BDocumentType * docType;
	
	// The DocumentType Content
	docType = _factory->DocumentTypeFactory();
	
	// If the external ID exists, then fetch the other stuff
	
	return OnBeginDocumentTypeNode(docType);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnExternalSubset(BString & publicID, BString & systemID,
												uint32	parseFlags)
{
	status_t err;
	BString name("<external>");
	BDocumentType * dt = GetDocumentType();
	BDocumentTypeDefinition * dtd = _factory->DTDFactory(name);
	// XXX The parse flags should be passed through
	err = _entityStore->FetchExternalDTD(publicID, systemID, _factory, dt, parseFlags, dtd);
	if (err != B_OK)
	{
		delete dtd;
		return err;
	}
	dtd->SetPublicID(publicID.String());
	dtd->SetSystemID(systemID.String());
	return OnExternalSubsetNode(dtd);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnInternalSubsetBegin()
{
	BDocumentTypeDefinition * dtd;
	
	// Note that this name is not a valid name.  Therefore there
	// will never be a conflict.
	BString name("<internal>");
	dtd = _factory->DTDFactory(name);
	
	_inDTD = true;
	
	return OnBeginInternalSubsetNode(dtd);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnInternalSubsetEnd()
{
	_inDTD = false;
	return OnEndInternalSubsetNode();
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnDocumentTypeEnd()
{
	return OnEndDocumentTypeNode();
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnProcessingInstruction(	BString		& target,
													BString		& data	)
{
	BProcessingInstruction * pi = _factory->PIFactory(target);
	pi->SetValue(data);
	
	if (_currentElement)
		_currentElement->AddChildLast(pi);
	
	return OnProcessingInstructionNode(pi);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnElementDecl(			BString		& name,
											BString		& data			)
{
	status_t err;
	
	if (!_inDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	BElementDecl * decl = _factory->ElementDeclFactory(name.String());
	
	err = decl->SetTo(data);
	if (err != B_OK)
		return err;
	
	return OnElementDeclNode(decl);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnAttributeDecl(	BString		& element,
											BString		& name,
											BString		& type,
											BString		& behavior,
											BString		& defaultVal	)
{
	if (!_inDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	uint32 typeCode;
	xml_attr_behavior behave;
	BString enumVals;
	
	// Choose typeCode
	if (type.ByteAt(0) == '(' && type.ByteAt(type.Length()-1) == ')')
	{
		type.CopyInto(enumVals, 1, type.Length()-2);
		typeCode = B_ENUM_TYPE;
	}
	else if (type == "CDATA")
		typeCode = B_STRING_TYPE;
	else if (type == "ID")
		typeCode = B_ID_TYPE;
	else if (type == "IDREF")
		typeCode = B_IDREF_TYPE;
	else if (type == "IDREFS")
		typeCode = B_IDREFS_TYPE;
	else if (type == "ENTITY")
		typeCode = B_ENTITY_TYPE;
	else if (type == "ENTITIES")
		typeCode = B_ENTITIES_TYPE;
	else if (type == "NMTOKEN")
		typeCode = B_NMTOKEN_TYPE;
	else if (type == "NMTOKENS")
		typeCode = B_NMTOKENS_TYPE;
	else
		return B_XML_INVALID_ATTR_TYPE;
	
	// Choose behave
	if (behavior == "" && defaultVal == "")
		behave = B_XML_ATTRIBUTE_OPTIONAL;
	else if (behavior == "")
		behave = B_XML_ATTRIBUTE_DEFAULT;
	else if (behavior == "REQUIRED")
		behave = B_XML_ATTRIBUTE_REQUIRED;
	else if (behavior == "IMPLIED")
		behave = B_XML_ATTRIBUTE_IMPLIED;
	else if (behavior == "FIXED")
		behave = B_XML_ATTRIBUTE_FIXED;
	else
		return B_XML_BAD_ATTR_BEHAVIOR;
	
	// Create the Attribute Decl
	BAttributeDecl * decl = _factory->AttributeDeclFactory(element, name, typeCode, enumVals, behave, defaultVal);
	
	return OnAttributeDeclNode(decl);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnInternalParsedEntityDecl(	BString	& name,
												BString & internalData,
												bool	parameter,
												uint32	parseFlags)
{
	BDocumentType * dt = GetDocumentType();
	status_t err;
	
	if (!_inDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	BEntityDecl * entity = _factory->EntityDeclFactory(name.String());
	entity->SetType(B_XML_PARSED_ENTITY);
	entity->SetScope(parameter ? B_XML_PARAMETER_ENTITY : B_XML_GENERAL_ENTITY);
	entity->SetStorage(B_XML_INTERNAL_ENTITY);
	
	err = entity->SetValueAdopt(internalData, _factory, parseFlags, dt, _entityStore);
	if (err != B_OK)
		return err;
	
	return OnEntityDeclNode(entity);
}


// =====================================================================
// External entity declarations are fetched and included int the
// internal DTD as if they had been typed directly into the DTD.
status_t
BXMLDocumentCreationContext::OnExternalParsedEntityDecl(	BString	& name,
												BString & publicID,
												BString & systemID,
												bool	 parameter,
												uint32	parseFlags)
{
	BDocumentType * dt = GetDocumentType();
	status_t err;
	
	if (!_inDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	BEntityDecl * entity;
	err = _entityStore->FetchEntity(publicID, systemID, &entity, parameter, _factory, parseFlags, dt);
	if (err != B_OK || !entity)
		return B_XML_ENTITY_NOT_FOUND;
	
	entity->SetName(name.String());
	entity->SetType(B_XML_PARSED_ENTITY);
	entity->SetScope(parameter ? B_XML_PARAMETER_ENTITY : B_XML_GENERAL_ENTITY);
	entity->SetStorage(B_XML_EXTERNAL_ENTITY);
	
	return OnEntityDeclNode(entity);
}


// =====================================================================
// Here we deviate from the spec a bit, mostly because unparsed entities
// aren't all that useful as far as I can tell.  (or at least won't be
// used much probably).  The spec says that we must "report" them. Instead
// we're going to create the entitiy like we're supposed to, but set the 
// replacement text equal to the external ID string
// otherwise. How do you "report it to the application?!?"  It seems like
// they wrote the spec without thinking about how it would actually be
// used.  SGML SUCKS!!!!!
status_t
BXMLDocumentCreationContext::OnUnparsedEntityDecl(		BString	& name,
											BString & publicID,
											BString & systemID,
											BString & notation			)
{
	(void) notation;
	
	status_t err;
	
	if (!_inDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	BEntityDecl * entity = _factory->EntityDeclFactory(name.String());
	entity->SetType(B_XML_UNPARSED_ENTITY);
	entity->SetScope(B_XML_GENERAL_ENTITY);
	entity->SetStorage(B_XML_EXTERNAL_ENTITY);
	
	// The deviation starts here
	BString val;
	if (publicID != "")
	{
		val = "PUBLIC \"";
		val += publicID;
		val += "\" \"";
	}
	else
	{
		val = "SYSTEM \"";
	}
	val += systemID;
	val += "\"";
	
	err = entity->SetValueText(val, _factory);
	if (err != B_OK)
		return err;
	
	return OnEntityDeclNode(entity);
}


// =====================================================================
status_t
BXMLDocumentCreationContext::OnNotationDecl(			BString	& name,
											BString	& value				)
{
	(void)name;
	(void)value;
	
	// XXX nothing yet, because we don't have any data structures for notation
	// BECAUSE THEY ARE USELESS!!!
	return B_NO_ERROR;
	// return OnNotationDeclNode(decl);
}

}; // namespace XML
}; // namespace B

