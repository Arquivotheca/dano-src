// ===========================================================================
//	ImageGlyph.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "ImageGlyph.h"
#include "HTMLTags.h"
#include "Image.h"
#include "HTMLDoc.h"
#include "TableGlyph.h"
#include "Protocols.h"
#include "MessageWindow.h"
#include "AnchorGlyph.h"

#include <malloc.h>
#include <stdio.h>
#include <ctype.h>

// ===========================================================================
//	Client side image map Areas

Area::Area(const char* coords, const char* href, const char* target, int showTitleInToolbar) : mHREF(href), mTarget(target), mShowTitleInToolbar(showTitleInToolbar)
{
	char *cstr = (char *)malloc(strlen(coords) + 1);
	strcpy(cstr, coords);
	
	char *c = cstr;
	while (*c) {
		if ((*c) == ',')
			*c = ' ';
		c++;
	}
			
//	sscanf(cstr,"%ld %ld %ld %ld",&mRect.left,&mRect.top,&mRect.right,&mRect.bottom);
	sscanf(cstr,"%f %f %f %f",&mRect.left,&mRect.top,&mRect.right,&mRect.bottom);
	
	free(cstr);
}

const char*	Area::GetHREF(float x, float y)
{
	if ((x >= mRect.right) || (x < mRect.left)) return NULL;
	if ((y >= mRect.bottom) || (y < mRect.top)) return NULL;
	return mHREF.String();
}

const char*	Area::GetTarget(float x, float y)
{
	if ((x >= mRect.right) || (x < mRect.left)) return NULL;
	if ((y >= mRect.bottom) || (y < mRect.top)) return NULL;
	return mTarget.String();
}

int	Area::GetShowTitleInToolbar(float x, float y)
{
	return mShowTitleInToolbar;
}

AreaCircle::AreaCircle(const char* coords, const char* href, const char *target, int showTitleInToolbar) : Area(coords,href,target, showTitleInToolbar)
{
}

const char*	AreaCircle::GetHREF(float x, float y)
{
	x = x - mRect.left;
	y = y - mRect.top;
	
	float radius = mRect.right;
	if ((x*x + y*y) < (radius*radius))
		return mHREF.String();
	
	return NULL;
}

const char*	AreaCircle::GetTarget(float x, float y)
{
	x = x - mRect.left;
	y = y - mRect.top;
	
	float radius = mRect.right;
	if ((x*x + y*y) < (radius*radius))
		return mTarget.String();
	
	return NULL;
}

AreaPoly::AreaPoly(const char* coords, const char* href, const char* target, int showTitleInToolbar) : Area(coords,href,target,showTitleInToolbar)
{
	char *cstr = (char *)malloc(strlen(coords) + 1);
	
	// Convert commas to spaces and strip out any extra spaces.
	char *c = cstr;
	while (*coords) {
		if ((*coords) == ',')
			*c = ' ';
		else
			*c = *coords;
		if (c == cstr || *c != ' ' || *(c - 1) != ' ' || (*c != ' ' && (*c < '0' || *c > '9')))
			c++;
		coords++;
	}
	
	*c = 0;
			
	c = cstr;
	mVertexCount = 0;
	while (c) {
		long x,y;
		if (sscanf(c, "%ld %ld", &x, &y) != 2)
			c = 0;
		else {
			mXVertices[mVertexCount] = x;
			mYVertices[mVertexCount] = y;
			mVertexCount++;
			c = strstr(c + 1, " ");
			if (c)
				c = strstr(c + 1, " ");
		}
	}
	
	free(cstr);
}

const char*	AreaPoly::GetHREF(float x, float y)
{
	if (PointInPoly(x,y))
		return mHREF.String();
	else
		return NULL;
}

const char*	AreaPoly::GetTarget(float x, float y)
{
	if (PointInPoly(x,y))
		return mTarget.String();
	else
		return NULL;
}

