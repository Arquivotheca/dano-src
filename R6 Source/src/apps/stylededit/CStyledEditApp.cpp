// ============================================================
//  CStyledEditApp.cpp	©1996 Hiroshi Lockheimer
// ============================================================

#include <Debug.h>
#include <string.h>
#include <unistd.h>
#include <Alert.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <Path.h>
#include <RadioButton.h>
#include <TextControl.h>
#include <fs_attr.h>
#include "CStyledEditApp.h"
#include "CStyledEditWindow.h"
#include "modal.h"
 
#ifdef SOKYO_TUBWAY
#include "BottomlineInput.h"
#endif


const ulong REPLACE_IN_ALL_DOCS = 'FOO!';


#ifdef SOKYO_TUBWAY
TBottomlineView* CStyledEditApp::sBottomline = NULL;
#endif


CStyledEditApp::CStyledEditApp()
	: BApplication("application/x-vnd.Be-STEE")
{
	fSearchString[0] = '\0';
	fReplaceString[0] = '\0';
	fSearchForward = TRUE;
	fSearchWrap = FALSE;
	fSearchSensitive = FALSE;
	fReplaceAll = FALSE;

#ifdef SOKYO_TUBWAY
	BFont theFont(be_plain_font);
	theFont.SetSize(24.0);

	font_height theFontHeight;
	theFont.GetHeight(&theFontHeight);

	BRect windowFrame;
	windowFrame.left = 7.0;
	windowFrame.top = 460.0;
	windowFrame.right = windowFrame.left + 500.0;
	windowFrame.bottom = windowFrame.top + ceil(theFontHeight.ascent + 
												theFontHeight.descent +
												theFontHeight.leading +
												8.0);

	BWindow *window = new BWindow(windowFrame, "Welcome to the Sokyo Tubway", 
								  B_TITLED_WINDOW, B_NOT_RESIZABLE | 
								  B_NOT_CLOSABLE | B_NOT_ZOOMABLE | 
								  B_NOT_MINIMIZABLE);

	windowFrame.OffsetTo(B_ORIGIN);
	window->Lock();	
	sBottomline = new TBottomlineView(windowFrame); 
	window->AddChild(sBottomline);
	sBottomline->SetFont(&theFont);
	sBottomline->MakeFocus();
	window->Unlock();
	window->Show();
#endif
}


void
CStyledEditApp::AboutRequested()
{
#ifdef SOKYO_TUBWAY
	(new BAlert(B_EMPTY_STRING,
				"7,500 yen and a SokyoTubway\n",
				"OK"))->Go();
#else
	(new BAlert(B_EMPTY_STRING,
				"StyledEdit\n",
				"OK"))->Go();
#endif
}	


void
CStyledEditApp::ReadyToRun()
{
	if (CStyledEditWindow::WindowCount() < 1)
		MakeNewWindow();
		//PostMessage(msg_NewWindow);
}


void
CStyledEditApp::MessageReceived(
	BMessage	*inMessage)
{
	switch (inMessage->what) {
		case B_SILENT_RELAUNCH:
			MakeNewWindow();
			break;

		case msg_WindowAdded:
			break;
			
		case msg_WindowRemoved:
			if (CStyledEditWindow::WindowCount() < 1)
				PostMessage(B_QUIT_REQUESTED);
			break;
			
//		case msg_NewWindow:
//			MakeNewWindow();
//			break;
			
//		case msg_OpenWindow:
//		{
//			BEntry dirEntry;
						
//			(new BFilePanel(B_OPEN_PANEL,BMessenger(this), dirEntry))->Show();
//			break;
//		}

		case REPLACE_IN_ALL_DOCS:
		{
			long		total = CStyledEditWindow::WindowCount();
			long		found = 0;

			for (long i = 0; i < total; i++) {
				CStyledEditWindow *doc = (CStyledEditWindow *)CStyledEditWindow::WindowList()->ItemAt(i);
				doc->Lock();
				doc->ReplaceSame();
				doc->Unlock();
			}
			break;	
		}
			
		case B_PRINTER_CHANGED:
		{
			long		total = CStyledEditWindow::WindowCount();
			long		found = 0;

			for (long i = 0; i < total; i++) {
				CStyledEditWindow *doc = (CStyledEditWindow *)CStyledEditWindow::WindowList()->ItemAt(i);
				doc->PostMessage(inMessage);
			}
			break;
		}

		default:
			BApplication::MessageReceived(inMessage);
			break;
	}
}


void
CStyledEditApp::RefsReceived(
	BMessage	*inMessage)
{
	uint32		encoding = kUTF8Conversion;
	BFilePanel	*panel = NULL;

	if (inMessage->FindPointer("stylededit_open_panel", (void **)&panel) == B_NO_ERROR)
		encoding = CStyledEditWindow::EncodingSettingOfFilePanel(panel);

	int32		count = 0;
	uint32		type = 0;
	entry_ref	ref;
	inMessage->GetInfo("refs", &type, &count);
	for (long i = 0; i < count; i++) {
		if (inMessage->FindRef("refs", i, &ref) == B_NO_ERROR) {
			int32 line = 0;
			int32 selectionOffset = 0;
			int32 selectionLength = 0;
			inMessage->FindInt32("be:line", i, &line);
			inMessage->FindInt32("be:selection_offset", i, &selectionOffset);
			inMessage->FindInt32("be:selection_length", i, &selectionLength);
			// Encoding information is stored in an attribute...
			BNode node(&ref);
			attr_info ai;
			if( (node.InitCheck() == B_OK) && (node.GetAttrInfo("be:encoding",&ai) == B_OK) )
				node.ReadAttr("be:encoding",B_INT32_TYPE,0,&encoding, sizeof(uint32));
			// No encoding information in the attribute, try and read from the open file panel...
			else if (inMessage->FindPointer("stylededit_open_panel", (void **)&panel) == B_NO_ERROR)
				encoding = CStyledEditWindow::EncodingSettingOfFilePanel(panel);
				
			OpenWindow(&ref, encoding, line, selectionOffset, selectionLength);
		}
	}
}


