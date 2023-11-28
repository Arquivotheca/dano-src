#include <SupportDefs.h>
#include <string.h>
#include <malloc.h>
#include <File.h>
#include <Entry.h>
#include <ByteOrder.h>

#include "ModePalette.h"
#include "JapaneseCommon.h"
#include "Japanese.h"
#include "KanaString.h"
#include "BitmapData.h"

#include <Bitmap.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>


const BPoint kUninitializedLocation(J_DEFAULT_PALETTE_WINDOW_LOC, J_DEFAULT_PALETTE_WINDOW_LOC);


BPoint ModePalette::sLocation(kUninitializedLocation);


ModePalette::ModePalette(
	JapaneseLooper	*owner,
	uint32			mode)
		: BWindow(BRect(100.0, 100.0, 100.0 + (kButtonBitsHeight * 2), 100.0 + kButtonBitsHeight), B_EMPTY_STRING, 
				  (window_look)25/*B_FLOATING_WINDOW_LOOK*/, B_FLOATING_ALL_WINDOW_FEEL,
				  B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE | B_NOT_ZOOMABLE |
				  B_NOT_MINIMIZABLE | B_NOT_CLOSABLE | B_AVOID_FOCUS)
{
	fOwner = owner;

	BRect buttonRect = Bounds();
	buttonRect.right = kButtonBitsWidth;
	buttonRect.bottom = kButtonBitsHeight;
	fModeButton = new ModeButton(buttonRect, fOwner, mode);
	AddChild(fModeButton);
	
	buttonRect.OffsetBy(kButtonBitsWidth, 0.0);

	BitmapButton *theButton = new BitmapButton(buttonRect, new BMessage(msg_LaunchJPrefs), BMessenger(owner));
	theButton->SetBits(kJPrefsBits, kJPrefsBitsDown);
	AddChild(theButton);

	BRect	screenFrame = BScreen().Frame();
	BPoint	windowLocation(sLocation);

	if (!screenFrame.Contains(windowLocation)) {	
		windowLocation = BScreen().Frame().RightBottom();
		windowLocation.x -= 140.0 + Bounds().Width();
		windowLocation.y -= 140.0 + Bounds().Height();
	}
	MoveTo(windowLocation);

	Show();
}


void
ModePalette::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_HiraganaInput:
		case msg_ZenkakuKatakanaInput:
		case msg_ZenkakuEisuuInput:
		case msg_HankakuKatakanaInput:
		case msg_HankakuEisuuInput:
		case msg_DirectInput:
		case msg_DirectHiraInput:
		case msg_DirectKataInput:
			PostMessage(message, fModeButton);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ModePalette::FrameMoved(
	BPoint	newLocation)
{
	sLocation = newLocation;
}


BitmapButton::BitmapButton(
	BRect		frame,
	BMessage	*message,
	BMessenger	target)
		: BView(frame, B_EMPTY_STRING, B_FOLLOW_ALL, B_WILL_DRAW),
		  BInvoker(message, target)
{
	fOffBits = NULL;
	fOnBits = NULL;

	fBitmap = new BBitmap(Bounds(), B_COLOR_8_BIT);
    memset(fBitmap->Bits(), 0, fBitmap->BitsLength());
}


BitmapButton::~BitmapButton()
{
	delete (fBitmap);
}


void
BitmapButton::Draw(
	BRect	updateRect)
{
	DrawBitmap(fBitmap);
}


void
BitmapButton::MouseDown(
	BPoint	where)
{
	BRect	bounds = Bounds();
	bool	inButton = false;
	uint32	buttons = 0;

	do {
		const uchar *newBits = NULL;

		if (bounds.Contains(where)) {
			if (!inButton) {
				newBits = fOnBits;
				inButton = true;
			}
		}
		else {
			if (inButton) {
				newBits = fOffBits;
				inButton = false;
			}
		}	

		if (newBits != NULL) {
			fBitmap->SetBits(newBits, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);
			Draw(bounds);
		}

		snooze(30000);
		GetMouse(&where, &buttons);
	} while (buttons != 0);

	if (inButton) {
		Invoke();
		
		fBitmap->SetBits(fOffBits, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);
		Draw(bounds);
	}
}


