#include <InputServerDevice.h>
#include <List.h>
#include <OS.h>
#include <SupportDefs.h>
#include "kb_device.h"


// export this for the input_server
extern "C" _EXPORT BInputServerDevice* instantiate_input_device();
 
class KeyboardInputDevice : public BInputServerDevice {
public:
						KeyboardInputDevice();
	virtual				~KeyboardInputDevice();

	virtual status_t	Start(const char *device, void *cookie);
	virtual	status_t	Stop(const char *device, void *cookie);
	virtual status_t	Control(const char	*device, 
								void		*cookie, 
								uint32		code, 
								BMessage	*message);

	static int32		keyboarder(void *arg);
	static void			SendEvent(keyboard_io *theKey,
								  char		*str,
								  char		*n_str, 
								  int32		type,
								  int32		repeat_count);

private:
	void				HandleNodeMonitor(BMessage *message);

public:
	BList							fKeyboards;
	static KeyboardInputDevice*		sDevice;
};
