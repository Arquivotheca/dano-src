// ===========================================================================
//	Select.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Selection.h"
#include "DrawPort.h"
#include "Glyph.h"
#include "TextGlyph.h"
#include "BeDrawPort.h"

#include <malloc.h>
#include <List.h>
#include <stdio.h>

// ===========================================================================

Selection::Selection()
{
	mRegion = 0;
	mFirst = 0;
	mLast = 0;
	mGlyphList = 0;
	mFirstH = -1;
	mFirstV = -1;
}

Selection::~Selection()
{
	delete mRegion;
	delete mGlyphList;
}

//	Builds a list of glyphs that are 'flat', recurses inside tables and cells, etc

void AddGlyphs(Glyph *glyph, BList *list)
{
	Glyph *g;
	for (g = glyph->GetChildren(); g; g = (Glyph *)g->Next())	// Add children first
		AddGlyphs(g,list);
	list->AddItem(glyph);
}

void Selection::BuildGlyphList(Glyph *rootGlyph)
{
	if (mGlyphList)
		delete(mGlyphList);
	mGlyphList = new BList;
	AddGlyphs(rootGlyph,mGlyphList);
	//pprintBig(">%d glyphs in page",mGlyphList->GetCount());
}

//	return an indexed glyph

Glyph *Selection::GetGlyph(long index)
{
	return (Glyph *)mGlyphList->ItemAt(index);
}

//	Find a glyph that h and v point to if any

Glyph *Selection::FindGlyph(float h, float v)
{
	Glyph *g;
	long i = 0;

	for (g = GetGlyph(i); g; g = GetGlyph(++i))
		if (g->Clicked(h,v)) return g;
		
	return 0;
}

//	Min and Max according to there order of appearance in the list

Glyph *Selection::MinGlyph(Glyph *a, Glyph *b)
{
	if (a == b) return a;
	
	Glyph *g;
	long i = 0;
	for (g = GetGlyph(i); g; g = GetGlyph(++i)) {
		if (g == a) return a;
		if (g == b) return b;
	}
	return a;
}

Glyph *Selection::MaxGlyph(Glyph *a, Glyph *b)
{
	if (a == b) return a;
	
	Glyph *g;
	long i = mGlyphList->CountItems()-1;
	for (g = GetGlyph(i); i; g = GetGlyph(--i)) {
		if (g == a) return a;
		if (g == b) return b;
	}
	return a;
}

//	Create a region between the first and last glyph

bool CleanupRect(BRect& r);

// Adding new rectangles to a BRegion can be slow if the region gets
// very large.  Let's look for opportunities to coalesce multiple adds.
// For example, if you try to add these two rectangles to a region:
// +----------+----------+
// +  Rect 1  |  Rect 2  |
// +----------+----------+
// or this:
// +----------+
// |  Rect 1  |
// +----------+
// |  Rect 2  |
// +----------+
// coalesce them into a single rectangle and add as one.  We do this by always
// keeping track of the previous rectangle that we wanted to add to the region.
// When comparing it to the next one, if it fits one of the two cases above, we
// just add the new rectangle to the previous and remember the union for the next
// time around.  If it doesn't fit, we do the actual add of the previous rectangle
// and remember just the new one for the next pass.

void FastInclude(BRegion *r, const BRect& rr, BRect& prevRR)
{
	if (prevRR.Height() == 0 && prevRR.Width() == 0) {
		prevRR = rr;
	} else if (rr.top == prevRR.top && rr.bottom == prevRR.bottom && rr.left - prevRR.right <= 1) {
		prevRR.right = rr.right;
	} else if (rr.left == prevRR.left && rr.right == prevRR.right && rr.top - prevRR.bottom <= 1) {
		prevRR.bottom = rr.bottom;
	} else {
		r->Include(prevRR);
		prevRR = rr;
	}
}

BRegion *Selection::MakeRegion(Glyph *first,Glyph *last, float firstLeft,float lastRight)
{
	BRegion *r = new BRegion;
	float top,left,bottom,right;
	BRect rr;
	BRect prevRR(0,0,0,0);
	
	top = left = 1e+300;
	bottom = right = -1e+300;

	Glyph *g = first;

	long i = 0;
	while ((bool)(g = GetGlyph(i))) {
		if (g == first) break;
		i++;
	}
		
	for (; g; g = GetGlyph(++i)) {
		if (g->IsText()) {
			if (g->IsLineBreak()) {
				if (top < 1e+299) {
					rr.Set(left,top + 1,right,bottom);
					if (CleanupRect(rr)) {
						FastInclude(r, rr, prevRR);
					}
				}
				top = left = 1e+300;
				bottom = right = -1e+300;
			}
			
			g->GetBounds(&rr);
			top = MIN(top,rr.top);
			if (g == first)
				left = firstLeft;
			else
				left = MIN(left,rr.left);
			bottom = MAX(bottom,rr.bottom);
			if (g == last)
				right = lastRight;
			else
				right = MAX(right,rr.right);
		} else {
			if (top < 1e+299) {
				rr.Set(left,top + 1,right,bottom);
				if (CleanupRect(rr)) {
					FastInclude(r, rr, prevRR);
				}
			}
			top = left = 1e+300;
			bottom = right = -1e+300;
		}

		if (g == last) break;
	}
	if (top < 1e+299) {
		rr.Set(left,top + 1,right,bottom);
		if (CleanupRect(rr)) {
			FastInclude(r, rr, prevRR);
		}
	}
	if (prevRR.Width() > 0 && prevRR.Height() > 0)
		r->Include(prevRR);
	return r;
}

