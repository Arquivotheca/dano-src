// ===========================================================================
//	AnchorGlyph.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================
 
#include "AnchorGlyph.h"
#include "HTMLTags.h"
#include "ImageGlyph.h"
#include "TextGlyph.h"
#include "BeDrawPort.h"
#include "URL.h"
#include "HTMLDoc.h"
#include "Utils.h"

#include <malloc.h>
#include <stdio.h> 

void *AnchorEndGlyph::sFreeList;
void *AnchorGlyph::sFreeList;
 // ===========================================================================
 
 AnchorEndGlyph::AnchorEndGlyph()
 {
 	mParent = 0;
	mHasBounds = true;
 }
 
 bool AnchorEndGlyph::IsAnchorEnd()
{
	return true;
}
 
 void AnchorEndGlyph::LayoutComplete(DrawPort *)
 {
 	((CompositeGlyph *)GetParent())->SetCurrentAnchor(0);
 }
 
 Glyph*	AnchorEndGlyph::GetParent()
 {
 	return mParent;
 }
 
 void AnchorEndGlyph::SetParent(Glyph *parent)
 {
 	mParent = parent;
 }
 
 // ===========================================================================
 
AnchorGlyph::AnchorGlyph(Document *document)
 {
	mDocument = document;
 	mRegion = new BRegion;
 	mImageMap = 0;
	// Set the top to -1.  If we try to scroll to the anchor and it hasn't
	// been laid out yet, this value will let us be able to know that.
 	mTop = -1;
 	mVisited = false;
	mHilite = 0;
	mShowTitleInToolbar = kShowTitleUnspecified;
#ifdef JAVASCRIPT
	mLocation.glyph = this;
	mLocation.view = NULL;
	mContainer = NULL;
#endif
 }
 
 AnchorGlyph::~AnchorGlyph()
 {
 	delete(mRegion);
 }
 
bool AnchorGlyph::IsAnchor()
{
	return true;
}

void AnchorGlyph::SetTop(float top)
 {
#if __INTEL__
	mRegion->OffsetBy(0, FastConvertFloatToIntRound(top - GetTop()));
#else
 	mRegion->OffsetBy(0,top - GetTop());
#endif
 	mTop = top;
 }
 
void AnchorGlyph::SetLeft(float left)
 {
#if __INTEL__
	mRegion->OffsetBy(FastConvertFloatToIntRound(left - GetLeft()), 0);
#else
 	mRegion->OffsetBy(left - GetLeft(),0);
#endif
 }

void AnchorGlyph::OffsetBy(const BPoint& offset)
{
#if __INTEL__
	mRegion->OffsetBy(FastConvertFloatToIntRound(offset.x),
					  FastConvertFloatToIntRound(offset.y));
#else
	mRegion->OffsetBy(offset.x, offset.y);
#endif
	mTop += offset.y;
}
 
 void AnchorGlyph::SetAttributeStr(long attributeID, const char* value)
 {
#ifdef JAVASCRIPT
	bool initJS = false;
#endif

 	switch (attributeID) {
 		case A_HREF:
			mHREF = value;
#ifdef JAVASCRIPT
			mLocation.location = value;
#endif
			break;
 		case A_NAME: {
 			char *temp = (char *)malloc(strlen(value) + 1);
 			CleanName(value, temp);	
 			mName = temp;
 			free(temp); 
 			break;
		}
#ifdef JAVASCRIPT
 		case A_ONMOUSEOVER:	mOnMouseOverScript = value; initJS = true; break;
 		case A_ONMOUSEOUT:	mOnMouseOutScript = value;	initJS = true; break;
		case A_ONCLICK:		mOnClickScript = value;		initJS = true; break;
#endif
 		case A_TARGET:		mTarget = value;		break;
 	}
#ifdef JAVASCRIPT
	if (initJS) {
		if (mDocument->LockDocAndWindow()) {
			mDocument->InitJavaScript();
			mDocument->UnlockDocAndWindow();
		}
	}
#endif
 }

