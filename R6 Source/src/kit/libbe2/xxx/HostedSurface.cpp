
#include <HostedSurface.h>

BHostedSurface::BHostedSurface()
{
}


BHostedSurface::~BHostedSurface()
{
}

status_t 
BHostedSurface::Invalidate(const BRegion &dirty)
{
	return B_UNSUPPORTED;
}

status_t 
BHostedSurface::BeginUpdate(IRender *&)
{
	return B_UNSUPPORTED;
}

status_t 
BHostedSurface::EndUpdate()
{
	return B_UNSUPPORTED;
}

