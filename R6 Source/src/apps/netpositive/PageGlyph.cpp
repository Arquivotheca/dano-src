// ===========================================================================
//	PageGlyph.cpp
// ===========================================================================
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "PageGlyph.h"
#include "HTMLTags.h"
#include "ImageGlyph.h"
#include "AnchorGlyph.h"
#include "TextGlyph.h"
#include "TableGlyph.h"
#include "BeDrawPort.h"
#include "MessageWindow.h"
#include "Utils.h"

#include <Window.h>
#include <stdio.h>
//====================================================================================
//	As a line is being layed out vertically, baseline is at zero.
//	Top is set to mark ascent, top + height defines descent
//	lastGlyph may be a line break or zero
//	Text has already been measured, top is set to - ascent
//	All glyphs have been positioned horizontally

#define kDebugFrames 0
#define kDetails 0

void *MarginGlyph::sFreeList;

PageGlyph::PageGlyph(Document* htmlDoc) :
	CompositeGlyph(htmlDoc)
{
	mAlign = AV_LEFT;

	mFloatingList = NULL;
	mDFloating = NULL;
	
	mCurrentAnchor = 0;
	mLeftMarginDefault = 0;
	mRightMarginDefault = 0;
	mMaxHPos = 0;
	mLineBegin = 0;

	mNextToPut = 0;
	mLastPut = 0;
	mLastLayedOut = 0;
	mOpen = false;
	
	mOldMinUsedWidth = -1;
	mOldMaxUsedWidth = -1;
	
	mRightMargin = 0;
	mLeftMargin = 0;
	mLaidOutVisibleGlyph = false;
	mCachedReadyForLayout = false;
	
	mStyleDepth = mAlignDepth = mListDepth = mLeftMarginDepth = mRightMarginDepth = 0;
}

PageGlyph::~PageGlyph()
{
	delete(mFloatingList);
	delete(mDFloating);
	delete(mDFloating);
}

#ifdef DEBUGMENU
void PageGlyph::PrintStr(BString& print)
{
	CompositeGlyph::PrintStr(print);
	print += " ";
	print += AttributeValueName(mAlign);
}
#endif

//	Remember floating images for later

void PageGlyph::AddChild(Glyph* glyph)
{
	if (glyph->Floating()) {
		if (mFloatingList == NULL)
			mFloatingList = new BList;
		mFloatingList->AddItem(glyph);
	}
#if kDetails
	BString str;
	glyph->PrintStr(str);
	pprint("Adding %s",(char*)str);
#endif
	CompositeGlyph::AddChild(glyph);
}

int	 PageGlyph::GetListStackDepth()
{
	return mListDepth;
}

int	 PageGlyph::GetAlignStackDepth()
{
	return mAlignDepth;
}

int	 PageGlyph::GetStyleStackDepth()
{
	return mStyleDepth;
}

void PageGlyph::SetStack(int style,int align,int list,int leftMargin, int rightMargin)
{
	mStyleDepth = style;
	mAlignDepth = align;
	mListDepth = list;
	mLeftMarginDepth = leftMargin;
	mRightMarginDepth = rightMargin;
}

void PageGlyph::GetStack(int* style,int* align,int* list,int* leftMargin, int* rightMargin)
{
	*style = mStyleDepth;
	*align = mAlignDepth;
	*list = mListDepth;
	*leftMargin = mLeftMarginDepth;
	*rightMargin = mRightMarginDepth;
}

bool	PageGlyph::IsPage()
{
	return true;
}

//	If a page is open, glyphs may still be coming in ...

void PageGlyph::Open()
{
	mOpen = true;
}

//	If it's closed, layout can finalize

void PageGlyph::Close()
{
	mOpen = false;
}

bool PageGlyph::ReadyForLayout()
{
	if (mCachedReadyForLayout) return true;		// Once ready, always ready
	if (mOpen) return false;	// Not ready, still open
	for (Glyph *childGlyph = GetChildren(); childGlyph; childGlyph = (Glyph *)childGlyph->Next())	// All the children must be ready too
		if (!childGlyph->ReadyForLayout())
			return false;
	mCachedReadyForLayout = true;
	return true;
}

void PageGlyph::SetLeftMarginDefault(float leftMarginDefault)
{
	// If margin is 0, no indent, just follow floaters.
	if (leftMarginDefault == 0)
		mLeftMarginDefault = 0;
	else {
		// First calculate margin due just to floaters.
		mLeftMarginDefault = 0;
		GetMargins(mVPos, &mLeftMargin, &mRightMargin);
		
		// Now set new default margin relative to that.
		mLeftMarginDefault = mLeftMargin - mLeft + leftMarginDefault;
	}

	GetMargins(mVPos,&mLeftMargin,&mRightMargin);
	mHPos = mLeftMargin;
}

void PageGlyph::SetRightMarginDefault(float rightMarginDefault)
{
	// If margin is 0, no indent, just follow floaters.
	if (rightMarginDefault == 0)
		mRightMarginDefault = 0;
	else {
		// First calculate margin due just to floaters.
		mRightMarginDefault = 0;
		GetMargins(mVPos, &mLeftMargin, &mRightMargin);
		
		// Now set new default margin relative to that.
		mRightMarginDefault =  GetWidth() + mLeftMargin - mLeftMarginDefault - mRightMargin + rightMarginDefault;
	}

	GetMargins(mVPos,&mLeftMargin,&mRightMargin);
	mHPos = mLeftMargin;
}

