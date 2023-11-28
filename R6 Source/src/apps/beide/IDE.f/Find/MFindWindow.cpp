//========================================================================
//	MFindWindow.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	The Find window.
//	The underlying find code has been taken from the Mac IDE (CW7) and
//	resides in two other files.  It would probably be best if the RegExp
//	code was encapsulated in a class since it has several global variables.
//	BDS

#include <string.h>
#include <ctype.h>
#include <Beep.h>

#include "MFindWindow.h"
#include "MTextWindow.h"
#include "MIDETextView.h"
#include "MTextView.h"
#include "MMultiFileListView.h"
#include "MFocusBox.h"
#include "MProjectWindow.h"
#include "MSectionLine.h"
#include "MSourceFile.h"
#include "MPictureMenuBar.h"
#include "MSaveFileSetWindow.h"
#include "MRemoveFileSetWindow.h"
#include "MPreferences.h"
#include "MKeyFilter.h"
#include "MLocker.h"
#include "IDEApp.h"
#include "IDEMessages.h"
#include "ProjectCommands.h"
#include "MainMenus.h"
#include "MAlert.h"

#define _ 0xff
#define B 0
#define w 63
#define g 63
#define h 26
#define i 16

const float kButtonBitmapHeight = 19.0;
const float kButtonBitmapWidth = 26.0;
const float kArrowBitmapHeight = 15.0;
const float kArrowBitmapWidth = 26.0;

const char sSingleIcon[] = {
	_,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,_,
	B,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,B,
	B,g,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,B,B,B,B,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,B,B,h,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,B,i,B,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,B,B,B,B,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,w,w,w,B,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,w,w,w,B,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,w,w,w,B,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,w,w,w,B,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,w,w,w,B,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,w,w,w,w,w,w,B,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,B,B,B,B,B,B,B,B,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,B,
	_,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,_
};

const char sDoubleIcon[] = {
	_,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,_,
	B,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,g,B,
	B,g,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,B,B,B,B,B,h,h,h,h,h,B,B,B,B,B,h,h,h,h,h,i,B,
	B,g,h,h,B,w,w,w,B,B,h,h,h,h,B,w,w,w,B,B,h,h,h,h,i,B,
	B,g,h,h,B,w,w,w,B,i,B,h,h,h,B,w,w,w,B,i,B,h,h,h,i,B,
	B,g,h,h,B,w,w,w,B,B,B,B,h,h,B,w,w,w,B,B,B,B,h,h,i,B,
	B,g,h,h,B,w,w,w,w,w,w,B,h,h,B,w,w,w,w,w,w,B,h,h,i,B,
	B,g,h,h,B,w,w,w,w,w,w,B,h,h,B,w,w,w,w,w,w,B,h,h,i,B,
	B,g,h,h,B,w,w,w,w,w,w,B,h,h,B,w,w,w,w,w,w,B,h,h,i,B,
	B,g,h,h,B,w,w,w,w,w,w,B,h,h,B,w,w,w,w,w,w,B,h,h,i,B,
	B,g,h,h,B,w,w,w,w,w,w,B,h,h,B,w,w,w,w,w,w,B,h,h,i,B,
	B,g,h,h,B,w,w,w,w,w,w,B,h,h,B,w,w,w,w,w,w,B,h,h,i,B,
	B,g,h,h,B,B,B,B,B,B,B,B,h,h,B,B,B,B,B,B,B,B,h,h,i,B,
	B,g,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,h,i,B,
	B,g,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i,B,
	_,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,_
};

const float kTriangleBitMapWidth = 8.0;
const float kTriangleBitMapHeight = 12.0;

#undef _
#define _ 29	/*background needs to be the same as kLtGrey*/
#define X 0x26
#define x 0x6c

static char sContractedData[] = {
	_,_,_,_,_,_,_,_,
	_,_,X,_,_,_,_,_,
	_,_,X,X,_,_,_,_,
	_,_,X,x,X,_,_,_,
	_,_,X,x,x,X,_,_,
	_,_,X,x,x,X,_,_,
	_,_,X,x,X,_,_,_,
	_,_,X,X,_,_,_,_,
	_,_,X,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
};

static char sExpandedData[] = {
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	X,X,X,X,X,X,X,X,
	_,X,x,x,x,x,X,_,
	_,_,X,x,x,X,_,_,
	_,_,_,X,X,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
	_,_,_,_,_,_,_,_,
};

#undef X
#undef x

#undef _
#undef B
#undef w
#undef g
#undef h
#undef i

const int32 kLeastRecentStringID = 14;
const int32 kMostRecentStringID = 0;
const int32 kFileSetsMenuRemoveItem = 1;
const int32 kFileSetsMenuListStart = 3;
const bool kBeepIfNotFound = true;
const int32 kSaveIndex = 0;
const int32 kRemoveIndex = 1;

//BRect sFindWindowFrame(100, 25, 470, 160);
//BRect sFindWindowFrame(100, 25, 500, 270);
BRect sFindWindowFrame(100, 25, 510, 310);

const float kExpandedHeight = 285.0;
const float kContractedHeight = 135.0;

MFindWindow*		MFindWindow::sFindWindow;

// ---------------------------------------------------------------------------
//	Regular expression help
// ---------------------------------------------------------------------------

class CannedRegularExpression {
public:
	const char* fTitle;
	const char* fExpression;
};

CannedRegularExpression kFindExpressions[] =
{
	{	"Any character", 			"."				},
	{	"Zero or more", 			"*"				},
	{	"One or more", 				"+"				},
	{	"Group or tag (abc)",		"(abc)"			},
	{	"Set of characters [abc]",	"[abc]"			},
	{	"Not in set [^abc]",		"[^abc]"		},
	{	"Range [a-z]",				"[a-z]"			},
	{	"Beginning of line",		"^"				},
	{	"End of line",				"$"				},
	{	"Quote special character",	"\\"			},
	{	"",							NULL			},
	{	"Decimal number",			"[0-9]+"		},
	{	"Hexidecimal number",		"0x[0-9a-fA-F]+"},
	{	"Identifier", 				"[A-Za-z_]+"	},
	{	"C++ comment",				"//.*"			},
	{	"C comment",				"/\\*.*\\*/"	},
	{	NULL,						NULL			}
};

CannedRegularExpression kReplaceExpressions[] =
{
	{	"Contents of Find string", 	"&"		},
	{	"Contents of Find tag", 	"\\1"	},
	{	NULL,						NULL	}
};

// ---------------------------------------------------------------------------
//		MFindWindow
// ---------------------------------------------------------------------------
//	Constructor

