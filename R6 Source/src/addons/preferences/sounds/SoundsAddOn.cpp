//*****************************************************************************
//
//	File:		 SoundsAddOn.cpp
//
//	Description: Main class for sounds preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "Sounds.h"
#include "PrefsAppExport.h"
#include "PPAddOn.h"
#include <image.h>
#include <string.h>

class BView;
class BBitmap;

class SoundsAddOn : public PPAddOn
{
public:
			SoundsAddOn(image_id i, PPWindow*w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i, PPWindow*w)
{
	return (PPAddOn *)new SoundsAddOn(i, w);
}

SoundsAddOn::SoundsAddOn(image_id i, PPWindow*w)
 : PPAddOn(i, w)
{
}

BView *SoundsAddOn::MakeView()
{
	return new SoundsView(this);
}

bool SoundsAddOn::UseAddOn()
{
	return true;
}

BBitmap* SoundsAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* SoundsAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *SoundsAddOn::Name()
{
	return "Sounds";
}

char *SoundsAddOn::InternalName()
{
	return "sounds";
}
