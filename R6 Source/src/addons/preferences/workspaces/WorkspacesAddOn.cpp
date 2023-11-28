//*****************************************************************************
//
//	File:		 WorkspacesAddOn.cpp
//
//	Description: Main class for workspaces preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "PPAddOn.h"
#include "PrefsAppExport.h"
#include "workspaces.h"
#include <image.h>
#include <string.h>

class BView;
class BBitmap;

class WorkspacesAddOn : public PPAddOn
{
public:
			WorkspacesAddOn(image_id i,PPWindow*w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i,PPWindow*w)
{
	return (PPAddOn *)new WorkspacesAddOn(i,w);
}

WorkspacesAddOn::WorkspacesAddOn(image_id i,PPWindow*w)
 : PPAddOn(i,w)
{
}

BView *WorkspacesAddOn::MakeView()
{
	return new WorkspacesView();
}

bool WorkspacesAddOn::UseAddOn()
{
	return true;
}

BBitmap* WorkspacesAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* WorkspacesAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *WorkspacesAddOn::Name()
{
	return "Workspaces";
}

char *WorkspacesAddOn::InternalName()
{
	return "workspaces";
}
