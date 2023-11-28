#include "Scanner.h"

#include <stdio.h>
#include <Point.h>
#include <Rect.h>
#include <Shape.h>
#include <Picture.h>
#include <File.h>
#include <InterfaceDefs.h>
#include <Font.h>
#include <UTF8.h>

int32 scanner_MovePen(Scanner *cp, BPoint delta);
int32 scanner_StrokeLine(Scanner *cp, BPoint p1, BPoint p2);
int32 scanner_StrokeRect(Scanner *cp, BRect r);
int32 scanner_FillRect(Scanner *cp, BRect r);
int32 scanner_StrokeRoundRect(Scanner *cp, BRect r, BPoint radius);
int32 scanner_FillRoundRect(Scanner *cp, BRect r, BPoint radius);
int32 scanner_StrokeBezier(Scanner *cp, BPoint *pt);
int32 scanner_FillBezier(Scanner *cp, BPoint *pt);
int32 scanner_StrokeArc(Scanner *cp, BPoint center, BPoint radius, float startAngle, float endAngle);
int32 scanner_FillArc(Scanner *cp, BPoint center, BPoint radius, float startAngle, float endAngle);
int32 scanner_StrokeEllipse(Scanner *cp, BPoint center, BPoint radius);
int32 scanner_FillEllipse(Scanner *cp, BPoint center, BPoint radius);
int32 scanner_StrokePolygon(Scanner *cp, int32 ptCount, BPoint *p, bool closed);
int32 scanner_FillPolygon(Scanner *cp, int32 ptCount, BPoint *p);
int32 scanner_StrokeShape(Scanner *cp, BShape *shape);
int32 scanner_FillShape(Scanner *cp, BShape *shape);
int32 scanner_DrawString(Scanner *cp, char *str_ptr, float float1, float float2);
int32 scanner_ClipToRects(Scanner *cp, BRect *rects, uint32 count);
int32 scanner_ClipToPicture(Scanner *cp, BPicture *pic, BPoint origin, uint32 inverse);
int32 scanner_PushState(Scanner *cp);
int32 scanner_PopState(Scanner *cp);
int32 scanner_SetOrigin(Scanner *cp, BPoint p);
int32 scanner_SetLocation(Scanner *cp, BPoint p);
int32 scanner_SetDrawOp(Scanner *cp, int32 drawOp);
int32 scanner_SetPenSize(Scanner *cp, float penSize);
int32 scanner_SetScale(Scanner *cp, float scale);
int32 scanner_SetForeColor(Scanner *cp, rgb_color color);
int32 scanner_SetBackColor(Scanner *cp, rgb_color color);
int32 scanner_SetPattern(Scanner *cp, pattern pat);
int32 scanner_SetFontFamily(Scanner *cp, char *string);
int32 scanner_SetFontStyle(Scanner *cp, char *string);
int32 scanner_SetFontSpacing(Scanner *cp, int32 spacing);
int32 scanner_SetFontEncoding(Scanner *cp, int32 encoding);
int32 scanner_SetFontFlags(Scanner *cp, int32 flags);
int32 scanner_SetFontFaces(Scanner *cp, int32 faces);
int32 scanner_SetFontBPP(Scanner *cp, int32 bpp);
int32 scanner_SetFontSize(Scanner *cp, float size);
int32 scanner_SetFontShear(Scanner *cp, float shear);
int32 scanner_SetFontRotate(Scanner *cp, float rotate);
int32 scanner_DrawPixels(Scanner *cp, 
	BRect src_rect, BRect dst_rect,
	int32 width, int32 height, int32 rowbyte,
	int32 format, int32 flags, uint8 *pixels);
int32 scanner_EnterStateChange(Scanner *cp);
int32 scanner_ExitStateChange(Scanner *cp);

