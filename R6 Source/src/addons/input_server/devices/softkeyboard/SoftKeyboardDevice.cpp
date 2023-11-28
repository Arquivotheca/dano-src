
#include "SoftKeyboardDevice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Debug.h>



BInputServerDevice*
instantiate_input_device()
{
	return (new SoftKeyboardDevice());
}



SoftKeyboardListener::SoftKeyboardListener(SoftKeyboardDevice * device)
	:BLooper("SoftKeyboardListener", B_DISPLAY_PRIORITY),
	 _device(device)
{

}


SoftKeyboardListener::~SoftKeyboardListener()
{

}


void
SoftKeyboardListener::MessageReceived(BMessage * msg)
{
	BMessage * eventMessage;
	
	//_sPrintf("SoftKeyboardListener: Received Message\n");
	
	switch (msg->what)
	{
		case 'enqu':
			eventMessage = new BMessage();
			const char * str;
			int32 mods;
			if (msg->FindMessage("message", eventMessage) == B_OK)
			{
#if 0
				_sPrintf("SoftKeyboardListener: Enqueuing Key Message: ");
				switch (eventMessage->what)
				{
					case B_KEY_DOWN:
						eventMessage->FindString("bytes", &str);
						eventMessage->FindInt32("key", &mods);
						_sPrintf("B_KEY_DOWN  \"%s\" (%d)", str, mods);
						break;
					case B_KEY_UP:
						eventMessage->FindString("bytes", &str);
						eventMessage->FindInt32("key", &mods);
						_sPrintf("B_KEY_UP  \"%s\" (%d)", str, mods);
						break;
					case B_MODIFIERS_CHANGED:
						eventMessage->FindInt32("modifiers", &mods);
						_sPrintf("B_MODIFIERS_CHANGED (0x%08x)", mods);
						break;
					default:
						break;
				}
				_sPrintf("\n");
#endif
				_device->EnqueueMessage(eventMessage);
			}
			break;
		default:
			BLooper::MessageReceived(msg);
	}
	
}



SoftKeyboardDevice::SoftKeyboardDevice()
{

}


SoftKeyboardDevice::~SoftKeyboardDevice()
{
	// Quit the listener threads
	_listener->Lock();
	_listener->Quit();
}


status_t
SoftKeyboardDevice::InitCheck()
{
	// Create the listener thread
	_listener = new SoftKeyboardListener(this);
	_listener->Run();
	
	// Register so we get control notifications
	input_device_ref ** devices = (input_device_ref **) malloc(sizeof(input_device_ref *) * 2);
	devices[0] = (input_device_ref *) malloc(sizeof(input_device_ref));
	devices[0]->name = strdup("SoftKeyboardDevice");
	devices[0]->type = B_KEYBOARD_DEVICE;
	devices[0]->cookie = this;
	devices[1] = NULL;
	
	RegisterDevices(devices);
	
	// Clean up
	free(devices[0]->name);
	free(devices[0]);
	free(devices);
	
	return B_OK;
}


status_t
SoftKeyboardDevice::Control(const char * name, void * cookie, uint32 command, BMessage * message)
{
	status_t	err;
	BMessenger	replyTo;
	BMessage	reply;
	
	if (command == 'Fmgr')
	{
		// Where do we send the message?
		err = message->FindMessenger("messenger", &replyTo);
		if (err != B_OK)
			return err;
		
		// What message do we send?
		err = message->FindMessage("message", &reply);
		if (err != B_OK)
			return err;
		
		// Fill it in
		reply.AddMessenger("messenger", BMessenger(_listener));
		
		// Send It
		replyTo.SendMessage(&reply);
		
		// Debugging Output
		_sPrintf("%s:  Sending Messenger for listener.\n", name);
	}
	
	return B_OK;
}