//	Ignore Floating glyphs until their Layout is Complete

void PageGlyph::PrepareFloating()
{
	if (!mFloatingList) return;
	for (short i = 0; i < mFloatingList->CountItems(); i++) {
		Glyph *g = (Glyph *)mFloatingList->ItemAt(i);
		g->Layout(mDrawPort);
	}
}

//	Floating images are layed out relative to margins

void PageGlyph::LayoutFloatingGlyph(Glyph *glyph)
{
	switch (glyph->GetAlign()) {
		case AV_LEFT:
			glyph->SetTop(mVPos);
			glyph->SetLeft(mLeftMargin);
			#if kDebugFrames
			BRect r;
			r.Set(mLeftMargin,mVPos,mLeftMargin + glyph->GetWidth(),mVPos + glyph->GetHeight());
			mDrawPort->SetColor(0,0xFF,0);
			mDrawPort->FrameRect(&r);
			mDrawPort->SetGray(0);
			#endif
			break;
		case AV_RIGHT:
			glyph->SetTop(mVPos);
			glyph->SetLeft(mRightMargin - glyph->GetWidth());			
			#if kDebugFrames
			r.Set(mRightMargin - glyph->GetWidth(),mVPos,mRightMargin,mVPos + glyph->GetHeight());
			mDrawPort->SetColor(0,0xFF,0);
			mDrawPort->FrameRect(&r);
			mDrawPort->SetGray(0);
			#endif
			break;
	}
			
	glyph->LayoutComplete(mDrawPort);		// NOT YET! It may get moved still!
	mLastLayedOut = glyph;					// Last item layed out
}

void PageGlyph::DeferFloatingLayout(Glyph *glyph)		// Defer layout until newline
{
	if (mDFloating == NULL) {
		mDFloating = new BList;
	}
	mDFloating->AddItem(glyph);
}

void PageGlyph::LayoutDeferedFloating()
{
	if (mDFloating && mDFloating->CountItems()) {
		for (short i = 0; i < mDFloating->CountItems(); i++) {
			
			Glyph *glyph = (Glyph *)mDFloating->ItemAt(i);

			LayoutFloatingGlyph(glyph);							

			GetMargins(mVPos,&mLeftMargin,&mRightMargin);
			if (mHPos < mLeftMargin)
				mHPos = mLeftMargin;
		}
				
		delete(mDFloating);
		mDFloating = NULL;
	}
}

//	Handle a line break .. may be a clear break

float PageGlyph::LineBreak(float vPos, short breakType)
{
	if (breakType < kHardLeft)				// Clear break
		return 0;
			
	if (GetMarginWidth() >= GetDefaultMarginWidth())	// Already at left margin
		return 0;
		
	if (mFloatingList == NULL || mFloatingList->CountItems() == 0)	// Only floaters left or right margins
		return 0;	

	float leftBottom = -1;
	float rightBottom = -1;
	for (short i = 0; i < mFloatingList->CountItems(); i++) {
		BRect r;
		Glyph *g = (Glyph *)mFloatingList->ItemAt(i);
		if (g->IsLayoutComplete()) {
			g->GetBounds(&r);
			if (vPos >= r.top && vPos < r.bottom) {
				if (g->GetAlign() == AV_LEFT)
					leftBottom = MAX(leftBottom,r.bottom);
				else
					rightBottom = MAX(rightBottom,r.bottom);
			}
		}
	}
	if (leftBottom == -1 && rightBottom == -1)
		return 0;

//	Break left, right or both

	switch (breakType) {
		case kHardLeft:
			if (leftBottom != -1)
				return leftBottom - vPos;
			break;
		case kHardRight:
			if (rightBottom != -1)
				return rightBottom - vPos;
			break;
		case kHardAll:
			return MAX(leftBottom,rightBottom) - vPos;
	}
	return 0;
}

//	Get left and right margins at current VPos

void PageGlyph::GetMargins(float vPos, float *left, float *right)
{
	*left = mLeft + mLeftMarginDefault;
	*right = mLeft + GetWidth() - mRightMarginDefault;
	
	if (mFloatingList == NULL || mFloatingList->CountItems() == 0)
		return;	// Only these can alter left or right margins

	for (int i = 0; i < mFloatingList->CountItems(); i++) {
		BRect r;
		Glyph *g = (Glyph *)mFloatingList->ItemAt(i);
		if (g->IsLayoutComplete()) {
			g->GetBounds(&r);
			if (vPos >= r.top && vPos < r.bottom) {
				if (g->GetAlign() == AV_LEFT)
					*left = MAX(*left,r.right);
				else
					*right = MIN(*right,r.left);
			}
		}
	}
	
	// Constrain margins to reasonable size.
	
	if ((mLeft + mWidth - *left) < 20)
		*left = MAX(mLeft, mLeft + mWidth - 20);
	if ((*right - *left) < 20)
		*right = MIN(*left + 20, mLeft + mWidth);
}

//	Width between left and right margins