void *ScannerCallbacks[] = {
	NULL,

	scanner_MovePen,
	
	scanner_StrokeLine,

	scanner_StrokeRect,
	scanner_FillRect,

	scanner_StrokeRoundRect,
	scanner_FillRoundRect,

	scanner_StrokeBezier,
	scanner_FillBezier,

	scanner_StrokeArc,
	scanner_FillArc,

	scanner_StrokeEllipse,
	scanner_FillEllipse,

	scanner_StrokePolygon,
	scanner_FillPolygon,

	scanner_StrokeShape,		// stroke shape
	scanner_FillShape,		// fill shape

	scanner_DrawString,
	scanner_DrawPixels,
	NULL,		// -- (blit)	

	scanner_ClipToRects,		// clip to rects
	scanner_ClipToPicture,		// clip to picture
	scanner_PushState,
	scanner_PopState,

	scanner_EnterStateChange,
	scanner_ExitStateChange,
	NULL,		// enter font state
	NULL,		// exit font state

	scanner_SetOrigin,
	scanner_SetLocation,
	scanner_SetDrawOp,
	NULL,		// set line mode
	scanner_SetPenSize,
	scanner_SetForeColor,
	scanner_SetBackColor,
	scanner_SetPattern,
	scanner_SetScale,

	scanner_SetFontFamily,
	scanner_SetFontStyle,
	scanner_SetFontSpacing,
	scanner_SetFontSize,
	scanner_SetFontRotate,
	scanner_SetFontEncoding,
	scanner_SetFontFlags,
	scanner_SetFontShear,
	scanner_SetFontBPP,
	scanner_SetFontFaces
	
//	NULL		// draw picture
};


Scanner::Scanner(BFile *spool)
{
	fWarnings = 0;
	fSpoolFile = spool;
}

Scanner::~Scanner()
{
	// empty
}

bool
Scanner::CheckPictureForUnhandledOps()
{
	BPoint	where; 
	BRect	clipRect;

	fSpoolFile->Read(&where, sizeof(BPoint));
	fSpoolFile->Read(&clipRect, sizeof(BRect));

	BPicture *p = new BPicture();
	p->Unflatten(fSpoolFile);

	p->Play(ScannerCallbacks,46,this);

	delete p;

	bool isUnhandled = (fWarnings != 0);
	fWarnings = 0;

	return isUnhandled;
}

int32 
Scanner::MovePen(BPoint delta)
{
	return B_OK;
}

int32 
Scanner::StrokeLine(BPoint p1, BPoint p2)
{
	return B_OK;
}

int32 
Scanner::StrokeRect(BRect r)
{
	return B_OK;
}

int32 
Scanner::FillRect(BRect r)
{
	return B_OK;
}

int32 
Scanner::StrokeRoundRect(BRect r, BPoint radius)
{
	return B_OK;
}

int32 
Scanner::FillRoundRect(BRect r, BPoint radius)
{
	return B_OK;
}

int32 
Scanner::StrokeBezier(BPoint *pt)
{
	return B_OK;
}

int32 
Scanner::FillBezier(BPoint *pt)
{
	return B_OK;
}

int32 
Scanner::StrokeArc(BPoint center, BPoint radius, float startAngle, float endAngle)
{
	return B_OK;
}

int32 
Scanner::FillArc(BPoint center, BPoint radius, float startAngle, float endAngle)
{
	return B_OK;
}

int32 
Scanner::StrokeEllipse(BPoint center, BPoint radius)
{
	return B_OK;
}

int32 
Scanner::FillEllipse(BPoint center, BPoint radius)
{
	return B_OK;
}

int32 
Scanner::StrokePolygon(int32 ptCount, BPoint *p, bool closed)
{
	return B_OK;
}

int32 
Scanner::FillPolygon(int32 ptCount, BPoint *p)
{
	return B_OK;
}

int32 
Scanner::StrokeShape(BShape *)
{
	return B_OK;
}

int32 
Scanner::FillShape(BShape *)
{
	return B_OK;
}

int32 
Scanner::DrawString(char *string, float spacing1, float spacing2)
{
	// possible problems; check for non-ISO Latin 1 encoding...
	return B_OK;
}

int32 
Scanner::ClipToRects(BRect *rects, uint32 count)
{
	return B_OK;
}

int32 
Scanner::ClipToPicture(BPicture *pic, BPoint origin, uint32 inverse)
{
	return B_OK;
}

int32 
Scanner::PushState()
{
	return B_OK;
}

int32 
Scanner::PopState()
{
	return B_OK;
}

int32 
Scanner::SetOrigin(BPoint p)
{
	return B_OK;
}

int32 
Scanner::SetLocation(BPoint p)
{
	return B_OK;
}

