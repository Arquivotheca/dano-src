// SmallPopup.h

#ifndef _SMALLPOPUP_H
#define _SMALLPOPUP_H

class SmallPopUpMenu : public BPopUpMenu
{
public :
	SmallPopUpMenu(	const char *title,
		bool radioMode = FALSE,
		bool autoRename = FALSE,
		menu_layout layout = B_ITEMS_IN_COLUMN);
		
	~SmallPopUpMenu();
	
virtual void	AttachedToWindow();
};

#endif
