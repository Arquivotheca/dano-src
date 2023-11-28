#ifndef _B_XML_PARSER_H
#define _B_XML_PARSER_H

#include <xml/BXMLDefs.h>
#include <xml/BContent.h>
#include <xml/BDataSource.h>

#include <SmartArray.h>

// Forward References not in namespace B::XML
// =====================================================================
class BDataIO;

namespace B {
namespace XML {

// Forward References in namespace B::XML
// =====================================================================
class BXMLObjectFactory;
class BXMLParseContext;
class BXMLDocumentCreationContext;
class BXMLDocumentParseContext;
class BParser;
class BDocumentParser;
class BXMLDataSource;
class BDocument;

// ParseXML -- Do some parsing.
// =====================================================================
status_t	ParseXML(BDocument * document, BDataIO * dataIO, uint32 flags = 0);
status_t	ParseXML(BDocument * document, const char * data, int32 length = -1, uint32 flags = 0);
status_t	ParseXML(BDocument * document, BXMLDataSource * data, uint32 flags = 0);
status_t	ParseXML(BXMLParseContext * context, BXMLDataSource * data, uint32 flags = 0);


// BXMLObjectFactory
// =====================================================================
// Called by DocumentCreationContexts to create the objects.  If you require
// subclasses of any of the BContent derivatives, then you can override this
// and create them yourself.
// For any of the non-const BString references, you're allowed to use to
// Adopt function to take ownership of the string in your implementations.
class BXMLObjectFactory
{
public:
	// The default implementation returns new allocated standard
	// Be objects, without any fancy subclassing
	virtual BNamespace				* NamespaceFactory(BString & prefix, BString & value,
											bool getDTD, BEntityStore * store,
											uint32 parseFlags);
	virtual BElement				* ElementFactory(BString & name, BNamespace * space);
	virtual BAttribute				* AttributeFactory(BString & name, BNamespace * space);
	virtual BText					* TextFactory();
	virtual BCData					* CDataFactory();
	virtual BComment				* CommentFactory();
	virtual BProcessingInstruction	* PIFactory(BString & target);
	virtual BDocumentType			* DocumentTypeFactory();
	virtual BEntityDecl				* EntityDeclFactory(const char * name);
	virtual BElementDecl			* ElementDeclFactory(const char * name);
	virtual BAttributeDecl			* AttributeDeclFactory(BString & element,
											BString & name, uint32 type,
											BString & enumVals, xml_attr_behavior behave,
											BString & defaultVal);
	virtual	BDocumentTypeDefinition	* DTDFactory(BString & name);
	
	virtual ~BXMLObjectFactory();
};


// BXMLParseContext -- Hook Functions for what the parser encounters
// =====================================================================
class BXMLParseContext
{
public:
				BXMLParseContext();
	virtual		~BXMLParseContext();
	
	// These fields are always set to the current line and column (or as
	// close of an approximation as possible).  They are probably most
	// useful for error messages.
	int32 line;
	int32 column;
	
						// Called at the beginning of the document parsing process.
	virtual status_t	OnDocumentBegin(uint32 flags);
	
						// Called at the end of the document parsing process.
						// If you get this call, then you know that parsing was successful.
	virtual status_t	OnDocumentEnd();
	
	// The following functions are fairly self explanitory.
	// Whenever there's a BString that isn't const, you are allowed to use the
	// BString::Adopt function to take the string buffer, and leave the original
	// string empty.  This is just a performance optimization.
	
	virtual status_t	OnStartTag(				BString		& name,
												BStringMap	& attributes		);
									
	virtual status_t	OnEndTag(				BString		& name				);
	
	virtual status_t	OnTextData(				const char	* data,
												int32		size				);
	
	virtual status_t	OnCData(				const char	* data,
												int32		size				);
	
	virtual status_t	OnComment(				const char	* data,
												int32		size				);
	
	virtual status_t	OnDocumentTypeBegin(	BString		& name				);
	
