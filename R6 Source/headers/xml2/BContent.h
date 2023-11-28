#ifndef _B_XML2_CONTENT_H
#define _B_XML2_CONTENT_H

#include <xml2/BXMLDefs.h>
#include <support2/String.h>
#include <support2/OrderedVector.h>


namespace B {
namespace XML {

using namespace Support2;

// Forward References
// =====================================================================
class BXMLObject;
class BNamespace;
class BContent;
class BNamed;
class BElement;
class BDocument;
class BValued;
class BAttribute;
class BText;
class BCData;
class BComment;
class BNamedSet;
class BDocumentType;
class BProcessingInstruction;
class BDocumentTypeDefinition;
class BDocumentTypeDefinition;
class BElementDecl;
class BAttributeDecl;
class BEntityDecl;
class BXMLObjectFactory;
class BEntityStore;


// Generic Type Coded XML Object
// =====================================================================
// Some xml objects (specifically BAttribute) don't have much in common
// functionally, but they all need a common base class.
class BXMLObject
{
public:
							BXMLObject(uint32 type);
	virtual					~BXMLObject();
	
							// Type Code
	uint32					Type() const;
	
private:
	uint32					_type;
};


// XML Content
// =====================================================================
// Base class of the classes that make up the document structure.
class BContent : public BXMLObject
{
public:
							// All elements have parents except for the
							// root element, where this returns null
	BElement				* Parent();
	const BElement			* Parent() const;
	
							// Each node has a pointer to the document it's
							// in (if it's in a document).  
	BDocument				* Document();
	const BDocument			* Document() const;
	
							// The next sibling in document order.
							// Mask looks until the next one that matches mask
							// Element is a convience wrapper for NextSibMask(ELEMENT);
	BContent				* NextSibling();
	const BContent			* NextSibling() const;
	BContent				* NextSibling(uint32 typeMask);
	const BContent			* NextSibling(uint32 typeMask) const;
	BContent				* PreviousSibling();
	const BContent			* PreviousSibling() const;
	BContent				* PreviousSibling(uint32 typeMask);
	const BContent			* PreviousSibling(uint32 typeMask) const;
	
	BElement				* NextSiblingElement();
	const BElement			* NextSiblingElement() const;
	BElement				* PreviousSiblingElement();
	const BElement			* PreviousSiblingElement() const;
	
	virtual BContent		* Clone() const;
	
protected:
	
							// You get a copy that is not connected to the document
							// hierarchy
	explicit				BContent(const BContent & copy);
	
							// Never create a plain BContent
							BContent(uint32 type);
	
	virtual					~BContent();
	
private:
							// Always need a type
							BContent();
	
	bool					check_content_for_connections();
	status_t				set_parent(BElement * parent);
	status_t				set_next_sibling(BContent * sibling);
	status_t				set_prev_sibling(BContent * sibling);
	status_t				set_document(BDocument * document);
	
	BElement				* _parent;
	BContent				* _nextSibling;
	BContent				* _prevSibling;
	BDocument				* _document;
	
	friend class	BElement;		// BElement manages its list of BContents
	friend class	BDocument;		// BDocument manages its list of BContents
};


// XML Named
// =====================================================================
// Simple mechanism for giving elements and attributes a name
// There is a corresponding BNamedSet that allows access to
// named things.
class BNamed
{
public:
	const char			* Name() const;
	
	virtual				~BNamed();

protected:
	explicit			BNamed(const BNamed & copy);
	
						BNamed(const char * name);
						BNamed(const BString & name);
						BNamed(BString & name, bool adopt = false);
	
						// protected, because not all named things have namespaces
	BNamespace			* Namespace();
	const BNamespace	* Namespace() const;
	
	virtual void		SetName(const char * name);
	virtual status_t	SetNamespace(BNamespace * space);
	
private:
	BString				_name;
	BNamespace			* _namespace;
	
