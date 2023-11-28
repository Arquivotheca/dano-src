// ===========================================================================
//	Parser.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Parser.h"
#include "Builder.h"
#include "HTMLTags.h"
#include "HTMLWindow.h"
#include "NPApp.h"
#include "MessageWindow.h"

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <UTF8.h>
#include <Debug.h>

inline void
X2UTF8(
	uint32		conversion,
	const char	*text,
	int32		length,
	BString		&result)
{
	int32 convState = 0;

	int32 i = 0;
	while (i < length) {
		int32	srcLen = length - i;
		int32	dstLen = 256;
		char	dst[256 + 1];
		if ( convert_to_utf8(conversion, 
							 text + i, 
							 &srcLen, 
							 dst, 
							 &dstLen,
							 &convState) != B_NO_ERROR )
			break;

		dst[dstLen] = '\0';
		result += dst;
	
		i += srcLen;		
	}
}


inline int32
SkipJISEscape(
	char	theChar,
	bool	*multibyte)
{
	int32 offset = 0;

	if ((theChar == '$') || (theChar == '('))
		offset = 3;

	if ((theChar == 'K') || (theChar == '$'))
		*multibyte = true;
	else
		*multibyte = false;

	return (offset);
} 



// ===========================================================================
//	Big str for big tags

BigStr::BigStr() : mStr(0),mCount(0),mSize(0)
{
}

BigStr::~BigStr()
{
	if (mStr)
		free(mStr);
}

void BigStr::Add(char c)
{
	if (mCount == mSize) {
		if (mStr == NULL) {
			mStr = (char *)malloc(1024);
//			NP_ASSERT(mStr);
			mSize = 1024;
		} else {
			char *s = (char *)malloc(mSize + 1024);
//			NP_ASSERT(s);
			memcpy(s,mStr,mCount);
			free(mStr);
			mStr = s;
			mSize += 1024;
			pprint("BigStr: Bumped to %d",mSize);
		}
	}
	mStr[mCount++] = c;
}

void BigStr::AddStr(char *s)
{
	while (*s) {	// Really slow but virtually never gets called
		Add(*s);
		s++;
	}
}

char BigStr::Char(const int index)
{
	if (index >= mCount)
		return 0;
	return mStr[index];
}

int BigStr::Finish()
{
	int count = mCount;
	Add(0);
	mCount = 0;
	return count;
}

// ============================================================================
// ============================================================================

Parser::Parser(DocumentBuilder *builder, uint32 encoding)
{
	mBuilder = builder;
	mEncoding = encoding;

	SetEncoding(encoding);

	mInJIS = false;

	mInQuote = 0;
	mInOption = 0;
	mCommentState = kNoComment;
	mNetscapeCommentText = "";
	mInBody = 0;
	mInTag = 0;
	mIgnoreTags = 0;

	mPRECount = 0;
	mInTextArea = false;
	mInStyle = false;
	mInSelect = false;
	mInScript = false;
	mInNoScriptPos = -1;
	
	// Start off by stripping leading spaces until we get some real content.
	mStripSpaces = true;
	mInAmpersand = false;
	mIgnoreSemiColon = false;
	mAmpersandCount = 0;

	mTextCount = 0;
	
#ifdef JAVASCRIPT
	mTempBuffer = 0;
#endif

	//int mCommentBeginCount = 0;
	//int mCommentEndCount = 0;
//	bool mInTagSingleQuote = 0;
//	bool mInTagDoubleQuote = 0;
//	int mLastChar = 0;
//	bool mLastCharNotSpace = 0;
}

Parser::~Parser()
{
//	mBuilder->Finalize();
	delete mBuilder;
#ifdef JAVASCRIPT
	if (mTempBuffer)
		delete mTempBuffer;
#endif
}


void Parser::SetComplete(bool complete)
{
	Consumer::SetComplete(complete);
	if (complete) {
		Complete();
		mBuilder->Finalize();
		Kill();
	}
}

void
Parser::SetEncoding(
	uint32	encoding)
{
	mEncoding = encoding;

	if (mBuilder != NULL)
		mBuilder->SetEncoding(mEncoding);
}

//	Ignore tags to parse as text

void Parser::IgnoreTags()
{
	Style textStyle = {0,1,0,0,0,0,0,0,0,1,0,0,0};	// Size 2, mono, pre
	mIgnoreTags = true;
	mPRECount = 1;
	mBuilder->AddTag(T_PRE,NULL);
	mBuilder->SetTagStyle(textStyle,true);
}

// ============================================================================
//	Lookup tag name and determine ID

short Parser::GetTagID()
{
	short 	neg,tag;
	char	tagName[32];
	
	char *tagStr = mTag.Chars();
	if ((bool)(neg = (*tagStr == '/')))
		tagStr++;

	char *tn = tagName;
	char *ts = tagStr;
	int length = 0;
	while (*ts && !isspace(*ts) && length++ < 30) {
		*tn++ = *ts++;
	}

	*tn = 0;

	if ((bool)(tag = LookupTag(tagName)))
		return neg ? -tag : tag;

	//pprintBig("Parser::GetTagID: '%s' not recognized",tagName);
	return T_UNKNOWN;
}

// ============================================================================
//	Break a tag string into pieces

char* Parser::NextAttrStrs(char *s, long *attributeID, char **valueStr, char **attrStr)
{
	char c;
	
	*attributeID = A_BAD;
	*valueStr = NULL;
	*attrStr = NULL;
	
	while (isspace(c = *s))
		s++;						// Strip leading spaces
	if (!c)
		return NULL;				// End of tag

//	Get Attribute ID

	char *attributeStr = s;
	while ((bool)(c = *s++)) {
		if (isspace(c) || c == '=') {
			s[-1] = 0;
			break;
		}
	}
	*attrStr = attributeStr;
	
	*attributeID = LookupAttribute(attributeStr);
	if (*attributeID == A_BAD)
		pprint("Unrecognized '%s' atttribue in '%s'",attributeStr,mTag.Chars());
		
//	 .. determine if there is a ' =value'

	bool attrIsNumeric = HasNumericValue(*attributeID);

	while (isspace(c))					// Strip leading spaces before '='
		c = *s++;
	if (c != '=')						// No '=' here
		return s-1;						// Attribute has no value, point to start of next attribute
	while (isspace(c = s[0]))
		s++;							// Strip spaces after '='
//	Process quoted values

	if (c == '\'' || c == '"' || c == '[') {	// Quoted values
		char quoteChar;
		quoteChar = (c == '[') ? ']' : c;		// What the string will (hopefully) terminate with
		s++;
		
// This keeps things like ATTR='' from working.  We really need to keep a stack of quote chars instead.
//		if (s[0] == '\'') {						// •••••••••• Some MetaTags include shit like CONTENT="'"Silly" "nonsense"'>
//			quoteChar = '\'';
//			s++;
//		}
		
		//we need to handle brain dead attributes that have unescaped 
		//quotes in them so that we absorb quotes without spaces after them
		*valueStr = s;
		while (s[0] && (s[0] != quoteChar || (!attrIsNumeric && s[0] == quoteChar && (!isspace(s[1]) && s[1] != 0) ))) {
			if (s[0] == '\n' || (s[0] == '\r')) {	// Remove line breaks from quoted strings
				for (int i = 0; s[i]; i++)
					s[i] = s[i + 1];
			} else
				s++;
		}
	} else {									// Space delimited values
		*valueStr = s;
		while (s[0] && !isspace(s[0]))
			s++;
	}
	
	if (s[0] != 0) {	// If we did't finish the string, terminate valueStr
		s[0] = 0;
		s++;
	}
	return s;
}

typedef struct {
	const char*		mName;
	long			mValue;
} ColorNameValuePair;

