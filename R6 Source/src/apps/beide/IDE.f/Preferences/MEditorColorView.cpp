//========================================================================
//	MEditorColorView.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MEditorColorView.h"
#include "IDEMessages.h"
#include "Utils.h"
#include <Bitmap.h>
#include <Message.h>
#include <stdlib.h>

const float kColorMargin = 65.0;
const float kColorWidth = 40.0;
const float kColorHeight = 12.0;

// ---------------------------------------------------------------------------
//		MEditorColorView
// ---------------------------------------------------------------------------

MEditorColorView::MEditorColorView(
	BRect 		inArea,
	const char*	inName,
	rgb_color&	inColor)
	: MBoxControlChild(
		inArea,
		inName),
		fColor(inColor)
{
	BMessage*		msg = new BMessage(msgViewClicked);
	msg->AddPointer("source", this);
	SetMessage(msg);
	SetEnabled(false);
}

// ---------------------------------------------------------------------------
//		~MEditorColorView
// ---------------------------------------------------------------------------

MEditorColorView::~MEditorColorView()
{
}

// ---------------------------------------------------------------------------
//		Draw
// ---------------------------------------------------------------------------

void
MEditorColorView::Draw(
	BRect	/*inArea*/)
{
	const BRect		bounds = Bounds();
	
	// Name
	MovePenTo(bounds.left + 2, bounds.bottom - 2);
	DrawString(Name());
	DrawString(":");

	fColorBox.Set(bounds.left + kColorMargin, bounds.bottom - 2 - kColorHeight, 
				  bounds.left + kColorMargin + kColorWidth, bounds.bottom - 2);
	this->DrawColorBox(fColorBox, this);

	DrawFrame();
}

// ---------------------------------------------------------------------------

void
MEditorColorView::DrawColorBox(const BRect& inBox, BView* inView)
{
	// draw the color box in the view provided
	// we modify the rect, so do the modifications to a local copy
	BRect colorBox(inBox);

	inView->StrokeRect(colorBox);
	rgb_color highColor = inView->HighColor();
	inView->SetHighColor(fColor);
	colorBox.InsetBy(1.0, 1.0);
	inView->FillRect(colorBox);
	inView->SetHighColor(highColor);
}

// ---------------------------------------------------------------------------
//		MouseMoved
// ---------------------------------------------------------------------------
//	make ourself the focus when we're about to be dragged onto

void
MEditorColorView::MouseMoved(
	BPoint				/*where*/,
	uint32				code,
	const BMessage *	inMessage)
{
	if (code == B_ENTERED_VIEW && inMessage != nil && inMessage->HasData("RGBColor", B_RGB_COLOR_TYPE))
	{
		MakeFocus(true);
	}
}

// ---------------------------------------------------------------------------

void
MEditorColorView::MouseDown(BPoint point)
{
	// handle selection correctly
	MBoxControlChild::MouseDown(point);
	
	if (this->MouseMovedWhileDown(point)) {
		// the user wants to drag the color information
		BMessage msg;
		BBitmap* dragImage = NULL;
		BPoint dragStart;
		if (fColorBox.Contains(point)) {
			dragStart.x = point.x - fColorBox.left;
			dragStart.y = point.y - fColorBox.top;
			dragImage = this->CreateColorBitmap(point);
			msg.AddData("RGBColor", B_RGB_COLOR_TYPE, (const void*) &fColor, sizeof(rgb_color));
		}
		this->DragMessage(&msg, dragImage, B_OP_BLEND, dragStart);
	}
}

// ---------------------------------------------------------------------------

const int32 dragDelta = 3;

bool
MEditorColorView::MouseMovedWhileDown(BPoint point)
{
	while (true) {
		BPoint newLoc;
		uint32 buttons;
	
		this->GetMouse(&newLoc, &buttons);
		
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0) {
			return false;
		}
		else if (abs((int)(newLoc.x - point.x)) > dragDelta || abs((int)(newLoc.y - point.y)) > dragDelta) {
			return true;
		}
		else {
			snooze(30000);
		}
	}
}

// ---------------------------------------------------------------------------

BBitmap*
MEditorColorView::CreateColorBitmap(const BPoint&)
{
	// create a little clone of our color box

	BRect rect(fColorBox);
	rect.InsetBy(2.0, 2.0);
	rect.OffsetTo(BPoint(0.0, 0.0));
	BBitmap* bitmap = new BBitmap(rect, B_COLOR_8_BIT, true);
	bitmap->Lock();
	BView* drawingView = new BView(rect, "", B_FOLLOW_NONE, B_WILL_DRAW);
	bitmap->AddChild(drawingView);
	this->DrawColorBox(rect, drawingView);
	drawingView->Sync();
	bitmap->Unlock();
	
	return bitmap;
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MEditorColorView::MessageReceived(
	BMessage * inMessage)
{
	rgb_color*		color;
	int32			len;

	if (inMessage->WasDropped() && B_OK == inMessage->FindData("RGBColor", B_RGB_COLOR_TYPE, (const void**)&color, &len))
	{
		SetValue(*color);
		Invoke();
	}
	else
		BView::MessageReceived(inMessage);
}

// ---------------------------------------------------------------------------
//		SetValue
// ---------------------------------------------------------------------------
//	Dependent on operator!= existing for rgb_color
//	This is to match the BColorControl api so the ColorControlDropFilter
//	will work.

void
MEditorColorView::SetValue(
	rgb_color	inColor)
{
	if (inColor != fColor)
	{
		fColor = inColor;
		
		BRect	r = Bounds();
		r.left = kColorMargin;
		Draw(r);
	}
}
