#ifndef _SCANNER_H_
#define _SCANNER_H_

#include <SupportDefs.h>

class BPoint;
class BRect;
class BShape;
class BPicture;
struct rgb_color;
struct pattern;
class BFile;

class Scanner
{
	public:
					Scanner(BFile*);
					~Scanner();

			bool	CheckPictureForUnhandledOps();

			int32	MovePen(BPoint delta);
			
			int32	StrokeLine(BPoint p1, BPoint p2);

			int32	StrokeRect(BRect r);
			int32	FillRect(BRect r);

			int32	StrokeRoundRect(BRect r, BPoint radius);
			int32	FillRoundRect(BRect r, BPoint radius);
			
			int32 	StrokeBezier(BPoint *pt);
			int32 	FillBezier(BPoint *pt);

			int32	StrokeArc(BPoint center, BPoint radius, 
						float startAngle, float endAngle);
			int32	FillArc(BPoint center, BPoint radius, 
						float startAngle, float endAngle);

						
			int32	StrokeEllipse(BPoint center, BPoint radius);
			int32	FillEllipse(BPoint center, BPoint radius);
			
			int32	StrokePolygon(int32 ptCount, BPoint *p, bool closed);
			int32	FillPolygon(int32 ptCount, BPoint *p);
			
			int32 	StrokeShape(BShape*);
			int32	FillShape(BShape*);
			
			int32	DrawString(char *string, float spacing1, float spacing2);

			int32	ClipToRects(BRect *rects, uint32 count);
			int32	ClipToPicture(BPicture *pic, BPoint origin, uint32 inverse);

			int32	PushState();
			int32	PopState();

			int32	SetOrigin(BPoint p);
			int32	SetLocation(BPoint p);

			int32	SetDrawOp(int32 drawOp);

			int32	SetPenSize(float penSize);

			int32	SetScale(float scale);

			int32	SetForeColor(rgb_color color);
			int32	SetBackColor(rgb_color color);

			int32	SetPattern(pattern pat);

			int32	SetFontFamily(char *string);
			int32	SetFontStyle(char *string);
			int32	SetFontSpacing(int32 spacing);
			int32	SetFontEncoding(int32 encoding);
			int32	SetFontFlags(int32 flags);
			int32	SetFontFaces(int32 faces);
			int32	SetFontBPP(int32 bpp);
			int32	SetFontSize(float size);
			int32	SetFontShear(float shear);
			int32	SetFontRotate(float rotate);

			int32 	DrawPixels(
						BRect src_rect, BRect dst_rect,
						int32 width, int32 height, int32 rowbyte,
						int32 format, int32 flags, uint8 *pixels);
	
			int32	EnterStateChange();
			int32	ExitStateChange();

	private:
			BFile*	fSpoolFile;
	
			int32	fWarnings;
};

#endif
