#include <Box.h>
#include <Button.h>
#include <TextControl.h>
#include <Rect.h>
#include "MediaPlayerApp.h"
#include "URLPanel.h"
#include "PlayerWindow.h"

const float kButtonHeight = 20.0;
const float kButtonWidth = 75.0;
const float kExternalSpacing = 12.0;
const float kInternalSpacing = 9.0;

const uint32 kOpenPressed = 'oppp';

URLPanel::URLPanel(BPoint pos, BMessenger *messenger, const char *text)
	:	BWindow(BRect(pos.x, pos.y, pos.x + 300, pos.y + 75), "Open URL",
			B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE),
		fMessenger(*messenger)
{
	BBox *background = new BBox(Bounds(), "background", B_FOLLOW_LEFT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	AddChild(background);

	BRect buttonRect(Bounds());
	buttonRect.InsetBy(kExternalSpacing, kExternalSpacing);
	buttonRect.top = buttonRect.bottom - kButtonHeight;
	buttonRect.left = buttonRect.right - kButtonWidth;
	
	BButton *openButton = new BButton(buttonRect, "open_button", "Open", new
		BMessage(kOpenPressed));		
	background->AddChild(openButton);
	openButton->MakeDefault(true);
		
	buttonRect.OffsetBy(-(kButtonWidth + kInternalSpacing), 0);
	BButton *cancelButton = new BButton(buttonRect, "cancel_button", "Cancel",
		new BMessage(B_QUIT_REQUESTED));		
	background->AddChild(cancelButton);

	BRect textRect(Bounds());
	textRect.InsetBy(kExternalSpacing, kExternalSpacing);
	textRect.bottom = buttonRect.top - kInternalSpacing;
	fTextControl = new BTextControl(textRect, "text_control", "URL: ", text, 0);
	fTextControl->SetDivider(25);
	fTextControl->MakeFocus(true);
	background->AddChild(fTextControl);
}

void URLPanel::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kOpenPressed: {
			BMessage notifyMessage(kOpenURL);
			notifyMessage.AddString("be:url", fTextControl->Text());
			fMessenger.SendMessage(&notifyMessage);
			Quit();
			break;
		}
		
		default:
			BWindow::MessageReceived(message);	
	}
}

void URLPanel::WindowActivated(bool active)
{
	if (active)
		fTextControl->MakeFocus(true);
}


