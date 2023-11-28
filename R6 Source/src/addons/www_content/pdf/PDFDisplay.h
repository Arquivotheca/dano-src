#ifndef _PDF_DISPLAY_H_
#define _PDF_DISPLAY_H_

#include "RenderEngine.h"

namespace BPrivate {

class PDFDisplay : public RenderEngine
{
	public:
							PDFDisplay(PDFObject *page, float scaleX = 1.0, float scaleY = 1.0);
		virtual				~PDFDisplay();
		
		status_t			Display(BView *view, const BRegion *clip);
		PDFObject *			Annotations();
		const BPoint		FrameOffset() const { return fBitFrame.LeftTop(); }
		const BRect			Frame() const { return fBitFrame; }
		const PDFObject *	Page() { return Content(); }
		BPoint				ViewOffset() const { return fViewOffset; };
	protected:
		virtual void		Pulse();
		virtual	void		ScaleChanged();
		virtual void		ContentChanged();

	private:
		// resize the bitmap:
		// 	if we resize to match the size of the page return BIT_MATCH_PAGE
		//	if we resize to the max ram size instead return BIT_MAX_RAM
		int32				ResizeBitmap(BRect &viewBounds, BRect &pageBounds, uint32 maxRAM);
		BBitmap *			fBackingBitmap;
		BView *				fView;
		BPoint				fViewOffset;
		BRect				fBitFrame;	// the content of the bitmap in page coordinates
		bool				fValidBits; // whether or not the contents of the bitmap can be used
		bool				fUnsupportedShown;		
};

}; //namespace BPrivate

#endif
