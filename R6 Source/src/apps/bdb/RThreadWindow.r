/*	$Id: RThreadWindow.r,v 1.13 1999/05/03 13:12:05 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#include "RTypes.r"
#include "DMessages.h"

Resource 'MENU' (0, "File Menu")
{
	"File",
	{
		Item		{ "Open In Editor", kMsgFileOpenInPreferredEditor, control, 'E' },
		Item		{ "Save State…", kMsgSaveState, none, noKey },
		Separator	{},
		Item		{ "About bdb...", kMsgAbout, none, noKey },
		Separator	{},
		Item		{ "Quit", kMsgQuit, none, 'Q' }
	}
};

Resource 'MENU' (1, "Debug Menu")
{
	"Debug",
	{
		Item		{ "Run", kMsgRun, none, 'R' },
		Item		{ "Step", kMsgStep, none, 'S' },
		Item		{ "Step Over", kMsgStepOver, none, 'T' },
		Item		{ "Step Out", kMsgStepOut, none, 'U' },
		Item		{ "Kill", kMsgKill, none, 'K' },
	}
};

Resource 'MENU' (2, "Data")
{
	"Data",
	{
		Item		{ "View Memory", kMsgDumpMemory, none, 'M' },
		Separator	{},
		Item		{ "Default", kMsgFormatDefault, none, noKey },
		Item		{ "Signed Decimal", kMsgFormatSigned, shift, 'D'},
		Item		{ "Unsigned Decimal", kMsgFormatUnsigned, shift, 'U'},
		Item		{ "Hexadecimal", kMsgFormatHex, shift, 'H'},
		item		{ "Octal", kMsgFormatOctal, shift, 'O'},
		Item		{ "Character", kMsgFormatChar, shift, 'C'},
		Item		{ "C String", kMsgFormatString, shift, 'S'},
		Item		{ "Float", kMsgFormatFloat, shift, 'F'},
		Item		{ "Enumeration", kMsgFormatEnum, shift, 'E'},
		Item		{ "View As…", kMsgViewAsCmd, none, noKey },
		Item		{ "View As Array", kMsgViewAsArray, none, noKey },
		Separator	{},
		Item		{ "Add Watchpoint", kMsgAddWatchpoint, none, noKey },
		Item		{ "Add Expression", kMsgAddExpression, none, noKey },
		Separator	{},
		Item		{ "DumpMessage", kMsgDumpAddOn, none, noKey },
	}
};

Resource 'MENU' (3, "Window")
{
	"Window",
	{
		Item		{ "Show Registers", kMsgShowRegisters, none, noKey },
		Item		{ "Show Breakpoints", kMsgShowBreakpoints, none, noKey },
		Item		{ "Show Watchpoints", kMsgShowWatchpoints, none, noKey },
		Item		{ "Show Assembly", kMsgShowAssemblyCmd, none, noKey },
		Item		{ "Show Debugger Message", kMsgShowDebuggerMsg, none, noKey },
		Item		{ "Set Watchpoint…", kMsgSetWatchpointCmd, none, noKey },
		Separator	{},
		Item		{ "Settings…", kMsgSettings, none, noKey },
	}
};

Resource 'MENU' (4, "Edit Menu")
{
	"Edit",
	{
		Item		{ "Find…", kMsgFind, none, 'F' },
		Item		{ "Find Again", kMsgFindAgain, none, 'G' },
		Item		{ "Find Selection", kMsgFindSelection, none, 'H' },
		Item		{ "Enter Search String", kMsgEnterSearchString, none, 'E' }
	}
};

Resource 'MBAR' (1, "ThreadWindowMenuBar" )
{
	{
		0, 4, 1, 2, 3
	}
};

#include "RButtonBar.r"

Resource 'BtnB' (0, "ThreadWindowButtonBar")
{
	0,
	{
		201, kMsgRun, 0, "Run",
		200, kMsgStop, 0, "Stop",
		204, kMsgKill, 0, "Kill",
		0, 0, space, "",
		205, kMsgStepOver, 0, "Step Over",
		202, kMsgStep, 0, "Step",
		203, kMsgStepOut, 0, "Step Out",
		0, 0, space, "",
		2000, kMsgFuncPopup, menu | toggle, "Function PopUpMenu"
	}
};

