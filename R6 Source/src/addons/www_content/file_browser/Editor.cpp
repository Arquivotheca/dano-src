/*-----------------------------------------------------------------*/
//
//	File:		Editor.cpp
//
//	Written by:	Robert Polic
//
//	Copyright 2000, Be Incorporated
//
/*-----------------------------------------------------------------*/

#include "Editor.h"

#include <Window.h>

#include <stdio.h>


//========================================================================

Editor::Editor(FileListItem* caller, BRect rect, BRect item, BString* source,
			   drawing_parameters* parameters, bool is_file, uint32 what)
 : BTextControl		(rect, "Editor", "", source->String(), new BMessage('EDIT'), B_FOLLOW_ALL),
   fIsFile			(is_file),
   fWhat			(what),
   fCaller			(caller),
   fParameters		(parameters)
{
	float			w, h;
	SetFont(&fParameters->list_font);
	BTextView*		text = TextView();

	SetDivider(0);

	if (fIsFile)
	{
		text->DisallowChar('/');
		text->SetMaxBytes(B_FILE_NAME_LENGTH - 1);
	}
	text->SetFontAndColor(&fParameters->list_font, B_FONT_ALL);
	text->Select(0, -1);
	GetPreferredSize(&w, &h);
	MoveBy(0, (rect.Height() + 1 - h) / 2);
	text->AddFilter(new EditFilter(caller, item));
//	ResizeToPreferred(); <- this is broken when divider is 0;
}


/*-----------------------------------------------------------------*/

void Editor::AttachedToWindow()
{
	SetViewColor(fParameters->colors[eListEditColor]);
}


/*-----------------------------------------------------------------*/

void Editor::MessageReceived(BMessage* msg)
{
	switch (msg->what)
	{
		case 'EDIT':
			//Accept();
			break;

		// this message is actually sent by the caller
		case eEditAccepted:
			Accept();
			break;

		default:
			BTextControl::MessageReceived(msg);
	}
}


/*-----------------------------------------------------------------*/

void Editor::Accept()
{
	BMessage	msg(fWhat);

	msg.AddString("string", Text());
	fCaller->EditorCallBack(&msg);
}


//========================================================================

EditFilter::EditFilter(FileListItem* target, BRect item)
	:BMessageFilter(B_KEY_DOWN),
	fItem(item),
	fTarget(target)
{
}


/*-----------------------------------------------------------------*/

filter_result EditFilter::Filter(BMessage* msg, BHandler** /* target */)
{
	uchar ch = 0;

	if (msg->FindInt8("byte", (int8 *)&ch) == B_NO_ERROR)
	{
		if (ch == B_ENTER)
		{
			BMessage	m(eEditAccepted);

			fTarget->EditorCallBack(&m);
			return B_SKIP_MESSAGE;
		}
		else if (ch == B_ESCAPE)
		{
			BMessage	m(eEditCanceled);

			fTarget->fFilesView->List()->Invalidate(fItem);
			fTarget->EditorCallBack(&m);
			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
}


//========================================================================

EditControls::EditControls(FileListItem* caller, BRect rect, BRect item, drawing_parameters* parameters)
 : BView		(rect, "controls", B_FOLLOW_ALL, B_WILL_DRAW),
   fAcceptState	(eEditAcceptUpIcon),
   fRejectState	(eEditRejectUpIcon),
   fTracking	(eTrackingNone),
   fItem		(item),
   fCaller		(caller),
   fParameters	(parameters)
{
	int32	loop;
	BRect	r = Bounds();

	SetViewColor(fParameters->colors[eListEditColor]);

	for (loop = eEditAcceptUpIcon; loop <= eEditRejectDownIcon; loop++)
	{
		if (fParameters->icons[loop])
		{
			int32	height;
			int32	width;
			uint32	flags;
			BRect	png;

			fParameters->icons[loop]->GetSize(&width, &height, &flags);
			width--;

			if (loop >= eEditRejectUpIcon)
			{
				png.left = r.left + 9;
				png.right = png.left + width;
			}
			else
			{
				png.right = r.right - 9;
				png.left = png.right - width;
			}
			png.top = (int)((rect.Height() - height) / 2) + 1;
			png.bottom = png.top + height - 1;
			fParameters->icons[loop]->FrameChanged(png, width, height);
		}
	}
}


/*-----------------------------------------------------------------*/

void EditControls::Draw(BRect)
{
	if (fParameters->icons[fAcceptState])
		fParameters->icons[fAcceptState]->Draw(this, Bounds());

	if (fParameters->icons[fRejectState])
		fParameters->icons[fRejectState]->Draw(this, Bounds());

}


/*-----------------------------------------------------------------*/

void EditControls::MouseDown(BPoint point)
{
	BRect	r;

	fTracking = eTrackingUnknown;

	if (fParameters->icons[fAcceptState])
	{
		r = fParameters->icons[fAcceptState]->FrameInParent();
		if (r.Contains(point))
		{
			fAcceptState = eEditAcceptDownIcon;
			Invalidate(r);
			fTracking = eTrackingAccept;
		}
	}

	if (fParameters->icons[fRejectState])
	{
		r = fParameters->icons[fRejectState]->FrameInParent();
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
	int32	state = 0;
	BRect	r;

	if ((fParameters->icons[fAcceptState]) && ((fTracking == eTrackingNone) ||
											   (fTracking == eTrackingAccept)))
	{
		r = fParameters->icons[fAcceptState]->FrameInParent();
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
		r = fParameters->icons[fRejectState]->FrameInParent();
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


/*-----------------------------------------------------------------*/

void EditControls::MouseUp(BPoint point)
{
	BMessage	m;
	BRect		r;

	if (fTracking == eTrackingAccept)
	{
		r = fParameters->icons[fAcceptState]->FrameInParent();
		if (r.Contains(point))
		{
			Parent()->Invalidate(fItem);
			m.what = eEditAccepted;
			fCaller->EditorCallBack(&m);
		}
	}
	else if (fTracking == eTrackingReject)
	{
		r = fParameters->icons[fRejectState]->FrameInParent();
		if (r.Contains(point))
		{
			Parent()->Invalidate(fItem);
			m.what = eEditCanceled;
			fCaller->EditorCallBack(&m);
		}
	}
	fTracking = eTrackingNone;
}
