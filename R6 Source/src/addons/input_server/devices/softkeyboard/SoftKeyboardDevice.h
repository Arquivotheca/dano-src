/*
	SoftKeyboardDevice is a BInputServerDevice.
	
	It receives keyboard event messages from other applications
	(the Soft Keyboard), and posts them to the event stream.
*/

#ifndef SOFTKEYBOARDDEVICE
#define SOFTKEYBOARDDEVICE

#include <Looper.h>
#include <InputServerDevice.h>
#include <syslog.h>

extern "C" _EXPORT BInputServerDevice* instantiate_input_device(); 

// Forward References
class SoftKeyboardDevice;

// Listens for soft keyboard Messages, and passes them along
class SoftKeyboardListener : public BLooper
{
public:
					SoftKeyboardListener(SoftKeyboardDevice * device);
	virtual			~SoftKeyboardListener();
	
	virtual void	MessageReceived(BMessage * msg);
	
private:
	SoftKeyboardDevice	* _device;
};


class SoftKeyboardDevice : public BInputServerDevice
{
public:
						SoftKeyboardDevice();
	virtual				~SoftKeyboardDevice();
	
	virtual status_t	InitCheck();
	virtual status_t	Control(const char	* name,
								void		* cookie,
								uint32		command,
								BMessage	* message);
	SoftKeyboardListener	* _listener;
};

#endif // SOFTKEYBOARDDEVICE