bool AreaPoly::PointInPoly(float x, float y)
{
	int i, j;
	bool c = false; 
	for (i = 0, j = mVertexCount - 1; i < mVertexCount; j = i++) { 
		if ((((mYVertices[i]<=y) && (y<mYVertices[j])) || 
		     ((mYVertices[j]<=y) && (y<mYVertices[i]))) && 
		    (x < (mXVertices[j] - mXVertices[i]) * (y - mYVertices[i]) / (mYVertices[j] - mYVertices[i]) + mXVertices[i])) 
		
		  c = !c; 
	} 
	return c; 
}


// ===========================================================================
//	Client side image map

ImageMap::ImageMap(char *name) : mName(name)
{
}

void ImageMap::AddArea(int shape, const char* coords, const char* url, const char *target, int showTitleInToolbar)
{
	Area* area = NULL;
	switch (shape) {
		case AV_RECT:	area = new Area(coords,url,target,showTitleInToolbar);			break;
		case AV_CIRCLE:	area = new AreaCircle(coords,url,target,showTitleInToolbar);	break;
		case AV_POLY:
		case AV_POLYGON:area = new AreaPoly(coords,url, target,showTitleInToolbar);	break;
	}
	if (area)
		mAreas.Add(area);
}

//	Return the HREF if an area is clicked in

const char*	ImageMap::GetHREF(float x, float y)
{
	Area* a;
	for (a = (Area*)mAreas.First(); a; a = (Area*)a->Next()) {
		if (const char *href = a->GetHREF(x,y))
			return href;
	}
	return NULL;
}

const char*	ImageMap::GetTarget(float x, float y)
{
	Area* a;
	for (a = (Area*)mAreas.First(); a; a = (Area*)a->Next()) {
		if (const char *href = a->GetTarget(x,y))
			return href;
	}
	return NULL;
}

int ImageMap::GetShowTitleInToolbar(float x, float y)
{
	Area* a;
	for (a = (Area*)mAreas.First(); a; a = (Area*)a->Next()) {
		if (const char *href = a->GetTarget(x,y))
			return a->GetShowTitleInToolbar(x,y);
	}
	return kShowTitleUnspecified;
}

const char*	ImageMap::GetName()
{
	return mName.String();
}

// ===========================================================================
//	ImageGlyph is an image!

ImageGlyph::ImageGlyph(
	ConnectionManager *mgr,
	bool	forceCache,
	BMessenger *listener,
	Document* htmlDoc,
	bool firstFrameOnly) :
		SpatialGlyph(htmlDoc)
{
	mDoesDrawing = true;
	mHasBounds = true;
	mForceCache = forceCache;

	mIsMap = false;
	
	mBorder = -1;
	mHSpace = -1;	// Default horizontal space outsize tables..
	mVSpace = -1;
	
	mAlign = AV_BASELINE;
	mPositionIsFinal = false;
	
	mImageHandle = NULL;
	

//	mCache = 0;
	mClickH = 0;
	mClickV = 0;
	mIsAnchor = 0;
//	mLoadImage = true;
	mNoBreak = false;
	
	mHasBGColor = false;
	mHasBGImage = false;
	mBGGlyph = NULL;
	mConnectionMgr = mgr;
	mConnectionMgr->Reference();
	
	mListener = listener;
	mHasWidthAttr = false;
	mHasHeightAttr = false;
	Reset();
	mAskedForLayout = 0;
	mFirstFrameOnly = firstFrameOnly;
}

ImageGlyph::~ImageGlyph()
{
	if (mImageHandle)
		mImageHandle->Dereference();
//	delete mImageHandle;
	mConnectionMgr->Dereference();
}

void ImageGlyph::Reset()
{
	mAborted = false;
	mGetSizeLater = 0;
	mLayoutComplete = 0;
	if (!mHasHeightAttr)
		mHeight = 0;
	if (!mHasWidthAttr)
		mWidth = 0;
	mAskedForLayout = 1;
}

void ImageGlyph::Unload()
{
	Reset();
	if (mImageHandle) {
		mImageHandle->Dereference();
		mImageHandle = NULL;
	}
}

//	Get Dimesions of Image set by tag

void ImageGlyph::GetKnownSize(float& width, float& height)
{
	width = mWidth;
	height = mHeight;
}

