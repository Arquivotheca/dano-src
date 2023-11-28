#include <Be.h>
#include "SmallPopup.h"

#include "MyDebug.h"

SmallPopUpMenu::SmallPopUpMenu(const char *title,
		bool radioMode, bool autoRename, menu_layout layout)
	: BPopUpMenu(title,radioMode,autoRename,layout)
{
}

SmallPopUpMenu::~SmallPopUpMenu()
{
}

void SmallPopUpMenu::AttachedToWindow()
{
	BPopUpMenu::AttachedToWindow();
	SetFont(be_plain_font);
}
