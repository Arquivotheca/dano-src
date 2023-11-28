// ---------------------------------------------------------------------------
/*
	FilterWindow.h
	
	Copyright (c) 2001 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			17 May 2001

*/
// ---------------------------------------------------------------------------

#ifndef FILTERWINDOW_H
#define FILTERWINDOW_H

class MTextAddOn;
class BListView;
class BTextControl;

#include "FilterHandler.h"
#include <Window.h>
#include <List.h>

class FilterWindow : public BWindow {
public:
					FilterWindow(FilterHandler* handler, BList& commands);
	virtual			~FilterWindow();

	virtual void	MessageReceived(BMessage* message);

private:
	const char*		GetSelectionText() const;
	int32			GetSelectedItem(BMessage* message) const;
	bool			IsValidSelection() const;
	void			BuildListView(BListView* list, BList& commands);

private:
	BListView*		fListView;
	BTextControl*	fEditBox;
	BButton*		fRemoveButton;
	BButton*		fOKButton;
	FilterHandler*	fHandler;
	int32			fCurrentSelection;
};

#endif
