// ---------------------------------------------------------------------------
/*
	MErrorMessageWindow.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			10 June 1999

*/
// ---------------------------------------------------------------------------

#ifndef _MERRORMESSAGEWINDOW_H
#define _MERRORMESSAGEWINDOW_H

#include "MMessageWindow.h"

class MMessageInfoView;
class BCheckBox;
class BPictureButton;
class BStringView;

class MProjectWindow;

// ---------------------------------------------------------------------------
// class MErrorMessageWindow
// ---------------------------------------------------------------------------

class MErrorMessageWindow : public MMessageWindow
{
public:
								MErrorMessageWindow(MProjectWindow* project);
	virtual						~MErrorMessageWindow();

	virtual	void				MessageReceived(BMessage * message);
	virtual bool				QuitRequested();

private:
	virtual MMessageInfoView*	CreateInfoView(BRect& bounds, const char* infoTitle);
	virtual void				BuildInfoView();
	virtual MProjectWindow*		GetAssociatedProject();
	void						ClearMessages(BMessage& inMessage);

	void						UpdateCounters();
	void						UpdateMessagesInView();
	void						UpdateCheckBoxes(BMessage& inMessage);

private:
	MProjectWindow*				fProject;
	
	int32						fErrors;
	int32						fWarnings;
	MessagesType				fVisibleTypes;
	
	MMessageInfoView*			fInfoView;
	BCheckBox*					fErrorsCheckBox;
	BCheckBox*					fWarningsCheckBox;
	BPictureButton*				fErrorsPicture;
	BPictureButton*				fWarningsPicture;
	BStringView*				fErrorsCaption;
	BStringView*				fWarningsCaption;
};

#endif
