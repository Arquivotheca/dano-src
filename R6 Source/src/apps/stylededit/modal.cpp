//-----------------------------------------------------------------
//
// File: Modal.cpp
// Date: 11/10/93 
//-----------------------------------------------------------------

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#include <Box.h>
#ifndef MODAL_H
#include "modal.h"
#endif

#include "CStyledEditApp.h"

#ifndef _TEXT_CONTROL_H
#include <TextControl.h>
#endif
//#ifndef MY_TEXT_VIEW_H
//#include <MyTextView.h>
//#endif
#ifndef _SCROLL_VIEW_H
#include <ScrollView.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _CHECK_BOX_H
#include <CheckBox.h>
#endif
#ifndef _MESSAGE_FILTER_H
#include <MessageFilter.h>
#endif
#include <ClassInfo.h>
#include <OS.h>

#include <stdio.h>
#include <string.h>

const short	kButtonfHeight = 23;
const short	kButtonfWidth = 70;

static const rgb_color back_color = {220, 220, 220, 255};

const ulong REPLACE_IN_ALL_DOCS_CB = 'RACB';


//-------------------------------------------------------------------

class TDialogFilter : public BMessageFilter {
public:
						TDialogFilter(BWindow *window);
virtual	filter_result	Filter(BMessage *message, BHandler **target);

		BWindow			*fWindow;
};

/* ------------------------------------------------------------------------- */

TDialogFilter::TDialogFilter(BWindow *window)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
	fWindow = window;
}

/* ------------------------------------------------------------------------- */

filter_result	TDialogFilter::Filter(BMessage *msg, BHandler **target)
{
	uchar ch = 0;

	if (msg->FindInt8("byte", (int8 *)&ch) == B_NO_ERROR) {
		if (ch == B_ESCAPE) {
			BView *v = fWindow->FindView("cancel");
			BButton *b = cast_as(v, BButton);
			if (b) {
				b->SetValue(1);
				b->Sync();
				snooze(50000);
				fWindow->PostMessage(cmdCancel);
				return B_SKIP_MESSAGE;
			}
		}
	}

	return B_DISPATCH_MESSAGE;
}

//-------------------------------------------------------------------

