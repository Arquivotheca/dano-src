

/*** InstallUtil.cpp ***/

#include "Util.h"
#include <View.h>

void DrawHSeparator(float left, float right, float y, BView *v)
{
	rgb_color saveColor = v->HighColor();
	
	v->SetHighColor(128,128,128);	
	v->StrokeLine(BPoint(left,y),BPoint(right,y));
	v->SetHighColor(255,255,255);
	y++;
	v->StrokeLine(BPoint(left,y),BPoint(right,y));
								
	v->SetHighColor(saveColor);
}


const rgb_color light_gray_background = {230,230,230,0};
const rgb_color label_red = {200,0,0};
