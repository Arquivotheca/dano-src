// ===========================================================================
//	PP_Messages.h			   ©1993-1996 Metrowerks Inc. All rights reserved.
// ===========================================================================

#ifndef _PPMESSAGES_H
#define _PPMESSAGES_H

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
	#pragma import on
#endif

//#include <PP_Types.h>
//#include "PP_Types.h"

	// Messages are 32-bit numbers used as parameters to a few PowerPlant
	// functions that you typically override:
	//		LCommander::ObeyCommand
	//		LListener::ListenerToMessage
	//		LAttachment::ExecuteSelf
	
	// These function each take a Message and a void* parameter called
	// "ioParam". For each Message defined below, the adjacent comment
	// specifies the data passed via ioParam.
	
	// If a message is sent as a result of a menu selection (either with
	// the mouse or keyboard equivalent), the ioParam is an Int32*, where
	// the hi Int16 is the MENU ID, and the lo Int16 is the menu item
	// number (value returned by MenuSelect/MenuKey).

											// ioParam Data
											
const MessageT	cmd_Nothing			= 0;	// nil
const MessageT	msg_Nothing			= 0;	// nil

const MessageT	cmd_About			= 1;	// nil

					// File Menu
const MessageT	cmd_New				= 2;	// nil
const MessageT	cmd_Open			= 3;	// nil
const MessageT	cmd_Close			= 4;	// nil
const MessageT	cmd_Save			= 5;	// nil
const MessageT	cmd_SaveAs			= 6;	// nil
const MessageT	cmd_Revert			= 7;	// nil
const MessageT	cmd_PageSetup		= 8;	// nil
const MessageT	cmd_Print			= 9;	// nil
const MessageT	cmd_PrintOne		= 17;	// nil
const MessageT	cmd_Quit			= 10;	// nil

					// Edit Menu
const MessageT	cmd_Undo			= 11;	// nil
const MessageT	cmd_Cut				= 12;	// nil
const MessageT	cmd_Copy			= 13;	// nil
const MessageT	cmd_Paste			= 14;	// nil
const MessageT	cmd_Clear			= 15;	// nil
const MessageT	cmd_SelectAll		= 16;	// nil
const MessageT	cmd_ClearSelection	= 27;	// nil

const MessageT	cmd_SaveCopyAs		= 18;	// nil
const MessageT	cmd_ShowClipboard	= 19;	// nil

					// Undo/Redo Editing Actions
const CommandT	cmd_ActionDeleted	= 20;	// LAction*
const CommandT	cmd_ActionDone		= 21;	// LAction*
const CommandT	cmd_ActionCut		= 22;	// LTECutAction*
const CommandT	cmd_ActionCopy		= 23;	// nil [not used]
const CommandT	cmd_ActionPaste		= 24;	// LTEPasteAction*
const CommandT	cmd_ActionClear		= 25;	// LTEClearAction*
const CommandT	cmd_ActionTyping	= 26;	// LTETypingAction*

const MessageT	msg_TabSelect		= 201;	// nil
const MessageT	msg_BroadcasterDied	= 202;	// LBroadcaster*
const MessageT	msg_ControlClicked	= 203;	// LControl*
const MessageT	msg_ThumbDragged	= 204;	// LStdControl*

					// Use these three command numbers to disable the menu
					// item when you use the font-related menus as
					// hierarchical menus.
const MessageT	cmd_FontMenu		= 250;	// nil
const MessageT	cmd_SizeMenu		= 251;	// nil
const MessageT	cmd_StyleMenu		= 252;	// nil

					// Size menu commands
const MessageT	cmd_FontLarger		= 301;	// nil
const MessageT	cmd_FontSmaller		= 302;	// nil
const MessageT	cmd_FontOther		= 303;	// nil

					// Style menu commands
const MessageT	cmd_Plain			= 401;	// nil
const MessageT	cmd_Bold			= 402;	// nil
const MessageT	cmd_Italic			= 403;	// nil
const MessageT	cmd_Underline		= 404;	// nil
const MessageT	cmd_Outline			= 405;	// nil
const MessageT	cmd_Shadow			= 406;	// nil
const MessageT	cmd_Condense		= 407;	// nil
const MessageT	cmd_Extend			= 408;	// nil

					// Text justification
const MessageT	cmd_JustifyDefault	= 411;	// nil
const MessageT	cmd_JustifyLeft		= 412;	// nil
const MessageT	cmd_JustifyCenter	= 413;	// nil
const MessageT	cmd_JustifyRight	= 414;	// nil
const MessageT	cmd_JustifyFull		= 415;	// nil

					// Mail menu commands
const MessageT	cmd_AddMailer		= 501;	// nil
const MessageT	cmd_ExpandMailer	= 502;	// nil
const MessageT	cmd_SendLetter		= 503;	// nil
const MessageT	cmd_Reply			= 504;	// nil
const MessageT	cmd_Forward			= 505;	// nil
const MessageT	cmd_TagLetter		= 506;	// nil
const MessageT	cmd_OpenNextLetter	= 507;	// nil
const MessageT	cmd_Sign			= 508;	// nil
const MessageT	cmd_Verify			= 509;	// nil

					// Miscellaneous Messages
const MessageT	msg_GrowZone		= 801;	// Int32* (in: bytes needed, out: bytes freed)
const MessageT	msg_EventHandlerNote= 802;	// SEventHandlerNote*

					// Attachment Messages
const MessageT	msg_Event			= 810;	// EventRecord*
const MessageT	msg_DrawOrPrint		= 811;	// Rect* (frame of Pane)
const MessageT	msg_Click			= 812;	// SMouseDownEvent*
const MessageT	msg_AdjustCursor	= 813;	// EventRecord*
const MessageT	msg_KeyPress		= 814;	// EventRecord* (KeyDown or AutoKey event)
const MessageT	msg_CommandStatus	= 815;	// SCommandStatus*
const MessageT	msg_PostAction		= 816;	// LAction*

const MessageT	msg_OK				= 900;	// nil
const MessageT	msg_Cancel			= 901;	// nil

const CommandT	cmd_UseMenuItem		= -1;	// --- (special flag, message is never sent)
const MessageT	msg_AnyMessage		= -2;	// --- (special flag, message is never sent)

#if defined(__CFM68K__) && !defined(__USING_STATIC_LIBS__)
	#pragma import reset
#endif

#endif
