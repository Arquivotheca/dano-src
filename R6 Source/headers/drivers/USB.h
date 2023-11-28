/*
** USB.h - Version 2 USB Device Driver API
**
** Copyright 1999, Be Incorporated. All Rights Reserved.
**
*/

/*

BE ENGINEERING INSIGHTS: A Device Driver Is Worth a Thousand 
Words 
By Brian Swetland swetland@be.com 

Once upon a time I wrote an article about using USB from user space. Those 
tools are handy for prototyping and many simple projects, but often you 
need a real USB driver (perhaps to fit into an existing driver model like 
mice, keyboards, or audio devices). We'll take a look at the gritty details 
and some important rules for USB drivers. A sample driver that can dump 
raw audio data to USB speakers is provided to illustrate the points below. 
This article refers to the V2 version of the bus manager that will ship in 
BeOS 5. The sample code includes header files for this new interface. Please 
note that this code will not operate correctly under R4.x (and will not 
even build if you use the system headers). 
You can get the source code and follow along if you like: <ftp://ftp.be.com/
pub/samples/drivers/usb_speaker.zip>. 
The USB 1.1 Specification and specific device class specifications are 
available online in PDF format: <http://www.usb.org/developers/docs.html
>. 
The USB Bus Manager 
The Universal Serial Bus is far more dynamic than some of the other busses 
we support under BeOS. Users can and will hot plug and unplug devices. The 
dull old "load driver," "look for devices," "publish devices," techniques will 
not work very well here. We're using USB as a test bed for the next revision 
of the BeOS driver model, which will treat all busses as dynamic entities 
and be better prepared for other interesting device events, like power 
management. 
When a USB driver loads, it's expected to register itself with the bus 
manager using usb->register_driver(). The name of the driver must be 
provided (so the bus manager can reload it on demand), as well as a list of 
supported devices. This takes the form of an array of 
usb_support_descriptor structures. Each entry represents a pattern that 
may have a value (or the wild card 0) for the class, subclass, protocol, 
vendor, and device fields. The sample driver provided supports USB audio 
devices (all devices that have a class value of 1). 
Once registered, the driver must request notification of devices that it may 
be interested in. A set of notify_hooks (currently device_added and 
device_removed) are installed using usb->install_notify(). These hooks 
must be uninstalled before the driver unloads using usb->uninstall_notify(). 
Once the hooks are installed, device_added() will be called every time a 
device that matches (in its configuration_descriptor or an interface 
descriptor) the pattern in one of the supplied support descriptors appears. 
If there are already such devices plugged in, these calls will occur before 
install_notify returns. 
Adding and Removing Devices 
The device_added() hook receives a usb_device pointer (the opaque handle 
that represents a particular USB device on the bus). The driver may further 
investigate the device (for example, our sample only supports audio devices 
that support 16-bit stereo streams) and indicate whether it wants to keep it. 
If B_OK is returned, the driver may use the usb_device handle and any 
associated resources until device_removed() is called. Once 
device_removed() returns, the driver MAY NOT use these resources again. 
The cookie parameter of device_added provides a value that will be passed 
to device_removed. If device_added() returns B_ERROR, the driver 
receives no further notification and may not use the usb_device. 
When usb->uninstall_notify() is called, any devices that still exist are 
"removed" (device_removed is called). 
Putting Theory into Practice 
The sample driver maintains a list of devices it knows about (by way of 
device_added() calls). The list is protected by a lock and also generates the 
list of device names when publish_devices() is called. Each device entry 
contains information needed for transfers, a count of how many instances 
of it are open, and a unique number allocated on creation. When a device is 
removed, its entry is removed from the list, but only actually removed from 
memory if the open count is zero. 
Devices, Interfaces, Endpoints, and Pipes 
As mentioned above, each device is represented by a usb_device pointer (an 
opaque object). The properties of a device, its configurations, interfaces, 
and endpoints are described by various descriptors defined by the USB 
specification (Chapter 9, in particular, is very useful). The header 
USB_spec.h contains constants and structures for the various standard 
descriptors. 
Every device has at least one -- and possibly several -- configurations. usb->
get_nth_configuration() may be used to obtain a configuration_info object 
that describes a particular configuration. usb->set_configuration() is used to 
select a configuration and inform the device of that choice. A configuration 
value of NULL means "unconfigure." usb->get_configuration() returns the 
current configuration (or NULL if the device is not configured). 
The configuration_info structure contains the configuration descriptor, as 
well as an array of interface_list structures. Each interface_list represents 
one interface, which may have several alternates. An alternate interface 
often provides a slightly different version of the same resource (for 
example, an audio device may provide 16-bit stereo, 8-bit mono, and several 
other formats). 
The interface_list also indicates the currently active interface (the default 
is always alt[0]). This value may only be set by usb->set_alt_interface() and 
must never be changed by hand. IMPORTANT: set_alt_interface() MUST 
happen before set_configuration() -- this is a limitation in the current USB 
stack. 
Each interface has a descriptor, a number of endpoints (which may be zero), 
and a list of "generic" descriptors -- descriptors specific to the interface 
(audio interfaces provide a descriptor that describes the audio data format, 
for example). 
Each endpoint structure includes the endpoint descriptor (which indicates 
the type, direction, max bandwidth, as per the USB spec) as well as a 
pointer to a usb_pipe. Initially, every endpoint's usb_pipe pointer is NULL, 
but when a device is configured (with usb->set_configuration), the pipes of 
the active interfaces are valid handles used to send or receive data. 
Here's an example of a typical audio device (usually the audio streaming 
interface has several alternates with various other supported formats): 


  usb_device 
    configuration[0] 
      interface[0] 
        (class 1, subclass 1: Audio Control) 
        alternate[0] 
      interface[1] 
        (class 1, subclass 2: Audio Streaming) 
        alternate[0] 
          (default, uses no bandwidth) 
        alternate[1] 
          endpoint[0] 
          (isochronous out, 56 bytes per packet max) 
          generic[0..2] 
          (audio descriptors, format 8-bit mono) 
        alternate[2] 
          endpoint[0] 
          (isochronous out, 224 bytes per packet max) 
          generic[0..2] 
          (audio descriptors, format 16-bit stereo) 
      interface[2] 
        (class 3: Human Interface (volume buttons)) 
        alternate[0] 
          endpoint[0] 
          (interrupt in, 1 byte per packet max) 
          generic[0] 
          (HID descriptor) 

Sending and Receiving Data 
One very important thing to remember with USB drivers is that you can't 
use stack-allocated structures with the queue_*() IO functions. They are 
completed in a service thread that doesn't have access to the stack that the 
operations were initiated from. If you use stack-allocated structures, your 
driver will crash. Also, the current stack requires that data buffers for bulk, 
isochronous, and interrupt transactions be contiguous in physical memory. 
This limitation may be lifted in a future version, but for now, be careful. 
Every device has a pipe to endpoint 0, the default control pipe. The usb->
send_request() call is used to send control requests (which involve an 8-byte 
command sent from the host, optionally followed by some number of bytes 
sent to or received from the device). Control requests are used to configure 
devices, clear error conditions, and do other housekeeping. Many of the 
convenience functions provided by the bus manager (set/clear feature, 
get_descriptor, and so on) result in control requests. 
For any other type of transaction you'll need a usb_pipe object, which will 
be allocated to each endpoint in each active interface when 
set_configuration() succeeds as described above. 
The usb->queue_interrupt() and usb->queue_bulk() calls may be used to 
enqueue requests to interrupt or bulk endpoints, respectively. A callback 
must be provided to allow the stack to inform your driver when the requests 
complete. The direction of data transfer is dictated by the direction of the 
endpoint (as detailed in its descriptor). The callback is called on 
completion of the transaction (with success or failure) or if the transfer is 
cancelled with usb->cancel_queued_transfers(). 
Isochronous transfers are more complicated, and require that the driver 
inform the stack ahead of time so that adequate resources may be dedicated. 
Since isochronous transfers provide guaranteed bandwidth, the stack needs 
to pre-allocate various transfer descriptors specific to the host controller 
to insure that everything is handled in a timely fashion. 
usb->set_pipe_policy() is employed to configure an isochronous pipe in 
this fashion. It uses the provided buffer count to determine how many 
requests may be outstanding at once, the duration in milliseconds to 
determine the number of frames of data that will be provided, and the 
sample size to insure that frames are always a correct multiple of the 
sample size. 
usb->queue_isochronous() starts an isochronous transfer on the next frame. 
While bulk and interrupt transfers provide a simple actual_length 
parameter to their callback, which indicates how much data has been sent 
or received (in the event of a short transfer), isochronous pipes use run-
length encoding records to describe which data actually made it intact 
(since it's possible that only bits and pieces of an inbound isochronous 
stream arrived intact). The USB_rle.h header has extensive comments 
explaining these structures. 
In the current stack, it takes 10ms for the first isochronous transfer on a 
pipe to start. Provided you keep queuing up additional transfers, there will 
be one packet per frame. 
About USB Transactions 
Isochronous transactions get dedicated bandwidth and happen every frame. 
Delivery is not guaranteed, but timing is. 
Interrupt transactions are scheduled to happen on a polling interval of N 
milliseconds (no less often than once every N frames). They give the device 
(for example, a mouse) an opportunity to send some data (up to the 
specified max packet size) or a NAK, indicating no data is ready. 
Control requests actually consist of two or three transactions -- a setup 
phase to send the 8-byte control message, an optional data phase for the 
device to send or receive data, and an acknowledge phase where the data 
(or lack thereof) is acknowledged or an error is signaled via a STALL 
condition. 
Bulk transactions guarantee delivery but not timing -- they use as much 
bandwidth as is available (and not allocated to interrupt or isochronous 
transfers) to send or receive data. 
Every endpoint specifies a max packet size -- the USB stack will never 
violate this size and will break up longer queued requests into as many 
individual packets as required to complete the request. The exception is 
isochronous transactions, which must declare buffering information ahead 
of time with set_pipe_policy() 
The Sample Driver (usb_speaker) 
A full USB Audio driver is a fairly complex beast -- the sample provided 
works with USB Audio Spec-compliant devices, but only if they provide a 
16-bit stereo interface. The sample only handles 44KHz sample rates -- you 
can use it by copying a raw 44.1KHz, 16-bit stereo audio file (perhaps a CD 
track saved to disk) to /dev/misc/usb_speaker/0, provided you have a 
compatible USB audio device plugged in. 
The driver uses large (4ms) buffers to keep things simple -- for a real audio 
driver you'd want to use smaller buffers and drive it from a realtime thread 
(most likely one in the media server).  

*/

