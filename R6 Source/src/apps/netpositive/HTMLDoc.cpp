// ===========================================================================
//	HTMLDoc.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "HTMLDoc.h"
#include "Parser.h"
#include "Builder.h"
#include "ImageGlyph.h"
#include "Selection.h"
#include "Form.h"
#include "HTMLTags.h"
#include "AnchorGlyph.h"
#include "Cache.h"
#include "InputGlyph.h"
#include "Image.h"
#include "BeDrawPort.h"
#include "TextGlyph.h"
#include "NPApp.h"
#include "Protocols.h"
#include "Strings.h"
#include "ObjectGlyph.h"
#ifdef JAVASCRIPT
#include "jseopt.h"
#include "sebrowse.h"
#endif
#include "HTMLView.h"
#ifdef ADFILTER
#include "AdFilter.h"
#endif
#include "MessageWindow.h"

#include <View.h>
#include <malloc.h>
#include <Screen.h>
#include <Autolock.h>
#include <stdio.h>
#include <ctype.h>

#if defined(JSE_DEBUGGABLE)
extern struct debugMe * debugme;
#endif

#ifdef JAVASCRIPT
jsebool JSE_CFUNC FAR_CALL
ContinueFunction(jseContext jsecontext)
{
   #if defined(JSE_DEBUGGABLE)
      if ( NULL != debugme )
      {
         debugmeDebug(jsecontext, &debugme);
         if ( jseQuitFlagged(jsecontext) )
            return False;
      }
   #endif

   jsecontext = jsecontext;  /* to prevent warning about unused variable */
   return True;
}
#endif

FrameItem::FrameItem(
	FrameItem	*jumpBackLeaf)
{
	mJumpBackLeaf = jumpBackLeaf;
	mColValue = 0;
	mColIsPercentage = false;
	mColPosition = 0;
	mRowValue = 0;
	mRowIsPercentage = false;
	mRowPosition = 0;
	mScrolling = kAutoScrolling;
	mMarginWidth = 0;
	mMarginHeight = 0;
	mURL = NULL;
	mName = NULL;
	mFrames = NULL;
	mView = NULL;
	mBorder = true;
	mBorderValue = 1;
}

FrameItem::~FrameItem()
{
	free(mName);
	Reset();
}

void
FrameItem::SetColValue(
	int32	value,
	bool	isPercentage)
{
	mColValue = value;
	mColIsPercentage = isPercentage;
}

int32
FrameItem::ColValue(
	bool	*isPercentage)
{
	*isPercentage = mColIsPercentage;	
	return (mColValue);
}

void
FrameItem::SetRowValue(
	int32	value,
	bool	isPercentage)
{
	mRowValue = value;
	mRowIsPercentage = isPercentage;
}

int32
FrameItem::RowValue(
	bool	*isPercentage)
{
	*isPercentage = mRowIsPercentage;	
	return (mRowValue);
}

void 
FrameItem::SetColPosition(int32 value)
{
	mColPosition = value;
}

int32 
FrameItem::ColPosition()
{
	return mColPosition;
}

void 
FrameItem::SetRowPosition(int32 value)
{
	mRowPosition = value;
}

int32 
FrameItem::RowPosition()
{
	return mRowPosition;
}

void
FrameItem::SetScrolling(
	int32	scroll)
{
	mScrolling = scroll;
}

int32
FrameItem::Scrolling()
{
	return (mScrolling);
}

void
FrameItem::SetMarginWidth(
	int32	width)
{
	mMarginWidth = width;
}

int32
FrameItem::MarginWidth()
{
	return (mMarginWidth);
}

void
FrameItem::SetMarginHeight(
	int32	height)
{
	mMarginHeight = height;
}

int32
FrameItem::MarginHeight()
{
	return (mMarginHeight);
}

void
FrameItem::SetName(
	const char	*name)
{
	free(mName);
	mName = NULL;
	
	if (name != NULL)
		mName = strdup(name);
}

const char*
FrameItem::Name()
{
	return (mName);
}

void
FrameItem::SetURL(
	const char	*url)
{
	Reset();

	if (url != NULL)
		mURL = strdup(url);	
}

const char*
FrameItem::URL()
{
	return (mURL);
}

void FrameItem::SetView(HTMLView *view)
{
	mView = view;
}

HTMLView *FrameItem::View()
{
	return mView;
}

void 
FrameItem::SetBorder(bool border)
{
	mBorder = border;
}

bool 
FrameItem::Border()
{
	return mBorder;
}

void 
FrameItem::SetBorderValue(int32 border)
{
	if(border >= 0)
		mBorderValue = border;
	else
		pprint("Tried to set FrameItem border value to %ld", border);
}

int32 
FrameItem::BorderValue()
{
	return mBorderValue;
}

void
FrameItem::SetFrames(
	FrameList	*frames)
{
	Reset();

	if (frames != NULL)
		mFrames = frames;
}

FrameList*
FrameItem::Frames()
{
	return (mFrames);
}

FrameItem*
FrameItem::JumpBackLeaf()
{
	return (mJumpBackLeaf);
}

void
FrameItem::Reset()
{
	free(mURL);
	mURL = NULL;

	if (mFrames)
		mFrames->Dereference();
	mFrames = NULL;
}

bool FrameItem::operator==(const FrameItem &fi) const
{
	if ((mFrames && !fi.mFrames) || (!mFrames && fi.mFrames))
		return false;
	else if (mFrames && fi.mFrames && !(*mFrames == *fi.mFrames))
		return false;

	return (mColValue			== fi.mColValue &&
			mRowValue			== fi.mRowValue &&
			mColIsPercentage	== fi.mColIsPercentage &&
			mRowIsPercentage	== fi.mRowIsPercentage &&
			mScrolling			== fi.mScrolling &&
			mMarginWidth		== fi.mMarginWidth &&
			mMarginHeight		== fi.mMarginHeight &&
			((!mName && !fi.mName) || 
			 (mName && fi.mName && strcmp(mName, fi.mName) == 0)));
}


bool FrameList::operator==(const FrameList &fl) const
{
	FrameItem	*first1 = (FrameItem *)First();
	FrameItem	*first2 = (FrameItem *)fl.First();
	int32		num1 = first1->Count();
	int32		num2 = first2->Count();
	
	if (num1 != num2)
		return false;
	
	FrameItem *frame1 = first1;
	FrameItem *frame2 = first2;
	
	while (frame1 != NULL && frame2 != NULL) {
		if (!(*frame1==*frame2))
			return false;
		
		frame1 = (FrameItem *)frame1->Next();
		frame2 = (FrameItem *)frame2->Next();
	}
	return true;
}


FrameItem *FrameList::FindFrame(const char *name) const
{
	if (!name)
		return NULL;
		
	FrameItem *result = NULL;
	FrameItem *frame = (FrameItem *)First();

	while (frame != NULL && result == NULL) {
		if (frame->Name() && strcmp(frame->Name(), name) == 0)
			result = frame;
		else if (frame->Frames())
			result = frame->Frames()->FindFrame(name);
		frame = (FrameItem *)frame->Next();
	}
	return result;
}

ExtraPageData::ExtraPageData()
{
	mScrollPos.Set(0.0,0.0);
	mUserValues = 0;
	mPluginData = 0;
}


ExtraPageData::~ExtraPageData()
{
	if (mUserValues)
		delete mUserValues;
	if (mPluginData) {
		while (mPluginData->CountItems()) {
			delete (PluginData *)mPluginData->ItemAt(0);
			mPluginData->RemoveItem((int32)0);
		}
	}
}

void ExtraPageData::SetPluginData(BList *data)
{
	if (mPluginData) {
		while (mPluginData->CountItems()) {
			delete (PluginData *)mPluginData->ItemAt(0);
			mPluginData->RemoveItem((int32)0);
		}
	}
	mPluginData = data;
}

// ===========================================================================

//	Create a new resource->parser->builder->document chain

Document *Document::CreateFromResource(
	ConnectionManager *mgr,
	UResourceImp	*r, 
	DrawPort	*drawPort, 
	long 		docRef, 
	bool 	openAsText, 
	uint32 		encoding, 
	bool 		forceCache,
	CLinkedList	*list,
	BList*		pluginData,
	BMessenger	*workerMessenger,
	const char *subframeName,
	int32		frameNumber,
	Document *parent
#ifdef JAVASCRIPT
	,struct BrowserWindow *bw
#endif
	)
{
//	NP_ASSERT(drawPort != 0);

	Document *d = new(Document);
	if (!d->Lock()) return NULL;
	d->mDocRef = docRef;
	d->SetForceCache(forceCache);
	if (r) {
		d->SetBase(r->GetURL());
		r->RefCount(1);
	}
	d->SetResource(r);
	d->mDrawPort = drawPort;
	d->mConnectionMgr = mgr;
	mgr->Reference();
	d->mWorkerMessenger = workerMessenger;
#ifdef JAVASCRIPT
	d->mBrowserWindow = bw;
	bw->doc = d;
	if (subframeName) {
		d->mIsSubview = true;
		d->mFrameName = subframeName;
		d->mFrameNumber = frameNumber;
	} else
		d->mIsSubview = false;
	d->mParent = parent;
#endif

#ifdef ADFILTER
	AdFilterList *filterList = 0;
	if (gPreferences.FindBool("FilterEnabled")) {
		const char *url = r ? r->GetURL() : "";
		URLParser parser;
		parser.SetURL(url);
		const char *hostname = parser.HostName();
		if (!hostname)
			hostname = "";
		
		filterList = AdFilter::BuildFilterList(hostname);
	}
#endif

	if (r) {
		DocumentBuilder *b = new DocumentBuilder(list, pluginData);
#ifdef ADFILTER
		b->SetDocument(d, filterList);
#else
		b->SetDocument(d, 0);
#endif
		b->SetForceCache(forceCache);
	
		Parser *p = new Parser(b, encoding);	

	//	Parse as text only if its a text/text file
	
		const char *rtype = r->GetContentType();
		if (openAsText || (rtype && (strstr(rtype,"text/plain") || strstr(rtype,"text/text"))))
			p->IgnoreTags();

		p->SetResourceImp(r);
		d->mEncoding = encoding;
		d->mParser = p;		
	
		if (workerMessenger) {
			BMessage msg(msg_AddConsumer);
			msg.AddPointer("Consumer", p);
			workerMessenger->SendMessage(&msg);
		}
	}
	
#ifdef JAVASCRIPT
	if (d->mParent) {
	 	d->mParent->mChildDocs.AddItem(d);
		if (d->mParent->mJavaScriptInitialized) {
			d->mJavaScriptEnabled = true;
			d->InitJavaScript();
		}
	}
#endif

	d->Unlock();
	return d;
}

//====================================================================================
//	As a glyph is added to the page, lay it out and draw it if possible...

DocumentGlyph::DocumentGlyph(Document* htmlDoc, bool isLayer) :
	PageGlyph(htmlDoc)
{
	mClipRect.Set(0,0,0,0);
	mDirty.Set(0,0,0,0);
	mBGImage = 0;
	mPagenateHeight = -1;
	mDrawBackground = true;
	mBGPixels = 0;
	mWeOwnBGPixels = false;
	mNeedsRelayout = false;
	mIsLayer = isLayer;
	mIsVisible = true;
}

DocumentGlyph::~DocumentGlyph()
{
	if (mWeOwnBGPixels)
		delete mBGPixels;
}

bool	DocumentGlyph::IsDocument()				// False
{
	return true;
}