void AnchorGlyph::SetAttribute(long attributeID, long value, bool isPercentage)
{
	if (attributeID == A_SHOWTITLEINTOOLBAR)
		mShowTitleInToolbar = (value == AV_YES) ? kShowTitle : kShowURL;
}


#ifdef JAVASCRIPT
void AnchorGlyph::SetContainer(jseVariable container)
{
	if (!mContainer) {
		if (!mDocument->LockDocAndWindow())
			return;
		mContainer = container;
		if (mOnMouseOverScript.Length())
			mOnMouseOver = mDocument->AddHandler(container, "onMouseOver", mOnMouseOverScript.String());
		else
			mOnMouseOver = 0;
		if (mOnMouseOutScript.Length())
			mOnMouseOut = mDocument->AddHandler(container, "onMouseOut", mOnMouseOutScript.String());
		else
			mOnMouseOut = 0;
		if (mOnClickScript.Length())
			mOnClick = mDocument->AddHandler(container, "onClick", mOnClickScript.String());
		else
			mOnClick = 0;
		mDocument->UnlockDocAndWindow();
	}
}
#endif
 
bool CleanupRect(BRect& r)
{
	// There are some bugs in BRegion that will cause it to give us a bogus region
	// if we don't add very well-formed rectangles to it.  Let's try to clean up
	// what we're adding to the region; don't add zero-sized (or at least very small)
	// rectangles, and don't add a rectangle if it's already in the region.  Also,
	// BRegion doesn't like non-integral coordinates; patch them up.
	r.top = floor(r.top);
	r.left = floor(r.left);
	r.bottom = floor(r.bottom /*+ 0.5*/);
	r.right = floor(r.right /*+ 0.5*/);
	if (r.Width() < 0.5 || r.Height() < 0.5)
		return false;
	return true;
}

 void AnchorGlyph::AddGlyph(Glyph *glyph)
 {
 	if (glyph->IsImage())
 		mImageMap = (ImageGlyph *)glyph;	// Store image map in anchor, if any
 	else {
 		if (!mHREF.Length())
 			return;							// Fragment anchor
 		BRect r;
 		glyph->GetBounds(&r);

		if (!CleanupRect(r))
			return;
			
		// This simple test for seeing if the region contains our rectangle won't
		// work for a lot of pathological cases, but it's good enough for us.
		if (mRegion->Contains(r.LeftTop()) && mRegion->Contains(r.RightBottom()))
			return;

 		mRegion->Include(r);				// Don't Add the image region to the anchor
 	}
 }
 
 float  AnchorGlyph::GetTop()
 {
 	return mTop;
 }
 
const char* AnchorGlyph::GetHREF()
{
 	return mHREF.String();
}

void AnchorGlyph::SetHREF(const char *href)
{
	mHREF = href;
}
 
const char* AnchorGlyph::GetName()
{
 	return mName.String();
}

const char* AnchorGlyph::GetOnMouseOver()
{
 	return mOnMouseOverScript.String();
}

const char* AnchorGlyph::GetTarget()
{
 	return mTarget.String();
}

bool AnchorGlyph::MouseEnter()
{
#ifdef JAVASCRIPT
	if (mOnMouseOverScript.Length() > 0) {
		mDocument->ExecuteHandler(mContainer, mOnMouseOver);
		return true;
	}
#endif
	return false;
}

void AnchorGlyph::MouseLeave()
{
#ifdef JAVASCRIPT
	if (mOnMouseOutScript.Length() > 0)
		mDocument->ExecuteHandler(mContainer, mOnMouseOut);
#endif
}

bool AnchorGlyph::ExecuteOnClickScript()
{
#ifdef JAVASCRIPT
	if (mOnClickScript.Length() > 0) {
		mDocument->ExecuteHandler(mContainer, mOnClick);
		return true;
	}
#endif
	return false;
}

