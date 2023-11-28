// ===========================================================================
//	MainMenus.cpp
//	
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//
// ===========================================================================
//	Main window menu data

#include "MainMenus.h"
#include "ProjectCommands.h"
#include "MKeyBindingManager.h"
#include "MDynamicMenuHandler.h"
#include "MMessageWindow.h"
#include "MFindWindow.h"
#include "MIDECommandList.h"
#include "MPathPopup.h"
#include "IDEConstants.h"
#include "IDEMessages.h"
#include "MIconMenuItem.h"

const char kProjectBarName[] = "ProjectBar";

const int32 kFilePathIndex = 1;

const int16 kEscapeValue = 27;

#define ELLIPSIS B_UTF8_ELLIPSIS

struct MM_item {
	const char *	name;
	uint32			command;
};

struct MM_menu {
	const char *		name;
	const MM_item *		items;
};


static MM_item MM_textFile[] = {
	{	"New Text",					cmd_New					},
	{	"New Project"ELLIPSIS,		cmd_NewProject			},
	{	"Open"ELLIPSIS,				cmd_OpenFile			},
	{	"Find & Open File"ELLIPSIS,	cmd_OpenSelection		},
	{	"",							0						},
	{	"Save",						cmd_Save				},
	{	"Save As"ELLIPSIS,			cmd_SaveAs				},
	{	"Save A Copy As"ELLIPSIS,	cmd_SaveCopyAs			},
	{	"Revert"ELLIPSIS,			cmd_Revert				},
	{	"Close",					cmd_Close				},
	{	"",							0						},
	{	"Page Setup"ELLIPSIS,		cmd_PageSetup			},
	{	"Print"ELLIPSIS,			cmd_Print				},
	{	"",							0						},
	{	"About BeIDE"ELLIPSIS,		cmd_About				},
	{	"",							0						},
	{	"Quit",						cmd_Quit				},
	{	NULL,						0						},
};

// ---------------------------------------------------------------------------
// Offsets based on all file menus...
// ---------------------------------------------------------------------------

const int32 kOpenRecentMenuItemIndex = 4;

// command used as key to find the corrent menu for the recent projects menu item
const CommandT kRecentProjectMenuKey = cmd_OpenFile;

// ---------------------------------------------------------------------------

static MM_item MM_textSearch[] = {
	{	"Find",							cmd_Find				},
	{	"Find Next",					cmd_FindNext			},
	{	"Find in Next File",			cmd_FindInNextFile,		},
	{	"Enter 'Find' String",			cmd_EnterFindString		},
	{	"Find Selection",				cmd_FindSelection		},
	{	"",								0						},
	{	"Replace",						cmd_Replace				},
	{	"Replace & Find Next",			cmd_ReplaceAndFindNext	},
	{	"Replace All",					cmd_ReplaceAll			},
	{	"",								0						},
	{	"Find Definition"ELLIPSIS,		cmd_FindDefinition		},
	{	"Lookup Documentation"ELLIPSIS,	cmd_FindDocumentation	},
	{	"",								0						},
	{	"Go To Line"ELLIPSIS,			cmd_GotoLine			},
	{	NULL,							0						},
};

// ---------------------------------------------------------------------------

static MM_item MM_textEdit[] = {
	{	"Undo",					cmd_Undo				},
	{	"Redo",					cmd_Redo				},
	{	"",						0						},
	{	"Cut",					cmd_Cut					},
	{	"Copy",					cmd_Copy				},
	{	"Paste",				cmd_Paste				},
	{	"",						0						},
	{	"Select All",			cmd_SelectAll			},
	{	"",						0						},
	{	"Balance",				cmd_Balance				},
	{	"Shift Left",			cmd_ShiftLeft			},
	{	"Shift Right",			cmd_ShiftRight			},
	{	"",						0						},
	{	"Preferences"ELLIPSIS,	cmd_Preferences			},
	{	NULL,					0						},
};

// ---------------------------------------------------------------------------

