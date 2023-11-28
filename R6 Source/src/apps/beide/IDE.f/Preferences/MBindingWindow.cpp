//========================================================================
//	MBindingWindow.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MBindingWindow.h"
#include "MTextView.h"
#include "MDefaultPrefs.h"
#include "MPreferences.h"
#include "MKeyView.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MainMenus.h"
#include "Utils.h"

// ---------------------------------------------------------------------------
//		MBindingWindow
// ---------------------------------------------------------------------------
//	Constructor

MBindingWindow::MBindingWindow(
	BView*				inOwner,
	MKeyBindingManager&	inManager)
	: BWindow(
		BRect(0, 0, 360, 135),
		"Change Binding",
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fCommand(0), fOwner(inOwner), fBindingManager(inManager)
{
	CenterWindow(this);
	BuildWindow();

	SetPulseRate(200000);
}

// ---------------------------------------------------------------------------
//		~MBindingWindow
// ---------------------------------------------------------------------------

MBindingWindow::~MBindingWindow()
{
}

// ---------------------------------------------------------------------------
//		QuitRequested
// ---------------------------------------------------------------------------
//	Determine if it's ok to close.  Tell the KeyBindingView to delete us.

bool
MBindingWindow::QuitRequested()
{
	fOwner->Looper()->PostMessage(msgBindingWindowClosed, fOwner);
	Hide();
	
	return false;
}

// ---------------------------------------------------------------------------
//		DispatchMessage
// ---------------------------------------------------------------------------
//	Change keydowns to special keydowns for the key views.  This prevents
//	any special handling of the keydowns by the window or view (e.g., trapping
//	of command key combos).

void
MBindingWindow::DispatchMessage(
	BMessage 	*message, 
	BHandler 	*receiver)
{
	switch(message->what)
	{
		case B_KEY_DOWN:
			if (receiver == fPrimaryKeyView || receiver == fAlternateKeyView)
			{
				message->what = msgSpecialKeydown;
			}
			break;
	}

	BWindow::DispatchMessage(message, receiver);
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MBindingWindow::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case msgTextChanged:
			fOKButton->SetEnabled(fTextBox->TextLength() > 0);
			break;

		case msgOK:
			ExtractInfo();
			break;

		case msgCancel:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case msgSetBinding:
		{
			KeyBindingInfo*		info;
			const char *		name;
			ssize_t				size;
			KeyBindingContext	context;
			bool				isPrefixKey = false;

			if (B_OK == inMessage->FindData(kBindingInfo, kBindInfoType, (const void**)&info, &size) &&
				B_OK == inMessage->FindString(kName, &name) &&
				B_OK == inMessage->FindInt32(kBindingContext, (int32*)&context))
			{
				inMessage->FindBool(kIsPrefixKey, &isPrefixKey);	// optional
				SetBinding(name, context, *info, isPrefixKey);
			}
			break;
		}
		case msgClear:
			ClearBinding(inMessage);
			break;

		default:
			if (! SpecialMessageReceived(*inMessage, this))
				BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		ExtractInfo
// ---------------------------------------------------------------------------
//	Post the binding info data to the binding view in the prefs window.

void
MBindingWindow::ExtractInfo()
{
	BMessage			msg(msgUpdateBinding);
	KeyBindingInfo		info;

	info.cmdNumber = fCommand;
	fPrimaryKeyView->GetBinding(info.binding1);
	fAlternateKeyView->GetBinding(info.binding2);

#if 1
	bool	allowAutoRepeat = true;
#else
	bool	allowAutoRepeat = fAutoRepeateCB->Value() == B_CONTROL_ON;
#endif

	info.binding1.allowAutoRepeat = info.binding2.allowAutoRepeat = allowAutoRepeat;

	msg.AddData(kBindingInfo, kBindInfoType, &info, sizeof(info));
	msg.AddInt32(kBindingContext, fContext);

	fOwner->Looper()->PostMessage(&msg, fOwner);
}

// ---------------------------------------------------------------------------
//		SetBinding
// ---------------------------------------------------------------------------

void
MBindingWindow::SetBinding(
	const char *			inName,
	KeyBindingContext		inContext,
	const KeyBindingInfo&	inBinding,
	bool					inIsPrefixKey)
{
	fCommand = inBinding.cmdNumber;
	fContext = inContext;
	fKeyName->SetText(inName);
	fPrimaryKeyView->SetBinding(inBinding.binding1, inIsPrefixKey);
	fAlternateKeyView->SetBinding(inBinding.binding2, false);
	fPrimaryKeyView->MakeFocus(true);
	
	const bool		enable = ! inIsPrefixKey;

	fAlternateKeyView->Enable(enable);
	fClearAlternateButton->SetEnabled(enable);
//	fAutoRepeateCB->SetEnabled(enable);
	if (enable)
		fAlternateName->SetHighColor(black);
	else
		fAlternateName->SetHighColor(kGrey120);
	fAlternateName->Invalidate();
	
//	fAutoRepeateCB->SetValue(inBinding.binding1.allowAutoRepeat);
}

// ---------------------------------------------------------------------------
//		ClearBinding
// ---------------------------------------------------------------------------

void
MBindingWindow::ClearBinding(
	BMessage *			inMessage)
{
	BButton*		button;

	if (B_NO_ERROR == inMessage->FindPointer("source", (void **)&button))
	{
		if (button == fClearPrimaryButton)
			fPrimaryKeyView->ClearBinding();
		else
			fAlternateKeyView->ClearBinding();
	}
}

// ---------------------------------------------------------------------------
//		BuildWindow
// ---------------------------------------------------------------------------

void
MBindingWindow::BuildWindow()
{
	BRect			bounds = Bounds();
	BRect			r;
	BMessage*		msg;
	BButton*		button;
	BStringView*	caption;
//	BCheckBox*		checkBox;
	BView*			topView;
	MKeyView*		keyView;
	float			top = 10.0;
	float			left = 20.0;
	float			right;
	const float		kButtonWidth = 80.0;

	// Build a special topview so we can have a grey background for
	// the window
	r = bounds;
	topView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(topView);
	SetGrey(topView, kLtGray);

	// Static text
	r.Set(left, top, 200, top + 16.0);
	caption = new BStringView(r, "st1", "Enter new key equivalents for:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);

	// Name of the key binding
	r.Set(205, top, bounds.right - 20, top + 16.0);
	caption = new BStringView(r, "st2", "keyname"); 
	topView->AddChild(caption);
	SetGrey(caption, kLtGray);
	fKeyName = caption;
	top += 25.0;

	// Primary
	r.Set(left, top, 75, top + 16.0);
	caption = new BStringView(r, "st3", "Primary:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);

	// KeyView
	left = 85.0;
	right = bounds.right - 20 - kButtonWidth - 10;
	r.Set(left, top, right, top + 16.0);
	keyView = new MKeyView(fBindingManager, r, "primary"); 
	topView->AddChild(keyView);
	topView->SetFont(be_bold_font);
	fPrimaryKeyView = keyView;

	left = bounds.right - 20 - kButtonWidth;
	// Clear button
	r.Set(left, top, left + kButtonWidth, top + 20);
	msg = new BMessage(msgClear);
	button = new BButton(r, "clear", "Clear", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);
	fClearPrimaryButton = button;

	top += 25.0;
	left = 20.0;

	// Alternate
	r.Set(left, top, 85, top + 16.0);
	caption = new BStringView(r, "st3", "Alternate:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);
	fAlternateName = caption;

	// KeyView
	left = 85.0;
	right = bounds.right - 20 - kButtonWidth - 10;
	r.Set(left, top, right, top + 16.0);
	keyView = new MKeyView(fBindingManager, r, "primary"); 
	topView->AddChild(keyView);
	topView->SetFont(be_bold_font);
	fAlternateKeyView = keyView;

	left = bounds.right - 20 - kButtonWidth;
	// Clear button
	r.Set(left, top, left + kButtonWidth, top + 20);
	msg = new BMessage(msgClear);
	button = new BButton(r, "clear", "Clear", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);
	fClearAlternateButton = button;

	top += 25.0;
	left = 20.0;

#if 0
	// Auto Repeat checkbox
	r.Set(left, top, 260, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "autorepeat", "Allow Auto Repeat", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this, nil);
	SetGrey(checkBox, kLtGray);
	fAutoRepeateCB = checkBox;

	top += 20.0;
#endif

	left = bounds.right - 20 - kButtonWidth - 10 - kButtonWidth;
	// Cancel button
	r.Set(left, top, left + kButtonWidth, top + 20);
	msg = new BMessage(msgCancel);
	button = new BButton(r, "cancel", "Cancel", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);

	left += kButtonWidth + 10;
	// OK button
	r.Set(left, top, left + kButtonWidth, top + 20);
	msg = new BMessage(msgOK);
	button = new BButton(r, "OK", "Apply", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	SetGrey(button, kLtGray);

	fPrimaryKeyView->MakeFocus();
}
