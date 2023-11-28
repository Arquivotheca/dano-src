//========================================================================
//	MTextAddOn.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <string.h>

#include "MTextAddOn.h"
#include "MIDETextView.h"
#include "MTextWindow.h"
#include "IDEMessages.h"
#include "MMessageWindow.h"

// ---------------------------------------------------------------------------
//		MTextAddOn
// ---------------------------------------------------------------------------

MTextAddOn::MTextAddOn(
	MIDETextView&	inTextView)
	: fText(inTextView)
{
}

// ---------------------------------------------------------------------------
//		~MTextAddOn
// ---------------------------------------------------------------------------

MTextAddOn::~MTextAddOn()
{
}

// ---------------------------------------------------------------------------
//		Text
// ---------------------------------------------------------------------------

const char *
MTextAddOn::Text()
{
	return fText.Text();
}

// ---------------------------------------------------------------------------
//		TextLength
// ---------------------------------------------------------------------------

int32
MTextAddOn::TextLength() const
{
	return fText.TextLength();
}

// ---------------------------------------------------------------------------
//		GetSelection
// ---------------------------------------------------------------------------

void
MTextAddOn::GetSelection(
	int32 *start, 
	int32 *end) const
{
	fText.GetSelection(start, end);
}

// ---------------------------------------------------------------------------
//		Select
// ---------------------------------------------------------------------------

void
MTextAddOn::Select(
	int32 newStart, 
	int32 newEnd)
{
	if (fText.IsEditable())
		fText.Select(newStart, newEnd);
}

// ---------------------------------------------------------------------------
//		Delete
// ---------------------------------------------------------------------------

void
MTextAddOn::Delete()
{
	if (fText.IsEditable())
		fText.AddonDelete();
}

// ---------------------------------------------------------------------------
//		Insert
// ---------------------------------------------------------------------------

void
MTextAddOn::Insert(
	const char* inText)
{
	Insert(inText, strlen(inText));
}

// ---------------------------------------------------------------------------
//		Insert
// ---------------------------------------------------------------------------

void
MTextAddOn::Insert(
	const char* text, 
	int32 length)
{
	if (fText.IsEditable())
		fText.AddonInsert(text, length);
}

// ---------------------------------------------------------------------------
//		Window
// ---------------------------------------------------------------------------

BWindow*
MTextAddOn::Window()
{
	return fText.Window();
}

// ---------------------------------------------------------------------------
//		GetRef
// ---------------------------------------------------------------------------

status_t
MTextAddOn::GetRef(
	entry_ref&	outRef)
{
	return ((MTextWindow*) fText.Window())->GetRef(outRef);
}

// ---------------------------------------------------------------------------
//		IsEditable
// ---------------------------------------------------------------------------
// Added after 1.3.2

bool
MTextAddOn::IsEditable()
{
	return fText.IsEditable();
}

// ---------------------------------------------------------------------------

void
MTextAddOn::AddError(const char* message)
{
	InfoStruct info;
	info.iTextOnly = true;
	strncpy(info.iLineText, message, kLineTextLength);
	info.iLineText[kLineTextLength-1] = '\0';
	
	BMessage msg(msgAddInfoToMessageWindow);
	msg.AddData(kInfoStruct, kInfoType, &info, sizeof(info));
	MMessageWindow::GetGeneralMessageWindow()->PostMessage(&msg);
}

// ---------------------------------------------------------------------------

void
MTextAddOn::ShowErrors()
{
	MMessageWindow::GetGeneralMessageWindow()->PostMessage(msgShowAndActivate);
}

