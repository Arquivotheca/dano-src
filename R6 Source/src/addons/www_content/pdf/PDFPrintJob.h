#ifndef _PDF_PRINT_JOB_H_
#define _PDF_PRINT_JOB_H_

#include <print/DirectPrintJob.h>

namespace BPrivate {
	class PDFDocument;
	class RenderEngine;
};

using namespace BPrivate;

class PDFPrintJob : public BDirectPrintJob
{
	public:
								PDFPrintJob(PDFDocument * doc);
								PDFPrintJob(PDFDocument * doc, const BMessage &settings);
		virtual					~PDFPrintJob();

	protected:
		virtual status_t 		BeginJob(const printer_descriptor_t& page);
		virtual status_t 		EndJob();
		virtual status_t 		BeginPage(const uint32 page);
		virtual status_t 		EndPage();
		virtual status_t 		FillBitmap(const bitmap_rect_t& rect, BBitmap& bitmap);

	private:
		PDFDocument *			fDoc;
		printer_descriptor_t	fPrinter;
		RenderEngine *			fEngine;
};


#endif
