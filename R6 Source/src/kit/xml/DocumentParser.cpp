
#include <XMLParser.h>
#include <stdio.h>
#include <ctype.h>
#include <EntityParser.h>
#include <XMLDTD.h>

#include "Map.cpp"

namespace BXmlKit {

template class BMap<BString, BXMLDocumentParser::unparsed_entity_record>;
template class BMap<BString, BXMLObjectSet *>;

// Set to 1 for printfs
#define DEBUG_ELEMENT_DECL	1
#define DEBUG_ATTLIST_DECL	1
#define DEBUG_ENTITY_DECL	1

// =====================================================================
BXMLDocumentParser::BXMLDocumentParser(uint32 flags)
	:BXMLParseTools(flags),
	 _document(NULL),
	 _currentElement(NULL),
	 _entityParser(NULL),
	 _internalDTD(NULL),
	 _entityStore(NULL),
	 _inInternalSubset(false),
	 _deleteEntityStoreOnDtor(false)
{
	// If there is no entity store, create one
	if (!_entityStore)
	{
		_deleteEntityStoreOnDtor = true;
		_entityStore = new BEntityStore;
	}
}


// =====================================================================
BXMLDocumentParser::~BXMLDocumentParser()
{
	if (_deleteEntityStoreOnDtor)
		delete _entityStore;
}


// =====================================================================
status_t
BXMLDocumentParser::Parse(BDocument * document)
{
	status_t err;
	_document = document;
	err = BXMLParseTools::Parse();
	delete _entityParser;
	_entityParser = NULL;
	return err;
}


// =====================================================================
status_t
BXMLDocumentParser::Parse(const void * data, BDocument * document, int32 len)
{
	status_t err;
	_document = document;
	err = BXMLParseTools::Parse(data, len);
	delete _entityParser;
	_entityParser = NULL;
	return err;
}


// =====================================================================
status_t
BXMLDocumentParser::Parse(BDataIO * data, BDocument * document)
{
	status_t err;
	_document = document;
	err = BXMLParseTools::Parse(data);
	delete _entityParser;
	_entityParser = NULL;
	return err;
}


// =====================================================================
status_t
BXMLDocumentParser::ParseDTD(BDocument * document)
{
	status_t err;
	_document = document;
	err = BXMLParseTools::ParseDTD();
	delete _entityParser;				// XXX I don't understand this line. joeo.
	_entityParser = NULL;
	return err;
}


// =====================================================================
status_t
BXMLDocumentParser::ParseDTD(const void * data, BDocument * document, int32 len)
{
	status_t err;
	_document = document;
	err = BXMLParseTools::ParseDTD(data, len);
	delete _entityParser;
	_entityParser = NULL;
	return err;
}


// =====================================================================
status_t
BXMLDocumentParser::ParseDTD(BDataIO * data, BDocument * document)
{
	status_t err;
	_document = document;
	err = BXMLParseTools::ParseDTD(data);
	delete _entityParser;
	_entityParser = NULL;
	return err;
}


// =====================================================================
status_t
BXMLDocumentParser::OnDocumentBegin()
{
	// Do initialization for parsing of ONE document
	_currentElement = NULL;
	return OnDocumentBegin(_document);
}


// =====================================================================
status_t
BXMLDocumentParser::OnDocumentEnd()
{
	_currentElement = NULL;
	return OnDocumentEnd(_document);
}


// =====================================================================
status_t
BXMLDocumentParser::OnStartTag(	BString					& name,
								BStringMap				& attributes		)
{
	// Create the element
	BElement * newElement = ElementFactory(name);
	
	// Add the attributes
	int32 count = attributes.CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BString * n, * v;
		attributes.PairAt(i, &n, &v);
		BAttribute * attr = AttributeFactory(*n);
		
		attr->SetValue(*v, true);
		
		newElement->AddAttribute(attr);
	}
	
