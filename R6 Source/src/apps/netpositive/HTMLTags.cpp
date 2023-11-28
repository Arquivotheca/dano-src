// ===========================================================================
//	HTMLTags.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "HTMLTags.h"

#include <SupportDefs.h>
#include <string.h>
#include <stdlib.h>

// ===========================================================================
//	Presorted, generated tags used by app

const char* gTags[] = {
	"BAD",
	"A",
	"ADDRESS",
	"APPLET",
	"AREA",
	"B",
	"BASE",
	"BASEFONT",
	"BGSOUND",
	"BIG",
	"BLINK",
	"BLOCKQUOTE",
	"BODY",
	"BR",
	"CAPTION",
	"CENTER",
	"CITE",
	"CODE",
	"COMMENT",
	"DD",
	"DFN",
	"DIR",
	"DIV",
	"DL",
	"DT",
	"EM",
	"EMBED",
	"FN",
	"FONT",
	"FORM",
	"FRAME",
	"FRAMESET",
	"H1",
	"H2",
	"H3",
	"H4",
	"H5",
	"H6",
	"HEAD",
	"HR",
	"HTML",
	"I",
	"ILAYER",
	"IMAGE",
	"IMG",
	"INPUT",
	"ISINDEX",
	"KBD",
	"L",
	"LAYER",
	"LI",
	"LINK",
	"LISTING",
	"MAP",
	"MARQUEE",
	"MENU",
	"META",
	"NETPOSITIVE",
	"NOBR",
	"NOEMBED",
	"NOFRAMES",
	"NOLAYER",
	"NOSCRIPT",
	"OBJECT",
	"OL",
	"OPTGROUP",
	"OPTION",
	"P",
	"PARAM",
	"PRE",
	"S",
	"SAMP",
	"SCRIPT",
	"SELECT",
	"SMALL",
	"STRONG",
	"STYLE",
	"SUB",
	"SUP",
	"TABLE",
	"TD",
	"TEXTAREA",
	"TH",
	"TITLE",
	"TR",
	"TT",
	"U",
	"UL",
	"UNKNOWN",
	"VAR",
	"XMP",
	NULL
};

// ===========================================================================
const char* gAttributes[] = {
	"BAD",
	"ABOVE",
	"ACTION",
	"ALIGN",
	"ALINK",
	"ALT",
	"BACKGROUND",
	"BELOW",
	"BGCOLOR",
	"BORDER",
	"CELLBORDER",
	"CELLPADDING",
	"CELLSPACING",
	"CHECKED",
	"CLASS",
	"CLASSID",
	"CLEAR",
	"CLIP",
	"CODE",
	"CODEBASE",
	"CODETYPE",
	"COLOR",
	"COLS",
	"COLSPAN",
	"CONTENT",
	"COORDS",
	"DATA",
	"DECLARE",
	"DOWNLOAD",
	"ENCTYPE",
	"FACE",
	"FRAMESPACING",
	"HEIGHT",
	"HIDDEN",
	"HREF",
	"HSPACE",
	"HTTP-EQUIV",
	"ID",
	"ISMAP",
	"LABEL",
	"LEFT",
	"LEFTMARGIN",
	"LINK",
	"LOOP",
	"MARGINHEIGHT",
	"MARGINWIDTH",
	"MAXLENGTH",
	"METHOD",
	"METHODS",
	"MULTIPLE",
	"NAME",
	"NOHREF",
	"NORESIZE",
	"NOSHADE",
	"NOTAB",
	"NOWRAP",
	"ONABORT",
	"ONBLUR",
	"ONCHANGE",
	"ONCLICK",
	"ONDRAGDROP",
	"ONERROR",
	"ONFOCUS",
	"ONKEYDOWN",
	"ONKEYPRESS",
	"ONKEYUP",
	"ONLOAD",
	"ONMOUSEDOWN",
	"ONMOUSEMOVE",
	"ONMOUSEOUT",
	"ONMOUSEOVER",
	"ONMOUSEUP",
	"ONMOVE",
	"ONRESET",
	"ONRESIZE",
	"ONSELECT",
	"ONSUBMIT",
	"ONUNLOAD",
	"PAGEX",
	"PAGEY",
	"PALETTE",
	"REL",
	"REV",
	"ROWS",
	"ROWSPAN",
	"SCROLLING",
	"SELECTED",
	"SHAPE",
	"SHAPES",
	"SHOWTITLEINTOOLBAR",
	"SHOWTOOLBAR",
	"SIZE",
	"SRC",
	"STANDBY",
	"START",
	"TABINDEX",
	"TARGET",
	"TEXT",
	"THING",
	"TITLE",
	"TOP",
	"TOPMARGIN",
	"TYPE",
	"UNITS",
	"URN",
	"USEMAP",
	"VALIGN",
	"VALUE",
	"VALUETYPE",
	"VISIBILITY",
	"VLINK",
	"VSPACE",
	"WIDTH",
	"WRAP",
	"Z-INDEX",
	NULL
};

