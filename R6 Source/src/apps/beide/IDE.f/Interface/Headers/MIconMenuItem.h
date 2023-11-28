//========================================================================
//	MIconMenuItem.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MICONMENUITEM_H
#define _MICONMENUITEM_H

#include <MenuItem.h>

class MIconMenuItem : public BMenuItem
{
public:
								MIconMenuItem(
									BBitmap*		inIcon,
									const char *	inLabel,
									BMessage *		inMessage,
									char 			inShortcut = 0,
									uint32 			inModifiers = 0);
								MIconMenuItem(
									const entry_ref&	inRef,
									const char *		inLabel,
									BMessage *			inMessage,
									char 				inShortcut = 0,
									uint32 				inModifiers = 0);
								MIconMenuItem(
									const entry_ref&	inRef,
									dev_t				inDevice,
									BMessage *			inMessage,
									char 				inShortcut = 0,
									uint32 				inModifiers = 0);
								MIconMenuItem(
									const entry_ref&	inRef,
									const char *		inLabel,
									BMenu*				inMenu,
									BMessage *			inMessage);
								~MIconMenuItem();

protected:
		void					GetContentSize(
									float *width, 
									float *height);
		void					DrawContent();
		void					GetIcon(
									const entry_ref&		inRef);
private:
	BBitmap*		fIcon;
};

#endif
