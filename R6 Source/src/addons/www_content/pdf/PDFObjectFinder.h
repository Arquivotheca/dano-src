#ifndef _PDF_OBJECT_FINDER_H_
#define _PDF_OBJECT_FINDER_H_


#include "Object2.h"

namespace BPrivate {

class PDFDocument;

class PDFObjectFinder {
	public:
							PDFObjectFinder(PDFObject *base);
							~PDFObjectFinder();

		status_t			SetBase(PDFObject *base);
		PDFObject *			Find(const char *name, bool inherited);
		PDFObject *			FindResource(const char *type, const char *name);
		PDFObject *			Base() const { return fBase; }
		PDFDocument *		Document() const { return fDoc; }
		
	private:
		void				Initialize();
		PDFObject *			fBase;
		object_array		fObjects;
		PDFDocument *		fDoc;
};

}; // namespace BPrivate

#endif
