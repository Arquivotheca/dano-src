
#include "BDrawable.h"

#ifndef _VIEWDRAWABLE_H
#define _VIEWDRAWABLE_H

class BViewDrawable : public BDrawable
{
	public:
								BViewDrawable(BView *view);
		virtual					~BViewDrawable();

				BView *			View();

		virtual	bool			IsPrinting() const;
		
		virtual	void			Flush() const;
		virtual	void			Sync() const;
		
		virtual	void			PushState();
		virtual	void			PopState();
		
		virtual	void			MoveOrigin(BPoint delta);
		virtual	void			ScaleBy(float xAmount, float yAmount);
		
		virtual	void			ClipToPicture(	BPicture *picture,
												BPoint where = B_ORIGIN,
												bool sync = true);
		virtual	void			ClipToInversePicture(	BPicture *picture,
														BPoint where = B_ORIGIN,
														bool sync = true);
		
		virtual	void			SetDrawingMode(drawing_mode mode);
		virtual	drawing_mode	DrawingMode() const;
		
		virtual	void			SetBlendingMode(	source_alpha srcAlpha,
													alpha_function alphaFunc);
		virtual	void	 		GetBlendingMode(	source_alpha *srcAlpha,
													alpha_function *alphaFunc) const;
		
		virtual	void			SetHighColor(rgb_color a_color);
		virtual	rgb_color		HighColor() const;
		
		virtual	void			SetLowColor(rgb_color a_color);
		virtual	rgb_color		LowColor() const;
		
		virtual	void			SetFont(const BFont *font, uint32 mask = B_FONT_ALL);
		virtual	void			GetFont(BFont *font) const;
		
		virtual	void			ForceFontAliasing(bool enable);
		
		virtual	void			SetPenSize(float size);
		virtual	float			PenSize() const;
		
		virtual	void			SetLineMode(	cap_mode lineCap,
												join_mode lineJoin,
												float miterLimit=B_DEFAULT_MITER_LIMIT);
		virtual void			GetLineMode(	cap_mode* lineCap,
												join_mode* lineJoin,
												float* miterLimit) const;
		
		// --------------- Core Drawing Functions ---------------
		
		virtual	void			MovePenTo(BPoint pt);
		virtual	BPoint			PenLocation() const;
		virtual	void			StrokeLineTo(BPoint toPt);
		
		virtual	void			StrokePolygon(	const BPoint *ptArray,
												int32 numPts,
												bool  closed = true);
		virtual	void			FillPolygon(const BPoint *ptArray, int32 numPts);
												    
		virtual	void			StrokeRect(BRect r);
		virtual	void			FillRect(BRect r);
		virtual	void			InvertRect(BRect r);
		
		virtual	void			StrokeRoundRect(BRect r, float xRadius, float yRadius);
		virtual	void			FillRoundRect(BRect r, float xRadius, float yRadius);
		
		virtual	void			StrokeEllipse(BPoint center, float xRadius, float yRadius);
		virtual	void			FillEllipse(BPoint center, float xRadius, float yRadius);
						
		virtual	void			StrokeArc(	BPoint center,
											float xRadius,
											float yRadius,
											float startAngle,
											float arcAngle);
		virtual	void			FillArc(BPoint center,
										float xRadius,
										float yRadius,
										float startAngle,
										float arcAngle);
		
		virtual	void			StrokeBezier(BPoint *controlPoints);
		virtual	void			FillBezier(BPoint *controlPoints);
		
		virtual	void			StrokeShape(BShape *shape);
		virtual	void			FillShape(BShape *shape);
		
		virtual	void			DrawPicture(const BPicture *aPicture);
		
		virtual	void			DrawString(const char *aString,
										   int32 length = -1,
										   escapement_delta *delta = NULL);
										   
		virtual	void			DrawClippedScaledBitmap(	const BBitmap *aBitmap,
															BRect srcRect,
															BRect dstRect);
	
	private:
	
		BView *					m_view;
};

#endif