//	Drawing inside these bounds

void DocumentGlyph::SetClipRect(BRect *r)
{
	mClipRect = *r;

	mDirty.left = mDirty.top = 0x7FFFFFFF;		// Reset the dirty rectangle
	mDirty.right = mDirty.bottom = -0x7FFFFFFF;

}

//	Anything get layed out that needs to be drawn?

bool DocumentGlyph::GetDirty(BRect *r)
{
	*r = mDirty;
	return mDirty.left < mDirty.right;
}

//	Invalidate area...

void DocumentGlyph::Invalidate(UpdateRegion& updateRegion)
{
	BRect r;
	updateRegion.GetRect(r);
	pprint("DocumentGlyph::Invalidate %f,%f,%f,%f", r.left, r.top, r.right, r.bottom);
	mDirty.left = MIN(r.left,mDirty.left);
	mDirty.right = MAX(r.right,mDirty.right);
	mDirty.top = MIN(r.top,mDirty.top);
	mDirty.bottom = MAX(r.bottom,mDirty.bottom);
}

//	Draw the background

void DocumentGlyph::DrawBackground(DrawPort *drawPort)
{
	if (!mDrawBackground)
		return;

	BRect r;
	if (mIsLayer)
		r.Set(GetLeft(), GetTop(), GetLeft() + GetWidth(), GetTop() + GetHeight());
	else
		r = mClipRect;
	
	bool isTransparent = mBGImage && mBGImage->IsTransparent();

//	if (!mBGImage) {
		// Sometimes, after printing, the view's low color gets messed up.  Let's reset it.
//		((BeDrawPort*)drawPort)->GetView()->SetLowColor((short)((mBGColor >> 16) & 0xFF),
//														(short)((mBGColor >> 8) & 0xFF),
//														(short)(mBGColor & 0xFF));
//	}
		
	if (mBGImage && (mBGImage->IsDead() || isTransparent)) {
		drawPort->SetColor(drawPort->GetBackColor());
		drawPort->PaintRect(&r);
	}

	if (mBGPixels ||
		(mBGImage && 
		 mBGImage->GetImageHandle() && 
		 mBGImage->GetImageHandle()->Complete() &&
		 mBGImage->GetImageHandle()->GetImage() && 
		 mBGImage->GetImageHandle()->GetImage()->GetPixels() && 
		 mBGImage->IsDead() == false)) {
		long	h,v;
		if (mBGPixels) {
			h = mBGImageWidth;
			v = mBGImageHeight;
		} else {
			BRect bgRect;
			mBGImage->GetImageHandle()->GetRect(&bgRect);
			h = (long)(bgRect.Width());
			v = (long)(bgRect.Height());
		}
		if (!mBGPixels) {
			InflateBGImage(mBGImage->GetImageHandle()->GetImage()->GetPixels(), &h, &v);
			if (!mBGImage->IsTransparent()) {
				if (HTMLView::OffscreenEnabled()) {
// HTMLDoc::Idle already takes care of this.
//					if (mLaidOutVisibleGlyph)
//						((BeDrawPort *)drawPort)->GetView()->Invalidate();
				} else {
					((BeDrawPort *)drawPort)->GetView()->SetViewBitmap(((BePixels *)mBGPixels)->GetBBitmap());
					((BeDrawPort *)drawPort)->GetView()->Invalidate();
					((BeDrawPort *)drawPort)->GetView()->Window()->UpdateIfNeeded();
				}
			}
		}
		if (HTMLView::OffscreenEnabled() || mBGImage->IsTransparent())
		{
			BRect src(0, 0, h, v);
			BRect dst;
			if (h && v) {
				drawPort->SetGray(0);
				
				dst.top = r.top;
				dst.bottom = MIN((((long)(dst.top))/v)*v + v, r.bottom);
				
				src.top = ((long)dst.top) % v;
				src.bottom = dst.bottom - dst.top + src.top;
	
				while (dst.top < r.bottom) {
					dst.left = r.left;
					dst.right = MIN((dst.left/h)*h+h, r.right);
					
					src.left = ((long)dst.left) % h;
					src.right = dst.right - dst.left + src.left;
	
					while (dst.left < r.right) {
						drawPort->DrawPixels(mBGPixels, &src, &dst, !isTransparent);
						dst.left = (((long)(dst.left))/h)*h + h;
						dst.right = MIN(dst.left+h, r.right);
		
						src.left = 0;
						src.right = dst.right - dst.left + src.left;
					}
					dst.top = (((long)(dst.top))/v)*v + v;
					dst.bottom = MIN(dst.top+v, r.bottom);
	
					src.top = 0;
					src.bottom = dst.bottom - dst.top + src.top;
				}
				return;
			}
		}
	}
	else if (HTMLView::OffscreenEnabled() || (mBGImage && !isTransparent))
	{
		drawPort->SetColor(drawPort->GetBackColor());
		drawPort->PaintRect(&r);
	}
}

void DocumentGlyph::InflateBGImage(Pixels *srcPixels, long *w, long *h)
{
	const int kDesiredSize = 64;
	
	if (mBGPixels)
		return;

	long srcW = *w;
	long srcH = *h;
	
	if (srcW >= kDesiredSize && srcH >= kDesiredSize) {
		mBGPixels = srcPixels;
		mWeOwnBGPixels = false;

		// The values passed in for w and h, not the width and height stored in the
		// bitmap, are the true width and height we are interested in.  The JPEG decoder
		// allocates a bitmap slightly larger than the true image size, to allow for some
		// necessary overflow while decoding.  We don't want to draw that overflow.
		// So, let's remeber the original width and height for later.
		mBGImageHeight = srcH;
		mBGImageWidth = srcW;
		return;
	}
	
	int dstHScale = srcW < (kDesiredSize / 2 + 1) ? ((int)(kDesiredSize / srcW)) : 1;
	int dstVScale = srcH < (kDesiredSize / 2 + 1) ? ((int)(kDesiredSize / srcH)) : 1;
	long dstW = srcW * dstHScale;
	long dstH = srcH * dstVScale;
	
	int srcDepth = srcPixels->GetDepth();
	int dstDepth = srcDepth;
	
	pprint("Trying to inflate bg image of %f,%f to %f,%f", srcW, srcH, dstW, dstH);

	mBGPixels = new BePixels(NetPositive::MainScreenColorSpace() == B_COLOR_8_BIT);
	
	if (mBGPixels->Create(dstW, dstH, dstDepth)) {
		mWeOwnBGPixels = true;
		
		uchar *srcAddr = srcPixels->GetBaseAddr();
		uchar *dstAddr = mBGPixels->GetBaseAddr();
		
		long srcRowBytes = srcPixels->GetRowBytes();
		long dstRowBytes = mBGPixels->GetRowBytes();
		
		*w = dstW;
		*h = dstH;

		for (int v = 0; v < dstVScale; v++) {
			for (int vv = 0; vv < srcH; vv++) {
				uchar *dstLineBegin = (uchar *)(dstAddr + (v * srcH + vv) * dstRowBytes);
				uchar *srcLineBegin = (uchar *)(srcAddr + vv * srcRowBytes);
				for (int h = 0; h < dstHScale; h++) {
					long srcWidth = srcW * srcDepth / 8;
					uchar *src = srcLineBegin;
					uchar *dst = dstLineBegin + h * srcWidth;
					memcpy(dst, src, srcWidth);
				}
			}
		}
		mBGImageHeight = dstH;
		mBGImageWidth = dstW;
	} else {
		pprint("Couldn't create BG pixels.  Returning");
		mWeOwnBGPixels = false;
		delete mBGPixels;
		mBGPixels = srcPixels;
		mBGImageHeight = srcH;
		mBGImageWidth = srcW;
	}
}

//	Set background image, reset drawing

void DocumentGlyph::SetBGImage(ImageGlyph *BGImage)
{
	mBGImage = BGImage;
	mDirty.left = mDirty.top = 0;
	mDirty.right = 0x0007FFFF;
	mDirty.bottom = 0x0007FFFF;	// Redraw everything
}

bool DocumentGlyph::HasBGImage()
{
	return (mBGImage != NULL);
}

//	Reset the layout from the beginning (resize perhaps?)

void DocumentGlyph::ResetLayout()
{
pprint("ResetLayout");
	mDirty.left = mDirty.top = 0;
	mDirty.right = mDirty.bottom = 0x0007ffff;
	
	mNextToPut = 0;
	mLastPut = 0;
	mLastLayedOut = 0;
	mOldVPos = 0;
	
	PageGlyph::ResetLayout();
}

void DocumentGlyph::SetNeedsRelayout()
{
	mNeedsRelayout = true;
}

void DocumentGlyph::SetVisibility(bool visible)
{
	mIsVisible = visible;
}

void DocumentGlyph::Layout(DrawPort *drawPort)
{
	if (mNeedsRelayout) {
		mNeedsRelayout = false;
		ResetLayout();
		PageGlyph::ResetLayout();
	}
	
	if (mNextToPut == NULL && mLastPut == NULL && mLastLayedOut != 0)  // Layout already complete
		return;
		
	drawPort->BeginDrawing(NULL);	// Need to start using drawport
	PageGlyph::Layout(drawPort);
	drawPort->EndDrawing();
}

//	Clip the drawing to the mClipRect

void DocumentGlyph::Draw(DrawPort *drawPort)
{
	BWindow *window = ((BeDrawPort *)drawPort)->GetView()->Window();
	if (!window->Lock())
		return;

	drawPort->BeginDrawing(&mClipRect);	// Set clip
	DrawBackground(drawPort);			// Draw Background
	if (mLastLayedOut != NULL)			// Nothing layed out yet
		PageGlyph::Draw(drawPort);		// Draw all the stuff on the page
	drawPort->EndDrawing();				// Unset clip
	window->Unlock();
}

//	Shift line position so a line does not cross a page boundary.
//	If a glyph is taller than a page (a big table, for example),
//	it will align to the top of the first page and flow across subsequent pages

float DocumentGlyph::LayoutLineV(float vPos, float space, short align, Glyph *firstGlyph, Glyph *lastGlyph)
{
	Glyph *glyph;
	float v = PageGlyph::LayoutLineV(vPos,space,align,firstGlyph,lastGlyph);
	if (mPagenateHeight != -1) {
		float pageBreak = ((mVPos/mPagenateHeight) + 1)*mPagenateHeight;
		if (v > pageBreak) {					// Crossed page
			float offset = v - pageBreak;			// Bump line down
			for (glyph = firstGlyph; glyph; glyph = (Glyph *)glyph->Next())
				glyph->SetTop(glyph->GetTop() + offset);
			v += offset;
		}
	}
	
//	Accumulate into dirty rectangle if it intersects with mClipRect	

	float clippedVPos = MIN(vPos, mClipRect.bottom);
		  clippedVPos = MAX(vPos, mClipRect.top);
	float clippedV = MAX(v, mClipRect.top);
		  clippedV = MIN(clippedV, mClipRect.bottom);
//	if (v >= mClipRect.top && vPos <= mClipRect.bottom) {
pprint("LayoutLineV.  %f   %f,%f", v, clippedVPos, clippedV);
		mDirty.top = MIN(mDirty.top,clippedVPos);
		mDirty.bottom = MAX(mDirty.bottom,clippedV);
		for (glyph = firstGlyph; glyph; glyph = (Glyph *)glyph->Next()) {
			float left = glyph->GetLeft();
			float width = glyph->GetWidth();
			mDirty.left = MIN(mDirty.left,left);
			mDirty.right = MAX(mDirty.right,left + width);
			if (glyph == lastGlyph)
				break;
		}
//	}

	
// If we have a large table that extends below the bottom of the clip rect, then
// it's possible that vPos will jump over a large portion of the screen that needs
// to be invalidated.  Compare the vPos passed in with the value the last time it
// was called, and see if we need to add to the invalidation area.
	if (vPos > mOldVPos && mDirty.top > mOldVPos && mOldVPos < mClipRect.bottom) {
		mDirty.top = MIN(mDirty.top, mOldVPos);
		mDirty.top = MAX(mDirty.top, mClipRect.top);
		mDirty.bottom = MAX(mDirty.bottom, vPos);
		mDirty.bottom = MIN(mDirty.bottom, mClipRect.bottom);
		mDirty.left = 0;
		mDirty.right = GetWidth();
	}
	mOldVPos = v;
	
	return v;
}

