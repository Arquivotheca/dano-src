/*-----------------------------------------------------------------*/
//
//	File:		Editor.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2001, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include <Looper.h>

#include <stdio.h>

#include "Editor.h"


//========================================================================

Editor::Editor(BRect rect, const char* text, BHandler* target,
			   drawing_parameters* parameters)
	:BTextControl(rect, "Editor", "", text, new BMessage('EDIT'), B_FOLLOW_ALL),
	fParameters(parameters)
{
	float		w, h;
	BTextView*	text = TextView();

	text->DisallowChar('/');
	text->SetMaxBytes(B_FILE_NAME_LENGTH - 1);
	text->SetFontAndColor(&fParameters->list_font, B_FONT_ALL);
	text->SelectAll();
	text->AddFilter(new EditFilter(target));

	SetFont(&fParameters->list_font);
	SetDivider(0);
	GetPreferredSize(&w, &h);
	MoveBy(0, (rect.Height() + 1 - h) / 2);
}


/*-----------------------------------------------------------------*/

void Editor::AttachedToWindow()
{
	SetViewColor(fParameters->colors[eListEditColor]);
}


/*-----------------------------------------------------------------*/

void Editor::WindowActivated(bool state)
{
	if (state)
		SetViewColor(fParameters->colors[eListEditColor]);
	else
		SetViewColor(fParameters->colors[eListColor]);
	Invalidate(Bounds());
	BTextControl::WindowActivated(state);
}


//========================================================================

EditFilter::EditFilter(BHandler* target)
	:BMessageFilter(B_KEY_DOWN),
	fTarget(target)
{
}


/*-----------------------------------------------------------------*/

filter_result EditFilter::Filter(BMessage* msg, BHandler** /* target */)
{
	uchar 			key = 0;
	filter_result	result = B_SKIP_MESSAGE;

	if (msg->FindInt8("byte", (int8*)&key) == B_NO_ERROR)
	{
		BMessage	m(kEDIT_COMPLETE);

		switch (key)
		{
			case B_ENTER:
				m.AddBool("accept", true);
				Looper()->PostMessage(&m, fTarget);
				break;

			case B_ESCAPE:
				m.AddBool("accept", false);
				Looper()->PostMessage(&m, fTarget);
				break;

			default:
				result = B_DISPATCH_MESSAGE;
		}
	}
	return result;
}


//========================================================================

EditControls::EditControls(BRect rect, BHandler* target, drawing_parameters* parameters)
	:BView(rect, "controls", B_FOLLOW_ALL, B_WILL_DRAW),
	fActive(true),
	fAcceptState(eEditAcceptUpIcon),
	fRejectState(eEditRejectUpIcon),
	fTracking(eTrackingNone),
	fTarget(target),
	fParameters(parameters)
{
	SetViewColor(fParameters->colors[eListEditColor]);
	SetDrawingMode(B_OP_ALPHA);
}


/*-----------------------------------------------------------------*/

void EditControls::Draw(BRect)
{
	if (fParameters->icons[fAcceptState])
		DrawBitmap(fParameters->icons[fAcceptState],
				   CalcDrawRect(fParameters->icons[fAcceptState], eAcceptControl));

	if (fParameters->icons[fRejectState])
		DrawBitmap(fParameters->icons[fRejectState],
				   CalcDrawRect(fParameters->icons[fRejectState], eRejectControl));
}


/*-----------------------------------------------------------------*/

void EditControls::MouseDown(BPoint point)
{
	BRect	r;

	fTracking = eTrackingUnknown;

	if (fParameters->icons[fAcceptState])
	{
		r = CalcDrawRect(fParameters->icons[fAcceptState], eAcceptControl);
		if (r.Contains(point))
		{
			fAcceptState = eEditAcceptDownIcon;
			Invalidate(r);
			fTracking = eTrackingAccept;
		}
	}

	if (fParameters->icons[fRejectState])
	{
		r = CalcDrawRect(fParameters->icons[fRejectState], eRejectControl);
		if (r.Contains(point))
		{
			fRejectState = eEditRejectDownIcon;
			Invalidate(r);
			fTracking = eTrackingReject;
		}
	}
}


/*-----------------------------------------------------------------*/

void EditControls::MouseMoved(BPoint point, uint32, const BMessage*)
{
	if (fActive)
	{
		int32	state = 0;
		uint32	buttons;
		BPoint	p;
		BRect	r;

		GetMouse(&p, &buttons);
		if (!buttons)
			fTracking = eTrackingNone;

		if ((fParameters->icons[fAcceptState]) && ((fTracking == eTrackingNone) ||
												   (fTracking == eTrackingAccept)))
		{
			r = CalcDrawRect(fParameters->icons[fAcceptState], eAcceptControl);
			if (r.Contains(point))
			{
				if (fTracking == eTrackingAccept)
					state = eEditAcceptDownIcon;
				else
					state = eEditAcceptOverIcon;
			}
			else
				state = eEditAcceptUpIcon;
	
			if ((state) && (fAcceptState != state))
			{
				fAcceptState = state;
				Invalidate(r);
			}
		}

		if ((fParameters->icons[fRejectState]) && ((fTracking == eTrackingNone) ||
												   (fTracking == eTrackingReject)))
		{
			r = CalcDrawRect(fParameters->icons[fRejectState], eRejectControl);
			if (r.Contains(point))
			{
				if (fTracking == eTrackingReject)
					state = eEditRejectDownIcon;
				else
					state = eEditRejectOverIcon;
			}
			else
				state = eEditRejectUpIcon;
	
			if ((state) && (fRejectState != state))
			{
				fRejectState = state;
				Invalidate(r);
			}
		}
	}
}


/*-----------------------------------------------------------------*/

void EditControls::MouseUp(BPoint point)
{
	BMessage	m(kEDIT_COMPLETE);
	BRect		r;

	if (fTracking == eTrackingAccept)
	{
		r = CalcDrawRect(fParameters->icons[fAcceptState], eAcceptControl);
		if (r.Contains(point))
		{
			m.AddBool("accept", true);
			Looper()->PostMessage(&m, fTarget);
		}
	}
	else if (fTracking == eTrackingReject)
	{
		r = CalcDrawRect(fParameters->icons[fRejectState], eRejectControl);
		if (r.Contains(point))
		{
			m.AddBool("accept", false);
			Looper()->PostMessage(&m, fTarget);
		}
	}
	fTracking = eTrackingNone;
}

/*-----------------------------------------------------------------*/

void EditControls::WindowActivated(bool state)
{
	if (state)
		SetViewColor(fParameters->colors[eListEditColor]);
	else
		SetViewColor(fParameters->colors[eListColor]);

	fActive = state;
	Invalidate(Bounds());
	BView::WindowActivated(state);
}


//--------------------------------------------------------------------

BRect EditControls::CalcDrawRect(BBitmap* icon, CONTROL control)
{
	BRect	draw;
	BRect	r = Bounds();

	if (control == eAcceptControl)
		draw.left = r.right - 8 - icon->Bounds().Width();
	else
		draw.left = r.left + 8;
	draw.right = draw.left + icon->Bounds().Width();
	draw.top = r.top + (int)((r.Height() - icon->Bounds().Height()) / 2);
	draw.bottom = draw.top + icon->Bounds().Height();
	return draw;
}