//	Cancel any current selection

void Selection::Deselect(DrawPort *drawPort)
{
	drawPort->BeginDrawing(NULL);
	if (mRegion) {
		drawPort->InvertRgn(mRegion);
		delete(mRegion);
		mRegion = 0;
		mFirst = mLast = 0;
	}	
}

//	Create a new selection from a text range

void Selection::SelectText(long start, long length, DrawPort *drawPort, Glyph *rootGlyph)
{
	Deselect(drawPort);
	BuildGlyphList(rootGlyph);
	
//	Locate the first and last glyphs in the selection

	Glyph *g;
	long i = 0;
	for (g = GetGlyph(i); g; g = GetGlyph(++i)) {
		if (g->IsText()) {
			mFirstIndex = ((TextGlyph *)g)->PoolToIndex(start);
			if (mFirstIndex != -1) {
				mFirst = g;		// Start of selection
				break;
			}
		}
	}
	for (g = GetGlyph(i); g; g = GetGlyph(++i)) {
		if (g->IsText()) {
			mLastIndex = ((TextGlyph *)g)->PoolToIndex(start + length);
			if (mLastIndex != -1) {
				mLast = g;		// End of selection
				break;
			}
		}
	}

	if (!(mFirst && mLast))		// Failed to make a selection
		return;
		
	mRegion = MakeRegion(mFirst,mLast,
		((TextGlyph *)mFirst)->IndexToPixel(mFirstIndex,drawPort),
		((TextGlyph *)mLast)->IndexToPixel(mLastIndex,drawPort));
		drawPort->InvertRgn(mRegion);
}

//	Return the offsets of the selected text

bool	Selection::GetSelectedText(long *start,long *length)
{
	if (!mRegion) return false;
	*start = ((TextGlyph *)mFirst)->IndexToPool(mFirstIndex);
	*length = ((TextGlyph *)mLast)->IndexToPool(mLastIndex) - *start;
//	NP_ASSERT(*length >= 0);
	return true;
}

//	Get a text selection complete with line breaks

char* Selection::GetFormattedSelection(long *length, Glyph *rootGlyph)
{
	if (!mRegion)
		return NULL;
	BuildGlyphList(rootGlyph);
	
	CBucket bucket;
	Glyph *g;
	long i = 0;
	while ((bool)(g = GetGlyph(i))) {
		if (g == mFirst) break;
		i++;
	}
	
//	char cr = 0x0D;
	const char cr = 0x0A;
	bool noCR = true;
	for (; g; g = GetGlyph(++i)) {
		if (noCR == false && mFirst != mLast)
			if (g->IsLineBreak())
				bucket.AddData(&cr,1);
		noCR = false;

		if (g->IsText()) {
			TextGlyph *t = (TextGlyph *)g;
			char *text = t->GetText();
			long count = t->GetTextCount();
			
			if (g == mFirst) {		// Adjust range if in first or last glyph
				text += mFirstIndex;
				count -= mFirstIndex;
				if (g == mLast)
					count = mLastIndex - mFirstIndex;
			} else if (g == mLast)
				count = mLastIndex;
								
			if (count > 0)
				bucket.AddData(text,count);
		}
		if (g == mLast) break;
	}

//	Sleasy... make a copy of selected data

	char *data = NULL;
	*length = bucket.GetCount();
	if (*length)
		if ((bool)(data = (char *)malloc(*length)))
			memcpy(data,bucket.GetData(),*length);
	return data;
}

float Selection::GetSelectionTop()
{
	if (!mRegion) return -1;
	BRect r = mRegion->Frame();
	return r.top;
}


float Selection::GetSelectionLeft()
{
	if (!mRegion) return -1;
	BRect r = mRegion->Frame();
	return r.left;
}


//	Returns character edge pixel within text glyphs

float Selection::GetPos(Glyph* g, float h, float, long *index, DrawPort *drawPort)
{
	if (!g->IsText())
		return g->GetLeft();
	*index = ((TextGlyph *)g)->PixelToIndex(h,drawPort);
	return ((TextGlyph *)g)->IndexToPixel(*index,drawPort);
}

