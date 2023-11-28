//*****************************************************************************
//
//	File:		 entrypoint.cpp
//
//	Description: preference panel entrypoiny
//
//	Copyright 1996 - 1999, Be Incorporated
//
//*****************************************************************************

#include "GeneralAddOn.h"
#include <image.h>

extern "C" PPAddOn * get_addon_object(image_id i, PPWindow *w)
{
	// the default panel is the General one, it will load the interface panels
	return new GeneralAddOn(i, w);
}
