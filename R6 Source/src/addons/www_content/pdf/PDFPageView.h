#ifndef _PDF_PAGE_VIEW_H_
#define _PDF_PAGE_VIEW_H_

#include "DocFramework.h"

#include <vector>
#include "pdf_doc.h"
#include "Transform2D.h"

class PDFContentInstance;

#include "PDFDisplay.h"

using namespace BPrivate;

class PDFPageView : public DocView
{
	public:
									PDFPageView(BRect frame, const char *name,
												PDFContentInstance *instance,
												PDFDocument *doc,
												uint32 openPage = 1,
												float scale = 1.0);
									~PDFPageView();		

		status_t					ScaleToFit(BRect frame, uint32 scaleFlags);
		status_t					SetScale(float x, float y);
		status_t					GetScale(float *x, float *y) const;
		uint32						GoToPage(uint32 pageNum);
		uint32						GoToPage(PDFObject *page);
#if PDF_PRINTING > 0
		bool						IsPrinting();
		void						Print();
#endif
		void						FrameResized(float newWidth, float newHeight);
		void						ScrollTo(BPoint where);
		void						Draw(BRect updateRect);
		void						MouseDown(BPoint where);
		void						MouseMoved(BPoint where, uint32 code, const BMessage *msg);
		
		virtual	void				GetPreferredSize(float *width, float *height);		
		
		const char *				Title();
		
	private:
		PDFContentInstance *		fInstance;
		PDFDocument *				fDoc;		
		PDFDisplay *				fEngine;
		float 						fScaleX;
		float						fScaleY;
#if PDF_PRINTING > 0
		thread_id					fPrintThread;
		static	int32				print_entry(void *);
		int32						PrintFunc();
#endif
		// annotations
		void						ProcessAnnotations(void);
		int32						HitCheckAnnots(const BPoint &where) const;
		PDFObject *					fRawAnnotsArray;
		struct	Annot
		{
			bool operator<(const Annot& other) const;
			BRect	fFrame;
			PDFObject *fDict;
		};
		vector<Annot>				fAnnots;
		Transform2D					fTransform;
		int32						fLastAnnot;
};

#endif
