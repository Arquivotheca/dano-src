// IconMenuItem.cpp
#include "IconMenuItem.h"
#include "MyDebug.h"

#include <Looper.h>

IconMenuItem::IconMenuItem(const char *label,
						BBitmap *icon,
						BMessage *message,
						bool _freeIcon)
		:	BMenuItem(label,message),
			freeIcon(_freeIcon)
{
	iconBmap = icon;
	space = FALSE;
}

IconMenuItem::IconMenuItem(BBitmap *icon,
						BMenu *submenu)
		:	BMenuItem(submenu),
			freeIcon(FALSE)
{
	iconBmap = icon;
	space = TRUE;
}

IconMenuItem::~IconMenuItem()
{
	if (freeIcon && iconBmap)
		delete iconBmap;
}

void IconMenuItem::SetIcon(BBitmap *icon)
{
	if (!freeIcon)
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
	
	if (iconBmap) {
		Menu()->MovePenTo(pt.x, Frame().bottom - 17);
		Menu()->SetDrawingMode(B_OP_OVER);
		Menu()->DrawBitmap(iconBmap);
		Menu()->SetDrawingMode(B_OP_COPY);
	}
}

void IconMenuItem::Highlight(bool high)
{
	BPoint pt = ContentLocation();
	
	BMenuItem::Highlight(high);
	if (iconBmap) {
		Menu()->MovePenTo(pt.x, Frame().bottom - 17);
		Menu()->SetDrawingMode(B_OP_OVER);
		Menu()->DrawBitmap(iconBmap);
		Menu()->SetDrawingMode(B_OP_COPY);
	}
}

void IconMenuItem::GetContentSize(float *width, float *height)
{
	BMenuItem::GetContentSize(width,height);
	if (*height < 16)
		*height = 16;
	*width = Menu()->StringWidth(Label()) + 20 + (space ? 18 : 0);
}

void IconMenuItem::MyInvoke()
{
	BMessage m(*Message());
	m.AddInt32("index",Menu()->IndexOf(this));
	Target()->Looper()->PostMessage(&m, Target());
}
