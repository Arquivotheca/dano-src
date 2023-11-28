/*

I'm sorry this function is so big.  There's really no better way.
You could also look at it as an efficiency gain.

I wrote it basically by staring at the XML spec for a while, and
then coding it.  If you ever have to maintain this code, make sure
you understand the intricacies of the productions in the spec. Most
of it's relatively easy, except for the Entity stuff.

Entities are either PARSED or UNPARSED, either INTERNAL or EXTERNAL,
and either PARAMETER or GENERAL.

- All entities are defined in DTDs
- GENERAL Entities are NEVER referenced in DTDs, except in attribute
  default values, where they are included as literal entities,
  and their values are not expanded.
        <!ENTITY Moo "This is the value">
- PARAMETER Entities are referenced only in DTDs. Put in a % to
  signify parameter entities:
        <!ENTITY % Moo "This is the value">
- INTERNAL Entities have the replacement text given:
        <!ENTITY Moo "This is the value">
- EXTERNAL Entities have the replacement text looked up somewhere else:
        <!ENTITY Moo SYSTEM "myfile.entity">
        <!ENTITY Moo PUBLIC "//formal-id-blah/" "myfile.entity">
- PARSED Entities have their text parsed by the processor, and their
  data goes directly into the document.  The data is XML.
- UNPARSED Entities do not have their text parsed by the processor.
  There data is not included in the document.  The data may or may
  not be XML (there is an errata in the XML Spec about this).  It
  may be presented along with the document (the sort of common 
  example of this is an image), but it is not actually part of the
  XML.

-joe

*/


#include <xml2/BParser.h>
#include <xml2/BStringUtils.h>
#include <parsing.h>
#include <stdlib.h>

