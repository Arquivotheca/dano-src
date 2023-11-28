// SaverModule.cp	©1996 Jon Watte
// Ported to the BScreenSaver API Jan 1999 by Duncan Wilcox

#include "StringTheme.h"
#include <StringView.h>
#include <stdlib.h>

//	utility
static float fl_rand(float base, float top)
{
	float r = (rand()&0x7fff)/32767.0*(top-base)+base;
	return r;
}

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new StringTheme(message, image);
}

StringTheme::StringTheme(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
}

void StringTheme::StartConfig(BView *view)
{
	BRect area(20,20, 320, 35);
	BStringView *text = new BStringView(area, "whom", 
		"StringTheme, by Jon Watte");
	view->AddChild(text);
	text->SetViewColor(view->ViewColor());
}

status_t StringTheme::StartSaver(BView *, bool /* preview */)
{
	srand((int)fmod(system_time()/1000.0, 32768.0));
	SetTickSize(3000000);
	return B_OK;
}

void StringTheme::Draw(BView *view, int32 frame)
{
	if(frame == 0)
		view->FillRect(view->Bounds(), B_SOLID_LOW);

#define PI180 M_PI
#define PI360 (M_PI*2.0)
#define PI120 (M_PI*2.0/3.0)

	BPoint posA, posB;
	float dirA, spdA, spinA;
	float dirB, spdB, spinB;
	float color;
	BRect bounds = view->Bounds();
	int NLINE = (int)fl_rand(50, 150);

	if (fl_rand(0, NLINE) > 36) {
		view->FillRect(bounds, B_SOLID_LOW);
	}
	view->BeginLineArray(NLINE);

	posA.x = fl_rand(0, bounds.Width()+1);
	posA.y = fl_rand(0, bounds.Height()+1);
	posB.x = fl_rand(0, bounds.Width()+1);
	posB.y = fl_rand(0, bounds.Height()+1);
	dirA = fl_rand(0, PI360);
	spdA = fl_rand(2, 6);
	spinA = fl_rand(-0.45, 0.45);
	spinA = spinA*fabs(spinA)*fabs(spinA);
	dirB = fl_rand(0, PI360);
	spdB = fl_rand(2, 6);
	spinB = fl_rand(-0.45, 0.45);
	spinB = spinB*fabs(spinB)*fabs(spinB);
	color = fl_rand(0, PI360);

	for (int ix=0; ix<NLINE; ix++) {
//	add line
		rgb_color rgb = {0, 0, 0, 0};
		rgb.red = (uint8)(sin(color)*127+128);
		rgb.green = (uint8)(sin(color+PI120)*127+128);
		rgb.blue = (uint8)(sin(color-PI120)*127+128);
		view->AddLine(posA, posB, rgb);
//	step model
//	A
		float stepAX = sin(dirA)*spdA;
		float stepAY = -cos(dirA)*spdA;
		posA.x += stepAX;
		posA.y += stepAY;
		if ((stepAX < 0) && (posA.x < 10)) {
			dirA = PI360-dirA;
			spinA = -spinA;
		}
		if ((stepAX > 0) && (posA.x > bounds.Width()-9)) {
			dirA = PI360-dirA;
			spinA = -spinA;
		}
		if ((stepAY < 0) && (posA.y < 10)) {
			dirA = PI180-dirA;
			if (dirA < 0)
				dirA += PI360;
			spinA = -spinA;
		}
		if ((stepAY > 0) && (posA.y > bounds.Height()-9)) {
			dirA = PI180-dirA;
			if (dirA < 0)
				dirA += PI360;
			spinA = -spinA;
		}
		dirA += spinA;
		if (dirA < 0)
			dirA += PI360;
		if (dirA > PI360)
			dirA -= PI360;
//	B
		float stepBX = sin(dirB)*spdB;
		float stepBY = -cos(dirB)*spdB;
		posB.x += stepBX;
		posB.y += stepBY;
		if ((stepBX < 0) && (posB.x < 10)) {
			dirB = PI360-dirB;
			spinB = -spinB;
		}
		if ((stepBX > 0) && (posB.x > bounds.Width()-9)) {
			dirB = PI360-dirB;
			spinB = -spinB;
		}
		if ((stepBY < 0) && (posB.y < 10)) {
			dirB = PI180-dirB;
			if (dirB < 0)
				dirB += PI360;
			spinB = -spinB;
		}
		if ((stepBY > 0) && (posB.y > bounds.Height()-9)) {
			dirB = PI180-dirB;
			if (dirB < 0)
				dirB += PI360;
			spinB = -spinB;
		}
		dirB += spinB;
		if (dirB < 0)
			dirB += PI360;
		if (dirB > PI360)
			dirB -= PI360;
//	color
		color += 0.05;
		if (color > PI360)
			color -= PI360;
	}
	view->EndLineArray();
}
