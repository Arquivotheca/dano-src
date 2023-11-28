// --------------------------------------------------------------------------- 
/* 
	Run Preferences View
	 
	Copyright (c) 2001 Be Inc. All Rights Reserved. 
	 
	Author:	Alan Ellis 
			March 31, 2001 
 
	Options for the application running from the Run menu.
*/ 
// --------------------------------------------------------------------------- 
#include <stdlib.h>		// itol()

#include <Alert.h>
#include <Box.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <String.h>
#include <TextControl.h>

#include "MRunPrefsView.h"

#define CUSTOM_MALLOC_SELECT	'_cms'
#define CUSTOM_MALLOC_DESELECT	'_cmu'
#define CUSTOM_MALLOC_D_VALUE	'_cmv'
#define ARGS_SET				'args'
#define RUN_IN_TERMINAL			'_rit'

// --------------------------------------------------------------------------- 

MRunPrefsView::MRunPrefsView(MPreferencesWindow &inWindow, BRect inFrame)
              :MPreferencesView(inWindow, inFrame, "RunPrefsView"),
               fMallocDebugLevel(NULL),
               fCustomMallocDLevel(NULL),
               fCommandLineArgs(NULL),
               fRunInTerminal(NULL)
{
	SetPointers(&fOld, &fNew, sizeof(fOld));
}

// ---------------------------------------------------------------------------

MRunPrefsView::~MRunPrefsView()
{
}


// ---------------------------------------------------------------------------

void MRunPrefsView::AttachedToWindow()
{
	MPreferencesView::AttachedToWindow();
	SetViewColor(Parent()->ViewColor());
	
	BBox* box = new BBox(Bounds(), "RuPrefs");
	box->SetLabel("Run Options");
	AddChild(box);
	
	BRect r(12, 15, 100, 40);
	
	fRunInTerminal = new BCheckBox(r, "fRunInTerminal", "Run in Terminal", new BMessage(RUN_IN_TERMINAL));
	box->AddChild(fRunInTerminal);
	fRunInTerminal->ResizeToPreferred();
	fRunInTerminal->SetTarget(this);
	
	r = fRunInTerminal->Frame();

	r.OffsetTo(r.left - 2, r.bottom + 5);
	BPopUpMenu* levelMenu = new BPopUpMenu("levelMenu");
	levelMenu->AddItem(new BMenuItem("0", new BMessage(CUSTOM_MALLOC_DESELECT)));
	levelMenu->AddItem(new BMenuItem("1", new BMessage(CUSTOM_MALLOC_DESELECT)));
	levelMenu->AddItem(new BMenuItem("5", new BMessage(CUSTOM_MALLOC_DESELECT)));
	levelMenu->AddItem(new BMenuItem("10", new BMessage(CUSTOM_MALLOC_DESELECT)));
	levelMenu->AddItem(new BMenuItem("15", new BMessage(CUSTOM_MALLOC_DESELECT)));
	levelMenu->AddItem(new BMenuItem("Custom", new BMessage(CUSTOM_MALLOC_SELECT)));
	levelMenu->SetLabelFromMarked(true);
	levelMenu->SetTargetForItems(this);
	levelMenu->FindItem("Custom")->SetMarked(true);
	
	fMallocDebugLevel = new BMenuField(r, "fMallocDebugLevel", "Malloc Debug Level  ",
	                                   levelMenu);
	
	box->AddChild(fMallocDebugLevel);
	fMallocDebugLevel->SetDivider(be_plain_font->StringWidth(fMallocDebugLevel->Label()));
	fMallocDebugLevel->ResizeToPreferred();
	r = fMallocDebugLevel->Frame();
	
	r.OffsetTo(fMallocDebugLevel->Frame().right + 5, r.top);
	r.right = r.left + be_plain_font->StringWidth("WWW"); 
	fCustomMallocDLevel = new BTextControl(r, "fCustomMallocDLevel", "", "", new BMessage(CUSTOM_MALLOC_D_VALUE));
	fCustomMallocDLevel->Hide();
	fCustomMallocDLevel->SetDivider(0.0);
	box->AddChild(fCustomMallocDLevel);
	fCustomMallocDLevel->SetTarget(this);
	
	r.OffsetTo(10, r.bottom + 5);
	r.right = Bounds().Width() - r.left;
	fCommandLineArgs = new BTextControl(r, "fCommandLineArgs", "Command line arguments:", "",
	                                    new BMessage(ARGS_SET), B_FOLLOW_LEFT_RIGHT);
	fCommandLineArgs->SetDivider(be_plain_font->StringWidth("Command line arguments: "));
	box->AddChild(fCommandLineArgs);
	fCommandLineArgs->SetTarget(this);
	
	// todo: Custom environment values?
}

// ---------------------------------------------------------------------------