float PageGlyph::GetMarginWidth()
{
	return mRightMargin - mLeftMargin;
}

// Width between left and right default margins

float PageGlyph::GetDefaultMarginWidth()
{
	return mWidth - mLeftMarginDefault - mRightMarginDefault;
}

//	Amout of width used in the draw
//	Get max used with

float PageGlyph::GetUsedWidth()
{
	float floatingWidth = 0;	// Floating images don't show up in mMaxHPos, make sure they are recognized...
	if (mFloatingList)
		for (short i = 0; i < mFloatingList->CountItems(); i++) {
			Glyph *g = (Glyph *)mFloatingList->ItemAt(i);
			floatingWidth = MAX(floatingWidth,g->GetWidth());
		}

	return MAX(floatingWidth,mMaxHPos - mLeft);
}

// Compute the minimum width by finding the longest string of non-separable glyphs.

float PageGlyph::GetMinUsedWidth(DrawPort *drawPort)
{
	if (mOldMinUsedWidth >= 0)
		return mOldMinUsedWidth;
		
	float minWidth = 0;
	float runWidth;
	float marginWidth = 0;

	Glyph* g = GetChildren();
	while (g) {
		runWidth = g->GetMinUsedWidth(drawPort);
		if (g->IsMargin()) {
			MarginGlyph *mg = (MarginGlyph *)g;
			marginWidth = (mg->GetLeftMargin());
		}
		while (g->Next() != NULL && !((Glyph*)g->Next())->Separable()) {
			g = (Glyph*)g->Next();
			runWidth += g->GetMinUsedWidth(drawPort);	// If next is text, it may return a min value for a larger word
														// than the one that is adjacent, and cannot be separated.
			if (g->IsMargin()) {
				MarginGlyph *mg = (MarginGlyph *)g;
				marginWidth = (mg->GetLeftMargin());
				}
		}
		runWidth += marginWidth;
		minWidth = MAX(runWidth, minWidth);
		g = (Glyph*)g->Next();
	}
	mOldMinUsedWidth = minWidth;	
	return minWidth;
}


float PageGlyph::GetMaxUsedWidth(DrawPort *drawPort)
{
	if (mOldMaxUsedWidth >= 0)
		return mOldMaxUsedWidth;
		
	float maxWidth = 0;
	float runWidth;
	float marginWidth = 0;
	
	Glyph *g = GetChildren();
	while (g) {
		runWidth = 0;
		if (g->IsMargin()) {
			MarginGlyph *mg = (MarginGlyph *)g;
			marginWidth = (mg->GetLeftMargin());
		}
		while (g != NULL) {
			if(runWidth != 0 && g->GetBreakType() == kHard){
				g = (Glyph *)g->Previous();
				break;
			}
			runWidth += g->GetMaxUsedWidth(drawPort);

			if (g->IsExplicitLineBreak())
				break;

			g = (Glyph *)g->Next();
			if (g && g->IsMargin()) {
				MarginGlyph *mg = (MarginGlyph *)g;
				marginWidth = (mg->GetLeftMargin());
			}
		}

		runWidth += marginWidth;
		maxWidth = MAX(runWidth, maxWidth);
		if (g)
			g = (Glyph *)g->Next();
	}
	mOldMaxUsedWidth = maxWidth;
	return maxWidth;
}

void PageGlyph::ResetLayout()
{
	mOldMaxUsedWidth = -1;
	mOldMinUsedWidth = -1;
	CompositeGlyph::ResetLayout();
	mLaidOutVisibleGlyph = false;
}


//	Calculate vertical positions of glyphs that have been arranged horizontally
//	Note: LineBreaks START a line, not finish it