//

void ImageGlyph::SetAnchor(Glyph* isAnchor)
{
	mIsAnchor = isAnchor;
}

Glyph* ImageGlyph::GetAnchor()
{
	return mIsAnchor;
}

// It is, in fact, and image

bool ImageGlyph::IsImage()
{
	return true;
}

bool ImageGlyph::IsTransparent()
{
	return mImageHandle->IsTransparent();
}

//	And it might be a floating image

bool ImageGlyph::Floating()
{
	return (mAlign == AV_LEFT || mAlign == AV_RIGHT);
}

//	Can be separated from other glyphs (not if mNoBreak)

bool	ImageGlyph::Separable()
{
	return mNoBreak == false;
	
	if (mNoBreak) {
		ImageGlyph *g = (ImageGlyph *)Previous();
		if (g && g->IsImage() && g->mNoBreak)
			return false;
	}
	return true;
}

void ImageGlyph::SetNoBreak(bool noBreak)
{
	mNoBreak = noBreak;
}

//	And it might be a map!

bool	ImageGlyph::IsMap()
{
	return mIsMap || (mUSEMAP.Length() != 0);
}

//	Remember where the image was last clicked

bool	ImageGlyph::Clicked(float h, float v)
{
	BRect r;

	mClickH = h;
	mClickV = v;
	GetImageBounds(r);
	if (h < r.left || h >= r.right) return 0;
	if (v < r.top || v >= r.bottom) return 0;
	return 1;
}

//	Get the H and V click info

bool	ImageGlyph::GetClickCoord(float* h, float* v)
{
	if (!IsMap())
		return false;
	BRect r;
	GetImageBounds(r);
	*h = mClickH - r.left;
	*v = mClickV - r.top;
	return true;
}

//	Include HSpace and VSpace in sizes

float ImageGlyph::GetWidth()
{
	return  mWidth + (((mHSpace < 0 ? 0 : mHSpace) + GetBorder()) << 1);
}

float ImageGlyph::GetHeight()
{
	return  mHeight + (((mVSpace < 0 ? 0 : mVSpace) + GetBorder()) << 1);
}

//	Set attributes that images understand

void ImageGlyph::SetAttribute(long attributeID, long value, bool isPercentage)
{
	switch (attributeID) {
		case A_ISMAP:	mIsMap = 1;			break;
		case A_WIDTH:	mWidth = value; mHasWidthAttr = true;		break;
		case A_HEIGHT:	mHeight = value; mHasHeightAttr = true;		break;
		case A_BORDER:	mBorder = (value == -1 ? 1 : MAX(value,0));	break;
		case A_ALIGN:	mAlign = value;		break;
		case A_HSPACE:	mHSpace = MAX(value, 0);					break;
		case A_VSPACE:	mVSpace = MAX(value, 0);					break;
		default:		Glyph::SetAttribute(attributeID,value,isPercentage);
	}
}

void ImageGlyph::SetAttributeStr(long attributeID, const char* value)
{
	switch (attributeID) {
		case A_SRC:		mSRC = (const char *)value;	break;
		case A_ALT:		mALT = (const char *)value;	break;
		case A_USEMAP:	mUSEMAP = (const char *)value;	break;
		case A_NAME:	mName = (const char *)value; break;
		default:		Glyph::SetAttributeStr(attributeID,value);
	}
}

short ImageGlyph::GetAlign()
{
	return mAlign;
}

//	Some part of the image was drawn, update it

void ImageGlyph::Invalidate(UpdateRegion &updateRegion)
{
	mUpdate.Invalidate(updateRegion);
}

//
	
bool ImageGlyph::GetUpdate(UpdateRegion &updateRegion)
{
	if (!mPositionIsFinal && KnowSize())
		mPositionIsFinal = true;
		
	if (!mPositionIsFinal)
		return false;
		
	updateRegion = mUpdate;
	mUpdate.Reset();
	
	BRect imR;						// Position update
	GetImageBounds(imR);
	updateRegion.OffsetBy(imR.left,imR.top);
	
	return updateRegion.NonZero();
}