	virtual status_t	OnExternalSubset(		BString		& publicID,
												BString		& systemID,
												uint32 		parseFlags			);
	
	virtual status_t	OnInternalSubsetBegin();
	
	virtual status_t	OnInternalSubsetEnd();
	
	virtual status_t	OnDocumentTypeEnd();
	
	virtual status_t	OnProcessingInstruction(BString		& target,
												BString		& data				);
	
	virtual	status_t	OnElementDecl(			BString		& name,
												BString		& data				);
	
	virtual status_t	OnAttributeDecl(		BString		& element,
												BString		& name,
												BString		& type,
												BString		& behavior,
												BString		& defaultVal		);
	
	virtual status_t	OnInternalParsedEntityDecl(	BString	& name,
													BString & internalData,
													bool	parameter,
													uint32	parseFlags			);
	
	virtual status_t	OnExternalParsedEntityDecl(	BString	& name,
													BString & publicID,
													BString & systemID,
													bool	 parameter,
													uint32	parseFlags			);
	
	virtual status_t	OnUnparsedEntityDecl(		BString	& name,
													BString & publicID,
													BString & systemID,
													BString & notation			);
	
	virtual status_t	OnNotationDecl(				BString	& name,
													BString	& value				);
	
						// This is a hook function to notify the subclass that we
						// encountered a PE in a text section.  Subclasses might
						// either look up replacement text and insert it, or look
						// parsed objects and insert them.
	virtual status_t	OnGeneralParsedEntityRef(	BString	& name				);
	
						// This is a hook function to find out the replacement text
						// for a general entity when it occurs in an attribute.  The
						// value is then substituted into the attribute as if it
						// had never been there.  If you want this behavior, you must
						// set the B_XML_HANDLE_ATTRIBUTE_ENTITIES flag.
	virtual status_t	OnGeneralParsedEntityRef(	BString	& name,
													BString & replacement		);
	
						// This is a hook function to notify the subclass when an 
						// entity occurred in the DTD, but in a place where it would
						// be better for the subclass to just insert its objects into
						// the stream than to send back the replacement text, and 
						// have this parser have to reparse it each time it occurs.
	virtual status_t	OnParameterEntityRef(		BString	& name				);
	
						// This is a hook function to find the replacement text for
						// a parameter entity.  It will then be parsed, and the normal
						// other functions will be called.
	virtual status_t	OnParameterEntityRef(		BString	& name,
													BString & replacement		);

						// An error occurred.  If you would like to ignore the error,
						// and continue processing, then return B_OK from this
						// function, and processing will continue.
						// If fatal is true, a fatal error occurred, and we're not
						// going to be able to recover, no matter what you return.
						// debugLineNo is the line number from which OnError was called
						// XXX Should debugLineNo be made public?
						// The code and data parameters are currently unused.  I have
						// visions of using them to help recover from errors 
						// (for example, pass in potentially corrupted structures, and
						// allow the OnError function to have a go at correcting them)
	virtual status_t	OnError(status_t error, bool fatal, int32 debugLineNo,
									uint32 code = 0, void * data = NULL);

};



// BXMLDocumentCreationContext
// =====================================================================
// This is a one-use-only object.  Never directly instantiate this class.
// It's a base class for DocumentParseContext and EntityParseContext. It's
// purpose is to create elements, and stuff, but just pass them on the
// their subclass when done.
// --> BXMLDocumentParseContext is the one that
//     adds the elements that are created here to their parents, etc.
// --> Note the naming differences.  This class does OnBeginXXX where
//     BXMLParseContext does OnXXXBegin.
// --> This is the class that does the namespaces, if the proper flag
//     (B_XML_HANDLE_NAMESPACES) is set
class BXMLDocumentCreationContext : public BXMLParseContext
{
public:
				BXMLDocumentCreationContext(bool inDTD, // true only for parameter entities
											BXMLObjectFactory * factory = NULL,
											BEntityStore * entityStore = NULL);
	virtual		~BXMLDocumentCreationContext();
	
