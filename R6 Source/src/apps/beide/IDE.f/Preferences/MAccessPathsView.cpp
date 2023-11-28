//========================================================================
//	MAccessPathsView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MAccessPathsView.h"
#include "MAccessPathListView.h"
#include "MDLOGListView.h"
#include "MDefaultPrefs.h"
#include "MFocusBox.h"
#include "MAlert.h"
#include "MFileUtils.h"
#include "MProjectWindow.h"
#include "IDEMessages.h"
#include "Utils.h"

#include <Button.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <Box.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Path.h>
#include <FilePanel.h>

const char * titleAP = "Access Paths";

// ---------------------------------------------------------------------------
//		MAccessPathsView
// ---------------------------------------------------------------------------
//	Constructor

MAccessPathsView::MAccessPathsView(
	MPreferencesWindow&	inWindow,
	BRect			inFrame)
	: MPreferencesView(inWindow, inFrame, "accesspathsview")
{
	fPathType = kAbsolutePath;
	fAddFilePanel = nil;
	fProject = nil;
}

// ---------------------------------------------------------------------------
//		~MAccessPathsView
// ---------------------------------------------------------------------------
//	Destructor

MAccessPathsView::~MAccessPathsView()
{
	MAccessPathsView::CloseFilePanel();
	// Delete all the structs from the lists
	EmptyList(fProjectPathList);
	EmptyList(fSystemPathList);
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MAccessPathsView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		// Controls in the view
		case msgAddDefaultAccessPath:
			AddDefaultAccessPath();
			break;

		case msgAddAccessPath:
			AddAccessPath();
			break;

		case msgChangeAccessPath:
			ChangeAccessPath();
			break;

		case msgRemoveAccessPath:
			RemoveAccessPath();
			break;

		case msgCheckBoxChanged:
			fNewSettings.pSearchInProjectTreeFirst = fTreater->Value() == 1;
			ValueChanged();
			AdjustButtons();
			break;

		case msgRadioButtonClicked:
			AdjustPathType();
			break;

		case msgListBecameFocus:
		case msgListSelectionChanged:
			AdjustButtons();
			break;

		case msgValueChanged:
			ValueChanged();
			break;

		// From the filepanel by way of the app
		case msgDoAccessPath:
			NewAccessPath(*inMessage);
			break;

		// From the Browser (or internally)
		case B_SIMPLE_DATA:
			MessageDropped(inMessage);
			break;

		default:
			MPreferencesView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		AdjustPathType
// ---------------------------------------------------------------------------
//	Get the current path type from the radio buttons.

void
MAccessPathsView::AdjustPathType()
{
	if (1 == fAbsoluteRB->Value())
		fPathType = kAbsolutePath;
	else
	if (1 == fProjectRelRB->Value())
		fPathType = kProjectRelativePath;
	else
	if (1 == fSystemRelRB->Value())
		fPathType = kSystemRelativePath;
}

// ---------------------------------------------------------------------------
//		NewAccessPath
// ---------------------------------------------------------------------------
//	The filepanel has sent us a message containing one or more refs to be
//	added or changed.  Actually changing with more than one is wierd.

void
MAccessPathsView::NewAccessPath(
	BMessage&	inMessage)
{
	type_code		messageType;
	int32			count;
	
	if (B_NO_ERROR == inMessage.GetInfo("refs", &messageType, &count))
	{
		for (int32 i = 0; i < count; i++)
		{
			entry_ref		ref;
			
			if (B_NO_ERROR == inMessage.FindRef("refs", i, &ref))
			{
				BEntry		entry(&ref);
				if (entry.IsDirectory())
				{
					// Get the list
					MDLOGListView*		list = (MDLOGListView*) Window()->CurrentFocus();
					
					if(list == fProjectView || list == fSystemView)
					{
						// Get the path
						String				path;
						AccessPathT			pathType;

						GetPathForRef(ref, path, pathType);
	
						// Is it already in the list?
						if (! PathInList(path, *list))
						{
							// Add it
							if (fState == aAdding || fState == aDragging)
								AddPathToListView(path, pathType, *list);
							else
							if (fState == aChanging && fChangingIndex >= 0)
							{
							// Replace it
						// The index could still be wrong if the user has rearranged or
						// deleted the list after putting up the changed window ????
								list->DeleteRows(fChangingIndex);
								AddPathToListView(path, pathType, *list, fChangingIndex);
								list->SelectRow(fChangingIndex, false, true);
							}

							ValueChanged();
						}
					}
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		PathInList
// ---------------------------------------------------------------------------
//	Is this path already in the list?  Shold be smarter so that relative
//	paths and absolute paths that point to the same folder are recognized.

bool
MAccessPathsView::PathInList(
	const char * 		inPath,
	MDLOGListView&		inList,
	bool				inShowAlert) const
{
	AccessPathData*		accessPath;

	bool		found = false;
	int32		count = inList.CountRows();

	for (int32 i = 0; i < count; i++)
	{
		accessPath = (AccessPathData*) inList.GetList()->ItemAt(i);
		if (0 == strcmp(inPath, accessPath->pathName))
		{
			found = true;
			break;
		}
	}
	
	if (found && inShowAlert)
	{
		String	text = "This path is already in the list.\n";
		text += inPath;
		
		MAlert	alert(text);
		alert.Go();
	}
	
	return found;
}

// ---------------------------------------------------------------------------
//		GetPathForRef
// ---------------------------------------------------------------------------
//	Build the correct path to this directory.

void
MAccessPathsView::GetPathForRef(
	entry_ref&		inRef,
	String&			outPath,
	AccessPathT&	outType)
{
	BEntry			dirEntry(&inRef);
	char			path[kPathSize];
	bool			forceAbsolutePath = false;
	status_t		err;

	switch (fPathType)
	{
		case kAbsolutePath:
		{
			BPath absolutePath;

			dirEntry.GetPath(&absolutePath);
			outPath = absolutePath.Path();
			outType = kAbsolutePath;
		}
			break;

		case kProjectRelativePath:
		{
			if (fProject && fProject->Lock())
			{
				entry_ref	ref;
				err = fProject->GetRef(ref);
				BEntry projFile(&ref);
				BDirectory projDir;
				BDirectory inputDir(&inRef);

				if (B_NO_ERROR == projFile.GetParent(&projDir)  &&
					MFileUtils::BuildRelativePath(projDir, inputDir, path))
				{
					outPath = path;
					outType = kProjectRelativePath;
				}
				else
					forceAbsolutePath = true;

				fProject->Unlock();
			}
			else
				forceAbsolutePath = true;
		}
			break;

		case kSystemRelativePath:
		{
			BDirectory		inputDir(&inRef);

			if (MFileUtils::BuildRelativePath(MFileUtils::SystemDirectory(), inputDir, path))
			{
				outPath = path;
				outType = kSystemRelativePath;
			}
			else
				forceAbsolutePath = true;
		}
			break;
	}

	if (forceAbsolutePath)
	{
		BPath		tempPath;

		dirEntry.GetPath(&tempPath);
		outPath = tempPath.Path();
		outType = kAbsolutePath;

		MAlert		alert("Couldn't import as a relative path.  Importing as an absolute path.");
		alert.Go();
	}
}

// ---------------------------------------------------------------------------
//		AddDefaultAccessPath
// ---------------------------------------------------------------------------
//	Respond to the Add Default Acces Path button by adding system or project
//	as appropriate.

void
MAccessPathsView::AddDefaultAccessPath()
{
	BView*		list = Window()->CurrentFocus();
	
	ASSERT(list == fProjectView || list == fSystemView);

	if (list == fProjectView)
	{
		String		text = kProjectPathName;
	
		AddPathToListView(text, kPathToProjectTree, *fProjectView, 0);
	}
	else
	if (list == fSystemView)
	{
		String		text = kSystemPathName;
	
		AddPathToListView(text, kPathToSystemTree, *fSystemView, 0);
	}
	
	ValueChanged();

	AdjustButtons();
	
	if (fState != aIdle)
	{
		this->CloseFilePanel();
		fState = aIdle;
	}
}

// ---------------------------------------------------------------------------
//		RemoveAccessPath
// ---------------------------------------------------------------------------
//	Remove an access path from the current list.

void
MAccessPathsView::RemoveAccessPath()
{
	MDLOGListView*		list = (MDLOGListView*) Window()->CurrentFocus();
	
	if(list == fProjectView || list == fSystemView)
	{
		int32		selectedRow;
		
		list->FirstSelected(selectedRow);
		list->DeleteRows(selectedRow);
		AdjustButtons();
		ValueChanged();
	}

	if (fState != aIdle)
	{
		this->CloseFilePanel();
		fState = aIdle;
	}
}

// ---------------------------------------------------------------------------

void
MAccessPathsView::ChangeAccessPath()
{
	// Open the file panel to choose an access path to change
	// Set the current directory to the parent of selection
	fChangingIndex = -1;
	MDLOGListView* list = (MDLOGListView*) Window()->CurrentFocus();
	list->FirstSelected(fChangingIndex);

	if (fChangingIndex >= 0) {
		BPath path;
		entry_ref ref;
		entry_ref* ref_to_show = nil;

		// tests done individually to not depend on order of evaluation
		if (((MAccessPathListView*)list)->ConvertItemToPath(fChangingIndex, path) == B_OK) {
			if (path.GetParent(&path) == B_OK) {
				if (get_ref_for_path(path.Path(), &ref) == B_OK) {
					ref_to_show = &ref;
				}
			}
		}

		this->ShowFilePanel("Change an Access Path", "Change", ref_to_show, aChanging);
	}
}

// ---------------------------------------------------------------------------

void
MAccessPathsView::AddAccessPath()
{
	// Open up the file panel to choose a path to be added to the
	// current list

	this->ShowFilePanel("Add an Access Path", "Add", nil, aAdding);
}

// ---------------------------------------------------------------------------
//		AdjustButtons
// ---------------------------------------------------------------------------
//	Enable or disable the four buttons depending on our state.

void
MAccessPathsView::AdjustButtons()
{
	MDLOGListView*		list = (MDLOGListView*) Window()->CurrentFocus();
	
	if(list == fProjectView || list == fSystemView)
	{
		int32		first;
		bool		hasSelection = (list->FirstSelected(first) > 0);
		
		fAdd->SetEnabled(true);
		fChange->SetEnabled(hasSelection);
		fRemove->SetEnabled(hasSelection);
		
		// Check if the default path is in this list already
		AccessPathData*		accessPath;
		bool				hasDefaultPath = false;
		AccessPathT			defaultPathType = kPathToProjectTree;

		if (list == fSystemView)
			defaultPathType = kPathToSystemTree;
		
		for (int32 i = 0; i < list->CountRows(); ++i)
		{
			accessPath = (AccessPathData*) list->GetList()->ItemAt(i);
			
			if (accessPath->pathType == defaultPathType)
			{
				hasDefaultPath = true;
				break;
			}
		}
		
		fAddDefault->SetEnabled(! hasDefaultPath);
	}
	else
	{
		fAddDefault->SetEnabled(false);
		fAdd->SetEnabled(false);
		fChange->SetEnabled(false);
		fRemove->SetEnabled(false);
	}
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MAccessPathsView::GetData(
	BMessage&	inOutMessage,
	bool		isProxy)
{
	// Update the counts
	fNewSettings.pSystemPaths = fSystemView->CountRows();
	fNewSettings.pProjectPaths = fProjectView->CountRows();

	// Add the AccessPaths struct to the message
	inOutMessage.AddData(kAccessPathsPrefs, kMWPrefs, &fNewSettings, sizeof(fNewSettings));

	// Can't remove anything from a message.
	// Since this message goes to the target where it will also
	// have access paths added we can't add them here.
	if (! isProxy)
	{
		// Add both sets of access paths to the message
		// These are the new paths in the two views
		int32				count = fSystemView->CountRows() + fProjectView->CountRows();

		if (count > 0 )
		{
			AccessPathData*		accessPathArray;
			AccessPathData*		accessPath;
			int32				i;
			int32				j = 0;
			int32				size = count * sizeof(AccessPathData);

			accessPathArray = new AccessPathData[count];

			count = fSystemView->CountRows();
			for (i = 0; i < count; i++)
			{
				accessPath = (AccessPathData*) fSystemView->GetList()->ItemAt(i);
				ASSERT(accessPath);
				accessPathArray[j++] = *accessPath;
			}
		
			count = fProjectView->CountRows();
			
			for (i = 0; i < count; i++)
			{
				accessPath = (AccessPathData*) fProjectView->GetList()->ItemAt(i);
				ASSERT(accessPath);
				accessPathArray[j++] = *accessPath;
			}
			
			inOutMessage.AddData(kAccessPathsData, kMWPrefs, accessPathArray, size, false);
			
			delete accessPathArray;
		}
	}
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MAccessPathsView::SetData(
	BMessage&	inOutMessage)
{
	ssize_t				length;
	AccessPathsPrefs*	inPrefs;

	if (B_NO_ERROR == inOutMessage.FindData(kAccessPathsPrefs, kMWPrefs, (const void**)&inPrefs, &length))
	{
		// grab our project so we can build project relative paths
		MProjectWindow* project;
		if (inOutMessage.FindPointer(kProjectMID, (void **) &project) == B_OK) {
			fProject = project;
		}
		else {
			fProject = nil;
		}
		
		// Tell the list views so they can construct their project relative paths
		fProjectView->SetProject(fProject);
		fSystemView->SetProject(fProject);
		
		// Do the access paths prefs struct
		ASSERT(length == sizeof(AccessPathsPrefs));

		fNewSettings = *inPrefs;
		fOldSettings = fNewSettings;

		// Fill the accesspath lists
		MFileUtils::GetAccessPathsFromMessage(inOutMessage, fSystemPathList, fProjectPathList, 
						fNewSettings.pSystemPaths, fNewSettings.pProjectPaths);

		CopyOldPathsToNewPaths();
		UpdateValues();
		AdjustButtons();
	}
}

// ---------------------------------------------------------------------------
//		CopyOldPathsToNewPaths
// ---------------------------------------------------------------------------
//	Empty out the list views and copy the paths from the lists to the views.

void
MAccessPathsView::CopyOldPathsToNewPaths()
{
	// Delete all the rows from the list views
	fProjectView->DeleteRows(0, fProjectView->CountRows());
	fSystemView->DeleteRows(0, fSystemView->CountRows());
	
	// Add copies of the path structs to the list views
	AccessPathData*		accessPath;
	AccessPathData*		newAccessPath;
	int32				i = 0;

	while (fProjectPathList.GetNthItem(accessPath, i++))
	{
		newAccessPath = new AccessPathData;
		memcpy(newAccessPath, accessPath, sizeof(AccessPathData));
		fProjectView->InsertRow(fProjectView->CountRows(), newAccessPath);
	}

	i = 0;
	while (fSystemPathList.GetNthItem(accessPath, i++))
	{
		newAccessPath = new AccessPathData;
		memcpy(newAccessPath, accessPath, sizeof(AccessPathData));
		fSystemView->InsertRow(fSystemView->CountRows(), newAccessPath);
	}
}

// ---------------------------------------------------------------------------
//		CopyNewPathsToOldPaths
// ---------------------------------------------------------------------------
//	Empty out the lists and copy the paths from the listviews to the lists.

void
MAccessPathsView::CopyNewPathsToOldPaths()
{
	// Delete all the structs from the lists
	EmptyList(fSystemPathList);
	EmptyList(fProjectPathList);

	// Add copies of the path structs to the lists
	AccessPathData*		accessPath;
	AccessPathData*		newAccessPath;
	int32				i;
	int32				count = fSystemView->CountRows();

	for (i = 0; i < count; i++)
	{
		accessPath = (AccessPathData*) fSystemView->GetList()->ItemAt(i);
		newAccessPath = new AccessPathData;
		memcpy(newAccessPath, accessPath, sizeof(AccessPathData));
		fSystemPathList.AddItem(newAccessPath);
	}

	count = fProjectView->CountRows();
	for (i = 0; i < count; i++)
	{
		accessPath = (AccessPathData*) fProjectView->GetList()->ItemAt(i);
		newAccessPath = new AccessPathData;
		memcpy(newAccessPath, accessPath, sizeof(AccessPathData));
		fProjectPathList.AddItem(newAccessPath);
	}
}

// ---------------------------------------------------------------------------
//		UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MAccessPathsView::UpdateValues()
{
	// CheckBox
	fTreater->SetValue(fNewSettings.pSearchInProjectTreeFirst);
}

// ---------------------------------------------------------------------------
//		DoSave
// ---------------------------------------------------------------------------
//	Called after the view's contents have been sent to the target.

void
MAccessPathsView::DoSave()
{
	fOldSettings = fNewSettings;
	CopyNewPathsToOldPaths();

	SetDirty(false);
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------
//	Called when the revert button has been hit.

void
MAccessPathsView::DoRevert()
{
	// Copy the old settings struct to the new settings struct,
	// update the controls in the window and check if we're dirty
	fNewSettings = fOldSettings;
	CopyOldPathsToNewPaths();

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		ValueChanged
// ---------------------------------------------------------------------------
//	Notify the preferences window that one of the values in the view has 
//	changed.

void
MAccessPathsView::ValueChanged()
{
	bool		isdirty = false;

	isdirty = 0 != memcmp(&fOldSettings, &fNewSettings, sizeof(fOldSettings));

	if (! isdirty)
	{
		isdirty = (fProjectView->CountRows() != fProjectPathList.CountItems() ||
			fSystemView->CountRows() != fSystemPathList.CountItems() ||
			ListsAreDifferent(*fProjectView, fProjectPathList) ||
			ListsAreDifferent(*fSystemView, fSystemPathList));
	}
	
	if (isdirty != IsDirty())
	{
		SetDirty(isdirty);

		Window()->PostMessage(msgPrefsViewModified);
		AdjustButtons();
	}
}

// ---------------------------------------------------------------------------
//		ListsAreDifferent
// ---------------------------------------------------------------------------
//	Are the items in the view also found in the list?  The order is important.
//	It is already assumed that the two have the same number of items.

bool
MAccessPathsView::ListsAreDifferent(
	MAccessPathListView&	inView,
	AccessPathList&			inList)
{
	ASSERT(inView.CountRows() == inList.CountItems());

	bool				areDifferent = false;
	AccessPathData*		listData;
	AccessPathData*		viewData;
	int32				index = 0;
	int32				count = inView.CountRows();

	while (inList.GetNthItem(listData, index) && areDifferent == false) {
		viewData = (AccessPathData*) inView.GetList()->ItemAt(index);
		if (listData->pathType != viewData->pathType ||
					listData->recursiveSearch != viewData->recursiveSearch ||
					strcmp(listData->pathName, viewData->pathName) != 0) {
			areDifferent = true;
		}
		index += 1;
	}
	
	return areDifferent;
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------
//	Update all the fields in the view to reflect the factory settings, or 
//	built-in defaults.

void
MAccessPathsView::DoFactorySettings()
{
	// Reset the struct
	MDefaultPrefs::SetAccessPathsDefaults(fNewSettings);

	// Reset the lists
	fProjectView->DeleteRows(0, fProjectView->CountRows());
	fSystemView->DeleteRows(0, fSystemView->CountRows());

	String		text = kProjectPathName;
	
	AddPathToListView(text, kPathToProjectTree, *fProjectView, 0);
	
	int32		i = 0;
	while (accessData[i].pathType <= kPathToSystemTree)
	{
		text = accessData[i].pathName;
		AddPathToListView(text, accessData[i].pathType, *fSystemView, -1, accessData[i].recursiveSearch);
		i++;
	}

	// Update the counters
	fNewSettings.pProjectPaths = 1;
	fNewSettings.pSystemPaths = fSystemView->CountRows();

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		AddPathToListView
// ---------------------------------------------------------------------------
//	Add a new path struct to the specified list view.  Pass the index to be
//	used or -1 for placement at the end of the list.  note the default parameter.

void
MAccessPathsView::AddPathToListView(
	String&			inString,
	AccessPathT		inType,
	MDLOGListView&	inList,
	int32			inIndex,
	bool			inRecursiveSearch)
{
	AccessPathData*		accessPath = new AccessPathData;

	memset(accessPath, '\0', sizeof(AccessPathData));
	accessPath->pathType = inType;
	accessPath->recursiveSearch = inRecursiveSearch;
	strcpy(accessPath->pathName, inString);

	if (inIndex < 0)
		inIndex = inList.CountRows();

	inList.InsertRow(inIndex, accessPath);
}

// ---------------------------------------------------------------------------
//		EmptyList
// ---------------------------------------------------------------------------
//	delete all the structs in the list.

void
MAccessPathsView::EmptyList(
	AccessPathList&	inList)
{
	AccessPathData*	accessPath;
	int32			i = 0;

	while (inList.GetNthItem(accessPath, i++))
	{
		delete accessPath;
	}

	inList.MakeEmpty();
}

// ---------------------------------------------------------------------------
//		MessageDropped
// ---------------------------------------------------------------------------

bool
MAccessPathsView::MessageDropped(	
	BMessage *	inMessage)
{
	bool		result = false;

	if (inMessage->what == B_SIMPLE_DATA || inMessage->HasRef("refs")) {
		BPoint 		where = inMessage->DropPoint();

		Window()->ConvertFromScreen(&where);
		MAccessPathListView*		newView = (MAccessPathListView*) Window()->FindView(where);
		
		if (newView == fProjectView || newView == fSystemView) {
			// decide if the drag is coming from internally or from an external source
			// (where external could be another project... so check for existence
			// of kListObject and that it is one of ours)
			
			MAccessPathListView* oldView;
			status_t viewResult = inMessage->FindPointer(kListObject, (void**) &oldView);
		
			if (viewResult == B_OK && (oldView == fProjectView || oldView == fSystemView)) {
				// This path is being moved
				// Either it's being rearranged in one of the
				// views or is being moved from one view to the other
				Window()->ConvertToScreen(&where);
				newView->ConvertFromScreen(&where);
				int32 oldRow;

				if (inMessage->FindInt32(kLineNumber, &oldRow) == B_OK) {
					int32 newRow = newView->GetRow(where.y);
					
					if (newRow == newView->CountRows() - 1) {
						newRow++;
					}
					
					AccessPathData* accessPath = (AccessPathData*) oldView->GetList()->ItemAt(oldRow);
					
					if ((newView != oldView && ! PathInList(accessPath->pathName, *newView) ||
						 	newRow != oldRow)) {
						newView->InsertRow(newRow, accessPath);
						if (newView == oldView && newRow < oldRow)
							oldRow++;
						oldView->RemoveRows(oldRow);
						ValueChanged();
					}
				}
			}
			else {
				// This is a drag from the Tracker
				AccessPathState state = fState;		// Save the state
				fState = aDragging;
				NewAccessPath(*inMessage);			// Add the path
				fState = state;						// Restore the state
			}
			
			result = true;
		}
	}

	AdjustButtons();
	
	return result;
}

// ---------------------------------------------------------------------------
//		GetTitle
// ---------------------------------------------------------------------------

const char *
MAccessPathsView::Title()
{
	return titleAP;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MAccessPathsView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		LastCall
// ---------------------------------------------------------------------------
//	Tell the app to close the file panel.

void
MAccessPathsView::LastCall()
{
	if (fState != aIdle)
	{
		this->CloseFilePanel();
		fState = aIdle;
	}
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MAccessPathsView::AttachedToWindow()
{
	BRect			bounds = Bounds();
	BRect			r;
	BBox*			box;
	MFocusBox*		focusBox;
	BStringView*	caption;
	BScrollView*	frame;
	BCheckBox*		checkBox;
	BButton*		button;
	BRadioButton*	radioButton;
	BMessage*		msg;
	MAccessPathListView*	list;
	float			top = 15.0;

	// Box
	r = bounds;
	box = new BBox(r, "AccessPaths");
	box->SetLabel("Access Paths");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Treat #include <...> as #include "...".
	r.Set(10, top, 300, 25);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "includer", "Treat #include <...> as #include \"...\".", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fTreater = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Static text
	// Project
	r.Set(10, top, 70, top + 16.0);
	caption = new BStringView(r, "st1", "Project:"); 
	box->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);
	top += 105.0;

	// System
	r.Set(10, top, 70, top + 16.0);
	caption = new BStringView(r, "st2", "System:"); 
	box->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);

	// ListBoxes
	// Project
	top -= 105.0;
	float	left = 120.0;
	r.Set(left, top, bounds.right - 15, top + 96.0);
	focusBox = new MFocusBox(r);						// Focus Box
	box->AddChild(focusBox);
	SetGrey(focusBox, kLtGray);
	focusBox->SetHighColor(kFocusBoxGray);

	r.InsetBy(10.0, 10.0);
	r.OffsetTo(3.0, 3.0);
	list = new MAccessPathListView(r, "project");	// List Box
	list->SetTarget(this);
	list->SetFocusBox(focusBox);

	frame = new BScrollView("frame", list, B_FOLLOW_NONE, B_WILL_DRAW, true, true, B_PLAIN_BORDER);		// For the border
	focusBox->AddChild(frame);
	fProjectView = list;
	top += 105.0;

	// System
	r.Set(left, top, bounds.right - 15, top + 96.0);
	focusBox = new MFocusBox(r);						// Focus Box
	box->AddChild(focusBox);
	SetGrey(focusBox, kLtGray);
	focusBox->SetHighColor(kFocusBoxGray);

	r.InsetBy(10.0, 10.0);
	r.OffsetTo(3.0, 3.0);
	list = new MAccessPathListView(r, "system");		// List Box
	list->SetTarget(this);
	list->SetFocusBox(focusBox);
	frame = new BScrollView("frame", list, B_FOLLOW_NONE, B_WILL_DRAW, true, true, B_PLAIN_BORDER);		// For the border
	focusBox->AddChild(frame);
	fSystemView = list;
	
	top += 20.0;
	left = 8.0;
	
	// Radio buttons
	// Absolute path
	r.Set(left, top, left + 110, top + 16);
	msg = new BMessage(msgRadioButtonClicked);
	radioButton = new BRadioButton(r, "rb1", "Absolute path", msg);
	box->AddChild(radioButton);
	radioButton->SetFont(be_bold_font);
	radioButton->SetTarget(this);
	SetGrey(radioButton, kLtGray);
	radioButton->SetValue(1);
	fAbsoluteRB = radioButton;
	top += 18.0;

	// Project Relative path
	r.Set(left, top, left + 110, top + 16);
	msg = new BMessage(msgRadioButtonClicked);
	radioButton = new BRadioButton(r, "rb2", "Project relative", msg);
	box->AddChild(radioButton);
	radioButton->SetFont(be_bold_font);
	radioButton->SetTarget(this);
	SetGrey(radioButton, kLtGray);
	fProjectRelRB = radioButton;
	top += 18.0;

	// IDE Relative path
	r.Set(left, top, left + 110, top + 16);
	msg = new BMessage(msgRadioButtonClicked);
	radioButton = new BRadioButton(r, "rb3", "IDE relative", msg);
	box->AddChild(radioButton);
	radioButton->SetFont(be_bold_font);
	radioButton->SetTarget(this);
	SetGrey(radioButton, kLtGray);
	fSystemRelRB = radioButton;

	// Buttons
	// Add default
	top += 45.0;
	left = 10.0;
	r.Set(left, top, left + 75, top + 20);
	msg = new BMessage(msgAddDefaultAccessPath);
	button = new BButton(r, "adddefault", "Add Default", msg); 
	box->AddChild(button);
	button->SetTarget(this);
	fAddDefault = button;
	left += 85.0;
	
	// Add
	r.Set(left, top, left + 75, top + 20);
	msg = new BMessage(msgAddAccessPath);
	button = new BButton(r, "add", "Add", msg); 
	box->AddChild(button);
	button->SetTarget(this);
	fAdd = button;
	left += 85.0;

	// Change
	r.Set(left, top, left + 75, top + 20);
	msg = new BMessage(msgChangeAccessPath);
	button = new BButton(r, "change", "Change", msg); 
	box->AddChild(button);
	button->SetTarget(this);
	fChange = button;
	left += 85.0;
	
	// Remove
	r.Set(left, top, left + 75, top + 20);
	msg = new BMessage(msgRemoveAccessPath);
	button = new BButton(r, "remove", "Remove", msg); 
	box->AddChild(button);
	button->SetTarget(this);
	fRemove = button;
	left += 85.0;
	
	// This is a bit goofy but serves to initialize the focus boxes correctly
	fSystemView->MakeFocus();
	fProjectView->MakeFocus();
}

// ---------------------------------------------------------------------------

void
MAccessPathsView::ShowFilePanel(const char* inTitle, const char* inButtonLabel,
								entry_ref* inDirectory, AccessPathState inState)
{
	
	// if we are running the wrong flavor - remove it
	if (fAddFilePanel != nil && fState != inState) {
		this->CloseFilePanel();
	}

	// set up our state, and then set up the file panel
	fState = inState;
	
	if (fAddFilePanel != nil) {
		if (inDirectory) {
			fAddFilePanel->SetPanelDirectory(inDirectory);
		}
		::ShowAndActivate(fAddFilePanel->Window());
	}
	else {
		BMessenger me(this);
		BMessage msg(msgDoAccessPath);
		BWindow* panel;
		fAddFilePanel = new BFilePanel(B_OPEN_PANEL, 
									   &me, 
									   inDirectory, 
									   B_DIRECTORY_NODE, 
									   true, 
									   &msg);
		panel = fAddFilePanel->Window();
		panel->SetTitle(inTitle);
		fAddFilePanel->SetButtonLabel(B_DEFAULT_BUTTON, inButtonLabel);
		fAddFilePanel->Show();
	}
}

// ---------------------------------------------------------------------------

void
MAccessPathsView::CloseFilePanel()
{
	if (fAddFilePanel != nil) {
		delete fAddFilePanel;
		fAddFilePanel = nil;
	}
}

// ---------------------------------------------------------------------------
//		• SwapBigToHost
// ---------------------------------------------------------------------------

void
AccessPathData::SwapBigToHost()
{
	ASSERT(sizeof(AccessPathData) == 264);
	if (B_HOST_IS_LENDIAN)
		pathType = (AccessPathT) B_BENDIAN_TO_HOST_INT32(pathType);
}

// ---------------------------------------------------------------------------
//		• SwapHostToBig
// ---------------------------------------------------------------------------

void
AccessPathData::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
		pathType = (AccessPathT) B_HOST_TO_BENDIAN_INT32(pathType);
}


