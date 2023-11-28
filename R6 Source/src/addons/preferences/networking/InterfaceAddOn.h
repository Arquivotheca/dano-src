#if ! defined INTERFACEADDON_INCLUDED
#define INTERFACEADDON_INCLUDED 1

#include "PPAddOn.h"
#include <image.h>

class BView;
class BBitmap;
class NetworkingCore;

class InterfaceAddOn : public PPAddOn
{
	NetworkingCore	*core;
	int32			interface;
	bool			def;
public:
			InterfaceAddOn(image_id i, PPWindow *w, NetworkingCore *nc, int32 interface, bool defaultify);

	BView	*MakeView();
	bool	UseAddOn();
	BBitmap *Icon();
	BBitmap *SmallIcon();
	char	*Name();
	char	*InternalName();
};

#endif
