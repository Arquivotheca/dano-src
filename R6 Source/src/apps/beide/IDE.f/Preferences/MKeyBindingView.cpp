//========================================================================
//	MKeyBindingView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MKeyBindingView.h"
#include "MKeyBindingsListView.h"
#include "MBindingWindow.h"
#include "MIDECommandList.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "MFocusBox.h"
#include "MPrefsListView.h"
#include "MTextControl.h"
#include "IDEMessages.h"
#include "Utils.h"
#include "MAlert.h"

#include <Button.h>
#include <File.h>
#include <NodeInfo.h>
#include <FilePanel.h>
#include <ScrollView.h>
#include <Box.h>

const char * titleKeyBindings = "Key Bindings";

const CommandT		msgImport = 13000;
const CommandT		msgExport = 13001;

// ---------------------------------------------------------------------------
//		MKeyBindingView
// ---------------------------------------------------------------------------
//	Constructor

MKeyBindingView::MKeyBindingView(
	MPreferencesWindow&	inWindow,
	BRect			inFrame)
	: MPreferencesView(inWindow, inFrame, "keybindingview"),
	fBindingWindow(nil), fNewBindingManager(nil),  fOldBindingManager(nil),
	fExportPanel(nil), fImportPanel(nil)
{
}

// ---------------------------------------------------------------------------
//		~MKeyBindingView
// ---------------------------------------------------------------------------
//	Destructor