	// Hook functions
	virtual status_t	OnBeginDocumentNode() = 0;
	virtual status_t	OnEndDocumentNode() = 0;
	virtual status_t	OnStartTagNode(BElement * element) = 0;
	virtual status_t	OnEndTagNode(BElement * element) = 0;
	virtual status_t	OnTextNode(BText * text) = 0;
	virtual status_t	OnCDataNode(BCData * cData) = 0;
	virtual status_t	OnCommentNode(BComment * comment) = 0;
	virtual status_t	OnBeginDocumentTypeNode(BDocumentType * dt) = 0;
	virtual status_t	OnExternalSubsetNode(BDocumentTypeDefinition * dtd) = 0;
	virtual status_t	OnBeginInternalSubsetNode(BDocumentTypeDefinition * dtd) = 0;
	virtual status_t	OnEndInternalSubsetNode() = 0;
	virtual status_t	OnEndDocumentTypeNode() = 0;
	virtual status_t	OnProcessingInstructionNode(BProcessingInstruction * pi) = 0;
	virtual	status_t	OnElementDeclNode(BElementDecl * decl) = 0;
	virtual status_t	OnAttributeDeclNode(BAttributeDecl * decl) = 0;
	virtual status_t	OnEntityDeclNode(BEntityDecl * decl) = 0;
	//virtual status_t	OnNotationDeclNode(BNotationDecl * decl) = 0;	XXX Not implemented
	
	virtual BDocumentType * GetDocumentType() = 0;
	
protected:
	BElement			* _currentElement;
	uint32				_flags;
	BXMLObjectFactory	* _factory;
	BEntityStore		* _entityStore;
	
private:
	bool				_inDTD;
	bool				_deleteEntityStoreWhenDone;
	
public:
	// Implementations of BXMLParseContext functions
	virtual status_t	OnDocumentBegin(uint32 flags);
	virtual status_t	OnDocumentEnd();
	virtual status_t	OnStartTag(BString & name, BStringMap & attributes);
	virtual status_t	OnEndTag(BString & name);
	virtual status_t	OnTextData(const char * data, int32 size);
	virtual status_t	OnCData(const char * data, int32 size);
	virtual status_t	OnComment(const char * data, int32 size);
	virtual status_t	OnDocumentTypeBegin(BString & name);
	virtual status_t	OnExternalSubset(BString & publicID, BString & systemID,
												uint32 parseFlags);
	virtual status_t	OnInternalSubsetBegin();
	virtual status_t	OnInternalSubsetEnd();
	virtual status_t	OnDocumentTypeEnd();
	virtual status_t	OnProcessingInstruction(BString & target, BString & data);
	virtual	status_t	OnElementDecl(BString & name, BString & data);
	virtual status_t	OnAttributeDecl(BString & element, BString & name,
										BString & type, BString & behavior,
										BString & defaultVal);
	virtual status_t	OnInternalParsedEntityDecl(BString & name, BString & internalData,
													bool parameter, uint32	parseFlags);
	virtual status_t	OnExternalParsedEntityDecl(BString & name, BString & publicID,
													BString & systemID, bool parameter,
													uint32	parseFlags);
	virtual status_t	OnUnparsedEntityDecl(BString & name, BString & publicID,
												BString & systemID, BString & notation);
	virtual status_t	OnNotationDecl(BString & name, BString	& value);
};



// BXMLDocumentParseContext
// =====================================================================
// This is a one-use-only object.  
class BXMLDocumentParseContext : public BXMLDocumentCreationContext 
{
public:
				BXMLDocumentParseContext(BDocument * document,
											BXMLObjectFactory * factory = NULL,
											BEntityStore * entityStore = NULL);
	virtual		~BXMLDocumentParseContext();
	
private:
	BDocument * _document;

public:
	// Implementations of BXMLDocumentCreationContext hook functions
	virtual status_t	OnBeginDocumentNode();
	virtual status_t	OnEndDocumentNode();
	virtual status_t	OnStartTagNode(BElement * element);
	virtual status_t	OnEndTagNode(BElement * element);
	virtual status_t	OnTextNode(BText * text);
	virtual status_t	OnCDataNode(BCData * cData);
	virtual status_t	OnCommentNode(BComment * comment);
	virtual status_t	OnBeginDocumentTypeNode(BDocumentType * dt);
	virtual status_t	OnExternalSubsetNode(BDocumentTypeDefinition * dtd);
	virtual status_t	OnBeginInternalSubsetNode(BDocumentTypeDefinition * dtd);
	virtual status_t	OnEndInternalSubsetNode();
	virtual status_t	OnEndDocumentTypeNode();
	virtual status_t	OnProcessingInstructionNode(BProcessingInstruction * pi);
	virtual	status_t	OnElementDeclNode(BElementDecl * decl);
	virtual status_t	OnAttributeDeclNode(BAttributeDecl * decl);
	virtual status_t	OnEntityDeclNode(BEntityDecl * decl);
	//virtual status_t	OnNotationDeclNode(BNotationDecl * decl);	XXX Not implemented
	
