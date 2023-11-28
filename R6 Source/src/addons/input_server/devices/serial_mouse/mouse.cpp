#include "mouse.h"
#include "mouse_protocol.h"
#include "InputServer.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <Debug.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Path.h>
#include <dirent.h>
#include <stdlib.h>
#include <termios.h>

struct mouse_device {
	thread_id	thread;
	int			fd;
	mouse_protocol *p;
	bool		force_mouse_present;
	node_ref	node;
	char 		*name;

	int32 		mouse_type;
	mouse_map	button_map;
	int32		mouse_speed;
	int32		mouse_accel;
	bigtime_t	click_speed;

	uchar		old_button;
	uchar		old_held_button;
	bigtime_t	last_click;
	int32		clicks;
	
	mouse_device(int d, thread_id t, mouse_protocol *np)
	{
		fd = d;
		thread = t;
		p = np;
		name = NULL;
		old_button = 0;
		old_held_button = 0;
		last_click = 0;
		clicks = 0;
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
	mouse_protocol *p;
	DIR				*ports_dir = NULL;	
	struct dirent	*port_ent = NULL;

	//StartMonitoringDevice("input/mouse/usb");

	ports_dir = opendir("/dev/ports");

	while ((port_ent = readdir(ports_dir)) != NULL) {
		bool force_mouse_present = false;
		char port_path[PATH_MAX + 1] = "/dev/ports/";

		if ((strcmp(port_ent->d_name, ".") == 0) || (strcmp(port_ent->d_name, "..") == 0))
			continue;

		strcpy(&port_path[strlen(port_path)], port_ent->d_name);

		//printf("serial_mouse: opening %s\n", port_path);
		int fd = open(port_path, O_CLOEXEC|O_EXCL|O_RDWR|O_NONBLOCK);

		if(fd < 0) {
			//printf("serial_mouse: %s busy\n", port_path);
		}
		else {
			p = detect_mouse(fd, force_mouse_present);
			fflush(stdout);
			if(p) {
				input_device_ref	*devices[2] = {NULL, NULL};
				input_device_ref	serDevice = {"Serial Mouse", B_POINTING_DEVICE, NULL};
				devices[0] = &serDevice;
				mouse_device *mouse = new mouse_device(fd, B_ERROR, p);
				serDevice.cookie = mouse;

				mouse->mouse_type = 3;
				mouse->button_map.left = 1;
				mouse->button_map.right = 2;
				mouse->button_map.middle = 4;
				mouse->click_speed = 500000;
				mouse->mouse_speed = 65536;
				mouse->mouse_accel = 65536;
				mouse->force_mouse_present = force_mouse_present;
			
				get_mouse_type(&mouse->mouse_type);
				get_mouse_map(&mouse->button_map);
				get_mouse_speed(&mouse->mouse_speed);
				get_mouse_acceleration(&mouse->mouse_accel);
				get_click_speed(&mouse->click_speed);
			
				RegisterDevices(devices);
				fMice.AddItem(mouse);
			}
			else {
				close(fd);
			}
		}
	}
}


MouseInputDevice::~MouseInputDevice()
{
//	StopMonitoringDevice("input/mouse/usb");

	mouse_device *mouse = NULL;
	while ((mouse = (mouse_device *)fMice.RemoveItem((int32)0)) != NULL) {
		close(mouse->fd);
		delete (mouse);
	}
}


status_t
MouseInputDevice::Start(
	const char	*device,
	void		*cookie)
{
	mouse_device *mouse = (mouse_device *)cookie;
	if(mouse == NULL)
		return B_ERROR;
	/* we want this guy to be just above the _input_server_event_loop_ thread */
	mouse->thread = spawn_thread(mouser, device, B_REAL_TIME_DISPLAY_PRIORITY+4, mouse);
	resume_thread(mouse->thread);

	return (B_NO_ERROR);
}


status_t
MouseInputDevice::Stop(
	const char	*/*device*/,
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
	const char	*/*device*/,
	void		*cookie,
	uint32		code,
	BMessage	*message)
{
	mouse_device *mouse = (mouse_device *)cookie;
	if(mouse == NULL)
		return B_ERROR;

	switch (code) {
		case B_MOUSE_TYPE_CHANGED:
			get_mouse_type(&mouse->mouse_type);
			break;
		case B_MOUSE_MAP_CHANGED:
			get_mouse_map(&mouse->button_map);
			break;
		case B_MOUSE_SPEED_CHANGED:
			get_mouse_speed(&mouse->mouse_speed);
			break;
		case B_MOUSE_ACCELERATION_CHANGED:
			get_mouse_speed(&mouse->mouse_accel);
			break;
		case B_CLICK_SPEED_CHANGED:
			get_click_speed(&mouse->click_speed);
			break;
		
		case B_NODE_MONITOR:
			HandleNodeMonitor(message);
			break;
	}

	return (B_NO_ERROR);
}


void
MouseInputDevice::HandleNodeMonitor(
	BMessage	*/*message*/)
{
#if 0
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
#endif
}

bool MouseInputDevice::mouse_state_changed(void *cookie, serial_mouse_state *state)
{
	mouse_device	*mouse = (mouse_device *)cookie;

	bigtime_t	mouse_time = system_time();

	if(mouse->mouse_type < 3 && state->buttons & 0x4) {
		state->buttons = state->buttons & 0x3 | 0x2;
	}
	if(mouse->mouse_type < 2 && state->buttons & 0x2) {
		state->buttons = 1;
	}

	uint32 b = 0;
	if (state->buttons & 0x1)
		b |= mouse->button_map.left;
	if (state->buttons & 0x2)
		b |= mouse->button_map.right;
	if (state->buttons & 0x4) {
		b |= mouse->button_map.middle;
	}
	
	int32 x = state->dx;
	int32 y = state->dy;

	InputServer::DoMouseAcceleration(&x,&y);
	
	if ((!mouse->old_button) && (b)) {
		if (mouse->last_click + mouse->click_speed > mouse_time && b==mouse->old_held_button)
			mouse->clicks++;
		else
			mouse->clicks = 1;
	}
	
	if (b) {
		mouse->last_click = mouse_time;
		mouse->old_held_button = b;
	}

	if ((x != 0) || (y != 0)) {
		BMessage *event = new BMessage(B_MOUSE_MOVED);
		event->AddInt64("when", mouse_time);
		event->AddInt32("x", x);
		event->AddInt32("y", y);
		event->AddInt32("buttons", mouse->old_button);

		sDevice->EnqueueMessage(event);
	}

	if (state->dw != 0) {
		BMessage *event = new BMessage(B_MOUSE_WHEEL_CHANGED);
		event->AddInt64("when", mouse_time);
		event->AddFloat("be:wheel_delta_x", 0.0);
		event->AddFloat("be:wheel_delta_y", state->dw);
		sDevice->EnqueueMessage(event);
	}

	uint32 downButtons = b & (~mouse->old_button);
	uint32 upButtons = (~b) & mouse->old_button;

	if (upButtons) {
		for (int32 k=0;k<3;k++) {
			if (upButtons & (1<<k)) {
				mouse->old_button &= ~(1<<k);
				BMessage *event = new BMessage(B_MOUSE_UP);
				event->AddInt64("when", mouse_time);
				event->AddInt32("x", 0);	// this is a down/up event, move events shouldn't go here!
				event->AddInt32("y", 0);	// this is a down/up event, move events shouldn't go here!
				//event->AddInt32("modifiers", theMods);	input_server does this for you
				event->AddInt32("buttons", mouse->old_button);
				sDevice->EnqueueMessage(event);
			};
		};
	};

	if (downButtons) {
		for (int32 k=0;k<3;k++) {
			if (downButtons & (1<<k)) {
				mouse->old_button |= (1<<k);
				BMessage *event = new BMessage(B_MOUSE_DOWN);
				event->AddInt64("when", mouse_time);
				event->AddInt32("x", 0);	// this is a down/up event, move events shouldn't go here!
				event->AddInt32("y", 0);	// this is a down/up event, move events shouldn't go here!
				//event->AddInt32("modifiers", theMods);	input_server does this for you
				event->AddInt32("buttons", mouse->old_button);
				event->AddInt32("clicks", mouse->clicks);
				sDevice->EnqueueMessage(event);
			};
		};
	};
	
	mouse->old_button = b;
	return true;
}

int32
MouseInputDevice::mouser(
	void	*arg)
{
	mouse_device	*mouse = (mouse_device *)arg;
	int				fd = mouse->fd;

	if(mouse == NULL)
	{
		printf("no dev pointer\n");
		return B_ERROR;
	}
	if(mouse->p == NULL) {
		printf("no protocol pointer\n");
		return B_ERROR;
	}
	read_mouse(fd, mouse->p, mouse, mouse_state_changed);

	return (B_NO_ERROR);
}