static MM_item MM_textProject[] = {
	{	"Add to Project",	cmd_AddWindow			},
	{	"",					0,						},
	{	"Compile",			cmd_Compile				},
	{	"Check Syntax",		cmd_CheckSyntax			},
	{	"Bring Up to Date",	cmd_BringUpToDate		},
	{	"Link",				CMD_LINK				},
	{	"Make",				cmd_Make				},
	{	"",					0						},
	{	"Precompile",		cmd_Precompile			},
	{	"Preprocess",		cmd_Preprocess			},
	{	"Disassemble",		cmd_Disassemble			},
	{	NULL,			0							},
};

// ---------------------------------------------------------------------------

// used for text, project, and message windows
static MM_item MM_textWindow[] = {
	{	"Stack",			cmd_Stack				},
	{	"Tile",				cmd_Tile				},
	{	"Tile Vertical",	cmd_TileVertical		},
	{	"Zoom Window",		cmd_ZoomWindow			},
	{	"",					0						},
	{	"No Project",		cmd_ShowProjectWindow	},
	{	"Errors & Warnings",cmd_ErrorMessageWindow	},
	{	"",					0						},
	{	NULL,				0						},
};

// ---------------------------------------------------------------------------
// Number of items in a window menu with no text windows
const int32 kWinMenuMinItemCount = 8;

// ---------------------------------------------------------------------------

static MM_item MM_projFile[] = {
	{	"New Text",					cmd_New					},
	{	"New Project"ELLIPSIS,		cmd_NewProject			},
	{	"Open"ELLIPSIS,				cmd_OpenFile			},
	{	"Find & Open File"ELLIPSIS,	cmd_OpenSelection		},
	{	"",							0						},
	{	"Save A Copy As"ELLIPSIS,	cmd_SaveCopyAs			},
	{	"Close",					cmd_Close				},
	{	"",							0						},
	{	"About BeIDE"ELLIPSIS,		cmd_About				},
	{	"",							0						},
	{	"Quit",						cmd_Quit				},
	{	NULL,						0						},
};

// ---------------------------------------------------------------------------

static MM_item MM_projEdit[] = {
	{	"Undo",							cmd_Undo				},
	{	"",								0						},
	{	"Cut",							cmd_Cut					},
	{	"Copy",							cmd_Copy				},
	{	"Paste",						cmd_Paste				},
	{	"",								0						},
	{	"Select All",					cmd_SelectAll			},
	{	"Clear Selection",				cmd_ClearSelection		},
	{	"",								0						},
	{	"Find In Files"ELLIPSIS,		cmd_Find				},
	{	"Lookup Documentation"ELLIPSIS,	cmd_FindDocumentation	},
	{	"",								0						},
	{	"Project Settings"ELLIPSIS,		cmd_ProjectSettings		},
	{	"Preferences"ELLIPSIS,			cmd_Preferences			},
	{	NULL,							0						},
};

// ---------------------------------------------------------------------------

static MM_item MM_projProject[] = {
	{	"Add Files"ELLIPSIS,cmd_AddFiles			},
	{	"New Group",	cmd_CreateGroup				},
	{	"Sort Group",	cmd_SortGroup				},
	{	"",				0							},
	{	"Remove Selected Items",cmd_RemoveFiles		},
	{	"",				0							},
	{	"Compile",		cmd_Compile					},
	{	"Check Syntax",	cmd_CheckSyntax				},
	{	"Bring Up to Date",	cmd_BringUpToDate		},
	{	"Link",			CMD_LINK					},
	{	"Make",			cmd_Make					},
	{	"",				0							},
	{	"Precompile",	cmd_Precompile				},
	{	"Preprocess",	cmd_Preprocess				},
	{	"Disassemble",	cmd_Disassemble				},
	{	"",				0							},
	{	"Remove Objects",	cmd_RemoveBinaries		},
	{	"Reset File Paths",	cmd_ResetFilePaths		},
	{	"",				0							},
	{	"Enable Debugger",	cmd_EnableDebugging		},
	{	"Run",			cmd_Run						},
	{	NULL,			0							},
};