// Inset by mHSpace, mVSpace and border

void ImageGlyph::GetImageBounds(BRect &r)	
{
	GetBounds(&r);
	r.InsetBy((mHSpace < 0 ? 0 : mHSpace) + GetBorder(),(mVSpace < 0 ? 0 : mVSpace) + GetBorder());
}

void ImageGlyph::Hilite(long value, DrawPort *drawPort)
{
	if (!KnowSize()) return;
	DrawBorder(value != 0,drawPort);
}

//	If the image is an anchor, default BORDER to 2

int ImageGlyph::GetBorder()
{
 	int border = mBorder < 0 ? 0 : mBorder;
	if (mIsAnchor && mBorder == -1)
		border = 2;
	return border;
}

//	Draw border around an image is BORDER tag is non-zero

void ImageGlyph::DrawBorder(bool selected, DrawPort *drawPort)
{
	if (!KnowSize()) return;

	int border = GetBorder();
	if (border) {
		BRect r;
		GetImageBounds(r);
		r.InsetBy(-border,-border);
		//move the border to one pixel outside the image
		r.left--;
		r.top--;
		r.bottom--;
		r.right--;
		drawPort->PenSize(border,border);
		if (mIsAnchor)
			drawPort->SetBorderColor(selected);
		else
			drawPort->SetGray(0);
		drawPort->FrameRect(&r);
		drawPort->PenSize(1,1);
		if (mIsAnchor)
			drawPort->SetGray(0);				// Turn off Anchor color
	}
}

//	Check to see if the image is dead

bool ImageGlyph::IsDead()
{
	if (mImageHandle == NULL)
		return mAborted;
	return mImageHandle->IsDead();		
}

void ImageGlyph::Draw(DrawPort *drawPort)
{
	if (!KnowSize()) {
//		DrawWaiting(drawPort);
		return;
	}
	
	mPositionIsFinal = true;

	if (mImageHandle && mBGGlyph && !mHasBGImage && mBGGlyph->mImageHandle->Complete() && !mBGGlyph->mImageHandle->IsDead())
		mImageHandle->SetBackgroundImage(mBGGlyph->mImageHandle, mLeft, mTop);

	DrawBorder(false,drawPort);
	
	if (IsDead()){
		DrawDead(drawPort);			// Draw a red rectangle for an image timeout
		if (mImageHandle) {
			BRect r;
			GetImageBounds(r);			// Destination
			mImageHandle->Draw(drawPort,&r);
		}
	}
	else {
		if (mImageHandle) {
			BRect r;
			GetImageBounds(r);			// Destination
			mImageHandle->Draw(drawPort,&r);
		} else
			DrawWaiting(drawPort);
	}
}

#ifdef DEBUGMENU
void ImageGlyph::PrintStr(BString& print)
{
	SpatialGlyph::PrintStr(print);
	BString name;
	GetName(name);
	if (print.Length() != 0)
		print += " ";
	print += name;
}
#endif

//	Name to use if image is missing

void ImageGlyph::GetName(BString& name)
{
	name = "";
	if (mALT.Length()) {
		name = mALT;
	} else {
		const char *n = mSRC.String();
		while (strchr(n,'/'))
			n = strchr(n,'/') + 1;
		name = n;
	}
}

//	Break text at a particular width

long BreakText(const char* text, int textCount, float width, bool atSpace, DrawPort *drawPort)
{
	long spaceBreak = -1;
	long punctBreak = -1;
	long n;
	
	// Try and break in a space

	n = 0;
	do {
		if (isspace(text[n])) {
			if (drawPort->TextWidth(text,n) >= width)
				break;
			spaceBreak = n + 1;
		}
	} while (++n < textCount);

	if (spaceBreak != -1)
		return spaceBreak;

	// Could not break in a space, try breaking at punctuation
	// Try and break in punctuation.

	n = 0;
	do {
		if (strchr("@!)/-+:;=>?]}",text[n])) {
			if (drawPort->TextWidth(text,n) >= width)
				break;
			punctBreak = n + 1;
		}
	} while (++n < textCount);

	if (punctBreak != -1)
		return punctBreak;

	// Failed to break in space, break anywhere if space break is not forced

	n = 0;
	while (!atSpace && drawPort->TextWidth(text,n) < width)
		if (++n >=  textCount)
			break;

	return n;
}

