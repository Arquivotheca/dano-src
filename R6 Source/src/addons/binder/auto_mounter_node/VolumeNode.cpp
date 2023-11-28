#include <OS.h>
#include <stdlib.h>
#include <stdio.h>
#include <Looper.h>
#include "VolumeNode.h"


/*=================================================================*/

VolumeNode::VolumeNode(BLooper* mounter,
					   const char* volume_name,
					   const char* mounted_at,
					   const char* device,
					   const char* drive,
					   int block_size,
					   int total_blocks,
					   int free_blocks,
					   int removable,
					   int read_only)
 : fAutoMounterLooper(mounter)
{
	SetOrdered(true);
	AddProperty("volume_name", volume_name);
	AddProperty("mounted_at", mounted_at);
	AddProperty("drive", drive);
	AddProperty("device", device);
	AddProperty("block_size", (int)block_size);
	AddProperty("total_blocks", (int)total_blocks);
	AddProperty("free_blocks", (int)free_blocks, permsRead | permsWrite);
	AddProperty("removable", (int)removable);
	AddProperty("read_only", (int)read_only);
	if (removable)
		AddProperty("_eject", (double)0, permsWrite);
}


/*-----------------------------------------------------------------*/

get_status_t VolumeNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	if (!strcmp("_eject", name))
	{
		property	device;

		if ((args.Count() == 1) &&
			(BinderContainer::ReadProperty("device", device) == B_NO_ERROR))
		{
			BMessage	msg('MNTR');

			msg.AddInt32("function", 'ejct');
			msg.AddString("device", device.String());
			fAutoMounterLooper->PostMessage(&msg);
		}
	}
	else
		return BinderContainer::ReadProperty(name, prop, args);
	return B_NO_ERROR;
}