// ---------------------------------------------------------------------------
// Offsets based on project menu...
// ---------------------------------------------------------------------------

const int32 kEnableDebuggerID = 19;
const int32 kRunID = 20;

// ---------------------------------------------------------------------------

static MM_item MM_messageFile[] = {
	{	"New Text",					cmd_New				},
	{	"New Project"ELLIPSIS,		cmd_NewProject		},
	{	"Open"ELLIPSIS,				cmd_OpenFile		},
	{	"Find & Open File"ELLIPSIS,	cmd_OpenSelection	},
	{	"",							0					},
	{	"Save A Copy As"ELLIPSIS,	cmd_SaveCopyAs		},
	{	"Close",					cmd_Close			},
	{	"",							0					},
	{	"Quit",						cmd_Quit			},
	{	NULL,						0					},
};

// ---------------------------------------------------------------------------

static MM_item MM_messageEdit[] = {
	{	"Undo",							cmd_Undo		},
	{	"",								0				},
	{	"Cut",							cmd_Cut			},
	{	"Copy",							cmd_Copy		},
	{	"Paste",						cmd_Paste		},
	{	"",								0				},
	{	"Select All",					cmd_SelectAll	},
	{	"",								0				},
	{	"Find In Files"ELLIPSIS,		cmd_Find		},
	{	NULL,							0				},
};

// ---------------------------------------------------------------------------

static MM_item MM_projectContextMenu[] = {
//	{	"Reveal in Tracker",	cmd_RevealInFinder	},
	{	"",						0					},
	{	"Open",					cmd_OpenCurrent		},
	{	"Open Header/Source",	cmd_AndyFeature		},
	{	"",						0					},
	{	"Compile",				cmd_Compile			},
	{	"Check Syntax",			cmd_CheckSyntax		},
	{	"Precompile",			cmd_Precompile		},
	{	"Preprocess",			cmd_Preprocess		},
	{	"Disassemble",			cmd_Disassemble		},
	{	"",						0					},
	{	"Remove Selected Items",cmd_RemoveFiles		},
	{	NULL,			0							},
};

// ---------------------------------------------------------------------------

// constants used here because we search for them in MDynamicMenuHandler
const char* kFileMenuName  = "File";
const char* kEditMenuName = "Edit";
const char* kSearchMenuName = "Search";
const char* kProjectMenuName = "Project";
const char* kWindowMenuName = "Window";

static MM_menu	MM_textMenus[] = {
	{	kFileMenuName,		MM_textFile		},
	{	kEditMenuName,		MM_textEdit		},
	{	kSearchMenuName,	MM_textSearch	},
	{	kProjectMenuName,	MM_textProject	},
	{	kWindowMenuName,	MM_textWindow	},
	{	NULL,				NULL			},
};

// ---------------------------------------------------------------------------

static MM_menu	MM_projMenus[] = {
	{	kFileMenuName,		MM_projFile		},
	{	kEditMenuName,		MM_projEdit		},
	{	kProjectMenuName,	MM_projProject	},
	{	kWindowMenuName,	MM_textWindow	},
	{	NULL,				NULL,			},
};

// ---------------------------------------------------------------------------
// Offsets based on project menu bar...
// ---------------------------------------------------------------------------

const int32 kProjectMenuIndex = 2;

// ---------------------------------------------------------------------------

static MM_menu	MM_messageMenus[] = {
	{	"File",			MM_messageFile		},
	{	"Edit",			MM_messageEdit		},
	{	"Window",		MM_textWindow		},
	{	NULL,			NULL,				},
};

// List of all the menu items that can be grouped
struct CommandMenuGroup
{
	int32		count;
	CommandT	cmd[2];
};

