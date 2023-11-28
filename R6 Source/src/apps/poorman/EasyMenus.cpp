//
// EasyMenus
//
// Quick-and-easy menu building.  You can create structures that define
// your menus, and create them all with a single call to EZCreateMenuBar.
//

#include <sys/types.h>
#include <Be.h>
#include "EasyMenus.h"


// The following Menu Item template can be used by clients
// to create a separator.

MenuItemTemplate ezMenuSeparator = {
	NULL, 0, 0, 0, NULL
};


//
// EZCreateMenuBar
//
// Create a new menu bar, including all its menus and items,
// given a template.
//
BMenuBar *EZCreateMenuBar(MenuBarTemplate *barTemplate, BRect frame) {
	BMenuBar	*bar;
	BMenu		*menu;
	BMenuItem	*item;
	MenuTemplate **menuTemplate;
	
	bar = new BMenuBar(frame, barTemplate->name,
						barTemplate->resizingMode, barTemplate->layout,
						barTemplate->resizeToFit);
	if (!bar) {
		return NULL;
	}
	
	menuTemplate = barTemplate->menuList;
	if (!menuTemplate) {
		goto scram;
	}
	
	while (*menuTemplate) {
		int itemCount;
		MenuItemTemplate **itemTemplate;
		unsigned char *evilPtr;
		
		menu = new BMenu((*menuTemplate)->label, (*menuTemplate)->layout);
		if (!menu) {
			delete bar;
			return NULL;
		}
		
		if ((*menuTemplate)->itemList) {
			itemTemplate = (*menuTemplate)->itemList;
		}
		else {
			goto scram;
		}
		
		while (*itemTemplate) {
			if ((*itemTemplate)->label) {
				item = new BMenuItem((*itemTemplate)->label,
										new BMessage((*itemTemplate)->messageType),
										(*itemTemplate)->shortcut,
										(*itemTemplate)->modifiers);
				if (!item) {
					delete bar;
					return NULL;
				}
				if ((*itemTemplate)->messageType == B_ABOUT_REQUESTED) {
					item->SetTarget(be_app);		// Let about go to the application
				}
				menu->AddItem(item);
				(*itemTemplate)->itemPtr = item;
			}
			else {
				menu->AddSeparatorItem();
				(*itemTemplate)->itemPtr = NULL;
			}
			itemTemplate++;
		}
scram:
		bar->AddItem(menu);
		menuTemplate++;
	}
	return bar;
}
