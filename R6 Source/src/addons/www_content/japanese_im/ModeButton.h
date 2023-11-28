#ifndef _MODEBUTTON_H
#define _MODEBUTTON_H

#include <View.h>
#include <Invoker.h>

const uint32 msg_HiraganaInput			= 'Jzhi';
const uint32 msg_ZenkakuKatakanaInput 	= 'Jzki';
const uint32 msg_ZenkakuEisuuInput 		= 'Jzei';
const uint32 msg_HankakuKatakanaInput	= 'Jhji';
const uint32 msg_HankakuEisuuInput 		= 'Jhei';
const uint32 msg_DirectInput 			= 'Jdii';
const uint32 msg_DirectHiraInput 		= 'Jdhi';
const uint32 msg_DirectKataInput 		= 'Jdki';
const uint32 msg_LaunchJPrefs			= 'Jljp';

class BBitmap;

class BitmapButton : public BView, public BInvoker {
public:
						BitmapButton(BRect frame, BMessage *message, BMessenger target);
	virtual				~BitmapButton();

	virtual void		Draw(BRect updateRect);
	virtual void		MouseDown(BPoint where);

	void				SetBits(const uchar *offBits, const uchar *onBits);

protected:
	const uchar			*fOffBits;
	const uchar			*fOnBits;
	BBitmap*			fBitmap;
};


class ModeButton : public BitmapButton {
public:
						ModeButton(BRect frame, BMessenger* japaneseMessenger, uint32 mode);
	virtual				~ModeButton();

	virtual void		AttachedToWindow();
	virtual void		MessageReceived(BMessage *message);
	virtual void		MouseDown(BPoint where);

private:
	void				HandleSetInputMode(uint32 mode);

private:
	BPopUpMenu*			fMenu;
	BMessenger			fJapaneseMessenger;
};

#endif