CommandMenuGroup		sCommandGroups[] = {
	// File Menu
	{ 2, { cmd_Save, cmd_SaveAll } },
	{ 2, { cmd_Close, cmd_CloseAll } },

	// Search Menu
	{ 2, { cmd_FindNext, cmd_FindPrevious } },
	{ 2, { cmd_FindInNextFile, cmd_FindInPrevFile } },
	{ 2, { cmd_EnterFindString, cmd_EnterReplaceString } },
	{ 2, { cmd_FindSelection, cmd_FindPrevSelection } },
	{ 2, { cmd_ReplaceAndFindNext, cmd_ReplaceAndFindPrev } },

	// Project Menu
	{ 2, { cmd_RemoveBinaries, cmd_RemoveBinariesCompact } },
	{ 2, { cmd_Run, cmd_RunOpposite } },
};
const int32 kCommandGroupCount = sizeof(sCommandGroups) / sizeof(CommandMenuGroup);

static void
MM_ReadMenuItems(
	BMenu * menu,
	const MM_item * data)
{
	ASSERT(data != NULL);
	while (data->name != NULL)
	{
		if (data->name[0] == '\0')
		{
			menu->AddSeparatorItem();
		}
		else
		{
			BMessage * message = new BMessage(data->command);
			
			KeyBinding		binding;
			
			// A prefix key can't be used in a menu command
			// Don't show command/option values because they don't show right (Alt is always there)
			// Also don't try to show anything less then escape since we can't print it anyway.
			if (! MKeyBindingManager::Manager().GetBinding(kBindingGlobal, data->command, binding) ||
				binding.prefixIndex != 0 ||
				(binding.modifiers & B_COMMAND_KEY) == 0 ||
				binding.keyCode <= kEscapeValue)
			{
				binding.keyCode = 0;
				binding.modifiers = 0;
			}
			
			BMenuItem * item = new BMenuItem(data->name, message, binding.keyCode, binding.modifiers);
			menu->AddItem(item);
		}
		data++;
	}
}


static void
MM_ReadMenus(
	BMenuBar * bar,
	MM_menu * data)
{
	ASSERT(data != NULL);

	while (data->name != NULL)
	{
		BMenu * menu = new BMenu(data->name);
		MM_ReadMenuItems(menu, data->items);
		bar->AddItem(menu);
		data++;
	}
}


BMenuBar *
MakeProjectMenus(
	BRect area)
{
	BMenuBar * bar = new BMenuBar(area, kProjectBarName);
	
	MM_ReadMenus(bar, MM_projMenus);
	MDynamicMenuHandler::AddHierarchalMenuItems(*bar);
	return bar;
}

MPathMenu*
MakeProjectContextMenu(
	entry_ref& 		inRef,
	BPopUpMenu&		inOutMenu,
	BWindow*		inWindow)
{
	MM_ReadMenuItems(&inOutMenu, MM_projectContextMenu);
	inOutMenu.SetTargetForItems(inWindow);
	MPathMenu*		path = new MPathMenu("File Path", inRef);

	// add parent ref to message
	BMessage*		msg = new BMessage(B_REFS_RECEIVED);
	BEntry			entry(&inRef);
	BDirectory		dir;
	BEntry			dirEntry;
	entry_ref		ref;
	status_t		err;

	err = entry.GetParent(&dir);
	err = dir.GetEntry(&dirEntry);
	if (B_NO_ERROR == err)
		err = dirEntry.GetRef(&ref);

	msg->AddRef("refs", &ref);

	MIconMenuItem*	item = new MIconMenuItem(inRef, inRef.name, path, msg);
	bool 			result = inOutMenu.AddItem(item, 0);	// insert at start
//	bool 			result = inOutMenu.AddItem(path, kFilePathIndex);
	
	return path;
}


BMenuBar *
MakeTextMenus(
	BRect area)
{
	BMenuBar * bar = new BMenuBar(area, "Text Window");

	MM_ReadMenus(bar, MM_textMenus);
	MDynamicMenuHandler::AddHierarchalMenuItems(*bar);

	return bar;
}

BMenuBar *
MakeMessageWindowMenus(
	BRect area)
{
	BMenuBar * bar = new BMenuBar(area, "Message Window");
	
	MM_ReadMenus(bar, MM_messageMenus);
	MDynamicMenuHandler::AddHierarchalMenuItems(*bar);
	return bar;
}


