// ============================================================
//  CColorMenuItem.cpp	©1996 Hiroshi Lockheimer
// ============================================================

#include "CColorMenuItem.h"


CColorMenuItem::CColorMenuItem(
	const char	*label,
	rgb_color	color,
	BMessage 	*message,
	bool		useColor,
	char 		shortcut,
	ulong 		modifiers)
		: BMenuItem(label, message, shortcut, modifiers)
{
	mColor = color;
	mUseColor = useColor;
	
	BMessage *msg = new BMessage(*message);
	msg->AddData("color", B_RGB_COLOR_TYPE, &mColor, sizeof(rgb_color));
	SetMessage(msg);
}


bool
CColorMenuItem::IsColorEqual(
	rgb_color	color)
{
	return ( (color.red == mColor.red) &&
			 (color.green == mColor.green) &&
			 (color.blue == mColor.blue) &&
			 (color.alpha == mColor.alpha) );
}


void
CColorMenuItem::DrawContent()
{
	if (!mUseColor)
		BMenuItem::DrawContent();
	else {
		BMenu *menu = Menu();

		rgb_color saveHColor;
		saveHColor = menu->HighColor();
		rgb_color saveLColor;
		saveLColor = menu->LowColor();

		menu->SetHighColor(mColor);
		menu->SetLowColor(menu->ViewColor());

		BMenuItem::DrawContent();
		
		menu->SetHighColor(saveHColor);
		menu->SetLowColor(saveLColor);
	}
}
