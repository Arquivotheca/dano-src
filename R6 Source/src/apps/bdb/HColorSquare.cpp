/*
	HColorSquare.cpp
	
	Copyright Hekkelman Programmatuur
	
	Created: 10/10/97 11:18:27
*/

#include "HColorSquare.h"
#include "lib.h"

#include <Bitmap.h>
#include <Message.h>

#include <algorithm>
using std::max;
using std::min;

HColorSquare::HColorSquare(BRect frame, const char *name)
	: BView(frame, name, 0, B_WILL_DRAW)
{
	BRect r(frame);
	r.InsetBy(1, 1);
	r.OffsetTo(0, 0);

	fValue = 1.0;

	fBitmap = new BBitmap(r, B_RGB_32_BIT);

	UpdateBitmap();
} /* HColorSquare::HColorSquare */

HColorSquare::~HColorSquare()
{
	delete fBitmap;
} /* HColorSquare::~HColorSquare */
		
void HColorSquare::Draw(BRect /*update*/)
{
	BRect b(Bounds());

	SetHighColor(kWhite);
	StrokeLine(b.RightTop(), b.RightBottom());
	StrokeLine(b.LeftBottom(), b.RightBottom());
	
	SetHighColor(kShadow);
	StrokeLine(b.LeftTop(), b.RightTop());
	StrokeLine(b.LeftTop(), b.LeftBottom());
	
	b.InsetBy(1, 1);
	SetHighColor(kDarkShadow);
	StrokeRect(b);

	b.InsetBy(1, 1);
	
	DrawBitmap(fBitmap, b);
	
	SetHighColor(kBlack);
	SetDrawingMode(B_OP_INVERT);
	StrokeLine(BPoint(fX, fY - 1), BPoint(fX, fY + 1));
	StrokeLine(BPoint(fX - 1, fY), BPoint(fX + 1, fY));
	SetDrawingMode(B_OP_COPY);
} /* HColorSquare::Draw */

void HColorSquare::MouseDown(BPoint where)
{
	unsigned long buttons;
	int mx, my;
	BMessage m(msg_ColorSquareChanged);
	
	mx = (int)fBitmap->Bounds().Width() + 2;
	my = (int)fBitmap->Bounds().Height() + 2;
	
	do
	{
		int nx = max(2, min((int)where.x, mx));
		int ny = max(2, min((int)where.y, my));
		
		if (nx != fX || ny != fY)
		{
			SetDrawingMode(B_OP_INVERT);
			StrokeLine(BPoint(fX, fY - 1), BPoint(fX, fY + 1));
			StrokeLine(BPoint(fX - 1, fY), BPoint(fX + 1, fY));
			SetDrawingMode(B_OP_COPY);
			
			fX = nx;
			fY = ny;
			
			SetDrawingMode(B_OP_INVERT);
			StrokeLine(BPoint(fX, fY - 1), BPoint(fX, fY + 1));
			StrokeLine(BPoint(fX - 1, fY), BPoint(fX + 1, fY));
			SetDrawingMode(B_OP_COPY);
			
			MessageReceived(&m);
		}
		
		GetMouse(&where, &buttons);
	}
	while (buttons);
} /* HColorSquare::MouseDown */

void HColorSquare::SetColor(rgb_color c)
{
	float r, g, b, a, h, s, v;
	
	rgb2f(c, r, g, b, a);
	rgb2hsv(r, g, b, h, s, v);
	
	fX = 2 + (int)(h * fBitmap->Bounds().Width());
	fY = 2 + (int)(s * fBitmap->Bounds().Height());
	
	Draw(Bounds());
} /* HColorSquare::SetColor */

rgb_color HColorSquare::Color() const
{
	float r, g, b, h, s, v;
	
	h = (fX - 2) / fBitmap->Bounds().Width();
	s = (fY - 2) / fBitmap->Bounds().Height();
	v = fValue;
	
	hsv2rgb(h, s, v, r, g, b);
	return f2rgb(r, g, b);
} /* HColorSquare::GetColor */

void HColorSquare::UpdateBitmap()
{
	int x, y, W, H;
	float h, s, v, r, g, b;
	
	v = fValue;

	W = (int)fBitmap->Bounds().Width();
	H = (int)fBitmap->Bounds().Height();
	
	for (y = 0; y <= H; y++)
	{
		rgb_color *row =
			(rgb_color *)((char *)fBitmap->Bits() + y * fBitmap->BytesPerRow());

		s = (float)y / H;

		for (x = 0; x <= W; x++)
		{
			h = (float)x / W;
			
			hsv2rgb(h, s, v, r, g, b);
			
			row[x].red = (int)(b * 255.0);
			row[x].green = (int)(g * 255.0);
			row[x].blue = (int)(r * 255.0);
		}
	}
			
} /* HColorSquare::UpdateBitmap */

void HColorSquare::SetValue(float value)
{
	fValue = value;
	UpdateBitmap();
	Draw(Bounds());
} /* HColorSquare::SetValue */
