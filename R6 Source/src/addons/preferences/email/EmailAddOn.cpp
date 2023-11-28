//*****************************************************************************
//
//	File:		 EmailAddOn.cpp
//
//	Description: Main class for e-mail preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "PPAddOn.h"
#include "PrefsAppExport.h"
#include "Email.h"
#include <image.h>
#include <string.h>

class BView;
class BBitmap;

class EmailAddOn : public PPAddOn
{
public:
			EmailAddOn(image_id i, PPWindow *w);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

extern "C" PPAddOn * get_addon_object(image_id i, PPWindow *w)
{
	return (PPAddOn *)new EmailAddOn(i, w);
}

EmailAddOn::EmailAddOn(image_id i, PPWindow *w)
 : PPAddOn(i, w)
{
}

BView *EmailAddOn::MakeView()
{
	return new TEmailView();
}

bool EmailAddOn::UseAddOn()
{
	return true;
}

BBitmap* EmailAddOn::Icon()
{
	return (BBitmap*)InstantiateFromResource(Resources,1);
}

BBitmap* EmailAddOn::SmallIcon()
{
	return (BBitmap*)InstantiateFromResource(Resources,2);
}

char *EmailAddOn::Name()
{
	return "e-mail";
}

char *EmailAddOn::InternalName()
{
	return "email";
}
