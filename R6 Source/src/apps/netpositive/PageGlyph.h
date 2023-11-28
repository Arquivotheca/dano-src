// ===========================================================================
//	PageGlyph.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __PAGEG__
#define __PAGEG__

#include "Glyph.h"

#include <String.h>
class BList;

class AnchorGlyph;

//=============================================================================
// PageGlyph has HTML style line layout

class PageGlyph : public CompositeGlyph {
public:
					PageGlyph(Document* htmlDoc);
	virtual			~PageGlyph();

#ifdef DEBUGMENU
	virtual	void	PrintStr(BString& print);
#endif
	
	virtual	void	AddChild(Glyph* glyph);
	virtual	bool	IsPage();

//	Preserving styles/alignment in nested tables

	virtual	int		GetListStackDepth();
	virtual	int		GetStyleStackDepth();
	virtual	int		GetAlignStackDepth();
	virtual	void	SetStack(int style,int align,int list,int leftMargin, int rightMargin);
	virtual	void	GetStack(int* style,int* align,int* list,int* leftMargin, int* rightMargin);

			void	Open();
			void	Close();
	virtual	bool ReadyForLayout();
			bool	LaidOutVisibleGlyph() {return mLaidOutVisibleGlyph;}

			void	SetLeftMarginDefault(float leftMarginDefault);
			void	SetRightMarginDefault(float rightMarginDefault);

			void	PrepareFloating();
			void	LayoutFloatingGlyph(Glyph *glyph);
			void	DeferFloatingLayout(Glyph *glyph);		// Defer layout until newline
			void	LayoutDeferedFloating();
			bool	HasCurrentFloater(float vPos, short align);

			float	LineBreak(float vPos, short breakType);	// Handle complex floating break
	
			void	GetMargins(float vPos, float *left, float *right);
			float	GetMarginWidth();
			float	GetDefaultMarginWidth();
			float	GetUsedWidth();
	virtual float	GetMinUsedWidth(DrawPort *drawPort);
	virtual float	GetMaxUsedWidth(DrawPort *drawPort);

	virtual	float	LayoutLineV(float vPos, float space, short align, Glyph *firstGlyph, Glyph *lastGlyph);
			void	PutLine(Glyph *lineEnd);
			Glyph	*PutGlyph(Glyph *glyph);
			float	SpaceLeftOnLine();

			void	RemoveSoftBreaks();

	virtual	void	SetCurrentAnchor(Glyph* anchorGlyph);
	virtual	void	SetupLayout(DrawPort *drawPort);
	virtual	void	Layout(DrawPort *drawPort);
	virtual	void	Draw(DrawPort *drawPort);
	virtual void	ResetLayout();

protected:
			AnchorGlyph*	mCurrentAnchor;
			BList		*mFloatingList;
			BList*		mDFloating;

			DrawPort*	 mDrawPort;
			float		mLeftMargin;
			float		mRightMargin;
			float		mLeftMarginDefault;
			float		mRightMarginDefault;

			float		mHPos;
			float		mVPos;

			float		mMaxHPos;

			Glyph*		mLineBegin;

			Glyph*		mNextToPut;
			Glyph*		mLastPut;
			Glyph*		mLastLayedOut;			
			
//			Stack locations of style paramaters for page glyphs

			float		mOldMinUsedWidth;
			float		mOldMaxUsedWidth;

			int			mStyleDepth : 16;
			int			mAlignDepth : 16;
			int			mListDepth : 16;
			int			mLeftMarginDepth : 16;
			int			mRightMarginDepth : 16;
			unsigned	mCachedReadyForLayout : 1;
			unsigned 	mOpen : 1;
			unsigned 	mLaidOutVisibleGlyph : 1;
};

// ===========================================================================
//	RuleGlyph Draws horizontal rules

class RuleGlyph : public CompositeGlyph {	// Because it needs mParent
public:
					RuleGlyph(Document* htmlDoc);
					
#ifdef DEBUGMENU
	virtual	void	PrintStr(BString& print);
#endif

	virtual	float	GetMinUsedWidth(DrawPort *drawPort);
	virtual	void	Layout(DrawPort *drawPort);
	virtual	void	Draw(DrawPort *drawPort);
			
			int		GetHAlign();

	virtual	void	SetAttribute(long attributeID, long value, bool isPercentage);
protected:
			short	mPercentageWidth;
			float	mKnownWidth;
			bool	mNoShade;
			int		mHAlign;
};

// ===========================================================================
//	MarginGlyph moves around margins on a newline
//	Does not have to descend from SpatialGlyph, just doing it for debugging

class MarginGlyph : public SpatialGlyph {
public:
					MarginGlyph(Document* htmlDoc);

#ifdef DEBUGMENU
	virtual	void	PrintStr(BString& print);
#endif

			Glyph*	GetParent();
			void	SetParent(Glyph *parent);

			void	SetLeftMargin(float margin);
			void	SetRightMargin(float margin);
	virtual void	Layout(DrawPort *drawPort);	// Layout sets parent glyph's alignment
			void	Draw(DrawPort *drawPort);	// Just f
virtual bool		IsMargin();
			float	GetLeftMargin();
			float	GetRightMargin();
protected:
			Glyph*	mParent;
			float	mLeftMargin;
			float	mRightMargin;
			bool	mBreakTypeValid;
//public:
//		void		*operator new(size_t size) {return PoolAlloc(sFreeList, ((sizeof(MarginGlyph) + 3) / 4) * 4);}
//		void		operator delete(void *ptr, size_t size) {PoolFree(sFreeList, ptr);}
private:
static void*		sFreeList;
};

// ===========================================================================
//	DDMarginGlyph
//	Don't break if the DT was not very wide

class DDMarginGlyph : public MarginGlyph {
public:
					DDMarginGlyph(Document* htmlDoc) : MarginGlyph(htmlDoc) {}
	virtual	short	GetBreakType();
};

// ===========================================================================
//	DotGlyph .. puts the dot left of the list items

class BulletGlyph : public SpatialGlyph {
public:
					BulletGlyph(Document* htmlDoc);
					
	bool			IsBullet();				// False

#ifdef DEBUGMENU
	virtual	void	PrintStr(BString& print);
#endif

	virtual	void	GetBounds(BRect* r);
	virtual	float	GetWidth();
	virtual	short	GetBreakType();
	virtual void	Layout(DrawPort *drawPort);	// Layout sets parent glyph's alignment

			void	GraphicBullet(DrawPort *drawPort, float left);
			void	TextBullet(DrawPort *drawPort, BString label, bool lowerCase);

			void	SetKind(uchar kind, int32 count);
			void	Draw(DrawPort *drawPort);
protected:
			int		mKind;
			int		mCount;
};

#endif
