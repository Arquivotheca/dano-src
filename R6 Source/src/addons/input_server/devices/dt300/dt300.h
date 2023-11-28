#ifndef DT300_H
#define DT300_H

#include <Entry.h>
#include <InputServerDevice.h>
#include <List.h>
#include <OS.h>
#include <SerialPort.h>
#include <SupportDefs.h>
#include <Messenger.h>
#include "controls.h"

// export this for the input_server
extern "C" _EXPORT BInputServerDevice* instantiate_input_device();
 

class DT300InputDevice : public BInputServerDevice 
{
public:
								DT300InputDevice();

	virtual status_t			Start(const char *device, void *cookie);
	virtual	status_t			Stop(const char *device, void *cookie);
	virtual status_t			Control(const char	*device,
										void		*cookie,
										uint32		code, 
										BMessage	*message);

	void						ControlChanged(control *ctl);
	void						ControlWindowVanished();
	
private:
	void						HandleButtons(unsigned short buttons);
	static int32				dt300portreader(void *arg);
	static int32				dt300er(void *arg);
	void						UpdateBrightNessContrast();
	void						ToggleSoftKeyboard();
		
	int							fFd;
	thread_id					fDeviceThread;
	thread_id					fPortThread;
	bigtime_t					fClickSpeed;
	port_id						fPort;
	bool						raw_mode;
	bool						recalibrate;
	bool						temp_recalibrate;
	char						temp_recalibrationstring[200];
	BMessenger					rawmsngr;
	int							brightness;
	int							contrast;
	ControlWindow				*fControlWindow;
	BMessenger					fSoftKeyboard;
	entry_ref					fBeepRef;
	sem_id						fBeepHandle;
};


#endif // DT300_H
