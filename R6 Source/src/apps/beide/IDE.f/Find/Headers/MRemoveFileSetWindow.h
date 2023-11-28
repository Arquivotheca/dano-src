// ---------------------------------------------------------------------------
/*
	MRemoveFileSetWindow.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			23 April 1999

*/
// ---------------------------------------------------------------------------

#ifndef _MREMOVEFILESETWINDOW_H
#define _MREMOVEFILESETWINDOW_H

#include <Window.h>

class BMessenger;
class MFileSetKeeper;
class BListView;
class BButton;

// ---------------------------------------------------------------------------
// class MRemoveFileSetWindow
// ---------------------------------------------------------------------------

class MRemoveFileSetWindow : public BWindow
{
public:

						MRemoveFileSetWindow(BMessenger* adoptTarget, 
											 MFileSetKeeper& fileSetKeeper);
						~MRemoveFileSetWindow();

	void				MessageReceived(BMessage * message);

private:
	void				BuildWindow();
	void				BuildListView();
	void				DoRemoveSet(BMessage& inMessage);

	BMessenger*			fMessenger;
	MFileSetKeeper&		fFileSetKeeper;
	BListView*			fListView;
	BButton*			fOKButton;
};

#endif