namespace B {
namespace XML {	

// Forward References
static status_t handle_attribute_decl(BXMLParseContext * context, BString & element, BString & data);
static status_t handle_entity_decl(BXMLParseContext * context, bool parameter, BString & name, BString & value, uint32 flags, bool doctypeBeginOnly);
static status_t handle_element_start(BXMLParseContext * context, BString & name, BValue & attributes, uint32 flags);
static status_t expand_char_ref(const BString & entity, BString & entityVal);
static status_t	expand_char_refs(BString & str);
static status_t expand_entities(BXMLParseContext * context, BString & str, char delimiter);

// Seems like we should probably make this configurable
#define PARSE_BUFFER_SIZE	4096

// From XML-2.3
#define IS_WHITESPACE(x) ((x)==0x20||(x)==0x09||(x)==0x0d||(x)==0x0a)


// Shamelessly taken from String.cpp
// =====================================================================
inline int32
UTF8CharLen(uchar ch)
{
	return ((0xe5000000 >> ((ch >> 3) & 0x1e)) & 3) + 1;
}


// XML 1.0 ss 2.3 (Production 4)
// =====================================================================
inline status_t
CheckForValidNameChar(uchar ch)
{
	(void) ch;
	return B_OK;
	return B_XML_BAD_NAME_CHARACTER;
}


// XML 1.0 ss 2.3 (Production 5)
// =====================================================================
inline status_t
CheckForValidFirstNameChar(uchar ch)
{
	(void) ch;
	return B_OK;
	return B_XML_BAD_NAME_CHARACTER;
}


// =====================================================================
typedef enum
{
	PARSER_IN_UNKNOWN,
	PARSER_IN_UNKNOWN_MARKUP,					// <		encountered
	PARSER_IN_UNKNOWN_MARKUP_GT_E,				// <!		encountered
	PARSER_IN_PROCESSING_INSTRUCTION_TARGET,	// <?		encountered
	PARSER_IN_PROCESSING_INSTRUCTION,			// <?...S	encountered
	PARSER_IN_ELEMENT_START_NAME,				// <...		encountered
	PARSER_IN_ELEMENT_START_TAG,
	PARSER_IN_ELEMENT_END_TAG,
	PARSER_IN_CDATA,
	PARSER_IN_COMMENT,
	PARSER_IN_DOCTYPE,
	PARSER_IN_ELEMENT_DECL,
	PARSER_IN_ATTLIST_DECL,
	PARSER_IN_ENTITY_DECL,
	PARSER_IN_NOTATION_DECL,
	PARSER_IN_GE_REF,
	PARSER_IN_PE_REF
}parser_state;


// =====================================================================
typedef enum
{
	PARSER_NORMAL,
	PARSER_NEARING_END_1
}forward_looking_state;


// =====================================================================
typedef enum
{
	PARSER_SUB_IN_UNKNOWN,
	PARSER_SUB_IN_WHITESPACE,
	PARSER_SUB_IN_NAME,
	PARSER_SUB_READY_FOR_VALUE,
	PARSER_SUB_IN_VALUE,
	PARSER_SUB_IN_ELEMENT,
	PARSER_SUB_IN_READY_FOR_WS_1,
	PARSER_SUB_IN_WHITESPACE_1,
	PARSER_SUB_IN_WHITESPACE_2,
	PARSER_SUB_IN_WHITESPACE_3,
	PARSER_SUB_IN_WHITESPACE_4,
	PARSER_SUB_IN_WHITESPACE_5,
	PARSER_SUB_IN_WHITESPACE_7,
	PARSER_SUB_IN_WHITESPACE_8,
	PARSER_SUB_IN_WHITESPACE_9,
	PARSER_SUB_IN_WHITESPACE_10,
	PARSER_SUB_IN_DOCTYPE_NAME,
	PARSER_SUB_IN_EXTERNAL_ID,
	PARSER_SUB_IN_INTERNAL_PARSED_VALUE,
	PARSER_SUB_IN_READIING_NDATA,
	PARSER_SUB_IN_READING_PUBLIC_ID,
	PARSER_SUB_IN_READING_SYSTEM_ID,
	PARSER_SUB_IN_SPEC,
	PARSER_SUB_IN_READING_NOTATION,
	PARSER_SUB_IN_ENTITY_REF
}parser_sub_state;


// =====================================================================
typedef enum
{
	PARSER_PE_DECL,
	PARSER_GE_DECL
}decl_type;



// =====================================================================
status_t
_do_the_parsing_yo_(BXMLDataSource * data, BXMLParseContext * context, bool dtdOnly, uint32 flags)
{
	status_t err = B_OK;
	
	uchar	* realBuff = (uchar *) malloc(PARSE_BUFFER_SIZE);
	uchar	* buff = realBuff;
	size_t	buffSize;
	int		done = false;
	
	// Characters remaining from the last iteration
	uchar	* remainingChars = NULL;
	int32	remainingCharsSize = 0;
	
	uchar	* parseText = NULL;
	int32	parseTextLength;
	
	uchar	* p;
	int32	characterSize;
	
	parser_state			state = PARSER_IN_UNKNOWN;
	forward_looking_state	upcomming = PARSER_NORMAL;
	parser_sub_state		subState = PARSER_SUB_IN_UNKNOWN;
	
	// The current token, until it has been completed, and we're ready to
	// move on to the next one.
	BString		currentToken;
	
	// Current Name meaning element, target, notation, or any of those other thigns
	BString		currentName;
	BString		savedName;
	BString		currentSubName;
	BString		entityValue;
	uchar		delimiter = '\0';
	
	// Mapping of name/values.  Use for attributes, everything.
	BValue		stringMap;

	uchar		* longStringData = NULL;
	uchar		carryoverLongData[4] = {'\0', '\0', '\0', '\0'};
	uchar		someChars[3];
	
	bool		inDTD = dtdOnly;
	decl_type	declType = PARSER_GE_DECL;
	
	BString		emptyString("");
	
	// Start the line numbers at 1.  Column numbers are handled later.
	context->line = 1;
	
	// Yeah dude!
	context->OnDocumentBegin(flags);
	
	while (!done)
	{
		buffSize = PARSE_BUFFER_SIZE;
		err = data->GetNextBuffer(&buffSize, &buff, &done);
		if (B_OK != err)
		{
			context->OnError(err, true, __LINE__);
			goto ERROR_1;
		}
		
		// don't ever try to parse text less than 3 bytes long.  It screws
		// up the stuff where you tell comments are ending
		if (buffSize + remainingCharsSize <= 3)
		{
			parseText = (uchar *) malloc(remainingCharsSize + buffSize + 1);
			if (remainingCharsSize > 0)
			{
				memcpy(parseText, remainingChars, remainingCharsSize);
				free(remainingChars);
			}
			memcpy(parseText+remainingCharsSize, buff, buffSize);
			remainingCharsSize = remainingCharsSize + buffSize;
			remainingChars = parseText;
			remainingChars[remainingCharsSize] = '\0';
			continue;
		}
		
		parseText = (uchar *) malloc(remainingCharsSize + buffSize + 1);
		if (remainingCharsSize > 0)
		{
			memcpy(parseText, remainingChars, remainingCharsSize);
			free(remainingChars);
		}
		memcpy(parseText+remainingCharsSize, buff, buffSize);
		parseTextLength = remainingCharsSize + buffSize;
		parseText[parseTextLength] = '\0';
		
		// longStringData, at this point might be dangling pointer, because we sent
		// the data to the context and then deleted it, however, we haven't found
		// the end of the section that it's in.  So set it to the next text.
		if (longStringData) 
		{
			longStringData = parseText;
		}
		
		// printf("--[%s]-- ", parseText);
		
		characterSize = 0;
		p = parseText;
		while (p < (parseText + parseTextLength))
		{
			// Nice error handling
			if (*p == '\n' || *p == '\r' && p[1] != '\n')
			{
				++context->line;
				context->column = 1;
			}
			else
			{
				++context->column;
			}
			
			characterSize = UTF8CharLen(*p);
			
			// If we don't have the entire character here in the buffer,
			// defer it to the next loop (UTF-8)
			// This is because it might be passed into a longStringData section
			if (p + characterSize > parseText + parseTextLength)
			{
				;
			}
			
			// Else, handle this character
			else
			{
				switch (state)
				{
					case PARSER_IN_UNKNOWN:
					{
						if (*p == '<')
						{
							if (longStringData)
							{
								// FINISHED Text Section
								err = context->OnTextData((char *)longStringData, p-longStringData);
								if (err != B_OK)
								{
									if (B_OK != (err = context->OnError(err, false, __LINE__)))
										goto ERROR_2;
								}							
								longStringData = NULL;
							}
							
							state = PARSER_IN_UNKNOWN_MARKUP;
						}
						else if (!dtdOnly && inDTD && *p == ']')
						{
							// FINISHED End Internal Subset
							err = context->OnInternalSubsetEnd();
							if (err != B_OK)
							{
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}							
						}
						else if (inDTD && *p == '>')
						{
							// FINISHED End Doctype Declaration
							err = context->OnDocumentTypeEnd();
							if (err != B_OK)
							{
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}							
							
							inDTD = false;
						}
						else if (inDTD && *p == '%')
						{
							currentName.Truncate(0, true);
							state = PARSER_IN_PE_REF;
						}
						else if ((dtdOnly || !inDTD) && *p == '&')
						{
							if (longStringData)
							{
								// FINISHED Text Section
								err = context->OnTextData((char *)longStringData, p-longStringData);
								if (err != B_OK)
								{
									if (B_OK != (err = context->OnError(err, false, __LINE__)))
										goto ERROR_2;
								}							
								
								longStringData = NULL;
							}
							currentName.Truncate(0, true);
							state = PARSER_IN_GE_REF;
						}
						else if ((dtdOnly || !inDTD) && !longStringData)
						{
							longStringData = p;
						}
					}
					break;
					case PARSER_IN_UNKNOWN_MARKUP:
					{
						if (*p == '/')
						{
							state = PARSER_IN_ELEMENT_END_TAG;
							currentName.Truncate(0, true);
						}
						else if (*p == '!')
						{
							state = PARSER_IN_UNKNOWN_MARKUP_GT_E;
							currentToken.Truncate(0, true);
						}
						else if (*p == '?')
						{
							state = PARSER_IN_PROCESSING_INSTRUCTION_TARGET;
							currentToken.Truncate(0, true);
						}
						else
						{
							state = PARSER_IN_ELEMENT_START_NAME;
							
							err = CheckForValidFirstNameChar(*p);
							if (err != B_OK)
							{
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}							
							
							currentName.Truncate(0, true);
							currentName += *p;
							stringMap.Undefine();

						}
					}
					break;
					case PARSER_IN_UNKNOWN_MARKUP_GT_E:
					{
						// Begin Comment
						if (currentToken == "--")
						{
							// This is the first character of a comment
							longStringData = p;
							state = PARSER_IN_COMMENT;
							someChars[0] = '\0';
							someChars[1] = '\0';
							someChars[2] = '\0';
						}
						
						// Begin CDATA Section
						else if (currentToken == "[CDATA[")
						{
							// This is the first character of a CData section
							longStringData = p;
							state = PARSER_IN_CDATA;
							someChars[0] = '\0';
							someChars[1] = '\0';
							someChars[2] = '\0';
						}
						
						// Begin INCLUDE Section
						// else if (currentToken == "[INCLUDE[")
						// {
						// 	
						// }
						// 
						// Begin IGNORE Section
						// else if (currentToken == "[IGNORE[")
						// {
						// 	
						// }
						
						// Begin DOCTYPE Declaration
						else if (currentToken == "DOCTYPE")
						{
							if (!IS_WHITESPACE(*p))
							{
								err = B_XML_PARSE_ERROR;
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}
							
							state = PARSER_IN_DOCTYPE;
							subState = PARSER_SUB_IN_WHITESPACE_1;
						}
						
						// Begin ELEMENT Declaration
						else if (currentToken == "ELEMENT")
						{
							state = PARSER_IN_ELEMENT_DECL;
							subState = PARSER_SUB_IN_WHITESPACE_1;
						}
						
						// Begin ATTLIST Declaration
						else if (currentToken == "ATTLIST")
						{
							state = PARSER_IN_ATTLIST_DECL;
							subState = PARSER_SUB_IN_WHITESPACE_1;
						}
						
						// Begin ENTITY Declaration
						else if (currentToken == "ENTITY")
						{
							state = PARSER_IN_ENTITY_DECL;
							subState = PARSER_SUB_IN_WHITESPACE_1;
						}
						
						// Begin NOTATION Section
						else if (currentToken == "NOTATION")
						{
							state = PARSER_IN_NOTATION_DECL;
							subState = PARSER_SUB_IN_WHITESPACE_1;
						}
						
						// No tokens that can be here are longer than 9 chars.
						// Signal an error.
						else if (currentToken.Length() > 9)
						{
							err = B_XML_PARSE_ERROR;
							goto ERROR_2;
						}
						// Build up currentToken
						else
						{
							currentToken += *p;
						}
					}
					break;
					case PARSER_IN_PROCESSING_INSTRUCTION_TARGET:
					{
						if (IS_WHITESPACE(*p))
						{
							currentName = currentToken;
							currentToken.Truncate(0, true);
							state = PARSER_IN_PROCESSING_INSTRUCTION;
						}
						else
						{
							currentToken += *p;
						}
					}
					break;
					case PARSER_IN_PROCESSING_INSTRUCTION:
					{
						// Skip whitespace until we find something
						if (IS_WHITESPACE(*p) && currentToken.Length() == 0)
							;
						// When we find ?> end
						else if (*p == '?')
							upcomming = PARSER_NEARING_END_1;
						else if (*p == '>' && upcomming == PARSER_NEARING_END_1)
						{
							// Expand Character Refs
							err = expand_char_refs(currentToken);
							if (err != B_OK)
								goto ERROR_2;
							
							// FINISHED Processing Instruction
							err = context->OnProcessingInstruction(currentName, currentToken);
							if (err != B_OK)
								goto ERROR_2;
							
							currentName.Truncate(0, true);
							currentToken.Truncate(0, true);
							state = PARSER_IN_UNKNOWN;
							upcomming = PARSER_NORMAL;
						}
						else
						{
							upcomming = PARSER_NORMAL;
							currentToken += *p;
						}
					}
					break;
					case PARSER_IN_ELEMENT_START_NAME:
					{
						// End of Element Tag
						if (*p == '/')
						{
							state = PARSER_IN_ELEMENT_START_TAG;
							upcomming = PARSER_NEARING_END_1;
						}
						else if (*p == '>')
						{
							// FINISHED Element Start Tag
							err = handle_element_start(context, currentName, stringMap, flags);
							if (err != B_OK)
								goto ERROR_2;
							
							state = PARSER_IN_UNKNOWN;
						}
						// End of the Element Name
						else if (IS_WHITESPACE(*p))
						{
							currentToken.Truncate(0, true);
							state = PARSER_IN_ELEMENT_START_TAG;
							subState = PARSER_SUB_IN_WHITESPACE;
						}
						else
						{
							currentName += *p;
						}
					}
					break;
					case PARSER_IN_ELEMENT_START_TAG:
					{
						if (subState != PARSER_SUB_IN_VALUE && *p == '/')
						{
							upcomming = PARSER_NEARING_END_1;
						}
						else if (*p == '>')
						{
							// FINISHED Element Start Tag
							savedName = currentName;
							err = handle_element_start(context, currentName, stringMap, flags);
							if (err != B_OK)
								goto ERROR_2;
							
							if (upcomming == PARSER_NEARING_END_1)
							{
								// FINISHED Implicit Element End Tag
								err = context->OnEndTag(savedName);
								if (err != B_OK)
									goto ERROR_2;
							}
							
							currentName.Truncate(0, true);
							currentToken.Truncate(0, true);
							state = PARSER_IN_UNKNOWN;
							upcomming = PARSER_NORMAL;
						}
						else
						{
							if (upcomming == PARSER_NEARING_END_1)
							{
								upcomming = PARSER_NORMAL;
								currentToken += '/';
							}
							
							// Do Attributes
							switch (subState)
							{
								case PARSER_SUB_IN_WHITESPACE:
								{
									 if (!IS_WHITESPACE(*p))
									 	subState = PARSER_SUB_IN_NAME;
									 currentToken.Truncate(0, true);
									 currentToken += *p;
								}
								break;
								case PARSER_SUB_IN_NAME:
								{
									if (IS_WHITESPACE(*p))
										subState = PARSER_SUB_IN_WHITESPACE_1;
									else if (*p == '=')
										subState = PARSER_SUB_READY_FOR_VALUE;
									else
										currentToken += *p;
								}
								break;
								case PARSER_SUB_IN_WHITESPACE_1:
								{
									if (IS_WHITESPACE(*p))
										; // nothing
									else if (*p == '=')
										subState = PARSER_SUB_READY_FOR_VALUE;
									 else
									 {
									 	err = B_XML_PARSE_ERROR;
									 	goto ERROR_2;
									 }
								}
								break;
								case PARSER_SUB_READY_FOR_VALUE:
								{
									 if (*p == '\'')
									 	delimiter = '\'';
									 else if (*p == '\"')
									 	delimiter = '\"';
									 else if (IS_WHITESPACE(*p))
									 	delimiter = '\0'; // nothing
									 else
									 {
									 	err = B_XML_PARSE_ERROR;
									 	goto ERROR_2;
									 }
									 if (delimiter != '\0')
									 {
										 currentSubName.Adopt(currentToken);
										 currentToken.Truncate(0, true);
										 subState = PARSER_SUB_IN_VALUE;
									 }
								}
								break;
								case PARSER_SUB_IN_VALUE:
								{
									if (*p == delimiter)
									{
										if (stringMap[BValue(currentSubName)])
										{
										 	err = B_XML_ATTR_NAME_IN_USE;
										 	goto ERROR_2;
										}
										stringMap.Overlay(currentSubName, currentToken);
										currentSubName.Truncate(0, true);
										currentToken.Truncate(0, true);
										subState = PARSER_SUB_IN_WHITESPACE;
									}
									else
										currentToken += *p;
								}
								break;
								default:
									;
							}
							
						}
					}
					break;
					case PARSER_IN_ELEMENT_END_TAG:
					{
						// End of the Element Name
						if (*p == '>')
						{
							// FINISHED Element End Tag
							err = context->OnEndTag(currentName);
							if (err != B_OK)
							{
								if (B_OK != context->OnError(err, false, __LINE__))
									goto ERROR_2;
							}
							
							currentName.Truncate(0, true);
							state = PARSER_IN_UNKNOWN;
							upcomming = PARSER_NORMAL;
						}
						else if (IS_WHITESPACE(*p))
						{
							upcomming = PARSER_NEARING_END_1;
						}
						else
						{
							if (upcomming == PARSER_NORMAL)
								currentName += *p;
						}
					}
					break;
					case PARSER_IN_COMMENT:
					{
						someChars[0] = someChars[1];
						someChars[1] = someChars[2];
						someChars[2] = *p;
						
						// See comment in PARSER_IN_CDATA
						if (carryoverLongData[0])
						{
							// See the comment for this in the CData section handler
							
							// FINISHED Comment Section
							err = context->OnComment((char *) carryoverLongData, 3);
							if (err != B_OK)
							{
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}							
							
							carryoverLongData[0] = '\0';
						}
						
						if (someChars[0] == '-' && someChars[1] == '-' && someChars[2] == '>')
						{
							// need to strip off the trailing "--"
							// this is okay because we have a minimum of 3 chars in the buffer
							// this line is why we have that restriction.
							*(p-2) = '\0';
							
							// FINISHED Comment Section
							err = context->OnComment((char *) longStringData, p-longStringData);
							if (err != B_OK)
							{
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}							
							
							state = PARSER_IN_UNKNOWN;
							longStringData = NULL;
						}
					}
					break;
					case PARSER_IN_CDATA:
					{
						
						someChars[0] = someChars[1];
						someChars[1] = someChars[2];
						someChars[2] = *p;
						
						// These three lonely characters get outputted whenever
						// they crossed the buffer boundary.  The idea is that
						// it's faster to just send them out as three lonely
						// characters in their own CData section than it is to
						// memcpy around the next one.
						if (carryoverLongData[0])
						{
							// FINISHED CData Section
							err = context->OnCData((char *) carryoverLongData, 3);
							if (err != B_OK)
							{
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}							
							
							carryoverLongData[0] = '\0';
						}
						
						if (someChars[0] == ']' && someChars[1] == ']' && someChars[2] == '>')
						{
							// See comment for this same thing in the comment handler
							*(p-2) = '\0';
							
							// FINISHED CData Section
							err = context->OnCData((char *) longStringData, p-longStringData);
							if (err != B_OK)
							{
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}							
							
							state = PARSER_IN_UNKNOWN;
							longStringData = NULL;
						}
					}
					break;
					case PARSER_IN_DOCTYPE:
					{
						// doctypeName
						switch (subState)
						{
							case PARSER_SUB_IN_WHITESPACE_1:
							{
								if (!IS_WHITESPACE(*p))
								{
									currentName.Truncate(0, true);
									currentName += *p;
									subState = PARSER_SUB_IN_DOCTYPE_NAME;
								}
							}
							break;
							case PARSER_SUB_IN_DOCTYPE_NAME:
							{
								if (*p == '[')
								{
									// FINISHED Begin Doctype Declaration
									// No ExternalID
									err = context->OnDocumentTypeBegin(currentName);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									emptyString = "";
									
									// FINISHED Begin Internal Subset
									err = context->OnInternalSubsetBegin();
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							

									state = PARSER_IN_UNKNOWN;
									subState = PARSER_SUB_IN_UNKNOWN;
									inDTD = true;
									longStringData = NULL;
								}
								else if (*p == '>')
								{
									// FINISHED Begin Doctype Declaration
									// No ExternalID
									err = context->OnDocumentTypeBegin(currentName);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									emptyString = "";
									
									// FINISHED End Doctype Declaration
									err = context->OnDocumentTypeEnd();
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									state = PARSER_IN_UNKNOWN;
									inDTD = false;
								}
								else if (IS_WHITESPACE(*p))
									subState = PARSER_SUB_IN_WHITESPACE_2;
								else
									currentName += *p;
							}
							break;
							case PARSER_SUB_IN_WHITESPACE_2:
							{
								if (*p == '[')
								{
									// FINISHED Begin Doctype Declaration
									// No ExternalID
									err = context->OnDocumentTypeBegin(currentName);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									emptyString = "";
									
									// FINISHED Begin Internal Subset
									err = context->OnInternalSubsetBegin();
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							

									state = PARSER_IN_UNKNOWN;
									subState = PARSER_SUB_IN_UNKNOWN;
									inDTD = true;
									longStringData = NULL;
								}
								else if (*p == '>')
								{
									// FINISHED Begin Doctype Declaration
									// No ExternalID
									err = context->OnDocumentTypeBegin(currentName);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									emptyString = "";
									
									// FINISHED End Doctype Declaration
									err = context->OnDocumentTypeEnd();
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									state = PARSER_IN_UNKNOWN;
									inDTD = false;
								}
								else if (!IS_WHITESPACE(*p))
								{
									subState = PARSER_SUB_IN_EXTERNAL_ID;
									currentToken.Truncate(0, true);
									currentToken += *p;
								}
							}
							break;
							case PARSER_SUB_IN_EXTERNAL_ID:
							{
								if (*p == '[')
								{
									// FINISHED Begin Doctype Declaration
									// With ExternalID
									err = context->OnDocumentTypeBegin(currentName);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									// Do the External Subset
									err = handle_entity_decl(context, false, emptyString, currentToken, flags, true);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									// FINISHED Begin Internal Subset
									err = context->OnInternalSubsetBegin();
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							

									state = PARSER_IN_UNKNOWN;
									subState = PARSER_SUB_IN_UNKNOWN;
									inDTD = true;
									longStringData = NULL;
								}
								else if (*p == '>')
								{
									// FINISHED Begin Doctype Declaration
									// With ExternalID
									err = context->OnDocumentTypeBegin(currentName);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
										
									// Do the External Subset
									err = handle_entity_decl(context, false, emptyString, currentToken, flags, true);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									// FINISHED End Doctype Declaration
									err = context->OnDocumentTypeEnd();
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									state = PARSER_IN_UNKNOWN;
									inDTD = false;
								}
								else
									currentToken += *p;
							}
							break;
							default:
								; // maybe should signal some type of error.
						}
						
					}
					break;
					case PARSER_IN_ELEMENT_DECL:
					{
						switch (subState)
						{
							case PARSER_SUB_IN_WHITESPACE_1:
							{
								if (!IS_WHITESPACE(*p))
								{
									currentName.Truncate(0, true);
									currentName += *p;
									subState = PARSER_SUB_IN_NAME;
								}
							}
							break;
							case PARSER_SUB_IN_NAME:
							{
								if (IS_WHITESPACE(*p))
									subState = PARSER_SUB_IN_WHITESPACE_2;
								else
									currentName += *p;
							}
							break;
							case PARSER_SUB_IN_WHITESPACE_2:
							{
								if (*p == '%')
								{
									currentToken.Truncate(0, true);
									subState = PARSER_SUB_IN_ENTITY_REF;
									currentSubName.Truncate(0, true);
								}
								else if (!IS_WHITESPACE(*p))
								{
									currentToken.Truncate(0, true);
									currentToken += *p;
									subState = PARSER_SUB_IN_SPEC;
								}
							}
							break;
							case PARSER_SUB_IN_SPEC:
							{
								if (*p == '%')
								{
									subState = PARSER_SUB_IN_ENTITY_REF;
									currentSubName.Truncate(0, true);
								}
								else if (*p == '>')
								{
									// FINISHED Element Decl
									err = context->OnElementDecl(currentName, currentToken);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									state = PARSER_IN_UNKNOWN;
									subState = PARSER_SUB_IN_UNKNOWN;
								}
								else
									currentToken += *p;
							}
							break;
							case PARSER_SUB_IN_ENTITY_REF:
							{
								if (*p == ';')
								{
									err = context->OnParameterEntityRef(currentSubName, entityValue);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									currentToken += entityValue;
									subState = PARSER_SUB_IN_SPEC;
								}
								else
									currentSubName += *p;
							}
							break;

							default:
								; // maybe should signal some type of error.
						}
					}
					break;
					
					case PARSER_IN_ATTLIST_DECL:
					{
						switch (subState)
						{
							case PARSER_SUB_IN_WHITESPACE_1:
							{
								if (!IS_WHITESPACE(*p))
								{
									currentName.Truncate(0, true);
									currentName += *p;
									subState = PARSER_SUB_IN_NAME;
								}
							}
							break;
							case PARSER_SUB_IN_NAME:
							{
								if (IS_WHITESPACE(*p))
									subState = PARSER_SUB_IN_WHITESPACE_2;
								else
									currentName += *p;
							}
							break;
							case PARSER_SUB_IN_WHITESPACE_2:
							{
								if (*p == '%')
								{
									currentToken.Truncate(0, true);
									subState = PARSER_SUB_IN_ENTITY_REF;
									currentSubName.Truncate(0, true);
								}
								else if (!IS_WHITESPACE(*p))
								{
									currentToken.Truncate(0, true);
									currentToken += *p;
									subState = PARSER_SUB_IN_SPEC;
								}
							}
							break;
							case PARSER_SUB_IN_SPEC:
							{
								if (*p == '%')
								{
									subState = PARSER_SUB_IN_ENTITY_REF;
									currentSubName.Truncate(0, true);
								}
								else if (*p == '>')
								{
									// FINISHED Attlist Decl
									err = handle_attribute_decl(context, currentName, currentToken);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									state = PARSER_IN_UNKNOWN;
									subState = PARSER_SUB_IN_UNKNOWN;
								}
								else
									currentToken += *p;
							}
							break;
							case PARSER_SUB_IN_ENTITY_REF:
							{
								if (*p == ';')
								{
									err = context->OnParameterEntityRef(currentSubName, entityValue);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									currentToken += entityValue;
									subState = PARSER_SUB_IN_SPEC;
								}
								else
									currentSubName += *p;
							}
							break;
							default:
								; // maybe should signal some type of error.
						}
					}
					break;
					
					case PARSER_IN_ENTITY_DECL:
					{
						switch (subState)
						{
							case PARSER_SUB_IN_WHITESPACE_1:
							{
								if (!IS_WHITESPACE(*p))
								{
									
									if (*p == '%')
									{
										declType = PARSER_PE_DECL;
										subState = PARSER_SUB_IN_WHITESPACE_2;
									}
									else
									{
										subState = PARSER_SUB_IN_NAME;
										currentName.Truncate(0, true);
										currentName += *p;
										declType = PARSER_GE_DECL;
									}
								}
							}
							break;
							case PARSER_SUB_IN_WHITESPACE_2:
							{
								if (!IS_WHITESPACE(*p))
								{
									subState = PARSER_SUB_IN_NAME;
									currentName.Truncate(0, true);
									currentName += *p;
								}
							}
							break;
							case PARSER_SUB_IN_NAME:
							{
								if (IS_WHITESPACE(*p))
									subState = PARSER_SUB_IN_WHITESPACE_3;
								else
									currentName += *p;
							}
							break;
							case PARSER_SUB_IN_WHITESPACE_3:
							{
								if (!IS_WHITESPACE(*p))
								{
									currentToken.Truncate(0, true);
									currentToken += *p;
									subState = PARSER_SUB_IN_SPEC;
								}
							}
							break;
							case PARSER_SUB_IN_SPEC:
							{
								if (*p == '%')
								{
									subState = PARSER_SUB_IN_ENTITY_REF;
									currentSubName.Truncate(0, true);
								}
								else if (*p == '>')
								{
									// FINISHED Entity Decl
									err = handle_entity_decl(context, declType == PARSER_PE_DECL, currentName, currentToken, flags, false);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									state = PARSER_IN_UNKNOWN;
									subState = PARSER_SUB_IN_UNKNOWN;
								}
								else
									currentToken += *p;
							}
							break;
							case PARSER_SUB_IN_ENTITY_REF:
							{
								if (*p == ';')
								{
									// You don't have to worry about recursive parameter entities
									// because, by parsing the document in one pass, we enforce
									// at total ordering on the entities, therefore, it will fail
									// if an entity references itself either directly or indirectly
									// because it has not been declared yet.  It gets declared a few
									// lines above (FINISHED Entity Decl), which always happens
									// after right now.
									err = context->OnParameterEntityRef(currentSubName, entityValue);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									currentToken += entityValue;
									subState = PARSER_SUB_IN_SPEC;
								}
								else
									currentSubName += *p;
							}
							break;
							default:
								; // maybe should signal some type of error.
						}
					}
					break;
					
					case PARSER_IN_NOTATION_DECL:
					{
						switch (subState)
						{
							case PARSER_SUB_IN_WHITESPACE_1:
							{
								if (!IS_WHITESPACE(*p))
								{
									currentName.Truncate(0, true);
									currentName += *p;
									subState = PARSER_SUB_IN_NAME;
								}
							}
							break;
							case PARSER_SUB_IN_NAME:
							{
								if (IS_WHITESPACE(*p))
									subState = PARSER_SUB_IN_WHITESPACE_2;
								else
									currentName += *p;
							}
							break;
							case PARSER_SUB_IN_WHITESPACE_2:
							{
								if (!IS_WHITESPACE(*p))
								{
									currentToken.Truncate(0, true);
									currentToken += *p;
									subState = PARSER_SUB_IN_SPEC;
								}
							}
							break;
							case PARSER_SUB_IN_SPEC:
							{
								if (*p == '>')
								{
									// FINISHED Attlist Decl
									err = context->OnNotationDecl(currentName, currentToken);
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
									
									state = PARSER_IN_UNKNOWN;
									subState = PARSER_SUB_IN_UNKNOWN;
								}
								else
									currentToken += *p;
							}
							break;
							default:
								; // maybe should signal some type of error.
						}
					}
					break;
					
					case PARSER_IN_GE_REF:
					{
						if (*p == ';')
						{
							if (currentName.ByteAt(0) == '#')
							{
								BString entityVal;
								err = expand_char_ref(currentName, entityVal);
								if (err != B_OK)
								{
									if (B_OK != (err = context->OnError(err, false, __LINE__)))
										goto ERROR_2;
								}							
								if (entityVal.Length() > 0)
								{
									err = context->OnTextData(entityVal.String(), entityVal.Length());
									if (err != B_OK)
									{
										if (B_OK != (err = context->OnError(err, false, __LINE__)))
											goto ERROR_2;
									}							
								}
							}
							else
							{
								err = context->OnGeneralParsedEntityRef(currentName);
								if (err != B_OK)
								{
									if (B_OK != (err = context->OnError(err, false, __LINE__)))
										goto ERROR_2;
								}							
							}
							state = PARSER_IN_UNKNOWN;
						}
						else
							currentName += *p;
					}
					break;
					
 					case PARSER_IN_PE_REF:
					{
						if (*p == ';')
						{
							err = context->OnParameterEntityRef(currentName);
							if (err != B_OK)
							{
								if (B_OK != (err = context->OnError(err, false, __LINE__)))
									goto ERROR_2;
							}							
							state = PARSER_IN_UNKNOWN;
						}
						else
							currentName += *p;
					}
					break;
					
					default:
						err = B_ERROR;
						context->OnError(err, true, __LINE__);
						goto ERROR_2;
				}
				
			}
			
			// XXX Should this be p++????
			// p += characterSize;
			// I think it should, because Sebastian is having problems with multibyte characters
			// in attributes.  In text data, it works because it just skips over it.  Attributes,
			// and most other things are stored in temporary BStrings (just not long text or CData
			// sections).  Therefore, I'm going to change it so it just does p++.
			p++;
			
		}
		
		if (p == (parseText + parseTextLength))
		{
			remainingChars = NULL;
			remainingCharsSize = 0;
		}
		else
		{
			remainingChars = (uchar *) strdup((char *) p - characterSize);
			remainingCharsSize = strlen((char *) remainingChars);
			// fprintf(stderr, "Using Remaining Characters (length %d): %s\n", (int) remainingCharsSize, remainingChars);
		}
		
		if (longStringData)
		{
			// if state == PARSER_IN_UNKNOWN then we we want to output all the characters
			// because that state ends when another begins, not at a terminator string.
			// Therefore, just send them.  parseText is automatically NULL terminated, so
			// there's no need to do it here.
			if (state != PARSER_IN_UNKNOWN)
			{
				// This okay because we guaranteed before that we never
				// process less than 3 characters at a time
				carryoverLongData[0] = parseText[parseTextLength-3];
				carryoverLongData[1] = parseText[parseTextLength-2];
				carryoverLongData[2] = parseText[parseTextLength-1];
				
				parseText[parseTextLength-3] = '\0';
			}
			
			// Long Text Sections -- CData, Comment, Text can be split across
			// buffer boundaries.  But, it's okay to generate them as separate
			// events.
			switch (state)
			{
				case PARSER_IN_CDATA:
					err = context->OnCData((char *) longStringData, p-longStringData);
					if (err != B_OK)
					{
						if (B_OK != (err = context->OnError(err, false, __LINE__)))
							goto ERROR_2;
					}							
					break;
				case PARSER_IN_COMMENT:
					err = context->OnComment((char *) longStringData, p-longStringData);
					if (err != B_OK)
					{
						if (B_OK != (err = context->OnError(err, false, __LINE__)))
							goto ERROR_2;
					}							
					break;
				default:
					err = context->OnTextData((char *)longStringData, p-longStringData);
					if (err != B_OK)
					{
						if (B_OK != (err = context->OnError(err, false, __LINE__)))
							goto ERROR_2;
					}							
					break;
			}
		}
		
		free(parseText);
	}
	
	
	// We won!
	err = context->OnDocumentEnd();
	if (err != B_OK)
	{
		if (B_OK != (err = context->OnError(err, false, __LINE__)))
			goto ERROR_2;
	}							
	
	return B_OK;
	
ERROR_2:
	if (remainingChars)
		free(remainingChars);
	if (parseText)
		free(parseText);
	
ERROR_1:
	free(buff);
	
	return err;
}


// =====================================================================
static status_t
handle_element_start(BXMLParseContext * context, BString & name, BValue & attributes, uint32 flags)
{
	status_t err;
	void * cookie;
	BValue key, value;
	
	if (flags & B_XML_HANDLE_ATTRIBUTE_ENTITIES)
	{
		// Go through each of the attributes, looking for entities.
		// If you find an entity, go ask for the replacement text,
		// and fill it in.
		
		cookie = NULL;
		while (B_OK == attributes.GetNextItem(&cookie, &key, &value))
		{
			BString v = value.AsString();
			
			err = expand_entities(context, v, '&');
			if (err != B_OK)
			{
				if (B_OK != (err = context->OnError(err, false, __LINE__)))
					return err;
			}							
			
			if (flags & B_XML_COALESCE_WHITESPACE)
				MushString(v);
			
			attributes.Overlay(key, v);
		}
		
		err = context->OnStartTag(name, attributes);
		if (err != B_OK)
		{
			if (B_OK != (err = context->OnError(err, false, __LINE__)))
				return err;
		}							
		return B_OK;
	}
	else if (flags & B_XML_COALESCE_WHITESPACE)
	{

		cookie = NULL;
		while (B_OK == attributes.GetNextItem(&cookie, &key, &value))
		{
			BString v = value.AsString();
			MushString(v);
			attributes.Overlay(key, v);
		}

		
		err = context->OnStartTag(name, attributes);
		if (err != B_OK)
		{
			if (B_OK != (err = context->OnError(err, false, __LINE__)))
				return err;
		}							
		return B_OK;
	}
	else
	{
		err = context->OnStartTag(name, attributes);
		if (err != B_OK)
		{
			if (B_OK != (err = context->OnError(err, false, __LINE__)))
				return err;
		}							
		return B_OK;
	}
}



// =====================================================================
static status_t
handle_entity_decl(BXMLParseContext * context, bool parameter, BString & name,
					BString & value, uint32 flags, bool doctypeBeginOnly)
{
	status_t err;
	
	// This second pass is required because it's just easier to expand
	// the entities while reading, and then process the results of that.
	
	// Determine what type of declaration it is.  The report
	// it with the correct event.
	const char * p = value.String();
	parser_sub_state subState = PARSER_SUB_IN_WHITESPACE_1;
	BString currentToken;
	BString publicID, systemID, internalData, spec, notation;
	bool foundInternalData = false;
	while (*p)
	{
		switch(subState)
		{
			case PARSER_SUB_IN_WHITESPACE_1:
			{
				if (*p == '\"')
				{
					// Internal Parsed
					subState = PARSER_SUB_IN_INTERNAL_PARSED_VALUE;
				}
				else if (*p == 'S' || *p == 'P')
				{
					// Either SYSTEM or PUBLIC
					spec.SetTo(*p,1);
					subState = PARSER_SUB_IN_SPEC;
				}
				else if (!IS_WHITESPACE(*p))
				{
					err = B_XML_PARSE_ERROR;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
			}
			break;
			
			
			// ====== Internal Parsed ==========================
			case PARSER_SUB_IN_INTERNAL_PARSED_VALUE:
			{
				if (*p == '\"')
				{
					subState = PARSER_SUB_IN_WHITESPACE_4;
					foundInternalData = true;
				}
				else
					internalData += *p;
			}
			break;
			case PARSER_SUB_IN_WHITESPACE_4:
			{
				// At this point, all we're allowed is whitespace.
				// We could ignore this and be forgiving, but instead,
				// we will be anal.
				if (!IS_WHITESPACE(*p))
				{
					err = B_XML_PARSE_ERROR;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
			}
			break;
			
			
			// ====== Choose ID Type ===========================
			case PARSER_SUB_IN_SPEC:
			{
				if (spec == "PUBLIC")
				{
					subState = PARSER_SUB_IN_WHITESPACE_5;
				}
				else if (spec == "SYSTEM")
				{
					subState = PARSER_SUB_IN_WHITESPACE_7;
				}
				else if (IS_WHITESPACE(*p))
				{
					err = B_XML_PARSE_ERROR;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
				else
				{
					spec += *p;
				} 
			}
			break;
			
			
			// ====== Public ID ================================
			case PARSER_SUB_IN_WHITESPACE_5:
			{
				if (*p == '\"')
					subState = PARSER_SUB_IN_READING_PUBLIC_ID;
				else if (!IS_WHITESPACE(*p))
				{
					err = B_XML_ENTITY_VALUE_NO_QUOTES;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
			}
			break;
			case PARSER_SUB_IN_READING_PUBLIC_ID:
			{
				if (*p == '\"')
				{
					// Now look for a systemID (WHITESPACE_6 got optimized out)
					subState = PARSER_SUB_IN_WHITESPACE_7;
				}
				else
					publicID += *p;
			}
			break;
			
			
			// ====== System ID ================================
			case PARSER_SUB_IN_WHITESPACE_7:
			{
				if (*p == '\"')
					subState = PARSER_SUB_IN_READING_SYSTEM_ID;
				else if (!IS_WHITESPACE(*p))
				{
					err = B_XML_ENTITY_VALUE_NO_QUOTES;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
			}
			break;
			case PARSER_SUB_IN_READING_SYSTEM_ID:
			{
				if (*p == '\"')
				{
					spec.Truncate(0, true);
					subState = PARSER_SUB_IN_WHITESPACE_8;
				}
				else
					systemID += *p;
			}
			break;
			
			
			// ====== End or NDATA =============================
			case PARSER_SUB_IN_WHITESPACE_8:
			{
				if (*p == 'N')
				{
					spec += *p;
					subState = PARSER_SUB_IN_READIING_NDATA;
				}
				else if (!IS_WHITESPACE(*p))
				{
					err = B_XML_PARSE_ERROR;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
			}
			break;
			case PARSER_SUB_IN_READIING_NDATA:
			{
				if (spec == "NDATA")
				{
					subState = PARSER_SUB_IN_WHITESPACE_9;
				}
				else if (IS_WHITESPACE(*p))
				{
					err = B_XML_PARSE_ERROR;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
				else
					spec += *p;
			}
			break;
			case PARSER_SUB_IN_WHITESPACE_9:
			{
				if (!IS_WHITESPACE(*p))
				{
					notation += *p;
					subState = PARSER_SUB_IN_READING_NOTATION;
				}
			}
			break;
			case PARSER_SUB_IN_READING_NOTATION:
			{
				if (IS_WHITESPACE(*p))
					subState = PARSER_SUB_IN_WHITESPACE_10;
				else
					notation += *p;
			}
			break;
			case PARSER_SUB_IN_WHITESPACE_10:
			{
				if (!IS_WHITESPACE(*p))
				{
					err = B_XML_PARSE_ERROR;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
			}
			break;
			
			default:
				; // error?
		}
		
		++p;
	}
	
	if (subState == PARSER_SUB_IN_WHITESPACE_1
			|| subState == PARSER_SUB_IN_SPEC
			|| subState == PARSER_SUB_IN_WHITESPACE_5
			|| subState == PARSER_SUB_IN_READING_PUBLIC_ID
			|| subState == PARSER_SUB_IN_READING_SYSTEM_ID
			|| subState == PARSER_SUB_IN_READIING_NDATA
			|| subState == PARSER_SUB_IN_WHITESPACE_9 )
	{
		err = B_XML_PARSE_ERROR;
		if (B_OK != (err = context->OnError(err, false, __LINE__)))
			return err;
	}
	if (doctypeBeginOnly)
	{
		// Technically, there can't be an empty systemID, but we're going to allow it because
		// we will fail later if it can't find the publicID and they don't want to try looking
		// for one.  --joeo
		//else if (systemID == "")
		//	return B_XML_PARSE_ERROR;
		if (foundInternalData || notation != "")
		{
			err = B_XML_PARSE_ERROR;
			if (B_OK != (err = context->OnError(err, false, __LINE__)))
				return err;
		}
		err = context->OnExternalSubset(publicID, systemID, flags);
		if (err != B_OK)
		{
			if (B_OK != (err = context->OnError(err, false, __LINE__)))
				return err;
		}							
		return B_OK;
	}
	else
	{
		if (foundInternalData)
		{
			err = context->OnInternalParsedEntityDecl(name, internalData, parameter, flags);
			if (err != B_OK)
			{
				if (B_OK != (err = context->OnError(err, false, __LINE__)))
					return err;
			}							
			return B_OK;
		}
		// Technically, there can't be an empty systemID, but we're going to allow it because
		// we will fail later if it can't find the publicID and they don't want to try looking
		// for one.  --joeo
		//else if (systemID == "")
		//	return B_XML_PARSE_ERROR;
		else if (notation != "")
		{
			if (parameter)
				err = B_XML_NO_UNPARSED_PARAMETER_ENTITIES;
			else
				err = context->OnUnparsedEntityDecl(name, publicID, systemID, notation);
			if (err != B_OK)
			{
				if (B_OK != (err = context->OnError(err, false, __LINE__)))
					return err;
			}
			return B_OK;
		}
		else
		{
			err = context->OnExternalParsedEntityDecl(name, publicID, systemID, parameter, flags);
			if (err != B_OK)
			{
				if (B_OK != (err = context->OnError(err, false, __LINE__)))
					return err;
			}
			return B_OK;
		}
	}
	// Will never get here.  Just make gcc happy
	return B_ERROR;
}


// =====================================================================
static status_t
handle_attribute_decl(BXMLParseContext * context, BString & element, BString & data)
{
	status_t err = B_ERROR;
	BString name;
	BString type;
	BString mode;
	BString def;
	BString elementTemp;
	bool okayToEnd = false;
	enum {BEGIN, NAME, FIRST_TYPE, TYPE_ENUM, TYPE, MODE_FIRST, MODE, DEF_FIRST, DEF, EAT_SPACE} state = NAME;
	MushString(data);
	const char * p = data.String();
	while (true)
	{
		switch (state)
		{
			case BEGIN:
				if (*p)
					okayToEnd = false;
				if (!IS_WHITESPACE(*p))
					state = NAME;
				else
					break;
			// Fall through
			case NAME:
				if (IS_WHITESPACE(*p))
					state = FIRST_TYPE;
				else
					name += *p;
				break;
			case FIRST_TYPE:
				if (*p == '(')
				{
					type.SetTo('(',1);
					state = TYPE_ENUM;
				}
				else
				{
					type.SetTo(*p,1);
					state = TYPE;
				}
				break;
			case TYPE_ENUM:
				if (*p == ')')
				{
					type += ')';
					state = EAT_SPACE;
				}
				else
					type += *p;
				break;
			case EAT_SPACE:
				if (!IS_WHITESPACE(*p))
				{
					state = MODE_FIRST;
					goto MODE_FIRST_1;
				}
				break;
			case TYPE:
				if (IS_WHITESPACE(*p))
					state = MODE_FIRST;
				else
					type += *p;
				break;
			case MODE_FIRST:
MODE_FIRST_1:
				if (*p == '#')
					state = MODE;
				else if (*p == '\"')
					state = DEF;
				else
					goto DONE_WITH_DECL;
				break;
			case MODE:
				if (IS_WHITESPACE(*p) || !*p)
				{
					if (mode == "REQUIRED" || mode == "IMPLIED")
						goto DONE_WITH_DECL;
					else
						state = DEF_FIRST;
				}
				else
					mode += *p;
				break;
			case DEF_FIRST:
				if (*p == '\"')
					state = DEF;
				else
				{
					err = B_XML_PARSE_ERROR;
					if (B_OK != (err = context->OnError(err, false, __LINE__)))
						return err;
				}
				break;
			case DEF:
				if (*p == '\"')
				{
DONE_WITH_DECL:
					// Temporary copy for API consistency, things expect to be able 
					// to adopt the storage of the BStrings to avoid a copy.
					elementTemp = element;
					
					// Can only affect enum types. Make it easier to parse later.
					StripWhitespace(type);
					
					// FINISHED Attribute Decl
					err = context->OnAttributeDecl(elementTemp, name, type, mode, def);
					if (err != B_OK)
					{
						if (B_OK != (err = context->OnError(err, false, __LINE__)))
							return err;
					}
					
					// Clean everything up for the next trip around
					state = BEGIN;
					name.Truncate(0, true);
					type.Truncate(0, true);
					mode.Truncate(0, true);
					def.Truncate(0, true);
					okayToEnd = true;
				}
				else
					def += *p;
				break;
		}
		if (!*p)
		{
			if (okayToEnd)
				return B_OK;
			else
			{
				if (B_OK != (err = context->OnError(err, false, __LINE__)))
					return err;
			}
		}
		p++;
	}
	
	return B_OK;
}


static char
hex2dec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else
		return 0;
}


// =====================================================================
static status_t
expand_char_ref(const BString & entity, BString & entityVal)
{
	int32 last;
	char c = 0;
	
	entityVal.Truncate(0, true);
	last = entity.Length()-1;
	if (entity.ByteAt(1) == 'x')
	{
		// Hex
		for (int32 i=last; i>1; --i)
		{
			if ((last - i) % 2 == 0)
				c = hex2dec(entity[i]);
			else
			{
				c |= (hex2dec(entity[i]) << 4);
				entityVal.Prepend(c, 1);
			}
		}
		if (c != 0 && last % 2 == 0)
			entityVal.Prepend(c, 1);
	}
	else
	{
		// Decimal
		int32 val = atoi(entity.String()+1);
		while (val > 0)
		{
			entityVal.Prepend((char) (val & 0x000000FF), 1);
			val /= 256;
		}
	}
	
	return B_NO_ERROR;
}


// =====================================================================
static status_t
expand_char_refs(BString & str)
{
	char delimiter = '&';
	status_t err;
	
	BString entity;
	BString entityVal;
	BString newValue("");
	
	int32 offset = 0, oldOffset = 0;
	int32 end = -1;
	
	while (true)
	{
		oldOffset = end + 1;
		offset = str.FindFirst(delimiter, oldOffset);
		if (offset < 0)
			break;
		end = str.FindFirst(';', offset);
		if (end < 0)
			break;
		
		newValue.Append(str.String() + oldOffset, offset-oldOffset);
		str.CopyInto(entity, offset+1, end-offset-1);
		
		if (entity.ByteAt(0) == '#')
		{
			err = expand_char_ref(entity, entityVal);
			if (err != B_OK)
				return err;
		}
		
		newValue.Append(entityVal);
	}
	
	newValue.Append(str.String() + oldOffset, str.Length()-oldOffset);
	str.Adopt(newValue);
	return B_OK;
}


// =====================================================================
static status_t
expand_entities(BXMLParseContext * context, BString & str, char delimiter)
{
	status_t err;
	
	BString entity;
	BString entityVal;
	BString newValue("");
	
	int32 offset = 0, oldOffset = 0;
	int32 end = -1;
	
	while (true)
	{
		oldOffset = end + 1;
		offset = str.FindFirst(delimiter, oldOffset);
		if (offset < 0)
			break;
		end = str.FindFirst(';', offset);
		if (end < 0)
			break;
		
		newValue.Append(str.String() + oldOffset, offset-oldOffset);
		str.CopyInto(entity, offset+1, end-offset-1);
		
		if (entity.ByteAt(0) == '#' && delimiter == '&')
		{
			err = expand_char_ref(entity, entityVal);
			if (err != B_OK)
			{
				if (B_OK != (err = context->OnError(err, false, __LINE__)))
					return err;
			}
		}
		else
		{
			if (delimiter == '%')
				err = context->OnParameterEntityRef(entity, entityVal);
			else
				err = context->OnGeneralParsedEntityRef(entity, entityVal);
			if (err != B_OK)
			{
				if (B_OK != (err = context->OnError(err, false, __LINE__)))
					return err;
			}
		}
		
		newValue.Append(entityVal);
	}
	
	newValue.Append(str.String() + oldOffset, str.Length()-oldOffset);
	str.Adopt(newValue);
	return B_OK;
}



// =====================================================================
//            PUBLIC FUNCTIONS
// =====================================================================


// =====================================================================
status_t
ParseXML(BDocument * document, IByteInput::arg data, uint32 flags)
{
	uint32 realFlags = flags | B_XML_HANDLE_ATTRIBUTE_ENTITIES | B_XML_HANDLE_CONTENT_ENTITIES;
	BXMLDocumentParseContext context(document, NULL, NULL);
	BXMLIByteInputSource source(data);
	return _do_the_parsing_yo_(&source, &context, false, realFlags);
}


// =====================================================================
status_t
ParseXML(BDocument * document, const char * data, int32 length, uint32 flags)
{
	uint32 realFlags = flags | B_XML_HANDLE_ATTRIBUTE_ENTITIES | B_XML_HANDLE_CONTENT_ENTITIES;
	BXMLDocumentParseContext context(document, NULL, NULL);
	BXMLBufferSource source(data, length);
	return _do_the_parsing_yo_(&source, &context, false, realFlags);
}


// =====================================================================
status_t
ParseXML(BDocument * document, BXMLDataSource * data, uint32 flags)
{
	uint32 realFlags = flags | B_XML_HANDLE_ATTRIBUTE_ENTITIES | B_XML_HANDLE_CONTENT_ENTITIES;
	BXMLDocumentParseContext context(document, NULL, NULL);
	return _do_the_parsing_yo_(data, &context, false, realFlags);
}


// =====================================================================
status_t
ParseXML(BXMLParseContext * context, BXMLDataSource * data, uint32 flags)
{
	uint32 realFlags = flags | B_XML_HANDLE_ATTRIBUTE_ENTITIES | B_XML_HANDLE_CONTENT_ENTITIES;
	return _do_the_parsing_yo_(data, context, false, realFlags);
}


}; // namespace XML
}; // namespace B

