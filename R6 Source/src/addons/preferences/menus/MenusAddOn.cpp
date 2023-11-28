//*****************************************************************************
//
//	File:		 MenusAddOn.cpp
//
//	Description: Main class for menus preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "Menus.h"
#include "PrefsAppExport.h"
#include "PPAddOn.h"
#include <image.h>
#include <string.h>

class BView;
class BBitmap;

class MenusAddOn : public PPAddOn
{
public:
			MenusAddOn(image_id i, PPWindow*w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i, PPWindow*w)
{
	return (PPAddOn *)new MenusAddOn(i, w);
}

MenusAddOn::MenusAddOn(image_id i, PPWindow*w)
 : PPAddOn(i, w)
{
}

BView *MenusAddOn::MakeView()
{
	return new MenusView();
}

bool MenusAddOn::UseAddOn()
{
	return true;
}

BBitmap* MenusAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* MenusAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *MenusAddOn::Name()
{
	return "Menus";
}

char *MenusAddOn::InternalName()
{
	return "menus";
}
