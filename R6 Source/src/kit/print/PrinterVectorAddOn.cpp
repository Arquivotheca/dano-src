// ***********************************************************************
// libprint.so
//
// (c) 2000, Be Incorporated
//
// Written by : Mathias Agopian
//
// ***********************************************************************

#include <stdio.h>
#include <print/PrintJobSettings.h>
#include <print/PrinterAddOnDefs.h>
#include <print/PrinterVectorAddOn.h>


#define m _rPrivate

namespace BPrivate
{
	struct _printer_vector_addon_data
	{
		bool cancelled;
	};	

	// The C callback functions
	int32 cb_MovePen(BPrintPictureIterator *, BPoint delta);
	int32 cb_StrokeLine(BPrintPictureIterator *, BPoint p1, BPoint p2);
	int32 cb_StrokeRect(BPrintPictureIterator *, BRect r);
	int32 cb_FillRect(BPrintPictureIterator *, BRect r);
	int32 cb_StrokeRoundRect(BPrintPictureIterator *, BRect r, BPoint radius);
	int32 cb_FillRoundRect(BPrintPictureIterator *, BRect r, BPoint radius);
	int32 cb_StrokeBezier(BPrintPictureIterator *, BPoint *pt);
	int32 cb_FillBezier(BPrintPictureIterator *, BPoint *pt);
	int32 cb_StrokeArc(BPrintPictureIterator *, BPoint center, BPoint radius, float startAngle, float endAngle);
	int32 cb_FillArc(BPrintPictureIterator *, BPoint center, BPoint radius, float startAngle, float endAngle);
	int32 cb_StrokeEllipse(BPrintPictureIterator *, BPoint center, BPoint radius);
	int32 cb_FillEllipse(BPrintPictureIterator *, BPoint center, BPoint radius);
	int32 cb_StrokePolygon(BPrintPictureIterator *, int32 ptCount, BPoint *p, bool closed);
	int32 cb_FillPolygon(BPrintPictureIterator *, int32 ptCount, BPoint *p);
	int32 cb_StrokeShape(BPrintPictureIterator *, BShape *shape);
	int32 cb_FillShape(BPrintPictureIterator *, BShape *shape);
	int32 cb_DrawString(BPrintPictureIterator *, char *str_ptr, float float1, float float2);
	int32 cb_ClipToRects(BPrintPictureIterator *, BRect *rects, int32 count);
	int32 cb_ClipToPicture(BPrintPictureIterator *, BPicture *pic, BPoint origin, uint32 inverse);
	int32 cb_PushState(BPrintPictureIterator *);
	int32 cb_PopState(BPrintPictureIterator *);
	int32 cb_EnterFontState(BPrintPictureIterator *);
	int32 cb_ExitFontState(BPrintPictureIterator *);
	int32 cb_SetOrigin(BPrintPictureIterator *, BPoint p);
	int32 cb_SetPenLocation(BPrintPictureIterator *, BPoint p);
	int32 cb_SetDrawOp(BPrintPictureIterator *, int32 drawOp);
	int32 cb_SetPenSize(BPrintPictureIterator *, float penSize);
	int32 cb_SetScale(BPrintPictureIterator *, float scale);
	int32 cb_SetForeColor(BPrintPictureIterator *, rgb_color color);
	int32 cb_SetBackColor(BPrintPictureIterator *, rgb_color color);
	int32 cb_SetStipplePattern(BPrintPictureIterator *, pattern pat);
	int32 cb_SetFontFamily(BPrintPictureIterator *, char *string);
	int32 cb_SetFontStyle(BPrintPictureIterator *, char *string);
	int32 cb_SetFontSpacing(BPrintPictureIterator *, int32 spacing);
	int32 cb_SetFontEncoding(BPrintPictureIterator *, int32 encoding);
	int32 cb_SetFontFlags(BPrintPictureIterator *, int32 flags);
	int32 cb_SetFontFaces(BPrintPictureIterator *, int32 faces);
	int32 cb_SetFontBPP(BPrintPictureIterator *, int32 bpp);
	int32 cb_SetFontSize(BPrintPictureIterator *, float size);
	int32 cb_SetFontShear(BPrintPictureIterator *, float shear);
	int32 cb_SetFontRotate(BPrintPictureIterator *, float rotate);
	int32 cb_DrawPixels(BPrintPictureIterator *, BRect, BRect, int32, int32, int32, int32, int32, void *);
	int32 cb_EnterStateChange(BPrintPictureIterator *);
	int32 cb_ExitStateChange(BPrintPictureIterator *);
	int32 cb_SetLineMode(BPrintPictureIterator *, int32, int32, float);
	int32 cb_DrawPicture(BPrintPictureIterator *, BPicture *picture, BPoint *p);

