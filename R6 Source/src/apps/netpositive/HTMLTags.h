//  ===========================================================================
//  HTMLTags.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1997 by Peter Barrett, All rights reserved.
//  ===========================================================================

#ifndef __HTMLTAGS__
#define __HTMLTAGS__

#include <SupportDefs.h>
#include <String.h>

// ===========================================================================
// Genertated by HTMLTAGS.C

int LookupTag(BString str);
int LookupAttribute(BString str);
int LookupAttributeValue(BString str);
int HasNumericValue(short attributeID);
const char* TagName(int tagID);
const char* AttributeName(int attributeID);
const char* AttributeValueName(int attributeValueID);

// ===========================================================================
#define kTagsCount 90
enum T_Tags {
	T_BAD=0,
	T_A,
	T_ADDRESS,
	T_APPLET,
	T_AREA,
	T_B,
	T_BASE,
	T_BASEFONT,
	T_BGSOUND,
	T_BIG,
	T_BLINK,
	T_BLOCKQUOTE,
	T_BODY,
	T_BR,
	T_CAPTION,
	T_CENTER,
	T_CITE,
	T_CODE,
	T_COMMENT,
	T_DD,
	T_DFN,
	T_DIR,
	T_DIV,
	T_DL,
	T_DT,
	T_EM,
	T_EMBED,
	T_FN,
	T_FONT,
	T_FORM,
	T_FRAME,
	T_FRAMESET,
	T_H1,
	T_H2,
	T_H3,
	T_H4,
	T_H5,
	T_H6,
	T_HEAD,
	T_HR,
	T_HTML,
	T_I,
	T_ILAYER,
	T_IMAGE,
	T_IMG,
	T_INPUT,
	T_ISINDEX,
	T_KBD,
	T_L,
	T_LAYER,
	T_LI,
	T_LINK,
	T_LISTING,
	T_MAP,
	T_MARQUEE,
	T_MENU,
	T_META,
	T_NETPOSITIVE,
	T_NOBR,
	T_NOEMBED,
	T_NOFRAMES,
	T_NOLAYER,
	T_NOSCRIPT,
	T_OBJECT,
	T_OL,
	T_OPTGROUP,
	T_OPTION,
	T_P,
	T_PARAM,
	T_PRE,
	T_S,
	T_SAMP,
	T_SCRIPT,
	T_SELECT,
	T_SMALL,
	T_STRONG,
	T_STYLE,
	T_SUB,
	T_SUP,
	T_TABLE,
	T_TD,
	T_TEXTAREA,
	T_TH,
	T_TITLE,
	T_TR,
	T_TT,
	T_U,
	T_UL,
	T_UNKNOWN,
	T_VAR,
	T_XMP
};

// ===========================================================================
#define kAttributesCount 113
enum A_Attributes {
	A_BAD=0,
	A_ABOVE,
	A_ACTION,
	A_ALIGN,
	A_ALINK,
	A_ALT,
	A_BACKGROUND,
	A_BELOW,
	A_BGCOLOR,
	A_BORDER,
	A_CELLBORDER,
	A_CELLPADDING,
	A_CELLSPACING,
	A_CHECKED,
	A_CLASS,
	A_CLASSID,
	A_CLEAR,
	A_CLIP,
	A_CODE,
	A_CODEBASE,
	A_CODETYPE,
	A_COLOR,
	A_COLS,
	A_COLSPAN,
	A_CONTENT,
	A_COORDS,
	A_DATA,
	A_DECLARE,
	A_DOWNLOAD,
	A_ENCTYPE,
	A_FACE,
	A_FRAMESPACING,
	A_HEIGHT,
	A_HIDDEN,
	A_HREF,
	A_HSPACE,
	A_HTTP_EQUIV,
	A_ID,
	A_ISMAP,
	A_LABEL,
	A_LEFT,
	A_LEFTMARGIN,
	A_LINK,
	A_LOOP,
	A_MARGINHEIGHT,
	A_MARGINWIDTH,
	A_MAXLENGTH,
	A_METHOD,
	A_METHODS,
	A_MULTIPLE,
	A_NAME,
	A_NOHREF,
	A_NORESIZE,
	A_NOSHADE,
	A_NOTAB,
	A_NOWRAP,
	A_ONABORT,
	A_ONBLUR,
	A_ONCHANGE,
	A_ONCLICK,
	A_ONDRAGDROP,
	A_ONERROR,
	A_ONFOCUS,
	A_ONKEYDOWN,
	A_ONKEYPRESS,
	A_ONKEYUP,
	A_ONLOAD,
	A_ONMOUSEDOWN,
	A_ONMOUSEMOVE,
	A_ONMOUSEOUT,
	A_ONMOUSEOVER,
	A_ONMOUSEUP,
	A_ONMOVE,
	A_ONRESET,
	A_ONRESIZE,
	A_ONSELECT,
	A_ONSUBMIT,
	A_ONUNLOAD,
	A_PAGEX,
	A_PAGEY,
	A_PALETTE,
	A_REL,
	A_REV,
	A_ROWS,
	A_ROWSPAN,
	A_SCROLLING,
	A_SELECTED,
	A_SHAPE,
	A_SHAPES,
	A_SHOWTITLEINTOOLBAR,
	A_SHOWTOOLBAR,
	A_SIZE,
	A_SRC,
	A_STANDBY,
	A_START,
	A_TABINDEX,
	A_TARGET,
	A_TEXT,
	A_THING,
	A_TITLE,
	A_TOP,
	A_TOPMARGIN,
	A_TYPE,
	A_UNITS,
	A_URN,
	A_USEMAP,
	A_VALIGN,
	A_VALUE,
	A_VALUETYPE,
	A_VISIBILITY,
	A_VLINK,
	A_VSPACE,
	A_WIDTH,
	A_WRAP,
	A_ZINDEX
};

// ===========================================================================
#define kAttributeValuesCount 44
enum AV_AttributeValues {
	AV_BAD=0,
	AV_ABSBOTTOM,
	AV_ABSMIDDLE,
	AV_ALL,
	AV_AUTO,
	AV_BASELINE,
	AV_BOTH,
	AV_BOTTOM,
	AV_BUTTON,
	AV_CENTER,
	AV_CHECKBOX,
	AV_CIRCLE,
	AV_DATA,
	AV_DEFAULT,
	AV_GET,
	AV_HARD,
	AV_HIDDEN,
	AV_HIDE,
	AV_IMAGE,
	AV_INHERIT,
	AV_INPUT,
	AV_LEFT,
	AV_MIDDLE,
	AV_NO,
	AV_NUMBER,
	AV_OBJECT,
	AV_OFF,
	AV_PASSWORD,
	AV_PHYSICAL,
	AV_POLY,
	AV_POLYGON,
	AV_POST,
	AV_RADIO,
	AV_RECT,
	AV_REF,
	AV_RESET,
	AV_RIGHT,
	AV_SHOW,
	AV_SOFT,
	AV_SUBMIT,
	AV_TEXT,
	AV_TEXTTOP,
	AV_TOP,
	AV_VIRTUAL,
	AV_YES
};
#endif

