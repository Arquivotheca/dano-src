// ===========================================================================
//	TableGlyph.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __TABLEG__
#define __TABLEG__

#include "PageGlyph.h"
#include <String.h>

// ===========================================================================

class CellGlyph : public PageGlyph {
public:
					CellGlyph(Document* htmlDoc);
	
#ifdef DEBUGMENU
	virtual	void	PrintStr(BString& print);
#endif
	
	virtual	bool	IsCell();
	virtual	void	SetAttribute(long attributeID, long value, bool isPercentage);
	

			short	GetRow() {return mRow;}
			short	GetCol() {return mCol;}
			short	GetRowSpan() {return mRowSpan;}
			short	GetColSpan() {return mColSpan;}
			void	SetColSpan(short span);
			void	SetRowSpan(short span);
			void	SetRowAndCol(short row,short col);
			
			float	GetKnownWidth();
			void	SetKnownWidth(float width, bool isPercentage);
			bool	IsKnownWidthPercentage();
			float		GetKnownHeight();
			bool	IsKnownHeightPercentage();
			
			long	GetBGColor();
			void	SetBGColor(long color);

			void	VAlignCell(float height);
			void	DrawInfo(DrawPort *drawPort);
	virtual void	ResetLayout();
			bool	IsNoWrap() {return mNoWrap;}

protected:
			float	mKnownWidth;
			float	mKnownHeight;
	
			int32	mBGColor;
			
			int		mRow : 16;
			int		mCol : 16;
			int		mRowSpan : 16;
			int		mColSpan : 16;
			int		mVAlign : 16;
			unsigned mNoWrap : 1;
			unsigned mIsWidthPercentage : 1;
			unsigned mIsHeightPercentage : 1;
	//public:
//		void		*operator new(size_t size) {return PoolAlloc(sFreeList, ((sizeof(CellGlyph) + 3) / 4) * 4);}
//		void		operator delete(void *ptr, size_t size) {PoolFree(sFreeList, ptr);}
private:
static void*		sFreeList;
};

// ===========================================================================

class TableGlyph : public PageGlyph {
public:
						TableGlyph(Document* htmlDoc);
	virtual				~TableGlyph();
	
#ifdef DEBUGMENU
	virtual	void		PrintStr(BString& print);
#endif

	virtual	bool		IsTable();
	virtual	bool		IsInCell();
	virtual	bool		IsInRow();
	virtual bool		Floating();
	virtual bool		IsLayoutComplete();
	virtual void		LayoutComplete(DrawPort *drawPort);
	
	virtual	float	 	GetWidth();
	virtual	float		GetMinUsedWidth(DrawPort *drawPort);
	virtual float		GetMaxUsedWidth(DrawPort *drawPort);
	
	virtual	void 		SetAttribute(long attributeID, long value, bool isPercentage);

			long		GetBGColor() {return mBGColor;}
		
			TableGlyph*	GetParentTable();
			void		SetParentTable(TableGlyph *parentTable);

			void		OpenTable();
			void		OpenRow();
			CellGlyph*	NewCell(bool isHeading);
			void		OpenCell(CellGlyph *cell, bool isHeading);
			void		CloseCell();
			void		CloseRow();
			void		CloseTable();

			void		OpenCaption(CellGlyph *caption);
			void		CloseCaption();

			float			GetMaxCelWidth();
			void		LayoutCaption(DrawPort *drawPort);
			void		LayoutRows(DrawPort *drawPort);
			void		LayoutCols(DrawPort *drawPort, bool layoutMinMaxOnly = false);
	virtual void		ResetLayout();

	virtual	void		Layout(DrawPort *drawPort);
	virtual	void		Draw(DrawPort *drawPort);
	virtual	void		AddChild(Glyph* child);
			void		DetermineWidth();

protected:
			void		DrawInfo(DrawPort *drawPort);
			void		SaveAttributes();
			void		RestoreAttributes();
			
			TableGlyph	*mParentTable;
			CellGlyph	*mInCell;
			CellGlyph	*mCaption;
	
			float		mKnownWidth;
			float		mMinUsedWidth;
			float		mMaxUsedWidth;
					
			float		*mOldRowHeights;
			float 		mOldKnownWidth;

			int32		mBGColor;
			int32		mOldBGColor;
	
			int			mRows : 16;
			int			mColumns : 16;
			int			mCurrentCol : 16;
			int			mCellSpacing : 16;
			int			mCellPadding : 16;
			int			mCellBorder : 16;
			int			mBorder : 16;
			int			mVAlign : 16;
			int			mCaptionAlign : 16;
			int			mRowAlign : 16;		
			int			mOldBorder : 16;
			int			mOldCellBorder : 16;
			int			mOldCellSpacing : 16;
			int			mOldCellPadding : 16;
			int	  		mOldAlign : 16;
			int			mOldVAlign : 16;
			unsigned 	mOldKnownPercentage : 1;
			unsigned	mLayoutComplete : 1;
			unsigned	mIsWidthPercentage : 1;
			unsigned	mInRow : 1;
};

#endif
