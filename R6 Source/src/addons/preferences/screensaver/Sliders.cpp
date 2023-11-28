#include "Sliders.h"
#include <StringView.h>
#include <InterfaceDefs.h>
#include <stdio.h>
#include <string.h>

static const int32 time_seconds[] =
{
	30, 60, 90, 120, 150, 180, 240, 300, 360, 420,
	480, 540, 600, 900, 1200, 1500, 1800, 2400, 3000,
	3600, 5400, 7200, 9000, 10800, 14400, 18000
};

TimeSlider::TimeSlider(BRect frame, const char *name,
	const char *label, BStringView *target, BMessage *message)
: BSlider(frame, name, label, message, 0, sizeof(time_seconds) / sizeof(time_seconds[0]) - 1, B_BLOCK_THUMB, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_FRAME_EVENTS|B_WILL_DRAW | B_NAVIGABLE),
  drawonupdate1(0), drawonupdate2(0), labeltarget(target)
{
	if(labeltarget)
		labeltarget->SetText(label);
}

void TimeSlider::DrawText()
{
	int32	seconds = Time();
	char	str[64];
	int32	num;

//	strcpy(str, Label());
//	strcat(str, " after");
	*str = 0;

	if(seconds < 60)
		sprintf(str, "%ld seconds", seconds);
	else if(seconds < 3600)
	{
		if((seconds % 60) == 0)
		{
			num = seconds / 60;
			sprintf(str, "%ld minute%s", num, num > 1 ? "s" : "");
		}
		else
			sprintf(str, "%.1f minutes", (float)seconds / 60.);
	}
	else
	{
		seconds /= 60;
		if((seconds % 60) == 0)
		{
			num = seconds / 60;
			sprintf(str, "%ld hour%s", num, num > 1 ? "s" : "");
		}
		else
			sprintf(str, "%.1f hours", (float)seconds / 60.);
	}

#if 0
const int32	kYGap = 4;
const int32 kHashHeight = 6;
const rgb_color kBlack = {0, 0, 0, 255 };

	BView* osView = OffscreenView();

	rgb_color textcolor = (IsEnabled()) ? kBlack : tint_color(kBlack, 0.5);

	osView->SetHighColor(textcolor);
	osView->SetLowColor(ViewColor());

	font_height finfo;
	osView->GetFontHeight(&finfo);
	float textHeight = ceil(finfo.ascent + finfo.descent + finfo.leading);

	float xoffset=0,offsetFromTop=0;
	if (Label()) {
		offsetFromTop = textHeight;
		osView->MovePenTo(2,offsetFromTop);
		osView->DrawString(str);
		offsetFromTop += kYGap;
	}

	offsetFromTop += kHashHeight;
	offsetFromTop += BarFrame().Height();
	offsetFromTop += kHashHeight;

	if (MinLimitLabel() && MaxLimitLabel()) {
		textHeight = ceil(finfo.ascent + finfo.descent);
		offsetFromTop += textHeight;
		osView->MovePenTo(2,offsetFromTop);
		osView->DrawString(MinLimitLabel());		

		xoffset = osView->StringWidth(MaxLimitLabel());
		osView->MovePenTo(Bounds().Width()-xoffset-2,offsetFromTop);
		osView->DrawString(MaxLimitLabel());		
	}
#endif
	if(labeltarget)
		labeltarget->SetText(str);
}

void TimeSlider::DrawThumb()
{
	BSlider::DrawThumb();
	if(drawonupdate1)
		drawonupdate1->Draw(drawonupdate1->Bounds());
	if(drawonupdate2)
		drawonupdate2->Draw(drawonupdate2->Bounds());
}

void TimeSlider::SetDrawOnUpdate(BSlider *d1, BSlider *d2)
{
	drawonupdate1 = d1;
	drawonupdate2 = d2;
}

int32 TimeSlider::Time()
{
	return time_seconds[BSlider::Value()];
}

void TimeSlider::SetTime(int32 t)
{
#define time_seconds_size (sizeof(time_seconds) / sizeof(time_seconds[0]))
	uint32 i;

	// find largest closest value
	for(i = 0; i < time_seconds_size; i++)
	{
		if(t <= time_seconds[i])
		{
			SetValue(i);
			break;
		}
	}
	if(i == time_seconds_size)
		SetValue(time_seconds_size);
}

UpperSlider::UpperSlider(BRect frame, const char *name,
	const char *label, BStringView *target, BMessage *message)
 : TimeSlider(frame, name, label, target, message), limit(0), a(0), b(0)
{
}

void UpperSlider::SetValue(int32 value)
{
	BSlider::SetValue(value);
	if(a && value < a->Value())
		a->SetValue(value);
	if(b && value < b->Value())
		b->SetValue(value);
}

void UpperSlider::SetLower(BSlider *_a, BSlider *_b)
{
	a = _a;
	b = _b;
}

void UpperSlider::DrawBar()
{
	const rgb_color kUnusedColor = {184,184,184,255};
	rgb_color slidercolor = tint_color(kUnusedColor, 0.5);

	TimeSlider::DrawBar();
	if(limit)
	{
		BRect	remotethumb = limit->ThumbFrame();
		BRect	r = BarFrame();
		r.InsetBy(2, 2);
		r.right = (remotethumb.left + remotethumb.right) / 2;
		BView *osView = OffscreenView();
		osView->SetHighColor(slidercolor);
		osView->FillRect(r, B_SOLID_HIGH);
	}
}

void UpperSlider::SetLimit(BSlider *s)
{
	limit = s;
}

RunSlider::RunSlider(BRect frame, const char *name,
	const char *label, BStringView *target, BMessage *message)
 : TimeSlider(frame, name, label, target, message), a(0), b(0)
{
}

void RunSlider::SetValue(int32 value)
{
	BSlider::SetValue(value);
	if(a && value > a->Value())
		a->SetValue(value);
	if(b && value > b->Value())
		b->SetValue(value);
}

void RunSlider::SetUpper(BSlider *_a, BSlider *_b)
{
	a = _a;
	b = _b;
}
