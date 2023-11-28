
#ifndef _PDF_VIEW_H_
#define _PDF_VIEW_H_

#include <View.h>
#include "pdf_doc.h"
class PageView;
class NumControl;
class ZoomControl;
class PDFContentInstance;

class PDFView : public BView {
	public:
								PDFView(BRect frame, const char *name, PDFDocument *doc, PDFContentInstance *instance, uint32 open = 1, float zoom = 1.0);
								~PDFView();
		void					MessageReceived(BMessage *msg);			
		void					AllAttached();
		void					DetachedFromWindow();
		void					FrameResized(float newWidth, float newHeight);
		void					ScrollTo(BPoint where);
		void					GetPreferredSize(float *width, float *height);
	private:
		PDFDocument *			fDoc;
		PageView *				fPageView;
		NumControl *			fPageNum;
		ZoomControl *			fZoom;
		PDFContentInstance *	fInstance;
};

#endif
