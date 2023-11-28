// ---------------------------------------------------------------------------
/*
	MLookupDocumentationTask.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			14 January 1999

	This thread looks in the BeBook index for the string specified
	If there is just one match, then NetPositive is opened with
	the html found.  If multiple matches are found, they are sent
	back to the IDEApp to be viewed in the message window. 
*/
// ---------------------------------------------------------------------------

#include "IDEApp.h"
#include "MLookupDocumentationTask.h"
#include "MLookupThreadInformationWindow.h"
#include "MMessageItem.h"
#include "IDEMessages.h"
#include "DocumentationLookup.h"
#include "MAlert.h"
#include <Directory.h>
#include <Entry.h>
#include <Application.h>
#include <Messenger.h>

// ---------------------------------------------------------------------------
// Utility function
// ---------------------------------------------------------------------------

const int32 kMaxLookupStringInTitle = 32;

MMessageWindow* CreateAndOpenMessageWindow(const BString& apiName)
{
	BString lookupString = apiName;
	BString infoTitle = "Documentation for \"";
	infoTitle += lookupString;
	infoTitle += "\"";

	BString windowTitle = "Lookup Results (";
	int32 origCount = lookupString.CountChars();
	lookupString.Truncate(kMaxLookupStringInTitle);
	if (lookupString.CountChars() < origCount) {
		lookupString += B_UTF8_ELLIPSIS;
	}
	windowTitle += lookupString;
	windowTitle += ")";
	
	BMessenger* threadHandler = new BMessenger(be_app_messenger);
	MMessageWindow* messageWindow = new MLookupThreadInformationWindow(windowTitle.String(), 
																	   infoTitle.String(),
																	   threadHandler);
	messageWindow->PostMessage(msgShowAndActivate);
	return messageWindow;
}

// ---------------------------------------------------------------------------
//	MLookupDocumentationTask - Member functions
// ---------------------------------------------------------------------------

MLookupDocumentationTask::MLookupDocumentationTask(IDEApp* theApp, const char* apiName)
						 : MThread("lookupdocumentation"),
						  fIDEApp(theApp),
						  fAPIName(apiName)
{
}

// ---------------------------------------------------------------------------

MLookupDocumentationTask::~MLookupDocumentationTask()
{
}

// ---------------------------------------------------------------------------
// Be Book bookmark locations...
// ---------------------------------------------------------------------------

const char* kDocumentationDirectory = "/boot/beos/documentation/Doc Bookmarks";
const char* kBookmarkFile = "Be Book Bookmarks";
const char* kBookmarkZipFile = "Be Book Bookmarks.zip";

// ---------------------------------------------------------------------------

