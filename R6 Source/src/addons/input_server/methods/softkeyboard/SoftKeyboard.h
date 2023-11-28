#ifndef SOFT_KEYBOARD_H
#define SOFT_KEYBOARD_H

#include <BeBuild.h>
#include <InputServerMethod.h>
#include <Looper.h>
#include <Messenger.h>
#include <Path.h>

#include "BasicKeyboard.h"

const uint32 msg_MethodActivated		= 'Jmac';
const uint32 msg_ShowPalette			= 'Jspl';
const uint32 msg_HidePalette			= 'Jhpl';

extern "C" _EXPORT BInputServerMethod* instantiate_input_method(); 

// ******************************************************************************************

class TSoftKeyboardInputMethod;
class TSoftKeyboardLooper : public BLooper {
public:
							TSoftKeyboardLooper(TSoftKeyboardInputMethod *method);
	virtual					~TSoftKeyboardLooper();

	virtual void			MessageReceived(BMessage *message);

	void					EnqueueMessage(BMessage *message);

private:	
	void					HandleMethodActivated(bool active);
	void					HandleKeyDown(BMessage *message);
	void					HandleShowHidePalette(bool show);

	void					ReplenishDropBox();

private:
	TSoftKeyboardInputMethod*	fOwnerInputMethod;
	BMessenger				fSelf;
	TBasicKeyboardWindow*	fSoftKeyboardWindow;
	
public:
	static bool				sModePaletteShown;
};

// ******************************************************************************************

class TSoftKeyboardInputMethod : public BInputServerMethod {
public:
							TSoftKeyboardInputMethod();
	virtual					~TSoftKeyboardInputMethod();

	virtual status_t		InitCheck();
	virtual status_t		MethodActivated(bool active);

	virtual filter_result	Filter(BMessage *message, BList *outList);
	
	void					SetInputMode(uint32 mode);
	uint32					InputMode() const;

	static port_id			sDropBox;
	
private:
	void					ReadSettings();
	void					WriteSettings();

	BMessenger				fSoftKeyboardLooper;
	uint32					fInputMode;
};

inline uint32
TSoftKeyboardInputMethod::InputMode() const
{
	return fInputMode;
}


#endif
