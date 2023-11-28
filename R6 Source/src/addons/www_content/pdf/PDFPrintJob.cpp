
#include "PDFPrintJob.h"
#include "pdf_doc.h"
#include "RenderEngine.h"
#include <Bitmap.h>

using namespace BPrivate;

#define BITMAP_WATCHER 0

#if BITMAP_WATCHER > 0
#include <Window.h>
#include <View.h>
static BWindow * bitWindow = NULL;
static BView * bitView = NULL;
static bool bitmap_watcher_init = false;

static
void update_bitmap_watcher(BBitmap *bitmap) {
	if (bitmap_watcher_init == false) {
		bitWindow = new BWindow(BRect(550, 50, 1050, 750), "PDFPrintJobWatcher", B_TITLED_WINDOW, 0);
		bitView = new BView(bitWindow->Bounds(), "bitView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
		bitView->SetViewColor(155, 155, 155);
		bitWindow->AddChild(bitView);
		bitWindow->Show();
		
		bitmap_watcher_init = true;
	}
	
	if (bitWindow && bitView && bitmap) {
		if (bitWindow->Lock()) {
//			bitView->DrawBitmap(bitmap, B_ORIGIN);
			bitView->DrawBitmap(bitmap, bitView->Bounds());
			bitWindow->Unlock();
		}
	}
}

#endif


PDFPrintJob::PDFPrintJob(PDFDocument *doc) :
	fDoc(doc),
	fEngine(NULL)
{
//	printf("PDFPrintJob::PDFPrintJob\n");
#if BITMAP_WATCHER > 0
	update_bitmap_watcher(NULL);
#endif	
}

PDFPrintJob::PDFPrintJob(PDFDocument *doc, const BMessage &settings) :
	BDirectPrintJob(settings),
	fDoc(doc),
	fEngine(NULL)
{
//	printf("PDFPrintJob::PDFPrintJob\n");
}


PDFPrintJob::~PDFPrintJob()
{
}

status_t 
PDFPrintJob::BeginJob(const printer_descriptor_t &page)
{
//	printf("PDFPrintJob::BeginJob\n");
	fPrinter = page;
	return B_OK;
}

status_t 
PDFPrintJob::EndJob()
{
	printf("PDFPrintJob::EndJob\n");
	return B_OK;
}

status_t 
PDFPrintJob::BeginPage(const uint32 pageNum)
{
//	printf("PDFPrintJob::BeginPage\n");
	if (!fEngine) {
		fEngine = new RenderEngine(fDoc->GetPage(pageNum));
	}
	else
		fEngine->SetContent(fDoc->GetPage(pageNum));

	// scale to fit onto the page
	float cw = fEngine->ContentWidth();
	float ch = fEngine->ContentWidth();
	
	float wScale = fPrinter.width / cw;	
	float hScale = fPrinter.height / ch;
	
	float scale = min_c(wScale, hScale);
	fEngine->SetScale(scale, scale);

//	fEngine->SetScale(wScale, hScale * fPrinter.aspect_ratio);
//	printf("PDFPrintJob: scaleX: %.2f scaleY: %.2f\n", fEngine->ScaleX(), fEngine->ScaleY());
	return B_OK;
}

status_t 
PDFPrintJob::EndPage()
{
//	printf("PDFPrintJob::EndPage\n");
	if (fEngine) {
		fEngine->SetContent(NULL);
	}
	return B_OK;
}

status_t 
PDFPrintJob::FillBitmap(const bitmap_rect_t &rect, BBitmap &bitmap)
{
//	printf("PDFPrintJob::FillBitmap: page:%ld rect: (%lu, %lu, %lu, %lu) color_space: %ld\n", rect.page, rect.left, rect.top, rect.left + rect.width, rect.top + rect.height, bitmap.ColorSpace());
	status_t status = B_ERROR;
	// always clear out the bits from previously
	uint32 *bits = (uint32 *)bitmap.Bits();
	const uint32 bpr = bitmap.BytesPerRow();
	memset(bits, 0xFF, bitmap.BitsLength());
	
	if (fEngine) {
		BRegion clip;
		clip.Set(BRect(rect.left, rect.top, rect.left + rect.width, rect.top + rect.height));
		status = fEngine->RenderBitmap(bitmap, BPoint(rect.left, rect.top), clip);
	}

#if BITMAP_WATCHER > 0
	update_bitmap_watcher(&bitmap);
#endif
	
	return status;
}

