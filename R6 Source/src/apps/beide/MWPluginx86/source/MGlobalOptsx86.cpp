//========================================================================
//	MGlobalOptsx86.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MGlobalOptsx86.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "Utils.h"
#include "IDEMessages.h"
#include "CString.h"

const char * titleGlobalOpts = "Code Generation/x86 Global Optimizations";

// ---------------------------------------------------------------------------
//		¥ MGlobalOptsx86
// ---------------------------------------------------------------------------
//	Constructor

MGlobalOptsx86::MGlobalOptsx86(
	BRect				inFrame)
	: MPlugInPrefsView(inFrame, "x86globalopts", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		¥ ~MGlobalOptsx86
// ---------------------------------------------------------------------------
//	Destructor

MGlobalOptsx86::~MGlobalOptsx86()
{
}

// ---------------------------------------------------------------------------
//		¥ MessageReceived
// ---------------------------------------------------------------------------

void
MGlobalOptsx86::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgCheckBoxChanged:
			UpdateCheckBoxes();
			break;

		default:
			MPlugInPrefsView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		¥ UpdateCheckBoxes
// ---------------------------------------------------------------------------

void
MGlobalOptsx86::UpdateCheckBoxes()
{
	fNewSettings.pOptimizeForSpace = fSpaceOptimizationsCB->Value() == 1;
	fNewSettings.pOptimizeForSpeed = fSpeedOptimizationsCB->Value() == 1;
	fNewSettings.pCommonSubExpressions = fCSECB->Value() == 1;
	fNewSettings.pLoopInvariants = fLoopInvariantsCB->Value() == 1;
	fNewSettings.pCopyPropagation = fCopyPropagationCB->Value() == 1;
	fNewSettings.pDeadStoreElimination = fDeadStoreEliminationCB->Value() == 1;
	fNewSettings.pStrenghtReduction = fStrenghReduction->Value() == 1;
	fNewSettings.pDeadCodeElimination = fDeadCodeEliminationCB->Value() == 1;
	fNewSettings.pLifetimeAnalysis = fLifetimeAnalysisCB->Value() == 1;

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MGlobalOptsx86::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToLittle();
	inOutMessage.AddData(kGlobalOptsPrefsx86, kMWCCx86Type, &fNewSettings, sizeof(fNewSettings));
	fNewSettings.SwapLittleToHost();
}

// ---------------------------------------------------------------------------
//		¥ SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MGlobalOptsx86::SetData(
	BMessage&	inMessage)
{
	GlobalOptimizations*		prefs;
	long						length;

	if (B_NO_ERROR == inMessage.FindData(kGlobalOptsPrefsx86, kMWCCx86Type, &prefs, &length))
	{
		ASSERT(length == sizeof(GlobalOptimizations));

		fNewSettings = *prefs;
		fNewSettings.SwapLittleToHost();
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		¥ UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MGlobalOptsx86::UpdateValues()
{
	// CheckBoxes
	fSpaceOptimizationsCB->SetValue(fNewSettings.pOptimizeForSpace);
	fCSECB->SetValue(fNewSettings.pCommonSubExpressions);
	fCopyPropagationCB->SetValue(fNewSettings.pCopyPropagation);
	fStrenghReduction->SetValue(fNewSettings.pStrenghtReduction);
	fLifetimeAnalysisCB->SetValue(fNewSettings.pLifetimeAnalysis);
	fSpeedOptimizationsCB->SetValue(fNewSettings.pOptimizeForSpeed);
	fLoopInvariantsCB->SetValue(fNewSettings.pLoopInvariants);
	fDeadStoreEliminationCB->SetValue(fNewSettings.pDeadStoreElimination);
	fDeadCodeEliminationCB->SetValue(fNewSettings.pDeadCodeElimination);
}

// ---------------------------------------------------------------------------
//		¥ DoFactorySettings
// ---------------------------------------------------------------------------

void
MGlobalOptsx86::DoFactorySettings()
{
	MDefaultPrefs::SetGlobalOptsDefaults(fNewSettings);

	UpdateValues();
//	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MGlobalOptsx86::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		¥ Title
// ---------------------------------------------------------------------------

const char *
MGlobalOptsx86::Title()
{
	return titleGlobalOpts;
}

// ---------------------------------------------------------------------------
//		¥ Targets
// ---------------------------------------------------------------------------

TargetT
MGlobalOptsx86::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		¥ ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MGlobalOptsx86::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kCompileUpdate;
}

// ---------------------------------------------------------------------------
//		¥ DoSave
// ---------------------------------------------------------------------------

void
MGlobalOptsx86::DoSave()
{
}

// ---------------------------------------------------------------------------
//		¥ DoRevert
// ---------------------------------------------------------------------------

void
MGlobalOptsx86::DoRevert()
{
}

// ---------------------------------------------------------------------------
//		¥ FilterKeyDown
// ---------------------------------------------------------------------------

bool
MGlobalOptsx86::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		¥ ValueChanged
// ---------------------------------------------------------------------------

void
MGlobalOptsx86::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		¥ AttachedToWindow
// ---------------------------------------------------------------------------

void
MGlobalOptsx86::AttachedToWindow()
{
	BRect			bounds = Bounds();
	BRect			r;
	BBox*			box;
	BCheckBox*		checkBox;
	BMessage*		msg;
	float			top = 15.0;
	float			left = 10.0;
	float			right = 190.0;

	// Box
	r = Bounds();
	box = new BBox(r, "processor");
	box->SetLabel("x86 Global Optimizations");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Optimize for Space
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb1", "Optimize for Space", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fSpaceOptimizationsCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Common Subexpression removal
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb2", "Common Subexpression Removal", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fCSECB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Copy/Const Propagation
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb3", "Copy/Const Propagation", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fCopyPropagationCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Strength Reduction
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb4", "Strength Reduction", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fStrenghReduction = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Lifetime Analysis
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb4", "Lifetime Analysis", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fLifetimeAnalysisCB = checkBox;
	SetGrey(checkBox, kLtGray);

	top = 15.0;
	left = 200.0;
	right = bounds.right - 10.0;

	// Optimize for Speed
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb1", "Optimize for Speed", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fSpeedOptimizationsCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Hoist Loop Invariants
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb2", "Hoist Loop Invariants", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fLoopInvariantsCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Dead Store Elimination
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb3", "Dead Store Elimination", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fDeadStoreEliminationCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Dead Code Elimination
	r.Set(left, top, right, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb4", "Dead Code Elimination", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fDeadCodeEliminationCB = checkBox;
	SetGrey(checkBox, kLtGray);

	UpdateCheckBoxes();
}

