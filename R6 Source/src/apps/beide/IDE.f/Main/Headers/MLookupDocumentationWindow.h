// ---------------------------------------------------------------------------
/*
	MLookupDocumentationWindow.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			14 January 1999

*/
// ---------------------------------------------------------------------------

#ifndef _MLOOKUPDOCUMENTATIONWINDOW_H
#define _MLOOKUPDOCUMENTATIONWINDOW_H

#include <Window.h>

class MTextView;

class MLookupDocumentationWindow : public BWindow
{
public:
					MLookupDocumentationWindow();
					~MLookupDocumentationWindow();

	virtual	void	MessageReceived(BMessage* message);
	virtual	bool	QuitRequested();

private:
	void			BuildWindow();
	void			ExtractInfo();

private:
	MTextView*		fTextBox;
	BButton*		fOKButton;
};

#endif
