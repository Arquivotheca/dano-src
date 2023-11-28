/*	$Id: RTeamWindow.r,v 1.13 1999/05/03 13:12:05 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#include "RTypes.r"
#include "DMessages.h"

Resource 'MENU' (100, "File Menu") {
	"File",
	{
		Item		{ "About bdb...", kMsgAbout, none, noKey },
		Separator {},
		Item		{ "Quit", kMsgQuit, none, 'Q' }
	}
};


Resource 'MENU' (101, "Debug Menu") {
	"Debug",
	{
		Item		{ "Run", kMsgRun, none, 'R' },
		Item		{ "Kill", kMsgKill, none, 'K' },
		Separator {},
		Item		{ "Where Is", kMsgWhereIs, none, noKey },
		Separator {},
		Item		{ "Break on C++ Exception", kMsgBreakOnException, none, noKey },
		Item		{ "Break after load_add_on", kMsgBreakOnLoadAddon, none, noKey },
		Item		{ "Break on spawn_thread", kMsgBreakOnSpawnThread, none, noKey },
	}
};

Resource 'MENU' (102, "Window Menu")
{
	"Window",
	{
		Item		{ "Show Breakpoints", kMsgShowBreakpoints, none, noKey },
		Item		{ "Show Watchpoints", kMsgShowWatchpoints, none, noKey },
		Item		{ "Show Addresses…", kMsgShowAddressCmd, none, noKey },
		Item		{ "Show Function…", kMsgShowFunctionCmd, none, noKey },
		Item		{ "Set Watchpoint…", kMsgSetWatchpointCmd, none, noKey },
		Item		{ "Settings…", kMsgSettings, none, noKey },
		Separator	{}
	}
};


Resource 'MBAR' (100, "TeamWindowMenuBar" ) {
	{ 100, 4, 101, 2, 102 }
};


#include "RButtonBar.r"

Resource 'BtnB' (100, "ThreadWindowButtonBar")
{
	0,
	{
		201, kMsgRun, 0, "Run",
		200, kMsgDebugThread, 0, "Stop",
		204, kMsgKill, 0, "Kill",
		0, 0, space, "",
		2000, kMsgFuncPopup, menu | toggle, "Function PopUpMenu"
	}
};

Resource 'BtnB' (101, "StackCrawlWindowButtonBar")
{
	0,
	{
		2000, kMsgFuncPopup, menu | toggle, "Function PopUpMenu"
	}
};

// This is the Show Addresses… dialog:

resource 'DLOG' (128, "Show Addresses") {
	{ 0, 0, 300, 70 },
	"Show Addresses…",
	B_TITLED_WINDOW,
	NORMAL,
	{
		Edit			{{  10,  10, 290,  26 }, "addr", "Addresses:", "", "0123456789abcdefABCDEF, x", 0, 60 },
		Button			{{  230,  40, 290,  60 }, "ok", "OK", 'ok  ' },
		Button			{{  160,  40,  220,  60 }, "cancel", "Cancel", 'cncl' }
	}
};

Resource 'DLOG' (129, "Find" )
{
	{ 0, 0, 200, 100 },
	"Find",
	B_TITLED_WINDOW,
	NORMAL,
	{
		Edit			{{  10,  10, 190,  26 }, "find", "", "", "", 0, 0 },
		CheckBox		{{  10,  30, 190,  46 }, "icase", "Ignore Case" },
		CheckBox		{{  10,  50, 190,  66 }, "startAtTop", "Start at Top" },
		Button			{{ 130,  70, 190,  90 }, "ok", "Find", 'ok  ' },
		Button			{{  60,  70, 120,  90 }, "cancel", "Cancel", 'cncl' }
	}
};

Resource 'MENU' (90, "Font menu")
{
	"Font",
	{
	}
};

Resource 'DLOG' (130, "Settings")
{
	{ 0, 0, 300, 410 },
	"Settings",
	B_TITLED_WINDOW,
	NORMAL,
	{
		CheckBox		{{  10,  10, 290,  26 }, "use cache", "Reduce memory usage using a cache (is slower)" },
		Slider			{{  30,  30, 290,  80 }, "cache block count", "Cache size", 'chng', 10, 50, triangle },
		
		Line			{{  10,  84, 290,  85 }},
		
		Slider			{{  10,  90, 290, 150 }, "malloc debug", "Malloc Debug Level", 'chng', 0, 10, triangle },
		CheckBox		{{  10,  140, 290,  156 }, "leak check", "Enable leak checker" },
		
		Line			{{  10, 174, 290, 175 }},
		
		CheckBox		{{  10, 180, 290, 196 }, "stop at main", "Stop at main entrypoint" },
		CheckBox		{{  10, 200, 290, 216 }, "don't stop for unwinding", "Don't stop at unwinding stack when throwing" },
		CheckBox		{{  10, 220, 290, 236 }, "output to window", "Don't stop for debugger() messages" },
		
		Line			{{  10, 251, 290, 252 }},
		
		PopupMenu		{{  10, 257, 190, 273 }, "font family", "Font:", 90, 50 },
		Edit			{{ 200, 257, 290, 273 }, "font size", "Size:", "", "0123456789", 2, 40 },
		
		Line			{{  10, 282, 290, 283 }},
		
		Edit			{{  10, 288, 150, 304 }, "array upper bound", "Default array size:", "", "0123456789", 3, 100 },
		
		Line			{{  10, 315, 290, 316 }},
		
		RadioButton		{{ 10, 325, 290, 345 }, "at&t style disasm", "AT&T style instructions... movl 0x20(%ebx), %eax" },
		RadioButton		{{ 10, 345, 290, 355 }, "intel style disasm", "Intel style instructions... mov eax, [ebx+20h]" },
		
		Button			{{ 230, 380, 290, 400 }, "ok", "Save", 'ok  ' },
		Button			{{ 160, 380, 220, 400 }, "cancel", "Revert", 'cncl' }

	}
};

resource 'DLOG' (131, "Set Watchpoint") {
	{ 0, 0, 160, 70 },
	"Set Watchpoint…",
	B_TITLED_WINDOW,
	NORMAL,
	{
		Edit			{{  10,  10, 150,  26 }, "addr", "Address:", "", "0123456789abcdefABCDEFx", 0, 50 },
		Button			{{  90,  40, 150,  60 }, "ok", "OK", 'ok  ' },
		Button			{{  20,  40,  80,  60 }, "cancel", "Cancel", 'cncl' }
	}
};

resource 'DLOG' (132, "View As") {
	{ 0, 0, 200, 70 },
	"View As…",
	B_TITLED_WINDOW,
	NORMAL,
	{
		Edit			{{  10,  10, 190,  26 }, "cast", "Cast To:", "", "", 0, 50 },
		Button			{{ 130,  40, 190,  60 }, "ok", "OK", 'ok  ' },
		Button			{{  60,  40, 120,  60 }, "cancel", "Cancel", 'cncl' }
	}
};

resource 'DLOG' (133, "View at Address") {
	{ 0, 0, 160, 70 },
	"View at Address…",
	B_TITLED_WINDOW,
	NORMAL,
	{
		Edit			{{  10,  10, 150,  26 }, "addr", "Address:", "", "0123456789abcdefABCDEFx", 0, 50 },
		Button			{{  90,  40, 150,  60 }, "ok", "OK", 'ok  ' },
		Button			{{  20,  40,  80,  60 }, "cancel", "Cancel", 'cncl' }
	}
};

resource 'DLOG' (134, "Show Function") {
	{ 0, 0, 220, 70 },
	"Show Function…",
	B_TITLED_WINDOW,
	NORMAL,
	{
		Edit			{{  10,  10, 210,  26 }, "name", "Name:", "", "", 0, 50 },
		Button			{{ 150,  40, 210,  60 }, "ok", "OK", 'ok  ' },
		Button			{{  80,  40, 140,  60 }, "cancel", "Cancel", 'cncl' }
	}
};
