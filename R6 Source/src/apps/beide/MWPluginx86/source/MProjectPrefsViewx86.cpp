//========================================================================
//	MProjectPrefsViewx86.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MProjectPrefsViewx86.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "MTextControl.h"
#include "IDEMessages.h"
#include "CString.h"
#include "Utils.h"

const char * titleProj = "Project/x86 Project";


// ---------------------------------------------------------------------------
//		� MProjectPrefsViewx86
// ---------------------------------------------------------------------------
//	Constructor

MProjectPrefsViewx86::MProjectPrefsViewx86(
	BRect			inFrame)
	: MPlugInPrefsView(inFrame, "projectview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		� ~MProjectPrefsViewx86
// ---------------------------------------------------------------------------
//	Destructor

MProjectPrefsViewx86::~MProjectPrefsViewx86()
{
}

// ---------------------------------------------------------------------------
//		� MessageReceived
// ---------------------------------------------------------------------------

void
MProjectPrefsViewx86::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgTextChanged:
		case msgPopupChanged:
			ExtractInfo();
			break;

		default:
			MPlugInPrefsView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		� ExtractInfo
// ---------------------------------------------------------------------------

void
MProjectPrefsViewx86::ExtractInfo()
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
#if __INTEL__
					// this ifdef is only needed prior to R3
					fTypeBox->SetText(B_PE_APP_MIME_TYPE);
#else
					fTypeBox->SetText("application/x-vnd.Be-peexecutable");
#endif
					break;

				case LibraryType:
					fTypeBox->SetText(kCWLinkLibx86MimeType);
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

 	// Base Address
	uint32		value = 0x80000000;

	sscanf(fBaseBox->Text(), "%lx", &value);
 	fNewSettings.pBaseAddress = value;

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		� GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MProjectPrefsViewx86::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToLittle();
	inOutMessage.AddData(kProjectPrefsx86, kMWCCx86Type, &fNewSettings, sizeof(fNewSettings), false);
	fNewSettings.SwapLittleToHost();
}

// ---------------------------------------------------------------------------
//		� SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MProjectPrefsViewx86::SetData(
	BMessage&	inMessage)
{
	ssize_t				length;
	ProjectPrefsx86*	inPrefs;

	if (B_NO_ERROR == inMessage.FindData(kProjectPrefsx86, kMWCCx86Type, &inPrefs, &length))
	{
		ASSERT(length == sizeof(ProjectPrefs));

		fNewSettings = *inPrefs;
		fNewSettings.SwapLittleToHost();
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		� UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MProjectPrefsViewx86::UpdateValues()
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

	// Base address
	char		baseAddress[24] = { '\0' };
	sprintf(baseAddress, "%#010x", fNewSettings.pBaseAddress);
	fBaseBox->SetText(baseAddress);
	fBaseBox->Select(0, 256);
}

// ---------------------------------------------------------------------------
//		� DoFactorySettings
// ---------------------------------------------------------------------------

void
MProjectPrefsViewx86::DoFactorySettings()
{
	MDefaultPrefs::SetProjectDefaultsx86(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		� Title
// ---------------------------------------------------------------------------

const char *
MProjectPrefsViewx86::Title()
{
	return titleProj;
}

// ---------------------------------------------------------------------------
//		� Targets
// ---------------------------------------------------------------------------

TargetT
MProjectPrefsViewx86::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		� GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MProjectPrefsViewx86::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		� ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MProjectPrefsViewx86::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kLinkUpdate;
}

// ---------------------------------------------------------------------------
//		� AttachedToWindow
// ---------------------------------------------------------------------------

void
MProjectPrefsViewx86::AttachedToWindow()
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

	menufield = new BMenuField(r, "projtype", "Project Type", popup);
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

	// Base Address
	r.Set(left, top, bounds.right - left, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "st4", "Base Address", "", msg);
	box->AddChild(textControl);
	textControl->SetTarget(this);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(90);
	SetGrey(textControl, kLtGray);

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(10);
	DisallowInvalidChars(*text);
	fBaseBox = text;
	fBaseControl = textControl;

	fAppNameBox->MakeFocus();
}

// ---------------------------------------------------------------------------
//		� DoSave
// ---------------------------------------------------------------------------

void
MProjectPrefsViewx86::DoSave()
{
}

// ---------------------------------------------------------------------------
//		� DoRevert
// ---------------------------------------------------------------------------

void
MProjectPrefsViewx86::DoRevert()
{
}
// ---------------------------------------------------------------------------
//		� FilterKeyDown
// ---------------------------------------------------------------------------

bool
MProjectPrefsViewx86::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		� ValueChanged
// ---------------------------------------------------------------------------

void
MProjectPrefsViewx86::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

