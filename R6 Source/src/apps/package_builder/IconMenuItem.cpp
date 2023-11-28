#include <Be.h>
// IconMenuItem.cpp
#include "IconMenuItem.h"
#include "MyDebug.h"

IconMenuItem::IconMenuItem(const char *label,
						BBitmap *icon,
						BMessage *message)
		: BMenuItem(label,message)
{
	iconBmap = icon;
	space = FALSE;
}

IconMenuItem::IconMenuItem(BBitmap *icon,
						BMenu *submenu)
		: BMenuItem(submenu)
{
	iconBmap = icon;
	space = TRUE;
}


void IconMenuItem::SetIcon(BBitmap *icon)
{
	iconBmap = icon;
}

BBitmap *IconMenuItem::GetIcon()
{
	return iconBmap;
}

void IconMenuItem::DrawContent()
{
	BPoint pt = ContentLocation();

	Menu()->MovePenTo(pt.x + 20 + (space ? 6 : 0),pt.y);
	BMenuItem::DrawContent();
	
	if (iconBmap != NULL) {
		Menu()->MovePenTo(pt.x, pt.y + (Frame().Height() - 16)/2.0 - 2);
		Menu()->SetDrawingMode(B_OP_OVER);
		Menu()->DrawBitmap(iconBmap);
		Menu()->SetDrawingMode(B_OP_COPY);
	}
}

void IconMenuItem::Highlight(bool high)
{
	BPoint pt = ContentLocation();
	
	if (iconBmap != NULL) {
		Menu()->MovePenTo(pt.x, pt.y + (Frame().Height() - 16)/2.0 - 2);
		Menu()->SetDrawingMode(B_OP_OVER);
		Menu()->DrawBitmap(iconBmap);
		Menu()->SetDrawingMode(B_OP_COPY);
	}
	BMenuItem::Highlight(high);
}

void IconMenuItem::GetContentSize(float *width, float *height)
{
	BMenuItem::GetContentSize(width,height);
	if (*height < 16)
		*height = 16;
	*width = Menu()->StringWidth(Label()) + 20 + (space ? 18 : 0);
}