	void *PictureCallbacks[] = {
		NULL,
		(void*)cb_MovePen,	(void*)cb_StrokeLine,
		(void*)cb_StrokeRect, (void*)cb_FillRect,	(void*)cb_StrokeRoundRect,	(void*)cb_FillRoundRect, (void*)cb_StrokeBezier, (void*)cb_FillBezier, (void*)cb_StrokeArc, (void*)cb_FillArc,
		(void*)cb_StrokeEllipse, (void*)cb_FillEllipse, (void*)cb_StrokePolygon,	(void*)cb_FillPolygon,	(void*)cb_StrokeShape, (void*)cb_FillShape, (void*)cb_DrawString, (void*)cb_DrawPixels,
		NULL,
		(void*)cb_ClipToRects, (void*)cb_ClipToPicture, (void*)cb_PushState, (void*)cb_PopState, (void*)cb_EnterStateChange, (void*)cb_ExitStateChange, (void*)cb_EnterFontState, (void*)cb_ExitFontState,
		(void*)cb_SetOrigin, (void*)cb_SetPenLocation, (void*)cb_SetDrawOp,
		(void*)cb_SetLineMode, (void*)cb_SetPenSize, (void*)cb_SetForeColor, (void*)cb_SetBackColor, (void*)cb_SetStipplePattern, (void*)cb_SetScale,
		(void*)cb_SetFontFamily, (void*)cb_SetFontStyle, (void*)cb_SetFontSpacing, (void*)cb_SetFontSize, (void*)cb_SetFontRotate, (void*)cb_SetFontEncoding, (void*)cb_SetFontFlags, (void*)cb_SetFontShear,
		(void*)cb_SetFontBPP, (void*)cb_SetFontFaces, (void*)cb_DrawPicture
	};
	const int kNbCallBackFunctions = sizeof(PictureCallbacks)/sizeof(PictureCallbacks[0]);

} using namespace BPrivate;


BPrinterVectorAddOn::BPrinterVectorAddOn(BTransportIO* transport, BNode *printer_file)
	: 	BPrinterAddOn(transport, printer_file),
		BPrintPictureIterator(*this),
	 	_fPrivate(new _printer_vector_addon_data),
		_rPrivate(*_fPrivate)
{
	m.cancelled = false;
}

BPrinterVectorAddOn::~BPrinterVectorAddOn()
{
	delete _fPrivate;
}

status_t BPrinterVectorAddOn::BeginJob()
{
	return BPrinterAddOn::BeginJob();
}

status_t BPrinterVectorAddOn::EndJob()
{
	return BPrinterAddOn::EndJob();
}

status_t BPrinterVectorAddOn::BeginPage()
{
	return B_OK;
}

status_t BPrinterVectorAddOn::EndPage()
{
	return B_OK;
}

status_t BPrinterVectorAddOn::Print(const page_t& page, const int nbCopies)
{
	const uint32 nbPictures = page.picture_count;
	BPicture const * const * pictures = page.pictures;
	const BRect *clips = page.clips;
	const BPoint *where = page.points;
	status_t result = B_OK;
	const float rendering_resolution = min_c(Settings().DeviceXdpi(), Settings().DeviceYdpi());

	// Compute the scale factor
	const float scale_factor = (rendering_resolution / (float)Settings().Xdpi()) * Settings().Scale();
	if ((result = cb_SetScale(this, scale_factor)) != B_OK)
		return result;

	// Handle multiple copy
	for (int i=0 ; ((i<nbCopies) && (result == B_OK) && (m.cancelled == false)); i++)
	{
		if ((result = BeginPage()) != B_OK)
			break;

		if ((result = cb_PushState(this)) != B_OK)
			return result;

		for (int i=0 ; i<nbPictures ; i++)
		{
			if ((result = cb_SetOrigin(this, where[i])) != B_OK)		// REVISIT: should we multiply by scale_factor?
				return result;

			// REVISIT: we should clip here. How?

			if ((result = cb_PushState(this)) != B_OK)
				return result;

			if ((result = Iterate(pictures[i])) != B_OK)
				return result;

			if ((result = cb_PopState(this)) != B_OK)
				return result;
		}
		
		if ((result = cb_PopState(this)) != B_OK)
			return result;

		if ((result = EndPage()) != B_OK)
			break;
	}

	return result;
}


status_t BPrinterVectorAddOn::Cancel()
{
	m.cancelled = true;
	return BPrinterAddOn::Cancel();
}

