

#define DEBUG 1

#include <OS.h>
#include <support2/Debug.h>
#include <xml2/BParser.h>
#include <xml2/BStringUtils.h>
#include <validate_children.h>

namespace B {
namespace XML {

#if DEBUG
#define CHECKPOINT(msg) printf("%s: %s\n", __FUNCTION__, (msg));
#define PRINTF(msg) printf msg ;
#else
#define CHECKPOINT(msg) ;
#define PRINTF(msg) ;
#endif

#define DIE_HERE()	debugger("XML Internal Parser Problem.  Please file a bug with a stack crawl.")


template <class T1, class T2>
static bool
namespaces_same(const T1 * e1, const T2 * e2)
{
	const BNamespace * ns1 = e1->Namespace();
	const BNamespace * ns2 = e2->Namespace();
	if (ns1 == ns2)
		return true;
	if (!ns1 || !ns2)
		return false;
	return 0 == strcmp(ns1->Value(), ns2->Value());
}

BXMLValidatingContext::BXMLValidatingContext(BDocument * document,
											BXMLObjectFactory * factory,
											BEntityStore * entityStore)
	:BXMLDocumentParseContext(document, factory, entityStore)
{
	
}


BXMLValidatingContext::~BXMLValidatingContext()
{
	
}


status_t
BXMLValidatingContext::OnBeginDocumentTypeNode(BDocumentType * dt)
{
	status_t err;
	
	err = BXMLDocumentParseContext::OnBeginDocumentTypeNode(dt);
	if (err != B_OK)
		return err;
	
	// Cache a pointer to the DocumentType object
	_dt = dt;
	
	return B_OK;
	
}


status_t
BXMLValidatingContext::OnEndDocumentTypeNode()
{
	if (!_dt)
		return B_ERROR;
	
	status_t err;
	
	err = BXMLDocumentParseContext::OnEndDocumentTypeNode();
	if (err != B_OK)
		return err;
	
	return B_OK;
}


// It would be nice to extend this to some of the other Be types (int32, etc.)
// This function should also add to any "to-check" lists that are checked
// when the processing is done.  This is to allow forward references.
status_t
BXMLValidatingContext::check_attr_type(BAttribute * attr, const BAttributeDecl * attrDecl) 
{
	int32 pos;
	BString value;
	BString split;
	BEntityDecl * entityDecl;
	
	switch(attrDecl->GetType())
	{
		case B_STRING_TYPE:
			return B_OK;
		
		case B_ID_TYPE:
		case B_IDREF_TYPE:
		case B_IDREFS_TYPE:
			return B_OK;
		
		case B_ENUM_TYPE:
		{
			attr->GetValue(&value);
			if (attrDecl->IsEnumValue(value.String()))
				return B_OK;
			else
				return B_XML_BAD_ENUM_VALUE;
		}
		case B_ENTITY_TYPE:
		{
			// See if the entity is declared -- must be an unparsed entity
			attr->GetValue(&value);
			entityDecl = _dt->FindEntityDecl(value.String(), true);
			if (!entityDecl)
				return B_XML_ENTITY_NOT_FOUND;
			if (entityDecl->GetType() != B_XML_UNPARSED_ENTITY)
				return B_XML_ENTITY_REF_NOT_UNPARSED;
			return B_OK;
		}
		case B_ENTITIES_TYPE:
		{
			attr->GetValue(&value);
			pos = 0;
			// Entities are separated by whitespace
			// Loop for each one
			while (SplitStringOnWhitespace(value, split, &pos))
			{
				// See if the entity is declared -- must be an unparsed entity
				entityDecl = _dt->FindEntityDecl(split.String(), true);
				
				if (!entityDecl)
					return B_XML_ENTITY_NOT_FOUND;
				if (entityDecl->GetType() != B_XML_UNPARSED_ENTITY)
					return B_XML_ENTITY_REF_NOT_UNPARSED;
			}
			return B_OK;
		}
		
		case B_NMTOKEN_TYPE:
		case B_NMTOKENS_TYPE:
		case B_NOTATION_TYPE:
		default:
			return B_BAD_TYPE;
	}
	
	// Should never be here
	DIE_HERE();
	return B_ERROR;
}


status_t
BXMLValidatingContext::check_next_element(const char * element)
{
	_ElementChildrenTracker_ * trak;
	
	// If elementTrackerStack is empty, then we haven't encountered
	// the top level element yet.  This _is_ the top level element
	if (_elementTrackerStack.CountItems() == 0)
	{
		// XXX Check to see if this matches the declared document root element
		return B_OK;
	}
	else
	{
		trak = element_stack_top();
		ASSERT(trak);
		switch (trak->decl->Mode())
		{
			case BElementDecl::EMPTY:
			{
				return B_XML_ELEMENT_NOT_EMPTY;
			}
			break;
			case BElementDecl::ANY:
			{
				return B_OK;
			}
			break;
			case BElementDecl::MIXED:
			{
				ASSERT(trak->decl);
				if (trak->decl->IsElementAllowed(element))
				{
					return B_OK;
				}
				else
				{
					return B_XML_CHILD_ELEMENT_NOT_ALLOWED;
				}
			}
			break;
			case BElementDecl::CHILDREN:
			{
				return _check_next_element_(element, &trak->state, trak->data);
			}
		}
	}
	
	// Should never be here
	DIE_HERE();
	return B_ERROR;
}

// The element stack pushes & pops as we go further into the nesting of elements
// 
void
BXMLValidatingContext::push_element_stack(const BElementDecl * decl)
{
	ASSERT(decl);
	_ElementChildrenTracker_ * newTrak;
	newTrak = new _ElementChildrenTracker_(decl);
	
	// If decl is a CHILDREN, get the table we're going to need to
	// do it.  But first, look up the _elementTrackerStack, so if
	// it has already been created for this ElementDecl, just
	// use that one
	if (decl->Mode() == BElementDecl::CHILDREN)
	{
		int32 count = _elementTrackerStack.CountItems();
		for (int32 i=0; i<count; i++)
		{
			_ElementChildrenTracker_ * stackTrak = 
							(_ElementChildrenTracker_ *) _elementTrackerStack.ItemAt(i);
			if (stackTrak->decl->Mode() == BElementDecl::CHILDREN && decl == stackTrak->decl)
			{
				newTrak->data = stackTrak->data;
				newTrak->dataFromAboveInStack = true;
			}
		}
		if (!newTrak->dataFromAboveInStack)
			_init_validate_table_(decl, &(newTrak->data));
		_init_validate_state_(&newTrak->state);
	}
	_elementTrackerStack.AddItem(newTrak);
	return ;
}


// Will fail if we're in child pattern mode and there are still more required elements
status_t
BXMLValidatingContext::pop_element_stack()
{
	status_t err;
	ASSERT(_elementTrackerStack.CountItems() != 0);

	_ElementChildrenTracker_ * oldTrak;
	oldTrak = element_stack_top();
	
	if (oldTrak->decl->Mode() == BElementDecl::CHILDREN)
	{
		err = _check_end_of_element_(oldTrak->state, oldTrak->data);
		if (err != B_OK)
			return err;
		if (!oldTrak->dataFromAboveInStack)
			_free_validate_table_(oldTrak->data);
	}
	
	_elementTrackerStack.RemoveItemsAt(_elementTrackerStack.CountItems()-1);
	delete oldTrak;
	return B_OK;
}

BXMLValidatingContext::_ElementChildrenTracker_ *
BXMLValidatingContext::element_stack_top()
{
	if (_elementTrackerStack.CountItems() == 0)
		return NULL;
	return (_ElementChildrenTracker_ *) _elementTrackerStack.ItemAt(_elementTrackerStack.CountItems()-1);
}


BXMLValidatingContext::_ElementChildrenTracker_::_ElementChildrenTracker_(const BElementDecl * elementDecl)
	:decl(elementDecl), data(NULL), dataFromAboveInStack(false), state(0)
{
	
}


static status_t
compare_attr_val(const BAttribute * attr, const BAttributeDecl * attrDecl)
{
	// invariant: attr/attrDecl are for a fixed value attribute
	BString value;
	attr->GetValue(&value);
	const BString & correctValue = attrDecl->GetDefault();
	switch (attrDecl->GetType())
	{
		case B_STRING_TYPE:
		case B_ENUM_TYPE:
		{
			status_t result = B_XML_ATTRIBUTE_BAD_FIXED_VALUE;
			if( value == correctValue )
			{
				result = B_OK;
			}
			
			return result;
		}
		break;
		case B_ID_TYPE:
		case B_IDREF_TYPE:
		case B_NOTATION_TYPE:
		case B_ENTITY_TYPE:
		case B_NMTOKEN_TYPE:
		case B_IDREFS_TYPE:
		case B_ENTITIES_TYPE:
		case B_NMTOKENS_TYPE:
		{
			BString fixed = correctValue;
			MushString(value);
			MushString(fixed);
			
			status_t result = B_XML_ATTRIBUTE_BAD_FIXED_VALUE;
			if( value == fixed )
			{
				result = B_OK;
 			}
	
			return result;			
		}
		break;
		default:
			return B_BAD_TYPE;
	}
	
	// Should never be here
	DIE_HERE();
	return B_ERROR;
}


status_t
BXMLValidatingContext::is_element_declared_somewhere(BElement * element, const BElementDecl ** decl)
{
	const BElementDecl * elementDecl;
	*decl = NULL;
	
	// Check for declaration in a namespace
	const BNamespace * ns = element->Namespace();
	if (ns)
	{
		const BDocumentTypeDefinition * nsDTD = ns->GetDTD();
		if (!nsDTD)
			return B_XML_NO_DTD;
		
		elementDecl = nsDTD->FindElementDecl(element->Name());
		if (!elementDecl)
			return B_XML_ELEMENT_NOT_DECLARED;
	}
	else
	{
		elementDecl = _dt->FindElementDecl(element->Name());
		if (!elementDecl)
			return B_XML_ELEMENT_NOT_DECLARED;
	}
	*decl = elementDecl;
	return B_OK;
}

status_t
BXMLValidatingContext::OnStartTagNode(BElement * element)
{
	status_t err;
	
	if (!_dt)
		return B_ERROR;
	
	// Validate the presence of this element
	
	// Is this element declared in the namespace that it claims to be from
	const BElementDecl * elementDecl;
	err = is_element_declared_somewhere(element, &elementDecl);
	if (err != B_OK)
		return err;
	
	if (!_currentElement || namespaces_same(_currentElement, element))
	{
		// The namespaces are the same, process normally
		
		// See if this element is allowed as a child of its parent
		err = check_next_element(element->Name());
		if (err != B_OK)
			return err;
	}
	else
	{
		// We have taken care of namespace defaulting in the DocumentCreationContext Level
		// The namespaces are different, we will allow it.
		;
	}
	
	// Validate the attributes
	// Iterate over the list of attributes in the decl and the list of attributes
	// in the element.  They are both stored in lists sorted by name.
	// "elem" here means how many/which one in the element that just happened
	// "decl" here means how many/which one in the dtd declaration
	int32 attrElemCount = element->CountAttributes();
	int32 attrElemNumber = 0;
	int32 attrDeclCount = elementDecl->CountAttributeDecls();
	int32 attrDeclNumber = 0;
	BVector<void*> attrsToAdd;
	while (attrDeclNumber < attrDeclCount)
	{
		BAttribute * attr = NULL;
		const BAttributeDecl * attrDecl = elementDecl->GetAttributeDecl(attrDeclNumber);
		int32 compared = 1;
		bool sameNamespaces = true;
		
		// If we run out of attributes in the element, there might be some in the 
		// decl that need to be added, or there might be one that is required.
		// So if we're past the end of the element attrs, we keep looking at the
		// attr decls until we finish with them, but without touching any element
		// attrs.
		
		if (attrElemNumber < attrElemCount)
		{
			attr = element->AttributeAt(attrElemNumber);
			
			// If the name of attr is less than the name of the attrDecl, then
			// there's a problem, because it's alphabetically before what should
			// come next
			compared = strcmp(attr->Name(), attrDecl->Name());
			if (compared < 0)
			{
				return B_XML_ATTRIBUTE_NOT_DECLARED;
			}
			else
			{
				sameNamespaces = namespaces_same(element, attr);
			}
		}
		
		if (sameNamespaces)
		{
			// The namespaces are the same.  Check for acceptability
			
			// If the name of attrDecl is less than the name of the attr, then
			// there is an attribute missing.  Process that.  Otherwise, they're
			// the same attribute, so check it.
			if (attrElemNumber >= attrElemCount || compared > 0)
			{
				// The attribute is not here maybe we should add it, or maybe it's required
				switch (attrDecl->GetBehavior())
				{
					// If the attribute is implied or default add it with the default val
					case B_XML_ATTRIBUTE_DEFAULT:
					{
						BString newAttributeName(attrDecl->Name());
						BString newAttributeValue(attrDecl->GetDefault());
						BAttribute * newAttribute = _factory->AttributeFactory(newAttributeName, element->Namespace());
						newAttribute->SetValue(newAttributeValue);
						attrsToAdd.AddItem(newAttribute);
						
						// Make sure the types match and are correct
						// This is because the DTD doesn't check it's own validity.
						// Maybe this should be looked into.
						err = check_attr_type(newAttribute, attrDecl);
						if (err != B_OK)
							return err;
					}
					break;
					
					// If the attribute is required, signal an error because it's not here
					case B_XML_ATTRIBUTE_REQUIRED:
						return B_XML_REQUIRED_ATTRIBUTE_MISSING;
					
					// If the attribute is fixed or implied, do not add it
					case B_XML_ATTRIBUTE_IMPLIED:
					case B_XML_ATTRIBUTE_FIXED:
						break;
					
					default:
						break;
				}
				
				// Advance only the decl index
				attrDeclNumber++;
			}
			else
			{
				// Make sure the types match and are correct
				err = check_attr_type(attr, attrDecl);
				if (err != B_OK)
					return err;
				
				// Check the behavior
				switch (attrDecl->GetBehavior())
				{
					case B_XML_ATTRIBUTE_FIXED:
						err = compare_attr_val(attr, attrDecl);
						if (err != B_OK)
							return err;
						break;
					case B_XML_ATTRIBUTE_DEFAULT:
					case B_XML_ATTRIBUTE_REQUIRED:
					case B_XML_ATTRIBUTE_IMPLIED:
						break;
					default:
						break;
				}
				
				// Advance both the decl index and the attr index
				attrDeclNumber++;
				attrElemNumber++;
			}
		}
		else
		{
			attrElemNumber++;
			// The namespaces are different.  Accept it.  It's okay.
			
			//  XXX.  Woah.  How are we to find out any information about this
			// attribute.  We need to know the following:
			//   - Is it declared (for another namespace's element???)
			//   - What is its type (and does it match)
			//   - What is its behavior (and if it's fixed, does the value check out okay)
		}
	}
	
	// If we didn't get through all of the attributes that are in the 
	// element, look to see if they have other namespaces.  If they do,
	// then they are legit.  If they come from the same namespace as the
	// element, then they are errors, because they must be undeclared.
	while (attrElemNumber < attrElemCount)
	{
		const BAttribute * attr = element->AttributeAt(attrElemNumber);
		if (namespaces_same(element, attr))
			return B_XML_ATTRIBUTE_NOT_DECLARED;
		else
		{
			// See the "XXX. Woah." comment from above for what we need to do here.
		}
		// printf("attrElemNumber %d\n", (int) attrElemNumber);
		attrElemNumber++;
	}
	
	// Now that we're done checking attributes, add in the ones we created
	int32 addAttrCount = attrsToAdd.CountItems();
	for (int32 i=0; i<addAttrCount; i++)
		element->AddAttribute((BAttribute *) attrsToAdd.ItemAt(i));
	
	// Everything is okay at this point, so get ready to start checking for
	// children of this element
	push_element_stack(elementDecl);
	
	// All is well, so let's add it in
	return BXMLDocumentParseContext::OnStartTagNode(element);
}


status_t
BXMLValidatingContext::OnEndTagNode(BElement * element)
{
	status_t err = pop_element_stack();
	if (err != B_OK)
		return err;
	return BXMLDocumentParseContext::OnEndTagNode(element);
}


status_t
BXMLValidatingContext::OnTextNode(BText * text)
{
	status_t err;
	BString value;
	bool allow;
	text->GetValue(&value);
	err = is_text_value_allowed(value, allow);
	if (err != B_OK)
		return err;
	if (allow)
		return BXMLDocumentParseContext::OnTextNode(text);
	return B_OK;
}


status_t
BXMLValidatingContext::OnCDataNode(BCData * cData)
{
	status_t err;
	BString value;
	bool allow;
	cData->GetValue(&value);
	err = is_text_value_allowed(value, allow);
	if (err != B_OK)
		return err;
	if (allow)
		return BXMLDocumentParseContext::OnCDataNode(cData);
	return B_OK;
}	


// Note: value will be stripped of whitespace
status_t
BXMLValidatingContext::is_text_value_allowed(BString & value, bool & allow)
{
	allow = false;
	StripWhitespace(value);
	
	// A text/cdata outside any elements.  This should have already been trapped by now,
	// so this is really just sanity checking
	if(_elementTrackerStack.CountItems() == 0)
	{
		if (value.Length() == 0)
		{
			// If we're coalescing whitespace, ignore it
			if (_flags & B_XML_COALESCE_WHITESPACE)
				return B_OK;
			else
			{
				allow = true;
				return B_OK;
			}
		}
		else
			return B_XML_PARSE_ERROR;
	}
	
	_ElementChildrenTracker_ * trak;

	trak = (_ElementChildrenTracker_ *) element_stack_top();
	ASSERT(trak);
	
	switch (trak->decl->Mode())
	{
		case BElementDecl::EMPTY:
		case BElementDecl::CHILDREN:
		{
			// No text/cdata children allowed.
			// If all whitespace, okay, else error
			if (value.Length() == 0)
				return B_OK;
			else
				return B_XML_CHILD_CDATA_NOT_ALLOWED;
		}
		break;
		case BElementDecl::ANY:
		case BElementDecl::MIXED:
		{
			allow = true;
			return B_OK;
		}
	}
	
	// Should never be here
	DIE_HERE();
	return B_ERROR;
}

}; // namespace XML
}; // namespace B


