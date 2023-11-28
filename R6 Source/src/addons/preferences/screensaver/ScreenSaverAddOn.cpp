//*****************************************************************************
//
//	File:		 ScreenSaverAddOn.cpp
//
//	Description: Main class for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "PPAddOn.h"
#include "PrefsAppExport.h"
#include "SetupView.h"
#include <image.h>
#include <string.h>

class BView;
class BBitmap;

class ScreenSaverAddOn : public PPAddOn
{
public:
			ScreenSaverAddOn(image_id i,PPWindow* w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap	*Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i,PPWindow* w)
{
	return (PPAddOn *)new ScreenSaverAddOn(i,w);
}

ScreenSaverAddOn::ScreenSaverAddOn(image_id i,PPWindow* w)
 : PPAddOn(i,w)
{
}

BView *ScreenSaverAddOn::MakeView()
{
	return new SetupView();
}

bool ScreenSaverAddOn::UseAddOn()
{
	return true;
}

BBitmap* ScreenSaverAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* ScreenSaverAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *ScreenSaverAddOn::Name()
{
	return "Screen Saver";
}

char *ScreenSaverAddOn::InternalName()
{
	return "screensaver";
}
