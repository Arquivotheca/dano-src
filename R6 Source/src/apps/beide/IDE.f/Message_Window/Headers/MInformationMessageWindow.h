// ---------------------------------------------------------------------------
/*
	MInformationMessageWindow.h
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			10 June 1999

*/
// ---------------------------------------------------------------------------

#ifndef _MINFORMATIONMESSAGEWINDOW_H
#define _MINFORMATIONMESSAGEWINDOW_H

#include "MMessageWindow.h"

class MMessageInfoView;
class BPictureButton;
class BStringView;


// ---------------------------------------------------------------------------
// class MInformationMessageWindow
// ---------------------------------------------------------------------------

class MInformationMessageWindow : public MMessageWindow
{
public:
								MInformationMessageWindow(const char* title, 
														  const char* infoTitle);
	virtual						~MInformationMessageWindow();

	virtual	void				MessageReceived(BMessage * message);

private:
	virtual MMessageInfoView*	CreateInfoView(BRect& bounds, const char* infoTitle);
	virtual void				BuildInfoView();
	virtual MProjectWindow*		GetAssociatedProject();
	void						ClearMessages(BMessage& inMessage);

	void						UpdateCounters();

private:
	int32						fInfoCount;
	
	MMessageInfoView*			fInfoView;
	BPictureButton*				fInformationPicture;
	BStringView*				fInformationCaption;
};

#endif