//	Layout into pages

float DocumentGlyph::Pagenate(float width, float height, float marginWidth)
{
	SetWidth(width - marginWidth * 2);					// gutter
	mPagenateHeight = height;
	ResetLayout();
	Layout(mDrawPort);							// Break into pages...
	mPagenateHeight = -1;
	return (GetHeight() + (height-1))/height;	// Number of pages
}

void
DocumentGlyph::SetDrawBackground(
	bool	draw)
{
	mDrawBackground = draw;
}


// ============================================================================

Document::Document() : mLocker("HTMLDoc locker")
{
	Lock();
	mBGImage = 0;
	mBGImageComplete = 0;
	mBGColor =		0x00ffffff;	// gray 198
	mFGColor =		0x00000000;
	mALinkColor =	0x00ff0000;
	mLinkColor =	0x000000ff;
	mVLinkColor = 	0x0052188c;
	mBGBeginTime = 0;

	mImageList = new BList;
	mAnchorList = new BList;
	mFormList = new BList;

	mDrawPort = 0;
	mTextPool = new(CBucket);

	mActionAnchor = 0;

	mResource = NULL;
	mParser = NULL;

	mRefreshTime = -1;
	mMarginWidth = mMarginHeight = 8;
	
	mRootGlyph = new DocumentGlyph(this, false);
	mRootGlyph->SetBGColor(mBGColor);
	mRootGlyph->SetWidth(600);		// Default width
	mRootGlyph->SetTop(mMarginHeight);
	mRootGlyph->SetLeft(mMarginWidth);
	mRootGlyph->Open();				// Open to accept glyphs

	mSelection = new(Selection);
	mInIdle = false;
	mDocRef = 0;
	
	mMouseAnchor = 0;
	mMouseImage = 0;
	mMouseTrack = HM_NOTHING;
	mShowImages = 1;
	mShowBGImages = true;
	mIsIndex = false;
	mForceCache = false;

	mFramesetLevel = -1;
	mFrames = NULL;
	mFrameLeaf = NULL;
	mDoneWithFrameset = false;
	mFrameBorder = 0;

	mOpen = true;
	mShowToolbar = kToolbarUnspecified;
	
#ifdef JAVASCRIPT
	// Lock the JSE context locker so that no JavaScripts can
	// be run until the script context is set up.
//	mJSEContextLocker.Lock();
	mExecutedFirstScript = false;
	mDocumentVar = NULL;
	mChangedSpoofState = false;
	mJavaScriptEnabled = false;
	mJavaScriptInitialized = false;
	mJSELinkData = NULL;
	mContextWrapper = NULL;
#endif
	
	Unlock();
}

Document::~Document()
{
	check_lock();
	Lock();
	
#ifdef JAVASCRIPT
	KillJSEContext();
#endif

//	delete(mResource);
	if (mResource)
		mResource->RefCount(-1);

	delete(mDrawPort);
	delete(mImageList);	// mRootGlyph will delete the actual image objects
	delete(mAnchorList);

	Form* form;
	for (short i = 0; i < mFormList->CountItems(); i++) {
		form = (Form *)mFormList->ItemAt(i);
		delete(form);
	}
	delete(mFormList);
	
	delete(mRootGlyph);
	delete(mSelection);
	delete(mBGImage);
	
	delete(mTextPool);
	
	while (mLayers.CountItems() > 0) {
		delete (LayerItem *)mLayers.ItemAt(0);
		mLayers.RemoveItem((int32)0);
	}
	
	if (mFrames)
		mFrames->Dereference();
		
	mConnectionMgr->Dereference();

#ifdef JAVASCRIPT
	if (mParent)
	 	mParent->mChildDocs.AddItem(this);
	
	for (int i = 0; i < mChildDocs.CountItems(); i++) {
		Document *doc = (Document *)mChildDocs.ItemAt(i);
		doc->mParent = NULL;
	}
#endif
}

#ifdef DEBUGMENU
void Document::check_lock()
{
	if (!mLocker.IsLocked())
		debugger("HTMLDoc must be locked");
}

void Document::check_window_lock()
{
	if (!((BeDrawPort *)mDrawPort)->GetView()->Window()->IsLocked())
		debugger("Window must be locked.");
}
#endif

bool Document::Lock()
{
/*
	if (!mLocker.Lock()) {
		status_t error = acquire_sem_etc(mLocker.Sem(), 1, B_TIMEOUT, B_INFINITE_TIMEOUT);
		printf("Lock failed!  Error 0x%x  B_OS_ERROR_BASE 0x%x  this 0x%x  semID %d\n", error, B_OS_ERROR_BASE, this, mLocker.Sem());
	}
	return true;
*/
	return mLocker.Lock();
}

bool Document::LockWindow()
{
	if (!mDrawPort || !((BeDrawPort *)mDrawPort)->GetView())
		return false;
	BWindow *window = ((BeDrawPort *)mDrawPort)->GetView()->Window();
	if (!window)
		return false;
	return window->Lock();
}

bool Document::LockDocAndWindow()
{
	if (!mDrawPort || !((BeDrawPort *)mDrawPort)->GetView())
		return false;
	BWindow *window = ((BeDrawPort *)mDrawPort)->GetView()->Window();
	if (!window)
		return false;
	if (!window->Lock())
		return false;
	if (!Lock()) {
		window->Unlock();
		return false;
	}
	return true;
}

void Document::Unlock()
{
#ifdef DEBUGMENU
	if (!(mLocker.IsLocked() && mLocker.LockingThread() == find_thread(NULL)))
		debugger("A different thread released the HTML lock");
#endif
	mLocker.Unlock();
}

void Document::UnlockWindow()
{
	if (!mDrawPort || !((BeDrawPort *)mDrawPort)->GetView())
		return;
	BWindow *window = ((BeDrawPort *)mDrawPort)->GetView()->Window();
	if (window)
		window->Unlock();
}

void Document::UnlockDocAndWindow()
{
	Unlock();
	UnlockWindow();
}

bool Document::IsLocked()
{
	return mLocker.IsLocked();
}

void
Document::SetForceCache(
	bool	forceCache)
{
	check_lock();
	mForceCache = forceCache;
}

#ifdef DEBUGMENU
void Document::Print()
{
	if (mRootGlyph != NULL)
		mRootGlyph->Print(-1);
}
#endif

DrawPort *Document::GetDrawPort()
{
	return mDrawPort;
}

void Document::SetTitle(const char* title)
{
	check_lock();
	mTitle = title;
	mTitle.Truncate(127);
}

const char* Document::GetTitle()
{
	check_lock();
	return mTitle.String();
}

void Document::SetMarginWidth(float width)
{
	check_lock();
	mMarginWidth = width;
	mRootGlyph->SetLeft(mMarginWidth);
}

void Document::SetMarginHeight(float height)
{
	check_lock();
	mMarginHeight = height;
	mRootGlyph->SetTop(mMarginHeight);
}

void Document::ShowImages(int showImages)
{
	check_lock();
	mShowImages = showImages;
}

void Document::ShowBGImages(int showBGImages)
{
	check_lock();
	mShowBGImages = showBGImages;
}

//	Returns true if document is fully parsed

bool	Document::IsParsingComplete()
{
	check_lock();
	return mParser == 0;
}

//	Actual used width of the document

float	Document::GetUsedWidth()
{
	check_lock();
	float width = mRootGlyph->GetUsedWidth();

	for (int i = 0; i < mLayers.CountItems(); i++) {
		DocumentGlyph *layer = ((LayerItem*)mLayers.ItemAt(i))->mDocumentGlyph;
		if (layer->IsVisible())
			width = MAX(width, layer->GetLeft() + layer->GetWidth());
	}
	return width;
}

void Document::SetIsIndex(bool isIndex)
{
	check_lock();
	mIsIndex = isIndex;
}

bool	Document::IsIndex()
{
	check_lock();
	return mIsIndex;
}

void Document::SetResource(UResourceImp* r)
{
	check_lock();
	if (mResource)
		mResource->RefCount(-1);
	mResource = r;
}

UResourceImp* Document::GetResource()
{
	return mResource;
}

//	Base URL is url of resource or from base tag

void Document::SetBase(const char* base)
{
	check_lock();
	mBase.SetURL(base);
}

//	When to refresh document

void Document::SetRefreshTime(int seconds)
{
	check_lock();
	mRefreshTime = seconds;
	mRefreshStart = system_time() / 1000000;
}

//	URL to load at refresh

void Document::SetRefreshURL(const char* url)
{
	check_lock();
	mRefreshURL = "";
	ResolveURL(mRefreshURL,url);
}

//	If time has expired, do refresh

bool Document::GetRefreshURL(BString& url)
{
	check_lock();
	if (mRefreshTime != -1 && mRefreshTime <= (system_time() / 1000000) - mRefreshStart) {
		mRefreshTime = -1;
		mRefreshStart = -1;
		url = mRefreshURL;	// May be null, realod the current doc
		mRefreshURL = "";
		return true;
	}
	return false;
}

void Document::ShowToolbar(bool show)
{
	if (mBase.Scheme() == kFILE)
		mShowToolbar = show ? kShowToolbar : kHideToolbar;
}

// Relsolve URL path

void Document::ResolveURL(BString& URL,const char* partialURL)	
{
	check_lock();

	if (partialURL == NULL || partialURL[0] == '\0') {	// NULL partial url inherits base
		pprint("ResolveURL: partialURL was nil");
		mBase.WriteURL(URL);
		return;
	}

	// Some sites use illegal http:blah.html or http:/dir/dir/blah.html URL's.  MSIE will follow these if
	// they're in a link in a page, but handles them differently if you type them into the location field.
	// To mimic it's behavior, let's special case it in the link handling code.  We'll look to see if the
	// URL begins with http: but doesn't have two slashes; if not, we'll strip it.
	
	if (strncasecmp(partialURL, "http:", 5) == 0 && strncasecmp(partialURL, "http://", 7) != 0)
		partialURL += 5;

	URLParser parser;
	parser.SetURL(partialURL);	
	parser.BuildURL(URL,mBase);
}

