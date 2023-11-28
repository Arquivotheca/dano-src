#include <xml2/BContent.h>
#include <xml2/BStringUtils.h>

#include <stdio.h>

namespace B {
namespace XML {


#if 0

static void
_print_detailed_element_decl_helper(BElementDecl::List * list, int32 level)
{
	int32 count = list->CountItems();
	for (int32 i=0; i<count; i++)
	{
		BElementDecl::ListNode * node = list->NodeAt(i);
		for (int32 j=0; j<level; j++)
			printf("\t");
		if (node->type == BElementDecl::ListNode::STRING)
			printf("STRING (cardinality %d): \"%s\"\n", node->cardinality, node->element.String());
		else
		{
			printf("LIST (cardinality %d)\n", node->cardinality);
			_print_detailed_element_decl_helper(node->list, level+1);
		}	
	}
}

void
_print_detialed_element_decl(BElementDecl * decl)
{
	switch (decl->Mode())
	{
		case BElementDecl::CHILDREN:
		{
			printf("LIST (cardinality %d)\n", decl->GetPattern()->cardinality);
			_print_detailed_element_decl_helper(decl->GetPattern()->list, 1);
		}
		default:
			;
	}
}

#endif


// =====================================================================
//     CONSTRUCTOR / DESTRUCTOR / ETC
// =====================================================================

// =====================================================================
BElementDecl::BElementDecl(const char * name)
	:BXMLObject(B_XML_ELEMENT_DECL_OBJECT),
	 BNamed(name),
	 _mode(EMPTY),
	 _mixed_AllowedChildren(),
	 _children_pattern(ListNode::NO_INIT, NULL),
	 _attributes()
{
	
}


// =====================================================================
BElementDecl::BElementDecl(const BString & name)
	:BXMLObject(B_XML_ELEMENT_DECL_OBJECT),
	 BNamed(name),
	 _mode(EMPTY),
	 _mixed_AllowedChildren(),
	 _children_pattern(ListNode::NO_INIT, NULL),
	 _attributes()
{
	
}


// =====================================================================
BElementDecl::~BElementDecl()
{
	_attributes.MakeEmpty(true);
}


// =====================================================================
status_t
BElementDecl::SetTo(BString & contentSpec)
{
	// Empty out everything
	_mixed_AllowedChildren.MakeEmpty();
	
	_children_pattern.type = ListNode::NO_INIT;
	delete _children_pattern.list;
	_children_pattern.list = NULL;
	_children_pattern.element = "";
	
	_attributes.MakeEmpty(true);
	
	// There is no significant whitespace in Element Decls
	// This makes it a whole lot easier to parse
	StripWhitespace(contentSpec);
	
	// ANY Content
	if (contentSpec == "ANY")
	{
		SetMode(ANY);
		return B_OK;
	}
	
	// EMPTY Content
	else if (contentSpec == "EMPTY")
	{
		SetMode(EMPTY);
		return B_OK;
	}
	
	else if (contentSpec.Length() < 3
				|| contentSpec.ByteAt(0) != '('
				|| (contentSpec[contentSpec.Length()-1] != ')'
					&& contentSpec[contentSpec.Length()-2] != ')'))
		return B_XML_PARSE_ERROR;
	
	// MIXED Content
	else if (0 == contentSpec.Compare("(#PCDATA", 8))
	{
		SetMode(MIXED);
		return handle_mixed_content_spec(contentSpec);
	}
	
	// ELEMENT Content
	else
	{
		SetMode(CHILDREN);
		return handle_element_content_spec(contentSpec);
	}
}


// =====================================================================
BElementDecl::element_children_allowed
BElementDecl::Mode() const
{
	return _mode;
}


// =====================================================================
void
BElementDecl::SetMode(BElementDecl::element_children_allowed mode)
{
	_mode = mode;
}



// =====================================================================
// MIXED Mode
// =====================================================================

// =====================================================================
status_t
BElementDecl::AllowElement(const char * name)
{
	BString str(name);
	return _mixed_AllowedChildren.AddItem(str);
}



// =====================================================================
status_t
BElementDecl::AllowElement(const char * name, int32 length)
{
	BString str(name, length);
	return _mixed_AllowedChildren.AddItem(str);
}


// =====================================================================
status_t
BElementDecl::DisallowElement(const char * name)
{
	return _mixed_AllowedChildren.RemoveItemFor(BString(name));
}


// =====================================================================
bool
BElementDecl::IsElementAllowed(const char * name) const
{
	size_t index;
	return _mixed_AllowedChildren.GetIndexOf(BString(name), &index);
}


// =====================================================================
int32
BElementDecl::CountAllowedElements() const
{
	return _mixed_AllowedChildren.CountItems();
}


// =====================================================================
const BString &
BElementDecl::AllowedElementAt(int32 index) const
{
	return _mixed_AllowedChildren.ItemAt(index);
}



// =====================================================================
// CHILDREN Mode
// =====================================================================

// =====================================================================
BElementDecl::ListNode::ListNode()
	:type(NO_INIT), list(NULL)
{
	
}


// =====================================================================
BElementDecl::ListNode::ListNode(Type atype, List * alist)
	:type(atype), list(alist)
{
	
}


// =====================================================================
BElementDecl::ListNode::~ListNode()
{
	delete list;
}



// =====================================================================
BElementDecl::List::~List()
{
	MakeEmpty();
}


// =====================================================================
void
BElementDecl::List::AddNode(ListNode * node)
{
	_list.AddItem(node);
}


// =====================================================================
void
BElementDecl::List::AddNode(ListNode * node, int32 index)
{
	_list.AddItemAt(node, index);
}


// =====================================================================
int32
BElementDecl::List::CountItems() const
{
	return _list.CountItems();
}


// =====================================================================
BElementDecl::ListNode *
BElementDecl::List::NodeAt(int32 index)
{
	return (ListNode *) _list.ItemAt(index);
}

const BElementDecl::ListNode *
BElementDecl::List::NodeAt(int32 index) const
{
	return (ListNode *) _list.ItemAt(index);
}


// =====================================================================
void
BElementDecl::List::RemoveNode(BElementDecl::ListNode * node)
{
	int32 c = _list.CountItems();
	for (int32 i=0; i<c; ++i)
		if (((BElementDecl::ListNode*)_list.ItemAt(i)) == node) {
			_list.RemoveItemsAt(i);
			return;
		}
}


// =====================================================================
void
BElementDecl::List::RemoveNode(int32 index)
{
	_list.RemoveItemsAt(index);
}


// =====================================================================
void
BElementDecl::List::MakeEmpty()
{
	int32 count = _list.CountItems();
	for (int32 i=0; i<count; ++i)
		delete (ListNode *) _list.ItemAt(i);
	_list.MakeEmpty();
}



// =====================================================================
BElementDecl::ListNode *
BElementDecl::GetPattern()
{
	return &_children_pattern;
}

const BElementDecl::ListNode *
BElementDecl::GetPattern() const
{
	return &_children_pattern;
}


// =====================================================================
// static function...
bool
BElementDecl::AllChildrenOptional(const ListNode * node)
{
	if (node->type == ListNode::STRING)
		return node->cardinality == ZERO_OR_MORE || node->cardinality == ONE_OR_ZERO;
	int32 count = node->list->CountItems();
	for (int32 i=0; i<count; i++)
	{
		ListNode * item = node->list->NodeAt(i);
		if (item->cardinality == ONE || item->cardinality == ONE_OR_MORE)
			return false;
		if (item->type == ListNode::LIST)
			if (!AllChildrenOptional(item))
				return false;
	}
	return true;
}



// =====================================================================
// ATTRIBUTES
// =====================================================================

// =====================================================================
status_t
BElementDecl::AddAttributeDecl(BAttributeDecl * decl)
{
	return _attributes.Add(decl);
}


// =====================================================================
status_t
BElementDecl::RemoveAttributeDecl(BAttributeDecl * decl)
{
	_attributes.Remove(decl);
	return B_OK;
}


// =====================================================================
BAttributeDecl *
BElementDecl::FindAttributeDecl(const char * name)
{
	return (BAttributeDecl *) _attributes.Find(name);
}

const BAttributeDecl *
BElementDecl::FindAttributeDecl(const char * name) const
{
	return (BAttributeDecl *) _attributes.Find(name);
}


// =====================================================================
int32
BElementDecl::CountAttributeDecls() const
{
	return  _attributes.CountItems();
}


// =====================================================================
BAttributeDecl *
BElementDecl::GetAttributeDecl(int32 index)
{
	return (BAttributeDecl *) _attributes.ItemAt(index);
}

const BAttributeDecl *
BElementDecl::GetAttributeDecl(int32 index) const
{
	return (const BAttributeDecl *) _attributes.ItemAt(index);
}



// =====================================================================
// Parsing Functions
// =====================================================================

// Mixed Content allows children that are Text, etc, as well as elements
// of given names.  However, with Mixed Content, you cannot specify the
// order or number of the elements, only whether they are allowed or not.
// =====================================================================
status_t
BElementDecl::handle_mixed_content_spec(BString & contentSpec)
{
	status_t err;
	int32 begin = 1;
	int32 end = 1;
	bool first = true;  // the first one will always be "#PCDATA", ignore it
	while ((end = contentSpec.FindFirst('|', begin)) > 0)
	{
		if (!first)
		{
			err = AllowElement(contentSpec.String()+begin, end-begin);
			if (err != B_OK)
				return err;
		}
		else
			first = false;
		begin = end + 1;
	}
	if (!first)
	{
		err = AllowElement(contentSpec.String()+begin, contentSpec.Length()-begin-1);
		if (err != B_OK)
			return err;
	}
	return B_OK;
}


// =====================================================================
status_t
BElementDecl::handle_element_content_spec(BString & contentSpec)
{
	const char * str = contentSpec.String();
	
	// Our top level _children_pattern is always a list node.
	_children_pattern.type = ListNode:: LIST;
	_children_pattern.list = new List;
	
	return handle_element_pattern(str, _children_pattern.list, _children_pattern.cardinality);
}


// =====================================================================
status_t
BElementDecl::handle_element_pattern(const char *& p, List * into, node_cardinality & cardin)
{
	status_t err;
	bool foundType = false;
	const char * begin = p+1;
	ListNode * node;
	char c;
	
	if (*p != '(')
		return B_XML_PARSE_ERROR;
	p++;
	
	while (*p != ')')
	{
		
		if (*p == '\0')
			return B_XML_PARSE_ERROR;
		
		// Handle Children
		if (*p == '(')
		{
			List * childList = new List;
			ListNode * childNode = new ListNode(ListNode::LIST, childList);
			err = handle_element_pattern(p, childList, childNode->cardinality);
			if (err != B_OK)
			{
				delete childNode;
				delete childList;
				return err;
			}
			
			// Skip over the ',' or the '|'
			// Otherwise, it's a closing ')'
			// don't skip those
			if (*p == ',' || *p == '|')
				p++;
			
			begin = p;
			into->AddNode(childNode);
			continue;
		}
		
		// Choice
		if (*p == '|' && (!foundType || into->type == List::CHOICE))
		{
			if (!foundType)
			{
				foundType = true;
				into->type = List::CHOICE;
			}
			
			BString element(begin, p-begin);
			node = new ListNode(ListNode::STRING, NULL);

			c = element.ByteAt(element.Length()-1);
			if (c == '+')
				node->cardinality = ONE_OR_MORE;
			else if (c == '*')
				node->cardinality = ZERO_OR_MORE;
			else if (c == '?')
				node->cardinality = ONE_OR_ZERO;
			else
				node->cardinality = ONE;	
			if (node->cardinality != ONE)
				element.Truncate(element.Length() - 1, true);
			
			node->element.Adopt(element);
			into->AddNode(node);
			
			begin = p+1;
		}
		
		// Seq
		if (*p == ',' && (!foundType || into->type == List::SEQ))
		{
			if (!foundType)
			{
				foundType = true;
				into->type = List::SEQ;
			}
			
			BString element(begin, p-begin);
			node = new ListNode(ListNode::STRING, NULL);

			c = element.ByteAt(element.Length()-1);
			if (c == '+')
				node->cardinality = ONE_OR_MORE;
			else if (c == '*')
				node->cardinality = ZERO_OR_MORE;
			else if (c == '?')
				node->cardinality = ONE_OR_ZERO;
			else
				node->cardinality = ONE;	
			if (node->cardinality != ONE)
				element.Truncate(element.Length() - 1, true);
			
			node->element.Adopt(element);
			into->AddNode(node);
			
			begin = p+1;
		}
		
		p++;
	}
	
	// Take care of the last string
	BString element(begin, p-begin);
	if (element != "")  // there isn't one of these on the outermost one.  why?
	{
		node = new ListNode(ListNode::STRING, NULL);
		
		c = element.ByteAt(element.Length()-1);
		if (c == '+')
			node->cardinality = ONE_OR_MORE;
		else if (c == '*')
			node->cardinality = ZERO_OR_MORE;
		else if (c == '?')
			node->cardinality = ONE_OR_ZERO;
		else
			node->cardinality = ONE;	
		if (node->cardinality != ONE)
			element.Truncate(element.Length() - 1, true);
	
		node->element.Adopt(element);
		into->AddNode(node);
	}
			
	// Set the type to SEQ if there was no type
	if (!foundType)
		into->type = List::SEQ;
	
	// Skip over the ')'
	p++;
	
	// Determine the Cardinality 
	if (*p == '+')
		cardin = ONE_OR_MORE;
	else if (*p == '*')
		cardin = ZERO_OR_MORE;
	else if (*p == '?')
		cardin = ONE_OR_ZERO;
	else
		cardin = ONE;	
	
	if (cardin != ONE)
		p++;
	
	return B_OK;
}



}; // namespace XML
}; // namespace B

