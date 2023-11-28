// ===========================================================================
//	MIDECommandList.cpp
//	
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//
// ===========================================================================

#include "MIDECommandList.h"
#include "ProjectCommands.h"

#include <InterfaceDefs.h>
#include <Debug.h>

#define ELLIPSIS B_UTF8_ELLIPSIS

static CommandInfo FileInfo[] = {
	{ cmd_New, "New Text", "" 				},
	{ cmd_NewProject, "New Project", "" 	},
	{ cmd_OpenFile, "Open"ELLIPSIS, "" 		},
	{ cmd_OpenSelection, "Open File"ELLIPSIS, "" },
	{ cmd_Close, "Close", "" 				},
	{ cmd_CloseAll, "Close All", "" 		},
	{ cmd_Save, "Save", "" 					},
	{ cmd_SaveAll, "Save All", "" 			},
	{ cmd_SaveAs, "Save As"ELLIPSIS, "" 	},
	{ cmd_SaveCopyAs, "Save A Copy As"ELLIPSIS, "" },
	{ cmd_Revert, "Revert"ELLIPSIS, "" 		},
	{ cmd_Print, "Print"ELLIPSIS, "" 		},
	{ cmd_PageSetup, "Page Setup"ELLIPSIS, "" },
	{ cmd_About, "About BeIDE"ELLIPSIS, "" 	},
	{ cmd_Quit, "Quit", "" 					},
};

static CommandInfo EditInfo[] = {
	{ cmd_Undo, "Undo", "" 					},
	{ cmd_Redo, "Redo", "" 					},
	{ cmd_Cut, "Cut", "" 					},
	{ cmd_Copy, "Copy", "" 					},
	{ cmd_Paste, "Paste", "" 				},
	{ cmd_Clear, "Clear", "" 				},
	{ cmd_SelectAll, "Select All", "" 		},
	{ cmd_ClearSelection, "Clear Selection", "" 			},
	{ cmd_Balance, "Balance", "" 			},
	{ cmd_ShiftLeft, "Shift Left", "" 		},
	{ cmd_ShiftRight, "Shift right", "" 	},
	{ cmd_ProjectSettings, "Project Settings Window", ""	},
	{ cmd_Preferences, "Preferences Window", ""				},
};

static CommandInfo SearchInfo[] = {
	{ cmd_Find, "Find", "" },
	{ cmd_FindNext, "Find Next", "" },
	{ cmd_FindPrevious, "Find Previous", "" },
	{ cmd_FindInNextFile, "Find in Next File", "" },
	{ cmd_FindInPrevFile, "Find in Previous File", "" },
	{ cmd_EnterFindString, "Enter 'Find' String", "" },
	{ cmd_EnterReplaceString, "Enter 'Replace' String", "" },
	{ cmd_FindSelection, "Find Selection", "" },
	{ cmd_FindPrevSelection, "Find Previous Selection", "" },
	{ cmd_Replace, "Replace", "" },
	{ cmd_ReplaceAndFindNext, "Replace & Find Next", "" },
	{ cmd_ReplaceAndFindPrev, "Replace & Find Previous", "" },
	{ cmd_ReplaceAll, "Replace All", "" },
	{ cmd_GoBack, "Go Back", "" },
	{ cmd_GotoLine, "Go To Line"ELLIPSIS, "" },
	{ cmd_FindDefinition, "Find Definition", "" },
	{ cmd_FindDocumentation, "Find Documentation", "" },
};

static CommandInfo ProjectInfo[] = {
	{ cmd_AddWindow, "Add to Project", "" },
	{ cmd_AddFiles, "Add Files"ELLIPSIS, "" },
	{ cmd_RemoveFiles, "Remove Selected Items", "" },
	{ cmd_CreateGroup, "New Group", "" },
	{ cmd_SortGroup, "Sort Group", "" },
	{ cmd_Compile, "Compile", "" },
	{ cmd_CheckSyntax, "Check Syntax", "" },
	{ cmd_BringUpToDate, "Bring Up to Date", "" },
	{ CMD_LINK, "Link", "" },
	{ cmd_Make, "Make", "" },
	{ cmd_Precompile, "Precompile", "" },
	{ cmd_Preprocess, "Preprocess", "" },
	{ cmd_Disassemble, "Disassemble", "" },
	{ cmd_RemoveBinaries, "Remove Objects", "" },
	{ cmd_RemoveBinariesCompact, "Remove Objects and Compact", "" },
	{ cmd_ResetFilePaths, "Reset File Paths", "" },
	{ cmd_EnableDebugging, "Enable Debugger", "" },
	{ cmd_Run, "Run", "" },
	{ cmd_RunOpposite, "Debug", "" },
};

