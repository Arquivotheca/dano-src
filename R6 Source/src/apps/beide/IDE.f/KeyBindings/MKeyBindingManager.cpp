//========================================================================
//	MKeyBindingManager.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "IDEConstants.h"
#include "MKeyBindingManager.h"
#include "ProjectCommands.h"

#include <ByteOrder.h>
#include <DataIO.h>
#include <Message.h>
#include <Debug.h>
#include <ctype.h>

const int16 CMD = B_COMMAND_KEY;
const int16 SHIFT = B_SHIFT_KEY;
const int16 OPT = B_OPTION_KEY;
const int16 CNTRL = B_CONTROL_KEY;
const int16 LEFT_ARROW = B_LEFT_ARROW;
const int16 RIGHT_ARROW = B_RIGHT_ARROW;
const int16 UP_ARROW = B_UP_ARROW;
const int16 DOWN_ARROW = B_DOWN_ARROW;
const int16 BACK_DELETE = B_BACKSPACE;
const int16 FWD_DELETE = B_DELETE;
const int16 PAGEUP = B_PAGE_UP;
const int16 PAGEDOWN = B_PAGE_DOWN;
const int16 HOME = B_HOME;
const int16 END = B_END;

// structs for the default key combinations
// those commands with no default key binding are commented out
static KeyBindingInfo DefaultGlobalBindings[] =
{
	//File menu
	{ cmd_New, { 0, CMD, 'N', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_NewProject, { 0, CMD+SHIFT, 'N', 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_OpenFile, { 0, CMD, 'O', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_OpenSelection, { 0, CMD, 'D', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_Close, { 0, CMD, 'W', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_CloseAll, { 0, CMD+OPT, 'W', 0, 1 },  { 0, CMD+CNTRL, 'W', 0, 1 } },
	{ cmd_Save, { 0, CMD, 'S', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SaveAll, { 0, CMD+OPT, 'S', 0, 1 }, { 0, CMD+CNTRL, 'S', 0, 1 } },
//	{ cmd_SaveAs, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_SaveCopyAs, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_Revert, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_Print, { 0, CMD, 'P', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_PageSetup, { 0, CMD+SHIFT, 'P', 0, 1 }, { 0, 0, 0, 0, 0, } },
//	{ cmd_About, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_Quit, { 0, CMD, 'Q', 0, 1 }, { 0, 0, 0, 0, 0, } },
	//Edit Menu
	{ cmd_Undo, { 0, CMD, 'Z', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_Redo, { 0, CMD+SHIFT, 'Z', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_Cut, { 0, CMD, 'X', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_Copy, { 0, CMD, 'C', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_Paste, { 0, CMD, 'V', 0, 1 }, { 0, 0, 0, 0, 0, } },
//	{ cmd_Clear, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectAll, { 0, CMD, 'A', 0, 1 },  { 0, 0, 0, 0, 0, } },
//	{ cmd_ClearSelect, { 0, 0, '0, 0, 0 },  { 0, 0, 0, 0, 0, } },
	{ cmd_Balance, { 0, CMD, 'B', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ShiftLeft, { 0, CMD, '[', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ShiftRight, { 0, CMD, ']', 0, 1 }, { 0, 0, 0, 0, 0, } },
//	{ cmd_ProjectSettings, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, } },
//	{ cmd_Preferences, { 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, } },
	//Search menu
	{ cmd_Find, { 0, CMD, 'F', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FindNext, { 0, CMD, 'G', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FindPrevious, { 0, CMD+SHIFT, 'G', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FindInNextFile, { 0, CMD, 'T', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FindInPrevFile, { 0, CMD+SHIFT, 'T', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_EnterFindString, { 0, CMD, 'E', 0, 1 },  { 0, 0, 0, 0, 0, } },
	{ cmd_EnterReplaceString, { 0, CMD+SHIFT, 'E', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FindSelection, { 0, CMD, 'H', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FindPrevSelection, { 0, CMD+SHIFT, 'H', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_Replace, { 0, CMD, '=', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ReplaceAndFindNext, { 0, CMD, 'L', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ReplaceAndFindPrev, { 0, CMD+SHIFT, 'L', 0, 1 }, { 0, 0, 0, 0, 0, } },
//	{ cmd_ReplaceAll, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_FindDefinition, { 0, CMD, '\'', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FindDocumentation, { 0, CMD+SHIFT, 'D', 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_GoBack, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_GotoLine, { 0, CMD, ',', 0, 1 }, { 0, 0, 0, 0, 0, } },
	//Project Menu
//	{ cmd_AddFiles, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_AddWindow, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_RemoveFiles, { 0, OPT, BACK_DELETE, 0, 1, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_CreateGroup, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_SortGroup, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_Compile, { 0, CMD, 'K', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_CheckSyntax, { 0, CMD, ';', 0, 1 },  { 0, 0, 0, 0, 0, } },
	{ cmd_BringUpToDate, { 0, CMD, 'U', 0, 1 }, { 0, 0, 0, 0, 0, } },
//	{ CMD_LINK, { 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_Make, { 0, CMD, 'M', 0, 1 }, { 0, 0, 0, 0, 0, } },
//	{ cmd_Precompile, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_Preprocess, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_Disassemble, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_RemoveBinaries, { 0, CMD, '-', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_RemoveBinariesCompact, { 0, CMD+OPT, '-', 0, 1 }, { 0, CMD+CNTRL, '-', 0, 1 } },
//	{ cmd_ResetFilePaths, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_EnableDebugging, { 0, 0, 0, 0, 0, } ,  { 0, 0, 0, 0, 0, } },
	{ cmd_Run, { 0, CMD, 'R', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_RunOpposite, { 0, CMD+OPT, 'R', 0, 1 }, { 0, 0, 0, 0, 0, } },
	//Window Menu
//	{ cmd_Stack, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_Tile, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_TileVertical, { 0, 0, 0, 0, 0, }, { 0, 0, 0, 0, 0, } },
	{ cmd_ZoomWindow, { 0, CMD, '/', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ErrorMessageWindow, { 0, CMD, 'I', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ShowProjectWindow, { 0, CMD, '0', 0, 1 },  { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+1, { 0, CMD, '1', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+2, { 0, CMD, '2', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+3, { 0, CMD, '3', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+4, { 0, CMD, '4', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+5, { 0, CMD, '5', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+6, { 0, CMD, '6', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+7, { 0, CMD, '7', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+8, { 0, CMD, '8', 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_FirstWindowListCmd+9, { 0, CMD, '9', 0, 1 }, { 0, 0, 0, 0, 0, } },
	//Misc Menu
	{ cmd_AndyFeature, { 0, CMD, TAB, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_PrevMessage, { 0, CMD+OPT, UP_ARROW, 0, 1, }, { 0, CMD+CNTRL, UP_ARROW, 0, 1, } },
	{ cmd_NextMessage, { 0, CMD+OPT, DOWN_ARROW, 0, 1, }, { 0, CMD+CNTRL, DOWN_ARROW, 0, 1, } },
	{ cmd_Cancel, { 0, CMD, '.', 0, 1, },  { 0, 0, 0, 0, 0, } },
};

static KeyBindingInfo DefaultEditorBindings[] =
{
	//Editor Commands
	{ cmd_MoveToPreviousCharacter, { 0, 0, LEFT_ARROW, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToNextCharacter, { 0, 0, RIGHT_ARROW, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToPreviousWord, { 0, OPT, LEFT_ARROW, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToNextWord, { 0, OPT, RIGHT_ARROW, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToPreviousSubWord, { 0, CNTRL, LEFT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToNextSubWord, { 0, CNTRL, RIGHT_ARROW, 0, 1 },  { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToStartOfLine, { 0, CMD, LEFT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToEndOfLine, { 0, CMD, RIGHT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToPreviousLine, { 0, 0, UP_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToNextLine, { 0, 0, DOWN_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToTopOfPage, { 0, OPT, UP_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToBottomOfPage, { 0, OPT, DOWN_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToTopOfFile, { 0, CMD, UP_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_MoveToBottomOfFile, { 0, CMD, DOWN_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_BackwardDelete, { 0, 0, BACK_DELETE, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ForwardDelete, { 0, 0, FWD_DELETE, 0, 1, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_DeleteToStartOfLine, { 0, 0, 0, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_DeleteToEndOfLine, { 0, CMD, FWD_DELETE, 0, 0, }, { 0, 0, 0, 0, 0, } },
//	{ cmd_DeleteToStartOfFile, { 0, 0, 0, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_DeleteToEndOfFile, { 0, CMD, BACK_DELETE, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectPreviousCharacter, { 0, SHIFT, LEFT_ARROW, 0, 1 },  { 0, 0, 0, 0, 0, } },
	{ cmd_SelectNextCharacter, { 0, SHIFT, RIGHT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectPreviousWord, { 0, OPT+SHIFT, LEFT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectNextWord, { 0, OPT+SHIFT, RIGHT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectPreviousSubWord, { 0, CNTRL+SHIFT, LEFT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectNextSubWord, { 0, CNTRL+SHIFT, RIGHT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectPreviousLine, { 0, SHIFT, UP_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectNextLine, { 0, SHIFT, DOWN_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectToStartOfLine, { 0, CMD+SHIFT, LEFT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectToEndOfLine, { 0, CMD+SHIFT, RIGHT_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectToTopOfPage, { 0, OPT+SHIFT, UP_ARROW, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectToBottomOfPage, { 0, OPT+SHIFT, DOWN_ARROW, 0, 1, }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectToStartOfFile, { 0, CMD+SHIFT, UP_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_SelectToEndOfFile, { 0, CMD+SHIFT, DOWN_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ScrollUpLine, { 0, CNTRL, UP_ARROW, 0, 1 },  { 0, 0, 0, 0, 0, } },
	{ cmd_ScrollDownLine, { 0, CNTRL, DOWN_ARROW, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ScrollUpPage, { 0, 0, PAGEUP, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ScrollDownPage, { 0, 0, PAGEDOWN, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ScrollToTopOfFile, { 0, 0, HOME, 0, 1 }, { 0, 0, 0, 0, 0, } },
	{ cmd_ScrollToEndOfFile, { 0, 0, END, 0, 1 }, { 0, 0, 0, 0, 0, } },
};


MKeyBindingManager*		MKeyBindingManager::sGlobalManager;

// ---------------------------------------------------------------------------
//		MKeyBindingManager
// ---------------------------------------------------------------------------
//	Constructor

MKeyBindingManager::MKeyBindingManager()
{
	// Check that these sizes don't change
	ASSERT(sizeof(KeyBinding) == 8);
	ASSERT(sizeof(KeyBindingPrefHeader) == 6);
	ASSERT(sizeof(KeyBindingInfo) == 4 + 8 + 8);
	
	InitContexts(kBindingContextCount);

	BMallocIO		defaults;
	
	BuildDefaultKeyBindings(defaults);
	
	BMemoryIO		memDefaults(defaults.Buffer(), defaults.BufferLength());
	
	SetKeyBindingsHost(memDefaults);
}

// ---------------------------------------------------------------------------
//		MKeyBindingManager
// ---------------------------------------------------------------------------
//	Constructor

MKeyBindingManager::MKeyBindingManager(
	BMemoryIO&	inData,
	EndianT		inEndian,		// big or host endian
	bool		inIsGlobal)
{
	// Check that these sizes don't change
	ASSERT(sizeof(KeyBinding) == 8);
	ASSERT(sizeof(KeyBindingPrefHeader) == 6);
	ASSERT(sizeof(KeyBindingInfo) == 4 + 8 + 8);

	InitContexts(kBindingContextCount);
	if (inEndian == kBigEndian)
		SetKeyBindingsBig(inData);
	else
		SetKeyBindingsHost(inData);
	
	if (inIsGlobal)
		sGlobalManager = this;
}

// ---------------------------------------------------------------------------
//		~MKeyBindingManager
// ---------------------------------------------------------------------------
//	Destructor

MKeyBindingManager::~MKeyBindingManager()
{
	ReleaseContexts();
}

// ---------------------------------------------------------------------------
//		operator =
// ---------------------------------------------------------------------------

MKeyBindingManager&
MKeyBindingManager::operator = (
	const MKeyBindingManager& inManager)
{
	if (&inManager != this)
	{
		BMallocIO	data;
		
		inManager.GetKeyBindingsHost(data);
		
		BMemoryIO	memData(data.Buffer(), data.BufferLength());
		SetKeyBindingsHost(memData);
	}
	
	return *this;
}

// ---------------------------------------------------------------------------
//		InitContexts
// ---------------------------------------------------------------------------

void
MKeyBindingManager::InitContexts(
	int32	inHowMany)
{
	fBindingContexts.MakeEmpty();
	fBindingContexts.AddItem(0);		// the zero item doesn't exist
	
	for (int32 i = 1; i <= inHowMany; ++i)
	{
		KeyBindingContextData*		data = new KeyBindingContextData;
		
		data->whichContext = data->bindingCount = 0; 
		data->bindingData = 0;
		fBindingContexts.AddItem(data);
	}
}

// ---------------------------------------------------------------------------
//		MessageToBinding
// ---------------------------------------------------------------------------
//	Extract the keydown info from a keydown message and translate it to a 
//	KeyBinding struct.  Return true if the keydown message could represent
//	a valid keybinding.  Doesn't set the prefixIndex field of the keybinding;
//	that needs to be done by the caller.

bool
MKeyBindingManager::MessageToBinding(
	BMessage*		inMessage, 
	KeyBinding&		outBinding)
{
	bool		goodBinding = true;
	bool		isVKey = false;;
	uint32		modifiers;
	int32		raw_char;
	int32		key;
	uchar		theChar;
	
	ASSERT(B_OK == inMessage->FindInt32("raw_char", &raw_char));	// notify if they ever remove this

	inMessage->FindInt32("modifiers", (int32*) &modifiers);
	inMessage->FindInt32("raw_char", &raw_char);
	inMessage->FindInt32("key", &key);

	// Handle the key, keypad keys are special
	switch (key)
	{
		// keycodes for keys on the keypad
		case 0x37:		// 7
		case 0x38:		// 8
		case 0x39:		// 9
		case 0x48:		// 4
		case 0x49:		// 5
		case 0x4A:		// 6
		case 0x58:		// 1
		case 0x59:		// 2
		case 0x5A:		// 3
		case 0x64:		// 0
		case 0x65:		// .
		{
			key_map*		keyMap;
			char*			keys;
			char*			theKey;
			int32*			map;
			bool			numLock = (modifiers & B_NUM_LOCK) != 0;
			bool			shift = (modifiers & B_SHIFT_KEY) != 0;

			get_key_map(&keyMap, &keys);
			
			if ((shift && ! numLock) || (numLock && ! shift))
				map = keyMap->shift_map;
			else
				map = keyMap->normal_map;

			theKey = &keys[map[key]];
			
			if (theKey[0] == 1)			// check length byte
			{
				theChar = theKey[1];
			}
			else
				goodBinding = false;	// key combos can't have utf8 chars

			free(keyMap);
			free(keys);
		}
			break;

		default:
			if (raw_char != B_FUNCTION_KEY)
			{
				// Normal keydown
				theChar = toupper(raw_char);
			}
			else
			{
				// Virtual key, function key
				// Save the keycode instead of the ascii char
				isVKey = true;
				theChar = key;
			}
			break;	
	}

	// we don't set the prefixIndex
	outBinding.modifiers = modifiers & kGoodModifiers;	// don't care about right/left keys
	outBinding.keyCode = theChar;
	outBinding.isVKey = isVKey;
	
	return goodBinding;
}

// ---------------------------------------------------------------------------
//		IsPrefixKey
// ---------------------------------------------------------------------------
//	Checks if the binding represents a prefix key.  If it is it sets the 
//	prefix index in the binding. 

bool
MKeyBindingManager::IsPrefixKey(
	KeyBinding&	inBinding) const
{
	bool		isPrefix = false;
	
	for (int i = 1; i < kBindingPrefixCount+1; i++)
	{
		if (inBinding == fPrefixKeys[i])
		{
			inBinding.prefixIndex = i;
			isPrefix = true;
			break;
		}
	}
	
	return isPrefix;
}

// ---------------------------------------------------------------------------
//		GetPrefixBinding
// ---------------------------------------------------------------------------

void
MKeyBindingManager::GetPrefixBinding(
	int32			inPrefixIndex,
	KeyBinding&		outBinding) const
{
	ASSERT(inPrefixIndex >= 0 && inPrefixIndex <= kBindingPrefixCount);
	
	outBinding = fPrefixKeys[inPrefixIndex];
}

// ---------------------------------------------------------------------------
//		SetPrefixBinding
// ---------------------------------------------------------------------------

void
MKeyBindingManager::SetPrefixBinding(
	int32				inPrefixIndex,
	const KeyBinding&	inBinding)
{
	ASSERT(inPrefixIndex >= 0 && inPrefixIndex <= kBindingPrefixCount);
	
	fPrefixKeys[inPrefixIndex] = inBinding;
}

// ---------------------------------------------------------------------------
//		GetCommand
// ---------------------------------------------------------------------------
//	returns the command that matches the key binding.  returns 0 if not found.

uint32
MKeyBindingManager::GetCommand(
	KeyBindingContext	inContext,
	const KeyBinding&	inBinding) const
{
	uint32		command = 0;

	KeyBindingContextData*		contextData = fBindingContexts.ItemAt(inContext);
	int32				term = contextData->bindingCount;
	KeyBindingInfo*		info = contextData->bindingData;

	for (int32 i = 0; i < term; i++)
	{
		if (inBinding == info[i].binding1 || 
			inBinding == info[i].binding2)
		{
			command = info[i].cmdNumber;
			break;
		}
	}
	
	if (command == 0 && inContext != kBindingGlobal)
	{
		contextData = fBindingContexts.ItemAt(kBindingGlobal);
		term = contextData->bindingCount;
		info = contextData->bindingData;

		for (int32 i = 0; i < term; i++)
		{
			if (inBinding == info[i].binding1 || 
				inBinding == info[i].binding2)
			{
				command = info[i].cmdNumber;
				break;
			}
		}
	}

	return command;
}

// ---------------------------------------------------------------------------
//		GetBinding
// ---------------------------------------------------------------------------
//	Find the binding that matches the command.  returns true if it was found.
//	returns the primary binding if there are two.
//	This is used for building menus.  Presence of prefix keys in the
//	binding may cause problems.

bool
MKeyBindingManager::GetBinding(
	KeyBindingContext	inContext,
	CommandT			inCommand,
	KeyBinding&			outBinding) const
{
	bool	found = false;
	
	KeyBindingContextData*		contextData = fBindingContexts.ItemAt(inContext);
	int32						term = contextData->bindingCount;
	KeyBindingInfo*				info = contextData->bindingData;

	for (int32 i = 0; i < term; i++)
	{
		if (inCommand == info[i].cmdNumber)
		{
			outBinding = info[i].binding1;
			found = true;
			break;
		}
	}

	return found;
}

// ---------------------------------------------------------------------------
//		GetBinding
// ---------------------------------------------------------------------------
//	Find the binding that matches the command.  returns true if it was found.

bool
MKeyBindingManager::GetBinding(
	KeyBindingContext	inContext,
	CommandT			inCommand,
	KeyBindingInfo&		outBinding) const
{
	bool	found = false;
	
	KeyBindingContextData*		contextData = fBindingContexts.ItemAt(inContext);
	int32				term = contextData->bindingCount;
	KeyBindingInfo*		info = contextData->bindingData;

	for (int32 i = 0; i < term; i++)
	{
		if (inCommand == info[i].cmdNumber)
		{
			outBinding = info[i];
			found = true;
			break;
		}
	}

	return found;
}

// ---------------------------------------------------------------------------
//		SetBinding
// ---------------------------------------------------------------------------

void
MKeyBindingManager::SetBinding(
	KeyBindingContext		inContext,
	const KeyBindingInfo&	inBinding)
{
	KeyBindingContextData*		contextData = fBindingContexts.ItemAt(inContext);
	int32				term = contextData->bindingCount;
	KeyBindingInfo*		info = contextData->bindingData;
	bool				found = false;
	int32 				i;

	// Find the binding
	for (i = 0; i < term; i++)
	{
		if (inBinding.cmdNumber == info[i].cmdNumber)
		{
			found = true;
			break;
		}
	}

	// Modify it in place or add a new one
	if (found)
	{
		if (inBinding.binding1.keyCode == 0 && inBinding.binding2.keyCode == 0)
		{
			// remove the binding since both keycodes are zero
			// don't resize the block
			contextData->bindingCount--;
			int32		blocks = contextData->bindingCount - i;
			if (blocks > 0)
			{
				memmove(&info[i], &info[i+1], blocks * sizeof(KeyBindingInfo));
			}
		}
		else
			info[i] = inBinding;
	}
	else
		AddNewBinding(inContext, inBinding);
}

// ---------------------------------------------------------------------------
//		AddNewBinding
// ---------------------------------------------------------------------------

void
MKeyBindingManager::AddNewBinding(
	KeyBindingContext		inContext,
	const KeyBindingInfo&	inBinding)
{
	KeyBindingContextData*		contextData = fBindingContexts.ItemAt(inContext);
	int32						count = contextData->bindingCount;
	KeyBindingInfo*				newData = new KeyBindingInfo[count + 1];

	memcpy(newData, contextData->bindingData, count * sizeof(KeyBindingInfo));
	newData[count] = inBinding;
	contextData->bindingCount++;
	
	delete [] contextData->bindingData;
	contextData->bindingData = newData;
}

// ---------------------------------------------------------------------------
//		BindingExists
// ---------------------------------------------------------------------------
//	Do the bindings in the Binding struct already exist for another command?
//	return true if the binding exists and set outCommand if the binding
//	exists.

bool
MKeyBindingManager::BindingExists(
	KeyBindingContext		inContext,
	const KeyBindingInfo&	inBinding,
	CommandT&				outCommand) const
{
	bool			exists = false;
	CommandT		command1 = cmd_Nothing;
	CommandT		command2= cmd_Nothing;
	const bool		bindingOneGood = (inBinding.binding1.modifiers != 0 || inBinding.binding1.keyCode != 0);
	const bool		bindingTwoGood = (inBinding.binding2.modifiers != 0 || inBinding.binding2.keyCode != 0);

	if (inContext != kBindingGlobal)
	{
		// Check the specified context
		if (bindingOneGood)
			command1 = GetCommand(inContext, inBinding.binding1);
		if (bindingTwoGood)
			command2 = GetCommand(inContext, inBinding.binding2);

		// if not found check the global context
		if (command1 == cmd_Nothing && command2 == cmd_Nothing)
		{
			inContext = kBindingGlobal;
			if (bindingOneGood)
				command1 = GetCommand(inContext, inBinding.binding1);
			if (bindingTwoGood)
				command2 = GetCommand(inContext, inBinding.binding2);
		}
	}
	else
	{
		// If the specified context is the global context
		// then check all the contexts
		int32		contextCount = ContextCount();
		int32		context;
		
		for (context = 1; context <= contextCount; context++)
		{
			if (bindingOneGood && command1 == cmd_Nothing)
				command1 = GetCommand(static_cast<KeyBindingContext>(context), inBinding.binding1);
			if (bindingTwoGood && command2 == cmd_Nothing)
				command2 = GetCommand(static_cast<KeyBindingContext>(context), inBinding.binding2);
		}
	}

	bool	command1IsDuplicate = (command1 != cmd_Nothing && command1 != inBinding.cmdNumber);
	bool	command2IsDuplicate = (command2 != cmd_Nothing && command2 != inBinding.cmdNumber);
	
	if (command1IsDuplicate || command2IsDuplicate)
	{
		exists = true;
		
		if (command1IsDuplicate )		
			outCommand = command1;
		else
			outCommand = command2;
	}

	return exists;
}

// ---------------------------------------------------------------------------
//		SetKeyBindingsBig
// ---------------------------------------------------------------------------
//	Import the key bindings.  The data passed in is bigendian and is
//	translated to host endian.

bool
MKeyBindingManager::SetKeyBindingsBig(
	BMemoryIO&	inData)
{
	inData.Seek(0, SEEK_SET);

	// Read the header
	KeyBindingPrefHeader	header;
	inData.Read(&header, sizeof(header));
	header.SwapBigToHost();

	// verify that we have what appears to be good data
	if (header.version != kKeyBindingsVersion 
			|| header.prefixCount != kBindingPrefixCount) {
		return false;
	}
	
	fPrefixTimeout = TicksToMicroSeconds(header.prefixTimeout);

	// Read the prefix bindings
	for (int i = 1; i < kBindingPrefixCount+1; ++i)
	{
		inData.Read(&fPrefixKeys[i], sizeof(KeyBinding));
		if (B_HOST_IS_LENDIAN)
			fPrefixKeys[i].SwapBigToHost();
	}

	// Read the context count
	int16		contextCount;
	
	inData.Read(&contextCount, sizeof(contextCount));
	contextCount = B_BENDIAN_TO_HOST_INT16(contextCount);

	// Reset all the context data
	ReleaseContexts();
	InitContexts(contextCount);
	
	// Read the rest of the bindings

	for (int32 i = 1; i <= contextCount; i++)
	{
		int16			whichContext;
		int16			bindingCount;
		KeyBindingInfo*	bindingData;

		// read the header
		inData.Read(&whichContext, sizeof(whichContext));
		inData.Read(&bindingCount, sizeof(bindingCount));
		whichContext = B_BENDIAN_TO_HOST_INT16(whichContext);
		bindingCount = B_BENDIAN_TO_HOST_INT16(bindingCount);

		bindingData = new KeyBindingInfo[bindingCount];
		
		// read the data
		inData.Read(bindingData, bindingCount * sizeof(KeyBindingInfo));
	
		if (B_HOST_IS_LENDIAN)
		{	
			for (int32 j = 0; j < bindingCount; j++)
				bindingData[j].SwapBigToHost();
		}

		KeyBindingContextData*		contextData = fBindingContexts.ItemAt(whichContext);

		contextData->whichContext = whichContext;
		contextData->bindingCount = bindingCount;
		contextData->bindingData = bindingData;
	}

	return true;
}

// ---------------------------------------------------------------------------
//		SetKeyBindingsHost
// ---------------------------------------------------------------------------
//	Import the key bindings.  The date passed in is hostendian.

bool
MKeyBindingManager::SetKeyBindingsHost(
	BMemoryIO&	inData)
{
	inData.Seek(0, SEEK_SET);

	// Read the header
	KeyBindingPrefHeader	header;
	inData.Read(&header, sizeof(header));

	// verify that we have what appears to be good data
	if (header.version != kKeyBindingsVersion 
			|| header.prefixCount != kBindingPrefixCount) {
		return false;
	}

	fPrefixTimeout = TicksToMicroSeconds(header.prefixTimeout);

	// Read the prefix bindings
	for (int i = 1; i < kBindingPrefixCount+1; ++i)
	{
		inData.Read(&fPrefixKeys[i], sizeof(KeyBinding));
	}

	// Read the context count
	int16		contextCount;
	
	inData.Read(&contextCount, sizeof(contextCount));

	// Reset all the context data
	ReleaseContexts();
	InitContexts(contextCount);
	
	// Read the rest of the bindings
	for (int32 i = 1; i <= contextCount; i++)
	{
		int16			whichContext;
		int16			bindingCount;
		KeyBindingInfo*	bindingData;

		// read the header
		inData.Read(&whichContext, sizeof(whichContext));
		inData.Read(&bindingCount, sizeof(bindingCount));

		bindingData = new KeyBindingInfo[bindingCount];
		
		// read the data
		inData.Read(bindingData, bindingCount * sizeof(KeyBindingInfo));
		
		KeyBindingContextData*		contextData = fBindingContexts.ItemAt(whichContext);

		contextData->whichContext = whichContext;
		contextData->bindingCount = bindingCount;
		contextData->bindingData = bindingData;
	}
	
	return true;
}

// ---------------------------------------------------------------------------
//		GetKeyBindingsBig
// ---------------------------------------------------------------------------
//	Export the key bindings.

void
MKeyBindingManager::GetKeyBindingsBig(
	BMallocIO&	outData) const
{
	outData.SetSize(0);

	// Write the header
	KeyBindingPrefHeader	header;
	
	header.version = kKeyBindingsVersion;
	header.prefixTimeout = MicroSecondsToTicks(fPrefixTimeout);
	header.prefixCount = kBindingPrefixCount;
	header.SwapHostToBig();
	outData.Write(&header, sizeof(header));

	// Write the prefix bindings
	for (int i = 1; i < kBindingPrefixCount+1; ++i)
	{
		KeyBinding		binding = fPrefixKeys[i];
		binding.SwapHostToBig();
		outData.Write(&binding, sizeof(KeyBinding));
	}
	
	// Write the context count
	int16		contextCount = fBindingContexts.CountItems() - 1;	// contexts count from 1
	int16		swapContextCount = B_HOST_TO_BENDIAN_INT16(contextCount);
	outData.Write(&swapContextCount, sizeof(swapContextCount));

	// Write out each context
	for (int32 i = 1; i <= contextCount; i++)
	{
		KeyBindingContextData		contextData = *fBindingContexts.ItemAt(i);
		KeyBindingInfo*				bindingData;
		int32						bindingcount = contextData.bindingCount;
		
		if (B_HOST_IS_LENDIAN)
		{
			bindingData = new KeyBindingInfo[bindingcount];
			memcpy(bindingData, contextData.bindingData, bindingcount * sizeof(KeyBindingInfo));

			for (int32 j = 0; j < bindingcount; j++)
				bindingData[j].SwapHostToBig();
		}
		else
			bindingData = contextData.bindingData;

		contextData.SwapHostToBig();
		outData.Write(&contextData.whichContext, sizeof(int16));
		outData.Write(&contextData.bindingCount, sizeof(int16));
		outData.Write(bindingData, bindingcount * sizeof(KeyBindingInfo));
		
		if (B_HOST_IS_LENDIAN)
			delete [] bindingData;
	}

	// Keybindings for add-ons will probably go here
}

// ---------------------------------------------------------------------------
//		GetKeyBindingsHost
// ---------------------------------------------------------------------------
//	Export the key bindings.

void
MKeyBindingManager::GetKeyBindingsHost(
	BMallocIO&	outData) const
{
	outData.SetSize(0);

	// Write the header
	KeyBindingPrefHeader	header;
	
	header.version = kKeyBindingsVersion;
	header.prefixTimeout = MicroSecondsToTicks(fPrefixTimeout);
	header.prefixCount = kBindingPrefixCount;
	outData.Write(&header, sizeof(header));

	// Write the prefix bindings
	for (int i = 1; i < kBindingPrefixCount+1; ++i)
	{
		outData.Write(&fPrefixKeys[i], sizeof(KeyBinding));
	}
	
	// Write the context count
	int16		contextCount = fBindingContexts.CountItems() - 1;	// contexts count from 1
	outData.Write(&contextCount, sizeof(contextCount));

	// Write out each context
	for (int32 i = 1; i <= contextCount; i++)
	{
		KeyBindingContextData*		contextData = fBindingContexts.ItemAt(i);

		outData.Write(&contextData->whichContext, sizeof(int16));
		outData.Write(&contextData->bindingCount, sizeof(int16));
		outData.Write(contextData->bindingData, contextData->bindingCount * sizeof(KeyBindingInfo));
	}

	// Keybindings for add-ons will probably go here
}

// ---------------------------------------------------------------------------
//		BuildDefaultKeyBindings
// ---------------------------------------------------------------------------
//	Build a default set of key bindings.  Written in host endian

void
MKeyBindingManager::BuildDefaultKeyBindings(
	BMallocIO&	outData)
{
	outData.SetSize(0);

	// Write the header
	KeyBindingPrefHeader	header;

	header.version = kKeyBindingsVersion;
	header.prefixTimeout = 120;
	header.prefixCount = kBindingPrefixCount;
	outData.Write(&header, sizeof(header));

	// Write empty prefix bindings
	KeyBinding		prefix = { 0, 0, 0, false, 0 };

	for (int i = 0; i < kBindingPrefixCount; ++i)
		outData.Write(&prefix, sizeof(prefix));

	// Write the rest of the bindings
	const int16		globalCount = sizeof(DefaultGlobalBindings) / sizeof(KeyBindingInfo);
	const int16		editorCount = sizeof(DefaultEditorBindings) / sizeof(KeyBindingInfo);
	int16			count = 2;

	// write the context count
	outData.Write(&count, sizeof(count));
	
	// Write the global context and bindings
	int16		index = 1;
	count = globalCount;
	outData.Write(&index, sizeof(index));
	outData.Write(&count, sizeof(count));
	outData.Write(&DefaultGlobalBindings, sizeof(DefaultGlobalBindings));

	// Write the editor context and bindings
	index = 2;
	count = editorCount;
	outData.Write(&index, sizeof(index));
	outData.Write(&count, sizeof(count));
	outData.Write(&DefaultEditorBindings, sizeof(DefaultEditorBindings));
}

// ---------------------------------------------------------------------------
//		ReleaseContexts
// ---------------------------------------------------------------------------
//	Release all the memory.

void
MKeyBindingManager::ReleaseContexts()
{
	KeyBindingContextData*		data;
	int32						i = 0;

	while (fBindingContexts.GetNthItem(data, i++))
	{
		if (data != nil)
		{
			delete [] data->bindingData;
			delete data;
		}
	}

	fBindingContexts.MakeEmpty();
}

// ---------------------------------------------------------------------------
//		PrefixTimeout
// ---------------------------------------------------------------------------

bigtime_t
MKeyBindingManager::PrefixTimeout()
{
	return fPrefixTimeout;
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
KeyBindingPrefHeader::SwapBigToHost()
{
	version = B_BENDIAN_TO_HOST_INT16(version);
	prefixTimeout = B_BENDIAN_TO_HOST_INT16(prefixTimeout);
	prefixCount = B_BENDIAN_TO_HOST_INT16(prefixCount);
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
KeyBindingPrefHeader::SwapHostToBig()
{
	version = B_HOST_TO_BENDIAN_INT16(version);
	prefixTimeout = B_HOST_TO_BENDIAN_INT16(prefixTimeout);
	prefixCount = B_HOST_TO_BENDIAN_INT16(prefixCount);
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
KeyBinding::SwapBigToHost()
{
	prefixIndex = B_BENDIAN_TO_HOST_INT16(prefixIndex);
	modifiers = B_BENDIAN_TO_HOST_INT16(modifiers);
	keyCode = B_BENDIAN_TO_HOST_INT16(keyCode);
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
KeyBinding::SwapHostToBig()
{
	prefixIndex = B_HOST_TO_BENDIAN_INT16(prefixIndex);
	modifiers = B_HOST_TO_BENDIAN_INT16(modifiers);
	keyCode = B_HOST_TO_BENDIAN_INT16(keyCode);
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
KeyBindingInfo::SwapBigToHost()
{
	cmdNumber = B_BENDIAN_TO_HOST_INT32(cmdNumber);
	binding1.SwapBigToHost();
	binding2.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
KeyBindingInfo::SwapHostToBig()
{
	cmdNumber = B_HOST_TO_BENDIAN_INT32(cmdNumber);
	binding1.SwapHostToBig();
	binding2.SwapHostToBig();
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
KeyBindingContextData::SwapHostToBig()
{
	whichContext = B_HOST_TO_BENDIAN_INT16(whichContext);
	bindingCount = B_HOST_TO_BENDIAN_INT16(bindingCount);
	// we don't swap the binding data here
}
