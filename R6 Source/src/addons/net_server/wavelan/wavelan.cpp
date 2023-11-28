/*
 * Copyright (C) 1996 Be, Inc.  All Rights Reserved
 */
#include "generic.h"

static const char TITLE[] = "Lucent 802.11b Wireless";
static const char CONFIG[] = "wavelan";
static const char LINK[] = "/dev/net/wavelan/0";

#pragma export on
extern "C" BNetDevice *
open_device(const char *device)
{
	GenericDevice *dev;

	dev = new GenericDevice();
	if (dev->Start(device) < B_NO_ERROR) {
		delete dev;
		return (NULL);
	}
	return (dev);
}

extern "C" BNetConfig *
open_config(const char *device)
{
	return (new GenericConfig(TITLE, CONFIG, LINK));
}

#pragma export reset
