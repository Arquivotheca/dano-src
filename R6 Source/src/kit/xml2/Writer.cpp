
#include <xml2/BContent.h>
#include <xml2/BWriter.h>

namespace B {
namespace XML {

#define WriteString(str) err = m_stream->Write((str), strlen((str))); if (err < 0) return err;
#define WriteStringLength(str, len) err = m_stream->Write((str), (len)); if (err < 0) return err;
#define WriteData(str, len) err = write_xml_data(m_stream, (str), (len)); if (err != B_OK) return err;

status_t write_xml_data(IByteOutput::arg stream, const char *data, int32 size);


// =====================================================================
static void
print_cardinality(BString & str, BElementDecl::node_cardinality c)
{
	switch (c)
	{
		case BElementDecl::ONE_OR_MORE:
			str += '+';
			break;
		case BElementDecl::ZERO_OR_MORE:
			str += '*';
			break;
		case BElementDecl::ONE_OR_ZERO:
			str += '?';
			break;
		default:
			;
	};
}


// =====================================================================
static void
print_children_list(BString & str, BElementDecl::List * list)
{
	char sep;
	if (list->type == BElementDecl::List::SEQ)
		sep = ',';
	else
		sep = '|';
	
	str += '(';
	int32 count = list->CountItems();
	for (int32 i=0; i<count; ++i)
	{
		if (i != 0)
			str += sep;
		
		BElementDecl::ListNode * node = list->NodeAt(i);
		
		if (node->type == BElementDecl::ListNode::LIST)
			print_children_list(str, node->list);
		else
			str += node->element;
		print_cardinality(str, node->cardinality);
	}
	str += ')';
}




// =====================================================================
BWriter::BWriter(IByteOutput::arg data, uint32 formattingStyle)
	:m_stream(data),
	 m_elementStack(),
	 m_formattingStyle(formattingStyle),
	 m_openStartTag(false),
	 m_doneDOCTYPE(false),
	 m_startedInternalDTD(false),
	 m_depth(0),
	 m_lastPrettyDepth(0)
{
}


// =====================================================================
BWriter::~BWriter()
{
	
}


// =====================================================================
status_t
BWriter::BeginDoctype(const BString & elementName, const BString & systemID, const BString & publicID)
{
	status_t err;
	
	WriteString("<!DOCTYPE ");
	WriteStringLength(elementName.String(), elementName.Length());
	
	if (publicID != "") {
		WriteString(" PUBLIC \"");
		WriteStringLength(publicID.String(), publicID.Length());
		WriteString("\" \"");
		WriteStringLength(systemID.String(), systemID.Length());
		WriteString("\"");
	} else if (systemID != "") {
		WriteString(" SYSTEM \"");
		WriteStringLength(systemID.String(), systemID.Length());
		WriteString("\"");
	}
	
	m_doneDOCTYPE = true;
	m_startedInternalDTD = false;
	
	return B_OK;
}


// =====================================================================
status_t
BWriter::EndDoctype()
{
	status_t err;
	
	if (m_startedInternalDTD) {
		WriteString("\n]>\n\n");
	} else {
		WriteString(">\n\n");
	}
	
	m_startedInternalDTD = false;
	m_doneDOCTYPE = false;
	
	return B_OK;
}


// =====================================================================
status_t
BWriter::OnElementDecl(const BElementDecl * decl)
{
	(void) decl;
	
	status_t err;
	int32 count, i;
	BString str;
	
	err = open_doctype();
	if (err != B_OK)
		return err;
	
	WriteString("\n\t<!ELEMENT ");
	WriteString(decl->Name());
	
	switch(decl->Mode())
	{
		case BElementDecl::EMPTY:
			str = " EMPTY";
			break;
		case BElementDecl::ANY:
			str = " ANY";
			break;
		case BElementDecl::MIXED:
			str = " (#PCDATA";
			count = decl->CountAllowedElements();
			for (i=0; i<count; ++i) {
				str += '|';
				str += decl->AllowedElementAt(i);
			}
			str += ")";
			break;
		case BElementDecl::CHILDREN:
			str += ' ';
			print_children_list(str, decl->GetPattern()->list);
			print_cardinality(str, decl->GetPattern()->cardinality);
			break;
	}
	str += '>';
	WriteStringLength(str.String(), str.Length());
	
	count = decl->CountAttributeDecls();
	if (count > 0) {
		WriteString("\n\t<!ATTLIST ");
		WriteString(decl->Name());
	}	
	for (i=0; i<count; i++) {
		const BAttributeDecl * attr = decl->GetAttributeDecl(i);
		
		WriteString("\n\t\t");
		WriteString(attr->Name());
		WriteString("\t");
		
		BString str;
		
		switch (attr->GetType())
		{
			case B_STRING_TYPE:
				str = "CDATA\t";
				break;
			case B_ENUM_TYPE:
			{
				str += '(';
				int32 enumCount = attr->CountEnumValues();
				for (int32 k=0; k<enumCount; k++) {
					str += attr->GetEnumValue(k);
					if (k < enumCount-1)
						str += '|';
				}
				str += ")\t";
			}
			break;
			case B_ID_TYPE:
				str += "ID\t";
				break;
			case B_IDREF_TYPE:
				str += "IDREF\t";
				break;
			case B_ENTITY_TYPE:
				str += "ENTITY\t";
				break;
			case B_NMTOKEN_TYPE:
				str += "NMTOKEN\t";
				break;
			case B_IDREFS_TYPE:
				str += "IDREFS\t";
				break;
			case B_ENTITIES_TYPE:
				str += "ENTITIES\t";
				break;
			case B_NMTOKENS_TYPE:
				str += "NMTOKENS\t";
				break;
			default:
				str += "**UNKNOWN**\t";
		}
		WriteStringLength(str.String(), str.Length());
		
		switch (attr->GetBehavior())
		{
			case B_XML_ATTRIBUTE_REQUIRED:
				str = "#REQUIRED";
				break;
			case B_XML_ATTRIBUTE_IMPLIED:
				str = "#IMPLIED";
				break;
			case B_XML_ATTRIBUTE_FIXED:
				str = "#FIXED ";
				// Fall Through
			case B_XML_ATTRIBUTE_DEFAULT:
				str = "\"";
				str += attr->GetDefault();
				str += "\"";
				break;
			case B_XML_ATTRIBUTE_OPTIONAL:
				str = "\"\"";
				break;
		}
		WriteStringLength(str.String(), str.Length());
		
	}
	if (count > 0) {
		WriteString("\n\t\t>");
	}
	return B_OK;
}


// =====================================================================
status_t
BWriter::OnEntityDecl(const BEntityDecl * decl)
{
	(void) decl;
	
	status_t err;
	
	err = open_doctype();
	if (err != B_OK)
		return err;
	
	
	return B_OK;
}


// =====================================================================
status_t
BWriter::StartTag(const BString &name, const BValue &attributes, uint32 formattingHints)
{
	status_t err;
	void * cookie;
	BValue key, value;
	
	if (m_openStartTag) {
		WriteString(">");
		m_openStartTag = false;
	}
	
	err = indent();
	if (err != B_OK)
		return err;
	
	WriteString("<");
	WriteStringLength(name.String(), name.Length());
	
	cookie = NULL;
	while (B_OK == attributes.GetNextItem(&cookie, &key, &value))
	{
		BString n = key.AsString();
		BString v = value.AsString();
		WriteString(" ");
		WriteStringLength(n.String(), n.Length());
		WriteString("=\"");
		WriteData(v.String(), v.Length());
		WriteString("\"");
	}
	
	// If we are currently adding whitespace, and we're going to be inside this one,
	// then increment perty depth
	if ((m_lastPrettyDepth == m_depth) && !(formattingHints & NO_EXTRA_WHITESPACE))
		m_lastPrettyDepth = m_depth+1;
		
	// m_depth tells us how far in we really are.
	m_depth++;
	
	m_openStartTag = true;
	m_elementStack.AddItem(name);
	return B_OK;
}


// =====================================================================
status_t
BWriter::EndTag()
{
	status_t err;
	
	if (m_elementStack.CountItems() <= 0)
		return B_BAD_INDEX;
	
	// This checks one level deeper than indent() will print, which is right
	bool noExtraWhitespace = !(m_lastPrettyDepth == m_depth);
	m_depth--;
	if (m_lastPrettyDepth > m_depth) m_lastPrettyDepth = m_depth;
	
	if (m_openStartTag) {
		WriteString(" />");
		m_openStartTag = false;
		m_elementStack.RemoveItemsAt(m_elementStack.CountItems()-1);
		return B_OK;
	}
	
	if (!noExtraWhitespace) {
		err = indent();
		if (err != B_OK)
			return err;
	}
	
	const BString & name = m_elementStack[m_elementStack.CountItems()-1];
	WriteString("</");
	WriteStringLength(name.String(), name.Length());
	WriteString(">");
	
	m_elementStack.RemoveItemsAt(m_elementStack.CountItems()-1);
	
	return B_OK;
}


// =====================================================================
status_t
BWriter::TextData(const char	* data, int32 size)
{
	status_t err;
	
	if (m_openStartTag) {
		WriteString(">");
		m_openStartTag = false;
	}
	
	err = indent();
	if (err != B_OK)
		return err;
	
	WriteData(data, size);
	
	return B_OK;
}


// =====================================================================
status_t
BWriter::CData(const char	* data, int32 size)
{
	status_t err;
	
	if (m_openStartTag) {
		WriteString(">");
		m_openStartTag = false;
	}
	
	err = indent();
	if (err != B_OK)
		return err;
	
	WriteString("<![CDATA[");
	WriteData(data, size);
	WriteString("]]>");
	
	return B_OK;
}


// =====================================================================
status_t
BWriter::Comment(const char *data, int32 size)
{
	status_t err;
	
	if (m_openStartTag) {
		WriteString(">");
		m_openStartTag = false;
	}
	
	err = indent();
	if (err != B_OK)
		return err;
	
	WriteString("<!--");
	WriteStringLength(data, size);
	WriteString("-->");
	
	return B_OK;
}


// =====================================================================
status_t
BWriter::ProcessingInstruction(const BString & target, const BString & data)
{
	status_t err;
	
	if (m_openStartTag) {
		WriteString(">");
		m_openStartTag = false;
	}
	
	err = indent();
	if (err != B_OK)
		return err;
	
	WriteString("<?");
	WriteStringLength(target.String(), target.Length());
	WriteString(" ");
	WriteStringLength(data.String(), data.Length());
	WriteString("?>");
	
	return B_OK;
}


// =====================================================================
status_t
BWriter::open_doctype()
{
	status_t err;
	if (!(m_formattingStyle & SKIP_DOCTYPE_OPENER)) {
		if (!m_doneDOCTYPE) {
			WriteString("<!DOCTYPE [");
			m_doneDOCTYPE = true;
			m_startedInternalDTD = true;
		} else if (!m_startedInternalDTD) {
			WriteString(" [");
			m_startedInternalDTD = true;
		}
	}	
	return B_OK;
}


// =====================================================================
const char *kIndents	= "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
const int32 kTabCount	= 16;
status_t
BWriter::indent()
{
	status_t err;
	
	// if they're different, then we're in a NO_EXTRA_WHITESPACE section
	if (m_formattingStyle & BALANCE_WHITESPACE
			&& m_lastPrettyDepth == m_depth) {
		const char *s = kIndents;
		int32 newlineOnce=1,size,depth = m_depth;
		do {
			size = depth;
			if (size > kTabCount) size = kTabCount;
			depth -= size;
			WriteStringLength(s, size+newlineOnce);
			s += newlineOnce;
			newlineOnce = 0;
		} while (depth);
	}
	
	return B_OK;
}


// =====================================================================
static status_t
write_xml_recursive(const BXMLObject * object, BWriter * writer)
{
	status_t err;
	int32 count, i;
	BString nm, name, value;
	
	const BElement * e = dynamic_cast<const BElement *>(object);
	if (e)
	{
		{
			BValue stringMap;
			
			// Namespace Attributes
			count = e->CountNamespaces();
			for (i=0; i<count; i++)
			{
				const BNamespace * ns = e->NamespaceAt(i);
				nm = ns->Name();
				if (nm.Length() == 0)
					name = "xmlns";
				else
				{
					name = "xmlns:";
					name += nm;
				}
				value = ns->Value();
				stringMap.Overlay(name, value);
			}
			
			// Real Attributes
			count = e->CountAttributes();
			for (int i=0; i<count; ++i)
			{
				BAttribute * attr = e->AttributeAt(i);
				name = "";
				if (attr->Namespace())
				{
					name = attr->Namespace()->Name();
					name += ":";
				}
				name += attr->Name();
				attr->GetValue(&value);
				stringMap.Overlay(name, value);
			}
			
			
			if (e->Namespace() && strcmp(e->Namespace()->Name(), "")) {
				name = e->Namespace()->Name();
				name += ":";
				name += e->Name();
			} else {
				name = e->Name();
			}
			
			writer->StartTag(name, stringMap, 0);
		}
		
		const BContent * childContent = e->FirstChild();
		while (childContent)
		{
			write_xml_recursive(childContent, writer);
			childContent = childContent->NextSibling();
		}
		
		writer->EndTag();
		
		return B_OK;
	}
	
	// It's a Text
	const BText * t = dynamic_cast<const BText *>(object);
	if (t)
	{
		t->GetValueRaw(value);
		return writer->TextData(value.String(), value.Length());
	}
	
	// It's a CData
	const BCData * d = dynamic_cast<const BCData *>(object);
	if (d)
	{
		d->GetValue(&value);
		return writer->CData(value.String(), value.Length());
	}
	
	// If it's a document
	const BDocument * doc = dynamic_cast<const BDocument *>(object);
	if (doc)
	{
		// Do Children
		const BContent * childContent = doc->FirstChild();
		while (childContent)
		{
			write_xml_recursive(childContent, writer);
			childContent = childContent->NextSibling();
		}
		return B_OK;
	}
	
	// It's a Comment
	const BComment * c = dynamic_cast<const BComment *>(object);
	if (c)
	{
		c->GetValueRaw(value);
		return writer->Comment(value.String(), value.Length());
	}
	
	// It's a Processing Instruction
	const BProcessingInstruction	* pi = dynamic_cast<const BProcessingInstruction *>(object);
	if (pi)
	{
		name = pi->Name();
		pi->GetValueRaw(value);
		return writer->ProcessingInstruction(name, value);
	}
	
	const BDocumentType * dt = dynamic_cast<const BDocumentType *>(object);
	if (dt)
	{
		const BDocumentTypeDefinition * dtd;
		
		// Doctype Begin
		const BDocument * doc = dt->Document();
		const BElement * e;
		if (doc && NULL != (e = doc->Element())) {
			name = e->Name();
		} else {
			name = "XXX";
		}
		
		BString systemID, publicID;
		dtd = dt->ExternalSubset();
		if (dtd)
		{
			const char * p = dtd->PublicID();
			const char * s = dtd->SystemID();
			if (p) {
				publicID = p;
			}
			if (s) {
				systemID = s;
			}
		}
		
		err = writer->BeginDoctype(name, systemID, publicID);
		if (err != B_OK)
			return err;
		
		dtd = dt->InternalSubset();
		if (dtd) {
			err = write_xml_recursive(dtd, writer);
			if (err != B_OK)
				return err;
		}
		
		return writer->EndDoctype();
	}
	
	const BDocumentTypeDefinition * dtd = dynamic_cast<const BDocumentTypeDefinition *>(object);
	if (dtd)
	{
		int32 count;
		
		// Element Decls
		count = dtd->CountElementDecls();
		for (i=0; i<count; ++i) {
			const BElementDecl * eld = dtd->ElementDeclAt(i);
			err = write_xml_recursive(eld, writer);
			if (err != B_OK)
				return err;
		}
		
		// Entity Decls
		count = dtd->CountEntityDecls();
		for (i=0; i<count; ++i) {
			const BEntityDecl * etd = dtd->EntityDeclAt(i);
			err = write_xml_recursive(etd, writer);
			if (err != B_OK)
				return err;
		}
		
		return B_OK;
	}

	const BElementDecl * eld = dynamic_cast<const BElementDecl *>(object);
	if (eld)
	{
		return writer->OnElementDecl(eld);
	}
	
	const BEntityDecl * etd = dynamic_cast<const BEntityDecl *>(object);
	if (etd)
	{
		return writer->OnEntityDecl(etd);
	}
	
	return B_OK;
}


// =====================================================================
status_t
WriteXML(const BXMLObject * object, IByteOutput::arg stream, bool noWhitespace)
{
	BWriter writer(stream, noWhitespace ? 0 : BWriter::BALANCE_WHITESPACE);
	
	return write_xml_recursive(object, &writer);
}

}; // namespace XML
}; // namespace B