void
BitmapButton::SetBits(
	const uchar	*offBits,
	const uchar	*onBits)
{
	fOffBits = offBits;
	fOnBits = onBits;

	fBitmap->SetBits(fOffBits, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);

	if (Window() != NULL)
		Draw(Bounds());
}


ModeButton::ModeButton(
	BRect			frame,
	JapaneseLooper	*owner,
	uint32			mode)
		: BitmapButton(frame, NULL, BMessenger())
{
	fMenu = new BPopUpMenu(B_EMPTY_STRING);
	fMenu->SetFont(be_plain_font);
	fMenu->AddItem(new BMenuItem("ひらがな (ローマ字入力)", new BMessage(msg_HiraganaInput)));
	fMenu->AddItem(new BMenuItem("全角カタカナ (ローマ字入力)", new BMessage(msg_ZenkakuKatakanaInput)));
	fMenu->AddItem(new BMenuItem("全角英数", new BMessage(msg_ZenkakuEisuuInput)));
	fMenu->AddItem(new BMenuItem("半角カタカナ", new BMessage(msg_HankakuKatakanaInput)));
	fMenu->AddItem(new BMenuItem("半角英数", new BMessage(msg_HankakuEisuuInput)));
	fMenu->AddItem(new BMenuItem("直接入力", new BMessage(msg_DirectInput)));
	fMenu->AddItem(new BMenuItem("ひらがな (ヵな入力)", new BMessage(msg_DirectHiraInput)));
	fMenu->AddItem(new BMenuItem("全角カタカナ (ヵな入力)", new BMessage(msg_DirectKataInput)));
	fMenu->SetTargetForItems(owner);
	fMenu->ItemAt(mode)->SetMarked(true);

	HandleSetInputMode(mode);
}


ModeButton::~ModeButton()
{
	delete (fMenu);
}


void
ModeButton::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_HiraganaInput:
			HandleSetInputMode(HIRA_INPUT);
			break;

		case msg_ZenkakuKatakanaInput:
			HandleSetInputMode(ZEN_KATA_INPUT);
			break;

		case msg_ZenkakuEisuuInput:
			HandleSetInputMode(ZEN_EISUU_INPUT);
			break;

		case msg_HankakuKatakanaInput:
			HandleSetInputMode(HAN_KATA_INPUT);
			break;

		case msg_HankakuEisuuInput:
			HandleSetInputMode(HAN_EISUU_INPUT);
			break;

		case msg_DirectInput:
			HandleSetInputMode(DIRECT_INPUT);
			break;

		case msg_DirectHiraInput:
			HandleSetInputMode(DIRECT_HIRA_INPUT);
			break;

		case msg_DirectKataInput:
			HandleSetInputMode(DIRECT_KATA_INPUT);
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
ModeButton::MouseDown(
	BPoint	where)
{
	BRect bounds = Bounds();

	fBitmap->SetBits(fOnBits, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);
	Draw(bounds);

	BMenuItem *item = fMenu->Go(ConvertToScreen(Bounds().LeftBottom()), true);
	if (item != NULL)
		Window()->PostMessage(item->Message(), this);

	fBitmap->SetBits(fOffBits, fBitmap->BitsLength(), 0, B_COLOR_8_BIT);
	Draw(bounds);
}


void
ModeButton::HandleSetInputMode(
	uint32	mode)
{
	fMenu->FindMarked()->SetMarked(false);
	fMenu->ItemAt(mode)->SetMarked(true);

	// XXX: temporary hack until we get art for direct hira/kata input modes
	if (mode == DIRECT_HIRA_INPUT) mode = HIRA_INPUT;
	if (mode == DIRECT_KATA_INPUT) mode = ZEN_KATA_INPUT;

	SetBits(kButtonBits[mode * 2], kButtonBits[(mode * 2) + 1]);
}