//	Track a selection

bool Selection::MouseDown(float h, float v, DrawPort *drawPort, Glyph *rootGlyph)
{
	Deselect(drawPort);
	BuildGlyphList(rootGlyph);
	mFirstH = h;
	mFirstV = v;
	mFirst = NULL;
	mLast = NULL;
	return true;
}

//	Select glyphs that fall in the rectangle

void Selection::SelectRect(float h, float v, DrawPort* drawPort)
{
	BRect r;
	Glyph *g;
	long i = 0;
	float firstH;
	float lastH;
		
	bool sameLine = (FindGlyph(mFirstH, v) == FindGlyph(mFirstH, mFirstV));
	bool regularDrag;
	if (sameLine)
		regularDrag = (h > mFirstH);
	else
		regularDrag = (v > mFirstV);

//	Locate the first and last glyphs in the selection
	float beginH = regularDrag ? mFirstH : h;
	float beginV = MIN(v, mFirstV);
	
	if (mFirst && mFirst->Clicked(beginH, beginV))	// Same as last time
		g = mFirst;
	else 
		g = FindGlyph(beginH, beginV);
	
	if (g && g->IsText()) {
		mFirst = g;				// Clicked right in it
		firstH = GetPos(g,beginH,beginV,&mFirstIndex,drawPort);
	} else {
		for (g = GetGlyph(i); g; g = GetGlyph(++i)) {	// Locate the first glyph
			if (g->IsText()) {
				g->GetBounds(&r);
				if (r.top >= beginV && r.bottom < beginV) {	// First one that intersects vertically
					mFirst = g;
					break;
				}
			}
		}
		if (mFirst == NULL) {
			mFirstH = -1;
			mFirstV = -1;
			return;
		}
		firstH = mFirst->GetLeft();
		mFirstIndex = 0;
	}

//	Locate the last glyph
	float endH = regularDrag ? h : mFirstH;
	float endV = MAX(v, mFirstV);

	if (mLast && mLast->Clicked(endH, endV))	// Same as last time
		g = mLast;
	else
		g = FindGlyph(endH, endV);

	if (g && g->IsText()) {
		mLast = g;				// Clicked right in it
		lastH = GetPos(g,endH,endV,&mLastIndex,drawPort);	// Clicked right in it
	} else {
		for (g = GetGlyph(i); g; g = GetGlyph(++i)) {				// Locate the last glyph
			if (g->IsText()) {
				g->GetBounds(&r);
				if (r.top >= endV)		// Last one
					break;
				mLast = g;
			}
		}
		if (mLast == NULL)
			mLast = mFirst;
		lastH = mLast->GetLeft() + mLast->GetWidth();
		mLastIndex = ((TextGlyph*)mLast)->GetTextCount();
	}
	
	if (mFirst == NULL || mLast == NULL) {
		mFirstH = -1;
		mFirstV = -1;
		return;
	}

//	Figure left and right clipping
	if (mFirst == mLast) {
		if (firstH > lastH) {
			float t = firstH;
			firstH = lastH;
			lastH = t;
		}
	}
	
//	Hilite that region
	BRegion* oldR = mRegion;
	
//	Do some sanity checking on the first and last (existance and order)	
	if(mFirst == NULL || mLast == NULL)
		return;
	if(mGlyphList->IndexOf(mFirst) > mGlyphList->IndexOf(mLast)){
		Glyph *tempG = mFirst;
		mFirst = mLast;
		mLast = tempG;
		if (firstH > lastH) {
			float t = firstH;
			firstH = lastH;
			lastH = t;
		}
	}
	
	mRegion = MakeRegion(mFirst,mLast,firstH,lastH);
	if (oldR) {
		BRegion* diff = new BRegion(*mRegion);
		diff->Include(oldR);
		BRegion tmp(*mRegion);
		tmp.IntersectWith(oldR);
		diff->Exclude(&tmp);
		
		drawPort->InvertRgn(diff);
		delete(diff);
	} else {
		drawPort->InvertRgn(mRegion);
	}
	if (oldR)
		delete oldR;
}

//	Drag Mouse
		
void Selection::MouseMove(float h, float v,DrawPort *drawPort, Glyph *)
{
	mFirstH = (mFirstH < 0) ? h : mFirstH;
	mFirstV = (mFirstV < 0) ? v : mFirstV;

	//BRect rr(MIN(mFirstH,h),MIN(mFirstV,v),MAX(mFirstH,h),MAX(mFirstV,v));

	SelectRect(h, v, drawPort);
}

//	Done with the selection

void Selection::MouseUp(float, float, DrawPort *, Glyph *)
{
	mFirstG = 0;
}

void Selection::Draw(DrawPort *drawPort)
{
	if (mRegion)
		drawPort->InvertRgn(mRegion);
}

bool Selection::HasSelection()
{
	return mRegion != 0;
}
