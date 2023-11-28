#ifndef GGRAPHPORT_H
#define GGRAPHPORT_H

#include <GraphicsDefs.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <MidiDefs.h>		// For B_NOW
#include <Font.h>
#include <View.h>

#include "gcolors.h"
#include "gmatrix.h"

// Things that we use
class BBitmap;
class BFont;
class BPicture;
class BMGraphic;

/*
	Interface: BPaint
	
	Paint represents the stuff that is used to do drawing.  It encapsulates
	the high color, low color, pattern, and drawing mode.  It can be easily
	constructed and saved for repeated usage for many graphics.
*/
/*
struct BPaint
{
			BPaint() :
				fDrawingMode(B_OP_COPY),
				fHighColor(kblack),
				fLowColor(kwhite),
				fPattern(B_SOLID_HIGH)
				{
				};
				
			BPaint(const rgb_color highColor,
					const rgb_color lowColor,
					const drawing_mode drawMode=B_OP_COPY,
					const pattern aPattern=B_SOLID_HIGH) :
				fDrawingMode(drawMode),
				fHighColor(highColor),
				fLowColor(lowColor),
				fPattern(aPattern)
				{};

	drawing_mode	fDrawingMode;
	rgb_color		fHighColor;
	rgb_color		fLowColor;
	pattern			fPattern;
};
*/
/*
	Interface: BPen
	
	A pen encapsulates the various parameters related to drawing lines.  These
	include a size, line cap, join mode, and miter limit.  The pen does not include
	the paint, that is what BPaint does.
*/
/*
struct BPen
{
			BPen() :
				fSize(1.0),
				fLineCap(B_ROUND_CAP),
				fJoinMode(B_ROUND_JOIN),
				fMiterLimit(B_DEFAULT_MITER_LIMIT)
				{
				};
				
			BPen(const float aSize, const cap_mode aCap, const join_mode aJoin, const float aMiter) :
				fSize(aSize),
				fLineCap(aCap),
				fJoinMode(aJoin),
				fMiterLimit(aMiter)
				{
				};
				
	float		fSize;
	cap_mode	fLineCap;
	join_mode	fJoinMode;
	float		fMiterLimit;
};
*/

/*
	Interface: BGraphPort
	
	This is the drawing portal into which all drawing commands must be addressed.
*/
class BGraphPort 
{
public:	
	virtual bool	BeginDrawing(const bigtime_t maxWaitTime=B_INFINITE_TIMEOUT);
	virtual void	EndDrawing();
			
	// The most important method here.  Draws into the realView
	virtual void	Draw(const BRect rect);		// This assumes window locked
	virtual void	Blit(const BRect rect, const bigtime_t waitAsLongAs = B_INFINITE_TIMEOUT);		// This will lock the window
	virtual void	NeedsToDraw(BMGraphic *, const BRect rect, const bigtime_t atTime=B_NOW, const bigtime_t waitAsLongAs = B_INFINITE_TIMEOUT);	

	virtual void	SetTransform(const BMatrix2D &aMatrix);
	virtual void	TransformPoint(BPoint *aPoint);
	virtual void	UntransformPoint(BPoint *aPoint);
	
	// Drawing interface taken from BView
	// It might be a good idea to eliminate some
	// of the interfaces
	virtual void	Invalidate(BRect invalRect);
	virtual void	Invalidate();
			void	GetClippingRegion(BRegion *region) const;
	virtual	void	ConstrainClippingRegion(BRegion *region);

	virtual void	Flush() const;
	virtual void	Sync() const;


	virtual	void		SetDrawingMode(drawing_mode mode);
		drawing_mode 	DrawingMode() const;

	virtual	void		SetPenSize(float size);
		float			PenSize() const;

	virtual	void		SetViewColor(rgb_color c);
		void			SetViewColor(uchar r, uchar g, uchar b, uchar a = 255);
		rgb_color		ViewColor() const;

	virtual	void		SetHighColor(rgb_color a_color);
	virtual	void		SetHighColor(uchar r, uchar g, uchar b, uchar a = 255);
	virtual	rgb_color	HighColor() const;

	virtual	void		SetLowColor(rgb_color a_color);
	virtual	void		SetLowColor(uchar r, uchar g, uchar b, uchar a = 255);
	virtual rgb_color	LowColor() const;

	virtual void		SetLineMode(	cap_mode lineCap,
								join_mode lineJoin,
								float miterLimit=B_DEFAULT_MITER_LIMIT);
	virtual join_mode	LineJoinMode() const;
	virtual cap_mode	LineCapMode() const;
	virtual float		LineMiterLimit() const;

	virtual void		SetOrigin(BPoint pt);
	virtual void		SetOrigin(float x, float y);
	virtual BPoint		Origin() const;

	virtual void		PushState();
	virtual void		PopState();

	virtual void		MovePenTo(BPoint pt);
	virtual void		MovePenTo(float x, float y);
	virtual void		MovePenBy(float x, float y);
	virtual BPoint		PenLocation() const;
	virtual void		StrokeLine(	BPoint toPt,
									pattern p = B_SOLID_HIGH);
	virtual void		StrokeLine(	BPoint pt0,
									BPoint pt1,
									pattern p = B_SOLID_HIGH);

	virtual void		BeginLineArray(int32 count);
	virtual void		AddLine(BPoint pt0, BPoint pt1, rgb_color col);
	virtual void		EndLineArray();
	