int32 
Scanner::SetDrawOp(int32 drawOp)
{
	if(drawOp != B_OP_COPY && drawOp != B_OP_OVER) {
		fWarnings++;
		printf("WARNING: drawing op is not COPY or OVER...\n");
	}
	return B_OK;
}

int32 
Scanner::SetPenSize(float penSize)
{
	return B_OK;
}

int32 
Scanner::SetScale(float scale)
{
	return B_OK;
}

int32 
Scanner::SetForeColor(rgb_color color)
{
	return B_OK;
}

int32 
Scanner::SetBackColor(rgb_color color)
{
	return B_OK;
}

int32 
Scanner::SetPattern(pattern pat)
{
	bool notMatch = false;
	for(int i=0; i < 8; i++) {
		if(pat.data[i] != B_SOLID_HIGH.data[i]) {
			notMatch = true;
			break;
		}
	}

	if(notMatch == false) {
		printf("matched b_solid_high...\n");
		return 0;
	} else {
		notMatch = false;
	}

	for(int i=0; i < 8; i++) {
		if(pat.data[i] != B_SOLID_LOW.data[i]) {
			notMatch = true;
			break;
		}
	}

	if(notMatch == false) {
		printf("matched b_solid_low...\n");
		return 0;
	} else {
		fWarnings++;
		printf("WARNING: pattern used...\n");
	}
	return B_OK;
}

int32 
Scanner::SetFontFamily(char *string)
{
	return B_OK;
}

int32 
Scanner::SetFontStyle(char *string)
{
	return B_OK;
}

int32 
Scanner::SetFontSpacing(int32 spacing)
{
	return B_OK;
}

int32 
Scanner::SetFontEncoding(int32 encoding)
{
	if(encoding != B_UNICODE_UTF8) {
		fWarnings++;
		printf("WARNING: encoding is not UTF8...\n");
	}
	return B_OK;
}

int32 
Scanner::SetFontFlags(int32 flags)
{
	return B_OK;
}

int32 
Scanner::SetFontFaces(int32 faces)
{
	return B_OK;
}

int32 
Scanner::SetFontBPP(int32 bpp)
{
	return B_OK;
}

int32 
Scanner::SetFontSize(float size)
{
	return B_OK;
}

int32 
Scanner::SetFontShear(float shear)
{
	return B_OK;
}

int32 
Scanner::SetFontRotate(float rotate)
{
	return B_OK;
}

int32 
Scanner::DrawPixels(BRect src_rect, BRect dst_rect, int32 width, int32 height, int32 rowbyte, int32 format, int32 flags, uint8 *pixels)
{
	return B_OK;
}

int32 
Scanner::EnterStateChange()
{
	return B_OK;
}

int32 
Scanner::ExitStateChange()
{
	return B_OK;
}









int32 scanner_MovePen(Scanner *cp, BPoint delta)
{
	return cp->MovePen(delta);
}

int32 scanner_StrokeLine(Scanner *cp, BPoint p1, BPoint p2)
{
	return cp->StrokeLine(p1,p2);
}

int32 scanner_StrokeRect(Scanner *cp, BRect r)
{
	return cp->StrokeRect(r);
}

int32 scanner_FillRect(Scanner *cp, BRect r)
{
	return cp->FillRect(r);
}

int32 scanner_StrokeRoundRect(Scanner *cp, BRect r, BPoint radius)
{
	return cp->StrokeRoundRect(r,radius);
}

int32 scanner_StrokeBezier(Scanner *cp, BPoint *pt)
{
	return cp->StrokeBezier(pt);
}

int32 scanner_FillBezier(Scanner *cp, BPoint *pt)
{
	return cp->FillBezier(pt);
}

int32 scanner_FillRoundRect(Scanner *cp, BRect r, BPoint radius)
{
	return cp->FillRoundRect(r,radius);
}

int32 scanner_StrokeArc(Scanner *cp, BPoint center, BPoint radius, float startAngle, float endAngle)
{
	return cp->StrokeArc(center,radius,startAngle,endAngle);
}

int32 scanner_FillArc(Scanner *cp, BPoint center, BPoint radius, float startAngle, float endAngle)
{
	return cp->FillArc(center,radius,startAngle,endAngle);
}

