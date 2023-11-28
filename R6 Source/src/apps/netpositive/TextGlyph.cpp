// ===========================================================================
//	TextGlyph.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett,  All rights reserved.
// ===========================================================================

#include "TextGlyph.h"
#include "HTMLDoc.h"
#include "TableGlyph.h"

#include <SupportDefs.h>
#include <UTF8.h>
#include <ctype.h>
#include <stdio.h>
#include <malloc.h>

void* TextGlyph::sFreeList = 0;

inline int32
UTF8CharLen(uchar c)
	{ return (((0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1); }

inline bool
IsInitialUTF8Byte(uchar b)	
	{ return ((b & 0xC0) != 0x80); }


inline uint32
UTF8CharToUint32(
	const uchar	*src,
	uint32		srcLen)
{
	uint32 result = 0;
	for (unsigned int i = 0; i < srcLen; i++)
		result |= src[i] << (24 - (i * 8));
	return (result);
}

const uint32 kBeginTaboos[] = {
0xE3808100, 0xE3808200, 0xEFBC8C00, 0xEFBC8E00, 0xEFBC9A00, 0xEFBC9B00,
0xEFBC9F00, 0xEFBC8100, 0xE2809900, 0xE2809D00, 0xEFBC8900, 0xE3809500,
0xEFBCBD00, 0xEFBD9D00, 0xE3808900, 0xE3808B00, 0xE3808D00, 0xE3808F00,
0xE3809100, 0xE3808500, 0xE383BC00, 0xE3818100, 0xE3818300, 0xE3818500,
0xE3818700, 0xE3818900, 0xE381A300, 0xE3828300, 0xE3828500, 0xE3828700,
0xE3828E00, 0xE382A100, 0xE382A300, 0xE382A500, 0xE382A700, 0xE382A900,
0xE3838300, 0xE383A300, 0xE383A500, 0xE383A700, 0xE383AE00, 0xE383B500,
0xE383B600, 0xE3829B00, 0xE3829C00, 0xE383BD00, 0xE383BE00, 0xE3829D00,
0xE3829E00, 0xE2809500, 0xE2809000, 0xC2B00000, 0xE280B200, 0xE280B300,
0xE2848300, 0xEFBC8500, 0xEFBDA100, 0xEFBDA300, 0xEFBDA400, 0xEFBDB000,
0xEFBE9E00, 0xEFBE9F00
};

const uint32 kNumBeginTaboos = sizeof(kBeginTaboos) / sizeof(kBeginTaboos[0]);


const uint32 kTerminateTaboos[] = {
0xE2809800, 0xE2809C00, 0xEFBC8800, 0xE3809400, 0xEFBCBB00, 0xEFBD9B00,
0xE3808800, 0xE3808A00, 0xE3808C00, 0xE3808E00, 0xE3809000, 0xEFBFA500,
0xEFBC8400, 0xC2A20000, 0xC2A30000, 0xEFBCA000, 0xC2A70000, 0xE3809200,
0xEFBC8300, 0xEFBDA200
};

const uint32 kNumTerminateTaboos = sizeof(kTerminateTaboos) / sizeof(kTerminateTaboos[0]);
 
// ===========================================================================
// ===========================================================================
//	TextGlyph represents a line of text or a line break

TextGlyph::TextGlyph(Document* htmlDoc, bool noBreak) :
	SpatialGlyph(htmlDoc)
{
	mDoesDrawing = true;
	mHasBounds = true;
	mTextPool = 0;
	mTextOffset = 0;
	mTextCount = 0;
	mStyle = gDefaultStyle;
	mHasBGColor = false;
	mNeedsLayout = true;
	mNeedsMinUsedWidth = true;
	mNeedsMaxUsedWidth = true;
	mNoBreak = noBreak;
}

TextGlyph::~TextGlyph()
{
}

bool TextGlyph::IsText()
{
	return true;
}

//	Return a string that describes this glyph

#ifdef DEBUGMENU
void TextGlyph::PrintStr(BString& print)
{
	SpatialGlyph::PrintStr(print);
	
	BString text;
	text.SetTo(GetText(),mTextCount);
	
	if (print.Length() != 0)
		print += " ";
	print += "\"";
	print += text;
	print += "\"";
}
#endif

//	Can this text block be separated from previous

bool TextGlyph::Separable()
{
	char *t = GetText();
	if (IsLineBreak())
		return true;
	if (mStyle.pre)
		return false;
	if (mNoBreak)
		return false;
	if (isspace(t[0])) return true;
	if (mTextOffset)
		if (isspace(t[-1])) return true;
	if (Previous() && !((Glyph *)Previous())->IsText())
		return true;
	return 0;
}

short TextGlyph::GetAlign()
{
	return 0;
}

//	Text glyphs point to text in a separate pool

void TextGlyph::SetText(CBucket *textPool, long textOffset, long textCount, Style& style)
{
	mTextPool = textPool;
	mTextOffset = textOffset;
	mTextCount = textCount;
	mStyle = style;
	mNeedsLayout = true;
	mNeedsMinUsedWidth = true;
	mNeedsMaxUsedWidth = true;	

	if (style.blink) {
		mHTMLDoc->AddBlinkGlyph(this);
	}
}

//	Split a text glyph into two pieces by inserting a soft line break
//	A newline glyph and a new text glyph will be created and inserted into the list after this

TextGlyph* TextGlyph::InsertSoftBreak(long offset,DrawPort *drawPort)
{
	TextGlyph *secondHalf = new TextGlyph(mHTMLDoc, false);

	secondHalf->SetBreakType(kSoft);
	secondHalf->SetText(mTextPool,mTextOffset + offset,mTextCount - offset,mStyle);
	mTextCount = offset;
	mNeedsLayout = true;
	mNeedsMinUsedWidth = true;
	mNeedsMaxUsedWidth = true;

	if (mStyle.blink) {
		DocAutolock lock(mHTMLDoc);
		mHTMLDoc->AddBlinkGlyph(secondHalf);
	}
	
	secondHalf->Layout(drawPort);	// Measure pieces
	SetWidth(GetWidth() - secondHalf->GetWidth());
	return secondHalf;
}


//	Merge this text glyph with previous one

void TextGlyph::RemoveSoftBreak(TextGlyph *secondHalf)
{
	if (secondHalf->mStyle.blink) {
		DocAutolock lock(mHTMLDoc);
		mHTMLDoc->RemoveBlinkGlyph(secondHalf);
	}
	mTextCount += secondHalf->mTextCount;
	mNeedsLayout = true;
	mNeedsMinUsedWidth = true;
	mNeedsMaxUsedWidth = true;
}

//	See if a start offset falls within this glyph, return index within glyph if it does

long TextGlyph::PoolToIndex(long start)
{
	if (start >= mTextOffset && start < (mTextOffset + mTextCount))
		return start - mTextOffset;
	return -1;
}

long TextGlyph::IndexToPool(long index)
{
	return mTextOffset + index;
}

//	Return the index of the character at position h

long TextGlyph::PixelToIndex(float h, DrawPort *drawPort)
{
	drawPort->SetStyle(mStyle);
	h -= GetLeft();
	if (h < 0) return 0;
	if (h >= GetWidth()) return mTextCount;
    
    char *t = GetText();
    float thisGap,lastGap = 0;
  
	int32 i = 0;
	while (i < mTextCount) {
		int32 theCharLen = UTF8CharLen(t[i]);
		i += theCharLen;
		if (i > mTextCount)
			return (-1);

		thisGap = drawPort->TextWidth(t, i);

		if (h <= thisGap) {
			if ((h - lastGap) <= (thisGap - h))	// Return closest charbounds
				return i - theCharLen;
			else
				return i;
		}
		lastGap = thisGap;
	}
	return -1;
}

//	Return a pointer to the glyphs text

char *TextGlyph::GetText()
{
	if (!mTextPool) return 0;
	
	return (char*)mTextPool->GetData() + mTextOffset;
}

//	Number of chars in glyph

long TextGlyph::GetTextCount()
{
	return mTextCount;
}

//	Return right pixel position of the indexed character

float TextGlyph::IndexToPixel(long index, DrawPort *drawPort)
{
	drawPort->SetStyle(mStyle);
	return GetLeft() + drawPort->TextWidth(GetText(),index);
}

//	Break text at a particular width

inline bool
CanEndLine(
	int32		offset,
	const char	*text,
	int32		textLen)
{
	uchar	theByte = text[offset];
	int32	charLen = UTF8CharLen(theByte);

	// sanity checking
	if ((offset + charLen) > textLen)
		return (TRUE);	// for the hell of it

	if (charLen == 1) {
		// 7-bit ASCII
		switch (theByte) {
			case '\0':
			case '\t':
			case '\n':
			case ' ':
			case '&':
			case '*':
			case '+':
			case '-':
			case '/':
			case '<':
			case '=':
			case '>':
			case '\\':
			case '^':
			case '|':
				return (TRUE);
			
			default:
				return (FALSE);
		}
	}

	uint32 theChar = UTF8CharToUint32((uchar *)text + offset, charLen);

	// can this character end a line?
	for (unsigned int i = 0; i < kNumTerminateTaboos; i++) {
		if (theChar == kTerminateTaboos[i])
			return (FALSE);
	}
	
	// can the next character start a line?
	int32 nextCharOffset = offset + charLen;
	if (nextCharOffset < textLen) {
		int32 nextCharLen = UTF8CharLen(text[nextCharOffset]);

		// sanity checking
		if ((nextCharOffset + nextCharLen) > textLen) 
			return (TRUE);	// for the hell of it
		else {
			uint32 theNextChar = UTF8CharToUint32((uchar *)text + nextCharOffset, nextCharLen);
			for (unsigned int i = 0; i < kNumBeginTaboos; i++) {
				if (theNextChar == kBeginTaboos[i])
					return (FALSE);
			}	
		}
	}

	// CJK symbols/puctuation, full/half-widths, hiragana, katakana, ideographs
	if ( ((theChar >= 0xE3808000) && (theChar <= 0xE380BF00)) ||
		 ((theChar >= 0xEFBC8000) && (theChar <= 0xEFBFAF00)) ||
		 ((theChar >= 0xE3818000) && (theChar <= 0xE3829F00)) ||
		 ((theChar >= 0xE382A000) && (theChar <= 0xE383BF00)) ||
		 ((theChar >= 0xE4B88000) && (theChar <= 0xE9BFBF00)) ||
		 ((theChar >= 0xEFA48000) && (theChar <= 0xEFABBF00)) ) 
		return (TRUE);

	return (FALSE);
}


long 
TextGlyph::BreakText(
	float			width,
	bool		atSpace,
	DrawPort	*drawPort)
{
	if (mStyle.pre || mNoBreak) 
		return (mTextCount);	// Don't wrap preformatted text

	drawPort->SetStyle(mStyle);

	char	*text = GetText();
	long	offset = 0;
	long	delta = 0;
	float	deltaWidth = 0;
	float	strWidth = 0;

	// wrap the text
	do {
		// find the next line break candidate
		for ( ; (offset + delta) < mTextCount ; delta++) {
			uchar theChar = text[offset + delta];
			if (IsInitialUTF8Byte(theChar)) {
				if (CanEndLine(offset + delta, text, mTextCount)) {
					delta += UTF8CharLen(theChar) - 1;
					break;
				}
			}
		}

		// add trailing spaces to delta
		for ( ; (offset + delta) < mTextCount; delta++) {
			uchar theChar = text[offset + delta];
			if (!IsInitialUTF8Byte(theChar)) 
				continue;

			if (!CanEndLine(offset + delta, text, mTextCount))
				break;
			
			// include all trailing spaces
			if (theChar != ' ')
				break;
		}
		delta = (delta < 1) ? 1 : delta;

		deltaWidth = drawPort->TextWidth(text + offset, delta);
		strWidth += deltaWidth;
		
		if (strWidth >= width)
			break;
		
		offset += delta;
		delta = 0;
	} while (offset < mTextCount);
	
	if (offset < 1) {
		offset = 0;

		if (!atSpace) {
			// there wasn't a single break in the line that fit, force a br
			strWidth = 0;

			while (offset < mTextCount) {
				int32 charLen = UTF8CharLen(text[offset]);

				deltaWidth = drawPort->TextWidth(text + offset, charLen);
				strWidth += deltaWidth;
			
				if (strWidth >= width) {
					strWidth -= deltaWidth;
					break;
				}
				
				offset += charLen;
			}

			offset = (offset < 1) ? UTF8CharLen(text[0]) : offset;
		}
	}

	offset = (offset < mTextCount) ? offset : mTextCount;

	return (offset);
}


// Measure and return the widest non-breakable chunk of text.

float
TextGlyph::GetMinUsedWidth(
	DrawPort	*drawPort)
{
	if (!mNeedsMinUsedWidth)
		return mOldMinUsedWidth;
	
	drawPort->SetStyle(mStyle);

	float		minWidth = 0;
	long	offset = 0;
	char*	text = GetText();

	do {
		long i = 0;
	
		if (mStyle.pre || mNoBreak)
			i = mTextCount;
		else {
			for (i = 0 ; (offset + i) < mTextCount ; i++) {
				uchar theChar = text[offset + i];
				if (IsInitialUTF8Byte(theChar)) {
					if (CanEndLine(offset + i, text, mTextCount)) {
						i += UTF8CharLen(theChar) - 1;
						break;
					}
				}
			}
			i = (i < 1) ? 1 : i;
		}

		minWidth = MAX(minWidth, 
					   drawPort->TextWidth(text + offset, i - offset));

		offset += i + 1;
	} while (offset < mTextCount);
	
	mNeedsMinUsedWidth = false;
	mOldMinUsedWidth = minWidth;

	return minWidth;
}

float TextGlyph::GetMaxUsedWidth(DrawPort *drawPort)
{
	if (!mNeedsMaxUsedWidth) {
		return mOldMaxUsedWidth;
	}
	drawPort->SetStyle(mStyle);

	mNeedsMaxUsedWidth = false;
	mOldMaxUsedWidth = drawPort->TextWidth(GetText(), mTextCount);
	return mOldMaxUsedWidth;
}

//	Layout for text just means measure

void TextGlyph::Layout(DrawPort *drawPort)
{
	if (!mNeedsLayout) {
		drawPort->SetStyle(mStyle);
		SetTop(-drawPort->GetFontAscent());
		return;
	}
	
	drawPort->SetStyle(mStyle);
	SetWidth(drawPort->TextWidth(GetText(),mTextCount));
	SetTop(-drawPort->GetFontAscent());
	SetHeight(drawPort->GetFontDescent() + drawPort->GetFontAscent());

	mNeedsLayout = false;
}

void TextGlyph::SpacingChanged()
{
	mNeedsMinUsedWidth = true;
	mNeedsMaxUsedWidth = true;
	mNeedsLayout = true;
}

//	Draw the text at its current position

void TextGlyph::Draw(DrawPort *drawPort)
{	
	long oldBGColor = drawPort->GetBackColor();
	if (!mHasBGImage && mHasBGColor)
		drawPort->SetBackColor(mBGColor);

	drawPort->SetStyle(mStyle);
	drawPort->DrawText(mLeft,mTop,GetText(),mTextCount, mWidth, mHasBGImage);
#ifdef DEBUGMENU
	if (modifiers() & B_CAPS_LOCK)
		Glyph::Draw(drawPort);				// Draw a border ¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥
#endif

	if (!mHasBGImage && mHasBGColor)
		drawPort->SetBackColor(oldBGColor);
}

void TextGlyph::SetVisited(bool visited)
{
	mStyle.visited = visited;
}

void TextGlyph::SetParent(Glyph *parent)
{
	SpatialGlyph::SetParent(parent);
	
	mHasBGColor = false;
	mHasBGImage = false;
	Glyph *currentGlyph = this;
	do {
		if (currentGlyph->IsDocument()) {
			DocumentGlyph *g = (DocumentGlyph *)currentGlyph;
			if (g->HasBGImage())
				mHasBGImage = true;
//			pprint("Found DocumentGlyph.  hasImageBG = %d", hasImageBG);
//		} else if (currentGlyph->IsTable()) {
//			TableGlyph *g = (TableGlyph *)currentGlyph;
		} else if (currentGlyph->IsCell()) {
			CellGlyph *g = (CellGlyph *)currentGlyph;
			if (!mHasBGColor && (mBGColor = g->GetBGColor()) != -1)
				mHasBGColor = true;
		}
		currentGlyph = currentGlyph->GetParent();
	} while (currentGlyph && !mHasBGImage && !mHasBGColor);
}