	// Polygon drawing
	virtual void		StrokePolygon(	const BPolygon *aPolygon,
									    bool  closed = true,
										pattern p = B_SOLID_HIGH);
	virtual void		StrokePolygon(	const BPoint *ptArray,
										int32 numPts,
									    bool  closed = true,
										pattern p = B_SOLID_HIGH);
	virtual void		StrokePolygon(	const BPoint *ptArray,
										int32 numPts,
										BRect bounds,
									    bool  closed = true,
										pattern p = B_SOLID_HIGH);
	virtual void		FillPolygon(const BPolygon *aPolygon,
									pattern p = B_SOLID_HIGH);
	virtual void		FillPolygon(const BPoint *ptArray,
									int32 numPts,
									pattern p = B_SOLID_HIGH);
	virtual void		FillPolygon(const BPoint *ptArray,
									int32 numPts,
									BRect bounds,
									pattern p = B_SOLID_HIGH);
	
	virtual void		StrokeTriangle(	BPoint pt1,
										BPoint pt2,
										BPoint pt3,
										BRect bounds,
										pattern p = B_SOLID_HIGH);
	virtual void		StrokeTriangle(	BPoint pt1,
										BPoint pt2,
										BPoint pt3,
										pattern p = B_SOLID_HIGH);
	virtual void		FillTriangle(	BPoint pt1,
										BPoint pt2,
										BPoint pt3,
										pattern p = B_SOLID_HIGH);
	virtual void		FillTriangle(	BPoint pt1,
										BPoint pt2,
										BPoint pt3,
										BRect bounds,
										pattern p = B_SOLID_HIGH);

	virtual void		StrokeRect(BRect r, pattern p = B_SOLID_HIGH);
	virtual void		FillRect(BRect r, pattern p = B_SOLID_HIGH);
	virtual void		FillRegion(BRegion *a_region, pattern p= B_SOLID_HIGH);
	virtual void		InvertRect(BRect r);

	virtual void		StrokeRoundRect(BRect r,
										float xRadius,
										float yRadius,
										pattern p = B_SOLID_HIGH);
	virtual void		FillRoundRect(	BRect r,
										float xRadius,
										float yRadius,
										pattern p = B_SOLID_HIGH);

	virtual void		StrokeEllipse(	BPoint center,
										float xRadius,
										float yRadius,
										pattern p = B_SOLID_HIGH);
	virtual void		StrokeEllipse(BRect r, pattern p = B_SOLID_HIGH);
	virtual void		FillEllipse(BPoint center,
									float xRadius,
									float yRadius,
									pattern p = B_SOLID_HIGH);
	virtual void		FillEllipse(BRect r, pattern p = B_SOLID_HIGH);
				
	virtual void		StrokeArc(	BPoint center,
									float xRadius,
									float yRadius,
									float start_angle,
									float arc_angle,
									pattern p = B_SOLID_HIGH);
	virtual void		StrokeArc(	BRect r,
									float start_angle,
									float arc_angle,
									pattern p = B_SOLID_HIGH);
	virtual void		FillArc(BPoint center,
								float xRadius,
								float yRadius,
								float start_angle,
								float arc_angle,
								pattern p = B_SOLID_HIGH);
	virtual void		FillArc(BRect r,
								float start_angle,
								float arc_angle,
								pattern p = B_SOLID_HIGH);

	virtual void		StrokeBezier(	BPoint *controlPoints,
								pattern p = B_SOLID_HIGH);
	virtual void		FillBezier(	BPoint *controlPoints,
								pattern p = B_SOLID_HIGH);
			
	virtual void		CopyBits(BRect src, BRect dst);
	virtual void		DrawBitmapAsync(	const BBitmap *aBitmap,
											BRect srcRect,
											BRect dstRect);
	virtual void		DrawBitmapAsync(const BBitmap *aBitmap);
	virtual void		DrawBitmapAsync(const BBitmap *aBitmap, BPoint where);
	virtual void		DrawBitmapAsync(const BBitmap *aBitmap, BRect dstRect);
	virtual void		DrawBitmap(	const BBitmap *aBitmap,
									BRect srcRect,
									BRect dstRect);
	virtual void		DrawBitmap(const BBitmap *aBitmap);
	virtual void		DrawBitmap(const BBitmap *aBitmap, BPoint where);
	virtual void		DrawBitmap(const BBitmap *aBitmap, BRect dstRect);

	virtual void		DrawChar(char aChar);
	virtual void		DrawChar(char aChar, BPoint location);
	virtual void		DrawString(const char *aString,
								   escapement_delta *delta = NULL);
	virtual void		DrawString(const char *aString, BPoint location,
								   escapement_delta *delta = NULL);
	virtual void		DrawString(const char *aString, int32 length,
								   escapement_delta *delta = NULL);
	virtual void		DrawString(const char *aString,
								   int32 length,
								   BPoint location,
								   escapement_delta *delta = 0L);
	virtual void        SetFont(const BFont *font, uint32 mask = B_FONT_ALL);
	virtual void        GetFont(BFont *font) ;
	virtual float		StringWidth(const char *string) const;
	virtual float		StringWidth(const char *string, int32 length) const;
	virtual void		GetStringWidths(char *stringArray[], 
										int32 lengthArray[],
										int32 numStrings,
										float widthArray[]) const;	
	virtual void		SetFontSize(float size);
	virtual void		GetFontHeight(font_height *height) const;
	
	// Picture drawing stuff
	virtual void		BeginPicture(BPicture *a_picture);
	virtual void		AppendToPicture(BPicture *a_picture);
	virtual BPicture	*EndPicture();
	virtual void		DrawPicture(const BPicture *a_picture);
	virtual void		DrawPicture(const BPicture *a_picture, BPoint where);


protected:
	
private:
};

#endif
