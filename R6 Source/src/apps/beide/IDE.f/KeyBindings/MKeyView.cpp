//========================================================================
//	MKeyView.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MKeyView.h"
#include "MKeyBindingManager.h"
#include "MKeyIcons.h"
#include "IDEMessages.h"
#include "Utils.h"

#include <Message.h>
#include <Window.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//		MKeyView
// ---------------------------------------------------------------------------

MKeyView::MKeyView(
	MKeyBindingManager&	inManager,
	BRect			inFrame,
	const char*		inName,
	uint32			inResizeMask,
	uint32			inFlags)
: BView(inFrame, inName, inResizeMask, inFlags),
	fBindingManager(inManager)
{
	fInfo.prefixIndex = 0;
	fInfo.modifiers = 0;
	fInfo.keyCode = 0;
	fInfo.isVKey = false;
	fInfo.allowAutoRepeat = 0;

	fPrefixIndex = 0;
	fKeyTime = 0;
	fLastModifiers = 0;
	fIsPrefixKey = false;
	fEnabled = true;
	fKeyUp = true;
	fKeyCode = 0;
	
	fModifierState = state_Start;
}

// ---------------------------------------------------------------------------
//		~MKeyView
// ---------------------------------------------------------------------------

MKeyView::~MKeyView()
{
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

const float kSplitterWidth = 75.0;

void
MKeyView::Draw(
	BRect inRect)
{
	// Draw focus box
	DrawFocusBox(IsFocus());
	
	// Draw the two bindings
	BRect		bounds = Bounds();
	
	if (fIsPrefixKey)
	{
		bounds.right = kSplitterWidth;
	
		if (bounds.Intersects(inRect))
		{
			MKeyIcons::DrawKeyBinding(this, bounds, fInfo, B_ALIGN_RIGHT);
		}
	}
	else
	{
		MKeyIcons::DrawKeyBinding(this, bounds, fInfo, fBindingManager, B_ALIGN_LEFT);
	}
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MKeyView::MessageReceived(
	BMessage * inMessage)
{
	switch (inMessage->what)
	{
		case msgSpecialKeydown:
			HandleKeyDown(inMessage);
			break;
		
		default:
			BView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		DrawFocusBox
// ---------------------------------------------------------------------------

void
MKeyView::DrawFocusBox(
	bool	inHilited)
{
	rgb_color color = HighColor();
	rgb_color boxColor;

	if (! fEnabled)
		boxColor = kGrey120;
	else
	if (inHilited)
		boxColor = keyboard_navigation_color();
	else
		boxColor = black;
		
	SetHighColor(boxColor);
	StrokeRect(Bounds());
	SetHighColor(color);
}

// ---------------------------------------------------------------------------
//		MakeFocus
// ---------------------------------------------------------------------------

void
MKeyView::MakeFocus(
	bool 	inBecomeFocus)
{
	if (fEnabled)
	{
		if (IsFocus() != inBecomeFocus)
		{
			DrawFocusBox(inBecomeFocus);

			if (inBecomeFocus)
				SetFlags(Flags() | B_PULSE_NEEDED);		// pulse on
			else
				SetFlags(Flags() & ~B_PULSE_NEEDED);	// pulse off
		}
		
		BView::MakeFocus(inBecomeFocus);
	}
}

// ---------------------------------------------------------------------------
//		MouseDown
// ---------------------------------------------------------------------------

void
MKeyView::MouseDown(
	BPoint /*where*/)
{
	MakeFocus(true);
}

// ---------------------------------------------------------------------------
//		Pulse
// ---------------------------------------------------------------------------
//	Look for modifier keydowns and if prefix keydowns have been timed out.

void
MKeyView::Pulse()
{
	if (IsFocus() && Window()->IsActive())
	{
		// semi-bogus way to check for keyups
		// since we don't get keyup events from the OS
		if (fInfo.keyCode == 0)
			fKeyUp = true;
		else
		{
			key_info states;
			if (B_OK == get_key_info(&states))
			{
			   if ((states.key_states[fKeyCode>>3] & (1 << (7 - (fKeyCode % 8)))) == 0) 
					fKeyUp = true;
			}
		}
	
		// Are any modifier keys being held down?
		uint32		modKeys = modifiers() & kGoodModifiers;

		switch (fModifierState)
		{
			case state_Start:
				if (modKeys != 0)
				{
					fInfo.prefixIndex = 0;
					fInfo.modifiers = 0;
					fInfo.keyCode = 0;
					fInfo.isVKey = false;
					fInfo.allowAutoRepeat = 0;
					fModifierState = state_GetModifiers1;
				}
				break;

			case state_GetModifiers1:
				if (modKeys != fInfo.modifiers)
				{
					fInfo.modifiers = modKeys;
					Invalidate();
				}
				break;

			case state_GetModifiers2:
			{
				bigtime_t		nowtime = system_time();
				
				if (nowtime - fKeyTime > fBindingManager.PrefixTimeout())
				{
					fInfo.prefixIndex = 0;
					fInfo.modifiers = modKeys;
					fInfo.keyCode = 0;
					fInfo.isVKey = false;
					fInfo.allowAutoRepeat = 0;
					fModifierState = state_Start;
					
					fKeyTime = 0;
					Invalidate();
				}
				else
				{
					fInfo.modifiers = modKeys;
					Invalidate();
				}
				break;
			}
			case state_WaitForModifierRelease:
				if (modKeys == 0)
				{
					fModifierState = state_Start;
				}
				break;
		}
	}
}

// ---------------------------------------------------------------------------
//		HandleKeyDown
// ---------------------------------------------------------------------------
//	Handle the special key down message that comes from the binding window.

void
MKeyView::HandleKeyDown(
	BMessage*	inMessage)
{
	KeyBinding		binding;
	
	if (MKeyBindingManager::MessageToBinding(inMessage, binding) && fKeyUp)
	{
		binding.prefixIndex = 0;

		// Calling IsPrefixKey sets the prefix index in the binding
		// if it's a prefix key
		bool		isPrefixKey = fBindingManager.IsPrefixKey(binding);
		
		if (isPrefixKey)
		{
			if (binding.prefixIndex != kQuoteKeyIndex)		// Don't allow entering an existing prefix key
			{
				fInfo.prefixIndex = binding.prefixIndex;
				fInfo.keyCode = 0;
				inMessage->FindInt64("when", &fKeyTime);
				fModifierState = state_GetModifiers2;
			}
			else
			{
				fInfo.prefixIndex = 0;
				fInfo.keyCode = 0;
				fKeyTime = 0;
				fModifierState = state_Start;				
			}
		}
		else
		{
			binding.prefixIndex = fInfo.prefixIndex;
			fInfo = binding;
			fModifierState = state_WaitForModifierRelease;
		}

		Invalidate();
		
		status_t	err = inMessage->FindInt32("key", &fKeyCode);
		ASSERT(err == B_OK);

		fKeyUp = false;
	}
}

// ---------------------------------------------------------------------------
//		SetBinding
// ---------------------------------------------------------------------------

void
MKeyView::SetBinding(
	const KeyBinding&	inBinding,
	bool				inIsPrefixKey)
{
	fInfo = inBinding;
	fIsPrefixKey = inIsPrefixKey;
	if (inIsPrefixKey)
		fPrefixIndex = fInfo.prefixIndex;
	fModifierState = state_Start;
	Invalidate();
}

// ---------------------------------------------------------------------------
//		GetBinding
// ---------------------------------------------------------------------------

void
MKeyView::GetBinding(
	KeyBinding&	outBinding) const
{
	outBinding = fInfo;
	if (fIsPrefixKey)
		outBinding.prefixIndex = fPrefixIndex;
}

// ---------------------------------------------------------------------------
//		ClearBinding
// ---------------------------------------------------------------------------

void
MKeyView::ClearBinding()
{
	fInfo.prefixIndex = 0;
	fInfo.modifiers = 0;
	fInfo.keyCode = 0;
	fInfo.isVKey = false;
	fInfo.allowAutoRepeat = 0;

	Invalidate();
}

// ---------------------------------------------------------------------------
//		Enable
// ---------------------------------------------------------------------------

void
MKeyView::Enable(
	bool	inEnable)
{
	if (fEnabled != inEnable)
	{
		if (! inEnable && IsFocus())
			MakeFocus(false);

		fEnabled = inEnable;

		Invalidate();
	}
}
