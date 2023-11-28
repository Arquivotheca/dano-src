#ifndef _PAGE_VIEW_H_
#define _PAGE_VIEW_H_

#include <View.h>
#include "RenderEngine.h"
#include <vector>

class PageView : public BView
{
	public:
						PageView(PDFObject *page, BRect frame, const char *name);
						~PageView();

		void			SetTo(PDFObject *page);
		void			SetZoom(float x, float y);
//		void			FrameResized(float newWidth, float newHeight);
//		void			ScrollTo(BPoint where);
virtual	void			Draw(BRect updateRect);
virtual	void			MouseDown(BPoint where);
virtual	void			MouseMoved(	BPoint where,
									uint32 code,
									const BMessage *a_message);
virtual	void			GetPreferredSize(float *width, float *height);
virtual	void			ResizeToPreferred();
	private:

		void			ProcessAnnotations(void);
		int32			HitCheckAnnots(const BPoint &where) const;
		RenderEngine	*fEngine;
		float			fXscale, fYscale;
		PDFObject		*fRawAnnotsArray;
		struct	Annot
		{
			bool operator<(const Annot& other) const;
			BRect	fFrame;
			PDFObject *fDict;
		};
		vector<Annot>	fAnnots;
		Transform2D		fTransform;
		int32			fLastAnnot;
};

#endif
