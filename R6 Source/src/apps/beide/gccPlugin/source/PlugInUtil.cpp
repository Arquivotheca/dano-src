// ---------------------------------------------------------------------------
/*
	PlugInUtil.cpp
	
	Utility functions for general plugin use.
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#include "PlugInUtil.h"
#include "PlugIn.h"
#include "PlugInPreferences.h"
#include "CommandLineText.h"
#include "MProject.h"
#include "ErrorMessage.h"

#include <string.h>

#include <TextView.h>
#include <View.h>
#include <Directory.h>
#include <Entry.h>
#include <List.h>
#include <Debug.h>


// ---------------------------------------------------------------------------
// DisallowInvalidChars
// Don't allow function type characters to be inserted into our argument text
// ---------------------------------------------------------------------------

void
PlugInUtil::DisallowInvalidChars(BTextView& inText)
{
	inText.DisallowChar(B_ESCAPE);
	inText.DisallowChar(B_INSERT);
	inText.DisallowChar(B_FUNCTION_KEY);
	inText.DisallowChar(B_PRINT_KEY);
	inText.DisallowChar(B_SCROLL_KEY);
	inText.DisallowChar(B_PAUSE_KEY);
}

// ---------------------------------------------------------------------------

void
PlugInUtil::SetViewGray(BView* inView)
{
	inView->SetViewColor(kPrefsGray);
	inView->SetLowColor(kPrefsGray);
}

// ---------------------------------------------------------------------------

void
PlugInUtil::GetProjectDirectory(MProject* theProject, BDirectory& projectDirectory)
{
	// given the MProject - fill in the BDirectory where it lives
	
	entry_ref projectID;
	if (theProject->GetProjectRef(projectID) == B_ERROR) {
		ASSERT_WITH_MESSAGE(false, "GCCBuilder::GetObjectFileDirectory - can't get entry_ref for project");
		return;
	}
	
	// create a BEntry and traverse if the projectID is a symbolic link
	BEntry projectEntry(&projectID, true);
	
	// now get the project's directory
	projectEntry.GetParent(&projectDirectory);
}


// ---------------------------------------------------------------------------

void
PlugInUtil::AddMultipleOptions(BList& outList, const char* optionString)
{
	// Iterate over all the options in optionString
	// and put each string (separated by space) into its own string.
	// Search for each space, and duplicate a string for each option
		
	// create our own private copy so that we can insert a null every so often
	char* optionText = new char[strlen(optionString) + 1];
	strcpy(optionText, optionString);

#ifdef _GCCDEBUG_
	fprintf(stderr, "PlugInUtil::AddMultipleOptions - optionText = %s\n", optionText);
#endif
	
	char* currentOption = optionText;
	while (currentOption) {
		char* nextSpacePtr = strchr(currentOption, ' ');
		// currentOption points to the beginning of the current option
		// nextSpacePtr points to the space or NIL if at end of string
		if (nextSpacePtr) {
			*nextSpacePtr = NIL;
		}
		// before adding the current option, verify that we have a string
		// (if we have multiple spaces, we could end up with zero length strings)
		if (strlen(currentOption)) {
			outList.AddItem(strdup(currentOption));
		}
		
		// now move to the next option (to the space and then one beyond)
		currentOption = nextSpacePtr;
		if (currentOption) {
			currentOption++;
		}
	}
	
	delete [] optionText;
}