	friend class	BNamedSet;	// BNamedSet uses the protected Namespace() function
};


// XML Namespace
// =====================================================================
// This gets attached to an element or document, and pointed to by
// BNamed objects that are in that namespace
// The BNamed base class is the name of the prefix, because that's
// what we look up by
class BNamespace : public BNamed
{
public:
					BNamespace(const BNamespace & copy);
					BNamespace(const char * prefix, const char * value);
					BNamespace(const BString & prefix, const BString & value);
					BNamespace(BString & prefix, BString & value, bool adopt);
	
	virtual			~BNamespace();
	
	const char *	Value() const;
	
									// Claims ownership
	virtual	void					SetDTD(BDocumentTypeDefinition * dtd);
	
	const BDocumentTypeDefinition	* GetDTD() const;
	BDocumentTypeDefinition			* GetDTD();
	
private:
								BNamespace();
	
	BString						_value;
	BDocumentTypeDefinition		* _dtd;
	
	friend class	BNamedSet;
};


// Set of things with names
// =====================================================================
// Provides a set of BXMLNamed objects.  Used to keep track of attributes
// and some of the DTD declarations.  If you go through in index order,
// you get list sorted by name.
class BNamedSet
{
public:
							BNamedSet();
	virtual					~BNamedSet();
	
	virtual status_t		Add(BNamed * named);
	virtual void			Remove(BNamed * named);
	virtual void			Remove(int32 index);
	virtual BNamed			* Find(const char * name);
	virtual const BNamed	* Find(const char * name) const;
	
	int32					CountItems() const;
	BNamed					* ItemAt(int32 index);
	const BNamed			* ItemAt(int32 index) const;
	
	virtual void			MakeEmpty(bool deleteData);
	
private:
	BVector<BNamed *> _data;
}; 


// XML Element
// =====================================================================
// BElement maps directly to the element construction in an XML document.
// Only BElement know about their children, and the only way to add an
// element to ta document is to add it to an element, even though it is
// actually stored as a linked list.
// All the add functions take ownership -- don't try to delete things you add
class BElement : public BContent, public BNamed
{
public:
	
	// --- Constructor and Destructor  ----------------------
							BElement(const char * name);
							BElement(const BString & name);
							BElement(BString & name, bool adopt = false);
							BElement(const BElement & copy);
	virtual					~BElement();
	
	virtual BContent		* Clone() const;
	BElement				& operator=(const BElement & copy);
	
	// --- Namespaces ---------------------------------------
							// Add this element to the namespace space
	virtual status_t		SetNamespace(BNamespace * space);
	using					BNamed::Namespace;
	
	virtual status_t		AddNamespace(BNamespace * space);
	
	int32					CountNamespaces() const;
	BNamespace				* NamespaceAt(int32 index);
	const BNamespace		* NamespaceAt(int32 index) const;
	
	BNamespace				* FindNamespaceByPrefix(const char * prefix);
	const BNamespace		* FindNamespaceByPrefix(const char * prefix) const;
	BNamespace				* FindNamespaceByValue(const char * value);
	const BNamespace		* FindNamespaceByValue(const char * value) const;
	
							// Find the element that the namespace is declared in.
							// NULL if we can't find it.  The element will be either
							// this element or one of its parents
	BElement				* FindNamespaceDecl(const BNamespace * space);
	const BElement			* FindNamespaceDecl(const BNamespace * space) const;
	
	// --- Getting Children ---------------------------------
							// Access to the first and last children for iteration
							// iteration over all of the children of any type.
							// With a mask, it will find the first that fits it
	BContent				* FirstChild();
	const BContent			* FirstChild() const;
	BContent				* FirstChild(uint32 typeMask);
	const BContent			* FirstChild(uint32 typeMask) const;
	BContent				* LastChild();
	const BContent			* LastChild() const;
	BContent				* LastChild(uint32 typeMask);
	const BContent			* LastChild(uint32 typeMask) const;
	
							// Find the index of the first element named
							// <name> at or after index <atOrAfter>
	status_t				FindElementNamed(const char * name, BElement ** element);
	
	// --- Adding Children ----------------------------------
							// Make it the first one, take ownership
	virtual status_t		AddChildFirst(BContent * content);
	
							// Make it the last one, take ownership
	virtual status_t		AddChildLast(BContent * content);
	
							// Put it before <before>, take ownership
	virtual status_t		AddChildBefore(BContent * content, BContent * before);
	
