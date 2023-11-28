/*
 *	MWConstants.h
 *	Copyright © 1996 Metrowerks Inc. All rights reserved.
 *
 *	Constants for PowerPlant command numbers
 */

#ifndef	_MWCONSTANTS_H_
#define _MWCONSTANTS_H_

#include "IDEConstants.h"

// File
const CommandT cmd_NewProject			= 1000;
const CommandT cmd_NewEmptyProject		= 1005;
const CommandT cmd_OpenSelection		= 1001;
const CommandT cmd_OpenFile				= 1006;
#ifdef SCRIPTING
const CommandT cmd_NewScript				= 1007;
#endif
const CommandT cmd_SwitchToDebugger		= 1002;
const CommandT cmd_CloseAll				= 1003;
const CommandT cmd_SaveAll				= 1004;

//Edit
const CommandT cmd_Redo					= 1105;
const CommandT cmd_Balance				= 1100;
const CommandT cmd_ShiftLeft			= 1101;
const CommandT cmd_ShiftRight			= 1102;
const CommandT cmd_InsertRefTemplate	= 1103;
const CommandT cmd_Preferences			= 1104;

//Search
const CommandT cmd_Find					= 1200;
const CommandT cmd_FindNext				= 1201;
const CommandT cmd_FindPrevious			= 1210;
const CommandT cmd_FindInNextFile		= 1202;
const CommandT cmd_FindInPrevFile		= 1211;
const CommandT cmd_EnterFindString		= 1203;
const CommandT cmd_EnterReplaceString	= 1212;
const CommandT cmd_FindSelection		= 1204;
const CommandT cmd_FindPrevSelection	= 1213;
const CommandT cmd_Replace				= 1205;
const CommandT cmd_ReplaceAndFindNext	= 1206;
const CommandT cmd_ReplaceAndFindPrev	= 1214;
const CommandT cmd_ReplaceAll			= 1207;
const CommandT cmd_FindDefAndRef		= 1208;
const CommandT cmd_FindDefinition		= 1215;
const CommandT cmd_FindReference		= 1218;
const CommandT cmd_GoBack				= 1216;
const CommandT cmd_GoForward			= 1217;
const CommandT cmd_GotoLine				= 1209;
const CommandT cmd_CompareFiles			= 1219;
const CommandT cmd_KeepFile1Changes		= 1220;		// copy file 1 changes to file 2
const CommandT cmd_KeepFile2Changes		= 1221;		// copy file 2 changes to file 1
const CommandT cmd_FindDocumentation	= 1222;

//Window
const CommandT cmd_Stack				= 1300;
const CommandT cmd_Tile					= 1301;
const CommandT cmd_TileVertical			= 1302;
const CommandT cmd_ZoomWindow			= 1303;
const CommandT cmd_SaveDefaultWindow	= 20012;
const CommandT cmd_ToolServerWorksheet	= 1306;
const CommandT cmd_BuildProgressWindow	= 1307;
const CommandT cmd_ShowProjectWindow	= 2000;
const CommandT cmd_WindowListDivider	= 1399;		// Command given to divider before the window list

// Project Document
const CommandT cmd_CheckSyntax			= 1400;
const CommandT cmd_Preprocess			= 1401;
const CommandT cmd_Precompile			= 1402;
const CommandT cmd_Compile				= 1403;
const CommandT cmd_Disassemble			= 1404;
const CommandT cmd_BringUpToDate		= 1405;
const CommandT cmd_Make					= 1406;
const CommandT cmd_RemoveBinaries   	= 1407;
const CommandT cmd_RemoveBinariesCompact = 1408;
const CommandT cmd_ResetFilePaths		= 1409;
const CommandT cmd_SyncProjectCaches	= 1410;
const CommandT cmd_EnableDebugging		= 1411;
const CommandT cmd_Run					= 1412;
const CommandT cmd_RunOpposite			= 1413;
const CommandT cmd_SetDefaultProject	= 1414;
const CommandT cmd_SetCurrentTarget		= 1415;
const CommandT cmd_ErrorMessageWindow	= 1416;
const CommandT cmd_ProjectSettings		= 1417;
const CommandT cmd_TargetSettings		= 1418;
const CommandT cmd_ShowInspector		= 1419;

// +++ commands 1419-1449 reserved for Project Document

// Project Views
const CommandT cmd_AddWindow			= 1450;
const CommandT cmd_AddFiles				= 1451;
const CommandT cmd_CreateGroup			= 1452;
const CommandT cmd_RemoveFiles			= 1453;
const CommandT cmd_CreateOverlay		= 1454;
const CommandT cmd_RevealInFinder		= 1455;
const CommandT cmd_OpenCurrent			= 1456;
const CommandT cmd_SortGroup			= 1457;

// +++ commands 1454-1499 reserved for project views

// Tools
const CommandT cmd_ToggleWindowToolbar	= 1500;
const CommandT cmd_ToggleGlobalToolbar	= 1501;
const CommandT cmd_ResetWindowToolbar	= 1502;
const CommandT cmd_ResetGlobalToolbar	= 1503;
const CommandT cmd_ClearWindowToolbar	= 1504;
const CommandT cmd_ClearGlobalToolbar	= 1505;
const CommandT cmd_ShowToolbarElements	= 1506;
const CommandT cmd_ToggleToolbarAnchor	= 1507;