static CommandInfo WindowInfo[] = {
	{ cmd_Stack, "Stack", "" 							},
	{ cmd_Tile, "Tile", "" 								},
	{ cmd_TileVertical, "Tile Vertical", "" 			},
	{ cmd_ZoomWindow, "Zoom Window", "" 				},
	{ cmd_ShowProjectWindow, "Project Window", "" 		},
	{ cmd_ErrorMessageWindow, "Errors & Warnings Window", "" },
	{ cmd_FirstWindowListCmd+1, "Select Document 1", "" },
	{ cmd_FirstWindowListCmd+2, "Select Document 2", "" },
	{ cmd_FirstWindowListCmd+3, "Select Document 3", "" },
	{ cmd_FirstWindowListCmd+4, "Select Document 4", "" },
	{ cmd_FirstWindowListCmd+5, "Select Document 5", "" },
	{ cmd_FirstWindowListCmd+6, "Select Document 6", "" },
	{ cmd_FirstWindowListCmd+7, "Select Document 7", "" },
	{ cmd_FirstWindowListCmd+8, "Select Document 8", "" },
	{ cmd_FirstWindowListCmd+9, "Select Document 9", "" },
};

static CommandInfo MiscInfo[] = {
	{ cmd_AndyFeature, "Go to header/source file", "" 		},
	{ cmd_PrevMessage, "Go to previous error message", "" 	},
	{ cmd_NextMessage, "Go to next error message", "" 		},
	{ cmd_Cancel, "Stop operation", "" 						},
};

static CommandInfo EditorCommandsInfo[] = {
	{ cmd_MoveToPreviousCharacter, "Move character left", "" },
	{ cmd_MoveToNextCharacter, "Move character right", "" },
	{ cmd_MoveToPreviousWord, "Move word left", "" },
	{ cmd_MoveToNextWord, "Move word right", "" },
	{ cmd_MoveToPreviousSubWord, "Move sub-word left", "" },
	{ cmd_MoveToNextSubWord, "Move sub-word right", "" },
	{ cmd_MoveToStartOfLine, "Move to start of line", "" },
	{ cmd_MoveToEndOfLine, "Move to end of line", "" },
	{ cmd_MoveToPreviousLine, "Move to previous line", "" },
	{ cmd_MoveToNextLine, "Move to next line", "" },
	{ cmd_MoveToTopOfPage, "Move to top of page", "" },
	{ cmd_MoveToBottomOfPage, "Move to bottom of page", "" },
	{ cmd_MoveToTopOfFile, "Move to start of file", "" },
	{ cmd_MoveToBottomOfFile, "Move to end of file", "" },
	{ cmd_BackwardDelete, "Delete character left", "" },
	{ cmd_ForwardDelete, "Delete character right", "" },
//	{ cmd_DeleteToStartOfLine, "Delete to start of line", "" },
	{ cmd_DeleteToEndOfLine, "Delete to end of line", "" },
//	{ cmd_DeleteToStartOfFile, "Delete to start of file", "" },
	{ cmd_DeleteToEndOfFile, "Delete to end of file", "" },
	{ cmd_SelectPreviousCharacter, "Character select left", "" },
	{ cmd_SelectNextCharacter, "Character select right", "" },
	{ cmd_SelectPreviousWord, "Select previous word", "" },
	{ cmd_SelectNextWord, "Select next word", "" },
	{ cmd_SelectPreviousSubWord, "Select previous sub-word", "" },
	{ cmd_SelectNextSubWord, "Select next sub-word", "" },
	{ cmd_SelectPreviousLine, "Select previous line", "" },
	{ cmd_SelectNextLine, "Select next line", "" },
	{ cmd_SelectToStartOfLine, "Select to start of line", "" },
	{ cmd_SelectToEndOfLine, "Select to end of line", "" },
	{ cmd_SelectToTopOfPage, "Select to start of page", "" },
	{ cmd_SelectToBottomOfPage, "Select to end of page", "" },
	{ cmd_SelectToStartOfFile, "Select to start of file", "" },
	{ cmd_SelectToEndOfFile, "Select to end of file", "" },
	{ cmd_ScrollUpLine, "Scroll line up", "" },
	{ cmd_ScrollDownLine, "Scroll line down", "" },
	{ cmd_ScrollUpPage, "Scroll page up", "" },
	{ cmd_ScrollDownPage, "Scroll page down", "" },
	{ cmd_ScrollToTopOfFile, "Scroll to start of file", "" },
	{ cmd_ScrollToEndOfFile, "Scroll to end of file", "" },
};

