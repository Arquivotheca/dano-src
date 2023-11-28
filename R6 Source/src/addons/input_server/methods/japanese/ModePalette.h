#ifndef _MODEPALETTE_H
#define _MODEPALETTE_H

#include <Window.h>
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
class JapaneseLooper;
class ModeButton;


class ModePalette : public BWindow {
public:
						ModePalette(JapaneseLooper *owner, uint32 mode);

	virtual void		MessageReceived(BMessage *message);
	virtual void		FrameMoved(BPoint newLocation);

private:
	JapaneseLooper*		fOwner;
	ModeButton*			fModeButton;

public:
	static BPoint		sLocation;
};


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
						ModeButton(BRect frame, JapaneseLooper *owner, uint32 mode);
	virtual				~ModeButton();

	virtual void		MessageReceived(BMessage *message);
	virtual void		MouseDown(BPoint where);

private:
	void				HandleSetInputMode(uint32 mode);

private:
	BPopUpMenu*			fMenu;
};

#endif