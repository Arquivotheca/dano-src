#include <Alert.h>
#include <Screen.h>
#include "utils.h"

void
What(const char* what)
{
	BAlert* a = new BAlert("", what, "Cancel", NULL, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT );
	a->Go();
}

void
CenterWindowOnScreen(BWindow* w)
{
	BRect	screenFrame = (BScreen(B_MAIN_SCREEN_ID).Frame());
	BPoint 	pt;
	pt.x = screenFrame.Width()/2 - w->Bounds().Width()/2;
	pt.y = screenFrame.Height()/2 - w->Bounds().Height()/2;

	if (screenFrame.Contains(pt))
		w->MoveTo(pt);
}

float
FontHeight(const BFont* font, bool full)
{
	font_height finfo;		
	font->GetHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