static const ColorNameValuePair gColorNameMap[] = {
	{"aliceblue", 0xf0f8ff},		{"antiquewhite", 0xfaebd7},		{"aqua", 0x00ffff},
	{"aquamarine", 0x7fffd4},		{"azure", 0xf0ffff},			{"beige", 0xf5f5dc},
	{"bisque", 0xffe4c4},			{"black", 0x000000},			{"blanchedalmond", 0xffebcd},
	{"blue", 0x0000ff},				{"blueviolet", 0x8a2be2},		{"brown", 0xa52a2a},
	{"burlywood", 0xdeb887},		{"cadetblue", 0x5f9ea0},		{"chartreuse", 0x7fff00},
	{"chocolate", 0xd2691e},		{"coral", 0xff7f50},			{"cornflowerblue", 0x6495ed},
	{"cornsilk", 0xfff8dc},			{"crimson", 0xdc143c},			{"cyan", 0x00ffff},
	{"darkblue", 0x00008b},			{"darkcyan", 0x008b8b},			{"darkgoldenrod", 0xb8860b},
	{"darkgray", 0xa9a9a9},			{"darkgreen", 0x006400},		{"darkgrey", 0xa9a9a9},
	{"darkkhaki", 0xbdb76b},		{"darkmagenta", 0x8b008b},		{"darkolivegreen", 0x556b2f},
	{"darkorange", 0xff8c00},		{"darkorchid", 0x9932cc},		{"darkred", 0x8b0000},
	{"darksalmon", 0xe9967a},		{"darkseagreen", 0x8fbc8f},		{"darkslateblue", 0x483d8b},
	{"darkslategray", 0x2f4f4f},	{"darkslategrey", 0x2f4f4f},	{"darkturquoise", 0x00ced1},
	{"darkviolet", 0x9400d3},		{"deeppink", 0xff1493},			{"deepskyblue", 0x00bfff},
	{"dimgray", 0x696969},			{"dimgrey", 0x696969},			{"dodgerblue", 0x1e90ff},
	{"firebrick", 0xb22222},		{"floralwhite", 0xfffaf0},		{"forestgreen", 0x228b22},
	{"fuchsia", 0xff00ff},			{"gainsboro", 0xdcdcdc},		{"ghostwhite", 0xf8f8ff},
	{"gold", 0xffd700},				{"goldenrod", 0xdaa520},		{"gray", 0x808080},
	{"green", 0x008000},			{"greenyellow", 0xadff2f},		{"grey", 0x808080},				
	{"honeydew", 0xf0fff0},			{"hotpink", 0xff69b4},			{"indianred", 0xcd5c5c},
	{"indigo", 0x4b0082},			{"ivory", 0xfffff0},			{"khaki", 0xf0e68c},
	{"lavender", 0xe6e6fa},			{"lavenderblush", 0xfff0f5},	{"lawngreen", 0x7cfc00},
	{"lemonchiffon", 0xfffacd},		{"lightblue", 0xadd8e6},		{"lightcoral", 0xf08080},
	{"lightcyan", 0xe0ffff},		{"lightgoldenrodyellow", 0xfafad2},	{"lightgray", 0xd3d3d3},
	{"lightgreen", 0x90ee90},		{"lightgrey", 0xd3d3d3},		{"lightpink", 0xffb6c1},
	{"lightsalmon", 0xffa07a},		{"lightseagreen", 0x20b2aa},	{"lightskyblue", 0x87cefa},
	{"lightslategray", 0x778899},	{"lightslategrey", 0x778899},	{"lightsteelblue", 0xb0c4de},
	{"lightyellow", 0xffffe0},		{"lime", 0x00ff00},				{"limegreen", 0x32cd32},
	{"linen", 0xfaf0e6},			{"magenta", 0xff00ff},			{"maroon", 0x800000},
	{"mediumaquamarine", 0x66cdaa}, {"mediumblue", 0x0000cd},		{"mediumgreen", 0x00fa9a},
	{"mediumorchid", 0xba55d3},		{"mediumpurple", 0x9370db},		{"mediumseagreen", 0x3cb371},
	{"mediumslateblue", 0x7b68ee},	{"mediumspringgreen", 0x0fa9a},	{"mediumturquoise", 0x48d1cc},
	{"mediumvioletred", 0xc71585},	{"midnightblue", 0x191970},		{"mintcream", 0xf5fffa},
	{"mistyrose", 0xffe4e1},		{"moccasin", 0xffe4b5},			{"navajowhite", 0xffdead},
	{"navy", 0x000080},				{"navyblue", 0x000080},			{"oldlace", 0xfdf5e6},
	{"olive", 0x808000},			{"olivedrab", 0x6b8e23},		{"orange", 0xffa500},
	{"orangered", 0xff4500},		{"orchid", 0xda70d6},			{"palegoldenrod", 0xeee8aa},
	{"palegreen", 0x98fb98},		{"paleturquoise", 0xafeeee},	{"palevioletred", 0xdb7093},
	{"papayawhip", 0xffefd5},		{"peachpuff", 0xffdab9},		{"peru", 0xcd853f},
	{"pink", 0xffc0cb},				{"plum", 0xdda0dd},				{"powderblue", 0xb0e0e6},
	{"purple", 0x800080},			{"red", 0xff0000},				{"rosybrown", 0xbc8f8f},
	{"royalblue", 0x4169e1},		{"saddlebrown", 0x8b4513},		{"salmon", 0xfa8072},
	{"sandybrown", 0xf4a460},		{"seagreen", 0x2e8b57},			{"seashell", 0xfff5ee},
	{"sienna", 0xa0522d},			{"silver", 0xc0c0c0},			{"skyblue", 0x87ceeb},
	{"slateblue", 0x6a5acd},		{"slategray", 0x708090},		{"slategrey", 0x708090},
	{"snow", 0xfffafa},				{"springgreen", 0x00ff7f},		{"steelblue", 0x4682b4},
	{"tan", 0xd2b48c},				{"teal", 0x008080},				{"thistle", 0xd8bfd8},
	{"tomato", 0xff6347},			{"turquoise", 0x40e0d0},		{"violet", 0xee82ee},
	{"wheat", 0xf5deb3},			{"white", 0xffffff},			{"whitesmoke", 0xf5f5f5},
	{"yellow", 0xffff00},			{"yellowgreen", 0x9acd32}
};

#define	kColorNameCount (sizeof(gColorNameMap)/sizeof(ColorNameValuePair))

//	Extract a color from a string .. "#RRGGBB", #RRGGBB, "RRGGBB", RRGGBB, "#RR GG BB", etc

long ParseColor(const char *valueStr)
{
	// A surprising number of sites use the letter 'O' instead of the number
	// zero in color strings.  May the fleas of a thousand camels infest the pubic
	// hair of the site authors responsible.

	BString mungedValueStr(valueStr);
	mungedValueStr.ReplaceAll('O','0');
	mungedValueStr.RemoveAll("\"");

	long value;
	int32 count = mungedValueStr.Length();
		
	if (mungedValueStr[0] == '#') {
		if (sscanf(mungedValueStr.String(),"#%lX",&value) == 1)
			return value;
		
		long r,g,b;
		if (sscanf(mungedValueStr.String(),"#%lX %lX %lX",&r,&g,&b) == 3) {		// Metrowerks does this
			value = (r << 16) | (g << 8) | b;						// Big/Little
			return value;
		}
		pprint("Error Parsing color value for %s!",mungedValueStr.String());
		return -1;
	}

//	Color lacks leading #

	if (count == 6 || count == 8) {
//		const char* s = SkipChars(mungedValueStr,"0123456789abcdefABCDEF ");		// Hex only
//		if (s[0] == 0) {
			if (sscanf(mungedValueStr.String(),"%lX",&value) == 1)
				return value;
//		}
	}
	
//	Try looking up the color value by name
	
	mungedValueStr = valueStr;
	mungedValueStr.ToLower();

	for (unsigned int i = 0; i < kColorNameCount; i++)
		if (mungedValueStr == gColorNameMap[i].mName)
			return gColorNameMap[i].mValue;
			
//	Failed. Bail
		
	pprint("Error Parsing color value for %s!",mungedValueStr.String());
	return 0x000000;	// Black
}