float PageGlyph::LayoutLineV(float vPos, float space, short align, Glyph *firstGlyph, Glyph *lastGlyph)
{
	float	maxAscent = 0;
	float	maxDescent = 0;
	float	height,ascent,hOffset;
	Glyph	*glyph;

	if (!firstGlyph)
		return vPos;
#if kDetails
	pprint("================LayoutLineV================");
	for (glyph = firstGlyph; glyph; glyph = (Glyph *)glyph->Next()) {
		BString str;
		glyph->PrintStr(str);
		pprint(str);
		if (glyph == lastGlyph)
			break;
	}
#endif

//	Figure the tallest piece of text in this line chunk

	for (glyph = firstGlyph; glyph; glyph = (Glyph *)glyph->Next()) {
		if (glyph->GetAlign() == 0) {
			float top = glyph->GetTop();
			float bottom = top + glyph->GetHeight();
			maxAscent = MAX(maxAscent,-top);
			maxDescent = MAX(maxDescent,bottom);
		}
		if (glyph == lastGlyph) break;
	}

//	Position images for AV_ALIGNTEXTTOP, AV_MIDDLE, AV_BOTTOM, AV_BASELINE

	for (glyph = firstGlyph; glyph; glyph = (Glyph *)glyph->Next()) {
		height = glyph->GetHeight();
		switch(glyph->GetAlign()) {
			case AV_TOP:
			case AV_TEXTTOP:								// Align to top of text
				glyph->SetTop(-maxAscent);
				maxDescent = MAX(maxDescent,height - maxAscent);
				break;
			case AV_MIDDLE:									// Center on baseline
				glyph->SetTop(-height/2);
				maxAscent = MAX(maxAscent,height/2);
				maxDescent = MAX(maxDescent,height - height/2);
				break;

			case 0:											// Text alignment handled above.
			case AV_ABSMIDDLE:
			case AV_ABSBOTTOM:								// Do nothing, handled in next pass.
			case AV_LEFT:
			case AV_RIGHT:
				break;

			case AV_CENTER:
			case AV_BOTTOM:									// Sit on baseline
			case AV_BASELINE:
			default:
				glyph->SetTop(-height);
				maxAscent = MAX(maxAscent,height);
				break;
		}
		if (glyph == lastGlyph) break;
	}

//	Position images for AV_TOP, AV_ABSMIDDLE, AV_ABSBOTTOM

	for (glyph = firstGlyph; glyph; glyph = (Glyph *)glyph->Next()) {
		height = glyph->GetHeight();
		
		switch(glyph->GetAlign()) {
			case AV_LEFT:
			case AV_RIGHT:									// Do nothing if handle by floating layout.
				if (glyph->Floating())						// Otherwise fall through to ABSMIDDLE
					break;
			case AV_ABSMIDDLE:								// Center in line
				ascent = maxAscent - ((maxAscent + maxDescent) - height)/2;
				glyph->SetTop(-ascent);
				maxAscent = MAX(maxAscent,ascent);
				maxDescent = MAX(maxDescent,height - ascent);
				break;
			case AV_ABSBOTTOM:								// Sit on bottom of line
				glyph->SetTop(-(height - maxDescent));
				maxAscent = MAX(maxAscent,height - maxDescent);
				break;
		}
		if (glyph == lastGlyph) break;
	}

//	Calculate the required horizontal offset

	switch (align) {
		case AV_CENTER:	hOffset = MAX(0,space/2);	break;
		case AV_RIGHT:	hOffset = MAX(0,space);		break;
		default:		hOffset = 0;
	}

//	Baseline is vpos + maxAscent, move everybody there

	vPos += maxAscent;
	for (glyph = firstGlyph; glyph; glyph = (Glyph *)glyph->Next()) {
		if (glyph->Floating() == false) {
			glyph->SetTop(glyph->GetTop() + vPos);
			glyph->SetLeft(glyph->GetLeft() + hOffset);
			mLastLayedOut = glyph;
			glyph->LayoutComplete(mDrawPort);
		}

		if (mCurrentAnchor)
			mCurrentAnchor->AddGlyph(glyph);

		if (glyph == lastGlyph) break;
	}
	vPos += maxDescent;
	return vPos;		// Return the new vPos
}

//=================================================================================
//	PutLine draws the line we have assembled
//	Rulers will always be 100% the width .. that will cause problems with mMaxHPos

void PageGlyph::PutLine(Glyph *lineEnd)
{
	if (mLineBegin == NULL)
		return;
		
	mVPos = LayoutLineV(mVPos,mRightMargin - mHPos,mAlign,mLineBegin,lineEnd);

	if (mLineBegin->IsRuler() == false)
		mMaxHPos = MAX(mMaxHPos,mHPos);

	mHPos = mLeftMargin;
	mLineBegin = NULL;
}

//=================================================================================
//	PutGlyph adds a glyph to the current line
//	If it fits, great
//	If it doesn't fit and there is only one glyph on the line (wide image), draw it
//	If it doesn't fit draw glyphs before it, try again with it

