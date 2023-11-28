#include <xml2/BContent.h>
#include <xml2/BEntityParseContext.h>

#include <parsing.h>

namespace B {
namespace XML {


// =====================================================================
// ==         CONSTRUCTOR AND DESTRUCTOR BEGIN HERE !!!               ==
// =====================================================================

// =====================================================================
BEntityDecl::BEntityDecl(const char * name)
	:BXMLObject(B_XML_ENTITY_DECL_OBJECT),
	 BNamed(name)
{
	
}


// =====================================================================
BEntityDecl::~BEntityDecl()
{
	int32 count = _objects.CountItems();
	for (int32 i=0; i<count; i++) {
		delete _objects.ItemAt(i);
	}
	_objects.MakeEmpty();
}



// =====================================================================
// ==       INFO                                                      ==
// =====================================================================

// =====================================================================
void
BEntityDecl::SetType(xml_entity_type type)
{
	_type = type;
}


// =====================================================================
xml_entity_type
BEntityDecl::GetType() const
{
	return _type;
}


// =====================================================================
void
BEntityDecl::SetScope(xml_entity_scope scope)
{
	_scope = scope;
}


// =====================================================================
xml_entity_scope
BEntityDecl::GetScope() const
{
	return _scope;
}


// =====================================================================
void
BEntityDecl::SetStorage(xml_entity_storage storage)
{
	_storage = storage;
}


// =====================================================================
xml_entity_storage
BEntityDecl::GetStorage() const
{
	return _storage;
}



// =====================================================================
// ==       VALUES / REPLACEMENT TEXT                                 ==
// =====================================================================


// =====================================================================
const BString &
BEntityDecl::GetReplacementText()
{
	return _replacementText;
}


// =====================================================================
BVector<BXMLObject *> &
BEntityDecl::GetReplacementContent()
{
	return _objects;
}

const BVector<BXMLObject *> &
BEntityDecl::GetReplacementContent() const
{
	return _objects;
}


// =====================================================================
status_t
BEntityDecl::SetValue(const BString & str, BXMLObjectFactory * factory,
								uint32 parseFlags, BDocumentType * dt,
								BEntityStore * entityStore)
{
	BString copy(str);
	return SetValueAdopt(copy, factory, parseFlags, dt, entityStore);
}


// =====================================================================
status_t
BEntityDecl::SetValueAdopt(BString & str, BXMLObjectFactory * factory,
								uint32 parseFlags, BDocumentType * dt,
								BEntityStore * entityStore)
{
	status_t err;
	
	BXMLEntityParseContext context(this, _scope == B_XML_PARAMETER_ENTITY, dt,
									factory, entityStore);
	BXMLBufferSource source(str.String(), str.Length());
	
	// Parse The Text -- this will expand the general entities
	err = _do_the_parsing_yo_(&source, &context, _scope == B_XML_PARAMETER_ENTITY, parseFlags);
	if (err != B_OK)
		return err;
	
	// Expand all Parameter Entities
	err = context.ExpandEntities(str, '%');
	if (err != B_OK)
		return err;
	
	// Expand all General Entities
	err = context.ExpandEntities(str, '&');
	if (err != B_OK)
		return err;
	
	// Set the replacement text
	_replacementText.Adopt(str);
	
	return B_OK;
}


// =====================================================================
status_t
BEntityDecl::SetValueText(BString & str, BXMLObjectFactory * factory)
{
	BText * text = factory->TextFactory();
	text->SetValue(str);
	AddItem(text);
	_replacementText.Adopt(str);
	return B_OK;
}


// =====================================================================
void
BEntityDecl::AddItem(BXMLObject * object)
{
	_objects.AddItem(object);
}


}; // namespace XML
}; // namespace B

