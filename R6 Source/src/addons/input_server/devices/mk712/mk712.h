#include <InputServerDevice.h>
#include <List.h>
#include <OS.h>
#include <SerialPort.h>
#include <SupportDefs.h>


// export this for the input_server
extern "C" _EXPORT BInputServerDevice* instantiate_input_device();
 

class mk712InputDevice : public BInputServerDevice {
public:
								mk712InputDevice();

	virtual status_t			Start(const char *device, void *cookie);
	virtual	status_t			Stop(const char *device, void *cookie);
	virtual status_t			Control(const char	*device,
										void		*cookie,
										uint32		code, 
										BMessage	*message);

private:
	static int32				mk712portreader(void *arg);
	static int32				mk712er(void *arg);

	thread_id					fPortThread;
	port_id						fPort;
	bool						raw_mode;
	bool						recalibrate;
	bool						temp_recalibrate;
	char						temp_recalibrationstring[200];
	BMessenger					rawmsngr;

	int							fFd;
	thread_id					fThread;
	bigtime_t					fClickSpeed;
};

