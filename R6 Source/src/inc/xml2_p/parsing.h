#ifndef _PARSING_P_H
#define _PARSING_P_H

#include <xml2/BParser.h>

namespace B {
namespace XML {

extern B::XML::BXMLObjectFactory be_default_xml_factory;

// This function does the parsing
// All the public xml parsing functinons, as well as some of the stuff in the entity handling
// uses this directly.
// Defined ParseXML.cpp
status_t _do_the_parsing_yo_(BXMLDataSource * data, BXMLParseContext * context, bool dtdOnly, uint32 flags);

}; // namespace XML
}; // namespace B


#endif // _PARSING_P_H