long ParseNumericValue(const char* valueStr, long& value, bool& isPercentage)
{
	isPercentage = false;
	value = -1;
	
	int i = 0;
	while (valueStr[i] && strchr("#-+%0123456789abcdefABCDEF",valueStr[i]))
		i++;

	// Some sites have bad html like <TABLE WIDTH="90% BORDER="3" ...> and the other browsers
	// basically deal with it.  We'll try to do the best we can rather than drop the whole
	// attribute on the floor.  If we find a space within a numeric value, don't reject it,
	// but stop parsing it right there.  We probably won't be able to pick up and parse the
	// next attribute correctly, but it's better than nothing.
	
	if (valueStr[i] != 0 && valueStr[i] != ' ' && i == 0)
		return false;		// Found some other character...
							// But don't punt if the first digit is valid.  Parse as much of it
							// as you can.
		
	if (sscanf(valueStr, "#%lx", &value) != 1 &&
		sscanf(valueStr, "%ld", &value) != 1 &&
		sscanf(valueStr, "%lx", &value) != 1)
			return false;
			
	isPercentage = strchr(valueStr, '%') || strchr("+-", *valueStr);
	return true;
}
// ============================================================================
//	Digest tag attributes

Tag *Parser::BuildTag(short tagID)
{
	long	attributeID,value;
	char*	valueStr;
	char*	attrNameStr;
	Tag		*tagList = NULL,*tag;
	bool isPercentage;
	char	*s;

	s = mTag.Chars() + strlen(TagName(tagID));

	while((*s) && (!isspace(*s)))
		s++;

	while ((bool)(s = NextAttrStrs(s,&attributeID,&valueStr,&attrNameStr))) {
		tag = new(Tag);

		// weird dependencies, don't try to optimize
		BString		utf8Text;
		const char	*attrStr = "";
		int32		valueStrLen = (valueStr != NULL) ?
									strlen(valueStr) : 0;
		
		if (valueStrLen > 0) {
			if(attributeID == A_VALUE) //do the &encoding conversion for form default values
				ConvertTextFromEncoding(utf8Text, valueStr, valueStrLen);
			else
				ConvertTextToUTF8(valueStr, valueStrLen, utf8Text);

			attrStr = utf8Text.String();
		}

		tag->SetOrigAttribute(attrNameStr, valueStr);
		
		
		switch (attributeID) {
			case A_ALIGN:				// Have values like "TOP", "LEFT" etc
			case A_VALIGN:
			case A_CLEAR:
			case A_METHOD:				// AV_GET, AV_POST
			case A_SHAPE:				// AV_RECT
			case A_VALUETYPE:			// AV_DATA,AV_REF,AV_OBJECT
			case A_WRAP:
			case A_SCROLLING:
			case A_VISIBILITY:
			case A_SHOWTITLEINTOOLBAR:
			case A_SHOWTOOLBAR:
				value = LookupAttributeValue(valueStr);
				tag->SetAttribute(attributeID,value,0);
				break;

			case A_TYPE:				// AV_CHECKBOX,AV_HIDDEN,AV_IMAGE,AV_RADIO,AV_RESET,AV_SUBMIT,AV_TEXT
				switch (tagID) {
					case T_DL:				// Lists
					case T_OL:
					case T_UL:
					case T_LI:
					case T_OBJECT:
					case T_EMBED:
					{
						// weird dependencies, don't try to optimize
						BString		utf8Text;
						const char	*attrStr = "";
						int32		valueStrLen = (valueStr != NULL) ? 
													strlen(valueStr) : 0;
					
						if (valueStrLen > 0) {
							ConvertTextToUTF8(valueStr, valueStrLen, utf8Text);
							attrStr = utf8Text.String();
						}
			
						tag->SetAttributeStr(attributeID, attrStr);
						break;
					}
					default:
						value = LookupAttributeValue(valueStr);
						tag->SetAttribute(attributeID,value,0);
				}
				break;

			case A_BGCOLOR:					// Colors are hex
			case A_TEXT:
			case A_LINK:
			case A_ALINK:
			case A_VLINK:
			case A_COLOR:
				if (attributeID == A_BGCOLOR && !gPreferences.FindBool("UseBGColors"))
					break;
				if ((attributeID == A_TEXT || attributeID == A_COLOR) && !gPreferences.FindBool("UseFGColors"))
					break;
				if (valueStr && strlen(valueStr) > 0) {
					value = ParseColor(valueStr);
					tag->SetAttribute(attributeID, value, false);
				}
				break;

			case A_BAD:
				break;

			case A_HREF: {
				if (valueStr && *valueStr) {
					//This functionality has been moved into NextAttrStrs since this causes problems with
					//unquoted hrefs and later attributes with quotes.
					// Some sites don't escape double-quotes in URL's, which makes for illegal, immoral
					// HTML, but we have to deal with it.  If we're parsing an HREF, peek ahead into the
					// HTML that follows.  If we don't find a space character, then we'll assume
					// that we've got an illegal quote in the middle, and we'll coalesce the following
					// bits until we find a space.
					//BString URL = valueStr;
					//bool keepGoing = true;
					//while (keepGoing) {
					//	if (!isspace(*s)) {
					//		char *nextPos = strchr(s, '"');
					//		if (nextPos) {
					//			*nextPos = 0;
					//			URL += "%22";
					//			URL += s;
					//			s = nextPos + 1;
					//		} else keepGoing = false;
					//	} else keepGoing = false;
					//}
					
					tag->SetAttributeStr(attributeID, valueStr);
					break;
				}
			}
				
			case A_SIZE:
				if (valueStr) {
				// SPECIAL CASE for size attributes.  Some sites use a bogus SIZE=nn,n notation.
				// we want to use only the first number.
				
				char *s = strchr(valueStr, ',');
				if (s)
					*s = 0;

				// Fall through to be handled by default case.
				}
				
			// SPECIAL CASE for cols/rows, behavior different for framesets
			case A_COLS:
			case A_ROWS:			
				if (tagID == T_FRAMESET) {
					tag->SetAttributeStr(attributeID, valueStr);
					break;
				}
				
				// else fall thru

			default:
				value = -1;								// Should default to something else?
				if (HasNumericValue(attributeID))  {
					if (valueStr) {
						if (ParseNumericValue(valueStr,value,isPercentage) == false)
							pprintBig("HTMLParser::BuildTag: cannot parse %s=%s", AttributeName(attributeID), valueStr);
						isPercentage = strchr(valueStr, '%') || strchr("+-", *valueStr);
					} else {
						//value = 0;			// default to -1 for missing values
						isPercentage = false;
						pprint("HTMLParser::BuildTag: Missing value for '%s'",AttributeName(attributeID));
					}
					tag->SetAttribute(attributeID, value, isPercentage);
				} else {
					// weird dependencies, don't try to optimize
					BString		utf8Text;
					const char	*attrStr = "";
					int32		valueStrLen = (valueStr != NULL) ?
												strlen(valueStr) : 0;
					
					if (valueStrLen > 0) {
						if(attributeID == A_VALUE){ //do the &encoding conversion for form default values
							ConvertTextFromEncoding(utf8Text, valueStr, valueStrLen);
						} else {
							ConvertTextToUTF8(valueStr, valueStrLen, utf8Text);
						}					
						attrStr = utf8Text.String();
					}

					tag->SetAttributeStr(attributeID, attrStr);
				}
				break;
		}
		AddOrSet(tagList,tag);
	}
	return tagList;
}

// ============================================================================
//	CheckTag removes useless runs of <P><P><P> .. etc
//	It ignores comments, and sets mInBody to avoid picking up extra spaces