							// Put it after <after>, take ownership
	virtual status_t		AddChildAfter(BContent * content, BContent * after);
	
	// --- Removing Children --------------------------------
							// Does not delete it
	virtual status_t		RemoveChild(BContent * content);
	
	// --- Add Attributes -----------------------------------
	// The only way to find attributes is through their element.  Because they are
	// attached through a BXMLNamedSet, not through the BXMLNode linked list mechanism.
	
	// replace says what to do if it's already there:
	//     false: return an error if it already exists
	//     true:  replace it and return B_OK
	
							// The other add attributes call this one
	virtual status_t		AddAttribute(BAttribute * attribute, bool replace = false);
	
	virtual status_t		AddAttribute(const char * name, const char * val, bool replace = false);
	virtual status_t		AddAttribute(const char * name, const BString & val, bool replace = false);
	
	// --- Find Attributes ----------------------------------
	int32					CountAttributes() const;
	BAttribute				* AttributeAt(int32 index) const;
	
							// Is there an attribute that exactly matches attribute, but may or
							// may not actually be the same object.
	bool					FindAttribute(const BAttribute * attribute) const;
	
	// Find by name
	status_t				FindAttribute(const char * name, BAttribute ** attribute);
	status_t				FindAttribute(const char * name, const BAttribute ** attribute) const;
	status_t				FindAttribute(const char * name, const char ** val) const;
	status_t				FindAttribute(const char * name, BString * val) const;
	
	
	// These take a default argument, if it doesn't find the attribute, it gives
	// you the default and returns false.  If it does find it it gives it to you
	// and returns true.  Use these if you don't want to do error checking, and you
	// have a default value that you can use without a problem.
	//
	// Replaces the code:
	// int32 val;
	// if (element->FindAttribute("name", &val) != B_OK)
	//		val = 5;
	//
	// With:
	// element->FindAttribute("name", &val, 5);
	//

	bool					FindAttribute(const char * name, const char ** val, const char * def) const;
	bool					FindAttribute(const char * name, BString * val, const char * def) const;
	
	// --- Remove Attributes --------------------------------
							// Does not delete <attribute>
	virtual status_t		RemoveAttribute(BAttribute * attribute);
							// Deletes the internally stored BAttribute
	virtual status_t		RemoveAttribute(const char * name);
	
	// --- Some Interesting Functions -----------------------
							// Put this element into some normalized form.  TBD.
							// For example, coalesce adjacent text or CData nodes
	// virtual void			Normalize();
	
	
	// --- Value of the Element... --------------------------
	
	// If the data inside this element can be coalesced by removing comments and/or
	// processing instructions, and joining all text and CData, then these functions
	// can get the values of the result of that coalescing.  If not, return an error.
	// Could also possibly put some convience methods into here to make
	// it do some fun stuff with attributes elements.  
	// For example, to get a rect, look at the top, left, bottom, right attributes,
	// if they all exist and can be floating point numbers, then this function would
	// work.  More thought needed here
	//
	// This makes the following constructs simple to access, where they would be a
	// pain; but it's a fairly common piece of XML.
	//		<Element>42</Element>	gives: "42"
	//		<Element>Whoah <!-- Hey Dude -->Momma!</Element>    gives: "Whoah Momma!"
	//		<Element>Whoah<!-- Hey Dude -->Momma!</Element>    gives: "WhoahMomma!"
	
	status_t				GetData(BString * data) const;
	
private:
	
	BContent			* _firstChild;
	BContent			* _lastChild;
	BNamedSet			_attributes;
	BNamedSet			_namespaces;
};


// XML Valued
// =====================================================================
// BValued does not attempt to coalesce anything like BElement does --
// they don't have children.
class BValued
{
public:
	
	// --- Getting Value ------------------------------------
	status_t				GetValue(const char ** value) const;
	status_t				GetValue(BString * value) const;

							// This function gets what you would see if you
							// outputed to XML and read with StyledEdit.
	virtual void			GetValueRaw(BString & value) const;
	
	// --- Setting Value ------------------------------------
	virtual status_t		SetValue(const char * value);
	virtual status_t		SetValue(const char * value, int32 length);
	
