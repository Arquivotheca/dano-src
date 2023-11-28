//========================================================================
//	MIconMenuItem.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include <stdio.h>

#include "MIconMenuItem.h"
#include "IDEConstants.h"

#include <Bitmap.h>
#include <Menu.h>
#include <View.h>
#include <Volume.h>
#include <NodeInfo.h>

const float kIconHeight = 16.0;
const float kIconWidth = 20.0;

// ---------------------------------------------------------------------------
//		MIconMenuItem
// ---------------------------------------------------------------------------

MIconMenuItem::MIconMenuItem(
	BBitmap*		inIcon,
	const char *	inLabel,
	BMessage *		inMessage,
	char 			inShortcut,
	uint32 			inModifiers)
	: BMenuItem(
		inLabel,
		inMessage,
		inShortcut,
		inModifiers),
	fIcon(inIcon)
{
}

// ---------------------------------------------------------------------------
//		MIconMenuItem
// ---------------------------------------------------------------------------

MIconMenuItem::MIconMenuItem(
	const entry_ref&	inRef,
	const char *		inLabel,
	BMessage *			inMessage,
	char 				inShortcut,
	uint32 				inModifiers)
	: BMenuItem(
		inLabel,
		inMessage,
		inShortcut,
		inModifiers)
{
	GetIcon(inRef);
}

// ---------------------------------------------------------------------------
//		MIconMenuItem
// ---------------------------------------------------------------------------
//	Constructor for the root directory of a volume.

MIconMenuItem::MIconMenuItem(
	const entry_ref&		inRef,
	dev_t			inDevice,
	BMessage *		inMessage,
	char 			inShortcut,
	uint32 			inModifiers)
	: BMenuItem(
		"",
		inMessage,
		inShortcut,
		inModifiers)
{
	BNode			node(&inRef);
	BNodeInfo		info(&node);
	BVolume			vol;
	FileNameT		name = { '\0' };
	const int32		iconSize = 16*16;
	bool			gotIt = false;

	fIcon = new BBitmap(BRect(0.0, 0.0, 15.0, 15.0), B_COLOR_8_BIT);

	if (B_OK == node.GetVolume(&vol) && B_OK == vol.GetName(name))
	{
		SetLabel(name);
		if (vol.GetIcon(fIcon, B_MINI_ICON) == B_OK) {
			gotIt = true;
		}
	}
	
	if (! gotIt)
	{
		if (name[0] == '\0')
			SetLabel("boot");
		if (B_NO_ERROR != info.GetTrackerIcon(fIcon, B_MINI_ICON))
		{
			delete fIcon;
			fIcon = nil;
		}
	}
}

// ---------------------------------------------------------------------------
//		MIconMenuItem
// ---------------------------------------------------------------------------
//	Constructor for a hierarchical MenuItem.

MIconMenuItem::MIconMenuItem(
	const entry_ref&	inRef,
	const char *		inLabel,
	BMenu*				inMenu,
	BMessage *			inMessage)
	: BMenuItem(
		inMenu,
		inMessage)
{
	GetIcon(inRef);
	if (inLabel != NULL)
		SetLabel(inLabel);
}

// ---------------------------------------------------------------------------
//		~MIconMenuItem
// ---------------------------------------------------------------------------

MIconMenuItem::~MIconMenuItem()
{
	delete fIcon;
}

// ---------------------------------------------------------------------------
//		GetIcon
// ---------------------------------------------------------------------------

void
MIconMenuItem::GetIcon(
	const entry_ref&	inRef)
{
	BNode		node(&inRef);
	BNodeInfo	info(&node);
	
	fIcon = new BBitmap(BRect(0.0, 0.0, 15.0, 15.0), B_COLOR_8_BIT);

#if DEBUG
	status_t	err = info.GetTrackerIcon(fIcon, B_MINI_ICON);
	if (err != B_OK)
		printf("GetTrackerIcon failed, err = %p\n", err);
#endif

	if (B_NO_ERROR != info.GetTrackerIcon(fIcon, B_MINI_ICON))
	{
		delete fIcon;
		fIcon = nil;
	}
}

// ---------------------------------------------------------------------------
//		GetContentSize
// ---------------------------------------------------------------------------

void
MIconMenuItem::GetContentSize(
	float *width, 
	float *height)
{
	BMenuItem::GetContentSize(width, height);
	
	if (fIcon != nil)
	{
		*width += kIconWidth;
		if (*height < kIconHeight)
			*height = kIconHeight;
	}
}

// ---------------------------------------------------------------------------
//		DrawContent
// ---------------------------------------------------------------------------

void
MIconMenuItem::DrawContent()
{
	// Draw the icon
	if (fIcon != nil)
	{
		BView*			view = Menu();
		BRect			frame = Frame();
		BPoint			where = view->PenLocation();
		drawing_mode	mode = view->DrawingMode();

		view->SetDrawingMode(B_OP_OVER);
		where.y = frame.top + 2.0;
		view->DrawBitmap(fIcon, where);
		view->MovePenBy(kIconWidth, 0.0);
		view->SetDrawingMode(mode);
	}

	// Let the inherited function draw the label
	BMenuItem::DrawContent();
}

