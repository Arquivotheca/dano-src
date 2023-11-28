#include "IconBits.h"

#include <Autolock.h>

static BResourceSet gResources;
static bool gResInitialized = false;
static BLocker gResLocker;

BResourceSet& Resources()
{
	if( gResInitialized ) return gResources;
	
	BAutolock l(&gResLocker);
	if( gResInitialized ) return gResources;
	
	gResources.AddResources((void*)&Resources);
	gResInitialized = true;
	
	return gResources;
}