// ----------------------------------------------------------------------------
// #pragma mark -

status_t BPrinterVectorAddOn::_Reserved_BPrinterVectorAddOn_0(int32 arg, ...) { return B_ERROR; }
status_t BPrinterVectorAddOn::_Reserved_BPrinterVectorAddOn_1(int32 arg, ...) { return B_ERROR; }
status_t BPrinterVectorAddOn::_Reserved_BPrinterVectorAddOn_2(int32 arg, ...) { return B_ERROR; }
status_t BPrinterVectorAddOn::_Reserved_BPrinterVectorAddOn_3(int32 arg, ...) { return B_ERROR; }
status_t BPrinterVectorAddOn::Perform(int32 arg, ...) { return B_ERROR; }


// ----------------------------------------------------------------------------
// #pragma mark -

BPrintPictureIterator::BPrintPictureIterator(BPrinterVectorAddOn& driver)
	: fDriver(driver)
{
}

BPrintPictureIterator::~BPrintPictureIterator()
{
}

status_t BPrintPictureIterator::Iterate(const BPicture *picture)
{
	return ((BPicture *)picture)->Play(PictureCallbacks, kNbCallBackFunctions, this);
}

BPrinterVectorAddOn& BPrintPictureIterator::Driver()
{
	return fDriver;
}

status_t BPrintPictureIterator::_Reserved_BPrintPictureIterator_0(int32 arg, ...) { return B_ERROR; }
status_t BPrintPictureIterator::_Reserved_BPrintPictureIterator_1(int32 arg, ...) { return B_ERROR; }
status_t BPrintPictureIterator::_Reserved_BPrintPictureIterator_2(int32 arg, ...) { return B_ERROR; }
status_t BPrintPictureIterator::_Reserved_BPrintPictureIterator_3(int32 arg, ...) { return B_ERROR; }
status_t BPrintPictureIterator::_Reserved_BPrintPictureIterator_4(int32 arg, ...) { return B_ERROR; }
status_t BPrintPictureIterator::_Reserved_BPrintPictureIterator_5(int32 arg, ...) { return B_ERROR; }
status_t BPrintPictureIterator::_Reserved_BPrintPictureIterator_6(int32 arg, ...) { return B_ERROR; }
status_t BPrintPictureIterator::_Reserved_BPrintPictureIterator_7(int32 arg, ...) { return B_ERROR; }
status_t BPrintPictureIterator::Perform(int32 arg, ...) { return B_ERROR; }


// ----------------------------------------------------------------------------
// #pragma mark -
// Default hooks