	if (!_currentElement)
	{
		// This is the document element
		if (_document)
			_document->AddChildLast(newElement);
	}
	else
		// This is not the first element
		_currentElement->AddChildLast(newElement);
	
	_currentElement = newElement;
	
	return OnElementBegin(newElement);
}


// =====================================================================
status_t
BXMLDocumentParser::OnEndTag(	BString					& name				)
{
	BElement * e = _currentElement;
	
	if (!_currentElement)
		return B_XML_BAD_PARENT;
	
	if (name != _currentElement->Name())
		return B_XML_BAD_ELEMENT_NESTING;
	
	// Pop back up the stack
	_currentElement = _currentElement->Parent();
	
	return OnElementEnd(e);
}


// =====================================================================
status_t
BXMLDocumentParser::OnTextData(	const char				* data				)
{
	BText * text = TextFactory();
	text->SetValue(data);
	
	if (!_currentElement)
	{
		if (_document)
			_document->AddChildLast(text);
	}
	else
		_currentElement->AddChildLast(text);
	
	return OnTextNode(text);
}


// =====================================================================
status_t
BXMLDocumentParser::OnCData(	const char				* data				)
{
	BCData * cData = CDataFactory(data);
	
	if (!_currentElement)
	{
		if (_document)
			_document->AddChildLast(cData);
	}
	else
		_currentElement->AddChildLast(cData);
	
	return OnCDataNode(cData);
}


// =====================================================================
status_t
BXMLDocumentParser::OnComment(	const char				* data				)
{
	BComment * comment = CommentFactory(data);
	
	if (!_currentElement)
	{
		if (_document)
			_document->AddChildLast(comment);
	}
	else
		_currentElement->AddChildLast(comment);

	return OnCommentNode(comment);
}