int32 scanner_StrokeEllipse(Scanner *cp, BPoint center, BPoint radius)
{
	return cp->StrokeEllipse(center,radius);
}

int32 scanner_FillEllipse(Scanner *cp, BPoint center, BPoint radius)
{
	return cp->FillEllipse(center,radius);
}

int32 scanner_StrokePolygon(Scanner *cp, int32 ptCount, BPoint *p, bool closed)
{
	return cp->StrokePolygon(ptCount,p,closed);
}

int32 scanner_FillPolygon(Scanner *cp, int32 ptCount, BPoint *p)
{
	return cp->FillPolygon(ptCount,p);
}

int32 scanner_StrokeShape(Scanner *cp, BShape *shape)
{
	return cp->StrokeShape(shape);
}

int32 scanner_FillShape(Scanner *cp, BShape *shape)
{
	return cp->FillShape(shape);
}

int32 scanner_DrawString(Scanner *cp, char *str_ptr, float float1, float float2)
{
	return cp->DrawString(str_ptr,float1,float2);
}

int32 scanner_ClipToRects(Scanner *cp, BRect *rects, uint32 count)
{
	return cp->ClipToRects(rects, count);
}

int32 scanner_ClipToPicture(Scanner *cp, BPicture *pic, BPoint origin, uint32 inverse)
{
	return cp->ClipToPicture(pic, origin, inverse);
}

int32 scanner_PushState(Scanner *cp)
{
	return cp->PushState();
}

int32 scanner_PopState(Scanner *cp)
{
	return cp->PopState();
}

int32 scanner_SetOrigin(Scanner *cp, BPoint p)
{
	return cp->SetOrigin(p);
}

int32 scanner_SetLocation(Scanner *cp, BPoint p)
{
	return cp->SetLocation(p);
}

int32 scanner_SetDrawOp(Scanner *cp, int32 drawOp)
{
	return cp->SetDrawOp(drawOp);
}

int32 scanner_SetPenSize(Scanner *cp, float penSize)
{
	return cp->SetPenSize(penSize);
}

int32 scanner_SetScale(Scanner *cp, float scale)
{
	return cp->SetScale(scale);
}

int32 scanner_SetForeColor(Scanner *cp, rgb_color color)
{
	return cp->SetForeColor(color);
}

int32 scanner_SetBackColor(Scanner *cp, rgb_color color)
{
	return cp->SetBackColor(color);
}

int32 scanner_SetPattern(Scanner *cp, pattern pat)
{
	return cp->SetPattern(pat);
}

int32 scanner_SetFontFamily(Scanner *cp, char *string)
{
	return cp->SetFontFamily(string);
}

int32 scanner_SetFontStyle(Scanner *cp, char *string)
{
	return cp->SetFontStyle(string);
}

int32 scanner_SetFontSpacing(Scanner *cp, int32 spacing)
{
	return cp->SetFontSpacing(spacing);
}

int32 scanner_SetFontEncoding(Scanner *cp, int32 encoding)
{
	return cp->SetFontEncoding(encoding);
}

int32 scanner_SetFontFlags(Scanner *cp, int32 flags)
{
	return cp->SetFontFlags(flags);
}

int32 scanner_SetFontFaces(Scanner *cp, int32 faces)
{
	return cp->SetFontFaces(faces);
}

int32 scanner_SetFontBPP(Scanner *cp, int32 bpp)
{
	return cp->SetFontBPP(bpp);
}

int32 scanner_SetFontSize(Scanner *cp, float size)
{
	return cp->SetFontSize(size);
}

int32 scanner_SetFontShear(Scanner *cp, float shear)
{
	return cp->SetFontShear(shear);
}

int32 scanner_SetFontRotate(Scanner *cp, float rotate)
{
	return cp->SetFontRotate(rotate);
}

int32 scanner_DrawPixels(Scanner *cp, 
	BRect src_rect, BRect dst_rect,
	int32 width, int32 height, int32 rowbyte,
	int32 format, int32 flags, uint8 *pixels)
{
	return cp->DrawPixels(src_rect,dst_rect,width,height,rowbyte,format,flags,pixels);
}

int32 scanner_EnterStateChange(Scanner *cp)
{
	return cp->EnterStateChange();
}

int32 scanner_ExitStateChange(Scanner *cp)
{
	return cp->EnterStateChange();
}

