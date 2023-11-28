#ifndef GBUFFEREDGRAPHPORT_H
#define GBUFFEREDGRAPHPORT_H

#include "ggraphport.h"

// Things we need
class BMGraphic;

/*
	A DrawRequest is a simple structure that represents a
	desire to draw something.  That something is pointed to
	by the fGraphic member.  The fRect tells us what part of the
	graphic specifically needs to be drawn.  The fTime variable
	tells us when the drawing should occur.  This structure is filled
	in when the NeedsToDraw() method is called.  That way, the
	graphport gets a chance to organize when drawing should occur
	and prioritize them in time order.
*/

struct DrawRequest
{
	BMGraphic	*fGraphic;
	BRect		fRect;
	bigtime_t	fTime;
};

class BBufferedGraphPort : public BGraphPort
{
public:
			BBufferedGraphPort(BView *realView, BMGraphic *);
	virtual	~BBufferedGraphPort();
	
	
	virtual void	SetTransform(const BMatrix2D &aMatrix);
	virtual void	TransformPoint(BPoint *aPoint);
	virtual void	UntransformPoint(BPoint *aPoint);

			bool	BeginDrawing(const bigtime_t maxWaitTime=B_INFINITE_TIMEOUT);
			void	EndDrawing();
			void	Flush() const;
			void	Sync() const;
			void	Invalidate(BRect invalRect);
			void	Invalidate();
			void	GetClippingRegion(BRegion *region) const;
	virtual	void	ConstrainClippingRegion(BRegion *region);
	virtual void	NeedsToDraw(BMGraphic *, const BRect rect, const bigtime_t atTime=B_NOW, const bigtime_t waitAsLongAs = 500000);	
	
	// The most important method here.  Draws into the realView
	virtual void	Draw(const BRect rect);		// This assumes window locked
	virtual void	Blit(const BRect rect, const bigtime_t waitAsLongAs = B_INFINITE_TIMEOUT);		// This will lock the window
	
virtual	void			SetDrawingMode(drawing_mode mode);
		drawing_mode 	DrawingMode() const;

virtual	void			SetPenSize(float size);
		float			PenSize() const;

virtual	void			SetViewColor(rgb_color c);
		void			SetViewColor(uchar r, uchar g, uchar b, uchar a = 255);
		rgb_color		ViewColor() const;

virtual	void			SetHighColor(rgb_color a_color);
		void			SetHighColor(uchar r, uchar g, uchar b, uchar a = 255);
		rgb_color		HighColor() const;

virtual	void			SetLowColor(rgb_color a_color);
		void			SetLowColor(uchar r, uchar g, uchar b, uchar a = 255);
		rgb_color		LowColor() const;

		void			SetLineMode(	cap_mode lineCap,
								join_mode lineJoin,
								float miterLimit=B_DEFAULT_MITER_LIMIT);
		join_mode		LineJoinMode() const;
		cap_mode		LineCapMode() const;
		float			LineMiterLimit() const;

		void			SetOrigin(BPoint pt);
		void			SetOrigin(float x, float y);
		BPoint			Origin() const;

		void			PushState();
		void			PopState();

		void			MovePenTo(BPoint pt);
		void			MovePenTo(float x, float y);
		void			MovePenBy(float x, float y);
		BPoint			PenLocation() const;
		void			StrokeLine(	BPoint toPt,
									pattern p = B_SOLID_HIGH);
		void			StrokeLine(	BPoint pt0,
									BPoint pt1,
									pattern p = B_SOLID_HIGH);
		void			BeginLineArray(int32 count);
		void			AddLine(BPoint pt0, BPoint pt1, rgb_color col);
		void			EndLineArray();
	
		void			StrokePolygon(	const BPolygon *aPolygon,
									    bool  closed = true,
										pattern p = B_SOLID_HIGH);
		void			StrokePolygon(	const BPoint *ptArray,
										int32 numPts,
									    bool  closed = true,
										pattern p = B_SOLID_HIGH);
		void			StrokePolygon(	const BPoint *ptArray,
										int32 numPts,
										BRect bounds,
									    bool  closed = true,
										pattern p = B_SOLID_HIGH);
		void			FillPolygon(const BPolygon *aPolygon,
									pattern p = B_SOLID_HIGH);
		void			FillPolygon(const BPoint *ptArray,
									int32 numPts,
									pattern p = B_SOLID_HIGH);
		void			FillPolygon(const BPoint *ptArray,
									int32 numPts,
									BRect bounds,
									pattern p = B_SOLID_HIGH);
	