status_t
MLookupDocumentationTask::Execute()
{
	// Before we do anything - check that the bookmarks are installed	
	bool bookmarkDirectoryExists = this->CheckBookmarkDirectory();
	
	// Query for the attribute that we want
	// If we get an exact match, we will open it in net+, otherwise
	// all matches are put into the message window
	
	// Note... the reason this method seems more complicated 
	// than it should be is because I want to open up the message window
	// or net+ as soon as possible.  If I wait until the end of the 
	// generalized search, the documentation appears to take way to long.
	
	// Even though I get an exact match, and open up net+, I will continue
	// with the generalized search so the message window can be used as
	// a "learn by association" exploration tool.
	
	// Do the query
	DocumentationLookup lookup(fAPIName.String());
	
	// Now go through the results...
	BMessage msg(msgAddDocInfoToMessageWindow);
	DocumentationLookupEntry aMatch;
	int count = 0;
	while (lookup.GetNextExactMatch(aMatch) == true) {
		count += 1;
		msg.AddData(kDocInfo, kInfoType, &aMatch, sizeof(DocumentationLookupEntry));
	}
	
	// figure out what we got from our exact matching
	// If we just got one, go ahead and open up net+
	// If we got more than one, throw them into the message window
	// In either case, go ahead and do the general matching also
	
	BString infoTitle = "Documentation for \"";
	infoTitle += fAPIName;
	infoTitle += "\"";
	MMessageWindow* messageWindow = nil;
	if (count == 1) {
		DocumentationLookupEntry* onlyMatch;
		ssize_t dummySize;
		msg.FindData(kDocInfo, kInfoType, (const void **)&onlyMatch, &dummySize);
		DocumentationLookup::Open(onlyMatch->fURL);
	}
	else if (count > 1) {
		messageWindow = ::CreateAndOpenMessageWindow(fAPIName);
		messageWindow->PostMessage(&msg);
		msg.MakeEmpty();
	}
	
	// Now do the generalized matches.  Even if we only get one in this case,
	// it wasn't an exact match, so put them all in the message window.
	// In this case, post each match (for the first 15) as we get it so that we can see 
	// some progress (the generalized matching takes longer than the exact matching)	
	// then bunch them into groups of 10 for any more matches
	
	while (lookup.GetNextGeneralMatch(aMatch) == true) {
		count += 1;
		msg.AddData(kDocInfo, kInfoType, &aMatch, sizeof(DocumentationLookupEntry));
		if (messageWindow == nil) {
			messageWindow = ::CreateAndOpenMessageWindow(fAPIName);
			messageWindow->PostMessage(&msg);
			msg.MakeEmpty();
		}
		else if (count < 15 || count % 10 == 0) {
			messageWindow->PostMessage(&msg);
			msg.MakeEmpty();
		}
		// check for user cancel on each loop
		if (this->Cancelled()) {
			// return without calling IDEApp::FindDocumentationDone()
			// however, if we have a messageWindow, make sure to tell
			// it that we are done
			if (messageWindow) {
				messageWindow->PostMessage(msgDoneWithMessageWindow);	
			}
			return B_OK;
		}
	}

	// we might have some left over results - post them too
	if (count > 1 && msg.IsEmpty() == false) {
		messageWindow->PostMessage(&msg);
		msg.MakeEmpty();	
	}
	
	// Tell the message window that we are done
	if (count > 1) {
		messageWindow->PostMessage(msgDoneWithMessageWindow);	
	}
	
	// If after all this, we still didn't get anything, tell the user
	if (count == 0) {
		// no matches for that lookup key
		BString text = "No documentation found for: ";
		text += fAPIName;
		// If the bookmark directory doesn't exist - give
		// the user a little more reason why lookup might
		// have failed
		if (bookmarkDirectoryExists == false) {
			text += ". (Be Book lookup bookmarks in \"";
			text += kDocumentationDirectory;
			text += "/";
			text += kBookmarkFile;
			text += "\" do not exist.)";
		}
		MAlert alert(text.String());
		alert.Go();
	}

	fIDEApp->FindDocumentationDone();
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------

bool
MLookupDocumentationTask::CheckBookmarkDirectory()
{
	// returns true if the bookmark directory exists
	
	// If we don't have the bookmark files, and we do have the zip file, ask the user
	// if they would like to install the bookmarks.  Why don't we worry about all the
	// other cases -- like the zip file not being found?
	// Because users can install 3rd party documentation that uses this same mechanism.
	// To skip over the Be Book documentation, all they need to do is remove the
	// .zip file, or not install the Be Book altogether.  
	// If no hits are found, I tell them a little about why not so they get some help
	// to diagnose an installation error problem.
	
	bool bookmarkDirectoryExists = false;
	bool bookmarkZipExists = false;
	BDirectory bookmarkDirectory(kDocumentationDirectory);
	if (bookmarkDirectory.InitCheck() == B_OK) {
		BEntry bookMarkEntry;
		BEntry zipFileEntry;
		bookmarkDirectoryExists = bookmarkDirectory.FindEntry(kBookmarkFile, &bookMarkEntry) == B_OK;
		if (bookmarkDirectoryExists == false && bookmarkDirectory.FindEntry(kBookmarkZipFile, &zipFileEntry) == B_OK) {
			MAlert alert("Be Book documentation lookup requires the Be Book bookmark files.  These files have never been installed.  Would you like to install them? (This may take a few minutes.)", "Install", "Don't Install");
			if (alert.Go() == kOKButton) {
				// expand the zip file - just do it with system...
				// make sure to get to the correct directory and then
				// quote all the directories because they might have spaces in them
				char commandline[512];
				sprintf(commandline, "cd \"%s\"; unzip -o \"%s/%s\"", 
									kDocumentationDirectory,
									kDocumentationDirectory,
									kBookmarkZipFile);
				system(commandline);
				bookmarkDirectoryExists = true;
			}
		}
	}
	return bookmarkDirectoryExists;
}
