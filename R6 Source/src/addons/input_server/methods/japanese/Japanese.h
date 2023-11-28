#ifndef _JAPANESE_H
#define _JAPANESE_H

#include <BeBuild.h>
#include <InputServerMethod.h>
#include <Looper.h>
#include <Messenger.h>
#include <Path.h>
#include "HenkanManager.h"

#include "DicCtrl.h"
#include "Grammar.h"


extern KGrammarDic gGrammar;


extern "C" _EXPORT BInputServerMethod* instantiate_input_method(); 


class BMenu;
class BMessenger;
class JapaneseLooper;
class InlineHenkanManager;
class ModePalette;


class JapaneseInputMethod : public BInputServerMethod {
public:
							JapaneseInputMethod();
	virtual					~JapaneseInputMethod();

	virtual status_t		InitCheck();
	virtual status_t		MethodActivated(bool active);

	virtual filter_result	Filter(BMessage *message, BList *outList);
	
	void					SetInputMode(uint32 mode);
	uint32					InputMode() const;

private:
	void					ReadSettings();
	void					WriteSettings();

private:
	BMessenger				fJapaneseLooper;
	uint32					fInputMode;

public:
	static port_id			sDropBox;
	static BPath			sUserDicPath;
	static KDicCtrl			sDicCtrl;
};


class JapaneseLooper : public BLooper {
public:
							JapaneseLooper(JapaneseInputMethod *method);
	virtual					~JapaneseLooper();

	virtual void			MessageReceived(BMessage *message);

	void					ResetInlineHenkanManager();
	void					EnqueueMessage(BMessage *message);

private:
	void					HandleMethodActivated(bool active);
	void					HandleKeyDown(BMessage *message);
	void					HandleShowHidePalette(bool show);
	void					HandleSetInputMode(uint32 mode, BMessage *message);
	void					HandleAddToDictionary(BMessage *message);

	void					ReplenishDropBox();

private:
	BMessenger				fSelf;
	BMessenger				fModeMessenger;
	JapaneseInputMethod*	fOwner;
	BMenu*					fMenu;
	ModePalette*			fModePalette;
	InlineHenkanManager*	fInlineHenkanManager;

public:
	static bool				sModePaletteShown;
};


class InlineHenkanManager : public HenkanManager {
public:
							InlineHenkanManager(JapaneseLooper *owner, uint32 mode);
	virtual					~InlineHenkanManager();

	virtual void			OpenInput();
	virtual void			CloseInput();
	virtual bool			ClauseLocation(BPoint *where, float *height);
	virtual void			Kakutei();
	virtual void			Update();

	void					HandleLocationReply(BMessage *message);

	void					SetInputMode(uint32 mode);
private:
	BMessage*				GenerateInputMethodEvent(bool confirmed);

private:
	JapaneseLooper*			fOwner;
};


#endif