Glyph *PageGlyph::PutGlyph(Glyph *glyph)
{
#if kDetails
	BString str;
	glyph->PrintStr(str);
	pprint("PutGlyph %s",(char*)str);
#endif
	bool atLeftMargin = mLeftMargin == mHPos;
	if (!mLaidOutVisibleGlyph && glyph->DoesDrawing())
		mLaidOutVisibleGlyph = true;

	if (glyph->IsLineBreak() && glyph != mLineBegin)
		PutLine((Glyph *)glyph->Previous());	// Draw line, stop before this glyph

	if (mLineBegin == NULL) {					// Start a new line
		LayoutDeferedFloating();				// Layout any floaters that need attention
		
		if (glyph->IsLineBreak()) {
			float v = LineBreak(mVPos, glyph->GetBreakType());	// Move vPos if this was a clear break
			if (v > 0)
				GetMargins(mVPos,&mLeftMargin,&mRightMargin);
			else if (glyph->IsExplicitLineBreak() && (atLeftMargin && (glyph->Previous() == NULL || ((Glyph*)glyph->Previous())->GetBreakType() != kHardAlign) || glyph->GetBreakType() == kParagraph))
				v = mDrawPort->BlankLineHeight();
			mVPos += v;
		}
		
		mLineBegin = glyph;
		//LayoutDeferedFloating();				// Layout any floaters that need attention
		GetMargins(mVPos,&mLeftMargin,&mRightMargin);
		mHPos = mLeftMargin;
	}

	glyph->Layout(mDrawPort);
	float width = glyph->GetWidth();

	if (glyph->Floating()) {				// Floating Glyphs
		if ((glyph->GetAlign() == AV_LEFT && mHPos > (mLeftMargin + 3)) ||		// Hack to forgive spaces
			(glyph->GetAlign() == AV_RIGHT && width > (mRightMargin - mHPos))) {	

			pprintBig("Problem floating image");
			DeferFloatingLayout(glyph);		// Defer layout until newline
			return (Glyph*)glyph->Next();
		}
		
/* This code forces us to have only one floater per side at any given vertical position.
   This is wrong, however -- the HTML expects floaters to stack up and can control them
   via BR CLEAR=xxx tags.
		if (HasCurrentFloater(mVPos, glyph->GetAlign())) {
			long v = LineBreak(mVPos, glyph->GetAlign() == AV_LEFT ? kHardLeft : kHardRight);
			if (v > 0) {
				v += mDrawPort->BlankLineHeight();
				mVPos += v;
				GetMargins(mVPos,&mLeftMargin,&mRightMargin);
				mHPos = mLeftMargin;
			}
		}
*/
		
		LayoutFloatingGlyph(glyph);
		float oldLeftMargin = mLeftMargin;
		GetMargins(mVPos,&mLeftMargin,&mRightMargin);
		
		//if (glyph->GetAlign() == AV_LEFT) {
		//	mHPos = mLeftMargin;
		//	mLineBegin = NULL;		// Start a new line? (THIS KILLED ANYTHING OF ZERO WIDTH ON THIS LINE)
		//}
		
		mHPos += mLeftMargin - oldLeftMargin;
		return (Glyph*)glyph->Next();
	}
	
//	Set the left edge in anticipation of its layout

	glyph->SetLeft(mHPos);
	


//	If the glyph fits, great

	if (mHPos + width <= mRightMargin) {	// Glyph Fits
		mHPos += width;
		return (Glyph *)glyph->Next();
	}
	
// If a table is laid out beside a floater, force the table to fit horizontally
// no matter what.

	if (glyph->IsTable() && mFloatingList) {
		for (short i = 0; i < mFloatingList->CountItems(); i++) {
			BRect r;
			Glyph *g = (Glyph *)mFloatingList->ItemAt(i);
			if (g->IsLayoutComplete()) {
				g->GetBounds(&r);
				if (mVPos >= r.top && mVPos <= r.bottom) {
					mHPos += width;
					return (Glyph *)glyph->Next();
				}
			}
		}
	}
	
// 	Try to break a piece of text to make it fit

	TextGlyph *text = glyph->IsText() ? (TextGlyph *)glyph : NULL;
	
	if (text && mHPos < mRightMargin) {
	
		if (text->GetStyle().pre) {	// Don't break preformatted text
			mHPos += width;
			//PutLine(glyph);	
			//NP_ASSERT((Glyph *)glyph->Next());
			return (Glyph *)glyph->Next();
		}

		float pos = text->BreakText(mRightMargin - mHPos,mHPos != mLeftMargin,mDrawPort);
		if (pos > 0 || text->Separable()) {
			Glyph* secondHalf = text->InsertSoftBreak(pos,mDrawPort);
			AddChildAfter(secondHalf,text);
			mHPos += text->GetWidth();
			return secondHalf;	// Soft Linebreak will invoke PutLine next time
		}
	}

//	Backup until we can separate item from previous one
	
	Glyph* endGlyph = glyph;
	while (!glyph->Separable() && glyph != mLineBegin && (Glyph*)glyph->Previous() != mLineBegin) {		
		glyph = (Glyph *)glyph->Previous();	
		mHPos -= glyph->GetWidth();

		// If the previous item is text, see if that text can be broken to avoid
		// breaking at a non-separable point.
		
		if (glyph->IsText() && glyph->GetWidth() > 1) {	
			TextGlyph *text = (TextGlyph *)glyph;	
			long pos = text->BreakText(glyph->GetWidth() - 1, true,mDrawPort);
			if (pos > 0) {
				Glyph* secondHalf = text->InsertSoftBreak(pos,mDrawPort);
				AddChildAfter(secondHalf,text);
				mHPos += text->GetWidth();
				return secondHalf;	// Soft Linebreak will invoke PutLine next time
			}
		}
	}
	
	// If we backed up to the start of the line, trying clearing floating margins.

	if (mHPos == mLeftMargin) {					// Non-separable items do not fit (wide image, for example)
		float v = LineBreak(mVPos, kHardAll);	// Clear any floaters.
		if (v > 0) {
			mVPos += v;
			GetMargins(mVPos,&mLeftMargin,&mRightMargin);
			mHPos = mLeftMargin;
			return glyph;					// Try again, in new margin;
		}
	}
	

	Glyph* previous = (Glyph*)glyph->Previous();
	if (glyph == mLineBegin || (previous == mLineBegin && !mLineBegin->IsSpatial())) {
#if 0
	// Insert soft break. Return of SoftBreak will invoke PutLine next time.
		if (glyph->Next() != NULL)
			((Glyph*)glyph->Next())->SetBreakType(kSoft);
		mHPos += glyph->GetWidth();
		return (Glyph*)glyph->Next();
#else
	//  If we walked back to the start of the line looking for a separable glyph, we can't break this group
	//	We have no choice but to move on to the next glyph
	
		do {
			mHPos += glyph->GetWidth();
			if (glyph != endGlyph)
				glyph = (Glyph *)glyph->Next();
		} while (glyph != endGlyph);
		
		return (Glyph*)endGlyph->Next();
#endif
	}
	
//	Nothing worked, break next line

	glyph->SetBreakType(kSoft);
	return glyph;
}