#ifndef _USB_H
#define _USB_H

#include <KernelExport.h>
#include <bus_manager.h>
#include <iovec.h>

#include <USB_spec.h>
#include <USB_rle.h>

#ifdef __cplusplus
extern "C" {
#endif

/* these are opaque handles to internal stack objects */
#if B_USB_USE_V2_MODULE
typedef void* usb_id;
typedef usb_id usb_device;	
typedef usb_id usb_interface;
typedef usb_id usb_pipe;
#else
typedef uint32 usb_id;
typedef usb_id usb_device;	
typedef usb_id usb_interface;
typedef usb_id usb_pipe;
#endif

typedef struct usb_endpoint_info usb_endpoint_info;
typedef struct usb_interface_info usb_interface_info;
typedef struct usb_interface_list usb_interface_list;
typedef struct usb_configuration_info usb_configuration_info;

typedef struct usb_notify_hooks {
	status_t (*device_added)(usb_device device, void **cookie);	/* FIXME */
	status_t (*device_removed)(void *cookie);	
} usb_notify_hooks;

typedef struct usb_support_descriptor {
	uint8 dev_class;
	uint8 dev_subclass;
	uint8 dev_protocol;
	uint16 vendor;
	uint16 product;
} usb_support_descriptor;

/* ie, I support any hub device: 
**   usb_support_descriptor hub_devs = { 9, 0, 0, 0, 0 };
*/

struct usb_endpoint_info {
	usb_endpoint_descriptor *descr;       /* descriptor and handle         */
	usb_pipe handle;                        /* of this endpoint/pipe         */
};

struct usb_interface_info {
	usb_interface_descriptor *descr;      /* descriptor and handle         */
	usb_interface handle;                        /* of this interface             */
		
	size_t endpoint_count;                /* count and list of endpoints   */
	usb_endpoint_info *endpoint;	      /* in this interface             */	

	size_t generic_count;                 /* unparsed descriptors in this  */
	usb_descriptor **generic;             /* interface                     */
};

struct usb_interface_list {
	size_t alt_count;                     /* count and list of alternate   */
	usb_interface_info *alt;              /* interfaces available          */
		
	usb_interface_info *active;           /* currently active alternate    */
};

struct usb_configuration_info {
	usb_configuration_descriptor *descr;  /* descriptor of this config     */
		
	size_t interface_count;               /* interfaces in this config     */
	usb_interface_list *interface;
};

	     		
typedef void (*usb_callback_func)(void *cookie, status_t status, 
								  void *data, uint32 actual_len);
								  
								 
typedef struct {
	int16	req_len;
	int16	act_len;
	status_t		status;	
} usb_iso_packet_descriptor;
 								 

#define USB_ISO_ASAP	0x00000001
						 
				
/* Old version 2 of the API, kept for driver compatability */
struct usb_module_info_v2 {
	bus_manager_info	binfo;
	
	/* inform the bus manager of our intent to support a set of devices */
	status_t (*register_driver)(const char *driver_name,
								const usb_support_descriptor *descriptors, 
								size_t count,
								const char *optional_republish_driver_name);								

	/* request notification from the bus manager for add/remove of devices we
	   support */
	status_t (*install_notify)(const char *driver_name, 
							   const usb_notify_hooks *hooks);
	status_t (*uninstall_notify)(const char *driver_name);
	
	/* get the device descriptor */
	const usb_device_descriptor *(*get_device_descriptor)(const usb_device *dev);
	
	/* get the nth supported configuration */	
	const usb_configuration_info *(*get_nth_configuration)(const usb_device *dev, uint index);

	/* get the active configuration */
	const usb_configuration_info *(*get_configuration)(const usb_device *dev);
	
	/* set the active configuration */	
	status_t (*set_configuration)(const usb_device *dev,
								  const usb_configuration_info *configuration); 

	status_t (*set_alt_interface)(const usb_device *dev, 
								  const usb_interface_info *ifc);
	
	/* standard device requests -- convenience functions        */
	/* obj may be a usb_device*, usb_pipe*, or usb_interface*   */
	status_t (*set_feature)(const void *object, uint16 selector);
	status_t (*clear_feature)(const void *object, uint16 selector);
	status_t (*get_status)(const void *object, uint16 *status);
	
	status_t (*get_descriptor)(const usb_device *d, 
							   uint8 type, uint8 index, uint16 lang,
							   void *data, size_t len, size_t *actual_len);

	/* generic device request function */	
	status_t (*send_request)(const usb_device *d, 
							 uint8 request_type, uint8 request,
							 uint16 value, uint16 index, uint16 length,
							 void *data, size_t data_len, size_t *actual_len);

	/* async request queueing */
	status_t (*queue_interrupt)(const usb_pipe *handle, 
								void *data, size_t len,
								usb_callback_func notify, void *cookie);
	
	status_t (*queue_bulk)(const usb_pipe *handle, 
						   void *data, size_t len,
						   usb_callback_func notify, void *cookie);
								
	status_t (*queue_isochronous)(const usb_pipe *handle, 
								  void *data, size_t len,
								  rlea* rle_array, uint16 buffer_duration_ms,
								  usb_callback_func notify, void *cookie);

	status_t (*queue_request)(const usb_device *d, 
							  uint8 request_type, uint8 request,
							  uint16 value, uint16 index, uint16 length,
							  void *data, size_t data_len, 
							  usb_callback_func notify, void *cookie);
	
	status_t (*set_pipe_policy)(const usb_pipe *handle, uint8 max_num_queued_packets,
								uint16 max_buffer_duration_ms, uint16 sample_size);
							 
	/* cancel pending async requests to an endpoint */
	status_t (*cancel_queued_transfers)(const usb_pipe *handle); 
	
	/* tuning, timeouts, etc */
	status_t (*usb_ioctl)(uint32 opcode, void* buf, size_t buf_size);
};

/* New version 3 of the API, use this instead of version 2 */
struct usb_module_info_v3 {
	bus_manager_info	binfo;
	
	/* inform the bus manager of our intent to support a set of devices */
	status_t (*register_driver)(const char *driver_name,
								const usb_support_descriptor *descriptors, 
								size_t count,
								const char *optional_republish_driver_name);								

	/* request notification from the bus manager for add/remove of devices we
	   support */
	status_t (*install_notify)(const char *driver_name, 
							   const usb_notify_hooks *hooks);
	status_t (*uninstall_notify)(const char *driver_name);
	
	/* get the device descriptor */
	const usb_device_descriptor *(*get_device_descriptor)(usb_device device);
	
	/* get the nth supported configuration */	
	const usb_configuration_info *(*get_nth_configuration)(usb_device device, uint index);

	/* get the active configuration */
	const usb_configuration_info *(*get_configuration)(usb_device device);
	
	/* set the active configuration */	
	status_t (*set_configuration)(usb_device device,
								  const usb_configuration_info *configuration); 

	status_t (*set_alt_interface)(usb_device device,
								  const usb_interface_info *ifc);
	
	/* standard device requests -- convenience functions        */
	/* handle may be a usb_device, usb_pipe, or usb_interface   */
	status_t (*set_feature)(usb_id handle, uint16 selector);
	status_t (*clear_feature)(usb_id handle, uint16 selector);
	status_t (*get_status)(usb_id handle, uint16 *status);
	
	status_t (*get_descriptor)(usb_device device, 
							   uint8 type, uint8 index, uint16 lang,
							   void *data, size_t len, size_t *actual_len);

	/* generic device request function */	
	status_t (*send_request)(usb_device device, 
							 uint8 request_type, uint8 request,
							 uint16 value, uint16 index, uint16 length,
							 void *data, size_t *actual_len);

	/* async request queueing */
	status_t (*queue_interrupt)(usb_pipe pipe, 
								void *data, size_t len,
								usb_callback_func notify, void *cookie);
	
	status_t (*queue_bulk)(usb_pipe pipe, 
						   void *data, size_t len,
						   usb_callback_func notify, void *cookie);
								
	status_t (*queue_bulk_v)(usb_pipe pipe, 
						   iovec *vec, size_t count,
						   usb_callback_func notify, void *cookie);

	status_t (*queue_isochronous)(usb_pipe pipe, 
								  void *data, size_t len,
								  usb_iso_packet_descriptor* packet_descriptors, uint32 packet_count,
  								  uint32* starting_frame_number, /* optional, can be NULL */
								  uint32 flags, 
								  usb_callback_func notify, void *cookie);

	status_t (*queue_request)(usb_device device, 
							  uint8 request_type, uint8 request,
							  uint16 value, uint16 index, uint16 length,
							  void *data, usb_callback_func notify, void *cookie);
	
	status_t (*set_pipe_policy)(usb_pipe pipe, uint8 max_num_queued_packets,
								uint16 max_buffer_duration_ms, uint16 sample_size);
							 
	/* cancel pending async requests to an endpoint */
	status_t (*cancel_queued_transfers)(usb_pipe pipe); 
	
	/* tuning, timeouts, etc */
	status_t (*usb_ioctl)(uint32 opcode, void* buf, size_t buf_size);
};

	
/* Versioning */
#if B_USB_USE_V2_MODULE
typedef struct usb_module_info_v2	usb_module_info;
#define B_USB_MODULE_NAME			"bus_managers/usb/v2"
#else
typedef struct usb_module_info_v3	usb_module_info;
#define	B_USB_MODULE_NAME			"bus_managers/usb/v3"
#endif

#ifdef __cplusplus
}
#endif

#endif
