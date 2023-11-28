#include "ModeButton.h"
//#include "JapaneseCommon.h"
//#include "Japanese.h"
#include "KanaString.h"	// for HIRA_INPUT, etc, definitions
#include "BitmapData.h"

#include <Bitmap.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <stdio.h>

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
	// XXX: this should be rewritten to use asynchronous mouse tracking
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
	BMessenger*		japaneseMessenger,
	uint32			mode)
		: BitmapButton(frame, NULL, BMessenger())
{
	fJapaneseMessenger = *japaneseMessenger;
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
	//fMenu->SetTargetForItems(fJapaneseMessenger);
	fMenu->ItemAt(mode)->SetMarked(true);

	HandleSetInputMode(mode);
}


ModeButton::~ModeButton()
{
	delete (fMenu);
}

void
ModeButton::AttachedToWindow()
{
	fMenu->SetTargetForItems(NULL);
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
	if (item != NULL) {
		BMessage message = *(item->Message());
		message.AddBool("from_modebutton", true);
		Window()->PostMessage(&message, this);
		
	}

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

	if (Looper() != NULL) {
		BMessage *curr = Looper()->CurrentMessage();
		// forward the message to the input method if it originated from the
		// popupmenu owned by this button
		if (curr != NULL && curr->HasBool("from_modebutton")) {
			fJapaneseMessenger.SendMessage(curr);
		}
	}
}
