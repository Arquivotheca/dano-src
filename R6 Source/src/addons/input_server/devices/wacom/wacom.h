#include <SerialPort.h>
#include <InputServerDevice.h>
#include <OS.h>
#include <SupportDefs.h>

struct tablet_info
{
	tablet_info() 
		{
			proximity = false;
			stylus = true;
			buttonflag = false;
			x = 0;
			y = 0;
			buttons = 0;
			pressuresign = 0;
			pressuredata = 0;
			xtiltsign = 0;
			xtiltdata = 0;
			ytiltsign = 0;
			ytiltdata = 0;
		};
	
	bool	proximity;
	bool	stylus;
	bool	buttonflag;
	int32	x;
	int32	y;
	uint32	buttons;
	bool	pressuresign;
	int32	pressuredata;
	bool	xtiltsign;
	int32	xtiltdata;
	bool	ytiltsign;
	int32	ytiltdata;
};

struct tablet_position
{
	tablet_position() 
		{
			proximity = false;
			stylus = true;
			buttonflag = false;
			x = 0;
			y = 0;
			buttons = 0;
			pressuresign = 0;
			pressuredata = 0;
			xtiltsign = 0;
			xtiltdata = 0;
			ytiltsign = 0;
			ytiltdata = 0;
		};
	
	bool	proximity;
	bool	stylus;
	bool	buttonflag;
	int32	x;
	int32	y;
	uint32	buttons;
	bool	pressuresign;
	int32	pressuredata;
	bool	xtiltsign;
	int32	xtiltdata;
	bool	ytiltsign;
	int32	ytiltdata;
};

class BTSWacomTablet
{
public:
			BTSWacomTablet(const char *device, status_t *err);
	virtual ~BTSWacomTablet();
	
	// Get Various Parameters
	virtual	status_t	GetModel(char *model);
	virtual	status_t	GetMaxCoordinates(int32 &x,int32 &y);
	virtual	status_t	GetCurrentSettings(char *buff, const int32 buffLen);
	virtual status_t	GetTabletInfo(tablet_info &);
	virtual status_t	GetPosition(int32 &x, int32 &y, int32 &buttons);
	virtual status_t	GetPosition(tablet_position &position);
	
	// Set various Parameters
	virtual status_t	SetASCIIFormat();
	virtual status_t	SetBinaryFormat();
	virtual status_t	SetRelativeMode();
	virtual status_t	SetAbsoluteMode();
	virtual	status_t	SetPointMode();
	virtual status_t	SetAlwaysTransmit(const int8 = 1);
	
	// Select the operational command set
	virtual status_t	SelectIISCommandSet();
	virtual status_t	SelectIVCommandSet();
	virtual status_t	Select1201CommandSet();
	
	virtual status_t	Reset();
	
protected:
	// Utility Commands
	virtual void	SendCommand(const char *cmd);
	virtual void	GetLine(char *buff, const long buffLen);
	virtual status_t	GetBytes(uint8 *buff, const uint32 buffLen);

	virtual status_t	SetTransmissionInterval(const int16);

	BSerialPort	fPort;
	long		fPortStatus;
	char		fCommandTerminator[10];
private:
};


// export this for the input_server
extern "C" _EXPORT BInputServerDevice* instantiate_input_device();
 

class WacomInputDevice : public BInputServerDevice {
public:
						WacomInputDevice();
	virtual				~WacomInputDevice();

	virtual status_t	Start(const char *device, void *cookie);
	virtual	status_t	Stop(const char *device, void *cookie);
	virtual status_t	Control(const char	*device, 
								void		*cookie, 
								uint32		code, 
								BMessage	*message);

	static int32		wacomer(void *arg);

public:
	BTSWacomTablet*				fTablet;
	thread_id					fWacomer;
	static WacomInputDevice*	sDevice;
};