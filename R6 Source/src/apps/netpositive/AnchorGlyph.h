// ===========================================================================
//	AnchorGlyph.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __ANCHORG__
#define __ANCHORG__

#include "Glyph.h"
#ifdef JAVASCRIPT
#include "jseopt.h"
#include "seall.h"
#include "HTMLDoc.h"
#endif

#include <String.h>

class ImageMap;
class ImageGlyph;

// ===========================================================================
// AnchorEndGlyph just turns things off at the end of an anchor
// it has a parent member

class AnchorEndGlyph: public Glyph {
public:
						AnchorEndGlyph();

		virtual	bool	IsAnchorEnd();

			Glyph*		GetParent();
			void		SetParent(Glyph *parent);

		virtual	void	LayoutComplete(DrawPort *drawPort);
protected:
		Glyph*			mParent;
//public:
//		void		*operator new(size_t size) {return PoolAlloc(sFreeList, ((sizeof(AnchorEndGlyph) + 3) / 4) * 4);}
//		void		operator delete(void *ptr, size_t size) {PoolFree(sFreeList, ptr);}
private:
static void*		sFreeList;
};

// ===========================================================================

enum {
	kShowTitle,
	kShowURL,
	kShowTitleUnspecified
};

#ifdef JAVASCRIPT
class AnchorGlyph : public AnchorEndGlyph, public ContainerUser
#else
class AnchorGlyph : public AnchorEndGlyph
#endif
{
public:
						AnchorGlyph(Document *document);
		virtual			~AnchorGlyph();

		virtual	bool	IsAnchor();

				bool	IsImageMap();
				bool	GetURL(BString& url, BString& target, int& showTitleInToolbar, ImageMap* imageMapList = NULL);
				void	SetURL(char *url);

		virtual	void	SetTop(float top);
		virtual	void	SetLeft(float left);
		virtual void	OffsetBy(const BPoint& offset);

		virtual	void	SetAttributeStr(long attributeID, const char* value);
		virtual	void	SetAttribute(long attributeID, long value, bool isPercentage);
		virtual	void	AddGlyph(Glyph *glyph);

		virtual	bool	Clicked(float h, float v);
		virtual	void	Hilite(long hilite, DrawPort *drawPort);
		bool			MouseEnter();
		void			MouseLeave();
		bool			ExecuteOnClickScript();

		virtual	float	GetTop();
		
		const char*		GetHREF();
		const char*		GetName();
		const char*		GetOnMouseOver();
		const char*		GetTarget();
		void			SetTarget(const char *target);
		void			SetHREF(const char *href);

				void	SetVisited(bool visited);

		virtual	void	Layout(DrawPort *drawPort);
		virtual	void	LayoutComplete(DrawPort *drawPort);
		void			GetAnchorBounds(BRect *rect);
		int				ShowTitleInToolbar() {return mShowTitleInToolbar;}
		
#ifdef JAVASCRIPT
				void	SetContainer(jseVariable container);
		struct BrowserLocation*	GetLocation() {return &mLocation;}
		Document*		GetDocument() {return mDocument;}
#endif

protected:
		BString			mHREF;
		BString			mName;
		BString			mTarget;
		BString			mOnMouseOverScript;
		BString			mOnMouseOutScript;
		BString			mOnClickScript;
		Document*		mDocument;
		
		BRegion*		mRegion;
		ImageGlyph*		mImageMap;
		float			mTop;		// For Fragments
		signed			mHilite : 16;
		unsigned		mVisited : 1;
		unsigned		mShowTitleInToolbar : 2;
#ifdef JAVASCRIPT
		jseVariable		mOnMouseOver;
		jseVariable		mOnMouseOut;
		jseVariable		mOnClick;
		struct BrowserLocation mLocation;
#endif
//public:
//		void		*operator new(size_t size) {return PoolAlloc(sFreeList, ((sizeof(AnchorGlyph) + 3) / 4) * 4);}
//		void		operator delete(void *ptr, size_t size) {PoolFree(sFreeList, ptr);}
private:
static void*		sFreeList;
};

#endif
