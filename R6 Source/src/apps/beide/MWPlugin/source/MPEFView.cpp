//========================================================================
//	MPEFView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MPEFView.h"
#include "MDefaultPrefs.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "MPlugin.h"

#include <Box.h>
#include <Menu.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Window.h>
#include <Debug.h>

const char * titlePEF = "Linker/PPC PEF";

// ---------------------------------------------------------------------------
//		 MPEFView
// ---------------------------------------------------------------------------
//	Constructor

MPEFView::MPEFView(
	BRect			inFrame)
	: MPlugInPrefsView(inFrame, "pefview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		 ~MPEFView
// ---------------------------------------------------------------------------
//	Destructor

MPEFView::~MPEFView()
{
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------

void
MPEFView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
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
MPEFView::ExtractInfo()
{
 	BMenuItem*		item = fExportSymbolsPopup->FindMarked();
 	ASSERT(item);
 	if (item)
 		fNewSettings.pExportSymbols = (ExportSymbolsT) fExportSymbolsPopup->IndexOf(item);

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MPEFView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToBig();
	inOutMessage.AddData(kPEFPrefs, kMWLDPlugType, &fNewSettings, sizeof(fNewSettings), false);
	fNewSettings.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		 SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MPEFView::SetData(
	BMessage&	inMessage)
{
	ssize_t			length;
	PEFPrefs*		prefs;

	if (B_NO_ERROR == inMessage.FindData(kPEFPrefs, kMWLDPlugType, (const void**) &prefs, &length))
	{
		ASSERT(length == sizeof(PEFPrefs));

		fNewSettings = *prefs;
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
MPEFView::UpdateValues()
{
	// Export Symbols popup
	BMenuItem*		item = fExportSymbolsPopup->ItemAt(fNewSettings.pExportSymbols);
	ASSERT(item);
	if (item && item != fExportSymbolsPopup->FindMarked())
		item->SetMarked(true);
}

// ---------------------------------------------------------------------------
//		 DoFactorySettings
// ---------------------------------------------------------------------------

void
MPEFView::DoFactorySettings()
{
	MDefaultPrefs::SetPEFDefaults(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MPEFView::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		 Title
// ---------------------------------------------------------------------------

const char *
MPEFView::Title()
{
	return titlePEF;
}

// ---------------------------------------------------------------------------
//		 Targets
// ---------------------------------------------------------------------------

TargetT
MPEFView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		 DoSave
// ---------------------------------------------------------------------------

void
MPEFView::DoSave()
{
}

// ---------------------------------------------------------------------------
//		 DoRevert
// ---------------------------------------------------------------------------

void
MPEFView::DoRevert()
{
}

// ---------------------------------------------------------------------------
//		 FilterKeyDown
// ---------------------------------------------------------------------------

bool
MPEFView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		 ValueChanged
// ---------------------------------------------------------------------------

void
MPEFView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		 ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MPEFView::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kLinkUpdate;
}

// ---------------------------------------------------------------------------
//		 AttachedToWindow
// ---------------------------------------------------------------------------

void
MPEFView::AttachedToWindow()
{
	BRect			r;
	BBox*			box;
	BPopUpMenu*		popup;
	BMessage*		msg;
	BMenuField*			menufield;
	float			top = 20.0;

	// Box
	r = Bounds();
	box = new BBox(r, "pefbox");
	box->SetLabel("PEF Settings");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Export Symbols popup
	r.Set(10, top, 300, top + 16);

	popup = new BPopUpMenu("symbolsmenu");
	fExportSymbolsPopup = popup;

	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("None", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("All Globals", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Use #pragma", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Use " B_UTF8_OPEN_QUOTE ".exp" B_UTF8_CLOSE_QUOTE " file", msg));

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "st1", "Export Symbols", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(120);
}