//	Remove all soft breaks from the page, layout will generate more

void PageGlyph::RemoveSoftBreaks()
{
	Glyph	*glyph;
	for (glyph = GetChildren(); glyph; glyph = (Glyph *)glyph->Next()) {
		if (glyph->GetBreakType() == kSoft) {
			Glyph* previous = (Glyph*)glyph->Previous();
			if (glyph->IsText() && previous->IsText()) {
				((TextGlyph *)previous)->RemoveSoftBreak((TextGlyph *)glyph);
				DeleteChild(glyph);
				glyph = previous;
			} else
				glyph->SetBreakType(kNoBreak);
		}
	}
}


// A glyph can call this during layout to find out how much room there is
// left on the line, not including its own width.
float PageGlyph::SpaceLeftOnLine()
{
	return mRightMargin - mHPos;
}

//	Set mCurrentAnchor when an anchor->Layout();

void PageGlyph::SetCurrentAnchor(Glyph* anchorGlyph)
{
	mCurrentAnchor = (AnchorGlyph *)anchorGlyph;
}

//=================================================================================
//	Layout all children

void PageGlyph::SetupLayout(DrawPort *drawPort)
{
	mDrawPort = drawPort;
	RemoveSoftBreaks();
	PrepareFloating();
	mVPos = mTop;
	mMaxHPos = 0;
	mLeftMarginDefault = 0;
	mRightMarginDefault = 0;
	mLineBegin = 0;
	mAlign = AV_LEFT;
}

//	Block layout until glyphs are ready

void PageGlyph::Layout(DrawPort *drawPort)
{
	if (mNextToPut == NULL) {
		if (mLastPut == NULL) {
			SetupLayout(drawPort);		// Setup layout first time thru
			mNextToPut = GetChildren();
		} else
			mNextToPut = (Glyph *)mLastPut->Next();
	}

	while (mNextToPut && mNextToPut->ReadyForLayout()) {
		mLastPut = mNextToPut;
		mNextToPut = PutGlyph(mNextToPut);
	}

	if (mNextToPut == NULL && !mOpen) {	// If page is closed and out of glyphs...
		PutLine(0);
		LayoutDeferedFloating();
		mLastPut = NULL;
		
//		Floating glyphs may have been layed out before the final Putline
//		Advance mLastLayedOut to end of glyphs....

		while (mLastLayedOut && mLastLayedOut->Next())
			mLastLayedOut = (Glyph*)mLastLayedOut->Next();
	}
	mHeight = mVPos - mTop;				// Height of layout so far ....

//	Bottom of floating images may hang below bottom of last line

	if (mFloatingList)
		for (short i = 0; i < mFloatingList->CountItems(); i++) {
//			ImageGlyph *image = (ImageGlyph *)mFloatingList->Get(i);
//			mHeight = MAX(mHeight,(image->GetTop() + image->GetHeight()) - mTop);
			Glyph *g = (Glyph *)mFloatingList->ItemAt(i);
			mHeight = MAX(mHeight,(g->GetTop() + g->GetHeight()) - mTop);
		}
}

//	Don't draw if it is open?

void PageGlyph::Draw(DrawPort *drawPort)
{
	if (mLastLayedOut == NULL)
		return;
	Glyph *glyph = GetChildren();
	if (glyph == NULL)		// Nothing to draw yet
		return;

//	Draw Glyphs that are clipped off top
	BRect clip = drawPort->GetClip();
#if __INTEL__
	// Boy, Intel sure sucks at floating point.  It's cheaper
	// to convert the coordinates to integer and do the comparisons
	// there than to do the comparisons in floating point.
	int top = FastConvertFloatToIntRound(clip.top);
	int bottom = FastConvertFloatToIntRound(clip.bottom);
//	int left = FastConvertFloatToIntRound(clip.left);
//	int right = FastConvertFloatToIntRound(clip.right);
#endif


//	BWindow *window = ((BeDrawPort *)drawPort)->GetView()->Window();
//	if (!window->Lock())
//		return;

	BRect r;
	while (glyph) {
		if (glyph->DoesDrawing()) {
			glyph->GetBounds(&r);
	//		if (r.Intersects(clip))
#if __INTEL__
			if (
				FastConvertFloatToIntRound(r.top) <= bottom &&
				FastConvertFloatToIntRound(r.bottom) >= top
// Believe it or not, it will on average cost us less than to simply draw the glyph
// if it's in the vertical range of the update region than to check to see if
// it's in the horizontal region.  The check is expensive, and chances are, it will
// be in the horizontal region.
//				FastConvertFloatToIntRound(r.left) <= right &&
//				FastConvertFloatToIntRound(r.right) >= left
				)
#else
			if (r.top <= clip.bottom &&
			    r.bottom >= clip.top &&
			    r.left <= clip.right &&
			    r.right >= clip.left)
#endif
					glyph->Draw(drawPort);
		}
	
		if (glyph == mLastLayedOut)
			break;
		glyph = (Glyph *)glyph->Next();
	}
//done:
//	window->Unlock();

//	Skip glyphs that are off bottom
}

