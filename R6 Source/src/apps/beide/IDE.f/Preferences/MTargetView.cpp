//========================================================================
//	MTargetView.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS
//	Should add the ability to drop a file into the lower portion
//	of the view and have its filetype and extension fill the fields.  ????

#include <string.h>

#include "MTargetView.h"
#include "MTargetListView.h"
#include "MPictureMenuField.h"
#include "MTargetTypes.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "MTextControl.h"
#include "MFocusBox.h"
#include "MBuildersKeeper.h"
#include "MPlugInLinker.h"
#include "IDEMessages.h"
#include "Utils.h"

#include <Button.h>
#include <Locker.h>
#include <Picture.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Box.h>
#include <Bitmap.h>
#include <ScrollView.h>

const char * titleTarget = "Target";
const float kCheckMarkWidth = 16.0;
const float kCheckMarkHeight = 16.0;

#define _ 0xff
#define B 0
#define w 63
#define h 26
#define i 16

const char sCheckMarkIcon[] = {
	B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,_,
	B,w,w,w,w,w,w,w,w,w,w,w,w,w,w,B,
	B,w,h,h,h,h,h,h,h,h,h,h,h,i,B,i,
	B,w,h,h,h,h,h,h,h,h,h,B,B,i,B,i,
	B,w,h,h,h,h,h,h,h,h,B,B,B,i,B,i,
	B,w,h,h,h,h,h,h,h,B,B,B,B,i,B,i,
	B,w,h,h,B,B,h,h,B,B,B,B,h,i,B,i,
	B,w,h,B,B,B,h,B,B,B,B,h,h,i,B,i,
	B,w,h,B,B,B,B,B,B,B,h,h,h,i,B,i,
	B,w,h,B,B,B,B,B,B,h,h,h,h,i,B,i,
	B,w,h,B,B,B,B,B,h,h,h,h,h,i,B,i,
	B,w,h,B,B,B,B,h,h,h,h,h,h,i,B,i,
	B,w,h,h,B,B,h,h,h,h,h,h,h,i,B,i,
	B,w,i,i,i,i,i,i,i,i,i,i,i,i,B,i,
	B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,i,
	_,i,i,i,i,i,i,i,i,i,i,i,i,i,i,i
};

#undef _
#undef B
#undef w
#undef h
#undef i

const int32 kHasResourcesIndex = 6;


// ---------------------------------------------------------------------------
//		MTargetView
// ---------------------------------------------------------------------------
//	Constructor

MTargetView::MTargetView(
	MPreferencesWindow&	inWindow,
	BRect			inFrame)
	: MPreferencesView(inWindow, inFrame, "targetview")
{
}

// ---------------------------------------------------------------------------
//		~MTargetView
// ---------------------------------------------------------------------------
//	Destructor