static CommandInfo PrefixInfo[] = {
	{ 0, "Quote key", "" 	},
	{ 1, "Prefix Key 1", "" },
	{ 2, "Prefix Key 2", "" },
	{ 3, "Prefix Key 3", "" },
};


const int32		kFileCommandCount = sizeof(FileInfo) / sizeof(CommandInfo);
const int32		kEditCommandCount = sizeof(EditInfo) / sizeof(CommandInfo);
const int32		kSearchCommandCount = sizeof(SearchInfo) / sizeof(CommandInfo);
const int32		kProjectCommandCount = sizeof(ProjectInfo) / sizeof(CommandInfo);
const int32		kWindowCommandCount = sizeof(WindowInfo) / sizeof(CommandInfo);
const int32		kMiscCommandCount = sizeof(MiscInfo) / sizeof(CommandInfo);
const int32		kEditorCommandsCommandCount = sizeof(EditorCommandsInfo) / sizeof(CommandInfo);
const int32		kPrefixCommandCount = sizeof(PrefixInfo) / sizeof(CommandInfo);

static CommandGroupInfo CommandGroups[] = {
	{ "File Menu", kFileCommandCount, FileInfo },
	{ "Edit Menu", kEditCommandCount, EditInfo },
	{ "Search Menu", kSearchCommandCount, SearchInfo },
	{ "Project Menu", kProjectCommandCount, ProjectInfo },
	{ "Window Menu", kWindowCommandCount, WindowInfo },
	{ "Misc.", kMiscCommandCount, MiscInfo },
	{ "Editor Commands", kEditorCommandsCommandCount, EditorCommandsInfo },
	{ "Prefix Keys", kPrefixCommandCount, PrefixInfo },
};

const int32		kCommandGroupCount = sizeof(CommandGroups) / sizeof(CommandGroupInfo);

// ---------------------------------------------------------------------------
//		GetCommandInfo
// ---------------------------------------------------------------------------

bool
MIDECommandList::GetCommandInfo(
	CommandT		inCommand,
	CommandInfo&	outInfo)
{
	for (int i = 0; i < kCommandGroupCount; i++)
	{
		const CommandGroupInfo&		info = CommandGroups[i];

		for (int j = 0; j < info.count; j++)
		{
			if (inCommand == info.commands[j].command)
			{
				outInfo = info.commands[j];
				return true;				// early exit
			}
		}
	}
	
	return false;
}

// ---------------------------------------------------------------------------
//		GetNthCommandGroup
// ---------------------------------------------------------------------------

bool
MIDECommandList::GetNthCommandGroup(
	int32				inIndex,
	CommandGroupInfo&	outInfo)
{
	ASSERT(inIndex >= 0);

	if (inIndex < kCommandGroupCount)
	{
		outInfo = CommandGroups[inIndex];
		return true;
	}
	else
		return false;	
}

// ---------------------------------------------------------------------------
//		Count
// ---------------------------------------------------------------------------

int32
MIDECommandList::Count()
{
	return kCommandGroupCount;
}
