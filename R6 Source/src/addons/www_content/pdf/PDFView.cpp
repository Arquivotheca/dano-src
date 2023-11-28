
#include "PDFView.h"
#include "PageView.h"
#include "NumControl.h"
#include "ZoomControl.h"
#include "PDFContent.h"

PDFView::PDFView(BRect frame, const char *name, PDFDocument *doc, PDFContentInstance *instance,
					uint32 open, float) :
	BView(frame, name, B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW),
	fDoc(doc),
	fPageView(NULL),
	fPageNum(NULL),
	fZoom(NULL),
	fInstance(instance)
{
	SetViewColor(ui_color(B_MENU_BACKGROUND_COLOR));
	BRect rect(0, 0, 75, 20);
	// build the page number control
	uint32 pageCount = 0;
	if (fDoc) pageCount = fDoc->PageCount();
	fPageNum = new NumControl(rect, "NumControl", new BMessage('goto'), open, pageCount, 1);
	AddChild(fPageNum);
		

	// build the zoom control
	rect.OffsetTo(80, 0);
	fZoom = new ZoomControl(rect, "ZoomCOntrol");
	AddChild(fZoom);	
	
	// build the page view
	BRect bounds(Bounds());
	bounds.top += 21;
	fPageView = new PageView(fDoc->GetPage(1), bounds, "PageView");
	AddChild(fPageView);
}


PDFView::~PDFView()
{
	printf("PDFView deleted.\n");
}

void 
PDFView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case 'goto': {
			if (fPageView) {
				int32 pagenum = 1;
				msg->FindInt32("value", &pagenum);
				
				fPageView->SetTo(fDoc->GetPage(pagenum));
				if (fInstance) fInstance->MarkDirty();
			}
			break;
		}
		
		case 'zoom': {
			printf("zoom recvd\n");
			if (fPageView) {
				float zoom = 1.0;
				msg->FindFloat("scale", &zoom);
				fPageView->SetZoom(zoom, zoom);
				if (fInstance) fInstance->MarkDirty();
				
			}
			break;
		}
		
		default:
			BView::MessageReceived(msg);
	}
}

void 
PDFView::AllAttached()
{
	fPageNum->SetTarget(this);
	fZoom->SetTarget(this);
}

void 
PDFView::DetachedFromWindow()
{
}

void 
PDFView::FrameResized(float newWidth, float newHeight)
{
	// we should not need to do anything special
	BView::FrameResized(newWidth, newHeight);
}

void 
PDFView::ScrollTo(BPoint)
{
	// I am sure that we need to do something here
}

void 
PDFView::GetPreferredSize(float *width, float *height)
{
	fPageView->GetPreferredSize(width, height);
	*height += 20;
	if (*width < 150) *width = 150;
}

