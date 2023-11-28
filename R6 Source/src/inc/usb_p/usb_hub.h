#ifndef _USB_HUB_H
#define _USB_HUB_H

#include <USB_spec.h>

#define USB_DESCRIPTOR_HUB_TYPE 0x29

typedef struct 
{
	uint8 length;
	uint8 descriptor_type;
	uint8 num_ports;
	uint16 characteristics;
	uint8 power_delay;
	uint8 control_current;
} _PACKED usb_hub_fixed_descriptor;

typedef struct 
{
	uint8 length;
	uint8 descriptor_type;
	uint8 num_ports;
	uint16 characteristics;
	uint8 power_delay;
	uint8 control_current;
	uint8 removable[8];
} _PACKED usb_hub_descriptor;

#define USB_HUB_REQUEST_GET_STATE	2

#define PORTSTAT_CONNECTION    0x0001
#define PORTSTAT_ENABLED       0x0002
#define PORTSTAT_SUSPEND       0x0004
#define PORTSTAT_OVER_CURRENT  0x0008
#define PORTSTAT_RESET         0x0010
#define PORTSTAT_POWER_ON      0x0100
#define PORTSTAT_LOW_SPEED     0x0200

#define CX_PORT_CONNECTION     0x0001
#define CX_PORT_ENABLE         0x0002
#define CX_PORT_SUSPEND        0x0004
#define CX_PORT_OVER_CURRENT   0x0008
#define CX_PORT_RESET          0x0010

/* USB 1.1, table 11-12, page 268, Hub Class Feature Selectors */
#define C_HUB_LOCAL_POWER		0
#define C_HUB_OVER_CURRENTR		1

#define PORT_CONNECTION			0			
#define PORT_ENABLE				1
#define PORT_SUSPEND			2
#define PORT_OVER_CURRENT		3
#define	PORT_RESET				4
#define	PORT_POWER				8
#define PORT_LOW_SPEED			9

#define C_PORT_CONNECTION		16
#define C_PORT_ENABLE			17
#define C_PORT_SUSPEND			18
#define C_PORT_OVER_CURRENT		19
#define C_PORT_RESET			20



#endif