	virtual BDocumentType *	GetDocumentType();
	
	// Implementations of BXMLParseContext functions
	virtual status_t	OnGeneralParsedEntityRef(BString & name);
	virtual status_t	OnGeneralParsedEntityRef(BString & name, BString & replacement);
	virtual status_t	OnParameterEntityRef(BString & name);
	virtual status_t	OnParameterEntityRef(BString & name, BString & replacement);
};

// BParser
// =====================================================================
// Validate the document while you parse.
// Produce an BContent object tree when done.
class BXMLValidatingContext : public BXMLDocumentParseContext
{
public:
	BXMLValidatingContext(BDocument * document,
							BXMLObjectFactory * factory = NULL,
							BEntityStore * entityStore = NULL);
	~BXMLValidatingContext();
	
	
private:
	BDocumentType	* _dt;
	BList 			_elementTrackerStack;
	
	
	struct _ElementChildrenTracker_
	{
		_ElementChildrenTracker_(BElementDecl * elementDecl);
		BElementDecl * decl;
		const void * data;
		bool dataFromAboveInStack;
		uint16 state;
	};

	status_t		check_attr_type(BAttribute * attr, const BAttributeDecl * attrDecl);
	status_t		check_next_element(const char * element);
	void			push_element_stack(BElementDecl * decl);
	status_t		pop_element_stack();
	_ElementChildrenTracker_ * element_stack_top();
	status_t		is_text_value_allowed(BString & value, bool & allow);
	status_t		is_element_declared_somewhere(BElement * element, BElementDecl ** decl);
	
public:
	// Implementations of BXMLDocumentCreationContext functions
	//virtual status_t	OnBeginDocumentNode();
	//virtual status_t	OnEndDocumentNode();
	virtual status_t	OnStartTagNode(BElement * element);
	virtual status_t	OnEndTagNode(BElement * element);
	virtual status_t	OnTextNode(BText * text);
	virtual status_t	OnCDataNode(BCData * cData);
	//virtual status_t	OnCommentNode(BComment * comment);
	virtual status_t	OnBeginDocumentTypeNode(BDocumentType * dt);
	//virtual status_t	OnExternalSubsetNode(BDocumentTypeDefinition * dtd);
	//virtual status_t	OnBeginInternalSubsetNode(BDocumentTypeDefinition * dtd);
	//virtual status_t	OnEndInternalSubsetNode();
	virtual status_t	OnEndDocumentTypeNode();
	//virtual status_t	OnProcessingInstructionNode(BProcessingInstruction * pi);
	//virtual	status_t	OnElementDeclNode(BElementDecl * decl);
	//virtual status_t	OnAttributeDeclNode(BAttributeDecl * decl);
	//virtual status_t	OnEntityDeclNode(BEntityDecl * decl);
	//virtual status_t	OnNotationDeclNode(BNotationDecl * decl);	XXX Not implemented
	
