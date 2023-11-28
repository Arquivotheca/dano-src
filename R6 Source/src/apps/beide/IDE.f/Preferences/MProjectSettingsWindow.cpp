// ---------------------------------------------------------------------------
/*
	MProjectSettingsWindow.cpp
	
	Copyright (c) 1999 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			12 January 1999

*/
// ---------------------------------------------------------------------------

#include "MProjectSettingsWindow.h"

#include "IDEMessages.h"
#include "MProjectWindow.h"
#include "MPlugInShepard.h"
#include "MTargetView.h"
#include "MAlert.h"
#include "MBuildersKeeper.h"
#include "PlugInPreferences.h"
#include "MPlugInPrefsView.h"
#include "MPrefsListView.h"
#include "MPlugInLinker.h"
#include "MRunPrefsView.h"

// ---------------------------------------------------------------------------

MProjectSettingsWindow::MProjectSettingsWindow(const char* title, MProjectWindow& project)
					  : MPreferencesWindow(title), 
					    fProject(project)
{
	fTargetView = nil;
}

// ---------------------------------------------------------------------------

MProjectSettingsWindow::~MProjectSettingsWindow()
{
}

// ---------------------------------------------------------------------------

void
MProjectSettingsWindow::MessageReceived(BMessage* inMessage)
{
	switch (inMessage->what) {
		case msgPluginViewModified:
		{
			MPlugInShepard* shepard = dynamic_cast<MPlugInShepard*>(this->GetCurrentView());
			if (shepard != nil) {
				shepard->AdjustDirtyState();
				this->PrefsViewModified();
			}
			break;
		}
		
		case msgLinkerChanged:
			// We get this message from the target view
			// Filter all our preferences based on the new linker name
			const char* linkerName;
			if (inMessage->FindString(kNewLinkerName, &linkerName) == B_OK) {
				this->FilterViewList(linkerName);
			}
			break;
			
		default:
			MPreferencesWindow::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------

bool
MProjectSettingsWindow::CanChangeSettings() const
{
	// Verify that we can modify the project...		
	return fProject.OKToModifyProject();
}

// ---------------------------------------------------------------------------

bool
MProjectSettingsWindow::OKToSave(BMessage& outMessage)
{
	// See if we can save
	// Check with user if they want to save even though it
	// requires a recompile/relink.
	// If so, modify the message to include the appropriate update
	
	bool doSave = true;

	MPreferencesView* currentView = this->GetCurrentView();
	if (currentView && currentView->ProjectRequiresUpdate(kCompileUpdate)) {
		MAlert alert("Changes in this preferences panel may require that sources be recompiled. Do you wish to save settings?", "Save", "Don't Save");
		doSave = (kOKButton == alert.Go());
		if (doSave) {
			outMessage.AddInt32(kNeedsUpdating, kCompileUpdate);
		}
	}
	else if (currentView && currentView->ProjectRequiresUpdate(kLinkUpdate)) {
		MAlert alert("Changes in this preferences panel may require that sources be relinked. Do you wish to save settings?", "Save", "Don't Save");
		doSave = (kOKButton == alert.Go());
		if (doSave) {
			outMessage.AddInt32(kNeedsUpdating, kLinkUpdate);
		}
	}
	
	return doSave;
}

// ---------------------------------------------------------------------------

void
MProjectSettingsWindow::GetTargetData(BMessage& outMessage)
{
	fProject.GetData(outMessage);
	// Add our current project to the preference data
	// so that the individual view has it available if needed
	outMessage.AddPointer(kProjectMID, &fProject);
	// Also add the builder project to the preference data
	// (again so that the individual view has it available if needed)
	outMessage.AddPointer(kBuilderProject, &fProject.ProjectView().BuilderProject());
}

// ---------------------------------------------------------------------------

void
MProjectSettingsWindow::SetTargetData(BMessage& message)
{
	fProject.SetData(message);
}

// ---------------------------------------------------------------------------

void
MProjectSettingsWindow::BuildViews()
{
	BRect r(fgLeftMargin, fgTopMargin, fgRightMargin, fgBottomMargin);

	// Project
	this->AddPreferenceCategory("Project");

	MPreferencesView* prefView = new MAccessPathsView(*this, r);
	this->AddView(prefView, nil);

	fTargetView = new MTargetView(*this, r);
	this->AddView(fTargetView, nil);
	
	MRunPrefsView* runPrefs = new MRunPrefsView(*this, r);
	this->AddView(runPrefs, nil);

	// Language
	this->AddPreferenceCategory("Language");

	// Code Generation
	this->AddPreferenceCategory("Code Generation");

	// Linker
	this->AddPreferenceCategory("Linker");

	// Plug-ins
	// Iterate the add-ons and get all their views
	bool hasOther = false;
	image_id id;
	MPlugInLinker* linker;
	for (int32 addOnCount = 0; MBuildersKeeper::GetNthAddOnID(id, addOnCount, linker); addOnCount++) {
		makeaddonView makeView = nil;
		get_image_symbol(id, "MakeAddOnView", B_SYMBOL_TYPE_TEXT, (void **) &makeView);

		if (makeView) {
			// Iterate through all the views provided by this add-on
			status_t err = B_OK;
			for (int32 viewCount = 0; err == B_OK; viewCount++) {
				MPlugInPrefsView* plugview = nil;
				err = makeView(viewCount, r, plugview);	// Call the plug-in
				if (plugview != nil) {
					MPreferencesView* shepard = new MPlugInShepard(*this, *plugview, r, plugview->Name());
					const char* title = plugview->Title();
					// Handle the following three titles...
					//	<KnownCategory>/title
					//	title
					//	Other/title
					if (hasOther == false && strncmp(title, "Other", 5) == 0) {
						this->AddPreferenceCategory("Other");
						hasOther = true;
					}
					
					int32 index = this->IndexOf(title);
					
					if (index >= 0) {
						const char* slash = strchr(title, '/');
						if (slash) {
							slash += 1;	// bump past the slash
						}
						this->AddView(shepard, linker, index, slash);
					} 
					else {
						if (hasOther == false) {
							this->AddPreferenceCategory("Other");
							hasOther = true;
						}
						this->AddView(shepard, linker);
					}
				}
			}
		}
	}

	// After all our views have been added, make sure the
	// correct target is initialized
	this->InitializeTargetView();
}

// ---------------------------------------------------------------------------

void
MProjectSettingsWindow::FilterViewList(const char* inLinkerName)
{
	// Adjust the items in the preferences list so that the
	// items shown are either built-in, have no specified linker,
	// or have a linker that matches inLinkerName.

	PrefsRowData* rowData;
	for (int32 row = 0; fDataList.GetNthItem(rowData, row); row++)
	{
		MPlugInLinker* linker = (MPlugInLinker*) rowData->cookie;
		int32 wideOpenIndex = fListView->WideOpenIndexOf(rowData);
		if (linker == nil || linker->LinkerName()[0] == '\0' ||
				strcmp(linker->LinkerName(), inLinkerName) == 0) {
			// add the item, (but only is currently not showing)
			if (wideOpenIndex < 0) {
				int32 index = this->VisibleIndexOf(rowData->view->Title());
				index = fListView->GetWideOpenIndex(index);
				if (index < 0) {
					index = fListView->CountRows();
				}
				fListView->InsertRowWideOpen(index, rowData);
			}
		}
		else {
			// remove the item, (but only if currently showing)
			if (wideOpenIndex >= 0) {
				fListView->RemoveRowsWideOpen(wideOpenIndex);			
			}
		}
	}
}


// ---------------------------------------------------------------------------

void
MProjectSettingsWindow::InitializeTargetView()
{
	// To initialize the target view, we need to
	// do three steps
	// (step 1)
	// Set up the tool list in the target view
	//	get the list of tools
	//	sort the names
	//	tell the target view
	//	delete the list
	// (step 2)
	// Set up the correct linker "target" in the target view
	//	get the data from the view (to get the correct type/name)
	//	ask the project for its current data
	//	set up the target view correctly
	// (step 3)
	// Make sure all our views apply to this target

	// (step 1)
	MList<char*> list;
	MBuildersKeeper::GetBuilderNames(list);
	list.SortItems((ListCompareFunc) ListCompareStrings);
	fTargetView->SetupTools(list);
	char* name = nil;
	for (int32 i = 0; list.GetNthItem(name, i); i++) {
		free(name);
	}
	
	// (step 2)
	BMessage msg;
	fTargetView->GetData(msg, true);
	this->GetTargetData(msg);
	fTargetView->SetData(msg);
	
	// (step 3)
	const char* linkerName = fTargetView->CurrentLinkerName();
	this->FilterViewList(linkerName);
}
