/***************************************************************************
//
//	File:			interface2/BDrawable.h
//
//	Description:	Abstract drawing interface.
//
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
***************************************************************************/

#ifndef _INTERFACE2_DRAWABLE_H
#define _INTERFACE2_DRAWABLE_H

#ifndef _FONT_H
#include <Font.h>
#endif

#ifndef _GRAPHICS_DEFS_H
#include <GraphicsDefs.h>
#endif

#ifndef _POINT_H
#include <Point.h>
#endif

#ifndef _RECT_H
#include <Rect.h>
#endif

class BBitmap;
class BPicture;
class BShape;

inline rgb_color make_color(uint8 red, uint8 green, uint8 blue, uint8 alpha=255)
{
	rgb_color c;
	c.red = red;
	c.green = green;
	c.blue = blue;
	c.alpha = alpha;
	return c;
}

class BDrawable
{
public:
						BDrawable();
virtual					~BDrawable();

// --------------- Core State Functions ---------------

virtual	bool			IsPrinting() const;

virtual	void			Flush() const = 0;
virtual	void			Sync() const = 0;

virtual	void			PushState() = 0;
virtual	void			PopState() = 0;

virtual	void			MoveOrigin(BPoint delta) = 0;
virtual	void			ScaleBy(float xAmount, float yAmount) = 0;

virtual	void			ClipToPicture(	BPicture *picture,
										BPoint where = B_ORIGIN,
										bool sync = true) = 0;
virtual	void			ClipToInversePicture(	BPicture *picture,
												BPoint where = B_ORIGIN,
												bool sync = true) = 0;

virtual	void			SetDrawingMode(drawing_mode mode) = 0;
virtual	drawing_mode	DrawingMode() const = 0;

virtual	void			SetBlendingMode(	source_alpha srcAlpha,
											alpha_function alphaFunc) = 0;
virtual	void	 		GetBlendingMode(	source_alpha *srcAlpha,
											alpha_function *alphaFunc) const = 0;

virtual	void			SetHighColor(rgb_color a_color) = 0;
virtual	rgb_color		HighColor() const = 0;

virtual	void			SetLowColor(rgb_color a_color) = 0;
virtual	rgb_color		LowColor() const = 0;

virtual	void			SetFont(const BFont *font, uint32 mask = B_FONT_ALL) = 0;
virtual	void			GetFont(BFont *font) const = 0;

virtual	void			ForceFontAliasing(bool enable) = 0;

virtual	void			SetPenSize(float size) = 0;
virtual	float			PenSize() const = 0;

virtual	void			SetLineMode(	cap_mode lineCap,
										join_mode lineJoin,
										float miterLimit=B_DEFAULT_MITER_LIMIT) = 0;
virtual void			GetLineMode(	cap_mode* lineCap,
										join_mode* lineJoin,
										float* miterLimit) const = 0;

// --------------- Core Drawing Functions ---------------

virtual	void			MovePenTo(BPoint pt) = 0;
virtual	BPoint			PenLocation() const = 0;
virtual	void			StrokeLineTo(BPoint toPt) = 0;

virtual	void			StrokePolygon(	const BPoint *ptArray,
										int32 numPts,
										bool  closed = true) = 0;
virtual	void			FillPolygon(const BPoint *ptArray, int32 numPts) = 0;
										    
virtual	void			StrokeRect(BRect r) = 0;
virtual	void			FillRect(BRect r) = 0;
virtual	void			InvertRect(BRect r) = 0;

virtual	void			StrokeRoundRect(BRect r, float xRadius, float yRadius) = 0;
virtual	void			FillRoundRect(BRect r, float xRadius, float yRadius) = 0;

virtual	void			StrokeEllipse(BPoint center, float xRadius, float yRadius) = 0;
virtual	void			FillEllipse(BPoint center, float xRadius, float yRadius) = 0;
				
virtual	void			StrokeArc(	BPoint center,
									float xRadius,
									float yRadius,
									float startAngle,
									float arcAngle) = 0;
virtual	void			FillArc(BPoint center,
								float xRadius,
								float yRadius,
								float startAngle,
								float arcAngle) = 0;

virtual	void			StrokeBezier(BPoint *controlPoints) = 0;
virtual	void			FillBezier(BPoint *controlPoints) = 0;

virtual	void			StrokeShape(BShape *shape) = 0;
virtual	void			FillShape(BShape *shape) = 0;

virtual	void			DrawPicture(const BPicture *aPicture) = 0;

virtual	void			DrawString(const char *aString,
								   int32 length = -1,
								   escapement_delta *delta = NULL) = 0;
								   
virtual	void			DrawClippedScaledBitmap(	const BBitmap *aBitmap,
													BRect srcRect,
													BRect dstRect) = 0;

// --------------- Optional Drawing Functions ---------------

virtual	void			MovePenBy(BPoint pt);

virtual	void			StrokeLine(BPoint pt0, BPoint pt1);

virtual	void			BeginLineArray(int32 count);
virtual	void			AddLine(BPoint pt0, BPoint pt1, rgb_color col);
virtual	void			EndLineArray();

virtual	void			StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3);
virtual	void			FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3);

virtual	void			StrokeInscribedEllipse(BRect r);
virtual	void			FillInscribedEllipse(BRect r);

virtual	void			StrokeInscribedArc(	BRect r,
											float startAngle,
											float arcAngle);
virtual	void			FillInscribedArc(	BRect r,
											float startAngle,
											float arcAngle);

virtual	void			DrawBitmapAt(const BBitmap *aBitmap, BPoint where);
virtual	void			DrawBitmap(const BBitmap *aBitmap);

virtual	void			DrawScaledBitmap(const BBitmap *aBitmap, BRect dstRect);

virtual	void			SetFontSize(float size);

virtual	void			GetFontHeight(font_height* height) const;
virtual	float			StringWidth(const char* str, int32 length=-1) const;

virtual	void			TruncateString(	BString* in_out,
										uint32 mode,
										float width) const;

virtual	void			DrawChar(char aChar);
virtual	void			DrawCharAt(char aChar, BPoint location);
virtual	void			DrawStringAt(const char *aString, BPoint location,
									 int32 length = -1,
									 escapement_delta *delta = NULL);

virtual	void			DrawPictureAt(const BPicture *aPicture, BPoint where);

// --------------- Convenience Functions ---------------

		source_alpha	BlendingSourceAlpha() const;
		alpha_function	BlendingAlphaFunction() const;
		
		join_mode		LineJoinMode() const;
		cap_mode		LineCapMode() const;
		float			LineMiterLimit() const;
};

#endif	// _INTERFACE2_DRAWABLE_H