static void
UpdateMenuItems(
	BMenu * menu,
	const MM_item * data)
{
	ASSERT(data != NULL);
	while (data->name != NULL)
	{
		if (data->name[0] != '\0')	// ignore separator items
		{
			BMenuItem * 	item = menu->FindItem(data->command);
			KeyBinding		binding;
			
			if (item != nil)
			{
				// Get the binding, if there is none the user may have removed it
				if (! MKeyBindingManager::Manager().GetBinding(kBindingGlobal, data->command, binding))
					binding = kEmptyBinding;

				uint32	modifiers;

				// Check if the existing shortcut in the menu is different and change it if it is
				if (item->Shortcut(&modifiers) != binding.keyCode ||
					modifiers != binding.modifiers)
					item->SetShortcut(binding.keyCode, binding.modifiers);
			}
		}
		data++;
	}
}

static void
UpdateMenus(
	BMenuBar * bar,
	const MM_menu * data)
{
	ASSERT(data != NULL);
	int32		i = 0;
	
	while (data->name != NULL)
	{
		BMenu * menu = bar->SubmenuAt(i++);
		ASSERT(0 == strcmp(data->name, menu->Name()));
		UpdateMenuItems(menu, data->items);
		data++;
	}
}

void
UpdateTextMenus(
	BMenuBar * bar)
{
	UpdateMenus(bar, MM_textMenus);
}

void
UpdateProjectMenus(
	BMenuBar * bar)
{
	UpdateMenus(bar, MM_projMenus);
}

void
UpdateMessageWindowMenus(
	BMenuBar * bar)
{
	UpdateMenus(bar, MM_messageMenus);
}

// ---------------------------------------------------------------------------
//		CommonWindowSetupSave
// ---------------------------------------------------------------------------

void
CommonWindowSetupSave(BWindow& inWindow)
{
	// Set up close all and save all shortcuts
	BMessage*		msg = new BMessage(cmd_CloseAll);
	inWindow.AddShortcut('W', B_OPTION_KEY, msg);	// cmd-option-w

	msg = new BMessage(cmd_CloseAll);
	inWindow.AddShortcut('W', B_CONTROL_KEY, msg);	// cmd-control-w

	msg = new BMessage(cmd_SaveAll);
	inWindow.AddShortcut('S', B_OPTION_KEY, msg);	// cmd-option-s

	msg = new BMessage(cmd_SaveAll);
	inWindow.AddShortcut('S', B_CONTROL_KEY, msg);	// cmd-control-s
}

// ---------------------------------------------------------------------------
//		CommonWindowSetupBuild
// ---------------------------------------------------------------------------
//	Allow these commands to work in windows that don't normally have these
//	menu commands.

void
CommonWindowSetupBuild(BWindow& inWindow)
{
	BMessage*		msg = new BMessage(cmd_Make);
	
	inWindow.AddShortcut('M', 0L, msg);

	msg = new BMessage(cmd_BringUpToDate);
	
	inWindow.AddShortcut('U', 0L, msg);
}

// ---------------------------------------------------------------------------
//		CommonWindowSetupFind
// ---------------------------------------------------------------------------
//	Allow these commands to work in windows that don't normally have these
//	menu commands.

void
CommonWindowSetupFind(BWindow& inWindow)
{
	BMessage*		msg = new BMessage(cmd_Find);
	
	inWindow.AddShortcut('F', 0L, msg);
}

// ---------------------------------------------------------------------------
//		CommonWindowSetupWindow
// ---------------------------------------------------------------------------
//	Allow these commands to work in windows that don't normally have these
//	menu commands.

void
CommonWindowSetupWindow(BWindow& inWindow)
{
	inWindow.AddShortcut('I', 0L, new BMessage(cmd_ErrorMessageWindow));
	inWindow.AddShortcut('0', 0L, new BMessage(cmd_ShowProjectWindow));
	
	inWindow.AddFilter(new MWindowMenuFilter);
}

// ---------------------------------------------------------------------------
//		CommonWindowSetupFile
// ---------------------------------------------------------------------------
//	Allow these commands to work in windows that don't normally have these
//	menu commands.

