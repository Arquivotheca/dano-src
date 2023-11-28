//*****************************************************************************
//
//	File:		 GeneralAddOn.cpp
//
//	Description: Main class for networking "General" preference panel
//
//	Copyright 1996 - 1999, Be Incorporated
//
//*****************************************************************************

#include "GeneralAddOn.h"
#include "PrefsAppExport.h"
#include "GeneralView.h"
#include <string.h>

GeneralAddOn::GeneralAddOn(image_id i, PPWindow *w)
 : PPAddOn(i, w), image(i), window(w)
{
}

BView *GeneralAddOn::MakeView()
{
	return v = new GeneralView(this, image, window);
}

bool GeneralAddOn::UseAddOn()
{
	return true;
}

BBitmap* GeneralAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* GeneralAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

bool GeneralAddOn::QuitRequested()
{
	return v->QuitRequested();
}

char *GeneralAddOn::Name()
{
	return "General";
}

char *GeneralAddOn::InternalName()
{
	return "general";
}
