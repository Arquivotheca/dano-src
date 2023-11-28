#include "mouse.h"

#include <string.h>
#include <unistd.h>

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Path.h>
#include <dirent.h>
#include <stdlib.h>

#include "kb_mouse_driver.h"

#include "InputServer.h"

void init_from_settings(int driver);


const int32 kPS2Cookie = -1;
const int32 kADBCookie = -1;
const int32 kUSBCookie = -1;


struct mouse_device {
	int			driver;
	thread_id	thread;
	void		*cookie;
	bool		usb;
	node_ref	node;
	char 		*name;
	
	mouse_device(int d, thread_id t, void *c, bool u = false)
	{
		driver = d;
		thread = t;
		cookie = c;
		usb = u;
		name = NULL;
	}
	
	~mouse_device()
	{
		free(name);
	}
};


MouseInputDevice* MouseInputDevice::sDevice = NULL;


BInputServerDevice*
instantiate_input_device()
{
	return (new MouseInputDevice());
}


MouseInputDevice::MouseInputDevice()
	: BInputServerDevice()
{
	sDevice = this;

	StartMonitoringDevice("input/mouse/usb");
	
	int ps2Mouse = open("/dev/input/mouse/ps2/0", O_RDWR);
	int adbMouse = open("/dev/input/mouse/adb/0", O_RDWR);
	int serialMouse = open("/dev/input/mouse/serial/0", O_RDWR);
	
	int numMice = 0;
	int numSerialMice = 0;
	BList usbMiceList;
	
	if (ps2Mouse != -1) {		
		init_from_settings(ps2Mouse);
		numMice++;
	}

	if (adbMouse != -1) {
		init_from_settings(adbMouse);
		numMice++;
	}

	if (serialMouse != -1) {
		init_from_settings(serialMouse);
		numSerialMice = ioctl(serialMouse, MS_NUM_SERIAL_MICE);
		numMice += numSerialMice;
	}

	struct dirent	*de = NULL;
	DIR 			*dp = opendir("/dev/input/mouse/usb");
	if (dp != NULL) {
		while (de = readdir(dp)) {
			char	port_path[PATH_MAX + 1] = "/dev/input/mouse/usb/";
			int 	fd = -1;
			
			if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0))
				continue;
	
			strcpy(&port_path[strlen(port_path)], de->d_name);
	
			if ((fd = open(port_path, O_RDWR)) < 0)
				continue;
			
			init_from_settings(fd);
			
			mouse_device	*mouse = new mouse_device(fd, B_ERROR, (void *)kUSBCookie, true);
			BNode			node(port_path);
			node.GetNodeRef(&mouse->node);

			usbMiceList.AddItem(mouse);
		}

		numMice += usbMiceList.CountItems();
		
		closedir(dp);
	}

	if (numMice > 0) {
		int32 				curDevice = 0;
		input_device_ref	**devices = (input_device_ref **)malloc(sizeof(input_device_ref *) * (numMice + 1));
		input_device_ref	ps2Device = {"PS/2 Mouse", B_POINTING_DEVICE, NULL};
		input_device_ref	adbDevice = {"ADB Mouse", B_POINTING_DEVICE, NULL};
		
		for (curDevice = 0; curDevice < numSerialMice; curDevice++) {
			mouse_device *mouse = new mouse_device(serialMouse, B_ERROR, (void *)curDevice);
			fMice.AddItem(mouse);

			devices[curDevice] = (input_device_ref *)malloc(sizeof(input_device_ref));	
			devices[curDevice]->name = (char *)malloc(32);
			sprintf(devices[curDevice]->name, "Serial Mouse %d", curDevice + 1);
			devices[curDevice]->type = B_POINTING_DEVICE;
			devices[curDevice]->cookie = (void *)mouse;
			
			mouse->name = devices[curDevice]->name;
		}

		for (int32 count = 1; curDevice < usbMiceList.CountItems() + numSerialMice;
				curDevice++, count++) {
			mouse_device *mouse = (mouse_device *)usbMiceList.ItemAt(count - 1);
			fMice.AddItem(mouse);

			devices[curDevice] = (input_device_ref *)malloc(sizeof(input_device_ref));
			devices[curDevice]->name = (char *)malloc(32);
			sprintf(devices[curDevice]->name, "USB Mouse %d", count);
			devices[curDevice]->type = B_POINTING_DEVICE;
			devices[curDevice]->cookie = (void *)mouse;
			
			mouse->name = devices[curDevice]->name;
		}
		
		if (ps2Mouse != -1) {
			mouse_device *mouse = new mouse_device(ps2Mouse, B_ERROR, (void *)kPS2Cookie);
			fMice.AddItem(mouse);

			ps2Device.cookie = (void *)mouse;
			mouse->name = strdup(ps2Device.name);		

			devices[curDevice++] = &ps2Device;
		}

		if (adbMouse != -1) {
			mouse_device *mouse = new mouse_device(adbMouse, B_ERROR, (void *)kADBCookie);
			fMice.AddItem(mouse);

			adbDevice.cookie = (void *)mouse;
			mouse->name = strdup(adbDevice.name);		

			devices[curDevice++] = &adbDevice;
		}

		devices[curDevice] = NULL;

		RegisterDevices(devices);

		int32 i = 0;
		for (i = 0; i < numSerialMice; i++)
			free(devices[i]);

		for (i; i < usbMiceList.CountItems() + numSerialMice; i++)
			free(devices[i]);
		
		free(devices);
	}
}