							// Adopt takes the buffer from value, so there
							// is no copy.
	virtual status_t		SetValue(BString & value, bool adopt = false);

	// --- Useful Functions ---------------------------------
							// Start will become the index of the first char (-1 for end)
							// Len is the number of chars to replace/delete, etc. (-1 is all)
	virtual status_t		Append(const char * str);
	virtual status_t		Insert(const char * str, int32 start = -1);
	virtual status_t		Remove(int32 start = -1, int32 len = -1);
	virtual status_t		Replace(const char * str, int32 start = 0, int32 len = -1);
	
protected:
	// --- Validation ---------------------------------------
							// Hook function.  Easier than re-implementing
							// all of the virtual set functions
							// Check to see if the new value is okay
							// return fals if it's not
							// default impl returns B_OK to approve it
	virtual status_t		ValidateValueChange(BString & newVal);
	
public:
	explicit				BValued(const BValued & copy);
							BValued(const char * value);
							BValued(const BString & copy);
							BValued();
	virtual					~BValued();
	
protected:
	// Give access just for efficiency.
	// Do not change this value directly, because ValidateValueChange will not be called.
	BString					_value;
};


// XML Attribute
// =====================================================================
// Elements are stored as a set of attributes and an ordered set of children
// nodes.  This is the object that represents attributes.
// All the value methods come from the BXMLValued class
class BAttribute	: public BValued, public BNamed, public BXMLObject
{
public:
							BAttribute(const char * name);
							BAttribute(const char * name, const char * value);
							BAttribute(const BString & name);
							BAttribute(BString & name, bool adopt = false);
							BAttribute(const BAttribute & copy); // deep copy
	virtual					~BAttribute();
	virtual BAttribute		* Clone() const;
	
							// Properly esacpes meaningful xml characters
	virtual void			GetValueRaw(BString & value) const;
	
	virtual status_t		SetNamespace(BNamespace * space);
	using					BNamed::Namespace;
	
private:
							BAttribute();
};


// XML Processing Instruction
// =====================================================================
class BProcessingInstruction 	: public BValued, public BNamed, public BContent
{
public:
							BProcessingInstruction(const char * target);
							BProcessingInstruction(const char * name, const char * value);
							BProcessingInstruction(const BString & name);
							BProcessingInstruction(BString & name, bool adopt = false);
							BProcessingInstruction(const BProcessingInstruction & copy);	// deep copy
	
	virtual					~BProcessingInstruction();
	
	virtual BProcessingInstruction	* Clone() const;
	
	virtual void			GetValueRaw(BString & value) const;
private:
							BProcessingInstruction();
};


// XML Text
// =====================================================================
// Text is character data that might get escaped when written to XML.
class BText : public BContent, public BValued
{
public:
							BText();
							BText(const BText & copy);
							BText(const char * value);
							BText(const BString & value);
	virtual					~BText();
	virtual BContent		* Clone() const;
};


// XML CData
// =====================================================================
// CData is text that does not get escaped, except for one case. It
// gets surrounded by the strings "<![CDATA[" and "]]>".  So if the
// string "]]>" appears in the string will be escaped to "]]&gt;"
// as required by Section 2.4 of the XML spec.
// The only difference between BText and BCData is how it is displayed
// in XML
class BCData : public BContent, public BValued
{
public:
							BCData();
							BCData(const BCData & copy);
							BCData(const char * value);
							BCData(const BString & value);
	virtual					~BCData();
	virtual BContent		* Clone() const;
};


// XML Comment
// =====================================================================
// Remember, comments in XML may not contain the string "--"
class BComment : public BContent, public BValued
{
public:
							BComment();
							BComment(const BComment & copy);
							BComment(const char * value);
							BComment(const BString & value);
	virtual					~BComment();
	virtual BContent		* Clone() const;
	
