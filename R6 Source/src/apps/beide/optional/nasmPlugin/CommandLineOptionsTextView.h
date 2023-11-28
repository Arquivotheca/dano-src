// ---------------------------------------------------------------------------
/*
	CommandLineOptionsTextView.h
	
	Provides an implementation of Metrowerks plug-in preference view that
	just implements a text box for typing.
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#ifndef _COMMANDLINETEXTVIEW_H
#define _COMMANDLINETEXTVIEW_H

#include "CommandLineText.h"
#include "GCCOptionsView.h"
#include "MPrefsStruct.h"

class MTextView;
class BStringView;

class CommandLineOptionsTextView : public GCCOptionsView<CommandLineText>
{
public:	
						CommandLineOptionsTextView(BRect inFrame, 
											const char* title,
											const char* caption,
											const char* messageName,
											type_code messageType,
											UpdateType updateType,
											const char* defaultText);
	virtual				~CommandLineOptionsTextView();


	virtual void		DoFactorySettings();
	
	virtual void		AttachedToWindow();
	virtual void		UpdateValues();

protected:
	virtual void		GetUserTextChanges();
	virtual void		GetUserCheckBoxesChanges();
	virtual void		GetUserPopupChanges();

private:
	char*				fCaption;
	char*				fDefaultText;
	MTextView*			fTextBox;
};

#endif