void Parser::CheckTag(short tagID)
{
	short	absTagID = tagID < 0 ? -tagID : tagID;

	// Set fStripSpaces to remove extra spaces before body text ...

	switch (absTagID) 
	{
		case T_OPTION:
			mInOption = true;
		case T_HTML:
		case T_HEAD:
		case T_TITLE:
		case T_BASE:
		case T_BODY:
		case T_BGSOUND:
		case T_H1:		// Headings
		case T_H2:
		case T_H3:
		case T_H4:
		case T_H5:
		case T_H6:
		case T_BLOCKQUOTE:
		case T_DL:		// Glossary List
		case T_OL:		// Ordered List
		case T_UL:		// Unordered List
		case T_L:		// Unordered List
		case T_DIR:
		case T_MENU:
		case T_DT:		// Glossary List term
		case T_DD:		// Glossary List def
		case T_LI:		// List Item
		case T_P:
		case T_DIV:
		case T_BR:
		case T_HR:
		case T_TABLE:
		case T_TH:
		case T_TR:
		case T_TD:
		case T_FORM:
		case T_SELECT:
		case T_EMBED:
		case T_MAP:
		case T_IMAGE:
		case T_IMG:
		case T_AREA:
			mInBody = false;
			break;
			
	//	case T_IMG:
		case T_TEXTAREA:
			mInBody = true;
			break;
			
		case T_PRE:
		case T_XMP:
		case T_LISTING:
			if (tagID > 0)
				mInBody = true;
			else 
				mInBody = false;
			break;
	}
}


// ============================================================================
//	DispatchTag tokenizes a tag and sends it to the glyph builder

bool Parser::DispatchTag()
{
	BString tagChars;
	if (mInTextArea) {
		tagChars = mTag.Chars();
		tagChars.Truncate(mTag.Count());
	}
		
	mTag.Finish();
	
//	pprint("TAG:<%s>",mTag.Chars());
	
	short tagID = GetTagID();
	
	// Process all tags as text while in text area or style
	
	if ((mInTextArea && -tagID != T_TEXTAREA) || (mInStyle && -tagID != T_STYLE) || (mInScript && -tagID != T_SCRIPT)) {
		mBuilder->AddText("<", 1);

		// weird dependencies, don't try to optimize
		BString		utf8Text;
		const char	*theText = "";
		int32		theTextLen = tagChars.Length();
					
		if (theTextLen > 0) {
			ConvertTextToUTF8(tagChars.String(), theTextLen, utf8Text);
			theText = utf8Text.String();
			theTextLen = utf8Text.Length();
		}
		mBuilder->AddText(theText, theTextLen);
		mBuilder->AddText(">", 1);
		return true;
	}
	
	if (tagID == T_UNKNOWN)
		return false;
		
	CheckTag(tagID);
	if (tagID > 0) {
		if (tagID == T_PRE || tagID == T_XMP || tagID == T_LISTING)
			mPRECount++;
		if (tagID == T_TEXTAREA && !mInTextArea) {
			mInTextArea = true;
			mPRECount++;
		}
		if (tagID == T_STYLE)
			mInStyle = true;
		if (tagID == T_SELECT)
			mInSelect = true;
		if (tagID == T_SCRIPT)
			mInScript = true;
		if (tagID == T_NOSCRIPT)
			mInNoScriptPos = 0;
			
		Tag* tag = BuildTag(tagID);			// Collect the attributes for this tag
		mBuilder->AddTag(tagID,&tag);		// Add it to builder
		if (tag)
			tag->DeleteAll();
	} else {
		if ((-tagID == T_PRE || -tagID == T_XMP || -tagID == T_LISTING) && mPRECount > 0)
			mPRECount--;
		if (-tagID == T_TEXTAREA && mInTextArea) {
			mInTextArea = false;
			if (mPRECount > 0)
				mPRECount--;
		}
		if (-tagID == T_STYLE)
			mInStyle = false;
		if (-tagID == T_SCRIPT) {
			mInScript = false;
			mStripSpaces = true;
		}
		if (-tagID == T_SELECT) {
			mInOption = false;
			mInSelect = false;
		}
		mBuilder->AddTag(tagID, NULL);
	}
	
	return true;
}

// ============================================================================
// Write a chunk of text to the page

void Parser::DispatchText()
{
	if (mTextCount == 0)
		return;

	mText[mTextCount] = 0;
	
	BString utf8Text;
	ConvertTextToUTF8(mText, mTextCount, utf8Text);
	mBuilder->AddText(utf8Text.String(), utf8Text.Length());

	mLastCharNotSpace = mText[mTextCount - 1] != ' ';
	mTextCount = 0;
	mInOption = false;
}


// ============================================================================
//	ISO8859-1 charset

typedef struct EntityEntry {
	const char *entityName;
	int32		entityCode;
} EntityEntry;

const EntityEntry	gEntities[] = {
{"AElig",198},	{"Aacute",193},	{"Acirc",194},	{"Agrave",192},	{"Alpha",913},	{"Aring",197},	
{"Atilde",195},	{"Auml",196},	{"Beta",914},	{"Ccedil",199},	{"Chi",935},	{"Dagger",8225},
{"Delta",916},	{"ETH",208},	{"Eacute",201},	{"Ecirc",202},	{"Egrave",200},	{"Epsilon",917},
{"Eta",919},	{"Euml",203},	{"Gamma",915},	{"Iacute",205},	{"Icirc",206},	{"Igrave",204},	
{"Iota",921},	{"Iuml",207},	{"Kappa",922},	{"Lambda",923},	{"Mu",924},		{"Ntilde",209},	
{"Nu",925},		{"OElig",338},	{"Oacute",211},	{"Ocirc",212},	{"Ograve",210},	{"Omega",937},	
{"Omicron",927},{"Oslash",216},	{"Otilde",213},	{"Ouml",214},	{"Phi",934},	{"Pi",928},		
{"Prime",8243},	{"Psi",936},	{"Rho",929},	{"Scaron",352},	{"Sigma",931},	{"THORN",222},	
{"Tau",932},	{"Theta",920},	{"Uacute",218},	{"Ucirc",219},	{"Ugrave",217},	{"Upsilon",933},
{"Uuml",220},	{"Xi",926},		{"Yacute",221},	{"Yuml",376},	{"Zeta",918},	{"aacute",225},
{"acirc",226},	{"acute",180},	{"aelig",230},	{"agrave",224},	{"alefsym",8501},{"alpha",945},	
{"amp",38},		{"and",8743},	{"ang",8736},	{"aring",229},	{"asymp",8776},	{"atilde",227},	
{"auml",228},	{"bdquo",8222}, {"beta",946},	{"brvbar",166},	{"bull",8226},	{"cap",8745},	
{"ccedil",231},	{"cedil",184},	{"cent",162},	{"chi",967},	{"circ",710},	{"clubs",9827},	
{"cong",8773},	{"copy",169},	{"crarr",8629},	{"cup",8746},	{"curren",164},	{"dArr",8659},	
{"dagger",8224},{"darr",8595},	{"deg",176},	{"delta",948},	{"diams",9830},	{"divide",247},	
{"eacute",233},	{"ecirc",234},	{"egrave",232},	{"empty",8709},	{"emsp",8195},	{"ensp",8194},
{"epsilon",949},{"equiv",8801},	{"eta",951},	{"eth",240},	{"euml",235},	{"euro",8364},
{"exist",8707},	{"fnof",402},	{"forall",8704},{"frac12",189},	{"frac14",188},	{"frac34",190},	
{"frasl",8260},	{"gamma",947},	{"ge",8805},	{"gt",62},		{"hArr",8660},	{"harr",8596},	
{"hearts",9829},{"hellip",8230},{"iacute",237},	{"icirc",238},	{"iexcl",161},	{"igrave",236},	
{"image",8465},	{"infin",8734},	{"int",8747},	{"iota",953},	{"iquest",191},	{"isin",8712},	
{"iuml",239},	{"kappa",954},	{"lArr",8656},	{"lambda",955},	{"lang",9001},	{"laquo",171},
{"larr",8592},	{"lceil",8968},	{"ldquo",8220},	{"le",8804},	{"lfloor",8970},{"lowast",8727},
{"loz",9674},	{"lrm",8206},	{"lsaquo",8249},{"lsquo",8216},	{"lt",60},		{"macr",175},	
{"mdash",8212},	{"micro",181},	{"middot",183},	{"minus",8722},	{"mu",956},		{"nabla",8711},	
{"nbsp",160},	{"ndash",8211},	{"ne",8800},	{"ni",8715},	{"not",172},	{"notin",8713},	
{"nsub",8836},	{"ntilde",241},	{"nu",957},		{"oacute",243},	{"ocirc",244},	{"oelig",339},
{"ograve",242},	{"oline",8254},	{"omega",969},	{"omicron",959},{"oplus",8853},	{"or",8744},	
{"ordf",170},	{"ordm",186},	{"oslash",248},	{"otilde",245},	{"otimes",8855},{"ouml",246},	
{"para",182},	{"part",8706},	{"permil",8240},{"perp",8869},	{"phi",966},	{"pi",960},		
{"piv",982},	{"plusmn",177},	{"pound",163},	{"prime",8242},	{"prod",8719},	{"prop",8733},	
{"psi",968},	{"quot",34},	{"rArr",8658},	{"radic",8730},	{"rang",9002},	{"raquo",187},	
{"rarr",8594},	{"rceil",8969},	{"rdquo",8221},	{"real",8476},	{"reg",174},	{"rfloor",8971},
{"rho",961},	{"rlm",8207},	{"rsaquo",8250},{"rsquo",8217},	{"sbquo",8218},	{"scaron",353},	
{"sdot",8901},	{"sect",167},	{"shy",173},	{"sigma",963},	{"sigmaf",962},	{"sim",8764},	
{"spades",9824},{"sub",8834},	{"sube",8838},	{"sum",8721},	{"sup",8835},	{"sup1",185},	
{"sup2",178},	{"sup3",179},	{"supe",8839},	{"szlig",223},	{"tau",964},	{"there4",8756},
{"theta",952},	{"thetasym",977},{"thinsp",8201},{"thorn",254},	{"tilde",732},	{"times",215},	
{"trade",8482},	{"uArr",8657},	{"uacute",250},	{"uarr",8593},	{"ucirc",251},	{"ugrave",249},
{"uml",168},	{"upsih",978},	{"upsilon",965},{"uuml",252},	{"weierp",8472},{"xi",958},		
{"yacute",253},	{"yen",165},	{"yuml",255},	{"zeta",950},	{"zwj",8205},	{"zwnj",8204},	
{NULL,0}
};