void
CommonWindowSetupFile(BWindow& inWindow)
{
	BMessage*		msg = new BMessage(cmd_New);
	
	inWindow.AddShortcut('N', 0L, msg);
	msg = new BMessage(cmd_OpenFile);
	inWindow.AddShortcut('O', 0L, msg);
	msg = new BMessage(cmd_OpenSelection);
	inWindow.AddShortcut('D', 0L, msg);
}

// ---------------------------------------------------------------------------
//		SpecialMessageReceived
// ---------------------------------------------------------------------------
//	Handle these messages.  Return true if the message was handled.

bool
SpecialMessageReceived(
	BMessage& 	inMessage,
	BWindow*	inWindow)
{
	bool		handled = true;

	switch (inMessage.what)
	{
		case cmd_Make:
		case cmd_BringUpToDate:
		case cmd_Run:
		case cmd_RunOpposite:
		case cmd_CloseAll:
		case cmd_SaveAll:
		case cmd_Preferences:
		case cmd_New:
		case cmd_NewProject:
		case cmd_OpenFile:
		case cmd_OpenSelection:
		case cmd_About:
		case cmd_Cancel:
			be_app_messenger.SendMessage(&inMessage);
			break;

		case cmd_Quit:
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
			break;

		case cmd_ErrorMessageWindow:
		case cmd_ShowProjectWindow:
			be_app_messenger.SendMessage(&inMessage);
			break;

		case cmd_Find:
			MFindWindow::GetFindWindow().PostMessage(msgFindFromTextWindow);
			break;

		case cmd_Close:
			inWindow->PostMessage(B_QUIT_REQUESTED);
			break;

		case cmd_ZoomWindow:
			inWindow->PostMessage(B_ZOOM);
			break;

		default:
			// If this is a keydown then change it into a WindowMenuClicked message
			// and add the correct index item, as if it was a real menu item message
			if (inMessage.what > cmd_FirstWindowListCmd && inMessage.what < cmd_LastWindowListCmd)
			{
				int32		index;

				if (B_OK != inMessage.FindInt32("index", &index))
				{
					index = MDynamicMenuHandler::IndexOf(inMessage.what - cmd_FirstWindowListCmd) + kWinMenuMinItemCount;
					inMessage.AddInt32("index", index);
				}
				inMessage.what = msgWindowMenuClicked;
				be_app_messenger.SendMessage(&inMessage);
			}
			handled = false;
			break;
	}
	
	return handled;
}

// ---------------------------------------------------------------------------

void
UpdateMenuItemsForModifierKeys(BMenuBar* inMenuBar)
{
	// Update all menus that can change with modifier keys
	
	// Did the user change the modifier keys?
	uint32		modKeys = (modifiers() & kGoodModifiers) | B_COMMAND_KEY ;	// ignore right and left keys
	ASSERT(modKeys < 256);

	// Look through all the dynamic menu items for any
	// that should be changed
	for (int i = 0; i < kCommandGroupCount; ++i)
	{
		KeyBinding		binding;
		
		for (int j = 0; j < sCommandGroups[i].count; ++j)
		{
			CommandT		command = sCommandGroups[i].cmd[j];

			if (MKeyBindingManager::Manager().GetBinding(kBindingGlobal, command, binding) &&
				modKeys == binding.modifiers)
			{
				BMenuItem*	item = nil;
				
				for (int k = 0; k < sCommandGroups[i].count && item == nil; ++k)
				{
					CommandT		aCommand = sCommandGroups[i].cmd[k];
					item = inMenuBar->FindItem(aCommand);
				}

				// item may be nil because that menu item isn't in this
				// window's menus (eg, project window specific items in a text window
				if (item != nil)
				{
					item->SetMessage(new BMessage(command));
					item->SetShortcut(binding.keyCode, binding.modifiers);
					CommandInfo	info;
					bool		found = MIDECommandList::GetCommandInfo(command, info);
					if (found)
						item->SetLabel(info.name);
					ASSERT(found);
				}
			}
		}
	}
}
