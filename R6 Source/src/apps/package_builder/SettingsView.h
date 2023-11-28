// SettingsView.h

#include "SmallPopup.h"

#ifndef _SETTINGSVIEW_H
#define _SETTINGSVIEW_H

class SettingsView : public BBox
{
public:

	SettingsView(BRect r,
			const char *name = NULL,
			ulong resizeFlags = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			ulong flags = B_WILL_DRAW);
	~SettingsView();

	virtual void	Draw(BRect up);
	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *msg);
	void			SetupAttributes();
	void			ItemsSelected(BMessage *msg);
	void			ConditionSet(bool);
	void			EnableDisableReplacementOptions(uint16 version);

	BMenuField		*groupsMenu;
	SmallPopUpMenu	*groupsPopup;
	SmallPopUpMenu	*destPopup;
	BMenuField		*destMenu;
	SmallPopUpMenu	*replPopup;
	BMenuField		*replMenu;
	//BCheckBox		*installCb;
	//BMenuField		*ifMenu;
	//BMenuField		*condMenu;
	BMenuField		*archMenu;
	BPopUpMenu		*archPopup;
	
	ulong		currentBitmap;
	
	int32		custStartIndex;	// first index of cust items
	int32		destCount;		// no. custom dests
	int32		lastMarked;
	int32		mergeOptionIndex; // index of the "merge folders" menu option
};

#endif
