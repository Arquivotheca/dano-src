#include <OS.h>
#include <stdlib.h>
#include <stdio.h>
#include <Looper.h>
#include "DriveNode.h"


/*=================================================================*/

DriveNode::DriveNode(const char* device,
					 bool removable,
					 int type,
					 const char* vendor,
					 const char* product,
					 const char* version)
{
	SetOrdered(true);
	AddProperty("device", device);
	AddProperty("removable", (int)removable);
	if (type != -1)
	{
		AddProperty("type", (int)type);
		AddProperty("vendor", vendor);
		AddProperty("product", product);
		AddProperty("version", version);
	}
	AddProperty("volumes", (BinderContainer*)(fVolumes = new BinderContainer),
		permsRead | permsWrite | permsDelete);
}


/*-----------------------------------------------------------------*/

DriveNode::~DriveNode()
{
	RemoveProperty("volumes");
	fVolumes = NULL;
}


/*-----------------------------------------------------------------*/

void DriveNode::AddVolume(const char* name, const char* volume)
{
	fVolumes->AddProperty(name, volume, permsRead | permsWrite | permsDelete);
}


/*-----------------------------------------------------------------*/

void DriveNode::RemoveVolume(const char* name)
{
	fVolumes->RemoveProperty(name);
}
