#ifndef _JAPANESEPREFS_H
#define _JAPANESEPREFS_H

#include <Application.h>
#include <Messenger.h>
#include <Path.h>
#include <View.h>
#include <Window.h>


class BTextControl;
class BMenuField;
class BButton;


class JapanesePrefsApp : public BApplication {
public:
								JapanesePrefsApp();
	virtual						~JapanesePrefsApp();

	virtual void				ReadyToRun();

private:
	void						ReadSettings();
	void						WriteSettings();

public:

	static bool					sPaletteWindow;
	static BPoint				sPaletteWindowLoc;
	static uint32				sKutoutenType;
	static bool					sFullWidthSpace;
	static int32				sWindowThreshold;
	static BMessenger			sMethod;
};


class JapanesePrefsWindow : public BWindow {
public:
								JapanesePrefsWindow();
	virtual						~JapanesePrefsWindow();
};


class SettingsView : public BView {
public:
								SettingsView(BRect frame);

	virtual void				AttachedToWindow();
	virtual void				MessageReceived(BMessage *message);
	
private:
	BMenuField*					fKutouten;
	BMenuField*					fSpace;
	BMenuField*					fThreshold;
};


class DictionaryView : public BView {
public:
								DictionaryView(BRect frame);

	virtual void				AttachedToWindow();
	virtual void				MessageReceived(BMessage *message);

private:
	void						AddToDictionary();

private:
	BTextControl*				fYomi;
	BTextControl*				fHyoki;
	BMenuField*					fHinshi;
	BButton*					fAddButton;
};


#endif