							// Comments are escaped slightly differently
	virtual void			GetValueRaw(BString & value) const;
	
private:
							// some substrings are illegal in comments
	virtual status_t		ValidateValueChange(BString & newVal);
};


// XML Document
// =====================================================================
// This class represents the document as a whole. It looks like an
// element, but it's not.  The document element has its parent set to NULL
// If it's NULL, you can always access the document through the Document() function.
// It's also not derived from BContent
// The document does not derive from element, but it has a bunch of similar
// functions, but it is much more limiting, because what a document can
// contain directly is limited by the XML spec.
class BDocument : public BXMLObject
{
public:
	
	// --- Constructor and Destructor  ----------------------
							BDocument();
							BDocument(const BDocument & copy);
	virtual					~BDocument();
	
	
	// --- Getting Children ---------------------------------
							// Access to the first and last children for iteration
							// iteration over all of the children of any type.
							// With a mask, it will find the first that fits it
	BContent				* FirstChild();
	const BContent			* FirstChild() const;
	BContent				* FirstChild(uint32 typeMask);
	const BContent			* FirstChild(uint32 typeMask) const;
	BContent				* LastChild();
	const BContent			* LastChild() const;
	BContent				* LastChild(uint32 typeMask);
	const BContent			* LastChild(uint32 typeMask) const;

							// Access to some of the interesting children
	BDocumentType			* DocumentType();
	const BDocumentType		* DocumentType() const;
	BElement				* Element();
	const BElement			* Element() const;
	
	
	// --- Adding Children ----------------------------------
							// Make it the first one, take ownership of <content>
	virtual status_t		AddChildFirst(BContent * content);
							// Make it the last one, take ownership of <content>
	virtual status_t		AddChildLast(BContent * content);
							// Put it before <before>, take ownership of <content>
	virtual status_t		AddChildBefore(BContent * content, BContent * before);
							// Put it after <after>, take ownership of <content>
	virtual status_t		AddChildAfter(BContent * content, BContent * after);
	
	// --- Removing Children --------------------------------
							// Will not delete <content>
	virtual status_t		RemoveChild(BContent * content);
	

	// --- Some Interesting Functions -----------------------
							// If there is a DTD, set it back to the default provided by
							// the DTD.  If there isn't a DTD, empty it out.  Also deletes
							// objects.  No leakage.
	// virtual status_t		ResetAll();
	// virtual status_t		ResetAttributes();
	// virtual status_t		ResetChildren();
	
							// Put this element into some normalized form.  TBD.
							// For example, coalesce adjacent text or CData nodes
	// void					Normalize();
	
private:
	BContent		* _firstChild;
	BContent		* _lastChild;
	BElement		* _element;
	BDocumentType	* _documentType;
};


// XML Document Type
// =====================================================================
// This class holds everything about the DTD that is used for validating the document
// It's the class that encapsulates all the things that can go into a <!DOCTYPE ...> section
// - Note that XML Namespaces are handled by the BDocumentTypeDefinitions added to
//   BElements.
class BDocumentType : public BContent
{
public:
	const BDocumentTypeDefinition		* InternalSubset() const;
	BDocumentTypeDefinition				* InternalSubset();
	const BDocumentTypeDefinition		* ExternalSubset() const;
	BDocumentTypeDefinition				* ExternalSubset();
	
	status_t					SetInternalSubset(BDocumentTypeDefinition * dtd);
	status_t					SetExternalSubset(BDocumentTypeDefinition * dtd);
	
	const BElementDecl	* FindElementDecl(const char * name) const;
	BElementDecl		* FindElementDecl(const char * name);
	const BEntityDecl	* FindEntityDecl(const char * name, bool parameter) const;
	BEntityDecl			* FindEntityDecl(const char * name, bool parameter);
	
					BDocumentType();
	virtual			~BDocumentType();
	
private:
	BDocumentTypeDefinition		* _internalSubset;
	BDocumentTypeDefinition		* _externalSubset;
};


// XML Document Type Definition
// =====================================================================
// This holds the DTD for both internal, external, and namespace DTDs
// For external DTDs (indeed, all external entities, of which external
// DTDs are a subset), there are two ways of referencing them, system IDs
// and public IDs.  The  System IDs tell how to access the dtd file
// (e.g. a URL), publicIDs are guaranteed to be unique and stable.
class BDocumentTypeDefinition : public BXMLObject, public BNamed
{
public:
	// Element Declarations
	status_t			AddElementDecl(BElementDecl * decl);
	status_t			RemoveElementDecl(BElementDecl * decl);
	const BElementDecl	* FindElementDecl(const char * name) const;
	BElementDecl		* FindElementDecl(const char * name);
	int32				CountElementDecls() const;
	BElementDecl		* ElementDeclAt(int32 index);
	const BElementDecl	* ElementDeclAt(int32 index) const;
	