status_t BPrintPictureIterator::MovePen(const BPoint& p) { return B_OK; }
status_t BPrintPictureIterator::StrokeLine(const BPoint& p0, const BPoint& p1) { return B_OK; }
status_t BPrintPictureIterator::StrokeRect(const BRect& r) { return B_OK; }
status_t BPrintPictureIterator::FillRect(const BRect& r) { return B_OK; }
status_t BPrintPictureIterator::StrokeRoundRect(const BRect& r, const BPoint& p) { return B_OK; }
status_t BPrintPictureIterator::FillRoundRect(const BRect& r, const BPoint& p) { return B_OK; }
status_t BPrintPictureIterator::StrokeBezier(const BPoint* p) { return B_OK; }
status_t BPrintPictureIterator::FillBezier(const BPoint* p) { return B_OK; }
status_t BPrintPictureIterator::StrokeArc(const BPoint& rx, const BPoint& ry, float alpha, float beta) { return B_OK; }
status_t BPrintPictureIterator::FillArc(const BPoint& rx, const BPoint& ry, float alpha, float beta) { return B_OK; }
status_t BPrintPictureIterator::StrokeEllipse(const BPoint& rx, const BPoint& ry) { return B_OK; }
status_t BPrintPictureIterator::FillEllipse(const BPoint& rx, const BPoint& ry) { return B_OK; }
status_t BPrintPictureIterator::StrokePolygon(int32 count, const BPoint* p, bool closed) { return B_OK; }
status_t BPrintPictureIterator::FillPolygon(int32 count, const BPoint* p) { return B_OK; }
status_t BPrintPictureIterator::StrokeShape(const BShape& shape) { return B_OK; }
status_t BPrintPictureIterator::FillShape(const BShape& shape) { return B_OK; }
status_t BPrintPictureIterator::DrawString(const char *s, float e0, float e1) { return B_OK; }
status_t BPrintPictureIterator::DrawPixels(const BRect& src, const BRect& dst, int32 w, int32 h, int32 rowByte, color_space format, int32 flags, const void *buffer) { return B_OK; }
status_t BPrintPictureIterator::ClipToRects(int32 count, const BRect *r) { return B_OK; }
status_t BPrintPictureIterator::ClipToPicture(BPicture& picture, const BPoint& p, uint32 inverse) { return B_OK; }
status_t BPrintPictureIterator::PushState() { return B_OK; }
status_t BPrintPictureIterator::PopState() { return B_OK; }
status_t BPrintPictureIterator::EnterStateChange() { return B_OK; }
status_t BPrintPictureIterator::ExitStateChange() { return B_OK; }
status_t BPrintPictureIterator::EnterFontState() { return B_OK; }
status_t BPrintPictureIterator::ExitFontState() { return B_OK; }
status_t BPrintPictureIterator::SetOrigin(const BPoint& p) { return B_OK; }
status_t BPrintPictureIterator::SetPenLocation(const BPoint& p) { return B_OK; }
status_t BPrintPictureIterator::SetDrawOp(drawing_mode mode) { return B_OK; }
status_t BPrintPictureIterator::SetLineMode(int32, int32, float) { return B_OK; }
status_t BPrintPictureIterator::SetPenSize(float size) { return B_OK; }
status_t BPrintPictureIterator::SetForeColor(const rgb_color& c) { return B_OK; }
status_t BPrintPictureIterator::SetBackColor(const rgb_color& c) { return B_OK; }
status_t BPrintPictureIterator::SetStipplePattern(const pattern& p) { return B_OK; }
status_t BPrintPictureIterator::SetScale(float scale) { return B_OK; }
status_t BPrintPictureIterator::SetFontFamily(const char *family) { return B_OK; }
status_t BPrintPictureIterator::SetFontStyle(const char *style) { return B_OK; }
status_t BPrintPictureIterator::SetFontSpacing(int32 spacing_mode) { return B_OK; }
status_t BPrintPictureIterator::SetFontSize(float size) { return B_OK; }
status_t BPrintPictureIterator::SetFontRotate(float alpha) { return B_OK; }
status_t BPrintPictureIterator::SetFontEncoding(int32) { return B_OK; }
status_t BPrintPictureIterator::SetFontFlags(int32 flags) { return B_OK; }
status_t BPrintPictureIterator::SetFontShear(float shear) { return B_OK; }
status_t BPrintPictureIterator::SetFontBPP(int32 bpp) { return B_OK; }
status_t BPrintPictureIterator::SetFontFaces(int32 face) { return B_OK; }
status_t BPrintPictureIterator::DrawPicture(const BPicture& picture, const BPoint& p) { return B_OK; }

// ----------------------------------------------------------------------------

