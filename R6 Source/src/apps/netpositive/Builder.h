// ===========================================================================
//	Builder.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __BUILDER__
#define __BUILDER__

#include "DrawPort.h"
#include <SupportDefs.h>
#include "Glyph.h"
#include <List.h>
#include <String.h>

class TableGlyph;
class ObjectGlyph;
class PageGlyph;
class AnchorGlyph;
class ImageMap;
class Form;
class Document;
class DocumentGlyph;

// ============================================================================
//	Tag

class Tag : public CLinkable {
public:
					Tag();
virtual				~Tag();

			Tag*	Find(long attributeID);
			void	SetAttribute(long attributeID, long value, bool isPercentage);
			void	SetAttributeStr(long attributeID, const char *value);
			void	SetOrigAttribute(const char *attrName, const char *attrValue);

			bool	mNumericValue;
			bool mIsPercentage;
			long	mAttributeID;
			long	mValue;

			char	*mOrigAttribute;
			char	*mOrigValue;
};


class InputValueItem : public CLinkable {
public:
						InputValueItem(const char *value, bool checked, CLinkedList *option);
	virtual 			~InputValueItem();

	BString				mValue;
	bool				mChecked;
	CLinkedList			*mOption;
};

// ============================================================================
//	The DocumentBuilder takes output from a Parser and builds a page

class AdFilterList;

class DocumentBuilder  /*: public NPObject*/ {
public:
					DocumentBuilder(CLinkedList *list, BList *pluginData);
	virtual			~DocumentBuilder();

			void	SetEncoding(uint32 encoding);
			void	SetForceCache(bool forceCache);

			void	SetDocument(Document *document, AdFilterList *filterList);
			void	AddTag(short tagID, Tag **tag);
			
			bool	IsStyleTag(short tagID, Tag *tag);
			void	NewStyle(short tagID);

			const char	*GetTitle();
			void	AddToTitle(const char *title);					// Document structure
			void	SetPath(char *path);
			void	AddBody(Tag *tag);
			void	AddMeta(Tag *tag);
			void	AddBase(Tag *tag);

			void 	AddIndex(Tag *tag);
			void 	AddLink(Tag *tag);
			void 	AddBackgroundSound(Tag *tag);
			void 	Big(Tag *tag);
			void 	Small(Tag *tag);

			void	OpenFrameset(Tag *tag);
			void	CloseFrameset();
			void 	AddFrame(Tag *tag);
			
			void 	OpenHeading(Tag *tag);
			void 	OpenDivision(Tag *tag);
			void 	OpenCenter(Tag *tag);
			void 	OpenParagraph(Tag *tag);
			void	OpenMarquee(Tag *tag);
			void 	OpenStyle(Tag *tag);

			void 	CloseHeading();
			void 	CloseDivision();
			void 	CloseCenter();
			void 	CloseParagraph(bool forceBlankLine = false);
			void 	CloseMarquee();
			void 	CloseStyle();

			void	AddGlyph(Glyph* glyph);
			void	AddText(const char *text, short textCount);	// Text and images
			void	AddNewLine(Tag *tag);
			void	AddImage(Tag *tag);
			void	AddRule(Tag *tag);
			void	AddArea(Tag *tag);
			void	BlankLine();

			void	OpenApplet(Tag* tag);					// Embeded objects
			void	CloseApplet();
			void	OpenEmbed(Tag* tag);
			void	CloseEmbed();
			void	OpenObject(Tag* tag);
			void	CloseObject();
			void	AddParam(Tag* tag);
			
			void	OpenLayer(bool isInline, Tag* tag);
			void	CloseLayer();
			
			void	OpenScript(Tag* tag);
			void	CloseScript();
			
			void	PushList(short tagID, Tag *tag);
			void	PopList();
			void	AddBullet(Tag* tag);
			void	MoveMargin(short count, bool permanent, bool rightToo);

			void	OpenAnchor(Tag *tag);					// Anchors
			void	CloseAnchor();

			void	PushStyle();
			void	PopStyle();
			void	SetTagStyle(Style style, bool set);	// Font Styles
			void	BaseFont(Tag *tag);
			void	Font(Tag *tag);
			void	Bold(bool on);
			void	Italic(bool on);
			void	UnderLine(bool on);
			
			void	PushAlignment();
			short	PopAlignment();
			void	Align(short align, bool force = false);

			void	OpenTable(Tag *tag);					// Tables
			void	OpenTableRow(Tag *tag);
			void	OpenTableCell(Tag *tag, bool isHeading);
			void	CloseTableCell();
			void	CloseTableRow();
			void	CloseTable();
			void	OpenCaption(Tag *tag);
			void	CloseCaption();

			void	OpenForm(Tag *tag);						// Forms
			void	AddInput(Tag *tag);
			void	OpenSelect(Tag *tag);
			void	OpenOption(Tag *tag);
			void	CloseSelect();
			void	OpenOptGroup(Tag *tag);
			void	CloseOptGroup();
			void	OpenTextArea(Tag *tag);
			void	SetTextAreaValue(char *text);
			void	CloseTextArea();
			void	CloseForm();

			void	OpenImageMap(Tag *tag);
			void	CloseImageMap();
			
			void	NetPositiveTag(Tag *tag);

			void	Finalize();

protected:
			void	SetGlyphAttributes(Glyph *glyph, Tag *tag);
			void	ReconstituteTag(Tag *tag);

			uint32	mEncoding;
			bool	mForceCache;

			bool	mInTitle;				// Inside a <TITLE> tag
			bool	mInScript;				// Inside a <SCRIPT> tag
			bool	mInNoBreak;				// Inside <NOBR> tag
			bool	mInParagraph;			// Inside <P> tag
			bool	mInStyle;				// Inside <STYLE> tag
			bool	mInTextArea;			// Inside <TEXTAREA> tag
			int		mPRECount;				// Inside n <PRE> tags
			int		mIECommentCount;
			bool	mNoFrame;
			bool	mNoLayer;
			bool	mNoEmbed;
			bool	mHaveTempMargin;
			bool	mParagraphIsEmpty;		// false until we add something visible to each new paragraph.
			
			
			BString	mStyleInfo;				// Contents of Style tag
			BString mScript;				// Contents of Script tag
			
			Glyph::BreakType	mPendingLineBreak;		// Line break to attach to next glyph

			Document*	mDocument;
			PageGlyph*	mGlyph;
			TableGlyph*	mTable;
			Form*		mForm;
			AnchorGlyph	*mCurrentAnchor;
			ImageMap*	mImageMap;
			DocumentGlyph *mLayer;
			BList		*mPluginData;

#ifdef ADFILTER
			AdFilterList *mFilterList;
#endif

			short		mBaseFont;
			
//			Font Style And Color

			Style	mCurrentStyle;
			long	mCurrentFontColor;
			int		mStyleStackDepth;
			CArray<Style,32>	mStyleStack;

//			Lists

			int		mDLCount;
			int		mListDepth;
			short	mListIndex[50];
			short	mListStack[50];
			int		mLeftMarginDepth;
			int		mRightMarginDepth;

//			Alignment

			short	mCurrentAlign;
			int		mAlignStackDepth;
			short	mAlignStack[100];

			bool			mInObject;				// Current object glyph
			bool			mInEmbed;
			ObjectGlyph*	mObject;
			ObjectGlyph*	mEmbed;
			BString			*mObjectTag;	

			BString			mBaseTarget;

			int		mGlyphCount;
			bool	mAddedBody;
			BList	mFormList;
			
	InputValueItem	*mUserValueHead;
};

#endif