// =====================================================================
status_t
BXMLDocumentParser::OnDocumentTypeBegin(	BString		& name,
											BString		& externalID		)
{
	status_t err;
	
	// The DocumentType Content
	_documentType = DocumentTypeFactory();
	
	err = _document->AddChildLast(_documentType);
	if (err != B_OK)
		return err;
	
	if (!dynamic_cast<BXMLEntityParser *>(this))
		_entityParser = new BXMLEntityParser(_flags, this,+ _documentType);
	
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnBeginInternalSubset()
{
	// Note that this name is not a valid name.  Therefore there
	// will never be a conflict.
	_internalDTD = DTDFactory("<internal>");
	_inInternalSubset = true;
	_documentType->SetInternalSubset(_internalDTD);
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnEndInternalSubset()
{
	_inInternalSubset = false;
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnDocumentTypeEnd()
{
	// XXX nothing yet
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnProcessingInstruction(BString		& target,
											BString		& data				)
{
	BProcessingInstruction * pi = PIFactory(target);
	pi->SetValue(data);
	
	if (!_currentElement)
	{
		if (_document)
			_document->AddChildLast(pi);
	}
	else
		_currentElement->AddChildLast(pi);
	
	return OnProcessingInstructionNode(pi);
}


// =====================================================================
status_t
BXMLDocumentParser::OnElementDecl(	BString				& name,
									BString				& data				)
{
	status_t err;
	
	if (!_inInternalSubset || !_internalDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	BElementDecl * decl = ElementDeclFactory(name.String());
	
	err = decl->SetTo(data);
	if (err != B_OK)
		return err;
	
	err = _internalDTD->AddElementDecl(decl);
	if (err != B_OK)
		return err;
	
	return OnElementDecl(decl);
}


// =====================================================================
status_t
BXMLDocumentParser::OnAttributeDecl(		BString		& element,
											BString		& name,
											BString		& type,
											BString		& behavior,
											BString		& defaultVal		)
{
	status_t err;
	
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
	
	BAttributeDecl * decl = AttributeDeclFactory(name, typeCode, enumVals, behave, defaultVal);
	
	// Add the Attribute Decl to the Element Decl
	err = _internalDTD->AddAttribtueDecl(element, decl);
	if (err != B_OK)
		return err;
	
	return OnAttributeDecl(element, decl);
}


// =====================================================================
status_t
BXMLDocumentParser::OnInternalParsedEntityDecl(	BString	& name,
												BString & internalData,
												bool parameter				)
{
	status_t err;
	
	if (!_inInternalSubset || !_internalDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	BEntityDecl * entity = EntityDeclFactory(name.String());
	entity->SetType(B_XML_PARSED_ENTITY);
	entity->SetScope(parameter ? B_XML_PARAMETER_ENTITY : B_XML_GENERAL_ENTITY);
	entity->SetStorage(B_XML_INTERNAL_ENTITY);
	
	err = entity->SetValueAdopt(internalData, _entityParser);
	if (err != B_OK)
		return err;
	
	err = _internalDTD->AddEntityDecl(entity);
	if (err != B_OK)
		return err;
	
	return OnInternalParsedEntityDecl(entity);
	
}


// =====================================================================
status_t
BXMLDocumentParser::OnExternalParsedEntityDecl(	BString	& name,
											BString & publicID,
											BString & systemID,
											bool parameter				)
{
	if (!_inInternalSubset || !_internalDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	status_t err;
	BEntityDecl * entity;
	err = _entityStore->Fetch(publicID, systemID, &entity, _entityParser);
	if (err != B_OK || !entity)
		return err;
	entity->SetName(name.String());
	entity->SetType(B_XML_PARSED_ENTITY);
	entity->SetScope(parameter ? B_XML_PARAMETER_ENTITY : B_XML_PARAMETER_ENTITY);
	entity->SetStorage(B_XML_EXTERNAL_ENTITY);

	err = _internalDTD->AddEntityDecl(entity);
	if (err != B_OK)
		return err;
	
	return OnExternalParsedEntityDecl(entity);
 
}


// =====================================================================
status_t
BXMLDocumentParser::OnUnparsedEntityDecl(		BString	& name,
											BString & publicID,
											BString & systemID,
											BString & notation			)
{
	if (!_inInternalSubset || !_internalDTD)
		return B_XML_DECLARATION_NOT_IN_DTD;
	
	status_t err;
	BEntityDecl * entity = EntityDeclFactory(name.String());
	entity->SetType(B_XML_UNPARSED_ENTITY);
	entity->SetScope(B_XML_GENERAL_ENTITY);
	entity->SetStorage(B_XML_EXTERNAL_ENTITY);
	
	err = entity->SetValueAdopt(publicID != "" ? publicID : systemID, _entityParser);
	if (err != B_OK)
		return err;
	
	return OnUnparsedEntityDecl(entity);
}


// =====================================================================
status_t
BXMLDocumentParser::OnNotationDecl(	BString				& name,
									BString				& value				)
{
	// XXX nothing yet
	return OnNotationDecl(NULL);
}



// =====================================================================
status_t
BXMLDocumentParser::OnParseError(status_t err								)
{
	return err;
}

// =====================================================================
status_t
BXMLDocumentParser::OnGeneralParsedEntityRef(	BString	& name				)
{
	status_t err;
	
	if (!_documentType)
		return B_XML_NO_DTD;
	
	BEntityDecl * decl = _documentType->FindEntityDecl(name.String(), false);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	if (decl->GetType() == B_XML_UNPARSED_ENTITY)
		return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	
	BXMLObjectSet * set = decl->GetReplacementContent();
	
	int32 count = set->CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BXMLObject * obj = set->ItemAt(i);
		BContent * c = dynamic_cast<BContent *>(obj);
		if (c)
			_currentElement->AddChildLast(c);
		err = DispatchXMLObject(obj);
		if (err != B_OK)
			return err;
	}
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnGeneralParsedEntityRef(	BString	& name,
												BString & replacement	)
{
	if (!_documentType)
		return B_XML_NO_DTD;
	
	BEntityDecl * decl = _documentType->FindEntityDecl(name.String(), false);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	if (decl->GetType() == B_XML_UNPARSED_ENTITY)
		return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	
	replacement = decl->GetReplacementText();
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnParameterEntityRef(		BString	& name,
												BString & replacement	)
{
	if (!_documentType)
		return B_XML_NO_DTD;
	
	BEntityDecl * decl = _documentType->FindEntityDecl(name.String(), true);
	if (!decl)
		return B_XML_ENTITY_NOT_FOUND;
	if (decl->GetType() == B_XML_UNPARSED_ENTITY)
		return B_XML_ILLEGAL_UNPARSED_ENTITY_REF;
	
	replacement = decl->GetReplacementText();
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnParameterEntityRef(		BString	& name				)
{
	OnTextData("|+++|");
	return B_OK;
}




// =====================================================================
// ==        FACTORY METHODS                                          ==
// =====================================================================


// =====================================================================
BElement *
BXMLDocumentParser::ElementFactory(BString & name)
{
	return new BElement(name, true);
}


// =====================================================================
BAttribute *
BXMLDocumentParser::AttributeFactory(BString & name)
{
	return new BAttribute(name, true);
}


// =====================================================================
BText *
BXMLDocumentParser::TextFactory()
{
	return new BText();
}


// =====================================================================
BCData *
BXMLDocumentParser::CDataFactory(const char * value)
{
	return new BCData(value);
}


// =====================================================================
BComment *
BXMLDocumentParser::CommentFactory(const char * value)
{
	return new BComment(value);
}


// =====================================================================
BProcessingInstruction *
BXMLDocumentParser::PIFactory(BString & target)
{
	return new BProcessingInstruction(target, true);
}


// =====================================================================
BDocumentType *
BXMLDocumentParser::DocumentTypeFactory()
{
	return new BDocumentType();
}


// =====================================================================
BDocumentTypeDefinition *
BXMLDocumentParser::DTDFactory(const char * name)
{
	return new BDocumentTypeDefinition(name);
}


// =====================================================================
BEntityDecl *
BXMLDocumentParser::EntityDeclFactory(const char * name)
{
	return new BEntityDecl(name);
}


// =====================================================================
BElementDecl *
BXMLDocumentParser::ElementDeclFactory(const char * name)
{
	return new BElementDecl(name);
}


// =====================================================================
BAttributeDecl *
BXMLDocumentParser::AttributeDeclFactory(BString & name, uint32 type,
											BString & enumVals, xml_attr_behavior behave,
											BString & defaultVal)
{
	BAttributeDecl * decl;
	decl = new BAttributeDecl(name.String());
	decl->SetType(type);
	decl->SetBehavior(behave);
	if (enumVals != "")
		decl->SetEnumValues(enumVals);
	decl->SetDefault(defaultVal.String());
	return decl;
}


// =====================================================================
// c means child, cE means child Element, cT means child Text, etc.
status_t
BXMLDocumentParser::DispatchXMLObject(BXMLObject * obj)
{
	status_t err;
	
	BElement * e = dynamic_cast<BElement *>(obj);
	if (e)
	{
		err = OnElementBegin(e);
		if (err != B_OK)
			return err;
		
		BContent * c = e->FirstChild();
		while (c)
		{
			DispatchXMLObject(c);
			c = c->NextSibling();
		}
		
		err = OnElementEnd(e);
		if (err != B_OK)
			return err;
		
		return B_OK;
	}
	
	BText * t = dynamic_cast<BText *>(obj);
	if (t)
	{
		return OnTextNode(t);
	}
	
	BCData * d = dynamic_cast<BCData *>(obj);
	if (d)
	{
		return OnCDataNode(d);
	}
	
	BComment * m = dynamic_cast<BComment *>(obj);
	if (m)
	{
		return OnCommentNode(m);
	}
	
	BProcessingInstruction * p = dynamic_cast<BProcessingInstruction *>(obj);
	if (p)
	{
		return OnProcessingInstructionNode(p);
	}
	
	return B_XML_PARSE_ERROR;
}


// =====================================================================
// ==        HOOK FUNCTION DEFAULT IMPLEMENTATIONS                    ==
// =====================================================================


// =====================================================================
status_t
BXMLDocumentParser::OnDocumentBegin(BDocument * doc)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnDocumentEnd(BDocument * doc)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnElementBegin(BElement * element)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnElementEnd(BElement * element)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnTextNode(BText * text)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnCDataNode(BCData * cData)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnCommentNode(BComment * comment)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnDocumentTypeNode(BDocumentType * docType)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnProcessingInstructionNode(BProcessingInstruction * pi)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnInternalParsedEntityDecl(BEntityDecl * decl)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnExternalParsedEntityDecl(BEntityDecl * decl)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnUnparsedEntityDecl(BEntityDecl * decl)
{
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnNotationDecl(BEntityDecl * decl)
{
	return B_OK;
}


// =====================================================================
#if DEBUG_ELEMENT_DECL
static void
PrintAllowedList(BElementDecl::List * list, int32 level)
{
	int32 count = list->CountItems();
	for (int32 i=0; i<count; ++i)
	{
		BElementDecl::ListNode * node = list->NodeAt(i);
		if (node->type == BElementDecl::ListNode::STRING)
		{
			for (int32 t=0; t<level; t++)
				printf("\t");
			if (node->cardinality == BElementDecl::ONE)
				printf("  ");
			else if (node->cardinality == BElementDecl::ONE_OR_MORE)
				printf("+ ");
			else if (node->cardinality == BElementDecl::ZERO_OR_MORE)
				printf("* ");
			else if (node->cardinality == BElementDecl::ONE_OR_ZERO)
				printf("? ");
			printf("\"%s\"\n", node->element.String());
		}
		else
		{
			for (int32 t=0; t<level+1; t++)
				printf("\t");
			if (node->cardinality == BElementDecl::ONE)
				printf("  ");
			else if (node->cardinality == BElementDecl::ONE_OR_MORE)
				printf("+ ");
			else if (node->cardinality == BElementDecl::ZERO_OR_MORE)
				printf("* ");
			else if (node->cardinality == BElementDecl::ONE_OR_ZERO)
				printf("? ");
			if (node->list->type == BElementDecl::List::SEQ)
				printf("SEQ \n");
			else if (node->list->type == BElementDecl::List::CHOICE)
				printf("CHOICE \n");
			PrintAllowedList(node->list, level+1);
		}
	}
	
}
#endif

status_t
BXMLDocumentParser::OnElementDecl(BElementDecl * decl)
{
#if DEBUG_ELEMENT_DECL
	if (decl->GetCardinality() == BElementDecl::ONE)
		printf("  ");
	else if (decl->GetCardinality() == BElementDecl::ONE_OR_MORE)
		printf("+ ");
	else if (decl->GetCardinality() == BElementDecl::ZERO_OR_MORE)
		printf("* ");
	else if (decl->GetCardinality() == BElementDecl::ONE_OR_ZERO)
		printf("? ");
	if (decl->GetPattern()->type == BElementDecl::List::SEQ)
		printf("SEQ \n");
	else if (decl->GetPattern() ->type == BElementDecl::List::CHOICE)
		printf("CHOICE \n");
	PrintAllowedList(decl->GetPattern(), 0);
#endif
	
	return B_OK;
}


// =====================================================================
status_t
BXMLDocumentParser::OnAttributeDecl(const BString & element, BAttributeDecl * decl)
{
#if DEBUG_ATTLIST_DECL
{
	printf("Element: \"%s\"  Attribute \"%s\"\n", element.String(), decl->Name());
	printf("\tTypeCode: %d  behave: %d\n", (int) decl->GetType(), (int) decl->GetBehavior());
	int32 count = decl->CountEnumValues();
	for (int32 i=0; i<count; ++i)
		printf("\tEnum: \"%s\"\n", decl->GetEnumValue(i));
}
#endif

	return B_OK;
}


}; //namespace BXmlKit
