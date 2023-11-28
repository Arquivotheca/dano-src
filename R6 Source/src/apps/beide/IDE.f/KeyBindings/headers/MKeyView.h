//========================================================================
//	MKeyView.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MKEYVIEW_H
#define _MKEYVIEW_H

#include "MKeyBindingManager.h"

#include <View.h>

class MKeyBindingManager;

class MKeyView : public BView {
public:
								MKeyView(
									MKeyBindingManager&	inManager,
									BRect			inFrame,
									const char*		inName,
									uint32			inResizeMask = B_FOLLOW_LEFT+ B_FOLLOW_TOP,
									uint32			inFlags = B_WILL_DRAW|B_PULSE_NEEDED|B_NAVIGABLE);
								~MKeyView();

virtual	void					Draw(
									BRect inRect);
virtual	void					Pulse();
virtual	void					MouseDown(
									BPoint where);
virtual void					MessageReceived(
									BMessage * message);
virtual	void					MakeFocus(
									bool focusState = true);

	void						SetBinding(
									const KeyBinding&	inBinding,
									bool				inIsPrefixKey);
	void						GetBinding(
									KeyBinding&	outBinding) const;
	void						ClearBinding();
	void						Enable(
									bool	inEnable);
	bool						Enabled();


private:

enum ModifierState{
	state_Start,
	state_GetModifiers1,
	state_GetModifiers2,
	state_WaitForModifierRelease
};


	MKeyBindingManager&			fBindingManager;
	KeyBinding					fInfo;
	int32						fPrefixIndex;
	uint32						fLastModifiers;
	int32						fKeyCode;
	ModifierState				fModifierState;
	bigtime_t					fKeyTime;
	bool						fIsPrefixKey;
	bool						fEnabled;
	bool						fKeyUp;

	void						DrawFocusBox(
									bool	inHilited);
	void						HandleKeyDown(
									BMessage*	inMessage);
};

#endif