class EntityComparison {
public:
	const char *string;
	int			count;
};

int CompareEntities(const void *key, const void *member)
{
	EntityComparison *keyEntry = (EntityComparison *)key;
	EntityEntry *memberEntry = (EntityEntry *)member;
	if (memberEntry->entityName == NULL)
		return -1;
	if (keyEntry->count > 0)
		return strncmp(keyEntry->string, memberEntry->entityName, keyEntry->count);
	else
		return strcmp(keyEntry->string, memberEntry->entityName);
}

/*
#ifdef WINDOWS
#define MAPCHAR(_x) (_x + 128)	// I think the windows charset matches
#else

//	Maps 8859-1 to Mac character set

#define MAPCHAR(_x) Lookup128To255[_x]


char Lookup128To255[] = {

	' ',	' ',	0xE2,	0xC4,	0xE3,	0xC9,	0xA0,	0xE0,	// 128
	0xF6,	0xE4,	0x54,	0xDC,	0xCE,	' ',	' ',	' ',	// 136
	' ',	0xD4,	0xD5,	0xD2,	0xD3,	0xE1,	0xD0,	0xD1,	// 144
	0xF7,	0xAA,	0x73,	0xDD,	0xCF,	' ',	' ',	0xD9,	// 152
	
	' ',	'¡',	'¢',	'£',	'€',	'¥',	'|',	'§',	// 160	// NOT A REAL NON_BREAKING SPACE...SHOULD ADD GLYPH?
	'¨',	'©',	'ª',	'«',	'¬',	'–',	'®',	'¯',	// 168
	'°',	'±',	'2',	'3',	'´',	'µ',	'¶',	'·',	// 176
	'¸',	'1',	'º',	'»',	'q',	'h',	't',	'¿',	// 184

	'À',	'Á',	'Â',	'Ã',	'Ä',	'Å',	'Æ',	'Ç',	// 192
	'È',	'É',	'Ê',	'Ë',	'Ì',	'Í',	'Î',	'Ï',	// 200
	'›',	'Ñ',	'Ò',	'Ó',	'Ô',	'Õ',	'Ö',	'x',	// 208
	'Ø',	'Ù',	'Ú',	'Û',	'Ü',	'Y',	'ﬁ',	'ß',	// 216

	'à',	'á',	'â',	'ã',	'ä',	'å',	'æ',	'ç',	// 224
	'è',	'é',	'ê',	'ë',	'ì',	'í',	'î',	'ï',	// 232
	'›',	'ñ',	'ò',	'ó',	'ô',	'õ',	'ö',	'÷',	// 240
	'ø',	'ù',	'ú',	'û',	'ü',	'y',	'ﬂ',	'ÿ'		// 248
};
#endif
*/

/*
typedef struct {
	short	cNum;
	char	*cStr;
} charInfo;

charInfo gCommonChars[] = {
	{'"',"quot"		},	{'&',"amp"		},	// Make sure these work for Windows
	{'<',"lt"		},	{'>',"gt"		},
	{'©',"copy"		},	{'®',"reg"		},
};
*/

//	#xxx entities

int32 NumberedChar(const char* d, bool* cantBe, uint32 encoding)
{
	*cantBe = false;
	d++;					//	Skip #
	int len = strlen(d);
	if (len < 2 || len > 15)
		return 0;			// n won't work, nn or nnn will
		
	int i;
	char num[16];
	for (i = 0; i < len; i++) {
		if (!isdigit(d[i])) {
			*cantBe = true;
			break;
		}
		num[i] = d[i];
	}
	num[i] = 0;
	
	if (i < 2) {			// Hit an alpha before end of second digit
		*cantBe = true;
		return 0;
	}
	
	long charNum = 0;
	sscanf(num, "%ld", &charNum);
	
	if (charNum > 255) {
		*cantBe = true;
		return 0;
	}
	
	char srcChar = charNum;
	char dstCharUTF8[5];
	uchar dstCharUnicode[2];
	int32 state;
	int32 srcLen = 1;
	int32 dstLenUTF8 = 5;
	int32 dstLenUnicode = 2;
	
	if (convert_to_utf8(encoding, &srcChar, &srcLen, dstCharUTF8, &dstLenUTF8, &state) == B_NO_ERROR &&
		convert_from_utf8(B_UNICODE_CONVERSION, dstCharUTF8, &dstLenUTF8, (char *)dstCharUnicode, &dstLenUnicode, &state) == B_NO_ERROR) {
		if (dstLenUnicode == 2)
			charNum = dstCharUnicode[0] << 8 | dstCharUnicode[1];
		else
			charNum = dstCharUnicode[0];
	}
	
#if 0
	if (len == 4 || charNum > 25) {	// three digit or a two digit that will exceed 255 if three
/*
		if (charNum >= 128) {
			if (encoding == B_MAC_ROMAN_CONVERSION)
				charNum = MAPCHAR(charNum-128);
		}
*/
		return charNum;
	}
	
	return 0;	// Wait for next char
#endif
	return charNum;
}

//	Return a char for a &tidle; like string in d

