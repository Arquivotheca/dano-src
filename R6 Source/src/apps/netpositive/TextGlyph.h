// ===========================================================================
//	TextGlyph.h
// 	Copyright 1995 by Peter Barrett, All rights reserved.
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#ifndef __TEXTGLYPH__
#define __TEXTGLYPH__

#include "Glyph.h"
#include <String.h>

// ===========================================================================
//	TextGlyph represents a line of text or a line break

class TextGlyph : public SpatialGlyph {
public:
					TextGlyph(Document* htmlDoc, bool noBreak);
	virtual			~TextGlyph();

#ifdef DEBUGMENU
	virtual	void	PrintStr(BString& print); 	// Add text to describe glyph
#endif

	virtual	bool	IsText();     
	virtual	short	GetAlign();

			char*	GetText();
			long 	GetTextCount();

			long	PoolToIndex(long start);
			long	IndexToPool(long index);

			long	PixelToIndex(float h, DrawPort *drawPort);
			float	IndexToPixel(long index, DrawPort *drawPort);

			void	SetText(CBucket *textPool,long textOffset, long textCount, Style& style);
			
			TextGlyph*	InsertSoftBreak(long offset,DrawPort *drawPort);
			void	RemoveSoftBreak(TextGlyph *secondHalf);
			
			long	BreakText(float width, bool atSpace, DrawPort *drawPort);
			float	GetMinUsedWidth(DrawPort *drawPort);
	virtual float	GetMaxUsedWidth(DrawPort *drawPort);

	virtual	bool	Separable();					// True if it ends with a space

			void	Layout(DrawPort *drawPort);
			void	Draw(DrawPort *drawPort);
	virtual void	SpacingChanged();

			void	SetVisited(bool visited);
	virtual	void	SetParent(Glyph *parent);

protected:
			CBucket	*mTextPool;
			long	mTextOffset;
			long	mTextCount;
			long	mBGColor;
			
			float	mOldMinUsedWidth;
			float	mOldMaxUsedWidth;

			unsigned	mHasBGColor : 1;
			unsigned	mHasBGImage : 1;
			unsigned	mNeedsLayout : 1;
			unsigned	mNeedsMinUsedWidth : 1;
			unsigned	mNeedsMaxUsedWidth : 1;
			unsigned	mNoBreak : 1;
			
//public:
//		void		*operator new(size_t size) {return PoolAlloc(sFreeList, ((sizeof(TextGlyph) + 3) / 4) * 4);}
//		void		operator delete(void *ptr, size_t size) {PoolFree(sFreeList, ptr);}
private:
static void*		sFreeList;
};

#endif
