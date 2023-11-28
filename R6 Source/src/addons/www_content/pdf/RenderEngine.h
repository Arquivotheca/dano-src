#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include "DescriptionEngine.h"
#include "PDFObjectFinder.h"

#include <GraphicsDefs.h>
#include <Point.h>
#include <Shape.h>
#include <Region.h>
#include <Picture.h>
#include "Transform2D.h"
#include <Bitmap.h>

class FontEngine;

namespace BPrivate {
class PDFDocument;
class GState;
class ColorInfo;

struct TObject {
				TObject(): isValid(false) {}
	void		Init() { isValid = true; Tm.Set(1, 0, 0, 1, 0, 0); Tlm.Set(1, 0, 0, 1, 0, 0);}
	void		Reset() { isValid = false; Tm.Set(1, 0, 0, 1, 0, 0); Tlm.Set(1, 0, 0, 1, 0, 0);}
	Transform2D	Tm;
	Transform2D	Tlm;
	bool		isValid;
};


class RenderEngine : public DescriptionEngine
{

	public:
							RenderEngine(PDFObject *object = NULL, float scaleX = 1.0, float scaleY = 1.0);
		virtual				~RenderEngine();		
							
		// render the content
		// inTo: the bitmap to render into
		// the area of Content covered by into is: into.Bounds().OffsetTo(offset)
		// clip is the expected region of the content area that needs to be updated
		status_t			RenderBitmap(BBitmap &inTo, const BPoint &offset, const BRegion &clip);

		// set up for rendering
		status_t			SetContent(PDFObject *content); // releases any previous content
		status_t			SetScale(float x, float y);

		// info accessors
		PDFObject *			Content() const { return fFinder.Base(); }
		PDFObject *			Find(const char *name, bool inherited) { return fFinder.Find(name, inherited); }
		PDFObject *			FindResource(const char *type, const char *name) { return fFinder.FindResource(type, name); }
		PDFDocument *		Document() { return fFinder.Document(); }
		float				ScaleX() const { return fScaleX; }
		float				ScaleY() const { return fScaleY; }
		float				ContentWidth() const { return fContentWidth; }
		float				ContentHeight() const { return fContentHeight; }
		float				Width() const { return fContentWidth * fScaleX; }
		float				Height() const { return fContentHeight * fScaleY; }
		const Transform2D &	DTM() const { return fDTM; }
		
		virtual	void		ScaleChanged();
		virtual void		ContentChanged();
		
	private:
		// data
		PDFObjectFinder		fFinder;
		float				fScaleX, fScaleY;
		BPoint				fCrop;
		float				fContentHeight, fContentWidth;
		BBitmap *			fBitmap;
		BView *				fBitmapView;
		BRect				fBitmapFrame;
		color_space			fBitSpace;
		BRegion				fClip;
		
		GState *			fGS;
		BPicture *			fClipPicture;
		Transform2D			fDTM;
		TObject				fTObj;
		rgb_color			fOldColor;
		Pusher *			fParent;
		bool				fUnsupportedDisplayed;

	private:
		// utility functions
		void				InitData();
		void				empty_GS_stack();
		/* color space */
		void				setrgbcolor(rgb_color &color);
		void				setgreycolor(rgb_color &color);
		void				setcmykcolor(rgb_color &color);
		void				setcolorspace(ColorInfo &color, const char *space);
		//void				setcolornary(PDFObject *which, rgb_color &color);
		void				setcolornary(ColorInfo &color);
		/* path rendering */
		void				closepath();
		void				fill();
		void				eofill();
		void				stroke();
		void				clear();
		void				TransformShape(void);
		void				setclipping();
		void				setlinemode();
		/* Text rendering */
		void	 			next_text_line(float x, float y);
		void	 			RenderText(PDFObject *string, float offset = 0.0);
		
		/* Image Rendering  */
		void				RenderImage(PDFObject *o);
		
		
protected:
	// description engine keyword functions
	virtual void			UnknownKeyword(PDFObject *keyword);
	virtual void			bFn();
	virtual void			bstarFn();
	virtual void			BFn();
	virtual void			BstarFn();
	#if 0
	virtual void			BDCFn();
	virtual void			BIFn();
	virtual void			BMCFn();
	#endif
	virtual void			BTFn();
	#if 0
	virtual void			BXFn();
	#endif
	virtual void			cFn();
	virtual void			cmFn();
	virtual void			csFn();
	virtual void			CSFn();
	#if 0
	virtual void			dFn();
	virtual void			d0Fn();
	virtual void			d1Fn();
	#endif
	virtual void			DoFn();
	#if 0
	virtual void			DPFn();
	#endif
	virtual void			EIFn();
	#if 0
	virtual void			EMCFn();
	#endif
	virtual void			ETFn();
	#if 0
	virtual void			EXFn();
	#endif
	virtual void			fFn();
	virtual void			fstarFn();
	#if 0
	virtual void			FFn();
	#endif
	virtual void			gFn();
	virtual void			gsFn();
	virtual void			GFn();
	virtual void			hFn();
	#if 0
	virtual void			iFn();
	virtual void			IDFn();
	#endif
	virtual void			jFn();
	virtual void			JFn();
	virtual void			kFn();
	virtual void			KFn();
	virtual void			lFn();
	virtual void			mFn();
	virtual void			MFn();
	#if 0
	virtual void			MPFn();
	#endif
	virtual void			nFn();
	#if 0
	virtual void			PSFn();
	#endif
	virtual void			qFn();
	virtual void			QFn();
	virtual void			reFn();
	virtual void			rgFn();
	virtual void			riFn();
	virtual void			RGFn();
	virtual void			sFn();
	virtual void			scFn();
	virtual void			scnFn();
	#if 0
	virtual void			shFn();
	#endif
	virtual void			SFn();
	virtual void			SCFn();
	virtual void			SCNFn();
	virtual void			TcFn();
	virtual void			TdFn();
	virtual void			TDFn();
	virtual void			TfFn();
	virtual void			TjFn();
	virtual void			TJFn();
	virtual void			TLFn();
	virtual void			TmFn();
	virtual void			TrFn();
	virtual void			TsFn();
	virtual void			TwFn();
	virtual void			TzFn();
	virtual void			TstarFn();
	virtual void			vFn();
	virtual void			wFn();
	virtual void			WFn();
	virtual void			WstarFn();
	virtual void			yFn();
	virtual void			tickFn();
	virtual void			ticktickFn();
};


}; // namespace BPrivate

#endif

