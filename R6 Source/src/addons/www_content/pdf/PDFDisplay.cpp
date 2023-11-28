
#include "PDFDisplay.h"
#include <Bitmap.h>
#include <View.h>

#define MAX_RAM_SIZE	(8 * 1024 * 1024)

enum {
	BIT_PAGE_MATCH = 0,
	BIT_MAX_RAM = 1
};


PDFDisplay::PDFDisplay(PDFObject *page, float scaleX, float scaleY) :
	RenderEngine(page, scaleX, scaleY),
	fBackingBitmap(NULL),
	fView(NULL)
{
	fPulseInterval = 1000000; // once a second
}


PDFDisplay::~PDFDisplay()
{
	if (fBackingBitmap)
		delete fBackingBitmap;
}


void 
PDFDisplay::ScaleChanged()
{
	fValidBits = false;
	
}

void 
PDFDisplay::ContentChanged()
{
	fValidBits = false;
}

status_t 
PDFDisplay::Display(BView *view, const BRegion *clip)
{
	// clip contains the region of the view that needs to be updated
	// we need to determine what that means in bitmap coordinate space
	fView = view;
	// we center the page in the view	
	BRect pageFrame(0, 0, (int32)Width() -1, (int32) Height() - 1);
	BRect viewBounds(view->Bounds());
	float xOffset = (int32)(viewBounds.Width() - pageFrame.Width()) / 2;
	if (xOffset < 0) xOffset = 0;
	float yOffset = (int32)(viewBounds.Height() - pageFrame.Height()) / 2;
	if (yOffset < 0) yOffset = 0;

	// fViewOffset: how far left and down to offset the bitmap drawing to keep it centered
	fViewOffset.Set(xOffset, yOffset);	

	// get a working region
	BRegion working(*clip);

	// paint the background black
	BRect pageInViewFrame(pageFrame);
	pageInViewFrame.OffsetBy(fViewOffset);
	BRegion background(working);
	// exclude the page rect from the background
	background.Exclude(pageInViewFrame);
	if(background.CountRects() != 0) {
		view->SetHighColor(0, 0, 0);
		view->FillRegion(&background);
		working.Exclude(&background);
	}

	
	if (working.CountRects() == 0) {
		// hmm -only the portion of the view representing the background needed updating
		// go figure
		fView = NULL;
		return B_OK;
	}
	
	// okay - there are some bits still left to draw
	// and they are in the page region
	
	if (fValidBits) {
		// we potentially have some bits to copy over
		BRegion copyBits(working);
		BRegion validBits;
		validBits.Include(fBitFrame);
		validBits.OffsetBy(xOffset, yOffset);		
		// the intersection of our valid bits and the clipping region
		// tells us what parts of the page need to be displayed
		copyBits.IntersectWith(&validBits);
		if (copyBits.CountRects() != 0) {
			// lets draw part of the bitmap
			view->DrawBitmap(fBackingBitmap, fBitFrame.LeftTop() + fViewOffset);
			working.Exclude(copyBits.Frame());
		}		
	}
	
	if (working.CountRects() == 0) {
		fView = NULL;
		return B_OK;
	}

	// we have to update the bitmap
	bool resize = false;
	// if there is no bitmap or the bitmap doesn't match the width of the page
	// resize the bitmap
	if (!fBackingBitmap || (fBackingBitmap->Bounds().Width() != pageFrame.Width()) || !fValidBits)
		resize = true;

	if (resize)
		ResizeBitmap(viewBounds, pageFrame, MAX_RAM_SIZE);
		
	// now we need to find the appropriate band of the page that encompasses all of the
	// remaining working region
	BRect workingFrame(working.Frame());
	float workingTop = workingFrame.top - yOffset;
	
	fNextPulse = system_time() + fPulseInterval;

	int32 linesToDraw = (int32)workingFrame.Height();
	int32 linesDrawn = 0;
	while ((linesToDraw - linesDrawn) > 0) {
		fBitFrame.OffsetTo(0, workingTop + linesDrawn);
		BRegion updateRegion;
		updateRegion.Include(fBitFrame);
		RenderBitmap(*fBackingBitmap, fBitFrame.LeftTop(), updateRegion);
		view->DrawBitmap(fBackingBitmap, fBitFrame.LeftTop() + fViewOffset);
		linesDrawn += (int32)fBitFrame.Height();
		fValidBits = true;
	}
	
	fView = NULL;
	return B_OK;
	
}

int32 
PDFDisplay::ResizeBitmap(BRect &viewBounds, BRect &pageBounds, uint32 maxRAM)
{
	// take the easy way out for the first pass
	// bitmap is always as wide as the page contents	
	int32 bitWidth = pageBounds.IntegerWidth();
	// bitmap is as high as the page contents or what maxRAM will allow
	int32 maxHeight = (int32)(maxRAM / (2 * bitWidth));
	int32 bitHeight = min_c(maxHeight, pageBounds.IntegerHeight());
	

	if (fBackingBitmap)
		delete fBackingBitmap;
		
	fBitFrame.Set(0, 0, bitWidth, bitHeight);
	fBackingBitmap = new BBitmap(fBitFrame, B_BITMAP_ACCEPTS_VIEWS, B_RGB16);
	fValidBits = false;
	int32 status;
	if (bitHeight == pageBounds.IntegerHeight())
		status = BIT_PAGE_MATCH;
	else
		status = BIT_MAX_RAM;
	return status;
}


void 
PDFDisplay::Pulse()
{
	if (fView)
	{
		fView->DrawBitmap(fBackingBitmap, fBitFrame.LeftTop() + fViewOffset);
		fView->Flush();
	}
}


PDFObject *
PDFDisplay::Annotations(void)
{
	// safe for null
	return Content()->Find(PDFAtom.Annots)->Resolve();
}
