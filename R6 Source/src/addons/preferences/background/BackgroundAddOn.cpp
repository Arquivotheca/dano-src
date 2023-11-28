//*****************************************************************************
//
//	File:		 BackgroundAddOn.cpp
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

class BackgroundsAddOn : public PPAddOn
{
public:
			BackgroundsAddOn(image_id i,PPWindow* w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i,PPWindow* w)
{
	return (PPAddOn *)new BackgroundsAddOn(i,w);
}

BackgroundsAddOn::BackgroundsAddOn(image_id i,PPWindow* w)
 : PPAddOn(i,w)
{
}

BView *BackgroundsAddOn::MakeView()
{
	return new SetupView(this);
}

bool BackgroundsAddOn::UseAddOn()
{
	return true;
}

BBitmap* BackgroundsAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* BackgroundsAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *BackgroundsAddOn::Name()
{
	return "Background";
}

char *BackgroundsAddOn::InternalName()
{
	return "background";
}