//-------------------------------------------------------------------
// T M E S S A G E D I A L O G
//-------------------------------------------------------------------
TMessageDialog::TMessageDialog(BRect bounds,
	const char *defaultButton)
	: BWindow(bounds, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{ 
	BButton	*theButton;
	BRect	theRect;			// Junk rect used to init views
	
	fPressed = 0;	// No button pressed yet
	fWidth = bounds.Width();
	fHeight = bounds.Height();
		
	BRect b = bounds;
	b.OffsetTo(B_ORIGIN);
	BView *parent = new BBox(b, "parent", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS |
							 B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	parent->SetViewColor(back_color);
	parent->SetLowColor(back_color);
	AddChild(parent);

	fTop = parent;
	//
	// We want the "OK" button to be positioned at
	// the bottom right of the dialog, whatever size it is
	//
	theRect.Set(fWidth - kButtonfWidth - 10,
			   fHeight - kButtonfHeight - 11,
			   fWidth - 10, fHeight - 11);
//+	PRINT(("%s: ", defaultButton)); PRINT_OBJECT(theRect);
	theButton = new BButton(theRect, "ok", defaultButton,
		new BMessage(cmdOK));
	fTop->AddChild(theButton);
	Lock();
	theButton->MakeDefault(TRUE);
	Unlock();

	TDialogFilter *df = new TDialogFilter(this);
	Lock();
	AddCommonFilter(df);
	Unlock();
}

//---------------------------------------------------------------
BView *TMessageDialog::Top()
{
	return fTop;
}

/* ------------------------------------------------------------------------- */
void TMessageDialog::ReAddView(const char *name, bool make_default)
{
	// ??? to reorder buttons for keyboard navigation!
	BView *v = FindView(name);
	if (v) {
		BButton *b = cast_as(v, BButton);
		if (make_default && b)
			b->MakeDefault(FALSE);
		v->RemoveSelf();
		Top()->AddChild(v);
		if (make_default && b)
			b->MakeDefault(TRUE);
	}
}

//---------------------------------------------------------------
void TMessageDialog::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case cmdOK:
		case cmdCancel:
		case cmdNo:
			fPressed = msg->what;
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}


//---------------------------------------------------------------
// T E N T R Y D I A L O G
//-------------------------------------------------------------------
TEntryDialog::TEntryDialog(BRect bounds, const char *message1,
	const char *initialText1, const char *message2, const char *initialText2,
	const char *defaultLabel, const char *cancelLabel, ulong max_chars1,
	bool numeric1)
	: TMessageDialog(bounds, defaultLabel)
{
	BTextView	*tview;
	BRect		theRect;	// Junk rect used to init views
	BRect		textRect;
	BScrollView	*theScrollView; // Used to provide border for text view
	BButton		*theButton;	

	Lock();
	// Create the BTextView for the user to enter text
	theRect.Set(15, 11, fWidth - 11, 39);
	textRect = theRect;
	textRect.InsetBy(1, 1);
	textRect.OffsetTo(1, 1);
	BTextControl *tc;

	tc = new BTextControl(theRect, "te1", message1, initialText1, NULL, B_FOLLOW_NONE);
	tc->SetDivider(tc->StringWidth(message1) + 6.0);
	tview = tc->TextView();
	tview->DisallowChar('\n');
	if (numeric1) {
		for (long i = 0; i < 256; i++)
			tview->DisallowChar(i);

		for (long i = '0'; i <= '9'; i++)
			tview->AllowChar(i);
	}
	if (max_chars1 > 0)
		tview->SetMaxBytes(max_chars1);
	fText1 = tview;	// For easy reference
	Top()->AddChild(tc);
	tview->SelectAll();
	tview->MakeFocus();

	if (message2) {
		theRect.Set(15, 36, fWidth - 11, 74);
		textRect = theRect;
		textRect.InsetBy(1, 1);
		textRect.OffsetTo(1, 1);
		BTextControl *tc2 = new BTextControl(theRect, "te2", message2, initialText2, NULL,
			B_FOLLOW_NONE);
		tc2->SetDivider(tc2->StringWidth(message2) + 6.0);
		tview = tc2->TextView();
		tview->DisallowChar('\n');
		fText2 = tview;	// For easy reference
		Top()->AddChild(tc2);
		if (initialText2) {
			tview->SelectAll();
		}

		float div1 = tc->Divider();
		float div2 = tc2->Divider();
		float theDiv = (div1 > div2) ? div1 : div2;
				
		tc->SetDivider(theDiv);
		tc2->SetDivider(theDiv);
	} else {
		fText2 = NULL;
	}

	// Now do the "Cancel" button...
	theRect.Set(fWidth - (2 * kButtonfWidth) - 19,
			   fHeight - kButtonfHeight - 11,
			   fWidth - kButtonfWidth - 19, fHeight - 11);
	theButton = new BButton(theRect, "cancel", cancelLabel,
		new BMessage(cmdCancel));
	Top()->AddChild(theButton);

	ReAddView("ok", TRUE);

	Unlock();
}

//-------------------------------------------------------------------
// T F I N D R E P L A C E D I A L O G
//-------------------------------------------------------------------
TFindReplaceDialog::TFindReplaceDialog(BPoint topleft, const char *findStr,
	const char *replaceStr)
	: TEntryDialog(BRect(topleft, topleft+BPoint(300, replaceStr ? 184 : 135)),
		"Find:", findStr,
		replaceStr ? "Replace with:" : NULL, replaceStr,
		replaceStr ? "Replace" : "Find")
{
	BRect	rect = Bounds();
	float	h = rect.Height();
	float	pad = replaceStr ? 30 : -3;

	CStyledEditApp *theApp = (CStyledEditApp *)be_app;
	BTextControl *tc = (BTextControl *)FindView("te1");
	BTextControl *tc2 = (BTextControl *)FindView("te2");
	float leftLoc = tc->Frame().left + tc->Divider() + 2;
	float topLoc = (tc2 != NULL) ? tc2->Frame().bottom + 7 : tc->Frame().bottom + 7;

	rect.Set(leftLoc, topLoc, leftLoc + 130, topLoc + 15);

	fSensitive = new BCheckBox(rect, "sensitive", "Case-sensitive", 
		new BMessage());
	fSensitive->SetValue(theApp->SearchSensitive());
	Top()->AddChild(fSensitive);

	rect.OffsetBy(0, 19);
	fWrap = new BCheckBox(rect, "wrap", "Wrap-around search", 
		new BMessage());
	fWrap->SetValue(theApp->SearchWrap());
	Top()->AddChild(fWrap);

	rect.OffsetBy(0, 19);
	fBackwards = new BCheckBox(rect, "backwards", "Search backwards", 
		new BMessage());
	fBackwards->SetValue(!theApp->SearchForward());
	Top()->AddChild(fBackwards);	

	Lock();
	ReAddView("wrap");
	ReAddView("sensitive");
	Unlock();

	if (replaceStr) {
		BButton	*button;

//+		rect.Set(20, h - pad - 25, 170, (h - pad - 25) + 15);
//+		rect.OffsetBy(170, 0);
		rect.OffsetBy(0, 19);
		BMessage *msg = new BMessage(REPLACE_IN_ALL_DOCS_CB);
		fGlobalReplace = new BCheckBox(rect, "all_docs",
			"Replace in all windows", msg);
		fGlobalReplace->SetValue(0);
		Top()->AddChild(fGlobalReplace);

		// Now do the "Replace All" button...
		rect.Set(10, h - kButtonfHeight - 11, 20 + kButtonfWidth + 8, h - 11);
		button = new BButton(rect, "Replace All", "Replace All",
			new BMessage(cmdReplaceAll));
		Top()->AddChild(button);

		rect.OffsetBy(rect.Width() + 10, 0);
		rect.top++;
		rect.right = rect.left + 1;
		Top()->AddChild(new TSeparatorLine(rect));
	}

	Lock();
	ReAddView("cancel");
	ReAddView("ok", TRUE);
	Unlock();
}

//-----------------------------------------------------------------
void TFindReplaceDialog::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case REPLACE_IN_ALL_DOCS_CB:
			{
			/*
			 the user just checked the "replace in all open windows" check box
			*/

			ASSERT(FindView("ok"));
			ASSERT(FindView("Replace All"));
			BButton	*replace = cast_as(FindView("ok"), BButton);
			BButton	*replaceAll = cast_as(FindView("Replace All"), BButton);
			ASSERT(replace);
			ASSERT(replaceAll);

			if (fGlobalReplace->Value() == 1) {
				/*
				 save the current values of "wrap" and "backwards" in
				 the BMessage
				*/
				BMessage *msg = fWrap->Message();
				if (msg->HasInt32("previous", 0)) {
					msg->ReplaceInt32("previous", 0, fWrap->Value());
				} else {
					msg->AddInt32("previous", fWrap->Value());
				}

				msg = fBackwards->Message();
				if (msg->HasInt32("previous", 0)) {
					msg->ReplaceInt32("previous", 0, fBackwards->Value());
				} else {
					msg->AddInt32("previous", fBackwards->Value());
				}

				// now turn on "wrap" option and turn off "backwards" option
				fWrap->SetValue(1);
				fBackwards->SetValue(0);

				// now disable those options because they need to say on
				fWrap->SetEnabled(FALSE);
				fBackwards->SetEnabled(FALSE);

				// disable the "Replace" button
				replace->SetEnabled(FALSE);
				SetDefaultButton(replaceAll);
			} else {
				// restore the saved settings
				BMessage *msg = fWrap->Message();
				ASSERT(msg->HasInt32("previous", 0));
				fWrap->SetValue(msg->FindInt32("previous"));

				msg = fBackwards->Message();
				ASSERT(msg->HasInt32("previous", 0));
				fBackwards->SetValue(msg->FindInt32("previous"));
				
				// re-enable those options
				fWrap->SetEnabled(TRUE);
				fBackwards->SetEnabled(TRUE);

				// enable the "Replace" button
				replace->SetEnabled(TRUE);
				SetDefaultButton(replace);
			}
			break;
			}
		case cmdReplaceAll:
			{
			fPressed = msg->what;
			break;
			}
		default:
			TEntryDialog::MessageReceived(msg);
			break;
	}
}


TSeparatorLine::TSeparatorLine(
	BRect	frame)
		: BView(frame, B_EMPTY_STRING, B_FOLLOW_ALL, B_WILL_DRAW)
{
}


void
TSeparatorLine::Draw(
	BRect	updateRect)
{
	const rgb_color kWhite	= {255, 255, 255, 255};
	const rgb_color kDark	= {100, 100, 100, 255};
	BRect bounds = Bounds();
	
	BeginLineArray(2);
	AddLine(bounds.LeftTop(), bounds.LeftBottom(), kDark);
	AddLine(bounds.RightTop(), bounds.RightBottom(), kWhite);
	EndLineArray();	
}