//====================================================================================
//====================================================================================
//	RuleGlyph Draws horizontal rules, gets its horizontal alignment from alignment glyphs

RuleGlyph::RuleGlyph(Document* htmlDoc) :
	CompositeGlyph(htmlDoc)
{
	mBreakType = kHard;
	mHAlign = AV_CENTER;
	mKnownWidth = 0;
	mPercentageWidth = false;
	mNoShade = false;
	
	mHeight = 6 + (1 + 1) + 6;
}

#ifdef DEBUGMENU
void RuleGlyph::PrintStr(BString& print)
{
	CompositeGlyph::PrintStr(print);
	print += " ";
	char str[128];
	if (mPercentageWidth)
		sprintf(str,"%d%%",mPercentageWidth);
	else 
		sprintf(str,"%f",mKnownWidth);
	print += str;
}
#endif
	
float RuleGlyph::GetMinUsedWidth(DrawPort *)
{
	return 8;	// A ruler can be less than 8 pixels wide ..
}

void RuleGlyph::Layout(DrawPort *)
{
	if (mPercentageWidth)
		mWidth = (mPercentageWidth * ((PageGlyph *)GetParent())->GetMarginWidth())/100;
	else {
		mWidth = ((PageGlyph *)GetParent())->GetMarginWidth();
		if (mKnownWidth)
			mWidth = MIN(mKnownWidth,mWidth);
	}
}

void RuleGlyph::Draw(DrawPort *drawPort)
{
	BRect r;
	GetBounds(&r);
	r.InsetBy(0,6);
	drawPort->DrawRule(&r,mNoShade);
}

int RuleGlyph::GetHAlign()
{
	return mHAlign;
}

void RuleGlyph::SetAttribute(long attributeID, long value, bool isPercentage)
{
	switch (attributeID) {				
		case A_SIZE:	mHeight = (value + 1) + 6 + 6;	// Funny number of pixels either side
						break;
	
		case A_ALIGN:	if (value > 0) mHAlign = value; 	break;
		
		case A_NOSHADE:	mNoShade = true; break;
		case A_WIDTH:
				if (isPercentage)
					mPercentageWidth = MAX(MIN(value, 100), 0);
				else
					mKnownWidth = MAX(value, 0);
			break;
	}
}

// ===========================================================================
//	MarginGlyph defines left margin position
//	It is used to implement lists and defs

#define kMarginWidth 40

MarginGlyph::MarginGlyph(Document* htmlDoc) :
	SpatialGlyph(htmlDoc)
{
	mLeftMargin = 0;
	mRightMargin = 0;
	mParent = 0;
	mBreakType = kHardAlign;
	mBreakTypeValid = false;
}

#ifdef DEBUGMENU
void MarginGlyph::PrintStr(BString& print)
{
	SpatialGlyph::PrintStr(print);
	char str[128];
	sprintf(str," At %f",mLeftMargin);
	print += str;
}
#endif

bool MarginGlyph::IsMargin()
{
	return true;
}

void MarginGlyph::SetLeftMargin(float margin)
{
	mLeftMargin = margin;
}

void MarginGlyph::SetRightMargin(float margin)
{
	mRightMargin = margin;
}

float MarginGlyph::GetLeftMargin()
{
	return mLeftMargin * kMarginWidth;
}

float MarginGlyph::GetRightMargin()
{
	return mRightMargin * kMarginWidth;
}

void MarginGlyph::Layout(DrawPort *)	// Set the alignment of parent
{
	if (GetParent()) {
		((PageGlyph *)GetParent())->SetLeftMarginDefault(mLeftMargin * kMarginWidth);
//		if (mRightMargin > 0)
			((PageGlyph *)GetParent())->SetRightMarginDefault(mRightMargin * kMarginWidth);
	}
	mBreakTypeValid = false;
}

#ifdef DEBUGMENU
void MarginGlyph::Draw(DrawPort *drawPort)
{
	if (modifiers() & B_CAPS_LOCK) {
		BRect r;
		GetBounds(&r);
		drawPort->SetColor(0xFF,0x00,0x00);	// Lists are red
		drawPort->MoveTo(r.left - 4,r.top - 2);
		drawPort->LineTo(r.left - 4,r.top + 3);
		drawPort->LineTo(r.left,r.top);
		drawPort->LineTo(r.left - 4,r.top - 2);
		drawPort->SetGray(0);
	}
}
#else
void MarginGlyph::Draw(DrawPort *)
{
}
#endif

Glyph* MarginGlyph::GetParent()
{
	return mParent;
}

void MarginGlyph::SetParent(Glyph *parent)
{
	mParent = parent;
}

// ===========================================================================
//	DDMarginGlyph defines left margin position for dictionary definitions