namespace BPrivate
{
int32 cb_MovePen(BPrintPictureIterator *This, BPoint delta) {
	return This->MovePen(delta);
}
int32 cb_StrokeLine(BPrintPictureIterator *This, BPoint p1, BPoint p2) {
	return This->StrokeLine(p1, p2);
}
int32 cb_StrokeRect(BPrintPictureIterator *This, BRect r) {
	return This->StrokeRect(r);
}
int32 cb_FillRect(BPrintPictureIterator *This, BRect r) {
	return This->FillRect(r);
}
int32 cb_StrokeRoundRect(BPrintPictureIterator *This, BRect r, BPoint radius) {
	return This->StrokeRoundRect(r, radius);
}
int32 cb_FillRoundRect(BPrintPictureIterator *This, BRect r, BPoint radius) {
	return This->FillRoundRect(r, radius);
}
int32 cb_StrokeBezier(BPrintPictureIterator *This, BPoint *pt) {
	return This->StrokeBezier(pt);
}
int32 cb_FillBezier(BPrintPictureIterator *This, BPoint *pt) {
	return This->FillBezier(pt);
}
int32 cb_StrokeArc(BPrintPictureIterator *This, BPoint center, BPoint radius, float startAngle, float endAngle) {
	return This->StrokeArc(center, radius, startAngle, endAngle);
}
int32 cb_FillArc(BPrintPictureIterator *This, BPoint center, BPoint radius, float startAngle, float endAngle) {
	return This->FillArc(center, radius, startAngle, endAngle);
}
int32 cb_StrokeEllipse(BPrintPictureIterator *This, BPoint center, BPoint radius) {
	return This->StrokeEllipse(center, radius);
}
int32 cb_FillEllipse(BPrintPictureIterator *This, BPoint center, BPoint radius) {
	return This->FillEllipse(center, radius);
}
int32 cb_StrokePolygon(BPrintPictureIterator *This, int32 ptCount, BPoint *p, bool closed) {
	return This-> StrokePolygon(ptCount, p, closed);
}
int32 cb_FillPolygon(BPrintPictureIterator *This, int32 ptCount, BPoint *p) {
	return This->FillPolygon(ptCount, p);
}
int32 cb_StrokeShape(BPrintPictureIterator *This, BShape *shape) {
	return This->StrokeShape(*shape);
}
int32 cb_FillShape(BPrintPictureIterator *This, BShape *shape) {
	return This->FillShape(*shape);
}
int32 cb_DrawString(BPrintPictureIterator *This, char *str_ptr, float float1, float float2) {
	return This->DrawString(str_ptr, float1, float2);
}
int32 cb_ClipToRects(BPrintPictureIterator *This, BRect *rects, int32 count) {
	return This->ClipToRects(count, rects);
}
int32 cb_ClipToPicture(BPrintPictureIterator *This, BPicture *pic, BPoint origin, uint32 inverse) {
	return This->ClipToPicture(*pic, origin, inverse);
}
int32 cb_PushState(BPrintPictureIterator *This) {
	return This->PushState();
}
int32 cb_PopState(BPrintPictureIterator *This) {
	return This->PopState();
}
int32 cb_EnterFontState(BPrintPictureIterator *This) {
	return This->EnterFontState();
}
int32 cb_ExitFontState(BPrintPictureIterator *This) {
	return This->ExitFontState();
}
int32 cb_SetOrigin(BPrintPictureIterator *This, BPoint p) {
	return This->SetOrigin(p);
}
int32 cb_SetPenLocation(BPrintPictureIterator *This, BPoint p) {
	return This->SetPenLocation(p);
}
int32 cb_SetDrawOp(BPrintPictureIterator *This, int32 drawOp) {
	return This->SetDrawOp((drawing_mode)drawOp);
}
int32 cb_SetPenSize(BPrintPictureIterator *This, float penSize) {
	return This->SetPenSize(penSize);
}
int32 cb_SetScale(BPrintPictureIterator *This, float scale) {
	return This->SetScale(scale);
}
int32 cb_SetForeColor(BPrintPictureIterator *This, rgb_color color) {
	return This->SetForeColor(color);
}
int32 cb_SetBackColor(BPrintPictureIterator *This, rgb_color color) {
	return This->SetBackColor(color);
}
int32 cb_SetStipplePattern(BPrintPictureIterator *This, pattern pat) {
	return This->SetStipplePattern(pat);
}
int32 cb_SetFontFamily(BPrintPictureIterator *This, char *string) {
	return This->SetFontFamily(string);
}
int32 cb_SetFontStyle(BPrintPictureIterator *This, char *string) {
	return This->SetFontStyle(string);
}
int32 cb_SetFontSpacing(BPrintPictureIterator *This, int32 spacing) {
	return This->SetFontSpacing(spacing);
}
int32 cb_SetFontEncoding(BPrintPictureIterator *This, int32 encoding) {
	return This->SetFontEncoding(encoding);
}
int32 cb_SetFontFlags(BPrintPictureIterator *This, int32 flags) {
	return This->SetFontFlags(flags);
}
int32 cb_SetFontFaces(BPrintPictureIterator *This, int32 faces) {
	return This->SetFontFaces(faces);
}
int32 cb_SetFontBPP(BPrintPictureIterator *This, int32 bpp) {
	return This->SetFontBPP(bpp);
}
int32 cb_SetFontSize(BPrintPictureIterator *This, float size) {
	return This->SetFontSize(size);
}
int32 cb_SetFontShear(BPrintPictureIterator *This, float shear) {
	return This->SetFontShear(shear);
}
int32 cb_SetFontRotate(BPrintPictureIterator *This, float rotate) {
	return This->SetFontRotate(rotate);
}
int32 cb_DrawPixels(BPrintPictureIterator *This, BRect s, BRect d, int32 w, int32 h, int32 r, int32 c, int32 f, void *bitmap) {
	return This->DrawPixels(s, d, w, h, r, (color_space)c, f, bitmap);
}
int32 cb_EnterStateChange(BPrintPictureIterator *This) {
	return This->EnterStateChange();
}
int32 cb_ExitStateChange(BPrintPictureIterator *This) {
	return This->ExitStateChange();
}
int32 cb_SetLineMode(BPrintPictureIterator *This, int32 b, int32 c, float s) {
	return This->SetLineMode(b, c, s);
}
int32 cb_DrawPicture(BPrintPictureIterator *This, BPicture *picture, BPoint *p) {
	return This->DrawPicture(*picture, *p);
}
}