	status_t			AddAttributeDecl(BAttributeDecl * decl);
	
	// Entity Declarations
	status_t			AddEntityDecl(BEntityDecl * decl);
	status_t			RemoveEntityDecl(BEntityDecl * decl);
	BEntityDecl			* FindEntityDecl(const char * name, bool parameter);
	const BEntityDecl	* FindEntityDecl(const char * name, bool parameter) const;
	int32				CountEntityDecls() const;
	BEntityDecl			* EntityDeclAt(int32 index);
	const BEntityDecl	* EntityDeclAt(int32 index) const;
	
						BDocumentTypeDefinition(const char * name);
						BDocumentTypeDefinition(const BString & name);
	virtual				~BDocumentTypeDefinition();
	
	// ExternalID / InternalID
	const char *		PublicID() const;
	const char *		SystemID() const;
	void				SetPublicID(const char * id);
	void				SetSystemID(const char * id);
	
private:				
	BNamedSet	_elements;
	BNamedSet	_paramEntities;
	BNamedSet	_generalEntities;
	BString		_publicID;
	BString		_systemID;
};


// XML Element Declaration
// =====================================================================
// The structure that declares what is allowed in an element -- including
// what its children may be, which elements it has, etc.
// There is some pattern matching allowed, and this class includes
// member classes for the data structures for that.
class BElementDecl : public BXMLObject, public BNamed
{
public:
	
	// --- Constructor / Destructor / Etc. ------------------
								BElementDecl(const char * name);
								BElementDecl(const BString & name);
	virtual						~BElementDecl();
	
	virtual status_t			SetTo(BString & contentSpec);
	
	
	enum element_children_allowed
	{
		EMPTY,
		ANY,
		MIXED,
		CHILDREN
	};
	
	element_children_allowed	Mode() const;
	void						SetMode(element_children_allowed mode);
	
	
	// --- MIXED Mode ---------------------------------------
	status_t					AllowElement(const char * name);
	status_t					AllowElement(const char * name, int32 length);
	status_t					DisallowElement(const char * name);
	bool						IsElementAllowed(const char * name) const;	// false on error
	int32						CountAllowedElements() const;
	const BString 				& AllowedElementAt(int32 index) const;
	
	
	// --- CHILDREN Mode ------------------------------------
	typedef enum
	{
		ONE,					// 
		ONE_OR_MORE,			// +
		ZERO_OR_MORE,			// *
		ONE_OR_ZERO				// ?
	}node_cardinality;
	
	class List;
	
	struct ListNode
	{
		typedef enum {LIST, STRING, NO_INIT} Type;
		Type type;
		List * list;
		node_cardinality cardinality;
		BString element;
		ListNode();
		ListNode(Type atype, List * alist);
		~ListNode();
	};
	
	class List
	{
	public:
		typedef enum {SEQ, CHOICE} Type;
		Type type;
		void AddNode(ListNode * node);
		void AddNode(ListNode * node, int32 index);
		int32 CountItems() const;
		ListNode * NodeAt(int32 index);
		const ListNode * NodeAt(int32 index) const;
		void RemoveNode(ListNode * node);
		void RemoveNode(int32 index);
		void MakeEmpty();
		~List();
	private:
		BVector<void*> _list;
	};
	
	ListNode			* GetPattern();
	const ListNode		* GetPattern() const;
	static bool			AllChildrenOptional(const ListNode * node);
	
	// --- Attributes ---------------------------------------
	
