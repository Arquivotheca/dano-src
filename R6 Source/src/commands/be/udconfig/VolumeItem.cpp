//
// Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#include <stdio.h>

#include <private/storage/DeviceMap.h>

#include "VolumeItem.h"


static const rgb_color kSelectedBg = { 203, 203, 255, 255 };

static const rgb_color kDisabledColor = { 128, 128, 128, 255 };

static const rgb_color kVolumeColor[2] = { { 0, 0, 255, 255 }, kDisabledColor };
static const rgb_color kFSColor[2] = { { 255, 0, 0, 255 }, kDisabledColor };
static const rgb_color kDevColor[2] = { { 0, 0, 0, 255 }, kDisabledColor };

static const float kVolumeWidth = 10;
static const float kFSWidth = 5;


char *
virtual_device_name_for(Partition *p, char *path)
{
	if(p->GetDevice()->CountPartitions() == 1) {
		strcpy(path, p->GetDevice()->Name());
	}
	else {
		char virtname[256];
		sprintf(virtname, p->GetDevice()->Name());
		virtname[strlen(virtname) - strlen("/raw")] = '\0';

		sprintf(path, "%s/%d_%d", virtname, p->GetSession()->Index(), p->Index());
	}
	return path;
}


void
VolumeItem::DrawItem(BView *owner, BRect frame, bool complete)
{
	float em = owner->StringWidth("m");

	rgb_color bgcolor;
	if(IsSelected()) {
		bgcolor = kSelectedBg;
	}
	else {
		bgcolor = owner->ViewColor();
	}
	// so text gets antialiased correctly
	//
	owner->SetLowColor(bgcolor);

	// possibly repaint the background
	//
	if(IsSelected() || complete) {
		owner->SetHighColor(bgcolor);
		owner->FillRect(frame);
	}

	int enabled = IsEnabled() ? 0 : 1;

	// give ourselves some space
	//
	frame.left += em/2;
	frame.bottom -= em/4;

	// draw volume name
	//
	owner->MovePenTo(frame.left, frame.bottom);
	owner->SetHighColor(kVolumeColor[enabled]);
	owner->DrawString(fPartition->VolumeName());
	frame.left += kVolumeWidth * em;

	// draw fs name
	//
	owner->MovePenTo(frame.left, frame.bottom);
	owner->SetHighColor(kFSColor[enabled]);
	owner->DrawString(fPartition->FileSystemShortName());
	frame.left += kFSWidth * em;

	// draw dev path
	//
	char path[256];
	owner->MovePenTo(frame.left, frame.bottom);
	owner->SetHighColor(kDevColor[enabled]);
	owner->DrawString(virtual_device_name_for(fPartition, path));
}