MouseInputDevice::~MouseInputDevice()
{
	StopMonitoringDevice("input/mouse/usb");

	mouse_device *mouse = NULL;
	while ((mouse = (mouse_device *)fMice.RemoveItem((int32)0)) != NULL) {
		close(mouse->driver);
		delete (mouse);
	}
}


status_t
MouseInputDevice::Start(
	const char	*device,
	void		*cookie)
{
	mouse_device *mouse = (mouse_device *)cookie;

	/* we want this guy to be just above the _input_server_event_loop_ thread */
	mouse->thread = spawn_thread(mouser, device, B_REAL_TIME_DISPLAY_PRIORITY+4, mouse);
	resume_thread(mouse->thread);

	return (B_NO_ERROR);
}


status_t
MouseInputDevice::Stop(
	const char	*device,
	void		*cookie)
{
	status_t dummy;
	mouse_device *mouse = (mouse_device *)cookie;

	kill_thread(mouse->thread);
	wait_for_thread(mouse->thread, &dummy);
	mouse->thread = B_ERROR;

	return (B_NO_ERROR);
}


status_t
MouseInputDevice::Control(
	const char	*device,
	void		*cookie,
	uint32		code,
	BMessage	*message)
{
	switch (code) {
		case B_MOUSE_TYPE_CHANGED:
		case B_MOUSE_MAP_CHANGED:
		case B_MOUSE_SPEED_CHANGED:
		case B_MOUSE_ACCELERATION_CHANGED:
		case B_CLICK_SPEED_CHANGED:
			init_from_settings(((mouse_device *)cookie)->driver);
			break;
		
		case B_NODE_MONITOR:
			HandleNodeMonitor(message);
			break;
	}

	return (B_NO_ERROR);
}


void
MouseInputDevice::HandleNodeMonitor(
	BMessage	*message)
{
	int32 opcode = 0;
	if (message->FindInt32("opcode", &opcode) != B_NO_ERROR)
		return;

	input_device_ref	*devices[2] = {NULL, NULL};
	input_device_ref	usbDevice = {NULL, B_POINTING_DEVICE, NULL};

	if (opcode == B_ENTRY_CREATED) {
		node_ref nodeRef;
		if (message->FindInt64("directory", &nodeRef.node) != B_NO_ERROR)
			return;
		if (message->FindInt32("device", &nodeRef.device) != B_NO_ERROR)
			return;

		BPath		path;		
		BDirectory	dir(&nodeRef);
		BEntry		entry(&dir, NULL);
		if (entry.GetPath(&path) != B_NO_ERROR)
			return;

		if (message->FindInt64("node", &nodeRef.node) != B_NO_ERROR)
			return;

		const char *devName = NULL;
		if (message->FindString("name", &devName) != B_NO_ERROR)
			return;
			
		path.Append(devName);

		int fd = open(path.Path(), O_RDWR);
		if (fd < 0)
			return;
	
		mouse_device *mouse = new mouse_device(fd, B_ERROR, (void *)kUSBCookie, true);
		mouse->node = nodeRef;
		
		fMice.AddItem(mouse);
		
		usbDevice.cookie = (void *)mouse;
		usbDevice.name = (char *)malloc(32);
		sprintf(usbDevice.name, "USB Mouse %d", atoi(devName) + 1);		
		devices[0] = &usbDevice;

		mouse->name = usbDevice.name;

		RegisterDevices(devices);
	}
	else {
		node_ref nodeRef;
		if (message->FindInt64("node", &nodeRef.node) != B_NO_ERROR)
			return;
		if (message->FindInt32("device", &nodeRef.device) != B_NO_ERROR)
			return;
					
		int32 numMice = fMice.CountItems();
		for (int32 i = 0; i < numMice; i++) {
			mouse_device *mouse = (mouse_device *)fMice.ItemAt(i);

			if (mouse->node != nodeRef)
				continue;

			usbDevice.name = mouse->name;				
			usbDevice.cookie = (void *)mouse;
			devices[0] = &usbDevice;

			UnregisterDevices(devices);
		
			close(mouse->driver);
		
			fMice.RemoveItem((int32)i);	
			delete (mouse);
			
			break;
		}
	}
}