MKeyBindingView::~MKeyBindingView()
{
	CloseBindingWindow();
	ClosePanels();

	delete fNewBindingManager;
	delete fOldBindingManager;
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MKeyBindingView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgSetBinding:
			OpenBindingWindow();
			fBindingWindow->PostMessage(inMessage);
			break;

		case msgUpdateBinding:
			KeyBindingInfo*		info;
			ssize_t				size;
			KeyBindingContext	context;

			if (B_OK == inMessage->FindData(kBindingInfo, kBindInfoType, (const void **)&info, &size) &&
				B_OK == inMessage->FindInt32(kBindingContext, (int32*) &context))
				UpdateBinding(context, *info);
			break;

		case msgBindingWindowClosed:
			CloseBindingWindow();
			break;

		case msgTextChanged:
		{
			int32		prefixTimeoutTicks = fTextControl->GetValue();
			fNewBindingManager->SetPrefixTimeout(TicksToMicroSeconds(prefixTimeoutTicks));
			ValueChanged();
			break;
		}
		
		case msgImport:
			StartImport();
			break;

		case msgExport:
			StartExport();
			break;

		case B_SAVE_REQUESTED:		// from the export panel
			DoExport(inMessage);
			break;

		case B_REFS_RECEIVED:		// from the import panel
			DoImport(inMessage);
			break;

		default:
			MPreferencesView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		OpenBindingWindow
// ---------------------------------------------------------------------------
//	Show the change binding window or activate it if it is already open.

void
MKeyBindingView::OpenBindingWindow()
{
	if (fBindingWindow != nil)
		fBindingWindow->Activate();
	else
	{
		fBindingWindow = new MBindingWindow(this, *fNewBindingManager);
		fBindingWindow->Show();
	}
}

// ---------------------------------------------------------------------------
//		CloseBindingWindow
// ---------------------------------------------------------------------------
//	Delete the change binding window.

void
MKeyBindingView::CloseBindingWindow()
{
	if (fBindingWindow != nil && fBindingWindow->Lock())
	{
		fBindingWindow->Quit();
		fBindingWindow = nil;
	}
}

// ---------------------------------------------------------------------------
//		UpdateBinding
// ---------------------------------------------------------------------------
//	The change binding window has asked us to update a binding.

void
MKeyBindingView::UpdateBinding(
	KeyBindingContext			inContext,
	const KeyBindingInfo&		inInfo)
{
	CommandT		duplicate;

	if (! fNewBindingManager->BindingExists(inContext, inInfo, duplicate))
	{
		fListView->UpdateBinding(inContext, inInfo, *fNewBindingManager);

		ValueChanged();
	}
	else
	{
		// Report duplicate binding error
		CommandInfo		info;
		MIDECommandList::GetCommandInfo(inInfo.cmdNumber, info);
		String			text = "Couldn't assign a binding to "B_UTF8_OPEN_QUOTE;
		text +=	info.name;
		text +=	B_UTF8_CLOSE_QUOTE" because the binding you entered is already assigned to the command "B_UTF8_OPEN_QUOTE;
		MIDECommandList::GetCommandInfo(duplicate, info);
		text +=	info.name;
		text +=	B_UTF8_CLOSE_QUOTE".";

		MAlert		alert(text);
		alert.Go();
	}
}

// ---------------------------------------------------------------------------
//		ClosePanels
// ---------------------------------------------------------------------------

void
MKeyBindingView::ClosePanels()
{
	if (fExportPanel != nil)
	{
		delete fExportPanel;
		fExportPanel = nil;
	}
	if (fImportPanel != nil)
	{
		delete fImportPanel;
		fImportPanel = nil;
	}
}

// ---------------------------------------------------------------------------
//		StartImport
// ---------------------------------------------------------------------------

void
MKeyBindingView::StartImport()
{
	if (fImportPanel != nil)
	{
		ShowAndActivate(fImportPanel->Window());
	}
	else
	{
		BMessenger		me(this);
		BWindow*		panel;

		fImportPanel = new BFilePanel(B_OPEN_PANEL, &me);

		panel = fImportPanel->Window();

		panel->SetTitle("Import Key Bindings");

		fImportPanel->Show();
	}
}

// ---------------------------------------------------------------------------
//		StartExport
// ---------------------------------------------------------------------------
//	Show the export panel.

void
MKeyBindingView::StartExport()
{
	if (fExportPanel != nil)
	{
		ShowAndActivate(fExportPanel->Window());
	}
	else
	{
		BMessenger		me(this);
		BWindow*		panel;

		fExportPanel = new BFilePanel(B_SAVE_PANEL, &me);

		panel = fExportPanel->Window();

		panel->SetTitle("Export Key Bindings");
		fExportPanel->SetSaveText("BeIDE Key Bindings");

		fExportPanel->Show();
	}
}

// ---------------------------------------------------------------------------
//		DoImport
// ---------------------------------------------------------------------------
//	Import the key binding data from the file specified in the import panel.

void
MKeyBindingView::DoImport(BMessage* inMessage)
{
	entry_ref ref;
	status_t err;

	if (inMessage->FindRef("refs", &ref) == B_OK)
	{
		BFile file(&ref, B_READ_WRITE);
		err = file.InitCheck();
	
		// Currently we don't check the mime type because when you 
		// copy the files around, you lose the mime attribute.
		// We need to put in the header the mime information so that
		// the sniffer can figure it out (when the sniffer is extensible in r5)
		// mime_t mimeType = { '\0' };
		// BNodeInfo mimefile(&file);
		// err = mimefile.GetType(mimeType);
		// check that mimeType == kKeyBindingsMimeType
		
		if (err == B_NO_ERROR) {
			off_t size = 0;
			err = file.GetSize(&size);
			
			BMallocIO data;
			err = data.SetSize(size);
			
			// minor hack, I don't think BMallocIO is designed to allow you
			// to write directly to its buffer
			err = file.Read((void*)data.Buffer(), size) == size ? B_OK : B_ERROR;
			
			BMemoryIO memData(data.Buffer(), data.BufferLength());
			if (err == B_OK && fNewBindingManager->SetKeyBindingsBig(memData) == true) {
				this->UpdateValues();
				this->ValueChanged();
			}
			else {
				err = B_ERROR;
			}
		}
		
		if (err != B_OK) {
			String text = "Problem importing key bindings from file "B_UTF8_OPEN_QUOTE;
			text +=	ref.name;
			text +=	B_UTF8_CLOSE_QUOTE".  Keeping current settings.";
			MAlert alert(text);
			alert.Go();
		}
	}
}

// ---------------------------------------------------------------------------
//		DoExport
// ---------------------------------------------------------------------------
//	Write the key binding data to the file specified in the export panel.

void
MKeyBindingView::DoExport(
	BMessage*	inMessage)
{
	entry_ref directory;
	inMessage->FindRef("directory", &directory);

	const char *name = NULL;
	inMessage->FindString("name", &name);

	BDirectory		dir(&directory);	// Specify directory
	BFile			file;
	status_t		err = dir.CreateFile(name, &file);// Specify file object and create it on disk
	
	if (err == B_NO_ERROR)
	{
		BMallocIO		data;

		fNewBindingManager->GetKeyBindingsBig(data);

		// Write the data out
		file.Write(data.Buffer(), data.BufferLength());
		BNodeInfo		mime(&file);
		
		mime.SetType(kKeyBindingsMimeType);
	}
}

// ---------------------------------------------------------------------------
//		LastCall
// ---------------------------------------------------------------------------
//	kill the binding window when we're hidden.  Called from 
//	MPreferencesView::Hide()

void
MKeyBindingView::LastCall()
{
	CloseBindingWindow();
	ClosePanels();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MKeyBindingView::GetData(
	BMessage&	inOutMessage,
	bool		isProxy)
{
	if (isProxy)
	{
		int32		ignore;
		inOutMessage.AddData(kKeyBindingPrefs, kMWPrefs, &ignore, sizeof(ignore), false);
	}
	else
	{
		ASSERT(fNewBindingManager != nil);
		
		BMallocIO		data;

		fNewBindingManager->GetKeyBindingsHost(data);
		status_t	err = inOutMessage.AddData(kKeyBindingPrefs, kMWPrefs, data.Buffer(), data.BufferLength(), false);
	}
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MKeyBindingView::SetData(
	BMessage&	inMessage)
{
	ssize_t		length;
	const void*	data;

	if (B_NO_ERROR == inMessage.FindData(kKeyBindingPrefs, kMWPrefs, &data, &length))
	{
		BMemoryIO		memData(data, length);

		if (fNewBindingManager == nil)
		{
			fNewBindingManager = new MKeyBindingManager(memData, kHostEndian);
			fOldBindingManager = new MKeyBindingManager(memData, kHostEndian);
		}
		else
		{
			fNewBindingManager->SetKeyBindingsHost(memData);
			fOldBindingManager->SetKeyBindingsHost(memData);
		}

		fListView->SetBindingManager(fNewBindingManager);

		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MKeyBindingView::UpdateValues()
{
	fListView->UpdateAllBindings(*fNewBindingManager);
	
	// Set the prefix timeout text
	int32		prefixTimeoutTicks = MicroSecondsToTicks(fNewBindingManager->PrefixTimeout());
	
	if (fTextControl->GetValue() != prefixTimeoutTicks)
	{
		String		ticks = prefixTimeoutTicks;
		fTimeoutBox->SetText(ticks);
	}
}

// ---------------------------------------------------------------------------
//		DoSave
// ---------------------------------------------------------------------------
//	Called after the view's contents have been sent to the target.

void
MKeyBindingView::DoSave()
{
	*fOldBindingManager = *fNewBindingManager;

	SetDirty(false);
}

// ---------------------------------------------------------------------------
//		DoRevert
// ---------------------------------------------------------------------------
//	Called when the revert button has been hit.

void
MKeyBindingView::DoRevert()
{
	// Copy the old settings struct to the new settings struct,
	// update the controls in the window and check if we're dirty
	*fNewBindingManager = *fOldBindingManager;

	fListView->Invalidate();

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		ValueChanged
// ---------------------------------------------------------------------------
//	Notify the preferences window that one of the values in the view has 
//	changed.

void
MKeyBindingView::ValueChanged()
{
	bool		isdirty = false;
	
	BMallocIO	newData;
	BMallocIO	oldData;

	fNewBindingManager->GetKeyBindingsHost(newData);
	fOldBindingManager->GetKeyBindingsHost(oldData);

	isdirty = (newData.BufferLength() != oldData.BufferLength()) ||
				0 != memcmp(newData.Buffer(), oldData.Buffer(), newData.BufferLength());
	
	if (isdirty != IsDirty())
	{
		SetDirty(isdirty);

		Window()->PostMessage(msgPrefsViewModified);
	}
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------

void
MKeyBindingView::DoFactorySettings()
{
	BMallocIO		data;
	
	MKeyBindingManager::BuildDefaultKeyBindings(data);

	BMemoryIO		memData(data.Buffer(), data.BufferLength());

	fNewBindingManager->SetKeyBindingsHost(memData);
	fListView->Invalidate();

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
MKeyBindingView::Title()
{
	return titleKeyBindings;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MKeyBindingView::Targets()
{
	return kMWDefaults;
}

// ---------------------------------------------------------------------------
//		ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MKeyBindingView::ProjectRequiresUpdate(
	UpdateType /*inType*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MKeyBindingView::AttachedToWindow()
{
	BRect			bounds;
	BRect			r;
	BBox*			box;
	MFocusBox*		focusBox;
	MKeyBindingsListView*	list;
	MTextControl*	textControl;
	BScrollView*	frame;
	BMessage*		msg;
	BButton*		button;
	float			top = 20.0;

	// Box
	bounds = Bounds();
	r = bounds;
	box = new BBox(r, "bindingsbox");
	box->SetLabel("Key Bindings");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Key Bindings list box
	r.Set(5.0, top, bounds.right - 5.0, bounds.bottom - 35.0);

	focusBox = new MFocusBox(r);						// Focus Box
	box->AddChild(focusBox);
	SetGrey(focusBox, kLtGray);
	focusBox->SetHighColor(kFocusBoxGray);

	r.InsetBy(10.0, 3.0);
	r.OffsetTo(3.0, 3.0);
	list = new MKeyBindingsListView(r, "listview", this);
	list->SetTarget(this);
	list->SetFocusBox(focusBox);

	frame = new BScrollView("frame", list, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS, false, true, B_PLAIN_BORDER);		// For the border
	focusBox->AddChild(frame);
	list->SetFont(be_plain_font);
	font_height		info = list->GetCachedFontHeight();
	list->SetDefaultRowHeight(info.ascent + info.descent + info.leading + 1.0);
	fListView = list;

	// Prefix Key timeout textcontrol
	top = bounds.bottom - 30.0;
	r.Set(20, top, 180, top + 16.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "tc1", "Prefix Key Timeout:", "", msg);
	box->AddChild(textControl);
	textControl->SetTarget(this);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(120);
	SetGrey(textControl, kLtGray);
	fTextControl = textControl;

	fTimeoutBox = (BTextView*) textControl->ChildAt(0);
	fTimeoutBox->SetMaxBytes(3);
	DisallowInvalidChars(*fTimeoutBox);
	DisallowNonDigitChars(*fTimeoutBox);
	fTimeoutBox->SetTabWidth(fTimeoutBox->StringWidth("M"));

	float	left = bounds.right - 10 - 80 - 15 - 80;
	float	right = left + 80;
	// Buttons
	// Import
	r.Set(left, top, right, top + 20.0);
	msg = new BMessage(msgImport);
	button = new BButton(r, "import", "Import...", msg); 
	box->AddChild(button);
	button->SetTarget(this);

	left = right + 15;
	right = left + 80;
	// Export
	r.Set(left, top, right, top + 20.0);
	msg = new BMessage(msgExport);
	button = new BButton(r, "export", "Export...", msg); 
	box->AddChild(button);
	button->SetTarget(this);

	fListView->BuildBindingList();
}