MTargetView::~MTargetView()
{
	EmptyList(fOldTargets);		// Delete all the structs from the list
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MTargetView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		// From controls in the view
		case msgListSelectionChanged:
			SelectionChanged();
			break;

		case msgAddTarget:
			AddNewRecord();
			break;

		case msgChangeTarget:
			ChangeRecord();
			break;

		case msgRemoveTarget:
			RemoveRecord();
			break;

		case msgFlagChosen:
			HandleFlagsPopup(*inMessage);
			break;

		case msgToolChosen:
			ValueChanged();
			break;

		case msgTextChanged:
			UpdateButtons();
			break;

		case msgTargetChosen:
			TargetPopupChanged(*inMessage);
			break;

		// From the Browser
		// Should fill in the fields in the bottom of the view with the info from this file ????
		case B_SIMPLE_DATA:
//			MessageDropped(inMessage);
			break;

		default:
			MPreferencesView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		UpdateButtons
// ---------------------------------------------------------------------------
//	Adjust the enabled state of the three buttons based on whether something
//	is selected in the list view, whether there is something in the 
//	edit boxes, and whether the edit boxes have been changed since the last
//	selection change.

void
MTargetView::UpdateButtons()
{
	int32		row = -1;
	bool		hasSelection = fListView->FirstSelected(row);
	bool		hasContents = (0 != strlen(fFileTypeBox->Text()) || 
					0 != strlen(fFileExtensionBox->Text()));
	bool		fieldsChanged = FieldsChanged();
	TargetRec	rec;

	FieldsToRecord(rec);	

	bool		alreadyExists = RecordExists(rec);

	fAddButton->SetEnabled(hasContents && fieldsChanged && ! alreadyExists);
	fChangeButton->SetEnabled(hasSelection && hasContents && fieldsChanged);
	fRemoveButton->SetEnabled(hasSelection);
}

// ---------------------------------------------------------------------------
//		FieldsChanged
// ---------------------------------------------------------------------------
//	return true if anything in the edit boxes or popup is different from
//	when the selection last changed in the edit box.

bool
MTargetView::FieldsChanged()
{
	TargetRec			rec;
	
	FieldsToRecord(rec);

	bool	changed = (fCurrentRec.Stage != rec.Stage) ||
				(fCurrentRec.Flags != rec.Flags) ||
				(0 != strcmp(fCurrentRec.MimeType, rec.MimeType)) ||
				(0 != strcmp(fCurrentRec.Extension, rec.Extension)) ||
				(0 != strcmp(fCurrentRec.ToolName, rec.ToolName));
	
	return changed;
}

// ---------------------------------------------------------------------------
//		TargetPopupChanged
// ---------------------------------------------------------------------------
//	Get the new linker name.

void
MTargetView::TargetPopupChanged(
	BMessage&	inMessage)
{
	BMenuItem*		item;

	if (B_NO_ERROR == inMessage.FindPointer("source", (void **)&item))
	{
		int32			index = fTargetPopup->IndexOf(item);
		MPlugInLinker*	linker;

		if (MBuildersKeeper::GetNthLinker(linker, index))
		{
			strcpy(fNewSettings.pLinkerName, linker->LinkerName());
		}

		ValueChanged();
	}
}

// ---------------------------------------------------------------------------
//		HandleFlagsPopup
// ---------------------------------------------------------------------------
//	Something from the flags popup was chosen.  The first five items are
//	essentially radio buttons, the last item is independent.

void
MTargetView::HandleFlagsPopup(
	BMessage&	inMessage)
{
	BMenuItem*		item;

	if (B_NO_ERROR == inMessage.FindPointer("source",  (void **)&item))
	{
		int32			index = fFlagsPopup->IndexOf(item);

		switch (index)
		{
			case ignoreStage:
			case precompileStage:
			case compileStage:
			case linkStage:
			case postLinkStage:
				if (item->IsMarked())
					index = ignoreStage;
				for (int32 i = 0; i <= postLinkStage; i++)
					fFlagsPopup->ItemAt(i)->SetMarked(false);
				fFlagsPopup->ItemAt(index)->SetMarked(true);
				break;

			default:
			// Has resources
				item->SetMarked(! item->IsMarked());
				break;
		}
		
		ValueChanged();
	}
}

// ---------------------------------------------------------------------------
//		SelectionChanged
// ---------------------------------------------------------------------------
//	The line selected in the list view changed so get the current line's 
//	info and fill the edit boxes and the popup with the appropriate values.

void
MTargetView::SelectionChanged()
{
	int32				row = -1;
	
	if (fListView->FirstSelected(row))
	{
		// Copy all the info to the fields and popup
		TargetRec*		rec = (TargetRec*) fListView->GetList()->ItemAt(row);
		
		fCurrentRec = *rec;
		fFileTypeBox->SetText(fCurrentRec.MimeType);
		fFileExtensionBox->SetText(fCurrentRec.Extension);
		BMenuItem*		item = nil;
		
		if (fCurrentRec.ToolName[0] != '\0')
			item = fToolsPopup->FindItem(fCurrentRec.ToolName);
		else
			item = fToolsPopup->ItemAt(0);

		if (item && ! item->IsMarked())
			item->SetMarked(true);

		for (int32 i = 0; i <= postLinkStage; i++)
			fFlagsPopup->ItemAt(i)->SetMarked(false);
		fFlagsPopup->ItemAt(fCurrentRec.Stage)->SetMarked(true);
		fFlagsPopup->ItemAt(kHasResourcesIndex)->SetMarked(TargetHasResources(fCurrentRec.Flags));
	}
	else
	{
		// Empty all the fields
		fFileTypeBox->SetText("");
		fFileExtensionBox->SetText("");
		BMenuItem*		item = fToolsPopup->ItemAt(0);
		if (item && ! item->IsMarked())
			item->SetMarked(true);

		for (int32 i = 1; i <= postLinkStage; i++)
			fFlagsPopup->ItemAt(i)->SetMarked(false);
		fFlagsPopup->ItemAt(ignoreStage)->SetMarked(true);
		fFlagsPopup->ItemAt(kHasResourcesIndex)->SetMarked(false);
	}

	UpdateButtons();
}

// ---------------------------------------------------------------------------
//		AddNewRecord
// ---------------------------------------------------------------------------
//	 Add a new record to the list view from the info in the lower panel.

void
MTargetView::AddNewRecord()
{
	TargetRec*		rec = new TargetRec;

	FieldsToRecord(*rec);
	fCurrentRec = *rec;
	AddRecToListView(rec);	
	
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		ChangeRecord
// ---------------------------------------------------------------------------
//	Change a record in the listview
void
MTargetView::ChangeRecord()
{
	int32				row = -1;
	
	if (fListView->FirstSelected(row))
	{
		TargetRec*		rec = (TargetRec*) fListView->GetList()->ItemAt(row);
		FieldsToRecord(*rec);
		fCurrentRec = *rec;
		fListView->InvalidateRow(row);
		ValueChanged();
	}
	else
		DEBUGGER("Attempt to change an empty record\n");
}

// ---------------------------------------------------------------------------
//		RemoveRecord
// ---------------------------------------------------------------------------

void
MTargetView::RemoveRecord()
{
	int32				row = -1;
	
	if (fListView->FirstSelected(row))
	{
		fListView->DeleteRows(row);
		ValueChanged();
	}
}

// ---------------------------------------------------------------------------
//		FieldsToRecord
// ---------------------------------------------------------------------------
//	Copy all of the info from the fields in the view to the specified targetrec.

void
MTargetView::FieldsToRecord(
	TargetRec&	inRec)
{
	// FileType
	strcpy(inRec.MimeType, fFileTypeBox->Text());	// Mime type

	strncpy(inRec.Extension, fFileExtensionBox->Text(), 8);	// Extension
	BMenuItem*	item = fToolsPopup->FindMarked();
	if (item && 0 != fToolsPopup->IndexOf(item))
		strncpy(inRec.ToolName, item->Label(), 64);			// Tool Name
	else
		inRec.ToolName[0] = '\0';

	// Make Stage
	inRec.unused1 = 0;
	for (int32 i = 0; i <= postLinkStage; i++)
	{
		if (fFlagsPopup->ItemAt(i)->IsMarked())
		{
			inRec.Stage = i;
			break;
		}
	}

	// Flags
	inRec.Flags = fFlagsPopup->ItemAt(kHasResourcesIndex)->IsMarked();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MTargetView::GetData(
	BMessage&	inOutMessage,
	bool		isProxy)
{
	int32	count = fListView->CountRows();

	fNewSettings.pCount = count;

	status_t	err = inOutMessage.AddData(kTargetPrefs, kMWPrefs, &fNewSettings, sizeof(fNewSettings));

	if (! isProxy && count > 0)
	{
		TargetRec*			targetArray;
		TargetRec*			target;
		int32				j = 0;
		int32				size = count * sizeof(TargetRec);

		targetArray = new TargetRec[count];

		for (int i = 0; i < count; i++)
		{
			target = (TargetRec*) fListView->GetList()->ItemAt(i);
			ASSERT(target);
			targetArray[j++] = *target;
		}

		inOutMessage.AddData(kTargetBlockPrefs, kMWPrefs, targetArray, size, false);
		
		delete [] targetArray;
	}
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MTargetView::SetData(
	BMessage&	inMessage)
{
	ssize_t			length;

	TargetPrefs*	inPrefs;
	if (B_NO_ERROR == inMessage.FindData(kTargetPrefs, kMWPrefs, (const void **)&inPrefs, &length))
	{
		fListView->DeleteRows(0, fListView->CountRows());	// Empty the view

		ASSERT(length == sizeof(TargetPrefs));

		fNewSettings = *inPrefs;
		fOldSettings = fNewSettings;

		int32			count = fNewSettings.pCount;
		
		TargetRec*		targetArray;
		if (count > 0 && B_NO_ERROR == inMessage.FindData(kTargetBlockPrefs, kMWPrefs, (const void **)&targetArray, &length))
		{
			if (targetArray)
			{
				TargetRec*		newTarget;
				int32			j = 0;

				for (int32 i = 0; i < count; i++)
				{
					newTarget = new TargetRec;
					*newTarget = targetArray[j++];
					AddRecToListView(newTarget);			// Add a record to the view
				}
			}	
		}

		CopyNewRecsToOld();									// Sync the view and the list
		ClearCurrentRecord();
		ResetFields();
		UpdateButtons();
		SetTargetPopup();
	}
}

// ---------------------------------------------------------------------------
//		SetTargetPopup
// ---------------------------------------------------------------------------
//	Set the name visible in the target popup to match the 
//	targetname of the linker.

void
MTargetView::SetTargetPopup()
{
	fLinker = MBuildersKeeper::GetLinker(fNewSettings.pLinkerName);	// might return nil

	if (fLinker != nil)
	{
		// Get the name of the target and set the target popup
		const char *		targetName = fLinker->TargetName();
		int32				targetCount = fTargetPopup->CountItems();
	
		for (int32 i = 0; i < targetCount; i++)
		{
			BMenuItem*		item = fTargetPopup->ItemAt(i);
			ASSERT(item != nil);
			
			if (0 == strcmp(targetName, item->Label()))
			{
				item->SetMarked(true);
				break;
			}
		}
	}
	else
	{
		// The plug in isn't there
		BMenuItem*		item = fTargetPopup->ItemAt(0);
		if (item != nil)
			item->SetMarked(true);
	}
}

// ---------------------------------------------------------------------------
//		CopyNewRecsToOld
// ---------------------------------------------------------------------------
//	Empty the fOldTargets list and copy all the recs from the view to the list.

void
MTargetView::CopyNewRecsToOld()
{
	EmptyList(fOldTargets);

	// Add copies of the target structs to the list view
	TargetRec*			rec;
	TargetRec*			newRec;
	int32				count = fListView->CountRows();

	for (int32 i = 0; i < count; i++)
	{
		rec = (TargetRec*) fListView->GetList()->ItemAt(i);
		newRec = new TargetRec(*rec);	// Use default copy constructor
		fOldTargets.AddItem(newRec);
	}
}

// ---------------------------------------------------------------------------
//		CopyOldRecsToNew
// ---------------------------------------------------------------------------
//	Empty the view and copy all of the recs from the fOldTargets list to the
//	view.

void
MTargetView::CopyOldRecsToNew()
{
	fListView->DeleteRows(0, fListView->CountRows());

	// Add copies of the target structs to the list view
	TargetRec*			rec;
	TargetRec*			newRec;
	int32				i = 0;

	while (fOldTargets.GetNthItem(rec, i++))
	{
		newRec = new TargetRec(*rec);	// Use default copy constructor
		AddRecToListView(newRec);
	}
}

// ---------------------------------------------------------------------------
//		EmptyList
// ---------------------------------------------------------------------------
//	delete all the structs in the list.

void
MTargetView::EmptyList(
	TargetList&	inList)
{
	TargetRec*		rec;
	int32			i = 0;

	while (inList.GetNthItem(rec, i++))
	{
		delete rec;
	}

	inList.MakeEmpty();
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------
//	Get the default list of target records and add them to the listview.

void
MTargetView::DoFactorySettings()
{
	// Empty the view
	fListView->DeleteRows(0, fListView->CountRows());

	MDefaultPrefs::SetTargetsDefaults(fNewSettings);

	TargetRec*		rec = fNewSettings.pTargetArray;
	TargetRec*		newRec;

	for (int i = 0 ; i < fNewSettings.pCount; i++)
	{
		newRec = new TargetRec(*rec);	// Use default copy constructor		
		AddRecToListView(newRec);
		rec++;
	}

	SetTargetPopup();
	ClearCurrentRecord();
	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------
//	Called when the revert button has been hit.  Because the
//	format of our prefs isn't a simple struct we can't use the 
//	inherited version of this function.

void
MTargetView::DoRevert()
{
	// Copy the old settings struct to the new settings struct,
	// update the controls in the window and check if we're dirty
	CopyOldRecsToNew();

	ClearCurrentRecord();
	
	fNewSettings = fOldSettings;
	SetTargetPopup();
	
	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		DoSave
// ---------------------------------------------------------------------------
//	Called after the view's contents have been sent to the target.

void
MTargetView::DoSave()
{
	CopyNewRecsToOld();
	ClearCurrentRecord();
	fOldSettings = fNewSettings;

	SetDirty(false);
	
	BMessage	msg(msgLinkerChanged);
	
	msg.AddString(kNewLinkerName, fNewSettings.pLinkerName);

	Window()->PostMessage(&msg);
}

// ---------------------------------------------------------------------------
//		ValueChanged
// ---------------------------------------------------------------------------
//	Notify the preferences window that one of the values in the view has 
//	changed.  Slightly different than the MPreferencesView version.

void
MTargetView::ValueChanged()
{
	bool		isdirty = false;

	isdirty = (fListView->CountRows() != fOldTargets.CountItems() ||
		ListsAreDifferent(*fListView, fOldTargets)) || 
		0 != strcmp(fOldSettings.pLinkerName, fNewSettings.pLinkerName);
	
	if (isdirty != IsDirty())
	{
		SetDirty(isdirty);

		Window()->PostMessage(msgPrefsViewModified);
	}
	
	UpdateButtons();
}

// ---------------------------------------------------------------------------
//		ListsAreDifferent
// ---------------------------------------------------------------------------
//	Are the items in the view also found in the list?  The order is irrelevant.
//	It is already assumed that the two have the same number of items.

bool
MTargetView::ListsAreDifferent(
	MTargetListView&	inView,
	TargetList&			inList)
{
	ASSERT(inView.CountRows() == inList.CountItems());

	bool				areDifferent = false;
	TargetRec*			listData;
	TargetRec*			viewData;
	int32				i = 0;
	int32				count = inView.CountRows();

	while (inList.GetNthItem(listData, i++) && ! areDifferent)
	{
		bool		found = false;
		
		for (int32 j = 0; j < count; j++)
		{
			viewData = (TargetRec*) inView.GetList()->ItemAt(j);
			if (listData->Stage == viewData->Stage &&
				listData->Flags == viewData->Flags &&
				0 == strcmp(listData->MimeType, viewData->MimeType) &&
				0 == strcmp(listData->Extension, viewData->Extension) &&
				0 == strcmp(listData->ToolName, viewData->ToolName))
				{
					found = true;
					break;
				}
		}
		
		areDifferent = ! found;
	}
	
	return areDifferent;
}

// ---------------------------------------------------------------------------
//		AddRecToListView
// ---------------------------------------------------------------------------
//	The list view displays the records in alphabetical order based on the 
//	file type and extension.

void
MTargetView::AddRecToListView(
	TargetRec*		inRec)
{
	bool		hasFileType = inRec->MimeType[0] != '\0';
	int32		count = fListView->CountRows();
	BList&		list = *fListView->GetList();
	int32		i = 0;

	while (i < count)
	{
		TargetRec*		rec = (TargetRec*) list.ItemAt(i);
		bool			recHasFileType = rec->MimeType[0] != '\0';

		if (hasFileType && recHasFileType)
		{
			int		compare = strcmp(inRec->MimeType, rec->MimeType);
			if (compare < 0 ||
				(compare == 0 && strcmp(inRec->Extension, rec->Extension) < 0))
			{
				break;
			}
		}
		else
		if ((hasFileType && ! recHasFileType) ||
			(! hasFileType && ! recHasFileType && strcmp(inRec->Extension, rec->Extension) < 0))
			break;

		i++;
	}

	fListView->InsertRow(i, inRec);
}

// ---------------------------------------------------------------------------
//		ClearCurrentRecord
// ---------------------------------------------------------------------------

void
MTargetView::ClearCurrentRecord()
{
	fCurrentRec.MimeType[0] = '\0';
	fCurrentRec.unused1 = 0;
	fCurrentRec.Stage = ignoreStage;
	fCurrentRec.Flags = noResources;
	fCurrentRec.Extension[0] = '\0';
	fCurrentRec.ToolName[0] = '\0';
}

// ---------------------------------------------------------------------------
//		ResetFields
// ---------------------------------------------------------------------------

void
MTargetView::ResetFields()
{
	fFileTypeBox->SetText("");
	fFileExtensionBox->SetText("");
	fToolsPopup->ItemAt(0)->SetMarked(true);	// initialize to <none>
	for (int32 i = 1; i <= kHasResourcesIndex; i++)
		fFlagsPopup->ItemAt(i)->SetMarked(false);
	fFlagsPopup->ItemAt(0)->SetMarked(true);	// Set the ignoreed by make flag
}

// ---------------------------------------------------------------------------
//		RecordExists
// ---------------------------------------------------------------------------
//	Does a record with the same file type and extension already exist in the
//	view?

bool
MTargetView::RecordExists(
	TargetRec&		inRec)
{
	int32		count = fListView->CountRows();
	BList&		list = *fListView->GetList();
	bool		exists = false;

	for (int32 i = 0; i < count; i++)
	{
		TargetRec*		rec = (TargetRec*) list.ItemAt(i);
		if (0 == strcmp(inRec.MimeType, rec->MimeType) && 0 == strcmp(inRec.Extension, rec->Extension))
		{
			exists = true;
			break;
		}
	}

	return exists;
}

// ---------------------------------------------------------------------------
//		GetTitle
// ---------------------------------------------------------------------------

void
MTargetView::GetTitle(
	String&	inString)
{
	inString = titleTarget;
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
MTargetView::Title()
{
	return titleTarget;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MTargetView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MTargetView::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kCompileUpdate;
}

// ---------------------------------------------------------------------------
//		SetupTools
// ---------------------------------------------------------------------------

void
MTargetView::SetupTools(
	MList<char*>&	inList)
{
	char*		name;
	int32		i = 0;

	while (inList.GetNthItem(name, i++))
	{
		BMenuItem*		item = new BMenuItem(name, new BMessage(msgToolChosen));
		fToolsPopup->AddItem(item);	
	}

	fToolsPopup->SetTargetForItems(this);		
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MTargetView::AttachedToWindow()
{
	BRect				bounds = Bounds();
	BRect				r;
	BBox*				box;
	BStringView*		caption;
	BPopUpMenu*			popup;
	BButton*			button;
	MFocusBox*			focusBox;
	BScrollView*		frame;
	MTargetListView*	list;
	BMessage*			msg;
	BMenuItem*			item;
	BTextView*			text;
	MTextControl*		textControl;
	BMenuField*			menufield;
	MPictureMenuField*	pictureMenuField;
	float				top = 20.0;
	float 				left = 15.0;
	const float			bottomTop = 190.0;

	// Box
	r = bounds;
	box = new BBox(r, "targetbox");
	box->SetLabel("Targets");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Targets popup
	r.Set(left - 2.0, top, bounds.right - 20.0, top + 16);

	popup = new BPopUpMenu("tools");
	fTargetPopup = popup;

	MBuildersKeeper::BuildTargetsPopup(*popup);

	popup->SetTargetForItems(this);
	menufield = new BMenuField(r, "targetsbar", "Target:", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(80);
	SetGrey(menufield, kLtGray);
	top += 25.0;	
	
	// Filetype
	r.Set(left, top, left + 50, top + 16);
	caption = new BStringView(r, "", "Filetype"); 
	box->AddChild(caption);
	caption->SetFont(be_plain_font);
	SetGrey(caption, kLtGray);
	left += 140;

	// Extension
	r.Set(left, top, left + 50, top + 16);
	caption = new BStringView(r, "", "Extension"); 
	box->AddChild(caption);
	caption->SetFont(be_plain_font);
	SetGrey(caption, kLtGray);
	left += 47;

	// Make Stage
	r.Set(left, top, left + 60, top + 16);
	caption = new BStringView(r, "", "Make Stage"); 
	box->AddChild(caption);
	caption->SetFont(be_plain_font);
	SetGrey(caption, kLtGray);
	left += 60;

	// Has resources
	r.Set(left, top, left + 60, top + 16);
	caption = new BStringView(r, "", "Resources"); 
	box->AddChild(caption);
	caption->SetFont(be_plain_font);
	SetGrey(caption, kLtGray);
	left += 55;

	// Tool Name
	r.Set(left, top, left + 50, top + 16);
	caption = new BStringView(r, "", "Tool Name"); 
	box->AddChild(caption);
	caption->SetFont(be_plain_font);
	SetGrey(caption, kLtGray);
	top += 25.0;

	// List View
	r.Set(10, top, bounds.right - 10.0, bottomTop - 15.0);
	focusBox = new MFocusBox(r);						// Focus Box
	box->AddChild(focusBox);
	SetGrey(focusBox, kLtGray);
	focusBox->SetHighColor(kFocusBoxGray);

	r.InsetBy(10.0, 3.0);
	r.OffsetTo(3.0, 3.0);
	list = new MTargetListView(r, "listview");
	list->SetTarget(this);
	list->SetFocusBox(focusBox);

	frame = new BScrollView("frame", list, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_PLAIN_BORDER);		// For the border
	focusBox->AddChild(frame);
	list->SetFont(be_plain_font);
	font_height		info = list->GetCachedFontHeight();
	list->SetDefaultRowHeight(info.ascent + info.descent + info.leading + 1.0);
	fListView = list;
	
	top = bottomTop;
	left = 10.0;

	// File Type
	r.Set(left, top, left + 260, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "ft", "File Type:", "", msg);
	box->AddChild(textControl);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(80);
	textControl->SetTarget(this);
	SetGrey(textControl, kLtGray);	

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(64);
	DisallowInvalidChars(*text);
	text->SetTabWidth(text->StringWidth("M"));
	fFileTypeBox = text;
	top += 25.0;

	// Extension
	r.Set(left, top, left + 140, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "ext", "Extension:", "", msg);
	box->AddChild(textControl);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(80);
	textControl->SetTarget(this);
	SetGrey(textControl, kLtGray);	

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(7);
	DisallowInvalidChars(*text);
	text->SetTabWidth(text->StringWidth("M"));
	fFileExtensionBox = text;

	// Tool Name Popup
	top += 20.0;
	left = 10.0;
	r.Set(left - 2.0, top, 180, top + 16);

	popup = new BPopUpMenu("tools");
	fToolsPopup = popup;
	
	item = new BMenuItem("<none>", new BMessage(msgToolChosen));
	popup->AddItem(item);
	item = new BSeparatorItem;
	popup->AddItem(item);

	menufield = new BMenuField(r, "toolsbar", "Tool Name:", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(85);
	SetGrey(menufield, kLtGray);

	// Flags popup
	top += 3.0;
	left = 200.0;
	r.Set(200, top, left + 80, top + 20);

	popup = new BPopUpMenu("flags", false, false);
	fFlagsPopup = popup;

	item = new BMenuItem("Ignored by Make", new BMessage(msgFlagChosen));
	popup->AddItem(item);
	item = new BMenuItem("Precompile Stage", new BMessage(msgFlagChosen));
	popup->AddItem(item);
	item = new BMenuItem("Compile Stage", new BMessage(msgFlagChosen));
	popup->AddItem(item);
	item = new BMenuItem("Link Stage", new BMessage(msgFlagChosen));
	popup->AddItem(item);
	item = new BMenuItem("PostLink Stage", new BMessage(msgFlagChosen));
	popup->AddItem(item);
	popup->AddSeparatorItem();
	item = new BMenuItem("Has Resources", new BMessage(msgFlagChosen));
	popup->AddItem(item);

	popup->SetTargetForItems(this);

	BBitmap*	bitmap1 = LoadBitmap(sCheckMarkIcon, kCheckMarkWidth, kCheckMarkHeight);
	box->BeginPicture(new BPicture);
	box->DrawBitmap(bitmap1, B_ORIGIN);
	BPicture*	picture1 = box->EndPicture();
	delete bitmap1;
	
	pictureMenuField = new MPictureMenuField(r, "flags", "Flags:", picture1, popup);
	box->AddChild(pictureMenuField);
	pictureMenuField->SetFont(be_bold_font);
	pictureMenuField->SetDivider(40);
	r.Set(0.0, 0.0, kCheckMarkWidth, kCheckMarkHeight);
	pictureMenuField->SetPictureFrame(r);
	SetGrey(pictureMenuField, kLtGray);
	
	// Buttons
	// Add
	top = bottomTop;
	left = 290.0;
	r.Set(left, top, left + 75, top + 20);
	msg = new BMessage(msgAddTarget);
	button = new BButton(r, "add", "Add", msg); 
	box->AddChild(button);
	button->SetTarget(this);
	fAddButton = button;
	top += 20;

	// Change
	r.Set(left, top, left + 75, top + 20);
	msg = new BMessage(msgChangeTarget);
	button = new BButton(r, "add", "Change", msg); 
	box->AddChild(button);
	button->SetTarget(this);
	fChangeButton = button;
	top += 20;

	// Remove
	r.Set(left, top, left + 75, top + 20);
	msg = new BMessage(msgRemoveTarget);
	button = new BButton(r, "add", "Remove", msg); 
	box->AddChild(button);
	button->SetTarget(this);
	fRemoveButton = button;
	top += 20;
	
	// Disable all of the buttons initially
	fAddButton->SetEnabled(false);
	fChangeButton->SetEnabled(false);
	fRemoveButton->SetEnabled(false);
	
	ResetFields();
	
	fListView->MakeFocus();
}