//ToolServer
const CommandT cmd_ToolServer			= 1600;
const CommandT cmd_Commando				= 1601;
const CommandT cmd_ExecuteScript		= 1602;
const CommandT cmd_LookupSymbol			= 1603;
const CommandT cmd_InsertTemplate		= 1604;
const CommandT cmd_ToolServerTools		= 1605;

//Script
const CommandT cmd_OpenScriptFolder		= 1700;

#ifdef SCRIPTING
const CommandT	cmd_ActiveScript_Run	= 1701;
const CommandT	cmd_ActiveScript_Stop	= 1702;
#endif

// +++ commands 2001-2499 reserved for window list
const CommandT cmd_FirstWindowListCmd	= 2000;
const CommandT cmd_LastWindowListCmd	= 2499;

// +++ commands 2501-2999 reserved for recent documents menu
const CommandT cmd_FirstRecentItemCmd	= 2500;
const CommandT cmd_LastRecentItemCmd	= 2999;

//Editor
const CommandT cmd_MoveToPreviousCharacter	= 3001;
const CommandT cmd_MoveToNextCharacter		= 3002;
const CommandT cmd_MoveToPreviousWord		= 3003;
const CommandT cmd_MoveToNextWord			= 3004;
const CommandT cmd_MoveToPreviousSubWord	= 3005;
const CommandT cmd_MoveToNextSubWord		= 3006;
const CommandT cmd_MoveToStartOfLine		= 3007;
const CommandT cmd_MoveToEndOfLine			= 3008;
const CommandT cmd_MoveToPreviousLine		= 3009;
const CommandT cmd_MoveToNextLine			= 3010;
const CommandT cmd_MoveToTopOfPage			= 3011;
const CommandT cmd_MoveToBottomOfPage		= 3012;
const CommandT cmd_MoveToTopOfFile			= 3013;
const CommandT cmd_MoveToBottomOfFile		= 3014;
const CommandT cmd_BackwardDelete			= 3015;
const CommandT cmd_ForwardDelete			= 3016;
const CommandT cmd_DeleteToStartOfLine		= 3017;
const CommandT cmd_DeleteToEndOfLine		= 3018;
const CommandT cmd_DeleteToStartOfFile		= 3019;
const CommandT cmd_DeleteToEndOfFile		= 3020;
const CommandT cmd_SelectPreviousCharacter	= 3021;
const CommandT cmd_SelectNextCharacter		= 3022;
const CommandT cmd_SelectPreviousWord		= 3023;
const CommandT cmd_SelectNextWord			= 3024;
const CommandT cmd_SelectPreviousSubWord	= 3025;
const CommandT cmd_SelectNextSubWord		= 3026;
const CommandT cmd_SelectPreviousLine		= 3027;
const CommandT cmd_SelectNextLine			= 3028;
const CommandT cmd_SelectToStartOfLine		= 3029;
const CommandT cmd_SelectToEndOfLine		= 3030;
const CommandT cmd_SelectToTopOfPage		= 3031;
const CommandT cmd_SelectToBottomOfPage		= 3032;
const CommandT cmd_SelectToStartOfFile		= 3033;
const CommandT cmd_SelectToEndOfFile		= 3034;
const CommandT cmd_ScrollUpLine				= 3035;
const CommandT cmd_ScrollDownLine			= 3036;
const CommandT cmd_ScrollUpPage				= 3037;
const CommandT cmd_ScrollDownPage			= 3038;
const CommandT cmd_ScrollToTopOfFile		= 3039;
const CommandT cmd_ScrollToEndOfFile		= 3040;

// misc. Commands
const CommandT cmd_AndyFeature				= 4000;
const CommandT cmd_PrevMessage				= 4003;
const CommandT cmd_NextMessage				= 4004;


#ifndef TRUE
#define TRUE 					true
#define FALSE 					false
#endif

// === AppleEvent Numbers ===
#define ae_AddFiles				10001
#define ae_CheckSyntax			10002
#define ae_CloseProject			10003
#define ae_CloseWindow			10004
#define ae_Compile				10005
#define ae_CreateProject		10006
#define ae_Disassemble			10035
#define ae_GetDefinition		10007
#define ae_GetOpenDocuments		10008
#define ae_GetPreferences		10009
#define ae_GetProjectFile		10010
#define ae_GetProjectSpecifier	10011
#define ae_GetSegments			10012
#define ae_GotoFunction			10013
#define ae_GotoLine				10014
#define ae_IsInProject			10015
#define ae_MakeProject			10016
#define ae_Precompile			10017
#define ae_Preprocess			10034
#define ae_RemoveBinaries		10018
#define ae_RemoveFiles			10019
#define ae_ResetFilePaths		10020
#define ae_RunProject			10021
#define ae_SaveMessageWindow	10022
#define ae_SetModificationDate	10023
#define ae_SetPreferences		10024
#define ae_SetProjectFile		10025
#define ae_SetSegment			10026
#define ae_Touch				10027
#define ae_UpdateProject		10028
#define ae_FindText				10029
#define ae_ReplaceText			10030
#define ae_ReplaceFindText		10031
#define ae_ReplaceAllText		10032
#define ae_SetFindOptions		10033
// next event # is 10036

#define ae_GetAETE				10100

#define	ae_SystemConfigNotice	2014

#endif
