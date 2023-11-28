#include "SoftKeyboard.h"
#include "SoftKeyboardCommon.h"

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <Input.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Path.h>
#include <Roster.h>

#include "utils.h"

// ******************************************************************************************

// method name/icon data
const char	*kSoftKeyboardName	= "SoftKeyboard";
const uchar	kSoftKeyboardIcon[]	= {
	0xff,0xff,0xff,0x2a,0x2a,0x2a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x00,0x17,0x17,0x00,0x00,0xff,0xff,0xff,0xff,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x00,0x17,0x0b,0x17,0x17,0x18,0x00,0x00,0xff,0xff,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x00,0x00,
	0x2a,0x2a,0x2a,0x2a,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0a,0x0b,0x00,
	0xff,0x2a,0x2a,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0a,0x0a,0x00,0x0f,
	0xff,0x2a,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,
	0xff,0x00,0x17,0x0b,0x17,0x0b,0x17,0x0b,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,0xff,
	0x00,0x17,0x17,0x17,0x0a,0x18,0x0b,0x17,0x17,0x0a,0x0b,0x00,0x0f,0x0f,0xff,0xff,
	0x00,0x11,0x11,0x17,0x17,0x0b,0x17,0x17,0x0a,0x0a,0x00,0x0f,0x0f,0xff,0xff,0xff,
	0x00,0x11,0x11,0x11,0x11,0x17,0x17,0x0b,0x0a,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,
	0xff,0x00,0x00,0x11,0x11,0x11,0x0b,0x0b,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0x00,0x00,0x11,0x0b,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

// ******************************************************************************************

const uint32 kInputModeMenuOffset = 2;

// static member variables
port_id		TSoftKeyboardInputMethod::sDropBox = B_ERROR;
bool		TSoftKeyboardLooper::sModePaletteShown = true;


BInputServerMethod*
instantiate_input_method()
{
	return (new TSoftKeyboardInputMethod());
}

// ******************************************************************************************

TSoftKeyboardInputMethod::TSoftKeyboardInputMethod()
	: BInputServerMethod(kSoftKeyboardName, kSoftKeyboardIcon)
{
	fInputMode = 0;
	ReadSettings();
}


TSoftKeyboardInputMethod::~TSoftKeyboardInputMethod()
{	
	delete_port(sDropBox);
	sDropBox = B_ERROR;

	BLooper *target = NULL;
	fSoftKeyboardLooper.Target(&target);
	if (target)			
		target->Quit();

	WriteSettings();
}


status_t
TSoftKeyboardInputMethod::InitCheck()
{
	sDropBox = create_port(1, SK_DROP_BOX_NAME);
	if (sDropBox < 0)
		return (sDropBox);

	fSoftKeyboardLooper = BMessenger(NULL, new TSoftKeyboardLooper(this));

	return B_OK;
}

//
//	sent when this method is turned on/off via DB replicant
//
status_t
TSoftKeyboardInputMethod::MethodActivated(bool active)
{
	BMessage message(msg_MethodActivated);

	if (active)
		message.AddBool("active", true);

	fSoftKeyboardLooper.SendMessage(&message);

	return B_OK;
}


filter_result
TSoftKeyboardInputMethod::Filter(BMessage *message, BList *outList)
{
	if (message->what == B_KEY_DOWN) {
		fSoftKeyboardLooper.SendMessage(message);
		return B_SKIP_MESSAGE;
	}
			
	return B_DISPATCH_MESSAGE;

}


void
TSoftKeyboardInputMethod::SetInputMode(uint32 mode)
{
	fInputMode = mode;
}


void
TSoftKeyboardInputMethod::ReadSettings()
{
}


void
TSoftKeyboardInputMethod::WriteSettings()
{
}

// ******************************************************************************************

TSoftKeyboardLooper::TSoftKeyboardLooper(TSoftKeyboardInputMethod	*method)
	: BLooper("Soft Keyboard Method Looper"),
		fOwnerInputMethod(method),
		fSoftKeyboardWindow(0)
{
	fSelf = BMessenger(NULL, this);
#if _SUPPORTS_SOFT_KEYBOARD
	fSoftKeyboardWindow = new TBasicKeyboardWindow(this, fOwnerInputMethod->InputMode());
	fSoftKeyboardWindow->Show();
		//	will show off-screen
#endif
	ReplenishDropBox();
	Run();
}


TSoftKeyboardLooper::~TSoftKeyboardLooper()
{
	if (fSoftKeyboardWindow)
		fSoftKeyboardWindow->PostMessage(B_CLOSE_REQUESTED);
	fOwnerInputMethod->SetMenu(NULL, BMessenger());
}


void
TSoftKeyboardLooper::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_KEY_DOWN:
			BLooper::MessageReceived(message);
			break;

		case msg_MethodActivated:
			HandleMethodActivated(message->HasBool("active"));
			break;

		case msg_ShowPalette:
//			HandleShowHidePalette(true);
			break;

		case msg_HidePalette:
//			HandleShowHidePalette(false);
			break;

		case SK_GRABBED_DROP_BOX:
			ReplenishDropBox();
			break;

		default:
			BLooper::MessageReceived(message);
			break;
	}
}


void
TSoftKeyboardLooper::EnqueueMessage(BMessage *message)
{
	//
	//	pass the message on to the input method
	//
	fOwnerInputMethod->EnqueueMessage(message);
}


void
TSoftKeyboardLooper::HandleMethodActivated(bool active)
{
	
#if _SUPPORTS_SOFT_KEYBOARD
	if (fSoftKeyboardWindow)
		fSoftKeyboardWindow->MakeActive(active, false, BRect(0,0,0,0));
#endif

	if (active) {
//#ifndef _SUPPORTS_SOFT_KEYBOARD
//		if (!fSoftKeyboardWindow) {
//			fSoftKeyboardWindow = new TBasicKeyboardWindow(this, fOwnerInputMethod->InputMode());
//			fSoftKeyboardWindow->Show();
//		}
//#endif
//
//		fOwnerInputMethod->SetMenu(fMenu, fSelf);
	} else {
//#ifndef _SUPPORTS_SOFT_KEYBOARD
//		if (fSoftKeyboardWindow) {
//			fSoftKeyboardWindow->PostMessage(B_CLOSE_REQUESTED);
//			fSoftKeyboardWindow = NULL;
//		}
//#endif

//		fOwnerInputMethod->SetMenu(NULL, fSelf);
	}
}

void
TSoftKeyboardLooper::HandleKeyDown(BMessage *message)
{
	EnqueueMessage(DetachCurrentMessage());
}

void
TSoftKeyboardLooper::HandleShowHidePalette(bool show)
{
	sModePaletteShown = show;

	if ((sModePaletteShown) && (fSoftKeyboardWindow == NULL))
		fSoftKeyboardWindow = new TBasicKeyboardWindow(this, fOwnerInputMethod->InputMode());
	else if ((!sModePaletteShown) && (fSoftKeyboardWindow != NULL)) {
		fSoftKeyboardWindow->PostMessage(B_CLOSE_REQUESTED);
		fSoftKeyboardWindow = NULL;
	}
}

void
TSoftKeyboardLooper::ReplenishDropBox()
{
	BMessage methodAddress;
	methodAddress.AddMessenger(SK_MESSENGER, fSelf);

	ssize_t	size = methodAddress.FlattenedSize();
	char	*buf = (char *)malloc(size);
	methodAddress.Flatten(buf, size);

	write_port(TSoftKeyboardInputMethod::sDropBox, 0, buf, size);

	free(buf);
}