int32 AmpersandCharacter(const char* d, bool* cantBe, uint32 encoding)
{
	*cantBe = true;

	if (d[0] == '#')
		return NumberedChar(d,cantBe, encoding);

	if (!isalpha(*d)) {
		*cantBe = true;		// Must be &xxx where x is a char
		return 0;
	}
	
	unsigned int count = strlen(d);	
	if (count > 10) {
		*cantBe = true;		// No amp > 10 chars
		return 0;
	}
		
//	short i;
/*
	for (i = 0; i < 6; i++)
		if (!strncasecmp(d,gCommonChars[i].cStr,count)) {
			if (strlen(gCommonChars[i].cStr) == count)
				return gCommonChars[i].cNum;
			else
				return 0;	// Matches but 
		}
*/

/*
	for (i = 0; i < 96; i++)
		if (!strncmp(d,gISO8859Chars[i],count))	{ 		// 160 to 255
			if (count == strlen(gISO8859Chars[i])) {
				if (encoding == B_MAC_ROMAN_CONVERSION)
					return (MAPCHAR(i + 32));
				else
					return (i + 0xA0);
			}
			else
				return 0;
		}
*/
/*
	for (i = 0; gEntities[i].entityName; i++) {
		if (!strncmp(d,gEntities[i].entityName,count)) {
			*cantBe = false;
			if (count == strlen(gEntities[i].entityName))
				return gEntities[i].entityCode;
		}
	}
*/

	EntityComparison ec;
	ec.string = d;

	// First, run the search and see if we get a hit based on the first (count) characters.
	// Use the results to properly set cantBe.
	ec.count = count;
	EntityEntry *e = (EntityEntry *)bsearch(&ec, gEntities, sizeof(gEntities) / sizeof(EntityEntry), sizeof(EntityEntry), CompareEntities);
	if (!e) {
		*cantBe = true;
		return 0;
	} else
		*cantBe = false;

	// Now, do it again, but do a full comparison to see if there's an exact match.  If there is, return it.
	ec.count = 0;
	
	e = (EntityEntry *)bsearch(&ec, gEntities, sizeof(gEntities) / sizeof(EntityEntry), sizeof(EntityEntry), CompareEntities);
	if (e) {
		*cantBe = false;
		return e->entityCode;
	}
		

	return 0;
}

inline void Parser::AppendText(const char *text, size_t len)
{
	ASSERT(mTextCount < kMaxTextLength - 1);
	ASSERT(len < kMaxTextLength - 1);
	if (mTextCount + len >= kMaxTextLength - 1)
		DispatchText();
		
	memcpy(mText + mTextCount, text, len);
	mTextCount += len;
	if (mTextCount == kMaxTextLength - 1)
		DispatchText();
}

inline void Parser::AppendText(char c)
{
	ASSERT(mTextCount < kMaxTextLength - 1);

	mText[mTextCount++] = c;
	if (mTextCount == kMaxTextLength - 1)
		DispatchText();
}


#ifdef JAVASCRIPT
void Parser::ReentrantWrite(uchar *data, long count)
{
	if (!mTempBuffer)
		mTempBuffer = new BucketStore(1024);
	mTempBuffer->Write(data, count);
}
#endif

#define kMaxParseChunk 0x7FFFFFFF

//	Returns amount of data used

