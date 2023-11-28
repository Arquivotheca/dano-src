//*****************************************************************************
//
//	File:		 FontsAddOn.cpp
//
//	Description: Main class for fonts preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "FontPanel.h"
#include "PrefsAppExport.h"
#include "PPAddOn.h"
#include <image.h>
#include <string.h>

class BView;
class BBitmap;

class FontsAddOn : public PPAddOn
{
public:
			FontsAddOn(image_id i, PPWindow *w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i, PPWindow*w)
{
	return (PPAddOn *)new FontsAddOn(i, w);
}

FontsAddOn::FontsAddOn(image_id i, PPWindow*w)
 : PPAddOn(i, w)
{
}

BView *FontsAddOn::MakeView()
{
	return new FontPanel();
}

bool FontsAddOn::UseAddOn()
{
	return true;
}

BBitmap* FontsAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* FontsAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *FontsAddOn::Name()
{
	return "Fonts";
}

char *FontsAddOn::InternalName()
{
	return "fonts";
}