//	Draw Wrapped text inside the box

void DrawWrappedText(BString& name, BRect& r, DrawPort *drawPort)
{
	float width = (r.right - r.left) - 4;
	if (width < 32)
		return;		// Not if really narrow
		
	short count = name.Length();
	const char* text = name.String();
	float offsetV = 2;
	float bump = drawPort->GetFontAscent() + drawPort->GetFontDescent();
	
	do {
		while (isspace(text[0])) {
			count--;
			text++;
		}
		int i = count;
		if (drawPort->TextWidth(text,count) > width) {
			i = BreakText(text,count,width,true,drawPort);
			if (i == 0)
				i = BreakText(text,count,width,false,drawPort);
		}
		if (i)
			drawPort->DrawText(r.left + 4,r.top + offsetV,text,i,0);
		offsetV += bump;
		text += i;
		count -= i;
	} while (count && (offsetV + bump) < (r.bottom - r.top));
}

//	What to draw when the image is not displayed

void ImageGlyph::DrawAsBox(DrawPort *drawPort, long fgColor, long bgColor)
{
	BRect r;
	GetImageBounds(r);
	drawPort->DrawBevel(&r);

//	Set anchor color if it is an anchor?

	Style style = { 2,0,0,0,0,0,0,0,0,0,0,1,0x000000 };	// Always black
	drawPort->SetStyle(style);

//	Get the name to display instead of the image

	BString name;
	GetName(name);
	if (name.Length() == 0)
		return;

//	Draw in a clearer font?
	
	r.InsetBy(1,1);
	drawPort->SetColor(bgColor);
	drawPort->PaintRect(&r);
	drawPort->SetColor(fgColor);
	DrawWrappedText(name,r,drawPort);
}

void ImageGlyph::DrawWaiting(DrawPort *drawPort)
{
	DrawAsBox(drawPort, drawPort->GetTextColor(), drawPort->GetBGColor());
}

void ImageGlyph::DrawDead(DrawPort *drawPort)
{
	DrawAsBox(drawPort, 0x00000000, 0x00ffffff);
}

