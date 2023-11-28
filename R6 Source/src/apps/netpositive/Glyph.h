// ===========================================================================
//	Glyph.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __GLYPH__
#define __GLYPH__

#ifdef DEBUGMENU
#include <typeinfo>
#endif

#include "DrawPort.h"
#include "Utils.h"
#include <String.h>

class Document;

// ===========================================================================
//	Glyph base class

class Glyph : public CLinkable {
protected:
					Glyph();
public:
//	Break types

	enum BreakType {
		kNoBreak = 0,
		kPREBreak,
		kSoft,
		kHard,
		kParagraph,
		kHardLeft,
		kHardRight,
		kHardAll,
		kHardAlign
	};

	virtual			~Glyph();

#ifdef DEBUGMENU
	virtual	void	Print(int level);		// Dump debugging contents
	virtual	void	PrintStr(BString& print); 	// Add text to describe glyph
	const char*		BreakStr();
#endif
	
	virtual	void	GetBounds(BRect *r);					// Zero by default

	virtual	void	SetTop(float top);
	virtual	void	SetLeft(float left);
	virtual	void	SetWidth(float width);
	virtual	void	SetHeight(float height);
	virtual void	OffsetBy(const BPoint& offset);

	virtual float	GetTop();
	virtual float	GetLeft();
	virtual	float	GetWidth();
	virtual	float	GetHeight();
	virtual	float	GetMinUsedWidth(DrawPort *drawPort);	// Smallest Glyph width
	virtual float	GetMaxUsedWidth(DrawPort *drawPort);

	virtual	short	GetAlign();
	virtual	void	SetAlign(short align);
	virtual	void	SetPRE(bool isPre);
	virtual	void	SetStyle(Style style);
	virtual	const Style&	GetStyle();
	
//	Preserving styles in nested tables
		
	virtual	int		GetListStackDepth();
	virtual	int		GetAlignStackDepth();
	virtual	int		GetStyleStackDepth();
	virtual	void	SetStack(int style,int align,int list,int leftMargin, int rightMargin);
	virtual	void	GetStack(int* style,int* align,int* list,int* leftMargin, int* rightMargin);

//	Draw and Layout

	virtual	void	Draw(DrawPort *drawPort);
	virtual	void	Layout(DrawPort *drawPort);
	virtual	void	LayoutComplete(DrawPort *drawPort);
	virtual bool	IsLayoutComplete();
	virtual void	ResetLayout();
	virtual void	SpacingChanged();

//	Line break info

	virtual	bool	IsLineBreak();
			bool	IsExplicitLineBreak();
	virtual	void	SetBreakType(BreakType breakType);
	virtual	short	GetBreakType();
	
//	Identification

	virtual	bool	IsImage();						// False
	virtual	bool	IsText();						// False
	virtual	bool	IsRuler();						// False
	virtual	bool	IsPage();						// False
	virtual	bool	IsTable();						// False
	virtual	bool	IsCell();						// False
	virtual	bool	IsAnchor();						// False
	virtual	bool	IsAnchorEnd();					// False
	virtual	bool	IsSpatial();					// False
	virtual	bool	IsBullet();						// False
	virtual bool	IsDocument();					// False
	virtual bool	IsMargin();						// False
	virtual	bool	Floating();						// False
	virtual	bool	Separable();					// True
	virtual	bool	ReadyForLayout();				// True

	virtual	void	SetAttribute(long attributeID, long value, bool isPercentage);
	virtual	void	SetAttributeStr(long attributeID, const char* value);

	virtual	Glyph*	GetChildren();
	virtual	Glyph*	GetLastChild();
	virtual	void	AddChild(Glyph* child);
	virtual	void	AddChildAfter(Glyph* child,Glyph* afterThis);
	virtual	void	DeleteChild(Glyph* child);

	virtual	Glyph*	GetParent();
	virtual	void	SetParent(Glyph *parent);

