//========================================================================
//	MBindingWindow.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS

#ifndef _MBINDINGWINDOW_H
#define _MBINDINGWINDOW_H

#include <Window.h>
#include "MKeyBindingManager.h"

class MTextView;
class MKeyView;
//class MKeyBindingManager;
struct KeyBindingInfo;
class BStringView;
class BTextView;

class MBindingWindow : public BWindow
{
public:

								MBindingWindow(
									BView*				inOwner,
									MKeyBindingManager&	inManager);
								~MBindingWindow();

virtual	void					MessageReceived(
									BMessage * message);
virtual	bool					QuitRequested();
virtual	void					DispatchMessage(
									BMessage *message, 
									BHandler *receiver);

private:

		CommandT				fCommand;
		KeyBindingContext		fContext;
		BView*					fOwner;
		MKeyBindingManager&		fBindingManager;
		MTextView*				fTextBox;
//		BCheckBox*				fAutoRepeateCB;
		BButton*				fOKButton;
		BStringView*			fKeyName;
		BStringView*			fAlternateName;
		BTextView*				fPrimaryKeyBox;
		BTextView*				fAlternateKeyBox;
		BButton*				fClearPrimaryButton;
		BButton*				fClearAlternateButton;
		MKeyView*				fPrimaryKeyView;
		MKeyView*				fAlternateKeyView;
		
		void					BuildWindow();
		void					ExtractInfo();
		void					GetPrefs();
		void					SetPrefs();
		void					SetBinding(
									const char *			inName,
									KeyBindingContext		inContext,
									const KeyBindingInfo&	inBinding,
									bool					inIsPrefixKey);
		void					ClearBinding(
									BMessage *			inMessage);
};

#endif
