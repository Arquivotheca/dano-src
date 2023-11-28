//========================================================================
//	MProjectPrefsView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MProjectPrefsView.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "MTextControl.h"
#include "IDEMessages.h"
#include "CString.h"
#include "Utils.h"
#include "MPlugin.h"

#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Box.h>
#include <PopUpMenu.h>
#include <Window.h>
#include <Mime.h>
#include <Debug.h>

const char * titleProj = "Project/PPC Project";


// ---------------------------------------------------------------------------
//		 MProjectPrefsView
// ---------------------------------------------------------------------------
//	Constructor

MProjectPrefsView::MProjectPrefsView(
	BRect			inFrame)
	: MPlugInPrefsView(inFrame, "projectview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		 ~MProjectPrefsView
// ---------------------------------------------------------------------------
//	Destructor

MProjectPrefsView::~MProjectPrefsView()
{
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------

void
MProjectPrefsView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgTextChanged:
		case msgCompileCountChanged:
		case msgPopupChanged:
			ExtractInfo();
			break;

		default:
			MPlugInPrefsView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		 ExtractInfo
// ---------------------------------------------------------------------------

void
MProjectPrefsView::ExtractInfo()
{
	// Project kind
 	BMenuItem*		item = fProjectKindPopup->FindMarked();
 	ASSERT(item);
 	if (item)
 	{
		ProjectT		projectKind = (ProjectT) fProjectKindPopup->IndexOf(item);
		
		if (projectKind != fNewSettings.pProjectKind)
		{
			fNewSettings.pProjectKind = projectKind;
			switch (projectKind)
			{
				case AppType:
				case SharedLibType:
				case DriverType:
					fTypeBox->SetText(B_PEF_APP_MIME_TYPE);
					break;

				case LibraryType:
					fTypeBox->SetText(kCWLibMimeType);
					break;
			}
		}
	}

	// AppName
	memset(fNewSettings.pAppName, 0, sizeof(fNewSettings.pAppName));
	strcpy(fNewSettings.pAppName, fAppNameBox->Text());

 	// Type
	memset(fNewSettings.pAppType, 0, sizeof(fNewSettings.pAppType));
	strcpy(fNewSettings.pAppType, fTypeBox->Text());

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MProjectPrefsView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToBig();
	inOutMessage.AddData(kProjectNamePrefs, kMWLDPlugType, &fNewSettings, sizeof(fNewSettings), false);
	fNewSettings.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		 SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MProjectPrefsView::SetData(
	BMessage&	inMessage)
{
	ssize_t			length;
	ProjectPrefs*	inPrefs;

	if (B_NO_ERROR == inMessage.FindData(kProjectNamePrefs, kMWLDPlugType, (const void**) &inPrefs, &length))
	{
		ASSERT(length == sizeof(ProjectPrefs));

		fNewSettings = *inPrefs;
		fNewSettings.SwapBigToHost();
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		 UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MProjectPrefsView::UpdateValues()
{
	// Application name
	fAppNameBox->SetText(fNewSettings.pAppName);
	fAppNameBox->Select(0, 256);

	fTypeBox->SetText(fNewSettings.pAppType);
	fTypeBox->Select(0, 256);

	// Project Kind popup
	BMenuItem*		item = fProjectKindPopup->ItemAt(fNewSettings.pProjectKind);
	ASSERT(item);
	if (item && item != fProjectKindPopup->FindMarked())
		item->SetMarked(true);
}

// ---------------------------------------------------------------------------
//		 DoFactorySettings
// ---------------------------------------------------------------------------

void
MProjectPrefsView::DoFactorySettings()
{
	MDefaultPrefs::SetProjectDefaults(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 Title
// ---------------------------------------------------------------------------

const char *
MProjectPrefsView::Title()
{
	return titleProj;
}

// ---------------------------------------------------------------------------
//		 Targets
// ---------------------------------------------------------------------------

TargetT
MProjectPrefsView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		 GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MProjectPrefsView::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		 ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MProjectPrefsView::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kLinkUpdate;
}

// ---------------------------------------------------------------------------
//		 AttachedToWindow
// ---------------------------------------------------------------------------

void
MProjectPrefsView::AttachedToWindow()
{
	BRect			bounds = Bounds();
	BRect			r = bounds;
	BBox*			box;
	BPopUpMenu*		popup;
	BMessage*		msg;
	BTextView*		text;
	MTextControl*	textControl;
	BMenuField*		menufield;
	float			top = 20.0;
	const float		left = 10.0;

	// Box
	box = new BBox(r, "AppInfo");
	box->SetLabel("Application Info");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Project Type popup
	r.Set(left, top, 350, top + 16);

	popup = new BPopUpMenu("projectkind");
	fProjectKindPopup = popup;

	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Application", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Shared Library", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Library", msg));

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "applytobar", "Project Type", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(130);
	SetGrey(menufield, kLtGray);
	top += 30.0;

	// TextControls
	// File Name
	r.Set(left, top, bounds.right - left, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "st1", "File Name", "", msg);
	box->AddChild(textControl);
	textControl->SetTarget(this);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(70);
	SetGrey(textControl, kLtGray);

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(B_FILE_NAME_LENGTH);
	DisallowInvalidChars(*text);
	fAppNameBox = text;
	top += 25.0;

	// File Type
	r.Set(left, top, bounds.right - left, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "st3", "File Type", "", msg);
	box->AddChild(textControl);
	textControl->SetTarget(this);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(70);
	SetGrey(textControl, kLtGray);
	
	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(B_FILE_NAME_LENGTH);
	DisallowInvalidChars(*text);
	fTypeBox = text;
	top += 25.0;

	fAppNameBox->MakeFocus();
//	SetFocus(fAppNameBox);
}

// ---------------------------------------------------------------------------
//		 DoSave
// ---------------------------------------------------------------------------

void
MProjectPrefsView::DoSave()
{
}

// ---------------------------------------------------------------------------
//		 DoRevert
// ---------------------------------------------------------------------------

void
MProjectPrefsView::DoRevert()
{
}
// ---------------------------------------------------------------------------
//		 FilterKeyDown
// ---------------------------------------------------------------------------

bool
MProjectPrefsView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		 ValueChanged
// ---------------------------------------------------------------------------

void
MProjectPrefsView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

