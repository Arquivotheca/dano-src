#include <InputServerDevice.h>
#include <List.h>
#include <OS.h>
#include <SerialPort.h>
#include <SupportDefs.h>


// export this for the input_server
extern "C" _EXPORT BInputServerDevice* instantiate_input_device();
 

class Cop8InputDevice : public BInputServerDevice {
public:
								Cop8InputDevice();

	virtual status_t			Start(const char *device, void *cookie);
	virtual	status_t			Stop(const char *device, void *cookie);
	virtual status_t			Control(const char	*device,
										void		*cookie,
										uint32		code, 
										BMessage	*message);

	static int32				cop8er(void *arg);

private:
	int							fFd;
	thread_id					fThread;
	bigtime_t					fClickSpeed;
};