	virtual	bool	Clicked(float h, float v);
	virtual	void	Hilite(long value, DrawPort *drawPort);
	virtual void	Commit(long value, DrawPort *drawPort);
	
	bool			DoesDrawing() {return mDoesDrawing;}
	bool			HasBounds() {return mHasBounds;}
	
#ifdef DEBUGMENU
	const char *GetClassName() {return typeid(*this).name();}
#endif

protected:
		BreakType	mBreakType : 4;
		unsigned	mDoesDrawing : 1;
		unsigned	mHasBounds : 1;
		Glyph*		mParent;
};

// ===========================================================================
//	SpatialGlyph has instance data for top, left, width, height

class SpatialGlyph : public Glyph {
public:
					SpatialGlyph(Document *htmlDoc);
	virtual			~SpatialGlyph();
	
	virtual	bool	IsSpatial();					// true
	virtual	void	SetStyle(Style style);
	virtual	void	SetPRE(bool isPre);
	virtual const	Style&	GetStyle();

	virtual	void	SetTop(float top);
	virtual	void	SetLeft(float left);
	virtual	void	SetWidth(float width);
	virtual	void	SetHeight(float height);
	virtual void	OffsetBy(const BPoint& offset);
	virtual void	ResetLayout();

	virtual float	GetTop();
	virtual float	GetLeft();
	virtual	float	GetWidth();
	virtual	float	GetHeight();

protected:
			float	mTop;	// Need to be signed for layout
			float	mLeft;
			float	mWidth;
			float	mHeight;
			
			Style	mStyle;
			Document*	mHTMLDoc;
};

// ===========================================================================
//	CompositeGlyph can contain other glyphs

class CompositeGlyph : public SpatialGlyph {
public:
					CompositeGlyph(Document* htmlDoc);
	virtual			~CompositeGlyph();

	virtual	void	SetTop(float top);
	virtual	void	SetLeft(float left);
	virtual void	OffsetBy(const BPoint& offset);

	virtual	void	SetAlign(short align);
	virtual	short	GetAlign();

	virtual	void	Draw(DrawPort *drawPort);
	virtual	void	Layout(DrawPort *drawPort);
	virtual void	ResetLayout();

	virtual	Glyph*	GetChildren();
	virtual	Glyph*	GetLastChild();
	virtual	void	AddChild(Glyph* child);
	virtual	void	AddChildAfter(Glyph* child,Glyph* afterThis);
	virtual	void	DeleteChild(Glyph* child);

	virtual	Glyph*	GetParent();
	virtual	void	SetParent(Glyph *parent);

	virtual	void	SetCurrentAnchor(Glyph* anchorGlyph);
	virtual void	SpacingChanged();

protected:
			CLinkedList	mChildren;
			int			mAlign;
};

// ===========================================================================
// Line breaks are fairly simple

class LineBreakGlyph : public Glyph {
public:
						LineBreakGlyph();
//public:
//		void		*operator new(size_t size) {return PoolAlloc(sFreeList, ((sizeof(LineBreakGlyph) + 3) / 4) * 4);}
//		void		operator delete(void *ptr, size_t size) {PoolFree(sFreeList, ptr);}
private:
static void*		sFreeList;
};

// ===========================================================================
//	AlignmentGlyph defines alignment of subsequent glyphs

class AlignmentGlyph : public LineBreakGlyph {
public:
					AlignmentGlyph();

#ifdef DEBUGMENU
virtual		void	PrintStr(BString& print);
#endif

			Glyph*	GetParent();
			void	SetParent(Glyph *parent);

			void	SetAlign(short align);
			void	Layout(DrawPort *drawPort);	// Layout sets parent glyph's alignment
protected:
			int		mAlign;
			Glyph*	mParent;
//public:
//		void		*operator new(size_t size) {return PoolAlloc(sFreeList, ((sizeof(AlignmentGlyph) + 3) / 4) * 4);}
//		void		operator delete(void *ptr, size_t size) {PoolFree(sFreeList, ptr);}
private:
static void*		sFreeList;
};


#endif
