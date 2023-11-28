#if ! defined GENERALADDON_INCLUDED
#define GENERALADDON_INCLUDED 1

#include "PPAddOn.h"
#include <image.h>

class BView;
class BBitmap;
class GeneralView;

class GeneralAddOn : public PPAddOn
{
	image_id	image;
	PPWindow	*window;
	GeneralView	*v;
public:
				GeneralAddOn(image_id i, PPWindow *w);

	bool		QuitRequested();
	BView		*MakeView();
	bool		UseAddOn();
	BBitmap		*Icon();
	BBitmap		*SmallIcon();
	char		*Name();
	char		*InternalName();
};

#endif
