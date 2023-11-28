
#include <Rect.h>
#include <Surface.h>

BSurface::BSurface()
{
}


BSurface::~BSurface()
{
}

BRect 
BSurface::Bounds()
{
	return BRect();
}

status_t 
BSurface::GetParent(surface_ptr &)
{
	return B_UNSUPPORTED;
}

status_t 
BSurface::SetParent(const surface_ptr &)
{
	return B_UNSUPPORTED;
}

status_t 
BSurface::MoveTo(BPoint )
{
	return B_UNSUPPORTED;
}

status_t 
BSurface::ResizeTo(BPoint )
{
	return B_UNSUPPORTED;
}

status_t 
BSurface::StartHosting(BHandler *, hostedsurface_ptr &)
{
	return B_UNSUPPORTED;
}