// ===========================================================================
const uchar gHasNumericValue[] = {
	0,	// BAD
	0,	// ABOVE
	0,	// ACTION
	0,	// ALIGN
	1,	// ALINK
	0,	// ALT
	0,	// BACKGROUND
	0,	// BELOW
	1,	// BGCOLOR
	1,	// BORDER
	1,	// CELLBORDER
	1,	// CELLPADDING
	1,	// CELLSPACING
	1,	// CHECKED
	0,	// CLASS
	0,	// CLASSID
	0,	// CLEAR
	0,	// CLIP
	0,	// CODE
	0,	// CODEBASE
	0,	// CODETYPE
	1,	// COLOR
	1,	// COLS
	1,	// COLSPAN
	0,	// CONTENT
	0,	// COORDS
	0,	// DATA
	0,	// DECLARE
	1,	// DOWNLOAD
	0,	// ENCTYPE
	0,	// FACE
	1,	// FRAMESPACING
	1,	// HEIGHT
	0,	// HIDDEN
	0,	// HREF
	1,	// HSPACE
	0,	// HTTP-EQUIV
	0,	// ID
	1,	// ISMAP
	0,	// LABEL
	1,	// LEFT
	1,	// LEFTMARGIN
	1,	// LINK
	1,	// LOOP
	1,	// MARGINHEIGHT
	1,	// MARGINWIDTH
	1,	// MAXLENGTH
	0,	// METHOD
	0,	// METHODS
	1,	// MULTIPLE
	0,	// NAME
	0,	// NOHREF
	0,	// NORESIZE
	1,	// NOSHADE
	0,	// NOTAB
	1,	// NOWRAP
	0,	// ONABORT
	0,	// ONBLUR
	0,	// ONCHANGE
	0,	// ONCLICK
	0,	// ONDRAGDROP
	0,	// ONERROR
	0,	// ONFOCUS
	0,	// ONKEYDOWN
	0,	// ONKEYPRESS
	0,	// ONKEYUP
	0,	// ONLOAD
	0,	// ONMOUSEDOWN
	0,	// ONMOUSEMOVE
	0,	// ONMOUSEOUT
	0,	// ONMOUSEOVER
	0,	// ONMOUSEUP
	0,	// ONMOVE
	0,	// ONRESET
	0,	// ONRESIZE
	0,	// ONSELECT
	0,	// ONSUBMIT
	0,	// ONUNLOAD
	1,	// PAGEX
	1,	// PAGEY
	0,	// PALETTE
	0,	// REL
	0,	// REV
	1,	// ROWS
	1,	// ROWSPAN
	0,	// SCROLLING
	1,	// SELECTED
	0,	// SHAPE
	0,	// SHAPES
	0,	// SHOWTITLEINTOOLBAR
	0,	// SHOWTOOLBAR
	1,	// SIZE
	0,	// SRC
	0,	// STANDBY
	1,	// START
	1,	// TABINDEX
	0,	// TARGET
	1,	// TEXT
	0,	// THING
	0,	// TITLE
	1,	// TOP
	1,	// TOPMARGIN
	0,	// TYPE
	0,	// UNITS
	0,	// URN
	0,	// USEMAP
	0,	// VALIGN
	0,	// VALUE
	1,	// VALUETYPE
	0,	// VISIBILITY
	1,	// VLINK
	1,	// VSPACE
	1,	// WIDTH
	1,	// WRAP
	1	// Z-INDEX
};

// ===========================================================================
const char* gAttributeValues[] = {
	"BAD",
	"ABSBOTTOM",
	"ABSMIDDLE",
	"ALL",
	"AUTO", 
	"BASELINE",
	"BOTH",
	"BOTTOM",
	"BUTTON",
	"CENTER",
	"CHECKBOX",
	"CIRCLE",
	"DATA",
	"DEFAULT",
	"GET",
	"HARD",
	"HIDDEN",
	"HIDE",
	"IMAGE",
	"INHERIT",
	"INPUT",
	"LEFT",
	"MIDDLE",
	"NO",
	"NUMBER",
	"OBJECT",
	"OFF",
	"PASSWORD",
	"PHYSICAL",
	"POLY",
	"POLYGON",
	"POST",
	"RADIO",
	"RECT",
	"REF",
	"RESET",
	"RIGHT",
	"SHOW",
	"SOFT",
	"SUBMIT",
	"TEXT",
	"TEXTTOP",
	"TOP",
	"VIRTUAL",
	"YES",
	NULL
};

// ===========================================================================
// ===========================================================================

int compStr(const void* a, const void* b)
{
	char *aa = 	*((char**)a);
	char *bb = 	*((char**)b);
	return strcmp(aa,bb);


//	return strcmp(*((char**)a),*((char**)b));
}

//	Skip "BAD" token at location 0

int SearchToken(const char** tokens, int count, const BString key)
{
	const char *keystr = key.String();
	const char** c = (const char**)bsearch(&keystr,tokens + 1,count,4,compStr);
	if (c == NULL)
		return 0;
	return (c - tokens);
}

// ===========================================================================

int LookupTag(BString str)
{
	if (str == NULL)
		return 0;

	str.ToUpper();
	return SearchToken(gTags,kTagsCount,str);
}

int LookupAttribute(BString str)
{
	if (str == NULL)
		return 0;

	str.ToUpper();
	return SearchToken(gAttributes,kAttributesCount,str);
}

int LookupAttributeValue(BString str)
{
	if (str == NULL)
		return 0;

	str.ToUpper();
	return SearchToken(gAttributeValues,kAttributeValuesCount,str);
}

int HasNumericValue(short attributeID)
{
	return gHasNumericValue[attributeID];
}

const char* TagName(int tagID)
{
	if (tagID < 0)
		tagID = -tagID;
	if (tagID > kTagsCount)
		return "BAD TAG (Out of range)";
	return gTags[tagID];
}

const char* AttributeName(int attributeID)
{
	if (attributeID < 0 || attributeID > kAttributesCount)
		return "BAD ATTRIBUTE (Out of range)";
	return gAttributes[attributeID];
}

const char* AttributeValueName(int attributeValueID)
{
	if (attributeValueID < 0 || attributeValueID > kAttributeValuesCount)
		return "BAD ATTRIBUTE (Out of range)";
	return gAttributeValues[attributeValueID];
}