void Document::AddBackground(char *background, long textColor, long bgColor, long link, long vlink, long alink)
{
	check_lock();
#ifdef DEBUGMENU
//	if (!((BeDrawPort *)mDrawPort)->GetView()->Window()->IsLocked())
//		debugger("To call Document::Background, you must also acquire a window lock.");
#endif
	mBGColor = bgColor;
	mFGColor = textColor;
	mALinkColor = alink;
	mLinkColor = link;
	mVLinkColor = vlink;
	
	mRootGlyph->SetBGColor(bgColor);
	if (background && *background) {
		BString URL;
		ResolveURL(URL,background);				// Relsolve URL path
		
		ImageGlyph *image = new ImageGlyph(mConnectionMgr, mForceCache, mWorkerMessenger, this, true);
		image->SetAttributeStr(A_SRC,URL.String());
		if (mShowBGImages) {
			image->Load(mDocRef);
			mDrawPort->SetTransparent();	
			mRootGlyph->SetBGImage(image);
		}
		image->SetAttribute(A_HSPACE,0,false);	// No space between background tiles
		image->SetAttribute(A_VSPACE,0,false);
		mBGImage = image;

		mBGImage->Idle(mDrawPort);
		
		if (mBGImage->GetComplete() || mBGImage->GetResourceComplete()) {
			bigtime_t startTime = system_time();
			while (system_time() - startTime < ONE_SECOND &&
				   (mBGImage->Idle(mDrawPort) == false || !mBGImage->GetComplete())) {	// Finish drawing resource into image
				snooze(20000);
			}
			mBGImageComplete = true;			// Background is complete
		}
	}
	if (!background || !mShowBGImages) {
		if (HTMLView::OffscreenEnabled()) {
			if (mReadyForDisplay)
				((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
		} else {
			((BeDrawPort *)mDrawPort)->GetView()->ClearViewBitmap();
			((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
			((BeDrawPort *)mDrawPort)->GetView()->Window()->UpdateIfNeeded();
		}
	}
//	don't do this until BGImage is loaded
	mDrawPort->SetColors(textColor,bgColor,link,vlink,alink);	// 5 important colors port needs to know about
}

void Document::SetALinkColor(long color)
{
	mALinkColor = color;
	mDrawPort->SetColors(mFGColor, mBGColor, mLinkColor, mVLinkColor, mALinkColor);
	if (mReadyForDisplay)
		((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
}


void Document::SetLinkColor(long color)
{
	mLinkColor = color;
	mDrawPort->SetColors(mFGColor, mBGColor, mLinkColor, mVLinkColor, mALinkColor);
	if (mReadyForDisplay)
		((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
}


void Document::SetVLinkColor(long color)
{
	mVLinkColor = color;
	mDrawPort->SetColors(mFGColor, mBGColor, mLinkColor, mVLinkColor, mALinkColor);
	if (mReadyForDisplay)
		((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
}


void Document::SetBGColor(long color)
{
	mBGColor = color;
	mRootGlyph->SetBGColor(color);
	mDrawPort->SetColors(mFGColor, mBGColor, mLinkColor, mVLinkColor, mALinkColor);
	if (mReadyForDisplay)
		((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
}


void Document::SetFGColor(long color)
{
	mFGColor = color;
	mDrawPort->SetColors(mFGColor, mBGColor, mLinkColor, mVLinkColor, mALinkColor);
	if (mReadyForDisplay)
		((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
}

void Document::AddBackgroundSound(const char *src, bool loop)
{
	check_lock();
	ResolveURL(mBGSoundURL, src);
	mBGSoundLoops = loop;
}

void Document::AddObject(ObjectGlyph *object)
{
	check_lock();
	mObjects.AddItem(object);
}

PageGlyph* Document::GetRootGlyph()
{
	check_lock();
	return mRootGlyph;
}

//	Layout root, start loading images

void Document::Finalize()
{
	check_lock();
	mRootGlyph->Close();
	
	if(mRootGlyph->LaidOutVisibleGlyph() == false)
		((BeDrawPort*)mDrawPort)->GetView()->Invalidate();
		
	for (int i = 0; i < mLayers.CountItems(); i++)
		((LayerItem*)mLayers.ItemAt(i))->mDocumentGlyph->Close();
	mOpen = false;
#ifdef JAVASCRIPT
	if (mOnLoadScript.Length()) {
		ExecuteScript(mOnLoadScript.String());
	}
#endif
}

void Document::Layout(float width)
{
	check_lock();
	mRootGlyph->SetWidth(width - mMarginWidth * 2);	// gutter
	mRootGlyph->ResetLayout();
	for (int i = 0; i < mLayers.CountItems(); i++)
		((LayerItem*)mLayers.ItemAt(i))->mDocumentGlyph->ResetLayout();
	mReadyForDisplay = false;
}

void Document::SpacingChanged()
{
	check_lock();
	mRootGlyph->SpacingChanged();
	for (int i = 0; i < mLayers.CountItems(); i++)
		((LayerItem*)mLayers.ItemAt(i))->mDocumentGlyph->SpacingChanged();
}

//	Layout into pages for printing

float Document::Pagenate(float width, float height)
{
	check_lock();
	return mRootGlyph->Pagenate(width,height, mMarginWidth);
}

//	Add text to a pool, pass an offset to the glyph

void Document::AddText(TextGlyph *glyph, Style& style, const char *text,long textCount)
{
	check_lock();
	glyph->SetText(mTextPool,mTextPool->GetCount(),textCount,style);
	mTextPool->AddData(text,textCount);
}

void Document::DoBlink()
{
	check_lock();
	BRect r;
	((BeDrawPort *)mDrawPort)->AdvanceBlinkState();
	for (int i = 0; i < mBlinkGlyphList.CountItems(); i++) {
		((TextGlyph *)mBlinkGlyphList.ItemAt(i))->GetBounds(&r);
		((BeDrawPort *)mDrawPort)->GetView()->Invalidate(r);
	}
}

void Document::GrazeLinks(BList *urlList)
{
	check_lock();
	for (int i = 0; i < mAnchorList->CountItems(); i++) {
		AnchorGlyph *glyph = (AnchorGlyph *)mAnchorList->ItemAt(i);
		BString url;
		ResolveURL(url, glyph->GetHREF());
		URLParser parser;
		parser.SetURL(url.String());
		if (strcmp(parser.HostName(), mBase.HostName()) == 0) {
			bool shouldAdd = true;
			for (int j = 0; j < urlList->CountItems() && shouldAdd; j++) {
				BString *listItem = (BString *)urlList->ItemAt(j);
				if (*listItem == url)
					shouldAdd = false;
			}
			if (shouldAdd)
				urlList->AddItem(new BString(url));
		}
	}
}

void Document::AddScript(const char *script, int32 scriptType)
{
#ifdef JAVASCRIPT
	if (!mJavaScriptEnabled)
		return;
		
	check_lock();
	check_window_lock();
	
	if (!mJavaScriptInitialized)
		InitJavaScript();

	if (!mChangedSpoofState && strstr(script, "NetPositive")) {
		mChangedSpoofState = true;
		SetupGeneralInfo(false);
	}

	switch(scriptType) {
		case A_ONKEYDOWN:
			mOnKeyDownScript = script;
			break;
		case A_ONKEYPRESS:
			mOnKeyPressScript = script;
			break;
		case A_ONKEYUP:
			mOnKeyUpScript = script;
			break;
		case A_ONLOAD:
			mOnLoadScript = script;
			break;
		case A_ONMOUSEDOWN:
			mOnMouseDownScript = script;
			break;
		case A_ONMOUSEUP:
			mOnMouseUpScript = script;
			break;
		case A_ONUNLOAD:
			mOnUnloadScript = script;
			break;
		case -1:
			ExecuteScript(script);
			break;
		default:
			pprint("Bad script type: %ld", scriptType);
			break;
	}

#endif
}

#ifdef JAVASCRIPT
void JSE_CFUNC FAR_CALL browserErrorMessageFunc(jseContext jsecontext,const jsechar *ErrorString);

void Document::Reopen()
{
	check_lock();

	mImageList->MakeEmpty();
	mAnchorList->MakeEmpty();
	
	Form* form;
	for (short i = 0; i < mFormList->CountItems(); i++) {
		form = (Form *)mFormList->ItemAt(i);
		delete(form);
	}
	mFormList->MakeEmpty();
	delete mRootGlyph;
	while (mLayers.CountItems() > 0) {
		delete (LayerItem*)mLayers.ItemAt(0);
		mLayers.RemoveItem((int32)0);
	}
	delete mSelection;
	delete mBGImage;
	delete mTextPool;
	if (mFrames)
		mFrames->Dereference();

	mBGImage = 0;
	mBGImageComplete = 0;
	mBGColor =		0x00ffffff;	// gray 198
	mFGColor =		0x00000000;
	mALinkColor =	0x00ff0000;
	mLinkColor =	0x000000ff;
	mVLinkColor = 	0x0052188c;
	mBGBeginTime = 0;

	mTextPool = new(CBucket);

	mActionAnchor = 0;

	mParser = NULL;

	mRefreshTime = -1;
	mMarginWidth = mMarginHeight = 8;
	
	mRootGlyph = new DocumentGlyph(this, false);
	mRootGlyph->SetBGColor(mBGColor);
	mRootGlyph->SetWidth(600);		// Default width
	mRootGlyph->SetTop(mMarginHeight);
	mRootGlyph->SetLeft(mMarginWidth);
	mRootGlyph->Open();				// Open to accept glyphs

	mSelection = new(Selection);
	mInIdle = false;
	mDocRef = 0;
	
	mMouseAnchor = 0;
	mMouseImage = 0;
	mMouseTrack = HM_NOTHING;
	mShowImages = 1;
	mShowBGImages = true;
	mIsIndex = false;
	mForceCache = false;

	mFramesetLevel = -1;
	mFrames = NULL;
	mFrameLeaf = NULL;
	mDoneWithFrameset = false;
	mOpen = true;

	DocumentBuilder *b = new DocumentBuilder(NULL);
	b->SetDocument(this, NULL);

	Parser *p = new Parser(b, mEncoding);	
	p->SetResourceImp(NULL);
	mParser = p;

}

void Document::InitJavaScript()
{
	if (mJavaScriptInitialized || !mJavaScriptEnabled)
		return;

	check_lock();
	check_window_lock();
	
	if (mIsSubview) {
		mJavaScriptInitialized = true;
		Document *doc = mParent;
		while (doc && doc->mParent)
			doc = doc->mParent;
		if (!doc)
			return;
		doc->mJavaScriptEnabled = true;
		doc->Lock();
		doc->InitJavaScript();
		doc->Unlock();
		mContextWrapper = doc->mContextWrapper;
		mContextWrapper->Reference();
		browserInitWindow(mContextWrapper->mContext, mBrowserWindow, true, mFrameName.String(), mFrameNumber, mParent->mBrowserWindow);
	} else 	if (!mJavaScriptInitialized) {
		mJavaScriptInitialized = true;
		
		jseExternalLinkParameters parms;
		parms.FileFindFunc = NULL;
		parms.PrintErrorFunc = browserErrorMessageFunc;
		parms.MayIContinue = ContinueFunction;
		parms.GetSourceFunc = NULL;
		parms.AppLinkFunc = NULL;
		parms.jseSecureCode = NULL;
		parms.options = jseOptDefault;
		parms.hashTableSize = 0;
		jseContext context = jseInitializeExternalLink(mJSELinkData, &parms, "window", "UKC6-1DCD-7316-9CCB-AC6C-8D47");
		mContextWrapper = new ContextWrapper(context);
		mContextWrapper->mOriginalGlobal = jseGlobalObject(mContextWrapper->mContext);

		InitializeInternalLib_BrowserLib(mContextWrapper->mContext);
#  if defined(JSE_DEBUGGABLE)
	    debugmeInit(mContextWrapper->mContext,"","10.113.215.5");
#  endif
	
		// Give the HTMLDoc a pointer to the BrowserWindow.  It'll need it for initialization.  Don't give it
		// the context, though; wait until initialization is done.  Once it gets both context and BrowserWindow,
		// it will allow pending scripts to run.
	//	mHTMLDoc->SetJSEContext(NULL, mBrowserWindow);
		
		SetupGeneralInfo(true, mContextWrapper->mContext);
	
		const FileTypeSpec *entry = GetCanHandleList();
		while (entry && entry->mimeType && *entry->mimeType) {
			struct SEMimeType type;
			type.description = entry->description;
			type.suffixes = entry->suffixes;
			type.type = entry->mimeType;
			type.next = NULL;
			browserAddMimeType(mContextWrapper->mContext, &type);
			entry++;
		}
		
		// When we support plugins, add them here via browserAddPlugin.
	
		browserInitWindow(mContextWrapper->mContext, mBrowserWindow, false, NULL, 0, NULL);
		
	//	mHTMLDoc->SetJSEContext(mJSEContext, mBrowserWindow);
	//	mJSEContext = NULL;
	//	mJSEContextLocker.Unlock();
	
		for (int i = 0; i < mChildDocs.CountItems(); i++) {
			Document *doc = (Document*)mChildDocs.ItemAt(i);
			doc->Lock();
			doc->mJavaScriptEnabled = true;
			doc->InitJavaScript();
			doc->Unlock();
		}

	}
}

void Document::DeleteVariable(jseVariable what)
{
	if (!mJavaScriptEnabled)
		return;
		
	check_lock();
	check_window_lock();

	if (!mJavaScriptInitialized)
		InitJavaScript();

	BAutolock autolock(mContextWrapper->mLocker);
	jseDestroyVariable(mContextWrapper->mContext, what);
}


void Document::ExecuteScript(const char *script)
{
	if (!mJavaScriptEnabled)
		return;
		
	check_lock();
	check_window_lock();

	if (!mJavaScriptInitialized)
		InitJavaScript();

//	jseInterpret(mJSEContext, NULL, script, NULL, jseNewNone, JSE_INTERPRET_CALL_MAIN,  NULL, NULL);
	if ((!mContextWrapper && !mIsSubview) || !mBrowserWindow)
		return;

	BAutolock autolock(mContextWrapper->mLocker);

	if (!mExecutedFirstScript && mDocumentVar) {
		UpdateObject(mDocumentVar);
		mExecutedFirstScript = true;
	}

	if (!mChangedSpoofState && strstr(script, "NetPositive")) {
		mChangedSpoofState = true;
		SetupGeneralInfo(false);
	}

	jseVariable ret = browserInterpret(mContextWrapper->mContext, mBrowserWindow, script);
	if (ret)
		jseDestroyVariable(mContextWrapper->mContext, ret);
}

void Document::ExecuteScriptFile(const char *path)
{
	// We don't expect the window or doc to be locked when we enter this function.  This
	// function will not return until the script is downloaded and executed.
	
	// However, we will assume that this function isn't called re-entrantly per document.  This is
	// a reasonable assumption since only the parser will call this, and since this function doesn't
	// return until it's done, the same parser can't call it re-entrantly.
	
	StoreStatus status;
	UResourceImp *imp = GetResourceSynchronously(path, &status);
	if (status == kComplete) {
		LockDocAndWindow();
		imp->Lock();
		long size = imp->GetContentLength();
		void *data = imp->GetData(0, size);
		ExecuteScript((char *)data);
		imp->ReleaseData(data, size);
		imp->Unlock(); 
		UnlockDocAndWindow();
	}
	if (imp)
		imp->RefCount(-1);
}

UResourceImp *Document::GetResourceSynchronously(const char *path, StoreStatus *status)
{
	Lock();
	BString fileURL;
	ResolveURL(fileURL, path);
	Unlock();
	
	BString error;
	pprint("Getting resource %s synchronously", fileURL.String());
	UResourceImp *imp = GetUResource(mConnectionMgr, fileURL, 0, error, false, NULL, NULL, NULL, true);
	
	// Okay, this is pretty evil.  We will poll until the resource is complete.  Ideally, we should set ourselves
	// up to receive some sort of notification and do things more asynchronously, but since we can't return
	// from this function until we're done, it makes little difference.
	
	bool complete = false;
	do {
		*status = imp->GetStatus();
		if (*status == kError || *status == kAbort || *status == kComplete)
			complete = true;
		snooze(100000);
	} while (!complete);
	pprint("Got resource, returning");
	return imp;
}

void Document::SetupGeneralInfo(bool spoofARealBrowser, jseContext jsecontext)
{
	if (!mJavaScriptEnabled)
		return;
		
	if (!jsecontext)
		jsecontext = mContextWrapper->mContext;
		
#ifdef NOSSL
	int32 securityBits = 0;
#elif defined(EXPORT)
	int32 securityBits = 40;
#else
	int32 securityBits = 128;
#endif

	BString copyrightString = GetLongVersionString();
	const char *copyrightPos = strstr(copyrightString.String(), "\302\251");
	if (copyrightPos) {
		BString rightSide = copyrightPos + 2;
		copyrightString.Truncate(copyrightPos - copyrightString.String());
		copyrightString += "&copy;";
		copyrightString += rightSide;
	}
	
	if (spoofARealBrowser)
#ifdef LAYERS
		browserGeneralInfo(jsecontext, "Mozilla", "Netscape", "4.0 (Win95, I)", false, securityBits, copyrightString.String());
#else
		browserGeneralInfo(jsecontext, "Mozilla", "Netscape", "3.0 (Win95, I)", false, securityBits, copyrightString.String());
#endif
	else
		browserGeneralInfo(jsecontext, "Mozilla", "NetPositive", GetVersionNumber(), false, securityBits, copyrightString.String());
}


jseVariable Document::AddHandler(jseVariable what, const char *handlerName, const char *script)
{		
	if (!mJavaScriptEnabled)
		return NULL;
		
	check_lock();
	check_window_lock();
	
	if (!mJavaScriptInitialized)
		InitJavaScript();

	if ((!mContextWrapper && !mIsSubview) || !mBrowserWindow)
		return NULL;

	if (mContextWrapper->mLocker.IsLocked()) {
		pprint("Failed to add %s handler\n", handlerName);
		return NULL;
	}

	BAutolock autolock(mContextWrapper->mLocker);		

	if (!mChangedSpoofState && strstr(script, "NetPositive")) {
		mChangedSpoofState = true;
		SetupGeneralInfo(false);
	}

	jseVariable handler = browserEventHandler(mContextWrapper->mContext, handlerName, mBrowserWindow, script, what);
	mHandlerList.AddItem(handler);
	
	return handler;
}

void Document::SetTimeoutThread(struct BrowserTimeout *tm)
{
	mBrowserWindow->mTimeout = tm;
}

extern jseVariable browserWindowObject(jseContext jsecontext,struct BrowserWindow *window);

void Document::ExecuteHandler(jseVariable what, jseVariable func)
{
	if (!mJavaScriptEnabled)
		return;
		
	if (!what || !func)
		return;
				
	// If the JSEContext is already locked by this thread, then this means
	// that we're already executing a script and anything we try to do
	// here will be using the context re-entrantly, which is illegal.
	// In this case, drop the update on the floor.  It may be the
	// legitimate thing to do, since the object may be calling this
	// routine in response to a programatic update, which means that
	// this update call is superfluous.
	if (mContextWrapper->mLocker.IsLocked())
		return;

	check_lock();
	check_window_lock();
	
	if (!mJavaScriptInitialized)
		InitJavaScript();
		
	if ((!mContextWrapper && !mIsSubview) || !mBrowserWindow)
		return;

	BAutolock autolock(mContextWrapper->mLocker);

	if (!mExecutedFirstScript && mDocumentVar) {
		UpdateObject(mDocumentVar);
		mExecutedFirstScript = true;
	}
	
	browserCallFunction(mContextWrapper->mContext, mBrowserWindow, what, func);
}

void Document::UpdateObject(jseVariable what)
{
	if (!mJavaScriptEnabled)
		return;
		
	check_lock();
	check_window_lock();
	
	if (!mContextWrapper || !mBrowserWindow)
		return;
		
	// If the JSEContext is already locked by this thread, then this means
	// that we're already executing a script and anything we try to do
	// here will be using the context re-entrantly, which is illegal.
	// In this case, drop the update on the floor.  It may be the
	// legitimate thing to do, since the object may be calling this
	// routine in response to a programatic update, which means that
	// this update call is superfluous.
	if (mContextWrapper->mLocker.IsLocked())
		return;
		
	BAutolock autolock(mContextWrapper->mLocker);

	browserUpdate(mContextWrapper->mContext, mBrowserWindow, what);
}

void Document::KillJSEContext()
{
	if (!mContextWrapper)
		return;
		
	{
		BAutolock autolock(mContextWrapper->mLocker);
		if (mBrowserWindow->mTimeout)
			browserClearTimeout(mContextWrapper->mContext, mBrowserWindow, mBrowserWindow->mTimeout);
		for (int i = 0; i < mContainerList.CountItems(); i++)
			jseDestroyVariable(mContextWrapper->mContext, mContainerList.ItemAt(i));
		for (int i = 0; i < mHandlerList.CountItems(); i++) {
			jseDestroyVariable(mContextWrapper->mContext, mHandlerList.ItemAt(i));
		}
		browserTermWindow(mContextWrapper->mContext, mBrowserWindow);
	}
	mContextWrapper->Dereference();
	mContextWrapper = NULL;
}

#endif

//	Maintain a list of images, mLoad them immediately!

void Document::AddImage(Glyph *image)
{
	check_lock();
	mImageList->AddItem(image);
	const char *url = ((ImageGlyph *)image)->GetSRC();
	
	BString URL;
	ResolveURL(URL,url);
	image->SetAttributeStr(A_SRC,URL.String());			// Each image has a full URL
	
	if (mShowImages == 0) {			// No images
		((ImageGlyph *)image)->Abort();
		return;
	}
	pprint("Added image %s", URL.String());
	((ImageGlyph*)image)->Load(mDocRef);	// Load immediately
#ifdef JAVASCRIPT
// Need to add the image to the JavaScript representation
	if (mJavaScriptInitialized) {
		check_lock();
		check_window_lock();
			
		BAutolock lock(mContextWrapper->mLocker);		
		mContainerList.AddItem(browserAddImage(mContextWrapper->mContext, (struct BrowserImage *)image, mBrowserWindow));
	}
#endif
}

ImageGlyph* Document::GetNextImage(ImageGlyph *previousImage)
{
	if (mImageList->CountItems() == 0)
		return NULL;
	if (previousImage == NULL)
		return (ImageGlyph *)mImageList->ItemAt(0);
	int32 index = mImageList->IndexOf(previousImage);
	if (index < 0)
		return (ImageGlyph *)mImageList->ItemAt(0);
	if (index == mImageList->CountItems())
		return NULL;
	return (ImageGlyph *)mImageList->ItemAt(index + 1);
}

AnchorGlyph* Document::GetNextAnchor(AnchorGlyph *previousAnchor)
{
	if (mAnchorList->CountItems() == 0)
		return NULL;
	if (previousAnchor == NULL)
		return (AnchorGlyph *)mAnchorList->ItemAt(0);
	int32 index = mAnchorList->IndexOf(previousAnchor);
	if (index < 0)
		return (AnchorGlyph *)mAnchorList->ItemAt(0);
	if (index == mAnchorList->CountItems())
		return NULL;
	return (AnchorGlyph *)mAnchorList->ItemAt(index + 1);
}

void Document::LoadImages()
{
	check_lock();
	bool resetBGImage = false;
	
	if (mBGImage && !mBGImage->GetImageHandle()) {
		mBGBeginTime = 0;
		mBGImageComplete = false;
		mBGImage->Reset();
		mBGImage->Load(mDocRef);
		mDrawPort->SetTransparent();	
		mRootGlyph->SetBGImage(mBGImage);
		resetBGImage = true;
#ifdef LAYERS
#warning Load images for layers
#endif
	}
	
	for (int i = 0; i < mImageList->CountItems(); i++) {
		ImageGlyph *image = (ImageGlyph *)mImageList->ItemAt(i);
		bool resetImage = false;
		if (image && !image->GetImageHandle()) {
			image->Reset();
			image->Load(mDocRef);
			resetImage = true;
		}
		if (image && (resetImage || resetBGImage))
			image->SetParent(image->GetParent());
	}
}

void Document::LoadImage(ImageGlyph *image)
{
	image->Load(mDocRef);
}

void Document::KillImageResources()
{
	check_lock();
	for (int i = 0; i < mImageList->CountItems(); i++) {
		ImageGlyph *image = (ImageGlyph *)mImageList->ItemAt(i);
		UResourceImp *r = UResourceCache::Cached(image->GetSRC(), false);
		if (r) {
			r->MarkForDeath();
		//delete r;
			r->RefCount(-1);
		}
	}
}

//	Maintain a list of anchors
//	Build a full url from a fragment and the documents path
//	Lookup URL and set visited flag if we have been there before (in cache log)

void Document::AddAnchor(Glyph *glyph)
{
	check_lock();
	const char	*url = 0;
	
	AnchorGlyph *anchor = (AnchorGlyph*) glyph;

	mAnchorList->AddItem(anchor);		// Keep a list for clicking, etc

	url = (anchor)->GetHREF();
	if (url == NULL) {
		if ((anchor)->GetName() == NULL)
			pprintBig("Fragment target name is nil");
		return;
	}
	if (url[0] == '#')					// Target is a fragment in this file
		return;
		
	BString URL;
	ResolveURL(URL,url);
	(anchor)->SetVisited(UResourceCache::HasBeenVisited(URL.String()));

	anchor->SetAttributeStr(A_HREF,URL.String());	// Each anchor has a full URL
#ifdef JAVASCRIPT
// Need to add the anchor to the JavaScript representation	
	if (mJavaScriptInitialized) {
		check_lock();
		check_window_lock();

		jseVariable container;
		{
			BAutolock lock(mContextWrapper->mLocker);
			container = browserAddAnchor(mContextWrapper->mContext, anchor->GetLocation(), mBrowserWindow);
			mContainerList.AddItem(container);
		}
		anchor->SetContainer(container);		
	}
#endif
}

//	Maintain a list of Forms

void Document::AddForm(Form *form)
{
	check_lock();
	mFormList->AddItem(form);
}

void Document::FormFinished(Form *form)
{
#ifdef JAVASCRIPT
	if (mJavaScriptInitialized) {
		check_lock();
		check_window_lock();

		jseVariable container;
		{
			BAutolock lock(mContextWrapper->mLocker);
			container = browserAddForm(mContextWrapper->mContext, (struct BrowserForm *)form, mBrowserWindow);
			mContainerList.AddItem(container);
		}
		form->SetContainer(container);
	}
#endif
}

Form* Document::GetNextForm(Form *previousForm)
{
	if (mFormList->CountItems() == 0)
		return NULL;
	if (previousForm == NULL)
		return (Form *)mFormList->ItemAt(0);
	int32 index = mFormList->IndexOf(previousForm);
	if (index < 0)
		return (Form *)mFormList->ItemAt(0);
	if (index == mFormList->CountItems())
		return NULL;
	return (Form *)mFormList->ItemAt(index + 1);
}

//	Maintain a list of image maps, if any

ImageMap* Document::AddImageMap(char *name)
{
	check_lock();
	ImageMap* imageMap = new ImageMap(name);
	mImageMapList.Add(imageMap);
	return imageMap;
}

//	Only draw background when scrolling if it is an image

void Document::Draw(BRect *r, bool back)
{
//pprint("Document::Draw %f,%f,%f,%f", r->top,r->left,r->bottom,r->right);
	check_lock();

	// If we were waiting for the first spatial glyph before invalidating the page, then it's
	// time to stop waiting now.  Even though we didn't get the glyph, we were forced to redraw
	// part of the screen, and it will be in a different background.  We won't set the ReadyForDisplay
	// bit, though, since other things may rely on it being correct.
	if (HTMLView::OffscreenEnabled() && !mReadyForDisplay && mRootGlyph->LaidOutVisibleGlyph()) {
//		mReadyForDisplay = true;
//		((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
	}

	mRootGlyph->SetClipRect(r);
	mRootGlyph->SetDrawBackground(back);
	mRootGlyph->Draw(mDrawPort);	// Draw everything that has been layed out

	for (int i = 0; i < mLayers.CountItems(); i++) {
		DocumentGlyph *layer = ((LayerItem *)mLayers.ItemAt(i))->mDocumentGlyph;
		if (layer->IsVisible()) {
			layer->SetClipRect(r);
			layer->SetDrawBackground(back);
			layer->Draw(mDrawPort);
		}
	}

	mSelection->Draw(mDrawPort);	// Draw the selection when we are done...
}


void Document::DrawBackground(BRect *r)
{
	check_lock();
	mRootGlyph->SetClipRect(r);
	mRootGlyph->DrawBackground(mDrawPort);	// Will draw the background
	
	for (int i = 0; i < mLayers.CountItems(); i++) {
		DocumentGlyph *layer = ((LayerItem*)mLayers.ItemAt(i))->mDocumentGlyph;
		if (layer->IsVisible()) {
			layer->SetClipRect(r);
			layer->DrawBackground(mDrawPort);
		}
	}
}

float Document::GetHeight()
{
	check_lock();
	float height = mRootGlyph->GetHeight() + mMarginHeight * 2;

	for (int i = 0; i < mLayers.CountItems(); i++) {
		DocumentGlyph *layer = ((LayerItem*)mLayers.ItemAt(i))->mDocumentGlyph;
		if (layer->IsVisible())
			height = MAX(height, layer->GetTop() + layer->GetHeight());
	}
	return height;
}

// ============================================================================
//	Selecting anchors and posting forms

//	Click in an anchor

AnchorGlyph* Document::GetAnchor(BString& url, BString& target, int& showTitleInToolbar)
{
	check_lock();
	url = "";
	target = "";

	if (mActionAnchor == 0) 
		return NULL;

	mActionAnchor->GetURL(url,target,showTitleInToolbar,(ImageMap *)mImageMapList.First());

	AnchorGlyph *retval = mActionAnchor;
	
	mActionAnchor = 0;
	
	return retval;
}

// ============================================================================
//	Text Selection

void Document::SelectText(long start,long length)
{
	check_lock();
	mSelection->SelectText(start,length,mDrawPort,mRootGlyph);
#ifdef LAYERS
#warning Account for layers
#endif
}

bool Document::GetSelectedText(long *start,long *length)
{
	check_lock();
	return mSelection->GetSelectedText(start,length);
}

long Document::GetTextLength()
{
	check_lock();
	return mTextPool->GetCount();
}

char *Document::GetTextPool()
{
	check_lock();
	return (char*)mTextPool->GetData();
}

char *Document::GetFormattedSelection(long *length)
{
	check_lock();
#ifdef LAYERS
#warning Account for layers
#endif
	return mSelection->GetFormattedSelection(length,mRootGlyph);
}

// ============================================================================

float Document::GetCurrentPosition(BRect *r)
{
	check_lock();
	Glyph *g;
	float i = 0;
	float v = r->top + (r->bottom - r->top)/3;	// Focus 1/3 the way down the page
	
#ifdef LAYERS
#warning Account for layers
#endif
	for (g = (Glyph *)mRootGlyph->First(); g; g = (Glyph *)g->Next()) {
		if (g->GetTop() && g->GetTop() >= v)
			return i;
		i++;
	}
	return -1;	// Can't get current position!
}

//	Turn a position in to pixel ordinate

float Document::GetPositionTop(float position)
{
	Glyph *g;
	float i = 0;
#ifdef LAYERS
#warning Account for layers
#endif
	for (g = (Glyph *)mRootGlyph->First(); g; g = (Glyph *)g->Next()) {
		if (i >= position && g->GetTop())
			return g->GetTop();
		i++;
	}
	return -1;	// Can't find this position!
}

//	Return to of selection, if any

float Document::GetSelectionTop()
{
	check_lock();
	return mSelection->GetSelectionTop();
}

float Document::GetSelectionLeft()
{
	check_lock();
	return mSelection->GetSelectionLeft();
}

//	Return the top of a fragment anchor

float Document::GetFragmentTop(char *fragment)
{
	check_lock();
	for (short i = 0; i < mAnchorList->CountItems(); i++) {
		AnchorGlyph *a = (AnchorGlyph *)(mAnchorList->ItemAt(i));
		const char *s = a->GetName();
		if (s && !strcmp(fragment,s))
			return a->GetTop();
	}
	return -1;
}

// ============================================================================
//	Form clicking

// ============================================================================
//	Find which anchor the mouse is pointing to, if any

AnchorGlyph *Document::MouseInAnchor(float h, float v)
{
	check_lock();
	AnchorGlyph *a;
	for (int32 i = 0; i < mAnchorList->CountItems(); i++) {
		a = static_cast<AnchorGlyph *>(mAnchorList->ItemAt(i));
		if(!a)
			return 0;
		if (a->Clicked(h,v))
			return a;
	}
	return 0;
}

ImageGlyph *Document::MouseInImage(float h, float v)
{
	check_lock();
	ImageGlyph *g;
	for (short i = 0; i < mImageList->CountItems(); i++) {
		g = (ImageGlyph*)(mImageList->ItemAt(i));
		if (g->Clicked(h,v)) {
			return g;
		}
	}
	return 0;
}

//	Check for Click in the anchors, forms or selection of glyphs

HTMLMessage Document::MouseDown(float h, float v, bool isRightClick)
{
	check_lock();
	mMouseTrack = HM_NOTHING;
	
	AnchorGlyph *a;
	ImageGlyph *g;
	if ((bool)(a = MouseInAnchor(h,v))) {
		mMouseAnchor = a;
		mMouseAnchor->Hilite(1,mDrawPort);	// Select it
		mMouseTrack = HM_TRACKANCHOR;		// Track an anchor click
#ifdef LAYERS
#warning Account for layers
#endif
	} else if (mSelection->MouseDown(h,v,mDrawPort,mRootGlyph))
		mMouseTrack = HM_TRACKSELECTION;	// Track a selection click
	if ((bool)(g = MouseInImage(h,v)) && isRightClick) {	// Only care about right-click in normal images; otherwise,
		mMouseImage = g;								// the selection code handles it.
		if (mMouseTrack == HM_TRACKANCHOR)
			mMouseTrack = HM_TRACKIMG_OR_ANC;
		else
			mMouseTrack = HM_TRACKIMAGE;
	}
	return mMouseTrack;
}

HTMLMessage Document::MouseMove(float h, float v)
{
	check_lock();
	switch (mMouseTrack) {
		case HM_NOTHING:
			if ((bool)(mMouseAnchor = MouseInAnchor(h,v))) {
				mMouseTrack = HM_OVERANCHOR;
				// If the anchor has a MouseEnter script, then the script is responsible for
				// setting the status information.  Return HM_NOTHING so that the HTMLView
				// won't do it.
				if (mMouseAnchor->MouseEnter())
					return HM_NOTHING;
				else
					return HM_ONTOANCHOR;
			}
			return HM_NOTHING;
			
		case HM_OVERANCHOR: {
			AnchorGlyph *oldAnchor = mMouseAnchor;
			
			if (mMouseAnchor->Clicked(h,v))				// Still over anchor
				if (!mMouseAnchor->IsImageMap())		// Show coordinates inside image map....
					return HM_NOTHING;
					
			if ((bool)(mMouseAnchor = MouseInAnchor(h,v))) {	// Did it move onto something?
				bool hasEnterScript = false;
				if (mMouseAnchor != oldAnchor) {
					if (oldAnchor)
						oldAnchor->MouseLeave();
					hasEnterScript = mMouseAnchor->MouseEnter();
				}
				mMouseTrack = HM_OVERANCHOR;
				if (hasEnterScript)
					return HM_NOTHING;
				else
					return HM_ONTOANCHOR;
			}
			if (oldAnchor)
				oldAnchor->MouseLeave();
			mMouseAnchor = 0;
			mMouseTrack = HM_NOTHING;			// Just Moved off
			return HM_OFFANCHOR;
		}
		case HM_TRACKANCHOR:
			mMouseAnchor->Hilite(mMouseAnchor->Clicked(h,v),mDrawPort);	// Has Hilite changed?
			break;
		case HM_TRACKSELECTION:
#ifdef LAYERS
#warning Account for layers
#endif
			mSelection->MouseMove(h,v,mDrawPort,mRootGlyph);
			break;

		default:
			break;
	}
	return mMouseTrack;
}

HTMLMessage Document::MouseUp(float h, float v)
{
	check_lock();
	switch (mMouseTrack) {
		case HM_TRACKANCHOR:
			mMouseTrack = HM_NOTHING;
			if (!mMouseAnchor->Clicked(h,v))
				return HM_NOTHING;				// Released outside anchor
				
			mActionAnchor = mMouseAnchor;
			mMouseAnchor->Hilite(0,mDrawPort);	// Released inside anchor
			return HM_ANCHOR;

		case HM_TRACKSELECTION:
#ifdef LAYERS
#warning Account for layers
#endif
			mSelection->MouseUp(h,v,mDrawPort,mRootGlyph);
			mMouseTrack = HM_NOTHING;
			return HM_SELECTION;

		default:
			mMouseTrack = HM_NOTHING;
			return HM_NOTHING;
	}
	mMouseTrack = HM_NOTHING;
	return mMouseTrack;
}

//	Return the anchor mouse is pointing to

bool Document::MouseURL(BString& url, bool ignoreAnchor, BRect *anchorBounds)
{
	check_lock();
	url = "";
	if (anchorBounds && mMouseAnchor)
		mMouseAnchor->GetAnchorBounds(anchorBounds);
	if (mMouseAnchor && !ignoreAnchor) {
		BString target;
		int showTitleInToolbar;
		return mMouseAnchor->GetURL(url,target,showTitleInToolbar,(ImageMap *)mImageMapList.First());
	} else if (mMouseImage) {
		const char *src = mMouseImage->GetSRC();
		if (src) {
			if(anchorBounds)
				mMouseImage->GetImageBounds(*anchorBounds);
			if(ignoreAnchor)
				url = src;
			return true;
		} else
			return false;
	}
	return false;
}

//	Abort the loading of images

HTMLMessage Document::Stop(BRect* r)
{
	check_lock();
	ImageGlyph *image;
	
#ifdef LAYERS
#warning Account for layers
#endif
	mRootGlyph->SetClipRect(r);		// Start accumulating dirty rectangle

	for (int i = 0; i < mImageList->CountItems(); i++) {
		image = (ImageGlyph *)mImageList->ItemAt(i);
		if (image->IsDead() == false)
		
			if (image->Abort()) {	// Redraw if required
				BRect imageR;
				image->GetBounds(&imageR);
				if (imageR.Intersects(*r)) {
					UpdateRegion update;
					update.Invalidate(imageR);
					mRootGlyph->Invalidate(update);
				}
			}
	}
	
	if (mRootGlyph->GetDirty(r))
		return HM_UPDATE;
	return HM_NOTHING;
}

// ============================================================================
//	Idle images and text fields
//	If Idle returns HM_UPDATE, r is set to rectangle to be updated

HTMLMessage Document::Idle(BRect *r)
{
	check_lock();
	check_window_lock();
	
	HTMLMessage message = HM_NOTHING;

	if (mInIdle)
		return HM_NOTHING;
	mInIdle = true;

	mRootGlyph->SetClipRect(r);		// Start accumulating dirty rectangle
	
//	Layout if there is anything to layout

	mRootGlyph->Layout(mDrawPort);
	
	for (int i = 0; i < mLayers.CountItems(); i++) {
		DocumentGlyph *layer = ((LayerItem*)mLayers.ItemAt(i))->mDocumentGlyph;
		layer->SetClipRect(r);
		layer->Layout(mDrawPort);
	}

	if (HTMLView::OffscreenEnabled() && !mReadyForDisplay && mRootGlyph->LaidOutVisibleGlyph()) {
//		mReadyForDisplay = true;
//		((BeDrawPort *)mDrawPort)->GetView()->Invalidate();
	}

// 	Redraw if background image is complete

#ifdef LAYERS
#warning Account for layers
#endif
	if (mBGImage && !mBGImageComplete) {
		mBGImage->Idle(mDrawPort);
		if (mBGImage->GetComplete()) {
			pprintBig("Background is complete");
			mRootGlyph->SetBGImage(mBGImage);	// Draw from scratch 'cause background showed up
			mBGImageComplete = true;			// Background is complete
		} else if (mBGImage) {
			if (mBGBeginTime == 0) {
				mBGBeginTime = system_time();
				message = HM_IDLEAGAIN;
			} else {
				bigtime_t delta = system_time() - mBGBeginTime;
				if (delta < 500000) {
					// Give the background image half a second to establish itself.  If it
					// doesn't, then continue on without it.
					message = HM_IDLEAGAIN;
				} else if (mBGBeginTime > 0 && delta >= 500000) {
					mBGBeginTime = -1;
					mRootGlyph->SetBGImage(mBGImage);
					message = HM_UPDATEANDIDLEAGAIN;					
				}
			}
		}
	}

//	Tickle images so they can establish their sizes

	ImageGlyph *image;
	for (short i = 0; i < mImageList->CountItems(); i++) {
		image = (ImageGlyph *)mImageList->ItemAt(i);
		image->Idle(mDrawPort);
	}
	
//	Draw images if they need an update

	for (short i = 0; i < mImageList->CountItems(); i++) {
		image = (ImageGlyph *)mImageList->ItemAt(i);
		UpdateRegion update;

		// IsPositionFinal will be set to true for the image the first time
		// it is drawn.  If it is called from the normal draw loop and not from
		// here, then that means its location is firmly set.  Until it is firmly
		// set, though, we should not be trying to draw it here.
		if (image->IsPositionFinal() && image->GetUpdate(update)) {
			BRect updateR;
			update.GetRect(updateR);
			if (updateR.Intersects(*r))
				image->Draw(mDrawPort);
// This causes flicker of GIF animations.  Why did I do this in the first place?
//				((BeDrawPort*)mDrawPort)->GetView()->Invalidate(updateR);
		}
	}

//	Return and Draw what was just layed out, if anything

	BRect layerDirtyRect;
	bool isDirty = false;
	for (int i = 0; i < mLayers.CountItems(); i++) {
		DocumentGlyph *layer = ((LayerItem*)mLayers.ItemAt(i))->mDocumentGlyph;
		if (layer->IsVisible()) {
			BRect layerR;
			bool layerDirty = layer->GetDirty(&layerR);
			if (layerDirty)
				layerDirtyRect = layerDirtyRect | layerR;
			isDirty |= layerDirty;
		}
	}

	if (message != HM_IDLEAGAIN && (isDirty || mRootGlyph->GetDirty(r))) {
		if (isDirty)
			*r = *r | layerDirtyRect;

		// If this is the first time we're drawing the page, and we actually have some content
		// to draw, make sure we invalidate the whole page so the whole background gets
		// redrawn.
		if (HTMLView::OffscreenEnabled() && !mReadyForDisplay && mRootGlyph->LaidOutVisibleGlyph()) {
			mReadyForDisplay = true;
			r->left = r->top = 0;
			r->bottom = r->right= 0x7fffffff;
		}
		pprint("dirtyR: %f %f %f %f",r->left,r->top,r->right,r->bottom);
		mInIdle = false;
		return HM_UPDATE;
	}
	
	if (!mRootGlyph->GetDirty(r) && !isDirty && !mRootGlyph->ReadyForLayout()) {
		// We likely have some stubborn images that aren't loaded yet,
		// even though the rest of the document is laid out.  Make sure
		// we get called again to check on the image status.
		pprint("Still not done with layout");
		message = HM_IDLEAGAIN;
	}

	mInIdle = false;
	return message;
}

//	See if the document is the target for keystrokes

bool Document::IsTarget()
{
	check_lock();
	for (short i = 0; i < mFormList->CountItems(); i++)
		if (((Form *)mFormList->ItemAt(i))->IsTarget())
			return true;
	return false;
}

//	User clicked in URL Text, untarget forms...

void Document::Untarget()
{
	check_lock();
	for (short i = 0; i < mFormList->CountItems(); i++)
		((Form *)mFormList->ItemAt(i))->Untarget(mDrawPort);
}

CLinkedList*
Document::UserValues()
{
	check_lock();
	CLinkedList *list = NULL;

	for (int32 f = 0; f < mFormList->CountItems(); f++) {
		Form *form = (Form *)mFormList->ItemAt(f);

		for (int32 i = 0 ; i < form->CountInput(); i++) {

			if (list == NULL)
				list = new CLinkedList();
			list->Add(form->GetInput(i)->UserValues());
		}
	}

	return (list);
}


void
Document::OpenFrameset(
	const char	*rowDims,
	const char	*colDims,
	int32		border)
{
	check_lock();
	if (mDoneWithFrameset)
		CloseFrameset();
	FrameList	*theFrames = new FrameList(mResource->GetURL());
	FrameItem	*jumpBackLeaf = (mFrameLeaf != NULL) ? (FrameItem *)mFrameLeaf->Next() : NULL;
	mFrameBorder = border;
	
	//Note: we assume the same number in xList and xPercentList (and will crash if it isn't)
	BList colList; //holds the dimensions n or -n for n*
	BList colPercentList; //determines whether a dimension in colList is a percentage or not
	BList rowList;//holds the dimensions n or -n for n*
	BList rowPercentList;//determines whether a dimension in rowList is a percentage or not

	
	if(colDims) //set up any column dimensions
		MakeListFromDims(&colList, &colPercentList, colDims);
	if(colList.CountItems() == 0){ //if there were no columns supplied, default to full height
		colList.AddItem(new int(-1));
		colPercentList.AddItem(new bool(false));
	}
	if(rowDims)//set up any row dimensions
		MakeListFromDims(&rowList, &rowPercentList, rowDims);
	if(rowList.CountItems() == 0){ //if there were no rows supplied, default to full width
		rowList.AddItem(new int(-1));
		rowPercentList.AddItem(new bool(false));
	}
	for(int i=0; i<rowList.CountItems(); ++i){
		bool *rowIsPercent = static_cast<bool *>(rowPercentList.ItemAt(i));
		int32 *rowDimValue = static_cast<int32 *>(rowList.ItemAt(i));
		for(int j=0; j<colList.CountItems(); ++j){
			bool *colIsPercent = static_cast<bool *>(colPercentList.ItemAt(j));
			int32 *colDimValue = static_cast<int32 *>(colList.ItemAt(j));
			FrameItem *theItem = new FrameItem(jumpBackLeaf);
			theItem->SetColValue(*colDimValue, *colIsPercent);
			theItem->SetColPosition(j);
			theItem->SetRowValue(*rowDimValue, *rowIsPercent);
			theItem->SetRowPosition(i);
			theFrames->Add(theItem);
		}
	}

	//clean up after ourselves
	for(int i=rowList.CountItems() - 1; i >= 0; --i){
		int32 *deadInt = static_cast<int32 *>(rowList.RemoveItem(i));
		delete deadInt;
		bool *deadBool = static_cast<bool *>(rowPercentList.RemoveItem(i));
		delete deadBool;
	}
	for(int i=colList.CountItems() - 1; i >= 0; --i){
		int32 *deadInt = static_cast<int32 *>(colList.RemoveItem(i));
		delete deadInt;
		bool *deadBool = static_cast<bool *>(colPercentList.RemoveItem(i));
		delete deadBool;
	}
	
	if (theFrames->First() == NULL) {
		// oops, frameset didn't have any frames in it!
		if (theFrames)
			theFrames->Dereference();
		return;
	}

	if (mFrames == NULL)
		mFrames = theFrames;
	else if (mFrameLeaf)
		mFrameLeaf->SetFrames(theFrames);
	mFrameLeaf = (FrameItem *)theFrames->First();		

	if (mFramesetLevel == -1)
		mFramesetLevel = 1;
	else
		mFramesetLevel++;
}

void 
Document::MakeListFromDims(
	BList *dimList,
	BList *percentList,
	const char *dims)
{
	if(!dimList || !percentList ||!dims){
		pprint("MakeListFromDims passed bad parameters");
		return;
	}
	for (const char *p = dims; *p != '\0'; ) {
		if ((isspace(*p)) || (*p == ',')){
			if(*p != '\0')
				p++;
			continue;
		}
		
		//theItem->Value = n where n < 0 in the case of n*,
		//or n > 0 in the case of relative values
		int32 theValue = 0;
		const char *pp = p;
		bool hasSplat = false;
		while (*pp && !(isspace(*pp) || (*pp) == ',')) {
			if (*pp == '*'){
				hasSplat = true;
				if(*p != '*'){ // this sets up the value of -nn for nn*, which is handled in the HTMLView SetupFrameRects
					if (sscanf(p, "%ld", &theValue) == 1) {
						char numStr[9];
						*numStr = 0;
						sprintf(numStr, "%ld", theValue);
						theValue *= -1;
						p += strlen(numStr);
						p--;
					}
				}
				else
					theValue = -1;
			}			
			pp++;
		}
		if (hasSplat){
			int32 *dimValue = new int32(theValue);
			dimList->AddItem(dimValue);
			bool *isPercent = new bool(false);
			percentList->AddItem(isPercent);
			p = pp;
		}
		else {
			if (sscanf(p, "%ld", &theValue) == 1) {
				char numStr[9];
				*numStr = 0;
				sprintf(numStr, "%ld", theValue);
				p += strlen(numStr);

				int32 *dimValue = new int32(theValue);
				dimList->AddItem(dimValue);
				bool *isPercent = new bool(*p == '%');
				percentList->AddItem(isPercent);

				p--;
			}
		}
		if(*p != '\0')
			p++;
	}
}

void
Document::CloseFrameset()
{
	check_lock();
	mDoneWithFrameset = false;
	if (mFrames != NULL) {
		if (mFramesetLevel > 0)
			mFramesetLevel--;	
		mFrameLeaf = (mFrameLeaf != NULL) ? mFrameLeaf->JumpBackLeaf() : NULL;
	}
}

void Document::OpenLayer(DocumentGlyph *glyph, int32 left, int32 top, int32 width, int32 height, int32 zIndex,
						  int32 visibility, const char *id, const char *src,
						  const char *above, const char *below, const char *clip,
						  long bgColor, const char *background)
{
	glyph->SetBGColor(bgColor);
	glyph->SetWidth(width);		// Default width
	glyph->SetTop(top);
	glyph->SetLeft(left);
	glyph->SetHeight(height);
	if (visibility == AV_HIDDEN || visibility == AV_HIDE)
		glyph->SetVisibility(false);
	else if (visibility == AV_INHERIT)
		;
	else
		glyph->SetVisibility(true);
	if (mShowBGImages && background && *background) {
		ImageGlyph *image = new ImageGlyph(mConnectionMgr, mForceCache, mWorkerMessenger, this, true);
		image->SetAttributeStr(A_SRC,background);
		image->Load(mDocRef);
//		mDrawPort->SetTransparent();	
		glyph->SetBGImage(image);
		image->SetAttribute(A_HSPACE,0,false);	// No space between background tiles
		image->SetAttribute(A_VSPACE,0,false);
		image->Idle(mDrawPort);
	}
	
	
	glyph->Open();				// Open to accept glyphs
	
	LayerItem *item = new LayerItem;
	item->mDocumentGlyph = glyph;	
	item->mDocument = this;
	item->mLeft = left;
	item->mTop = top;
	item->mWidth = width;
	item->mHeight = height;
	item->mZIndex = zIndex;
	item->mVisibility = visibility;
	item->mName = id;
	item->mSRC = src;
	item->mAbove = above;
	item->mBelow = below;
	item->mClip = clip;
	item->mBGColor = bgColor;
	item->mBackground = background;
	mCurrentLayer = item;
	
	mLayers.AddItem(item);
}

void Document::CloseLayer()
{
	mCurrentLayer->mDocumentGlyph->Close();
	mCurrentLayer = NULL;

#ifdef JAVASCRIPT
	// We'll assume that if the page uses layers, it has JavaScript.
	InitJavaScript();
	
	check_lock();
	check_window_lock();
		
	BAutolock lock(mContextWrapper->mLocker);		
	mContainerList.AddItem(browserAddLayer(mContextWrapper->mContext, (struct BrowserLayer *)mCurrentLayer, mBrowserWindow));
#endif
}

#ifdef JAVASCRIPT
void Document::UpdateLayer(LayerItem *layer)
{
	BRect rect;
	rect.left = layer->mDocumentGlyph->GetLeft();
	rect.top = layer->mDocumentGlyph->GetTop();
	rect.right = rect.left + layer->mDocumentGlyph->GetWidth();
	rect.bottom = rect.top + layer->mDocumentGlyph->GetHeight();
	((BeDrawPort *)mDrawPort)->GetView()->Invalidate(rect);

#warning Not implemented properly
	layer->mDocumentGlyph->SetBGColor(layer->mBGColor);
	layer->mDocumentGlyph->SetWidth(layer->mWidth);		// Default width
	layer->mDocumentGlyph->SetTop(layer->mTop);
	layer->mDocumentGlyph->SetLeft(layer->mLeft);
	layer->mDocumentGlyph->SetHeight(layer->mHeight);
	if (layer->mVisibility == AV_HIDDEN || layer->mVisibility == AV_HIDE)
		layer->mDocumentGlyph->SetVisibility(false);
	else if (layer->mVisibility == AV_INHERIT)
		;
	else
		layer->mDocumentGlyph->SetVisibility(true);

	rect.left = layer->mDocumentGlyph->GetLeft();
	rect.top = layer->mDocumentGlyph->GetTop();
	rect.right = rect.left + layer->mDocumentGlyph->GetWidth();
	rect.bottom = rect.top + layer->mDocumentGlyph->GetHeight();
	((BeDrawPort *)mDrawPort)->GetView()->Invalidate(rect);
}
#endif


void
Document::AddFrame(
	int32		scroll,
	int32		marginWidth,
	int32		marginHeight,
	const char	*name,
	const char	*url,
	bool border)
{
	check_lock();
	if (mDoneWithFrameset)
		CloseFrameset();
	if (mFrameLeaf != NULL) {
		mFrameLeaf->SetScrolling(scroll);

		mFrameLeaf->SetMarginWidth(marginWidth);
		mFrameLeaf->SetMarginHeight(marginHeight);
		mFrameLeaf->SetBorder(border);
		mFrameLeaf->SetBorderValue(mFrameBorder);
		char newName[16];
		if (name == NULL)
			if (url != NULL)
				name = url;
			else {
				sprintf(newName, "0x%x", (unsigned int)mFrameLeaf);
				name = newName;
			}
		mFrameLeaf->SetName(name);

		BString theURL;
		ResolveURL(theURL, url);
		mFrameLeaf->SetURL(theURL.String());

		FrameItem *nextFrame = (FrameItem *)mFrameLeaf->Next();
		if (nextFrame != NULL)
			mFrameLeaf = nextFrame;
		else
			// Some sites are very lax about framesets and will forget closing </FRAMESET> tags.
			// Once we get the number of frames that were specified in the frameset, then mark it for
			// closure.
			mDoneWithFrameset = true;
	}
}	

void Document::RemoveFrame()
{
	check_lock();
	FrameItem *nextFrame = (FrameItem *)mFrameLeaf->Next();
	mFrames->Delete(mFrameLeaf);
	mFrameLeaf = nextFrame;
}

bool
Document::GetFrameset(
	FrameList	**frames)
{
	check_lock();
	if ((mFramesetLevel == 0) && (mFrames != NULL)) {
		*frames = mFrames;
		mFramesetLevel = -1;
		mFrames = NULL;
		mFrameLeaf = NULL;
		return (true);
	}

	return (false);
}

//	Reset a form when a button is clicked

int32 
Document::GetFrameBorder()
{
	return mFrameBorder;
}

void Document::FormReset(InputGlyph *g)
{
	check_lock();
//	NP_ASSERT(g);
	Form* f = g->GetForm();
//	NP_ASSERT(f);
	f->Reset(mDrawPort);
}

//	Return submission data when a button is clicked

BString* Document::FormSubmission(InputGlyph *g, BString& URL)
{
	check_lock();
//	NP_ASSERT(g);
	Form* f = g->GetForm();
//	NP_ASSERT(f);

	return FormSubmission(f, g, URL);
}

BString* Document::FormSubmission(Form *f, InputGlyph *g, BString& URL)
{
	BString partialURL;
	BString* post = f->GetSubmission(g,partialURL);
	ResolveURL(URL,partialURL.String());
	
	return post;
}

const char* Document::FormTargetFrame(InputGlyph *g)
{
	check_lock();
	Form* f = g->GetForm();
	
	return f->GetTargetFrame();
}

LayerItem::~LayerItem()
{
	delete mDocumentGlyph;
}

#ifdef JAVASCRIPT
ContextWrapper::~ContextWrapper()
{
#   if defined(JSE_DEBUGGABLE)
    {
       if ( NULL != debugme )
       {
          debugmeHasTerminated(debugme);

          while ( debugme )
          {
             debugmeDebug(mContext, &debugme);
          }
       }
       debugmeTerm(mContext, &debugme);
    }
#   endif
	jseSetGlobalObject(mContext, mOriginalGlobal);
	jseTerminateExternalLink(mContext);
}
#endif