		void			StrokeTriangle(	BPoint pt1,
										BPoint pt2,
										BPoint pt3,
										BRect bounds,
										pattern p = B_SOLID_HIGH);
		void			StrokeTriangle(	BPoint pt1,
										BPoint pt2,
										BPoint pt3,
										pattern p = B_SOLID_HIGH);
		void			FillTriangle(	BPoint pt1,
										BPoint pt2,
										BPoint pt3,
										pattern p = B_SOLID_HIGH);
		void			FillTriangle(	BPoint pt1,
										BPoint pt2,
										BPoint pt3,
										BRect bounds,
										pattern p = B_SOLID_HIGH);

		void			StrokeRect(BRect r, pattern p = B_SOLID_HIGH);
		void			FillRect(BRect r, pattern p = B_SOLID_HIGH);
		void			FillRegion(BRegion *a_region, pattern p= B_SOLID_HIGH);
		void			InvertRect(BRect r);

		void			StrokeRoundRect(BRect r,
										float xRadius,
										float yRadius,
										pattern p = B_SOLID_HIGH);
		void			FillRoundRect(	BRect r,
										float xRadius,
										float yRadius,
										pattern p = B_SOLID_HIGH);

		void			StrokeEllipse(	BPoint center,
										float xRadius,
										float yRadius,
										pattern p = B_SOLID_HIGH);
		void			StrokeEllipse(BRect r, pattern p = B_SOLID_HIGH);
		void			FillEllipse(BPoint center,
									float xRadius,
									float yRadius,
									pattern p = B_SOLID_HIGH);
		void			FillEllipse(BRect r, pattern p = B_SOLID_HIGH);
				
		void			StrokeArc(	BPoint center,
									float xRadius,
									float yRadius,
									float start_angle,
									float arc_angle,
									pattern p = B_SOLID_HIGH);
		void			StrokeArc(	BRect r,
									float start_angle,
									float arc_angle,
									pattern p = B_SOLID_HIGH);
		void			FillArc(BPoint center,
								float xRadius,
								float yRadius,
								float start_angle,
								float arc_angle,
								pattern p = B_SOLID_HIGH);
		void			FillArc(BRect r,
								float start_angle,
								float arc_angle,
								pattern p = B_SOLID_HIGH);

		void			StrokeBezier(	BPoint *controlPoints,
								pattern p = B_SOLID_HIGH);
		void			FillBezier(	BPoint *controlPoints,
								pattern p = B_SOLID_HIGH);
			
		void			CopyBits(BRect src, BRect dst);
		void			DrawBitmapAsync(	const BBitmap *aBitmap,
											BRect srcRect,
											BRect dstRect);
		void			DrawBitmapAsync(const BBitmap *aBitmap);
		void			DrawBitmapAsync(const BBitmap *aBitmap, BPoint where);
		void			DrawBitmapAsync(const BBitmap *aBitmap, BRect dstRect);
		void			DrawBitmap(	const BBitmap *aBitmap,
									BRect srcRect,
									BRect dstRect);
		void			DrawBitmap(const BBitmap *aBitmap);
		void			DrawBitmap(const BBitmap *aBitmap, BPoint where);
		void			DrawBitmap(const BBitmap *aBitmap, BRect dstRect);

		void			DrawChar(char aChar);
		void			DrawChar(char aChar, BPoint location);
		void			DrawString(const char *aString,
								   escapement_delta *delta = NULL);
		void			DrawString(const char *aString, BPoint location,
								   escapement_delta *delta = NULL);
		void			DrawString(const char *aString, int32 length,
								   escapement_delta *delta = NULL);
		void			DrawString(const char *aString,
								   int32 length,
								   BPoint location,
								   escapement_delta *delta = 0L);
virtual void            SetFont(const BFont *font, uint32 mask = B_FONT_ALL);
		void            GetFont(BFont *font) ;
		float			StringWidth(const char *string) const;
		float			StringWidth(const char *string, int32 length) const;
		void			GetStringWidths(char *stringArray[], 
										int32 lengthArray[],
										int32 numStrings,
										float widthArray[]) const;	
		void			SetFontSize(float size);
		void			GetFontHeight(font_height *height) const;
	

		void			BeginPicture(BPicture *a_picture);
		void			AppendToPicture(BPicture *a_picture);
		BPicture		*EndPicture();
		void			DrawPicture(const BPicture *a_picture);
		void			DrawPicture(const BPicture *a_picture, BPoint where);


protected:
	//BMInteractiveGraphicGroup	fRootGraphic;
	BMatrix2D	fTransformMatrix;
	BMatrix2D	fInverseTransform;
	BView	*fRealView;				// The view we really draw into
	BView	*fView;					// View to draw into backing store
	BBitmap	*fBitmap;				// Backing store
	BMGraphic	*fRootGraphic;
		
private:
};

#endif
