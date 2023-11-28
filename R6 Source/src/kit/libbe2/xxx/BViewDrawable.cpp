
#include "BViewDrawable.h"
#include <View.h>

BViewDrawable::BViewDrawable(BView *view)
{
	m_view = view;
}

BViewDrawable::~BViewDrawable()
{
}
bool 
BViewDrawable::IsPrinting() const
{
}

void 
BViewDrawable::Flush() const
{
}

void 
BViewDrawable::Sync() const
{
}

void 
BViewDrawable::PushState()
{
	m_view->PushState();
}

void 
BViewDrawable::PopState()
{
	m_view->PopState();
}

void 
BViewDrawable::MoveOrigin(BPoint delta)
{
	m_view->SetOrigin(m_view->Origin()+delta);
}

void 
BViewDrawable::ScaleBy(float xAmount, float yAmount)
{
}

void 
BViewDrawable::ClipToPicture(BPicture *picture, BPoint where, bool sync)
{
}

void 
BViewDrawable::ClipToInversePicture(BPicture *picture, BPoint where, bool sync)
{
}

void 
BViewDrawable::SetDrawingMode(drawing_mode mode)
{
}

drawing_mode 
BViewDrawable::DrawingMode() const
{
}

void 
BViewDrawable::SetBlendingMode(source_alpha srcAlpha, alpha_function alphaFunc)
{
}

void 
BViewDrawable::GetBlendingMode(source_alpha *srcAlpha, alpha_function *alphaFunc) const
{
}

void 
BViewDrawable::SetHighColor(rgb_color a_color)
{
	m_view->SetHighColor(a_color);
}

rgb_color 
BViewDrawable::HighColor() const
{
}

void 
BViewDrawable::SetLowColor(rgb_color a_color)
{
}

rgb_color 
BViewDrawable::LowColor() const
{
}

void 
BViewDrawable::SetFont(const BFont *font, uint32 mask)
{
}

void 
BViewDrawable::GetFont(BFont *font) const
{
}

void 
BViewDrawable::ForceFontAliasing(bool enable)
{
}

void 
BViewDrawable::SetPenSize(float size)
{
}

float 
BViewDrawable::PenSize() const
{
}

void 
BViewDrawable::SetLineMode(cap_mode lineCap, join_mode lineJoin, float miterLimit)
{
}

void 
BViewDrawable::GetLineMode(cap_mode *lineCap, join_mode *lineJoin, float *miterLimit) const
{
}

void 
BViewDrawable::MovePenTo(BPoint pt)
{
}

BPoint 
BViewDrawable::PenLocation() const
{
}

void 
BViewDrawable::StrokeLineTo(BPoint toPt)
{
}

void 
BViewDrawable::StrokePolygon(const BPoint *ptArray, int32 numPts, bool closed)
{
}

void 
BViewDrawable::FillPolygon(const BPoint *ptArray, int32 numPts)
{
}

void 
BViewDrawable::StrokeRect(BRect r)
{
	m_view->StrokeRect(r);
}

void 
BViewDrawable::FillRect(BRect r)
{
	m_view->FillRect(r);
}

void 
BViewDrawable::InvertRect(BRect r)
{
}

void 
BViewDrawable::StrokeRoundRect(BRect r, float xRadius, float yRadius)
{
}

void 
BViewDrawable::FillRoundRect(BRect r, float xRadius, float yRadius)
{
}

void 
BViewDrawable::StrokeEllipse(BPoint center, float xRadius, float yRadius)
{
}

void 
BViewDrawable::FillEllipse(BPoint center, float xRadius, float yRadius)
{
}

void 
BViewDrawable::StrokeArc(BPoint center, float xRadius, float yRadius, float startAngle, float arcAngle)
{
}

void 
BViewDrawable::FillArc(BPoint center, float xRadius, float yRadius, float startAngle, float arcAngle)
{
}

void 
BViewDrawable::StrokeBezier(BPoint *controlPoints)
{
}

void 
BViewDrawable::FillBezier(BPoint *controlPoints)
{
}

void 
BViewDrawable::StrokeShape(BShape *shape)
{
}

void 
BViewDrawable::FillShape(BShape *shape)
{
}

void 
BViewDrawable::DrawPicture(const BPicture *aPicture)
{
}

void 
BViewDrawable::DrawString(const char *aString, int32 length, escapement_delta *delta)
{
}

void 
BViewDrawable::DrawClippedScaledBitmap(const BBitmap *aBitmap, BRect srcRect, BRect dstRect)
{
	m_view->DrawBitmapAsync(aBitmap,srcRect,dstRect);
}

BView *
BViewDrawable::View()
{
	return m_view;
}
