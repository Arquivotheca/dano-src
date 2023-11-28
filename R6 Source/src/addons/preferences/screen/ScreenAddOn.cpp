//*****************************************************************************
//
//	File:		 ScreenAddOn.cpp
//
//	Description: Main class for screen preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "PPAddOn.h"
#include "PrefsAppExport.h"
#include "screen_utils.h"
#include "screen.h"
#include <image.h>
#include <string.h>

class BView;
class BBitmap;

class ScreenAddOn : public PPAddOn
{
public:
			ScreenAddOn(image_id i,PPWindow* w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i,PPWindow* w)
{
	return (PPAddOn *)new ScreenAddOn(i,w);
}

ScreenAddOn::ScreenAddOn(image_id i,PPWindow* w)
 : PPAddOn(i,w)
{
}

BView *ScreenAddOn::MakeView()
{
	return new TBox();
}

bool ScreenAddOn::UseAddOn()
{
	return true;
}

BBitmap* ScreenAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* ScreenAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *ScreenAddOn::Name()
{
	return "Screen";
}

char *ScreenAddOn::InternalName()
{
	return "screen";
}
