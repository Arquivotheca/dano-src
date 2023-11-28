#ifndef __LOADER_H__
#define __LOADER_H__

#include <add-ons/graphics/Accelerant.h>

typedef struct ConfigModeRec ConfigMode;

struct ConfigModeRec {
	int32 w;
	int32 h;
	int32 bpp;
	int32 hz;
	int32 enabled;
	ConfigMode *next;
};

typedef struct DevicesRec
{
	uint32 monitorId;
	char path[B_OS_NAME_LENGTH + 6];
	char filename[B_OS_NAME_LENGTH];
	void *data;
	uint8 isPrimary;
	uint8 has2D;
	uint8 has3D;
	float gamma[3];
	char name[256];
	ConfigMode *firstMode;
} Devices;

extern Devices *deviceList;
extern int32 deviceCount;


extern void __glInitDeviceList();

#endif

