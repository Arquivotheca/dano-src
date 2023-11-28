/*
	ColorSlider
	
	Copyright 1997, Hekkelman Programmatuur
*/

#include "HColorSlider.h"
#include "lib.h"

#include <algorithm>

#include <Message.h>
#include <Window.h>

using std::min;
using std::max;

HColorSlider::HColorSlider(BRect frame, const char *name, rgb_color max)
	: BView(frame, name, 0, B_WILL_DRAW | B_NAVIGABLE)
{
	fMax = max;
	fValue = 0;
	fHorizontal = frame.Width() > frame.Height();
} /* HColorSlider::HColorSlider */

HColorSlider::~HColorSlider()
{
} /* HColorSlider::~HColorSlider */

void HColorSlider::SetMax(rgb_color max)
{
	fMax = max;
	Draw(Bounds());
} /* HColorSlider::SetMax */

void HColorSlider::Draw(BRect /* updateRect */)
{
	BRect r = Bounds();
	
	if (IsFocus())
	{
		SetHighColor(keyboard_navigation_color());
		StrokeRect(r);
	}
	else
	{
		SetHighColor(kShadow);
		StrokeLine(r.LeftTop(), r.LeftBottom());
		StrokeLine(r.LeftTop(), r.RightTop());
		SetHighColor(kWhite);
		StrokeLine(r.LeftBottom(), r.RightBottom());
		StrokeLine(r.RightTop(), r.RightBottom());
	}

	r.InsetBy(1, 1);
	
	SetHighColor(kDarkShadow);
	StrokeRect(r);
	
	r.InsetBy(1, 1);

	BRect knob;
	float m = min(r.Width(), r.Height());
	knob.Set(r.left, r.top, m + r.left, m + r.top);
	
	if (fHorizontal)
	{
		knob.OffsetBy((r.Width() - m) * fValue, 0);
		
		BeginLineArray((int)r.Width() + 1);
		
		for (float x = r.left; x <= r.right; x += 1)
		{
			float v = (x - r.left) / (r.Width() + 1);
			rgb_color c = fMax;
			c.red = (int)((float)c.red * v);
			c.green = (int)((float)c.green * v);
			c.blue = (int)((float)c.blue * v);
			
			AddLine(BPoint(x, r.top), BPoint(x, r.bottom), c);
		}
		
		EndLineArray();
	}
	else
	{
		float a = r.Height();
		knob.OffsetBy(0, a * (1.0 - fValue));
		
		BeginLineArray((int)a + 1);
		
		float y = r.top;
		for (;y <= r.bottom; y += 1)
		{
			float v = 1.0 - (y - 1) / a;
			rgb_color c = fMax;
			c.red = (int)((float)c.red * v);
			c.green = (int)((float)c.green * v);
			c.blue = (int)((float)c.blue * v);
			
			float Y = y < knob.top ? y : y + knob.Height();
			
			AddLine(BPoint(r.left, Y), BPoint(r.right, Y), c);
		}
		
		EndLineArray();
	}
	
	SetHighColor(kViewColor);
	FillRect(knob);
	
	BeginLineArray(6);
	AddLine(BPoint(knob.left + 1, knob.top + 1),	BPoint(knob.left + 1, knob.bottom - 2),	kWhite);
	AddLine(BPoint(knob.left + 1, knob.top + 1),	BPoint(knob.right - 2, knob.top + 1),	kWhite);
	AddLine(BPoint(knob.left, knob.bottom),			BPoint(knob.right, knob.bottom),		kDarkShadow);
	AddLine(BPoint(knob.right, knob.bottom),		BPoint(knob.right, knob.top), 			kDarkShadow);
	AddLine(BPoint(knob.left + 2, knob.bottom - 1),	BPoint(knob.right - 1, knob.bottom - 1),kShadow);
	AddLine(BPoint(knob.right - 1, knob.bottom - 1),BPoint(knob.right - 1, knob.top + 1), 	kShadow);
	EndLineArray();
	
	SetHighColor(kBlack);
	
	Sync();
} /* HColorSlider::Draw */

void HColorSlider::MouseDown(BPoint where)
{
	BRect b(Bounds()), knob;
	
	b.InsetBy(1, 1);
	
	float a, s;
	
	knob = b;
	
	if (fHorizontal)
	{
		knob.right = b.left + b.Height();
		a = b.Width() - knob.Width();
		knob.OffsetBy(fValue * a, 0);
		s = where.x - knob.left;
	}
	else
	{
		knob.bottom = b.top + b.Width();
		a = b.Height() - knob.Height();
		knob.OffsetBy(0, (1 - fValue) * a);
		s = where.y - knob.top;
	}
	
	if (knob.Contains(where))
	{
		BPoint oldPt(-1, -1);
		ulong btns;
		BMessage msg(msg_SliderChanged);
		msg.AddFloat("value", fValue);
		msg.AddPointer("sender", this);
		
		do
		{
			if (oldPt != where)
			{
				if (fHorizontal)
					fValue = min(1.0, max(0.0, (where.x - s) / (double)a));
				else
					fValue = 1 - min(1.0, max(0.0, (where.y - s) / (double)a));
				
				msg.ReplaceFloat("value", fValue);
				
				Draw(Bounds());
				MessageReceived(&msg);
				
				oldPt = where;
			}
			
			GetMouse(&where, &btns);
		}
		while (btns);
		
		Window()->PostMessage(msg_EndTracking);
	}
} /* HColorSlider::MouseDown */

float HColorSlider::Value() const
{
	return fValue;
} /* HColorSlider::Value */

void HColorSlider::SetValue(float v)
{
	fValue = v;
	Draw(Bounds());
} /* HColorSlider::SetValue */

void HColorSlider::MakeFocus(bool focus)
{
	BView::MakeFocus(focus);
	Draw(Bounds());
} /* HColorSlider::MakeFocus */

void HColorSlider::KeyDown(const char *bytes, int32 numBytes)
{
	BRect r(Bounds());
	
	r.InsetBy(1, 1);
	float a, nv = fValue;
	
	if (fHorizontal)
		a = r.Width() - r.Height();
	else
		a = r.Height() - r.Width();
	
	switch (bytes[0])
	{
		case B_UP_ARROW:
			if (!fHorizontal)
				nv = min(1.0, (fValue * a + 1) / (double)a);
			break;
			
		case B_LEFT_ARROW:
			if (fHorizontal)
				nv = max(0.0, (fValue * a - 1) / (double)a);
			break;
		
		case B_DOWN_ARROW:
			if (!fHorizontal)
				nv = max(0.0, (fValue * a - 1) / (double)a);
			break;
		
		case B_RIGHT_ARROW:
			if (fHorizontal)
				nv = min(1.0, (fValue * a + 1) / (double)a);
			break;
		
		default:
			BView::KeyDown(bytes, numBytes);
	}
	
	if (nv != fValue)
	{
		fValue = nv;
		Draw(Bounds());

		BMessage msg(msg_SliderChanged);
		msg.AddFloat("value", fValue);
		msg.AddPointer("sender", this);
		MessageReceived(&msg);
	}
} /* HColorSlider::KeyDown */