void MRunPrefsView::MessageReceived(BMessage *message)
{
	switch( message->what )
	{
		case CUSTOM_MALLOC_SELECT:
			if( true == fCustomMallocDLevel->IsHidden() )
			{
				fCustomMallocDLevel->Show();
			}
			
			fNew.MallocDebugLevel = atol(fCustomMallocDLevel->Text());
			break;
			
		case CUSTOM_MALLOC_DESELECT:
			if( false == fCustomMallocDLevel->IsHidden() )
			{
				fCustomMallocDLevel->Hide();
			}
			
			fNew.MallocDebugLevel = atol(fMallocDebugLevel->Menu()->FindMarked()->Label());
//			fCustomMallocDLevel->SetText("");
			break;
		
		case ARGS_SET:
			if( BString(fCommandLineArgs->Text()).Length() > RUN_ARGS_SIZE )
			{
				BString info("There is currently a limit of ");
				info << RUN_ARGS_SIZE << " bytes of program arguments allowed at this time.";
				(new BAlert("BeIDE Limitation",  info.String(), "Dang"))->Go();
				return;
			}
			else
			{
				strcpy(fNew.Args, fCommandLineArgs->Text());
			}
			break;
			
		case CUSTOM_MALLOC_D_VALUE:
			fNew.MallocDebugLevel = atol(fCustomMallocDLevel->Text());
			break;
		
		case RUN_IN_TERMINAL:
			fNew.RunInTerminal = (bool)fRunInTerminal->Value();
			break;
				
		default:
			MPreferencesView::MessageReceived(message);
			return;
	}	

	ValueChanged();
}

// ---------------------------------------------------------------------------

void MRunPrefsView::LastCall()
{
	MPreferencesView::LastCall();
}

// ---------------------------------------------------------------------------

void MRunPrefsView::DoSave()
{
	MPreferencesView::DoSave();
}

// ---------------------------------------------------------------------------

void MRunPrefsView::DoRevert()
{
	MPreferencesView::DoRevert();
}

// ---------------------------------------------------------------------------

void MRunPrefsView::DoFactorySettings()
{
	fNew.RunInTerminal = false;
	fNew.MallocDebugLevel = 0;
	strcpy(fNew.Args, "");

	UpdateValues();	
	MPreferencesView::DoFactorySettings();	
}

// ---------------------------------------------------------------------------

bool MRunPrefsView::ProjectRequiresUpdate(UpdateType inType)
{
	return MPreferencesView::ProjectRequiresUpdate(inType);
}

// ---------------------------------------------------------------------------

void MRunPrefsView::GetData(BMessage &inOutMessage, bool isProxy)
{
	inOutMessage.PrintToStream();

	if( isProxy )
	{
		// Add the correct data type so that it gets filled elsewhere.
		int32		ignore(0);
		inOutMessage.AddData(kRunPrefs, kMWPrefs, &ignore, sizeof(ignore), false);
	}
	else
	{
		inOutMessage.AddData(kRunPrefs, kMWPrefs, &fNew, sizeof(fNew));
	}
}

// ---------------------------------------------------------------------------

void MRunPrefsView::SetData(BMessage& inOutMessage)
{
	inOutMessage.PrintToStream();

	ssize_t			length;
	RunPreferences*	prefs;

	if (B_NO_ERROR == inOutMessage.FindData(kRunPrefs, kMWPrefs, (const void**)&prefs, &length))
	{		
		fNew = *prefs;
		
		UpdateValues();
	}
	
	SetDirty(false);
}

// ---------------------------------------------------------------------------

const char* MRunPrefsView::Title()
{
	return "Run Options";
}

// ---------------------------------------------------------------------------

TargetT 
MRunPrefsView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------

MakeActionT MRunPrefsView::Actions()
{
	return MPreferencesView::Actions();
}

// ---------------------------------------------------------------------------

void MRunPrefsView::ValueChanged()
{
	MPreferencesView::ValueChanged();
}

// ---------------------------------------------------------------------------

void MRunPrefsView::SetDirty(bool inDirty)
{
	MPreferencesView::SetDirty(inDirty);
}

// ---------------------------------------------------------------------------

bool MRunPrefsView::IsDirty()
{
	return MPreferencesView::IsDirty();
}

// ---------------------------------------------------------------------------

void MRunPrefsView::UpdateValues()
{
	fRunInTerminal->SetValue(fNew.RunInTerminal);
	fCommandLineArgs->SetText(fNew.Args);

	// find appropriate menu item for value.
	// if none, set as custom
	BString level;
	level << fNew.MallocDebugLevel;
	
	BMenuItem* item = fMallocDebugLevel->Menu()->FindItem(level.String());
	
	if (NULL == item)
	{
		if( true == fCustomMallocDLevel->IsHidden() )
		{
			fCustomMallocDLevel->Show();
		}
		
		fCustomMallocDLevel->SetText(level.String());
	}
	else
	{
		item->SetMarked(true);
	}

	MPreferencesView::UpdateValues();
}

// ---------------------------------------------------------------------------