void
AnchorGlyph::SetTarget(
	const char	*target)
{
	mTarget = "";

	if (target != NULL)
		mTarget = target;
}


 //	Set the visited link in all the text inside this anchor
 
 void AnchorGlyph::SetVisited(bool visited)
 {
 	mVisited = visited;
 	Glyph *g;
 	for (g = (Glyph *)Next(); g; g = (Glyph *)g->Next()) {
 		if (g->IsText())
 			if (((TextGlyph *)g)->GetStyle().anchor)
 				((TextGlyph *)g)->SetVisited(visited);
 			else
 				return;
 	}
 }
 
 void AnchorGlyph::Layout(DrawPort *)
 {
 	mRegion->MakeEmpty();
 }
 
 void AnchorGlyph::LayoutComplete(DrawPort *)
 {
 	((CompositeGlyph *)GetParent())->SetCurrentAnchor(this);
 }
 
 //	Track a click in the anchor .... image maps don't hilite...
 
 bool AnchorGlyph::Clicked(float h, float v)
 {
 	if (mImageMap && mImageMap->Clicked(h,v))	// Remember where in the image was clicked
 		return true;
 	return mRegion->Contains(BPoint(h,v));
 }
 
 void AnchorGlyph::Hilite(long value, DrawPort *drawPort)
 {
 	if (mImageMap) {
 		if (mImageMap->IsMap()) return;	// Don't hilite maps, do hilite images
 		mImageMap->Hilite(value,drawPort);
 	} else {
 		if (mHilite != value)
			drawPort->InvertRgn(mRegion);
		if (value != -1)
			mHilite = value;
	}
 }

void AnchorGlyph::GetAnchorBounds(BRect *rect)
{
	if(mRegion){
		BRect testRect = mRegion->Frame();
		if(testRect.IsValid())
			*rect = testRect;
	}
}
 
 //	True if anchor glyph is an image map
 
 bool AnchorGlyph::IsImageMap()
 {
 	return mImageMap && mImageMap->IsMap();
 }
 
 //	Get the url of the anchor, add image map info if it exists
 
 bool AnchorGlyph::GetURL(BString& url, BString& target, int& showTitleInToolbar, ImageMap* imageMapList)
 {
 	url= "";
	target = "";
 	const char *href = GetHREF();
 		
 	if (mImageMap && mImageMap->IsMap()) {
 		float h;
 		float v;
 		mImageMap->GetClickCoord(&h,&v);
 		
 //		Try and use client side image maps, if present
 
 		const char *usemap = mImageMap->GetUSEMAP();

		// You are allowed to have non-local image maps by specifying a URL followed by # and the
		// map name.  We don't support non-local image maps, so we'll just assume that it's local by
		// stripping off everything leading up to the '#'.
		usemap = strchr(usemap, '#');
 		if (usemap && imageMapList && *usemap++ == '#') {		// Only use local image maps....
 			ImageMap* imageMap;
 			for (imageMap = (ImageMap *)imageMapList->First(); imageMap; imageMap = (ImageMap*)imageMap->Next())
 				if (strcmp(usemap,imageMap->GetName()) == 0) {
 					const char *map = imageMap->GetHREF(h,v);
 					if (map) {
						if (strcmp(map, "__netpositive__nohref__") == 0)
							return false;
						url = map;
	 					const char *tgt = imageMap->GetTarget(h,v);
	 					if (tgt)
	 						target = tgt;
						showTitleInToolbar = imageMap->GetShowTitleInToolbar(h,v);
 						return true;		// Use the client side image map
 					}
 				}
 		}
 		
 //		Fall thru to normal click if client side image map fails
 		
 		if (href == NULL) return false;
		url << href << "?" << (int32)h << "," << (int32)v;
// 		sprintf(url,"%s?%d,%d",href, (int)h, (int)v);

 		if (mTarget.Length())
 			target = mTarget;

 	} else {
 	
 //		Or just return the regular anchor
 
 		if (href == NULL) return false;
		url = href;

 		if (mTarget.Length())
 			target = mTarget;

		showTitleInToolbar = mShowTitleInToolbar;
 	}
 	return true;
 }
 
 void AnchorGlyph::SetURL(char *url)
 {
 	SetAttributeStr(A_HREF,url);
 }
 