long Parser::Write(uchar * data, long count)
{
	int charRead = -1;
	uchar* d = (uchar *)data;
	uchar* dEnd = (uchar *)d + MIN(count, kMaxParseChunk);
	
#ifdef JAVASCRIPT
	bool usingTempBuffer = false;
	long tempPos = 0;
#endif

	int c;

	while (d < dEnd) {
		// Get Next Char
		if (charRead >= 0)
			mLastChar = charRead;

#ifdef JAVASCRIPT
		// If there's data in the temporary buffer, process it until it's gone before
		// we resume reading from the normal data stream.
		if (mTempBuffer) {
			if (!usingTempBuffer) {
				usingTempBuffer = true;
				tempPos = 0;
			}
			if (tempPos == mTempBuffer->GetLength()) {
				// Buffer has run dry, delete and resume normal operation.
				usingTempBuffer = false;
				delete mTempBuffer;
				mTempBuffer = NULL;
				charRead = c = *d++;
			} else {
				// We have to call GetData on the buffer each time through, since the buffer
				// may have grown and been reallocated since the last character we processed.
				charRead = c = *((uchar *)mTempBuffer->GetData(tempPos++, 1));
			}
		} else
			charRead = c = *d++;

		if (mInNoScriptPos >= 0) {
			const char *endNoScriptTag = "</NOSCRIPT>";
			
			char cc = c;
			if (cc >= 'a' && cc <= 'z')
				cc = cc - ('a' - 'A');
			if (cc == endNoScriptTag[mInNoScriptPos])
				mInNoScriptPos++;
			else
				mInNoScriptPos = 0;

			if (mInNoScriptPos == 11)
				mInNoScriptPos = -1;
			continue;
		}
#else
		charRead = c = *d++;
#endif

		bool charIsSpace = isspace(c);
	
		
		// Ignore control characters, with the exception of ESC (0x1B)
		if (mInScript && (c == '\r' || c == '\n')) {
			AppendText('\n');
			continue;
		}
		
		if ((c < ' ') && (!charIsSpace)) { 
			if ((c == 0x1B) && ((dEnd - d) > 1))
				SkipJISEscape(*((char *)d), &mInJIS);
			else
				continue;
		}


		// 	Skip comments

		if (mCommentState != kNoComment)	{	
			// Match Netscape's bizarre comment handling. Comments starting with "<!"
			// end with ">". Comments starting with "<!--" end with "-->", or with ">"
			// if no "-->" is found.

			switch (c) {
				case '>':
					if (mCommentState == kStandardComment) {
						mCommentState = kNoComment;
						break;	// break!!!
					}
					else {
						int32 length = mNetscapeCommentText.Length();

						if (length > 1) {
							const char *theText = mNetscapeCommentText.String();

							if ( (theText[length - 1] == '-') && 
								 (theText[length - 2] == '-') ) {
								mCommentState = kNoComment;
								mNetscapeCommentText = "";
								break; // break
							}
						}
					}
					// fall thru

				default:
					if (mCommentState == kNetscapeComment) {
						char moreText[2];
						moreText[0] = c;
						moreText[1] = '\0';
						mNetscapeCommentText += moreText;
					}
					break;

			}
			continue;
		}
	
		// Ignore semicolon left over from '&'
		
		if (mIgnoreSemiColon) {
			mIgnoreSemiColon = false;
			if (c == ';')
				continue;
		}

		if (mInScript && (c == '\n' || c == '&')) {
			AppendText(c);
			continue;
		}
		
		// If the last character was a space, strip additional spaces
		
		if (mStripSpaces && charIsSpace)
			 continue;

		mStripSpaces = false;
		
		// Strip white space if not preformatted
		
		bool pre = mPRECount && !mInSelect;	// No such thing as pre inside a <select> Tag
		if (!pre) {
			if (charIsSpace)	{
				if (mInTag) {
					// Pass spaces /t,/r,/n to tag
				} else {
					mStripSpaces = true;			
					c = ' ';
				}
			}
		} else {	// Tab to next 8 space alignment on tab in PRE.	
			if (c == '	') {
				for  (short tabCount = (7 - (mTextCount % 8)); tabCount > 0; tabCount--)
					AppendText(' ');
					
				c = ' ';
			}
		}

	//	Attempt to interpret string with each character, because ';' may be missing.


		if (mInAmpersand) {
			bool	cantBe = false;
			int32	unicodeChar = 0;

			if (c != ';')
				mAmpersand[mAmpersandCount++] = c;
			else {
				if (mAmpersandCount < 1) {
					mAmpersand[mAmpersandCount++] = c;
					cantBe = true;
					unicodeChar = 0;
				}
			}

			mAmpersand[mAmpersandCount] = '\0';
			if (mAmpersandCount > 10) {
				pprint("BadSize: %d, %s", mAmpersandCount, mAmpersand);
			}

//			c = AmpersandCharacter(mAmpersand,&cantBe, mEncoding);
			unicodeChar = AmpersandCharacter(mAmpersand,&cantBe, mEncoding);
						
			if (c != ';' && !cantBe && mAmpersandCount <= 10)
				continue;
				
			// Previously, it would match if the entity had an ending semicolon or
			// not.  However, we can't do this if we want to support HTML 4.0 entities,
			// because some share the first few characters, like &not; and &notin;.
			// We will enforce the semicolon rule better; we will try to end the entity only
			// when we hit an ampersand or an illegal character (where we will back up one)
			if (cantBe && mAmpersandCount > 0 && !mInTag) {
				char temp = mAmpersand[--mAmpersandCount];
				mAmpersand[mAmpersandCount] = 0;
				bool junk;
				unicodeChar = AmpersandCharacter(mAmpersand,&junk, mEncoding);
				// If we didn't get anything when we backed up one, then we will throw up
				// our hands, un-back it up, and give up.
				if (!unicodeChar)
					mAmpersand[mAmpersandCount++] = temp;
			}
//			else if (c != ';' && (cantBe || mAmpersandCount > 10) && mAmpersandCount > 1) {
//				mAmpersand[mAmpersandCount - 1] = 0;
//				unicodeChar = AmpersandCharacter(mAmpersand, &cantBe, mEncoding);
//				d--;	// Back up and un-eat the previous character.
//			}

			// Let &quote; entities in quoted strings in tags pass through unmolested.  This lets us
			// have quotes inside of form fields.
			if (mInTagDoubleQuote && unicodeChar == '"') {
				unicodeChar = 0;
				cantBe = true;
			}
			
			if (unicodeChar != 0) {
				if (mInTag) {
					// We're pretty much hosed here because we can't force the tag through with no conversion.
					// It'll get converted into garbage.
					if (unicodeChar > 255)
						mTag.Add((unicodeChar & 0xff00) >> 8);
					mTag.Add(unicodeChar & 0xff);
				}
				else {
					DispatchText();
					char text[3];
					text[0] = (unicodeChar & 0xff00) >> 8;
					text[1] = unicodeChar & 0xff;
					text[2] = 0;

					BString convText;
					X2UTF8(B_UNICODE_CONVERSION, text, 2, convText);
					mBuilder->AddText(convText.String(), convText.Length());
					mInBody = true;
				}
				
				//if (mAmpersandCount != 4)
				//	pprint("BadSize");
				//pprint("mAmpersand: '%s' got %d (%d)",mAmpersand,c,mAmpersandCount);
				
				char charOnEnd = mAmpersand[mAmpersandCount-1];
				mIgnoreSemiColon = true;
				mInAmpersand = false;

				// If unicodeChar is nonzero and cantBe is true, then it means we hit the end of an entity
				// without finding a semicolon, backed up one, and used the entity defined by that string.
				// In this case, unicodeChar contains the entity char and c contains the char that was rejected
				// off the end of the entity -- c must be handled like a normal character.
				if (!cantBe)
					unicodeChar = c = 0;
				
				if (mAmpersand[0] == '#') {
					if (!isdigit(charOnEnd) && charOnEnd != ';') {
						unicodeChar = c = charOnEnd;
						mIgnoreSemiColon = false;
					}
				}
			}
			else if (cantBe) {		// If this can't be a valid '&' string, Copy to text
				if (mInTag) {
					unicodeChar = c = mAmpersand[--mAmpersandCount];
					mAmpersand[mAmpersandCount] = 0;
				
					mTag.Add('&');					
					mTag.AddStr(mAmpersand);
				} else {
					AppendText('&');
					AppendText(mAmpersand, mAmpersandCount - 1);
					mInBody = true;
					unicodeChar = c = mAmpersand[mAmpersandCount-1];
				}
				mInAmpersand = false;
			}
			if (unicodeChar == 0)
				continue;
		}
		
//		Open and close tags, Dispatch text and process '&'
		

		switch (c)  {			
			case '<':
				if (mInJIS)
					goto DEFAULT;
	
				DispatchText();

				if (mIgnoreTags) {				// Dont process tags if we a just doing text
					AppendText(c);
					break;
				}

				// If we're in a tag quote, ignore this.
				
				if (mInTag && (mInTagSingleQuote || mInTagDoubleQuote)) {
					mTag.Add(c);
					break;
				}
					
				// If we already started a tag, try to handle the tag. If its
				// unknown, dispatch it as text.
				
				if (mInTag && !DispatchTag()) {
					AppendText('<');
					DispatchText();

					BString		utf8Text;
					int			count = mTag.Finish();
					const char	*theText = "";

					if (count > 0) {
						ConvertTextToUTF8(mTag.Chars(), count, utf8Text);
						theText = utf8Text.String();
						count = utf8Text.Length();
					}

					mBuilder->AddText(theText, count);
				}
				mInTag = true;
				mInTagDoubleQuote = false;
				mInTagSingleQuote = false;
				break;
				
			case '>':
				if (mInJIS)
					goto DEFAULT;

				if (mInTag) {
					if (!mInTagDoubleQuote && !mInTagSingleQuote) {
						mInTag = false; 
						DispatchTag();
					} else
						mTag.Add(c);			
				} else
					AppendText(c);

				break;
				
			case '&':
				if (mIgnoreTags) {
					AppendText('&');
					break;
				}
				if (mInAmpersand) {		// Clear if already in amp &&nbsp
					AppendText('&');
					AppendText(mAmpersand, mAmpersandCount);
					mInBody = true;
				}
				mInAmpersand = true;
				mAmpersandCount = 0;
				break;
				
//			Newlines in pre can't appear in select tags

			case '\n':
			case '\r':
				if (pre) {	// Pre is ALWAYS In body
//				if (pre && mInBody) {
					
					bool	inOption = mInOption;
					if (!mInTextArea && mTextCount && mText[mTextCount - 1] != ' '
						|| mTextCount == 0 && mLastCharNotSpace)
						AppendText(' ');
					
					if (!mInTextArea)
						DispatchText();

					if (mInTextArea)
						AppendText(c);
						
					else if (!inOption) {
						if (c == '\r' || mLastChar != '\r')
							mBuilder->AddTag(T_BR, NULL);
					} else
						mInBody = false;
				} else {
					if (mInTag)
						mTag.Add(c);
				}
				break;
				
			// Characters between 128 and 156 have no equivalents in ISO-8859-1 and will be converted to
			// dead characters by the UTF-8 conversion routines.  Catch some of the common ones and
			// sanitize them.
			
//			Most chars wind up here

DEFAULT:	// this is really evil
			default:
				if (mInTag) {
					if (mTag.Count() == 0 && c == '!' && mInScript) {
						AppendText("//<!", 4);
						mTag.Finish();
						mInTag = false;
						break;
					}
					// Handle start of comment.
					if (mTag.Count() == 0 && c == '!' && !mInTextArea && !mInStyle) {
						mCommentState = kStandardComment;
						if ((dEnd - d) > 1) {
							if ((d[0] == '-') && (d[1] == '-')) {
								mCommentState = kNetscapeComment;
								mNetscapeCommentText = "";
							}
						}
						//mCommentBeginCount = 0;
						//mCommentEndCount = 0;
						mInTag = false;
						break;
					}
										
					// Respect quotes in tags following '='.
#if 1
					switch (c) {
						case '\'':	
							if (!mInTagDoubleQuote) {
								if (mInTagSingleQuote)
									mInTagSingleQuote = false;
								else {
									long tagIndex = mTag.Count() - 1;
									while (tagIndex >= 0 && isspace(mTag.Char(tagIndex)))
										tagIndex--;
									if (tagIndex >= 0 && mTag.Char(tagIndex) == '=')
										mInTagSingleQuote = true;
								}
							}
							break;
						case '"':
							if (!mInTagSingleQuote) {
								if (mInTagDoubleQuote)
									mInTagDoubleQuote = false;
								else {
									long tagIndex = mTag.Count() - 1;
									while (tagIndex >= 0 && isspace(mTag.Char(tagIndex)))
										tagIndex--;
									if (tagIndex >= 0 && mTag.Char(tagIndex) == '=')
										mInTagDoubleQuote = true;
								}
							}
							break;
						default:		
							break;
					}
#endif
//if (mInScript && mInTag)
//printf("In script and tag.  %s (%d)\n", mTag.Chars(), mTag.Count());
					mTag.Add(c);
					if (mInScript && mInTag && strncasecmp(mTag.Chars(), "/SCRIPT", mTag.Count()) != 0) {
						mInTag = false;
						DispatchText();
						AppendText('<');
						AppendText(mTag.Chars(), mTag.Count());
						mTag.Finish();
					}
					
				} else if (!pre && !mInBody && charIsSpace) {
					// strip more spaces
				} else  {
					AppendText(c);
					mInBody = true;
				}
				break;
		}
	}

	if (charRead >= 0)
		mLastChar = charRead;
	
	return d - (uchar*)data;
}



