// ===========================================================================
//	Select.h
//  Copyright 1998 by Be Incorporated.
// 	Copyrihgt 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __SELECT__
#define __SELECT__

#include <Region.h>

class DrawPort;
class Glyph;
class BList;

// ===========================================================================

class Selection  /*: public NPObject*/ {
public:
					Selection();
		virtual		~Selection();

		void		Draw(DrawPort *drawPort);
		bool		HasSelection();

		void		SelectText(long start, long length, DrawPort *drawPort, Glyph *rootGlyph);
		bool		GetSelectedText(long *start,long *length);
		float		GetSelectionTop();
		float		GetSelectionLeft();
		char*		GetFormattedSelection(long *length,Glyph *rootGlyph);
		
		bool		MouseDown(float h, float v, DrawPort *drawPort, Glyph *rootGlyph);
		void		MouseMove(float h, float v, DrawPort *drawPort, Glyph *rootGlyph);
		void		MouseUp(float h, float v, DrawPort *drawPort, Glyph *rootGlyph);

protected:

		void		SelectRect(float h, float v, DrawPort* drawPort);
		

		void		Deselect(DrawPort *drawPort);

		void		BuildGlyphList(Glyph *rootGlyph);
		Glyph		*GetGlyph(long index);

		Glyph*		FindGlyph(float h, float v);
		Glyph*		MinGlyph(Glyph *a, Glyph *b);
		Glyph*		MaxGlyph(Glyph *a, Glyph *b);

		float		GetPos(Glyph* g, float h, float v, long *index, DrawPort *drawPort);
		BRegion*	MakeRegion(Glyph *first,Glyph *last, float firstLeft,float lastRight);

		BRegion*	mRegion;
		Glyph*		mFirst;					// Resolved first glyph of selection
		Glyph*		mLast;					// last glyph of selection
		long		mFirstIndex;
		long		mLastIndex;

		float		mFirstH,mFirstV;

		float		mFirstGLeft;	// First glyph clicked in, first hp
		Glyph		*mFirstG;				// First glyph clicked in
		
		BList		*mGlyphList;			// Sorted, flat list of glyph used by iterator
};

#endif