short DDMarginGlyph::GetBreakType()
{
//	Backup to the previous line break to see if DD should be on same line as DT

	if (!mBreakTypeValid) {
		float width = 0;
		Glyph *g;
		for (g = (Glyph *)Previous(); g; g = (Glyph*)g->Previous()) {
			if (g->IsLineBreak()) break;
			width += g->GetWidth();
		}
		mBreakType = kHardAlign;
		if (width && width < kMarginWidth)	// Don't line break if DT was narrow
			mBreakType = kNoBreak;
		else if ((bool)(g = (Glyph *)Previous()) && g->IsLineBreak())	// Don't break twice
			mBreakType = kNoBreak;
		mBreakTypeValid = true;
	}
	return mBreakType;
}

// ===========================================================================

void MakeAlphaStr(char *label,long count)
{
	if (count < 0) count = -count;
	label[0] = 'A' - 1 + (count % 26);	// Could do aa, aaa etc.
	label[1] = 0;
}

char *gRHun[] = {"C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM"};
char *gRTen[] = {"X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC"};
char *gROne[] = {"I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};

void MakeRomanStr(char *label, long count)
{
	if (count >= 1000 || count <= 0) {
		strcpy(label, "?");
		return;
	}
	
	*label = 0;
	int digit = count / 100;
	count -= digit * 100;
	if (digit > 0)
		strcat(label, gRHun[digit - 1]);

	digit = count / 10;
	count -= digit * 10;
	if (digit > 0)
		strcat(label, gRTen[digit - 1]);

	if (count > 0)
		strcat(label, gROne[count - 1]);
}

// ===========================================================================
//	BulletGlyph .. puts the dot left of the list items
//	kind contains 4 bits of kind
//	12 bits of ordinal value
//

BulletGlyph::BulletGlyph(Document* htmlDoc) :
	SpatialGlyph(htmlDoc)
{
	mBreakType = kHardAlign;
	mKind = mCount = 0;
	mDoesDrawing = true;
	mHasBounds = true;
}

void BulletGlyph::GetBounds(BRect* r)
{
	SpatialGlyph::GetBounds(r);
	if (mKind != 0)
		r->left = r->right - kMarginWidth;
}

#ifdef DEBUGMENU
void BulletGlyph::PrintStr(BString& print)
{
	SpatialGlyph::PrintStr(print);
	char str[128];
	sprintf(str," %d [x]",mCount);
	str[strlen(str)-2] = mKind;
	print += str;
}
#endif

bool	BulletGlyph::IsBullet()				// False
{
	return true;
}

void BulletGlyph::SetKind(uchar kind, int32 count)
{
	mKind = kind;
	mCount = count;
}

//	If no containing list exisits, be a dot with width

float BulletGlyph::GetWidth()
{
	return mKind ? 0 : (kMarginWidth/2);
}

void BulletGlyph::Layout(DrawPort *drawPort)
{
	SpatialGlyph::Layout(drawPort);
	drawPort->SetStyle(gDefaultStyle);
	SetHeight(drawPort->GetFontDescent() + drawPort->GetFontAscent());
}

short BulletGlyph::GetBreakType()
{
	return SpatialGlyph::GetBreakType();
	Glyph *g;
	if ((bool)(g = (Glyph *)Previous()) && g->IsLineBreak())	// Don't break twice
		return kNoBreak;
	return kHard;			// This thing is a line break
}

void BulletGlyph::GraphicBullet(DrawPort *drawPort, float left)
{
	drawPort->SetStyle(gDefaultStyle);
	BRect r;
	r.left = left - 2;
	r.top = mTop + drawPort->GetFontAscent() - 2.0;
	r.right = r.left + 4;
	r.bottom = r.top + 4;

	switch (mKind) {
		case 0:		drawPort->DrawBevel(&r);		break;
		case 'd':	drawPort->DrawRoundBevel(&r);	break;	// Disc
		case 'c':	drawPort->DrawTriBevel(&r);		break;	// Triangle
		case 's':	drawPort->DrawAntiBevel(&r);	break;	// Square
	}
}

//	Draw a text based bullet

void BulletGlyph::TextBullet(DrawPort *drawPort, BString label, bool lowerCase)
{
	if (lowerCase) label.ToLower();	// Make lower case if req.
	short i = label.Length();
//	label[i] = '.';
//	label[i+1] = 0;					// Add a period
	label += '.';

	drawPort->SetStyle(gDefaultStyle);
	float width = drawPort->TextWidth(label.String(),label.Length()) + 8;
	drawPort->DrawText(mLeft - width,mTop + drawPort->GetFontDescent(),label.String(),i + 1,0);
}

//	Decide what kind of bullet to draw, either text or graphic

void BulletGlyph::Draw(DrawPort *drawPort)
{
	char label[32];
	label[0] = 0;

	switch (mKind) {
		case 0:
			GraphicBullet(drawPort,mLeft + 16);	// Not inside a list, draw a Disc
			break;
		case 'd':
		case 'c':
		case 's':
			GraphicBullet(drawPort,mLeft - 10);			// Draw the graphic bullets
			break;
		case '1':
			sprintf(label,"%d",mCount);					// Numeric
			TextBullet(drawPort,label,false);
			break;
		case 'A':
		case 'a':
			MakeAlphaStr(label,mCount);
			TextBullet(drawPort,label,mKind == 'a');	// Alpha
			break;
		case 'I':
		case 'i':
			MakeRomanStr(label,mCount);
			TextBullet(drawPort,label,mKind == 'i');	// Roman
			break;
	}
}