void ImageGlyph::SetParent(Glyph *parent)
{
	SpatialGlyph::SetParent(parent);
	
	mHasBGColor = false;
	mHasBGImage = false;
	Glyph *currentGlyph = this;
	do {
		if (currentGlyph->IsDocument()) {
			DocumentGlyph *g = (DocumentGlyph *)currentGlyph;
			mHasBGColor = true;
			mBGColor = g->GetBGColor();
			if (g->HasBGImage()) {
				mBGGlyph = g->GetBGImage();
				if (mImageHandle && mBGGlyph->mImageHandle && mBGGlyph->mImageHandle->Complete() && !mBGGlyph->mImageHandle->IsDead())
					mHasBGImage = true;
			}
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
	if (!mHasBGColor && !mHasBGImage) {
		mHasBGColor = true;
		mBGColor = 0xc0c0c0;
	}
	if (mHasBGImage && mImageHandle && mBGGlyph->mImageHandle) {
		mImageHandle->SetBackgroundImage(mBGGlyph->mImageHandle, mLeft, mTop);
	} else if (mHasBGColor && mImageHandle)
		mImageHandle->SetBackgroundColor(mBGColor);
}

//	Return the source URL of image

const char *ImageGlyph::GetSRC()
{
	return mSRC.String();
}

//	Get URL of map to use for client size image maps

const char *ImageGlyph::GetUSEMAP()
{
	return mUSEMAP.String();
}

//	Create an image resource and start loading image

void ImageGlyph::Load(long docRef)
{
	if (mSRC.Length() == 0) {
		pprint("ImageGlyph::Load: No SRC tag, can't load image");
		return;
	}
	mImageHandle = new ImageHandle(this,mSRC,docRef, mForceCache, mConnectionMgr, mListener, mFirstFrameOnly);
	if (mHasBGColor)
		mImageHandle->SetBackgroundColor(mBGColor);
	mGetSizeLater = KnowSize() == false;
/*
printf("Loaded %s\n", mSRC.String());
	if (mImageHandle->Complete()) {
printf("Invalidating %s %f, %f, %f, %f\n", mSRC.String(), mTop, mLeft, mTop + mHeight, mLeft + mWidth);
		UpdateRegion region;
		region.Invalidate(BRect(mLeft, mTop, mLeft + mWidth, mTop + mHeight));
		Invalidate(region);
		BMessage msg(msg_ViewUpdate);
		mListener->SendMessage(&msg);
	}
*/
}

// Ready for layout if it knows its size

bool ImageGlyph::ReadyForLayout()
{
	if (KnowSize())
		return true;
	else {
		if (mAskedForLayout == 0) {
			mAskedForLayout = system_time();
			return false;
		} else if (mAskedForLayout == -1)
			return true;
		else {
			if  (system_time() > mAskedForLayout + 1000000) {
				// Set mAskedForLayout to a special value so we'll know that we
				// lied and said that we were ready.  We'll have to pay for our sin
				// later and ask the document to relayout.
				mAskedForLayout = -1;
				return true;
			} else
				return false;
		}
	}
}

//	See if incoming data can resolve our size

bool ImageGlyph::KnowSize()
{
	BRect	r;
	
	if (mAborted)
		return true;

	if (mWidth > 0 && mHeight > 0)
		return true;					// We know our size already

	if (mImageHandle == NULL)
		return false;
	
	if (mImageHandle->IsDead())
		return true;

	if (mImageHandle->GetRect(&r) && r.Width() > 0 && r.Height() > 0) {
		if (mWidth == 0)					// We may have defined a width and a height explicitly
			mWidth = r.right - r.left;		// Resource knows our real size
		if (mHeight == 0)
			mHeight = r.bottom - r.top;
			
		if (mAskedForLayout == -1) {
			Glyph *parent = GetParent();
			while (parent && !parent->IsDocument())
				parent = parent->GetParent();
			if (parent) {
				((DocumentGlyph *)parent)->SetNeedsRelayout();
				mLayoutComplete = 0;
			}
		}
		return true;
	}
	return false;
}

//	Stop loading this image
//	Redraw after the abort

bool ImageGlyph::Abort()
{
	pprint("ImageGlyph Abort");
	if (mImageHandle && mImageHandle->IsDead() == false) {
		mImageHandle->Abort();
		return true;
	}
	if (!KnowSize()) {
		mWidth = 16;
		mHeight = 16;
	}
	mAborted = true;
	return false;
}	

bool ImageGlyph::Idle(DrawPort *)
{
//	If the Image is dead

	if (IsDead()) {
		if (mWidth == 0) {	// If it didn't now it size, know it now
			mWidth = 16;	// Measure the width of alternate string?
			mHeight = 16;
		}
		return true;		// Image is complete
	}

	if (mImageHandle == NULL)
		return true;

	KnowSize();
	mImageHandle->Idle();
	return true;	// Image is be complete
}

//	Complete if all the data is in

bool ImageGlyph::GetComplete()
{
	if (mAborted || mImageHandle == NULL)
		return true;									// Probably dead
	return mImageHandle->Complete();
}

bool ImageGlyph::GetResourceComplete()
{
	if (mAborted || mImageHandle == NULL)
		return true;									// Probably dead
	return mImageHandle->ResourceComplete();
}

//	Ignore a floating image until its layout is complete

bool ImageGlyph::IsLayoutComplete()
{
	return mLayoutComplete;
}

//	Try and know size as soon as possible

void ImageGlyph::Layout(DrawPort *)
{
	if (Floating() && mHSpace < 0)
		mHSpace = 3;

	mLayoutComplete = 0;
	mTop = mLeft = 0;
	KnowSize();
}

void ImageGlyph::LayoutComplete(DrawPort *)
{
	mLayoutComplete = 1;
}
