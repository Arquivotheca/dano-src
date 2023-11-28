//*****************************************************************************
//
//	File:		 ScrollBarAddOn.cpp
//
//	Description: Main class for scrollbar preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "PPAddOn.h"
#include "PrefsAppExport.h"
#include "ScrollBar.h"
#include <image.h>
#include <string.h>

class BView;
class BBitmap;

class ScrollBarAddOn : public PPAddOn
{
public:
			ScrollBarAddOn(image_id i, PPWindow*w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i, PPWindow*w)
{
	return (PPAddOn *)new ScrollBarAddOn(i, w);
}

ScrollBarAddOn::ScrollBarAddOn(image_id i, PPWindow*w)
 : PPAddOn(i, w)
{
}

BView *ScrollBarAddOn::MakeView()
{
	return new TScrollBarView();
}

bool ScrollBarAddOn::UseAddOn()
{
	return true;
}

BBitmap* ScrollBarAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* ScrollBarAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *ScrollBarAddOn::Name()
{
	return "ScrollBar";
}

char *ScrollBarAddOn::InternalName()
{
	return "scrollbar";
}