	//virtual BDocumentType *	GetDocumentType();
	
	// Implementations of BXMLParseContext functions
	//virtual status_t	OnGeneralParsedEntityRef(BString & name);
	//virtual status_t	OnGeneralParsedEntityRef(BString & name, BString & replacement);
	//virtual status_t	OnParameterEntityRef(BString & name);
	//virtual status_t	OnParameterEntityRef(BString & name, BString & replacement);
	
};


// BParser
// =====================================================================
// You can make lots of different subclasses of BParser, and then use them
// to create other non XML kit objects based on whatever XML comes along.
// Then you can pass a new parser back from the StartTag section for it to
// use inside there.
class BParser
{
public:
						BParser();
	virtual				~BParser();
	
	virtual status_t	StartTag(BString &name, BStringMap &attributes, BParser **newParser);
	virtual status_t	EndTag(BString &name);
	virtual status_t	TextData(const char	* data, int32 size);
};

// BDocumentParser
// =====================================================================
// BDocumentParser does the XML side of the BParser parsing.  It calls the
// correct methods on the BParser implementations you pass in.
// It's a ParseContext, so you can pass it to any of the ParseXML functions
// that take ParseContexts
class BDocumentParser : public BXMLParseContext
{
public:
							// Pass in a BParser to start with.  
							BDocumentParser(BParser *root);
	virtual					~BDocumentParser();

	virtual status_t		OnDocumentBegin(uint32 flags);
	virtual status_t		OnDocumentEnd();

	virtual status_t		OnStartTag(BString &name, BStringMap &attributes);
	virtual status_t		OnEndTag(BString &name);
	virtual status_t		OnTextData(const char *data, int32 size);
	virtual status_t		OnCData(const char *data, int32 size);

	virtual status_t		OnGeneralParsedEntityRef(BString &name);
	virtual status_t		OnGeneralParsedEntityRef(BString &name, BString &replacement);

private:

	struct parser_rec
	{
		BParser *parser;
		int32 depth;
		
		parser_rec() { parser = NULL; depth = 0; };
		~parser_rec() { if (parser) delete parser; };
	};

	uint32					m_flags;
	SmartArray<parser_rec>	m_stack;
};


// XML Entity Store
// =====================================================================
// When given a publicID, BEntityStore will look in the 
class BEntityStore
{
public:
	// Constructor / Destructor
					BEntityStore(uint32 parseFlags);
	virtual			~BEntityStore();
	
	// Fetch will go get it if necessary
	status_t		FetchExternalDTD(const BString & publicID, const BString & systemID,
							BXMLObjectFactory * factory, BDocumentType * dt,
							uint32 parseFlags, BDocumentTypeDefinition * dtd);
	status_t		FetchEntity(const BString & publicID, const BString & systemID,
							BEntityDecl ** returnDecl, bool parameter,
							BXMLObjectFactory * factory, uint32 parseFlags,
							BDocumentType * dt);
	
	// XXX Namespace parsing doesn't go through the DTD, it goes through this.
	// This needs to be rethought some, but I'm on plane right now, and I'd
	// like to get something working by the time it lands.
	// So the way this will work is that you can ask for it, and release it
	// when you're done.  If BEntityStore ever gets connected to the protocol
	// plugins and the resource cache, this will be better.  Aargh.
	status_t		GetNamespaceDTD(const BString & nsName,
							BXMLObjectFactory * factory, uint32 parseFlags,
							BDocumentTypeDefinition ** dtd);
	
private:
	status_t	go_get_document(const BString & publicID, const BString & systemID,
								BDataIO ** dataIO);
	static status_t	try_public_id(const BString & publicID, BDataIO ** dataIO);
	static status_t	try_system_id(const BString & systemID, BDataIO ** dataIO);
	
	uint32 _parseFlags;
};


}; // namespace XML
}; // namespace B


#endif // _B_XML_PARSER_H