void
CStyledEditApp::ArgvReceived(
	int32	argc,
	char	**argv)
{
	BMessage	*message = CurrentMessage();
	const char	*cwd = NULL;

	if (message->FindString("cwd", &cwd) == B_NO_ERROR) {
		BDirectory dir(cwd);
		for (int32 i = 1; i < argc; i++) {
			BEntry entry;
			if (entry.SetTo(&dir, argv[i]) == B_NO_ERROR) {
				entry_ref ref;
				if (entry.GetRef(&ref) == B_NO_ERROR) 
					OpenWindow(&ref);
			}
		}
	}
}


void
CStyledEditApp::MakeNewWindow()
{
	char cwd[PATH_MAX + 1];
	getcwd(cwd, PATH_MAX);

	// this should not be needed
	//	if (strcmp("/", cwd) == 0) {
	//		BPath thePath;
	//		find_directory(B_USER_DIRECTORY, &thePath);
	//		strcpy(cwd, thePath.Path());
	//	}

	entry_ref dirRef;
	get_ref_for_path(cwd, &dirRef);
	
	BWindow *window = new CStyledEditWindow(&dirRef);
	window->Show();
}


void
CStyledEditApp::OpenWindow(
	entry_ref	*inRef,
	uint32		encoding,
	int32		line,
	int32		selectionOffset,
	int32		selectionLength)
{
	CStyledEditWindow *window = CStyledEditWindow::FindWindow(inRef);

	if (window != NULL)
		window->Activate();
	else {
		BEntry refEntry(inRef);
		if (refEntry.IsFile()) {
			BEntry parentEntry;
			refEntry.GetParent(&parentEntry);
			entry_ref parentRef;
			parentEntry.GetRef(&parentRef);
	
			window = new CStyledEditWindow(inRef, &parentRef, encoding);
			window->Show();
		}
	}
	if (line > 0) {
		BMessage msg(msg_GotoLine);
		msg.AddInt32("line", line);
		if (selectionOffset + selectionLength > 0) {
			msg.AddInt32("offset", selectionOffset);
			msg.AddInt32("length", selectionLength);
		}
		window->PostMessage(&msg);
	}
}


// I've stolen the following code from the pre-DR9 version of Edit...

bool
CStyledEditApp::GetFindString(
	bool	replace, 
	BWindow	*window)
{
	TFindReplaceDialog	*theDialog;
	bool				result;
	int32				rval;
	long				tlen;
	long				rlen;

	theDialog = new TFindReplaceDialog(BPoint(100,100), fSearchString,
		replace ? fReplaceString : NULL);

	// Show the dialog

	// This is terrible code!!! A busy wait!

	theDialog->Show();
	while (!theDialog->fPressed) {
		window->UpdateIfNeeded();
		snooze(50000);
	}
	
	// User dismissed the dialog so...

	rval = theDialog->fPressed;
	result = (rval != cmdCancel);
//	PRINT(("pressed = %4s(%d), result=%d\n",
//		&rval, rval, result));

	fReplaceAll = rval == cmdReplaceAll;

	tlen = min_c(MAX_BUFFER_LEN-1, theDialog->fText1->TextLength());
	if (replace)
		rlen = min_c(MAX_BUFFER_LEN-1, theDialog->fText2->TextLength());
	
	if (result) {
		fSearchForward = !theDialog->fBackwards->Value();
		fSearchWrap = theDialog->fWrap->Value();
		fSearchSensitive = theDialog->fSensitive->Value();
	}

	if (result && tlen) {
		strncpy(fSearchString, theDialog->fText1->Text(), tlen);
		fSearchString[tlen] = 0;
		if (replace) {
			strncpy(fReplaceString, theDialog->fText2->Text(), rlen);
			fReplaceString[rlen] = 0;
		}
	} else if (tlen == 0) {
		result = FALSE;
	}
	
	if (replace && theDialog->fGlobalReplace->Value() == 1) {
		/*
		 Let's restore the global "wrap" and "backward" search flags.
		 Using "replace in all documents" forces those option, so let's
		 restore them to their proper state.
		*/
		BMessage *msg = theDialog->fBackwards->Message();
		ASSERT(msg->HasInt32("previous", 0));
		fSearchForward = !(msg->FindInt32("previous"));

		msg = theDialog->fWrap->Message();
		ASSERT(msg->HasInt32("previous", 0));
		fSearchWrap = msg->FindInt32("previous");

		if (result) {
			/*
			 do "replace in all open documents" command. Lets set 'result'
			 to false so the the document that started the 'replace' doesn't
			 try to do the replace in its own text. This will be taken
			 care of by the command we're about to post.
			*/
			result = FALSE;
			PostMessage(REPLACE_IN_ALL_DOCS);
		}
	}

	if (!result)
		rval = cmdCancel;

	theDialog->Lock();
	theDialog->Close();
	return (rval != cmdCancel);
}
