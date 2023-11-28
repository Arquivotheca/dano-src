//
// Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#ifndef _VOLUME_ITEM_H_
#define _VOLUME_ITEM_H_

#include <ListItem.h>

class Partition;

class VolumeItem : public BListItem {
public:
	VolumeItem(Partition *p) : fPartition(p) {};
	~VolumeItem() {};

	void DrawItem(BView *owner, BRect frame, bool complete = false);

public:
	Partition *GetPartition() { return fPartition; };

private:
	Partition *fPartition;
};

#endif // _VOLUME_ITEM_H_
