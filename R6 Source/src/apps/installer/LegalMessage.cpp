//
// (c) 1997 Be Incorporated
//
// Legal disclaimer for installer
//
// I didn't write this, someone else did, I swear 
// And even if I did, I was forced at gunpoint
//
//								- Pavel
//
//

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <TextView.h>
#include <ScrollView.h>
#include <stdio.h>
#include <Window.h>
#include <Screen.h>
#include <string.h>
#include <stdlib.h>

#include "LegalMessage.h"
#include "LegalDisclaimerText.h"

const BPoint kSmallButtonSize(60, 20);
const BRect kLegalMessageRect(0.0, 0.0, 500.0, 350.0);
const int32 M_OK = '__ok';
const int32 M_CANCEL = 'canc';

#define bla 1
#if bla
#include <Alert.h>
#include <StopWatch.h>
BStopWatch *stopwatch;
#endif

LegalMessage::LegalMessage()
	:	BWindow(kLegalMessageRect, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	
	BBox *box = new BBox(Bounds(), "box", B_FOLLOW_ALL, B_WILL_DRAW 
		| B_FRAME_EVENTS, B_PLAIN_BORDER);
	box->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	AddChild(box);
	
	BRect textFrame(box->Bounds());
	textFrame.InsetBy(13.0, 13.0);
	textFrame.bottom -= kSmallButtonSize.y + 20;
	textFrame.right -= 14.0;
	BRect textRect(0, 0, textFrame.Width(), 1000);
	textRect.bottom += 100;

	BTextView *textView = new BTextView(textFrame, "legal stuff", textRect,
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS
		| B_NAVIGABLE);
	textView->SetStylable(true);
	textView->SetWordWrap(true);
	textView->SetText(legalDisclaimer);
	textView->MakeEditable(false);
	
	BScrollView *scroll = new BScrollView("", textView, B_FOLLOW_ALL_SIDES, 
		0, false, true);
	box->AddChild(scroll);

	BRect buttonRect(box->Bounds());
	buttonRect.InsetBy(10.0, 14.0);
	
	buttonRect.SetLeftTop(buttonRect.RightBottom() - kSmallButtonSize);
	BButton *button = new BButton(buttonRect, "", "Agree", new BMessage(M_OK));

	box->AddChild(button);
	button->MakeDefault(true);
	button->SetEnabled(true);
	
	buttonRect.OffsetBy(-(buttonRect.Width() + 9.0), 0);
	
	button = new BButton(buttonRect, "", "Disagree", 
		new BMessage(M_CANCEL));
	box->AddChild(button);

#if bla
	stopwatch = new BStopWatch("asdfg", true);
#endif
	
	BScreen screen(this);
	MoveTo((screen.Frame().Width() - Frame().Width()) / 2.0, (screen.Frame().Height() - Frame().Height()) / 2.0);
}

LegalMessage::~LegalMessage()
{
}

void 
LegalMessage::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_OK:
#if bla
			if (stopwatch->ElapsedTime() < 400000) {
				(new BAlert("", "YOU DID NOT READ THE LICENSE AGREEMENT! "
					"GO BACK AND READ IT NOW!", 
					"Or else the strong arm of the law will get you",
					NULL, NULL,
					B_WIDTH_FROM_LABEL, B_STOP_ALERT))->Go();
				stopwatch->Reset();
				return;
			}
#endif
			Quit();
			return;
		case M_CANCEL:
			be_app->PostMessage(B_QUIT_REQUESTED);
			Quit();
			break;
	}
	inherited::MessageReceived(msg);
}
