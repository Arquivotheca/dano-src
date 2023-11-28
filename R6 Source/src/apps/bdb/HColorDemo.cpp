/*
	HColorDemo.cpp
	
	Copyright Hekkelman Programmatuur
	
	Created: 10/10/97 13:42:21
*/

#include "HColorDemo.h"
#include "lib.h"

HColorDemo::HColorDemo(BRect frame, const char *name)
	: BView(frame, name, 0, B_WILL_DRAW)
{
} /* HColorDemo::HColorDemo */

void HColorDemo::Draw(BRect /*update*/)
{
	BRect r(Bounds());
	
	BeginLineArray(8);
	AddLine(r.LeftTop(), r.RightTop(), kShadow);
	AddLine(r.LeftTop(), r.LeftBottom(), kShadow);
	AddLine(r.LeftBottom(), r.RightBottom(), kWhite);
	AddLine(r.RightBottom(), r.RightTop(), kWhite);
	r.InsetBy(1, 1);
	AddLine(r.LeftTop(), r.RightTop(), kDarkShadow);
	AddLine(r.LeftTop(), r.LeftBottom(), kDarkShadow);
	AddLine(r.LeftBottom(), r.RightBottom(), kDarkShadow);
	AddLine(r.RightBottom(), r.RightTop(), kDarkShadow);
	EndLineArray();
	
	r.InsetBy(1, 1);
	BRect r2(r);

	r.right = r.left + r.Width() / 2;
	SetHighColor(fOldColor);
	FillRect(r);
	
	r = r2;
	r.left = r.right - r.Width() / 2;
	SetHighColor(fNewColor);
	FillRect(r);
} /* HColorDemo::Draw */
		
void HColorDemo::SetOldColor(rgb_color c)
{
	fOldColor = c;
	
	BRect r(Bounds());
	r.InsetBy(2, 2);
	r.right = r.left + r.Width() / 2;
	SetHighColor(fOldColor);
	FillRect(r);
} /* HColorDemo::SetOldColor */

void HColorDemo::SetNewColor(rgb_color c)
{
	fNewColor = c;
	
	BRect r(Bounds());
	r.InsetBy(2, 2);
	r.left = r.right - r.Width() / 2;
	SetHighColor(fNewColor);
	FillRect(r);
} /* HColorDemo::SetNewColor */
