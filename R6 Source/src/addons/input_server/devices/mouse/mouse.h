#include <InputServerDevice.h>
#include <List.h>
#include <OS.h>
#include <SupportDefs.h>


// export this for the input_server
extern "C" _EXPORT BInputServerDevice* instantiate_input_device();
 

class MouseInputDevice : public BInputServerDevice {
public:
						MouseInputDevice();
	virtual				~MouseInputDevice();

	virtual status_t	Start(const char *device, void *cookie);
	virtual	status_t	Stop(const char *device, void *cookie);
	virtual status_t	Control(const char	*device,
								void		*cookie,
								uint32		code, 
								BMessage	*message);

	static int32		mouser(void *arg);

private:
	void				HandleNodeMonitor(BMessage *message);

public:
	BList						fMice;
	static MouseInputDevice*	sDevice;
};

