
#include "BDrawable.h"
#include "interface/Bitmap.h"

BDrawable::BDrawable()
{
}

BDrawable::~BDrawable()
{
}

bool BDrawable::IsPrinting() const
{
	return false;
}

void BDrawable::MovePenBy(BPoint pt)
{
	MovePenTo(PenLocation() + pt);
}

void BDrawable::StrokeLine(BPoint pt0, BPoint pt1)
{
	MovePenTo(pt0);
	StrokeLineTo(pt1);
}

void BDrawable::BeginLineArray(int32 /*count*/)
{
}

void BDrawable::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	SetHighColor(col);
	StrokeLine(pt0, pt1);
}

void BDrawable::EndLineArray()
{
}

void BDrawable::StrokeTriangle(BPoint pt1, BPoint pt2, BPoint pt3)
{
	BPoint poly[3];
	poly[0] = pt1;
	poly[1] = pt2;
	poly[2] = pt3;
	StrokePolygon(poly, 3, true);
}

void BDrawable::FillTriangle(BPoint pt1, BPoint pt2, BPoint pt3)
{
	BPoint poly[3];
	poly[0] = pt1;
	poly[1] = pt2;
	poly[2] = pt3;
	FillPolygon(poly, 3);
}

void BDrawable::StrokeInscribedEllipse(BRect r)
{
	StrokeEllipse(BPoint((r.left+r.right)/2, (r.top+r.bottom)/2),
				  r.Width()/2, r.Height()/2);
}

void BDrawable::FillInscribedEllipse(BRect r)
{
	FillEllipse(BPoint((r.left+r.right)/2, (r.top+r.bottom)/2),
				r.Width()/2, r.Height()/2);
}

void BDrawable::StrokeInscribedArc(	BRect r,
									float startAngle,
									float arcAngle)
{
	StrokeArc(BPoint((r.left+r.right)/2, (r.top+r.bottom)/2),
				r.Width()/2, r.Height()/2, startAngle, arcAngle);
}

void BDrawable::FillInscribedArc(	BRect r,
									float startAngle,
									float arcAngle)
{
	FillArc(BPoint((r.left+r.right)/2, (r.top+r.bottom)/2),
			r.Width()/2, r.Height()/2, startAngle, arcAngle);
}

// --------------- Convenience Functions ---------------

source_alpha BDrawable::BlendingSourceAlpha() const
{
	source_alpha srcAlpha;
	alpha_function alphaFunc;
	GetBlendingMode(&srcAlpha, &alphaFunc);
	return srcAlpha;
}

alpha_function BDrawable::BlendingAlphaFunction() const
{
	source_alpha srcAlpha;
	alpha_function alphaFunc;
	GetBlendingMode(&srcAlpha, &alphaFunc);
	return alphaFunc;
}
		
join_mode BDrawable::LineJoinMode() const
{
	cap_mode lineCap;
	join_mode lineJoin;
	float miterLimit;
	GetLineMode(&lineCap, &lineJoin, &miterLimit);
	return lineJoin;
}

cap_mode BDrawable::LineCapMode() const
{
	cap_mode lineCap;
	join_mode lineJoin;
	float miterLimit;
	GetLineMode(&lineCap, &lineJoin, &miterLimit);
	return lineCap;
}

float BDrawable::LineMiterLimit() const
{
	cap_mode lineCap;
	join_mode lineJoin;
	float miterLimit;
	GetLineMode(&lineCap, &lineJoin, &miterLimit);
	return miterLimit;
}

void BDrawable::DrawBitmapAt(const BBitmap *aBitmap, BPoint where)
{
	BRect b(aBitmap->Bounds());
	DrawClippedScaledBitmap(aBitmap, b, b.OffsetToCopy(where));
}

void BDrawable::DrawBitmap(const BBitmap *aBitmap)
{
	BRect b(aBitmap->Bounds());
	DrawClippedScaledBitmap(aBitmap, b, b.OffsetToCopy(PenLocation()));
}

void BDrawable::DrawScaledBitmap(const BBitmap *aBitmap, BRect dstRect)
{
	DrawClippedScaledBitmap(aBitmap, aBitmap->Bounds(), dstRect);
}

void BDrawable::SetFontSize(float size)
{
	BFont font;
	font.SetSize(size);
	SetFont(&font, B_FONT_SIZE);
}

void BDrawable::GetFontHeight(font_height* height) const
{
	BFont font;
	GetFont(&font);
	font.GetHeight(height);
}

float BDrawable::StringWidth(const char* str, int32 length) const
{
	BFont font;
	GetFont(&font);
	if (length < 0) return font.StringWidth(str);
	else return font.StringWidth(str, length);
}

void BDrawable::TruncateString(	BString* in_out,
										uint32 mode,
										float width) const
{
	BFont font;
	GetFont(&font);
	font.TruncateString(in_out, mode, width);
}

void BDrawable::DrawChar(char aChar)
{
	DrawString(&aChar, 1, NULL);
}

void BDrawable::DrawCharAt(char aChar, BPoint location)
{
	MovePenTo(location);
	DrawString(&aChar, 1, NULL);
}

void BDrawable::DrawStringAt(const char *aString, BPoint location,
							int32 length, escapement_delta *delta)
{
	MovePenTo(location);
	DrawString(aString, length, delta);
}

void BDrawable::DrawPictureAt(const BPicture *aPicture, BPoint where)
{
	MovePenTo(where);
	DrawPicture(aPicture);
}

