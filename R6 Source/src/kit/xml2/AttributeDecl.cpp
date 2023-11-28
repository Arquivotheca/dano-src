#include <xml2/BContent.h>

#include <stdio.h>

namespace B {
namespace XML {


// From XML-2.3
#define IS_WHITESPACE(x) ((x)==0x20||(x)==0x09||(x)==0x0d||(x)==0x0a)

// =====================================================================
//     CONSTRUCTOR / DESTRUCTOR / ETC
// =====================================================================

// =====================================================================
BAttributeDecl::BAttributeDecl(const char * name)
	:BXMLObject(B_XML_ATTRIBUTE_DECL_OBJECT),
	 BNamed(name),
	 _element(""),
	 _typeCode(B_STRING_TYPE),
	 _enumVals(),
	 _defaultVal(""),
	 _behavior(B_XML_ATTRIBUTE_OPTIONAL)
{
	
}


// =====================================================================
BAttributeDecl::BAttributeDecl(const BString & name)
	:BXMLObject(B_XML_ATTRIBUTE_DECL_OBJECT),
	 BNamed(name),
	 _element(""),
	 _typeCode(B_STRING_TYPE),
	 _enumVals(),
	 _defaultVal(""),
	 _behavior(B_XML_ATTRIBUTE_OPTIONAL)
{
	
}


// =====================================================================
BAttributeDecl::BAttributeDecl(const BAttributeDecl & copy)
	:BXMLObject(B_XML_ATTRIBUTE_DECL_OBJECT),
	 BNamed(copy.Name()),
	 _element(""),
	 _typeCode(copy._typeCode),
	 _enumVals(copy._enumVals),
	 _defaultVal(copy._defaultVal),
	 _behavior(copy._behavior)
{
	
}


// =====================================================================
BAttributeDecl::~BAttributeDecl()
{
	// nothing to do
}


// =====================================================================
BAttributeDecl &
BAttributeDecl::operator=(const BAttributeDecl & copy)
{
	SetName(copy.Name());
	_typeCode = copy._typeCode;
	_enumVals = copy._enumVals;
	_defaultVal = copy._defaultVal;
	_behavior = copy._behavior;
	return *this;
}


// =====================================================================
//     TYPE CODE (B_STRING_TYPE, B_ENUM_TYPE, etc.)
// =====================================================================

// =====================================================================
void
BAttributeDecl::SetType(uint32 type)
{
	// XXX. Right now, limit it, and silently convert everything
	// else to B_STRING_TYPE.  This might want to be changed
	
	if (type == B_ENUM_TYPE || type == B_NOTATION_TYPE ||
			type == B_ID_TYPE || type == B_IDREF_TYPE ||
			type == B_IDREFS_TYPE || type == B_ENTITY_TYPE ||
			type == B_ENTITIES_TYPE || type == B_NMTOKEN_TYPE ||
			type == B_NMTOKENS_TYPE)
		_typeCode = type;
	else
		_typeCode = B_STRING_TYPE;
}


// =====================================================================
uint32
BAttributeDecl::GetType() const
{
	return _typeCode;
}



// =====================================================================
//     BEHAVIOR
// =====================================================================

// =====================================================================
void
BAttributeDecl::SetBehavior(xml_attr_behavior behavior)
{
	_behavior = behavior;
}


// =====================================================================
xml_attr_behavior
BAttributeDecl::GetBehavior() const
{
	return _behavior;
}



// =====================================================================
//     ENUMERATION FUNCTIONS
// =====================================================================

// =====================================================================
status_t
BAttributeDecl::AddEnumValue(const char * val)
{
	BString str(val);
	_enumVals.AddItem(str);
	return B_OK;
}


// =====================================================================
status_t
BAttributeDecl::RemoveEnumValue(const char * val)
{
	return _enumVals.RemoveItemFor(BString(val));
}


// =====================================================================
int32
BAttributeDecl::CountEnumValues() const
{
	return _enumVals.CountItems();
}


// =====================================================================
const char *
BAttributeDecl::GetEnumValue(int32 index) const
{
	return _enumVals.ItemAt(index).String();
}


// =====================================================================
bool
BAttributeDecl::IsEnumValue(const char * val) const
{
	size_t index;
	return _enumVals.GetIndexOf(BString(val), &index);
}


// =====================================================================
status_t
BAttributeDecl::SetEnumValues(BString & str)
{
	status_t err;
	char * p = str.LockBuffer(str.Length());
	int32 count = 1;	// Don't skip the last one
	char * s = p;
	while (*p)
	{
		if (*p == '|')
		{
			count++;
			*p = '\0';
		}
		p++;
	}
	
	p = s;
	for (int32 i=0; i<count; ++i)
	{
		s = p;
		while (*p)
			p++;
		err = AddEnumValue(s);
		if (err != B_OK)
			goto ERR;
		p++;
	}
	
	err = B_OK;
ERR:
	str.UnlockBuffer(0);
	return err;
}



// =====================================================================
//     DEFAULT VALUE FUNCTIONS
// =====================================================================

// =====================================================================
void
BAttributeDecl::SetDefault(const char * value)
{
	_defaultVal = value;
}


// =====================================================================
const BString &
BAttributeDecl::GetDefault() const
{
	return _defaultVal;
}



// =====================================================================
//     ELEMENT FUNCTIONS
// =====================================================================

// =====================================================================
const BString &
BAttributeDecl::GetElement() const
{
	return _element;
}


// =====================================================================
void
BAttributeDecl::SetElement(const char * element)
{
	_element = element;
}


}; // namespace XML
}; // namespace B
