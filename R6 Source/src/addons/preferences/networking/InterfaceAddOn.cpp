//*****************************************************************************
//
//	File:		 InterfaceAddOn.cpp
//
//	Description: Main class for networking "Interface" preference panel
//
//	Copyright 1996 - 1999, Be Incorporated
//
//*****************************************************************************

#include "InterfaceAddOn.h"
#include "PrefsAppExport.h"
#include "InterfaceView.h"
#include "NetworkingCore.h"
#include <string.h>

InterfaceAddOn::InterfaceAddOn(image_id i, PPWindow *w, NetworkingCore *nc, int32 intf, bool defaultify)
 : PPAddOn(i, w), core(nc), interface(intf), def(defaultify)
{
}

BView *InterfaceAddOn::MakeView()
{
	return new InterfaceView(this, core, interface, def);
}

bool InterfaceAddOn::UseAddOn()
{
	return true;
}

BBitmap* InterfaceAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,3);
}

BBitmap* InterfaceAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,4);
}

char *InterfaceAddOn::Name()
{
	return "Interface";	// TODO: use pretty name?
}

char *InterfaceAddOn::InternalName()
{
	return "interface";	// TODO: use unique name
}
