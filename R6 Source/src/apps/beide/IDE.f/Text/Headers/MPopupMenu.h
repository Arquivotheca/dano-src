//========================================================================
//	MPopupMenu.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MPOPUPMENU_H
#define _MPOPUPMENU_H

#include <PopUpMenu.h>


class MPopupMenu : public BPopUpMenu
{
public:
								MPopupMenu(
									const char *	inTitle,
									bool			radioMode = TRUE,
									bool 			autoRename = TRUE);
								~MPopupMenu();

	virtual	BPoint				ScreenLocation();

	void						SetBottomLeft(
									BPoint	inPoint);
protected:

	BPoint				fBottomLeft;
};

#endif