	status_t				AddAttributeDecl(BAttributeDecl * decl);
	BAttributeDecl			* FindAttributeDecl(const char * name);
	const BAttributeDecl	* FindAttributeDecl(const char * name) const;
	int32					CountAttributeDecls() const;
	BAttributeDecl			* GetAttributeDecl(int32 index);
	const BAttributeDecl	* GetAttributeDecl(int32 index) const;
	status_t				RemoveAttributeDecl(BAttributeDecl * decl);
	
private:
	
	status_t	handle_mixed_content_spec(BString & contentSpec);
	status_t	handle_element_content_spec(BString & contentSpec);
	status_t	handle_element_pattern(const char *& p, List * into, node_cardinality & cardin);
	
	// General Info
	element_children_allowed	_mode;
	
	// For MIXED mode
	BOrderedVector<BString>		_mixed_AllowedChildren;
	
	// For CHILDREN mode
	ListNode					_children_pattern;
	
	// Attributes
	BNamedSet					_attributes;
};


// XML Attribute Declaration
// =====================================================================
// This class contains all the validation information about attributes.
// There are a ton of options.  This section of the spec is pretty clear.
// The validation doesn't check all of the types, but it will parse and
// properly output everything.
class BAttributeDecl : public BXMLObject, public BNamed
{
public:
	// A Be type code
	void				SetType(uint32 type);
	uint32				GetType() const;
	
	xml_attr_behavior	GetBehavior() const;
	void				SetBehavior(xml_attr_behavior behavior);
	
	// If it's type is enumeration
	status_t			AddEnumValue(const char * val);
	status_t			RemoveEnumValue(const char * val);
	int32				CountEnumValues() const;
	const char			* GetEnumValue(int32 index) const;
	bool				IsEnumValue(const char * val) const;
	
						// A string in the form "a|b|c"
	status_t			SetEnumValues(BString & str);
	
	// If it's behavior is "default" or "fixed"
	void				SetDefault(const char * value);
	const BString &		GetDefault() const;
	
	// Constructor
						BAttributeDecl(const char * name);
						BAttributeDecl(const BAttributeDecl & copy);
						BAttributeDecl(const BString & name);
	virtual				~BAttributeDecl();
	BAttributeDecl		& operator=(const BAttributeDecl & copy);
	
	const BString &		GetElement() const;
	void				SetElement(const char * element);
	
private:
	BString						_element;	// I would really like to not store this.
	uint32						_typeCode;	// Be type code
	BOrderedVector<BString>		_enumVals;
	BString						_defaultVal;
	xml_attr_behavior			_behavior;
};


// XML Entity Declaration
// =====================================================================
// Go try and figure out the XML spec.  I dare you.  Basically entities are
// replacement text, but there are a million and one ways of declaring them, 
// and finding out where they come from.  The XML kit supports most of it,
// but not all.
class BEntityDecl : public BXMLObject, public BNamed
{
public:
						BEntityDecl(const char		* name);
	virtual				~BEntityDecl();
	
	using BNamed::SetName;
	
	void				SetType(xml_entity_type type);
	xml_entity_type		GetType() const;
	
	void				SetScope(xml_entity_scope scope);
	xml_entity_scope	GetScope() const;
	
	void				SetStorage(xml_entity_storage storage);
	xml_entity_storage	GetStorage() const;
	
	const BString				& GetReplacementText();
	BVector<BXMLObject *>		& GetReplacementContent();
	const BVector<BXMLObject *>	& GetReplacementContent() const;
	
	status_t			SetValue(const BString & str, BXMLObjectFactory * factory,
								uint32 parseFlags, BDocumentType * dt,
								BEntityStore * entityStore);

										// Will take the string data
	status_t			SetValueAdopt(BString & str, BXMLObjectFactory * factory,
								uint32 parseFlags, BDocumentType * dt,
								BEntityStore * entityStore);

										// Will not parse anything.  Will adopt the string.
	status_t			SetValueText(BString & str, BXMLObjectFactory * factory);
	
	void				AddItem(BXMLObject * object);
	
private:
	xml_entity_type			_type;
	xml_entity_scope		_scope;
	xml_entity_storage		_storage;
	BString					_replacementText;
	BVector<BXMLObject *>	_objects;
};


}; // namespace XML
}; // namespace B


#endif // _B_XML2_CONTENT_H
