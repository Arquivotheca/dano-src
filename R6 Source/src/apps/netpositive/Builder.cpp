// ===========================================================================
//	Builder.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Builder.h"
#include "HTMLTags.h"
#include "HTMLDoc.h"
#include "Form.h"
#include "AnchorGlyph.h"
#include "ObjectGlyph.h"
#include "ImageGlyph.h"
#include "TableGlyph.h"
#include "InputGlyph.h"
#include "TextGlyph.h"
#include "JavaGlyph.h"
#include "BeInput.h"
#include "NPApp.h"
#include "HTMLView.h"
#ifdef ADFILTER
#include "AdFilter.h"
#endif
#include "UResource.h"
#include "BeDrawPort.h"
#include "FontSubstitution.h"
#include "MessageWindow.h"

#include <UTF8.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <Autolock.h>

typedef short AttributeValue;

extern const char *kSearchableIndexText;
extern const char *kSearchableKeywordsText;

// ============================================================================

typedef struct {
	short	tagID;
	Style	style;
	bool	hasMargin;
	short	count;
	bool	permanent;
	bool marginOnly;
} StyleList;

static StyleList const gStyleList[] = {
//   Tag			   Siz,Mno,Bld,Itc,Uln,Str,Sub,Sup,Blk	Mrg,Move,Perm,MOnly
	{ 0, 			{	0, 	0, 	0, 	0, 	0,  0,  0,  0,  0},  0,  0,   0,   0	}, 	// Default text
	
	{ T_TT,			{	0, 	1, 	0, 	0, 	0,  0,  0,  0,  0},  0,  0,   0,   0	}, 	// Typographic
	{ T_B,			{	0, 	0, 	1, 	0, 	0,  0,  0,  0,  0},  0,  0,   0,   0	},
	{ T_I,			{	0, 	0, 	0, 	1, 	0,  0,  0,  0,  0},  0,  0,   0,   0	}, 
	{ T_U,			{	0, 	0, 	0, 	0, 	1,  0,  0,  0,  0},  0,  0,   0,   0	}, 
	{ T_S,			{	0, 	0, 	0, 	0, 	0,  1,  0,  0,  0},  0,  0,   0,   0	}, 
	{ T_SUB,		{	0, 	0, 	0, 	0, 	0,  0,  1,  0,  0},  0,  0,   0,   0	}, 
	{ T_SUP,		{	0, 	0, 	0, 	0, 	0,  0,  0,  1,  0},  0,  0,   0,   0	},
	{ T_BLINK,		{	0, 	0, 	0, 	0, 	0,  0,  0,  0,  1},  0,  0,   0,   0	},
	
	{ T_H1,			{	6, 	0, 	1, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Headings
	{ T_H2,			{	5, 	0, 	1, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_H3,			{	4, 	0, 	1, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_H4,			{	3, 	0, 	1, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_H5,			{	2, 	0, 	1, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_H6,			{	1, 	0, 	1, 	0, 	0	}, 			 0,  0,   0,   0	}, 

	{ T_DL,			{	0, 	0, 	0, 	0, 	0	},			 1,  0,   1,   0	}, 	// Glossary list
	{ T_OL,			{	0, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Ordered list
	{ T_UL,			{	0, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Unordered list
	{ T_L,			{	0, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Unordered list
	{ T_DIR,		{	0, 	0, 	1, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Directory list
	{ T_MENU,		{	0, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   0	}, 	// Menu list

	{ T_DT,			{	0, 	0, 	0, 	0, 	0	},		 	 1,  0,   0,   1	}, 	// Not permanent, margin only
	{ T_DD,			{	0, 	0, 	0, 	0, 	0	}, 			 1,  +1,  0,   1	}, 	// See DocumentBuidler::MoveMargin
	{ T_LI,			{	0, 	0, 	0, 	0, 	0	},		 	 0,  0,   0,   1	}, 

	{ T_PRE,		{	0, 	1, 	0, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Preformatted
	{ T_XMP,		{	0, 	1, 	0, 	0, 	0	},		 	 0,  0,   0,   0	}, 
	{ T_LISTING,	{	2, 	1, 	0, 	0, 	0	}, 			 0,  0,   0,   0	},
	{ T_ADDRESS,	{	0, 	0, 	0, 	1, 	0	}, 			 1, +1,   1,   0	}, 
	{ T_BLOCKQUOTE,	{	0, 	0, 	0, 	0, 	0	}, 			 1, +1,   1,   1	}, 
	
	{ T_CODE,		{	0, 	1, 	0, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Code
	{ T_KBD,		{	0, 	1, 	0, 	0, 	0	},		 	 0,  0,   0,   0	}, 	// Keyboard
	{ T_SAMP,		{	0, 	1, 	0, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Sample

	{ T_EM,			{	0, 	0, 	0, 	1, 	0	}, 			 0,  0,   0,   0	}, 	// Emphasis
	{ T_CITE,		{	0, 	0, 	0, 	1, 	0	}, 			 0,  0,   0,   0	}, 	// Citation
	{ T_VAR,		{	0, 	0, 	0, 	1, 	0	}, 			 0,  0,   0,   0	}, 	// Variable
	
	{ T_STRONG,		{	0, 	0, 	1, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Strong
	{ T_DFN,		{	0, 	0, 	0, 	0, 	0	}, 			 0,  0,   0,   0	}, 	// Definition?

	{ -1, {0, 0, 0, 0, 0} , 0, 0, 0, 0 }
};

// ============================================================================
// Tags are linkables containing an attribute ID and a value

Tag::Tag()
{
	mNumericValue = 1;
	mIsPercentage = 0;
	mAttributeID = 0;
	mValue = 0;
	mOrigAttribute = 0;
	mOrigValue = 0;
}

Tag::~Tag()
{
	if (!mNumericValue && mValue)		// Delete value string
		free((void *)mValue);
	if (mOrigAttribute)
		free((void *)mOrigAttribute);
	if (mOrigValue)
		free((void *)mOrigValue);
}

void Tag::SetAttributeStr(long attributeID, const char *value)
{
	mNumericValue = 0;
	mAttributeID = attributeID;
	if (value)
		mValue = (long)SetStr((char*)mValue,value);	// Only copy string if it is nonzero
	else
		mValue = (long)SetStr((char*)mValue,0);
}

void Tag::SetAttribute(long attributeID, long value, bool isPercentage)
{
	mNumericValue = 1;
	mIsPercentage = isPercentage;
	mAttributeID = attributeID;
	mValue = value;
}

void Tag::SetOrigAttribute(const char *attrName, const char *attrValue)
{
	mOrigAttribute = SetStr(mOrigAttribute, attrName);
	mOrigValue = SetStr(mOrigValue, attrValue);
}

Tag* Tag::Find(long attributeID)
{
	Tag* tag;
	for (tag = (Tag *)First(); tag; tag = (Tag *)tag->Next())
		if (attributeID == tag->mAttributeID)
			return tag;
	return NULL;				
}

InputValueItem::InputValueItem(
	const char	*value,
	bool		checked,
	CLinkedList	*option)
{
	mValue = value;
	mChecked = checked;
	mOption = option;
}

InputValueItem::~InputValueItem()
{
	delete (mOption);
}

// ============================================================================
// PageBuilder

DocumentBuilder::DocumentBuilder(
	CLinkedList	*list,
	BList *pluginData)
{
	mEncoding = B_MS_WINDOWS_CONVERSION;
	mForceCache = false;

	mDocument = 0;
	mGlyph = 0;
	mTable = 0;
	mForm = 0;
	mCurrentAnchor = 0;
#ifdef ADFILTER
	mFilterList = 0;
#endif
	mLayer = 0;

	mListDepth = 0;
	mBaseFont = 3;
	mCurrentStyle = gDefaultStyle;
	mStyleStackDepth = 0;
	mImageMap = 0;
	
	mInTitle = false;				// Inside a <TITLE> tag
	mInScript = false;				// Inside a <SCRIPT> tag
	mInNoBreak = false;				// Inside <NOBR> tag
	
	mInParagraph = false;			// Inside <P> tag
	mInStyle = false;				// Inside <STYLE> tag
	mInTextArea = false;			// Inside <TEXTAREA> tag
	mPRECount = 0;					// Inside n <PRE> tags
	mHaveTempMargin = false;
		
	mGlyphCount = 0;
	mIECommentCount = 0;
	mNoFrame = false;
	mNoLayer = false;
	mNoEmbed = false;
		
	mCurrentAlign = AV_LEFT;
	mAlignStackDepth = 0;
	mDLCount = 0;
	mListDepth = 0;
	mLeftMarginDepth = 0;
	mRightMarginDepth = 0;

	mPendingLineBreak = Glyph::kNoBreak;		// Line break to attach to next glyph

	mInObject = false;				// Inside <OBJECT> tag
	mInEmbed = false;
	mObject = NULL;
	mObjectTag = NULL;
	mAddedBody = false;

	mUserValueHead = NULL;
	if ((list != NULL) && (list->First() != NULL))
		mUserValueHead = (InputValueItem *)list->First();
	mPluginData = pluginData;
}

DocumentBuilder::~DocumentBuilder()
{
	delete (mObjectTag);
#ifdef ADFILTER
	delete (mFilterList);
#endif
//	Finalize();
	if (mDocument) {
		mDocument->Lock();
		if (mDocument->Dereference() > 0)
			mDocument->Unlock();
	}
}

void
DocumentBuilder::SetEncoding(
	uint32	encoding)
{
	mEncoding = encoding;
	for (int i = 0; i < mFormList.CountItems(); i++)
		((Form *)mFormList.ItemAt(i))->SetEncoding(encoding);
}


void
DocumentBuilder::SetForceCache(
	bool	forceCache)
{
	mForceCache = forceCache;
}


void DocumentBuilder::SetDocument(Document *document, AdFilterList *filterList)
{
	document->Reference();
	mDocument = document;
	if (!mDocument->Lock()) return;
	mGlyph = mDocument->GetRootGlyph();
	mDocument->Unlock();
#ifdef ADFILTER
	mFilterList = filterList;
#endif
}

// ============================================================================
//	Main tag dispatcher

void DocumentBuilder::AddTag(short tagID, Tag **origTag)
{
//	if (fStatus != kPending)
//		return;

	if (mInScript && -tagID != T_SCRIPT)
		return;
		
	if (mInTitle && -tagID != T_TITLE)
		return;
		
	if (mNoFrame && -tagID != T_NOFRAMES)
		return;
		
	if (mNoLayer && -tagID != T_NOLAYER)
		return;

	if (mNoEmbed && -tagID != T_NOEMBED)
		return;
		
//	Only PARAM is valid inside OBJECT tag

	if (mInObject) {
		if (((-tagID == T_OBJECT) || (-tagID == T_APPLET) || (tagID == T_PARAM) ||
			 (tagID == T_EMBED) || (-tagID == T_EMBED)) == false)
			return;
	}
	
	if (tagID > 0)  {		
#ifdef ADFILTER
		// Allow FRAME tags to pass through.  They get filtered separately, because if the
		// tag is stripped, then we want to reclaim the view created in the document.
		if (mFilterList && tagID == T_FRAME || !AdFilter::ShouldFilterTag(mFilterList, tagID, origTag)) {
#endif
			Tag* tag = origTag ? *origTag : NULL;
			switch (tagID) {		// Start tags <TAG>
				case T_HTML:
				case T_HEAD:										break;
				
				case T_ISINDEX:		AddIndex(tag);					break;
				case T_LINK:		AddLink(tag);					break;
				case T_META:		AddMeta(tag);					break;
				case T_BASE:		AddBase(tag);					break;
							
				case T_TITLE:		mInTitle = true;				break;
				case T_BGSOUND:		AddBackgroundSound(tag);		break;
				case T_BODY:		AddBody(tag);					break;
				
#ifdef LAYERS
				case T_ILAYER:
				case T_LAYER:		OpenLayer(tagID==T_ILAYER, tag);break;
#endif
				
				case T_DIV:			OpenDivision(tag);				break;
				case T_CENTER:		OpenCenter(tag);				break;
				case T_P:			OpenParagraph(tag);				break;
				case T_BR:			AddNewLine(tag);				break;
				case T_NOBR:		/*AddNewLine(NULL);*/mInNoBreak = true;				break;
				
				case T_HR:			AddRule(tag);					break;
				
				case T_IMAGE:
				case T_IMG:			AddImage(tag);					break;
				case T_FN:	
				case T_A:			mCurrentStyle.fontColorFlag = 0;OpenAnchor(tag);	break;
				
				case T_FONT:		Font(tag);						break;
				case T_BASEFONT:	BaseFont(tag);					break;
				case T_BIG:			Big(tag);						break;
				case T_SMALL:		Small(tag);						break;
			
				case T_TABLE:		OpenTable(tag);					break;
				case T_TR:			OpenTableRow(tag);				break;
				case T_TH:			OpenTableCell(tag, true);		break;	// Heading
				case T_TD:			OpenTableCell(tag, false);		break;
				case T_CAPTION:		OpenCaption(tag);				break;
				
				case T_FORM:		OpenForm(tag);					break;
				case T_INPUT:		AddInput(tag);					break;
	
				case T_SELECT:		OpenSelect(tag);				break;
				case T_OPTION:		OpenOption(tag);				break;
				case T_OPTGROUP:	OpenOptGroup(tag);				break;
				case T_TEXTAREA:	OpenTextArea(tag);				break;
				
				case T_OBJECT:		OpenObject(tag);				break;
				case T_PARAM:		AddParam(tag);					break;
				case T_APPLET:		OpenApplet(tag);				break;
				case T_EMBED:		OpenEmbed(tag);					break;
	
				case T_MAP:			OpenImageMap(tag);				break;
				case T_AREA:		AddArea(tag);					break;
				
				case T_MARQUEE:		OpenMarquee(tag);				break;
				case T_COMMENT:		mIECommentCount++;				break;
				
				case T_SCRIPT:		OpenScript(tag);				break;
				case T_STYLE:		OpenStyle(tag);					break;
				
				case T_FRAMESET:	OpenFrameset(tag);				break;
				case T_FRAME:		AddFrame(tag);					break;
				case T_NOFRAMES:	mNoFrame = true;				break;
#ifdef LAYERS
				case T_NOLAYER:		if (gPreferences.FindBool("SupportLayerTag"))
										mNoLayer = true;			break;
#endif
				case T_NOEMBED:		mNoEmbed = true;				break;
				case T_NETPOSITIVE:	NetPositiveTag(tag);			break;
	
				default:
					if (!IsStyleTag(tagID,tag))
						pprintBig("Unhandled Tag: %s","trats");
			}
#ifdef ADFILTER
		}
#endif
	} else {
		switch (-tagID) {		// Stop tags </TAG>	
			case T_HTML:										break;
			case T_HEAD:		CloseAnchor();					break;
			
			case T_TITLE:		mInTitle = false;				break;
			case T_BODY:										break;
			
#ifdef LAYERS
			case T_ILAYER:
			case T_LAYER:		CloseLayer();					break;
#endif
			case T_DIV:			CloseDivision();				break;
			case T_CENTER:		CloseCenter();					break;
			case T_P:			CloseParagraph(true);			break;
//			case T_P:			CloseParagraph();				break;
			case T_BR:			AddNewLine(NULL);				break;
			case T_NOBR:		mInNoBreak = false;				break;

			case T_FN:
			case T_A:			CloseAnchor();					break;
			
			case T_FONT:
			case T_BIG:
			case T_SMALL:		PopStyle();						break;
	
			case T_TABLE:		CloseTable();					break;
			case T_TR:			CloseTableRow();				break;
			case T_TH:		
			case T_TD:			CloseTableCell();				break;
			case T_CAPTION:		CloseCaption();					break;
			
			case T_EMBED:		CloseEmbed();					break;
			case T_APPLET:		CloseApplet();					break;
			case T_OBJECT:		CloseObject();					break;

			case T_FORM:		CloseForm();					break;
			case T_SELECT:		CloseSelect();					break;
			case T_OPTGROUP:	CloseOptGroup();				break;
			case T_TEXTAREA:	CloseTextArea();				break;
			case T_MAP:			CloseImageMap();				break;
			
			case T_MARQUEE:		CloseMarquee();					break;
			case T_COMMENT:		if (mIECommentCount > 0)
									mIECommentCount--;			break;
			
			case T_SCRIPT:		CloseScript();					break;
			case T_STYLE:		CloseStyle();					break;
			
			case T_FRAMESET:	CloseFrameset();				break;
			case T_FRAME:
			case T_NOFRAMES:	mNoFrame = false;				break;
#ifdef LAYERS
			case T_NOLAYER:		mNoLayer = false;				break;
#endif
			case T_NOEMBED:		mNoEmbed = false;				break;
			default:
				if (!IsStyleTag(tagID,0))
					pprintBig("Unhandled Tag: %s","trats");
		}
	}
}

// ============================================================================
//	Main style dispatcher

// IsStyleTag determines if tag sets styles that define a block of text
// A T_H2 tag is a style tag because it overrides any current style
// a T_B tag isn't because its effects are local

bool DocumentBuilder::IsStyleTag(short tagID, Tag *tag)
{
	if (tagID > 0) {		
		switch (tagID) {			
			case T_H1:				// Headings
			case T_H2:
			case T_H3:
			case T_H4:
			case T_H5:
			case T_H6:
				OpenHeading(tag);
				NewStyle(tagID);
				return true;
			
			case T_LI:				// List Item
				AddBullet(tag);
				return true;
				
			case T_DL:				// Lists
				mDLCount++;
			case T_OL:
			case T_UL:
			case T_L:
			case T_DIR:
			case T_MENU:
				PushList(tagID, tag);
				NewStyle(tagID);
				return true;
				
			case T_PRE:
			case T_XMP:
			case T_LISTING:
				mPRECount++;
			case T_BLOCKQUOTE:
				BlankLine();
				NewStyle(tagID);
				return true;

			case T_S:
			case T_ADDRESS:
			case T_DT:				// Glossary List Term
			case T_DD:				// Glossary List Definition
			
			case T_TT:
			case T_B:
			case T_I:
			case T_U:
			case T_SUB:
			case T_SUP:
			case T_KBD:
			case T_SAMP:
			case T_CODE:
			case T_EM:
			case T_CITE:
			case T_VAR:
			case T_STRONG:
			case T_DFN:
			case T_BLINK:
				NewStyle(tagID);
				return true;
		}
	} else {		
		switch (-tagID) {			
			case T_H1:				// Headings
			case T_H2:
			case T_H3:
			case T_H4:
			case T_H5:
			case T_H6:
				NewStyle(tagID);
				CloseHeading();
				return true;
				
			case T_DL:				// Lists
				if (mDLCount > 0)
					mDLCount--;
			case T_OL:
			case T_UL:
			case T_L:
			case T_DIR:
			case T_MENU:
				PopList();			// Forget the kind of list
				NewStyle(tagID);
				return true;

			case T_PRE:
			case T_XMP:
			case T_LISTING:
				if (mPRECount > 0)
					mPRECount--;
			case T_BLOCKQUOTE:
				NewStyle(tagID);
				BlankLine();
				return true;

			case T_S:
			case T_DT:				// Glossary List Term
			case T_DD:				// Glossary List Definition
			case T_ADDRESS:

			case T_TT:
			case T_B:
			case T_I:
			case T_U:
			case T_SUB:
			case T_SUP:
			case T_KBD:
			case T_SAMP:
			case T_CODE:
			case T_EM:
			case T_CITE:
			case T_VAR:
			case T_STRONG:
			case T_DFN:
			case T_BLINK:
				NewStyle(tagID);
				return true;
		}
	}
	return false;
}	

void DocumentBuilder::NewStyle(short tagID)
{
	// Lookup a tag in the style list, move margins and set style
	
	short t = (tagID < 0) ? -tagID : tagID;
	short i;
	
	for (i = 0; gStyleList[i].tagID != -1; i++)
		if (gStyleList[i].tagID == t)
			break;
	if (gStyleList[i].tagID == -1)
		i = 0;
	
	if (!gStyleList[i].marginOnly)
		SetTagStyle(gStyleList[i].style, tagID > 0);	// Set or unset a style
		
	if (gStyleList[i].hasMargin) {
		short	count = gStyleList[i].count;
		
		// Special case DL. Only nested DLs indent. /DL always outdent
		if ((tagID == T_DL && mDLCount > 1) || (-tagID == T_DL))
			count++;
			
		if (tagID > 0)
			MoveMargin(count, gStyleList[i].permanent, tagID == T_BLOCKQUOTE);
		else
			MoveMargin(-count, gStyleList[i].permanent, tagID == -T_BLOCKQUOTE);
	}
}

// ============================================================================
//	Reposition margin according to a tag
//	DD tags have a count of zero and are handled differently:
//		If a DT is not very wide (< 32 pixels) dont start the DD on a new line
//		DDMarginGlyph checks the page and will not break if (mHPos - mLeftMargin) < 32;

void DocumentBuilder::MoveMargin(short count, bool permanent, bool rightToo)
{
	short depth = MAX(0,mLeftMarginDepth + count);
//	MarginGlyph *g = count ? new(MarginGlyph) : new(DDMarginGlyph);	// See above
	MarginGlyph *g  = new MarginGlyph(mDocument);	// See above
	g->SetLeftMargin(depth);
	if (rightToo) {
		short rightDepth = MAX(0, mRightMarginDepth + count);
		g->SetRightMargin(rightDepth);
		if (permanent)
			mRightMarginDepth = rightDepth;
	}
	AddGlyph(g);
	if (permanent)
		mLeftMarginDepth = depth;
	
	mHaveTempMargin = !permanent && count != 0;
}

// Determine the kind of oranment to be used at this list depth

void DocumentBuilder::PushList(short tagID, Tag* tag)
{
	short start = 1;
	short type = '1';		// Ordered lists like numbers
	
	if (tagID != T_OL) {	// Otherwise, default to dots
		switch (mListDepth % 3) {				
			case 0:	type = 'd';	break;
			case 1:	type = 'c';	break;
			case 2:	type = 's';	break;
		}
	}
		
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next()) {
			switch (tag->mAttributeID) 	{				
			case A_TYPE: {	
				const char *value = (const char*)tag->mValue;
				type = value ? *value : 0;
				break;	// Kind is A|a|I|i|1 for ordered lists, d|c|s for UL
			}
			case A_START:
				if(tag->mValue >= 0)
					start = tag->mValue;
				break;
		}
	}

	if (mListDepth == 0)
		BlankLine();	// Add a blank line before first list 

	if (mListDepth < 50) {		
		mListStack[++mListDepth] = type;
		mListIndex[mListDepth] = start;
	}
}

void DocumentBuilder::PopList()
{
	if (mListDepth > mGlyph->GetListStackDepth()) {
		mListDepth--;
		if (mListDepth == 0)
			BlankLine();
	}
}

void DocumentBuilder::AddBullet(Tag* tag)
{
	// Get The style of the current list (UL/OL etc ...)
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next()) {
			switch (tag->mAttributeID) 	{				
				case A_TYPE:
					if(tag->mValue != 0)
						mListStack[mListDepth] = *((char *)tag->mValue);	//	Kind is A|a|I|i|1 for ordered lists, d|c|s for UL
					break;
				case A_VALUE: {
					long value = mListIndex[mListDepth];
					if(tag->mValue != 0)
						sscanf((char *)tag->mValue,"%ld",&value);
					mListIndex[mListDepth] = value;
					break;
				}
				default:
					break;
			}
		}

	// Add a dot to a list item
	// Kind is A|a|I|i|1 for ordered lists, d|c|s for UL
	// Index is its ordinal
	
	if (mHaveTempMargin)
		MoveMargin(0, false, true);

	BulletGlyph* bullet = new BulletGlyph(mDocument);
	bullet->SetKind(mListStack[mListDepth], mListIndex[mListDepth]);
	bullet->SetStyle(mCurrentStyle);
	AddGlyph(bullet);

	mListIndex[mListDepth]++;
}

// ============================================================================
// HEAD tags

const char* DocumentBuilder::GetTitle()
{
	if (!mDocument->Lock()) return NULL;
	const char *val = mDocument->GetTitle();
	mDocument->Unlock();
	return val;
}

void DocumentBuilder::AddToTitle(const char *title)
{
	if (!mDocument->Lock()) return;
	BString currentTitle = mDocument->GetTitle();
	currentTitle += title;
	mDocument->SetTitle(currentTitle.String());
	mDocument->Unlock();
}

void DocumentBuilder::AddMeta(Tag *tag)
{
	if (tag == NULL)
		return;
		
	const Tag*	httpAttr = tag->Find(A_HTTP_EQUIV);
	const Tag*	contentAttr = tag->Find(A_CONTENT);
			
	if (httpAttr == NULL || httpAttr->mValue == 0)
		return;
		
	if (contentAttr && contentAttr->mValue) {
		BString equiv = (char*)httpAttr->mValue;
		equiv.ToLower();
		char* content = (char*)contentAttr->mValue;

		if (equiv == "refresh") {	
			long value;
			
			if (sscanf(content, "%ld", &value) == 1) {
				if (!mDocument->Lock()) return;
				mDocument->SetRefreshTime(MAX(value, 0));
				
				while (content[0] && content[0] != '=')
					content++;
				if (content[0])
					mDocument->SetRefreshURL(content+1);	// load this url
				mDocument->Unlock();
			}
		} else if (equiv == "pragma") {
			if(strcmp(content, "no-cache") == 0 &&
			   mDocument &&
			   mDocument->GetResource()) {
				pprint("Got HTTP-EQUIV \"PRAGMA\" CONTENT=\"NO-CACHE\", marking resource as expired");
				mDocument->GetResource()->SetExpires(time(NULL));
			}
		}
		
	}
}

//	Set the base of the document to something other than the default path

void DocumentBuilder::AddBase(Tag* tag)
{
	if (tag == NULL)
		return;

	if (!mDocument->Lock()) return;
	for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next()) {
		switch (tag->mAttributeID) {
			case A_HREF:
				pprint("Base: '%s'",(char*)tag->mValue);
				mDocument->SetBase((char*)tag->mValue);
				break;

			case A_TARGET:
				pprint("set base target to %s\n", (char *)tag->mValue);
				mBaseTarget = (char *)tag->mValue;
				break;
		}
	}
	mDocument->Unlock();
}

// ============================================================================
// BODY Tags

void DocumentBuilder::AddBody(Tag *tag)
{
	if (mAddedBody)
		return;	
	mAddedBody = true;
	
	char* background = 0;
	long bgcolor = 0x00C0C0C0;	// gray 198
	//long bgcolor = GetSysColor (COLOR_BTNFACE);
	long link = 0x0000FF;		// Link color
	long vlink = 0x52188C;		// Visited link color
	long alink = 0xFF0000;		// Active link color
	long text = 0x000000;		// Color of normal text
	BString loadScript;
	BString unloadScript;
	long marginWidth = -1;
	long marginHeight = -1;
	
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
			case A_BACKGROUND:{
					background = (char *)tag->mValue;// Image SRC
					break;	
			}					
			case A_LINK:		link = tag->mValue;					break;	// Color of links
			case A_VLINK:		vlink = tag->mValue;				break;	// Color of visited links
			case A_ALINK:		alink = tag->mValue;				break;	// Color of active links
			case A_BGCOLOR:		bgcolor = tag->mValue;				break;	// Background color
			case A_TEXT:		text = tag->mValue;					break;	// Color of text
			case A_ONLOAD:
				loadScript = (const char *)tag->mValue;
				break;
			case A_ONUNLOAD:
				unloadScript = (const char *)tag->mValue;
				break;
			case A_MARGINWIDTH:
			case A_LEFTMARGIN:
				marginWidth = tag->mValue;
				break;
			case A_MARGINHEIGHT:
			case A_TOPMARGIN:
				marginHeight = tag->mValue;
				break;
		}

	if (!mDocument->LockDocAndWindow())
		return;

	mDocument->AddBackground(background,text,bgcolor,link,vlink,alink);
#ifdef JAVASCRIPT
	if (loadScript.Length())
		mDocument->AddScript(loadScript.String(), A_ONLOAD);
	if (unloadScript.Length())
		mDocument->AddScript(unloadScript.String(), A_ONUNLOAD);
#endif
	if (marginWidth >= 0)
		mDocument->SetMarginWidth(marginWidth);
	if (marginHeight >= 0)
		mDocument->SetMarginHeight(marginHeight);
		
	mDocument->UnlockDocAndWindow();
}


// ============================================================================
//	Add a child to the current page

void DocumentBuilder::AddGlyph(Glyph* glyph)
{
	if (mGlyph->IsTable())						// Don't add glyphs into raw tables?
		return;

	if (mPRECount > 0 && glyph->IsText())		// Set text to PRE if required
		glyph->SetPRE(true);
		
	if (mPendingLineBreak != Glyph::kNoBreak) {	// Attach break to glyph
		glyph->SetBreakType(mPendingLineBreak);
		mPendingLineBreak = Glyph::kNoBreak;
	}
	mGlyph->AddChild(glyph);
	
//	Cound number of glyphs generated

	if (glyph->IsSpatial())
		mParagraphIsEmpty = false;
	
	mGlyphCount++;		
	if (mGlyphCount % 100 == 0)
		pprint("Glyph Count: %d",mGlyphCount);
}

//	Let the form have first crack at the text, return if it handled it
//	Textcount may be NULL, forms still find that useful

void DocumentBuilder::AddText(const char *text, short textCount)
{
	if (mInScript) {
		if (mScript.Length() == 0 && strncmp(text, "<--", 3) == 0)
			mScript += (text + 3);
		else
			mScript += text;
		return;
	}
	
	if (mIECommentCount > 0)
//	if (mInScript || mIECommentCount > 0)
		return;
		
	if (mInObject)
		return;
		
	if (mInEmbed || mNoEmbed)
		return;
		
	if (mInTitle) {
		AddToTitle(text);
		return;
	}

	if (mNoFrame)
		return;
	
#if 0
	if (mMarquee != NULL) {
		mMarquee->AddText(text);
		return;
	}
#endif

	if (mInStyle) {
		mStyleInfo += text;
		return;
	}
	
	if (mInTextArea) {
		if (mForm)
			mForm->AddTextAreaText(text);
		return;
	}

	if (mForm && mForm->AddText(text,textCount))
		return;

	if (!textCount)
		return;
		
	TextGlyph *g = new TextGlyph(mDocument, mInNoBreak);				// Add text to current glyph
	Style style = mCurrentStyle;
	if (mCurrentAnchor) {
		const char *href = ((AnchorGlyph *)mCurrentAnchor)->GetHREF();	// Can't be fragment
		if (href && *href)
			style.anchor = 1;
	}
	if (!mDocument->Lock()) return;
	mDocument->AddText(g,style,text,textCount);	// Make the document remember text
	mDocument->Unlock();

	AddGlyph(g);
}

void DocumentBuilder::OpenScript(Tag* tag)
{
	if (mInScript)
		return;
		
	const char *src = NULL;
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			if (tag->mAttributeID == A_SRC){
				src = (char *)tag->mValue;
			}
	if (src) {
		// Don't lock the document and window for this.  We could be waiting here
		// a long time, because we may block while the script file is downloaded.
#ifdef JAVASCRIPT
		mDocument->ExecuteScriptFile(src);
#endif
	}

	mInScript = true;
	mScript = "";
}

void DocumentBuilder::CloseScript()
{
	mInScript = false;
	
	if (mScript.Length()) {
		if (!mDocument->LockDocAndWindow())
			return;

		mScript.ReplaceAll("\r\n","\n");
		mScript.ReplaceAll('\r','\n');
		const char *scriptString = mScript.String();
		while (*scriptString && *scriptString == '\n')
			scriptString++;
		if (strncmp(scriptString, "<!--", 4) == 0)
			scriptString += 4;
#ifdef JAVASCRIPT
		mDocument->AddScript(scriptString);	// Make the document remember text
#endif
		mDocument->UnlockDocAndWindow();
	}
}


void DocumentBuilder::OpenLayer(bool isInline, Tag* tag)
{
#ifdef LAYERS
	if (!gPreferences.FindBool("SupportLayerTag"))
		return;
	int32 left = -1, top = -1, width = -1, height = -1;
	int32 zIndex = 0;
	int32 visibility = AV_SHOW;
	BString id;
	BString src;
	BString above;
	BString below;
	BString clip;
	long bgColor = 0xffffff;
	BString background;
	
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
			case A_ID:
				id = (char *)tag->mValue;
				break;
			case A_LEFT:		left = tag->mValue;					break;
			case A_TOP:			top = tag->mValue;					break;
			case A_PAGEX:		left = tag->mValue;					break;
			case A_PAGEY:		top = tag->mValue;					break;
			case A_SRC:
				src = (char *)tag->mValue;
				break;
			case A_ZINDEX:		zIndex = tag->mValue;				break;
			case A_ABOVE:
				above = (char *)tag->mValue;
				break;
			case A_BELOW:
				below = (char *)tag->mValue;
				break;
			case A_WIDTH:		width = tag->mValue;				break;
			case A_HEIGHT:		height = tag->mValue;				break;
			case A_CLIP:		clip = tag->mValue;					break;
			case A_VISIBILITY:	visibility = tag->mValue;			break;
			case A_BGCOLOR:		bgColor = tag->mValue;				break;
			case A_BACKGROUND:
				background = (char *)tag->mValue;
				break;
		}

	if (!mDocument->Lock()) return;
	mLayer = new DocumentGlyph(mDocument, true);
	mLayer->SetParent(mGlyph);
	mGlyph = mLayer;
	mDocument->OpenLayer(mLayer, left, top, width, height, zIndex, visibility, id.String(), src.String(), above.String(), below.String(), clip.String(), bgColor, background.String());
	mDocument->Unlock();
#endif
}


void DocumentBuilder::CloseLayer()
{
#ifdef LAYERS
	if (!gPreferences.FindBool("SupportLayerTag"))
		return;
		
	if (!mLayer)
		return;
		
	mGlyph = (PageGlyph *)mLayer->GetParent();
	
	mLayer = NULL;
	
	if (!mDocument->LockDocAndWindow()) return;
	mDocument->CloseLayer();
	mDocument->UnlockDocAndWindow();
#endif
}

//===============================================
//	Embedded Objects

void DocumentBuilder::OpenObject(Tag* tag)
{
	//note, OpenObject and OpenEmbed need to do very similar things
	if (mInObject || mInEmbed)
		return;
	
	BString origTag;
	for (Tag *tag2 = tag; tag2; tag2 = (Tag *)tag2->Next()) {
		origTag += tag2->mOrigAttribute;
		if (tag2->mOrigValue && *tag2->mOrigValue) {
			origTag += "=";
			origTag += tag2->mOrigValue;
		}
		if (tag2->Next())
			origTag += " ";
	}

	BMessage *msg = 0;
	if (mPluginData)
		for (int i = 0; i < mPluginData->CountItems(); i++) {
			PluginData *data = (PluginData *)mPluginData->ItemAt(i);
			if (data->mParameters == origTag) {
				msg = data->mData;
				break;
			}
		}

	mObject = ObjectGlyph::CreateObjectGlyph(tag, origTag.String(), mDocument, msg);
	if (!mObject)
		return;

	mInObject = true;
	SetGlyphAttributes(mObject,tag);
}

void DocumentBuilder::CloseObject()
{
	if (!mInObject)
		return;

	mInObject = false;
	AddGlyph(mObject);
	if (mObject) {
		if (!mDocument->Lock()) return;
		mDocument->AddObject(mObject);
		mDocument->Unlock();
	}
	mObject = NULL;
}

void DocumentBuilder::AddParam(Tag* tag)
{
	if (!mInObject)
		return;

	if (mObjectTag == NULL)
		return;

	(*mObjectTag) += "<PARAM";
	ReconstituteTag(tag);
	(*mObjectTag) += ">";	
}

//	Older but still used

void DocumentBuilder::OpenApplet(Tag* tag)		// Embeded objects
{
#ifdef JAVAGE
	if (mInObject)
		return;

	delete (mObjectTag);
	mObjectTag = new BString;

	(*mObjectTag) = "<APPLET";
	ReconstituteTag(tag);
	(*mObjectTag) += ">";

	BString theFullURL;
	if (!mDocument->Lock()) return;
	mDocument->ResolveURL(theFullURL, NULL);
	mDocument->Unlock();

	URLParser theURLParser;
	theURLParser.SetURL(theFullURL.String());

	BString theURL;
	switch (theURLParser.Scheme()) {
		case kFILE:
			theURL = "file://";
			break;

		case kFTP:
			theURL = "ftp://";
			break;

		case kHTTP:
		default:
			theURL = "http://";
			break;
	}
	theURL += theURLParser.NetLocation();
	theURL += theURLParser.BasePath();

	mInObject = true;
	mObject = new JavaGlyph(mDocument);
	mObject->SetURL(theURL.String());
	SetGlyphAttributes(mObject, tag);
#endif
}

void DocumentBuilder::CloseApplet()
{
#ifdef JAVAGE
	if (!mInObject)
		return;

	if (mObjectTag == NULL)
		return;

	(*mObjectTag) += "</APPLET>";

	mObject->SetTag((*mObjectTag).String());
	AddGlyph(mObject);
	mInObject = false;
	mObject = NULL;

	delete (mObjectTag);
	mObjectTag = NULL;
#endif
}

void DocumentBuilder::OpenEmbed(Tag* tag)
{
	//note, OpenObject and OpenEmbed need to do very similar things
	if (mInEmbed || mInObject)
		return;

	BString origTag;
	for (Tag *tag2 = tag; tag2; tag2 = (Tag *)tag2->Next()) {
		origTag += tag2->mOrigAttribute;
		if (tag2->mOrigValue && *tag2->mOrigValue) {
			origTag += "=";
			origTag += tag2->mOrigValue;
		}
		if (tag2->Next())
			origTag += " ";
	}

	BMessage *msg = 0;
	if (mPluginData)
		for (int i = 0; i < mPluginData->CountItems(); i++) {
			PluginData *data = (PluginData *)mPluginData->ItemAt(i);
			if (data->mParameters == origTag) {
				msg = data->mData;
				break;
			}
		}

	mEmbed = ObjectGlyph::CreateObjectGlyph(tag, origTag.String(), mDocument, msg);
	if (!mEmbed)
		return;

	mInEmbed = true;
//	mObject = new ObjectGlyph;
	SetGlyphAttributes(mEmbed,tag);
}

void DocumentBuilder::CloseEmbed()
{
	mInEmbed = false;
	if (mEmbed)
		AddGlyph(mEmbed);
	mEmbed = NULL;
}


//===============================================

void DocumentBuilder::AddBackgroundSound(Tag *tag)
{
	if (gPreferences.FindBool("PlaySounds")) {
		if (!mDocument->Lock()) return;
		const char*	src = NULL;
		bool loop = false;
	
		if (tag)
			for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
				switch (tag->mAttributeID) {
					case A_SRC:		src = (const char*)tag->mValue;		break;
					case A_LOOP:	loop = (bool)(int)tag->mValue;		break;
				}
		mDocument->AddBackgroundSound(src, loop);
		mDocument->Unlock();
	}
}

void DocumentBuilder::AddLink(Tag *tag)
{
	const char*	rel = NULL;
	const char* href = NULL;

	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
				case A_REL:		rel = (const char*)tag->mValue;		break;
				case A_HREF:	href = (const char*)tag->mValue;	break;
			}	

//	If this is a "Next" link, create a resource to start preloading of the page.
//	..sometime...

}

// Add a simple form to allow entry of search keywords.

void DocumentBuilder::AddIndex(Tag *)
{	
	AddRule(NULL);
	
	if (!mDocument->Lock()) return;
	TextGlyph* g = new TextGlyph(mDocument, mInNoBreak);
	mDocument->AddText(g,mCurrentStyle,kSearchableIndexText,strlen(kSearchableIndexText));
	AddGlyph(g);
	AddNewLine(NULL);
	BlankLine();

	OpenForm(NULL);
	
	g = new TextGlyph(mDocument, mInNoBreak);
	mDocument->AddText(g,mCurrentStyle,kSearchableKeywordsText,strlen(kSearchableKeywordsText));
	AddGlyph(g);

	AddInput(NULL);
	CloseForm();
	
	AddRule(NULL);
	mDocument->SetIsIndex(true);
	mDocument->Unlock();
}


//	Linebreaks include breaking left, right an all around floating pictures
//	<P> or <Hx> tags can include alignment stuff too

void DocumentBuilder::AddNewLine(Tag *tag)
{
	Glyph::BreakType	breakType = Glyph::kHard;
	
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next()) {
			switch (tag->mAttributeID) {
				case A_CLEAR:
					switch (tag->mValue) {
						case AV_LEFT:	breakType = Glyph::kHardLeft;	break;
						case AV_RIGHT:	breakType = Glyph::kHardRight;	break;
						case AV_BOTH:
							// CLEAR=BOTH is illegal HTML, but some sites use it.
							tag->mValue = AV_ALL;
						case AV_ALL:
						default:		breakType = Glyph::kHardAll;	break;
					}
					break;
			}
		}
	
	if (mHaveTempMargin)
		MoveMargin(0, false, true);
		
	if (mPendingLineBreak != Glyph::kNoBreak) {
		LineBreakGlyph* lineBreak = new(LineBreakGlyph);
		lineBreak->SetBreakType(mPendingLineBreak);
		AddGlyph(lineBreak);
	}
	mPendingLineBreak = breakType;
}

//	Add a Blank line if there isn't one there already

void DocumentBuilder::BlankLine()
{
	Glyph* g, *gg;
	
	// If a paragraph break is already pending..
	
	if (mPendingLineBreak == Glyph::kParagraph || mPendingLineBreak == Glyph::kHardAll)
		return;

	// Determine if this is the first line or a new margin.
	
	for (g = mGlyph->GetLastChild(); g && (g->IsAnchor() || g->IsAnchorEnd() || g->IsText()); g = (Glyph*)g->Previous()) {
		if (g->IsText()) {
			TextGlyph* text = (TextGlyph*)g;
			if (text->GetTextCount() > 1 || *text->GetText() != ' ')
				break;
		}
	}
	
	// First in page or new margin, align only.
	
	if (g == NULL || g->Floating() || g->IsBullet())	
		return;

	g = mGlyph->GetLastChild();
	gg = (Glyph *)g->Previous();

	if (!g->IsSpatial() && g->GetBreakType() == Glyph::kParagraph)
		return;
		
	if (g->IsSpatial() || !g->IsExplicitLineBreak() || !(gg != NULL && gg->IsExplicitLineBreak()))
		mPendingLineBreak = Glyph::kParagraph;
}


void DocumentBuilder::AddImage(Tag *tag)
{		
	if (!mDocument->LockDocAndWindow())
		return;

	ImageGlyph *image = new ImageGlyph(mDocument->GetConnectionManager(), mForceCache, mDocument->mWorkerMessenger, mDocument);
	SetGlyphAttributes(image,tag);
//	if (mInNoBreak || mPRECount > 0)
//		image->SetBreakType(Glyph::kNoBreak);
	image->SetNoBreak(mInNoBreak || mPRECount > 0);
	
	AddGlyph(image);

//	Image contains ImageMap anchors but isnt one...
			
	if (image->GetUSEMAP() && *image->GetUSEMAP() && mCurrentAnchor == NULL) {
		OpenAnchor(NULL);
		mCurrentAnchor->AddGlyph(image);
		image->SetAnchor(mCurrentAnchor);
		CloseAnchor();
	}
	
//	Not for target anchors or compound anchors with images

	if (mCurrentAnchor != NULL && mCurrentAnchor->GetHREF() && *mCurrentAnchor->GetHREF())
		image->SetAnchor(mCurrentAnchor);

//	Add the image to the document for loading

	mDocument->AddImage(image);
	mDocument->UnlockDocAndWindow();
}

//	Add a horizontal rule

void DocumentBuilder::AddRule(Tag *tag)
{
	CloseParagraph();			// Close current paragraph.
	
	RuleGlyph *rule = new RuleGlyph(mDocument);
	SetGlyphAttributes(rule,tag);

	int oldAlign = mCurrentAlign;
	Align(rule->GetHAlign());		
	AddGlyph(rule);
	Align(oldAlign);
	AddNewLine(NULL);
}

// ============================================================================
// Anchors

void DocumentBuilder::CloseStyle()
{
	mInStyle = false;
}

void DocumentBuilder::OpenHeading(Tag* tag)
{
	PushAlignment();
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
				case A_ALIGN:	Align((AttributeValue)tag->mValue); 	break;		// Get the alignment of the heading
			}
		
	BlankLine();		// Need at lest one space before heading
}

void DocumentBuilder::OpenFrameset(Tag* tag)
{
	char *rows = NULL;
	char *cols = NULL;
	int32 border = 1;
	bool gotBorder = false;
	if (tag != NULL)
		for (Tag *theTag = (Tag *)tag->First(); theTag != NULL; theTag = (Tag *)theTag->Next()){
			if(theTag->mAttributeID == A_FRAMESPACING){
				if(!gotBorder)//border always overrides in other browsiers
					border = theTag->mValue;
			}
			else if(theTag->mAttributeID == A_BORDER){
				gotBorder = true;
				border = theTag->mValue;
			}
			else if (theTag->mAttributeID == A_COLS){
				cols = (char *)theTag->mValue;
			}
			else if (theTag->mAttributeID == A_ROWS){
				rows = (char *)theTag->mValue;
			}
		}
		
	if(border > 10) //too much is just plain silly
		border = 10;
		
	if (!mDocument->Lock()) return;
	mDocument->OpenFrameset(rows, cols, border);
	mDocument->Unlock();
}

void DocumentBuilder::CloseFrameset()
{
	if (!mDocument->Lock()) return;
	mDocument->CloseFrameset();
	mDocument->Unlock();
}

void DocumentBuilder::AddFrame(Tag* tag)
{	
	if (tag == NULL)
		return;
	
//	Get a src

	Tag* tempTag = tag->Find(A_SRC);
	if (tempTag == NULL) return;
	const char* src = (const char*)tempTag->mValue;
	if (!src || !(*src))
		return;
	
#ifdef ADFILTER
	if (mFilterList && AdFilter::ShouldFilterTag(mFilterList, T_FRAME, &tag)) {
		if (!mDocument->Lock()) return;
		mDocument->RemoveFrame();
		mDocument->Unlock();
		return;
	}
#endif
	
//	Get a name
	const char *name = NULL;
	tempTag = tag->Find(A_NAME);
	if (tempTag != NULL) 
		name = (const char*)tempTag->mValue;

	int32 scroll = kAutoScrolling;
	tempTag = tag->Find(A_SCROLLING);
	if (tempTag != NULL) {
		switch (tempTag->mValue) {
			case AV_NO:
				scroll = kNoScrolling;
				break;

			case AV_YES:
				scroll = kYesScrolling;
				break;

			case AV_AUTO:
			default:
				scroll = kAutoScrolling;
				break;				
		}
	}

	int32 mwidth = 0;
	tempTag = tag->Find(A_MARGINWIDTH);
	if (tempTag != NULL)
		mwidth = tempTag->mValue;

	int32 mheight = 0;
	tempTag = tag->Find(A_MARGINHEIGHT);
	if (tempTag != NULL)
		mheight = tempTag->mValue;

	int32 border = 1; //we'll default to having a border and set to false only if the attribute == 0
	tempTag = tag->Find(A_BORDER);
	if (tempTag != NULL)
		border = tempTag->mValue;

	if (!mDocument->Lock()) return;
	mDocument->AddFrame(scroll, mwidth, mheight, name, src, border != 0);
	mDocument->Unlock();
}

void DocumentBuilder::OpenStyle(Tag*)
{
	CloseStyle();
	mStyleInfo = "";
	mInStyle = true;
}

// Only mark InParagraph and push alignment if an explicit alignment if used.

void DocumentBuilder::OpenParagraph(Tag* tag)
{
	// If nothing interesting has been added since the last paragraph tag, then
	// the paragraph is empty.  Per the HTML spec, drop it on the floor.
	if (mParagraphIsEmpty)
		return;
		
	CloseParagraph();
	
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
				case A_ALIGN:
					mInParagraph = true;
					PushAlignment();
					Align((AttributeValue)tag->mValue);
				break;
			}

	if (mPRECount)
		AddNewLine(NULL);
	else
		BlankLine();
		
	mParagraphIsEmpty = true;
}

void DocumentBuilder::OpenCenter(Tag*)
{
	PushAlignment();	
	Align(AV_CENTER, true);
	mInParagraph = false;
}

void DocumentBuilder::OpenDivision(Tag* tag)
{
	short	align = mCurrentAlign;
	
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
				case A_ALIGN:	align = tag->mValue;	break;
			}
		
	PushAlignment();
	Align(align, true);
}

void DocumentBuilder::OpenMarquee(Tag*)
{
#if 0
	Marquee*	marquee = new(Marquee);
	
	SetDisplayableAttributes(marquee, tagList);
	
	AddDisplayable(marquee);
	fDocument->AddDynamicDisplayable(marquee);
	
	marquee->SetStyle(fCurrentStyle);
	
	fMarquee = marquee;
#endif
}

void DocumentBuilder::PushAlignment()
{
	if (mAlignStackDepth < 100) {
		int align = mCurrentAlign;
		if (mInParagraph)
			align = -align;
		mAlignStack[mAlignStackDepth] = align;
	}
	mAlignStackDepth++;
}

short DocumentBuilder::PopAlignment()
{
	if (mAlignStackDepth > mGlyph->GetAlignStackDepth()) {
		mAlignStackDepth--;
		if (mAlignStackDepth < 100) {
			int align = mAlignStack[mAlignStackDepth];
			if (align < 0) {
				mInParagraph = true;		// Restore paragraph state
				align = - align;
			}
			return align;
		}
	}
	return mCurrentAlign;
}


void	DocumentBuilder::OpenAnchor(Tag *tag)
{
	CloseAnchor();
	AnchorGlyph *g = new AnchorGlyph(mDocument);
	g->SetTarget(mBaseTarget.String());
	SetGlyphAttributes(g,tag);
	if (mGlyph) {
		AddGlyph(g);
		mCurrentAnchor = g;	// May ave been deleted by addchild!еееееееееееееееее
	}
}

void	DocumentBuilder::CloseAnchor()
{
	if (mCurrentAnchor == NULL)
		return;
	if (!mDocument->LockDocAndWindow())
		return;

	mDocument->AddAnchor(mCurrentAnchor);	// Add anchors on close
	mDocument->UnlockDocAndWindow();
	AddGlyph(new AnchorEndGlyph);
	mCurrentAnchor = NULL;
}

// ============================================================================
// Font Styles

//	Nest styles correctly

void DocumentBuilder::PushStyle()
{
	if (mStyleStackDepth < 100)
		mStyleStack[mStyleStackDepth] = mCurrentStyle;
	mStyleStackDepth++;
}

void DocumentBuilder::PopStyle()
{
	if (mStyleStackDepth > mGlyph->GetStyleStackDepth()) {
		mStyleStackDepth--;
		mCurrentStyle = mStyleStack[mStyleStackDepth];
	} else
		pprint("PopStyle(): stack emptied");
}

void DocumentBuilder::SetTagStyle(Style style, bool set)
{
	if (set) {
		PushStyle();
		if (style.fontSize != 0)
			mCurrentStyle.fontSize = style.fontSize;
		else
			mCurrentStyle.fontSize = mBaseFont;
		if (style.fontID) mCurrentStyle.fontID = 1;
		if (style.bold) mCurrentStyle.bold = 1;
		if (style.italic) mCurrentStyle.italic = 1;
		if (style.underline) mCurrentStyle.underline = 1;
		if (style.pre) mCurrentStyle.pre = 1;
		if (style.blink) mCurrentStyle.blink = 1;

		// If the style specifies a monospaced font, always override any pending
		// font face setting.  Ideally, we should check to see if the font face
		// setting is itself monospaced, but that's too hard.
		if (style.pre || style.fontID) *mCurrentStyle.fontFace = 0;

		if (style.subscript) mCurrentStyle.subscript = 1;
		if (style.superscript) mCurrentStyle.superscript = 1;
	} else
		PopStyle();
}

void DocumentBuilder::BaseFont(Tag *tag)
{
	if (tag) {
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next()) {
			if (tag->mAttributeID == A_SIZE) {
				if (tag->mIsPercentage)
					mBaseFont += tag->mValue;
				else
					mBaseFont = tag->mValue;
	
				// This is imperfect, because it may incorrectly apply the new
				// base font to the current style (if it is a relative font size off of
				// the base font), and it won't go through pushed style states to reset
				// their sizes.
				mBaseFont = MIN(MAX(mBaseFont, 1), 7);
				mCurrentStyle.fontSize = mBaseFont;
				gDefaultStyle.fontSize = mBaseFont;
			}
		}
	}
}

void DocumentBuilder::Font(Tag *tag)
{
	if (!tag) {
		PopStyle();
	} else {
		PushStyle();
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
				case A_SIZE:
					if (tag->mIsPercentage)
						mCurrentStyle.fontSize = MIN(7,MAX(1,tag->mValue + mBaseFont));	// Relative
					else
						mCurrentStyle.fontSize = MIN(7,MAX(1,tag->mValue));				// Absolute
					break;
				case A_COLOR:
					mCurrentStyle.fontColorFlag = 1;
					mCurrentStyle.fontColor = tag->mValue;
					break;
				case A_FACE: {
					if (gPreferences.FindBool("UseFonts") &&
						(mEncoding == B_ISO1_CONVERSION || mEncoding == B_MS_WINDOWS_CONVERSION)) {
						// First, look for an exact match.
						const char *face = NULL;
						const char *curPos = 0;
						curPos = (const char *)tag->mValue;
						BString curFont;
						while (face == NULL) {
							while (curPos && *curPos && isspace(*curPos))
								curPos++;
							if (!curPos || !(*curPos))
								break;
							curFont = curPos;
							const char *commaPos = strchr(curPos, ',');
							if (commaPos)
								curFont.Truncate(commaPos - curPos);
							int matchType;
							const char *tmp = FontSubFolder::MapFont(curFont.String(), matchType);
							if (matchType == FontSubFolder::kMatched) {
								face = tmp;
								break;
							}
							if (commaPos)
								curPos = commaPos + 1;
							else
								break;
						}
						
						curPos = (const char *)tag->mValue;
							
						// Now, look for a suitable substitution.
						while (face == NULL) {
							while (curPos && *curPos && isspace(*curPos))
								curPos++;
							if (!curPos || !(*curPos))
								break;
							curFont = curPos;
							const char *commaPos = strchr(curPos, ',');
							if (commaPos)
								curFont.Truncate(commaPos - curPos);
							int matchType;
							const char *tmp = FontSubFolder::MapFont(curFont.String(), matchType);
							if (matchType == FontSubFolder::kMapped) {
								face = tmp;
								break;
							}
							if (commaPos)
								curPos = commaPos + 1;
							else
								break;
						}
	
						if (face) {
							strncpy(mCurrentStyle.fontFace, face, 63);
							mCurrentStyle.fontFace[63] = 0;
						} else {
							*mCurrentStyle.fontFace = 0;
						}
					}
					break;
				}
			}
	}
}

void DocumentBuilder::Big(Tag *)
{
	PushStyle();
	if (mCurrentStyle.fontSize == 0)
		mCurrentStyle.fontSize = 4;
	else
		mCurrentStyle.fontSize = MIN(7, mCurrentStyle.fontSize + 1);
}

void DocumentBuilder::Small(Tag *)
{
	PushStyle();
	if (mCurrentStyle.fontSize == 0)
		mCurrentStyle.fontSize = 2;
	else
		mCurrentStyle.fontSize = MAX(1, mCurrentStyle.fontSize - 1);
}

//	Support nesting of styles

void DocumentBuilder::Bold(bool on)
{
	if (on) {
		PushStyle();
		mCurrentStyle.bold = 1;
	} else
		PopStyle();
}

void DocumentBuilder::Italic(bool on)
{
	if (on) {
		PushStyle();
		mCurrentStyle.italic = 1;
	} else
		PopStyle();
}

void DocumentBuilder::UnderLine(bool on)
{
	if (on) {
		PushStyle();
		mCurrentStyle.underline = 1;
	} else
		PopStyle();
}

void DocumentBuilder::Align(short align, bool force)
{
	if (align != mCurrentAlign || force) {
		AlignmentGlyph* g = new(AlignmentGlyph);	// Set the current style
		g->SetAlign(align);
		AddGlyph(g);
		mCurrentAlign = align;
	}
}

//==================================================================

void DocumentBuilder::CloseCenter()
{
	Align(PopAlignment(), true);
}

void DocumentBuilder::CloseDivision()
{
	CloseParagraph();
	Align(PopAlignment(), true);
}

void DocumentBuilder::CloseHeading()
{
	BlankLine();
	Align(PopAlignment());
}

void DocumentBuilder::CloseMarquee()
{
#if 0
	if (mMarquee != NULL) {
		mMarquee->Close();		
		mMarquee = NULL;
	}
#endif
}

void DocumentBuilder::CloseParagraph(bool forceBlankLine)
{
	if (mInParagraph) {
		Align(PopAlignment());
		mInParagraph = false;
		BlankLine();
	}
	// Do the blank line even if we were not in a paragraph
	// because Netscape does.
	else if (forceBlankLine)
		BlankLine();
}

// ============================================================================
// Tables


void DocumentBuilder::OpenTable(Tag *tag)
{
// If we're in a table and not currently in a cell, close the table.

	if (mTable != NULL && mGlyph->IsCell() == false)
		CloseTable();

//	Create the table but don't add it yet

	TableGlyph* table = new TableGlyph(mDocument);
	SetGlyphAttributes(table,tag);
	table->SetParentTable(mTable);
	table->SetParent(mGlyph);
	
	mTable = table;	
	table->Open();
}

void DocumentBuilder::OpenTableRow(Tag *tag)
{
	if (mTable == NULL)
		return;
		
	CloseTableRow();
	mTable->OpenRow();
	SetGlyphAttributes(mTable,tag);
}

void DocumentBuilder::OpenTableCell(Tag *tag, bool isHeading)
{
	if (mTable == NULL)
		return;

	CloseTableCell();
	
//	Finish any pending line breaks.
	
	if (mPendingLineBreak != Glyph::kNoBreak) {
		AddNewLine(NULL);
		mPendingLineBreak = Glyph::kNoBreak;
	}

//	Create and open a new cell

	CellGlyph *g = mTable->NewCell(isHeading);
	SetGlyphAttributes(g,tag);
	
	mTable->OpenCell(g,isHeading);
	mGlyph = g;
	
//	Preserve Style, Alignment and List Depth
	
	PushStyle();
	PushAlignment();
	mGlyph->SetStack(mStyleStackDepth,mAlignStackDepth,mListDepth,mLeftMarginDepth,mRightMarginDepth);
	
	mCurrentStyle = gDefaultStyle;
//	mCurrentAlign = mGlyph->GetPageAlign();
	mCurrentAlign = mGlyph->GetAlign();
	Align(mCurrentAlign,true);
	mLeftMarginDepth = 0;
	mRightMarginDepth = 0;
	mParagraphIsEmpty = true;
	
//	Default to bold

	if (isHeading)
		mCurrentStyle.bold = true;
}

void DocumentBuilder::CloseTableCell()
{
	CloseAnchor();
	if (mTable == NULL || !mTable->IsInCell())
		return;

//	Finish any pending line breaks.

	if (mPendingLineBreak != Glyph::kNoBreak) {
		AddNewLine(NULL);
		mPendingLineBreak = Glyph::kNoBreak;
	}
	mTable->CloseCell();

//	Restore glyph first, so that we use the stack base limits
//	of the parent when popping style and alignment.
	
	mGlyph->GetStack(&mStyleStackDepth,&mAlignStackDepth,&mListDepth,&mLeftMarginDepth,&mRightMarginDepth);
	mGlyph = (PageGlyph*)mTable->GetParent();
	
//	Restore style,alignment,list depth and margin depth to values before cell open
	
	PopStyle();
	mCurrentAlign = PopAlignment();	
}

void DocumentBuilder::CloseTableRow()
{
	if (mTable == NULL || !mTable->IsInRow())
		return;
		
	CloseTableCell();
	mTable->CloseRow();
}

void DocumentBuilder::CloseTable()
{
	if (mTable == NULL)
		return;
		
	CloseTableRow();		
	mTable->CloseTable();
	AddGlyph(mTable);
	
//	Text following table must align.
	
	mPendingLineBreak = Glyph::kHardAlign;
	mTable = mTable->GetParentTable();
}

//	Save current style and stack depth to restore on table close

void DocumentBuilder::OpenCaption(Tag *tag)
{
	if (mTable == NULL)
		return;
		
	CellGlyph *g = new CellGlyph(mDocument);
	SetGlyphAttributes(g,tag);
	
	mTable->OpenCaption(g);
	mGlyph = g;
	
	//	Preserve Style, Alignment and List Depth
	
	PushStyle();
	PushAlignment();
	mGlyph->SetStack(mStyleStackDepth,mAlignStackDepth,mListDepth,mLeftMarginDepth,mRightMarginDepth);
	
	mCurrentStyle = gDefaultStyle;
//	mCurrentAlign = mGlyph->GetPageAlign();
	mCurrentAlign = mGlyph->GetAlign();
	Align(mCurrentAlign,true);
	mLeftMarginDepth = 0;
	mRightMarginDepth = 0;
}

//	Restore style to the style in effect at caption open

void DocumentBuilder::CloseCaption()
{
	if (mTable == NULL)
		return;
		
	mTable->CloseCaption();

	mGlyph->GetStack(&mStyleStackDepth,&mAlignStackDepth,&mListDepth,&mLeftMarginDepth,&mRightMarginDepth);
	mGlyph = (PageGlyph*)mTable->GetParent();
	
//	Restore style,alignment,list depth and margin depth to values before cell open
	
	PopStyle();
	mCurrentAlign = PopAlignment();	
}

// ============================================================================
// Forms

void DocumentBuilder::OpenForm(Tag *tag)
{
	short method = AV_GET;
	char* action = 0;
	char* target = 0;
	char* name = 0;
	char* enctype = 0;
	BString onSubmitScript;
	BString	onResetScript;

	//Copy Other BrIEwsers
	if (mPRECount)
		AddNewLine(NULL);
	else
		BlankLine();
		
	Form *form = new Form(mDocument);
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
				case A_METHOD:	method = tag->mValue;			break;
				case A_ACTION:
					action = (char*)tag->mValue;
					break;	// Autoexecute?
				case A_TARGET:
					target = (char*)tag->mValue;
					break;
				case A_NAME:
					name = (char*)tag->mValue;
					break;
				case A_ONSUBMIT:
					onSubmitScript = (char*)tag->mValue;
					break;
				case A_ONRESET:
					onResetScript = (char*)tag->mValue;
					break;
				case A_ENCTYPE:
					enctype = (char*)tag->mValue;
					break;
			}
	form->OpenForm(action,method, target, name, enctype);
	form->SetEncoding(mEncoding);
#ifdef JAVASCRIPT
	if (onSubmitScript.Length())
		form->SetOnSubmitScript(onSubmitScript.String());
	if (onResetScript.Length())
		form->SetOnResetScript(onResetScript.String());
#endif
	if (!mDocument->Lock()) return;
	mDocument->AddForm(form);
	mDocument->Unlock();
	mForm = form;
	mFormList.AddItem(form);
}

InputGlyph*	NewInputGlyph(short type, bool forceCache, ConnectionManager *mgr, BMessenger *listener, Document *htmlDoc)
{
	InputGlyph *g=0;
	switch (type) {
		case AV_NUMBER:		// HTML does not define a "NUMBER" input type, but some sites use it
							// and other browsers accept it.  We'll map it to an InputText type.
							
		default:			// HTML sucks.  Some sites make up their own types and expect the browser
							// to treat them as TEXT.
		case AV_INPUT:
		case AV_TEXT:		g = new BeInputText(false, htmlDoc);	break;
		case AV_PASSWORD:	g = new BeInputText(true, htmlDoc);	break;
		case AV_IMAGE:		g = new BeInputImage(mgr, forceCache, listener, htmlDoc);	break;
		
		case AV_CHECKBOX:
		case AV_RADIO:
		case AV_SUBMIT:
		case AV_RESET:
		case AV_BUTTON:
			g = new BeControl(htmlDoc);
			break;
		
		case AV_HIDDEN:		g = new InputGlyph(htmlDoc, true);	break;
		
		case T_SELECT:		g = new BeSelect(htmlDoc);		break;
//		default:
//			pprint("CreateInput: Failed to create input: %d",type);
//			g = new InputGlyph(htmlDoc);
	}
	return g;
}

//	Should this be a factory function in ButtonsAndDials?

void DocumentBuilder::AddInput(Tag *tag)
{
	short inputType = AV_TEXT;
	if (!mForm) return;
	Tag *t = tag;

//	Determine what kind of input should be created

	if (t) {
		for (t = (Tag *)tag->First(); t; t = (Tag *)t->Next())
			if (t->mAttributeID == A_TYPE)
				inputType = t->mValue;
	}

	if (!mDocument->Lock()) return;
	InputGlyph* g = NewInputGlyph(inputType, mForceCache, mDocument->GetConnectionManager(), mDocument->mWorkerMessenger, mDocument);	// DrawPort has a input glyph FF
	mDocument->Unlock();
	if (g == 0) return;

	g->SetForm(mForm);	// this is a hack for the encoding stuff...
	SetGlyphAttributes(g,tag);
	mForm->AddInput(g);
	AddGlyph(g);

//	Nasty special case for input glyphs that own images ееееееееееее
//	Could be a floating input image in a form .. teriffic

	if (inputType == AV_IMAGE) {
		ImageGlyph *image = ((InputImage *)g)->GetImage();
		AddGlyph(image);
		if (!mDocument->LockDocAndWindow())
			return;
		mDocument->AddImage(image);
		mDocument->UnlockDocAndWindow();
	}
}

void DocumentBuilder::OpenSelect(Tag *tag)
{
	if (!mForm) return;
	mDocument->Lock();
	InputGlyph* g = NewInputGlyph(T_SELECT, mForceCache, mDocument->GetConnectionManager(), mDocument->mWorkerMessenger, mDocument);	// DrawPort has a input glyph FF
	mDocument->Unlock();

	SetGlyphAttributes(g,tag);
	mForm->OpenSelect(g);
}

void DocumentBuilder::OpenOption(Tag *tag)
{
	bool selected = 0;
	char* value = 0;

	if (!mForm) return;
	if (tag) {
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
				case A_SELECTED:	selected = true;				break;
				case A_VALUE:{
					value = (char *)tag->mValue;
					break;
				}			
			}
	}
	// Be careful here.  If value is NULL, then there was no value attribute.
	// If it's an empty string, then there was a blank value attribute.  We have
	// to handle these cases differently.
	mForm->Option(selected,value);
}

void DocumentBuilder::OpenOptGroup(Tag *tag)
{
	char *label = 0;
	if (!mForm) return;
	if (tag)
		for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
			switch (tag->mAttributeID) {
				case A_LABEL:{
					label = (char *)tag->mValue;
					break;
				}
			}
	if (!label)
		return;
	mForm->OpenOptGroup(label);
}

void DocumentBuilder::CloseOptGroup()
{
	if (!mForm) return;
	mForm->CloseOptGroup();
}

void	DocumentBuilder::CloseSelect()
{
	if (!mForm) return;
	InputGlyph* g = mForm->CloseSelect();
	if (g)
		AddGlyph(g);
}

void DocumentBuilder::OpenTextArea(Tag *tag)
{
	if (!mForm)
		return;

	if (!mDocument->Lock()) return;
	InputGlyph* g = NewInputGlyph(AV_TEXT, mForceCache, mDocument->GetConnectionManager(), mDocument->mWorkerMessenger, mDocument);	// DrawPort has a input glyph FF
	mDocument->Unlock();

	SetGlyphAttributes(g,tag);
	mForm->OpenTextArea(g);
	AddGlyph(g);
	
	mInTextArea = true;
}

void DocumentBuilder::CloseTextArea()
{
	if (!mForm) return;

	if (!mDocument->LockDocAndWindow())
		return;

	mForm->CloseTextArea();
	mInTextArea = false;

	mDocument->UnlockDocAndWindow();
}

void DocumentBuilder::CloseForm()
{
	if (!mForm) return;

	if (!mDocument->LockDocAndWindow())
		return;

	mDocument->FormFinished(mForm);
	CloseTextArea();
	CloseSelect();

	//Copy Other BrIEwsers
	if (mPRECount)
		AddNewLine(NULL);
	else
		BlankLine();

	mDocument->UnlockDocAndWindow();
}

//	Client side image maps

void DocumentBuilder::OpenImageMap(Tag *tag)
{
	CloseImageMap();

	// hiroshi - do this for malformed sites like www.powermacintosh.apple.com
	if (tag == NULL) 
		return;	

	char *name = NULL;
	for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
		switch (tag->mAttributeID) {
			case A_NAME:
				name = (char *)tag->mValue;
				break;
		}
	if (name) {
		if (!mDocument->Lock()) return;
		mImageMap = mDocument->AddImageMap(name);	// Add to document, it will expand hrefs with mPath
		mDocument->Unlock();
	}
}

void DocumentBuilder::AddArea(Tag *tag)
{
	if (mImageMap == NULL)
		return;
		
	char *coords = NULL;
	char *href = NULL;
	const char *target = NULL;
	int shape = AV_RECT;
	int	showTitleInToolbar = kShowTitleUnspecified;
	bool nohref = false;
	
	for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next())
		switch (tag->mAttributeID) {
			case A_SHAPE:	shape = tag->mValue;			break;
			case A_COORDS:
				coords = (char *)tag->mValue;
				break;
			case A_HREF:
				href = (char *)tag->mValue;
				break;
			case A_TARGET:
				target = (char *)tag->mValue;
				break;
			case A_SHOWTITLEINTOOLBAR: showTitleInToolbar = (tag->mValue == AV_YES) ? kShowTitle : kShowURL; break;
			case A_NOHREF:	nohref = true;					break;
		}
	if (nohref && coords)
		mImageMap->AddArea(shape,coords,"__netpositive__nohref__",NULL,showTitleInToolbar);
	else if (coords && href) {
		BString URL;
		if (!mDocument->Lock()) return;
		mDocument->ResolveURL(URL,href);		// Relative to absolute URL
		mDocument->Unlock();
		if (!target || !(*target))
			target = mBaseTarget.String();
		mImageMap->AddArea(shape,coords,URL.String(),target,showTitleInToolbar);
	}
}

void DocumentBuilder::CloseImageMap()
{
	mImageMap = NULL;
}

//	Close all the things that may still be open

void DocumentBuilder::Finalize()
{
	CloseFrameset();
	while (mTable)
		CloseTable();
	CloseParagraph();
	CloseDivision();
	CloseCenter();
	CloseStyle();
	CloseMarquee();
	CloseAnchor();
	CloseSelect();
	CloseForm();
	CloseImageMap();
	CloseObject();
	CloseLayer();

	if (!mDocument->LockDocAndWindow())
		return;
	mDocument->Finalize();
	mDocument->UnlockDocAndWindow();
}

// ============================================================================
// Protected

//	Pass Attributes to tags

void DocumentBuilder::SetGlyphAttributes(Glyph *glyph, Tag *tag)
{
	if (!tag) return;
	for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next()) {
		if (tag->mNumericValue)
			glyph->SetAttribute(tag->mAttributeID,tag->mValue,tag->mIsPercentage);
		else
			glyph->SetAttributeStr(tag->mAttributeID,(char *)tag->mValue);
	}

	if (mUserValueHead != NULL) {
		InputGlyph *ig = dynamic_cast<InputGlyph *>(glyph);
		if (ig != NULL) {	
			ig->SetUserValues(mUserValueHead->mValue.String(), mUserValueHead->mChecked, mUserValueHead->mOption);
			mUserValueHead = (InputValueItem *)mUserValueHead->Next();
		}
	}
}

// ============================================================================

void
DocumentBuilder::ReconstituteTag(
	Tag	*tag)
{
	if (mObjectTag == NULL)
		return;

	for (Tag *iTag = (Tag *)tag->First(); iTag != NULL; iTag = (Tag *)iTag->Next()) {
		(*mObjectTag) += " ";
		(*mObjectTag) += AttributeName(iTag->mAttributeID);
		(*mObjectTag) += "=";
		if (!iTag->mNumericValue) {
			(*mObjectTag) += "\"";
			(*mObjectTag) += (char *)iTag->mValue;
			(*mObjectTag) += "\"";
		}
		else {
			if (iTag->mAttributeID == A_ALIGN) 
				(*mObjectTag) += AttributeValueName(iTag->mValue);
			else {
				char numStr[10] = "";
				sprintf(numStr, "%ld", iTag->mValue);
				(*mObjectTag) += numStr;
			}
		}
	}
}

void DocumentBuilder::NetPositiveTag(Tag *tag)
{
	// The NetPositive tag!  You knew it was bound to happen sooner or later...
	for (tag = (Tag *)tag->First(); tag; tag = (Tag *)tag->Next()) {
		if (tag->mAttributeID == A_SHOWTOOLBAR)
			mDocument->ShowToolbar(tag->mValue == AV_YES);
	}

}