int32
MouseInputDevice::mouser(
	void	*arg)
{
	mouse_device	*mouse = (mouse_device *)arg;
	int				theMouseDriver = mouse->driver;
	int 			serialCookie = (int)mouse->cookie;

	uchar		button = 0;
	uchar		old_button = 0;
	int32		clicks = 0;
	int32		num_events = 0;
	bigtime_t	mouse_time = 0;
	bigtime_t	time = 0;
	bigtime_t	lasttime = 0;
	int32		counter_move = 0;
	int32		counter_message = 0;
	int32		h_delta = 0;
	int32		v_delta = 0;
	int32		wheel_delta = 0;
	int32		mod_amount = 3;

	while (true) {
		num_events = ioctl(theMouseDriver, MS_NUM_EVENTS, (int)serialCookie);

		if (num_events < 0) {
			if (mouse->usb)
				return 0;
			else		
				num_events = 0;
		}
			
		for( ; num_events >= 0; num_events--) {
			mouse_pos mouseState;
			mouseState.serial_cookie = serialCookie;
			mouseState.wheel_delta = 0;

			if (ioctl(theMouseDriver, MS_READ, (char *)&mouseState) != B_NO_ERROR)
				continue;

			h_delta = mouseState.xdelta;
			v_delta = mouseState.ydelta;
			button = mouseState.buttons;
			clicks = mouseState.clicks;
			mouse_time = mouseState.time;
			wheel_delta = mouseState.wheel_delta;

			InputServer::DoMouseAcceleration(&h_delta,&v_delta);

			/*
			   if there is no button state change and there's still a bunch
			   of outstanding events, just skip all this junk and catch up
			   reading all the intervening events.
		    */	   
			if ((button == old_button) && (num_events > 2)) {
				counter_move++;

				/*
				   we need this for serial mice to smooth out their
				   motion as otherwise it gets too jerky (especially
				   when dragging very large bitmaps).
				*/   
				if ((num_events < 8) && ((counter_move % mod_amount) == 0)) {
					BMessage *event = new BMessage(B_MOUSE_MOVED);
					event->AddInt64("when", mouse_time);
					event->AddInt32("x", h_delta);
					event->AddInt32("y", v_delta);
					event->AddInt32("buttons", old_button);
				
					sDevice->EnqueueMessage(event);
				}

				continue;
			}

			if ((h_delta != 0) || (v_delta != 0)) {
				counter_move++;
				counter_message++;				
				time = system_time();

				if ( (((time - lasttime) >= 15000) && ((counter_message % 4) == 0)) ||
					 (num_events <= 1) || (counter_move % mod_amount == 0) || (button != old_button) ) {
					BMessage *event = new BMessage(B_MOUSE_MOVED);
					event->AddInt64("when", mouse_time);
					event->AddInt32("x", h_delta);
					event->AddInt32("y", v_delta);
					event->AddInt32("buttons", old_button);

					sDevice->EnqueueMessage(event);
						
					lasttime = time;	
				}
			}

			if(wheel_delta != 0) {
				BMessage *event = new BMessage(B_MOUSE_WHEEL_CHANGED);
				event->AddInt64("when", mouse_time);
				event->AddFloat("be:wheel_delta_x", 0.0);
				event->AddFloat("be:wheel_delta_y", wheel_delta);
				sDevice->EnqueueMessage(event);
			}

			uint32 downButtons = button & (~old_button);
			uint32 upButtons = (~button) & old_button;

			if (upButtons) {
				for (int32 k=0;k<3;k++) {
					if (upButtons & (1<<k)) {
						old_button &= ~(1<<k);
						BMessage *event = new BMessage(B_MOUSE_UP);
						event->AddInt64("when", mouse_time);
						event->AddInt32("x", 0);	// this is a down/up event, move events shouldn't go here!
						event->AddInt32("y", 0);	// this is a down/up event, move events shouldn't go here!
						//event->AddInt32("modifiers", theMods);	input_server does this for you
						event->AddInt32("buttons", old_button);
						sDevice->EnqueueMessage(event);
					};
				};
			};

			if (downButtons) {
				for (int32 k=0;k<3;k++) {
					if (downButtons & (1<<k)) {
						old_button |= (1<<k);
						BMessage *event = new BMessage(B_MOUSE_DOWN);
						event->AddInt64("when", mouse_time);
						event->AddInt32("x", 0);	// this is a down/up event, move events shouldn't go here!
						event->AddInt32("y", 0);	// this is a down/up event, move events shouldn't go here!
						//event->AddInt32("modifiers", theMods);	input_server does this for you
						event->AddInt32("buttons", old_button);
						event->AddInt32("clicks", clicks);
						sDevice->EnqueueMessage(event);
					};
				};
			};
			
			old_button = button;
		}
	}

	return (B_NO_ERROR);
}


void
init_from_settings(
	int	driver)
{
	int32 type = 0;
	if (get_mouse_type(&type) == B_NO_ERROR)
		ioctl(driver, MS_SETTYPE, &type);

	mouse_map map;
	if (get_mouse_map(&map) == B_NO_ERROR)
		ioctl(driver, MS_SETMAP, &map);

	int32 speed = 0;
	int32 accel=0;
	if ((get_mouse_speed(&speed) == B_NO_ERROR)&&(get_mouse_acceleration(&accel) == B_NO_ERROR)) {
		mouse_accel msspeed;
		ioctl(driver, MS_GETA, &msspeed);
		msspeed.speed = speed;
		msspeed.accel_factor = accel;
		ioctl(driver, MS_SETA, &msspeed);
	}

	bigtime_t click = 0;	
	if (get_click_speed(&click) == B_NO_ERROR)
		ioctl(driver, MS_SETCLICK, &click);
}
