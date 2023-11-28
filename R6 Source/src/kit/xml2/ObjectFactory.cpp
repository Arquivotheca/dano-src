
#include <xml2/BParser.h>

namespace B {
namespace XML {

// ====================================================================
BXMLObjectFactory::~BXMLObjectFactory()
{
	
}


// =====================================================================
BNamespace *
BXMLObjectFactory::NamespaceFactory(BString & prefix, BString & value,
									bool getDTD, BEntityStore * store,
									uint32 parseFlags)
{
	BDocumentTypeDefinition * dtd = NULL;
	if (getDTD)
	{
		if (!store)
			return NULL;		// We a store to find the dtd from
		
		if (B_OK != store->GetNamespaceDTD(value, this, parseFlags, &dtd))
			return NULL;
	}
	BNamespace * ns = new BNamespace(prefix, value, true);
	if (dtd)
		ns->SetDTD(dtd);
	return ns;
}


// =====================================================================
BElement *
BXMLObjectFactory::ElementFactory(BString & name, BNamespace * space)
{
	(void) space;
	return new BElement(name, true);
}


// =====================================================================
BAttribute *
BXMLObjectFactory::AttributeFactory(BString & name, const BNamespace * space)
{
	(void) space;
	return new BAttribute(name, true);
}


// =====================================================================
BText *
BXMLObjectFactory::TextFactory()
{
	return new BText();
}


// =====================================================================
BCData *
BXMLObjectFactory::CDataFactory()
{
	return new BCData();
}


// =====================================================================
BComment *
BXMLObjectFactory::CommentFactory()
{
	return new BComment();
}


// =====================================================================
BProcessingInstruction *
BXMLObjectFactory::PIFactory(BString & target)
{
	return new BProcessingInstruction(target, true);
}


// =====================================================================
BDocumentType *
BXMLObjectFactory::DocumentTypeFactory()
{
	return new BDocumentType();
}


// =====================================================================
BDocumentTypeDefinition *
BXMLObjectFactory::DTDFactory(BString & name)
{
	return new BDocumentTypeDefinition(name.String());
}


// =====================================================================
BEntityDecl *
BXMLObjectFactory::EntityDeclFactory(const char * name)
{
	return new BEntityDecl(name);
}


// =====================================================================
BElementDecl *
BXMLObjectFactory::ElementDeclFactory(const char * name)
{
	return new BElementDecl(name);
}


// =====================================================================
BAttributeDecl *
BXMLObjectFactory::AttributeDeclFactory(BString & element, BString & name, uint32 type,
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
	decl->SetElement(element.String());
	return decl;
}



}; // namespace XML
}; // namespace B



