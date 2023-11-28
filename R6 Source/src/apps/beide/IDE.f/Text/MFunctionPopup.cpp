//========================================================================
//	MFunctionPopup.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Modified from CW7 version of MWParseFunctions.c

#include <string.h>

#include "MFunctionPopup.h"
#include "MIDETextView.h"
#include "IDEMessages.h"

#include <InterfaceKit.h>

bool	MFunctionPopup::sSort;

// ---------------------------------------------------------------------------
//		MFunctionPopup
// ---------------------------------------------------------------------------

MFunctionPopup::MFunctionPopup(
	MIDETextView&		inTextView,
	const BWindow&		inWindow,
	bool				inOpposite) 
	: MPopupMenu("functionpopup"), 
	fTextView(inTextView)
{
	int32			selStart;
	int32			selEnd;
	SuffixType		suffixType = GetSuffixType(inWindow.Title());
	
	SetFont(be_plain_font);
	inTextView.GetSelection(&selStart, &selEnd);
	
	fParser.Parse(inTextView.Text(), selStart, suffixType, true);

	switch (fParser.FunctionCount())
	{
		BMessage*	msg;
		BMenuItem*	item;

		case -1:
			msg = new BMessage(msgNull);
			item = new BMenuItem("Not a source file", msg);
			AddItem(item);
			item->SetEnabled(false);
			break;

		case 0:
			msg = new BMessage(msgNull);
			item = new BMenuItem("No functions defined", msg);
			AddItem(item);
			item->SetEnabled(false);
			break;

		default:
			BuildPopUpText(inOpposite != sSort);
			break;
	}
}

// ---------------------------------------------------------------------------
//		~MFunctionPopup
// ---------------------------------------------------------------------------
//	Destructor

MFunctionPopup::~MFunctionPopup()
{
}

// ---------------------------------------------------------------------------
//		SortAllPopups
// ---------------------------------------------------------------------------
//	Static function.

void
MFunctionPopup::SortAllPopups(
	bool	inSort)
{
	sSort = inSort;
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MFunctionPopup::AttachedToWindow()
{
	BPopUpMenu::AttachedToWindow();
	SetFont(be_plain_font);
}

// ---------------------------------------------------------------------------
//		SelectAFunction
// ---------------------------------------------------------------------------

void
MFunctionPopup::SelectAFunction(
	BMenuItem* inItem)
{
	int32					index = IndexOf(inItem);
	const DeclarationInfo*	info = fParser.FunctionStorage();
	
	ASSERT(index >= 0 && index < fParser.FunctionCount());

	fTextView.ScrollToFunction(info[index].selectionStart, info[index].selectionEnd);
}

// ---------------------------------------------------------------------------
//		BuildPopUpText
// ---------------------------------------------------------------------------
#pragma mark -

void 
MFunctionPopup::BuildPopUpText(
	bool 				inSort)
{
	if (inSort)
		fParser.Sort();

	int32					term = fParser.FunctionCount();
	const DeclarationInfo*	info = fParser.FunctionStorage();

	for (int32 i = 0; i < term; i++)
	{
		BMessage*	msg;
		BMenuItem*	item;

		if (0 == strcmp(info[i].name, "-"))
			item = new BSeparatorItem;
		else
		{
			msg = new BMessage(msgNull);
			item = new BMenuItem(info[i].name, msg);
		}

		AddItem(item);

		if ((info[i].style & PROC_STYLE_CHECKED) != 0)
			item->SetMarked(true);
	}
}
