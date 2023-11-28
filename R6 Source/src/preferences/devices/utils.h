#include <interface_misc.h>
#include <config_manager.h>

#include <View.h>
#include <Window.h>

#include "cm_wrapper.h"
	
void What(const char* what);
float FontHeight(BView* target, bool full);
void CenterWindowOnScreen(BWindow* w);
void ScreenSize(float* width, float* height);

status_t CountResourceDescriptors( struct device_configuration* config,
	resource_type type);
status_t GetNthResourceDescriptor( struct device_configuration *deviceConfig,
	uint32 index, resource_type type, resource_descriptor *info, uint32 size);