MFindWindow::MFindWindow()
	: BWindow(
	 sFindWindowFrame, 
	 "Find",
	B_TITLED_WINDOW,
	B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fSourcesList(),
	fSysHeadersList(),
	fProjectHeadersList(),
	fOtherList(),
	fAllList(),
	fDataLock("finddatalock")
{
	sFindWindow = this;
	fWindowIsBuilt = false;
	fTextWindow = nil;
	fTextWindowHasSelection = false;
	fFileSetWindow = nil;
	fRemoveSetWindow = nil;
	fHasSources = false;
	fHasSystemHeaders = false;
	fHasProjectHeaders = false;
	fSelectedProjectIndex = -1;

	// make sure we always have an fProjectPopup menu around
	// so that we can add projects whenever we want 
	fProjectPopup = new BPopUpMenu("No Project", false, false);

	GetPrefs();

	PostMessage(msgBuildWindow);	// defer building the window
	Run();
}

// ---------------------------------------------------------------------------
//		~MFindWindow
// ---------------------------------------------------------------------------
//	Destructor

MFindWindow::~MFindWindow()
{
	fFinder.CancelMultiFindThread();

	SetPrefs();

	EmptyList(fSourcesList);
	EmptyList(fSysHeadersList);
	EmptyList(fProjectHeadersList);
	fListView->RemoveRows(0, fListView->CountRows());
}

// ---------------------------------------------------------------------------
//		GetPrefs
// ---------------------------------------------------------------------------
//	Should verify length and version number here.

void
MFindWindow::GetPrefs()
{
	size_t			length;
	BRect			frame;
	FindDataT		data;

	length = sizeof(data);
	if (B_NO_ERROR == MPreferences::GetPreference(kFindPrefs, kPrefsType, &data, length))
	{
		fData.fBatch = data.sBatch;
		fData.fWrap = data.sWrap;
		fData.fIgnoreCase = data.sIgnoreCase;
		fData.fEntireWord = data.sEntireWord;
		fData.fRegexp = data.sRegexp;
		fData.fStopAtEOF = data.sStopAtEOF;
		fMultiVisible = data.sMultiVisible;
	}
	else
	{
		// Defaults
		fData.fBatch = false;
		fData.fWrap = true;
		fData.fIgnoreCase = true;
		fData.fEntireWord = false;
		fData.fRegexp = false;
		fData.fStopAtEOF = false;	
		fMultiVisible = false;
	}

	fData.fResetMulti = false;
	fData.fMultiFile = false;
	fMultiFinding = false;

	length = sizeof(BRect);
	if (B_NO_ERROR == MPreferences::GetPreference(kFindFrame, B_RECT_TYPE, &frame, length))
	{
		SwapRectToBig(frame);
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
		ValidateWindowFrame(frame, *this);
	}
}

// ---------------------------------------------------------------------------
//		SetPrefs
// ---------------------------------------------------------------------------

void
MFindWindow::SetPrefs()
{
	size_t				length;
	BRect				frame = Frame();
	FindDataT			data;

	data.sBatch = fData.fBatch;
	data.sWrap = fData.fWrap;
	data.sIgnoreCase = fData.fIgnoreCase;
	data.sEntireWord = fData.fEntireWord;
	data.sRegexp = fData.fRegexp;
	data.sStopAtEOF = fData.fStopAtEOF;
	data.sMultiVisible = Bounds().Height() > kContractedHeight;

	length = sizeof(data);
	(void) MPreferences::SetPreference(kFindPrefs, kPrefsType, &data, length);
	length = sizeof(frame);
	SwapRectToHost(frame);
	(void) MPreferences::SetPreference(kFindFrame, B_RECT_TYPE, &frame, length);
}

// ---------------------------------------------------------------------------
//		RemovePreferences
// ---------------------------------------------------------------------------

void
MFindWindow::RemovePreferences()
{
	(void) MPreferences::RemovePreference(kFindPrefs, kPrefsType);
	(void) MPreferences::RemovePreference(kFindFrame, B_RECT_TYPE);
	MFileSetKeeper::RemovePreferences();
}

// ---------------------------------------------------------------------------
//		GetFindWindow
// ---------------------------------------------------------------------------

MFindWindow&
MFindWindow::GetFindWindow()
{
	while (! sFindWindow->fWindowIsBuilt)
		snooze(100000);

	return *sFindWindow;
}

// ---------------------------------------------------------------------------
//		CanFind
// ---------------------------------------------------------------------------
//	We can't respond to direct calls from text windows during a multifile 
//	find.

inline bool
MFindWindow::CanFind()
{
	return fFinder.CanFind();
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MFindWindow::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		// Text window
		case msgSetFindString:
			SetFindString(*inMessage);
			break;

		case msgFindSelection:
			ASSERT(false);
			FindSelection(*inMessage);
			break;

		case cmd_FindInNextFile:
		case cmd_FindInPrevFile:
			if (CanFind())
			{
				fData.fForward = inMessage->what == cmd_FindInNextFile;
				SetUpFind();
				fFinder.DoFindInNextFile();
			}
			break;

		// Buttons
		// These buttons are the equivalent of the menu commands in
		// text windows
		case msgFind:
			if (fFindButton->IsEnabled())		// cmd-F is a shortcut for this button
			{
				fData.fForward = true;

				if (fData.fMultiFile)
					DoMultiFileFind();
				else
				if (fData.fBatch)
					DoBatchFind();
				else
				{
					LockData();
					ASSERT(fTextWindow);
					if (fTextWindow)
						fTextWindow->PostMessage(cmd_FindNext);
					UnlockData();
				}

				UpdateRecentStrings(fFindRecentStrings);
			}
			break;

		case msgFindFromTextWindow:
		{
			fFindStringBox->SelectAll();
			fFindStringBox->MakeFocus();
			// Move to the current workspace
			BRect	frame = Frame();
			SetWorkspaces(B_CURRENT_WORKSPACE);
			if (frame != Frame())
				MoveTo(frame.left, frame.top);
			if (IsHidden())
				Show();
			Activate();
			ValidateWindowFrame(*this);		// make sure that we're onscreen
			break;
		}
		
		case msgReplace:
			LockData();
			ASSERT(fTextWindow);
			if (fTextWindow)
				fTextWindow->PostMessage(cmd_Replace);
			UpdateRecentStrings(fReplaceRecentStrings);
			UnlockData();
			break;

		case msgReplaceAndFind:
			LockData();
			ASSERT(fTextWindow);
			if (fTextWindow)
				fTextWindow->PostMessage(cmd_ReplaceAndFindNext);
			UpdateRecentStrings(fFindRecentStrings);
			UpdateRecentStrings(fReplaceRecentStrings);
			UnlockData();
			break;

		case msgReplaceAll:
			if (fData.fMultiFile)
			{
				fData.fReplaceAll = true;
				fData.fForward = true;
				DoMultiFileFind();
			}
			else
			{
				LockData();
				ASSERT(fTextWindow);
				if (fTextWindow)
					fTextWindow->PostMessage(cmd_ReplaceAll);
				UnlockData();
			}
			UpdateRecentStrings(fFindRecentStrings);
			UpdateRecentStrings(fReplaceRecentStrings);
			break;

		// Text Boxes
		case msgTextChanged:
			UpdateButtons();
			ResetMultiFind();
			break;

		// CheckBoxes
		case msgCheckBoxChanged:
			CheckBoxChanged();
			break;

		case msgSources:
		case msgSystemHeaders:
		case msgProjectHeaders:
			ModifyFileList(inMessage->what);
			break;

		case msgOther:
			RemoveFromAllList(fOtherList);
			EmptyList(fOtherList);
			UpdateListBox();
			fOtherCB->SetValue(B_CONTROL_OFF);
			fOtherCB->SetEnabled(false);
			break;

		case msgMultiChanged:
			SetMultiFileFind(B_CONTROL_ON == fMultFindButton->Value());
			break;

		// Popup Menus
		case msgRecentFindString:
		case msgRecentReplaceString:
			StringChanged(*inMessage);
			break;

		case msgFindRegExp:
		case msgReplaceRegExp:
			this->InsertRegularExpression(*inMessage);
			break;
			
		case msgSaveFileSet:
			this->DoFileSet(*inMessage);
			break;

		case msgRemoveFileSet:
			this->RemoveFileSet(*inMessage);
			break;

		case msgFileSetChosen:
			this->SwitchFileSet(*inMessage);
			break;

		case msgSaveFileSetWindowClosed:
			fFileSetWindow = nil;
			break;

		case msgRemoveFileSetWindowClosed:
			fRemoveSetWindow = nil;
			break;
			
		// File List
		case msgBlueRowChanged:
			fFinder.BlueRowChanged();
			fData.fResetMulti = false;
			break;

		// Open Panel
		case msgAddOtherFile:
			AddToOtherList(*inMessage);
			break;

		// WindowList
		case msgProjectClosed:
			if (inMessage->HasPointer(kProjectMID)) {
				MProjectWindow* proj;
				if (B_NO_ERROR == inMessage->FindPointer(kProjectMID, (void **) &proj))
					this->ProjectClosed(proj);
			}
			break;

		case msgProjectOpened:
			if (inMessage->HasPointer(kProjectMID)) {
				MProjectWindow*		proj;
				if (B_NO_ERROR == inMessage->FindPointer(kProjectMID, (void **) &proj))
					this->ProjectOpened(proj);
			}
			break;

		case msgTextWindowClosed:
			TextWindowClosed();
			break;

		case msgProjectSelected:
			this->HandleProjectSelected(*inMessage);
			break;

		// Special
		case cmd_Cancel:
			fFinder.CancelMultiFindThread();
			break;

		case msgTriangleChanged:
			ToggleWindowSize();
			break;

		case msgUpdateButtons:
			UpdateButtons();
			break;

		case msgBuildWindow:
			BuildWindow();
			SetPulseRate(kSlowPulseRate);	// off
			break;

		// Hilite color changed
		case msgColorsChanged:
			fListView->HiliteColorChanged();
			break;

		default:
			if (! SpecialMessageReceived(*inMessage, this))
				BWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		SetMultiFileFind
// ---------------------------------------------------------------------------
//	Set the enabled state of all the controls based on whether we have
//	multifile find or not.

void
MFindWindow::SetMultiFileFind(
	bool	inMulti)
{
	if (inMulti)
	{
		fData.fMultiFile = true;
	}
	else
	{
		fData.fMultiFile = false;
		if (CurrentFocus() == fListView)
			fFindStringBox->MakeFocus();
		this->CloseFileSetWindows();
	}
	
	if (inMulti != (B_CONTROL_ON == fMultFindButton->Value()))
		fMultFindButton->SetValue(inMulti ? 1 : 0);

	bool haveProject = fProjectList.CountItems() > 0;
	if (! fMultiFinding)
	{
		fSourcesCB->SetEnabled(inMulti && haveProject);
		fSystemHeadersCB->SetEnabled(inMulti && haveProject);
		fProjectHeadersCB->SetEnabled(inMulti && haveProject);
		fOtherCB->SetEnabled(inMulti && fOtherList.CountItems() > 0);
		fOtherButton->SetEnabled(inMulti);
		fStopAtEOFCB->SetEnabled(inMulti);
		fListView->SetEnabled(inMulti);
		fFileSetsPopup->SetEnabled(inMulti);
		fProjectPopup->SetEnabled(inMulti);
		
		UpdateButtons();
	}
}

// ---------------------------------------------------------------------------
//		SetFindString
// ---------------------------------------------------------------------------
//	A text window has sent us a setfindstring message in response to cmd-E.
//	Or in response to cmd-H.

void
MFindWindow::SetFindString(
	BMessage&	inMessage)
{
	if (inMessage.HasData(kAsciiText, B_ASCII_TYPE))
	{
		bool			findIt = true;	// Find it or replace it?
		const char *	text;
		ssize_t			textLen;

		if (B_NO_ERROR == inMessage.FindData(kAsciiText, B_ASCII_TYPE, (const void**) &text, &textLen))
		{
			if (inMessage.HasBool(kFindIt))
				findIt = inMessage.FindBool(kFindIt);
			
			SetFindString(text, textLen, findIt);
		}
	}
}

// ---------------------------------------------------------------------------
//		SetFindString
// ---------------------------------------------------------------------------
//	A text window has sent us a setfindstring message in response to cmd-E.
//	Or in response to cmd-H.

void
MFindWindow::SetFindString(
	const char*	inText,
	int32		inLength,
	bool		inFindIt)		// false means replace it
{
	MTextView*		box;

	if (inFindIt)
	{
		fFindString.Set(inText, inLength);
		box = fFindStringBox;
	}
	else
	{
		fReplaceString.Set(inText, inLength);
		box = fReplaceStringBox;
	}

	box->SetText(inText, inLength);
	box->Invalidate();

	SetMultiFileFind(false);
	ResetMultiFind();
	if (fData.fBatch)
	{
		fData.fBatch = false;
		fBatchCB->SetValue(B_CONTROL_OFF);
	}
}

// ---------------------------------------------------------------------------
//		FindSelection
// ---------------------------------------------------------------------------
//	A text window has sent us a setfindstring message in response to cmd-E.
//	Or in response to cmd-H.

void
MFindWindow::FindSelection(
	BMessage&	inMessage)
{
	MIDETextView*		textView;

	if (B_NO_ERROR == inMessage.FindPointer(kTextView, (void**) &textView))
	{
		bool				forward = true;
		bool				findIt = true;
		forward = inMessage.FindBool(kForward);

		if (! forward)
			inMessage.ReplaceBool(kForward, true);

		SetFindString(inMessage);

		if (textView->Window()->Lock())
		{
			DoFindNext(*textView, forward);
			textView->Window()->Unlock();
		}
	}
}

// ---------------------------------------------------------------------------
//		BuildWindow
// ---------------------------------------------------------------------------
//	Build the window.
//	It takes ten minutes to compile this function with optimization level 4
//	and the code generated is no different than at level 1.

#ifdef __MWERKS__
#if __option(global_optimizer)
#pragma optimization_level 1
#endif
#endif

void
MFindWindow::BuildWindow()
{
	BRect				r;
	BMessage*			msg;
	BButton*			button;
	BStringView*		caption;
	BStringView*		findcaption;
	BStringView*		replacecaption;
	BScrollView*		box;
	BCheckBox*			checkBox;
	MFocusBox*			focusBox;
	BScrollView*		frame;
	MMultiFileListView*	list;
	BView*				topView;
	BPopUpMenu*			popup;

	float				top;
	float				left;
	const float			cbLeft = 270.0;
	const float			butLeft = 310.0;
	const float			mfTop = 135.0;
	const float 		kBlueViewWidth = 12.0;

	// Build a special topview so we can have a grey background for
	// the window
	r = Bounds();
	topView = new BView(r, "ourtopview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(topView);
	SetGrey(topView, kLtGray);

	// Static text
	// Find:
	r.Set(10, 10, 50, 26);
	findcaption = new BStringView(r, "st1", "Find:"); 
	topView->AddChild(findcaption);
	findcaption->SetFont(be_bold_font);
	SetGrey(findcaption, kLtGray);

	// Replace:
	r.Set(10, 45, 60, 61);
	replacecaption = new BStringView(r, "st2", "Replace:"); 
	topView->AddChild(replacecaption);
	replacecaption->SetFont(be_bold_font);
	SetGrey(replacecaption, kLtGray);

	// Multi-File Search
	r.Set(10, mfTop, 110, mfTop + 16);
	caption = new BStringView(r, "st3", "Multi-File Search:"); 
	topView->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);

	// TextBoxes
	// FindStringBox
	
	int32 findBoxTop = 10;
	int32 replaceBoxTop = 45;
	
	r.Set(70, findBoxTop, 260, findBoxTop + 30);
	fFindStringBox = new MTextView(r, "FindString"); 
	fFindStringBox->SetTarget(this);

	box = new BScrollView("box1", fFindStringBox);			// For the border
	topView->AddChild(box);

	fFindStringBox->MakeFocus();
	fFindStringBox->SetText("");
	fFindStringBox->SetMaxBytes(255);
	fFindStringBox->SetWordWrap(true);
	fFindStringBox->SetTabWidth(fFindStringBox->StringWidth("M"));
	DisallowInvalidChars(*fFindStringBox);

	// ReplaceStringBox
	r.Set(70, replaceBoxTop, 260, replaceBoxTop + 30);
	fReplaceStringBox = new MTextView(r, "ReplaceString"); 
	fReplaceStringBox->SetTarget(this);

	box = new BScrollView("box2", fReplaceStringBox);		// For the border
	topView->AddChild(box);

	fReplaceStringBox->SetText("");
	fReplaceStringBox->SetMaxBytes(255);
	fReplaceStringBox->SetWordWrap(true);
	fReplaceStringBox->SetTabWidth(fReplaceStringBox->StringWidth("M"));
	DisallowInvalidChars(*fReplaceStringBox);

	// CheckBoxes
	// Batch
	r.Set(70, 85, 120, 100);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "Batch", "Batch", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this);
	checkBox->SetFont(be_plain_font);
	SetGrey(checkBox, kLtGray);
	if (fData.fBatch)
		checkBox->SetValue(1);
	fBatchCB = checkBox;

	// Wrap
	r.Set(70, 105, 120, 120);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "Wrap", "Wrap", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this);
	checkBox->SetFont(be_plain_font);
	SetGrey(checkBox, kLtGray);
	if (fData.fWrap)
		checkBox->SetValue(1);
	fWrapCB = checkBox;

	// Ignore Case
	r.Set(130, 85, 210, 100);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "IgnoreCase", "Ignore Case", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this);
	checkBox->SetFont(be_plain_font);
	SetGrey(checkBox, kLtGray);
	if (fData.fIgnoreCase)
		checkBox->SetValue(1);
	fIgnoreCaseCB = checkBox;

	// Entire Word
	r.Set(130, 105, 210, 120);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "EntireWord", "Entire Word", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this);
	checkBox->SetFont(be_plain_font);
	SetGrey(checkBox, kLtGray);
	if (fData.fEntireWord)
		checkBox->SetValue(1);
	fEntireWordCB = checkBox;

	// Regexp
	r.Set(215, 85, 275, 100);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "Regexp", "Regexp", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this);
	checkBox->SetFont(be_plain_font);
	SetGrey(checkBox, kLtGray);
	if (fData.fRegexp)
		checkBox->SetValue(1);
	fRegexpCB = checkBox;

	// Buttons
	// Find
	r.Set(butLeft, 13, butLeft + 80.0, 33);
	msg = new BMessage(msgFind);
	button = new BButton(r, "Find", "Find", msg); 
	topView->AddChild(button);
	button->SetTarget(this);
	button->SetFont(be_plain_font);
	SetDefaultButton(button);
	fFindButton = button;

	// Replace
	r.Set(butLeft, 45, butLeft + 80.0, 65);
	msg = new BMessage(msgReplace);
	button = new BButton(r, "Replace", "Replace", msg); 
	topView->AddChild(button);
	button->SetFont(be_plain_font);
	button->SetTarget(this);
	fReplaceButton = button;

	// ReplaceAndFind
	r.Set(butLeft, 75, butLeft + 80.0, 95);
	msg = new BMessage(msgReplaceAndFind);
	button = new BButton(r, "ReplaceAndFind", "Replace & Find", msg); 
	topView->AddChild(button);
	button->SetFont(be_plain_font);
	button->SetTarget(this);
	fReplaceAndFindButton = button;

	// ReplaceAll
	r.Set(butLeft, 105, butLeft + 80.0, 125);
	msg = new BMessage(msgReplaceAll);
	button = new BButton(r, "ReplaceAll", "Replace All", msg); 
	topView->AddChild(button);
	button->SetFont(be_plain_font);
	button->SetTarget(this);
	fReplaceAllButton = button;


	top = 175.0;
	
	// Sources Checkbox
	r.Set(cbLeft, top, cbLeft + 100.0, top + 15);
	msg = new BMessage(msgSources);
	checkBox = new BCheckBox(r, "sources", "Sources", msg); 
	topView->AddChild(checkBox);
	checkBox->SetFont(be_plain_font);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fSourcesCB = checkBox;
	top += 20.0;

	// System headers
	r.Set(cbLeft, top, cbLeft + 100.0, top + 15);
	msg = new BMessage(msgSystemHeaders);
	checkBox = new BCheckBox(r, "systemHed", "System Headers", msg); 
	topView->AddChild(checkBox);
	checkBox->SetFont(be_plain_font);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fSystemHeadersCB = checkBox;
	top += 20.0;

	// Project headers
	r.Set(cbLeft, top, cbLeft + 100.0, top + 15);
	msg = new BMessage(msgProjectHeaders);
	checkBox = new BCheckBox(r, "projectHed", "Project Headers", msg); 
	topView->AddChild(checkBox);
	checkBox->SetFont(be_plain_font);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fProjectHeadersCB = checkBox;
	top += 20.0;

	// Other Checkbox
	r.Set(cbLeft, top, cbLeft + 15.0, top + 15);
	msg = new BMessage(msgOther);
	checkBox = new BCheckBox(r, "other", "", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	checkBox->SetEnabled(false);
	fOtherCB = checkBox;

	top -= 3;
	// Others button
	r.Set(cbLeft + 22, top, cbLeft + 22 + 80.0, top + 20.0);
	msg = new BMessage(msgStartOther);
	button = new BButton(r, "others", "Others...", msg); 
	topView->AddChild(button);
	button->SetTarget(be_app);
	button->SetFont(be_plain_font);
	fOtherButton = button;
	top += 28.0;

	// Stop at EOF checkbox
	r.Set(cbLeft, top, cbLeft + 110.0, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "eof", "Stop at End of File", msg); 
	topView->AddChild(checkBox);
	checkBox->SetTarget(this);
	checkBox->SetFont(be_plain_font);
	SetGrey(checkBox, kLtGray);
	if (fData.fStopAtEOF)
		checkBox->SetValue(1);
	fStopAtEOFCB = checkBox;

	top = 175.0;

	// Multifile list box
	r.Set(25, top, 260, top+85);
	focusBox = new MFocusBox(r);						// Focus Box
	topView->AddChild(focusBox);
	focusBox->SetHighColor(kFocusBoxGray);

	r.InsetBy(10.0, 3.0);
	r.OffsetTo(3.0, 3.0);
	list = new MMultiFileListView(r, "list", B_FOLLOW_NONE, B_PULSE_NEEDED | B_WILL_DRAW | B_NAVIGABLE);
	list->SetFocusBox(focusBox);
	list->AddFilter(new FindWindowDropFilter(*this));

	frame = new BScrollView("frame", list, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_PLAIN_BORDER);	// For the border
	focusBox->AddChild(frame);
	list->SetFont(be_plain_font);
	font_height		info = list->GetCachedFontHeight();
	list->SetDefaultRowHeight(info.ascent + info.descent + info.leading + 1.0);
	list->SetTarget(this);
	fListView = list;

	// Multifind picture toggle button
	top = 95;
	left = 25;
	r.Set(left, top, left + kButtonBitmapWidth, top + kButtonBitmapHeight);
	r.OffsetTo(B_ORIGIN);
	BBitmap*	bitmap1 = LoadBitmap(sSingleIcon, kButtonBitmapWidth, kButtonBitmapHeight);
	BBitmap*	bitmap2 = LoadBitmap(sDoubleIcon, kButtonBitmapWidth, kButtonBitmapHeight);

	fFindStringBox->BeginPicture(new BPicture);
	fFindStringBox->DrawBitmap(bitmap1, B_ORIGIN);
	BPicture*	picture1 = fFindStringBox->EndPicture();
	fFindStringBox->BeginPicture(new BPicture);
	fFindStringBox->DrawBitmap(bitmap2, B_ORIGIN);
	BPicture*	picture2 = fFindStringBox->EndPicture();
	fFindStringBox->Invalidate();
	delete bitmap1;
	delete bitmap2;
	
	msg = new BMessage(msgMultiChanged);
	r.Set(left, top, left + kButtonBitmapWidth, top + kButtonBitmapHeight);
	fMultFindButton = new BPictureButton(r, "multibutt", picture1, picture2, msg, B_TWO_STATE_BUTTON);
	topView->AddChild(fMultFindButton);

	// Triangle picture toggle button
	left = 5;
	top += 5;
	r.Set(left, top, left + kTriangleBitMapWidth, top + kTriangleBitMapHeight);
	r.OffsetTo(B_ORIGIN);
	
	bitmap1 = LoadBitmap(sContractedData, kTriangleBitMapWidth, kTriangleBitMapHeight);
	bitmap2 = LoadBitmap(sExpandedData, kTriangleBitMapWidth, kTriangleBitMapHeight);
	topView->BeginPicture(new BPicture);
	SetGrey(topView, kLtGray);
	topView->DrawBitmap(bitmap1, B_ORIGIN);
	picture1 = topView->EndPicture();
	topView->BeginPicture(new BPicture);
	SetGrey(topView, kLtGray);
	topView->DrawBitmap(bitmap2, B_ORIGIN);
	picture2 = topView->EndPicture();
	delete bitmap1;
	delete bitmap2;

	msg = new BMessage(msgTriangleChanged);
	r.Set(left, top, left + kTriangleBitMapWidth, top + kTriangleBitMapHeight);
	fTriangleButton = new BPictureButton(r, "triangle", picture1, picture2, msg, B_TWO_STATE_BUTTON);
	topView->AddChild(fTriangleButton);
	SetGrey(fTriangleButton, kLtGray);
	if (fMultiVisible)
		fTriangleButton->SetValue(B_CONTROL_ON);

	// Popups
	// Find String recent strings
	// note... if we use "*" in the name, it still doesn't show
	// (I don't know why), but then it doesn't match the separator
	// so the popup doesn't shift to select the separator as the
	// "current item".
	popup = new BPopUpMenu("*", false, false);
	fFindRecentStrings = new BMenu("Recent Strings");
	popup->AddItem(fFindRecentStrings);
	fFindRecentStrings->SetTargetForItems(this);

	BMenu* regExpressions = new BMenu("Regular Expressions");
	this->BuildExpressionHelper(*regExpressions, true);
	popup->AddItem(regExpressions);

	r.Set(cbLeft, findBoxTop, cbLeft + 20, findBoxTop + 20);
	BMenuField* popupLabel = new BMenuField(r, "", "", popup,
							 B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	topView->AddChild(popupLabel);
	popupLabel->SetDivider(0.0);
	SetGrey(popupLabel, kLtGray);
	
	// Replace String recent strings
	popup = new BPopUpMenu("*", false, false);
	fReplaceRecentStrings = new BMenu("Recent Strings");
	popup->AddItem(fReplaceRecentStrings);
	fReplaceRecentStrings->SetTargetForItems(this);

	regExpressions = new BMenu("Regular Expressions");
	this->BuildExpressionHelper(*regExpressions, false);
	popup->AddItem(regExpressions);

	r.Set(cbLeft, replaceBoxTop, cbLeft + 20, replaceBoxTop + 20);
	popupLabel = new BMenuField(r, "", "", popup,
					 B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	topView->AddChild(popupLabel);
	popupLabel->SetDivider(0.0);
	SetGrey(popupLabel, kLtGray);
	
	// File Sets Popup
	float mfPopups = mfTop+15.0;
	top = mfPopups;
	popup = new BPopUpMenu("*", false, false);
	fFileSetsPopup = popup;

	msg = new BMessage(msgSaveFileSet);
	popup->AddItem(new BMenuItem("Save this File Set...", msg));
	msg = new BMessage(msgRemoveFileSet);
	popup->AddItem(new BMenuItem("Remove a File Set", msg));
	popup->AddSeparatorItem();
	popup->SetTargetForItems(this);

	// File Sets MenuLabel
	float fsLeft = 25.0;
	r.Set(fsLeft, mfPopups, fsLeft + 80, mfPopups + 16);
	popupLabel = new BMenuField(r, "st4", "File Sets:", popup,
					 B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	topView->AddChild(popupLabel);
	popupLabel->SetDivider(55.0);
	SetGrey(popupLabel, kLtGray);
	fFileSetsMenu = popupLabel;

	// Project Popup (constructed in MFindWindow::MFindWindow)
	top = mfPopups;
	fProjectPopup->SetTargetForItems(this);
	fProjectPopup->SetLabelFromMarked(true);
	
	// Project MenuLabel
	float pLeft = cbLeft - 55.0;
	r.Set(pLeft, mfPopups, pLeft + 175, mfPopups + 16);
	fProjectMenu = new BMenuField(r, "st5", "Project:", fProjectPopup,
					 B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	topView->AddChild(fProjectMenu);
	fProjectMenu->SetDivider(55.0);
	SetGrey(fProjectMenu, kLtGray);
	
	// General stuff...
	
	SetKeyMenuBar(nil);		// We don't have a key menu bar in this window

	fListView->MakeFocus();
	fFindStringBox->MakeFocus();

	// Command F activates the Find button if it's enabled
	msg = new BMessage(msgFind);
	AddShortcut('F', 0L, msg);

	AddCommonFilter(new MTextKeyFilter(this, kBindingGlobal));

	SetMultiFileFind(false);

	fFinder.SetFileList(&fAllList);
	fFinder.SetListView(fListView);
	this->UpdateFileSetMenu();

	ToggleWindowSize();

	fWindowIsBuilt = true;
}
#ifdef __MWERKS__
#if __option(global_optimizer)
#pragma optimization_level 4
#endif
#endif

// ---------------------------------------------------------------------------

void
MFindWindow::BuildExpressionHelper(BMenu& inMenu, bool isFind)
{
	// build up the menu of regular expression helpers
	
	CannedRegularExpression* expressions = NULL;
	uint32 whatMessage = 0;
	
	if (isFind) {
		whatMessage = msgFindRegExp;
		expressions = kFindExpressions;
	}
	else {
		whatMessage = msgReplaceRegExp;
		expressions = kReplaceExpressions;
	}
	
	for (int i = 0; expressions[i].fTitle != NULL; i++) {
		if (expressions[i].fExpression == NULL) {
			inMenu.AddSeparatorItem();
		}
		else {
			BMessage* msg = new BMessage(whatMessage);
			BMenuItem* item = new BMenuItem(expressions[i].fTitle, msg);
			inMenu.AddItem(item);
		}
	}
}

// ---------------------------------------------------------------------------

void
MFindWindow::InsertRegularExpression(BMessage& inMessage)
{
	// The user asked to insert a regular expression
	// put it into either the find or replace box
	// also make sure regular expressions are turned on
	
	MTextView* textBox;
	int32 index = -1;
	(void) inMessage.FindInt32("index", &index);
	CannedRegularExpression* expressions = NULL;
	
	if (inMessage.what == msgFindRegExp) {
		textBox = fFindStringBox;
		expressions = kFindExpressions;
	}
	else {
		textBox = fReplaceStringBox;
		expressions = kReplaceExpressions;
	}
	
	if (index >=0) {
		// replace the selection -if any-
		// in the find (or replace) window
		textBox->Delete();
		textBox->Insert(expressions[index].fExpression);
		textBox->Invalidate();
				
		// make sure the regular expression box is checked if they chose
		// a canned regular expression
		fRegexpCB->SetValue(1);
		this->CheckBoxChanged();
	}
}

// ---------------------------------------------------------------------------
//		DispatchMessage
// ---------------------------------------------------------------------------
//	This is a hack to trap option-returns and option-tab.  Without it 
//	option-return is treated as a return and is passed to the default button.
//	I can't figure out exactly where option-tab goes but it doesn't otherwise
//	go to the current focus view.

void
MFindWindow::DispatchMessage(
	BMessage 	*message, 
	BHandler 	*receiver)
{
	switch(message->what)
	{
		case B_KEY_DOWN:
			const char *		bytes;
			uint32				modifiers;

			if (B_OK == message->FindInt32("modifiers", (int32*) &modifiers) && 
				B_OK == message->FindString("bytes", &bytes))
			{
				switch (bytes[0])
				{
					case B_RETURN:
					case B_TAB:
						if ((modifiers & (B_CONTROL_KEY | B_OPTION_KEY)) != 0)
						{
							BView*		currentFocus = CurrentFocus();
							
							if (currentFocus)
							{
								currentFocus->KeyDown(bytes, 1);
							}
							
							message->what = 0;
						}
						break;
	
					// Handle the backspace key in the list view
					case B_BACKSPACE:
						if (CurrentFocus() == fListView)
						{
							DeleteSelectionInListBox();
							AdjustOtherCheckBox();
						}
						break;
				}
			}
			break;
	}
	
	// BWindow ignores the cmd-arrowkeys but if we don't send it the message
	// it hilites the file menu when the cmd key comes up
	BWindow::DispatchMessage(message, receiver);
}

// ---------------------------------------------------------------------------
//		WindowActivated
// ---------------------------------------------------------------------------
//	We need to adjust the activation state of the buttons depending on
//	whether there is a selection in the frontmost text window and whether
//	we have something to find.

void
MFindWindow::WindowActivated(
	bool inActive)
{
	BWindow::WindowActivated(inActive);
	if (inActive)
	{
		TextWindowClosed(false);

		// Fix up the cursor if needed
		BPoint 	where;
		uint32	buttons;
		fFindStringBox->GetMouse(&where, &buttons);
		if (! fFindStringBox->Bounds().Contains(where))
		{
			fFindStringBox->ConvertToScreen(&where);
			fReplaceStringBox->ConvertFromScreen(&where);
			if (! fReplaceStringBox->Bounds().Contains(where))
				be_app->SetCursor(B_HAND_CURSOR);
		}
	}
	else
	{
		LockData();
		fTextWindow = nil;
		UnlockData();
	}
	
	UpdateButtons();
}

// ---------------------------------------------------------------------------
//		TextWindowClosed
// ---------------------------------------------------------------------------
//	We need to adjust the activation state of the buttons depending on
//	whether there is a selection in the frontmost text window and whether
//	we have something to find. note default parameter.

void
MFindWindow::TextWindowClosed(
	bool	inPostAMessage)
{
	MTextWindow*	textWindow = MDynamicMenuHandler::GetFrontTextWindow();
	bool			textWindowHasSelection = false;
	
	// Get the selection when the find window is activated
	// this prevents having to lock the text window at other
	// times which can cause a deadlock.  Getting the selection
	// here and using it later is dependent on the assumption
	// that the selection in a text window cannot change while
	// the Find window is in front.
	if (textWindow && textWindow->Lock())
	{
		textWindowHasSelection = textWindow->HasSelection();
		textWindow->Unlock();
	}
		
	sFindWindow->LockData();
	sFindWindow->fTextWindow = textWindow;
	sFindWindow->fTextWindowHasSelection = textWindowHasSelection;
	sFindWindow->UnlockData();

	// Post a message so we don't have to lock the Find window
	// while in the text window's thread
	if (inPostAMessage)
		sFindWindow->PostMessage(msgUpdateButtons);
}

// ---------------------------------------------------------------------------
//		CheckBoxChanged
// ---------------------------------------------------------------------------
//	One of the checkboxes changed.

void
MFindWindow::CheckBoxChanged()
{
	fData.fBatch = fBatchCB->Value() == B_CONTROL_ON;
	fData.fWrap = fWrapCB->Value() == B_CONTROL_ON;
	fData.fIgnoreCase = fIgnoreCaseCB->Value() == B_CONTROL_ON;
	fData.fEntireWord = fEntireWordCB->Value() == B_CONTROL_ON;
	fData.fRegexp = fRegexpCB->Value() == B_CONTROL_ON;
	fData.fStopAtEOF = fStopAtEOFCB->Value() == B_CONTROL_ON;

	UpdateButtons();
	ResetMultiFind();
}

// ---------------------------------------------------------------------------
//		UpdateButtons
// ---------------------------------------------------------------------------
//	Update the state of the buttons when the find text changes or when 
//	we're activated.

void
MFindWindow::UpdateButtons()
{
	LockData();

	if (fTextWindow || fData.fMultiFile)
	{
		bool		hasFindString = HasFindString();
		bool		hasSelection = (fTextWindow != nil) && fTextWindowHasSelection;

		fFindButton->SetEnabled(hasFindString);
		fReplaceButton->SetEnabled(hasSelection);
		fReplaceAndFindButton->SetEnabled(hasFindString && hasSelection && !fData.fBatch);
		fReplaceAllButton->SetEnabled(hasFindString && !fData.fBatch);
	}
	else
	{
		// No text window is open
		fFindButton->SetEnabled(false);
		fReplaceButton->SetEnabled(false);
		fReplaceAndFindButton->SetEnabled(false);
		fReplaceAllButton->SetEnabled(false);		
	}
	
	UnlockData();
}

// ---------------------------------------------------------------------------
//		QuitRequested
// ---------------------------------------------------------------------------
//	 Hide ourselves when we're closed.

bool
MFindWindow::QuitRequested()
{
	fFinder.CancelMultiFindThread();

	if (IDEApp::BeAPP().IsQuitting())
	{
		return true;
	}
	else
	{
		CloseFileSetWindows();
		if (! IsHidden()) {
			Hide();
			be_app_messenger.SendMessage(msgFindWindowHidden);
		}
		return false;
	}
}

// ---------------------------------------------------------------------------
//		Show
// ---------------------------------------------------------------------------

void
MFindWindow::Show()
{
	SetPulseRate(kNormalPulseRate);
	BWindow::Show();
}

// ---------------------------------------------------------------------------
//		Hide
// ---------------------------------------------------------------------------

void
MFindWindow::Hide()
{
	SetPulseRate(kSlowPulseRate);	// off
	BWindow::Hide();
}

// ---------------------------------------------------------------------------
//		HasFindString
// ---------------------------------------------------------------------------

bool
MFindWindow::HasFindString()
{
	bool	hasIt = fFindStringBox->TextLength() > 0;
	
	return hasIt;
}

// ---------------------------------------------------------------------------
//		CanFindInNextFile
// ---------------------------------------------------------------------------

bool
MFindWindow::CanFindInNextFile()
{
	LockData();
	bool	canDoIt = fData.fMultiFile && ! fData.fBatch && fFindStringBox->TextLength() > 0;
	UnlockData();
	
	return canDoIt;
}

// ---------------------------------------------------------------------------
//		CanReplaceAll
// ---------------------------------------------------------------------------

bool
MFindWindow::CanReplaceAll()
{
	LockData();
	bool	canDoIt = ! fData.fBatch && fFindStringBox->TextLength() > 0;
	UnlockData();
	
	return canDoIt;
}

// ---------------------------------------------------------------------------
//		DoFindNext
// ---------------------------------------------------------------------------
//	Called from a text window.

bool
MFindWindow::DoFindNext(
	MIDETextView& 	inTextView,
	bool			inForward)
{
	bool	found = true;

	if (CanFind())
	{
		MLocker<BLooper>	lock(this);

		fData.fForward = inForward;

		if (fData.fMultiFile)
			DoMultiFileFind();
		else
		{
			SetUpFind();
			found = fFinder.DoFindNext(inTextView, true);
		}

		UpdateRecentStrings(fFindRecentStrings);
	}

	return found;
}

// ---------------------------------------------------------------------------
//		DoFindSelection
// ---------------------------------------------------------------------------
//	Called from a text window.

bool
MFindWindow::DoFindSelection(
	MIDETextView& 	inTextView,
	const char *	inText,
	int32			inLength,
	bool			inForward)
{
	bool	found = true;

	if (CanFind())
	{
		MLocker<BLooper>	lock(this);
		SetFindString(inText, inLength, true);
	
		found = DoFindNext(inTextView, inForward);
	}
	
	return found;
}

// ---------------------------------------------------------------------------
//		DoReplace
// ---------------------------------------------------------------------------

bool
MFindWindow::DoReplace(
	MIDETextView& 	inTextView)
{
	bool	found = false;

	if (CanFind())
	{
		MLocker<BLooper>	lock(this);
		SetUpFind();
		found = fFinder.DoReplace(inTextView);
		UpdateRecentStrings(fReplaceRecentStrings);
	}
		

	return found;
}

// ---------------------------------------------------------------------------
//		DoReplaceAndFind
// ---------------------------------------------------------------------------

void
MFindWindow::DoReplaceAndFind(
	MIDETextView& 	inTextView,
	bool			inForward)
{
	if (CanFind())
	{
		MLocker<BLooper>	lock(this);

		fData.fForward = inForward;
		SetUpFind();
		fFinder.DoReplaceAndFind(inTextView);
		UpdateRecentStrings(fReplaceRecentStrings);
	}
}

// ---------------------------------------------------------------------------
//		DoReplaceAll
// ---------------------------------------------------------------------------

void
MFindWindow::DoReplaceAll(
	MIDETextView& 	inTextView)
{
	if (CanFind())
	{
		MLocker<BLooper>	lock(this);

		fData.fForward = true;
		SetUpFind();
		fFinder.DoReplaceAll(inTextView, kBeepIfNotFound);
		UpdateRecentStrings(fReplaceRecentStrings);
	}
}

// ---------------------------------------------------------------------------
//		DoBatchFind
// ---------------------------------------------------------------------------
//	Do a batch find for a single file.

void
MFindWindow::DoBatchFind()
{
	LockData();
	ASSERT(fTextWindow);

	if (fTextWindow && fTextWindow->Lock())
	{
		if (CanFind())
		{
			fData.fForward = true;
			SetUpFind();
			fFinder.DoBatchFind(*fTextWindow);
		}

		fTextWindow->Unlock();
	}
	
	UnlockData();
}

// ---------------------------------------------------------------------------
//		DoMultiFileFind
// ---------------------------------------------------------------------------

void
MFindWindow::DoMultiFileFind()
{
	if (CanFind())
	{
		SetUpFind();
		fFinder.DoMultiFileFind();
		fData.fResetMulti = false;
		PseudoLock();
		fMultiFinding = true;
	}
}

// ---------------------------------------------------------------------------
//		MultiFileFindDone
// ---------------------------------------------------------------------------

void
MFindWindow::MultiFileFindDone(
	bool		inFound)
{
	Lock();

	if ((fData.fBatch || ! inFound) && ! fData.fOnlyOne)
		ResetMultiFind();

	fData.fReplaceAll = false;
	fData.fResetMulti = false;	// may not be right if the user has made a change during the find
	fMultiFinding = false;

	PseudoUnlock();

	Unlock();
}

// ---------------------------------------------------------------------------
//		SetUpFind
// ---------------------------------------------------------------------------
//	Pass our find settings to the finder object before doing a find.
//	This allows the user to change settings in the window while a multifind
//	is in progress without affecting the find.

void
MFindWindow::SetUpFind()
{
	strcpy(fData.FindString, fFindStringBox->Text());
	strcpy(fData.ReplaceString, fReplaceStringBox->Text());
	fFinder.SetFindData(fData);
}

// ---------------------------------------------------------------------------
//		ResetMultiFind
// ---------------------------------------------------------------------------
//	Various actions cause the file list to jump back to the first file.

void
MFindWindow::ResetMultiFind()
{
	fData.fResetMulti = true;
	fData.fReplaceAll = false;
	fListView->SetBlueRow(0);
	fFinder.BlueRowChanged();
}

// ---------------------------------------------------------------------------
//		PseudoLock
// ---------------------------------------------------------------------------
//	Certain actions can't be performed during a find so we disable the
//	controls to prevent them from being done.

void
MFindWindow::PseudoLock()
{
	fFindButton->SetEnabled(false);
	fReplaceButton->SetEnabled(false);
	fReplaceAndFindButton->SetEnabled(false);
	fReplaceAllButton->SetEnabled(false);		

	fSourcesCB->SetEnabled(false);
	fSystemHeadersCB->SetEnabled(false);
	fProjectHeadersCB->SetEnabled(false);
	fOtherCB->SetEnabled(false);
	fOtherButton->SetEnabled(false);
	fStopAtEOFCB->SetEnabled(false);
	fListView->SetEnabled(false);
	fFileSetsPopup->SetEnabled(false);
	fProjectPopup->SetEnabled(false);
}

// ---------------------------------------------------------------------------
//		PseudoUnlock
// ---------------------------------------------------------------------------

void
MFindWindow::PseudoUnlock()
{
	UpdateButtons();
	SetMultiFileFind(fData.fMultiFile);
}

// ---------------------------------------------------------------------------
//		Filter
// ---------------------------------------------------------------------------
//	Add a dropped file or folder to the other list.

filter_result		
FindWindowDropFilter::Filter(
	BMessage *inMessage, 
	BHandler **/*target*/)
{
	fFindWindow.AddToOtherList(*inMessage);
	
	return B_SKIP_MESSAGE;
}

// ---------------------------------------------------------------------------
// FileListAdder
// Local helper class to add files recursively to a list
// (see MFindWindow::AddFolderToOtherList)
// ---------------------------------------------------------------------------

class FileListAdder : public MDirIteratorFunc
{
public:
						FileListAdder(MSourceFileList& list);
	virtual bool		ProcessItem(BEntry& inEntry,
									node_flavor inFlavor,
									const BDirectory& inDir);

	int32				GetCount() { return fCount; }
	
private:
	MSourceFileList&	fAddList;
	int32				fCount;
};

// ---------------------------------------------------------------------------

FileListAdder::FileListAdder(MSourceFileList& list)
			  : fAddList(list),
			    fCount(0)
{
}

// ---------------------------------------------------------------------------

bool
FileListAdder::ProcessItem(BEntry& inEntry,
						   node_flavor inFlavor,
						   const BDirectory& /*inDir*/)
{
	// if we have a directory, continue the iteration
	// if we have a file - add it to the given find list

	bool more = true;	
	if (inFlavor == B_DIRECTORY_NODE) {
		BDirectory newDir(&inEntry);
		IterateDirectory(newDir, *this);
	}
	else {
		uint32 type = MimeType(inEntry);			
		// Check if it's a text file.  Fix the file type if we have to.
		if (type == kNULLType) {
			::FixFileType(&inEntry);
			type = MimeType(inEntry);
		}
		if (type == kTextType) {
			MSourceFile* sourceFile = new MSourceFile(inEntry, false, MSourceFile::kSourceFileKind, nil);
			fAddList.AddItem(sourceFile);
			fCount++;
			// TODO: Add thread + progress bar + cancel
			// Here would be a good place for a progress bar with cancel...
			// Until then stop when we get over 5000 files.
			if (fCount % 5000 == 0) {
				String text = "Sorry, recursive folder additions are limited to 5000 files.  Please add addtional folders manually.  Stopped at ";
				BPath path(&inEntry);
				text += path.Path();
				text += ".";
				MAlert alert(text);
				alert.Go();
				throw false;
			}
		}
	}	
	return more;
}

// ---------------------------------------------------------------------------

bool
MFindWindow::AddFolderToOtherList(entry_ref& inRef)
{	
	BDirectory dir(&inRef);
	FileListAdder adderFunction(fOtherList);
	
	// allow the iterator to throw an exception when cancelling
	try {
		IterateDirectory(dir, adderFunction);
	}
	catch (...) {
	}
	return adderFunction.GetCount() > 0;
}

// ---------------------------------------------------------------------------
//		AddToOtherList
// ---------------------------------------------------------------------------

bool
MFindWindow::AddToOtherList(BMessage& inMessage)
{
	bool result = false;
	type_code messageType;
	int32 count;	
	if (B_NO_ERROR == inMessage.GetInfo("refs", &messageType, &count)) {
		RemoveFromAllList(fOtherList);
		entry_ref ref;
		BEntry entry;

		for (int32 i = 0; i < count; i++) {
			if (B_NO_ERROR == inMessage.FindRef("refs", i, &ref)) {
				entry.SetTo(&ref);
				
				// go ahead and traverse one level of symlinks
				// (we don't traverse them in AddFolderToOtherList)
				if (entry.IsSymLink()) {
					entry_ref ref;
					entry.GetRef(&ref);
					entry.SetTo(&ref, true);
				}
				
				// Now look at the file or directory
				if (entry.IsFile()) {
					MSourceFile* sourceFile = new MSourceFile(entry, false, MSourceFile::kSourceFileKind, nil);					
					fOtherList.AddItem(sourceFile);
					result = true;
				}
				else if (entry.IsDirectory()) {
					result = this->AddFolderToOtherList(ref);
				}
			}
		}

		AddToAllList(fOtherList);
	}
	
	if (result) {
		UpdateListBox();
		AdjustOtherCheckBox();
	}

	return result;
}

// ---------------------------------------------------------------------------
//		AdjustOtherCheckBox
// ---------------------------------------------------------------------------

void
MFindWindow::AdjustOtherCheckBox()
{
	bool		shouldBeOn = fOtherList.CountItems() > 0;
	
	fOtherCB->SetValue(shouldBeOn ? 1 : 0 );
	fOtherCB->SetEnabled(shouldBeOn);
}

// ---------------------------------------------------------------------------
//		ModifyFileList
// ---------------------------------------------------------------------------
//	One of the multifind checkboxes was clicked.  Add or remove the appropriate
//	files from the allfilelist and from the view.

void
MFindWindow::ModifyFileList(
	uint32	inKind)
{
	BCheckBox*			checkBox = nil;
	MSourceFileList*	list;
	SourceListT			sourceKind;
	int32				newValue;

	switch (inKind)
	{
		case msgSources:
			newValue = fSourcesCB->Value();
			if (newValue != fHasSources)
			{
				checkBox = fSourcesCB;
				list = &fSourcesList;
				sourceKind = kSourceFiles;
				fHasSources = newValue;
			}
			break;

		case msgSystemHeaders:
			newValue = fSystemHeadersCB->Value();
			if (newValue != fHasSystemHeaders)
			{
				checkBox = fSystemHeadersCB;
				list = &fSysHeadersList;
				sourceKind = kSystemHeaderFiles;
				fHasSystemHeaders = newValue;
			}
			break;

		case msgProjectHeaders:
			newValue = fProjectHeadersCB->Value();
			if (newValue != fHasProjectHeaders)
			{
				checkBox = fProjectHeadersCB;
				list = &fProjectHeadersList;
				sourceKind = kProjectHeaderFiles;
				fHasProjectHeaders = newValue;
			}
			break;
	}
	
	if (checkBox != nil)
	{
		if (newValue == B_CONTROL_ON)
		{
			// Add files to the list and view
			ASSERT(fProjectIndex >= 0);
			MProjectWindow* project = fProjectList.ItemAt(fSelectedProjectIndex);
			if (project) {
				EmptyList(*list);
				project->FillFileList(*list, sourceKind);
				AddToAllList(*list);
				UpdateListBox();
			}
		}
		else
		{
			// Remove files from the list and view
			RemoveFromAllList(*list);
			EmptyList(*list);
			UpdateListBox();
		}
	}
}

// ---------------------------------------------------------------------------

void
MFindWindow::AddToAllList(MSourceFileList& inList)
{
	// Add all of the sourcefile objects from the list to the all list.
	// Since we are using a MSourceFileSet, "duplicate" leaf names are 
	// allowed.  Only actual duplicate files are filtered out.

	int32 index;
	bool found;
	MSourceFile* sourceFile;
	int32 i = 0;

	while (inList.GetNthItem(sourceFile, i++))
	{
		found = fAllList.FindItem(sourceFile, index);
		if (found == false) {
			fAllList.AddItem(sourceFile, index);
		}
	}
}

// ---------------------------------------------------------------------------
//		RemoveFromAllList
// ---------------------------------------------------------------------------
//	Utility to remove the contents of this list from the allfilelist.

void
MFindWindow::RemoveFromAllList(
	MSourceFileList&	inList)
{
	int32			index;
	MSourceFile*	sourceFile;
	int32			i = 0;

	while (inList.GetNthItem(sourceFile, i++))
	{
		index = fAllList.IndexOf(sourceFile);
		if (index >= 0)
		{
			fAllList.RemoveItemAt(index);
		}
	}
}

// ---------------------------------------------------------------------------
//		EmptyList
// ---------------------------------------------------------------------------

void
MFindWindow::EmptyList(
	MSourceFileList&	inList)
{
	MSourceFile*		item;
	int32				i = 0;

	while (inList.GetNthItem(item, i++))
	{
		delete item;
	}
	
	inList.MakeEmpty();
}

// ---------------------------------------------------------------------------
//		UpdateListBox
// ---------------------------------------------------------------------------

void
MFindWindow::UpdateListBox()
{
	MSourceFile*	sourceFile;
	int32			i = 0;

	fListView->RemoveRows(0, fListView->CountRows());

	while (fAllList.GetNthItem(sourceFile, i))
	{
		fListView->InsertRow(i, sourceFile, -1);

		i++;
	}
	ResetMultiFind();
	
	if (fListView->CountRows() == 0)
		CloseFileSetWindows();
}

// ---------------------------------------------------------------------------
//		DeleteSelectionInListBox
// ---------------------------------------------------------------------------
//	Delete any selected files from the multifile listbox.

void
MFindWindow::DeleteSelectionInListBox()
{
	int32		theRow = -1;

	while (fListView->NextSelected(theRow))
	{
		MSourceFile*		sourceFile = (MSourceFile*) fListView->GetList()->ItemAt(theRow);

		fSourcesList.RemoveItem(sourceFile);
		fSysHeadersList.RemoveItem(sourceFile);
		fProjectHeadersList.RemoveItem(sourceFile);
		fOtherList.RemoveItem(sourceFile);
		fAllList.RemoveItem(sourceFile);

		fListView->RemoveRows(theRow, 1);
		
		delete sourceFile;
	}
}

// ---------------------------------------------------------------------------
//		UpdateRecentStrings
// ---------------------------------------------------------------------------
//	Update the recent strings popups so that the most recent string from
//	the text box is first in the list.  The list is limited to five strings.

void
MFindWindow::UpdateRecentStrings(BMenu* inMenu)
{
	// Get the text from the text box
	const char *	findText;

	if (inMenu == fFindRecentStrings)
		findText = fFindStringBox->Text();
	else
		findText = fReplaceStringBox->Text();
		
	// Does an item with this label exist?
	BMenuItem*		item = inMenu->FindItem(findText);
	
	if (item == nil)
	{
		// If not then get the last item in the list
		item = inMenu->ItemAt(kLeastRecentStringID);
		if (item)
		{
			item->SetLabel(findText);
		}
		else
		{
			// Build a new item if there are fewer than ten
			BMessage*		msg = new BMessage(msgRecentFindString);
			
			if (inMenu == fReplaceRecentStrings)
				msg->what = msgRecentReplaceString;
			item = new BMenuItem(findText, msg);
		}
	}

	// Remove the item from its current position and move
	// it to the front of the list
	if (inMenu->IndexOf(item) != kMostRecentStringID)
	{
		inMenu->RemoveItem(item);
		inMenu->AddItem(item, kMostRecentStringID);
	}
}

// ---------------------------------------------------------------------------
//		StringChanged
// ---------------------------------------------------------------------------
//	A string from one of the recent strings popups has been chosen.  Copy
//	it to the associated text box.

void
MFindWindow::StringChanged(
	BMessage&	inMessage)
{
	BMenuItem*		item;

	if (B_NO_ERROR == inMessage.FindPointer("source", (void**) &item))
	{
		const char *	name = item->Label();		
		MTextView*		textBox;

		if (inMessage.what == msgRecentFindString)
		{
			textBox = fFindStringBox;
		}
		else
		{
			textBox = fReplaceStringBox;
		}

		textBox->SetText(name);
		textBox->Invalidate();
		item->SetMarked(false);
	}
}

// ---------------------------------------------------------------------------

void
MFindWindow::ProjectOpened(MProjectWindow* inWindow)
{
	if (inWindow->Lock())
	{
		fProjectList.AddItem(inWindow);
		fProjectPopup->AddItem(new BMenuItem(inWindow->Title(), new BMessage(msgProjectSelected)));
		if (fData.fMultiFile) {
			SetMultiFileFind(fData.fMultiFile);
		}		
		inWindow->Unlock();
	}
	
	// If this is the only project that we have, go ahead and select it
	if (fProjectList.CountItems() == 1) {
		this->ProjectSelected(0);
	}
}

// ---------------------------------------------------------------------------

void
MFindWindow::HandleProjectSelected(BMessage& inMessage)
{
	// Get the index of the project selected
	
	BMenuItem* item;
	if (inMessage.FindPointer("source", (void**) &item) == B_NO_ERROR)
	{
		ASSERT(item);
		int32 selectedIndex = fProjectPopup->IndexOf(item);
		if (selectedIndex >= 0) {
			this->ProjectSelected(selectedIndex);
		}
	}
}

// ---------------------------------------------------------------------------

void
MFindWindow::ProjectSelected(int32 index)
{
	fSelectedProjectIndex = index;
	
	// Clean up all the lists from old project
	RemoveFromAllList(fSourcesList);
	RemoveFromAllList(fSysHeadersList);
	RemoveFromAllList(fProjectHeadersList);	// other list items remain
	UpdateListBox();

	EmptyList(fSourcesList);
	EmptyList(fSysHeadersList);
	EmptyList(fProjectHeadersList);

	fSourcesCB->SetValue(B_CONTROL_OFF);
	fSystemHeadersCB->SetValue(B_CONTROL_OFF);
	fProjectHeadersCB->SetValue(B_CONTROL_OFF);
	fHasSources = false;
	fHasSystemHeaders = false;
	fHasProjectHeaders = false;

	// Now select the new project and get applicable file sets
	if (index >= 0) {
		BMenuItem* menuItem = fProjectPopup->ItemAt(fSelectedProjectIndex);
		MProjectWindow* project = fProjectList.ItemAt(fSelectedProjectIndex);
		ASSERT(menuItem);
		ASSERT(project);
		menuItem->SetMarked(true);
		
		// Get the file sets applicable to this project
		BMessage msg(msgSetData);
		FileSetHeader rec = { 0, 0, 0, 0 };
		msg.AddData(kFileSet, kMWPrefs, &rec, sizeof(rec), false);
		project->GetData(msg);
			
		ssize_t			size;
		FileSetHeader*	recArray;
		
		if (msg.FindData(kFileSet, kMWPrefs, (const void**) &recArray, &size) == B_NO_ERROR) {
			fSetKeeper.ReplaceProjectSets((FileSetRec*) recArray);
			this->UpdateFileSetMenu();
		}
	}
	else {
		// fProjectPopup->SetLabel("No Project");
		// no project - disable everything	
		SetMultiFileFind(fData.fMultiFile);

		fSetKeeper.ReplaceProjectSets(nil);
		this->UpdateFileSetMenu();
	}
}

// ---------------------------------------------------------------------------

void
MFindWindow::ProjectClosed(MProjectWindow* inProject)
{
	// Get the index of the project
	// Find the project in our list and remove it
	
	int32 index = fProjectList.IndexOf(inProject);
	if (index >= 0) {
		fProjectList.RemoveItemAt(index);
		fProjectPopup->RemoveItem(index);
	}

	// If we are removing the current project, then
	// cancel any active search and select a new project
	
	if (index == fSelectedProjectIndex) {
		fFinder.CancelMultiFindThread();

		// Try to go ahead and select the first project in the list
		// (if we have one, otherwise, signal no project selection)
		if (fProjectList.CountItems() > 0) {
			this->ProjectSelected(0);
		}
		else {
			this->ProjectSelected(-1);
		}
	}	
}

// ---------------------------------------------------------------------------
//		DoFileSet
// ---------------------------------------------------------------------------
//	Called both from the popupmenu and from the file set window.

void
MFindWindow::DoFileSet(
	BMessage& inMessage)
{
	if (inMessage.HasString(kFileName))
	{
		const char *	name = inMessage.FindString(kFileName);
		bool			isProjectList = false;

		// global or project file set?
		if (inMessage.HasBool(kIsInProject))
			isProjectList = inMessage.FindBool(kIsInProject);

		fSetKeeper.AddFileSet(name, isProjectList, fAllList);
		this->UpdateFileSetMenu();
		fFileSetWindow = nil;
		
		if (isProjectList)
		{
			UpdateProjectSets();
		}
	}
	else
	{
		if (fFileSetWindow)
		{
			fFileSetWindow->Activate();
		}
		else
		{
			fFileSetWindow = new MSaveFileSetWindow(*fFileSetsPopup, fSelectedProjectIndex >= 0);
		}
	}
}

// ---------------------------------------------------------------------------
//		UpdateProjectSets
// ---------------------------------------------------------------------------

void
MFindWindow::UpdateProjectSets()
{
	MProjectWindow* selectedProject = fProjectList.ItemAt(fSelectedProjectIndex);
	
	if (selectedProject && selectedProject->OKToModifyProject() == true) {
		BMessage msg(msgSetData);
		MSetHolder set;
		
		fSetKeeper.GetProjectSets(set);
		msg.AddData(kFileSet, kMWPrefs, set.Set(), set.Size(), false);
		selectedProject->SetData(msg);
	}
}

// ---------------------------------------------------------------------------

void
MFindWindow::UpdateFileSetMenu()
{
	// Remove all the items that are file sets themselves
	int32 menuItems = fFileSetsPopup->CountItems();
	for (int32 i = menuItems-1; i >= kFileSetsMenuListStart; i--) {
		delete fFileSetsPopup->RemoveItem(i);
	}	

	bool setAdded = false;
	
	// Now add all the current items back in
	// first global, and then project sets...
	const MFileSet* aSet = NULL;
	for (int32 i = 0; aSet = fSetKeeper.GetNthFileSet(i, false); i++) {
		BMenuItem *newItem = new BMenuItem(aSet->fName, new BMessage(msgFileSetChosen));
		fFileSetsPopup->AddItem(newItem);
		setAdded = true;
	}
	
	for (int32 i = 0; aSet = fSetKeeper.GetNthFileSet(i, true); i++) {
		BMenuItem *newItem = new BMenuItem(aSet->fName, new BMessage(msgFileSetChosen));
		fFileSetsPopup->AddItem(newItem);
		setAdded = true;
	}
	
	// enable the "remove file set" if we have added any sets
	
	BMenuItem* removeSetItem = fFileSetsPopup->ItemAt(kFileSetsMenuRemoveItem);
	removeSetItem->SetEnabled(setAdded);
}

// ---------------------------------------------------------------------------
//		SwitchFileSet
// ---------------------------------------------------------------------------
//	A file set was chosen from the file sets popup.

void
MFindWindow::SwitchFileSet(
	BMessage& inMessage)
{
	// Clear all the lists and the view
	RemoveFromAllList(fSourcesList);
	RemoveFromAllList(fSysHeadersList);
	RemoveFromAllList(fProjectHeadersList);
	RemoveFromAllList(fOtherList);
	EmptyList(fSourcesList);
	EmptyList(fSysHeadersList);
	EmptyList(fProjectHeadersList);
	EmptyList(fOtherList);

	// Get the name of the new file set
	BMenuItem*		item;
	if (B_NO_ERROR == inMessage.FindPointer("source", (void**) &item))
	{
		ASSERT(item);
		
		const MFileSet*		fileSet = fSetKeeper.GetFileSet(item->Label());
		ASSERT(fileSet);
		const FileSetRec*	setRec = fileSet->fRecs;

		// Build the file set list of sourcefile objects from
		// the list of files
		for (int32 i = 0; i < fileSet->fCount; ++i)
		{
			const char *		name = strrchr(setRec->path, '/');
			if (name != nil)
				name++;
			else
				name = setRec->path;

			MSourceFile*	sourceFile = 
				new MSourceFile(setRec->path, 
								FSIsSystemHeader(setRec->flags), 
								FSIsHeader(setRec->flags) ? MSourceFile::kHeaderFileKind : MSourceFile:: kSourceFileKind, 
								name, 
								nil);
			if (FSIsOther(setRec->flags))
				fOtherList.AddItem(sourceFile);
			else
			if (FSIsSource(setRec->flags))
				fSourcesList.AddItem(sourceFile);
			else
			if (FSIsSystemHeader(setRec->flags))
				fSysHeadersList.AddItem(sourceFile);
			else
			if (FSIsProjectHeader(setRec->flags))
				fProjectHeadersList.AddItem(sourceFile);

			fAllList.AddItem(sourceFile);
			setRec = (const FileSetRec*)((char*) setRec + setRec->length);
		}
	}

	fHasSources = fSourcesList.CountItems() > 0;
	fHasSystemHeaders = fSysHeadersList.CountItems() > 0;
	fHasProjectHeaders = fProjectHeadersList.CountItems() > 0;

	fSourcesCB->SetValue(fHasSources ? B_CONTROL_ON : B_CONTROL_OFF);
	fSystemHeadersCB->SetValue(fHasSystemHeaders ? B_CONTROL_ON : B_CONTROL_OFF);
	fProjectHeadersCB->SetValue(fHasProjectHeaders ? B_CONTROL_ON : B_CONTROL_OFF);
	fOtherCB->SetValue(fOtherList.CountItems() > 0 ? B_CONTROL_ON : B_CONTROL_OFF);
	fOtherCB->SetEnabled(fOtherList.CountItems() > 0);

	UpdateListBox();
}

// ---------------------------------------------------------------------------

void
MFindWindow::RemoveFileSet(BMessage& inMessage)
{
	// if the message contains a file name, then that
	// is the message from the remove file set window
	// if the message doesn't contain a file name, then
	// just open up the window and let it choose the file set
	// to remove

	const char* name = NULL;
	if (name = inMessage.FindString(kFileName)) {
		if (fSetKeeper.RemoveFileSet(name)) {
			this->UpdateProjectSets();
		}
		this->UpdateFileSetMenu();
	}
	else {
		if (fRemoveSetWindow) {
			fRemoveSetWindow->Activate();
		}
		else {
			fRemoveSetWindow = new MRemoveFileSetWindow(new BMessenger(this), fSetKeeper);
		}
	}
}

// ---------------------------------------------------------------------------
//		CloseFileSetWindows
// ---------------------------------------------------------------------------

void
MFindWindow::CloseFileSetWindows()
{
	if (fFileSetWindow && fFileSetWindow->Lock()) {
		fFileSetWindow->Quit();
		fFileSetWindow = nil;		
	}

	if (fRemoveSetWindow && fRemoveSetWindow->Lock()) {
		fRemoveSetWindow->Quit();
		fRemoveSetWindow = nil;
	}
}

static void
SetNavigable(
	BView* 	inView,
	bool	inNavigable)
{
	if (inNavigable)
		inView->SetFlags(inView->Flags() | B_NAVIGABLE);
	else
		inView->SetFlags(inView->Flags() & ~B_NAVIGABLE);
}

// ---------------------------------------------------------------------------
//		ToggleWindowSize
// ---------------------------------------------------------------------------
//	Adjust the window size and set the navigable state
//	of views in the bottom half of the window that might
//	or might not be visible.

void
MFindWindow::ToggleWindowSize()
{
	float 		height;

	if (fTriangleButton->Value() == B_CONTROL_ON)
		height = kExpandedHeight;
	else
		height = kContractedHeight;

	ResizeTo(Bounds().Width(), height);
	
	// These views are navigable if the window is expanded, multifile is on,
	// and they are enabled.  Views are made not navigable when they are
	// hidden so could also do this by hiding/showing the views.
	bool		navigable = height == kExpandedHeight && fData.fMultiFile;

	SetNavigable(fStopAtEOFCB, navigable && fStopAtEOFCB->IsEnabled());
	SetNavigable(fSourcesCB, navigable && fSourcesCB->IsEnabled());
	SetNavigable(fSystemHeadersCB, navigable && fSystemHeadersCB->IsEnabled());
	SetNavigable(fProjectHeadersCB, navigable && fProjectHeadersCB->IsEnabled());
	SetNavigable(fOtherButton, navigable && fOtherButton->IsEnabled());
	SetNavigable(fListView, navigable && fListView->IsEnabled());
	
	if (height == kContractedHeight)
	{
		BView*		focus = CurrentFocus();
		
		if (focus != nil)
		{
			BRect		bounds = focus->Bounds();
			focus->ConvertToScreen(&bounds);
			ConvertFromScreen(&bounds);
			if (bounds.top >= kContractedHeight)
				fFindStringBox->MakeFocus();
		}
	}
}