//	Complete will finish of the last tag/text

bool Parser::Complete()
{
	bool result = Consumer::Complete();
	if (result) {
		if (mCommentState == kNetscapeComment) {
			while (mNetscapeCommentText.Length() > 0) {
				char *temp = strchr(mNetscapeCommentText.String(), '>') + 1;
				BString theText;
				if (temp > (char *)1)
					theText = temp;				

				mCommentState = kNoComment;
				mNetscapeCommentText = "";

				if (theText.Length() > 0) 
					Write((uchar *)theText.String(), theText.Length());	
			}
		}

		if (mInTag) {
			mInTag = false;
			DispatchTag();
		} else
			DispatchText();
 	}
 	
	return result;
}


inline uint32
AutodetectJConversion(
	const char	*text,
	int32		length)
{
	uint32 conversion = N_AUTOJ_CONVERSION;

	for ( int32 i = 0; 
		  (i < length) && (conversion == N_AUTOJ_CONVERSION); 
		  i++) {
		uchar c = text[i];

		if (c == 0x1B) {
			if ((i + 1) < length) {
				uchar c2 = text[i + 1];

				if ((c2 == '$') || (c2 == '(')) {
					conversion = B_JIS_CONVERSION;
					break;
				}
			}
		}
		else {
			if (((c >= 129) && (c <= 141)) || ((c >= 143) && (c <= 159))) {
				conversion = B_SJIS_CONVERSION;
				break;
			}
			else {
				if (c == 142) {
					if ((i + 1) < length) {
						uchar c2 = text[i + 1];
						
						if ( ((c2 >= 64) && (c2 <= 126)) || 
							 ((c2 >= 128) && (c2 <= 160)) || 
							 ((c2 >= 224) && (c2 <= 252)) ) {
							conversion = B_SJIS_CONVERSION;
							break;
						} 
					}
				}
				else {
					if ((c >= 161) && (c <= 223)) {
						if ((i + 1) < length) {
							uchar c2 = text[i + 1];
		
							if ((c2 >= 240) && (c2 <= 254)) {
								conversion = B_EUC_CONVERSION;
								break;
							}
						}
					}
					else {
						if ((c >= 240) && (c <= 254)) {
							conversion = B_EUC_CONVERSION;	
							break;
						}
						else {
							if ((c >= 224) && (c <= 239)) {
								if ((i + 1) < length) {
									uchar c2 = text[i + 1];
								
									if ( ((c2 >= 64) && (c2 <= 126)) || 
										 ((c2 >= 128) && (c2 <= 160)) ) {
										conversion = B_SJIS_CONVERSION;
										break;
									}
									else {
										if ((c2 >= 253) && (c2 <= 254)) {
											conversion = B_EUC_CONVERSION;
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return (conversion);
}

void
Parser::ConvertTextToUTF8(
	const char	*text,
	short		length,
	BString		&convertedText)
{
	if (mEncoding == N_NO_CONVERSION) {
		convertedText.SetTo(text, length);
		return;
	}
		
	if (mEncoding == N_AUTOJ_CONVERSION) {
		mEncoding = AutodetectJConversion(text, length);
		if ((mEncoding != N_AUTOJ_CONVERSION) && (mBuilder != NULL))
			mBuilder->SetEncoding(mEncoding);
	}

	uint32 enc = (mEncoding == N_AUTOJ_CONVERSION) ? B_SJIS_CONVERSION : mEncoding;
	X2UTF8(enc, text, length, convertedText);
}

void
Parser::ConvertTextFromEncoding(
	BString &result,
	const char *text,
	int32 length)
{
	BString inText;
	int32 amperIndex = 0;
	int32 oldAmperIndex = 0;
	int32 unicodeChar = 0;
	bool cantBe = false;
	bool atEnd = false;

	result = "";
	inText.SetTo(text,length);
	
	while(!atEnd){
			
		amperIndex = inText.FindFirst("&",amperIndex);
		if(amperIndex < 0){
			if(inText.Length() >= oldAmperIndex) {
				BString convText;
				ConvertTextToUTF8(inText.String() + oldAmperIndex, length - oldAmperIndex, convText);
				result.Append(convText.String());
			}
			break;
		}
		if(amperIndex - oldAmperIndex > 0 || amperIndex >= inText.Length()){
			BString convText;
			ConvertTextToUTF8(inText.String() + oldAmperIndex, amperIndex - oldAmperIndex, convText);
			result.Append(convText.String());
		}
		oldAmperIndex = amperIndex + 1;
	
		BString amperStr;
		amperIndex++;
		cantBe = false;
		char c = 'T';
		while(true){ //figure out whether to write the intext to the result or the unicode character, then do it

			if(inText.Length() > amperIndex)
				c = *(inText.String() + amperIndex++);
			else
				atEnd = true;
																
			//if we get a ; or run out of inText, then we sink or swim the encoding
			if(atEnd || c == ';'){
				if(unicodeChar != 0){ //write the unicode character to result, and move on
					char text[3] = { (unicodeChar & 0xff00) >> 8, unicodeChar & 0xff, 0 };
					BString convText;
					X2UTF8(B_UNICODE_CONVERSION, text, 2, convText);
					result += convText;
					if(atEnd == true){
						oldAmperIndex = amperIndex;
						return;
					}
					else {
						oldAmperIndex = amperIndex;
					}
					break;
				}
				else{ //couldn't find a unicode character, so write the inText to result
					result << "&" << amperStr;
					break;
				}
			}

			amperStr += c;
			unicodeChar = AmpersandCharacter(amperStr.String(),&cantBe, mEncoding);
	
			if(amperStr.Length() > 10){ //couldn't find a unicode character within 10 chars, so write the inText to result
				result<< "&" << amperStr;
				break;
			}
			
			if (!cantBe)
				continue;
			
			//give it one last try with the amprStr less one char
			//amperStr.Truncate(amperStr.Length()-1, false);
			BString amperStrTwo = amperStr;
			amperStrTwo.Truncate(amperStr.Length()-1, false);
			unicodeChar = AmpersandCharacter(amperStrTwo.String(),&cantBe, mEncoding);
			
			if(unicodeChar != 0 && !cantBe){
				char text[3] = {(unicodeChar & 0xff00) >> 8, unicodeChar & 0xff, 0};
				BString convText;
				X2UTF8(B_UNICODE_CONVERSION, text, 2, convText);
				result += convText;
				if(amperIndex < inText.Length()){
					oldAmperIndex = amperIndex - 1;
					amperIndex--;
				}
				else{
					result.Append(amperStrTwo.String() + amperStrTwo.Length() - 1);
					oldAmperIndex = amperIndex;
					atEnd = true;
				}
				break;
			}
			else{
				result << "&" << amperStr;
				oldAmperIndex = amperIndex;
				break;
			}
		}			
	}
}
