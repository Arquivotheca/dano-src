//========================================================================
//	MKeyFilter.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "IDEConstants.h"
#include "MKeyFilter.h"
#include "MKeyBindingManager.h"
#include "ProjectCommands.h"
#include <Debug.h>
#include <InterfaceKit.h>

// ---------------------------------------------------------------------------
//		MKeyFilter
// ---------------------------------------------------------------------------
//	Constructor

MKeyFilter::MKeyFilter(
	BWindow* 	inWindow,
	KeyBindingContext	inContext)
: BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN),
fWindow(inWindow), fBar(inWindow->KeyMenuBar()), fContext(inContext), 
fPrefixIndex(kInvalidPrefixIndex)
{
}

// ---------------------------------------------------------------------------
//		Filter
// ---------------------------------------------------------------------------

filter_result		
MKeyFilter::Filter(
	BMessage *inMessage, 
	BHandler **/*target*/)
{
	ASSERT(inMessage->what == B_KEY_DOWN);

	KeyBinding		binding;
	filter_result	result = B_DISPATCH_MESSAGE;

	if (MessageToBinding(inMessage, binding))
	{
		if (MKeyBindingManager::Manager().IsPrefixKey(binding))
		{
			fIsPrefix = true;
			fPrefixIndex = binding.prefixIndex;
			inMessage->FindInt64("when", &fPrefixTime);
			result = B_SKIP_MESSAGE;		// do nothing further with a prefix keydown
		}
		else
		{
			// Change the message and pass it on
			uint32			what = MKeyBindingManager::Manager().GetCommand(fContext, binding);
			if (what != 0)
			{
				if (CommandIsValid(what))		// is command enabled in the menu bar?
					inMessage->what = what;
				else
					result = B_SKIP_MESSAGE;
			}
		}
	}

	return result;
}

// ---------------------------------------------------------------------------
//		MessageToBinding
// ---------------------------------------------------------------------------
//	Translate the event to a keybinding struct.  return true if the message
//	might be a valid keybinding.

bool
MKeyFilter::MessageToBinding(
	BMessage*		inMessage, 
	KeyBinding&		outBinding)
{
	bool		validBinding = false;
	
	// reset prefix if it has timed out
	if (fPrefixIndex > 0)
	{
		bigtime_t		prefixTime;

		inMessage->FindInt64("when", &prefixTime);

		// inlining this function causes an internal compiler error
		if (prefixTime - fPrefixTime > MKeyBindingManager::Manager().PrefixTimeout())
		{
			fPrefixIndex = kInvalidPrefixIndex;
			fIsPrefix = false;
		}
	}

	// If the last keydown was the quote key then pass on this keydown unchanged
	if (fPrefixIndex == kQuoteKeyIndex)
	{
		fPrefixIndex = kInvalidPrefixIndex;
	}
	else
	{
		validBinding = MKeyBindingManager::MessageToBinding(inMessage, outBinding);
		if (validBinding)
			outBinding.prefixIndex = fPrefixIndex;
	}

	return validBinding;
}

// ---------------------------------------------------------------------------
//		CommandIsValid
// ---------------------------------------------------------------------------
//	We assume that a command is valid unless it is already in the menu bar
//	and is disabled.  Invalid commands are skipped.
//	This function should be made virtual if there is a need to subclass.

bool
MKeyFilter::CommandIsValid(
	CommandT	inCommand)
{
	bool	valid = true;

	// Check if the item is enabled
	BMenuBar*		bar = fBar;
	BMenuItem*		item;

	if (bar != nil)// if no menu bar assume enabled
	{
		fWindow->MenusBeginning();	// let the window update the menu items
		item = bar->FindItem(inCommand);
		if (item != nil)
		{
			if (! item->IsEnabled())
				valid = false;
		}
		
		fWindow->MenusEnded();
	}

	return valid;
}

// ---------------------------------------------------------------------------
//		MTextKeyFilter
// ---------------------------------------------------------------------------
//	Constructor
//	This filter should be used in app windows that have BTextViews in them.
//	It will pass on cmd-C, cmd-X, cmd-V, which are hard-coded in that
//	view, rather than translating these key combos to our internal commands.

MTextKeyFilter::MTextKeyFilter(
	BWindow* 	inWindow,
	KeyBindingContext	inContext)
: MKeyFilter(inWindow, inContext)
{
}

// ---------------------------------------------------------------------------
//		Filter
// ---------------------------------------------------------------------------

filter_result		
MTextKeyFilter::Filter(
	BMessage *inMessage, 
	BHandler **target)
{
	bool			handled = false;
	uint32			modifiers;
	int32			raw_char;

	inMessage->FindInt32("modifiers", (int32*) &modifiers);
	inMessage->FindInt32("raw_char", &raw_char);

	if ((modifiers & B_COMMAND_KEY) != 0)
	{
		switch (raw_char)
		{
			case 'c':
			case 'v':
			case 'x':
				handled = true;
				break;
		}
	}
	
	if (handled)
		return B_DISPATCH_MESSAGE;
	else
		return MKeyFilter::Filter(inMessage, target);
}

// ---------------------------------------------------------------------------
//		MTextKeyFilter
// ---------------------------------------------------------------------------
//	Constructor
//	This filter should be used in app windows that have BTextViews in them.
//	It will pass on cmd-C, cmd-X, cmd-V, which are hard-coded in that
//	view, rather than translating these key combos to our internal commands.

MProjectKeyFilter::MProjectKeyFilter(
	BWindow* 	inWindow,
	KeyBindingContext	inContext)
: MKeyFilter(inWindow, inContext)
{
}

// ---------------------------------------------------------------------------
//		Filter
// ---------------------------------------------------------------------------

filter_result		
MProjectKeyFilter::Filter(
	BMessage *inMessage, 
	BHandler **target)
{
	bool			handled = false;
	uint32			modifiers;
	int32			raw_char;

	inMessage->FindInt32("modifiers", (int32*) &modifiers);
	inMessage->FindInt32("raw_char", &raw_char);

	if ((modifiers & B_COMMAND_KEY) != 0)
	{
		switch (raw_char)
		{
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW:
			// the project window only has bindings for left/right
			// arrow -- allow up/down to be interpreted elsewhere
			// case B_UP_ARROW:
			// case B_DOWN_ARROW:
				handled = true;
				break;
		}
	}
	
	if (handled)
		return B_DISPATCH_MESSAGE;
	else
		return MKeyFilter::Filter(inMessage, target);
}

